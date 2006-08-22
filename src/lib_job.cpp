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

#include "lib_dir.hpp"
#include "lib_job.hpp"
using namespace LibDLS;

/*****************************************************************************/

/**
   Constructor
*/

Job::Job()
{
}

/*****************************************************************************/

/**
   Destructor
*/

Job::~Job()
{
}

/*****************************************************************************/

/**
   Imports job information.
*/

void Job::import(const string &dls_path, /**< DLS directory path */
                 unsigned int index /**< job index */
                 )
{
    stringstream job_dir;

    job_dir << dls_path << "/job" << index;

    _path = job_dir.str();
    _index = index;
    _channels.clear();

    try {
        _preset.import(dls_path, _index);
    }
    catch (ECOMJobPreset &e) {
        cout << "WARNING: " << e.msg << endl;
        return;
    }
}

/*****************************************************************************/

/**
   Imports all channels.
*/

void Job::fetch_channels()
{
    stringstream str;
    DIR *dir;
    struct dirent *dir_ent;
    string channel_dir_name;
    int channel_index;
    Channel channel;

    str.exceptions(ios::failbit | ios::badbit);

    _channels.clear();

    if (!(dir = opendir(_path.c_str()))) {
        cerr << "ERROR: Failed to open job directory \"" << _path << "\"."
             << endl;
        return;
    }

    while ((dir_ent = readdir(dir))) {
        channel_dir_name = dir_ent->d_name;
        if (channel_dir_name.find("channel")) continue;

        str.str("");
        str.clear();
        str << channel_dir_name.substr(7);

        try {
            str >> channel_index;
        }
        catch (...) {
            continue;
        }

        try {
            channel.import(_path, channel_index);
        }
        catch (ChannelException &e) {
            cerr << "WARNING: " << e.msg << endl;
            continue;
        }

        _channels.push_back(channel);
    }

    closedir(dir);
}

/*************************************************************************/

/**
*/

LibDLS::Channel &LibDLS::Job::channel(unsigned int index)
{
    list<Channel>::iterator channel_i;
    for (channel_i = _channels.begin();
         channel_i != _channels.end();
         channel_i++) {
        return *channel_i;
    }
    throw JobException("Channel index out of range!");
}

/*************************************************************************/

/**
   Less-Operator for sorting.
   \return the "left" job is less than the "right" one
*/

bool LibDLS::Job::operator<(const Job &right) const
{
    return _index < right._index;
}

/*****************************************************************************/