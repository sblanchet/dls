//---------------------------------------------------------------
//
//  C T L _ D I A L O G _ J O B _ E D I T . H P P
//
//---------------------------------------------------------------

#ifndef CTLDialogJobEditHpp
#define CTLDialogJobEditHpp

//---------------------------------------------------------------

#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Return_Button.h>
#include <FL/Fl_Input.h>

//---------------------------------------------------------------

#include "ctl_job_preset.hpp"

//---------------------------------------------------------------

/**
   Dialog zum Editieren der Grunddaten eines Messauftrages
*/

class CTLDialogJobEdit
{
public:
  CTLDialogJobEdit(const string &);
  ~CTLDialogJobEdit();

  void show(CTLJobPreset *);
  bool updated();

private:
  Fl_Double_Window *_wnd;       /**< Dialogfenster */
  Fl_Return_Button *_button_ok; /**< Bestätigungs-Button */
  Fl_Button *_button_cancel;    /**< Button zum Abbrechen */
  Fl_Input *_input_desc;        /**< Eingabefeld für die Beschreibung */
  Fl_Input *_input_source;      /**< Eingabefeld für die Datenquelle */
  Fl_Input *_input_trigger;     /**< Eingabefeld für den Trigger-Parameter */
  Fl_Input *_input_quota_time;  /**< Eingabefeld für Zeit-Quota */
  Fl_Input *_input_quota_size;  /**< Eingabefeld für Daten-Quota */

  string _dls_dir;    /**< DLS-Datenverzeichnis */
  CTLJobPreset *_job; /**< Zeiger auf den Messauftrag */
  bool _updated;      /**< Wurde etwas geändert? */

  static void _callback(Fl_Widget *, void *);
  void _button_ok_clicked();
  void _button_cancel_clicked();
  bool _save_job();
  bool _create_job();
  bool _get_new_id(int *);
  void _convert_time_quota_to_str(long long, string *);
  void _convert_size_quota_to_str(long long, string *);
  bool _convert_str_to_time_quota(const string &, long long *);
  bool _convert_str_to_size_quota(const string &, long long *);
};

//---------------------------------------------------------------

/**
   Gibt zurück, ob sich etwas geändert hat

   \return true, wenn Daten geändert wurden
*/

inline bool CTLDialogJobEdit::updated()
{
  return _updated;
}

//---------------------------------------------------------------

#endif
