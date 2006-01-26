//---------------------------------------------------------------
//
//  C O M _ F I L E . C P P
//
//---------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sstream>
using namespace std;

#include "com_file.hpp"

//---------------------------------------------------------------

/**
   Konstruktor
*/

COMFile::COMFile()
{
  _open = false;
  _size = 0;
}

//---------------------------------------------------------------

/**
   Destruktor

   Schliesst die Datei, wenn nötig, allerdings, ohne
   Fehler auszugeben.
*/

COMFile::~COMFile()
{
  try
  {
    COMFile::close();
  }
  catch (...)
  {
  }
}

//---------------------------------------------------------------

/**
   Öffnet die Datei zum Lesen

   \param filename Name der zu öffnenden Datei
   \throw ECOMFile Datei konnte nicht geöffnet werden
*/

void COMFile::open_read(const char *filename)
{
  stringstream err;

  COMFile::close();

  if ((_fd = ::open(filename, O_RDONLY)) == -1)
  {
    switch (errno)
    {
      case EACCES: err << "no permission."; break;
      default: err << "unknown error"; break;
    }

    throw ECOMFile(err.str());
  }

  _open = true;
  _opened_read_only = true;
  
  if ((_size = lseek(_fd, 0, SEEK_END)) == (off_t) - 1)
  {
    try
    {
      COMFile::close();
    }
    catch (...)
    {
    }

    err << "could not determine file size!";
    throw ECOMFile(err.str());
  }

  if (lseek(_fd, 0, SEEK_SET) == (off_t) - 1)
  {
    try
    {
      COMFile::close();
    }
    catch (...)
    {
    }

    err << "could not seek back to beginning of file!";
    throw ECOMFile(err.str());
  }
}

//---------------------------------------------------------------

/**
   Öffnet die Datei zum Schreiben

   Wenn die Datei noch nicht existiert, wird sie angelegt.
   Existiert sie, wird sie überschrieben. Die Initialrechte
   sind momentan 644.

   \param filename Name der zu öffnenden Datei
   \throw ECOMFile Datei konnte nicht geöffnet werden
*/

void COMFile::open_write(const char *filename)
{
  stringstream err;

  COMFile::close();

  if ((_fd = ::open(filename, O_WRONLY | O_CREAT, 0644)) == -1)
  {
    switch (errno)
    {
      case EACCES: err << "no access to directory."; break;
      default: err << "unknown error"; break;
    }

    throw ECOMFile(err.str());
  }

  _open = true;
  _opened_read_only = false;
  _size = 0;
}

//---------------------------------------------------------------

/**
   Öffnet die Datei zum Anfügen

   Wenn die Datei noch nicht existiert, wird sie angelegt.
   Existiert sie, wird der Schreibzeiger auf das Dateiende
   gesetzt. Die Initialrechte sind momentan 644.

   \param filename Name der zu öffnenden Datei
   \throw ECOMFile Datei konnte nicht geöffnet werden
*/

void COMFile::open_append(const char *filename)
{
  stringstream err;

  COMFile::close();

  if ((_fd = ::open(filename, O_WRONLY | O_CREAT, 0644)) == -1)
  {
    switch (errno)
    {
      case EACCES: err << "no access to directory."; break;
      default: err << "unknown error"; break;
    }

    throw ECOMFile(err.str());
  }

  _open = true;
  _opened_read_only = false;

  if ((_size = lseek(_fd, 0, SEEK_END)) == (off_t) - 1)
  {
    try
    {
      COMFile::close();
    }
    catch (...)
    {
    }

    err << "could not determine file size!";
    throw ECOMFile(err.str());
  }
}

//---------------------------------------------------------------

/**
   Schliesst die Datei, wenn nötig

   \throw ECOMFile Wartende Daten konnten nicht gespeichert
                   werden, oder Datei war schon geschlossen.
*/

