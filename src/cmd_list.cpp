/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <iostream>
using namespace std;

#include "lib_dir.hpp"
using namespace LibDLS;

/*****************************************************************************/

extern unsigned int sig_int_term;

static string dls_dir_path;
static unsigned int job_id = 0;

/*****************************************************************************/

int list_jobs(Directory *);
int list_chunks(Job *);
void list_get_environment();
void list_get_options(int, char **);
void list_print_usage();

/*****************************************************************************/

int list_main(int argc, char *argv[])
{
    Directory dls_dir;
    Job *job;

    list_get_environment();
    list_get_options(argc, argv);

    dls_dir.import(dls_dir_path);

    if (!job_id) {
        return list_jobs(&dls_dir);
    }
    else {
        if (!(job = dls_dir.find_job(job_id))) {
            cerr << "No such job - " << job_id << "." << endl;
            cerr << "Call \"dls list\" to list available jobs." << endl;
            return 1;
        }
        return list_chunks(job);
    }
}

/*****************************************************************************/

int list_jobs(Directory *dir)
{
    list<Job>::iterator job_i;

    for (job_i = dir->jobs().begin(); job_i != dir->jobs().end(); job_i++) {
        cout << " " << setw(4) << job_i->preset().id()
             << "  " << job_i->preset().description() << endl;
    }

    return 0;
}

/*****************************************************************************/

struct IndexList {
    list<unsigned int> indices;
};

int list_chunks(Job *job)
{
    list<Channel>::iterator channel_i;
    list<Chunk>::const_iterator chunk_i;
    list<IndexList> groups;
    list<IndexList>::iterator group_i;
    bool same_chunks_found;
    unsigned int index;
    Channel *channel;
    list<unsigned int>::iterator index_i;

    job->fetch_channels();

    for (channel_i = job->channels().begin();
         channel_i != job->channels().end();
         channel_i++) {
        channel_i->fetch_chunks();

        // group channels with same chunks together
        same_chunks_found = false;
        for (group_i = groups.begin(); group_i != groups.end(); group_i++) {
            index = group_i->indices.front();
            if (!(channel = job->find_channel(index))) {
                cerr << "Invalid channel." << endl;
                return 1;
            }
            if (channel_i->has_same_chunks_as(*channel)) {
                group_i->indices.push_back(channel_i->dir_index());
                same_chunks_found = true;
                break;
            }
        }
        if (!same_chunks_found) {
            IndexList new_group;
            new_group.indices.push_back(channel_i->dir_index());
            groups.push_back(new_group);
        }
    }

    for (group_i = groups.begin(); group_i != groups.end(); group_i++) {
        bool first = true;
        for (index_i = group_i->indices.begin();
             index_i != group_i->indices.end();
             index_i++) {
            if (!first) {
                cout << "," << endl;
            }
            channel = job->find_channel(*index_i);
            cout << "Channel " << channel->dir_index()
                 << " " << channel->name();
            first = false;
        }
        cout << ":" << endl;

        for (chunk_i = channel->chunks().begin();
             chunk_i != channel->chunks().end();
             chunk_i++) {
            cout << "  Chunk from " << chunk_i->start().to_real_time()
                 << " to " << chunk_i->end().to_real_time()
                 << " (" << chunk_i->start().diff_str_to(chunk_i->end())
                 << ")" << endl;
        }
    }

    return 0;
}

/*****************************************************************************/

void list_get_environment()
{
    char *env;

    if ((env = getenv("DLS_DIR"))) {
        dls_dir_path = env;
    }
}

/*****************************************************************************/

void list_get_options(int argc, char *argv[])
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
                list_print_usage();
                exit(0);

            default:
                list_print_usage();
                exit(1);
        }
    }

    if (optind < argc) {
        cerr << "Extra parameter given!" << endl;
        list_print_usage();
        exit(1);
    }

    if (dls_dir_path == "") {
        cerr << "No DLS data directory specified!" << endl;
        list_print_usage();
        exit(1);
    }
}

/*****************************************************************************/

void list_print_usage()
{
    cout << "Usage: 1. dls list [OPTIONS]" << endl;
    cout << "       2. dls list -j JOB [OPTIONS]" << endl;
    cout << endl;
    cout << "Description:" << endl;
    cout << "       1. Lists all available jobs." << endl;
    cout << "       2. Lists chunks in the specified job." << endl;
    cout << endl;
    cout << "Options:" << endl;
    cout << "        -d DIR   Specify DLS data directory." << endl;
    cout << "        -j JOB   Specify job ID." << endl;
    cout << "        -h       Print this help." << endl;
}

/*****************************************************************************/
