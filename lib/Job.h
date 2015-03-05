/*****************************************************************************
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
 ****************************************************************************/

#ifndef LibDLSJobH
#define LibDLSJobH

/****************************************************************************/

#include <string>
#include <list>
#include <set>
using namespace std;

#include "Exception.h"
#include "Time.h"
#include "JobPreset.h"

#include "Channel.h"

/****************************************************************************/

namespace LibDLS {

/****************************************************************************/

/**
   Exception eines DLSJob-Objektes
*/

class JobException : public Exception
{
public:
    JobException(const string &pmsg) : Exception(pmsg) {};
};

/****************************************************************************/

/**
   Darstellung eines Kanals in der Anzeige
*/

class Job
{
public:
    Job();
    ~Job();

    void import(const string &, unsigned int);
    void fetch_channels();

    list<Channel> &channels();
    Channel *channel(unsigned int);
    Channel *find_channel(unsigned int);
    set<Channel *> find_channels_by_name(const std::string &);

    const string &path() const;
    unsigned int id() const;
    const JobPreset &preset() const;

    bool operator<(const Job &) const;

    struct Message
    {
        Time time;
        enum Type {
            Unknown = -1,
            Info,
            Warning,
            Error,
            Critical,
            Broadcast,
            TypeCount
        };
        Type type;
        string text;

        bool operator<(const Message &other) const {
            return time < other.time;
        }
    };

    list<Message> load_msg(Time, Time) const;

private:
    string _path; /**< DLS job directory path */
    unsigned int _id; /**< Job index. */
    JobPreset _preset; /**< job preset */
    list<Channel> _channels; /**< list of recorded channels */
};

/****************************************************************************/

/**
   Returns the job's path.
   \return job path
*/

inline const string &Job::path() const
{
    return _path;
}

/****************************************************************************/

/**
   Returns the job's ID.
   \return job ID
*/

inline unsigned int Job::id() const
{
    return _id;
}

/****************************************************************************/

/**
*/

inline const JobPreset &Job::preset() const
{
    return _preset;
}

/****************************************************************************/

/**
   Returns the list of channels.
   \return list of channels
*/

inline list<Channel> &Job::channels()
{
    return _channels;
}

/*****************************************************************************/

} // namespace

/*****************************************************************************/

#endif
