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

#include "com_job_preset.hpp"

//---------------------------------------------------------------

class CTLDialogChannel
{
public:
  CTLDialogChannel(const string &);
  ~CTLDialogChannel();

  void show(COMJobPreset *, const list<const COMChannelPreset *> *);
  bool updated() const;

private:
  Fl_Double_Window *_wnd;
  Fl_Return_Button *_button_ok;
  Fl_Button *_button_cancel;
  Fl_Input *_input_freq;
  Fl_Input *_input_block;
  Fl_Input *_input_mask;
  Fl_Input *_input_red;
  Fl_Choice *_choice_format;
  Fl_Choice *_choice_mdct;
  Fl_Input *_input_accuracy;

  string _dls_dir;
  COMJobPreset *_job;
  const list<const COMChannelPreset *> *_channels;
  bool _updated;
  bool _format_selected;
  bool _mdct_selected;

  static void _callback(Fl_Widget *, void *);
  void _button_ok_clicked();
  void _button_cancel_clicked();
  void _choice_format_changed();
  void _choice_mdct_changed();

  void _load_channel();
  bool _save_channels();
};

//---------------------------------------------------------------

inline bool CTLDialogChannel::updated() const
{
  return _updated;
}

//---------------------------------------------------------------

#endif
