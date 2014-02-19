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

#ifndef COMGlobalsHpp
#define COMGlobalsHpp

/*****************************************************************************/

#include <stdint.h>

#include <string>
#include <sstream>
using namespace std;

#include "../config.h"
#include "lib_globals.hpp"

/*****************************************************************************/

#define MSRD_PORT 2345

#define REC_BUFFER_SIZE         4096 // Bytes
#define COMPRESSION_BUFFER_SIZE 8192 // Bytes. Wird nach Belieben verdoppelt.

#define DLS_PID_FILE "dlsd.pid"
#define ENV_DLS_DIR "DLS_DIR" // Name der Umgebungsvariablen
#define ENV_DLS_USER "DLS_USER" // Name der Umgebungsvariablen

#define DLS_FORMAT_COUNT 3

#define DLS_FORMAT_INVALID (-1)
#define DLS_FORMAT_ZLIB    (0)
#define DLS_FORMAT_MDCT    (1)
#define DLS_FORMAT_QUANT   (2)

/*
  Hinzufügen von Kompressionsarten:
  - com_globals.cpp       (String-Array)
  - com_compression_t.hpp (Neue Klasse)
*/

/*****************************************************************************/

string dls_meta_type_str(DLSMetaType);

/*****************************************************************************/

#pragma pack(push)
#pragma pack(1)

/**
   Index-Record für einen Datendatei-Index innerhalb eines Chunks
*/

struct COMIndexRecord
{
    uint64_t start_time;
    uint64_t end_time;
    uint32_t position;
};

/*****************************************************************************/

/**
   Index für alle Datendateien eines Chunks
*/

struct COMGlobalIndexRecord
{
    uint64_t start_time;
    uint64_t end_time;
};

/*****************************************************************************/

/**
   Index für Messages
*/

struct COMMessageIndexRecord
{
    uint64_t time;
    uint32_t position;
};

#pragma pack(pop)

/*****************************************************************************/

/**
   Beschreibt einen Kanal des MSR-Moduls
*/

struct COMRealChannel
{
    string name;            /**< Name des Kanals */
    string unit;            /**< Einheit */
    COMChannelType type;    /**< Kanaltyp (TUINT, TDBL, usw.) */
    unsigned int frequency; /**< Maximale Abtastrate des Kanals */
};

bool operator<(const COMRealChannel &a, const COMRealChannel &b);

/*****************************************************************************/

COMChannelType dls_str_to_channel_type(const string &);
const char *dls_channel_type_to_str(COMChannelType);
string convert_to_bin(const void *, unsigned int, int);

/*****************************************************************************/

// Globale Variablen

// Format-Strings
extern const char *dls_format_strings[DLS_FORMAT_COUNT];

// Daemon
extern bool is_daemon;

/*****************************************************************************/

#endif
