/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <syslog.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>

#include <fstream>
using namespace std;

/*****************************************************************************/

#include "com_xml_parser.hpp"
#include "com_time.hpp"
#include "com_job_preset.hpp"
#include "dls_globals.hpp"
#include "dls_proc_mother.hpp"

/*****************************************************************************/

//#define DEBUG

/*****************************************************************************/

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

/*****************************************************************************/

/**
   Destruktor

   Schliesst die Verbindung zum syslogd
*/

DLSProcMother::~DLSProcMother()
{
    // Syslog schliessen
    closelog();
}

/*****************************************************************************/

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
#ifdef DEBUG
    int p;
#endif

    _dls_dir = dls_dir;

    msg() << "----- DLS Mother process started -----";
    log(DLSInfo);

    msg() << "dlsd " << PACKAGE_VERSION << " revision " << REVISION;
    log(DLSInfo);

    msg() << "Using dir \"" << _dls_dir << "\"";
    log(DLSInfo);

    // Spooling-Verzeichnis leeren
    _empty_spool();

    // Anfangs einmal alle Aufträge laden
    _check_jobs();

    while (!_exit)
    {
        // Sind zwischenzeitlich Signale eingetroffen?
        _check_signals();

        if (_exit) break;

        // Hat sich im Spooling-Verzeichnis etwas getan?
        _check_spool();

        if (_exit) break;

        // Laufen alle Prozesse noch?
        _check_processes();

        if (process_type != dlsMotherProcess || _exit) break;

        // Alle Arbeit ist getan - Schlafen legen
        sleep(JOB_CHECK_INTERVAL);
    }

    if (process_type == dlsMotherProcess)
    {
        // Auf alle Erfassungprozesse Warten
        while (_processes_running())
        {
            // Warten ...
            sleep(JOB_CHECK_INTERVAL);

            // ... auf SIGCHLD
            _check_signals();
        }

        msg() << "----- DLS Mother process finished. -----";
        log(DLSInfo);
    }

    return _exit_error ? -1 : 0;
}

/*****************************************************************************/

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

        msg() << "Could not open spooling directory \""
              << dirname << "\"";
        log(DLSError);

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

            msg() << "Could not empty spooling directory \""
                  << dirname << "\"!";
            log(DLSError);
            break;
        }
    }

    closedir(dir);
}

/*****************************************************************************/

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
    DLSJobPreset job;

    str.exceptions(ios::badbit | ios::failbit);

    // Das Hauptverzeichnis öffnen
    if ((dir = opendir(_dls_dir.c_str())) == NULL)
    {
        _exit = true;
        _exit_error = true;

        msg() << "Could not open DLS directory \"" << _dls_dir << "\"";
        log(DLSError);

        return;
    }

    // Alle Dateien und Unterverzeichnisse durchlaufen
    while ((dir_ent = readdir(dir)) != NULL)
    {
        // Verzeichnisnamen kopieren
        dirname = dir_ent->d_name;

        // Wenn das Verzeichnis nicht mit "job" beginnt,
        // das nächste verarbeiten
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
            msg() << "Importing job (" << job_id << "): " << e.msg;
            log(DLSError);
            continue;
        }

        // Auftrag in die Liste einfügen
        _jobs.push_back(job);
    }

    closedir(dir);
}

/*****************************************************************************/

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
    int status;
    pid_t pid;
    int exit_code;
    list<DLSJobPreset>::iterator job_i;
    list<pid_t> terminated;
    list<pid_t>::iterator term_i;

    if (sig_int_term)
    {
        // Rücksetzen, um nochmaliges Auswerten zu verhindern
        sig_int_term = 0;

        _exit = true;

        msg() << "SIGINT or SIGTERM received.";
        log(DLSInfo);

        // Alle Kindprozesse beenden
        job_i = _jobs.begin();
        while (job_i != _jobs.end())
        {
            if (job_i->process_exists())
            {
                msg() << "Terminating process for job "
                      << job_i->id_desc();
                msg() << " with PID " << job_i->process_id();
                log(DLSInfo);

                try
                {
                    // Prozess terminieren
                    job_i->process_terminate();
                }
                catch (ECOMJobPreset &e)
                {
                    msg() << e.msg;
                    log(DLSWarning);
                }
            }

            job_i++;
        }

        return;
    }

    while (_sig_child != sig_child)
    {
        _sig_child++;

        pid = wait(&status); // Zombie töten!
        exit_code = (signed char) WEXITSTATUS(status);

        job_i = _jobs.begin();
        while (job_i != _jobs.end())
        {
            if (job_i->process_id() == pid)
            {
                job_i->process_exited(exit_code);

                msg() << "Process for job " << job_i->id_desc()
                    << " with PID " << pid
                    << " exited with code " << exit_code
                    << ". Restarting in " << WAIT_BEFORE_RESTART << " s.";
                log(DLSInfo);

                break;
            }

            job_i++;
        }
    }
}

