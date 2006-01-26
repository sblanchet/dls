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

#include "com_job_preset.hpp"

//---------------------------------------------------------------

class CTLDialogJobEdit
{
public:
  CTLDialogJobEdit(const string &);
  ~CTLDialogJobEdit();

  void show(COMJobPreset *);
  bool updated();

private:
  Fl_Double_Window *_wnd;
  Fl_Return_Button *_button_ok;
  Fl_Button *_button_cancel;
  Fl_Input *_input_desc;
  Fl_Input *_input_source;
  Fl_Input *_input_trigger;
  string _dls_dir;
  COMJobPreset *_job;
  bool _updated;

  static void _callback(Fl_Widget *, void *);
  void _button_ok_clicked();
  void _button_cancel_clicked();
  bool _save_job();
  bool _create_job();
  bool _get_new_id(int *);
  bool _load_job();
};

//---------------------------------------------------------------

#endif
