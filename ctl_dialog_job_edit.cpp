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

#include "ctl_globals.hpp"
#include "ctl_dialog_job_edit.hpp"
#include "ctl_dialog_msg.hpp"

//---------------------------------------------------------------

RCS_ID("$Header: /home/fp/dls/src/RCS/ctl_dialog_job_edit.cpp,v 1.17 2005/03/08 08:55:33 fp Exp $");

//---------------------------------------------------------------

#define WIDTH 220
#define HEIGHT 300

//---------------------------------------------------------------

/**
   Konstruktor
*/

CTLDialogJobEdit::CTLDialogJobEdit(const string &dls_dir)
{
  int x = Fl::w() / 2 - WIDTH / 2;
  int y = Fl::h() / 2 - HEIGHT / 2;

  _dls_dir = dls_dir;
  _updated = false;

  _wnd = new Fl_Double_Window(x, y, WIDTH, HEIGHT, "Auftrag �ndern");
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

  _input_quota_time = new Fl_Input(10, 180, 90, 25, "Zeit-Quota");
  _input_quota_time->align(FL_ALIGN_TOP_LEFT);

  _choice_time_ext = new Fl_Choice(110, 180, 100, 25);
  _choice_time_ext->add("Sekunden");
  _choice_time_ext->add("Minuten");
  _choice_time_ext->add("Stunden");
  _choice_time_ext->add("Tage");
  _choice_time_ext->value(2);

  _input_quota_size = new Fl_Input(10, 230, 90, 25, "Daten-Quota");
  _input_quota_size->align(FL_ALIGN_TOP_LEFT);

  _choice_size_ext = new Fl_Choice(110, 230, 100, 25);
  _choice_size_ext->add("Byte");
  _choice_size_ext->add("Megabyte");
  _choice_size_ext->add("Gigabyte");
  _choice_size_ext->value(1);

  _wnd->end();
}

//---------------------------------------------------------------

/**
   Destruktor
*/

CTLDialogJobEdit::~CTLDialogJobEdit()
{
  delete _wnd;
}

//---------------------------------------------------------------

/**
   Dialog anzeigen

   \param job zeiger auf den zu editiernden Messauftrag
*/

void CTLDialogJobEdit::show(CTLJobPreset *job)
{
  _job = job;
  _updated = false;

  // Wenn ein bestehender Auftrag editiert werden soll,
  // m�ssen zuerst dessen Daten geladen werden
  if (_job)
  {
    _input_desc->value(_job->description().c_str());
    _input_source->value(_job->source().c_str());
    _input_trigger->value(_job->trigger().c_str());
    _display_time_quota();
    _display_size_quota();
  }

  // Wenn der Auftrag schon existiert, soll kein Editieren
  // der Datenquelle mehr m�glich sein!
  _input_source->readonly(_job != 0);

  // Fenster anzeigen
  _wnd->show();
      
  // Solange in der Event-Loop bleiben, wie das fenster sichtbar ist
  while (_wnd->shown()) Fl::wait();
}

//---------------------------------------------------------------

/**
   Statische Callback-Funktion

   \param sender Zeiger auf das Widget, dass den Callback ausgel�st hat
   \param data Zeiger auf den Dialog
*/

void CTLDialogJobEdit::_callback(Fl_Widget *sender, void *data)
{
  CTLDialogJobEdit *dialog = (CTLDialogJobEdit *) data;

  if (sender == dialog->_button_ok) dialog->_button_ok_clicked();
  if (sender == dialog->_button_cancel) dialog->_button_cancel_clicked();
  if (sender == dialog->_wnd) dialog->_button_cancel_clicked();
}

//---------------------------------------------------------------

/**
   Callback: Der "OK"-Button wurde geklickt
*/

void CTLDialogJobEdit::_button_ok_clicked()
{
  // Wenn ein existierender Auftrag editiert wird
  if (_job)
  {
    // Diesen Speichern
    if (!_save_job())
    {
      // Fehler beim Speichern. Dialog offen lassen!
      return;
    }
  }

  // Wenn ein neuer Auftrag editiert wird
  else
  {
    // Diesen erstellen
    if (!_create_job())
    {
       // Fehler beim Erstellen. Dialog offen lassen!
      return;
    }
  }

  // Alles OK! Fenster schliessen
  _wnd->hide();
}