/*****************************************************************************/

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
    unsigned int job_id;
    string filename;
    fstream file;

    file.exceptions(ios::badbit | ios::failbit);

    // Das Spoolverzeichnis öffnen
    if ((dir = opendir(spool_dir.c_str())) == NULL)
    {
        _exit = true;
        _exit_error = true;

        msg() << "Could not open spool directory \""
              << spool_dir << "\"";
        log(DLSError);

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
            file >> job_id;
        }
        catch (...)
        {
            file.close();
            continue;
        }

        file.close();

        // Auftrag überprüfen
        if (_spool_job(job_id))
        {
            // Spooling-Datei löschen
            unlink(filename.c_str());
        }
    }

    closedir(dir);
}

/*****************************************************************************/

/**
   Bestimmt, wie mit einer "gespoolten" Job-ID verfahren wird

   Der Rückgabewert dient vornehmlich dazu, festzustellen,
   ob die Spooling-Datei gelöscht werden kann.

   \param job_id Auftrags-ID aus einer Spooling-Datei
   \return true, wenn das Spooling erfolgreich war
*/

bool DLSProcMother::_spool_job(unsigned int job_id)
{
    DLSJobPreset *job;
    stringstream job_file_name;
    struct stat stat_buf;

    // Prüfen, ob ein Auftrag mit dieser ID schon in der Liste ist
    if ((job = _job_exists(job_id)) == 0)
    {
        // Nein. Den Auftrag zur Liste hinzufügen
        return _add_job(job_id);
    }
    else // Der Auftrag existiert in der Liste
    {
        job_file_name << _dls_dir << "/job" << job_id << "/job.xml";

        // Prüfen, ob die Auftragsvorgabendatei noch existiert
        if (stat(job_file_name.str().c_str(), &stat_buf) == -1)
        {
            if (errno == ENOENT) // Datei existiert nicht
            {
                // Den entsprechenden Auftrag entfernen
                return _remove_job(job_id);
            }
            else // Fehler beim Aufruf von stat()
            {
                _exit = true;
                _exit_error = E_DLS_ERROR;

                msg() << "Calling stat() on \"" << job_file_name.str()
                      << "\": " << strerror(errno);
                log(DLSError);

                return false;
            }
        }
        else // Auftrags-Vorgabendatei existiert noch
        {
            // Den Auftrag neu importieren
            return _change_job(job);
        }
    }
}

/*****************************************************************************/

/**
   Fügt einen neuen Auftrag in die Liste ein

   \param job_id Auftrags-ID des neuen Auftrags
   \return true, wenn der Auftrag importiert und hinzugefügt wurde
*/

bool DLSProcMother::_add_job(unsigned int job_id)
{
    DLSJobPreset new_job;

    try
    {
        // Auftragsdatei auswerten
        new_job.import(_dls_dir, job_id);
    }
    catch (ECOMJobPreset &e)
    {
        return false;
    }

    // Auftrag in die Liste einfügen
    _jobs.push_back(new_job);

    msg() << "New job " << new_job.id_desc();
    log(DLSInfo);

    return true;
}

/*****************************************************************************/

/**
   Importiert einen Auftrag neu

   \param job Zeiger auf den zu ändernden Auftrags
   \return true, wenn der Auftrag importiert und geändert wurde
*/

bool DLSProcMother::_change_job(DLSJobPreset *job)
{
    DLSJobPreset changed_job;

    try
    {
        // Auftragsdatei auswerten
        changed_job.import(_dls_dir, job->id());
    }
    catch (ECOMJobPreset &e)
    {
        return false;
    }

    msg() << "Changed job " << job->id_desc();
    log(DLSInfo);

    // PID des laufenden Prozesses übernehmen
    changed_job.process_started(job->process_id());

    // Daten kopieren
    *job = changed_job;

    if (job->process_exists())
    {
        msg() << "Notifying process for job " << job->id_desc();
        msg() << " with PID " << job->process_id();
        log(DLSInfo);

        try
        {
            // Prozess benachrichtigen
            job->process_notify();
        }
        catch (ECOMJobPreset &e)
        {
            msg() << e.msg;
            log(DLSWarning);
        }
    }
    else
    {
        job->allow_restart();
    }

    return true;
}

