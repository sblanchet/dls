/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef CTLDialogChannelsHpp
#define CTLDialogChannelsHpp

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

#include "fl_grid.hpp"

/*****************************************************************************/

/**
   Auswahldialog f�r Kan�le der Datenquelle
*/

class CTLDialogChannels
{
public:
    CTLDialogChannels(const string &, uint16_t);
    ~CTLDialogChannels();

    void show();

    const list<COMRealChannel> *channels() const;

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
    vector<COMRealChannel> _channels; /**< Vektor mit den geladenen Kan�len */
    string _error; /**< Fehlerstring, wird vom Thread gesetzt */
    list<COMRealChannel> _selected; /**< Liste mit ausgew�hlten Kan�len */

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

inline const list<COMRealChannel> *CTLDialogChannels::channels() const
{
    return &_selected;
}

/*****************************************************************************/

#endif
