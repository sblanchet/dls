//---------------------------------------------------------------
//
//  C T L _ D I A L O G _ J O B . C P P
//
//---------------------------------------------------------------

#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

#include <FL/Fl.h>

#include "ctl_dialog_channel.hpp"
#include "ctl_dialog_channels.hpp"
#include "ctl_dialog_job_edit.hpp"
#include "ctl_dialog_job.hpp"

#define WIDTH 600
#define HEIGHT 470

//---------------------------------------------------------------

CTLDialogJob::CTLDialogJob(const string &dls_dir)
{
  int x = Fl::w() / 2 - WIDTH / 2;
  int y = Fl::h() / 2 - HEIGHT / 2;

  _dls_dir = dls_dir;

  _wnd = new Fl_Double_Window(x, y, WIDTH, HEIGHT, "Auftrag bearbeiten");
  _wnd->callback(_callback, this);
  _wnd->set_modal();

  _button_close = new Fl_Button(WIDTH - 90, HEIGHT - 35, 80, 25, "Schliessen");
  _button_close->callback(_callback, this);

  _output_desc = new Fl_Output(10, 25, 200, 25, "Beschreibung");
  _output_desc->align(FL_ALIGN_TOP_LEFT);
  _output_desc->callback(_callback, this);

  _output_source = new Fl_Output(220, 25, 110, 25, "Quelle");
  _output_source->align(FL_ALIGN_TOP_LEFT);
  _output_source->callback(_callback, this);

  _output_trigger = new Fl_Output(340, 25, 160, 25, "Trigger");
  _output_trigger->align(FL_ALIGN_TOP_LEFT);
  _output_trigger->callback(_callback, this);

  _button_change = new Fl_Button(WIDTH - 90, 25, 80, 25, "Ändern...");
  _button_change->callback(_callback, this);

  _grid_channels = new Fl_Grid(10, 60, WIDTH - 20, 200);
  _grid_channels->add_column("name", "Kanal", 300);
  _grid_channels->add_column("freq", "Abtastrate");
  _grid_channels->add_column("block", "Blockgröße");
  _grid_channels->add_column("mask", "Meta-Maske");
  _grid_channels->add_column("red", "Untersetzung");
  _grid_channels->add_column("format", "Format", 200);
  _grid_channels->callback(_callback, this);
  _grid_channels->take_focus();
  _grid_channels->select_mode(flgMultiSelect);
  
  _grid_messages = new Fl_Grid(10, 270, WIDTH - 20, 150);
  _grid_messages->add_column("time", "Zeit");
  _grid_messages->add_column("text", "Nachricht", 230);
  _grid_messages->select_mode(flgNoSelect);
  _grid_messages->callback(_callback, this);

  _button_add = new Fl_Button(10, HEIGHT - 35, 150, 25, "Kanäle hinzufügen...");
  _button_add->callback(_callback, this);

  _button_edit = new Fl_Button(170, HEIGHT - 35, 150, 25, "Kanäle editieren...");
  _button_edit->callback(_callback, this);
  _button_edit->deactivate();

  _button_rem = new Fl_Button(330, HEIGHT - 35, 150, 25, "Kanäle entfernen");
  _button_rem->callback(_callback, this);
  _button_rem->deactivate();

  _wnd->resizable(_grid_channels);
}

//---------------------------------------------------------------

CTLDialogJob::~CTLDialogJob()
{
  Fl::remove_timeout(_static_timeout, this);

  delete _wnd;
}

//---------------------------------------------------------------

void CTLDialogJob::show(COMJobPreset *job)
{
  _job = job;

  _changed = false;
  _updated = false;

  _output_desc->value(_job->description().c_str());
  _output_source->value(_job->source().c_str());
  _output_trigger->value(_job->trigger().c_str());

  _grid_channels->record_count(_job->channels()->size());

  if (_load_messages())
  {
    _wnd->show();

    //Fl::add_timeout(1.0, _static_timeout, this);

    while (_wnd->shown()) Fl::wait();
  }
}

//---------------------------------------------------------------

