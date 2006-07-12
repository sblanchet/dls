/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef CTLDialogMainHpp
#define CTLDialogMainHpp

/*****************************************************************************/

#include <vector>
using namespace std;

#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Tile.h>
#include <FL/Fl_Button.h>

/*****************************************************************************/

#include "fl_grid.hpp"
#include "com_time.hpp"
#include "ctl_job_preset.hpp"
#include "ctl_globals.hpp"

/*****************************************************************************/

/**
   Hauptdialog des DLS-Managers
*/

class CTLDialogMain
{
public:
    CTLDialogMain(const string &);
    ~CTLDialogMain();

    void show();

private:
    Fl_Double_Window *_wnd; /**< Dialog-Fenster */
    Fl_Grid *_grid_jobs; /**< Grid für alle Erfassungsaufträge */
    Fl_Button *_button_close; /**< Button zum Schliessen des Dialoges */
    Fl_Button *_button_add; /**< Button für das Hinzufügen eines Auftrages */
    Fl_Button *_button_state; /**< Button zum Starten oder Anhalten
                                 der Erfassung */

    string _dls_dir;            /**< DLS-Datenverzeichnis */
    vector<CTLJobPreset> _jobs; /**< Vektor mit allen Erfassungsaufträgen */

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

    CTLDialogMain(); // Soll nicht aufgerufen werden
};

/*****************************************************************************/

#endif
