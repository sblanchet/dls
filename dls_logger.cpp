//---------------------------------------------------------------
//
//  D L S _ L O G G E R . C P P
//
//---------------------------------------------------------------

#include <fstream>
#include <sstream>
using namespace std;

//---------------------------------------------------------------

#include "com_base64.hpp"
#include "com_xml_tag.hpp"
#include "dls_globals.hpp"
#include "dls_job.hpp"
#include "dls_saver_gen_t.hpp"
#include "dls_logger.hpp"

//---------------------------------------------------------------

/**
   Kontruktor

   \param job Zeiger auf das besitzende Auftrags-Objekt
   \param channel_preset Kanalvorgaben
   \param dls_dir DLS-Datenverzeichnis
*/
   
DLSLogger::DLSLogger(const DLSJob *job,
                     const COMChannelPreset *channel_preset,
                     const string &dls_dir)
{
  _parent_job = job;
  _channel_preset = *channel_preset;
  _dls_dir = dls_dir;

  _saver = 0;
  _change_in_progress = false;
  _finished = true;
  _chunk_created = false;
}

//---------------------------------------------------------------

/**
   Destruktor

   Gibt eine Fehlermeldung aus, wenn noch Daten im Speicher sind

   \throw EDLSLogger Fehler beim Löschen des Saver-Objektes
*/

DLSLogger::~DLSLogger()
{
  if (!_finished)
  {
    msg() << "logger not finished!";
    log(DLSWarning);
  }

  try
  {
    if (_saver) delete _saver;
  }
  catch (EDLSSaver &e)
  {
    throw EDLSLogger("deleting saver: " + e.msg);
  }
}

//---------------------------------------------------------------

/**
   Holt sich den zu den Vorgaben passenden msrd-Kanal

   Die Liste der msrd-Kanäle wird auf den Namen des
   Vorgabekanals hin durchsucht. Wird dieser gefunden, dann
   werden die Eigenschaften des msrd-Kanals für spätere
   Zwecke kopiert.

   Da jetzt der Datentyp des Kanals bekannt ist, kann
   jetzt eine Instanz der DLSSaverGenT - Template-Klasse
   gebildet und damit ein Saver-Objekt erzeugt werden.

   \param channels Konstante Liste der msrd-Kanäle
   \throw EDLSLogger Kanal nicht gefunden, unbekannter Datentyp,
   oder Fehler beim Erstellen des Saver-Objektes
*/

void DLSLogger::get_real_channel(const list<COMRealChannel> *channels)
{
  list<COMRealChannel>::const_iterator channel_i = channels->begin();

  while (channel_i != channels->end())
  {
    if (channel_i->name() == _channel_preset.name)
    {
      _real_channel = *channel_i;

      if (_saver) delete _saver;
      _saver = 0;

      try
      {
        switch (_real_channel.type())
        {
          case TCHAR:
            _saver = new DLSSaverGenT<char>(this);
            break;
          case TUCHAR:
            _saver = new DLSSaverGenT<unsigned char>(this);
            break;
          case TINT:
            _saver = new DLSSaverGenT<int>(this);
            break;
          case TUINT:
            _saver = new DLSSaverGenT<unsigned int>(this);
            break;
          case TLINT:
            _saver = new DLSSaverGenT<long>(this);
            break;
          case TULINT:
            _saver = new DLSSaverGenT<unsigned long>(this);
            break;
          case TFLT:
            _saver = new DLSSaverGenT<float>(this);
            break;
          case TDBL:
            _saver = new DLSSaverGenT<double>(this);
            break;

          default: throw EDLSLogger("Wrong data type!");
        }
      }
      catch (EDLSSaver &e)
      {
        throw EDLSLogger("constructing new saver: " + e.msg);
      }
      catch (...)
      {
        throw EDLSLogger("out of memory while constructing saver!");
      }

      if (_channel_preset.meta_mask & DLSMetaMean)
      {
        _saver->add_meta_saver(DLSMetaMean);
      }
      if (_channel_preset.meta_mask & DLSMetaMin)
      {
        _saver->add_meta_saver(DLSMetaMin);
      } 
      if (_channel_preset.meta_mask & DLSMetaMax)
      {
        _saver->add_meta_saver(DLSMetaMax);
      }

      return;
    }

    channel_i++;
  }

  throw EDLSLogger("channel does not exist!");
}

