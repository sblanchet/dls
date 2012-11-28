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

#ifndef DLSJobChannelHpp
#define DLSJobChannelHpp

/*****************************************************************************/

#include <string>
using namespace std;

/*****************************************************************************/

#include "com_exception.hpp"
#include "lib_globals.hpp"

/*****************************************************************************/

class COMXMLTag;

/*****************************************************************************/

/**
   Exception eines COMChannelPreset-Objektes
*/

class ECOMChannelPreset : public COMException
{
public:
    ECOMChannelPreset(const string &pmsg) : COMException(pmsg) {};
};

/*****************************************************************************/

/**
   Kanalvorgabe

   Enthält Kanalname, Abtastrate, Blockgröße, Meta-Vorgaben
   und das Format, in dem die Daten gespeichert werden sollen.
*/

class COMChannelPreset
{
public:
    COMChannelPreset();
    ~COMChannelPreset();

    bool operator!=(const COMChannelPreset &) const;

    void read_from_tag(const COMXMLTag *);
    void write_to_tag(COMXMLTag *) const;

    void clear();

    string name;                   /**< Kanalname */
    double sample_frequency; /**< Abtastrate, mit der aufgezeichnet
                                      werden soll */
    unsigned int block_size;       /**< Blockgröße, mit der aufgezeichnet
                                      werden soll */
    unsigned int meta_mask;        /**< Bitmaske mit den aufzuzeichnenden
                                      Meta-Typen */
    unsigned int meta_reduction;   /**< Meta-Untersetzung */
    int format_index;              /**< Index des Formates zum Speichern
                                      der Daten */
    unsigned int mdct_block_size;  /**< Blockgröße für MDCT */
    double accuracy;               /**< Genauigkeit von verlustbehafteten
                                      Kompressionen */

    COMChannelType type;          /**< Datentyp des Kanals (nur für
                                     MDCT-Prüfung) */
};

/*****************************************************************************/

#endif
