/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <iostream>
using namespace std;

#include "lib_data.hpp"
using namespace LibDLS;

/*****************************************************************************/

/**
   Konstruktor
*/

Data::Data()
{
}

/*****************************************************************************/

/**
   Destruktor
*/

Data::~Data()
{
}

/*****************************************************************************/

/**
   Appends a data block.
*/

void Data::push_back(const Data &other)
{
    unsigned int i;

    if (other._time_per_value != _time_per_value
        || other._start_time != end_time() + _time_per_value) {
        cerr << "WARNING: Data appending failed!" << endl;
        return;
    }

    for (i = 0; i < other._data.size(); i++)
        _data.push_back(other._data[i]);
}

/*****************************************************************************/

int Data::calc_min_max(double *min, double *max) const
{
    vector<double>::const_iterator data_i;
    double current_min, current_max;

    if (_data.empty()) {
        *min = 0.0;
        *max = 0.0;
        return 0;
    }

    data_i = _data.begin();

    current_min = *data_i;
    current_max = *data_i;
    data_i++;

    while (data_i != _data.end()) {
        if (*data_i < current_min) current_min = *data_i;
        if (*data_i > current_max) current_max = *data_i;
        data_i++;
    }

    *min = current_min;
    *max = current_max;
    return 1;
}

/*****************************************************************************/