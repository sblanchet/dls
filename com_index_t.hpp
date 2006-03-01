/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef ComIndexTHpp
#define ComIndexTHpp

/*****************************************************************************/

#include <fcntl.h>

/*****************************************************************************/

#include "com_exception.hpp"
#include "com_time.hpp"
#include "com_file.hpp"

/*****************************************************************************/

/**
   Exception eines COMIndexT-Objektes
*/

class ECOMIndexT : public COMException
{
public:
  ECOMIndexT(const string &pmsg) : COMException(pmsg) {};
};

/*****************************************************************************/

/**
   Binäre Indexdatei mit beliebiger Datenstruktur

   Unterstützt schnelles Auslesen, überprüfen
   und Schreiben
*/

template <class REC>
class COMIndexT
{
public:
  COMIndexT();
  ~COMIndexT();
  
  // Dazeistatus
  void open_read(const string &);
  void open_read_write(const string &);
  void open_read_append(const string &);
  void close();
  bool open() const;

  // Lesezugriff
  REC operator[](unsigned int);

  // Schreibzugriff
  void append_record(const REC *);
  void change_record(unsigned int, const REC *);

  unsigned int record_count() const;

private:
  COMFile _file;
  unsigned int _record_count;
  unsigned int _position;
};

/*****************************************************************************/

/**
   Konstruktor
*/

template <class REC>
COMIndexT<REC>::COMIndexT()
{
}

/*****************************************************************************/

/**
   Destruktor
*/

template <class REC>
COMIndexT<REC>::~COMIndexT()
{
  try
  {
    _file.close();
  }
  catch (ECOMFile &e)
  {
    // Fehler schlucken
  }
}

/*****************************************************************************/

/**
   Öffnen einer Index-Datei

   Öffnet die Datei, liest die Dateigröße aus und berechnet
   so die Anzahl der Index-Einträge.

   \param file_name Dateiname der Index-Datei
   \throw ECOMIndexT Datei nicht zu öffnen oder ungültig
*/

template <class REC>
void COMIndexT<REC>::open_read(const string &file_name)
{
  stringstream err;
  long long size;

  try
  {
    _file.open_read(file_name.c_str());
    size = _file.calc_size();
    _file.seek(0);
  }
  catch (ECOMFile &e)
  {
    throw ECOMIndexT(e.msg);
  }

  if (size % sizeof(REC) != 0)
  {
    err << "Illegal size of index file \"" << file_name << "\"";

    try
    {
      close();
    }
    catch (ECOMFile &e)
    {
      err << " - closing: " << e.msg;
    }

    throw ECOMIndexT(err.str());
  }

  _record_count = size / sizeof(REC);
  _position = 0;
}

/*****************************************************************************/

/**
   Öffnet eine binäre Index-Datei zum Lesen und Schreiben

   Siehe open()

   \param file_name Dateiname der Index-Datei
   \throw ECOMIndexT Datei nicht zu öffnen oder ungültig
*/

template <class REC>
void COMIndexT<REC>::open_read_write(const string &file_name)
{
  stringstream err;
  long long size;

  try
  {
    _file.open_read_write(file_name.c_str());
    size = _file.calc_size();
    _file.seek(0);
  }
  catch (ECOMFile &e)
  {
    throw ECOMIndexT(e.msg);
  }
  
  if (size % sizeof(REC) != 0)
  {
    err << "Illegal size of index file \"" << file_name << "\"";

    try
    {
      close();
    }
    catch (ECOMFile &e)
    {
      err << " - closing: " << e.msg;
    }

    throw ECOMIndexT(err.str());
  }

  _record_count = size / sizeof(REC);
  _position = 0;
}

/*****************************************************************************/

/**
   Öffnet eine binäre Index-Datei zum Lesen und Anhängen

   Siehe open()

   \param file_name Dateiname der Index-Datei
   \throw ECOMIndexT Datei nicht zu öffnen oder ungültig
*/

