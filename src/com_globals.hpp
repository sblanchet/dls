/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef COMGlobalsHpp
#define COMGlobalsHpp

/*****************************************************************************/

#include <string>
#include <sstream>
using namespace std;

/*****************************************************************************/

#define DLS_MASTER_VERSION  1
#define DLS_SUB_VERSION     0
#define DLS_SUB_SUB_VERSION 0

#define LITERAL(X) #X
#define STRINGIFY(X) LITERAL(X)

#define DLS_VERSION_STR STRINGIFY(DLS_MASTER_VERSION) \
                        "." STRINGIFY(DLS_SUB_VERSION) \
                        "." STRINGIFY(DLS_SUB_SUB_VERSION)

#define DLS_VERSION_CODE(M, S, U) (((M) << 16) | ((S) << 8) | (U))

#define DLS_VERSION (DLS_VERSION_CODE(DLS_MASTER_VERSION, \
                                      DLS_SUB_VERSION, \
                                      DLS_SUB_SUB_VERSION))

#define MSRD_PORT 2345

#define REC_BUFFER_SIZE         4096 // Bytes
#define COMPRESSION_BUFFER_SIZE 8192 // Bytes. Wird nach Belieben verdoppelt.

#define DEFAULT_DLS_DIR "/vol/dls_data"
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

/**
   Index-Record für einen Datendatei-Index innerhalb eines Chunks
*/

struct COMIndexRecord
{
    long long start_time;
    long long end_time;
    unsigned int position;
};

/*****************************************************************************/

/**
   Index für alle Datendateien eines Chunks
*/

struct COMGlobalIndexRecord
{
    long long start_time;
    long long end_time;
};

/*****************************************************************************/

/**
   Index für Messages
*/

struct COMMessageIndexRecord
{
    long long time;
    unsigned int position;
};

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
void log(DLSLogType, int = 0);

COMChannelType dls_str_to_channel_type(const string &);
char *dls_channel_type_to_str(COMChannelType);
string convert_to_bin(const void *, unsigned int, int);

/*****************************************************************************/

// Globale Variablen

// Format-Strings
extern const char *dls_format_strings[DLS_FORMAT_COUNT];

// Daemon
extern bool is_daemon;

/*****************************************************************************/

#endif
