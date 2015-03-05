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

#include "globals.h"
#include "ProcLogger.h"
#include "Job.h"
using namespace LibDLS;

/*****************************************************************************/

//#define DEBUG
//#define DEBUG_SIZES

/*****************************************************************************/

/**
   Konstruktor

   \param parent_proc Zeiger auf den besitzenden Logging-Prozess
   \param dls_dir DLS-Datenverzeichnis
*/

Job::Job(ProcLogger *parent_proc, const string &dls_dir)
{
    stringstream dir;

    _parent_proc = parent_proc;
    _dls_dir = dls_dir;

    _id_gen = 0;
    _logging_started = false;
    _msg_chunk_created = false;
}

/*****************************************************************************/

/**
   Destruktor
*/

Job::~Job()
{
    _clear_loggers();
}

/*****************************************************************************/

/**
   Importiert die Vorgaben für den aktuellen Auftrag

   \param job_id ID des zu importierenden Auftrags
   \throw EJob Fehler während des Importierens
*/

void Job::import(unsigned int job_id)
{
    try
    {
        _preset.import(_dls_dir, job_id);
    }
    catch (EJobPreset &e)
    {
        throw EJob("Importing job preset: " + e.msg);
    }
}

/*****************************************************************************/

/**
   Startet die Datenerfassung

   Erstellt ein Logger-Objekt für jeden Kanal, der erfasst werden soll.
*/

void Job::start_logging()
{
    _logging_started = true;
    _sync_loggers(slQuiet);
}

/*****************************************************************************/

/**
   Übernimmt Änderungen der Vorgaben für die Datenerfassung
*/

void Job::change_logging()
{
    if (_logging_started) _sync_loggers(slVerbose);
}

/*****************************************************************************/

/**
   Hält die Datenerfassung an

   Entfernt alle Logger-Objekte.
*/

