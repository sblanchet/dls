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
                unsigned int, unsigned int &, T*, unsigned int);

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
                          unsigned int decimation,
                          unsigned int &decimationCounter,
                          T *data,
                          unsigned int size
                          )
{
    unsigned int i;

    _start_time = time + tpv * decimationCounter;
    _time_per_value = tpv * decimation;
    _meta_type = meta_type;
    _meta_level = meta_level;
    _data.clear();

    for (i = 0; i < size; i++) {
        if (!decimationCounter) {
            _data.push_back((double) data[i]);
            decimationCounter = decimation - 1;
        } else {
            decimationCounter--;
        }
    }
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
