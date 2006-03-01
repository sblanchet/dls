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

class CTLDialogMsg;

extern CTLDialogMsg *msg_win;

//---------------------------------------------------------------

// Versions-String mit Build-Nummer aus ctl_build.cpp
extern const char *ctl_version_str;

//---------------------------------------------------------------

/**
   Nachricht des DLSD mit Zeitpunkt, Typ und Text
*/

struct CTLMessage
{
  string time;
  int type;
  string text;
};

//---------------------------------------------------------------

#endif
