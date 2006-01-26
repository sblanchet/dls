//---------------------------------------------------------------
//
//  C O M _ G L O B A L S . H P P
//
//---------------------------------------------------------------

#ifndef COMGlobalsHpp
#define COMGlobalsHpp

//---------------------------------------------------------------

#define MSRD_PORT 2345
#define REC_BUFFER_SIZE 4096 // Bytes

#define DEFAULT_DLS_DIR "/vol/dls_data"

#define E_DLS_NO_ERROR (0)
#define E_DLS_ERROR (-1)
#define E_DLS_TIME_TOLERANCE (-2)

#define MDCT_MIN_EXP 4           // Minimal 16 Werte für MDCT
#define MDCT_MAX_EXP_PLUS_ONE 11 // Maximal 1024 Werte für MDCT

#define DLS_FORMAT_COUNT 1

#define DLS_FORMAT_INVALID -1
#define DLS_FORMAT_ZLIB 0
#define DLS_FORMAT_MDCT 1

extern const char *dls_format_strings[DLS_FORMAT_COUNT];

//---------------------------------------------------------------

#endif