//---------------------------------------------------------------

/**
   Callback: Der "Abbrechen"-Button wurde geklickt
*/

void CTLDialogJobEdit::_button_cancel_clicked()
{
  // Dialog schlie�en
  _wnd->hide();
}

//---------------------------------------------------------------

/**
   Ge�nderte Auftragsdaten speichern

   \return true, wenn Daten gespeichert wurden
*/

bool CTLDialogJobEdit::_save_job()
{
  CTLJobPreset job_copy;
  string quota_str;
  long long quota_time, quota_size;
  
  if (!_calc_time_quota(&quota_time)) return false;
  if (!_calc_size_quota(&quota_size)) return false;

  if (_job->description() != _input_desc->value()
      || _job->source() != _input_source->value()
      || _job->trigger() != _input_trigger->value()
      || _job->quota_time() != quota_time
      || _job->quota_size() != quota_size)
  {
    // Alte Daten sichern
    job_copy = *_job;

    // Neue Daten setzen
    _job->description(_input_desc->value());
    _job->source(_input_source->value());
    _job->trigger(_input_trigger->value());
    _job->quota_time(quota_time);
    _job->quota_size(quota_size);

    try
    {
      // Versuchen, die Daten zu speichern
      _job->write(_dls_dir);
    }
    catch (ECOMJobPreset &e)
    {
      // Fehler! Alte Daten zur�ckladen
      *_job = job_copy;
      msg_win->str() << e.msg;
      msg_win->error();
      return false;
    }
    
    // Die Auftragsdaten wurden jetzt ver�ndert
    _updated = true;

    try
    {
      // Versuchen, einen Spooling-Eintrag zu erzeugen
      _job->spool(_dls_dir);
    }
    catch (ECOMJobPreset &e)
    {
      // Fehler! Aber nur Warnung ausgeben.
      msg_win->str() << "Konnte den dlsd nicht benachrichtigen: " << e.msg;
      msg_win->warning();
    }
  }
  
  return true;
}

//---------------------------------------------------------------

/**
   Einen neuen Erfassungsauftrag erstellen

   \return true, wenn neue Auftrag erstellt wurde
*/

bool CTLDialogJobEdit::_create_job()
{
  int new_id;
  CTLJobPreset job;
  long long quota_time, quota_size;

  if (!_calc_time_quota(&quota_time)) return false;
  if (!_calc_size_quota(&quota_size)) return false;

  // Neue Job-ID aus der ID-Sequence-Datei lesen
  if (!_get_new_id(&new_id))
  {
    msg_win->str() << "Es konnte keine neue Auftrags-ID ermittelt werden!";
    msg_win->error();
    return false;
  }

  // Auftragsdaten setzen
  job.id(new_id);
  job.description(_input_desc->value());
  job.source(_input_source->value());
  job.trigger(_input_trigger->value());
  job.quota_time(quota_time);
  job.quota_size(quota_size);
  job.running(false);

  try
  {
    // Versuchen, die neuen Daten zu speichern
    job.write(_dls_dir);
  }
  catch (ECOMJobPreset &e)
  {
    // Fehler beim Speichern!
    msg_win->str() << e.msg;
    msg_win->error();
    return false;
  }
  
  // Die Auftragsdaten wurden ver�ndert
  _updated = true;
  
  try
  {
    // Versuchen, eine Spooling-Eintrag zu erzeugen
    job.spool(_dls_dir);
  }
  catch (ECOMJobPreset &e)
  {
    // Fehler! Aber nur eine Warnung ausgeben.
    msg_win->str() << "Konnte den dlsd nicht benachrichtigen: " << e.msg;
    msg_win->warning();
  }
  
  return true;
}

//---------------------------------------------------------------

