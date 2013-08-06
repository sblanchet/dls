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

#ifndef DlsLibGlobalsHpp
#define DlsLibGlobalsHpp

/*****************************************************************************/

enum COMChannelType
{
    DLS_TUNKNOWN,
    DLS_TCHAR,
    DLS_TUCHAR,
    DLS_TSHORT,
    DLS_TUSHORT,
    DLS_TINT,
    DLS_TUINT,
    DLS_TLINT,
    DLS_TULINT,
    DLS_TFLT,
    DLS_TDBL
};

/*
  ---------------- Hinzufügen von Kanaltypen: ---------------------

  Editiert werden muss an folgenden Stellen:

  - Die beiden Konvertierungsfunktionen in com_globals.cpp
  - In DLSLogger::create_gen_saver()
  - In ViewChannel::fetch_chunks()

*/

/*****************************************************************************/

// Beim Erweitern bitte auch die Behandlungszweige
// in "_meta_value()" und "_ending()" anpassen!

enum DLSMetaType
{
    DLSMetaGen = 0,
    DLSMetaMean = 1,
    DLSMetaMin = 2,
    DLSMetaMax = 4
};

/*****************************************************************************/

typedef void (*DlsLoggingCallback)(const char *, void *);

void dls_set_logging_callback(DlsLoggingCallback, void *);

void dls_log(const string &);

/*****************************************************************************/

#endif
