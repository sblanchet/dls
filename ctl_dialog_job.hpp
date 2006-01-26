//---------------------------------------------------------------
//
//  C T L _ D I A L O G _ J O B . H P P
//
//---------------------------------------------------------------

#ifndef CTLDialogJobHpp
#define CTLDialogJobHpp

//---------------------------------------------------------------

#include <vector>
using namespace std;

#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Tile.h>
#include <FL/Fl_Button.h>
#include <FL/Fl_Input.h>
#include <FL/Fl_Output.h>

//---------------------------------------------------------------

#include "fl_grid.hpp"
#include "ctl_globals.hpp"
#include "ctl_job_preset.hpp"

//---------------------------------------------------------------

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
  Fl_Tile *_tile;               /**< Horizontale Aufteilung zw. Kanälen und Meldungen */
  Fl_Button *_button_close;     /**< "Schliessen" - Button */
  Fl_Output *_output_desc;      /**< Ausgabefeld für die Auftragsbeschreibung */
  Fl_Output *_output_source;    /**< Ausgabefeld für die Datenquelle */
  Fl_Output *_output_trigger;   /**< Ausgabefeld für den Namen des Trigger-Parameters */
  Fl_Button *_button_change;    /**< Button für den Dialog zur Änderung der Beschreibung, usw. */
  Fl_Grid *_grid_channels;      /**< Das Grid zur Darstellung der Kanäle */
  Fl_Grid *_grid_messages;      /**< Das Grid zur Darstellung des Meldungen */
  Fl_Button *_button_add;       /**< Button zum Hinzufügen von Kanälen */
  Fl_Button *_button_rem;       /**< Button zum entfernen von gewählten Kanälen */
  Fl_Button *_button_edit;      /**< Button zum Editieren von gewählten Kanälen */
  string _dls_dir;              /**< DLS-Datenverzeichnis */
  CTLJobPreset *_job;           /**< Zeiger auf den zu bearbeitenden Auftrag */
  vector<CTLMessage> _messages; /**< Vektor mit den geladenen Meldungen */
  bool _changed;                /**< Flag, das anzeigt, ob etwas am Auftrag geändert wurde */
  bool _updated;                /**< Flag, das angibt, ob der Auftrag geändert gespeichert wurde */

  static void _callback(Fl_Widget *, void *);
  void _grid_channels_callback();
  void _grid_messages_callback();
  void _button_close_clicked();
  void _button_change_clicked();
  void _button_add_clicked();
  void _button_rem_clicked();
  void _button_edit_clicked();

  bool _load_channels();
  bool _load_messages();

  bool _save_job();
  void _edit_channels();
  void _insert_channels(const list<COMRealChannel> *);
  void _remove_channels(const list<unsigned int> *);

  void _grid_selection_changed();

  static void _static_timeout(void *);
  void _timeout();
};

//---------------------------------------------------------------

/**
   Gibt zurück, ob der Auftrag geändert und gespeichert wurde

   \return true, wenn geändert und gespeichert
*/

inline bool CTLDialogJob::updated() const
{
  return _updated;
}

//---------------------------------------------------------------

#endif
