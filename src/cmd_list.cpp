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

/*****************************************************************************/

void list_get_environment();
void list_get_options(int, char **);
void list_print_usage();

/*****************************************************************************/

int list_main(int argc, char *argv[])
{
    Directory dls_dir;

    list_get_environment();
    list_get_options(argc, argv);

    dls_dir.import(dls_dir_path);

    cout << dls_dir.jobs().size() << " jobs." << endl;

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
        if ((c = getopt(argc, argv, "d:h")) == -1) break;

        switch (c) {
            case 'd':
                dls_dir_path = optarg;
                break;

            case 'h':
                list_print_usage();
                exit(0);

            default:
                list_print_usage();
                exit(1);
        }
    }

    if (optind < argc || dls_dir_path == "") {
        list_print_usage();
        exit(1);
    }
}

/*****************************************************************************/

void list_print_usage()
{
    cout << "Usage: dls list [OPTIONS]" << endl;
    cout << "Options:" << endl;
    cout << "        -d DIR   Specify DLS data directory." << endl;
    cout << "        -h       Print this help." << endl;
}

/*****************************************************************************/
