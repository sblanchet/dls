//---------------------------------------------------------------
//
//  C T L _ D I A L O G _ C H A N N E L . C P P
//
//---------------------------------------------------------------

#include <math.h>

#include <iostream>
#include <sstream>
using namespace std;

#include <FL/Fl.h>

#include "com_job_preset.hpp"
#include "com_channel_preset.hpp"
#include "ctl_globals.hpp"
#include "ctl_dialog_channel.hpp"

#define WIDTH 270
#define HEIGHT 240

//---------------------------------------------------------------

CTLDialogChannel::CTLDialogChannel(const string &dls_dir)
{
  stringstream str;
  string format;
  int i, x, y;
  unsigned int j;

  x = Fl::w() / 2 - WIDTH / 2;
  y = Fl::h() / 2 - HEIGHT / 2;

  _dls_dir = dls_dir;

  _format_selected = false;
  _mdct_selected = false;

  _wnd = new Fl_Double_Window(x, y, WIDTH, HEIGHT, "Kanäle bearbeiten");
  _wnd->callback(_callback, this);
  _wnd->set_modal();

  _input_freq = new Fl_Input(10, 25, 120, 25, "Abtastrate (Hz)");
  _input_freq->align(FL_ALIGN_TOP_LEFT);
  _input_freq->callback(_callback, this);

  _input_block = new Fl_Input(140, 25, 120, 25, "Blockgröße");
  _input_block->align(FL_ALIGN_TOP_LEFT);
  _input_block->callback(_callback, this);

  _input_mask = new Fl_Input(10, 70, 120, 25, "Meta-Maske");
  _input_mask->align(FL_ALIGN_TOP_LEFT);
  _input_mask->callback(_callback, this);
  _input_mask->readonly(META_MASK_FIXED);

  _input_red = new Fl_Input(140, 70, 120, 25, "Untersetzung");
  _input_red->align(FL_ALIGN_TOP_LEFT);
  _input_red->callback(_callback, this);
  _input_red->readonly(META_REDUCTION_FIXED);

  _choice_format = new Fl_Choice(10, 115, 250, 25, "Format");
  _choice_format->align(FL_ALIGN_TOP_LEFT);
  _choice_format->callback(_callback, this);

  // Alle Kompressionsformate einfügen
  for (i = 0; i < DLS_FORMAT_COUNT; i++)
  {
    format = dls_format_strings[i];

    for (j = 1; j <= format.length(); j++) // Slashes escapen
    {
      if (format[j] == '/')
      {
        format.insert(j, "\\");
        j++;
      }
    }

    _choice_format->add(format.c_str());
  }

  _choice_mdct = new Fl_Choice(10, 160, 120, 25, "MDCT-Blockgröße");
  _choice_mdct->align(FL_ALIGN_TOP_LEFT);
  _choice_mdct->deactivate();
  _choice_mdct->callback(_callback, this);

  // Alle gültigen MDCT-Blockgrößen einfügen
  for (i = MDCT_MIN_EXP; i < MDCT_MAX_EXP_PLUS_ONE; i++)
  {
    str.str("");
    str.clear();
    str << (int) pow(2.0, i);
    _choice_mdct->add(str.str().c_str());
  }

  _button_ok = new Fl_Return_Button(WIDTH - 90, HEIGHT - 35, 80, 25, "OK");
  _button_ok->callback(_callback, this);

  _button_cancel = new Fl_Button(WIDTH - 180, HEIGHT - 35, 80, 25, "Abbrechen");
  _button_cancel->callback(_callback, this);
}

//---------------------------------------------------------------

CTLDialogChannel::~CTLDialogChannel()
{
  delete _wnd;
}

//---------------------------------------------------------------

