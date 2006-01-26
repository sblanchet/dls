//---------------------------------------------------------------
//
//  D L S _ M A I N . C P P
//
//---------------------------------------------------------------

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#include <iostream>
using namespace std;

//---------------------------------------------------------------

#include "dls_globals.hpp"
#include "dls_proc_mother.hpp"
#include "dls_proc_logger.hpp"

//---------------------------------------------------------------

unsigned int sig_int_term = 0;
unsigned int sig_hangup = 0;
unsigned int sig_child = 0;

unsigned int job_id = 0;
bool process_forked = false;

//---------------------------------------------------------------

void signal_handler(int);
void set_signal_handlers();
void init_daemon();
void closeSTDx();
void get_directory(int, char **, string *);
void check_running(const string *);
void create_pid_file(const string *);
void remove_pid_file(const string *);

//---------------------------------------------------------------

/**
   Hauptfunktion

   Setzt die Signalhandler und startet den Mutterprozess.
   Wenn dieser zurückkommt, handelt es sich entweder um ein
   ge'fork'tes Kind, in dem Fall einen Logging-Prozess, oder
   um die Beendigung des Mutterprozesses.

   \todo Prüfen, ob schon gestartet!

   \param argc Anzahl der Kommandozeilenparameter
   \param argv Array der Kommandozeilenparameter
   \returns Exit-Code
*/

int main(int argc, char **argv)
{
  DLSProcMother *mother_process;
  DLSProcLogger *logger_process;
  int exit_code;
  string dls_dir;

  cout << "starting data logging server version " << DLS_VERSION << endl;

  // Welches DLS-Verzeichnis soll benutzt werden?
  get_directory(argc, argv, &dls_dir);

  // Prüfen, ob auf dem gegebenen DLS-Verzeichnis
  // schon ein Daemon läuft
  check_running(&dls_dir);

  // Jetzt in einen Daemon verwandeln
  init_daemon();
  
  // PID-Datei erzeugen
  create_pid_file(&dls_dir);

  // STDIN, STDOUT und STDERR schliessen
  closeSTDx();

  // Signalhandler installieren
  set_signal_handlers();
  
  // Mutterprozess starten
  mother_process = new DLSProcMother();
  exit_code = mother_process->start(dls_dir);
  delete mother_process;

  if (process_forked)
  {
    // Multiplexer-Prozess starten
    logger_process = new DLSProcLogger(dls_dir, job_id);
    exit_code = logger_process->start();
    delete logger_process;
  }
  else
  {
    // PID-Datei der Mutterprozesses entfernen
    remove_pid_file(&dls_dir);
  }

  exit(exit_code);
}

//---------------------------------------------------------------

void signal_handler(int sig)
{
    switch (sig)
    {
	case SIGHUP:
	    sig_hangup++;
	    break;

	case SIGCHLD:
	    sig_child++;
	    break;

	case SIGINT:
	case SIGTERM:
	    sig_int_term++;
	    break;

	case SIGSEGV:
	    syslog(LOG_INFO, "CRITICAL: process caused a SEGMENTATION VIOLATION!");
	    exit(-1);
	    break;

	case SIGILL:
	    syslog(LOG_INFO, "CRITICAL: process became ILL!");
	    exit(-1);
	    break;

	case SIGFPE:
	    syslog(LOG_INFO, "CRITICAL: process caused a FLOTING POINT EXCEPTION!");
	    exit(-1);
	    break;
  }

  // Signalhandler wieder installieren
  set_signal_handlers();
}

//---------------------------------------------------------------

void set_signal_handlers()
{
  signal(SIGHUP, signal_handler);
  signal(SIGCHLD, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGSEGV, signal_handler);
  signal(SIGILL, signal_handler);
  signal(SIGFPE, signal_handler);
}

//---------------------------------------------------------------

void init_daemon()
{
  pid_t pid;

  if ((pid = fork()) < 0)
  {
    cerr << endl << "ERROR: could not fork()!" << endl << endl;
    exit(-1);
  }
  else if (pid) // Nur das geforkte Kind soll weiterleben!
  {
    exit(0);
  }

  cout << "data logging server daemon running with PID " << getpid() << endl;

  if (setsid() == -1) // Session leader werden
  {
    cerr << "ERROR: could not become session leader!" << endl;
    exit(-1);
  }

  if (chdir("/") < 0) // Nach root wechseln - jemand könnte ein Dateisystem
                      // unmounten, auf dem wir stehen
  {
    cerr << "ERROR: could not change to file root!" << endl;
    exit(-1);
  }

  umask(0); // Datei-Erstellungsmaske (kann keinen Fehler erzeugen)
}

