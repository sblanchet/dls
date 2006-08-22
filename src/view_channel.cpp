/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <fcntl.h>
#include <dirent.h>

#include <iostream>
#include <sstream>
#include <fstream>
using namespace std;

/*****************************************************************************/

#include "com_xml_parser.hpp"
#include "view_globals.hpp"
#include "view_channel.hpp"

//#define DEBUG

/*****************************************************************************/

/**
   Konstruktor
*/

ViewChannel::ViewChannel()
{
    _channel = NULL;
    _min_level = 0;
    _max_level = 0;
}

/*****************************************************************************/

/**
   Destruktor
*/

ViewChannel::~ViewChannel()
{
}

/*****************************************************************************/

/**
 */

void ViewChannel::set_channel(Channel *channel)
{
    _channel = channel;
}

/*****************************************************************************/

/**
 */

int data_callback(Data *data, void *cb_data)
{
    ViewChannel *channel = (ViewChannel *) cb_data;

    switch (data->meta_type()) {
        case DLSMetaGen:
            channel->_gen_data.push_back(*data);
            break;
        case DLSMetaMin:
            channel->_min_data.push_back(*data);
            break;
        case DLSMetaMax:
            channel->_max_data.push_back(*data);
            break;
        default:
            break;
    }

    return 0; // not adopted
}

/*****************************************************************************/

/**
 */

void ViewChannel::fetch_data(COMTime start,
                             COMTime end,
                             unsigned int min_values
                             )
{
    _gen_data.clear();
    _min_data.clear();
    _max_data.clear();

    _channel->fetch_data(start, end, min_values, data_callback, this);

    _calc_min_max();
}

/*****************************************************************************/

/**
   Berechnet die Extrema der geladenen Daten
*/

void ViewChannel::_calc_min_max()
{
    bool first;

    _min = 0.0;
    _max = 0.0;
    _min_level = 0;
    _max_level = 0;
    first = true;

    _calc_min_max_data(_gen_data, &first);
    _calc_min_max_data(_min_data, &first);
    _calc_min_max_data(_max_data, &first);
}

/*****************************************************************************/

/**
*/

void ViewChannel::_calc_min_max_data(const list<Data> &data, bool *first)
{
    list<Data>::const_iterator data_i;
    double current_min, current_max;

    if (data.empty()) return;

    for (data_i = data.begin(); data_i != data.end(); data_i++) {
        if (!data_i->calc_min_max(&current_min, &current_max)) continue;

        if (*first) {
            _min = current_min;
            _max = current_max;
            _min_level = data_i->meta_level();
            _max_level = data_i->meta_level();
            *first = false;
        }
        else {
            if (current_min < _min) _min = current_min;
            if (current_max > _max) _max = current_max;
            if (data_i->meta_level() < _min_level)
                _min_level = data_i->meta_level();
            if (data_i->meta_level() > _max_level)
                _max_level = data_i->meta_level();
        }
    }
}

/*****************************************************************************/

bool ViewChannel::operator<(const ViewChannel &right) const
{
    if (_channel && right._channel)
        return _channel->index() < right._channel->index();
    else
        return true;
}

/*****************************************************************************/
