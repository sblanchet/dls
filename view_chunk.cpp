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
#include "view_chunk.hpp"
#include "view_channel.hpp"

//---------------------------------------------------------------

RCS_ID("$Header: /home/fp/dls/src/RCS/view_chunk.cpp,v 1.12 2005/03/11 10:44:26 fp Exp $");

//---------------------------------------------------------------

/**
   Konstruktor
*/

ViewChunk::ViewChunk()
{
  _level = 0;
}

//---------------------------------------------------------------

/**
   Destruktor
*/

ViewChunk::~ViewChunk()
{
}

//---------------------------------------------------------------

/**
   Setzt das Chunk-Verzeichnis, auf dem das Objekt arbeiten soll

   \param dir Chunk-Verzeichnis
*/

void ViewChunk::set_dir(const string &dir)
{
  _dir = dir;
}

//---------------------------------------------------------------

/**
   Importiert die Informationen aus "chunk.xml"
*/

void ViewChunk::import()
{
  stringstream err;
  string chunk_file_name, format_str;
  fstream file;
  COMXMLParser xml;
  int i;

  chunk_file_name = _dir + "/chunk.xml";

  file.open(chunk_file_name.c_str(), ios::in);

  if (!file.is_open())
  {
    err << "Could not open chunk file \"" << chunk_file_name << "\"!";
    throw EViewChunk(err.str());
  }

  try
  {
    xml.parse(&file, "dlschunk", dxttBegin);
    xml.parse(&file, "chunk", dxttSingle);
    
    _sample_frequency = xml.tag()->att("sample_frequency")->to_int();
    _meta_reduction = xml.tag()->att("meta_reduction")->to_int();
    format_str = xml.tag()->att("format")->to_str(); 

    _format_index = DLS_FORMAT_INVALID;
    for (i = 0; i < DLS_FORMAT_COUNT; i++)
    {
      if (format_str == dls_format_strings[i])
      {
        _format_index = i;
        break;
      }
    }

    if (_format_index == DLS_FORMAT_INVALID)
    {
      throw EViewChunk("Unknown compression format!");
    }

    if (_format_index == DLS_FORMAT_MDCT)
    {
      _mdct_block_size = xml.tag()->att("mdct_block_size")->to_int(); 
    }

    //xml.parse(&file, "dlschunk", dxttEnd);
  }
  catch (ECOMXMLParser &e)
  {
    file.close();
    err << "Parsing error: " << e.msg;
    throw EViewChunk(err.str());
  }
  catch (ECOMXMLParserEOF &e)
  {
    file.close();
    err << "Parsing error: " << e.msg;
    throw EViewChunk(err.str());
  }
  catch (ECOMXMLTag &e)
  {
    file.close();
    err << "Parsing (tag) error: " << e.msg;
    throw EViewChunk(err.str());
  }

  file.close();
}

//---------------------------------------------------------------

/**
   Holt den Zeitbereich des Chunks
*/

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
    global_index.open_read(global_index_file_name);
  }
  catch (ECOMIndexT &e)
  {
    err << "Opening global index: " << e.msg << endl; 
    throw EViewChunk(err.str());
  }

  if (global_index.record_count() == 0)
  {
    err << "Global index file \"" << global_index_file_name << "\" has no records!"; 
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
    err << "Could not read first and last record from global index file \"" << global_index_file_name << "\"!"; 
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
      index.open_read(index_file_name.str());
    }
    catch (ECOMIndexT &e)
    {
      err << "Could not open index file \"" << index_file_name.str() << "\": " << e.msg; 
      throw EViewChunk(err.str());
    }

    if (index.record_count() == 0)
    {
      err << "Index file \"" << index_file_name.str() << "\" has no records!"; 
      throw EViewChunk(err.str());
    }

    try
    {
      // Letzten Record lesen
      index_record = index[index.record_count() - 1];
    }
    catch (ECOMIndexT &e)
    {
      err << "Could not read from index file \"" << index_file_name.str() << "\": " << e.msg; 
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

/**
   Berechnet die optimale Meta-Ebene für die angegebene Auflösung

   \param start Anfang des zu ladenden zeitbereichs
   \param end Ende des zu ladenden Zeitbereichs
   \param values_wanted Maximale Anzahl der zu ladenden Werte
*/

void ViewChunk::_calc_optimal_level(COMTime start,
                                    COMTime end,
                                    unsigned int values_wanted)
{
  double level;

  level = floor(log10(_sample_frequency * (end - start).to_dbl_time() / values_wanted)
               / log10((double) _meta_reduction));

#ifdef DEBUG
  cout << "values wanted: " << values_wanted;
  cout << " level: " << level;
  cout << " values got: " << 
    _sample_frequency * (end - start).to_dbl_time() / pow((double) _meta_reduction, (int) (level > 0 ? level : 0)) << endl;
#endif

  if (level < 0) level = 0;

  _level = (unsigned int) level;
}

//---------------------------------------------------------------

/**
   Gibt die Zeit für einen Schritt zurück

   \return Delta t
*/

double ViewChunk::_time_per_value() const
{
  return pow((double) _meta_reduction, (int) _level)
    / (double) _sample_frequency * 1000000.0;
}

//---------------------------------------------------------------
