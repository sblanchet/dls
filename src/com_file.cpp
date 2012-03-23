/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <iostream>
#include <sstream>
using namespace std;

/*****************************************************************************/

#include "com_globals.hpp"
#include "com_file.hpp"

//#define DEBUG

/*****************************************************************************/

/**
   Konstruktor
*/

COMFile::COMFile()
{
    _mode = fomClosed;
}

/*****************************************************************************/

/**
   Destruktor

   Schliesst die Datei, wenn nötig, allerdings, ohne
   Fehler auszugeben.
*/

COMFile::~COMFile()
{
    try {
        COMFile::close();
    }
    catch (ECOMFile &e) {
        // Exception schlucken
    }
}

/*****************************************************************************/

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
        err << "Could not open file";
        err << " \"" << filename << "\"";
        err << " for reading: " << strerror(errno);
        throw ECOMFile(err.str());
    }

    _mode = fomOpenRead;
    _path = filename;
}

/*****************************************************************************/

/**
   Öffnet die Datei zum Schreiben

   Wenn die Datei noch nicht existiert, wird sie angelegt.
   Existiert sie, wird sie überschrieben. Die Initialrechte
   sind momentan 644.

   \param filename Name der zu öffnenden Datei
   \throw ECOMFile Datei konnte nicht geöffnet werden
*/

void COMFile::open_read_write(const char *filename)
{
    stringstream err;
    struct stat stat_buf;

    COMFile::close();

    // Prüfen, ob die Datei bereits existiert
    if (lstat(filename, &stat_buf) == -1)
    {
        if (errno == ENOENT) // Alles ok, die Datei existiert nur noch nicht.
        {
            // Datei neu erstellen
            if ((_fd = ::open(filename, O_RDWR | O_CREAT, 0644)) == -1)
            {
                err << "Could not create file";
                err << " \"" << filename << "\"";
                err << " for writing: " << strerror(errno);
                throw ECOMFile(err.str());
            }
        }
        else // Fehler in stat
        {
            err << "Could not stat file \"" << filename
                << "\": " << strerror(errno);
            throw ECOMFile(err.str());
        }
    }
    else // Datei existiert
    {
        // Existierende Datei zum Schreiben öffnen
        if ((_fd = ::open(filename, O_RDWR)) == -1)
        {
            err << "Could not open file";
            err << " \"" << filename << "\"";
            err << " for writing: " << strerror(errno);
            throw ECOMFile(err.str());
        }
    }

    _mode = fomOpenReadWrite;
    _path = filename;
}

/*****************************************************************************/

/**
   Öffnet die Datei zum Anfügen

   Wenn die Datei noch nicht existiert, wird sie angelegt.
   Existiert sie, wird der Schreibzeiger auf das Dateiende
   gesetzt. Die Initialrechte sind momentan 644.

   \param filename Name der zu öffnenden Datei
   \throw ECOMFile Datei konnte nicht geöffnet werden
*/

void COMFile::open_read_append(const char *filename)
{
    stringstream err;
    struct stat stat_buf;

    COMFile::close();

    // Prüfen, ob die Datei bereits existiert
    if (lstat(filename, &stat_buf) == -1)
    {
        if (errno == ENOENT) // Alles ok, die Datei existiert nur noch nicht.
        {
            // Datei neu erstellen
            if ((_fd = ::open(filename, O_RDWR | O_CREAT | O_APPEND, 0644))
                == -1)
            {
                err << "Could not create file";
                err << " \"" << filename << "\"";
                err << " for appending: " << strerror(errno);
                throw ECOMFile(err.str());
            }
        }
        else // Fehler in stat
        {
            err << "Could not stat file \"" << filename
                << "\": " << strerror(errno);
            throw ECOMFile(err.str());
        }
    }
    else // Datei existiert
    {
        // Existierende Datei zum Schreiben öffnen und leeren
        if ((_fd = ::open(filename, O_RDWR | O_APPEND)) == -1)
        {
            err << "Could not open file";
            err << " \"" << filename << "\"";
            err << " for appending: " << strerror(errno);
            throw ECOMFile(err.str());
        }
    }

    _mode = fomOpenReadAppend;
    _path = filename;
}

/*****************************************************************************/

/**
   Schliesst die Datei, wenn nötig

   \throw ECOMFile Wartende Daten konnten nicht gespeichert
   werden, oder Datei war schon geschlossen.
*/

