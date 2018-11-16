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

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sstream>
#include <fstream>
using namespace std;


#include "lib/LibDLS/globals.h"
#include "lib/XmlTag.h"

#include "CtlJobPreset.h"

/*****************************************************************************/

/**
   Constructor
*/

CtlJobPreset::CtlJobPreset():
    LibDLS::JobPreset()
{
    process_watchdog = 0;
    logging_watchdog = 0;
};

/*****************************************************************************/

/**
   Save job specification in  XML file

   Write all job specifications (including channel specification)
   into a XML file in the correct directory. Afterwards, the parent
   process is notified via a spooling line that it should read in the
   defaults again.

   \param dls_dir DLS data directory
   \throw EJobPreset invalid data or impossible writing
*/

void CtlJobPreset::write(const string &dls_dir)
{
    stringstream dir_name, err;
    string file_name;
    fstream file;
    LibDLS::XmlTag tag;
    vector<LibDLS::ChannelPreset>::iterator channel_i;
    int fd;
    char *tmp_file_name;

    // Construct directory names
    dir_name << dls_dir << "/job" << _id;

    if (mkdir(dir_name.str().c_str(), 0755) != 0)
    {
        if (errno != EEXIST)
        {
            err << "Could not create \"" << dir_name.str()
                << "\" (errno " << errno << ")!";
            throw LibDLS::EJobPreset(err.str());
        }
    }

    // Construct filenames
    file_name = dir_name.str() + "/job.xml";

    try {
        tmp_file_name = new char[strlen(file_name.c_str()) + 8];
    }
    catch (...) {
        err << "Failed to allocate memory for file name!";
        throw LibDLS::EJobPreset(err.str());
    }

    sprintf(tmp_file_name, "%s.XXXXXX", file_name.c_str());

    fd = mkstemp(tmp_file_name);
    if (fd == -1) {
        err << "Could not create \"" << tmp_file_name
            << "\": " << strerror(errno);
        delete [] tmp_file_name;
        throw LibDLS::EJobPreset(err.str());
    }

    if (chmod(tmp_file_name, 0644) == -1) {
        err << "Could not change rights of \"" << tmp_file_name
            << "\": " << strerror(errno);
        delete [] tmp_file_name;
        throw LibDLS::EJobPreset(err.str());
    }

    // Open file
    file.open(tmp_file_name, ios::out);
    close(fd);

    if (!file.is_open())
    {
        err << "Could not attach to file \"" << tmp_file_name << "\"";
        delete [] tmp_file_name;
        throw LibDLS::EJobPreset(err.str());
    }

    file.exceptions(fstream::failbit | fstream::badbit);

    try
    {
        tag.clear();
        tag.type(LibDLS::dxttBegin);
        tag.title("dlsjob");
        file << tag.tag() << endl;

        tag.clear();
        tag.title("description");
        tag.push_att("text", _description);
        file << " " << tag.tag() << endl;

        tag.clear();
        tag.title("state");
        tag.push_att("name", _running ? "running" : "paused");
        file << " " << tag.tag() << endl;

        tag.clear();
        tag.title("source");
        tag.push_att("address", _source);
        tag.push_att("port", _port);
        file << " " << tag.tag() << endl;

        tag.clear();
        tag.title("quota");
        tag.push_att("size", _quota_size);
        tag.push_att("time", _quota_time);
        file << " " << tag.tag() << endl;

        tag.clear();
        tag.title("trigger");
        tag.push_att("parameter", _trigger);
        file << " " << tag.tag() << endl;

        file << endl;

        tag.clear();
        tag.title("channels");
        tag.type(LibDLS::dxttBegin);
        file << " " << tag.tag() << endl;

        channel_i = _channels.begin();
        while (channel_i != _channels.end())
        {
            channel_i->write_to_tag(&tag);
            file << "  " << tag.tag() << endl;
            channel_i++;
        }

        tag.clear();
        tag.title("channels");
        tag.type(LibDLS::dxttEnd);
        file << " " << tag.tag() << endl;

        tag.clear();
        tag.title("dlsjob");
        tag.type(LibDLS::dxttEnd);
        file << tag.tag() << endl;
    }
    catch (LibDLS::EChannelPreset &e)
    {
        file.close();
        unlink(tmp_file_name);
        delete [] tmp_file_name;
        err << "Failed to write: " << e.msg;
        throw LibDLS::EJobPreset(err.str());
    }
    catch (ios_base::failure &e)
    {
        // file.close() throws exception
        unlink(tmp_file_name);
        delete [] tmp_file_name;
        err << "Failed to write: " << e.what();
        throw LibDLS::EJobPreset(err.str());
    }

    file.close();

    if (rename(tmp_file_name, file_name.c_str()) == -1) {
        delete [] tmp_file_name;
        err << "Failed to rename " << tmp_file_name << " to "
            << file_name << ": " << strerror(errno);
        throw LibDLS::EJobPreset(err.str());
    }

    delete [] tmp_file_name;
}

