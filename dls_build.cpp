
//---------------------------------------------------------------

#include "dls_globals.hpp"

//---------------------------------------------------------------

#define DLS_BUILD 203 // Build-Nummer, wird automatisch erhöht!

#ifndef BUILDER
#define BUILDER ???
#endif

#ifdef DEBUG_INFO
#define DEBUG_INFO_STR " (with debug symbols)"
#else
#define DEBUG_INFO_STR ""
#endif

//---------------------------------------------------------------

const char *dls_version_str = dls_version_str =

  "dlsd build " STRINGIFY(DLS_BUILD)
  " (DLS version " DLS_VERSION_STR ")"
  " - " __DATE__ ", " __TIME__
  " - built by " STRINGIFY(BUILDER)
  DEBUG_INFO_STR;

//---------------------------------------------------------------
