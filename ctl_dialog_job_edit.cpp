//---------------------------------------------------------------
//
//  C T L _ D I A L O G _ J O B _ E D I T . C P P
//
//---------------------------------------------------------------

#include <iostream>
#include <fstream>
using namespace std;

#include <FL/Fl.h>

//---------------------------------------------------------------

#include "com_job_preset.hpp"
#include "ctl_dialog_job_edit.hpp"

//---------------------------------------------------------------

#define WIDTH 220
#define HEIGHT 200

//---------------------------------------------------------------

CTLDialogJobEdit::CTLDialogJobEdit(const string &dls_dir)
{
  int x = Fl::w() / 2 - WIDTH / 2;
  int y = Fl::h() / 2 - HEIGHT / 2;

  _dls_dir = dls_dir;
  _updated = false;

  _wnd = new Fl_Double_Window(x, y, WIDTH, HEIGHT, "Auftrag anlegen");
  _wnd->callback(_callback, this);
  _wnd->set_modal();

  _button_ok = new Fl_Return_Button(WIDTH - 90, HEIGHT - 35, 80, 25, "OK");
  _button_ok->callback(_callback, this);

  _button_cancel = new Fl_Button(WIDTH - 180, HEIGHT - 35, 80, 25, "Abbrechen");
  _button_cancel->callback(_callback, this);

  _input_desc = new Fl_Input(10, 30, 200, 25, "Beschreibung");
  _input_desc->align(FL_ALIGN_TOP_LEFT);
  _input_desc->take_focus();

  _input_source = new Fl_Input(10, 80, 200, 25, "Quelle");
  _input_source->align(FL_ALIGN_TOP_LEFT);

  _input_trigger = new Fl_Input(10, 130, 200, 25, "Trigger");
  _input_trigger->align(FL_ALIGN_TOP_LEFT);

  /** \todo Template-Jobs! */
}

//---------------------------------------------------------------

CTLDialogJobEdit::~CTLDialogJobEdit()
{
  delete _wnd;
}

//---------------------------------------------------------------

void CTLDialogJobEdit::show(COMJobPreset *job)
{
  _job = job;

  if (_job && !_load_job()) return;

  _input_source->readonly(_job != 0);

  _wnd->show();
      
  while (_wnd->shown()) Fl::wait();
}

//---------------------------------------------------------------

void CTLDialogJobEdit::_callback(Fl_Widget *sender, void *data)
{
  CTLDialogJobEdit *dialog = (CTLDialogJobEdit *) data;

  if (sender == dialog->_button_ok) dialog->_button_ok_clicked();
  if (sender == dialog->_button_cancel) dialog->_button_cancel_clicked();
  if (sender == dialog->_wnd) dialog->_button_cancel_clicked();
}

//---------------------------------------------------------------

void CTLDialogJobEdit::_button_ok_clicked()
{
  if (_job)
  {
    _save_job();
  }
  else
  {
    _create_job();
  }

  _wnd->hide();
}

//---------------------------------------------------------------

void CTLDialogJobEdit::_button_cancel_clicked()
{
  _wnd->hide();
}

//---------------------------------------------------------------

bool CTLDialogJobEdit::_load_job()
{
  _input_desc->value(_job->description().c_str());
  _input_source->value(_job->source().c_str());
  _input_trigger->value(_job->trigger().c_str());

  _updated = false;

  return true;
}

//---------------------------------------------------------------

bool CTLDialogJobEdit::_save_job()
{
  COMJobPreset job_copy;

  if (_job->description() != _input_desc->value()    ||
      _job->source()      != _input_source->value()  ||
      _job->trigger()     != _input_trigger->value())
  {
    // Alte Vorgaben sichern
    job_copy = *_job;

    // Neue Vorgaben setzen
    _job->description(_input_desc->value());
    _job->source(_input_source->value());
    _job->trigger(_input_trigger->value());

    try
    {
      _job->write(_dls_dir);
    }
    catch (ECOMJobPreset &e)
    {
      *_job = job_copy;
      cout << "ERROR: " << e.msg << endl;
      return false;
    }
    
    try
    {
      _job->notify_changed(_dls_dir);
    }
    catch (ECOMJobPreset &e)
    {
      cout << "WARNING: dlsd could not be notified: " << e.msg << endl;
    }

    _updated = true;
  }
  
  return true;
}

//---------------------------------------------------------------

bool CTLDialogJobEdit::_create_job()
{
  int new_id;
  COMJobPreset job;

  if (!_get_new_id(&new_id))
  {
    cout << "ERROR: could not fetch new id!" << endl;
    return false;
  }

  job.id(new_id);
  job.description(_input_desc->value());
  job.source(_input_source->value());
  job.trigger(_input_trigger->value());
  job.running(false);

  try
  {
    job.write(_dls_dir);
  }
  catch (ECOMJobPreset &e)
  {
    cout << "ERROR: " << e.msg << endl;
    return false;
  }
  catch (...)
  {
    cout << "ERROR: unknown!" << endl;
    return false;
  } 
    
  try
  {
    job.notify_new(_dls_dir);
  }
  catch (ECOMJobPreset &e)
  {
    cout << "WARNING: dlsd could not be notified: " << e.msg << endl;
  }
  catch (...)
  {
    cout << "ERROR: unknown!" << endl;
    return false;
  } 
  
  _updated = true;
  return true;
}

//---------------------------------------------------------------

bool CTLDialogJobEdit::_get_new_id(int *id)
{
  fstream id_in_file, id_out_file;
  string id_file_name = _dls_dir + "/id_sequence";

  id_in_file.open(id_file_name.c_str(), ios::in);

  if (!id_in_file.is_open())
  {
    return false;
  }

  id_in_file.exceptions(ios::badbit | ios::failbit);

  try
  {
    id_in_file >> *id;
  }
  catch (...)
  {
    id_in_file.close();
    return false;
  }

  id_in_file.close();

  id_out_file.open(id_file_name.c_str(), ios::out | ios::trunc);

  if (!id_out_file.is_open())
  {
    return false;
  }

  try
  {
    id_out_file << (*id + 1);
  }
  catch (...)
  {
    id_out_file.close();
    return false;
  }
  

  id_out_file.close();

  return true;
}

//---------------------------------------------------------------

bool CTLDialogJobEdit::updated()
{
  return _updated;
}  

//---------------------------------------------------------------
