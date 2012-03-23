/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <sstream>
#include <fstream>
using namespace std;

/*****************************************************************************/

#include "com_globals.hpp"
#include "com_xml_parser.hpp"
#include "com_job_preset.hpp"

/*****************************************************************************/

/**
   Konstruktor
*/

COMJobPreset::COMJobPreset():
    _port(MSRD_PORT)
{
    _id = 0;
    _running = false;
    _quota_time = 0;
    _quota_size = 0;
}

/*****************************************************************************/

/**
   Destruktor
*/

COMJobPreset::~COMJobPreset()
{
}

/*****************************************************************************/

/**
   Importiert Auftragsvorgaben aus einer XML-Datei

   Der Dateiname wird aus dem Datenverzeichnis und der
   Auftrags-ID generiert. Die Datei wird geöffnet und ge'parse't.

   \param dls_dir Das DLS-Datenverzeichnis
   \param id Auftrags-ID
   \throw ECOMJobPreset Fehler während des Importierens
*/

void COMJobPreset::import(const string &dls_dir, unsigned int id)
{
    COMChannelPreset channel;
    string value;
    stringstream file_name;
    fstream file;
    COMXMLParser parser;
    stringstream err;
    const COMXMLTag *tag;

    _id = id;

    _channels.clear();

    // Dateinamen konstruieren
    file_name << dls_dir << "/job" << id << "/job.xml";

    // Datei öffnen
    file.open(file_name.str().c_str(), ios::in);

    if (!file.is_open())
    {
        err << "Could not open file \"" << file_name.str() << "\"";
        throw ECOMJobPreset(err.str());
    }

    try
    {
        parser.parse(&file, "dlsjob", dxttBegin);

        _description = parser.parse(&file, "description",
                                    dxttSingle)->att("text")->to_str();

        value = parser.parse(&file, "state",
                             dxttSingle)->att("name")->to_str();

        if (value == "running") _running = true;
        else if (value == "paused") _running = false;
        else
        {
            file.close();
            throw ECOMJobPreset("Unknown state \"" + value + "\"!");
        }

        tag = parser.parse(&file, "source",
                               dxttSingle);
        _source = tag->att("address")->to_str();
        if (tag->has_att("port")) {
            _port = tag->att("port")->to_int();
        } else {
            _port = MSRD_PORT;
        }

        parser.parse(&file, "quota", dxttSingle);

        if (parser.tag()->has_att("time"))
        {
            _quota_time = parser.tag()->att("time")->to_uint64();
        }
        else
        {
            _quota_time = 0;
        }

        if (parser.tag()->has_att("size"))
        {
            _quota_size = parser.tag()->att("size")->to_uint64();
        }
        else
        {
            _quota_size = 0;
        }

        _trigger = parser.parse(&file, "trigger",
                                dxttSingle)->att("parameter")->to_str();

        parser.parse(&file, "channels", dxttBegin);

        while (1)
        {
            parser.parse(&file);

            if (parser.tag()->title() == "channels"
                && parser.tag()->type() == dxttEnd)
            {
                break;
            }

            if (parser.tag()->title() == "channel"
                && parser.tag()->type() == dxttSingle)
            {
                try
                {
                    channel.read_from_tag(parser.tag());
                }
                catch (ECOMChannelPreset &e)
                {
                    file.close();
                    err << "Error reading channel: " << e.msg;
                    throw ECOMJobPreset(err.str());
                }

                _channels.push_back(channel);
            }
            else
            {
                file.close();
                err << "Expected channel/ or /channels!";
                throw ECOMJobPreset(err.str());
            }
        }

        parser.parse(&file, "dlsjob", dxttEnd);
    }
    catch (ECOMXMLParser &e)
    {
        file.close();
        err << "Parsing: " << e.msg;
        throw ECOMJobPreset(err.str());
    }
    catch (ECOMXMLParserEOF &e)
    {
        file.close();
        err << "Parsing: " << e.msg;
        throw ECOMJobPreset(err.str());
    }
    catch (ECOMXMLTag &e)
    {
        file.close();
        err << "Tag: " << e.msg;
        throw ECOMJobPreset(err.str());
    }

    file.close();
}

/*****************************************************************************/

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

/*****************************************************************************/

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

/*****************************************************************************/
