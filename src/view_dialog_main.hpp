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

#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Tile.h>
#include <FL/Fl_Button.h>
#include <FL/Fl_Choice.h>

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
  string _dls_dir;           /**< DLS-Datenverzeichnis */
  Fl_Double_Window *_wnd;    /**< Dialogfenster */
  Fl_Tile *_tile_ver;        /**< Vertikaler Trenner zw. Anzeigen und Kanalliste */
  Fl_Tile *_tile_hor;        /**< Horizontaler Trenner zwischen Kanälen und Messages */
  Fl_Choice *_choice_job;    /**< Auswahlfeld zum Wählen des Auftrags */
  Fl_Button *_button_full;   /**< Button zum Anzeigen der gesamten Zeitspanne */
  Fl_Button *_button_reload; /**< Button zum erneuten laden der Daten */
  
  Fl_Button *_button_close;  /**< "Schliessen"-Button */
  Fl_Grid *_grid_channels;   /**< Grid zum Anzeigen der Kanalliste*/
  ViewViewData *_view_data;  /**< Anzeige für die Kanaldaten */
  ViewViewMsg *_view_msg;    /**< Anzeige für die Messages */
  
  vector<COMJobPreset> _jobs;    /**< Vektor mit allen Aufträgen */
  vector<ViewChannel> _channels; /**< Vektor mit den Kanälen des aktuellen Auftrages */
  unsigned int _job_id;          /**< ID des aktuellen Auftrages */

  static void _callback(Fl_Widget *, void *);
  void _button_close_clicked();
  void _button_reload_clicked();
  void _button_full_clicked();
  void _choice_job_changed();
  void _grid_channels_changed();

  static void _data_range_callback(COMTime, COMTime, void *);

  bool _load_jobs();
  bool _load_channels();
  void _load_data();
};

/*****************************************************************************/

#endif
