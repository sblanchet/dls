/******************************************************************************
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

#include <unistd.h>
#include <errno.h>
#include <string.h> // strerror()
#include <stdlib.h> // getenv()
#include <stdio.h> // rename()
#include <sys/stat.h> // fchmod()

#include <iostream>
#include <iomanip>
using namespace std;

#include "lib/LibDLS/Dir.h"
#include "lib/IndexT.h"
using namespace LibDLS;

/*****************************************************************************/

extern unsigned int sig_int_term;

extern string dls_dir_path;

static unsigned int job_id = 0;

/*****************************************************************************/

void index_print_usage()
{
    cout << "Usage: 1. dls index [OPTIONS]" << endl;
    cout << endl;
    cout << "Description:" << endl;
    cout << "        Re-create indices." << endl;
    cout << endl;
    cout << "Options:" << endl;
    cout << "        -d DIR   Specify DLS data directory." << endl;
    cout << "        -j JOB   Specify job ID." << endl;
    cout << "        -h       Print this help." << endl;
}

/*****************************************************************************/

void index_get_options(int argc, char *argv[])
{
    int c;

    while (1) {
        if ((c = getopt(argc, argv, "d:j:h")) == -1) break;

        switch (c) {
            case 'd':
                dls_dir_path = optarg;
                break;

            case 'j':
                job_id = strtoul(optarg, NULL, 10);
                break;

            case 'h':
                index_print_usage();
                exit(0);

            default:
                index_print_usage();
                exit(1);
        }
    }

    if (optind < argc) {
        cerr << "Extra parameter given!" << endl;
        index_print_usage();
        exit(1);
    }

    if (dls_dir_path == "") {
        cerr << "No DLS data directory specified!" << endl;
        index_print_usage();
        exit(1);
    }
}

/*****************************************************************************/

int index_reindex_channel(Channel *channel)
{
    cout << "    Channel " << channel->dir_index() << " - "
        << channel->name() << endl;

    try {
        channel->fetch_chunks();
    }
    catch (ChannelException &e) {
        cerr << "      Failed to fetch chunks: " << e.msg << endl;
        return 1;
    }

    IndexT<ChannelIndexRecord> index;

    stringstream path;
    path << channel->path() << "/channel.idx";
    string index_path(path.str());

    stringstream tmp;
    tmp << channel->path() << "/.channel.idx.XXXXXX";
    string tmp_path(tmp.str());

    int tmp_fd = mkstemp((char *) tmp_path.c_str());
    if (tmp_fd == -1) {
        cerr << "      Failed to create " << tmp_path << ":" << endl
            << "      " << strerror(errno) << endl;
        return 1;
    }

    int ret = fchmod(tmp_fd, 0644);
    if (ret == -1) {
        cerr << "      Failed to set mode of " << tmp_path << ":" << endl
            << "      " << strerror(errno) << endl;
    }

    try {
        index.open_read_append(tmp_path);
    }
    catch (EIndexT &e) {
        cerr << "      Failed to open index:" << endl
            << "      " << e.msg << endl;
        close(tmp_fd);
        unlink(tmp_path.c_str());
        return 1;
    }

    close(tmp_fd);

    unsigned int record_count(0);
    unsigned int incomplete(0);

    for (Channel::ChunkMap::const_iterator chunk_i =
            channel->chunks().begin();
            chunk_i != channel->chunks().end(); chunk_i++) {
        const Chunk *c = &chunk_i->second;
        ChannelIndexRecord rec;
        rec.start_time = c->start().to_uint64();
        if (c->incomplete()) {
            rec.end_time = 0ULL;
            incomplete++;
        }
        else {
            rec.end_time = c->end().to_uint64();
        }
        index.append_record(&rec);
        record_count++;
    }

    index.close();

    if (rename(tmp_path.c_str(), index_path.c_str()) == -1) {
        unlink(tmp_path.c_str());
        cerr << "Failed to rename " << tmp_path << " to "
            << index_path << ": " << strerror(errno) << endl;
    }

    cout << "       Created channel index with " << record_count
        << " records (" << incomplete << " incomplete)." << endl;

    return 0;
}

/*****************************************************************************/

int index_reindex_job(Job *job)
{
    cout << "  Job " << job->preset().id()
        << " - " << job->preset().description() << endl;

    try {
        job->fetch_channels();
    }
    catch (Exception &e) {
        cerr << "    Failed to fetch channels: " << e.msg << endl;
        return 1;
    }

    for (list<Channel>::iterator channel_i = job->channels().begin();
            channel_i != job->channels().end(); channel_i++) {
        index_reindex_channel(&*channel_i);
    }

    return 0;
}

/*****************************************************************************/

int index_main(int argc, char *argv[])
{
    Directory dls_dir;

    index_get_options(argc, argv);

    try {
        dls_dir.set_uri(dls_dir_path);
    }
    catch (DirectoryException &e) {
        cerr << "Passing URI failed: " << e.msg << endl;
        return 1;
    }

    try {
        dls_dir.import();
    }
    catch (DirectoryException &e) {
        cerr << "Import failed: " << e.msg << endl;
        return 1;
    }

    if (!job_id) {
        for (list<Job *>::iterator job_i = dls_dir.jobs().begin();
                job_i != dls_dir.jobs().end(); job_i++) {
            int ret = index_reindex_job(*job_i);
            if (ret) {
                return ret;
            }
        }
    }
    else {
        Job *job;
        if (!(job = dls_dir.find_job(job_id))) {
            cerr << "No such job - " << job_id << "." << endl;
            cerr << "Call \"dls list\" to list available jobs." << endl;
            return 1;
        }
        return index_reindex_job(job);
    }

    return 0;
}

/*****************************************************************************/