void COMFile::close()
{
    stringstream err;
    bool error = false;

    if (_mode != fomClosed) {
        if (fsync(_fd) == -1) {
            error = true;
            err << "Could not sync pending data (" << strerror(errno) << ").";
        }

        do {
            if (::close(_fd) == 0) break;

            if (errno != EINTR) {
                if (error) {
                    err << " ";
                }
                else {
                    error = true;
                }

                err << "Could not close file (" << strerror(errno) << ").";
            }
        }
        while (errno == EINTR);

        _mode = fomClosed;

        if (error) throw ECOMFile(err.str());
    }
}

/*****************************************************************************/

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

    if (_mode == fomClosed)
    {
        throw ECOMFile("File not open.");
    }

    if (_mode == fomOpenRead)
    {
        throw ECOMFile("File opened read only.");
    }

    if (_mode == fomOpenReadAppend)
    {
        throw ECOMFile("File opened for appending. Use append().");
    }

    while (written < length) // Solange, wie noch
        // nicht alles geschrieben
    {
        write_ret = ::write(_fd, buffer + written, length - written);

        if (write_ret > -1)
        {
            written += write_ret;
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
                err << "malicious buffer pointer (" << strerror(errno) << ").";
            }
            else // Anderer, Fataler Fehler
            {
                error = true;
                err << strerror(errno);

                try
                {
                    COMFile::close();
                }
                catch (ECOMFile &e)
                {
                    err << " - closing: " << e.msg;
                }
            }

            if (error)
            {
                throw ECOMFile(err.str());
            }
        }
    }
}

/*****************************************************************************/

/**
   Schreibt Daten an das Ende der Datei

   Setzt so viele Schreibbefehle ab, wie nötig sind,
   um die gesamten Daten zu speichern. Wenn das Schreiben
   durch ein Signal unterbrochen wird, wird es danach
   fortgesetzt.

   \param buffer Zeiger auf den Datenpuffer
   \param length Länge der zu schreibenden Daten in Bytes
   \throw ECOMFile Daten konnten nicht vollständig
   gespeichert werden.
*/

void COMFile::append(const char *buffer, unsigned int length)
{
    int write_ret;
    unsigned int written = 0;
    bool error = false;
    stringstream err;

    if (_mode == fomClosed)
    {
        throw ECOMFile("File not open.");
    }

    if (_mode == fomOpenRead)
    {
        throw ECOMFile("File opened read only.");
    }

    if (_mode == fomOpenReadWrite)
    {
        throw ECOMFile("File opened for writing. Use write()!");
    }

    while (written < length) // Solange noch nicht alles geschrieben
    {
        write_ret = ::write(_fd, buffer + written, length - written);

        if (write_ret > -1)
        {
            written += write_ret;
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
                err << "malicious buffer pointer (" << strerror(errno) << ").";
            }
            else // Anderer, Fataler Fehler
            {
                error = true;
                err << strerror(errno);

                try
                {
                    COMFile::close();
                }
                catch (ECOMFile &e)
                {
                    err << " - closing: " << e.msg;
                }
            }

            if (error)
            {
                throw ECOMFile(err.str());
            }
        }
    }

#ifdef DEBUG
    msg() << "Appended " << written << " byte(s).";
    log(DLSDebug);
#endif
}

/*****************************************************************************/

/**
   Springt an einen bestimmten Punkt in der Datei

   \param position Position, an die gesprungen werden soll
   \throw ECOMFile Position nicht erreichbar
*/

void COMFile::seek(unsigned int position)
{
    stringstream err;

    if (_mode == fomClosed)
    {
        throw ECOMFile("File not open.");
    }

    if (lseek(_fd, position, SEEK_SET) == (off_t) - 1)
    {
        err << "Position could not be reached! Seek: " << strerror(errno);
        throw ECOMFile(err.str());
    }
}

/*****************************************************************************/

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

    if (_mode == fomClosed)
    {
        throw ECOMFile("File not open.");
    }

    if (length > 0)
    {
        while (1)
        {
            if ((read_ret = ::read(_fd, target, length)) == -1)
            {
                if (errno != EINTR)
                {
                    err << "Read error: " << strerror(errno);
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

/*****************************************************************************/

/**
   Berechnet die Dateigröße mit einem Sprung ans Ende

   \return Dateigröße in Bytes
*/

uint64_t COMFile::calc_size()
{
    stringstream err;
    uint64_t size;

    if ((size = lseek(_fd, 0, SEEK_END)) == (off_t) - 1)
    {
        err << "Could not determine file size! Seek: " << strerror(errno);

        try
        {
            COMFile::close();
        }
        catch (ECOMFile &e)
        {
            err << " - Close: " << e.msg;
        }

        throw ECOMFile(err.str());
    }

    return size;
}

/*****************************************************************************/

