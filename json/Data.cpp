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
#include <algorithm> // swap()
using namespace std;

#include "lib/LibDLS/Dir.h"

using namespace LibDLS;

#include "ErrorCodes.h"

int dataCallback(LibDLS::Data *, void *);

//helper to concat blocks
struct DataBlockInfo {
  json_object *jData;
  json_object *jBlockArray;  //hier werden die Werte angehängt
  Time curTime;
  Time dt;
  MetaType metaType;
  unsigned int metaLevel;
};

/*****************************************************************************/

void fetch_channel_data(json_object *id,Job *job,json_object *channel_names,Time start_time,Time end_time,int min_values);

/*****************************************************************************/

/* request 

{ "jsonrpc": "2.0",
     "id": 1,
     "method": "fetch_data",
     "params": {
	 "path": "/vol/data/dls_data",
	 "job": 100,
	 "start":12345, 
	 "end":12346,
	 "min_values":500,
	 "channels":["/groundDistance",
		     "/pressureGas"
		    ]
     }    
}	       

   path: mandatory
   job: mandatory
   start,end (int64): Interval Time (Epoch usecs)
   min_values: see Channel.cpp

   Todo: end might be the string "now"
*/

void fetch_data(json_object *id,json_object *params)
   {
     string dls_dir_path;
     unsigned int job_id = 0;

     Directory dls_dir;
     Job *job;
     json_object *channels = NULL;

     Time start_time;
     Time end_time;

     int min_values=0; 

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

	 if(string(key) == "start") {
	   start_time = json_object_get_int64(val);
	 }

	 if(string(key) == "end") {
	   end_time = json_object_get_int64(val);
	 }

	 if(string(key) == "min_values") {
	   min_values = json_object_get_int(val);
	 }
	 if(string(key) == "channels") {
	   channels = val;
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
       jsonError(id,DLS_JOBID_MISSING);
     }
     else {
       if (!(job = dls_dir.find_job(job_id))) {
	 jsonError(id,DLS_NO_SUCH_JOB);
       }
       fetch_channel_data(id,job,channels,start_time,end_time,min_values);
     }
   }

/*****************************************************************************/


void fetch_channel_data(json_object *id,Job *job,json_object *channel_names,Time start_time,Time end_time,int min_values)
{

  Time channels_start, channels_end;

  list<Channel *> channels;
  list<Channel *>::iterator channel_i;

  try {
    job->fetch_channels();
  }
  catch (Exception &e) {
    jsonError(id,DLS_FETCH_CHANNELS_FAILED);
  }

  enum json_type channels_type = json_object_get_type(channel_names);

  if(channels_type != json_type_array) {
    jsonError(id,WRONG_TYPE);
  }

  //build list of channels
  for(int i=0; i < json_object_array_length(channel_names); i++) {
    json_object *jchannel = json_object_array_get_idx(channel_names, i);

    set<Channel *> chs = job->find_channels_by_name(json_object_get_string(jchannel));

    if (chs.empty()) {
      jsonError(id,DLS_NO_SUCH_CHANNEL);
    }

    if (chs.size() > 1) {
      jsonError(id,DLS_CHANNELNAME_NOT_UNIQUE);
    }

    set<Channel *>::iterator ch_i = chs.begin();
    channels.push_back(*ch_i);
  }


  //prepare start and end-Time
  for (channel_i = channels.begin();channel_i != channels.end();channel_i++) {

    try {
      (*channel_i)->fetch_chunks();
    } catch (ChannelException &e) {
      jsonError(id,DLS_FETCH_CHUNKS_FAILED);
    }

    if (!(*channel_i)->start().is_null()) {
      if (channels_start.is_null()) {
	channels_start = (*channel_i)->start();
      } else if ((*channel_i)->start() < channels_start) {
	channels_start = (*channel_i)->start();
      }
    }

    if (!(*channel_i)->end().is_null()) {
      if (channels_end.is_null()) {
	channels_end = (*channel_i)->end();
      } else if ((*channel_i)->end() > channels_end) {
	channels_end = (*channel_i)->end();
      }
    }
  }

  if (start_time.is_null()) start_time = channels_start;
  if (end_time.is_null()) end_time = channels_end;

  if (start_time < channels_start) {
    start_time = channels_start;
  }
  if (end_time > channels_end) {
    end_time = channels_end;
  }

#if 0
   cerr << "Start time: " << start_time.to_uint64() << endl
	<< "  End time: " << end_time.to_uint64() << endl
	<< "min values: " << min_values << endl;
#endif

#if 0
   cerr << "Start time: " << start_time.to_real_time() << endl
         << "  End time: " << end_time.to_real_time() << endl
         << "  duration: " << start_time.diff_str_to(end_time) << endl;
#endif

  if (start_time >= end_time) {
    jsonError(id,DLS_INVALID_TIME_RANGE);
  }

  //RPC-Header (response)
  json_object *jresponse = json_object_new_object();
  json_object_object_add(jresponse,"jsonrpc",json_object_new_string("2.0"));
    
  if(id) { //place id in response
    json_object_object_add(jresponse,"id",id);
  }

  json_object_object_add(jresponse,"job_id",json_object_new_int(job->preset().id()));

  /*Channelarray*/
  json_object *jChannelArray = json_object_new_array();
  json_object_object_add(jresponse,"channels", jChannelArray);


  for (channel_i = channels.begin();channel_i != channels.end();channel_i++) {


    json_object * jobj = json_object_new_object();
    json_object_array_add(jChannelArray,jobj);
    json_object_object_add(jobj,"path",json_object_new_string((*channel_i)->name().c_str()));

    json_object *jDataArray = json_object_new_array();
    json_object_object_add(jobj,"data",jDataArray);

    DataBlockInfo dataBlockInfo;
    dataBlockInfo.jData = jDataArray;
    dataBlockInfo.jBlockArray = NULL;

    try {
      (*channel_i)->fetch_data(start_time, end_time,
			  min_values, dataCallback,&dataBlockInfo,1);
    } catch (ChannelException &e) {
      jsonError(id,DLS_FETCH_DATA_FAILED);
    }


  }

#if 0
  cout << "end callbacks" << endl;
#endif

  cout << json_object_to_json_string(jresponse) << endl;
  //Speicher freigeben
  json_object_put(jresponse);

  exit(0);

}