void CTLDialogJob::_callback(Fl_Widget *sender, void *data)
{
  CTLDialogJob *dialog = (CTLDialogJob *) data;

  if (sender == dialog->_grid_channels) dialog->_grid_channels_callback();
  if (sender == dialog->_grid_messages) dialog->_grid_messages_callback();
  if (sender == dialog->_button_close) dialog->_button_close_clicked();
  if (sender == dialog->_wnd) dialog->_button_close_clicked();
  if (sender == dialog->_button_add) dialog->_button_add_clicked();
  if (sender == dialog->_button_rem) dialog->_button_rem_clicked();
  if (sender == dialog->_button_edit) dialog->_button_edit_clicked();
  if (sender == dialog->_button_change) dialog->_button_change_clicked();
}

//---------------------------------------------------------------

void CTLDialogJob::_grid_channels_callback()
{
  unsigned int i;
  stringstream str;
  const COMChannelPreset *channel;

  switch (_grid_channels->current_event())
  {
    case flgContent:
      i = _grid_channels->current_record();
      channel = &(*_job->channels())[i];

      if (_grid_channels->current_col() == "name")
      {
        _grid_channels->current_content(channel->name);
      }
      else if (_grid_channels->current_col() == "freq")
      {
        str << channel->sample_frequency << " Hz";
        _grid_channels->current_content(str.str());
      }
      else if (_grid_channels->current_col() == "block")
      {
        str << channel->block_size;
        _grid_channels->current_content(str.str());
      }
      else if (_grid_channels->current_col() == "mask")
      {
        str << channel->meta_mask;
        _grid_channels->current_content(str.str());
      }
      else if (_grid_channels->current_col() == "red")
      {
        str << channel->meta_reduction;
        _grid_channels->current_content(str.str());
      }
      else if (_grid_channels->current_col() == "format")
      {
        if (channel->format_index >= 0 && channel->format_index < DLS_FORMAT_COUNT)
        {
          _grid_channels->current_content(dls_format_strings[channel->format_index]);
        }
        else
        {
          _grid_channels->current_content("UNGÜLTIG!");
        }
      }
      break;

    case flgSelect:
    case flgDeSelect:
      _grid_selection_changed();
      break;

    case flgDoubleClick:
      _edit_channels();
      break;

    default:
      break;
  }
}

//---------------------------------------------------------------

void CTLDialogJob::_grid_messages_callback()
{
  unsigned int i;

  switch (_grid_messages->current_event())
  {
    case flgContent:
      i = _grid_messages->current_record();

      if (_grid_messages->current_col() == "time")
      {
        _grid_messages->current_content(_messages[i].time);
      }
      else if (_grid_messages->current_col() == "text")
      {
        if (_messages[i].type == 1 && !_grid_messages->current_selected())
        {
          _grid_messages->current_content_color(FL_RED);
        }

        _grid_messages->current_content(_messages[i].text);
      }
      break;

    default:
      break;
  }
}

//---------------------------------------------------------------

void CTLDialogJob::_button_close_clicked()
{
  _wnd->hide();
}

//---------------------------------------------------------------

void CTLDialogJob::_button_add_clicked()
{
  list<COMRealChannel *>::const_iterator ch_i;
  CTLDialogChannels *dialog;

  dialog = new CTLDialogChannels(_job->source());
  dialog->show();

  _insert_channels(dialog->channels());

  delete dialog;
}

//---------------------------------------------------------------

void CTLDialogJob::_button_rem_clicked()
{
  _remove_channel(_grid_channels->selected_index());
}

//---------------------------------------------------------------

void CTLDialogJob::_button_change_clicked()
{
  CTLDialogJobEdit *dialog = new CTLDialogJobEdit(_dls_dir);

  dialog->show(_job);

  if (dialog->updated())
  {
    _output_desc->value(_job->description().c_str());
    _output_source->value(_job->source().c_str());
    _output_trigger->value(_job->trigger().c_str());
  }

  delete dialog;
}

//---------------------------------------------------------------

void CTLDialogJob::_button_edit_clicked()
{
  _edit_channels();
}

//---------------------------------------------------------------

bool CTLDialogJob::_load_messages()
{
  /** \todo Messages für Job */

  /*

  for...
  {

    struct DLSMessage msg;

    msg.time = 
    msg.type = 
    msg.text = 

    _messages.push_back(msg);
  }

  _grid_messages->record_count(count);

  */

  return true;
}

//---------------------------------------------------------------

