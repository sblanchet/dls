/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef DLSGlobalsHpp
#define DLSGlobalsHpp

/*****************************************************************************/

#include "com_globals.hpp"

/*****************************************************************************/

#define JOB_CHECK_INTERVAL     1        // in Sekunden
#define LISTEN_TIMEOUT         1.0      // in Sekunden
#define TRIGGER_INTERVAL       2        // in Sekunden
#define WATCHDOG_INTERVAL      1        // in Sekunden
#define RECEIVE_RING_BUF_SIZE  1048576  // 1MB
#define SAVER_MAX_FILE_SIZE    10485760 // 10MB
#define ALLOWED_TIME_VARIANCE  500      // in Prozent rel. Fehler
#define TIME_TOLERANCE_RESTART 30       // in Sekunden
#define BUFFER_LEVEL_WARNING   90       // in Prozent Füllstand
#define QUOTA_PART_QUOTIENT    10       // Anzahl Chunks in Quota-Bereich
#define NO_DATA_WARNING        61       // Sekunden, nach denen gewarnt wird
#define WRITE_TIME_WARNING     1.0      // Sekunden

#define MSR_VERSION(V, P, S) (((V) << 16) + ((P) << 8) + (S))
#define MSR_V(CODE) (((CODE) >> 16) & 0xFF)
#define MSR_P(CODE) (((CODE) >> 8) & 0xFF)
#define MSR_S(CODE) ((CODE) & 0xFF)

#define E_DLS_NO_ERROR       (0)
#define E_DLS_ERROR          (-1)
#define E_DLS_TIME_TOLERANCE (-2)
#define E_DLS_SIGNAL         (-3)

/*****************************************************************************/

enum DLSArchitecture
{
    LittleEndian,
    BigEndian
};

enum DLSProcessType
{
    dlsMotherProcess,
    dlsLoggingProcess,
    dlsCleanupProcess
};

/*****************************************************************************/

// Globale Variablen

// Signal-Zähler
extern unsigned int sig_int_term;
extern unsigned int sig_hangup;
extern unsigned int sig_child;

// Architektur
extern DLSArchitecture arch;
extern DLSArchitecture source_arch;

// Forking
extern enum DLSProcessType process_type;
extern unsigned int job_id;

// Versions-String mit Build-Nummer aus dls_build.cpp
extern const char *dls_version_str;

/*****************************************************************************/

#endif

