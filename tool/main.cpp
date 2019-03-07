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

#include <signal.h>
#include <stdlib.h>

#include <iostream>
using namespace std;

#include "../config.h"

/*****************************************************************************/

string command;

string dls_dir_path;
string dls_export_dir;
string dls_export_format;

/*****************************************************************************/

void print_usage();

extern int list_main(int, char *[]);
extern int export_main(int, char *[]);
extern int index_main(int, char *[]);

/*****************************************************************************/

int main(int argc, char *argv[])
{
    string command;
    char *env;

    cout << "dls " << PACKAGE_VERSION << " revision " << REVISION << endl;

    if (argc <= 1) {
        print_usage();
        exit(1);
    }

    command = argv[1];

    if ((env = getenv("DLS_DIR"))) {
        dls_dir_path = env;
    }

    if ((env = getenv("DLS_EXPORT"))) {
        dls_export_dir = env;
    }

    if ((env = getenv("DLS_EXPORT_FMT"))) {
        dls_export_format = env;
    }

    if (command == "list") {
        return list_main(argc - 1, argv + 1);
    }
    else if (command == "export") {
        return export_main(argc - 1, argv + 1);
    }
    else if (command == "index") {
        return index_main(argc - 1, argv + 1);
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
    cout << "   index - (Re-)generate indices." << endl;
    cout << "    help - Print this help." << endl;
    cout << "Enter \"dls COMMAND -h\" for command-specific help." << endl;
}

/*****************************************************************************/
