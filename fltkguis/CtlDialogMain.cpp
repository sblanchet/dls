/******************************************************************************
 *
 *  This file is part of the Data Logging Service (DLS).
 *
 *  DLS is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  DLS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with DLS. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <sstream>
#include <fstream>
using namespace std;

#include <FL/Fl.H>
#include <FL/fl_ask.H>

/*****************************************************************************/

#include "lib/XmlParser.h"

#include "CtlGlobals.h"
#include "CtlDialogJobEdit.h"
#include "CtlDialogJob.h"
#include "CtlDialogMain.h"
#include "CtlDialogMsg.h"

/*****************************************************************************/

#define WIDTH 700
#define HEIGHT 300

#define WATCHDOG_TIMEOUT 1.0

/*****************************************************************************/

/**
   Constructor

   \param dls_dir DLS data directory
*/

CtlDialogMain::CtlDialogMain(const string &dls_dir)
{
    int x = Fl::w() / 2 - WIDTH / 2;
    int y = Fl::h() / 2 - HEIGHT / 2;

    _dls_dir = dls_dir;

    _wnd = new Fl_Double_Window(x, y, WIDTH, HEIGHT, "DLS Manager");
    _wnd->callback(_callback, this);

    _grid_jobs = new Fl_Grid(10, 10, WIDTH - 20, HEIGHT - 55);
    _grid_jobs->add_column("job", "Job", 200);
    _grid_jobs->add_column("source", "Source");
    _grid_jobs->add_column("state", "Status");
    _grid_jobs->add_column("trigger", "Trigger");
    _grid_jobs->add_column("proc", "Process");
    _grid_jobs->add_column("logging", "Logging");
    _grid_jobs->callback(_callback, this);

    _button_close = new Fl_Button(WIDTH - 90, HEIGHT - 35, 80, 25,
                                  "Close");
    _button_close->callback(_callback, this);

    _button_add = new Fl_Button(10, HEIGHT - 35, 130, 25, "New job");
    _button_add->callback(_callback, this);

    _button_state = new Fl_Button(345, HEIGHT - 35, 100, 25);
    _button_state->callback(_callback, this);
    _button_state->hide();

    _wnd->end();

    _wnd->resizable(_grid_jobs);
}

/*****************************************************************************/

/**
   Destructor
*/

CtlDialogMain::~CtlDialogMain()
{
    Fl::remove_timeout(_static_watchdog_timeout, this);

    delete _wnd;
}

/*****************************************************************************/

/**
   Show dialog
*/

void CtlDialogMain::show()
{
    // Show window
    _wnd->show();

    // Check if the specified directory is
    // already a DLS data directory
    _check_dls_dir();

    _load_jobs();
    _load_watchdogs();

    // Add timeout for watchdog update
    Fl::add_timeout(WATCHDOG_TIMEOUT, _static_watchdog_timeout, this);

    // Stay in the event loop while the window is visible
    while (_wnd->shown()) Fl::wait();
}

/*****************************************************************************/

/**
   Static callback function

   \param sender Pointer to the widget that triggered the callback
   \param data User data, here pointer to the dialog
*/

void CtlDialogMain::_callback(Fl_Widget *sender, void *data)
{
    CtlDialogMain *dialog = (CtlDialogMain *) data;

    if (sender == dialog->_grid_jobs) dialog->_grid_jobs_callback();
    if (sender == dialog->_button_close) dialog->_button_close_clicked();
    if (sender == dialog->_wnd) dialog->_button_close_clicked();
    if (sender == dialog->_button_add) dialog->_button_add_clicked();
    if (sender == dialog->_button_state) dialog->_button_state_clicked();
}

/*****************************************************************************/

/**
   Callback for the collection request grid
*/

