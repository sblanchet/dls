//---------------------------------------------------------------
//
//  D L S _ P R O C _ M O T H E R . H P P
//
//---------------------------------------------------------------

#ifndef DLSProcMotherHpp
#define DLSProcMotherHpp

//---------------------------------------------------------------

#include <string>
#include <list>
#include <sstream>
using namespace std;

//---------------------------------------------------------------

#include "com_job_preset.hpp"
#include "dls_globals.hpp"

//---------------------------------------------------------------

/**
   DLS-Mutterprozess

   Startet und überwacht die erfassenden DLSProcLogger-Prozesse.
   Überwacht gleichzeitig die Erfassungs-Vorgaben per Spooling
   und signalisiert den Erfassungsprozessen aufgetretene Änderungen
*/

class DLSProcMother
{
public:
  DLSProcMother();
  ~DLSProcMother();

  int start(const string &);

private:
  string _dls_dir;          /**< DLS-Datenverzeichnis */
  list<COMJobPreset> _jobs; /**< Liste von Auftragsvorgaben */
  unsigned int _sig_child;  /**< Zähler für empfangene SIGCHLD-Signale */
  bool _exit;               /**< true, wenn der Prozess beendet werden soll */
  bool _exit_error;         /**< true, wenn die Beendigung mit Fehler erfolgen soll */
  stringstream _msg;        /**< Auszugebende Nachricht */

  void _empty_spool();
  void _check_jobs();
  void _check_signals();
  void _check_spool();
  void _check_processes();
  COMJobPreset *_job_exists(int);
  void _child_process_forked(string);
  static void _signal_callback_handler(int, void *, void *);
  void _set_exit_code(struct process_exit *);
  void _log(DLSLogType);
  bool _process_exists(COMJobPreset *);
  void _process_term(COMJobPreset *);
  void _process_notify(const COMJobPreset *);
  void _add_job(int);
};

//---------------------------------------------------------------

#endif


