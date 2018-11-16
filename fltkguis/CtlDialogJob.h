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

#ifndef CtlDialogJobH
#define CtlDialogJobH

/*****************************************************************************/

#include <vector>
using namespace std;

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>

/*****************************************************************************/

#include "fl_grid.h"
#include "CtlGlobals.h"
#include "CtlJobPreset.h"

/*****************************************************************************/

/**
   Dialog for editing a job specification
*/

class CtlDialogJob
{
public:
    CtlDialogJob(const string &);
    ~CtlDialogJob();

    void show(CtlJobPreset *);
    bool updated() const;
    bool imported() const;

private:
    Fl_Double_Window *_wnd; /**< Dialog box */
    Fl_Button *_button_close; /**< "Close" - Button */
    Fl_Output *_output_desc; /**< Output field for the job description */
    Fl_Output *_output_source; /**< Output filed for the data source */
    Fl_Output *_output_trigger; /**< Output field for the trigger parameters */
    Fl_Button *_button_change; /**< Button to change the description */
    Fl_Grid *_grid_channels; /**< The grid to display the channels */
    Fl_Button *_button_add; /**< Button to add channels */
    Fl_Button *_button_rem; /**< Button to remove the selected channels */
    Fl_Button *_button_edit; /**< Button to edit the selected channels */
    string _dls_dir; /**< DLS data directory */
    CtlJobPreset *_job; /**< Pointer to the job to be processed */
    bool _changed; /**< Flag, that indicates if something has changed
                      on the job */
    bool _updated; /**< Flag, that indicates if the job
                      has been changed */

    static void _callback(Fl_Widget *, void *);
    void _grid_channels_callback();
    void _button_close_clicked();
    void _button_change_clicked();
    void _button_add_clicked();
    void _button_rem_clicked();
    void _button_edit_clicked();

    bool _load_channels();

    bool _save_job();
    void _edit_channels();
    void _insert_channels(const list<LibDLS::RealChannel> *);
    void _remove_channels(const list<unsigned int> *);

    void _grid_selection_changed();
};

/*****************************************************************************/

/**
   Return whether the job has been modified and saved

   \return true, if changed and saved
*/

inline bool CtlDialogJob::updated() const
{
    return _updated;
}

/*****************************************************************************/

#endif