//---------------------------------------------------------------

/**
   Verifiziert die angegebenen Vorgaben

-# Die Sampling-Frequenz muss größer 0 sein
-# Die Sampling-Frequenz darf den für den Kanal angegebenen Maximalwert nicht übersteigen.
-# Die Sampling-Frequenz muss zu einer ganzzahligen Untersetzung führen.
-# blocksize * reduction < channel buffer size / 2!

   \param channel Konstanter Zeiger auf zu prüfende Kanalvorgaben.
   Weglassen dieses Parameters erwirkt die Prüfung der eigenen Vorgaben.
   \return true, wenn die Vorgaben in Ordnung sind
*/

bool DLSLogger::presettings_valid(const COMChannelPreset *channel) const
{
  double reduction;
  unsigned int block_size;

  if (!channel) channel = &_channel_preset;

  if (channel->sample_frequency <= 0)
  {
    msg() << "channel \"" << channel->name << "\": ";
    msg() << "illegal frequency! (" << channel->sample_frequency << " Hz)";
    log(DLSError);
    return false;
  }

  if (channel->sample_frequency > _real_channel.frequency())
  {
    msg() << "channel \"" << channel->name << "\": ";
    msg() << "frequency exceeds channel maximum!";
    msg() << " (" << channel->sample_frequency << " / ";
    msg() << _real_channel.frequency() << " Hz)";
    log(DLSError);
    return false;
  }

  reduction = _real_channel.frequency() / (double) channel->sample_frequency;

  if (reduction != (unsigned int) reduction)
  {
    msg() << "channel \""<< channel->name << "\": ";
    msg() << "frequency leads to non-integer reduction!";
    log(DLSError);
    return false;
  }

  block_size = channel->sample_frequency;
  
  if (block_size * reduction > _real_channel.bufsize() / 2)
  {
    msg() << "channel \""<< channel->name << "\": ";
    msg() << "buffer limit exceeded! ";
    msg() << block_size * reduction << " > " << _real_channel.bufsize() / 2;
    log(DLSError);
    return false;
  }

  if (channel->format_index == DLS_FORMAT_MDCT
      && _real_channel.type() != TFLT
      && _real_channel.type() != TDBL)
  {
    msg() << "MDCT compression only for floating point channels!";
    log(DLSError);
    return false;
  }

  return true;
}

//---------------------------------------------------------------

/**
   Erzeugt den Startbefehl für die Erfassung des Kanals

   \param channel Konstanter Zeiger auf die Kanalvorgabe
   \param id ID, die dem Startbefehl beigefügt wird, um eine
   Bestätigung zu bekommen. Kann weggelassen werden.
   \return XML-Startbefehl
*/

string DLSLogger::start_tag(const COMChannelPreset *channel,
                            const string &id) const
{
  stringstream tag;
  double reduction;
  unsigned int block_size;

  reduction = _real_channel.frequency() / channel->sample_frequency;

  if (reduction != (unsigned int) reduction)
  {
    throw EDLSLogger("frequency results in no integer reduction!");
  }

  block_size = channel->sample_frequency;
  
  // Blocksize begrenzen (msrd kann nur 1024)
  if (block_size > 1000) block_size = 1000;

  // Blocksize muss ein Teiler von Sample-Frequency sein!
  if (channel->sample_frequency % block_size != 0)
  {
    stringstream err;
    err << "block size (" << block_size << ")";
    err << " doesn't match frequency (" << channel->sample_frequency << ")!";
    throw EDLSLogger(err.str());
  }

  tag << "<xsad channels=\"" << _real_channel.index() << "\"";
  tag << " reduction=\"" << (unsigned int) reduction << "\"";
  tag << " blocksize=\"" << block_size << "\"";
  tag << " coding=\"Base64\"";
  if (id != "") tag << " id=\"" << id << "\"";
  tag << ">";

  return tag.str();
}

//---------------------------------------------------------------

/**
   Erzuegt den Befehl zum Anhalten der Datenerfassung

   \returns XML-Stoppbefehl
*/

string DLSLogger::stop_tag() const
{
  stringstream tag;

  tag << "<xsod channels=\"" << _real_channel.index() << "\">";
  return tag.str();
}

