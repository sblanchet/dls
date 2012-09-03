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

#ifndef DLSJobHpp
#define DLSJobHpp

/*****************************************************************************/

#include <list>
using namespace std;

/*****************************************************************************/

#include "com_exception.hpp"
#include "com_time.hpp"
#include "com_job_preset.hpp"
#include "com_file.hpp"
#include "com_index_t.hpp"
#include "dls_globals.hpp"
#include "dls_logger.hpp"

/*****************************************************************************/

class DLSProcLogger; // Nötig, da gegenseitige Referenzierung

/*****************************************************************************/

enum SyncLoggerMode {slQuiet, slVerbose};

/*****************************************************************************/

/**
   Exception eines Auftrags-Objektes
*/

class EDLSJob : public COMException
{
public:
    EDLSJob(string pmsg) : COMException(pmsg) {};
};

/*****************************************************************************/

/**
   Das Arbeitsobjekt eines Logging-Prozesses

   Enthält Auftragsvorgaben und stellt Methoden zur
   Steuerung und durchführung der Datenerfassung bereit.
   Übernimmt ausserdem die Message-Behandlung.
*/

class DLSJob
{
public:
    DLSJob(DLSProcLogger *, const string &);
    ~DLSJob();

    void import(unsigned int);

    //@{
    void start_logging();
    void change_logging();
    void stop_logging();
    //@}

    //@{
    void process_data(COMTime, int, const string &);
    uint64_t data_size() const;
    //@}

    void ack_received(const string &);
    void message(const COMXMLTag *);
    void finish();
    void discard_data();

    const COMJobPreset *preset() const;

private:
    DLSProcLogger *_parent_proc; /**< Zeiger auf den besitzenden
                                    Logging-Prozess */
    string _dls_dir; /**< DLS-Datenverzeichnis */
    COMJobPreset _preset; /**< Auftragsvorgaben */
    list<DLSLogger *> _loggers; /**< Zeigerliste aller aktiven Logger */
    unsigned int _id_gen; /**< Sequenz für die ID-Generierung */
    bool _logging_started; /**< Logging gestartet? */

    //@{
    COMFile _message_file; /**< Dateiobjekt für Messages */
    COMIndexT<COMMessageIndexRecord> _message_index; /**< Index für Messages */
    bool _msg_chunk_created; /**< true, wenn es einen aktuellen
                                Message-Chunk gibt. */
    string _msg_chunk_dir; /**< Pfad des aktuellen Message-
                              Chunks-Verzeichnisses */
    //@}

    void _clear_loggers();
    void _sync_loggers(SyncLoggerMode);
    bool _add_logger(const COMChannelPreset *);
    bool _change_logger(DLSLogger *, const COMChannelPreset *);
    void _remove_logger(DLSLogger *);
    DLSLogger *_logger_exists_for_channel(const string &);
    string _generate_id();
};

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf die Auftragsvorgaben

   \return Konstanter Zeiger auf aktuelle Auftragsvorgaben
*/

inline const COMJobPreset *DLSJob::preset() const
{
    return &_preset;
}

/*****************************************************************************/

#endif


