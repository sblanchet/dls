//---------------------------------------------------------------
//
//  C T L _ J O B _ P R E S E T . H P P
//
//---------------------------------------------------------------

#ifndef CTLJobPresetHpp
#define CTLJobPresetHpp

//---------------------------------------------------------------

#include <string>
using namespace std;

//---------------------------------------------------------------

#include "com_job_preset.hpp"

//---------------------------------------------------------------

/**
   Erweiterung von COMJobPreset für den DLS-Manager
*/

class CTLJobPreset : public COMJobPreset
{
public:
  using COMJobPreset::description;
  using COMJobPreset::source;
  using COMJobPreset::trigger;
  using COMJobPreset::running;
  using COMJobPreset::id;
  using COMJobPreset::quota_size;
  using COMJobPreset::quota_time;

  CTLJobPreset();

  void write(const string &);
  void spool(const string &);

  void id(unsigned int);
  void description(const string &);
  void running(bool);
  void source(const string &);
  void trigger(const string &);
  void quota_time(long long);
  void quota_size(long long);

  void toggle_running();
  void add_channel(const COMChannelPreset *);
  void change_channel(const COMChannelPreset *);
  void remove_channel(const string &);

  time_t process_watchdog;        /**< Zeitstempel der Watchdog-Datei */
  unsigned int process_bad_count; /**< Anzahl letzter Watchdog-Prüfungen,
                                       ohne dass sich etwas geändert hat */
  bool process_watch_determined;  /**< Watchdog-Information steht fest */
  time_t logging_watchdog;        /**< Zeitstempel der Logging-Watchdog-Datei */
  unsigned int logging_bad_count; /**< Anzahl letzter Logging-Watchdog-Prüfungen,
                                       ohne dass sich etwas geändert hat */
  bool logging_watch_determined;  /**< Logging-Watchdog-Information steht fest */
};

//---------------------------------------------------------------

#endif
