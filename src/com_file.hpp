/******************************************************************************
 *
 *  $Id$
 *
 *  This file is part of the Data Logging Service (DLS).
 *
 *  DLS is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  DLS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with DLS. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef DLSFileHpp
#define DLSFileHpp

/*****************************************************************************/

#include <stdint.h>

#include "com_exception.hpp"

/*****************************************************************************/

/**
   Exception eines COMFile-Objektes
*/

class ECOMFile : public COMException
{
public:
    ECOMFile(const string &pmsg) : COMException(pmsg) {};
};

/*****************************************************************************/

enum COMFileOpenMode
{
    fomClosed, fomOpenRead, fomOpenReadWrite, fomOpenReadAppend
};

/*****************************************************************************/

/**
   Dateiobjekt, benutzt nur UNIX System-Calls
*/

class COMFile
{
public:
    COMFile();
    ~COMFile();

    enum OpenFlag {
        Text,
        Binary
    };

    //@{
    void open_read(const char *, OpenFlag = Text);
    void open_read_write(const char *, OpenFlag = Text);
    void open_read_append(const char *, OpenFlag = Text);
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

    uint64_t calc_size();

private:
    int _fd;                /**< File-Descriptor */
    COMFileOpenMode _mode;  /**< �ffnungsmodus */
    string _path;           /**< Pfad der ge�ffneten Datei */
};

/*****************************************************************************/

/**
   Erm�glicht Lesezugriff auf den Dateizustand

   \return true, wenn Datei ge�ffnet
*/

inline bool COMFile::open() const
{
    return _mode != fomClosed;
}

/*****************************************************************************/

/**
   Erm�glicht Lesezugriff auf den �ffnungszustand

   \return Zustand
*/

inline COMFileOpenMode COMFile::open_mode() const
{
    return _mode;
}

/*****************************************************************************/

/**
   Gibt den Pfad der ge�ffneten date zur�ck

   \return Pfad
*/

inline string COMFile::path() const
{
    return _path;
}

/*****************************************************************************/

#endif
