//---------------------------------------------------------------
//
//  D L S _ J O B _ P R E S E T . C P P
//
//---------------------------------------------------------------

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <sstream>
#include <fstream>
using namespace std;

//---------------------------------------------------------------

#include "com_xml_parser.hpp"
#include "com_job_preset.hpp"

//---------------------------------------------------------------

COMJobPreset::COMJobPreset()
{
  _id = 0;
  _running = false;
  _quota = 0;
  process_watchdog = 0;
  logging_watchdog = 0;
}

//---------------------------------------------------------------

COMJobPreset::~COMJobPreset()
{
}

//---------------------------------------------------------------

/**
   Importiert Auftragsvorgaben aus einer XML-Datei

   Der Dateiname wird aus dem Datenverzeichnis und der
   Auftrags-ID generiert. Die Datei wird geöffnet und ge'parse't.

   \param dls_dir Das DLS-Datenverzeichnis
   \param id Auftrags-ID
   \throw ECOMJobPreset Fehler während des Importierens
*/

void COMJobPreset::import(const string &dls_dir, int id)
{
  COMChannelPreset channel;
  string value;
  stringstream file_name;
  fstream file;
  COMXMLParser parser;
  stringstream err;

  _id = id;

  _channels.clear();

  // Dateinamen konstruieren
  file_name << dls_dir << "/job" << id << "/job.xml";

  // Datei öffnen
  file.open(file_name.str().c_str(), ios::in);

  if (!file.is_open())
  {
    err << "could not open file \"" << file_name.str() << "\""; 
    throw ECOMJobPreset(err.str());
  }

  try
  {
    parser.parse(&file, "dlsjob", dxttBegin);

    _description = parser.parse(&file, "description", dxttSingle)->att("text")->to_str();

    value = parser.parse(&file, "state", dxttSingle)->att("name")->to_str();

    if (value == "running") _running = true;
    else if (value == "paused") _running = false;
    else
    {
      file.close();
      throw ECOMJobPreset("unknown state \"" + value + "\"!");
    }

    _source = parser.parse(&file, "source", dxttSingle)->att("address")->to_str();
    _quota = parser.parse(&file, "quota", dxttSingle)->att("size")->to_int();
    _trigger = parser.parse(&file, "trigger", dxttSingle)->att("parameter")->to_str();

    parser.parse(&file, "channels", dxttBegin);

    while (1)
    {
      parser.parse(&file);

      if (parser.last_tag()->title() == "channels"
          && parser.last_tag()->type() == dxttEnd)
      {
        break;
      }

      if (parser.last_tag()->title() == "channel"
          && parser.last_tag()->type() == dxttSingle)
      {
        try
        {
          channel.read_from_tag(parser.last_tag());
        }
        catch (ECOMChannelPreset &e)
        {
          file.close();
          err << "error reading channel: " << e.msg;
          throw ECOMJobPreset(err.str());
        }

        _channels.push_back(channel);
      }
      else
      {
        file.close();
        err << "expected channel/ or /channels!";
        throw ECOMJobPreset(err.str());
      }
    }

    parser.parse(&file, "dlsjob", dxttEnd);
  }
  catch (ECOMXMLParser &e)
  {
    file.close();
    err << "parsing: " << e.msg;
    throw ECOMJobPreset(err.str());
  }
  catch (ECOMXMLParserEOF &e)
  {
    file.close();
    err << "parsing: " << e.msg;
    throw ECOMJobPreset(err.str());
  }
  catch (ECOMXMLTag &e)
  {
    file.close();
    err << "tag: " << e.msg;
    throw ECOMJobPreset(err.str());
  }

  file.close();
}

//---------------------------------------------------------------

/**
   Auftragsvorgaben in XML-Datei speichern

   Schreibt alle Auftragsvorgaben (inclusive Kanalvorgaben)
   in eine XML-Datei im richtigen Verzeichnis. Danach wird
   der Server über eine Spooling-Datei benachrichtigt,
   dass er die Vorgaben neu einlesen soll.

   \param dls_dir DLS-Datenverzeichnis
   \throw ECOMJobPreset Ungültige Daten oder Schreiben nicht möglich
*/

