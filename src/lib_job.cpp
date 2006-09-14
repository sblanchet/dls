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
                 unsigned int job_id /**< job ID */
                 )
{
    stringstream job_dir;

    job_dir << dls_path << "/job" << job_id;

    _path = job_dir.str();
    _channels.clear();

    try {
        _preset.import(dls_path, job_id);
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

    _channels.sort();

    closedir(dir);
}

/*************************************************************************/

/**
*/

LibDLS::Channel *LibDLS::Job::channel(unsigned int index)
{
    list<Channel>::iterator channel_i;

    for (channel_i = _channels.begin();
         channel_i != _channels.end();
         channel_i++, index--) {
	if (!index) return &(*channel_i);
    }

    return NULL;
}

/*************************************************************************/

/**
*/

LibDLS::Channel *LibDLS::Job::find_channel(unsigned int index)
{
    list<Channel>::iterator channel_i;

    for (channel_i = _channels.begin();
         channel_i != _channels.end();
         channel_i++) {
	if (channel_i->index() == index) return &(*channel_i);
    }

    return NULL;
}

/*************************************************************************/

/**
   Less-Operator for sorting.
   \return the "left" job is less than the "right" one
*/

bool LibDLS::Job::operator<(const Job &right) const
{
    return preset().id() < right.preset().id();
}

/*****************************************************************************/
