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

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <syslog.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

#include <fstream>
using namespace std;

/*****************************************************************************/

#include "lib/XmlParser.h"
#include "lib/Time.h"
#include "lib/JobPreset.h"

#include "../config.h"
#include "globals.h"
#include "ProcMother.h"

/*****************************************************************************/

//#define DEBUG

/*****************************************************************************/

/**
   Konstruktor

   Führt nötige Initialisierungen durch und öffnet
   die Verbindung zum syslogd
*/

ProcMother::ProcMother()
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

ProcMother::~ProcMother()
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

int ProcMother::start(const string &dls_dir)
{
#ifdef DEBUG
    int p;
#endif

    _dls_dir = dls_dir;

    msg() << "----- DLS Mother process started -----";
    log(Info);

    msg() << "dlsd " << PACKAGE_VERSION << " revision " << REVISION;
    log(Info);

    msg() << "Using dir \"" << _dls_dir << "\"";
    log(Info);

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

        if (process_type != MotherProcess || _exit) break;

        // Alle Arbeit ist getan - Schlafen legen
        sleep(JOB_CHECK_INTERVAL);
    }

    if (process_type == MotherProcess) {
        while (_processes_running()) {
            // wait for SIGCHLD
            sleep(JOB_CHECK_INTERVAL);
            _check_signals();
        }

        msg() << "----- DLS Mother process finished. -----";
        log(Info);
    }

    return _exit_error ? -1 : 0;
}

/*****************************************************************************/

/**
   Löscht den gesamten Inhalt des Spooling-Verzeichnisses
*/

void ProcMother::_empty_spool()
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
        log(Error);

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
            log(Error);
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