/*****************************************************************************/

/**
   Entfernt einen Auftrag aus der Liste und beendet die Erfassung

   \param job_id Auftrags-ID des zu entfernenden Auftrags
   \return true, wenn der Auftrag gefunden und entfernt wurde
*/

bool DLSProcMother::_remove_job(unsigned int job_id)
{
    list<DLSJobPreset>::iterator job_i;

    // Auftrag suchen
    job_i = _jobs.begin();
    while (job_i != _jobs.end())
    {
        if (job_i->id() == job_id)
        {
            if (job_i->process_exists())
            {
                msg() << "Terminating process for job "
                      << job_i->id_desc();
                msg() << " with PID " << job_i->process_id();
                log(DLSInfo);

                try
                {
                    job_i->process_terminate();
                }
                catch (ECOMJobPreset &e)
                {
                    msg() << e.msg;
                    log(DLSWarning);
                }
            }

            // TODO: Hier noch nicht löschen,
            // erst wenn Prozess beendet.
            _jobs.erase(job_i);

            return true;
        }

        job_i++;
    }

    // Auftrag nicht gefunden!
    msg() << "Job (" << job_id << ") was not found!";
    log(DLSError);

    return false;
}

/*****************************************************************************/

/**
   Überwacht die aktuellen Erfassungsprozesse

   Startet für jeden Auftrag, der gerade erfasst werden soll,
   einen entsprechenden Prozess.
*/

void DLSProcMother::_check_processes()
{
    int fork_ret;
    list<DLSJobPreset>::iterator job_i = _jobs.begin();
    string dir;

    while (job_i != _jobs.end())
    {
        // Prozess sollte laufen
        if (job_i->running()

            // Tut er aber nicht!
            && !job_i->process_exists()

            // und er darf entweder gestartet werden...
            && (job_i->last_exit_code() == E_DLS_NO_ERROR

                // ...oder er ist fehlerhaft beendet worden, soll aber neu
                // gestartet werden...
                || (job_i->last_exit_code() == E_DLS_ERROR_RESTART

                    // ...und die Wartezeit ist um!
                    && (job_i->exit_time() <=
                        COMTime::now() - COMTime(WAIT_BEFORE_RESTART * 1e6)))))
        {
            if (job_i->last_exit_code() == E_DLS_ERROR_RESTART) {
                msg() << "Restarting process for job "
                      << job_i->id_desc();
                msg() << " after error.";
            } else {
                msg() << "Starting process for job "
                      << job_i->id_desc() << ".";
            }

            log(DLSInfo);

            if ((fork_ret = fork()) == 0) // Kindprozess
            {
                // Globale Forking-Flags setzen
                process_type = dlsLoggingProcess;
                dlsd_job_id = job_i->id();

                break;
            }
            else if (fork_ret > 0) // Elternprozess
            {
                job_i->process_started(fork_ret);

                msg() << "Started process with PID "
                      << job_i->process_id();
                log(DLSInfo);
            }
            else // Fehler
            {
                msg() << "FATAL: Error " << errno << " in fork()";
                log(DLSError);
            }
        }

        job_i++;
    }
}

/*****************************************************************************/

/**
   Prüft, ob ein Auftrag mit einer bestimmten ID in der Liste ist

   Wenn der Auftrag existiert, wird ein Zeiger auf das
   entsprechende Objekt zurückgegeben. Sonst wird 0
   zurückgeliefert.

   \param id Auftrags-ID
   \return Zeiger auf Auftragsvorgaben oder 0
*/

DLSJobPreset *DLSProcMother::_job_exists(unsigned int id)
{
    list<DLSJobPreset>::iterator job_i = _jobs.begin();

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

/*****************************************************************************/

/**
   Prüft, ob noch Erfassungprozesse laufen

   \return Anzahl der laufenden Erfassungprozesse
*/

unsigned int DLSProcMother::_processes_running()
{
    list<DLSJobPreset>::iterator job_i;
    unsigned int process_count = 0;

    job_i = _jobs.begin();
    while (job_i != _jobs.end())
    {
        if (job_i->process_exists())
        {
            process_count++;
        }

        job_i++;
    }

    return process_count;
}

/*****************************************************************************/
