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

#ifndef COMZLibHpp
#define COMZLibHpp

/*****************************************************************************/

#include "com_exception.hpp"

/*****************************************************************************/

/**
   Exception eines COMZlib-Objektes
*/

class ECOMZLib : public COMException
{
public:
    ECOMZLib(const string &pmsg) : COMException(pmsg) {};
};

/*****************************************************************************/

/**
   ZLib-Kompressionsklasse

   Stellt alle nötigen Funktionen bereit, um Daten mit der
   ZLib zu komprimieren und zu dekomprimieren.
*/

class COMZLib
{
public:
    COMZLib();
    ~COMZLib();

    void compress(const char *, unsigned int);
    void uncompress(const char *, unsigned int, unsigned int);

    const char *output() const;
    unsigned int output_size() const;

    void free();

private:
    char *_out_buf;         /**< Ausgabepuffer */
    unsigned int _out_size; /**< Länge der datem im Ausgabepuffer */
};

/*****************************************************************************/

/**
   Ermöglicht lesenden Zugriff auf den Ausgabepuffer

   \return Konstanter Zeiger auf den Ausgabepuffer
*/

inline const char *COMZLib::output() const
{
    return _out_buf;
}

/*****************************************************************************/

/**
   Liefert die Länge der ausgegebenen Daten

   \return Anzahl Zeichen im Ausgabepuffer
*/

inline unsigned int COMZLib::output_size() const
{
    return _out_size;
}

/*****************************************************************************/

#endif
