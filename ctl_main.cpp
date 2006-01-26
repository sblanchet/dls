//---------------------------------------------------------------
//
//  C T L _ M A I N . C P P
//
//---------------------------------------------------------------

#include <unistd.h>

#include <iostream>
#include <string>
using namespace std;

#include <FL/Fl.h>

//---------------------------------------------------------------

#include "com_dialog_msg.hpp"
#include "ctl_globals.hpp"
#include "ctl_dialog_main.hpp"
#include "ctl_msg_wnd.hpp"

//---------------------------------------------------------------

RCS_ID("$Header: /home/fp/dls/src/RCS/ctl_main.cpp,v 1.5 2005/01/24 13:08:54 fp Exp $");

//---------------------------------------------------------------

COMDialogMsg *msg = 0;
string dls_dir;

void get_options(int, char **);
void print_usage();

//---------------------------------------------------------------

int main(int argc, char **argv)
{
  CTLDialogMain *dialog_main;

  cout << ctl_version_str << endl;

  // Kommandozeile verarbeiten
  get_options(argc, argv);

  Fl::visual(FL_DOUBLE | FL_INDEX);
  Fl::lock();

  msg = new COMDialogMsg();

  dialog_main = new CTLDialogMain(dls_dir);
  dialog_main->show();
  delete dialog_main;

  delete msg;

  return 0;
}

//---------------------------------------------------------------

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
  cout << "using dls directory \"" << dls_dir << "\"" << endl;
}

//---------------------------------------------------------------

void print_usage()
{
  cout << "Aufruf: dls_ctl [OPTIONEN]" << endl;
  cout << "        -d [Verzeichnis]   DLS-Datenverzeichnis angeben" << endl;
  cout << "        -h                 Diese Hilfe anzeigen" << endl;
  exit(0);
}

//---------------------------------------------------------------
