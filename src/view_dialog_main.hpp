/******************************************************************************
 *
 *  $Id$
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

#include "lib_dir.hpp"
using namespace LibDLS;

#include "fl_grid.hpp"
#include "com_job_preset.hpp"
#include "view_view_data.hpp"
#include "view_view_msg.hpp"

/*****************************************************************************/

/**
   Hauptdialog des DLS-Viewers
*/

class ViewDialogMain
{
public:
    ViewDialogMain(const string &);
    ~ViewDialogMain();

    void show();

private:
    string _dls_dir_path; /**< DLS-Datenverzeichnis */
    Fl_Double_Window *_wnd; /**< Dialogfenster */
    Fl_Tile *_tile_ver; /**< Vertikaler Trenner zwischen Anzeigen
                           und Kanalliste */
    Fl_Tile *_tile_hor; /**< Horizontaler Trenner zwischen Kan�len
                           und Messages */
    Fl_Choice *_choice_job; /**< Auswahlfeld zum W�hlen des Auftrags */
    Fl_Button *_button_full; /**< Button zum Anzeigen der gesamten
                                Zeitspanne */
    Fl_Button *_button_reload; /**< Button zum erneuten Laden der Daten */
    Fl_Button *_button_export; /**< Button zum Exportieren der Daten */
    Fl_Check_Button *_checkbutton_messages; /**< Checkbutton zum Anzeigen der
                                    Nachrichten. */
    Fl_Button *_button_close; /**< "Schliessen"-Button */
    Fl_Grid *_grid_channels; /**< Grid zum Anzeigen der Kanalliste*/

    ViewViewData *_view_data; /**< Anzeige f�r die Kanaldaten */
    ViewViewMsg *_view_msg; /**< Anzeige f�r die Messages */

    Directory _dls_dir; /**< LibDLS directory object */
    Job *_current_job; /**< current job */

    static void _callback(Fl_Widget *, void *);
    void _button_close_clicked();
    void _button_reload_clicked();
    void _button_full_clicked();
    void _button_export_clicked();
    void _checkbutton_messages_clicked();
    void _choice_job_changed();
    void _grid_channels_changed();

    static void _data_range_callback(COMTime, COMTime, void *);
};

/*****************************************************************************/

#endif
