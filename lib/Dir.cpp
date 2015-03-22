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

#include <iostream>
#include <sstream>
using namespace std;

#include <dirent.h>

#include "Dir.h"
using namespace LibDLS;

/*****************************************************************************/

Directory::Directory()
{
}

/*****************************************************************************/

Directory::~Directory()
{
}

/*****************************************************************************/

void Directory::import(const string &path) // importLocal
{
    stringstream str;
    DIR *dir;
    struct dirent *dir_ent;
    Job job;
    string dir_name;
    unsigned int job_id;

    str.exceptions(ios::failbit | ios::badbit);

    _path = path;
    _jobs.clear();

    if (!(dir = opendir(_path.c_str()))) {
        stringstream err;
        err << "Failed to open DLS directory \"" << _path << "\"!";
        throw DirectoryException(err.str());
    }

    while ((dir_ent = readdir(dir))) {
        dir_name = dir_ent->d_name;
        if (dir_name.find("job")) continue;

        str.str("");
        str.clear();
        str << dir_name.substr(3);

        try {
            str >> job_id;
        }
        catch (...) {
            continue;
        }

        try {
            job.import(_path, job_id);
        }
        catch (JobException &e) {
			stringstream err;
            err << "WARNING: Failed to import job "
                 << job_id << ": " << e.msg;
			log(err.str());
            continue;
        }

        _jobs.push_back(job);
    }

    // Verzeichnis schliessen
    closedir(dir);

    // Nach Job-ID sortieren
    _jobs.sort();
}

/*****************************************************************************/

Job *Directory::job(unsigned int index)
{
    list<Job>::iterator job_i;

    for (job_i = _jobs.begin(); job_i != _jobs.end(); job_i++, index--) {
        if (!index) return &(*job_i);
    }

    return NULL;
}

/*****************************************************************************/

Job *Directory::find_job(unsigned int job_id)
{
    list<Job>::iterator job_i;

    for (job_i = _jobs.begin(); job_i != _jobs.end(); job_i++) {
        if (job_i->preset().id() == job_id) return &(*job_i);
    }

    return NULL;
}

/*****************************************************************************/