//---------------------------------------------------------------

/**
   Nimmt erfasste Daten zum Speichern entgegen

   Geht davon aus, dass die entgegengenommenen Daten
   Base64-kodierte Binärdaten sind. Diese werden
   dekodiert und intern dem Saver-Objekt übergeben.

   \param data Konstante Referenz auf die Daten
   \param time Zeitpunkt des letzten Einzelwertes
   \throw EDLSLogger Fehler beim Dekodieren oder Speichern
   \throw EDLSLoggerTime Toleranzfehler! Prozess beenden!
*/

void DLSLogger::process_data(const string &data, COMTime time)
{
  COMBase64 base64;

  // Jetzt ist etwas im Gange
  if (_finished) _finished = false;

  try
  {
    // Daten dekodieren
    base64.decode(data.c_str(), data.length());
  }
  catch (ECOMBase64 &e)
  {
    throw EDLSLogger("base64 error: " + e.msg);    
  }

  try
  {
    // Daten an Saver übergeben
    _saver->process_data(base64.output(), base64.length(), time);
  }
  catch (EDLSSaver &e)
  {
    throw EDLSLogger("process_data: " + e.msg);
  }
}

//---------------------------------------------------------------

/**
   Erzeugt ein neues Chunk-Verzeichnis

   \param time_of_first Zeit des ersten Einzelwertes zur
   Generierung des Verzeichnisnamens
   \throw EDLSLogger Fehler beim Erstellen des Verzeichnisses
*/

void DLSLogger::create_chunk(COMTime time_of_first)
{
  DIR *dir;
  stringstream dir_name;
  stringstream err;
  fstream file;
  COMXMLTag tag;

  _chunk_created = false;

  if (_channel_preset.format_index < 0 || _channel_preset.format_index >= DLS_FORMAT_COUNT)
  {
    throw EDLSSaver("invalid channel format!");
  }

  dir_name << _dls_dir;
  dir_name << "/job" << _parent_job->preset()->id();
  dir_name << "/channel" << _real_channel.index();

  // Prüfen, ob Kanalverzeichnis bereits existiert
  if ((dir = opendir(dir_name.str().c_str())) == 0)
  {
    if (errno == ENOENT)
    {
      // Verzeichnis existiert nicht. Anlegen!
      if (mkdir(dir_name.str().c_str(), 0755) != 0)
      {
        err << "create_chunk: could not create directory \"";
        err << dir_name.str() << "\"!";
        throw EDLSSaver(err.str());
      }

      // channel.xml anlegen
      file.open((dir_name.str() + "/channel.xml").c_str(), ios::out);

      if (!file.is_open())
      {
        err << "create_chunk: could not write channel.xml into dir \"";
        err << dir_name.str() << "\"!";
        throw EDLSSaver(err.str());
      }

      tag.clear();
      tag.title("dlschannel");
      tag.type(dxttBegin);
      file << tag.tag() << endl;
      
      tag.clear();
      tag.title("channel");
      tag.push_att("name", _channel_preset.name);
      tag.push_att("index", _real_channel.index());
      tag.push_att("unit", _real_channel.unit());
      tag.push_att("type", _real_channel.type_str());
      file << " " << tag.tag() << endl;

      tag.clear();
      tag.title("dlschannel");
      tag.type(dxttEnd);
      file << tag.tag() << endl;

      file.close();
    }
    else
    {
      err << "create_chunk: could not open directory";
      err << " \"" << dir_name.str() << "\".";

      if (errno == EACCES) err << " permission denied.";
      else if (errno == ENOTDIR) err << " exists, but is no dir!";
      else err << " fatal error.";

      throw EDLSSaver(err.str());
    }
  }
  else
  {
    // Verzeichnis existiert. Wieder schliessen
    closedir(dir);
  }

  dir_name << "/chunk" << time_of_first;

  if (mkdir(dir_name.str().c_str(), 0755) != 0)
  {
    err << "create_chunk: could not create directory \"";
    err << dir_name.str() << "\"!";

    if (errno == EEXIST) err << " exists already!";

    throw EDLSLogger(err.str());
  } 

  // chunk.xml anlegen
  file.open((dir_name.str() + "/chunk.xml").c_str(), ios::out);

  if (!file.is_open())
  {
    err << "create_chunk: could not write chunk.xml into dir \"";
    err << dir_name.str() << "\"!";
    throw EDLSSaver(err.str());
  }

  tag.clear();
  tag.title("dlschunk");
  tag.type(dxttBegin);
  file << tag.tag() << endl;

  tag.clear();
  tag.title("chunk");
  tag.push_att("sample_frequency", _channel_preset.sample_frequency);
  tag.push_att("meta_mask", _channel_preset.meta_mask);
  tag.push_att("meta_reduction", _channel_preset.meta_reduction);
  tag.push_att("format", dls_format_strings[_channel_preset.format_index]);
  file << " " << tag.tag() << endl;

  tag.clear();
  tag.title("time");
  tag.push_att("start", time_of_first.to_str());
  file << " " << tag.tag() << endl;

  tag.clear();
  tag.title("dlschunk");
  tag.type(dxttEnd);
  file << tag.tag() << endl;

  file.close();

  _chunk_dir = dir_name.str();
  _chunk_created = true;
}

