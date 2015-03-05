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

#ifndef JobH
#define JobH

/*****************************************************************************/

#include <list>
using namespace std;

/*****************************************************************************/

#include "lib/Exception.h"
#include "lib/Time.h"
#include "lib/File.h"
#include "lib/IndexT.h"

#include "globals.h"
#include "Logger.h"
#include "JobPreset.h"

/*****************************************************************************/

class ProcLogger; // N�tig, da gegenseitige Referenzierung

/*****************************************************************************/

enum SyncLoggerMode {slQuiet, slVerbose};

/*****************************************************************************/

/**
   Exception eines Auftrags-Objektes
*/

class EJob: public LibDLS::Exception
{
public:
    EJob(string pmsg): Exception(pmsg) {};
};

/*****************************************************************************/

/**
   Das Arbeitsobjekt eines Logging-Prozesses

   Enth�lt Auftragsvorgaben und stellt Methoden zur
   Steuerung und durchf�hrung der Datenerfassung bereit.
   �bernimmt ausserdem die Message-Behandlung.
*/

class Job
{
public:
    Job(ProcLogger *, const string &);
    ~Job();

    void import(unsigned int);

    //@{
    void start_logging();
    void change_logging();
    void stop_logging();
    //@}

    //@{
    uint64_t data_size() const;
    //@}

    void message(LibDLS::Time, const std::string &, const std::string &);

    void finish();
    void discard();

    const JobPreset *preset() const;

    void notify_error(int);
    void notify_data();

private:
    ProcLogger * const _parent_proc; /**< Zeiger auf den besitzenden
                                    Logging-Prozess */
    string _dls_dir; /**< DLS-Datenverzeichnis */
    JobPreset _preset; /**< Auftragsvorgaben */
    list<Logger *> _loggers; /**< Zeigerliste aller aktiven Logger */
    unsigned int _id_gen; /**< Sequenz f�r die ID-Generierung */
    bool _logging_started; /**< Logging gestartet? */

    //@{
    LibDLS::File _message_file; /**< Dateiobjekt f�r Messages */
    LibDLS::IndexT<LibDLS::MessageIndexRecord> _message_index; /**< Index f�r
                                                                 Messages */
    bool _msg_chunk_created; /**< true, wenn es einen aktuellen
                                Message-Chunk gibt. */
    string _msg_chunk_dir; /**< Pfad des aktuellen Message-
                              Chunks-Verzeichnisses */
    //@}

    void _clear_loggers();
    void _sync_loggers(SyncLoggerMode);
    bool _add_logger(const LibDLS::ChannelPreset *);
    void _stop_logger(Logger *);
    Logger *_logger_exists_for_channel(const string &);
};

/*****************************************************************************/

/**
   Erm�glicht Lesezugriff auf die Auftragsvorgaben

   \return Konstanter Zeiger auf aktuelle Auftragsvorgaben
*/

inline const JobPreset *Job::preset() const
{
    return &_preset;
}

/*****************************************************************************/

#endif


