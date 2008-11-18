/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <signal.h>
#include <stdlib.h>

#include <iostream>
using namespace std;

#include "com_globals.hpp"

/*****************************************************************************/

string command;

/*****************************************************************************/

void print_usage();

extern int list_main(int, char *[]);
extern int export_main(int, char *[]);

/*****************************************************************************/

int main(int argc, char *argv[])
{
    string command;

    cout << "dls " << PACKAGE_VERSION << " revision " << REVISION << endl;

    if (argc <= 1) {
        print_usage();
        exit(1);
    }

    command = argv[1];

    if (command == "list") {
        return list_main(argc - 1, argv + 1);
    }
    else if (command == "export") {
        return export_main(argc - 1, argv + 1);
    }

    // invalid command
    print_usage();
    return 1;
}

/*****************************************************************************/

void print_usage()
{
    cout << "Usage: dls COMMAND [OPTIONS]" << endl;
    cout << "Commands:" << endl;
    cout << "    list - List available chunks." << endl;
    cout << "  export - Export collected data." << endl;
    cout << "    help - Print this help." << endl;
    cout << "Enter \"dls COMMAND -h\" for command-specific help." << endl;
}

/*****************************************************************************/
