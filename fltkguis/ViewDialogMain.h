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

#ifndef ViewDialogMainHpp
#define ViewDialogMainHpp

/*****************************************************************************/

#include <vector>
using namespace std;

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Check_Button.H>

#include "lib/LibDLS/Dir.h"
#include "lib/LibDLS/JobPreset.h"

#include "fl_grid.h"
#include "ViewViewData.h"
#include "ViewViewMsg.h"

/*****************************************************************************/

/**
   Main dialog of the DLS viewer
*/

class ViewDialogMain
{
public:
    ViewDialogMain(const string &);
    ~ViewDialogMain();

    void show();

private:
    string _dls_dir_path; /**< DLS data directory */
    Fl_Double_Window *_wnd; /**< Dialog box */
    Fl_Tile *_tile_ver; /**< Vertical separator between ads and channel list */
    Fl_Tile *_tile_hor; /**< Horizontal separator between channels and messages */
    Fl_Choice *_choice_job; /**< selection box for job */
    Fl_Button *_button_full; /**< Button to display the entire time span */
    Fl_Button *_button_reload; /**< Button to reload the data */
    Fl_Button *_button_export; /**< Button ton export the data */
    Fl_Check_Button *_checkbutton_messages; /**< Checkbutton to display the messages. */
    Fl_Button *_button_close; /**< "Close"-Button */
    Fl_Grid *_grid_channels; /**< Grid to display the channel list*/

    ViewViewData *_view_data; /**< Display for the channel data */
    ViewViewMsg *_view_msg; /**< Display for the messages */

    LibDLS::Directory _dls_dir; /**< LibDLS directory object */
    LibDLS::Job *_current_job; /**< current job */

    static void _callback(Fl_Widget *, void *);
    void _button_close_clicked();
    void _button_reload_clicked();
    void _button_full_clicked();
    void _button_export_clicked();
    void _checkbutton_messages_clicked();
    void _choice_job_changed();
    void _grid_channels_changed();

    static void _data_range_callback(LibDLS::Time, LibDLS::Time, void *);
};

/*****************************************************************************/

#endif
