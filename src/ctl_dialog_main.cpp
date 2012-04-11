/******************************************************************************
 *
 *  $Id$
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

#include "com_xml_parser.hpp"
#include "ctl_globals.hpp"
#include "ctl_dialog_job_edit.hpp"
#include "ctl_dialog_job.hpp"
#include "ctl_dialog_main.hpp"
#include "ctl_dialog_msg.hpp"

/*****************************************************************************/

#define WIDTH 700
#define HEIGHT 300

#define WATCHDOG_TIMEOUT 1.0

/*****************************************************************************/

/**
   Konstruktor

   \param dls_dir DLS-Datenverzeichnis
*/

CTLDialogMain::CTLDialogMain(const string &dls_dir)
{
    int x = Fl::w() / 2 - WIDTH / 2;
    int y = Fl::h() / 2 - HEIGHT / 2;

    _dls_dir = dls_dir;

    _wnd = new Fl_Double_Window(x, y, WIDTH, HEIGHT, "DLS Manager");
    _wnd->callback(_callback, this);

    _grid_jobs = new Fl_Grid(10, 10, WIDTH - 20, HEIGHT - 55);
    _grid_jobs->add_column("job", "Auftrag", 200);
    _grid_jobs->add_column("source", "Quelle");
    _grid_jobs->add_column("state", "Status");
    _grid_jobs->add_column("trigger", "Trigger");
    _grid_jobs->add_column("proc", "Prozess");
    _grid_jobs->add_column("logging", "Erfassung");
    _grid_jobs->callback(_callback, this);

    _button_close = new Fl_Button(WIDTH - 90, HEIGHT - 35, 80, 25,
                                  "Schliessen");
    _button_close->callback(_callback, this);

    _button_add = new Fl_Button(10, HEIGHT - 35, 130, 25, "Neuer Auftrag");
    _button_add->callback(_callback, this);

    _button_state = new Fl_Button(345, HEIGHT - 35, 100, 25);
    _button_state->callback(_callback, this);
    _button_state->hide();

    _wnd->end();

    _wnd->resizable(_grid_jobs);
}

/*****************************************************************************/

/**
   Destruktor
*/

CTLDialogMain::~CTLDialogMain()
{
    Fl::remove_timeout(_static_watchdog_timeout, this);

    delete _wnd;
}

/*****************************************************************************/

/**
   Dialog anzeigen
*/

void CTLDialogMain::show()
{
    // Fenster zeigen
    _wnd->show();

    // �berpr�fen, ob das angegebene Verzeichnis
    // schon ein DLS-Datenverzeichnis ist
    _check_dls_dir();

    _load_jobs();
    _load_watchdogs();

    // Timeout f�r die Aktualisierung der Watchdogs hinzuf�gen
    Fl::add_timeout(WATCHDOG_TIMEOUT, _static_watchdog_timeout, this);

    // Solange in der Event-Loop bleiben, wie das Fenster sichtbar ist
    while (_wnd->shown()) Fl::wait();
}

/*****************************************************************************/

/**
   Statische Callback-Funktion

   \param sender Zeiger aud das Widget, das den Callback ausgel�st hat
   \param data User-Data, hier Zeiger auf den Dialog
*/

void CTLDialogMain::_callback(Fl_Widget *sender, void *data)
{
    CTLDialogMain *dialog = (CTLDialogMain *) data;

    if (sender == dialog->_grid_jobs) dialog->_grid_jobs_callback();
    if (sender == dialog->_button_close) dialog->_button_close_clicked();
    if (sender == dialog->_wnd) dialog->_button_close_clicked();
    if (sender == dialog->_button_add) dialog->_button_add_clicked();
    if (sender == dialog->_button_state) dialog->_button_state_clicked();
}

/*****************************************************************************/

/**
   Callback f�r das Erfassungsauftrags-Grid
*/

