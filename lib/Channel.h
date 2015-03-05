/*****************************************************************************
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
 ****************************************************************************/

#ifndef LibDLSChannelH
#define LibDLSChannelH

/****************************************************************************/

#include <string>
#include <map>
using namespace std;

#include "Exception.h"
#include "Time.h"

#include "Chunk.h"

/****************************************************************************/

namespace LibDLS {

/****************************************************************************/

class Job;

/****************************************************************************/

/**
   Channel exception.
*/

class ChannelException: public Exception
{
public:
    ChannelException(const string &pmsg): Exception(pmsg) {};
};

/****************************************************************************/


/**
   Darstellung eines Kanals in der Anzeige
*/

class Channel
{
public:
    Channel(Job *);
    ~Channel();

    Job *getJob() const { return _job; }

    void import(const string &, unsigned int);
    void fetch_chunks();
    void fetch_data(Time, Time, unsigned int,
                    DataCallback, void *, unsigned int = 1) const;

    string path() const;
    unsigned int dir_index() const;

    const string &name() const;
    const string &unit() const;
    ChannelType type() const;

    typedef map<int64_t, Chunk> ChunkMap;
    const ChunkMap &chunks() const;
    bool has_same_chunks_as(const Channel &) const;

    Time start() const;
    Time end() const;

    bool operator<(const Channel &) const;

private:
    Job * const _job; /**< Parent job. */
    string _path; /**< channel directory path */
    unsigned int _dir_index; /**< index of the channel directory */

    string _name; /**< channel name */
    string _unit; /**< channel unit */
    ChannelType _type; /**< channel type */

    ChunkMap _chunks; /**< list of chunks */
    Time _range_start; /**< start of channel data range */
    Time _range_end; /**< end of channel data range */

    Channel();
};

/****************************************************************************/

/**
   Returns the path of the channel directory.
*/

inline string Channel::path() const
{
    return _path;
}

/****************************************************************************/

/**
   Returns the channel's directory index.
   \return channel name
*/

inline unsigned int Channel::dir_index() const
{
    return _dir_index;
}

/****************************************************************************/

/**
   Returns the channel's name.
   \return channel name
*/

inline const string &Channel::name() const
{
    return _name;
}

/****************************************************************************/

/**
   Returns the channel's unit.
   \return channel unit
*/

inline const string &LibDLS::Channel::unit() const
{
    return _unit;
}

/****************************************************************************/

/**
   Returns the channel's data type.
   Data types are defined in globals.h.
   \return channel type
*/

inline ChannelType LibDLS::Channel::type() const
{
    return _type;
}

/****************************************************************************/

/**
   Returns the channel's chunk list.
   \return chunk list
*/

inline const LibDLS::Channel::ChunkMap &LibDLS::Channel::chunks() const
{
    return _chunks;
}

/****************************************************************************/

/**
   Returns the start time of the channel's first chunk.
   \return data range start
*/

inline Time LibDLS::Channel::start() const
{
    return _range_start;
}

/****************************************************************************/

/**
   Returns the end time of the channel's last chunk.
   \return data range end
*/

inline Time LibDLS::Channel::end() const
{
    return _range_end;
}

/****************************************************************************/

inline bool LibDLS::Channel::operator<(const Channel &right) const
{
    return _dir_index < right._dir_index;
}

/****************************************************************************/

} // namespace

/****************************************************************************/

#endif