void CtlDialogMain::_grid_jobs_callback()
{
    unsigned int i;
    stringstream str;

    switch (_grid_jobs->current_event())
    {
        case flgContent:
            i = _grid_jobs->current_record();

            if (_grid_jobs->current_col() == "job")
            {
                str << _jobs[i].id_desc();
                _grid_jobs->current_content(str.str());
            }
            else if (_grid_jobs->current_col() == "source")
            {
                _grid_jobs->current_content(_jobs[i].source());
            }
            else if (_grid_jobs->current_col() == "state")
            {
                _grid_jobs->current_content(
                    _jobs[i].running() ? "started" : "required");
            }
            else if (_grid_jobs->current_col() == "trigger")
            {
                _grid_jobs->current_content(_jobs[i].trigger());
            }
            else if (_grid_jobs->current_col() == "proc")
            {
                if (_jobs[i].running())
                {
                    if (_jobs[i].process_watch_determined)
                    {
                        if (_jobs[i].process_bad_count < WATCH_ALERT)
                        {
                            if (!_grid_jobs->current_selected())
                            {
                                _grid_jobs->current_content_color(
                                    FL_DARK_GREEN);
                            }

                            _grid_jobs->current_content("running");
                        }
                        else
                        {
                            if (!_grid_jobs->current_selected())
                            {
                                _grid_jobs->current_content_color(FL_RED);
                            }

                            _grid_jobs->current_content("not running!");
                        }
                    }
                    else
                    {
                        if (!_grid_jobs->current_selected())
                        {
                            _grid_jobs->current_content_color(FL_DARK_YELLOW);
                        }

                        _grid_jobs->current_content("(unknown)");
                    }
                }
            }
            else if (_grid_jobs->current_col() == "logging")
            {
                if (_jobs[i].running())
                {
                    if (_jobs[i].logging_watch_determined)
                    {
                        if (_jobs[i].logging_bad_count < WATCH_ALERT)
                        {
                            if (!_grid_jobs->current_selected())
                            {
                                _grid_jobs->current_content_color(
                                    FL_DARK_GREEN);
                            }

                            _grid_jobs->current_content("running");
                        }
                        else
                        {
                            if (!_grid_jobs->current_selected())
                            {
                                _grid_jobs->current_content_color(FL_RED);
                            }

                            _grid_jobs->current_content("not running!");
                        }
                    }
                    else
                    {
                        if (!_grid_jobs->current_selected())
                        {
                            _grid_jobs->current_content_color(FL_DARK_YELLOW);
                        }

                        _grid_jobs->current_content("(unknown)");
                    }
                }
            }
            break;

        case flgSelect:
            _update_button_state();
            break;

        case flgDeSelect:
            _update_button_state();
            break;

        case flgDoubleClick:
            _edit_job(_grid_jobs->current_record());
            break;

        default:
            break;
    }
}

/*****************************************************************************/

/**
   Callback: The "Close" button was clicked
*/

void CtlDialogMain::_button_close_clicked()
{
    _wnd->hide();
}

/*****************************************************************************/

/**
   Callback: The "Add" button was clicked
*/

void CtlDialogMain::_button_add_clicked()
{
    CtlDialogJobEdit *dialog = new CtlDialogJobEdit(_dls_dir);
    dialog->show(0); // 0 = New job

    if (dialog->updated())
    {
        _load_jobs();
    }

    delete dialog;
}

/*****************************************************************************/

/**
   Callback: The "Start/Stop" button was clicked
*/

void CtlDialogMain::_button_state_clicked()
{
    int index = _grid_jobs->selected_index();
    CtlJobPreset job_copy;

    if (index < 0 || index >= (int) _jobs.size())
    {
        msg_win->str() << "Invalid job index!";
        msg_win->error();
        return;
    }

    job_copy = _jobs[index];

    job_copy.toggle_running();

    try
    {
        job_copy.write(_dls_dir);
    }
    catch (LibDLS::EJobPreset &e)
    {
        msg_win->str() << "Write the specification data: " << e.msg;
        msg_win->error();
        return;
    }

    try
    {
        job_copy.spool(_dls_dir);
    }
    catch (LibDLS::EJobPreset &e)
    {
        msg_win->str() << "Cannot notify dlsd!";
        msg_win->warning();
    }

    _jobs[index] = job_copy;
    _grid_jobs->redraw();
    _update_button_state();
}

/*****************************************************************************/

/**
   Load all watchdog information
*/

