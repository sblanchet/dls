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
