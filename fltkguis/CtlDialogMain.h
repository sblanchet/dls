/******************************************************************************
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

#ifndef CtlDialogMainH
#define CtlDialogMainH

/*****************************************************************************/

#include <vector>
using namespace std;

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Button.H>

/*****************************************************************************/

#include "lib/LibDLS/Time.h"

#include "fl_grid.h"
#include "CtlJobPreset.h"
#include "CtlGlobals.h"

/*****************************************************************************/

/**
   DLS Manager main dialog
*/

class CtlDialogMain
{
public:
    CtlDialogMain(const string &);
    ~CtlDialogMain();

    void show();

private:
    Fl_Double_Window *_wnd; /**< Dialog window */
    Fl_Grid *_grid_jobs; /**< Grid for all entry jobs */
    Fl_Button *_button_close; /**< Button to close dialog */
    Fl_Button *_button_add; /**< Button to add a job */
    Fl_Button *_button_state; /**< Button to start or stop acquisition */

    string _dls_dir;            /**< DLS data directory */
    vector<CtlJobPreset> _jobs; /**< Vector with all acquistion jobs */

    void _edit_job(unsigned int);

    static void _callback(Fl_Widget *, void *);
    void _grid_jobs_callback();
    void _button_close_clicked();
    void _button_state_clicked();
    void _button_add_clicked();

    void _load_jobs();
    void _load_watchdogs();
    void _update_button_state();

    void _check_dls_dir();

    static void _static_watchdog_timeout(void *);

    CtlDialogMain(); // Should not be called
};

/*****************************************************************************/

#endif
