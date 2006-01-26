//---------------------------------------------------------------
//
//  D L S _ J O B . C P P
//
//---------------------------------------------------------------

#include <sstream>
#include <fstream>
using namespace std;

#include "dls_globals.hpp"
#include "dls_proc_logger.hpp"
#include "dls_job.hpp"

//---------------------------------------------------------------

/**
   Konstruktor

   \param parent_proc Zeiger auf den besitzenden Logging-Prozess
   \param dls_dir DLS-Datenverzeichnis
   \param id Auftrags-ID
*/

DLSJob::DLSJob(DLSProcLogger *parent_proc, const string &dls_dir, int id)
{
  stringstream dir;

  _parent_proc = parent_proc;
  _dls_dir = dls_dir;
  _job_id = id;

  _id_gen = 0;
  _logging_started = false;
  _finished = true;
}

//---------------------------------------------------------------

/**
   Destruktor

   Gibt eine Warnung aus, wenn wartende Daten nicht gesichert wurden.
*/

DLSJob::~DLSJob()
{
  if (!_finished)
  {
    msg() << "job not finished!";
    log(DLSWarning);
  }

  _clear_loggers();
}

//---------------------------------------------------------------

/**
   Importiert die Vorgaben f�r den aktuellen Auftrag

   \throw EDLSJob Fehler w�hrend des Importierens
*/

void DLSJob::import()
{
  try
  {
    _preset.import(_dls_dir, _job_id);
  }
  catch (ECOMJobPreset &e)
  {
    throw EDLSJob("jobinfo::import(): " + e.msg);
  }
}

//---------------------------------------------------------------

/**
   Startet die Datenerfassung

   Erstellt ein Logger-Objekt f�r jeden Kanal, der erfasst werden soll.
 */

void DLSJob::start_logging()
{
  _logging_started = true;

  _sync_loggers(slQuiet);
}

//---------------------------------------------------------------

/**
   �bernimmt �nderungen der Vorgaben f�r die Datenerfassung
 */

void DLSJob::change_logging()
{
  if (_logging_started)
  {
    _sync_loggers(slVerbose);
  }
}

//---------------------------------------------------------------

/**
   H�lt die Datenerfassung an

   Entfernt alle Logger-Objekte.
*/

void DLSJob::stop_logging()
{
  list<DLSLogger *>::iterator logger_i;

  _logging_started = false;

  logger_i = _loggers.begin();
  while (logger_i != _loggers.end())
  {
    _remove_logger(*logger_i);
    logger_i++;
  }

  _clear_loggers();
}

//---------------------------------------------------------------

/**
   Synchronisiert die Liste der Logger-Objekte mit den Vorgaben

   �berpr�ft alle Kan�le in den aktuellen Vorgaben. Wenn f�r einen
   Kanal keinen Logger gibt, wird er erstellt. Wenn sich die Vorgaben
   f�r einen existierenden Logger ge�ndert haben, wird dieser
   ge�ndert. Gibt es noch Logger f�r Kan�le, die nicht mehr erfasst
   werden sollen, werden diese entsprechend entfernt.

   \param
*/

void DLSJob::_sync_loggers(SyncLoggerMode mode)
{
  vector<COMChannelPreset>::const_iterator channel_i;
  list<DLSLogger *>::iterator logger_i, del_i;
  DLSLogger *found_logger;
  unsigned int add_count = 0, chg_count = 0, rem_count = 0;

  if (!_logging_started) return;

  // Neue Kan�le hinzuf�gen / existierende Kan�le �ndern
  channel_i = _preset.channels()->begin();
  while (channel_i != _preset.channels()->end())
  {
    if ((found_logger = _logger_exists_for_channel(channel_i->name)) == 0)
    {
      if (mode == slVerbose)
      {
        msg() << "ADD \"" << channel_i->name << "\"";
        log(DLSInfo);
      }

      if (_add_logger(&(*channel_i)))
      {
        add_count++;
      }
    }
    else if (*channel_i != *found_logger->channel_preset())
    {
      if (mode == slVerbose)
      {
        msg() << "CHANGE \"" << channel_i->name << "\"";
        log(DLSInfo);
      }

      if (_change_logger(found_logger, &(*channel_i)))
      {
        chg_count++;
      }
    }

    channel_i++;
  }

  // Wegfallende Kan�le l�schen
  logger_i = _loggers.begin();
  while (logger_i != _loggers.end())
  {
    if (!_preset.channel_exists((*logger_i)->channel_preset()->name))
    {
      if (mode == slVerbose)
      {
        msg() << "REM \"" << (*logger_i)->channel_preset()->name << "\"";
        log(DLSInfo);
      }

      _remove_logger(*logger_i);
      rem_count++;

      delete *logger_i;
      del_i = logger_i;
      logger_i++;
      _loggers.erase(del_i);
    }
    else logger_i++;
  }

  if (add_count)
  {
    msg() << "ADDED " << add_count << " channels";
    log(DLSInfo);
  }
    
  if (chg_count)
  {
    msg() << "CHANGED " << chg_count << " channels";
    log(DLSInfo);
  }

  if (rem_count)
  {
    msg() << "REMOVED " << rem_count << " channels";
    log(DLSInfo);
  }

  if (!add_count && !chg_count && !rem_count)
  {
    msg() << "SYNC: it was nothing to do!";
    log(DLSInfo);
  }
}

