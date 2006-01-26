//---------------------------------------------------------------
//
//  C T L _ D I A L O G _ M A I N . C P P
//
//---------------------------------------------------------------

#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <iostream>
#include <sstream>
#include <fstream>
using namespace std;

#include <FL/Fl.h>
#include <FL/fl_ask.h>

//---------------------------------------------------------------

#include "com_xml_parser.hpp"
#include "ctl_globals.hpp"
#include "ctl_dialog_job_edit.hpp"
#include "ctl_dialog_job.hpp"
#include "ctl_dialog_main.hpp"
#include "ctl_msg_wnd.hpp"

//---------------------------------------------------------------

RCS_ID("$Header: /home/fp/dls/src/RCS/ctl_dialog_main.cpp,v 1.11 2005/01/24 13:00:42 fp Exp $");

//---------------------------------------------------------------

#define WIDTH 700
#define HEIGHT 400

//---------------------------------------------------------------

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

  _tile = new Fl_Tile(10, 10, WIDTH - 20, 350);

  _grid_jobs = new Fl_Grid(10, 10, WIDTH - 20, 200);
  _grid_jobs->add_column("job", "Auftrag", 200);
  _grid_jobs->add_column("source", "Quelle");
  _grid_jobs->add_column("state", "Status");
  _grid_jobs->add_column("trigger", "Trigger");
  _grid_jobs->add_column("proc", "Prozess");
  _grid_jobs->add_column("logging", "Erfassung");
  _grid_jobs->callback(_callback, this);

  _grid_messages = new Fl_Grid(10, 210, WIDTH - 20, 150);
  _grid_messages->add_column("time", "Zeit");
  _grid_messages->add_column("text", "Nachricht", 230);
  _grid_messages->select_mode(flgNoSelect);
  _grid_messages->callback(_callback, this);

  _tile->end();

  _button_close = new Fl_Button(WIDTH - 90, HEIGHT - 35, 80, 25, "Schliessen");
  _button_close->callback(_callback, this);

  _button_add = new Fl_Button(10, HEIGHT - 35, 130, 25, "Neuer Auftrag");
  _button_add->callback(_callback, this);

  _button_rem = new Fl_Button(150, HEIGHT - 35, 130, 25, "Auftrag l�schen");
  _button_rem->callback(_callback, this);
  _button_rem->deactivate();

  _button_state = new Fl_Button(345, HEIGHT - 35, 100, 25);
  _button_state->callback(_callback, this);
  _button_state->hide();

  _wnd->resizable(_grid_jobs);
}

//---------------------------------------------------------------

/**
   Destruktor
*/

CTLDialogMain::~CTLDialogMain()
{
  Fl::remove_timeout(_static_timeout, this);

  delete _wnd;
}

//---------------------------------------------------------------

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
  _load_messages();
  _load_watchdogs();

  // Timeout f�r die aktualisierung der Nachrichten hinzuf�gen
  Fl::add_timeout(1.0, _static_timeout, this);

  // Solange in der Event-Loop bleiben, wie das Fenster sichtbar ist
  while (_wnd->shown()) Fl::wait();
}

//---------------------------------------------------------------

/**
   Statische Callback-Funktion

   \param sender Zeiger aud das Widget, das den Callback ausgel�st hat
   \param data User-Data, hier Zeiger auf den Dialog
*/

void CTLDialogMain::_callback(Fl_Widget *sender, void *data)
{
  CTLDialogMain *dialog = (CTLDialogMain *) data;

  if (sender == dialog->_grid_jobs) dialog->_grid_jobs_callback();
  if (sender == dialog->_grid_messages) dialog->_grid_messages_callback();
  if (sender == dialog->_button_close) dialog->_button_close_clicked();
  if (sender == dialog->_wnd) dialog->_button_close_clicked();
  if (sender == dialog->_button_add) dialog->_button_add_clicked();
  if (sender == dialog->_button_rem) dialog->_button_rem_clicked();
  if (sender == dialog->_button_state) dialog->_button_state_clicked();
}

//---------------------------------------------------------------

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
        _grid_jobs->current_content(_jobs[i].running() ? "gestartet" : "angehalten");
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
                _grid_jobs->current_content_color(FL_DARK_GREEN);
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
                _grid_jobs->current_content_color(FL_DARK_GREEN);
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
      _button_rem->activate();
      _update_button_state();
      break;

    case flgDeSelect:
      _button_rem->deactivate();
      _update_button_state();
      break;

    case flgDoubleClick:
      _edit_job(_grid_jobs->current_record());
      break;

    default:
      break;
  }
}

//---------------------------------------------------------------

/**
   Callback f�r das Nachrichten-Grid
*/

void CTLDialogMain::_grid_messages_callback()
{
  unsigned int i;

  switch (_grid_messages->current_event())
  {
    case flgContent:
      i = _grid_messages->current_record();

      if (_grid_messages->current_col() == "time")
      {
        _grid_messages->current_content(_messages[i].time);
      }
      else if (_grid_messages->current_col() == "text")
      {
        if (_messages[i].type == 1 && !_grid_messages->current_selected())
        {
          _grid_messages->current_content_color(FL_RED);
        }

        _grid_messages->current_content(_messages[i].text);
      }
      break;

    default:
      break;
  }
}