void Job::stop_logging()
{
    list<Logger *>::iterator logger_i;

    _logging_started = false;

    logger_i = _loggers.begin();
    while (logger_i != _loggers.end())
    {
        _remove_logger(*logger_i);
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

void Job::_sync_loggers(SyncLoggerMode mode)
{
    vector<ChannelPreset>::const_iterator channel_i;
    list<Logger *>::iterator logger_i, del_i;
    Logger *found_logger;
    unsigned int add_count = 0, chg_count = 0, rem_count = 0;

    if (!_logging_started) return;

    // add new loggers / delete existing loggers
    for (channel_i = _preset.channels()->begin();
            channel_i != _preset.channels()->end();
            channel_i++) {
        if (!(found_logger = _logger_exists_for_channel(channel_i->name))) {
            if (mode == slVerbose) {
                msg() << "ADD \"" << channel_i->name << "\"";
                log(Info);
            }

            if (_add_logger(&(*channel_i)))
                add_count++;
        }
        else if (*channel_i != *found_logger->channel_preset()) {
            if (mode == slVerbose) {
                msg() << "CHANGE \"" << channel_i->name << "\"";
                log(Info);
            }

            if (_change_logger(found_logger, &(*channel_i)))
                chg_count++;
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
            log(Info);
        }

        _remove_logger(*logger_i);
        rem_count++;

#ifdef DEBUG
        msg() << "_remove_logger() finished.";
        log(Debug);
#endif

        delete *logger_i;
        del_i = logger_i;
        logger_i++;
        _loggers.erase(del_i);

#ifdef DEBUG
        msg() << "logger_i deleted.";
        log(Debug);
#endif
    }

    if (add_count) {
        msg() << "ADDED " << add_count << " channels";
        log(Info);
    }

    if (chg_count) {
        msg() << "CHANGED " << chg_count << " channels";
        log(Info);
    }

    if (rem_count) {
        msg() << "REMOVED " << rem_count << " channels";
        log(Info);
    }

    if (!add_count && !chg_count && !rem_count) {
        msg() << "SYNC: It was nothing to do!";
        log(Info);
    }
}

/*****************************************************************************/

/**
   Fügt einen Logger für einen Kanal hinzu

   Ein Logger wird für den angegebenen Kanal erstellt. Dann werden
   Informationen über den msrd-Kanal geholt, um damit die Vorgaben
   zu verifizieren. Wenn diese in Ordnung sind, wird das
   Start-Kommando gesendet und der Logger der Liste angehängt.

   \param channel Kanalvorgaben für den neuen Logger
*/

bool Job::_add_logger(const ChannelPreset *channel)
{
    Logger *new_logger = new Logger(this, channel, _dls_dir);

    try
    {
        // Kanalparameter holen
        new_logger->get_real_channel(_parent_proc->real_channels());

        // Kanalvorgaben auf Gültigkeit prüfen
        new_logger->check_presettings();

        // Saver-Objekt erstellen
        new_logger->create_gen_saver();

        // Startkommando senden
        _parent_proc->send_command(new_logger->start_tag(channel));

        // Alles ok, Logger in die Liste aufnehmen
        _loggers.push_back(new_logger);
    }
    catch (ELogger &e)
    {
        delete new_logger;

        msg() << "Channel \"" << channel->name << "\": " << e.msg;
        log(Error);

        return false;
    }

    return true;
}

/*****************************************************************************/

/**
   Ändert die Vorgaben für einen bestehenden Logger

   Zuerst werden die neuen Vorgaben verifiziert. Wenn alles in
   Ordnung ist, wird eine eindeutige ID generiert, die dann dem
   erneuten Startkommando beigefügt wird. Die Änderung wird
   vorgemerkt.
   Wenn dan später die Bestätigung des msrd kommt, wird die
   Änderung aktiv.

   \param logger Der zu ändernde Logger
   \param channel Die neuen Kanalvorgaben
*/

bool Job::_change_logger(Logger *logger,
                            const ChannelPreset *channel)
{
    string id;

    try
    {
        // Neue Vorgaben prüfen
        logger->check_presettings(channel);

        // ID generieren
        id = _generate_id();

        // Änderungskommando senden
        _parent_proc->send_command(logger->start_tag(channel, id));

        // Wartende Änderung vermerken
        logger->set_change(channel, id);
    }
    catch (ELogger &e)
    {
        msg() << "Channel \"" << channel->name << "\": " << e.msg;
        log(Error);

        return false;
    }

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

void Job::_remove_logger(Logger *logger)
{
#ifdef DEBUG
    msg() << "Removing logger...";
    log(Debug);
#endif

    _parent_proc->send_command(logger->stop_tag());

    try
    {
        logger->finish();
    }
    catch (ELogger &e)
    {
        msg() << "Finishing channel \"";
        msg() << logger->channel_preset()->name << "\": " << e.msg;
        log(Error);
    }

#ifdef DEBUG
    msg() << "Logger removed.";
    log(Debug);
#endif
}

/*****************************************************************************/

/**
   Führt eine vorgemerkte Änderung durch

   Diese Methode muss vom Logging-Prozess aufgerufen werden,
   sobald eine Kommandobestätigung a la <ack> eintrifft.
   Dann wird überprüft, ob einer der Logger ein dazu passendes
   Kommando gesendet hat. Wenn ja, wird die entsprechende Änderung
   durchgeführt.

   \param id Bestätigungs-ID aus dem <ack>-Tag
*/

void Job::ack_received(const string &id)
{
    list<Logger *>::iterator logger_i;

    msg() << "Acknowledge received: \"" << id << "\"";
    log(Info);

    logger_i = _loggers.begin();
    while (logger_i != _loggers.end())
    {
        if ((*logger_i)->change_is(id))
        {
            (*logger_i)->do_change();
            return;
        }
        logger_i++;
    }

    msg() << "Change ID \"" << id << "\" does not exist!";
    log(Warning);
}

/*****************************************************************************/

/**
   Verarbeitet empfangene Daten

   Diese Methode nimmt empfangene Daten für einen bestimmten
   Kanal auf, und gibt sie intern an den dafür zuständigen Logger
   weiter.

   \todo Effizientere Suche

   \param time_of_last die Zeit des letzten Datenwertes im Block
   \param channel_index Kanalindex aus dem <F>-Tag
   \param data Empfangene Daten
   \throw EJob Fehler während der Datenverarbeitung
   \throw ETimeTolerance Zeittoleranzfehler!
*/

void Job::process_data(Time time_of_last,
                          int channel_index,
                          const string &data)
{
    list<Logger *>::iterator logger_i;

    for (logger_i = _loggers.begin();
            logger_i != _loggers.end();
            logger_i++) {
        if ((*logger_i)->real_channel()->index != channel_index)
            continue;

        try {
            (*logger_i)->process_data(data, time_of_last);
        }
        catch (ELogger &e) {
            throw EJob("Logger: " + e.msg);
        }

        return;
    }

    msg() << "Channel " << channel_index << " not required!";
    log(Warning);
}

/*****************************************************************************/

/**
   Löscht alle Daten, die noch im Speicher sind
*/

void Job::discard_data()
{
    list<Logger *>::iterator logger_i;

    // Message-Chunk beenden
    _msg_chunk_created = false;

    logger_i = _loggers.begin();
    while (logger_i != _loggers.end())
    {
        (*logger_i)->discard_chunk();
        logger_i++;
    }
}

/*****************************************************************************/

/**
   Errechnet die Größe aller aktuell erfassenden Chunks

   \return Größe in Bytes
*/

uint64_t Job::data_size() const
{
    list<Logger *>::const_iterator logger_i;
    uint64_t size = 0;

    logger_i = _loggers.begin();
    while (logger_i != _loggers.end())
    {

#ifdef DEBUG_SIZES
        msg() << "Logger for channel " << (*logger_i)->real_channel()->index;
        msg() << " " << (*logger_i)->data_size() << " bytes.";
        log(Info);
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

   \throw EJob Fehler beim Löschen ein oder mehrerer Logger
*/

void Job::_clear_loggers()
{
    list<Logger *>::iterator logger;
    stringstream err;
    bool error = false;

    logger = _loggers.begin();
    while (logger != _loggers.end())
    {
        delete *logger;
        logger++;
    }

    _loggers.clear();

    if (error)
    {
        throw EJob(err.str());
    }
}

/*****************************************************************************/

/**
   Prüft, ob ein Logger für einen bestimmten Kanal existiert

   \param name Kanalname
   \return Zeiger auf gefundenen Logger, oder 0
*/

Logger *Job::_logger_exists_for_channel(const string &name)
{
    list<Logger *>::const_iterator logger = _loggers.begin();

    while (logger != _loggers.end())
    {
        if ((*logger)->channel_preset()->name == name) return *logger;
        logger++;
    }

    return 0;
}

/*****************************************************************************/

/**
   Generiert eine eindeutige Kommando-ID

   Die ID wird aus der Adresse des Auftrags-Objektes und einer
   Sequenz-Nummer zusammengesetzt.

   \return Generierte ID
*/

string Job::_generate_id()
{
    stringstream id;

    // ID aus Adresse und Counter erzeugen
    id << (unsigned long) this << "_" << ++_id_gen;

    return id.str();
}

/*****************************************************************************/

/**
   Speichert alle wartenden Daten

   Weist alle Logger an, ihre Daten zu speichern. Wenn dies
   fehlerfrei geschehen ist, führt ein "delete" des Job-Objektes
   nicht zu Datenverlust.

   \throw EJob Ein oder mehrere Logger konnten ihre
   Daten nicht speichern - Datenverlust!
*/

void Job::finish()
{
    list<Logger *>::iterator logger;
    stringstream err;
    bool errors = false;

    msg() << "Finishing job...";
    log(Info);

    // Message-Chunk beenden
    _msg_chunk_created = false;

    // Alle Logger beenden
    for (logger = _loggers.begin();
            logger != _loggers.end();
            logger++) {
        try {
            (*logger)->finish();
        }
        catch (ELogger &e) {
            errors = true;
            if (err.str().length()) err << "; ";
            err << e.msg;
        }
    }

    if (errors) {
        throw EJob("Logger::finish(): " + err.str());
    }

    msg() << "Job finished without errors.";
    log(Info);
}

/*****************************************************************************/

/**
   Speichert eine den Auftrag betreffende Nachricht

   \param info_tag Info-Tag
*/

void Job::message(const XmlTag *info_tag)
{
    stringstream filename, dirname;
    MessageIndexRecord index_record;
    Time time;
    string tag;
    struct stat stat_buf;

    try
    {
        // Zeit der Nachricht ermitteln
        time.from_dbl_time(info_tag->att("time")->to_dbl());
    }
    catch (EXmlTag &e)
    {
        msg() << "Could not get message time. Tag: \"" << info_tag
              << "\": " << e.msg;
        log(Error);
        return;
    }

#ifdef DEBUG
    msg() << "Message! Time: " << time;
    log(Debug);
#endif

    if (!_msg_chunk_created)
    {
#ifdef DEBUG
        msg() << "Creating new message chunk.";
        log(Debug);
#endif

        _message_file.close();
        _message_index.close();

        dirname << _dls_dir << "/job" << _preset.id() << "/messages";

        // Existiert das Message-Verzeichnis?
        if (stat(dirname.str().c_str(), &stat_buf) == -1)
        {
            // Messages-Verzeichnis anlegen
            if (mkdir(dirname.str().c_str(), 0755) == -1)
            {
                msg() << "Could not create message directory: ";
                msg() << " \"" << dirname.str() << "\"!";
                log(Error);
                return;
            }
        }

        dirname << "/chunk" << time;

        if (mkdir(dirname.str().c_str(), 0755) != 0)
        {
            msg() << "Could not create message chunk directory: ";
            msg() << " \"" << dirname.str() << "\"!";
            log(Error);
            return;
        }

        _msg_chunk_created = true;
        _msg_chunk_dir = dirname.str();
    }

    if (!_message_file.open() || !_message_index.open())
    {
        filename << _msg_chunk_dir << "/messages";

        try
        {
            _message_file.open_read_append(filename.str().c_str());
            _message_index.open_read_append((filename.str() + ".idx").c_str());
        }
        catch (EFile &e)
        {
            msg() << "Could not open message file for message \"" << info_tag
                  << "\": " << e.msg;
            log(Error);
            return;
        }
        catch (EIndexT &e)
        {
            msg() << "Could not open message index for message \"" << info_tag
                  << "\": " << e.msg;
            log(Error);
            return;
        }
    }

    // Aktuelle Zeit und Dateiposition als Einsprungspunkt merken
    index_record.time = time.to_uint64();
    index_record.position = _message_file.calc_size();

    tag = info_tag->tag() + "\n";

    try
    {
        _message_file.append(tag.c_str(), tag.length());
        _message_index.append_record(&index_record);
    }
    catch (EFile &e)
    {
        msg() << "Could not write file for message \"" << info_tag
              << "\": " << e.msg;
        log(Error);
        return;
    }
    catch (EIndexT &e)
    {
        msg() << "Could not write index for message \"" << info_tag
              << "\": " << e.msg;
        log(Error);
        return;
    }
}

/*****************************************************************************/
