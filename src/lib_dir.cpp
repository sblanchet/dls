/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <iostream>
using namespace std;

#include <dirent.h>

#include "lib_dir.hpp"
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

void Directory::import(const string &path)
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
            cerr << "WARNING: Failed to import job "
                 << job_id << ": " << e.msg << endl;
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

