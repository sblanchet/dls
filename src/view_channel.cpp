/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <fcntl.h>
#include <dirent.h>

#include <iostream>
#include <sstream>
#include <fstream>
using namespace std;

/*****************************************************************************/

#include "com_xml_parser.hpp"
#include "view_globals.hpp"
#include "view_chunk_t.hpp"
#include "view_channel.hpp"

//#define DEBUG

/*****************************************************************************/

/**
   Konstruktor
*/

ViewChannel::ViewChannel()
{
    _index = 0;
    _min_level = 0;
    _max_level = 0;
}

/*****************************************************************************/

/**
   Destruktor
*/

ViewChannel::~ViewChannel()
{
    clear();
}

/*****************************************************************************/

/**
   Importiert die Kanalinformationen aus "channel.xml"

   \param dls_dir DLS-Datenverzeichnis
   \param job_id Auftrags-ID
   \param channel_id Kanal-Index
   \throw EViewChannel Kanalinformationen konnten
   nicht importiert werden
*/

void ViewChannel::import(const string &dls_dir,
                         unsigned int job_id,
                         unsigned int channel_id)
{
    stringstream channel_dir_name, err;
    string channel_file_name;
    fstream file;
    COMXMLParser xml;

    _index = channel_id;

    channel_dir_name << dls_dir;
    channel_dir_name << "/job" << job_id;
    channel_dir_name << "/channel" << _index;

    channel_file_name = channel_dir_name.str() + "/channel.xml";

    file.open(channel_file_name.c_str(), ios::in);

    if (!file.is_open())
    {
        err << "Could not open channel file \"" << channel_file_name << "\"!";
        throw EViewChannel(err.str());
    }

    try
    {
        xml.parse(&file, "dlschannel", dxttBegin);
        xml.parse(&file, "channel", dxttSingle);

        _name = xml.tag()->att("name")->to_str();
        _unit = xml.tag()->att("unit")->to_str();

        if ((_type = dls_str_to_channel_type(
                 xml.tag()->att("type")->to_str())) == TUNKNOWN)
        {
            file.close();
            err << "Unknown channel type \""
                << xml.tag()->att("type")->to_str() << "\"!";
            throw EViewChannel(err.str());
        }

        xml.parse(&file, "dlschannel", dxttEnd);
    }
    catch (ECOMXMLParser &e)
    {
        file.close();
        err << "Channel " << _index << " parsing error: " << e.msg;
        throw EViewChannel(err.str());
    }
    catch (ECOMXMLParserEOF &e)
    {
        file.close();
        err << "Channel " << _index << " parsing error: " << e.msg;
        throw EViewChannel(err.str());
    }
    catch (ECOMXMLTag &e)
    {
        file.close();
        err << "Channel " << _index << " parsing (tag) error: " << e.msg;
        throw EViewChannel(err.str());
    }

    file.close();
}

/*****************************************************************************/

/**
   Lädt die Liste der Chunks und importiert deren Eigenschaften

   \param dls_dir DLS-Datenverzeichnis
   \param job_id Auftrags-ID
*/

void ViewChannel::fetch_chunks(const string &dls_dir, unsigned int job_id)
{
    stringstream channel_dir_name;
    DIR *dir;
    struct dirent *dir_ent;
    string dir_ent_name;
    ViewChunk *chunk;
    bool first = true;

    _range_start = (long long) 0;
    _range_end = (long long) 0;

    // Alle bisherigen Chunks entfernen
    clear();

    // Kanal-Verzeichnisnamen konstruieren
    channel_dir_name << dls_dir << "/job" << job_id << "/channel" << _index;

    // Kanalverzeichnislisting öffnen
    if ((dir = opendir(channel_dir_name.str().c_str())) == NULL)
    {
        cout << "ERROR: could not open \""
             << channel_dir_name.str() << "\"." << endl;
        return;
    }

    // Alle Einträge im Kanalverzeichnis durchforsten
    while ((dir_ent = readdir(dir)) != NULL)
    {
        dir_ent_name = dir_ent->d_name;

        // Es interessieren nur die Einträge, die mit "chunk" beginnen
        if (dir_ent_name.find("chunk") != 0) continue;

        try
        {
            switch (_type)
            {
                case TCHAR:
                    chunk = new ViewChunkT<char>();
                    break;
                case TUCHAR:
                    chunk = new ViewChunkT<unsigned char>();
                    break;
                case TSHORT:
                    chunk = new ViewChunkT<short int>();
                    break;
                case TUSHORT:
                    chunk = new ViewChunkT<unsigned short int>();
                    break;
                case TINT:
                    chunk = new ViewChunkT<int>();
                    break;
                case TUINT:
                    chunk = new ViewChunkT<unsigned int>();
                    break;
                case TLINT:
                    chunk = new ViewChunkT<long>();
                    break;
                case TULINT:
                    chunk = new ViewChunkT<unsigned long>();
                    break;
                case TFLT:
                    chunk = new ViewChunkT<float>();
                    break;
                case TDBL:
                    chunk = new ViewChunkT<double>();
                    break;

                default:
                    closedir(dir);
                    cout << "ERROR: Unknown channel type " << _type
                         << "!" << endl;
                    return;
            }
        }
        catch (...)
        {
            closedir(dir);
            cout << "ERROR: Could no allocate memory for chunk object!"
                 << endl;
            return;
        }

        // Chunk-Verzeichnis setzen
        chunk->set_dir(channel_dir_name.str() + "/" + dir_ent_name);

        try
        {
            chunk->import();
        }
        catch (EViewChunk &e)
        {
            cout << "WARNING: could not import chunk: " << e.msg << endl;
            continue;
        }

        try
        {
            // Start- und Endzeiten holen
            chunk->fetch_range();
        }
        catch (EViewChunk &e)
        {
            cout << "WARNING: could not fetch range: " << e.msg << endl;
            continue;
        }

        // Minimal- und Maximalzeiten mitnehmen
        if (first)
        {
            _range_start = chunk->start();
            _range_end = chunk->end();
            first = false;
        }
        else
        {
            if (chunk->start() < _range_start) _range_start = chunk->start();
            if (chunk->end() > _range_end) _range_end = chunk->end();
        }

        // Chunk in die Liste einfügen
        _chunks.push_back(chunk);
    }

    closedir(dir);
}

