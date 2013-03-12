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

#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>

#include "com_globals.hpp"
#include "com_exception.hpp"

/*****************************************************************************/

const char *dls_format_strings[DLS_FORMAT_COUNT] =
{
    "ZLib/Base64",
    "MDCT/ZLib/Base64",
    "Quant/ZLib/Base64"
};

bool is_daemon = false;

/*****************************************************************************/

COMChannelType dls_str_to_channel_type(const string &str)
{
    if (str == "TCHAR")  return TCHAR;
    if (str == "TUCHAR") return TUCHAR;
    if (str == "TSHORT") return TSHORT;
    if (str == "TUSHORT")return TUSHORT;
    if (str == "TINT")   return TINT;
    if (str == "TUINT")  return TUINT;
    if (str == "TLINT")  return TLINT;
    if (str == "TULINT") return TULINT;
    if (str == "TFLT")   return TFLT;
    if (str == "TDBL")   return TDBL;

    return TUNKNOWN;
}

/*****************************************************************************/

const char *dls_channel_type_to_str(COMChannelType type)
{
    switch (type) {
        case TCHAR:  return "TCHAR";
        case TUCHAR: return "TUCHAR";
        case TSHORT: return "TSHORT";
        case TUSHORT: return "TUSHORT";
        case TINT:   return "TINT";
        case TUINT:  return "TUINT";
        case TLINT:  return "TLINT";
        case TULINT: return "TULINT";
        case TFLT:   return "TFLT";
        case TDBL:   return "TDBL";
        default: return "-";
    }
}

/*****************************************************************************/

string dls_meta_type_str(DLSMetaType meta_type)
{
    switch (meta_type) {
        case DLSMetaGen: return "gen";
        case DLSMetaMean: return "mean";
        case DLSMetaMin: return "min";
        case DLSMetaMax: return "max";
        default: return "???";
    }
}

/*****************************************************************************/

string convert_to_bin(const void *data,
                      unsigned int bytes,
                      int bytes_in_row)
{
    unsigned int i, row;
    unsigned char byte;
    string ret;
    bool reverse = bytes_in_row < 0;

    if (bytes_in_row < 0) bytes_in_row *= -1;

    i = 0;
    row = 0;

    while (i < bytes)
    {
        if (i % bytes_in_row == 0 && i > 0)
        {
            row++;
            ret += "\n";
        }

        if (reverse)
        {
            byte = *((unsigned char *) data
                     + row * bytes_in_row
                     + bytes_in_row
                     - (i % bytes_in_row) - 1);
        }
        else
        {
            byte = *((unsigned char *) data + i);
        }

        ret += byte & 128 ? "1" : "0";
        ret += byte &  64 ? "1" : "0";
        ret += byte &  32 ? "1" : "0";
        ret += byte &  16 ? "1" : "0";
        ret += byte &   8 ? "1" : "0";
        ret += byte &   4 ? "1" : "0";
        ret += byte &   2 ? "1" : "0";
        ret += byte &   1 ? "1" : "0";

        i++;

        if (i % bytes_in_row != 0) ret += " ";
    }

    return ret;
}

/*****************************************************************************/

bool operator<(const COMRealChannel &a, const COMRealChannel &b) {
    return a.name < b.name;
}

/*****************************************************************************/