void CtlDialogMain::_load_watchdogs()
{
    vector<CtlJobPreset>::iterator job_i;
    struct stat file_stat;
    stringstream dir_name;

    job_i = _jobs.begin();
    while (job_i != _jobs.end())
    {
        // Construct filenames
        dir_name.str("");
        dir_name.clear();
        dir_name << _dls_dir << "/job" << job_i->id();

        if (stat((dir_name.str() + "/watchdog").c_str(), &file_stat) == 0)
        {
            // If the new file time is younger than the last read...
            if (file_stat.st_mtime > job_i->process_watchdog)
            {
                job_i->process_watchdog = file_stat.st_mtime;

                if (!job_i->process_watch_determined)
                {
                    job_i->process_watch_determined = true;
                    _grid_jobs->redraw();
                }

                if (job_i->process_bad_count > 0)
                {
                    job_i->process_bad_count = 0;
                    _grid_jobs->redraw();
                }
            }
            else // Watchdog not changed
            {
                job_i->process_bad_count++;
            }
        }
        else // Cannot read watchdog
        {
            job_i->process_bad_count++;
        }

        if (job_i->process_bad_count == WATCH_ALERT)
        {
            job_i->process_watch_determined = true;
            _grid_jobs->redraw();
        }

        if (stat((dir_name.str() + "/logging").c_str(), &file_stat) == 0)
        {
            // if the new file time is younger that the last read...
            if (file_stat.st_mtime > job_i->logging_watchdog)
            {
                job_i->logging_watchdog = file_stat.st_mtime;

                if (!job_i->logging_watch_determined)
                {
                    job_i->logging_watch_determined = true;
                    _grid_jobs->redraw();
                }

                if (job_i->logging_bad_count > 0)
                {
                    job_i->logging_bad_count = 0;
                    _grid_jobs->redraw();
                }
            }
            else // Watchdog not changed
            {
                job_i->logging_bad_count++;
            }
        }
        else // Cannot read watchdog
        {
            job_i->logging_bad_count++;
        }

        if (job_i->logging_bad_count == WATCH_ALERT)
        {
            job_i->logging_watch_determined = true;
            _grid_jobs->redraw();
        }


        job_i++;
    }
}

/*****************************************************************************/

/**
   Load all jobs entries

   Run through the directory, remember all entries that are called
   "jobXXX", sort this list and then check in each case whether it is
   a valid job directory. If so, the correspoding job is imported and inserted
   into the list.
*/

void CtlDialogMain::_load_jobs()
{
    unsigned int job_id;
    DIR *dir;
    struct dirent *dir_ent;
    string dirname;
    stringstream str;
    fstream file;
    CtlJobPreset job;
    struct stat file_stat;
    stringstream watch_file_name;
    list<unsigned int> job_ids;
    list<unsigned int>::iterator job_id_i;

    str.exceptions(ios::failbit | ios::badbit);

    // Empty the list of jobs
    _grid_jobs->record_count(0);
    _jobs.clear();

    // Open the main directory
    if ((dir = opendir(_dls_dir.c_str())) == NULL)
    {
        msg_win->str() << "Cannot open the data \"" << _dls_dir
                       << "\" directory!";
        msg_win->error();
        return;
    }

    // Go through all files and subdirectories
    while ((dir_ent = readdir(dir)) != NULL)
    {
        // Copy directory name
        dirname = dir_ent->d_name;

        // If the directory does not start with "job",
        // process the next one
        if (dirname.substr(0, 3) != "job") continue;

        str.str("");
        str.clear();
        str << dirname.substr(3);

        try
        {
            // Read ID from the directory name
            str >> job_id;
        }
        catch (...)
        {
            // The rest of the directory name is not a number!
            continue;
        }

        job_ids.push_back(job_id);
    }

    // Sort jobs ID in ascending order
    job_ids.sort();

    // Go through all "remembered" , potential job IDs
    for (job_id_i = job_ids.begin(); job_id_i != job_ids.end(); job_id_i++)
    {
        // Is there a file job.xml in the directory?
        str.str("");
        str.clear();
        str << _dls_dir << "/job" << *job_id_i << "/job.xml";
        file.open(str.str().c_str(), ios::in);
        if (!file.is_open()) continue;

        try
        {
            job.import(_dls_dir, *job_id_i);
        }
        catch (LibDLS::EJobPreset &e)
        {
            msg_win->str() << "Import the job " << *job_id_i
                           << ": " << e.msg;
            msg_win->error();
            continue;
        }

        job.process_bad_count = 0;
        job.process_watch_determined = false;
        job.process_watchdog = 0;
        job.logging_bad_count = 0;
        job.logging_watch_determined = false;
        job.logging_watchdog = 0;

        // Add job to the list
        _jobs.push_back(job);

        // Construct filenames
        watch_file_name.str("");
        watch_file_name.clear();
        watch_file_name << _dls_dir << "/job" << *job_id_i << "/watchdog";

        if (stat(watch_file_name.str().c_str(), &file_stat) == 0)
        {
            _jobs.back().process_watchdog = file_stat.st_mtime;
        }

        watch_file_name.str("");
        watch_file_name.clear();
        watch_file_name << _dls_dir << "/job" << *job_id_i << "/logging";

        if (stat(watch_file_name.str().c_str(), &file_stat) == 0)
        {
            _jobs.back().logging_watchdog = file_stat.st_mtime;
        }
    }

    _grid_jobs->record_count(_jobs.size());
}