//---------------------------------------------------------------

/**
   Callback: Der "Beenden"-Button wurde geklickt
*/

void CTLDialogMain::_button_close_clicked()
{
  _wnd->hide();
}

//---------------------------------------------------------------

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

//---------------------------------------------------------------

/**
   Callback: Der "Entfernen"-Button wurde geklickt
*/

void CTLDialogMain::_button_rem_clicked()
{
#if 0
  stringstream sql, text;
  int sel = _grid_jobs->selected_index();
  
  text << "Dieser Befehl l�scht alle Daten zu diesem Auftrag" << endl;
  text << "Wollen Sie wirklich fortfahren?";

  if (fl_ask("%s", text.str().c_str()))
  {
    sql << "delete from jobs where id = " << _jobs[sel].id;
    
    try
    {
      _db->exec(sql.str().c_str());
    }
    catch (EDLSDB &e)
    {
      cout << "error deleting job: " << e.msg << endl;
    }
    
    _load_jobs();
  }
#endif
}

//---------------------------------------------------------------

/**
   Callback: Der "Starten/Anhalten"-Button wurde geklickt
*/

void CTLDialogMain::_button_state_clicked()
{
  int index = _grid_jobs->selected_index();
  CTLJobPreset job_copy;

  if (index < 0 || index >= (int) _jobs.size())
  {
    msg->str() << "Job index out of range!";
    msg->error();
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
    msg->str() << "Error writing preset file: " << e.msg;
    msg->error();
    return;
  }

  try
  {
    job_copy.notify_changed(_dls_dir);
  }
  catch (ECOMJobPreset &e)
  {
    msg->str() << "WARNING: dlsd could not be notified!";
    msg->warning();
  }

  _jobs[index] = job_copy;
  _grid_jobs->redraw();
  _update_button_state();
}

//---------------------------------------------------------------

/**
   Statische Callback-Funktion f�r den Nachrichten-Timeout

   \param data Zeiger auf den Dialog
*/

void CTLDialogMain::_static_timeout(void *data)
{
  CTLDialogMain *dialog = (CTLDialogMain *) data;

  dialog->_timeout();

  Fl::add_timeout(1.0, _static_timeout, dialog);
}

//---------------------------------------------------------------

/**
   Callback: Timeout! Die Nachrichten m�ssen aktualisiert werden
*/

void CTLDialogMain::_timeout()
{
  unsigned int i, top = 0;
  string top_time;
  bool found;

  // Watchdogs aktualisieren
  _load_watchdogs();

  // Messages aktualisieren

  if (_messages.size() > 0)
  {
    top = _grid_messages->top_index();
    top_time = _messages[top].time;
  }

  _load_messages();

  if (top > 0)
  {
    i = 0;
    found = false;

    while (i < _messages.size() && !found)
    {
      if (_messages[i].time == top_time) found = true;
      else i++;
    }

    if (found)
    {
      _grid_messages->scroll(i);
    }
  }
}

//---------------------------------------------------------------

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

//---------------------------------------------------------------

/**
   Alle Erfassungsauftr�ge laden
*/

