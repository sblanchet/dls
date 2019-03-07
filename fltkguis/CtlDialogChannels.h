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

#ifndef CtlDialogChannelsH
#define CtlDialogChannelsH

/*****************************************************************************/

#include <pthread.h>

#include <vector>
#include <list>
using namespace std;

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Check_Button.H>

#include "lib/LibDLS/globals.h"

#include "fl_grid.h"

/*****************************************************************************/

/**
   Selection dialog for channels of the data source
*/

class CtlDialogChannels
{
public:
    CtlDialogChannels(const string &, uint16_t);
    ~CtlDialogChannels();

    void show();

    const list<LibDLS::RealChannel> *channels() const;

private:
    Fl_Double_Window *_wnd;    /**< Dialog box */
    Fl_Button *_button_ok;     /**< "OK" button */
    Fl_Button *_button_cancel; /**< "Cancel" button */
    Fl_Grid *_grid_channels;   /**< Grid for the MSR channels */
    Fl_Box *_box_message;      /**< Box for the error display */
    Fl_Check_Button *_checkbutton_reduceToOneHz;

    string _source; /**< IP-Adress/Hostname of the data source */
    uint16_t _port; /**< Port of the data source */
    int _socket; /**< File descriptor for the TCP connection */
    pthread_t _thread; /**< Thread for the query */
    bool _thread_running; /**< true, if the thread is running */
    bool _imported; /**< true, if all channels have been imported */
    vector<LibDLS::RealChannel> _channels; /**< Vector with the charged channels */
    string _error; /**< Error string is set by the thread */
    list<LibDLS::RealChannel> _selected; /**< List of selected channels */

    static void _callback(Fl_Widget *, void *);
    void _grid_channels_callback();
    void _button_ok_clicked();
    void _button_cancel_clicked();

    static void *_static_thread_function(void *);
    void _thread_function();

    void _thread_finished();

 };

/*****************************************************************************/

/**
   Return the list of selected channels
*/

inline const list<LibDLS::RealChannel> *CtlDialogChannels::channels() const
{
    return &_selected;
}

/*****************************************************************************/

#endif
