/******************************************************************************
 *
 *  $Id$
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

enum COMChannelType
{
    TUNKNOWN,
    TCHAR,
    TUCHAR,
    TSHORT,
    TUSHORT,
    TINT,
    TUINT,
    TLINT,
    TULINT,
    TFLT,
    TDBL
};

/*
  ---------------- Hinzufügen von Kanaltypen: ---------------------

  Editiert werden muss an folgenden Stellen:

  - Die beiden Konvertierungsfunktionen in com_globals.cpp
  - In DLSLogger::create_gen_saver()
  - In ViewChannel::fetch_chunks()

*/

/*****************************************************************************/

// Beim Erweitern bitte auch die Behandlungszweige
// in "_meta_value()" und "_ending()" anpassen!

enum DLSMetaType
{
    DLSMetaGen = 0,
    DLSMetaMean = 1,
    DLSMetaMin = 2,
    DLSMetaMax = 4
};

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
    int index;              /**< Index */
    COMChannelType type;    /**< Kanaltyp (TUINT, TDBL, usw.) */
    unsigned int bufsize;   /**< Größe des Kanalpuffers im MSR-Modul */
    unsigned int frequency; /**< Maximale Abtastrate des Kanals */
};

bool operator<(const COMRealChannel &a, const COMRealChannel &b);

/*****************************************************************************/

enum DLSLogType
{
    DLSInfo,
    DLSError,
    DLSWarning,
    DLSDebug
};

/*****************************************************************************/

stringstream &msg();
void log(DLSLogType);

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
