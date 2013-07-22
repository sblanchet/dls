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

#include <iostream>
using namespace std;

#include "lib_globals.hpp"

/*****************************************************************************/

DlsLoggingCallback loggingCallback = NULL;
void *loggingCallbackData;

/*****************************************************************************/

void dls_set_logging_callback(DlsLoggingCallback cb, void *data)
{
	loggingCallback = cb;
	loggingCallbackData = data;
}

/*****************************************************************************/

void dls_log(const string &msg)
{
	if (loggingCallback) {
		loggingCallback(msg.c_str(), loggingCallbackData);
	}
	else {
		cerr << msg << endl;
	}
}

/*****************************************************************************/