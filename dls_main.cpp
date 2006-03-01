//---------------------------------------------------------------
//
//  D L S _ M A I N . C P P
//
//---------------------------------------------------------------

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#include <iostream>
using namespace std;

//---------------------------------------------------------------

#include "dls_globals.hpp"
#include "dls_proc_mother.hpp"
#include "dls_proc_logger.hpp"

//---------------------------------------------------------------

RCS_ID("$Header: /home/fp/dls/src/RCS/dls_main.cpp,v 1.26 2005/02/23 13:44:09 fp Exp $");

//---------------------------------------------------------------

unsigned int sig_int_term = 0;
unsigned int sig_hangup = 0;
unsigned int sig_child = 0;
DLSProcessType process_type = dlsMotherProcess;
unsigned int job_id = 0;
string dls_dir = "";
DLSArchitecture arch;
DLSArchitecture source_arch;

//---------------------------------------------------------------

void get_options(int, char **);
void print_usage();
void signal_handler(int);
void set_signal_handlers();
void dump_signal(int);
void init_daemon();
void closeSTDXXX();
void check_running(const string *);
void create_pid_file(const string *);
void remove_pid_file(const string *);
void get_endianess();

//---------------------------------------------------------------

/**
   Hauptfunktion

   Setzt die Signalhandler und startet den Mutterprozess.
   Wenn dieser zurückkommt, handelt es sich entweder um ein
   ge'fork'tes Kind, in dem Fall einen Logging-Prozess, oder
   um die Beendigung des Mutterprozesses.

   \param argc Anzahl der Kommandozeilenparameter
   \param argv Array der Kommandozeilenparameter
   \returns Exit-Code
*/

int main(int argc, char **argv)
{
  DLSProcMother *mother_process;
  DLSProcLogger *logger_process;
  int exit_code;

  cout << dls_version_str << endl;

  is_daemon = true;

  // Endianess ermitteln
  get_endianess();

  // Kommandozeilenparameter verarbeiten
  get_options(argc, argv);

  // Prüfen, ob auf dem gegebenen DLS-Verzeichnis
  // schon ein Daemon läuft
  check_running(&dls_dir);

  // In einen Daemon verwandeln, wenn gewuenscht
  if (is_daemon) init_daemon();
  
  // PID-Datei erzeugen
  create_pid_file(&dls_dir);

  // Signalhandler installieren
  set_signal_handlers();

  // "Up and running!"
  cout << "DLS running with PID " << getpid();
  if (is_daemon) cout << " [daemon]";
  cout << endl;

  // Bei Bedarf STDIN, STDOUT und STDERR schliessen
  if (is_daemon) closeSTDXXX();

  try
  {
    // Mutterprozess starten
    mother_process = new DLSProcMother();
    exit_code = mother_process->start(dls_dir);
    delete mother_process;

    if (process_type == dlsLoggingProcess)
    {
      // Erfassungsprozess starten
      logger_process = new DLSProcLogger(dls_dir, job_id);
      exit_code = logger_process->start();
      delete logger_process;
    }
    else
    {
      // PID-Datei der Mutterprozesses entfernen
      remove_pid_file(&dls_dir);
    }
  }
  catch (COMException &e)
  {
    syslog(LOG_INFO, "CRITICAL: UNCATCHED KNOWN EXCEPTION! text: %s", e.msg.c_str());
  }
  catch (...)
  {
    syslog(LOG_INFO, "CRITICAL: UNCATCHED UNKNOWN EXCEPTION!");
  }

  // Evtl. allokierte Speicher der MDCT freigeben
  mdct_cleanup();

  exit(exit_code);
}

//---------------------------------------------------------------

void get_options(int argc, char **argv)
{
  int c;
  bool dir_set = false;
  char *env;

  do
  {
    c = getopt(argc, argv, "d:kh");

    switch (c)
    {
      case 'd':
        dir_set = true;
        dls_dir = optarg;
        break;

      case 'k':
        is_daemon = false;
        break;

      case 'h':
      case '?':
        print_usage();
        break;

      default:
        break;
    }
  }
  while (c != -1);
  
  // Weitere Parameter vorhanden?
  if (optind < argc)
  {
    print_usage();
  }

  if (!dir_set)
  {
    // DLS-Verzeichnis aus Umgebungsvariable $DLS_DIR einlesen
    if ((env = getenv(ENV_DLS_DIR)) != 0) dls_dir = env;

    // $DLS_DIR leer: Standardverzeichnis nutzen
    else dls_dir = DEFAULT_DLS_DIR;
  }

  // Benutztes Verzeichnis ausgeben
  cout << "using dls directory \"" << dls_dir << "\"" << endl;

  if (!is_daemon)
  {
    cout << "NOT detaching from tty!" << endl;
  }
}

