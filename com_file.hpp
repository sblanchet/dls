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
  COMFileOpenMode _mode;  /**< �ffnungsmodus */
  string _path;           /**< Pfad der ge�ffneten Datei */
};

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf den Dateizustand

   \return true, wenn Datei ge�ffnet
*/

inline bool COMFile::open() const
{
  return _mode != fomClosed;
}

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf den �ffnungszustand

   \return Zustand
*/

inline COMFileOpenMode COMFile::open_mode() const
{
  return _mode;
}

//---------------------------------------------------------------

/**
   Gibt den Pfad der ge�ffneten date zur�ck

   \return Pfad
*/

inline string COMFile::path() const
{
  return _path;
}

//---------------------------------------------------------------

#endif
