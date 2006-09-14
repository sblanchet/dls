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
#include "lib_chunk.hpp"
#include "lib_job.hpp"
#include "lib_channel.hpp"
using namespace LibDLS;

/*****************************************************************************/

/**
   Constructor.
*/

Channel::Channel()
{
    _index = 0;
}

/*****************************************************************************/

/**
   Destructor.
*/

Channel::~Channel()
{
    _chunks.clear();
}

/*****************************************************************************/

/**
   Imports channel information.
*/

void Channel::import(const string &job_path, /**< job directory path */
                     unsigned int index /**< channel index */
                     )
{
    stringstream channel_dir;
    string channel_info_file_name;
    fstream file;
    COMXMLParser xml;

    channel_dir << job_path << "/channel" << index;

    _path = channel_dir.str();
    _index = index;

    channel_info_file_name = _path + "/channel.xml";
    file.open(channel_info_file_name.c_str(), ios::in);

    if (!file.is_open()) {
        stringstream err;
        err << "Failed to open channel file \""
            << channel_info_file_name << "\".";
        throw ChannelException(err.str());
    }

    try
    {
        xml.parse(&file, "dlschannel", dxttBegin);
        xml.parse(&file, "channel", dxttSingle);

        _name = xml.tag()->att("name")->to_str();
        _unit = xml.tag()->att("unit")->to_str();

        if ((_type = dls_str_to_channel_type(xml.tag()->att("type")->to_str()))
            == TUNKNOWN) {
            stringstream err;
            file.close();
            err << "Unknown channel type \""
                << xml.tag()->att("type")->to_str() << "\".";
            throw ChannelException(err.str());
        }

        xml.parse(&file, "dlschannel", dxttEnd);
    }
    catch (ECOMXMLParser &e) {
        stringstream err;
        file.close();
        err << "Channel " << _index << " parsing error: " << e.msg;
        throw ChannelException(err.str());
    }
    catch (ECOMXMLParserEOF &e) {
        stringstream err;
        file.close();
        err << "Channel " << _index << " parsing error: " << e.msg;
        throw ChannelException(err.str());
    }
    catch (ECOMXMLTag &e) {
        stringstream err;
        file.close();
        err << "Channel " << _index << " parsing (tag) error: " << e.msg;
        throw ChannelException(err.str());
    }

    file.close();
}

/*****************************************************************************/

/**
   Lädt die Liste der Chunks und importiert deren Eigenschaften

   \param dls_dir DLS-Datenverzeichnis
   \param job_id Auftrags-ID
*/

void Channel::fetch_chunks()
{
    DIR *dir;
    struct dirent *dir_ent;
    string dir_ent_name;
    Chunk chunk;
    bool first = true;

    _range_start.set_null();
    _range_end.set_null();
    _chunks.clear();

    if (!(dir = opendir(path().c_str()))) {
        stringstream err;
        err << "Failed to open \"" << path() << "\".";
        throw ChannelException(err.str());
    }

    while ((dir_ent = readdir(dir))) {
        dir_ent_name = dir_ent->d_name;
        if (dir_ent_name.find("chunk") != 0) continue;

        try {
            chunk.import(path() + "/" + dir_ent_name, _type);
        }
        catch (ChunkException &e) {
            cerr << "WARNING: Failed import chunk: " << e.msg << endl;
            continue;
        }

        try {
            // Start- und Endzeiten holen
            chunk.fetch_range();
        }
        catch (ChunkException &e) {
            cerr << "WARNING: Failed to fetch chunk range: " << e.msg << endl;
            continue;
        }

        if (first) {
            _range_start = chunk.start();
            _range_end = chunk.end();
            first = false;
        }
        else {
            if (chunk.start() < _range_start) _range_start = chunk.start();
            if (chunk.end() > _range_end) _range_end = chunk.end();
        }

        _chunks.push_back(chunk);
    }

    closedir(dir);

    // Chunks aufsteigend nach Anfangszeit sortieren
    _chunks.sort();
}

/*****************************************************************************/

/**
   Loads data values of the specified time range and resolution.

   If \a min_values is non-zero, the smallest resolution is chosen, that
   contains at least \a min_values for the requested range.

   Otherwise, all values (the maximum resolution) is loaded.

   The data are passed via the callback function.
*/

void Channel::fetch_data(COMTime start, /**< start of requested time range */
                         COMTime end, /**< end of requested time range */
                         unsigned int min_values, /**< minimal number */
                         DataCallback cb, /**< callback */
                         void *cb_data /**< arbitrary callback parameter */
                         ) const
{
    list<Chunk>::const_iterator chunk_i;
    COMRingBuffer ring(100000);

    if (start >= end) return;

    for (chunk_i = _chunks.begin(); chunk_i != _chunks.end(); chunk_i++) {
        chunk_i->fetch_data(start, end, min_values, &ring, cb, cb_data);
    }
}

/*****************************************************************************/

/**
   Returns true, if this channel has exactly the same chunk times
   as the other channel.
   \return true, if channels have the same chunks.
*/

bool Channel::has_same_chunks_as(const Channel &other) const
{
    return _chunks == other._chunks;
}

/*****************************************************************************/
