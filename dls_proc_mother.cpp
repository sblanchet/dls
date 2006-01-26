//---------------------------------------------------------------
//
//  D L S _ P R O C _ M O T H E R . C P P
//
//---------------------------------------------------------------

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>

#include <fstream>
using namespace std;

//---------------------------------------------------------------

#include "com_xml_parser.hpp"
#include "com_time.hpp"
#include "com_job_preset.hpp"
#include "dls_globals.hpp"
#include "dls_proc_mother.hpp"

//---------------------------------------------------------------

/**
   Konstruktor

   Führt nötige Initialisierungen durch und öffnet
   die Verbindung zum syslogd
*/

DLSProcMother::DLSProcMother()
{
  _sig_child = 0;
  _exit = false;
  _exit_error = false;

  // Syslog initialisieren
  openlog("dlsd_mother", LOG_PID, LOG_DAEMON);
}

//---------------------------------------------------------------

/**
   Destruktor

   Schliesst die Verbindung zum syslogd
*/

DLSProcMother::~DLSProcMother()
{
  // Syslog schliessen
  closelog();
} 

//---------------------------------------------------------------

/**
   Starten des Mutterprozesses

   Löscht zuerst alle Einträge im Spooling-verzeichnis. Es
   wird sowieso alles neu eingelesen.

   Importiert dann alle Auftragsvorgaben.

   Main-Loop:

- Signale überprüfen
- Spooling-Verzeichnis überprüfen
- Prozesse den Vorgaben angleichen

*/

int DLSProcMother::start(const string &dls_dir)
{
  _dls_dir = dls_dir;

  _msg << "----- mother process started. -----";
  _log(DLSInfo);

  _msg << "using dir \"" << _dls_dir << "\"";
  _log(DLSInfo);

  // Spooling-Verzeichnis leeren
  _empty_spool();

  // Alle Aufträge laden
  _check_jobs();

  while (1)
  {
    // Sind zwischenzeitlich Signale eingetroffen?
    _check_signals();

    if (_exit) break;

    // Hat sich im Spooling-Verzeichnis etwas getan?
    _check_spool();

    if (_exit) break;

    // Laufen alle Prozesse noch?
    _check_processes();

    if (process_forked || _exit) break;

    // Alle Arbeit ist getan - Schlafen legen
    sleep(JOB_CHECK_INTERVAL);
  }

  if (!process_forked)
  {
    _msg << "----- mother process finished. -----";
    _log(DLSInfo);
  }

  return _exit_error ? -1 : 0;
}

//---------------------------------------------------------------

/**
   Löscht den gesamten Inhalt des Spooling-Verzeichnisses
*/

void DLSProcMother::_empty_spool()
{
  DIR *dir;
  string dirname, filename;
  struct dirent *dir_ent;

  dirname = _dls_dir + "/spool";

  // Das Hauptverzeichnis öffnen
  if ((dir = opendir(dirname.c_str())) == NULL)
  {
    _exit = true;
    _exit_error = true;

    _msg << "could not open spooling directory \"" << dirname << "\"";
    _log(DLSError);

    return;
  }

  // Alle Dateien durchlaufen
  while ((dir_ent = readdir(dir)) != NULL)
  {
    filename = dir_ent->d_name;

    if (filename == "." || filename == "..") continue;

    filename = dirname + "/" + filename;

    if (unlink(filename.c_str()) == -1)
    {
      _exit = true;
      _exit_error = true;

      _msg << "could not empty spooling directory \"" << dirname << "\"!";
      _log(DLSError);
      break;
    }
  }

  if (closedir(dir) == -1)
  {
    _msg << "closedir failed.";
    _log(DLSError);
  }
}

//---------------------------------------------------------------

/**
   Alle Auftragsvorgaben importieren

   Durchsucht das DLS-Datenverzeichnis nach Unterverzeichnissen
   mit Namen "job<ID>". Versucht dann, eine Datei "job.xml" zu
   öffnen und Auftragsvorgaben zu importieren. Klappt dies,
   wird der Auftrag in die Liste aufgenommen.
*/