//---------------------------------------------------------------

void print_usage()
{
  cout << "Aufruf: dlsd [OPTIONEN]" << endl;
  cout << "        -d [Verzeichnis]   DLS-Datenverzeichnis angeben" << endl;
  cout << "        -k                 Nicht von der Konsole trennen" << endl;
  cout << "        -h                 Diese Hilfe anzeigen" << endl;
  exit(0);
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
      
    default:
      dump_signal(sig);
      _exit(E_DLS_SIGNAL);
  }
}

//---------------------------------------------------------------

void set_signal_handlers()
{
  struct sigaction action;

  action.sa_handler = signal_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;

  sigaction(SIGHUP, &action, 0);
  sigaction(SIGCHLD, &action, 0);
  sigaction(SIGINT, &action, 0);
  sigaction(SIGTERM, &action, 0);
  sigaction(SIGSEGV, &action, 0);
  sigaction(SIGILL, &action, 0);
  sigaction(SIGFPE, &action, 0);
  sigaction(SIGQUIT, &action, 0);
  sigaction(SIGABRT, &action, 0);
  sigaction(SIGPIPE, &action, 0);
  sigaction(SIGALRM, &action, 0);
  sigaction(SIGUSR1, &action, 0);
  sigaction(SIGUSR2, &action, 0);
  sigaction(SIGTRAP, &action, 0);
}

//---------------------------------------------------------------

void dump_signal(int sig)
{
  int fd;
  stringstream err, file;

  file << dls_dir << "/error_" << getpid();

  err << "process (" << getpid() << ")";
  err << " caught a signal: \"" << sys_siglist[sig] << "\"";
  err << " at " << time(0) << endl;

  fd = open(file.str().c_str(), O_WRONLY | O_CREAT, 0644);
  if (fd != -1)
  {
    write(fd, err.str().c_str(), err.str().length());
    close(fd);
  }

  if (!is_daemon) cout << "CRITICAL: " << err;
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

  if (setsid() == -1) // Session leader werden
  {
    cerr << "ERROR: could not become session leader!" << endl;
    exit(-1);
  }

  if (chdir("/") < 0) // Nach root wechseln - jemand könnte ein Dateisystem
                      // unmounten wollen, auf dem wir stehen!
  {
    cerr << "ERROR: could not change to file root!" << endl;
    exit(-1);
  }

  umask(0); // Datei-Erstellungsmaske (kann keinen Fehler erzeugen)
}

//---------------------------------------------------------------

void closeSTDXXX()
{
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
}

//---------------------------------------------------------------

void check_running(const string *dls_dir)
{
  int pid_fd, pid, ret;
  string pid_file_name;
  char buffer[11];
  stringstream str;

  str.exceptions(ios::badbit | ios::failbit);

  pid_file_name = *dls_dir + "/" + DLS_PID_FILE;

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

  pid_file_name = *dls_dir + "/" + DLS_PID_FILE;

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

  pid_file_name = *dls_dir + "/" + DLS_PID_FILE;

  if (unlink(pid_file_name.c_str()) == -1)
  {
    err << "ERROR: could not delete pid file \"" << pid_file_name << "\"!";
    syslog(LOG_INFO, err.str().c_str());
  }
}

//---------------------------------------------------------------

void get_endianess()
{
  unsigned int i, value;
  unsigned char *byte;
  bool is_little_endian, is_big_endian;

  // Test-Integer vorbelegen
  value = 0;
  for (i = 0; i < sizeof(value); i++) value += (1 << (i * 8)) * (i + 1);
  
  byte = (unsigned char *) &value;

  is_little_endian = true;
  for (i = 0; i < sizeof(value); i++) if (byte[i] != (i + 1)) is_little_endian = false;
  
  if (is_little_endian)
  {
    arch = LittleEndian;
    return;
  }

  is_big_endian = true;
  for (i = 0; i < sizeof(value); i++) if (byte[i] != sizeof(value) - i) is_big_endian = false;
  
  if (is_big_endian)
  {
    arch = BigEndian;
    return;
  }

  cerr << "ERROR: Unknown architecture!" << endl;
  exit(-1);
}

//---------------------------------------------------------------
