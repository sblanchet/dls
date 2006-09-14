/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef LibDirHpp
#define LibDirHpp

/*****************************************************************************/

#include <string>
#include <list>
using namespace std;

#include "com_exception.hpp"
#include "lib_job.hpp"

namespace LibDLS
{
    /*************************************************************************/

    /**
       DLS Directory Exception
    */

    class DirectoryException : public COMException
    {
    public:
        DirectoryException(const string &pmsg) : COMException(pmsg) {};
    };

    /*************************************************************************/

    /**
       DLS Data Directory.
    */

    class Directory
    {
    public:
        Directory();
        ~Directory();

        void import(const string &);
        list<Job> &jobs();
        Job *job(unsigned int);
        Job *find_job(unsigned int);

        const string &path() const;

    private:
        string _path; /**< path to DLS data directory */
        list<Job> _jobs; /**< list of jobs */
    };
}

/*************************************************************************/

/**
   Returns the list of jobs.
   \return list of jobs
*/

inline list<LibDLS::Job> &LibDLS::Directory::jobs()
{
    return _jobs;
}

/*************************************************************************/

/**
   Returns the directory's path.
   \return DLS data directory path.
*/

inline const string &LibDLS::Directory::path() const
{
    return _path;
}

/*****************************************************************************/

#endif