void CTLDialogChannel::show(COMJobPreset *job,
                             const list<const COMChannelPreset *> *channels)
{
  stringstream str;
  list<const COMChannelPreset *>::const_iterator channel_i;
  unsigned int freq, block, mask, red, mdct_block_size;
  int format_index;
  bool freq_equal = true, block_equal = true, mask_equal = true, red_equal = true;
  bool format_equal = true, mdct_block_size_equal = true;
  double exp2;

  _job = job;
  _channels = channels;

  if (_channels->size() == 0) return;

  channel_i = _channels->begin();

  freq = (*channel_i)->sample_frequency;
  block = (*channel_i)->block_size;
  mask = (*channel_i)->meta_mask;
  red = (*channel_i)->meta_reduction;
  format_index = (*channel_i)->format_index;
  mdct_block_size = (*channel_i)->mdct_block_size;

  channel_i++;

  while (channel_i != _channels->end())
  {
    if ((*channel_i)->sample_frequency != freq) freq_equal = false;
    if ((*channel_i)->block_size != block) block_equal = false;
    if ((*channel_i)->meta_mask != mask) mask_equal = false;
    if ((*channel_i)->meta_reduction != red) red_equal = false;
    if ((*channel_i)->format_index != format_index) format_equal = false;
    if ((*channel_i)->mdct_block_size != mdct_block_size) mdct_block_size_equal = false;
    channel_i++;
  }
  
  if (freq_equal)
  {
    str.str("");
    str.clear();
    str << freq;
    _input_freq->value(str.str().c_str());
  }
  else
  {
    _input_freq->value("");
  }

  if (block_equal)
  {
    str.str("");
    str.clear();
    str << block;
    _input_block->value(str.str().c_str());
  }
  else
  {
    _input_block->value("");
  }

  if (mask_equal)
  {
    str.str("");
    str.clear();
    str << mask;
    _input_mask->value(str.str().c_str());
  }
  else
  {
    _input_mask->value("");
  }

  if (red_equal)
  {
    str.str("");
    str.clear();
    str << red;
    _input_red->value(str.str().c_str());
  }
  else
  {
    _input_red->value("");
  }

  if (format_equal && format_index >= 0 && format_index < DLS_FORMAT_COUNT)
  {
    _choice_format->value(format_index);
    _format_selected = true;

    if (format_index == DLS_FORMAT_MDCT)
    {
      _choice_mdct->activate();
    }
  }
  else
  {
    ((Fl_Menu_*) _choice_format)->value((Fl_Menu_Item*) 0);
    _choice_format->redraw();
    _format_selected = false;
  }

  if (mdct_block_size_equal && _format_selected && format_index == DLS_FORMAT_MDCT)
  {
    exp2 = logb(mdct_block_size) / logb(2);

    if (exp2 == (int) exp2 && exp2 >= MDCT_MIN_EXP && exp2 < MDCT_MAX_EXP_PLUS_ONE)
    {
      str.str("");
      str.clear();
      str << mdct_block_size;
      _choice_mdct->value((int) exp2 - MDCT_MIN_EXP);
      _mdct_selected = true;
    }
  }
  if (!_mdct_selected)
  {
    ((Fl_Menu_*) _choice_mdct)->value((Fl_Menu_Item*) 0);
    _choice_mdct->redraw();
    _mdct_selected = false;
  }

  _updated = false;

  _wnd->show();

  while (_wnd->shown()) Fl::wait();
}

//---------------------------------------------------------------

void CTLDialogChannel::_callback(Fl_Widget *sender, void *data)
{
  CTLDialogChannel *dialog = (CTLDialogChannel *) data;

  if (sender == dialog->_button_ok) dialog->_button_ok_clicked();
  if (sender == dialog->_button_cancel) dialog->_button_cancel_clicked();
  if (sender == dialog->_wnd) dialog->_button_cancel_clicked();
  if (sender == dialog->_choice_format) dialog->_choice_format_changed();
  if (sender == dialog->_choice_mdct) dialog->_choice_mdct_changed();
}

//---------------------------------------------------------------

void CTLDialogChannel::_button_ok_clicked()
{
  if (!_save_channels()) return;

  _wnd->hide();
}

//---------------------------------------------------------------

void CTLDialogChannel::_button_cancel_clicked()
{
  _updated = false;
  _wnd->hide();
}

//---------------------------------------------------------------

void CTLDialogChannel::_choice_format_changed()
{
  if (_choice_format->value() == DLS_FORMAT_MDCT)
  {
    _choice_mdct->activate();
  }
  else
  {
    ((Fl_Menu_*) _choice_mdct)->value((Fl_Menu_Item*) 0);
    _choice_mdct->deactivate();
    _mdct_selected = false;
  }
}

