/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <iostream>
using namespace std;

#include "lib_dir.hpp"
#include "lib_export.hpp"
using namespace LibDLS;

/*****************************************************************************/

extern unsigned int sig_int_term;

static string dls_dir_path;
static string dls_export_dir;
static unsigned int job_id = 0;
static string channels_str;
static string start_time_str;
static string end_time_str;
static bool export_ascii = false;
static bool export_matlab = false;

/*****************************************************************************/

typedef struct
{
    list<Export *> *exporters;
}
ExportInfo;

/*****************************************************************************/

int export_data_callback(Data *, void *);
void export_get_environment();
void export_get_options(int, char **);
list<unsigned int> parse_intlist(const char *);
COMTime parse_time(const char *);
void export_print_usage();

/*****************************************************************************/

int export_main(int argc, char *argv[])
{
    Directory dls_dir;
    Job *job;
    list<unsigned int> channel_indices;
    list<unsigned int>::iterator index_i;
    list<Channel *> channels;
    list<Channel *>::iterator channel_i;
    list<Channel>::iterator job_channel_i;
    Channel *channel;
    COMTime start, end, channels_start, channels_end;
    list<Export *> exporters;
    list<Export *>::iterator exp_i;
    ExportInfo info;

    export_get_environment();
    export_get_options(argc, argv);

    if (dls_export_dir == "") dls_export_dir = ".";

    if (export_ascii) exporters.push_back(new ExportAscii());
    if (export_matlab) exporters.push_back(new ExportMat4());

    if (exporters.empty()) {
        cerr << "No exporters active! Enable at least one." << endl;
        export_print_usage();
        exit(1);
    }

    dls_dir.import(dls_dir_path);

    if (!(job = dls_dir.find_job(job_id))) {
        cerr << "No such job - " << job_id << "." << endl;
        cerr << "Call \"dls list\" to list available jobs." << endl;
        return 1;
    }

    channel_indices = parse_intlist(channels_str.c_str());
    job->fetch_channels();

    if (channels_str != "") {
        for (index_i = channel_indices.begin();
             index_i != channel_indices.end();
             index_i++) {
            if (!(channel = job->find_channel(*index_i))) {
                cerr << "No such channel - " << *index_i << "." << endl;
                cerr << "Call \"dls list -j " << job_id
                     << "\" to list available channels." << endl;
                exit(1);
            }
            channels.push_back(channel);
        }
    }
    else {
        for (job_channel_i = job->channels().begin();
             job_channel_i != job->channels().end();
             job_channel_i++) {
            channels.push_back(&(*job_channel_i));
        }
    }

    if (channels.empty()) {
        cerr << "No channels to export." << endl;
        exit(1);
    }

    cout << "Channels to export:" << endl;
    for (channel_i = channels.begin();
         channel_i != channels.end();
         channel_i++) {
        channel = *channel_i;
        cout << "  " << channel->index() << " " << channel->name() << endl;

        channel->fetch_chunks();

        if (!channel->start().is_null()) {
            if (channels_start.is_null()) {
                channels_start = channel->start();
            }
            else if (channel->start() < channels_start) {
                channels_start = channel->start();
            }
        }

        if (!channel->end().is_null()) {
            if (channels_end.is_null()) {
                channels_end = channel->end();
            }
            else if (channel->end() > channels_end) {
                channels_end = channel->end();
            }
        }
    }

    start = parse_time(start_time_str.c_str());
    end = parse_time(end_time_str.c_str());

    if (start.is_null()) start = channels_start;
    if (end.is_null()) end = channels_end;

    cout << "Start time: " << start.to_real_time() << endl
         << "  End time: " << end.to_real_time() << endl
         << "  duration: " << start.diff_str_to(end) << endl;

    if (start >= end) {
        cerr << "Invalid time range!" << endl;
        exit(1);
    }

    cout << "Exporting to \"" << dls_export_dir << "\"." << endl;

    info.exporters = &exporters;

    // actual exporting
    for (channel_i = channels.begin();
         channel_i != channels.end();
         channel_i++) {
        channel = *channel_i;

        for (exp_i = exporters.begin(); exp_i != exporters.end(); exp_i++)
            (*exp_i)->begin(*channel, dls_export_dir);

        channel->fetch_data(start, end, 0, export_data_callback, &info);

        for (exp_i = exporters.begin(); exp_i != exporters.end(); exp_i++)
            (*exp_i)->end();

#if 0
        current_channel++;

        // display progress
        info.channel_percentage = 100.0 * current_channel / total_channels;
        _set_progress_value((int) (info.channel_percentage + 0.5));
#endif

        if (sig_int_term) {
            cerr << "Interrupt detected." << endl;
            break;
        }
    }

    cout << "Export finished." << endl;
    return 0;
}

/*****************************************************************************/

/**
 */

int export_data_callback(Data *data, void *cb_data)
{
    ExportInfo *info = (ExportInfo *) cb_data;

    list<Export *>::iterator exp_i;
    //double diff_time;
    //double percentage;

    for (exp_i = info->exporters->begin();
         exp_i != info->exporters->end();
         exp_i++)
        (*exp_i)->data(data);

#if 0
    // display progress
    diff_time = (data->end_time() - info->dialog->_start).to_dbl();
    percentage = info->channel_percentage + diff_time * info->channel_factor;
    info->dialog->_set_progress_value((int) (percentage + 0.5));
#endif

    return 0; // not adopted
}

