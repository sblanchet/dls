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
#include <sys/stat.h>
#include <unistd.h>

#include <sstream>
#include <fstream>
using namespace std;

/*****************************************************************************/

#include "dls_globals.hpp"
#include "dls_proc_logger.hpp"
#include "dls_job.hpp"

/*****************************************************************************/

//#define DEBUG
//#define DEBUG_SIZES

/*****************************************************************************/

/** Konstruktor

   \param parent_proc Zeiger auf den besitzenden Logging-Prozess
   \param dls_dir DLS-Datenverzeichnis
*/
DLSJob::DLSJob(
		DLSProcLogger *parent_proc,
		const string &dls_dir
		):
	_parent_proc(parent_proc),
	_dls_dir(dls_dir),
	_id_gen(0),
	_logging_started(false),
    _msg_chunk_created(false)
{
}

/*****************************************************************************/

/**
   Destruktor
*/

DLSJob::~DLSJob()
{
    _clear_loggers();
}

/*****************************************************************************/

/**
   Importiert die Vorgaben für den aktuellen Auftrag

   \param job_id ID des zu importierenden Auftrags
   \throw EDLSJob Fehler während des Importierens
*/

void DLSJob::import(unsigned int job_id)
{
    try
    {
        _preset.import(_dls_dir, job_id);
    }
    catch (ECOMJobPreset &e)
    {
        throw EDLSJob("Importing job preset: " + e.msg);
    }
}

/*****************************************************************************/

/**
   Startet die Datenerfassung

   Erstellt ein Logger-Objekt für jeden Kanal, der erfasst werden soll.
*/

void DLSJob::start_logging()
{
    _logging_started = true;
    _sync_loggers(slQuiet);
}

/*****************************************************************************/

/**
   Übernimmt Änderungen der Vorgaben für die Datenerfassung
*/

void DLSJob::change_logging()
{
    if (_logging_started) {
		_sync_loggers(slVerbose);
	}
}

/*****************************************************************************/

/**
   Hält die Datenerfassung an

   Entfernt alle Logger-Objekte.
*/

void DLSJob::stop_logging()
{
    list<DLSLogger *>::iterator logger_i;

    _logging_started = false;

    logger_i = _loggers.begin();
    while (logger_i != _loggers.end()) {
        _stop_logger(*logger_i);
        logger_i++;
    }

    _clear_loggers();
}

/*****************************************************************************/

/**
   Synchronisiert die Liste der Logger-Objekte mit den Vorgaben

   Überprüft alle Kanäle in den aktuellen Vorgaben. Wenn für einen
   Kanal keinen Logger gibt, wird er erstellt. Wenn sich die Vorgaben
   für einen existierenden Logger geändert haben, wird dieser
   geändert. Gibt es noch Logger für Kanäle, die nicht mehr erfasst
   werden sollen, werden diese entsprechend entfernt.

   \param
*/

void DLSJob::_sync_loggers(SyncLoggerMode mode)
{
    vector<COMChannelPreset>::const_iterator channel_i;
    list<DLSLogger *>::iterator logger_i, del_i;
    unsigned int add_count = 0, chg_count = 0, rem_count = 0;

    if (!_logging_started) {
        return;
    }

    // add new loggers / delete existing loggers
    for (channel_i = _preset.channels()->begin();
            channel_i != _preset.channels()->end();
            channel_i++) {
        DLSLogger *logger = _logger_exists_for_channel(channel_i->name);
        if (!logger) {
            if (mode == slVerbose) {
                msg() << "ADD \"" << channel_i->name << "\"";
                log(DLSInfo);
            }

            if (_add_logger(&(*channel_i))) {
                add_count++;
			}
        }
        else if (*channel_i != *logger->channel_preset()) {
            if (mode == slVerbose) {
                msg() << "CHANGE \"" << channel_i->name << "\"";
                log(DLSInfo);
            }

            _loggers.remove(logger);
			_stop_logger(logger);
            delete logger;

			if (_add_logger(&(*channel_i))) {
				chg_count++;
			}
		}
	}

    // search for logger to remove
    logger_i = _loggers.begin();
    while (logger_i != _loggers.end()) {
        if (_preset.channel_exists((*logger_i)->channel_preset()->name)) {
            logger_i++;
            continue;
        }

        if (mode == slVerbose) {
            msg() << "REM \"" << (*logger_i)->channel_preset()->name
                << "\"";
            log(DLSInfo);
        }

        _stop_logger(*logger_i);
        rem_count++;

#ifdef DEBUG
        msg() << "_stop_logger() finished.";
        log(DLSDebug);
#endif

        delete *logger_i;
        del_i = logger_i;
        logger_i++;
        _loggers.erase(del_i);

#ifdef DEBUG
        msg() << "logger_i deleted.";
        log(DLSDebug);
#endif
    }

    if (add_count) {
        msg() << "ADDED " << add_count << " channels";
        log(DLSInfo);
    }

    if (chg_count) {
        msg() << "CHANGED " << chg_count << " channels";
        log(DLSInfo);
    }

    if (rem_count) {
        msg() << "REMOVED " << rem_count << " channels";
        log(DLSInfo);
    }

    if (!add_count && !chg_count && !rem_count) {
        msg() << "SYNC: It was nothing to do!";
        log(DLSInfo);
    }
}

