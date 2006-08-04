/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef ViewChunkHpp
#define ViewChunkHpp

/*****************************************************************************/

#include "com_exception.hpp"
#include "com_time.hpp"
#include "com_index_t.hpp"
#include "com_file.hpp"
#include "view_data.hpp"

/*****************************************************************************/

class ViewChannel;
class ViewBlockList;
class ViewBlock;

/*****************************************************************************/

/**
   Exception eines ViewChunk-Objektes
*/

class EViewChunk : public COMException
{
public:
    EViewChunk(const string &pmsg) : COMException(pmsg) {};
};

/*****************************************************************************/

/**
   Chunk-Objekt zur Anzeige. Abstrakte Basisklasse für ViewChunkT
*/

class ViewChunk
{
public:
    ViewChunk();
    virtual ~ViewChunk();

    void set_dir(const string &);
    void import();
    void fetch_range();

    bool operator<(const ViewChunk &) const;

    virtual void fetch_data(COMTime, COMTime, unsigned int) = 0;
    virtual int export_data(COMTime, COMTime, ofstream &) const = 0;
    virtual void calc_min_max(double *, double *) const = 0;
    virtual unsigned int blocks_fetched() const = 0;
    virtual bool has_data() const = 0;

    virtual const ViewData *gen_data() const = 0;
    virtual const ViewData *min_data() const = 0;
    virtual const ViewData *max_data() const = 0;

    COMTime start() const;
    COMTime end() const;
    int format_index() const;
    unsigned int mdct_block_size() const;
    unsigned int current_level() const;

protected:
    string _dir;                    /**< Chunk-Verzeichnis */
    unsigned int _sample_frequency; /**< Abtastfrequenz */
    unsigned int _meta_reduction;   /**< Meta-Untersetzung */
    int _format_index;              /**< Kompressionsformat */
    unsigned int _mdct_block_size;  /**< MDCT-Blockgröße */
    COMTime _start;                 /**< Startzeit des Chunks */
    COMTime _end;                   /**< Endzeit des Chunks */
    unsigned int _level;            /**< Level, in dem Daten geladen wurden */

    void _calc_optimal_level(COMTime, COMTime, unsigned int);
    double _time_per_value() const;
};

/*****************************************************************************/

/**
   Liefert die Startzeit des Chunks

   \return Startzeit
*/

inline COMTime ViewChunk::start() const
{
    return _start;
}

/*****************************************************************************/

/**
   Liefert die Endzeit des Chunks

   \return Endzeit
*/

inline COMTime ViewChunk::end() const
{
    return _end;
}

/*****************************************************************************/

/**
   Liefert die Meta-Ebene, aus der die aktuellen Daten geladen wurden

   \return Meta-Ebene
*/

inline unsigned int ViewChunk::current_level() const
{
    return _level;
}

/*****************************************************************************/

/**
   Liefert das Format, in dem die Daten geladen werden sollen

   \return Format-Index
*/

inline int ViewChunk::format_index() const
{
    return _format_index;
}

/*****************************************************************************/

/**
   Gibt die MDCT-Blockgröße zurück

   Liefert nur einen vernünftigen Wert, wenn das Format
   auch DLS_FORMAT_MDCT ist

   \return MDCT-Blockgröße ("Dimension")
*/

inline unsigned int ViewChunk::mdct_block_size() const
{
    return _mdct_block_size;
}

/*****************************************************************************/

#endif