void DLSProcMother::_check_jobs()
{
  DIR *dir;
  string dirname;
  struct dirent *dir_ent;
  stringstream str;
  unsigned int job_id;
  fstream file;
  COMJobPreset job;

  str.exceptions(ios::badbit | ios::failbit);

  // Das Hauptverzeichnis öffnen
  if ((dir = opendir(_dls_dir.c_str())) == NULL)
  {
    _exit = true;
    _exit_error = true;

    _msg << "could not open dls directory \"" << _dls_dir << "\"";
    _log(DLSError);

    return;
  }

  // Alle Dateien und Unterverzeichnisse durchlaufen
  while ((dir_ent = readdir(dir)) != NULL)
  {
    // Verzeichnisnamen kopieren
    dirname = dir_ent->d_name;

    // Wenn das Verzeichnis nicht mit "job" beginnt, das nächste verarbeiten
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
      // Auftragsdatei auswerten
      job.import(_dls_dir, job_id);
    }
    catch (ECOMJobPreset &e)
    {
      _msg << "importing job (" << job_id << "): " << e.msg;
      _log(DLSError);
      continue;
    }

    job.pid = 0;
    job.last_exit_code = 0;

    // Auftrag in die Liste einfügen
    _jobs.push_back(job);
  }

  if (closedir(dir) == -1)
  {
    _msg << "closedir failed.";
    _log(DLSError);
  }
}

//---------------------------------------------------------------

/**
   Überprüft, ob zwischenzeitlich Signale empfangen wurden

   Wurde SIGINT, oder SIGTERM empfangen, wird das Flag zum
   Beenden des Mutterprozesses gesetzt und das Signal
   an alle laufenden Erfassungsprozesse weitergeleitet.

   Wird SIGCHLD empfangen, wird der entsprechende
   Zombie-Prozess erlöst und der Exit-Code und die
   Zeit in das Vorgabenobjekt übernommen.   
*/

void DLSProcMother::_check_signals()
{
  int status, pid, exitcode;
  list<COMJobPreset>::iterator job_i;

  if (sig_int_term)
  {
    _exit = true;

    _msg << "SIGINT or SIGTERM received.";
    _log(DLSInfo);

    // Alle Kindprozesse beenden
    job_i = _jobs.begin();
    while (job_i != _jobs.end())
    {
      if (_process_exists(&(*job_i))) _process_term(&(*job_i));
      job_i++;
    }

    return;
  }

  while (_sig_child != sig_child)
  {
    pid = wait(&status); // Zombie töten!
    exitcode = (signed char) WEXITSTATUS(status);

    job_i = _jobs.begin();
    while (job_i != _jobs.end())
    {
      if (job_i->pid == pid)
      {
        job_i->last_exit_code = exitcode;
        job_i->exit_time.set_now();

        _msg << "process for job " << job_i->id_desc();
        _msg << " with PID " << job_i->pid;
        _msg << " exited with code " << job_i->last_exit_code;
        _log(DLSInfo);

        break;
      }

      job_i++;
    }

    _sig_child++;
  }
}

//---------------------------------------------------------------

/**
   Durchsucht das Spooling-Verzeichnis nach Änderungen

   Wird eine Datei im Spooling-Verzeichnis gefunden, dann
   wird zuerst überprüft, ob sie eine gültige
   Spooling-Information enthält. Davon gibt es folgende:

-new    Ein neuer Auftrag wurde hinzugefügt. Es wird ein
        passendes Vorgabenobjekt in die Liste eingefügt.
-change Ein bestehender Auftrag wurde geändert. Wenn ein
        Erfassungsprozess existiert, wird dieser
        benachrichtigt.
-delete Ein bestehender Auftrag wurde entfernt. Das
        zugehörige Objekt wird aus der Liste entfernt.
        Existiert ein Erfassungprozess, wird dieser
        beendet.

   Grundsätzlich gilt: Passiert ein Fehler bei der
   Verarbeitung einer Spooling-Datei, wird diese nicht
   gelöscht. Das Löschen dient als Bestätigung der
   fehlerfreien Verarbeitung.
   (Ausnahme: Der Erfassungsprozess eines geänderten
   Auftrages kann nicht benachrichtigt werden.)
*/