//---------------------------------------------------------------

/**
   Merkt eine Änderung der Kanalvorgaben vor

   Wenn ein Änderungsbefehl gesendet wurde, die
   Bestätigung aber noch nicht da ist, kann es sein,
   dass noch Daten entsprechend der "alten" Vorgabe empfangen
   werden. Deshalb darf die Vorgabe solange nicht übernommen
   werden, bis die Bestätigung da ist.

   Das passiert mit dieser Methode. Sie merkt die neue Vorgabe vor.
   Sobald eine Bestätigung mit der angegebenen ID
   empfangen wird, muss do_change() aufgerufen werden.

   \param channel Die neue, "wartende" Kanalvorgabe
   \param id Änderungs-ID, die mit dem Änderungsbefehl
   gesendet wurde.
   \see change_is()
   \see do_change()
*/

void DLSLogger::set_change(const COMChannelPreset *channel,
                           const string &id)
{
  if (_change_in_progress)
  {
    msg() << "change in progress!";
    log(DLSWarning);
  }
  else
  {
    _change_in_progress = true;
  }

  _change_id = id;
  _change_channel = *channel;
}

//---------------------------------------------------------------

/**
   Abfrage auf eine bestimmte Änderungs-ID

   Gibt "true" zurück, wenn die Angegebene ID
   auf die vorgemerkte passt und zudem eine Änderung wartet

   \return true, wenn ID passt
*/

bool DLSLogger::change_is(const string &id) const
{
  return _change_id == id && _change_in_progress;
}

//---------------------------------------------------------------

/**
   Führt eine vorgemerkte Änderung durch

   \throw EDLSLogger Fehler beim Speichern von wartenden Daten
 */

void DLSLogger::do_change()
{
  if (!_change_in_progress) return;

  // Chunks beenden!
  finish();

  // Änderungen übernehmen
  _channel_preset = _change_channel;

  _change_in_progress = false;
}

//---------------------------------------------------------------

/**
   Speichert wartende Daten

   Speichert alle Daten, die noch nicht im Dateisystem sind.

   \throw EDLSLogger Fehler beim Speichern - Datenverlust!
*/

void DLSLogger::finish()
{
  stringstream err;
  bool error = false;

  try
  {
    // Alle Daten speichern
    if (_saver) _saver->flush();
  }
  catch (EDLSSaver &e)
  {
    error = true;
    err << "saver::flush(): " << e.msg;

  }
  catch (EDLSLogger &e)
  {
    error = true;
    err << "saver::flush(): " << e.msg;
  }

  // Chunk beenden
  _chunk_created = false;

  if (error)
  {
    throw EDLSLogger(err.str());
  }

  _finished = true;
}

//---------------------------------------------------------------

/**
   Nimmt eine Nachricht zum späteren Loggen auf

   \return Referenz auf den msg-Stream des Logging-Prozesses
   \see DLSJob::msg()
*/

stringstream &DLSLogger::msg() const
{
  return _parent_job->msg();
}

//---------------------------------------------------------------

/**
   Speichert eine vorher aufgezeichnete Nachricht

   \param type Typ der Nachricht
   \see DLSJob::log()
*/

void DLSLogger::log(DLSLogType type) const
{
  _parent_job->log(type);
}

//---------------------------------------------------------------
