/******************************************************************************
 *
 *  $Id$
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

#include <FL/Fl.h>

/*****************************************************************************/

#include "ctl_globals.hpp"
#include "ctl_dialog_main.hpp"
#include "ctl_dialog_msg.hpp"

/*****************************************************************************/

CTLDialogMsg *msg_win = 0;
string dls_dir, dls_user;

void get_options(int, char **);
void print_usage();

/*****************************************************************************/

int main(int argc, char **argv)
{
    CTLDialogMain *dialog_main;
    struct passwd *pwd;

    cout << "dls_ctl " << PACKAGE_VERSION << " revision " << REVISION << endl;

    // Kommandozeile verarbeiten
    get_options(argc, argv);

    Fl::visual(FL_DOUBLE | FL_INDEX);
    Fl::lock();

    // Erst Fenster erstellen
    msg_win = new CTLDialogMsg();
    dialog_main = new CTLDialogMain(dls_dir);

    // Zu angegebenem Benutzer wechseln
    if (dls_user != "") {
        if (!(pwd = getpwnam(dls_user.c_str()))) {
            cerr << "FEHLER: Benutzer \"" << dls_user
                 << "\" existiert nicht!" << endl;
            exit(1);
        }

        cout << "Wechsele zu UID " << pwd->pw_uid << "." << endl;

        if (setuid(pwd->pw_uid) == -1) {
            cerr << "FEHLER: Wechseln zu UID " << pwd->pw_uid;
            cerr << "fehlgeschlagen: " << strerror(errno);
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

    // Weitere Parameter vorhanden?
    if (optind < argc) {
        print_usage();
    }

    if (!dir_set) {
        // DLS-Verzeichnis aus Umgebungsvariable $DLS_DIR einlesen
        if ((env = getenv(ENV_DLS_DIR)) != 0) dls_dir = env;

        // $DLS_DIR leer: Aktuelles Verzeichnis nutzen
        else dls_dir = ".";
    }

    if (!user_set) {
        // DLS-Benutzer aus Umgebungsvariable $DLS_USER einlesen
        if ((env = getenv(ENV_DLS_USER)) != 0) dls_user = env;
    }

    // Benutztes Verzeichnis ausgeben
    cout << "DLS-Datenverzeichnis \"" << dls_dir << "\"" << endl;
}

/*****************************************************************************/

void print_usage()
{
    cout << "Aufruf: dls_ctl [OPTIONEN]" << endl;
    cout << "        -d [Verzeichnis]   DLS-Datenverzeichnis angeben" << endl;
    cout << "        -u [Benutzer]      DLS-Benutzer angeben" << endl;
    cout << "        -h                 Diese Hilfe anzeigen" << endl;
    exit(0);
}

/*****************************************************************************/
