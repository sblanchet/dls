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

#ifndef LibJobHpp
#define LibJobHpp

/*****************************************************************************/

#include <string>
#include <list>
using namespace std;

#include "com_exception.hpp"
#include "com_time.hpp"
#include "com_job_preset.hpp"

#include "lib_channel.hpp"

namespace LibDLS
{
    /*************************************************************************/

    /**
       Exception eines DLSJob-Objektes
    */

    class JobException : public COMException
    {
    public:
        JobException(const string &pmsg) : COMException(pmsg) {};
    };

    /*************************************************************************/

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

        const string &path() const;
        const COMJobPreset &preset() const;

        bool operator<(const Job &) const;

    private:
        string _path; /**< DLS job directory path */
        COMJobPreset _preset; /**< job preset */
        list<Channel> _channels; /**< list of recorded channels */
    };
}

/*************************************************************************/

/**
   Returns the job's path.
   \return job path
*/

inline const string &LibDLS::Job::path() const
{
    return _path;
}

/*************************************************************************/

/**
*/

inline const COMJobPreset &LibDLS::Job::preset() const
{
    return _preset;
}

/*************************************************************************/

/**
   Returns the list of channels.
   \return list of channels
*/

inline list<LibDLS::Channel> &LibDLS::Job::channels()
{
    return _channels;
}

/*****************************************************************************/

#endif
