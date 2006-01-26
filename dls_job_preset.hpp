//---------------------------------------------------------------
//
//  D L S _ J O B _ P R E S E T . H P P
//
//---------------------------------------------------------------

#ifndef DLSJobPresetHpp
#define DLSJobpresetHpp

//---------------------------------------------------------------

#include <string>
#include <list>
#include <sstream>
using namespace std;

//---------------------------------------------------------------

#include "com_job_preset.hpp"

//---------------------------------------------------------------

/**
   Erweiterung von COMJobPreset für den DLS-Mutterprozess

   Im Kontext des dlsd ist eine Auftragsvorgabe immer
   mit einem Prozess verbunden. Diese Spezialisierung
   fügt alles hinzu, was mit PIDs, Exit-Codes usw. zu
   tun hat.
*/

class DLSJobPreset : public COMJobPreset
{
public:
  DLSJobPreset();

  void process_started(pid_t);
  void process_exited(int);

  void process_terminate();
  void process_notify();
  void allow_restart();

  pid_t process_id() const;
  int last_exit_code() const;
  COMTime exit_time() const;

  bool process_exists();

private:
  pid_t _pid;                  /**< PID des ge'fork'ten Kindprozesses */
  int _last_exit_code; /**< Exitcode des letzten Prozesses */
  COMTime _exit_time;          /**< Beendigungszeit des letzten Prozesses */
};

//---------------------------------------------------------------

inline pid_t DLSJobPreset::process_id() const
{
  return _pid;
}

//---------------------------------------------------------------

inline int DLSJobPreset::last_exit_code() const
{
  return _last_exit_code;
}

//---------------------------------------------------------------

inline COMTime DLSJobPreset::exit_time() const
{
  return _exit_time;
}

//---------------------------------------------------------------

#endif


