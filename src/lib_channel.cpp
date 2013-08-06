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

#include <fcntl.h>
#include <dirent.h>

#include <iostream>
#include <sstream>
#include <fstream>
using namespace std;

/*****************************************************************************/

#include "com_globals.hpp"
#include "lib_chunk.hpp"
#include "lib_job.hpp"
#include "lib_channel.hpp"
#include "com_xml_parser.hpp"
using namespace LibDLS;

/*****************************************************************************/

/**
   Constructor.
*/

Channel::Channel(Job *job):
    _job(job),
    _dir_index(0)
{
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

void Channel::import(
        const string &channel_path, /**< channel directory path */
        unsigned int dir_index /**< index of the channel directory */
        )
{
    string file_name;
    fstream file;
    COMXMLParser xml;

    _path = channel_path;
    _dir_index = dir_index;

    file_name = _path + "/channel.xml";
    file.open(file_name.c_str(), ios::in);

    if (!file.is_open()) {
        stringstream err;
        err << "Failed to open channel file \"" << file_name << "\".";
        throw ChannelException(err.str());
    }

    try
    {
        xml.parse(&file, "dlschannel", dxttBegin);
        xml.parse(&file, "channel", dxttSingle);

        _name = xml.tag()->att("name")->to_str();
        _unit = xml.tag()->att("unit")->to_str();

        if ((_type = dls_str_to_channel_type(xml.tag()->att("type")->to_str()))
            == DLS_TUNKNOWN) {
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
        err << "Parse error in " << file_name << ": " << e.msg;
        throw ChannelException(err.str());
    }
    catch (ECOMXMLParserEOF &e) {
        stringstream err;
        file.close();
        err << "Parse error in " << file_name << ": " << e.msg;
        throw ChannelException(err.str());
    }
    catch (ECOMXMLTag &e) {
        stringstream err;
        file.close();
        err << "XML tag parse error in " << file_name << ": " << e.msg;
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
			stringstream err;
            err << "WARNING: Failed import chunk: " << e.msg;
			dls_log(err.str());
            continue;
        }

        try {
            // Start- und Endzeiten holen
            chunk.fetch_range();
        }
        catch (ChunkException &e) {
			stringstream err;
            err << "WARNING: Failed to fetch chunk range: " << e.msg;
			dls_log(err.str());
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
                         void *cb_data, /**< arbitrary callback parameter */
                         unsigned int decimation /**< Decimation. */
                         ) const
{
    list<Chunk>::const_iterator chunk_i;
    COMRingBuffer ring(100000);

    if (start >= end) return;

    for (chunk_i = _chunks.begin(); chunk_i != _chunks.end(); chunk_i++) {
        chunk_i->fetch_data(start, end, min_values, &ring, cb, cb_data,
                decimation);
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