//---------------------------------------------------------------

/**
   F�gt einen Logger f�r einen Kanal hinzu

   Ein Logger wird f�r den angegebenen Kanal erstellt. Dann werden
   Informationen �ber den msrd-Kanal geholt, um damit die Vorgaben
   zu verifizieren. Wenn diese in orgnung sind, wird das
   Start-Kommando gesendet und der Logger der Liste angeh�ngt.

   \param channel Kanalvorgaben f�r den neuen Logger
*/

bool DLSJob::_add_logger(const COMChannelPreset *channel)
{
  DLSLogger *new_logger = new DLSLogger(this, channel, _dls_dir);

  try
  {
    // Kanalparameter holen
    new_logger->get_real_channel(_parent_proc->real_channels());

    // Kanalvorgaben auf G�ltigkeit pr�fen
    if (!new_logger->presettings_valid())
    {
      delete new_logger;
      return false;
    }

    // Startkommando senden
    _parent_proc->send_command(new_logger->start_tag(channel));

    // Alles ok, Logger in die Liste aufnehmen
    _loggers.push_back(new_logger);
  }
  catch (EDLSLogger &e)
  {
    delete new_logger;

    msg() << "channel \"" << channel->name << "\": " << e.msg;
    log(DLSError);

    return false;
  }

  return true;
}

//---------------------------------------------------------------

/**
   �ndert die Vorgaben f�r einen bestehenden Logger

   Zuerst werden die neuen Vorgaben verifiziert. Wenn alles in
   Ordnung ist, wird eine eindeutige ID generiert, die dann dem
   erneuten Startkommando beigef�gt wird. Die �nderung wird
   vorgemerkt.
   Wenn dan sp�ter die Best�tigung des msrd kommt, wird die
   �nderung aktiv.

   \param logger Der zu �ndernde Logger
   \param channel Die neuen Kanalvorgaben
*/

bool DLSJob::_change_logger(DLSLogger *logger,
                            const COMChannelPreset *channel)
{
  string id;

  if (!logger->presettings_valid(channel)) return false;

  id = _generate_id();

  try
  {
    _parent_proc->send_command(logger->start_tag(channel, id));
    logger->set_change(channel, id);
  }
  catch (EDLSLogger &e)
  {
    msg() << "channel \"" << channel->name << "\": " << e.msg;
    log(DLSError);

    return false;
  }

  return true;
}

//---------------------------------------------------------------

/**
   Entfernt einen Logger aus der Liste

   Sendet das Stop-Kommando, so dass keine neuen Daten mehr f�r
   diesen Kanal kommen. Dann wird der Logger angewieden, seine
   wartenden Daten zu sichern.
   Der Aufruf von delete erfolgt in sync_loggers().

   \param logger Zeiger auf den zu entfernenden Logger
   \see sync_loggers()
*/

void DLSJob::_remove_logger(DLSLogger *logger)
{
  _parent_proc->send_command(logger->stop_tag());

  try
  {
    logger->finish();
  }
  catch (EDLSLogger &e)
  {
    msg() << "finish channel \"";
    msg() << logger->channel_preset()->name << "\": " << e.msg;
    log(DLSError);
  }
}

//---------------------------------------------------------------

/**
   F�hrt eine vorgemerkte �nderung durch

   Diese Methode muss vom Logging-Prozess aufgerufen werden,
   sobald eine Kommandobest�tigung a la <ack> eintrifft.
   Dann wird �berpr�ft, ob einer der Logger ein dazu passendes
   Kommando gesendet hat. Wenn ja, wird die entsprechende �nderung
   durchgef�hrt.

   \param id Best�tigungs-ID aus dem <ack>-Tag
*/

void DLSJob::ack_received(const string &id)
{
  list<DLSLogger *>::iterator logger_i;

  msg() << "ACK: \"" << id << "\"";
  log(DLSInfo);

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
  
  msg() << "change id \"" << id << "\" does not exist!";
  log(DLSWarning);
}

//---------------------------------------------------------------

