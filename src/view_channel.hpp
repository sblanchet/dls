/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef ViewChannelHpp
#define ViewChannelHpp

/*****************************************************************************/

#include <string>
#include <list>
using namespace std;

#include "com_exception.hpp"
#include "com_time.hpp"
#include "com_ring_buffer_t.hpp"

#include "lib_channel.hpp"
using namespace LibDLS;

#include "view_globals.hpp"

/*****************************************************************************/

/**
   Union of a channel and loaded data.
*/

class ViewChannel
{
    friend int data_callback(Data *, void *);

public:
    ViewChannel();
    ~ViewChannel();

    void set_channel(Channel *);
    void fetch_data(COMTime, COMTime, unsigned int);
    Channel *channel();

    const Channel *channel() const;
    const list<Data> &gen_data() const;
    const list<Data> &min_data() const;
    const list<Data> &max_data() const;

    double min() const;
    double max() const;
    unsigned int min_level() const;
    unsigned int max_level() const;

    bool operator<(const ViewChannel &) const;

private:
    Channel *_channel;
    list<Data> _gen_data;
    list<Data> _min_data;
    list<Data> _max_data;

    double _min;
    double _max;
    unsigned int _min_level;
    unsigned int _max_level;

    void _calc_min_max();
    void _calc_min_max_data(const list<Data> &, bool *);
};

/*****************************************************************************/

/**
*/

inline Channel *ViewChannel::channel()
{
    return _channel;
}

/*****************************************************************************/

/**
*/

inline const Channel *ViewChannel::channel() const
{
    return _channel;
}

/*****************************************************************************/

/**
*/

inline const list<Data> &ViewChannel::gen_data() const
{
    return _gen_data;
}

/*****************************************************************************/

/**
*/

inline const list<Data> &ViewChannel::min_data() const
{
    return _min_data;
}

/*****************************************************************************/

/**
*/

inline const list<Data> &ViewChannel::max_data() const
{
    return _max_data;
}

/*****************************************************************************/

/**
   Gibt den kleinsten, geladenen Wert zurück

   \return Kleinster Wert
*/

inline double ViewChannel::min() const
{
    return _min;
}

/*****************************************************************************/

/**
   Gibt den größten, geladenen Wert zurück

   \return Größter Wert
*/

inline double ViewChannel::max() const
{
    return _max;
}

/*****************************************************************************/

/**
   Gibt die niedrigste Meta-Ebene zurück, aus der geladen wurde

   \return Meta-Ebene
*/

inline unsigned int ViewChannel::min_level() const
{
    return _min_level;
}

/*****************************************************************************/

/**
   Gibt die höchste Meta-Ebene zurück, aus der geladen wurde

   \return Meta-Ebene
*/

inline unsigned int ViewChannel::max_level() const
{
    return _max_level;
}

/*****************************************************************************/

#endif