void COMJobPreset::write(const string &dls_dir)
{
  stringstream dir_name, err;
  string file_name;
  fstream file;
  COMXMLTag tag;
  vector<COMChannelPreset>::iterator channel_i;

  // Verzeichnisnamen konstruieren
  dir_name << dls_dir << "/job" << _id;

  if (mkdir(dir_name.str().c_str(), 0755) != 0)
  {
    if (errno != EEXIST)
    {
      err << "could not create \"" << dir_name.str() << "\" (errno " << errno << ")!";
      throw ECOMJobPreset(err.str());     
    }
  }

  // Dateinamen konstruieren
  file_name = dir_name.str() + "/job.xml";

  // Datei öffnen
  file.open(file_name.c_str(), ios::out);

  if (!file.is_open())
  {
    err << "could not write file \"" << file_name << "\""; 
    throw ECOMJobPreset(err.str());
  }

  try
  {
    tag.clear();
    tag.type(dxttBegin);
    tag.title("dlsjob");
    file << tag.tag() << endl;

    tag.clear();
    tag.title("description");
    tag.push_att("text", _description);
    file << " " << tag.tag() << endl;

    tag.clear();
    tag.title("state");
    tag.push_att("name", _running ? "running" : "paused");
    file << " " << tag.tag() << endl;

    tag.clear();
    tag.title("source");
    tag.push_att("address", _source);
    file << " " << tag.tag() << endl;

    tag.clear();
    tag.title("quota");
    tag.push_att("size", _quota);
    file << " " << tag.tag() << endl;

    tag.clear();
    tag.title("trigger");
    tag.push_att("parameter", _trigger);
    file << " " << tag.tag() << endl;

    file << endl;

    tag.clear();
    tag.title("channels");
    tag.type(dxttBegin);
    file << " " << tag.tag() << endl;

    channel_i = _channels.begin();
    while (channel_i != _channels.end())
    {
      channel_i->write_to_tag(&tag);
      file << "  " << tag.tag() << endl;
      channel_i++;
    }

    tag.clear();
    tag.title("channels");
    tag.type(dxttEnd);
    file << " " << tag.tag() << endl;

    tag.clear();
    tag.title("dlsjob");
    tag.type(dxttEnd);
    file << tag.tag() << endl;
  }
  catch (ECOMChannelPreset &e)
  {
    file.close();
    err << "could not write: " << e.msg;
    throw ECOMJobPreset(err.str());
  }
  catch (...)
  {
    file.close();
    throw ECOMJobPreset("could not write!");
  }

  file.close();
}

//---------------------------------------------------------------

/**
   \todo doc
   \param dls_dir DLS-Datenverzeichnis
*/

void COMJobPreset::notify_new(const string &dls_dir)
{
  _write_spooling_file(dls_dir, "new");
}

//---------------------------------------------------------------

/**
   \todo doc
   \param dls_dir DLS-Datenverzeichnis
*/

void COMJobPreset::notify_changed(const string &dls_dir)
{
  _write_spooling_file(dls_dir, "change");
}

//---------------------------------------------------------------

/**
   \todo doc
   \param dls_dir DLS-Datenverzeichnis
*/

void COMJobPreset::notify_deleted(const string &dls_dir)
{
  _write_spooling_file(dls_dir, "delete");
}

//---------------------------------------------------------------

/**
   Spooling-Datei erzeugen, um den Server zu benachrichtigen

   Diese Methode benachrichtigt den Server bezüglich
   einer Änderung einer Auftragsvorgabe. Es kann z. B.
   eine neue Vorgabe erstellt worden sein, eine
   vorhandene Vorgabe kann sich geändert haben, oder es
   wurde eine Vorgabe gelöscht.

   \param dls_dir DLS-Datenverzeichnis
   \param action "new", "change", oder "delete"
*/

void COMJobPreset::_write_spooling_file(const string &dls_dir,
                                        const string &action)
{
  fstream file;
  stringstream filename, err;
  COMXMLTag tag;
  struct timeval tv;

  tag.title("dls");
  tag.push_att("action", action);
  tag.push_att("job", _id);

  gettimeofday(&tv, 0);

  // Eindeutigen Dateinamen erzeugen
  filename << dls_dir << "/spool/";
  filename << tv.tv_sec << "_" << tv.tv_usec;
  filename << "_" << (unsigned int) this;

  file.open(filename.str().c_str(), ios::out);

  if (!file.is_open())
  {
    err << "could not write spooling file \"" << filename.str() << "\"!";
    throw ECOMJobPreset(err.str());
  }

  file << tag.tag() << endl;
  file.close();
}     