/**
   Verarbeitet empfangene Daten

   Diese Methode nimmt empfangene Daten f�r einen bestimmten
   Kanal auf, und gibt sie intern an den daf�r zust�ndigen Logger
   weiter.

   \param time_of_last die Zeit des letzten Datenwertes im Block
   \param channel_index Kanalindex aus dem <F>-Tag
   \param data Empfangene Daten
   \throw EDLSJob Fehler w�hrend der Datenverarbeitung
   \throw EDLSJobTime Zeittoleranzfehler!
*/

void DLSJob::process_data(COMTime time_of_last,
                          int channel_index,
                          const string &data)
{
  list<DLSLogger *>::iterator logger_i;

  logger_i = _loggers.begin();
  while (logger_i != _loggers.end())
  {
    if ((*logger_i)->real_channel()->index() == channel_index)
    {
      try
      {
        (*logger_i)->process_data(data, time_of_last);
      }
      catch (EDLSLogger &e)
      {
        throw EDLSJob("logger::process_data: " + e.msg);
      }

      return;
    }
    logger_i++;
  }

  msg() << "channel " << channel_index << " not required!";
  log(DLSWarning);
}

//---------------------------------------------------------------

/**
   Entfernt alle Logger aus der Liste

   Diese Methode ruft ein "delete" f�r jeden Logger auf, auch wenn
   ein Fehler passiert. Das vermeidet Speicherlecks. Fehler
   werden am Schluss gesammelt ausgegeben.

   \throw EDLSJob Fehler beim L�schen ein oder mehrerer Logger
*/

void DLSJob::_clear_loggers()
{
  list<DLSLogger *>::iterator logger;
  stringstream err;
  bool error = false;

  logger = _loggers.begin();
  while (logger != _loggers.end())
  {
    try
    {
      delete *logger;
    }
    catch (EDLSLogger &e)
    {
      error = true;
      err << "deleting logger: " << e.msg << " ";
    }

    logger++;
  }

  _loggers.clear();

  if (error)
  {
    throw EDLSJob(err.str());
  }
}

//---------------------------------------------------------------

/**
   Pr�ft, ob ein Logger f�r einen bestimmten Kanal existiert

   \param name Kanalname
   \return Zeiger auf gefundenen Logger, oder 0
*/

DLSLogger *DLSJob::_logger_exists_for_channel(const string &name)
{
  list<DLSLogger *>::const_iterator logger = _loggers.begin();
  while (logger != _loggers.end())
  {
    if ((*logger)->channel_preset()->name == name) return *logger;
    logger++;
  }

  return 0;
}

//---------------------------------------------------------------

/**
   Generiert eine eindeutige Kommando-ID

   Die ID wird aus der Adresse des Auftrags-Objektes und einer
   Sequenz-Nummer zusammengesetzt.

   \return Generierte ID
*/

string DLSJob::_generate_id()
{
  stringstream id;

  // ID aus Adresse und Counter erzeugen
  id << (int) this << "_" << ++_id_gen;

  return id.str();
}

//---------------------------------------------------------------

/**
   Speichert alle wartenden Daten

   Weist alle Logger an, ihre Daten zu speichern. Wenn dies
   fehlerfrei geschehen ist, f�hrt ein "delete" des Job-Objektes
   nicht zu Datenverlust.

   \throw EDLSJob Ein oder mehrere Logger konnten ihre
   Daten nicht speichern - Datenverlust!
*/
   
void DLSJob::finish()
{
  list<DLSLogger *>::iterator logger;
  stringstream err;
  bool errors = false;

  logger = _loggers.begin();
  while (logger != _loggers.end())
  {
    try
    {
      (*logger)->finish();
    }
    catch (EDLSLogger &e)
    {
      errors = true;
      if (err.str().length()) err << "; ";
      err << e.msg;
    }

    logger++;
  }

  if (errors)
  {
    throw EDLSJob("logger::finish(): " + err.str());
  }

  msg() << "job finished without errors.";
  log(DLSInfo);
}

//---------------------------------------------------------------

/**
   Speichert eine den Auftrag betreffende Nachricht

   \param info_tag Info-Tag
*/

void DLSJob::message(const string &info_tag)
{
  fstream file;
  stringstream filename;

  filename << _dls_dir << "/job" << _job_id << "/messages.xml";

  file.open(filename.str().c_str(), ios::out | ios::app);

  if (!file.is_open())
  {
    msg() << "could not log message: " << info_tag;
    log(DLSError);
  }

  file << info_tag << endl;
  file.close();
}

//---------------------------------------------------------------

/**
   Nimmt eine Logging-Nachricht als Stream auf

   \return Referenz auf den Nachrichtenstream des Logging-Prozesses
*/

stringstream &DLSJob::msg() const
{
  return _parent_proc->msg();
}

//---------------------------------------------------------------

/**
   Loggt eine vorher aufgezeichnete Nachricht

   \param type Typ der Nachricht
*/

void DLSJob::log(DLSLogType type) const
{
  _parent_proc->log(type);
}

//---------------------------------------------------------------
