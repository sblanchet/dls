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

#ifndef LibChannelHpp
#define LibChannelHpp

/*****************************************************************************/

#include <string>
#include <list>
using namespace std;

#include "lib_globals.hpp"
#include "com_exception.hpp"
#include "com_time.hpp"

#include "lib_chunk.hpp"

/*****************************************************************************/

namespace LibDLS {

    class Job;

    /*************************************************************************/

    /**
       Channel exception.
    */

    class ChannelException : public COMException
    {
    public:
        ChannelException(const string &pmsg) : COMException(pmsg) {};
    };

    /*************************************************************************/

    /**
       Darstellung eines Kanals in der Anzeige
    */

    class Channel
    {
    public:
        Channel();
        ~Channel();

        void import(const string &, unsigned int);
        void fetch_chunks();
        void fetch_data(COMTime, COMTime, unsigned int,
                        DataCallback, void *, unsigned int = 1) const;

        string path() const;
        unsigned int dir_index() const;

        const string &name() const;
        const string &unit() const;
        COMChannelType type() const;

        const list<LibDLS::Chunk> &chunks() const;
        bool has_same_chunks_as(const Channel &) const;

        COMTime start() const;
        COMTime end() const;

	bool operator<(const Channel &) const;

    private:
        string _path; /**< channel directory path */
        unsigned int _dir_index; /**< index of the channel directory */

        string _name; /**< channel name */
        string _unit; /**< channel unit */
        COMChannelType _type; /**< channel type */

        list<Chunk> _chunks; /**< list of chunks */
        COMTime _range_start; /**< start of channel data range */
        COMTime _range_end; /**< end of channel data range */
    };
}

/*****************************************************************************/

/**
   Returns the path of the channel directory.
*/

inline string LibDLS::Channel::path() const
{
    return _path;
}

/*****************************************************************************/

/**
   Returns the channel's directory index.
   \return channel name
*/

inline unsigned int LibDLS::Channel::dir_index() const
{
    return _dir_index;
}

/*****************************************************************************/

/**
   Returns the channel's name.
   \return channel name
*/

inline const string &LibDLS::Channel::name() const
{
    return _name;
}

/*****************************************************************************/

/**
   Returns the channel's unit.
   \return channel unit
*/

inline const string &LibDLS::Channel::unit() const
{
    return _unit;
}

/*****************************************************************************/

/**
   Returns the channel's data type.
   Data types are defined in com_globals.hpp.
   \return channel type
*/

inline COMChannelType LibDLS::Channel::type() const
{
    return _type;
}

/*****************************************************************************/

/**
   Returns the channel's chunk list.
   \return chunk list
*/

inline const list<LibDLS::Chunk> &LibDLS::Channel::chunks() const
{
    return _chunks;
}

/*****************************************************************************/

/**
   Returns the start time of the channel's first chunk.
   \return data range start
*/

inline COMTime LibDLS::Channel::start() const
{
    return _range_start;
}

/*****************************************************************************/

/**
   Returns the end time of the channel's last chunk.
   \return data range end
*/

inline COMTime LibDLS::Channel::end() const
{
    return _range_end;
}

/*****************************************************************************/

inline bool LibDLS::Channel::operator<(const Channel &right) const
{
    return _dir_index < right._dir_index;
}

/*****************************************************************************/

#endif
