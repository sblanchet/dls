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

#ifndef CtlDialogJobEditH
#define CtlDialogJobEditH

/*****************************************************************************/

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>

/*****************************************************************************/

#include "CtlJobPreset.h"

/*****************************************************************************/

/**
   Dialog to edit the basic setting of an acquisition job
*/

class CtlDialogJobEdit
{
public:
    CtlDialogJobEdit(const string &);
    ~CtlDialogJobEdit();

    void show(CtlJobPreset *);
    bool updated();

private:
    Fl_Double_Window *_wnd; /**< Dialog box */
    Fl_Return_Button *_button_ok; /**< Button OK */
    Fl_Button *_button_cancel; /**< Button Cancel */
    Fl_Input *_input_desc; /**< Input field for the description */
    Fl_Input *_input_source; /**< Input field for the datasource */
    Fl_Input *_input_trigger; /**< Input field for the trigger parameter */
    Fl_Input *_input_quota_time; /**< Input field for the time quota */
    Fl_Choice *_choice_time_ext; /**< Selection menu for the time unit */
    Fl_Input *_input_quota_size; /**< Input field for the data quota */
    Fl_Choice *_choice_size_ext; /**< Selection menu for the data unit */

    string _dls_dir;    /**< DLS data directory */
    CtlJobPreset *_job; /**< Pointer to the job settings */
    bool _updated;      /**< Has something been changed? */

    static void _callback(Fl_Widget *, void *);
    void _button_ok_clicked();
    void _button_cancel_clicked();
    bool _save_job();
    bool _create_job();
    bool _get_new_id(int *);
    void _display_time_quota();
    void _display_size_quota();
    bool _calc_time_quota(uint64_t *);
    bool _calc_size_quota(uint64_t *);
};

/*****************************************************************************/

/**
   Return if something has been changed

   \return true, if data has been changed
*/

inline bool CtlDialogJobEdit::updated()
{
    return _updated;
}

/*****************************************************************************/

#endif
