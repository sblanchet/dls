//---------------------------------------------------------------
//
//  V I E W _ C H U N K . C P P
//
//---------------------------------------------------------------

#include <dirent.h>
#include <fcntl.h>

#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

//---------------------------------------------------------------

#include "com_xml_parser.hpp"
#include "com_index_t.hpp"
#include "com_file.hpp"
#include "view_globals.hpp"
#include "view_block_t.hpp"
#include "view_chunk.hpp"
#include "view_channel.hpp"

//---------------------------------------------------------------

ViewChunk::ViewChunk()
{
  _level = 0;
}

//---------------------------------------------------------------

ViewChunk::~ViewChunk()
{
  clear();
}

//---------------------------------------------------------------

void ViewChunk::set_dir(const string &dir)
{
  _dir = dir;
}

//---------------------------------------------------------------

void ViewChunk::import()
{
  stringstream err;
  string chunk_file_name;
  fstream file;
  COMXMLParser xml;

  chunk_file_name = _dir + "/chunk.xml";

  file.open(chunk_file_name.c_str(), ios::in);

  if (!file.is_open())
  {
    err << "could not open chunk file \"" << chunk_file_name << "\"!";
    throw EViewChunk(err.str());
  }

  try
  {
    xml.parse(&file, "dlschunk", dxttBegin);
    xml.parse(&file, "chunk", dxttSingle);
    
    _sample_frequency = xml.last_tag()->att("sample_frequency")->to_int();
    _meta_reduction = xml.last_tag()->att("meta_reduction")->to_int();

    //xml.parse(&file, "dlschunk", dxttEnd);
  }
  catch (ECOMXMLParser &e)
  {
    file.close();
    err << "parsing error: " << e.msg;
    throw EViewChunk(err.str());
  }
  catch (ECOMXMLParserEOF &e)
  {
    file.close();
    err << "parsing error: " << e.msg;
    throw EViewChunk(err.str());
  }
  catch (ECOMXMLTag &e)
  {
    file.close();
    err << "parsing (tag) error: " << e.msg;
    throw EViewChunk(err.str());
  }

  file.close();
}

//---------------------------------------------------------------

void ViewChunk::fetch_range()
{
  string global_index_file_name;
  stringstream err, index_file_name;
  COMIndexT<COMGlobalIndexRecord> global_index;
  COMGlobalIndexRecord first_global_index_record;
  COMGlobalIndexRecord last_global_index_record;
  COMIndexT<COMIndexRecord> index;
  COMIndexRecord index_record;

  _start = (long long) 0;
  _end = (long long) 0;

  global_index_file_name = _dir + "/level0/data_gen.idx";

  try
  {
    global_index.open(global_index_file_name);
  }
  catch (ECOMIndexT &e)
  {
    err << "opening global index: " << e.msg << endl; 
    throw EViewChunk(err.str());
  }

  if (global_index.record_count() == 0)
  {
    err << "global index file \"" << global_index_file_name << "\" has no records!"; 
    throw EViewChunk(err.str());
  }

  try
  {
    // Ersten und letzten Record lesen, um die Zeitspanne zu bestimmen
    first_global_index_record = global_index[0];
    last_global_index_record = global_index[global_index.record_count() - 1];
  }
  catch (ECOMIndexT &e)
  {
    err << "could not read first and last record from global index file \"" << global_index_file_name << "\"!"; 
    throw EViewChunk(err.str());
  }

  // In die letzte Datendatei wird noch erfasst
  // -> Die aktuelle, letzte Zeit aus dem Datendatei-Index holen
  if (last_global_index_record.end_time == 0)
  {
    index_file_name << _dir << "/level0/data" << last_global_index_record.start_time << "_gen.idx";

    try
    {
      // Index öffnen
      index.open(index_file_name.str());
    }
    catch (ECOMIndexT &e)
    {
      err << "could not open index file \"" << index_file_name.str() << "\": " << e.msg; 
      throw EViewChunk(err.str());
    }

    if (index.record_count() == 0)
    {
      err << "index file \"" << index_file_name.str() << "\" has no records!"; 
      throw EViewChunk(err.str());
    }

    try
    {
      // Letzten Record lesen
      index_record = index[index.record_count() - 1];
    }
    catch (ECOMIndexT &e)
    {
      err << "could not read from index file \"" << index_file_name.str() << "\": " << e.msg; 
      throw EViewChunk(err.str());
    }

    last_global_index_record.end_time = index_record.end_time;

    index.close();
  }

  global_index.close();

  _start = first_global_index_record.start_time;
  _end = last_global_index_record.end_time;
}