void CTLDialogMain::_load_jobs()
{
  int job_id;
  DIR *dir;
  struct dirent *dir_ent;
  string dirname;
  stringstream str;
  fstream file;
  CTLJobPreset job;
  struct stat file_stat;
  stringstream watch_file_name;

  str.exceptions(ios::failbit | ios::badbit);

  // Liste der Auftr�ge leeren
  _grid_jobs->record_count(0);
  _jobs.clear();

   // Das Hauptverzeichnis �ffnen
  if ((dir = opendir(_dls_dir.c_str())) == NULL)
  {
    msg->str() << "Could not open dls directory \"" << _dls_dir << "\"";
    msg->error();
    return;
  }

  // Alle Dateien und Unterverzeichnisse durchlaufen
  while ((dir_ent = readdir(dir)) != NULL)
  {
    // Verzeichnisnamen kopieren
    dirname = dir_ent->d_name;

    // Wenn das Verzeichnis nicht mit "job" beginnt, das n�chste verarbeiten
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

    // Gibt es in dem Verzeichnis eine Datei job.xml?
    str.str("");
    str.clear();
    str << _dls_dir << "/" << dirname << "/job.xml";
    file.open(str.str().c_str(), ios::in);
    if (!file.is_open()) continue;
    file.close();

    try
    {
      job.import(_dls_dir, job_id);
    }
    catch (ECOMJobPreset &e)
    {
      msg->str() << "Could not import job " << job_id << ": " << e.msg;
      msg->error();
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
    watch_file_name << _dls_dir << "/job" << job_id << "/watchdog";

    if (stat(watch_file_name.str().c_str(), &file_stat) == 0)
    {
      _jobs.back().process_watchdog = file_stat.st_mtime;
    }

    watch_file_name.str("");
    watch_file_name.clear();
    watch_file_name << _dls_dir << "/job" << job_id << "/logging";

    if (stat(watch_file_name.str().c_str(), &file_stat) == 0)
    {
      _jobs.back().logging_watchdog = file_stat.st_mtime;
    }
  }

  _grid_jobs->record_count(_jobs.size());
}

//---------------------------------------------------------------

/**
   Alle allgemeinen Nachrichten laden
*/

void CTLDialogMain::_load_messages()
{
#if 0
  DLSDBQuery *query;
  int count, i;
  stringstream sql;

  _grid_messages->record_count(0);
  _messages.clear();

  sql << "select time, type, text"; // Optimiert: Klasse 1
  sql << " from messages_mother order by time desc";

  try
  {
    query = _db->query(sql.str().c_str());
    count = query->row_count();
  }
  catch (EDLSDB &e)
  {
    cout << "error fetching messages: " << e.msg << endl;
    return;
  }

  for (i = 0; i < count; i++)
  {
    struct DLSMessage msg;

    try
    {
      msg.time = query->value(i, "time").to_str();
      msg.type = query->value(i, "type").to_int();
      msg.text = query->value(i, "text").to_str();
    }
    catch (EDLSDB &e)
    {
      cout << "error reading msg: " << e.msg << endl;
      delete query;
      return;
    }

    _messages.push_back(msg);
  }

  delete query;

  _grid_messages->record_count(count);
#endif
}

//---------------------------------------------------------------

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

//---------------------------------------------------------------

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

//---------------------------------------------------------------

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

    if (fl_ask(str.str().c_str()) == 0) return;

    build_dls_dir = true;

    if (mkdir(_dls_dir.c_str(), 0755) == -1)
    {
      msg->str() << "Konnte das Verzeichnis \"" << _dls_dir << "\"";
      msg->str() << " nicht anlegen: " << strerror(errno);
      msg->error();
      return;
    }
  }
  else
  {
    // Pr�fen, ob das angegebene DLS-Datenverzeichnis �berhaupt
    // ein Verzeichnis ist
    if (!S_ISDIR(stat_buf.st_mode))
    {
      msg->str() << "\"" << _dls_dir << "\" ist kein Verzeichnis!";
      msg->error();
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

      if (fl_ask(str.str().c_str()) == 0) return;

      build_dls_dir = true;
    }

    // Spooling-Verzeichnis anlegen
    if (mkdir((_dls_dir + "/spool").c_str(), 0755) == -1)
    {
      msg->str() << "Konnte das Verzeichnis \"" << (_dls_dir + "/spool") << "\"";
      msg->str() << " nicht anlegen: " << strerror(errno);
      msg->error();
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

      if (fl_ask(str.str().c_str()) == 0) return;

      build_dls_dir = true;
    }

    // Datei anlegen
    if ((fd = open((_dls_dir + "/id_sequence").c_str(), O_WRONLY | O_CREAT, 0644)) == -1)
    {
      msg->str() << "Konnte die Datei \"" << (_dls_dir + "/id_sequence") << "\"";
      msg->str() << " nicht anlegen: " << strerror(errno);
      msg->error();
      return;
    }

    if (write(fd, "100\n", 4) != 4)
    {
      close(fd);
      msg->str() << "Konnte die Datei \"" << (_dls_dir + "/id_sequence") << "\"";
      msg->str() << " nicht beschreiben! Bitte manuell l�schen!";
      msg->error();
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
      msg->str() << "Konnte die Datei \"" << (_dls_dir + "/" + DLS_PID_FILE) << "\" nicht �ffnen!";
      msg->error();
      return;
    }

    try
    {
      pid_file >> pid;
    }
    catch (...)
    {
      pid_file.close();

      msg->str() << "Datei \"" << (_dls_dir + "/" + DLS_PID_FILE) << "\" ist korrupt!";
      msg->error();
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
        msg->str() << "Konnte Prozess " << pid << "nicht signalisieren!";
        msg->error();
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

    if (fl_ask(str.str().c_str()) == 1)
    {
      if ((pid = fork()) == -1)
      {
        msg->str() << "Could not fork()!";
        msg->error();
        return;
      }

      if (pid == 0) // Kindprozess
      {
        const char *params[4] = {"dlsd", "-d", _dls_dir.c_str(), 0};
        
        if (execvp("dlsd", (char * const *) params) == -1)
        {
          cerr << "ERROR: Could not exec dlsd: " << strerror(errno) << endl;
          exit(-1);
        }
      }
      else // Mutterprozess
      {
        waitpid(pid, &status, 0);
        
        if ((signed char) WEXITSTATUS(status) == -1)
        {
          msg->str() << "Could not execute dlsd! See console for error message.";
          msg->error();
          return;
        }
      }
    }
  }
}

//---------------------------------------------------------------
