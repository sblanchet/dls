/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef CTLDialogJobHpp
#define CTLDialogJobHpp

/*****************************************************************************/

#include <vector>
using namespace std;

#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Tile.h>
#include <FL/Fl_Button.h>
#include <FL/Fl_Input.h>
#include <FL/Fl_Output.h>

/*****************************************************************************/

#include "fl_grid.hpp"
#include "ctl_globals.hpp"
#include "ctl_job_preset.hpp"

/*****************************************************************************/

/**
   Dialog zum bearbeiten einer Auftragsvorgabe
*/

class CTLDialogJob
{
public:
  CTLDialogJob(const string &);
  ~CTLDialogJob();

  void show(CTLJobPreset *);
  bool updated() const;
  bool imported() const;

private:
  Fl_Double_Window *_wnd;       /**< Dialogfenster */
  Fl_Button *_button_close;     /**< "Schliessen" - Button */
  Fl_Output *_output_desc;      /**< Ausgabefeld f�r die Auftragsbeschreibung */
  Fl_Output *_output_source;    /**< Ausgabefeld f�r die Datenquelle */
  Fl_Output *_output_trigger;   /**< Ausgabefeld f�r den Namen des Trigger-Parameters */
  Fl_Button *_button_change;    /**< Button f�r den Dialog zur �nderung der Beschreibung, usw. */
  Fl_Grid *_grid_channels;      /**< Das Grid zur Darstellung der Kan�le */
  Fl_Button *_button_add;       /**< Button zum Hinzuf�gen von Kan�len */
  Fl_Button *_button_rem;       /**< Button zum entfernen von gew�hlten Kan�len */
  Fl_Button *_button_edit;      /**< Button zum Editieren von gew�hlten Kan�len */
  string _dls_dir;              /**< DLS-Datenverzeichnis */
  CTLJobPreset *_job;           /**< Zeiger auf den zu bearbeitenden Auftrag */
  bool _changed;                /**< Flag, das anzeigt, ob etwas am Auftrag ge�ndert wurde */
  bool _updated;                /**< Flag, das angibt, ob der Auftrag ge�ndert gespeichert wurde */

  static void _callback(Fl_Widget *, void *);
  void _grid_channels_callback();
  void _button_close_clicked();
  void _button_change_clicked();
  void _button_add_clicked();
  void _button_rem_clicked();
  void _button_edit_clicked();

  bool _load_channels();

  bool _save_job();
  void _edit_channels();
  void _insert_channels(const list<COMRealChannel> *);
  void _remove_channels(const list<unsigned int> *);

  void _grid_selection_changed();
};

/*****************************************************************************/

/**
   Gibt zur�ck, ob der Auftrag ge�ndert und gespeichert wurde

   \return true, wenn ge�ndert und gespeichert
*/

inline bool CTLDialogJob::updated() const
{
  return _updated;
}

/*****************************************************************************/

#endif
