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
#include "ctl_msg_wnd.hpp"

//---------------------------------------------------------------

RCS_ID("$Header: /home/fp/dls/src/RCS/ctl_dialog_job_edit.cpp,v 1.14 2005/02/02 10:36:41 fp Exp $");

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

  _input_quota_time = new Fl_Input(10, 180, 200, 25, "Zeit-Quota");
  _input_quota_time->align(FL_ALIGN_TOP_LEFT);

  _input_quota_size = new Fl_Input(10, 230, 200, 25, "Daten-Quota");
  _input_quota_size->align(FL_ALIGN_TOP_LEFT);

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
  string quota_str;

  _job = job;
  _updated = false;

  // Wenn ein bestehender Auftrag editiert werden soll,
  // müssen zuerst dessen Daten geladen werden
  if (_job)
  {
    _input_desc->value(_job->description().c_str());
    _input_source->value(_job->source().c_str());
    _input_trigger->value(_job->trigger().c_str());

    _convert_time_quota_to_str(_job->quota_time(), &quota_str);
    _input_quota_time->value(quota_str.c_str());

    _convert_size_quota_to_str(_job->quota_size(), &quota_str);
    _input_quota_size->value(quota_str.c_str());
  }

  // Wenn der Auftrag schon existiert, soll kein Editieren
  // der Datenquelle mehr möglich sein!
  _input_source->readonly(_job != 0);

  // Fenster anzeigen
  _wnd->show();
      
  // Solange in der Event-Loop bleiben, wie das fenster sichtbar ist
  while (_wnd->shown()) Fl::wait();
}

//---------------------------------------------------------------

/**
   Statische Callback-Funktion

   \param sender Zeiger auf das Widget, dass den Callback ausgelöst hat
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
  // Dialog schließen
  _wnd->hide();
}

//---------------------------------------------------------------

/**
   Geänderte Auftragsdaten speichern

   \return true, wenn Daten gespeichert wurden
*/

bool CTLDialogJobEdit::_save_job()
{
  CTLJobPreset job_copy;
  string quota_str;
  long long quota_time, quota_size;
  
  if (!_convert_str_to_time_quota(_input_quota_time->value(), &quota_time)) return false;
  if (!_convert_str_to_size_quota(_input_quota_size->value(), &quota_size)) return false;

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
      // Fehler! Alte Daten zurückladen
      *_job = job_copy;
      msg_win->str() << e.msg;
      msg_win->error();
      return false;
    }
    
    // Die Auftragsdaten wurden jetzt verändert
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

  if (!_convert_str_to_time_quota(_input_quota_time->value(), &quota_time)) return false;
  if (!_convert_str_to_size_quota(_input_quota_size->value(), &quota_size)) return false;

  // Neue Job-ID aus der ID-Sequence-Datei lesen
  if (!_get_new_id(&new_id))
  {
    msg_win->str() << "Could not fetch new id!";
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
  
  // Die Auftragsdaten wurden verändert
  _updated = true;
  
  try
  {
    // Versuchen, eine Spooling-Eintrag zu erzeugen
    job.spool(_dls_dir);
  }
  catch (ECOMJobPreset &e)
  {
    // Fehler! Aber nur eine Warnung ausgeben.
    msg_win->str() << "Could not notify dlsd: " << e.msg;
    msg_win->warning();
  }
  
  return true;
}

//---------------------------------------------------------------

/**
   Erhöht die ID in der Sequenzdatei und liefert den alten Wert

   \param id Zeiger auf Speicher für die neue ID
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
   Konvertiert die aktuelle Zeit-Quota zu einem lesbaren String

   Bei keiner Zeit-Quota wird ein leerer String
   zurückgeliefert (der auch korrekt zurückkonvertiert wird).
   Andernfalls werden die Einheiten Sekunde (s), Minute (m),
   Stunde (h) oder Tag (d) angehängt.

   \param time_quota Zu konvertierende Zeit-Quota
   \param quota_str Zeiger auf den String, in dem das
                    konvertierte Ergebnis hinterlegt werden soll
*/

