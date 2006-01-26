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

enum COMFileOpenMode
{
  fomClosed, fomOpenRead, fomOpenReadWrite, fomOpenReadAppend
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
  void open_read_write(const char *);
  void open_read_append(const char *);
  void close();
  //@}

  //@{
  bool open() const;
  COMFileOpenMode open_mode() const;
  string path() const;
  //@}

  //@{
  void read(char *, unsigned int, unsigned int * = 0);
  void write(const char *, unsigned int);
  void append(const char *, unsigned int);
  void seek(unsigned int);
  //@}

  long long calc_size();

private:
  int _fd;                /**< File-Descriptor */
  COMFileOpenMode _mode;  /**< Öffnungsmodus */
  string _path;           /**< Pfad der geöffneten Datei */
};

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf den Dateizustand

   \return true, wenn Datei geöffnet
*/

inline bool COMFile::open() const
{
  return _mode != fomClosed;
}

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf den Öffnungszustand

   \return Zustand
*/

inline COMFileOpenMode COMFile::open_mode() const
{
  return _mode;
}

//---------------------------------------------------------------

/**
   Gibt den Pfad der geöffneten date zurück

   \return Pfad
*/

inline string COMFile::path() const
{
  return _path;
}

//---------------------------------------------------------------

#endif