/*****************************************************************************/

/**
   Fügt einen Logger für einen Kanal hinzu

   Ein Logger wird für den angegebenen Kanal erstellt. Dann werden
   Informationen über den msrd-Kanal geholt, um damit die Vorgaben
   zu verifizieren. Wenn diese in Ordnung sind, wird das
   Start-Kommando gesendet und der Logger der Liste angehängt.

   \param preset Kanalvorgaben für den neuen Logger
*/

bool DLSJob::_add_logger(const COMChannelPreset *preset)
{
	PdCom::Variable *pv = _parent_proc->findVariable(preset->name);

	if (!pv) {
		msg() << "Channel \"" << preset->name << "\" does not exist!";
		log(DLSError);
		return false;
	}

    DLSLogger *logger;

    try {
        logger = new DLSLogger(this, preset, _dls_dir, pv);
    }
    catch (EDLSLogger &e)
    {
        msg() << "Channel \"" << preset->name << "\": " << e.msg;
        log(DLSError);

        return false;
    }

    // Alles ok, Logger in die Liste aufnehmen
    _loggers.push_back(logger);
    return true;
}

/*****************************************************************************/

/**
   Entfernt einen Logger aus der Liste

   Sendet das Stop-Kommando, so dass keine neuen Daten mehr für
   diesen Kanal kommen. Dann wird der Logger angewieden, seine
   wartenden Daten zu sichern.
   Der Aufruf von delete erfolgt in sync_loggers().

   \param logger Zeiger auf den zu entfernenden Logger
   \see sync_loggers()
*/

void DLSJob::_stop_logger(DLSLogger *logger)
{
#ifdef DEBUG
    msg() << "Stopping logger...";
    log(DLSDebug);
#endif

    try {
        logger->finish();
    }
    catch (EDLSLogger &e) {
        msg() << "Finishing channel \"";
        msg() << logger->channel_preset()->name << "\": " << e.msg;
        log(DLSError);
    }

#ifdef DEBUG
    msg() << "Logger stopped.";
    log(DLSDebug);
#endif
}

/*****************************************************************************/

/** Löscht alle Daten, die noch im Speicher sind
 */
void DLSJob::discard()
{
    list<DLSLogger *>::iterator logger_i;

    // Message-Chunk beenden
    _msg_chunk_created = false;

    logger_i = _loggers.begin();
    while (logger_i != _loggers.end()) {
        (*logger_i)->discard();
        logger_i++;
    }
}

/*****************************************************************************/

/** Notifies the parent process about an error.
 */
void DLSJob::notify_error(int code)
{
    _parent_proc->notify_error(code);
}

/*****************************************************************************/

/** Notifies the parent process about received data.
 */
void DLSJob::notify_data()
{
    _parent_proc->notify_data();
}

/*****************************************************************************/

/**
   Errechnet die Größe aller aktuell erfassenden Chunks

   \return Größe in Bytes
*/

uint64_t DLSJob::data_size() const
{
    list<DLSLogger *>::const_iterator logger_i;
    uint64_t size = 0;

    logger_i = _loggers.begin();
    while (logger_i != _loggers.end())
    {

#ifdef DEBUG_SIZES
        msg() << "Logger for channel " << (*logger_i)->real_channel()->index;
        msg() << " " << (*logger_i)->data_size() << " bytes.";
        log(DLSInfo);
#endif

        size += (*logger_i)->data_size();
        logger_i++;
    }

    return size;
}

/*****************************************************************************/

/**
   Entfernt alle Logger aus der Liste

   Diese Methode ruft ein "delete" für jeden Logger auf, auch wenn
   ein Fehler passiert. Das vermeidet Speicherlecks. Fehler
   werden am Schluss gesammelt ausgegeben.
*/

