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

#ifndef ViewDialogExportHpp
#define ViewDialogExportHpp

/*****************************************************************************/

#include <pthread.h>

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Box.H>

#include "lib/LibDLS/Export.h"

#include "ViewChannel.h"

/*****************************************************************************/

/**
   Export dialog of the DLS viewer
*/

class ViewDialogExport
{
public:
    ViewDialogExport(const string &);
    ~ViewDialogExport();

    void show(list<LibDLS::Channel> *, LibDLS::Time, LibDLS::Time);

private:
    string _dls_dir;           /**< DLS data directory */
    Fl_Double_Window *_wnd;    /**< Dialog box */
    Fl_Box *_box_info; /**< Info line */
    Fl_Output *_output_times; /**< Timestamp */
#if 0
    Fl_Output *_output_time; /**< Remaining time */
    Fl_Output *_output_size; /**< Estimated size */
#endif
    Fl_Check_Button *_check_ascii; /**< export to ASCII */
    Fl_Check_Button *_check_mat4; /**< export to MATLAB level 4 file */
    Fl_Spinner *_spinner_decimation; /**< Decimation spinner. */
    Fl_Progress *_progress; /**< Progress */
    Fl_Button *_button_export; /**< Export-Button */
    Fl_Button *_button_close;  /**< "Close"-Button */

    list<LibDLS::Channel> *_channels;
    unsigned int _channel_count;
    LibDLS::Time _start, _end;
    list<LibDLS::Export *> _exporters;
    unsigned int _decimation;
    string _export_dir;
    bool _export_finished;
    pthread_t _thread; /**< Export-Thread */
    bool _thread_running; /**< true, when the thread is running */

    static void _callback(Fl_Widget *, void *);
    void _button_close_clicked();
    void _button_export_clicked();
    void _set_progress_value(int);

    static void *_static_thread_function(void *);
    void _thread_function();

    static int _export_data_callback(LibDLS::Data *, void *);
    void _clear_exporters();
};

/*****************************************************************************/

#endif
