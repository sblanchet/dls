/******************************************************************************
 *
 *  $Id$
 *
 *  This file is part of the Data Logging Service (DLS).
 *
 *  DLS is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  DLS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with DLS. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef ViewGlobalsHpp
#define ViewGlobalsHpp

/*****************************************************************************/

#include "com_globals.hpp"
#include "com_time.hpp"

/*****************************************************************************/

#define READ_RING_SIZE 100000
#define WATCH_ALERT 3

/*****************************************************************************/

// Versions-String mit Build-Nummer aus dls_build.cpp
extern const char *view_version_str;

/*****************************************************************************/

#define MSG_COUNT 5

#define MSG_UNKNOWN   -1
#define MSG_INFO      0
#define MSG_WARNING   1
#define MSG_ERROR     2
#define MSG_CRITICAL  3
#define MSG_BROADCAST 4

/*****************************************************************************/

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

/*****************************************************************************/

#endif
