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

#include <signal.h>
#include <stdlib.h>
#include "json-c/json.h"

#include <iostream>
#include <sstream>
#include "ErrorCodes.h"

using namespace std;

/****************************************************************************/

void jsonError(json_object *id,int code) {

  //RPC-Header
  json_object * jresponse = json_object_new_object();
  json_object_object_add(jresponse,"jsonrpc",json_object_new_string("2.0"));
  
 if(id) {
   json_object_object_add(jresponse,"id",id);
 }
 json_object_object_add(jresponse,"error",json_object_new_int(code));
 
 cout << json_object_to_json_string(jresponse) << endl;

 json_object_put(jresponse);

 exit(0);
}

