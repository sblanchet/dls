//---------------------------------------------------------------
//
//  D L S _ F I L E . H P P
//
//---------------------------------------------------------------

#ifndef DLSFileHpp
#define DLSFileHpp

//---------------------------------------------------------------

#include "com_exception.hpp"

//---------------------------------------------------------------

/**
   Exception eines COMFile-Objektes
*/

class ECOMFile : public COMException
{
public:
  ECOMFile(const string &pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

/**
   Dateiobjekt, benutzt nur UNIX System-Calls
*/

class COMFile
{
public:
  COMFile();
  ~COMFile();

  //@{
  void open_read(const char *);
  void open_write(const char *);
  void open_append(const char *);
  void close();
  bool open() const;
  //@}

  void write(const char *, unsigned int);
  void seek(unsigned int);
  void read(char *, unsigned int, unsigned int * = 0);
  
  long long size() const;

private:
  int _fd;                /**< File-Descriptor */
  bool _open;             /**< Ist die Datei momentan geöffnet? */
  bool _opened_read_only; /**< Darf auch geschrieben werden? */
  long long _size;        /**< Größe der geschriebenen Datenmenge */
};

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf den Dateizustand

   \return true, wenn Datei geöffnet
*/

inline bool COMFile::open() const
{
  return _open;
}

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf Größe der bisherigen Daten

   \return Dateigröße in Bytes
*/

inline long long COMFile::size() const
{
  return _size;
}

//---------------------------------------------------------------

#endif