/****************************************************************************/

int dataCallback(LibDLS::Data *data, void *cb_data)
{

  DataBlockInfo *dataBlockInfo = (DataBlockInfo *)cb_data;

  json_object *jDataArray = dataBlockInfo->jData;

  Time _diff;
  _diff = dataBlockInfo->curTime /* + dataBlockInfo->dt */ - data->start_time();
  //FIXME: sollte es nicht +dt sein, Hm
#if 0
  cerr << "diff: " << _diff.to_int64() << endl;
#endif

  //check if we can reuse and concat data to the previous block, or if we have to start a new one
  if(!dataBlockInfo->jBlockArray ||
     dataBlockInfo->metaType != data->meta_type() ||
     dataBlockInfo->metaLevel != data->meta_level() ||
     dataBlockInfo->dt != data->time_per_value() ||
     //check, if samples fit within one samples period
     abs(_diff.to_int64()) > dataBlockInfo->dt.to_int64()) 
    {
      dataBlockInfo->metaType = data->meta_type();
      dataBlockInfo->metaLevel = data->meta_level();
      dataBlockInfo->dt = data->time_per_value();

      json_object * jobj = json_object_new_object();

      json_object_array_add(jDataArray,jobj);

      switch (data->meta_type()) {
      case LibDLS::MetaGen:
	json_object_object_add(jobj,"meta",json_object_new_string("gen"));
	break;
      case LibDLS::MetaMin:
	json_object_object_add(jobj,"meta",json_object_new_string("min"));
	break;
      case LibDLS::MetaMax:
	json_object_object_add(jobj,"meta",json_object_new_string("max"));
	break;
      default:
	break;
      }

      json_object_object_add(jobj,"dt",json_object_new_double(data->time_per_value().to_uint64()));
      json_object_object_add(jobj,"start_time",json_object_new_double(data->start_time().to_uint64()));

      //for test
#if 0
      cout << data->size() << endl;
#endif

#if 0
      for(unsigned int i = 0;i<data->size();i++) {
	cout << data->time(i).to_uint64() << "," << data->value(i) << endl;
      }
#endif

      dataBlockInfo->jBlockArray = json_object_new_array();
      json_object_object_add(jobj,"block",dataBlockInfo->jBlockArray);
    } 

  //we expect dataBlockInfo->jBlockArray to be a valid pointer here!

  for(unsigned int i = 0;i<data->size();i++) {
    json_object *j2DataArray = json_object_new_array();
    json_object_array_add(j2DataArray,json_object_new_int64(data->time(i).to_uint64()));
    json_object_array_add(j2DataArray,json_object_new_double(data->value(i)));
    json_object_array_add(dataBlockInfo->jBlockArray,j2DataArray);

  }

  dataBlockInfo->curTime = data->end_time();

  return 0; //*data to be handled by caller of dataCallback
}
