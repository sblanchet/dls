//---------------------------------------------------------------
//
//  C T L _ D I A L O G _ C H A N N E L S . H P P
//
//---------------------------------------------------------------

#ifndef CTLDialogChannelsHpp
#define CTLDialogChannelsHpp

//---------------------------------------------------------------

#include <pthread.h>

#include <vector>
#include <list>
using namespace std;

#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Button.h>
#include <FL/Fl_Box.h>

//---------------------------------------------------------------

#include "fl_grid.hpp"
#include "com_real_channel.hpp"

//---------------------------------------------------------------

class CTLDialogChannels
{
public:
  CTLDialogChannels(const string &);
  ~CTLDialogChannels();

  void show();

  const list<COMRealChannel> *channels() const;

private:
  Fl_Double_Window *_wnd;
  Fl_Button *_button_ok;
  Fl_Button *_button_cancel;
  Fl_Grid *_grid_channels;
  Fl_Box *_box_message;
  string _source;
  int _socket;
  pthread_t _thread;
  bool _imported;
  vector<COMRealChannel> _channels;
  COMRealChannel _channel;
  bool _thread_running;
  string _error;
  list<COMRealChannel> _selected;

  static void _callback(Fl_Widget *, void *);
  void _grid_channels_callback();
  void _button_ok_clicked();
  void _button_cancel_clicked();

  static void *_static_thread_function(void *);
  void _thread_function();

  void _thread_finished();
};

//---------------------------------------------------------------

#endif
