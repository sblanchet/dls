//---------------------------------------------------------------
//
//  V I E W _ D I A L O G _ M A I N . H P P
//
//---------------------------------------------------------------

#ifndef ViewDialogMainHpp
#define ViewDialogMainHpp

//---------------------------------------------------------------

#include <vector>
using namespace std;

#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Button.h>
#include <FL/Fl_Choice.h>

#include "fl_grid.hpp"
#include "com_job_preset.hpp"
#include "view_view_data.hpp"

//---------------------------------------------------------------

class ViewDialogMain
{
public:
  ViewDialogMain(const string &);
  ~ViewDialogMain();

  void show();

private:
  Fl_Double_Window *_wnd;
  string _dls_dir;
  Fl_Choice *_choice_job;
  Fl_Grid *_grid_channels;
  Fl_Button *_button_close;
  Fl_Button *_button_reload;
  Fl_Button *_button_full;
  ViewViewData *_view_data;
  
  vector<COMJobPreset> _jobs;
  vector<ViewChannel> _channels;
  unsigned int _job_id;

  static void _callback(Fl_Widget *, void *);
  void _button_close_clicked();
  void _button_reload_clicked();
  void _button_full_clicked();
  void _choice_job_changed();
  void _grid_channels_changed();

  bool _load_jobs();
  bool _load_channels();
  void _load_data();
};

//---------------------------------------------------------------

#endif
