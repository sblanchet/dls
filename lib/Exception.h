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

#ifndef LibDLSExceptionH
#define LibDLSExceptionH

/*****************************************************************************/

#include <string>
using namespace std;

/*****************************************************************************/

namespace LibDLS {

/*****************************************************************************/

/**
   Basisklasse aller Exceptions der DLS-Klassen
*/

class Exception
{
public:

    /**
       Konstruktor

       \param pmsg Nachricht der zu werfenden Exception
    */

    Exception(const string &pmsg) {msg = pmsg;};

    string msg; /**< Nachricht der Exception */

private:

    /**
       Standardkonstruktor

       Privat, da er nicht aufgerufen werden soll.
    */

    Exception();
};

/*****************************************************************************/

} // namespace

/*****************************************************************************/

#endif
