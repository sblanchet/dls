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

#include <unistd.h>
#include <stdlib.h> // getenv()
#include "json-c/json.h"

#include <iostream>
#include <iomanip>
using namespace std;

#include "lib/LibDLS/Dir.h"
using namespace LibDLS;

#include "ErrorCodes.h"

/*****************************************************************************/

void list_jobs(json_object *id,Directory *dir);
void list_channels(json_object *id,Job *job);

/*****************************************************************************/

/* request 
   { "jsonrpc": "2.0",
     "id": 1,
     "method": "list",
     "params": {
	 "path": "/vol/data/dls_data",
	 "job": 100
     }    
   }

   path may be empty: DLS_DIR is used
   job may be empty: jobs are listed
   otherwise channels are listed.
*/

void list_data(json_object *id,json_object *params)
{
  string dls_dir_path;
  unsigned int job_id = 0;

    Directory dls_dir;
    Job *job;

    char *env;

    if ((env = getenv("DLS_DIR"))) {
        dls_dir_path = env;
    }

    if(params) {
      json_object_object_foreach(params, key, val) { /*Passing through every element*/

	if(string(key) == "path") {
	  dls_dir_path = string(json_object_get_string(val));
	  //overwrite env
	}
	
	if(string(key) == "job") {
	  job_id = json_object_get_int(val); //FIXME checking if val is an int
	}
      }
    }

    try {
        dls_dir.set_uri(dls_dir_path);
    }
    catch (DirectoryException &e) {
      jsonError(id,DLS_PARSING_URI_FAILED);
    }

    try {
        dls_dir.import();
    }
    catch (DirectoryException &e) {
      jsonError(id,DLS_IMPORT_FAILED);
    }

    if (!job_id) {
        list_jobs(id,&dls_dir);
    }
    else {
        if (!(job = dls_dir.find_job(job_id))) {
	  jsonError(id,DLS_NO_SUCH_JOB);
        }
        list_channels(id,job);
    }
}

/*****************************************************************************/

void list_jobs(json_object *id,Directory *dir)
{
    list<Job *>::iterator job_i;

    //RPC-Header
    json_object * jresponse = json_object_new_object();
    json_object_object_add(jresponse,"jsonrpc",json_object_new_string("2.0"));
    
    if(id) { //place id in response
      json_object_object_add(jresponse,"id",id);
    }

    /*jobarray*/
    json_object *jJobArray = json_object_new_array();
    json_object_object_add(jresponse,"jobs", jJobArray);

    for (job_i = dir->jobs().begin(); job_i != dir->jobs().end(); job_i++) {
      json_object * jobj = json_object_new_object();
      json_object_object_add(jobj,"job_id",json_object_new_int((*job_i)->preset().id()));
      json_object_object_add(jobj,"description",json_object_new_string((*job_i)->preset().description().c_str()));
      json_object_array_add(jJobArray,jobj);
    }

    cout << json_object_to_json_string(jresponse) << endl;

    //Speicher freigeben
    json_object_put(jresponse);

    exit(0);
}

/*****************************************************************************/

void list_channels(json_object *id,Job *job)
{

    list<Channel>::iterator channel_i;

    try {
        job->fetch_channels();
    }
    catch (Exception &e) {
      jsonError(id,DLS_FETCH_CHANNELS_FAILED);
    }

    //RPC-Header
    json_object * jresponse = json_object_new_object();
    json_object_object_add(jresponse,"jsonrpc",json_object_new_string("2.0"));
    
    if(id) { //place id in response
      json_object_object_add(jresponse,"id",id);
    }

    json_object_object_add(jresponse,"job_id",json_object_new_int(job->preset().id()));

    /*Channelarray*/
    json_object *jChannelArray = json_object_new_array();
    json_object_object_add(jresponse,"channels", jChannelArray);


    for (channel_i = job->channels().begin();
         channel_i != job->channels().end();
         channel_i++) {
      json_object * jobj = json_object_new_object();
      json_object_object_add(jobj,"path",json_object_new_string(channel_i->name().c_str()));
      json_object_object_add(jobj,"index",json_object_new_int(channel_i->dir_index()));
      json_object_array_add(jChannelArray,jobj);
    }

    cout << json_object_to_json_string(jresponse) << endl;
    //Speicher freigeben
    json_object_put(jresponse);

    exit(0);

}

