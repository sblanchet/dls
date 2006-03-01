/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <unistd.h>

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
string dls_dir;

void get_options(int, char **);
void print_usage();

/*****************************************************************************/

int main(int argc, char **argv)
{
  CTLDialogMain *dialog_main;

  cout << "dls_ctl " << DLS_VERSION_STR
       << " revision " << STRINGIFY(SVNREV) << endl;

  // Kommandozeile verarbeiten
  get_options(argc, argv);

  Fl::visual(FL_DOUBLE | FL_INDEX);
  Fl::lock();

  msg_win = new CTLDialogMsg();

  dialog_main = new CTLDialogMain(dls_dir);
  dialog_main->show();
  delete dialog_main;

  delete msg_win;

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

  // Weitere Parameter vorhanden?
  if (optind < argc)
  {
    print_usage();
  }

  if (!dir_set)
  {
    // DLS-Verzeichnis aus Umgebungsvariable $DLS_DIR einlesen
    if ((env = getenv(ENV_DLS_DIR)) != 0) dls_dir = env;

    // $DLS_DIR leer: Standardverzeichnis nutzen
    else dls_dir = DEFAULT_DLS_DIR;
  }

  // Benutztes Verzeichnis ausgeben
  cout << "DLS-Datenverzeichnis \"" << dls_dir << "\"" << endl;
}

/*****************************************************************************/

void print_usage()
{
  cout << "Aufruf: dls_ctl [OPTIONEN]" << endl;
  cout << "        -d [Verzeichnis]   DLS-Datenverzeichnis angeben" << endl;
  cout << "        -h                 Diese Hilfe anzeigen" << endl;
  exit(0);
}

/*****************************************************************************/
