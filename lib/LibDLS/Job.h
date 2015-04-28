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
    JobException(const std::string &pmsg) : Exception(pmsg) {};
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

    void import(const std::string &, unsigned int);
    void fetch_channels();

    std::list<Channel> &channels() { return _channels; }
    Channel *channel(unsigned int);
    Channel *find_channel(unsigned int);
    std::set<Channel *> find_channels_by_name(const std::string &);

    const std::string &path() const { return _path; }
    unsigned int id() const { return _preset.id(); }
    const JobPreset &preset() const { return _preset; }

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
        std::string text;

        bool operator<(const Message &other) const {
            return time < other.time;
        }
    };

    std::list<Message> load_msg(Time, Time) const;

private:
    std::string _path; /**< Job path. */
    JobPreset _preset; /**< Job preset. */
    std::list<Channel> _channels; /**< List of recorded channels. */
};

/*****************************************************************************/

} // namespace

/*****************************************************************************/

#endif
