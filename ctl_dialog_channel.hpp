//---------------------------------------------------------------
//
//  C T L _ D I A L O G _ C H A N N E L . H P P
//
//---------------------------------------------------------------

#ifndef CTLDialogChannelHpp
#define CTLDialogChannelHpp

//---------------------------------------------------------------

#include <list>
using namespace std;

#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Return_Button.h>
#include <FL/Fl_Input.h>
#include <FL/Fl_Choice.h>

//---------------------------------------------------------------

#include "ctl_job_preset.hpp"

//---------------------------------------------------------------

/**
   Dialog zum �ndern der Eigenschaften von zu erfassenden Kan�len

   �ber diese Eingabemaske kann der Benutzer die Abtastrate,
   Meta-Untersetzung, Kompression usw. eines Kanals bestimmen.
*/

class CTLDialogChannel
{
public:
  CTLDialogChannel(const string &);
  ~CTLDialogChannel();

  void show(CTLJobPreset *, const list<const COMChannelPreset *> *);
  bool updated() const;

private:
  Fl_Double_Window *_wnd;       /**< Dialogfenster */
  Fl_Return_Button *_button_ok; /**< "OK"-Button */
  Fl_Button *_button_cancel;    /**< "Abbrechen"-Button */
  Fl_Input *_input_freq;        /**< Eingabefeld f�r die Abtastfrequenz */
  Fl_Input *_input_block;       /**< Eingabefeld f�r die Blockgr��e */
  Fl_Input *_input_mask;        /**< Eingabefeld f�r die Meta-Maske */
  Fl_Input *_input_red;         /**< Eingabefeld f�r die Meta-Untersetzung */
  Fl_Choice *_choice_format;    /**< Auswahlfeld f�r die Kompression */
  Fl_Choice *_choice_mdct;      /**< Auswahlfeld f�r die MDCT-Blockgr��e */
  Fl_Input *_input_accuracy;    /**< Eingabefeld f�r die MDCT-Genauigkeit */

  string _dls_dir;                                 /**< DLS-Datenverzeichnis */
  CTLJobPreset *_job;                              /**< Zeiger auf das Auftrags-Objekt */
  const list<const COMChannelPreset *> *_channels; /**< Liste der zu �ndernden Kan�le */
  bool _updated;                                   /**< Es wurden Kan�le ge�ndert */
  bool _choice_format_selected;                    /**< Es wurde ein Format ausgew�hlt */
  bool _choice_mdct_selected;                      /**< Es wurde eine MDCT-Blockgr��e gew�hlt */

  static void _callback(Fl_Widget *, void *);
  void _button_ok_clicked();
  void _button_cancel_clicked();
  void _choice_format_changed();
  void _choice_mdct_changed();

  void _load_channel();
  bool _save_channels();
};

//---------------------------------------------------------------

/**
   Gibt zur�ck, ob Kan�le ge�ndert wurden

   \return true, wenn Kan�le ge�ndert wurden
*/

inline bool CTLDialogChannel::updated() const
{
  return _updated;
}

//---------------------------------------------------------------

#endif