void ProcMother::_check_jobs()
{
    DIR *dir;
    string dirname;
    struct dirent *dir_ent;
    stringstream str;
    unsigned int job_id;
    fstream file;
    JobPreset job;

    str.exceptions(ios::badbit | ios::failbit);

    // Das Hauptverzeichnis öffnen
    if ((dir = opendir(_dls_dir.c_str())) == NULL)
    {
        _exit = true;
        _exit_error = true;

        msg() << "Could not open DLS directory \"" << _dls_dir << "\"";
        log(Error);

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
        catch (LibDLS::EJobPreset &e)
        {
            msg() << "Importing job (" << job_id << "): " << e.msg;
            log(Error);
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

void ProcMother::_check_signals()
{
    int status;
    pid_t pid;
    int exit_code;
    list<JobPreset>::iterator job_i;
    list<pid_t> terminated;
    list<pid_t>::iterator term_i;

    if (sig_int_term) {
        // Rücksetzen, um nochmaliges Auswerten zu verhindern
        sig_int_term = 0;

        _exit = true;

        msg() << "SIGINT or SIGTERM received.";
        log(Info);

        // Alle Kindprozesse beenden
        for (job_i = _jobs.begin(); job_i != _jobs.end(); job_i++) {
            if (!job_i->process_exists())
                continue;

            msg() << "Terminating process for job " << job_i->id_desc()
                << " with PID " << job_i->process_id();
            log(Info);

            try {
                // Prozess terminieren
                job_i->process_terminate();
            }
            catch (LibDLS::EJobPreset &e) {
                msg() << e.msg;
                log(Warning);
            }
        }

        return;
    }

    while (_sig_child != sig_child) {
        _sig_child++;

        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            exit_code = (signed char) WEXITSTATUS(status);

            for (job_i = _jobs.begin(); job_i != _jobs.end(); job_i++) {
                if (job_i->process_id() != pid)
                    continue;

                job_i->process_exited(exit_code);
                msg() << "Process for job " << job_i->id_desc()
                    << " with PID " << pid
                    << " exited with code " << exit_code << ".";
                if (exit_code == E_DLS_ERROR_RESTART)
                    msg() << " Restarting in " << wait_before_restart << " s.";
                log(Info);
                break;
            }
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

void ProcMother::_check_spool()
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
        log(Error);

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

bool ProcMother::_spool_job(unsigned int job_id)
{
    JobPreset *job;
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
                log(Error);

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

bool ProcMother::_add_job(unsigned int job_id)
{
    JobPreset new_job;

    try
    {
        // Auftragsdatei auswerten
        new_job.import(_dls_dir, job_id);
    }
    catch (LibDLS::EJobPreset &e)
    {
        return false;
    }

    // Auftrag in die Liste einfügen
    _jobs.push_back(new_job);

    msg() << "New job " << new_job.id_desc();
    log(Info);

    return true;
}

/*****************************************************************************/

/**
   Importiert einen Auftrag neu

   \param job Zeiger auf den zu ändernden Auftrags
   \return true, wenn der Auftrag importiert und geändert wurde
*/

bool ProcMother::_change_job(JobPreset *job)
{
    JobPreset changed_job;

    try
    {
        // Auftragsdatei auswerten
        changed_job.import(_dls_dir, job->id());
    }
    catch (LibDLS::EJobPreset &e)
    {
        return false;
    }

    msg() << "Changed job " << job->id_desc();
    log(Info);

    // PID des laufenden Prozesses übernehmen
    changed_job.process_started(job->process_id());

    // Daten kopieren
    *job = changed_job;

    if (job->process_exists())
    {
        msg() << "Notifying process for job " << job->id_desc();
        msg() << " with PID " << job->process_id();
        log(Info);

        try
        {
            // Prozess benachrichtigen
            job->process_notify();
        }
        catch (LibDLS::EJobPreset &e)
        {
            msg() << e.msg;
            log(Warning);
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

bool ProcMother::_remove_job(unsigned int job_id)
{
    list<JobPreset>::iterator job_i;

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
                log(Info);

                try
                {
                    job_i->process_terminate();
                }
                catch (LibDLS::EJobPreset &e)
                {
                    msg() << e.msg;
                    log(Warning);
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
    log(Error);

    return false;
}

/*****************************************************************************/

/**
   Überwacht die aktuellen Erfassungsprozesse

   Startet für jeden Auftrag, der gerade erfasst werden soll,
   einen entsprechenden Prozess.
*/

void ProcMother::_check_processes()
{
    int fork_ret;
    list<JobPreset>::iterator job_i;
    string dir;

    for (job_i = _jobs.begin(); job_i != _jobs.end(); job_i++) {
        if (!job_i->running()
                || job_i->process_exists()
                || (job_i->last_exit_code() != E_DLS_SUCCESS
                    && (job_i->last_exit_code() != E_DLS_ERROR_RESTART
                        || (LibDLS::Time::now() - job_i->exit_time()
                            < LibDLS::Time(wait_before_restart * 1e6)))))
            continue;

        if (job_i->last_exit_code() == E_DLS_ERROR_RESTART) {
            msg() << "Restarting process for job " << job_i->id_desc();
            msg() << " after error.";
        } else {
            msg() << "Starting process for job " << job_i->id_desc() << ".";
        }

        log(Info);

        if (!(fork_ret = fork())) { // Kindprozess
            // Globale Forking-Flags setzen
            process_type = LoggingProcess;
            dlsd_job_id = job_i->id();
            break;
        } else if (fork_ret > 0) { // Elternprozess
            job_i->process_started(fork_ret);
            msg() << "Started process with PID "
                << job_i->process_id();
            log(Info);
        } else { // Fehler
            msg() << "FATAL: Error " << errno << " in fork()";
            log(Error);
        }
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

JobPreset *ProcMother::_job_exists(unsigned int id)
{
    list<JobPreset>::iterator job_i = _jobs.begin();

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

unsigned int ProcMother::_processes_running()
{
    list<JobPreset>::iterator job_i;
    unsigned int process_count = 0;

    for (job_i = _jobs.begin(); job_i != _jobs.end(); job_i++) {
        if (job_i->process_exists()) {
            process_count++;
        }
    }

    return process_count;
}

/*****************************************************************************/
