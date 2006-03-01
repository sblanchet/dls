
//---------------------------------------------------------------

#include "ctl_globals.hpp"

//---------------------------------------------------------------

#define CTL_BUILD 4 // Build-Nummer, wird automatisch erhöht!

#ifndef BUILDER
#define BUILDER ???
#endif

#ifdef DEBUG_INFO
#define DEBUG_INFO_STR " (with debug symbols)"
#else
#define DEBUG_INFO_STR ""
#endif

//---------------------------------------------------------------

const char *ctl_version_str = ctl_version_str =

  "dls_ctl build " STRINGIFY(CTL_BUILD)
  " (DLS version " DLS_VERSION_STR ")"
  " - " __DATE__ ", " __TIME__
  " - built by " STRINGIFY(BUILDER)
  DEBUG_INFO_STR;

//---------------------------------------------------------------
