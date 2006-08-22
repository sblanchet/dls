/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef LibDataHpp
#define LibDataHpp

/*****************************************************************************/

#include <iostream>
#include <vector>
using namespace std;

#include "com_globals.hpp"
#include "com_time.hpp"


namespace LibDLS {

    /*************************************************************************/

    /**
       Block of data values.
    */

    class Data
    {
    public:
        Data();
        ~Data();

        template <class T>
        void import(COMTime, COMTime, DLSMetaType, unsigned int,
                    T*, unsigned int);

        void push_back(const Data &);

        COMTime start_time() const;
        COMTime end_time() const;
        COMTime time_per_value() const;
        DLSMetaType meta_type() const;
        unsigned int meta_level() const;

        size_t size() const;
        double value(unsigned int) const;
        COMTime time(unsigned int) const;

        int calc_min_max(double *, double *) const;

    protected:
        COMTime _start_time;
        COMTime _time_per_value;
        DLSMetaType _meta_type;
        unsigned int _meta_level;
        vector<double> _data;
    };
}

/*****************************************************************************/

/**
   Returns the start time of the data block.
   \return start time
*/

inline COMTime LibDLS::Data::start_time() const
{
    return _start_time;
}

/*****************************************************************************/

/**
*/

inline COMTime LibDLS::Data::time_per_value() const
{
    return _time_per_value;
}

/*****************************************************************************/

/**
*/

inline DLSMetaType LibDLS::Data::meta_type() const
{
    return _meta_type;
}

/*****************************************************************************/

/**
   Returns the meta level of the data block.
   \return level
*/

inline unsigned int LibDLS::Data::meta_level() const
{
    return _meta_level;
}

/*****************************************************************************/

/**
   Imports data block properties.
*/

template <class T>
void LibDLS::Data::import(COMTime time,
                          COMTime tpv,
                          DLSMetaType meta_type,
                          unsigned int meta_level,
                          T *data,
                          unsigned int size
                          )
{
    unsigned int i;

    _start_time = time;
    _time_per_value = tpv;
    _meta_type = meta_type;
    _meta_level = meta_level;
    _data.clear();

    for (i = 0; i < size; i++) _data.push_back((double) data[i]);
}

/*****************************************************************************/

/**
   Returns the time of the last data value.
   \return end time
*/

inline COMTime LibDLS::Data::end_time() const
{
    return _start_time + _time_per_value * _data.size();
}

/*****************************************************************************/

/**
*/

inline size_t LibDLS::Data::size() const
{
    return _data.size();
}

/*****************************************************************************/

/**
*/

inline double LibDLS::Data::value(unsigned int index) const
{
    return _data[index];
}

/*****************************************************************************/

/**
*/

inline COMTime LibDLS::Data::time(unsigned int index) const
{
    return _start_time + _time_per_value * index;
}

/*****************************************************************************/

#endif