void DLSProcMother::_check_spool()
{
  DIR *dir;
  struct dirent *dir_ent;
  string spool_dir = _dls_dir + "/spool";
  COMXMLParser parser;
  int job_id;
  string action, filename;
  fstream file;
  COMJobPreset new_job, *job;
  list<COMJobPreset>::iterator job_i;

  // Das Spoolverzeichnis öffnen
  if ((dir = opendir(spool_dir.c_str())) == NULL)
  {
    _exit = true;
    _exit_error = true;

    _msg << "could not open spool directory \"" << spool_dir << "\"";
    _log(DLSError);

    return;
  }

  // Alle Dateien
  while ((dir_ent = readdir(dir)) != NULL)
  {
    filename = dir_ent->d_name;

    if (filename == "." || filename == "..") continue;

    filename = spool_dir + "/" + filename; 
    file.open(filename.c_str(), ios::in);

    if (!file.is_open()) continue;

    try
    {
      parser.parse(&file, "dls");
      action = parser.last_tag()->att("action")->to_str();
      job_id = parser.last_tag()->att("job")->to_int();
    }
    catch (...)
    {
      file.close();
      continue;
    }

    file.close();

    if (action == "new")
    {
      try
      {
        // Auftragsdatei auswerten
        new_job.import(_dls_dir, job_id);
      }
      catch (ECOMJobPreset &e)
      {
        continue;
      }

      new_job.pid = 0;
      new_job.last_exit_code = 0;

      // Spooling-Datei löschen
      if (unlink(filename.c_str()) != 0) continue;

      // Auftrag in die Liste einfügen
      _jobs.push_back(new_job);

      _msg << "new job " << new_job.id_desc();
      _log(DLSInfo);
    }

    else if (action == "change")
    {
      // Abbrechen, wenn der Auftrag nicht existiert
      if ((job = _job_exists(job_id)) == 0) continue;

      try
      {
        // Auftragsdatei auswerten
        new_job.import(_dls_dir, job_id);
      }
      catch (ECOMJobPreset &e)
      {
        continue;
      }

      // ERST Spooling-Datei löschen
      if (unlink(filename.c_str()) != 0) continue;

      // Daten kopieren
      new_job.pid = job->pid;
      new_job.last_exit_code = job->last_exit_code;
      *job = new_job;

      _msg << "changed job " << new_job.id_desc();
      _log(DLSInfo);

      if (_process_exists(&(*job)))
      {
        // Prozess benachrichtigen
        _process_notify(&(*job));
      }
      else
      {
        job->last_exit_code = 0;
      }
    }

    else if (action == "delete")
    {
      // Auftrag suchen
      job_i = _jobs.begin();
      while (job_i != _jobs.end())
      {
        if (job_i->id() == job_id)
        {
          if (_process_exists(&(*job_i))) _process_term(&(*job_i));

          _jobs.erase(job_i);

          // Spooling-Datei löschen
          unlink(filename.c_str());

          continue;
        }
        job_i++;
      }

      // Auftrag nicht gefunden!
      _msg << "job (" << job_id << ") not found!";
      _log(DLSError);
      continue;
    }
  }

  if (closedir(dir) == -1)
  {
    _msg << "closedir failed.";
    _log(DLSError);
  }
}

//---------------------------------------------------------------

/**
   Überwacht die aktuellen Erfassungsprozesse

   Startet für jeden Auftrag, der gerade erfasst werden soll,
   einen entsprechenden Prozess.
*/