//---------------------------------------------------------------

void ViewChunk::_calc_optimal_level(COMTime start,
                                    COMTime end,
                                    unsigned int values_wanted)
{
  long long gen_values, values, use_values;
  unsigned int level;

  // Die Anzahl der generischen Datenwerte für diese Zeitspanne
  gen_values = (long long) ((end - start).to_dbl() / 1000000.0 * _sample_frequency);

  level = 1;
  use_values = gen_values;

  while (1)
  {
    values = (unsigned int) (gen_values / pow((double) _meta_reduction, (int) level));
      
    // Es müssen mehr Werte als gefordert in der Meta-Ebene existieren,
    // aber so wenig wie möglich
    if (values >= values_wanted)
    {
      // Diese Ebene kommt schonmal in Frage
      use_values = values;
      
      // Nächste Ebene prüfen
      level++;
    }
    else
    {
      // Jetzt waren es zuviel Werte. Eine Ebene zurück und beenden.
      level--;
      break;
    }
  }

  _level = level;
}

//---------------------------------------------------------------

bool ViewChunk::fetch_data(ViewChannel *channel,
                           COMTime start,
                           COMTime end,
                           unsigned int values_wanted)
{
  stringstream level_dir_name;

  clear();

  // Optimalen Meta-Level errechnen
  _calc_optimal_level(start, end, values_wanted);

  level_dir_name << _dir << "/level" << _level;

  if (_level == 0)
  {
    _load_blocks(&_blocks, level_dir_name.str(), "gen", channel, start, end);
  }
  else
  {
    _load_blocks(&_min_blocks, level_dir_name.str(), "min", channel, start, end);
    //_load_blocks(&_blocks, level_dir_name.str(), "mean", channel, start, end);
    _load_blocks(&_max_blocks, level_dir_name.str(), "max", channel, start, end);
  }

  return true;
}

//---------------------------------------------------------------

