/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <iostream>
using namespace std;

/*****************************************************************************/

extern unsigned int sig_int_term;

static string dls_dir;
static string dls_export_dir;

/*****************************************************************************/

void export_get_environment();
void export_get_options(int, char **);
void export_print_usage();

/*****************************************************************************/

int export_main(int argc, char *argv[])
{
    export_get_environment();
    export_get_options(argc, argv);

    cout << "export" << endl;

    return 0;
}

/*****************************************************************************/

void export_get_environment()
{
    char *env;

    if ((env = getenv("DLS_DIR"))) {
        dls_dir = env;
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
        if ((c = getopt(argc, argv, "d:e:h")) == -1) break;

        switch (c) {
            case 'd':
                dls_dir = optarg;
                break;

            case 'e':
                dls_export_dir = optarg;
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
        export_print_usage();
        exit(1);
    }
}

/*****************************************************************************/

void export_print_usage()
{
    cout << "Usage: dls export [OPTIONS]" << endl;
    cout << "Options:" << endl;
    cout << "        -d DIR   Specify DLS data directory." << endl;
    cout << "        -e DIR   Specify DLS export directory." << endl;
    cout << "        -h       Print this help." << endl;
}

/*****************************************************************************/
