//---------------------------------------------------------------
//
//  V I E W _ C H A N N E L . C P P
//
//---------------------------------------------------------------

#include <fcntl.h>
#include <dirent.h>

#include <iostream>
#include <sstream>
#include <fstream>
using namespace std;

#include "com_xml_parser.hpp"
#include "view_channel.hpp"

//---------------------------------------------------------------

ViewChannel::ViewChannel()
{
  _index = 0;
  _min_level = 0;
  _max_level = 0;
}

//---------------------------------------------------------------

ViewChannel::~ViewChannel()
{
  clear();
}

//---------------------------------------------------------------

void ViewChannel::import(const string &dls_dir,
                         unsigned int job_id,
                         unsigned int channel_id)
{
  stringstream channel_dir_name, err;
  string channel_file_name;
  fstream file;
  COMXMLParser xml;

  _index = channel_id;

  channel_dir_name << dls_dir;
  channel_dir_name << "/job" << job_id;
  channel_dir_name << "/channel" << _index;

  channel_file_name = channel_dir_name.str() + "/channel.xml";

  file.open(channel_file_name.c_str(), ios::in);

  if (!file.is_open())
  {
    err << "could not open channel file \"" << channel_file_name << "\"!";
    throw EViewChannel(err.str());
  }

  try
  {
    xml.parse(&file, "dlschannel", dxttBegin);
    xml.parse(&file, "channel", dxttSingle);
    
    _name = xml.last_tag()->att("name")->to_str();
    _unit = xml.last_tag()->att("unit")->to_str();
    _type = xml.last_tag()->att("type")->to_str();

    xml.parse(&file, "dlschannel", dxttEnd);
  }
  catch (ECOMXMLParser &e)
  {
    file.close();
    err << "channel " << _index << " parsing error: " << e.msg;
    throw EViewChannel(err.str());
  }
  catch (ECOMXMLParserEOF &e)
  {
    file.close();
    err << "channel " << _index << " parsing error: " << e.msg;
    throw EViewChannel(err.str());
  }
  catch (ECOMXMLTag &e)
  {
    file.close();
    err << "channel " << _index << " parsing (tag) error: " << e.msg;
    throw EViewChannel(err.str());
  }

  file.close();
}

//---------------------------------------------------------------

void ViewChannel::fetch_chunks(const string &dls_dir, unsigned int job_id)
{
  stringstream channel_dir_name;
  DIR *dir;
  struct dirent *dir_ent;
  string dir_ent_name;
  ViewChunk chunk;
  bool first = true;

  _range_start = (long long) 0;
  _range_end = (long long) 0;

  // Alle bisherigen Chunks entfernen
  _chunks.clear();

  // Kanal-Verzeichnisnamen konstruieren
  channel_dir_name << dls_dir << "/job" << job_id << "/channel" << _index;

  // Kanalverzeichnislisting öffnen
  if ((dir = opendir(channel_dir_name.str().c_str())) == NULL)
  {
    cout << "ERROR: could not open \"" << channel_dir_name.str() << "\"." << endl;
    return;
  }

  // Alle Einträge im Kanalverzeichnis durchforsten
  while ((dir_ent = readdir(dir)) != NULL)
  {
    dir_ent_name = dir_ent->d_name;
    
    // Es interessieren nur die Einträge, die mit "chunk" beginnen
    if (dir_ent_name.find("chunk") != 0) continue;

    // Chunk-Verzeichnis setzen
    chunk.set_dir(channel_dir_name.str() + "/" + dir_ent_name);
    
    try
    {
      chunk.import();
    }
    catch (EViewChunk &e)
    {
      cout << "WARNING: could not import chunk: " << e.msg << endl;
      continue;
    }

    try
    {
      // Start- und Endzeiten holen
      chunk.fetch_range();
    }
    catch (EViewChunk &e)
    {
      cout << "WARNING: could not fetch range: " << e.msg << endl;
      continue;
    }

    // Minimal- und Maximalzeiten mitnehmen
    if (first)
    {
      _range_start = chunk.start();    
      _range_end = chunk.end();
      first = false;
    }
    else
    {
      if (chunk.start() < _range_start) _range_start = chunk.start();    
      if (chunk.end() > _range_end) _range_end = chunk.end();
    }

    // Chunk in die Liste einfügen
    _chunks.push_back(chunk);
  }
}

//---------------------------------------------------------------

void ViewChannel::load_data(COMTime start,
                            COMTime end,
                            unsigned int values_wanted)
{
  list<ViewChunk>::iterator chunk_i;
  bool first = true;
  unsigned int level;

  _min_level = 0;
  _max_level = 0;

  if (start >= end) return;

  chunk_i = _chunks.begin();
  while (chunk_i != _chunks.end())
  {
    // Wenn der Chunk überhaupt Anteile am gesuchten Zeitbereich hat
    if (chunk_i->start() <= end && chunk_i->end() >= start)
    {
      // Daten laden
      chunk_i->fetch_data(this, start, end, values_wanted);

      // Geladenen Level ermitteln
      level = chunk_i->current_level();

      if (first)
      {
        first = false;
        _min_level = level;
        _max_level = level;
      }
      else
      {
        if (level < _min_level) _min_level = level;
        if (level > _max_level) _max_level = level;
      }
    }
    else
    {
      chunk_i->clear();
    }

    chunk_i++;
  }

  // Wertespanne errechnen
  _calc_min_max();
}

//---------------------------------------------------------------

void ViewChannel::clear()
{
  _chunks.clear();
}

//---------------------------------------------------------------

void ViewChannel::_calc_min_max()
{
  list<ViewChunk>::const_iterator chunk_i;
  list<ViewBlock *>::const_iterator block_i;
  double min, max;
  bool first = true;
  
  _min = 0;
  _max = 0;

  chunk_i = _chunks.begin();
  while (chunk_i != _chunks.end())
  {
    block_i = chunk_i->blocks()->begin();
    while (block_i != chunk_i->blocks()->end())
    {
      min = (*block_i)->min();
      max = (*block_i)->max();

      if (first)
      {
        _min = min;
        _max = max;
        first = false;
      }
      else
      {
        if (min < _min) _min = min;
        if (max > _max) _max = max;
      }

      block_i++;
    }

    block_i = chunk_i->min_blocks()->begin();
    while (block_i != chunk_i->min_blocks()->end())
    {
      min = (*block_i)->min();
      max = (*block_i)->max();

      if (first)
      {
        _min = min;
        _max = max;
        first = false;
      }
      else
      {
        if (min < _min) _min = min;
        if (max > _max) _max = max;
      }

      block_i++;
    }
    
    block_i = chunk_i->max_blocks()->begin();
    while (block_i != chunk_i->max_blocks()->end())
    {
      min = (*block_i)->min();
      max = (*block_i)->max();

      if (first)
      {
        _min = min;
        _max = max;
        first = false;
      }
      else
      {
        if (min < _min) _min = min;
        if (max > _max) _max = max;
      }

      block_i++;
    }

    chunk_i++;
  }
}

//---------------------------------------------------------------

unsigned int ViewChannel::blocks_fetched() const
{
  unsigned int blocks = 0;
  list<ViewChunk>::const_iterator chunk_i;

  chunk_i = _chunks.begin();
  while (chunk_i != _chunks.end())
  {
    blocks += chunk_i->blocks()->size();
    blocks += chunk_i->min_blocks()->size();
    blocks += chunk_i->max_blocks()->size();
    chunk_i++;
  }

  return blocks;
}

//---------------------------------------------------------------