//---------------------------------------------------------------

void CTLDialogChannel::_choice_mdct_changed()
{
  _mdct_selected = true;
}

//---------------------------------------------------------------

bool CTLDialogChannel::_save_channels()
{
  COMChannelPreset channel;
  stringstream str;
  list<const COMChannelPreset *>::const_iterator channel_i;
  unsigned int freq, block, mask, red;
  bool write_freq, write_block, write_mask, write_red;
  bool channels_changed = false;
  list<COMChannelPreset> channel_backups;
  list<COMChannelPreset>::iterator backup_i;

  str.exceptions(ios::failbit | ios::badbit);

  if (_format_selected && _choice_format->value() == DLS_FORMAT_MDCT && !_mdct_selected)
  {
    cout << "ERROR: no mdct blocksize selected!" << endl;
    return false;
  }

  try
  {
    if ((write_freq = (string(_input_freq->value()) != "")))
    {
      str.str("");
      str.clear();
      str << _input_freq->value();
      str >> freq;
    }
    
    if ((write_block = (string(_input_block->value()) != "")))
    {
      str.str("");
      str.clear();
      str << _input_block->value();
      str >> block;
    }
    
    if ((write_mask = (string(_input_mask->value()) != "")))
    {
      str.str("");
      str.clear();
      str << _input_mask->value();
      str >> mask;
    }

    if ((write_red = (string(_input_red->value()) != "")))
    {
      str.str("");
      str.clear();
      str << _input_red->value();
      str >> red;
    }
  }
  catch (...)
  {
    cout << "ERROR: illegal value!" << endl;
    return false;
  }

  channel_i = _channels->begin();
  while (channel_i != _channels->end())
  {
    // Daten von Original-Kanal kopieren
    channel = **channel_i;

    // Soll sich für diesen Kanal etwas ändern?
    if ((write_freq && channel.sample_frequency != freq) ||
        (write_block && channel.block_size != block) ||
        (write_mask && channel.meta_mask != mask) ||
        (write_red && channel.meta_reduction != red) ||
        (_format_selected && channel.format_index != _choice_format->value()) ||
        (_format_selected && _choice_format->value() == DLS_FORMAT_MDCT
         && channel.mdct_block_size != (unsigned int) pow(2.0, _choice_mdct->value() + MDCT_MIN_EXP)))
    {
      // Alte Vorgaben sichern, falls etwas schief geht
      channel_backups.push_back(channel);

      // Neue Daten übernehmen
      if (write_freq) channel.sample_frequency = freq;
      if (write_block) channel.block_size = block;
      if (write_mask) channel.meta_mask = mask;
      if (write_red) channel.meta_reduction = red;
      if (_format_selected)
      {
        channel.format_index = _choice_format->value();

        if (channel.format_index == DLS_FORMAT_MDCT)
        {
          channel.mdct_block_size = (unsigned int) pow(2.0, _choice_mdct->value() + MDCT_MIN_EXP);
        }
      }

      try
      {
        _job->change_channel(&channel);
      }
      catch (ECOMJobPreset &e)
      {
        cout << "ERROR: " << e.msg << endl;
        return false;
      }

      channels_changed = true;
    }

    channel_i++;
  }

  if (channels_changed)
  {
    try
    {
      _job->write(_dls_dir);
    }
    catch (ECOMJobPreset &e)
    {
      cout << "ERROR: " << e.msg << endl;

      try
      {
        backup_i = channel_backups.begin();
        while (backup_i != channel_backups.end())
        {
          _job->change_channel(&(*backup_i));
          backup_i++;
        }
      }
      catch (ECOMChannelPreset &e)
      {
        cout << "FATAL: " << e.msg << "!" << endl;
        cout << "Please restart application to avoid data loss!" << endl;
      }
    
      return false;
    }

    try
    {
      _job->notify_changed(_dls_dir);
    }
    catch (ECOMJobPreset &e)
    {
      cout << "WARNING: dlsd could not be notified!" << endl;
    }
  
    _updated = true;
  }
  else
  {
    _updated = false;
  }

  return true;
}  

//---------------------------------------------------------------
