//---------------------------------------------------------------
//
//  C O M _ I N D E X _ T . H P P
//
//---------------------------------------------------------------

#ifndef ComIndexTHpp
#define ComIndexTHpp

//---------------------------------------------------------------

#include "com_exception.hpp"
#include "com_time.hpp"

//---------------------------------------------------------------

/**
   Exception eines COMIndexT-Objektes
*/

class ECOMIndexT : public COMException
{
public:
  ECOMIndexT(const string &pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

/**
   Index-Record für einen Datendatei-Index innerhalb eines Chunks
*/

struct COMIndexRecord
{
  long long start_time;
  long long end_time;
  unsigned int position;
};

//---------------------------------------------------------------

/**
   Index für alle Datendateien eines Chunks
*/

struct COMGlobalIndexRecord
{
  long long start_time;
  long long end_time;
};

//---------------------------------------------------------------

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
  void open(const string &);
  void open_rw(const string &);
  void close();

  // Lesezugriff
  REC operator[](unsigned int);

  // Schreibzugriff
  void append_record(const REC *);
  void change_record(unsigned int, const REC *);

  unsigned int record_count() const;

private:
  int _fd;
  bool _open;
  bool _opened_read_only;
  unsigned int _record_count;
  unsigned int _position;
};

//---------------------------------------------------------------

/**
   Konstruktor
*/

template <class REC>
COMIndexT<REC>::COMIndexT()
{
  _open = false;
}

//---------------------------------------------------------------

/**
   Destruktor
*/

template <class REC>
COMIndexT<REC>::~COMIndexT()
{
  close();
}

//---------------------------------------------------------------

/**
   Öffnen einer Index-Datei

   Öffnet die Datei, liest die Dateigröße aus und berechnet
   so die Anzahl der Index-Einträge.

   \param file_name Dateiname der Index-Datei
   \throw ECOMIndexT Datei nicht zu öffnen oder ungültig
*/

template <class REC>
void COMIndexT<REC>::open(const string &file_name)
{
  stringstream err;
  off_t seek_ret;

  close();

  if ((_fd = ::open(file_name.c_str(), O_RDONLY)) == -1)
  {
    err << "could not open index file \"" << file_name << "\"";
    throw ECOMIndexT(err.str());
  }

  _open = true;
  _opened_read_only = true;

  if ((seek_ret = lseek(_fd, 0, SEEK_END)) == (off_t) - 1)
  {
    close();
    err << "seek failed in index file \"" << file_name << "\"";
    throw ECOMIndexT(err.str());
  }

  if (seek_ret % sizeof(REC) != 0)
  {
    close();
    err << "illegal size of index file \"" << file_name << "\"";
    throw ECOMIndexT(err.str());
  }

  _record_count = seek_ret / sizeof(REC);

  if ((seek_ret = lseek(_fd, 0, SEEK_SET)) == (off_t) - 1)
  {
    close();
    err << "seek failed in index file \"" << file_name << "\"";
    throw ECOMIndexT(err.str());
  }

  _position = 0;
}

//---------------------------------------------------------------

/**
   Öffnet eine binäre Index-Datei zum Lesen und Schreiben

   Siehe open()

   \param file_name Dateiname der Index-Datei
   \throw ECOMIndexT Datei nicht zu öffnen oder ungültig
*/

template <class REC>
void COMIndexT<REC>::open_rw(const string &file_name)
{
  stringstream err;
  off_t seek_ret;

  close();

  if ((_fd = ::open(file_name.c_str(), O_RDWR | O_CREAT, 0644)) == -1)
  {
    err << "could not open index file (RD/WR) \"" << file_name << "\"";
    throw ECOMIndexT(err.str());
  }

  _open = true;
  _opened_read_only = false;

  if ((seek_ret = lseek(_fd, 0, SEEK_END)) == (off_t) - 1)
  {
    close();
    err << "seek failed in index file \"" << file_name << "\"";
    throw ECOMIndexT(err.str());
  }

  if (seek_ret % sizeof(REC) != 0)
  {
    close();
    err << "illegal size of index file \"" << file_name << "\"";
    throw ECOMIndexT(err.str());
  }

  _record_count = seek_ret / sizeof(REC);

  if ((seek_ret = lseek(_fd, 0, SEEK_SET)) == (off_t) - 1)
  {
    close();
    err << "seek failed in index file \"" << file_name << "\"";
    throw ECOMIndexT(err.str());
  }

  _position = 0;
}

//---------------------------------------------------------------

/**
   Schliesst die Index-Datei
*/

template <class REC>
void COMIndexT<REC>::close()
{
  if (_open)
  {
    ::close(_fd);
    _open = false;
  }
}

//---------------------------------------------------------------

/**
   Liefert die Anzahl der Einträge im Index

   \return Anzahl der Records
*/

template <class REC>
inline unsigned int COMIndexT<REC>::record_count() const
{
  return _record_count;
}

//---------------------------------------------------------------

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
  REC index_record;

  if (!_open)
  {
    throw ECOMIndexT("index not open!");
  }

  if (index >= _record_count)
  {
    throw ECOMIndexT("index out of range!");
  }

  if (_position != index * sizeof(REC))
  {
    if (lseek(_fd, index * sizeof(REC), SEEK_SET) == (off_t) - 1)
    {
      close();
      throw ECOMIndexT("could not seek in file!");
    }
  }

  if (read(_fd, &index_record, sizeof(REC)) != sizeof(REC))
  {
    close();
    throw ECOMIndexT("could not read from file!");
  }

  _position = (index + 1) * sizeof(REC);

  return index_record;
}

//---------------------------------------------------------------

/**
   Fügt einen neuen Record an den Index an

   \param index_record Konstanter Zeiger auf den neuen Record
   \throw ECOMIndexT Datei nicht zum Schreiben geöffnet, oder
                     Schreibfehler
*/

template <class REC>
void COMIndexT<REC>::append_record(const REC *index_record)
{
  if (!_open)
  {
    throw ECOMIndexT("index not open!");
  }

  if (_opened_read_only)
  {
    throw ECOMIndexT("index opened with read access only!");
  }

  if (_position != _record_count * sizeof(REC))
  {
    if (lseek(_fd, 0, SEEK_END) == (off_t) - 1)
    {
      close();
      throw ECOMIndexT("could not seek in file!");
    }
  }

  if (::write(_fd, index_record, sizeof(REC)) != sizeof(REC))
  {
    close();
    throw ECOMIndexT("could not write to file!");
  }

  _record_count++;
  _position = _record_count * sizeof(REC);
}

//---------------------------------------------------------------

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
  if (!_open)
  {
    throw ECOMIndexT("index not open!");
  }

  if (_opened_read_only)
  {
    throw ECOMIndexT("index opened with read access only!");
  }

  if (index >= _record_count)
  {
    throw ECOMIndexT("index out of range!");
  }

  if (_position != index * sizeof(REC))
  {
    if (lseek(_fd, index * sizeof(REC), SEEK_SET) == (off_t) - 1)
    {
      close();
      throw ECOMIndexT("could not seek in file!");
    }
  }

  if (::write(_fd, index_record, sizeof(REC)) != sizeof(REC))
  {
    close();
    throw ECOMIndexT("could not write to file!");
  }

  _position = (index + 1) * sizeof(REC);
}
 
//---------------------------------------------------------------

#endif
