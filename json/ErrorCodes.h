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

#ifndef ERRORCODESH
#define ERRORCODESH

#include "json-c/json.h"

/* Errorcodes */

/* JSON */
#define INVALID_JSON 1
#define IS_NOT_JSON_RPC 2
#define UNKNOWN_METHOD 3
#define WRONG_TYPE 4

/* DLS */
#define DLS_PARSING_URI_FAILED 100
#define DLS_IMPORT_FAILED 101
#define DLS_NO_SUCH_JOB 102
#define DLS_FETCH_CHANNELS_FAILED 103
#define DLS_JOBID_MISSING 104
#define DLS_NO_SUCH_CHANNEL 105
#define DLS_CHANNELNAME_NOT_UNIQUE 106
#define DLS_INVALID_TIME_RANGE 107
#define DLS_FETCH_CHUNKS_FAILED 108
#define DLS_FETCH_DATA_FAILED 109



void jsonError(json_object *id,int code);

#endif
