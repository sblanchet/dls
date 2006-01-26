//---------------------------------------------------------------
//
//  C O M _ J O B _ P R E S E T . C P P
//
//---------------------------------------------------------------

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <sstream>
#include <fstream>
using namespace std;

//---------------------------------------------------------------

#include "com_globals.hpp"
#include "com_xml_parser.hpp"
#include "com_job_preset.hpp"

//---------------------------------------------------------------

RCS_ID("$Header: /home/fp/dls/src/RCS/com_job_preset.cpp,v 1.8 2005/01/21 08:59:31 fp Exp $");

//---------------------------------------------------------------

/**
   Konstruktor
*/

COMJobPreset::COMJobPreset()
{
  _id = 0;
  _running = false;
  _quota_time = 0;
  _quota_size = 0;
}

//---------------------------------------------------------------

/**
   Destruktor
*/

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

    parser.parse(&file, "quota", dxttSingle);

    if (parser.last_tag()->has_att("time"))
    {
      _quota_time = parser.last_tag()->att("time")->to_ll();
    }
    else
    {
      _quota_time = 0;
    }

    if (parser.last_tag()->has_att("size"))
    {
      _quota_size = parser.last_tag()->att("size")->to_ll();
    }
    else
    {
      _quota_size = 0;
    }

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
