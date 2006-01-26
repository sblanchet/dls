//---------------------------------------------------------------
//
//  C O M _ G L O B A L S . H P P
//
//---------------------------------------------------------------

#ifndef COMGlobalsHpp
#define COMGlobalsHpp

//---------------------------------------------------------------

#include <string>
using namespace std;

//---------------------------------------------------------------

#define DLS_MASTER_VERSION 0
#define DLS_SUB_VERSION    9

#define LITERAL(X) #X
#define STRINGIFY(X) LITERAL(X)

#define DLS_VERSION_STR STRINGIFY(DLS_MASTER_VERSION) "." STRINGIFY(DLS_SUB_VERSION)
#define DLS_VERSION_CODE(M, S) (((M) << 8) | (S))
#define DLS_VERSION (DLS_VERSION_CODE(DLS_MASTER_VERSION, DLS_SUB_VERSION))

#define RCS_ID(ID) static char *RCSID = RCSID = "RCS ID: " ID 

#define MSRD_PORT 2345
#define REC_BUFFER_SIZE 4096 // Bytes

#define DEFAULT_DLS_DIR "/vol/dls_data"
#define DLS_PID_FILE "dlsd.pid"
#define ENV_DLS_DIR "DLS_DIR"           // Name der Umgebungsvariable

#define E_DLS_NO_ERROR (0)
#define E_DLS_ERROR (-1)
#define E_DLS_TIME_TOLERANCE (-2)
#define E_DLS_SIGNAL (-3)

#define MDCT_MIN_EXP2 4           // Minimal 16 Werte für MDCT
#define MDCT_MAX_EXP2_PLUS_ONE 11 // Maximal 1024 Werte für MDCT

#if DLS_VERSION == DLS_VERSION_CODE(0, 8)
#define DLS_FORMAT_COUNT 1
#else
#define DLS_FORMAT_COUNT 2
#endif

#define DLS_FORMAT_INVALID -1
#define DLS_FORMAT_ZLIB 0
#define DLS_FORMAT_MDCT 1

extern const char *dls_format_strings[DLS_FORMAT_COUNT];

//---------------------------------------------------------------

/*
  Hinzufügen von Kanaltypen:

  Editiert werden muss an folgenden Stellen:

  - Die beiden Konvertierungsfunktionen in com_globals.cpp
  - In DLSLogger::create_gen_saver()
  - In ViewChannel::fetch_chunks()

*/

enum COMChannelType
{
  TCHAR,
  TUCHAR,
  TINT,
  TUINT,
  TLINT,
  TULINT,
  TFLT,
  TDBL
};

COMChannelType dls_str_to_channel_type(const string &);
char *dls_channel_type_to_str(COMChannelType);

//---------------------------------------------------------------

/**
   Index-Record für einen Datendatei-Index innerhalb eines Chunks
*/

struct COMIndexRecord
{
  long long start_time;
  long long end_time;
  unsigned int position;
};

//---------------------------------------------------------------

/**
   Index für alle Datendateien eines Chunks
*/

struct COMGlobalIndexRecord
{
  long long start_time;
  long long end_time;
};

//---------------------------------------------------------------

/**
   Index für Messages
*/

struct COMMessageIndexRecord
{
  long long time;
  unsigned int position;
};

//---------------------------------------------------------------

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

//---------------------------------------------------------------

#endif
