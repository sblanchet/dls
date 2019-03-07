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
#include <pwd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <string>
using namespace std;

#include <FL/Fl.H>

/*****************************************************************************/

#include "../config.h"
#include "CtlGlobals.h"
#include "CtlDialogMain.h"
#include "CtlDialogMsg.h"

/*****************************************************************************/

CtlDialogMsg *msg_win = 0;
string dls_dir, dls_user;

void get_options(int, char **);
void print_usage();

/*****************************************************************************/

int main(int argc, char **argv)
{
    CtlDialogMain *dialog_main;
    struct passwd *pwd;

    cout << "dls_ctl " << PACKAGE_VERSION << " revision " << REVISION << endl;

    // Process command line
    get_options(argc, argv);

    Fl::visual(FL_DOUBLE | FL_INDEX);
    Fl::lock();

    // First create a window
    msg_win = new CtlDialogMsg();
    dialog_main = new CtlDialogMain(dls_dir);

    // Switch to the specified user
    if (dls_user != "") {
        if (!(pwd = getpwnam(dls_user.c_str()))) {
            cerr << "ERROR: User \"" << dls_user
                 << "\" does not exist!" << endl;
            exit(1);
        }

        cout << "Changing to UID " << pwd->pw_uid << "." << endl;

        if (setuid(pwd->pw_uid) == -1) {
            cerr << "ERROR: Changing to UID " << pwd->pw_uid;
            cerr << "failed: " << strerror(errno);
            exit(1);
        }
    }

    dialog_main->show();
    delete dialog_main;

    delete msg_win;

    return 0;
}

/*****************************************************************************/

void get_options(int argc, char **argv)
{
    int c;
    bool dir_set = false, user_set = false;
    char *env;

    do {
        c = getopt(argc, argv, "d:u:h");

        switch (c) {
            case 'd':
                dir_set = true;
                dls_dir = optarg;
                break;

            case 'u':
                user_set = true;
                dls_user = optarg;
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

    // Are there other parameter available?
    if (optind < argc) {
        print_usage();
    }

    if (!dir_set) {
        // read DLS directory from environment variable $DLS_DIR
        if ((env = getenv(ENV_DLS_DIR)) != 0) dls_dir = env;

        // $DLS_DIR empty: use current directory
        else dls_dir = ".";
    }

    if (!user_set) {
        // Read DLS user from environment variable $DLS_USER
        if ((env = getenv(ENV_DLS_USER)) != 0) dls_user = env;
    }

    // Output used directory
    cout << "DLS data directory \"" << dls_dir << "\"" << endl;
}

/*****************************************************************************/

void print_usage()
{
    cout << "Call: dls_ctl [OPTIONS]" << endl;
    cout << "        -d [directory]     DLS data directory" << endl;
    cout << "        -u [user]          DLS user" << endl;
    cout << "        -h                 Show this help" << endl;
    exit(0);
}

/*****************************************************************************/
