//---------------------------------------------------------------
//
//  V I E W _ G L O B A L S . H P P
//
//---------------------------------------------------------------

#ifndef ViewGlobalsHpp
#define ViewGlobalsHpp

//---------------------------------------------------------------

#include "com_globals.hpp"
#include "com_time.hpp"

//---------------------------------------------------------------

#define READ_RING_SIZE 100000
#define WATCH_ALERT 3

//---------------------------------------------------------------

// Versions-String mit Build-Nummer aus dls_build.cpp
extern const char *view_version_str;

//---------------------------------------------------------------

#define MSG_COUNT 5

#define MSG_UNKNOWN   -1
#define MSG_INFO      0
#define MSG_WARNING   1
#define MSG_ERROR     2
#define MSG_CRITICAL  3
#define MSG_BROADCAST 4

//---------------------------------------------------------------

/**
   MSR-Nachricht mit Zeit, Typ, Text und Ebene der Anzeige
*/

struct ViewMSRMessage
{
  COMTime time;
  int type;
  string text;

  int level;
};

//---------------------------------------------------------------

#endif
