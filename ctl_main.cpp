//---------------------------------------------------------------
//
//  C T L _ M A I N . C P P
//
//---------------------------------------------------------------

#include <iostream>
#include <string>
using namespace std;

#include <FL/Fl.h>

#include "ctl_globals.hpp"
#include "ctl_dialog_main.hpp"

//---------------------------------------------------------------

int main(int argc, char **argv)
{
  CTLDialogMain *dialog;
  string dls_dir;
  char *env;

  if (argc == 2) dls_dir = argv[1];
  else if ((env = getenv("DLS_DIR")) != 0) dls_dir = env;
  else dls_dir = DEFAULT_DLS_DIR;

  Fl::visual(FL_DOUBLE | FL_INDEX);
  Fl::lock();

  dialog = new CTLDialogMain(dls_dir);
  dialog->show();
  delete dialog;

  return 0;
}

//---------------------------------------------------------------