void CTLDialogJobEdit::_convert_time_quota_to_str(long long time_quota,
                                                  string *quota_str)
{
  stringstream str;

  if (time_quota > 0)
  {
    if (time_quota % 86400 == 0)
    {
      str << time_quota / 86400 << "d";
    }
    else if (time_quota % 3600 == 0)
    {
      str << time_quota / 3600 << "h";
    }
    else if (time_quota % 60 == 0)
    {
      str << time_quota / 60 << "m";
    }
    else
    {
      str << time_quota << "s";
    }
  }

  *quota_str = str.str();
}

//---------------------------------------------------------------

/**
   Konvertiert die aktuelle Daten-Quota zu einem lesbaren String

   Bei keiner Daten-Quota wird ein leerer String
   zurückgeliefert (der auch korrekt zurückkonvertiert wird).
   Andernfalls werden die Einheiten Kilobyte (K), Megabyte (M),
   oder Gigabyte (G) angehängt.

   \param size_quota Zu konvertierende Daten-Quota
   \param quota_str Zeiger auf den String, in dem das
                    konvertierte Ergebnis hinterlegt werden soll
*/

void CTLDialogJobEdit::_convert_size_quota_to_str(long long size_quota,
                                                  string *quota_str)
{
  stringstream str;

  if (size_quota > 0)
  {
    if (size_quota % 1073741824 == 0) // GB
    {
      str << size_quota / 1073741824 << "G";
    }
    else if (size_quota % 1048576 == 0) // MB
    {
      str << size_quota / 1048576 << "M";
    }
    else
    {
      str << size_quota;
    }
  }

  *quota_str = str.str();
}

//---------------------------------------------------------------

/**
   Konvertiert den eingegebenen String in eine Zeit-Quota

   \param quota_str  Zeiger auf den String, in dem das
                     konvertierte Ergebnis hinterlegt werden soll
   \param time_quota Zu konvertierende Daten-Quota
*/

bool CTLDialogJobEdit::_convert_str_to_time_quota(const string &quota_str,
                                                  long long *time_quota)
{
  stringstream str;
  long long number;
  string extension;

  number = 0;

  str.exceptions(ios::badbit | ios::failbit);

  if (quota_str.length() > 0)
  {
    str << quota_str;

    try
    {
      str >> number;
    }
    catch (...)
    {
      msg_win->str() << "Time quota must be numeric!";
      msg_win->error();
      return false;
    }

    str >> extension;

    if (extension != "s" && extension != "")
    {
      if (extension == "m")
      {
        number *= 60;
      }
      else if (extension == "h")
      {
        number *= 3600;
      }
      else if (extension == "d")
      {
        number *= 86400;
      }
      else
      {
        msg_win->str() << "Unknown time extension! Valid are s, m, h and d.";
        msg_win->error();
        return false;
      }
    }
  }

  *time_quota = number;
  return true;
}

//---------------------------------------------------------------

/**
   Konvertiert den eingegebenen String in eine Zeit-Quota

   \param quota_str  Zeiger auf den String, in dem das
                     konvertierte Ergebnis hinterlegt werden soll
   \param size_quota Zu konvertierende Daten-Quota
*/

bool CTLDialogJobEdit::_convert_str_to_size_quota(const string &quota_str,
                                                  long long *size_quota)
{
  stringstream str;
  long long number;
  string extension;

  number = 0;

  str.exceptions(ios::badbit | ios::failbit);

  if (quota_str.length() > 0)
  {
    str << quota_str;

    try
    {
      str >> number;
    }
    catch (...)
    {
      msg_win->str() << "Size quota must be numeric!";
      msg_win->error();
      return false;
    }

    str >> extension;

    if (extension != "")
    {
      if (extension == "M")
      {
        number *= 1048576;
      }
      else if (extension == "G")
      {
        number *= 1073741824;
      }
      else
      {
        msg_win->str() << "Unknown size extension! Valid M and G.";
        msg_win->error();
        return false;
      }
    }
  }

  *size_quota = number;
  return true;
}

//---------------------------------------------------------------
