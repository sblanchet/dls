/******************************************************************************
 *
 *  $Id$
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

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pwd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
using namespace std;

/*****************************************************************************/

#include "dls_globals.hpp"
#include "dls_proc_mother.hpp"
#include "dls_proc_logger.hpp"
#include "mdct.hpp"

/*****************************************************************************/

unsigned int sig_int_term = 0;
unsigned int sig_hangup = 0;
unsigned int sig_child = 0;
DLSProcessType process_type = dlsMotherProcess;
unsigned int dlsd_job_id = 0;
string dls_dir = "";
DLSArchitecture arch;
DLSArchitecture source_arch;
char user_name[100 + 1];
unsigned long num_files;
#define WORKING_DIR_SIZE 100
char working_dir[WORKING_DIR_SIZE + 1];
unsigned int wait_before_restart = DEFAULT_WAIT_BEFORE_RESTART;

/*****************************************************************************/

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

/*****************************************************************************/

/**
   Hauptfunktion

   Setzt die Signalhandler und startet den Mutterprozess.
   Wenn dieser zur�ckkommt, handelt es sich entweder um ein
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
    int exit_code = 1;
    struct rlimit rlim;
    struct passwd *pwd;

    cout << "dlsd " << PACKAGE_VERSION << " revision " << REVISION << endl;

    is_daemon = true;
    strcpy(user_name, "");
    num_files = FOPEN_MAX;

    // remember current working directory
    if (!getcwd(working_dir, WORKING_DIR_SIZE)) {
        cerr << "ERROR: Failed to get working directory: "
            << strerror(errno) << endl;
        exit(-1);
    }

    // Endianess ermitteln
    get_endianess();

    // Kommandozeilenparameter verarbeiten
    get_options(argc, argv);

    // Pr�fen, ob auf dem gegebenen DLS-Verzeichnis
    // schon ein Daemon l�uft
    check_running(&dls_dir);

    // In einen Daemon verwandeln, wenn gewuenscht
    if (is_daemon) init_daemon();

    if (num_files != FOPEN_MAX)
    {
        // Maximale Anzahl offener Dateien aendern

        rlim.rlim_cur = num_files;
        rlim.rlim_max = num_files;

        if (setrlimit(RLIMIT_NOFILE, &rlim) == -1)
        {
            cerr << "ERROR: Could not set maximal";
            cerr << " number of open files: " << strerror(errno) << endl;
            exit(1);
        }

        getrlimit(RLIMIT_NOFILE, &rlim);
        cout << "Maximal number of open files:" << endl;
        cout << "   Soft: " << rlim.rlim_cur;
        cout << " Hard: " << rlim.rlim_max;
        cout << " Kern: " << sysconf(_SC_OPEN_MAX) << endl;
    }

    if (strcmp(user_name, "") != 0) // Wenn Benutzer angegeben
    {
        // Zu angegebenem Benutzer wechseln

        if ((pwd = getpwnam(user_name)) == NULL)
        {
            cerr << "ERROR: User \"" << user_name
                 << "\" does not exist!" << endl;
            exit(1);
        }

        cout << "Switching to UID " << pwd->pw_uid << "." << endl;

        if (setuid(pwd->pw_uid) == -1)
        {
            cerr << "ERROR: Could not switch to UID " << pwd->pw_uid;
            cerr << ": " << strerror(errno);
            exit(1);
        }
    }

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
            logger_process = new DLSProcLogger(dls_dir, dlsd_job_id);
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
        syslog(LOG_INFO, "CRITICAL: UNCATCHED KNOWN EXCEPTION! text: %s",
               e.msg.c_str());
    }
    catch (...)
    {
        syslog(LOG_INFO, "CRITICAL: UNCATCHED UNKNOWN EXCEPTION!");
    }

    // Evtl. allozierte Speicher der MDCT freigeben
    mdct_cleanup();

    exit(exit_code);
}

/*****************************************************************************/

