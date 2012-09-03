/******************************************************************************
 *
 *  $Id$
 *
 *  This file is part of the Data Logging Service (DLS).
 *
 *  DLS is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  DLS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with DLS. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef DLSJobPresetHpp
#define DLSJobpresetHpp

/*****************************************************************************/

#include <string>
#include <list>
#include <sstream>
using namespace std;

/*****************************************************************************/

#include "com_job_preset.hpp"

/*****************************************************************************/

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
    pid_t _pid; /**< PID des ge'fork'ten Kindprozesses */
    int _last_exit_code; /**< Exitcode des letzten Prozesses */
    COMTime _exit_time; /**< Beendigungszeit des letzten Prozesses */
};

/*****************************************************************************/

inline pid_t DLSJobPreset::process_id() const
{
    return _pid;
}

/*****************************************************************************/

inline int DLSJobPreset::last_exit_code() const
{
    return _last_exit_code;
}

/*****************************************************************************/

inline COMTime DLSJobPreset::exit_time() const
{
    return _exit_time;
}

/*****************************************************************************/

#endif


