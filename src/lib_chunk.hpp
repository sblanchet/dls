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

#ifndef LibChunkHpp
#define LibChunkHpp

/*****************************************************************************/

#include "com_globals.hpp"
#include "com_exception.hpp"
#include "com_time.hpp"
#include "com_ring_buffer_t.hpp"
#include "com_xml_tag.hpp"
#include "com_compression_t.hpp"

#include "lib_data.hpp"

namespace LibDLS
{
    class Channel;
    class Data;

    /** Data callback.
     *
     * \return non-zero, if the Data object is adopted. In this case, the
     * caller has to delete the object.
     */
    typedef int (*DataCallback)(Data *, void *);

    /*************************************************************************/

    /**
       Chunk Exception.
    */

    class ChunkException : public COMException
    {
    public:
        ChunkException(const string &pmsg) : COMException(pmsg) {};
    };

    /*************************************************************************/

    /**
       Chunk class.
    */

    class Chunk
    {
    public:
        Chunk();
        virtual ~Chunk();

        void import(const string &, COMChannelType);
        void fetch_range();

        COMTime start() const;
        COMTime end() const;
        void fetch_data(COMTime, COMTime, unsigned int,
                        COMRingBuffer *,
                        DataCallback, void *,
                        unsigned int) const;

        bool operator<(const Chunk &) const;
        bool operator==(const Chunk &) const;

    protected:
        string _dir;                    /**< Chunk-Verzeichnis */
        double _sample_frequency;       /**< Abtastfrequenz */
        unsigned int _meta_reduction;   /**< Meta-Untersetzung */
        int _format_index;              /**< Kompressionsformat */
        unsigned int _mdct_block_size;  /**< MDCT-Blockgroesse */
        COMTime _start;                 /**< Startzeit des Chunks */
        COMTime _end;                   /**< Endzeit des Chunks */
        COMChannelType _type; /**< channel type */

        unsigned int _calc_optimal_level(COMTime, COMTime, unsigned int) const;
        COMTime _time_per_value(unsigned int) const;

        void _fetch_level_data_wrapper(COMTime, COMTime,
                                       DLSMetaType,
                                       unsigned int,
                                       COMTime,
                                       COMRingBuffer *,
                                       Data **,
                                       DataCallback,
                                       void *,
                                       unsigned int,
                                       unsigned int &) const;

        template <class T>
        void _fetch_level_data(COMTime, COMTime,
                               DLSMetaType,
                               unsigned int,
                               COMTime,
                               COMRingBuffer *,
                               Data **,
                               DataCallback,
                               void *,
                               unsigned int,
                               unsigned int &) const;

        template <class T>
        void _process_data_tag(const COMXMLTag *,
                               COMTime,
                               DLSMetaType,
                               unsigned int,
                               COMTime,
                               COMCompressionT<T> *,
                               Data **,
                               DataCallback,
                               void *,
                               unsigned int,
                               unsigned int &) const;
    };
}

/*****************************************************************************/

/**
   Returns the chunk's start time.
   \return start time
*/

inline COMTime LibDLS::Chunk::start() const
{
    return _start;
}

/*****************************************************************************/

/**
   Returns the chunk's end time.
   \return end time
*/

inline COMTime LibDLS::Chunk::end() const
{
    return _end;
}

/*****************************************************************************/

#endif