/*****************************************************************************/

/**
   Edit acquisition job

   Open a new dialog to open the job

   \param index Index of the job to be processed in the list
*/

void CtlDialogMain::_edit_job(unsigned int index)
{
    CtlDialogJob *dialog = new CtlDialogJob(_dls_dir);

    dialog->show(&_jobs[index]);

    if (dialog->updated())
    {
        _load_jobs();
    }

    delete dialog;
}

/*****************************************************************************/

/**
   Update the "Start/Stop" buttons
*/

void CtlDialogMain::_update_button_state()
{
    int i;

    if (_grid_jobs->select_count())
    {
        i = _grid_jobs->selected_index();

        if (_jobs[i].running())
        {
            _button_state->label("Stop");
        }
        else
        {
            _button_state->label("Start");
        }

        _button_state->show();
    }
    else
    {
        _button_state->hide();
    }
}

/*****************************************************************************/

/**
   Check if the directory is a DLS data directory
*/

void CtlDialogMain::_check_dls_dir()
{
    struct stat stat_buf;
    stringstream str;
    bool build_dls_dir = false;
    int fd;
    pid_t pid;
    int status;
    fstream pid_file;
    bool start_dlsd;

    // Check if the directory exists
    if (stat(_dls_dir.c_str(), &stat_buf) == -1)
    {
        str << "The directory \"" << _dls_dir << "\"" << endl;
        str << "does not exist. Shall it be" << endl;
        str << "created as a DLS data directory?";

        if (fl_choice(str.str().c_str(), "No", "Yes", NULL) == 0) return;

        build_dls_dir = true;

        if (mkdir(_dls_dir.c_str(), 0755) == -1)
        {
            msg_win->str() << "Cannot create the directory \"" << _dls_dir << "\"";
            msg_win->str() << " : " << strerror(errno);
            msg_win->error();
            return;
        }
    }
    else
    {
        // Check if the specified DLS data directory is the
        // main directory
        if (!S_ISDIR(stat_buf.st_mode))
        {
            msg_win->str() << "\"" << _dls_dir << "\" is not a directory!";
            msg_win->error();
            return;
        }
    }

    // Does the spooling directory exist?
    if (stat((_dls_dir + "/spool").c_str(), &stat_buf) == -1)
    {
        if (!build_dls_dir)
        {
            str.clear();
            str.str("");
            str << "The directory \"" << _dls_dir << "\"" << endl;
            str << "is not a DLS data directory." << endl;
            str << "Shall it be initialized?";

            if (fl_choice(str.str().c_str(), "No", "Yes", NULL) == 0) return;

            build_dls_dir = true;
        }

        // Create spooling directory
        if (mkdir((_dls_dir + "/spool").c_str(), 0755) == -1)
        {
            msg_win->str() << "Cannot create the directory \""
                           << (_dls_dir + "/spool") << "\"";
            msg_win->str() << " : " << strerror(errno);
            msg_win->error();
            return;
        }
    }

    // Does the file with the ID sequence exist?
    if (stat((_dls_dir + "/id_sequence").c_str(), &stat_buf) == -1)
    {
        if (!build_dls_dir)
        {
            str.clear();
            str.str("");
            str << "The directory \"" << _dls_dir << "\"" << endl;
            str << "is not yet a DLS data directory." << endl;
            str << "Shall it be initialized?";

            if (fl_choice(str.str().c_str(), "No", "Yes", NULL) == 0) return;

            build_dls_dir = true;
        }

        // Create file
        if ((fd = open((_dls_dir + "/id_sequence").c_str(),
                       O_WRONLY | O_CREAT, 0644)) == -1)
        {
            msg_win->str() << "Cannot create the file \""
                           << (_dls_dir + "/id_sequence") << "\"";
            msg_win->str() << " : " << strerror(errno);
            msg_win->error();
            return;
        }

        if (write(fd, "100\n", 4) != 4)
        {
            close(fd);
            msg_win->str() << "Cannot write the file \""
                           << (_dls_dir + "/id_sequence") << "\"";
            msg_win->str() << "! Please delete manually!";
            msg_win->error();
            return;
        }

        close(fd);
    }

    start_dlsd = false;

    // Does the PID file of dlsd exist?
    if (stat((_dls_dir + "/" + DLS_PID_FILE).c_str(), &stat_buf) == -1)
    {
        start_dlsd = true;
    }
    else
    {
        pid_file.exceptions(ios::badbit | ios::failbit);

        pid_file.open((_dls_dir + "/" + DLS_PID_FILE).c_str(), ios::in);

        if (!pid_file.is_open())
        {
            msg_win->str() << "Cannot open the file \""
                           << (_dls_dir + "/" + DLS_PID_FILE)
                           << "\" !";
            msg_win->error();
            return;
        }

        try
        {
            pid_file >> pid;
        }
        catch (...)
        {
            pid_file.close();

            msg_win->str() << "File \"" << (_dls_dir + "/" + DLS_PID_FILE)
                           << "\" is corrupted!";
            msg_win->error();
            return;
        }

        pid_file.close();

        if (kill(pid, 0) == -1)
        {
            if (errno == ESRCH) // Process does not exist
            {
                start_dlsd = true;
            }
            else
            {
                msg_win->str() << "Cannot signal process " << pid << "!";
                msg_win->error();
                return;
            }
        }
    }

    if (start_dlsd)
    {
        str.clear();
        str.str("");
        str << "The directory \"" << _dls_dir << "\"" << endl;
        str << "has no running DLS daemon yet. Start now?";

        if (fl_choice(str.str().c_str(), "No", "Yes", NULL) == 1)
        {
            if ((pid = fork()) == -1)
            {
                msg_win->str() << "fork() failed!";
                msg_win->error();
                return;
            }

            if (pid == 0) // child process
            {
                const char *params[4] = {"dlsd", "-d", _dls_dir.c_str(), 0};

                if (execvp("dlsd", (char * const *) params) == -1)
                {
                    cerr << "ERROR: Can not exec dlsd: "
                         << strerror(errno) << endl;
                    exit(-1);
                }
            }
            else // Parent process
            {
                waitpid(pid, &status, 0);

                if ((signed char) WEXITSTATUS(status) == -1)
                {
                    msg_win->str() << "Cannot execute dlsd!"
                                   << " See console for error messages.";
                    msg_win->error();
                    return;
                }
            }
        }
    }
}

/*****************************************************************************/

/**
   Static callback function for the watchdog timeout

   \param data Pointer to the dialog
*/

void CtlDialogMain::_static_watchdog_timeout(void *data)
{
    CtlDialogMain *dialog = (CtlDialogMain *) data;

    dialog->_load_watchdogs();

    Fl::add_timeout(WATCHDOG_TIMEOUT, _static_watchdog_timeout, dialog);
}

/*****************************************************************************/