/*****************************************************************************/

void export_get_environment()
{
    char *env;

    if ((env = getenv("DLS_DIR"))) {
        dls_dir_path = env;
    }

    if ((env = getenv("DLS_EXPORT"))) {
        dls_export_dir = env;
    }
}

/*****************************************************************************/

void export_get_options(int argc, char *argv[])
{
    int c;

    while (1) {
        if ((c = getopt(argc, argv, "d:o:j:c:s:e:amh")) == -1) break;

        switch (c) {
            case 'd':
                dls_dir_path = optarg;
                break;

            case 'o':
                dls_export_dir = optarg;
                break;

            case 'j':
                job_id = strtoul(optarg, NULL, 10);
                break;

            case 'c':
                channels_str = optarg;
                break;

            case 's':
                start_time_str = optarg;
                break;

            case 'e':
                end_time_str = optarg;
                break;

            case 'a':
                export_ascii = true;
                break;

            case 'm':
                export_matlab = true;
                break;

            case 'h':
                export_print_usage();
                exit(0);

            default:
                export_print_usage();
                exit(1);
        }
    }

    if (optind < argc) {
        cerr << "Extra parameter given!" << endl;
        export_print_usage();
        exit(1);
    }

    if (dls_dir_path == "") {
        cerr << "No DLS data directory specified!" << endl;
        export_print_usage();
        exit(1);
    }

    if (!job_id) {
        cerr << "You must specify a job ID!" << endl;
        cerr << "Call \"dls list\" to list available jobs." << endl;
        export_print_usage();
        exit(1);
    }
}

/*****************************************************************************/

list<unsigned int> parse_intlist(const char *intlist_str)
{
    unsigned int index;
    const char *current;
    char *remainder;
    list<unsigned int> numbers;

    if (intlist_str == "") return numbers;

    current = intlist_str;

    while (1) {
        index = strtoul(current, &remainder, 10);

        if (remainder == current) goto error;

        numbers.push_back(index);

        if (!strlen(remainder)) break;
        if (remainder[0] != ',') goto error;

        current = remainder + 1;
    }

    numbers.sort();
    numbers.unique();

    return numbers;

 error:
    cerr << "Parse error in parameter \"" << intlist_str << "\"." << endl;
    export_print_usage();
    exit(1);
}

/*****************************************************************************/

COMTime parse_time(const char *time_str)
{
    struct tm t;
    unsigned int usec;
    const char *current;
    char *remainder;

    if (!strlen(time_str)) return COMTime();

    t.tm_year = 0;
    t.tm_mon = 0;
    t.tm_mday = 1;
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_isdst = -1;
    usec = 0;

    current = time_str;

    t.tm_year = strtoul(current, &remainder, 10) - 1900;
    if (remainder == current) goto error;
    if (!strlen(remainder)) goto mktime;
    if (remainder[0] != '-') goto error;
    current = remainder + 1;

    t.tm_mon = strtoul(current, &remainder, 10) - 1;
    if (remainder == current) goto error;
    if (!strlen(remainder)) goto mktime;
    if (remainder[0] != '-') goto error;
    current = remainder + 1;

    t.tm_mday = strtoul(current, &remainder, 10);
    if (remainder == current) goto error;
    if (!strlen(remainder)) goto mktime;
    if (remainder[0] != '-' && remainder[0] != ' ') goto error;
    current = remainder + 1;

    t.tm_hour = strtoul(current, &remainder, 10);
    if (remainder == current) goto error;
    if (!strlen(remainder)) goto mktime;
    if (remainder[0] != '-' && remainder[0] != ':') goto error;
    current = remainder + 1;

    t.tm_min = strtoul(current, &remainder, 10);
    if (remainder == current) goto error;
    if (!strlen(remainder)) goto mktime;
    if (remainder[0] != '-' && remainder[0] != ':') goto error;
    current = remainder + 1;

    t.tm_sec = strtoul(current, &remainder, 10);
    if (remainder == current) goto error;
    if (!strlen(remainder)) goto mktime;
    if (remainder[0] != '-' && remainder[0] != '.' && remainder[0] != ',')
        goto error;
    current = remainder + 1;

    usec = strtoul(current, &remainder, 10);
    if (remainder == current || strlen(remainder)) goto error;

 mktime:
    return COMTime(&t, usec);

 error:
    cerr << "Parse error in time \"" << time_str << "\"." << endl;
    export_print_usage();
    exit(1);
}

/*****************************************************************************/

void export_print_usage()
{
    cout << "Usage: dls export -j ID [OPTIONS]" << endl
         << "Options:" << endl
         << "        -d DIR       DLS data directory" << endl
         << "        -o DIR       Output (DLS export) directory" << endl
         << "        -j ID        Job to export (mandatory)" << endl
         << "        -c I,J,K     Channel indices to export (default: all)"
         << endl
         << "        -s TIMEFMT   Start time (default: start of recording)"
         << endl
         << "        -e TIMEFMT   End time (default: end of recording)"
         << endl
         << "        -a           Enable ASCII-Exporter" << endl
         << "        -m           Enable MATLAB-Exporter" << endl
         << "        -h           Print this help" << endl
         << "TIMEFMT is YYYY[-MM[-DD[-HH[-MM[-SS[-UUUUUU]]]]]]" << endl
         << "        or YYYY[.MM[.SS[ HH[:MM[:SS[,UUUUUU]]]]]]." << endl;
}

/*****************************************************************************/
