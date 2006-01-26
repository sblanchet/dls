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
  bool _open;             /**< Ist die Datei momentan ge�ffnet? */
  bool _opened_read_only; /**< Darf auch geschrieben werden? */
  long long _size;        /**< Gr��e der geschriebenen Datenmenge */
};

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf den Dateizustand

   \return true, wenn Datei ge�ffnet
*/

inline bool COMFile::open() const
{
  return _open;
}

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf Gr��e der bisherigen Daten

   \return Dateigr��e in Bytes
*/

inline long long COMFile::size() const
{
  return _size;
}

//---------------------------------------------------------------

#endif