bool ViewChunk::_load_blocks(list<ViewBlock*> *blocks,
                             const string &level_dir_name,
                             const string &meta_type,
                             ViewChannel *channel,
                             COMTime start, COMTime end)
{
  ViewBlock *block;
  string global_index_file_name;
  stringstream data_file_name;
  COMXMLParser xml;
  COMIndexT<COMIndexRecord> index;
  COMIndexRecord index_record;
  char *write_ptr;
  unsigned int write_len;
  COMRingBufferT<char, unsigned int> ring(100000);
  COMIndexT<COMGlobalIndexRecord> global_index;
  COMGlobalIndexRecord global_index_record;
  COMFile data_file;

  global_index_file_name = level_dir_name; 
  global_index_file_name += "/data_" + meta_type + ".idx";
  
  try
  {
    global_index.open(global_index_file_name);
  }
  catch (ECOMIndexT &e)
  {
    // Globaler Index in dieder Ebene nicht gefunden.
    // Keine Meldung und beenden!
    return false;
  }

  if (global_index.record_count() == 0)
  {
    cout << "ERROR: global index \"" << global_index_file_name << "\" has no entries!" << endl;
    return false;
  }

  for (unsigned int j = 0; j < global_index.record_count(); j++)
  {
    try
    {
      global_index_record = global_index[j];
    }
    catch (ECOMIndexT &e)
    {
      cout << "ERROR: could not read record " << j << " from global index \"" << global_index_file_name << "\"!" << endl;
      return false;
    }

    // Wenn die vom Index referenzierte Datendatei Anteil am gesuchten Zeitbereich hat
    if (COMTime(global_index_record.end_time) < start && global_index_record.end_time != 0) continue;

    // Ab hier liegt alles nach dem gesuchten Zeitbereich. Suche beenden.
    if (COMTime(global_index_record.start_time) > end) break;

    // Namen der Datendatei generieren
    data_file_name.str("");
    data_file_name.clear();
    data_file_name << level_dir_name << "/data" << global_index_record.start_time;
    data_file_name << "_" << meta_type;
    
    try
    {
      // Versuchen, den Index der Datendatei zu öffnen
      index.open(data_file_name.str() + ".idx");
    }
    catch (ECOMIndexT &e)
    {
      cout << "ERROR: could not open index \"" << data_file_name.str() << ".idx\": " << e.msg << endl;
      return false;
    }

    try
    {
      // Die Datendatei selber öffnen
      data_file.open_read(data_file_name.str().c_str());
    }
    catch (ECOMFile &e)
    {
      cout << "ERROR: could not open data file \"" << data_file_name.str() << "\": " << e.msg << endl;
      return false;
    }

    for (unsigned int i = 0; i < index.record_count(); i++)
    {
      try
      {
        index_record = index[i];
      }
      catch (ECOMIndexT &e)
      {
        cout << "ERROR: could not read from index: " << e.msg << endl;
        return false;
      }

      // Der Block liegt noch vor der gesuchten Zeit. Den Nächsten versuchen!
      if (COMTime(index_record.end_time) < start) continue;

      // Jetzt liegen alle Blöcke hinter der gesuchten Zeit. Abbrechen!
      if (COMTime(index_record.start_time) > end) break;
      
      try
      {
        data_file.seek(index_record.position);
      }
      catch (ECOMFile &e)
      {
        cout << "ERROR: could not seek in data file!" << endl;
        return false;
      }

      ring.clear();  

      while (1)
      {
        ring.write_info(&write_ptr, &write_len);

        try
        {
          data_file.read(write_ptr, write_len, &write_len); // TODO: LIEST VOLLE RINGLÄNGE!
        }
        catch (ECOMFile &e)
        { 
          cout << "ERROR: could not read from data file!" << endl;
          return false;
        }

        if (write_len == 0) // EOF
        {
          cout << "ERROR: EOF in file \"" << data_file_name.str() << "\"!" << endl;
          return false;
        }

        ring.written(write_len);
        
        try
        {
          xml.parse(&ring);
          
          if      (channel->type() == "TUINT")
          {
            block = new ViewBlockT<unsigned int>();
          }
          else if (channel->type() == "TULINT")
          {
            block = new ViewBlockT<unsigned long>();
          }
          else if (channel->type() == "TINT")
          {
            block = new ViewBlockT<int>();
          }
          else if (channel->type() == "TDBL")
          {
            block = new ViewBlockT<double>();
          }
          else
          {
            cout << "unknown type \"" << channel->type() << "\"..." << endl;
            return false;
          }
          
          block->start_time(index_record.start_time);
          block->end_time(index_record.end_time);
          block->time_per_value(pow((double) _meta_reduction, (int) _level)
                                / (double) _sample_frequency * 1000000.0);
          
          if (xml.last_tag()->title() == "d")
          {
            block->size(xml.last_tag()->att("s")->to_int());
            
            if (!block->read(xml.last_tag()->att("d")->to_str()))
            {
              delete block;
              cout << "ERROR: could not read block!" << endl;
              return false;
            }

            blocks->push_back(block);
          }
          
          break;
        }
        catch (ECOMXMLParser &e)
        {
          cout << "parsing error: " << e.msg << endl;
          return false;
        }
        catch (ECOMXMLTag &e)
        {
          cout << "parsing (tag) error: " << e.msg << endl;
          return false;
        }
        catch (ECOMXMLParserEOF &e)
        {
        }
      }
    }

    data_file.close();
    index.close();
  }

  global_index.close();

  return true;
}

//---------------------------------------------------------------

void ViewChunk::clear()
{
  list<ViewBlock *>::iterator block_i;

  block_i = _blocks.begin();
  while (block_i != _blocks.end())
  {
    delete *block_i;
    block_i++;
  }

  _blocks.clear();

  block_i = _min_blocks.begin();
  while (block_i != _min_blocks.end())
  {
    delete *block_i;
    block_i++;
  }

  _min_blocks.clear();

  block_i = _max_blocks.begin();
  while (block_i != _max_blocks.end())
  {
    delete *block_i;
    block_i++;
  }

  _max_blocks.clear();
}

//---------------------------------------------------------------