/**
   Erh�ht die ID in der Sequenzdatei und liefert den alten Wert

   \param id Zeiger auf Speicher f�r die neue ID
   \return true, wenn die neue ID geholt werden konnte
*/

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
    // Dateiinhalt in einen Integer konvertieren
    id_in_file >> *id;
  }
  catch (...)
  {
    // Fehler beim Konvertieren
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
    // Neuen ID-Wert in die Datei schreiben
    id_out_file << (*id + 1) << endl;
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

/**
   Zeigt die aktuelle Zeit-Quota an
*/

void CTLDialogJobEdit::_display_time_quota()
{
  long long time_quota = _job->quota_time();
  stringstream str;

  if (time_quota > 0)
  {
    if (time_quota % 86400 == 0)
    {
      time_quota /= 86400;
      _choice_time_ext->value(3);
    }
    else if (time_quota % 3600 == 0)
    {
      time_quota /= 3600;
      _choice_time_ext->value(2);
    }
    else if (time_quota % 60 == 0)
    {
      time_quota /= 60;
      _choice_time_ext->value(1);
    }
    else
    {
      _choice_time_ext->value(0);
    }

    str << time_quota;
  }
  else
  {
    _choice_time_ext->value(2);
  }

  _input_quota_time->value(str.str().c_str());
}

//---------------------------------------------------------------

/**
   Zeigt die aktuelle Daten-Quota an
*/

void CTLDialogJobEdit::_display_size_quota()
{
  long long size_quota = _job->quota_size();
  stringstream str;

  if (size_quota > 0)
  {
    if (size_quota % 1073741824 == 0) // GB
    {
      size_quota /= 1073741824;
      _choice_size_ext->value(2);
    }
    else if (size_quota % 1048576 == 0) // MB
    {
      size_quota /= 1048576;
      _choice_size_ext->value(1);
    }
    else
    {
      _choice_size_ext->value(0);
    }

    str << size_quota;
  }
  else
  {
    _choice_size_ext->value(1);
  }

  _input_quota_size->value(str.str().c_str());
}

//---------------------------------------------------------------

/**
   Berechnet die Zeit-Quota

   \param time_quota Zeiger auf Variable f�r berechnete Quota
   \return true, wenn Quota berechnet werden konnte
*/

bool CTLDialogJobEdit::_calc_time_quota(long long *time_quota)
{
  stringstream str;
  string quota_str = _input_quota_time->value();
  int ext = _choice_time_ext->value();

  str.exceptions(ios::badbit | ios::failbit);

  if (quota_str.length() > 0)
  {
    str << quota_str;

    try
    {
      str >> *time_quota;
    }
    catch (...)
    {
      msg_win->str() << "Die Zeit-Quota muss eine Ganzzahl sein!";
      msg_win->error();
      return false;
    }

    if (ext == 1)
    {
      *time_quota *= 60;
    }
    else if (ext == 2)
    {
      *time_quota *= 3600;
    }
    else if (ext == 3)
    {
       *time_quota *= 86400;
    }
  }
  else
  {
    *time_quota = 0;
  }

  return true;
}

//---------------------------------------------------------------

/**
   Berechnet die angegebene Daten-Quota

   \param size_quota Zeiger auf Variable f�r berechnete Quota
   \return true, wenn Quota berechnet wurde
*/

bool CTLDialogJobEdit::_calc_size_quota(long long *size_quota)
{
  stringstream str;
  string quota_str = _input_quota_size->value();
  int ext = _choice_size_ext->value();

  str.exceptions(ios::badbit | ios::failbit);

  if (quota_str.length() > 0)
  {
    str << quota_str;

    try
    {
      str >> *size_quota;
    }
    catch (...)
    {
      msg_win->str() << "Die Daten-Quota muss eine Ganzzahl sein!";
      msg_win->error();
      return false;
    }

    if (ext == 1)
    {
      *size_quota *= 1048576;
    }
    else if (ext == 2)
    {
      *size_quota *= 1073741824;
    }
  }
  else
  {
    *size_quota = 0;
  }

  return true;
}

//---------------------------------------------------------------
