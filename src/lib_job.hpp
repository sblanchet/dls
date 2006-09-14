/******************************************************************************
 *
 *  $Id$
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
        Channel &channel(unsigned int);

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
