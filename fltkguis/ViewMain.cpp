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

#include <iostream>
#include <string>
#include <stdlib.h> // getenv()
using namespace std;

#include <FL/Fl.H>

/*****************************************************************************/

#include "lib/mdct.h"

#include "../config.h"
#include "ViewGlobals.h"
#include "ViewDialogMain.h"

/*****************************************************************************/

string dls_dir;

void get_options(int, char **);
void print_usage();

/*****************************************************************************/

int main(int argc, char **argv)
{
    ViewDialogMain *dialog;

    cout << "dls_view " << PACKAGE_VERSION
        << " revision " << REVISION << endl;

    // Process command line
    get_options(argc, argv);

    Fl::visual(FL_DOUBLE | FL_INDEX);
    Fl::lock();

    dialog = new ViewDialogMain(dls_dir);
    dialog->show();
    delete dialog;

    // Release reserved MDCT memory
    LibDLS::mdct_cleanup();

    return 0;
}

/*****************************************************************************/

void get_options(int argc, char **argv)
{
    int c;
    bool dir_set = false;
    char *env;

    do
    {
        c = getopt(argc, argv, "d:h");

        switch (c)
        {
            case 'd':
                dir_set = true;
                dls_dir = optarg;
                break;

            case 'h':
            case '?':
                print_usage();
                break;

            default:
                break;
        }
    }
    while (c != -1);

    // Are other parameters available?
    if (optind < argc) {
        print_usage();
    }

    if (!dir_set) {
        // Read DLS directory from environment variable $DLS_DIR
        if ((env = getenv(ENV_DLS_DIR)) != 0) dls_dir = env;

        // $DLS_DIR empty: use current directory
        else dls_dir = ".";
    }

    // Output used directory
    cout << "Using DLS directory \"" << dls_dir << "\"" << endl;
}

/*****************************************************************************/

void print_usage()
{
    cout << "Call: dls_view [OPTIONS]" << endl;
    cout << "        -d [directory]   DLS data directory" << endl;
    cout << "        -h               Show this help" << endl;
    exit(0);
}

/*****************************************************************************/