template <class REC>
void COMIndexT<REC>::open_read_append(const string &file_name)
{
  stringstream err;
  long long size;

  try
  {
    _file.open_read_append(file_name.c_str());
    size = _file.calc_size();
    _file.seek(0);
  }
  catch (ECOMFile &e)
  {
    throw ECOMIndexT(e.msg);
  }
  
  if (size % sizeof(REC) != 0)
  {
    err << "Illegal size of index file \"" << file_name << "\"";

    try
    {
      close();
    }
    catch (ECOMFile &e)
    {
      err << " - closing: " << e.msg;
    }

    throw ECOMIndexT(err.str());
  }

  _record_count = size / sizeof(REC);
  _position = 0;
}

/*****************************************************************************/

/**
   Schliesst die Index-Datei
*/

template <class REC>
void COMIndexT<REC>::close()
{
  try
  {
    _file.close();
  }
  catch (ECOMFile &e)
  {
    throw ECOMIndexT(e.msg);
  }
}

/*****************************************************************************/

/**
   Gibt zurück, ob die Index-Datei geöffnet ist

   \return true, wenn Datei offen
*/

template <class REC>
bool COMIndexT<REC>::open() const
{
  return _file.open();
}

/*****************************************************************************/

/**
   Liefert die Anzahl der Einträge im Index

   \return Anzahl der Records
*/

template <class REC>
inline unsigned int COMIndexT<REC>::record_count() const
{
  return _record_count;
}

/*****************************************************************************/

/**
   Index-Operator: Liest einen Eintrag aus dem Index

   Springt zu der entsprechenden Position in der Datei
   und liest dort einen Record.

   \param index Index des Records
   \return Record-Daten
   \throw ECOMIndexT Datei nicht offen, falscher
                     Index oder Lesefehler
*/

template <class REC>
REC COMIndexT<REC>::operator[](unsigned int index)
{
  stringstream err;
  REC index_record;
  unsigned int bytes_read;

  if (!_file.open())
  {
    throw ECOMIndexT("Index not open!");
  }

  if (index >= _record_count)
  {
    throw ECOMIndexT("Index out of range!");
  }

  try
  {
    if (_position != index * sizeof(REC))
    {
      _file.seek(index * sizeof(REC));
    }

    _file.read((char *) &index_record, sizeof(REC), &bytes_read);

    if (bytes_read != sizeof(REC))
    {
      err << "Did not read enough bytes!";

      try
      {
        _file.close();
      }
      catch (ECOMFile &e)
      {
        err << " - close: " << e.msg;
      }

      throw ECOMIndexT(err.str());
    }
  }
  catch (ECOMFile &e)
  {
    throw ECOMIndexT(e.msg);
  }

  _position = (index + 1) * sizeof(REC);

  return index_record;
}

/*****************************************************************************/

/**
   Fügt einen neuen Record an den Index an

   \param index_record Konstanter Zeiger auf den neuen Record
   \throw ECOMIndexT Datei nicht zum Schreiben geöffnet, oder
                     Schreibfehler
*/

template <class REC>
void COMIndexT<REC>::append_record(const REC *index_record)
{
  if (!_file.open())
  {
    throw ECOMIndexT("Index not open!");
  }

  if (_file.open_mode() != fomOpenReadAppend)
  {
    throw ECOMIndexT("Index not opened for appending!");
  }

  try
  {
    _file.append((const char *) index_record, sizeof(REC));
  }
  catch (ECOMFile &e)
  {
    throw ECOMIndexT(e.msg);
  }

  _record_count++;
  _position = _record_count * sizeof(REC);
}

/*****************************************************************************/

/**
   Ändert einen bestimmten Record im Index

   \param index Index des zu ändernden Records
   \param index_record Konstanter Zeiger auf neuen Record
   \throw ECOMIndexT Record konnte nicht geändert werden
*/

template <class REC>
void COMIndexT<REC>::change_record(unsigned int index,
                                   const REC *index_record)
{
  if (_file.open_mode() != fomOpenReadWrite)
  {
    throw ECOMIndexT("Index not open for writing!");
  }

  if (index >= _record_count)
  {
    throw ECOMIndexT("Index out of range!");
  }

  try
  {
    if (_position != index * sizeof(REC))
    {
      _file.seek(index * sizeof(REC));
    }

    _file.write((char *) index_record, sizeof(REC));
  }
  catch (ECOMFile &e)
  {
    throw ECOMIndexT(e.msg);
  }

  _position = (index + 1) * sizeof(REC);
}

/*****************************************************************************/

#endif
