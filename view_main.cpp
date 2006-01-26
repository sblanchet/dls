//---------------------------------------------------------------
//
//  V I E W _ M A I N . C P P
//
//---------------------------------------------------------------

#include <iostream>
#include <string>
using namespace std;

#include <FL/Fl.h>

//---------------------------------------------------------------

#include "view_globals.hpp"
#include "view_dialog_main.hpp"

//---------------------------------------------------------------

RCS_ID("$Header: /home/fp/dls/src/RCS/view_main.cpp,v 1.3 2005/01/24 13:16:45 fp Exp $");

//---------------------------------------------------------------

string dls_dir;

void get_options(int, char **);
void print_usage();

//---------------------------------------------------------------

int main(int argc, char **argv)
{
  ViewDialogMain *dialog;

  cout << view_version_str << endl;

  // Kommandozeile verarbeiten
  get_options(argc, argv);

  Fl::visual(FL_DOUBLE | FL_INDEX);

  dialog = new ViewDialogMain(dls_dir);
  dialog->show();
  delete dialog;

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
  cout << "Using DLS directory \"" << dls_dir << "\"" << endl;
}

//---------------------------------------------------------------

void print_usage()
{
  cout << "Aufruf: dls_view [OPTIONEN]" << endl;
  cout << "        -d [Verzeichnis]   DLS-Datenverzeichnis angeben" << endl;
  cout << "        -h                 Diese Hilfe anzeigen" << endl;
  exit(0);
}

//---------------------------------------------------------------
