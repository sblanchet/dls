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

#include "LibDLS/globals.h"
#include "LibDLS/Chunk.h"
#include "LibDLS/Job.h"
#include "LibDLS/Channel.h"

#include "proto/dls.pb.h"

#include "XmlParser.h"
using namespace LibDLS;

#define DEBUG_TIMING 0

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

Channel::Channel(Job *job, const DlsProto::ChannelInfo &info):
    _job(job),
    _dir_index(info.id()),
    _name(info.name()),
    _unit(info.unit()),
    _type((ChannelType) info.type())
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
    XmlParser xml;

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

        if ((_type = str_to_channel_type(xml.tag()->att("type")->to_str()))
            == TUNKNOWN) {
            stringstream err;
            file.close();
            err << "Unknown channel type \""
                << xml.tag()->att("type")->to_str() << "\".";
            throw ChannelException(err.str());
        }

        xml.parse(&file, "dlschannel", dxttEnd);
    }
    catch (EXmlParser &e) {
        stringstream err;
        file.close();
        err << "Parse error in " << file_name << ": " << e.msg;
        throw ChannelException(err.str());
    }
    catch (EXmlParserEOF &e) {
        stringstream err;
        file.close();
        err << "Parse error in " << file_name << ": " << e.msg;
        throw ChannelException(err.str());
    }
    catch (EXmlTag &e) {
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
    Chunk new_chunk, *chunk;
    bool first = true;
    int64_t dir_time;

#if DEBUG_TIMING
    Time ts, te;
    ts.set_now();
#endif

    _range_start.set_null();
    _range_end.set_null();

    if (!(dir = opendir(path().c_str()))) {
        stringstream err;
        err << "Failed to open \"" << path() << "\".";
        throw ChannelException(err.str());
    }

    /* TODO: Delete chunks, if no existing any more. */

    while ((dir_ent = readdir(dir))) {
        dir_ent_name = dir_ent->d_name;
        if (dir_ent_name.find("chunk") != 0) {
            continue;
        }

        // get chunk time from directory name for map indexing
        stringstream str;
        str << dir_ent_name.substr(5);
        str >> dir_time;

        ChunkMap::iterator chunk_i = _chunks.find(dir_time);
        if (chunk_i == _chunks.end()) {
            try {
                new_chunk.import(path() + "/" + dir_ent_name, _type);
            }
            catch (ChunkException &e) {
                stringstream err;
                err << "WARNING: Failed import chunk: " << e.msg;
                log(err.str());
                continue;
            }

            pair<int64_t, Chunk> val(dir_time, new_chunk);
            pair<ChunkMap::iterator, bool> ret = _chunks.insert(val);
            chunk = &ret.first->second;
        }
        else {
            // chunk existing
            chunk = &chunk_i->second;
        }

        if (chunk->incomplete()) {
            // chunk is still logging, fetch current end time
            try {
                chunk->fetch_range();
            }
            catch (ChunkException &e) {
                stringstream err;
                err << "WARNING: Failed to fetch chunk range: " << e.msg;
                log(err.str());
                continue;
            }
        }

        if (first) {
            _range_start = chunk->start();
            _range_end = chunk->end();
            first = false;
        }
        else {
            if (chunk->start() < _range_start) {
                _range_start = chunk->start();
            }
            if (chunk->end() > _range_end) {
                _range_end = chunk->end();
            }
        }
    }

    closedir(dir);

#if DEBUG_TIMING
    te.set_now();
    stringstream msg;
    msg << "fetch_chunks " << ts.diff_str_to(te);
    log(msg.str());
#endif
}

/*****************************************************************************/

/**
   Loads data values of the specified time range and resolution.

   If \a min_values is non-zero, the smallest resolution is chosen, that
   contains at least \a min_values for the requested range.

   Otherwise, all values (the maximum resolution) is loaded.

   The data are passed via the callback function.
*/

void Channel::fetch_data(Time start, /**< start of requested time range */
                         Time end, /**< end of requested time range */
                         unsigned int min_values, /**< minimal number */
                         DataCallback cb, /**< callback */
                         void *cb_data, /**< arbitrary callback parameter */
                         unsigned int decimation /**< Decimation. */
                         ) const
{
#if DEBUG_TIMING
    Time ts, te;
    ts.set_now();
#endif

    ChunkMap::const_iterator chunk_i;
    RingBuffer ring(100000);

    if (start < end) {
        for (chunk_i = _chunks.begin(); chunk_i != _chunks.end(); chunk_i++) {
            chunk_i->second.fetch_data(start, end,
                    min_values, &ring, cb, cb_data, decimation);
        }
    }

#if DEBUG_TIMING
    te.set_now();
    stringstream msg;
    msg << "fetch_data " << ts.diff_str_to(te);
    log(msg.str());
#endif
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

void Channel::set_channel_info(DlsProto::ChannelInfo *ch_info) const
{
    ch_info->set_id(_dir_index);
    ch_info->set_name(_name);
    ch_info->set_unit(_unit);
    ch_info->set_type((DlsProto::ChannelType) _type);
}

/*****************************************************************************/
