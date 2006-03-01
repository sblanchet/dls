
//---------------------------------------------------------------

#include "view_globals.hpp"

//---------------------------------------------------------------

#define VIEW_BUILD 5 // Build-Nummer, wird automatisch erhöht!

#ifndef BUILDER
#define BUILDER ???
#endif

#ifdef DEBUG_INFO
#define DEBUG_INFO_STR " (with debug symbols)"
#else
#define DEBUG_INFO_STR ""
#endif

//---------------------------------------------------------------

const char *view_version_str = view_version_str =

  "dls_view build " STRINGIFY(VIEW_BUILD)
  " (DLS version " DLS_VERSION_STR ")"
  " - " __DATE__ ", " __TIME__
  " - built by " STRINGIFY(BUILDER)
  DEBUG_INFO_STR;

//---------------------------------------------------------------
