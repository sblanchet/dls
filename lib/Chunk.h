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

#ifndef LibDLSChunkH
#define LibDLSChunkH

/*****************************************************************************/

#include "globals.h"
#include "Exception.h"
#include "Time.h"
#include "Data.h"

/*****************************************************************************/

namespace LibDLS {

/*****************************************************************************/

template <class TYPE, class SIZE> class RingBufferT;
typedef class RingBufferT<char, unsigned int> RingBuffer;
class XmlTag;
template <class T> class CompressionT;
class Channel;
class Data;

/*****************************************************************************/

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

class ChunkException : public Exception
{
public:
    ChunkException(const string &pmsg) : Exception(pmsg) {};
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

    void import(const string &, ChannelType);
    void fetch_range();

    Time start() const { return _start; }
    Time end() const { return _end; }
    bool incomplete() const { return _incomplete; }

    void fetch_data(Time, Time, unsigned int,
                    RingBuffer *,
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
    Time _start;                 /**< Startzeit des Chunks */
    Time _end;                   /**< Endzeit des Chunks */
    ChannelType _type; /**< channel type */
    bool _incomplete; /**< Data ist still logged. */

    unsigned int _calc_optimal_level(Time, Time, unsigned int) const;
    Time _time_per_value(unsigned int) const;

    void _fetch_level_data_wrapper(Time, Time,
                                   MetaType,
                                   unsigned int,
                                   Time,
                                   RingBuffer *,
                                   Data **,
                                   DataCallback,
                                   void *,
                                   unsigned int,
                                   unsigned int &) const;

    template <class T>
    void _fetch_level_data(Time, Time,
                           MetaType,
                           unsigned int,
                           Time,
                           RingBuffer *,
                           Data **,
                           DataCallback,
                           void *,
                           unsigned int,
                           unsigned int &) const;

    template <class T>
    void _process_data_tag(const XmlTag *,
                           Time,
                           MetaType,
                           unsigned int,
                           Time,
                           CompressionT<T> *,
                           Data **,
                           DataCallback,
                           void *,
                           unsigned int,
                           unsigned int &) const;
};

/*****************************************************************************/

} // namespace

/*****************************************************************************/

#endif