void COMFile::close()
{
  int sync_ret, close_ret;
  stringstream err;
  bool error = false;

  if (_open)
  {
    sync_ret = fsync(_fd);

    if (sync_ret == -1 && (errno == EBADF || errno == EIO))
    {
      error = true;
      err << "could not sync pending data.";
    }

    do
    {
      close_ret = ::close(_fd);

      if (close_ret == 0) break;

      if (errno == EBADF || errno == EIO)
      {
        if (error)
        {
          err << " ";
        }
        else
        {
          error = true;
        }

        err << "could not close file.";
      }
    }
    while (errno == EINTR);

    _open = false;
    _size = 0;

    if (error)
    {
      throw ECOMFile(err.str());
    }
  }
}

//---------------------------------------------------------------

/**
   Schreibt Daten in die Datei

   Setzt so viele Schreibbefehle ab, wie nötig sind,
   um die gesamten Daten zu speichern. Wenn das Schreiben
   durch ein Signal unterbrochen wird, wird es danach
   fortgesetzt.

   \param buffer Zeiger auf den Datenpuffer
   \param length Länge der zu schreibenden Daten in Bytes
   \throw ECOMFile Daten konnten nicht vollständig
                   gespeichert werden.
*/

void COMFile::write(const char *buffer, unsigned int length)
{
  int write_ret;
  unsigned int written = 0;
  bool error = false;
  stringstream err;

  if (!_open)
  {
    throw ECOMFile("file not open.");
  }

  if (_opened_read_only)
  {
    throw ECOMFile("file opened read only.");
  }

  while (written < length) // Solange, wie noch
                           // nicht alles geschrieben
  {
    write_ret = ::write(_fd, buffer + written, length - written);

    if (write_ret > -1)
    {
      written += write_ret;
      _size += write_ret;
    }
    else // Ein Fehler ist aufgetreten
    {
      if (errno == EINTR) // Signal empfangen
      {
        // Einfach nochmal versuchen
      }
      else if (errno == EFAULT) // Speicherfehler
      {
        error = true;
        err << "malicious buffer pointer.";
      }
      else // Anderer, Fataler Fehler
      {
        error = true;
        err << "unknown write error.";
        
        try
        {
          COMFile::close();
        }
        catch (ECOMFile &e)
        {
          err << " " << e.msg;
        }
      }

      if (error)
      {
        throw ECOMFile(err.str());
      }
    }
  }
}

//---------------------------------------------------------------

/**
   Springt an einen bestimmten Punkt in der Datei

   \param position Position, an die gesprungen werden soll
   \throw ECOMFile Position nicht erreichbar
*/

void COMFile::seek(unsigned int position)
{
  stringstream err;

  if (!_open)
  {
    throw ECOMFile("file not open.");
  }

  if (!_opened_read_only)
  {
    throw ECOMFile("seek supported in read only mode only.");
  }
  
  if (position > _size)
  {
    err << "position out of range!";
    throw ECOMFile(err.str());
  }

  if (lseek(_fd, position, SEEK_SET) == (off_t) - 1)
  {
    err << "position could not be reached!";
    throw ECOMFile(err.str());
  }
}

//---------------------------------------------------------------

/**
   Liest Daten aus der Datei

   \param target Puffer, in dem die gelesenen Daten gespeichert werden
   \param length Länge der zu Lesenden Daten
   \param bytes_read Zeiger auf einen Integer, in den die Anzahl der
          gelesenen Bytes gespeichert wird. Ist dieser 0, wird dies
          ausgelassen.
   \throw ECOMFile Position nicht erreichbar
*/

void COMFile::read(char *target, unsigned int length, unsigned int *bytes_read)
{
  stringstream err;
  unsigned int bytes = 0;
  int read_ret;

  if (!_open)
  {
    throw ECOMFile("file not open.");
  }

  if (!_opened_read_only)
  {
    throw ECOMFile("file not opened in read mode!");
  }
  
  if (length > 0)
  {
    while (1)
    {
      if ((read_ret = ::read(_fd, target, length)) == -1)
      {
        if (errno != EINTR)
        {
          err << "read error (errno = " << errno << ")";
          throw ECOMFile(err.str());
        }
      }
      else
      {
        bytes = read_ret;
        break;
      }
    }
  }

  if (bytes_read) *bytes_read = bytes;

  return;
}

//---------------------------------------------------------------

