//---------------------------------------------------------------
//
//  V I E W _ D I A L O G _ M A I N . C P P
//
//---------------------------------------------------------------

#include <dirent.h>

#include <iostream>
#include <sstream>
using namespace std;

#include <FL/Fl.h>

//---------------------------------------------------------------

#include "view_channel.hpp"
#include "view_dialog_main.hpp"

//---------------------------------------------------------------

#define WIDTH 780
#define HEIGHT 550

//---------------------------------------------------------------

ViewDialogMain::ViewDialogMain(const string &dls_dir)
{
  int x = Fl::w() / 2 - WIDTH / 2;
  int y = Fl::h() / 2 - HEIGHT / 2;

  _dls_dir = dls_dir;

  _wnd = new Fl_Double_Window(x, y, WIDTH, HEIGHT, "Datenansicht");
  _wnd->callback(_callback, this);
  _wnd->set_modal();

  _choice_job = new Fl_Choice(10, 25, 240, 25, "Auftrag");
  _choice_job->align(FL_ALIGN_TOP_LEFT);
  _choice_job->callback(_callback, this);

  _button_full = new Fl_Button(260, 25, 100, 25, "Gesamt");
  _button_full->callback(_callback, this);

  _button_reload = new Fl_Button(370, 25, 100, 25, "Aktualisieren");
  _button_reload->callback(_callback, this);

  _button_close = new Fl_Button(480, 25, 80, 25, "Schliessen");
  _button_close->callback(_callback, this);

  _view_data = new ViewViewData(10, 60, WIDTH - 230, HEIGHT - 70);
  _view_data->take_focus();

  _grid_channels = new Fl_Grid(WIDTH - 210, 10, 200, HEIGHT - 20);
  _grid_channels->add_column("channel", "Kanal");
  _grid_channels->select_mode(flgNoSelect);
  _grid_channels->check_boxes(true);
  _grid_channels->callback(_callback, this);

  _wnd->resizable(_view_data);
}

//---------------------------------------------------------------

ViewDialogMain::~ViewDialogMain()
{
  delete _wnd;
}

//---------------------------------------------------------------

void ViewDialogMain::show()
{
  if (_load_jobs())
  {
    _wnd->show();

    while (_wnd->shown()) Fl::wait();
  }
}

//---------------------------------------------------------------

void ViewDialogMain::_callback(Fl_Widget *sender, void *data)
{
  ViewDialogMain *dialog = (ViewDialogMain *) data;

  if (sender == dialog->_button_close) dialog->_button_close_clicked();
  if (sender == dialog->_wnd) dialog->_button_close_clicked();
  if (sender == dialog->_choice_job) dialog->_choice_job_changed();
  if (sender == dialog->_grid_channels) dialog->_grid_channels_changed();
  if (sender == dialog->_button_reload) dialog->_button_reload_clicked();
  if (sender == dialog->_button_full) dialog->_button_full_clicked();
}

//---------------------------------------------------------------

void ViewDialogMain::_button_close_clicked()
{
  _wnd->hide();
}

//---------------------------------------------------------------

void ViewDialogMain::_button_reload_clicked()
{
  _view_data->update();
}

//---------------------------------------------------------------

void ViewDialogMain::_button_full_clicked()
{
  _view_data->full_range();
}

//---------------------------------------------------------------

void ViewDialogMain::_choice_job_changed()
{
  int index = _choice_job->value();

  _job_id = _jobs[index].id();

  _view_data->set_job(_dls_dir, _job_id);

  _load_channels();
}

//---------------------------------------------------------------

void ViewDialogMain::_grid_channels_changed()
{
  unsigned int i = _grid_channels->current_record();

  switch (_grid_channels->current_event())
  {
    case flgContent:
      if (_grid_channels->current_col() == "channel")
      {
        _grid_channels->current_content(_channels[i].name());
      }
      break;

    case flgChecked:
      _grid_channels->current_checked(_view_data->has_channel(&_channels[i]));
      break;

    case flgCheck:
      if (_view_data->has_channel(&_channels[i]))
      {
        _view_data->rem_channel(&_channels[i]);
      }
      else
      {
        _view_data->add_channel(&_channels[i]);
      }
      break;

    default:
      break;
  }
}

//---------------------------------------------------------------

bool ViewDialogMain::_load_jobs()
{
  stringstream str;
  DIR *dir;
  struct dirent *dir_ent;
  COMJobPreset job;
  string dir_name;
  int index;

  str.exceptions(ios::failbit | ios::badbit);

  _choice_job->clear();
  _jobs.clear();

  if ((dir = opendir(_dls_dir.c_str())) == NULL)
  {
    cout << "ERROR: could not open dls directory \"" << _dls_dir << "\"" << endl;
    return false;
  }

  while ((dir_ent = readdir(dir)) != NULL)
  {
    dir_name = dir_ent->d_name;

    if (dir_name.find("job") != 0) continue;

    str.str("");
    str.clear();
    str << dir_name.substr(3);

    try
    {
      str >> index;
    }
    catch (...)
    {
      continue;
    }

    try
    {
      job.import(_dls_dir, index);
    }
    catch (ECOMJobPreset &e)
    {
      cout << "WARNING: " << e.msg << endl;
      continue;
    }

    _jobs.push_back(job);
    _choice_job->add(job.id_desc().c_str());
  }

  closedir(dir);

  return true;  
}

//---------------------------------------------------------------

bool ViewDialogMain::_load_channels()
{
  stringstream str, job_dir;
  DIR *dir;
  struct dirent *dir_ent;
  string dir_name;
  ViewChannel channel;
  int index;

  job_dir << _dls_dir << "/job" << _job_id;

  str.exceptions(ios::failbit | ios::badbit);

  _grid_channels->clear();
  _channels.clear();

  if ((dir = opendir(job_dir.str().c_str())) == NULL)
  {
    cout << "ERROR: could not open job directory \"" << job_dir.str() << "\"" << endl;
    return false;
  }

  while ((dir_ent = readdir(dir)) != NULL)
  {
    dir_name = dir_ent->d_name;

    if (dir_name.find("channel") != 0) continue;

    str.str("");
    str.clear();
    str << dir_name.substr(7);

    try
    {
      str >> index;
    }
    catch (...)
    {
      continue;
    }

    try
    {
      channel.import(_dls_dir, _job_id, index);
    }
    catch (EViewChannel &e)
    {
      cout << "WARNING: " << e.msg << endl;
      continue;
    }

    _channels.push_back(channel);
  }

  closedir(dir);

  _grid_channels->record_count(_channels.size());

  return true;
}

//---------------------------------------------------------------