void DLSProcMother::_check_processes()
{
  int fork_ret;
  list<COMJobPreset>::iterator job_i = _jobs.begin();
  string dir;

  while (job_i != _jobs.end())
  {
    // Prozess sollte laufen
    if (job_i->running()    

        // Tut er aber nicht!
        && !_process_exists(&(*job_i))      

        // und er darf entweder gestartet werden...
        && (job_i->last_exit_code == E_DLS_NO_ERROR 

            // ...oder er wurde auf Grund eines Zeit-Toleranzfehlers beendet...
            || (job_i->last_exit_code == E_DLS_TIME_TOLERANCE 

                // ...und die Wartezeit ist um!
                && job_i->exit_time <= COMTime::now() - COMTime(TIME_TOLERANCE_RESTART * 1000000.0))))
    {
      if (job_i->last_exit_code == E_DLS_TIME_TOLERANCE)
      {
        _msg << "restarting process for job " << job_i->id_desc();
        _msg << " after time tolerance error";
      }
      else
      {
        _msg << "starting process for job " << job_i->id_desc();
      }

      _log(DLSInfo);
      
      if ((fork_ret = fork()) == 0) // Kindprozess
      {
        job_id = job_i->id();
        process_forked = true; // Globales Flag setzen

        break;
      }
      else if (fork_ret > 0) // Elternprozess
      {
        job_i->pid = fork_ret;

        _msg << "started process with PID " << job_i->pid;
        _log(DLSInfo);
      }
      else // Fehler
      {
        job_i->pid = 0;

        _msg << "error " << errno << " in fork()";
        _log(DLSError);
      }
    }

    job_i++;
  }
}

//---------------------------------------------------------------

/**
   Prüft, ob ein Auftrag mit einer bestimmten ID in der Liste ist

   Wenn der Auftrag existiert, wird ein Zeiger auf das
   entsprechende Objekt zurückgegeben. Sonst wird 0
   zurückgeliefert.

   \param id Auftrags-ID
   \return Zeiger auf Auftragsvorgaben oder 0
*/

COMJobPreset *DLSProcMother::_job_exists(int id)
{
  list<COMJobPreset>::iterator job_i = _jobs.begin();

  while (job_i != _jobs.end())
  {
    if (job_i->id() == id)
    {
      return &(*job_i);
    }

    job_i++;
  }

  return 0;
}

//---------------------------------------------------------------

/**
   Prüft, ob zu einem Auftrag ein Erfassungsprozess läuft

   \param job Zeiger auf die Auftragsvorgaben
   \return true, wenn der Prozess läuft
*/

bool DLSProcMother::_process_exists(COMJobPreset *job)
{
  if (job->pid == 0) return false;

  if (kill(job->pid, 0) == -1)
  {
    if (errno == ESRCH)
    {
      job->pid = 0;
      return false;
    }
  }

  return true;
}

//---------------------------------------------------------------

/**
   Beendet einen Erfassungsprozess

   \param job Zeiger auf die Auftragsvorgaben
*/

void DLSProcMother::_process_term(COMJobPreset *job)
{
  if (job->pid == 0) return;

  _msg << "terminating process for job " << job->id_desc();
  _msg << " with PID " << job->pid;
  _log(DLSInfo);

  if (kill(job->pid, SIGTERM) == -1)
  {
    _msg << "kill(): process not terminated!";
    _log(DLSError);
  }
  else
  {
    job->pid = 0;
  }
}

//---------------------------------------------------------------

/**
   Benachrichtigt einen Erfassungsprozess über eine Änderung

   \param job Konstanter Zeiger auf die Auftragsvorgaben
*/

void DLSProcMother::_process_notify(const COMJobPreset *job)
{
  if (job->pid == 0) return;

  _msg << "notifying process for job " << job->id_desc();
  _msg << " with PID " << job->pid;
  _log(DLSInfo);

  if (kill(job->pid, SIGHUP) == -1)
  {
    _msg << "error in kill() - process not notified!";
    _log(DLSError);
  }
}

//---------------------------------------------------------------

/**
   Schreibt eine Nachricht in die Logging-Dateien

   Die Nachricht muss zuvor in _msg gespeichert
   worden sein. _msg wird dann geleert.

   \param type Typ der Nachricht
*/

void DLSProcMother::_log(DLSLogType type)
{
  string mode;
  COMTime time;

  time.set_now();

  if (type == DLSError) mode = "ERROR";
  else if (type == DLSInfo) mode = "INFO";
  else if (type == DLSWarning) mode = "WARNING";
  else mode = "UNKNOWN";

  syslog(LOG_INFO, "%s: %s", mode.c_str(), _msg.str().c_str());
  
  // Nachricht entfernen
  _msg.str("");
}

//---------------------------------------------------------------