//---------------------------------------------------------------

void closeSTDx()
{

#if NEVER

  if (close(0) < 0)
  {
    cerr << "WARNING: could not close STDIN" << endl;
  }

  if (close(1) < 0)
  {
    cerr << "WARNING: could not close STDOUT" << endl;
  }

  if (close(2) < 0)
  {
    cerr << "WARNING: could not close STDERR" << endl;
  }

#endif

}

//---------------------------------------------------------------

void get_directory(int argc, char **argv, string *dls_dir)
{
  char *env;

  // Ein Kommandozeilenparameter: Dieser ist das DLS-Verzeichnis
  if (argc == 2) *dls_dir = argv[1];

  // Kein Parameter: DLS-Verzeichnis aus Umgebungsvariable $DLS_DIR einlesen
  else if ((env = getenv("DLS_DIR")) != 0) *dls_dir = env;

  // Kein Parameter und $DLS_DIR leer: Standardverzeichnis nutzen
  else *dls_dir = DEFAULT_DLS_DIR;

  // Benutztes Verzeichnis ausgeben
  cout << "using dls directory \"" << *dls_dir << "\"" << endl;
}

//---------------------------------------------------------------

void check_running(const string *dls_dir)
{
  int pid_fd, pid, ret;
  string pid_file_name;
  char buffer[11];
  stringstream str;

  str.exceptions(ios::badbit | ios::failbit);

  pid_file_name = *dls_dir + "/" + PID_FILE_NAME;

  if ((pid_fd = open(pid_file_name.c_str(), O_RDONLY)) == -1)
  {
    if (errno == ENOENT)
    {
      // PID-Datei existiert nicht. Alles ok!
      return;
    }
    else
    {
      cerr << "ERROR: could not open PID file \"" << pid_file_name << "\"!" << endl;
      exit(-1);
    }
  }

  if ((ret = read(pid_fd, buffer, 10)) < 0)
  {
    close(pid_fd);
    cerr << "ERROR: could not read from PID file \"" << pid_file_name << "\"!" << endl;
    exit(-1);
  }

  close(pid_fd);

  buffer[ret] = 0;
  str << buffer;

  try
  {
    str >> pid;
  }
  catch (...)
  {
    cerr << "ERROR: PID file \"" << pid_file_name << "\" has no valid content!" << endl;
    exit(-1);
  }

  if (kill(pid, 0) == -1)
  {
    if (errno == ESRCH) // Prozess mit angegebener PID existiert nicht
    {
      cout << "INFO: deleting old pid file" << endl;
      
      if (unlink(pid_file_name.c_str()) == -1)
      {
        cerr << "ERROR: could not delete pid file \"" << pid_file_name << "\"!" << endl;
        exit(-1);
      }

      return;
    }
    else
    {
      cerr << "ERROR: could not signal process " << pid << "!" << endl;
      exit(-1);
    }
  }

  cerr << endl << "ERROR: process already running with PID " << pid << "!" << endl << endl;
  exit(-1);
}

//---------------------------------------------------------------

void create_pid_file(const string *dls_dir)
{
  int pid_fd, ret;
  string pid_file_name;
  stringstream str;

  pid_file_name = *dls_dir + "/" + PID_FILE_NAME;

  if ((pid_fd = open(pid_file_name.c_str(), O_WRONLY | O_CREAT, 0644)) == -1)
  {
    cerr << "ERROR: could not create PID file \"" << pid_file_name << "\"!" << endl;
    exit(-1);
  }

  str << getpid() << endl;
  
  if ((ret = write(pid_fd, str.str().c_str(), str.str().length())) != (int) str.str().length())
  {
    cerr << "ERROR: could not write to PID file \"" << pid_file_name << "\"!" << endl;
    exit(-1);
  }

  close(pid_fd);
}

//---------------------------------------------------------------

void remove_pid_file(const string *dls_dir)
{
  string pid_file_name;
  stringstream err;

  pid_file_name = *dls_dir + "/" + PID_FILE_NAME;

  if (unlink(pid_file_name.c_str()) == -1)
  {
    err << "ERROR: could not delete pid file \"" << pid_file_name << "\"!";
    syslog(LOG_INFO, err.str().c_str());
  }
}

//---------------------------------------------------------------



