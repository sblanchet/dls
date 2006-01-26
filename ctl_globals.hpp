//---------------------------------------------------------------
//
//  C T L _ G L O B A L S . H P P
//
//---------------------------------------------------------------

#ifndef CTLGlobalsHpp
#define CTLGlobalsHpp

//---------------------------------------------------------------

#include "com_globals.hpp"

//---------------------------------------------------------------

#define WATCH_ALERT 3

#define META_MASK_FIXED true
#define META_REDUCTION_FIXED true

//---------------------------------------------------------------

struct CTLMessage
{
  string time;
  int type;
  string text;
};

//---------------------------------------------------------------

#endif
