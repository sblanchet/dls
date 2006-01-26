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
#include <FL/Fl_Button.h>
#include <FL/Fl_Input.h>
#include <FL/Fl_Output.h>

#include "fl_grid.hpp"
#include "com_job_preset.hpp"
#include "com_real_channel.hpp"
#include "ctl_globals.hpp"

//---------------------------------------------------------------

class CTLDialogJob
{
public:
  CTLDialogJob(const string &);
  ~CTLDialogJob();

  void show(COMJobPreset *);
  bool updated() const;
  bool imported() const;

private:
  Fl_Double_Window *_wnd;
  Fl_Button *_button_close;
  Fl_Output *_output_desc;
  Fl_Output *_output_source;
  Fl_Output *_output_trigger;
  Fl_Button *_button_change;
  Fl_Grid *_grid_channels;
  Fl_Grid *_grid_messages;
  Fl_Button *_button_add;
  Fl_Button *_button_rem;
  Fl_Button *_button_edit;
  string _dls_dir;
  COMJobPreset *_job;
  vector<CTLMessage> _messages;
  bool _changed, _updated;

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
  void _remove_channel(int);

  void _grid_selection_changed();

  static void _static_timeout(void *);
  void _timeout();
};

//---------------------------------------------------------------

inline bool CTLDialogJob::updated() const
{
  return _updated;
}

//---------------------------------------------------------------

#endif