/*****************************************************************************/

/**
   Create a spooling file to notify the process

   This method notifies the process of a change in a job specification.
   For example, it can notify that a new specification has been created,
   an existing preset have changed or was deleted.

   The spooling file contains only the job ID. The process
   decides independently what to do.

   \param dls_dir DLS data directory
*/

void CtlJobPreset::spool(const string &dls_dir)
{
    fstream file;
    stringstream filename, err;
    struct timeval tv;

    gettimeofday(&tv, 0);

    // Create an unique filename
    filename << dls_dir << "/spool/";
    filename << tv.tv_sec << "_" << tv.tv_usec;
    filename << "_" << (unsigned long) this;

    file.open(filename.str().c_str(), ios::out);

    if (!file.is_open())
    {
        err << "Could not write spooling file \"" << filename.str() << "\"!";
        throw LibDLS::EJobPreset(err.str());
    }

    file << _id << endl;
    file.close();
}

/*****************************************************************************/

/**
   Set the job ID
*/

void CtlJobPreset::id(unsigned int id)
{
    _id = id;
}

/*****************************************************************************/

/**
   Set the job description
*/

void CtlJobPreset::description(const string &desc)
{
    _description = desc;
}

/*****************************************************************************/

/**
   Set the target status
*/

void CtlJobPreset::running(bool run)
{
    _running = run;
}

/*****************************************************************************/

/**
   Change the target status
*/

void CtlJobPreset::toggle_running()
{
    _running = !_running;
}

/*****************************************************************************/

/**
   Set the data source
*/

void CtlJobPreset::source(const string &src)
{
    _source = src;
}

/*****************************************************************************/

/**
   Set the name of the trigger parameter
*/

void CtlJobPreset::trigger(const string &trigger_name)
{
    _trigger = trigger_name;
}

/*****************************************************************************/

/**
   Set the size of time quota

   \param seconds maximum number of seconds to capture
*/

void CtlJobPreset::quota_time(uint64_t seconds)
{
    _quota_time = seconds;
}

/*****************************************************************************/

/**
   Set the size of the data quota

   \param bytes maximum number of bytes to capture
*/

void CtlJobPreset::quota_size(uint64_t bytes)
{
    _quota_size = bytes;
}

/*****************************************************************************/

/**
   Add a channel preset

   \param channel New channel
   \throw EJobPreset A default for this channel already exists!
*/

void CtlJobPreset::add_channel(const LibDLS::ChannelPreset *channel)
{
    stringstream err;

    if (channel_exists(channel->name))
    {
        err << "Channel \"" << channel->name << "\" already exists!";
        throw LibDLS::EJobPreset(err.str());
    }

    _channels.push_back(*channel);
}

/*****************************************************************************/

/**
   Change a channel preset

   The channel to be changed is determined by the channel name
   in the new specification.

   \param new_channel Pointer to the new channel preset name
   \throw EJobPreset There is no default for the specifed channel
*/

void CtlJobPreset::change_channel(const LibDLS::ChannelPreset *new_channel)
{
    vector<LibDLS::ChannelPreset>::iterator channel_i;
    stringstream err;

    channel_i = _channels.begin();
    while (channel_i != _channels.end())
    {
        if (channel_i->name == new_channel->name)
        {
            *channel_i = *new_channel;
            return;
        }

        channel_i++;
    }

    err << "Preset for channel \"" << new_channel->name << "\" doesn't exist!";
    throw LibDLS::EJobPreset(err.str());
}

/*****************************************************************************/

/**
   Remove a channel preset

   \param channel_name Channel name of the channel whose default
   should be removed
   \throw EJobPreset There is not default for the specified channel
*/

void CtlJobPreset::remove_channel(const string &channel_name)
{
    vector<LibDLS::ChannelPreset>::iterator channel_i;
    stringstream err;

    channel_i = _channels.begin();
    while (channel_i != _channels.end())
    {
        if (channel_i->name == channel_name)
        {
            _channels.erase(channel_i);
            return;
        }

        channel_i++;
    }

    err << "Preset for channel \"" << channel_name << "\" doesn't exist!";
    throw LibDLS::EJobPreset(err.str());
}

/*****************************************************************************/
