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

#include "lib_globals.hpp"
#include "com_exception.hpp"
#include "com_time.hpp"
#include "lib_data.hpp"

template <class TYPE, class SIZE> class COMRingBufferT;
typedef class COMRingBufferT<char, unsigned int> COMRingBuffer;
class COMXMLTag;
template <class T> class COMCompressionT;

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

        COMTime start() const { return _start; }
        COMTime end() const { return _end; }
        bool incomplete() const { return _incomplete; }

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
        bool _incomplete; /**< Data ist still logged. */

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

#endif
