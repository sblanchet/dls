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

#ifndef COMBase64Hpp
#define COMBase64Hpp

/*****************************************************************************/

#include "com_globals.hpp"
#include "com_exception.hpp"

/*****************************************************************************/

/**
   Exception eines COMBase64-Objektes
*/

class ECOMBase64 : public COMException
{
public:
    ECOMBase64(const string &pmsg) : COMException(pmsg) {};
};

/*****************************************************************************/

/**
   Base64 Kodierung/Dekodierung

   Ermöglicht Base64 Kodierung und Dekodierung und speichert die
   Ergebnisse in einem internen Puffer.
*/

class COMBase64
{
public:
    COMBase64();
    ~COMBase64();

    void encode(const char *, unsigned int);
    void decode(const char *, unsigned int);

    const char *output() const;
    unsigned int output_size() const;

    void free();

private:
    char *_out_buf;         /**< Zeiger auf den Ergebnispuffer */
    unsigned int _out_size; /**< Länge des Ergebnispuffers */
};

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf den Ergebnispuffer

   \return Konstanter Zeiger auf den Ergebnispuffer
*/

inline const char *COMBase64::output() const
{
    return _out_buf;
}

/*****************************************************************************/

/**
   Ermittelt die Länge der Daten im Ergebnispuffer

   \return Länge in Bytes
*/

inline unsigned int COMBase64::output_size() const
{
    return _out_size;
}

/*****************************************************************************/

#endif
