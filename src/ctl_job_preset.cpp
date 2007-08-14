/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <sstream>
#include <fstream>
using namespace std;

/*****************************************************************************/

#include "com_globals.hpp"
#include "ctl_job_preset.hpp"

/*****************************************************************************/

/**
   Konstruktor
*/

CTLJobPreset::CTLJobPreset() : COMJobPreset()
{
    process_watchdog = 0;
    logging_watchdog = 0;
};

/*****************************************************************************/

/**
   Auftragsvorgaben in XML-Datei speichern

   Schreibt alle Auftragsvorgaben (inclusive Kanalvorgaben)
   in eine XML-Datei im richtigen Verzeichnis. Danach wird
   der Mutterprozess über eine Spooling-Datei benachrichtigt,
   dass er die Vorgaben neu einlesen soll.

   \param dls_dir DLS-Datenverzeichnis
   \throw ECOMJobPreset Ungültige Daten oder Schreiben nicht möglich
*/

void CTLJobPreset::write(const string &dls_dir)
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
            err << "Could not create \"" << dir_name.str()
                << "\" (errno " << errno << ")!";
            throw ECOMJobPreset(err.str());
        }
    }

    // Dateinamen konstruieren
    file_name = dir_name.str() + "/job.xml";

    // Datei öffnen
    file.open(file_name.c_str(), ios::out);

    if (!file.is_open())
    {
        err << "Could not write file \"" << file_name << "\"";
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
        tag.push_att("size", _quota_size);
        tag.push_att("time", _quota_time);
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
        err << "Could not write: " << e.msg;
        throw ECOMJobPreset(err.str());
    }
    catch (...)
    {
        file.close();
        throw ECOMJobPreset("Could not write!");
    }

    file.close();
}

/*****************************************************************************/

/**
   Spooling-Datei erzeugen, um den Prozess zu benachrichtigen

   Diese Methode benachrichtigt den Prozess bezüglich
   einer Änderung einer Auftragsvorgabe. Es kann z. B.
   eine neue Vorgabe erstellt worden sein, eine
   vorhandene Vorgabe kann sich geändert haben, oder es
   wurde eine Vorgabe gelöscht.

   Die Spooling-Datei enthält nur die ID des Auftrags. Der
   Prozess entscheidet damit selbstständig, was zu tun ist.

   \param dls_dir DLS-Datenverzeichnis
*/

void CTLJobPreset::spool(const string &dls_dir)
{
    fstream file;
    stringstream filename, err;
    struct timeval tv;

    gettimeofday(&tv, 0);

    // Eindeutigen Dateinamen erzeugen
    filename << dls_dir << "/spool/";
    filename << tv.tv_sec << "_" << tv.tv_usec;
    filename << "_" << (unsigned int) this;

    file.open(filename.str().c_str(), ios::out);

    if (!file.is_open())
    {
        err << "Could not write spooling file \"" << filename.str() << "\"!";
        throw ECOMJobPreset(err.str());
    }

    file << _id << endl;
    file.close();
}

/*****************************************************************************/

/**
   Setzt die Auftrags-ID
*/

void CTLJobPreset::id(unsigned int id)
{
    _id = id;
}

/*****************************************************************************/

/**
   Setzt die Auftragsbeschreibung
*/

void CTLJobPreset::description(const string &desc)
{
    _description = desc;
}

/*****************************************************************************/

/**
   Setzt den Sollstatus
*/

void CTLJobPreset::running(bool run)
{
    _running = run;
}

/*****************************************************************************/

/**
   Wechselt den Sollstatus
*/

void CTLJobPreset::toggle_running()
{
    _running = !_running;
}

/*****************************************************************************/

/**
   Setzt die Datenquelle
*/

void CTLJobPreset::source(const string &src)
{
    _source = src;
}

/*****************************************************************************/

/**
   Setzt den Namen des Trigger-Parameters
*/

void CTLJobPreset::trigger(const string &trigger_name)
{
    _trigger = trigger_name;
}

/*****************************************************************************/

/**
   Setzt die Größe der Zeit-Quota

   \param seconds Anzahl der maximal zu erfassenden Sekunden
*/

void CTLJobPreset::quota_time(long long seconds)
{
    _quota_time = seconds;
}

/*****************************************************************************/

/**
   Setzt die Größe der Daten-Quota

   \param bytes Anzahl der maximal zu erfassenden Bytes
*/

void CTLJobPreset::quota_size(long long bytes)
{
    _quota_size = bytes;
}

/*****************************************************************************/

/**
   Fügt eine Kanalvorgabe hinzu

   \param channel Neue Kanalvorgabe
   \throw ECOMJobPreset Eine Vorgabe für diesen Kanal
   existiert bereits!
*/

void CTLJobPreset::add_channel(const COMChannelPreset *channel)
{
    stringstream err;

    if (channel_exists(channel->name))
    {
        err << "Channel \"" << channel->name << "\" already exists!";
        throw ECOMJobPreset(err.str());
    }

    _channels.push_back(*channel);
}

/*****************************************************************************/

/**
   Ändert eine Kanalvorgabe

   Der zu ändernde Kanal wird anhand des Kanalnamens in der
   neuen Vorgabe bestimmt.

   \param new_channel Zeiger auf neue KanalvorgabeKanalname
   \throw ECOMJobPreset Es existiert keine Vorgabe für
   den angegebenen Kanal
*/

void CTLJobPreset::change_channel(const COMChannelPreset *new_channel)
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

    err << "Preset for channel \"" << new_channel->name << "\" doesn't exist!";
    throw ECOMJobPreset(err.str());
}

/*****************************************************************************/

/**
   Entfernt eine Kanalvorgabe

   \param channel_name Kanalname des Kanals, dessen Vorgabe
   entfernt werden soll
   \throw ECOMJobPreset Es existiert keine Vorgabe für
   den angegebenen Kanal
*/

void CTLJobPreset::remove_channel(const string &channel_name)
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

    err << "Preset for channel \"" << channel_name << "\" doesn't exist!";
    throw ECOMJobPreset(err.str());
}

/*****************************************************************************/
