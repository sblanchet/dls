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

#include "dls_job_preset.hpp"
#include "dls_globals.hpp"

//---------------------------------------------------------------

/**
   DLS-Mutterprozess

   Startet und �berwacht die erfassenden DLSProcLogger-Prozesse.
   �berwacht gleichzeitig die Erfassungs-Vorgaben per Spooling
   und signalisiert den Erfassungsprozessen aufgetretene �nderungen
*/

class DLSProcMother
{
public:
  DLSProcMother();
  ~DLSProcMother();

  int start(const string &);

private:
  string _dls_dir;             /**< DLS-Datenverzeichnis */
  list<DLSJobPreset> _jobs;    /**< Liste von Auftragsvorgaben */
  unsigned int _sig_child;     /**< Z�hler f�r empfangene SIGCHLD-Signale */
  bool _exit;                  /**< true, wenn der Prozess beendet werden soll */
  bool _exit_error;            /**< true, wenn die Beendigung mit Fehler erfolgen soll */
  stringstream _msg;           /**< Auszugebende Nachricht */

  void _empty_spool();
  void _check_jobs();
  void _check_signals();
  void _check_spool();
  void _check_processes();
  DLSJobPreset *_job_exists(int);
  void _log(DLSLogType);
  unsigned int _processes_running();
};

//---------------------------------------------------------------

#endif


