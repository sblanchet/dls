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
using namespace std;

#include "../config.h"

#include "ErrorCodes.h"

/*****************************************************************************/

void print_usage();

//extern int list_main(int, char *[]);
//extern int export_main(int, char *[]);

extern void list_data(json_object *,json_object *);
extern void fetch_data(json_object *,json_object *);

/****************************************************************************/

void parseJson(const std::string &input)
{
  enum json_type type;
  bool isJsonRpc = false;

  struct json_object* id = NULL;
  std::string method = "";
  std::string result = "";
  json_object *jobj = NULL;
  json_object *params = NULL;

  if(input.length() > 0) {
    jobj = json_tokener_parse(input.c_str()); 
    
    if(jobj) {
      json_object_object_foreach(jobj, key, val) { /*Passing through every element*/
	type = json_object_get_type(val);

	if(string(key) == "jsonrpc") {//we do not care about the version
	  isJsonRpc = true;
	}

	if(string(key) == "id") { //save the id
	  id = val;
	}

	if((string(key) == "method") && (type == json_type_string)) {
	  method = json_object_get_string(val);
	}

	if(string(key) == "params")
	  params = val;
      }
    } else {
      cerr << input << endl;
      jsonError(id,INVALID_JSON);
    }
  }

  if(!isJsonRpc) {
    jsonError(id,IS_NOT_JSON_RPC);
  }

  if(isJsonRpc && id && method.length() > 0) {
    if(method == "list") {
      list_data(id,params);
    } else {
      if(method == "fetch_data") {
	fetch_data(id,params);	
      } else {
	jsonError(id,UNKNOWN_METHOD);
      }
    }
  }
  //Speicher freigeben
  if(jobj)
    json_object_put(jobj);
}

/*****************************************************************************/

int main(int argc, char *argv[])
{
    string option;

    if (argc > 1) {
      option = argv[1];

      if (option == "help") {
	cout << "dlsjson " << PACKAGE_VERSION << " revision " << REVISION << endl;
	print_usage();
      }
    } else {
      /* ok, we are used as a cgi-program */
      cout <<  "Content-type: application/json" << endl << endl;

      std::ostringstream std_input; 
      std_input << std::cin.rdbuf();
      parseJson(std_input.str());
    }

    return 0;
}

/*****************************************************************************/

void print_usage()
{
    cout << "Usage: dlsjson options [json input]" << endl;
    cout << "dlsjson acepts jsonrpc commands via stdin" << endl;
    cout << "and sends it output to stdout." << endl;
    cout << "It is intended to be used by a webserver as cgi-script." << endl;
    cout << "Input data is expected on stdin, so use POST for sending JSON." << endl;
    cout << "Options:" << endl;
    cout << "    help - Print this help." << endl;
    //    cout << "Enter \"dls COMMAND -h\" for command-specific help." << endl;
}

/*****************************************************************************/