void CTLDialogJob::_edit_channels()
{
  CTLDialogChannel *dialog;
  list<const COMChannelPreset *> channels_to_edit;
  list<unsigned int>::const_iterator sel_i;

  sel_i = _grid_channels->selected_list()->begin();
  while (sel_i != _grid_channels->selected_list()->end())
  {
    // Adressen aller zu Ändernden Kanäle in eine Liste einfügen
    channels_to_edit.push_back(&(*_job->channels())[*sel_i]);
    sel_i++;
  }

  if (channels_to_edit.size() == 0) return;

  dialog = new CTLDialogChannel(_dls_dir);
  dialog->show(_job, &channels_to_edit);
  
  if (dialog->updated())
  {
    _changed = true;
    _grid_channels->redraw();
  }

  delete dialog;
}  

//---------------------------------------------------------------

void CTLDialogJob::_insert_channels(const list<COMRealChannel> *channels)
{
  list<COMRealChannel>::const_iterator ch_i;
  COMChannelPreset new_channel;
  COMJobPreset job_copy;

  if (channels->size() == 0) return;

  job_copy = *_job;

  ch_i = channels->begin();
  while (ch_i != channels->end())
  {
    if (job_copy.channel_exists(ch_i->name()))
    {
      cout << "channel \"" << ch_i->name() << "\" already exists!" << endl;
    }
    else
    { 
      new_channel.name = ch_i->name();
      new_channel.sample_frequency = ch_i->frequency();
      new_channel.block_size = (ch_i->frequency() >= 10 ? ch_i->frequency() : 10);
      new_channel.meta_mask = 7;
      new_channel.meta_reduction = 30;
      new_channel.format_index = DLS_FORMAT_ZLIB;

      job_copy.add_channel(&new_channel);
    }

    ch_i++;
  }

  try
  {
    job_copy.write(_dls_dir);
  }
  catch (ECOMJobPreset &e)
  {
    cout << "error writing preset file: " << e.msg << endl;
    return;
  }

  try
  {
    job_copy.notify_changed(_dls_dir);
  }
  catch (ECOMJobPreset &e)
  {
    cout << "WARNING: could not notify dlsd." << endl;
  }

  *_job = job_copy;
  _changed = true;
  _grid_channels->record_count(_job->channels()->size());
}

//---------------------------------------------------------------

void CTLDialogJob::_remove_channel(int index)
{
  COMJobPreset job_copy;

  if (index < 0 || index >= (int) _job->channels()->size())
  {
    cout << "channel index out of range!" << endl;
    return;
  }

  job_copy = *_job;

  job_copy.remove_channel((*_job->channels())[index].name);

  try
  {
    job_copy.write(_dls_dir);
  }
  catch (ECOMJobPreset &e)
  {
    cout << "error writing preset file: " << e.msg << endl;
    return;
  }

  try
  {
    job_copy.notify_changed(_dls_dir);
  }
  catch (ECOMJobPreset &e)
  {
    cout << "WARNING: could not notify dlsd." << endl;
  }

  *_job = job_copy;
  _changed = true;
  _grid_channels->record_count(_job->channels()->size());
}

//---------------------------------------------------------------

void CTLDialogJob::_grid_selection_changed()
{
  bool selected = _grid_channels->select_count() > 0;

  if (selected)
  {
    _button_rem->activate();
    _button_edit->activate();
  }
  else
  {
    _button_rem->deactivate();
    _button_edit->deactivate();
  }
}

//---------------------------------------------------------------

void CTLDialogJob::_static_timeout(void *data)
{
  CTLDialogJob *dialog = (CTLDialogJob *) data;

  dialog->_timeout();
}

//---------------------------------------------------------------

void CTLDialogJob::_timeout()
{
  unsigned int i, top = 0;
  string top_time;
  bool found;

  if (_messages.size() > 0)
  {
    top = _grid_messages->top_index();
    top_time = _messages[top].time;
  }

  _load_messages();

  if (top > 0)
  {
    i = 0;
    found = false;
    while (i < _messages.size() && !found)
    {
      if (_messages[i].time == top_time) found = true;
      else i++;
    }

    if (found)
    {
      _grid_messages->scroll(i);
    }
  }

  Fl::add_timeout(1.0, _static_timeout, this);
}

//---------------------------------------------------------------