//---------------------------------------------------------------

/**
   Prüfen, ob ein bestimmter Kanal in den Vorgaben existiert

   \param name Kanalname
   \return true, wenn der Kanal in den Vorgaben existiert
*/

bool COMJobPreset::channel_exists(const string &name) const
{
  vector<COMChannelPreset>::const_iterator channel = _channels.begin();
  while (channel != _channels.end())
  {
    if (channel->name == name) return true;
    channel++;
  }

  return false;
}

//---------------------------------------------------------------

/**
   Setzt die Auftrags-ID
*/

void COMJobPreset::id(int id)
{
  _id = id;
}

//---------------------------------------------------------------

/**
   Setzt die Auftragsbeschreibung
*/

void COMJobPreset::description(const string &desc)
{
  _description = desc;
}

//---------------------------------------------------------------

/**
   Setzt den Sollstatus
*/

void COMJobPreset::running(bool run)
{
  _running = run;
}

//---------------------------------------------------------------

/**
   Wechselt den Sollstatus
*/

void COMJobPreset::toggle_running()
{
  _running = !_running;
}

//---------------------------------------------------------------

/**
   Setzt die Datenquelle
*/

void COMJobPreset::source(const string &src)
{
  _source = src;
}

//---------------------------------------------------------------

/**
   Setzt den Namen des Trigger-Parameters
*/

void COMJobPreset::trigger(const string &trigger_name)
{
  _trigger = trigger_name;
}

//---------------------------------------------------------------

/**
   Fügt eine Kanalvorgabe hinzu

   \param channel Neue Kanalvorgabe
   \throw ECOMJobPreset Eine Vorgabe für diesen Kanal
   existiert bereits!
*/

void COMJobPreset::add_channel(const COMChannelPreset *channel)
{
  stringstream err;

  if (channel_exists(channel->name))
  {
    err << "channel \"" << channel->name << "\" already exists!";
    throw ECOMJobPreset(err.str());
  }

  _channels.push_back(*channel);
}

//---------------------------------------------------------------

/**
   Ändert eine Kanalvorgabe

   Der zu ändernde Kanal wird anhand des Kanalnamens in der
   neuen Vorgabe bestimmt.

   \param new_channel Zeiger auf neue KanalvorgabeKanalname
   \throw ECOMJobPreset Es existiert keine Vorgabe für
   den angegebenen Kanal
*/

void COMJobPreset::change_channel(const COMChannelPreset *new_channel)
{
  vector<COMChannelPreset>::iterator channel_i;
  stringstream err;

  channel_i = _channels.begin();
  while (channel_i != _channels.end())
  {
    if (channel_i->name == new_channel->name)
    {
      *channel_i = *new_channel;
      return;
    }

    channel_i++;
  }

  err << "preset for channel \"" << new_channel->name << "\" doesn't exist!";
  throw ECOMJobPreset(err.str());
}

//---------------------------------------------------------------

/**
   Entfernt eine Kanalvorgabe

   \param channel_name Kanalname des Kanals, dessen Vorgabe
   entfernt werden soll
   \throw ECOMJobPreset Es existiert keine Vorgabe für
   den angegebenen Kanal
*/

void COMJobPreset::remove_channel(const string &channel_name)
{
  vector<COMChannelPreset>::iterator channel_i;
  stringstream err;

  channel_i = _channels.begin();
  while (channel_i != _channels.end())
  {
    if (channel_i->name == channel_name)
    {
      _channels.erase(channel_i);
      return;
    }

    channel_i++;
  }

  err << "preset for channel \"" << channel_name << "\" doesn't exist!";
  throw ECOMJobPreset(err.str());
}

//---------------------------------------------------------------

/**
   Erstellt einen String aus ID und Beschreibung

   Format "(<ID>) <Beschreibung>"

   \returns String
   \see _id
   \see _name
*/

string COMJobPreset::id_desc() const
{
  stringstream ret;

  ret << "(" << _id << ") " << _description;

  return ret.str();
}

//---------------------------------------------------------------