void DLSJob::_clear_loggers()
{
    list<DLSLogger *>::iterator logger;
    stringstream err;

    logger = _loggers.begin();
    while (logger != _loggers.end())
    {
        delete *logger;
        logger++;
    }

    _loggers.clear();
}

/*****************************************************************************/

/**
   Prüft, ob ein Logger für einen bestimmten Kanal existiert

   \param name Kanalname
   \return Zeiger auf gefundenen Logger, oder 0
*/

DLSLogger *DLSJob::_logger_exists_for_channel(const string &name)
{
    list<DLSLogger *>::const_iterator logger = _loggers.begin();

    while (logger != _loggers.end()) {
        if ((*logger)->channel_preset()->name == name) {
            return *logger;
        }
        logger++;
    }

    return 0;
}

/*****************************************************************************/

/**
   Speichert alle wartenden Daten

   Weist alle Logger an, ihre Daten zu speichern. Wenn dies
   fehlerfrei geschehen ist, führt ein "delete" des Job-Objektes
   nicht zu Datenverlust.

   \throw EDLSJob Ein oder mehrere Logger konnten ihre
   Daten nicht speichern - Datenverlust!
*/

void DLSJob::finish()
{
    list<DLSLogger *>::iterator logger;
    stringstream err;
    bool errors = false;

    msg() << "Finishing job...";
    log(DLSInfo);

    // Message-Chunk beenden
    _msg_chunk_created = false;

    // Alle Logger beenden
    for (logger = _loggers.begin();
            logger != _loggers.end();
            logger++) {
        try {
            (*logger)->finish();
        }
        catch (EDLSLogger &e) {
            errors = true;
            if (err.str().length()) err << "; ";
            err << e.msg;
        }
    }

    if (errors) {
        throw EDLSJob("Logger::finish(): " + err.str());
    }

    msg() << "Job finished without errors.";
    log(DLSInfo);
}

/*****************************************************************************/

/**
   Speichert eine den Auftrag betreffende Nachricht

   \param info_tag Info-Tag
*/

void DLSJob::message(COMTime time, const string &type, const string &message)
{
    stringstream filename, dirname;
    COMMessageIndexRecord index_record;
    struct stat stat_buf;

#ifdef DEBUG
    msg() << "Message! Time: " << time;
    log(DLSDebug);
#endif

    if (!_msg_chunk_created) {
#ifdef DEBUG
        msg() << "Creating new message chunk.";
        log(DLSDebug);
#endif

        _message_file.close();
        _message_index.close();

        dirname << _dls_dir << "/job" << _preset.id() << "/messages";

        // Existiert das Message-Verzeichnis?
        if (stat(dirname.str().c_str(), &stat_buf) == -1) {
            // Messages-Verzeichnis anlegen
            if (mkdir(dirname.str().c_str(), 0755) == -1) {
                msg() << "Could not create message directory: ";
                msg() << " \"" << dirname.str() << "\"!";
                log(DLSError);
                return;
            }
        }

        dirname << "/chunk" << time;

        if (mkdir(dirname.str().c_str(), 0755) != 0) {
            msg() << "Could not create message chunk directory: ";
            msg() << " \"" << dirname.str() << "\"!";
            log(DLSError);
            return;
        }

        _msg_chunk_created = true;
        _msg_chunk_dir = dirname.str();
    }

    if (!_message_file.open() || !_message_index.open()) {
        filename << _msg_chunk_dir << "/messages";

        try {
            _message_file.open_read_append(filename.str().c_str());
            _message_index.open_read_append(
                    (filename.str() + ".idx").c_str());
        }
        catch (ECOMFile &e) {
            msg() << "Failed to open message file:" << e.msg;
            log(DLSError);
            return;
        }
        catch (ECOMIndexT &e) {
            msg() << "Failed to open message index: " << e.msg;
            log(DLSError);
            return;
        }
    }

    // Aktuelle Zeit und Dateiposition als Einsprungspunkt merken
    index_record.time = time.to_uint64();
    index_record.position = _message_file.calc_size();

    stringstream tag;

    tag << "<" << type << " time=\"" << fixed << time.to_dbl_time()
        << "\" text=\"" << message << "\"/>" << endl;

    try {
        _message_file.append(tag.str().c_str(), tag.str().size());
        _message_index.append_record(&index_record);
    }
    catch (ECOMFile &e) {
        msg() << "Could not write message file: " << e.msg;
        log(DLSError);
        return;
    }
    catch (ECOMIndexT &e) {
        msg() << "Could not write message index: " << e.msg;
        log(DLSError);
        return;
    }
}

/*****************************************************************************/
