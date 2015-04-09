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

#ifndef LoggerH
#define LoggerH

/*****************************************************************************/

#include <string>
using namespace std;

/*****************************************************************************/

#include "lib/LibDLS/Exception.h"
#include "lib/LibDLS/ChannelPreset.h"

/*****************************************************************************/

class Job; // Nötig, da gegenseitige Referenzierung
class SaverGen;

/*****************************************************************************/

/**
   Allgemeine Exception eines Logger-Objektes
*/

class ELogger: public LibDLS::Exception
{
public:
    ELogger(string pmsg): Exception(pmsg) {};
};

/*****************************************************************************/

/**
   Speichert Daten für einen Kanal entsprechend einer Vorgabe.

   Verwaltet selbständig Chunk-Verzeichnisse und kann Online-
   Änderungen in den Kanalvorgaben verarbeiten. Ein Logger
   ist das prozessseitige Äquivalent zu einem Chunk.
   Die Größe der erzeugten Daten wird hier ebenfalls gespeichert.
   Für das eigentliche Speichern der Daten wird ein
   SaverGen - Objekt vorgehalten.
*/

class Logger
{
public:

    Logger(const Job *, const LibDLS::ChannelPreset *, const string &);
    ~Logger();

    //@{
    void get_real_channel(const list<LibDLS::RealChannel> *);
    void check_presettings(const LibDLS::ChannelPreset * = 0) const;
    void create_gen_saver();
    void process_data(const string &, LibDLS::Time);
    uint64_t data_size() const;
    void finish();
    void discard_chunk();
    //@}

    //@{
    const LibDLS::ChannelPreset *channel_preset() const;
    const LibDLS::RealChannel *real_channel() const;
    //@}

    //@{
    string start_tag(const LibDLS::ChannelPreset *, const string & = "") const;
    string stop_tag() const;
    //@}

    //@{
    void set_change(const LibDLS::ChannelPreset *, const string &);
    bool change_is(const string &) const;
    void do_change();
    //@}

    //@{
    bool chunk_created() const;
    void create_chunk(LibDLS::Time);
    const string &chunk_dir_name() const;
    //@}

    void bytes_written(unsigned int);

private:
    const Job *_parent_job; /**< Zeiger auf das besitzende Auftragsobjekt */
    string _dls_dir;           /**< DLS-Datenverzeichnis */

    //@{
	LibDLS::ChannelPreset _channel_preset; /**< Aktuelle Kanalvorgaben */
	LibDLS::RealChannel _real_channel;     /**< Informationen über den msrd-Kanal */
    //@}

    //@{
    SaverGen *_gen_saver; /**< Zeiger auf das Objekt zur Speicherung
                                der generischen Daten */
    uint64_t _data_size;    /**< Größe der bisher erzeugten Daten */
    //@}

    //@{
    bool _channel_dir_acquired; /**< channel directory already acquired */
    string _channel_dir_name; /**< name of the channel directory */
    bool _chunk_created; /**< the current chunk directory was created */
    string _chunk_dir_name; /**< name of the current chunk directory */
    //@}

    //@{
    bool _change_in_progress; /**< Wartet eine Vorgabenänderung
                                 auf Bestätigung? */
    string _change_id; /**< ID des Änderungsbefehls, auf dessen
                          Bestätigung gewartet wird */
	LibDLS::ChannelPreset _change_channel; /**< Neue Kanalvorgaben, die nach der
                                         Bestätigung aktiv werden */
    //@}

    bool _finished; /**< Keine Daten mehr im Speicher -
                       kein Datenverlust bei "delete"  */

    void _acquire_channel_dir();
    int _channel_dir_matches(const string &) const;
};

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf die aktuellen Kanalvorgaben

   \return Konstanter Zeiger auf die Kanalvorgaben
*/

inline const LibDLS::ChannelPreset *Logger::channel_preset() const
{
    return &_channel_preset;
}

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf die Eigenschaften des
   zu Grunde liegenden MSR-Kanals

   \return Konstanter Zeiger auf die Kanalvorgaben
*/

inline const LibDLS::RealChannel *Logger::real_channel() const
{
    return &_real_channel;
}

/*****************************************************************************/

/**
   Prüft, ob ein aktuelles Chunk-Verzeichnis erstellt wurde

   Wenn ja, gibt chunk_dir_name() den Pfad zurück.

   \return true, wenn es ein aktuelles Chunk-Verzeichnis gibt
*/

inline bool Logger::chunk_created() const
{
    return _chunk_created;
}

/*****************************************************************************/

/**
   Ermöglicht Auslesen des aktuellen Chunk-Verzeichnisses

   \return Pfad des Chunk-Verzeichnisses
*/

inline const string &Logger::chunk_dir_name() const
{
    return _chunk_dir_name;
}

/*****************************************************************************/

/**
   Teilt dem Logger mit, dass Daten gespeichert wurden

   Dient dem Logger dazu, die Größe der bisher gespeicherten
   Daten mitzuführen und wird von den tieferliegenden
   SaverT-Derivaten aufgerufen.
*/

inline void Logger::bytes_written(unsigned int bytes)
{
    _data_size += bytes;
}

/*****************************************************************************/

/**
   Gibt die Größe des Chunks in Bytes zurück
*/

inline uint64_t Logger::data_size() const
{
    return _data_size;
}

/*****************************************************************************/

#endif
