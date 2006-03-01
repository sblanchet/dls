/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <signal.h>

/*****************************************************************************/

#include "dls_globals.hpp"
#include "dls_job_preset.hpp"

/*****************************************************************************/

/**
   Konstruktor
*/

DLSJobPreset::DLSJobPreset() : COMJobPreset()
{
  _pid = 0;
  _last_exit_code = 0;
  _exit_time = (long long) 0;
}

/*****************************************************************************/

/**
   Erlaubt ein erneutes Starten des Prozesses nach einer Beendigung
*/

void DLSJobPreset::allow_restart()
{
  _last_exit_code = E_DLS_NO_ERROR;
}

/*****************************************************************************/

/**
   Vermerkt, dass ein Prozess gestartet wurde
   
   \param pid Process-ID des gestarteten Prozesses
*/

void DLSJobPreset::process_started(pid_t pid)
{
  _pid = pid;
  _last_exit_code = 0;
  _exit_time = (long long) 0;
}

/*****************************************************************************/

/**
   Vermerkt, dass ein Prozess beendet wurde

   \param exit_code Der Exit-Code des beendeten Prozesses
*/

void DLSJobPreset::process_exited(int exit_code)
{
  _pid = 0;
  _last_exit_code = exit_code;
  _exit_time.set_now();
}

/*****************************************************************************/

/**
   Beendet einen Erfassungsprozess

   \throw ECOMJobPreset Prozess konnte nicht terminiert werden
*/

void DLSJobPreset::process_terminate()
{
  if (_pid == 0) return;

  if (kill(_pid, SIGTERM) == -1)
  {
    throw ECOMJobPreset("kill(): Process not terminated!");
  }
}

/*****************************************************************************/

/**
   Benachrichtigt einen Erfassungsprozess über eine Änderung

   \throw ECOMJobPreset Prozess konnte nicht benachrichtigt werden
*/

void DLSJobPreset::process_notify()
{
  if (_pid == 0) return;

  if (kill(_pid, SIGHUP) == -1)
  {
    throw ECOMJobPreset("Error in kill() - Process not notified!");
  }
}

/*****************************************************************************/

/**
   Prüft, ob zu einem Auftrag ein Erfassungsprozess läuft

   \return true, wenn der Prozess läuft
*/

bool DLSJobPreset::process_exists()
{
  if (_pid == 0) return false;

  if (kill(_pid, 0) == -1)
  {
    if (errno == ESRCH)
    {
      _pid = 0;
      return false;
    }
  }

  return true;
}

/*****************************************************************************/


