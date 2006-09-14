/******************************************************************************
 *
 *  $Id$
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
                        DataCallback, void *) const;

        bool operator<(const Chunk &) const;
        bool operator==(const Chunk &) const;

    protected:
        string _dir;                    /**< Chunk-Verzeichnis */
        unsigned int _sample_frequency; /**< Abtastfrequenz */
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
                                       void *) const;

        template <class T>
        void _fetch_level_data(COMTime, COMTime,
                               DLSMetaType,
                               unsigned int,
                               COMTime,
                               COMRingBuffer *,
                               Data **,
                               DataCallback,
                               void *) const;

        template <class T>
        void _process_data_tag(const COMXMLTag *,
                               COMTime,
                               DLSMetaType,
                               unsigned int,
                               COMTime,
                               COMCompressionT<T> *,
                               Data **,
                               DataCallback,
                               void *) const;
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
