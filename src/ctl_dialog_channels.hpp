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
   Auswahldialog für Kanäle der Datenquelle
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
    Fl_Grid *_grid_channels;   /**< Grid für die MSR-Kanäle */
    Fl_Box *_box_message;      /**< Box für die Fehleranzeige */
    Fl_Check_Button *_checkbutton_reduceToOneHz;

    string _source; /**< IP-Adresse/Hostname der Datenquelle */
    uint16_t _port; /**< Port der Datenquelle */
    int _socket; /**< File-Deskriptor für die TCP-Verbindung */
    pthread_t _thread; /**< Thread für die Abfrage */
    bool _thread_running; /**< true, wenn der Thread läuft */
    bool _imported; /**< true, wenn alle Kanäle importiert wurden */
    vector<COMRealChannel> _channels; /**< Vektor mit den geladenen Kanälen */
    string _error; /**< Fehlerstring, wird vom Thread gesetzt */
    list<COMRealChannel> _selected; /**< Liste mit ausgewählten Kanälen */

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
   Liefert die Liste der ausgewählten Kanäle
*/

inline const list<COMRealChannel> *CTLDialogChannels::channels() const
{
    return &_selected;
}

/*****************************************************************************/

#endif
