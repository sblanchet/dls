//---------------------------------------------------------------
//
//  D L S _ G L O B A L S . H P P
//
//---------------------------------------------------------------

#ifndef DLSGlobalsHpp
#define DLSGlobalsHpp

//---------------------------------------------------------------

#include "com_globals.hpp"

//---------------------------------------------------------------

#define DLS_VERSION "0.8"

#define JOB_CHECK_INTERVAL 1 // Sekunden
#define LISTEN_TIMEOUT 1.0 // Sekunden
#define TRIGGER_INTERVAL 2 // Sekunden
#define WATCHDOG_INTERVAL 1 // Sekunden
#define RECEIVE_RING_BUF_SIZE 1048576 // 1MB
#define SAVER_BUF_SIZE 131072 // 2^17 ~ 100KB
#define SAVER_MAX_FILE_SIZE 10485760 // 10MB
#define ALLOWED_TIME_VARIANCE 500 // Prozent
#define TIME_TOLERANCE_RESTART 30 // Sekunden
#define BUFFER_LEVEL_WARNING 90 // Prozent Füllstand 

#define DEBUG_SEND false
#define DEBUG_REC false
#define DEBUG_DONT_CLOSE_STDXXX false

#define PID_FILE_NAME "dlsd.pid"

#define MSR_VERSION(V, P, S) (((V) << 16) + ((P) << 8) + (S))
#define MSR_V(CODE) (((CODE) >> 16) & 0xFF)
#define MSR_P(CODE) (((CODE) >> 8) & 0xFF)
#define MSR_S(CODE) ((CODE) & 0xFF)

//---------------------------------------------------------------

enum DLSLogType
{
  DLSInfo,
  DLSError,
  DLSWarning
};

//---------------------------------------------------------------

extern unsigned int sig_int_term;
extern unsigned int sig_hangup;
extern unsigned int sig_child;

extern bool process_forked;
extern unsigned int job_id;

//---------------------------------------------------------------

#endif
