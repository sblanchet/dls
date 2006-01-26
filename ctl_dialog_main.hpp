//---------------------------------------------------------------
//
//  C T L _ D I A L O G _ M A I N . H P P
//
//---------------------------------------------------------------

#ifndef CTLDialogMainHpp
#define CTLDialogMainHpp

//---------------------------------------------------------------

#include <time.h> // Für time_t

#include <vector>
using namespace std;

#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Button.h>

//---------------------------------------------------------------

#include "com_time.hpp"
#include "com_job_preset.hpp"
#include "fl_grid.hpp"
#include "ctl_globals.hpp"

//---------------------------------------------------------------

class CTLDialogMain
{
public:
  CTLDialogMain(const string &);
  ~CTLDialogMain();

  void show();

private:
  Fl_Double_Window *_wnd;  
  Fl_Grid *_grid_jobs;
  Fl_Grid *_grid_messages;
  Fl_Button *_button_close;
  Fl_Button *_button_add;
  Fl_Button *_button_rem;
  Fl_Button *_button_state;

  string _dls_dir;
  vector<COMJobPreset> _jobs;
  vector<CTLMessage> _messages;

  void _edit_job(unsigned int);

  static void _callback(Fl_Widget *, void *);
  void _grid_jobs_callback();
  void _grid_messages_callback();
  void _button_close_clicked();
  void _button_state_clicked();
  void _button_add_clicked();
  void _button_rem_clicked();

  static void _static_timeout(void *);
  void _timeout();

  void _load_jobs();
  void _load_messages();
  void _load_watchdogs();
  void _update_button_state();
};

//---------------------------------------------------------------

#endif
