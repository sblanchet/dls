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

/*****************************************************************************/

#include "lib/globals.h"

#include "fl_grid.h"

/*****************************************************************************/

/**
   Auswahldialog f�r Kan�le der Datenquelle
*/

class CtlDialogChannels
{
public:
    CtlDialogChannels(const string &, uint16_t);
    ~CtlDialogChannels();

    void show();

    const list<LibDLS::RealChannel> *channels() const;

private:
    Fl_Double_Window *_wnd;    /**< Dialogfenster */
    Fl_Button *_button_ok;     /**< "OK"-Button */
    Fl_Button *_button_cancel; /**< "Abbrechen"-Button */
    Fl_Grid *_grid_channels;   /**< Grid f�r die MSR-Kan�le */
    Fl_Box *_box_message;      /**< Box f�r die Fehleranzeige */
    Fl_Check_Button *_checkbutton_reduceToOneHz;

    string _source; /**< IP-Adresse/Hostname der Datenquelle */
    uint16_t _port; /**< Port der Datenquelle */
    int _socket; /**< File-Deskriptor f�r die TCP-Verbindung */
    pthread_t _thread; /**< Thread f�r die Abfrage */
    bool _thread_running; /**< true, wenn der Thread l�uft */
    bool _imported; /**< true, wenn alle Kan�le importiert wurden */
    vector<LibDLS::RealChannel> _channels; /**< Vektor mit den geladenen Kan�len */
    string _error; /**< Fehlerstring, wird vom Thread gesetzt */
    list<LibDLS::RealChannel> _selected; /**< Liste mit ausgew�hlten Kan�len */

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
   Liefert die Liste der ausgew�hlten Kan�le
*/

inline const list<LibDLS::RealChannel> *CtlDialogChannels::channels() const
{
    return &_selected;
}

/*****************************************************************************/

#endif