/*****************************************************************************/

/**
   Lädt die Daten zu einer Zeitspanne und Auflösung

   Die Auflösung wird implizit durch die Angabe
   der Breite der Anzeige in Pixeln angegeben. Es wird
   die kleinste Auflösung gewählt, die für diese Zeitspanne
   mindestens so viele Werte liefert.

   \param start Anfang der Zeitspanne
   \param end Ende der Zeitspanne
   \param values_wanted Optimale Anzahl Datenwerte
*/

void ViewChannel::load_data(COMTime start,
                            COMTime end,
                            unsigned int values_wanted)
{
    list<ViewChunk *>::iterator chunk_i;
    bool first = true;
    unsigned int level;

    _min_level = 0;
    _max_level = 0;

    if (start >= end) return;

    chunk_i = _chunks.begin();
    while (chunk_i != _chunks.end())
    {
        // Daten laden
        (*chunk_i)->fetch_data(start, end, values_wanted);

        // Geladenen Level ermitteln
        level = (*chunk_i)->current_level();

        if (first)
        {
            first = false;
            _min_level = level;
            _max_level = level;
        }
        else
        {
            if (level < _min_level) _min_level = level;
            if (level > _max_level) _max_level = level;
        }

        chunk_i++;
    }

    // Wertespanne errechnen
    _calc_min_max();
}

/*****************************************************************************/

/**
   Exportiert Daten einer Zeitspanne.
*/

int ViewChannel::export_data(COMTime start, /**< Anfang der Zeitspanne */
                             COMTime end, /**< Ende der Zeitspanne */
                             const string &path /**< Export-Pfad */
                             ) const
{
    ofstream file;
    stringstream filename;
    list<ViewChunk *>::const_iterator chunk_i;

    if (start >= end) return 1;

    filename << path << "/channel" << _index << ".dat";
    file.open(filename.str().c_str(), ios::trunc);

    if (!file.is_open()) {
        cerr << "Failed to open file \"" << filename.str() << "\"!" << endl;
        return 1;
    }

    file << "% --- DLS exported data ---" << endl;
    file << "%" << endl;
    file << "% Channel: " << _name << endl;
    file << "%    Unit: " << _unit << endl;
    file << "%" << endl;

    for (chunk_i = _chunks.begin(); chunk_i != _chunks.end(); chunk_i++) {
        if ((*chunk_i)->export_data(start, end, file)) {
            cerr << "Failed to export chunk for channel " << _name << endl;
            file.close();
            return 1;
        }
    }

    file.close();
    return 0;
}

/*****************************************************************************/

/**
   Entfernt alle Chunks
*/

void ViewChannel::clear()
{
    list<ViewChunk *>::iterator chunk_i;

    chunk_i = _chunks.begin();
    while (chunk_i != _chunks.end())
    {
        delete *chunk_i;
        chunk_i++;
    }

    _chunks.clear();
}

/*****************************************************************************/

/**
   Berechnet die Extrema der geladenen Daten
*/

void ViewChannel::_calc_min_max()
{
    list<ViewChunk *>::const_iterator chunk_i;
    double min, max;
    bool first = true;

    _min = 0;
    _max = 0;

    chunk_i = _chunks.begin();
    while (chunk_i != _chunks.end())
    {
        if ((*chunk_i)->has_data())
        {
            (*chunk_i)->calc_min_max(&min, &max);

            if (first)
            {
                _min = min;
                _max = max;
                first = false;
            }
            else
            {
                if (min < _min) _min = min;
                if (max > _max) _max = max;
            }
        }

#if DEBUG_VIEW_CHANNEL
        cout << "min: " << _min << " max: " << _max << endl;
#endif

        chunk_i++;
    }
}

/*****************************************************************************/

/**
   Gibt die Anzahl der geladenen Blöcke zurück

   \return Geladenene Blöcke
*/

unsigned int ViewChannel::blocks_fetched() const
{
    unsigned int blocks;
    list<ViewChunk *>::const_iterator chunk_i;

    blocks = 0;

    chunk_i = _chunks.begin();
    while (chunk_i != _chunks.end())
    {
        blocks += (*chunk_i)->blocks_fetched();

        chunk_i++;
    }

    return blocks;
}

/*****************************************************************************/