void get_options(int argc, char **argv)
{
    int c;
    bool dir_set = false;
    char *env, *remainder;

    do {
        c = getopt(argc, argv, "d:u:n:kw:h");

        switch (c) {
            case 'd':
                dir_set = true;
                dls_dir = optarg;
                break;

            case 'u':
                strncpy(user_name, optarg, 100);
                break;

            case 'n':
                num_files = strtoul(optarg, &remainder, 10);

                if (remainder == optarg || *remainder || num_files == 0) {
                    cerr << "Invalid number of open files: "
                        << optarg << endl;
                    print_usage();
                }

                break;

            case 'k':
                is_daemon = false;
                break;

            case 'w':
                wait_before_restart = strtoul(optarg, &remainder, 10);

                if (remainder == optarg || *remainder) {
                    cerr << "Invalid wait time: " << optarg << endl;
                    print_usage();
                }

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
    if (optind < argc) {
        print_usage();
    }

    if (!dir_set) {
        // DLS-Verzeichnis aus Umgebungsvariable $DLS_DIR einlesen
        if ((env = getenv(ENV_DLS_DIR)) != 0) dls_dir = env;
    }

    // make dls_dir absolute
    if (!dls_dir.size() || dls_dir[0] != '/')
        dls_dir = string(working_dir) + "/" + dls_dir;

    // Benutztes Verzeichnis ausgeben
    cout << "Using dls directory \"" << dls_dir << "\"" << endl;

    if (!is_daemon) {
        cout << "NOT detaching from tty!" << endl;
    }
}

/*****************************************************************************/

void print_usage()
{
    cout
        << "Usage: dlsd [OPTIONS]" << endl
        << "  -d <dir>      Set DLS data directory." << endl
        << "  -u <user>     Switch to <user>." << endl
        << "  -n <number>   Set maximal number of open files." << endl
        << "  -k            Do not detach from console." << endl
        << "  -w <seconds>  Wait time before restarting logging" << endl
        << "                  process after an error. Default is "
        << DEFAULT_WAIT_BEFORE_RESTART << "." << endl
        << "  -h            Show this help." << endl;
    exit(0);
}

/*****************************************************************************/

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

/*****************************************************************************/

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

/*****************************************************************************/

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
        if (write(fd, err.str().c_str(), err.str().length()) == -1
                && !is_daemon)
            cerr << "Failed to dump signal." << endl;
        close(fd);
    }

    if (!is_daemon) cout << "CRITICAL: " << err;
}

/*****************************************************************************/

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

    if (chdir("/") < 0) // Nach root wechseln - jemand k�nnte ein Dateisystem
        // unmounten wollen, auf dem wir stehen!
    {
        cerr << "ERROR: could not change to file root!" << endl;
        exit(-1);
    }

    umask(0); // Datei-Erstellungsmaske (kann keinen Fehler erzeugen)
}

/*****************************************************************************/

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

/*****************************************************************************/

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
            cerr << "ERROR: could not open PID file \"" << pid_file_name
                 << "\"!" << endl;
            exit(-1);
        }
    }

    if ((ret = read(pid_fd, buffer, 10)) < 0)
    {
        close(pid_fd);
        cerr << "ERROR: could not read from PID file \"" << pid_file_name
             << "\"!" << endl;
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
        cerr << "ERROR: PID file \"" << pid_file_name
             << "\" has no valid content!" << endl;
        exit(-1);
    }

    if (kill(pid, 0) == -1)
    {
        if (errno == ESRCH) // Prozess mit angegebener PID existiert nicht
        {
            cout << "INFO: deleting old pid file" << endl;

            if (unlink(pid_file_name.c_str()) == -1)
            {
                cerr << "ERROR: could not delete pid file \""
                     << pid_file_name << "\"!" << endl;
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

    cerr << endl << "ERROR: process already running with PID " << pid
         << "!" << endl << endl;
    exit(-1);
}

/*****************************************************************************/

void create_pid_file(const string *dls_dir)
{
    int pid_fd, ret;
    string pid_file_name;
    stringstream str;

    pid_file_name = *dls_dir + "/" + DLS_PID_FILE;

    if ((pid_fd = open(pid_file_name.c_str(), O_WRONLY | O_CREAT, 0644)) == -1)
    {
        cerr << "ERROR: could not create PID file \"" << pid_file_name
             << "\": " << strerror(errno) << endl;
        exit(-1);
    }

    str << getpid() << endl;

    if ((ret = write(pid_fd, str.str().c_str(),
                     str.str().length())) != (int) str.str().length())
    {
        cerr << "ERROR: could not write to PID file \"" << pid_file_name
             << "\": " << strerror(errno) << endl;
        exit(-1);
    }

    close(pid_fd);
}

/*****************************************************************************/

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

/*****************************************************************************/

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
    for (i = 0; i < sizeof(value); i++)
        if (byte[i] != (i + 1)) is_little_endian = false;

    if (is_little_endian)
    {
        arch = LittleEndian;
        return;
    }

    is_big_endian = true;
    for (i = 0; i < sizeof(value); i++)
        if (byte[i] != sizeof(value) - i) is_big_endian = false;

    if (is_big_endian)
    {
        arch = BigEndian;
        return;
    }

    cerr << "ERROR: Unknown architecture!" << endl;
    exit(-1);
}

/*****************************************************************************/