void CTLDialogMain::_grid_jobs_callback()
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
                    _jobs[i].running() ? "gestartet" : "angehalten");
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

                            _grid_jobs->current_content("l�uft");
                        }
                        else
                        {
                            if (!_grid_jobs->current_selected())
                            {
                                _grid_jobs->current_content_color(FL_RED);
                            }

                            _grid_jobs->current_content("l�uft nicht!");
                        }
                    }
                    else
                    {
                        if (!_grid_jobs->current_selected())
                        {
                            _grid_jobs->current_content_color(FL_DARK_YELLOW);
                        }

                        _grid_jobs->current_content("(unbekannt)");
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

                            _grid_jobs->current_content("l�uft");
                        }
                        else
                        {
                            if (!_grid_jobs->current_selected())
                            {
                                _grid_jobs->current_content_color(FL_RED);
                            }

                            _grid_jobs->current_content("l�uft nicht!");
                        }
                    }
                    else
                    {
                        if (!_grid_jobs->current_selected())
                        {
                            _grid_jobs->current_content_color(FL_DARK_YELLOW);
                        }

                        _grid_jobs->current_content("(unbekannt)");
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
   Callback: Der "Beenden"-Button wurde geklickt
*/

void CTLDialogMain::_button_close_clicked()
{
    _wnd->hide();
}

/*****************************************************************************/

/**
   Callback: Der "Hinzuf�gen"-Button wurde geklickt
*/

void CTLDialogMain::_button_add_clicked()
{
    CTLDialogJobEdit *dialog = new CTLDialogJobEdit(_dls_dir);
    dialog->show(0); // 0 = Neuer Auftrag

    if (dialog->updated())
    {
        _load_jobs();
    }

    delete dialog;
}

/*****************************************************************************/

/**
   Callback: Der "Starten/Anhalten"-Button wurde geklickt
*/

void CTLDialogMain::_button_state_clicked()
{
    int index = _grid_jobs->selected_index();
    CTLJobPreset job_copy;

    if (index < 0 || index >= (int) _jobs.size())
    {
        msg_win->str() << "Ung�ltiger Auftrags-Index!";
        msg_win->error();
        return;
    }

    job_copy = _jobs[index];

    job_copy.toggle_running();

    try
    {
        job_copy.write(_dls_dir);
    }
    catch (ECOMJobPreset &e)
    {
        msg_win->str() << "Schreiben der Vorgabendatei: " << e.msg;
        msg_win->error();
        return;
    }

    try
    {
        job_copy.spool(_dls_dir);
    }
    catch (ECOMJobPreset &e)
    {
        msg_win->str() << "Konnte den dlsd nicht benachrichtigen!";
        msg_win->warning();
    }

    _jobs[index] = job_copy;
    _grid_jobs->redraw();
    _update_button_state();
}

/*****************************************************************************/

/**
   Alle Watchdog-Informationen laden
*/

void CTLDialogMain::_load_watchdogs()
{
    vector<CTLJobPreset>::iterator job_i;
    struct stat file_stat;
    stringstream dir_name;

    job_i = _jobs.begin();
    while (job_i != _jobs.end())
    {
        // Dateinamen konstruieren
        dir_name.str("");
        dir_name.clear();
        dir_name << _dls_dir << "/job" << job_i->id();

        if (stat((dir_name.str() + "/watchdog").c_str(), &file_stat) == 0)
        {
            // Wenn die neue Dateizeit j�nger ist, als die zuletzt gelesene...
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
            else // Watchdog nicht ver�ndert
            {
                job_i->process_bad_count++;
            }
        }
        else // Konnte Watchdog nicht lesen
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
            // Wenn die neue Dateizeit j�nger ist, als die zuletzt gelesene...
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
            else // Watchdog nicht ver�ndert
            {
                job_i->logging_bad_count++;
            }
        }
        else // Konnte Watchdog nicht lesen
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
   Alle Erfassungsauftr�ge laden

   L�uft durch das Verzeichnis, merkt sich alle Eintr�ge, die
   "jobXXX" heissen, sortiert diese Liste und �berpr�ft dann
   jeweils, ob es sich um ein g�ltiges Auftragsverzeichnis
   handelt. Wenn ja, wird der entsprechende Auftrag importiert
   und in die Liste eingef�gt.
*/

void CTLDialogMain::_load_jobs()
{
    unsigned int job_id;
    DIR *dir;
    struct dirent *dir_ent;
    string dirname;
    stringstream str;
    fstream file;
    CTLJobPreset job;
    struct stat file_stat;
    stringstream watch_file_name;
    list<unsigned int> job_ids;
    list<unsigned int>::iterator job_id_i;

    str.exceptions(ios::failbit | ios::badbit);

    // Liste der Auftr�ge leeren
    _grid_jobs->record_count(0);
    _jobs.clear();

    // Das Hauptverzeichnis �ffnen
    if ((dir = opendir(_dls_dir.c_str())) == NULL)
    {
        msg_win->str() << "Konnte das Datenverzeichnis \"" << _dls_dir
                       << "\" nicht �ffnen!";
        msg_win->error();
        return;
    }

    // Alle Dateien und Unterverzeichnisse durchlaufen
    while ((dir_ent = readdir(dir)) != NULL)
    {
        // Verzeichnisnamen kopieren
        dirname = dir_ent->d_name;

        // Wenn das Verzeichnis nicht mit "job" beginnt,
        // das n�chste verarbeiten
        if (dirname.substr(0, 3) != "job") continue;

        str.str("");
        str.clear();
        str << dirname.substr(3);

        try
        {
            // ID aus dem Verzeichnisnamen lesen
            str >> job_id;
        }
        catch (...)
        {
            // Der Rest des Verzeichnisnamens ist keine Nummer!
            continue;
        }

        job_ids.push_back(job_id);
    }

    // Auftrags-IDs aufsteigend sortieren
    job_ids.sort();

    // Alle "gemerkten", potentiellen Auftrags-IDs durchlaufen
    for (job_id_i = job_ids.begin(); job_id_i != job_ids.end(); job_id_i++)
    {
        // Gibt es in dem Verzeichnis eine Datei job.xml?
        str.str("");
        str.clear();
        str << _dls_dir << "/job" << *job_id_i << "/job.xml";
        file.open(str.str().c_str(), ios::in);
        if (!file.is_open()) continue;

        try
        {
            job.import(_dls_dir, *job_id_i);
        }
        catch (ECOMJobPreset &e)
        {
            msg_win->str() << "Importieren des Auftrags " << *job_id_i
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

        // Auftrag in die Liste einf�gen
        _jobs.push_back(job);

        // Dateinamen konstruieren
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
   Erfassungsauftrag editieren

   �ffnet einen neuen Dialog, um den Auftrag zu bearbeiten

   \param index Index des zu bearbeitenden Auftrags in der Liste
*/

void CTLDialogMain::_edit_job(unsigned int index)
{
    CTLDialogJob *dialog = new CTLDialogJob(_dls_dir);

    dialog->show(&_jobs[index]);

    if (dialog->updated())
    {
        _load_jobs();
    }

    delete dialog;
}

/*****************************************************************************/

/**
   Aktualisieren des "Starten/Anhalten"-Buttons
*/

void CTLDialogMain::_update_button_state()
{
    int i;

    if (_grid_jobs->select_count())
    {
        i = _grid_jobs->selected_index();

        if (_jobs[i].running())
        {
            _button_state->label("Anhalten");
        }
        else
        {
            _button_state->label("Starten");
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
   �berpr�ft, ob das Verzeichnis ein DLS-Datenverzeichnis ist
*/

void CTLDialogMain::_check_dls_dir()
{
    struct stat stat_buf;
    stringstream str;
    bool build_dls_dir = false;
    int fd;
    pid_t pid;
    int status;
    fstream pid_file;
    bool start_dlsd;

    // Pr�fen, ob das Verzeichnis �berhaupt existiert
    if (stat(_dls_dir.c_str(), &stat_buf) == -1)
    {
        str << "Das Verzeichnis \"" << _dls_dir << "\"" << endl;
        str << "existiert noch nicht. Soll es als" << endl;
        str << "DLS-Datenverzeichnis angelegt werden?";

        if (fl_choice(str.str().c_str(), "Nein", "Ja", NULL) == 0) return;

        build_dls_dir = true;

        if (mkdir(_dls_dir.c_str(), 0755) == -1)
        {
            msg_win->str() << "Konnte das Verzeichnis \"" << _dls_dir << "\"";
            msg_win->str() << " nicht anlegen: " << strerror(errno);
            msg_win->error();
            return;
        }
    }
    else
    {
        // Pr�fen, ob das angegebene DLS-Datenverzeichnis �berhaupt
        // ein Verzeichnis ist
        if (!S_ISDIR(stat_buf.st_mode))
        {
            msg_win->str() << "\"" << _dls_dir << "\" ist kein Verzeichnis!";
            msg_win->error();
            return;
        }
    }

    // Existiert das Spooling-Verzeichnis?
    if (stat((_dls_dir + "/spool").c_str(), &stat_buf) == -1)
    {
        if (!build_dls_dir)
        {
            str.clear();
            str.str("");
            str << "Das Verzeichnis \"" << _dls_dir << "\"" << endl;
            str << "ist noch kein DLS-Datenverzeichnis." << endl;
            str << "Soll es als solches initialisiert werden?";

            if (fl_choice(str.str().c_str(), "Nein", "Ja", NULL) == 0) return;

            build_dls_dir = true;
        }

        // Spooling-Verzeichnis anlegen
        if (mkdir((_dls_dir + "/spool").c_str(), 0755) == -1)
        {
            msg_win->str() << "Konnte das Verzeichnis \""
                           << (_dls_dir + "/spool") << "\"";
            msg_win->str() << " nicht anlegen: " << strerror(errno);
            msg_win->error();
            return;
        }
    }

    // Existiert die Datei mit der ID-Sequenz?
    if (stat((_dls_dir + "/id_sequence").c_str(), &stat_buf) == -1)
    {
        if (!build_dls_dir)
        {
            str.clear();
            str.str("");
            str << "Das Verzeichnis \"" << _dls_dir << "\"" << endl;
            str << "ist noch kein DLS-Datenverzeichnis." << endl;
            str << "Soll es als solches initialisiert werden?";

            if (fl_choice(str.str().c_str(), "Nein", "Ja", NULL) == 0) return;

            build_dls_dir = true;
        }

        // Datei anlegen
        if ((fd = open((_dls_dir + "/id_sequence").c_str(),
                       O_WRONLY | O_CREAT, 0644)) == -1)
        {
            msg_win->str() << "Konnte die Datei \""
                           << (_dls_dir + "/id_sequence") << "\"";
            msg_win->str() << " nicht anlegen: " << strerror(errno);
            msg_win->error();
            return;
        }

        if (write(fd, "100\n", 4) != 4)
        {
            close(fd);
            msg_win->str() << "Konnte die Datei \""
                           << (_dls_dir + "/id_sequence") << "\"";
            msg_win->str() << " nicht beschreiben! Bitte manuell l�schen!";
            msg_win->error();
            return;
        }

        close(fd);
    }

    start_dlsd = false;

    // Existiert die PID-Datei des dlsd?
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
            msg_win->str() << "Konnte die Datei \""
                           << (_dls_dir + "/" + DLS_PID_FILE)
                           << "\" nicht �ffnen!";
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

            msg_win->str() << "Datei \"" << (_dls_dir + "/" + DLS_PID_FILE)
                           << "\" ist korrupt!";
            msg_win->error();
            return;
        }

        pid_file.close();

        if (kill(pid, 0) == -1)
        {
            if (errno == ESRCH) // Prozess existiert nicht
            {
                start_dlsd = true;
            }
            else
            {
                msg_win->str() << "Konnte Prozess " << pid
                               << " nicht signalisieren!";
                msg_win->error();
                return;
            }
        }
    }

    if (start_dlsd)
    {
        str.clear();
        str.str("");
        str << "F�r das Verzeichnis \"" << _dls_dir << "\"" << endl;
        str << "l�uft noch kein DLS-Daemon. Jetzt starten?";

        if (fl_choice(str.str().c_str(), "Nein", "Ja", NULL) == 1)
        {
            if ((pid = fork()) == -1)
            {
                msg_win->str() << "fork() fehlgeschlagen!";
                msg_win->error();
                return;
            }

            if (pid == 0) // Kindprozess
            {
                const char *params[4] = {"dlsd", "-d", _dls_dir.c_str(), 0};

                if (execvp("dlsd", (char * const *) params) == -1)
                {
                    cerr << "ERROR: Could not exec dlsd: "
                         << strerror(errno) << endl;
                    exit(-1);
                }
            }
            else // Mutterprozess
            {
                waitpid(pid, &status, 0);

                if ((signed char) WEXITSTATUS(status) == -1)
                {
                    msg_win->str() << "Konnte dlsd nicht ausf�hren!"
                                   << " Siehe Konsole f�r Fehlermeldungen.";
                    msg_win->error();
                    return;
                }
            }
        }
    }
}

/*****************************************************************************/

/**
   Statische Callback-Funktion f�r den Watchdog-Timeout

   \param data Zeiger auf den Dialog
*/

void CTLDialogMain::_static_watchdog_timeout(void *data)
{
    CTLDialogMain *dialog = (CTLDialogMain *) data;

    dialog->_load_watchdogs();

    Fl::add_timeout(WATCHDOG_TIMEOUT, _static_watchdog_timeout, dialog);
}

/*****************************************************************************/
