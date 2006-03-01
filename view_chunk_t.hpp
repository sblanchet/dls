/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef ViewChunkTHpp
#define ViewChunkTHpp

/*****************************************************************************/

#include "com_exception.hpp"
#include "com_time.hpp"
#include "com_index_t.hpp"
#include "com_file.hpp"
#include "com_compression_t.hpp"
#include "view_globals.hpp"
#include "view_chunk.hpp"
#include "view_data_t.hpp"

//#define DEBUG false

/*****************************************************************************/

/**
   Chunk-Template-Klasse zur Anzeige

   Kann einen Datenbereich eines Chunks in den Speicher laden
*/

template <class T>
class ViewChunkT : public ViewChunk
{
public:
  ViewChunkT();
  ~ViewChunkT();
  
  void fetch_data(COMTime, COMTime, unsigned int);
  void clear();

  const ViewData *gen_data() const;
  const ViewData *min_data() const;
  const ViewData *max_data() const;

  void calc_min_max(double *, double *) const;
  unsigned int blocks_fetched() const;
  bool has_data() const;

private:
  ViewDataT<T> _gen_data;       /**< Datenliste für generische Daten */
  ViewDataT<T> _min_data;       /**< Datenliste für Minimum-Daten */
  ViewDataT<T> _max_data;       /**< Datenliste für Maximum-Daten */
  unsigned int _blocks_fetched; /**< Anzahl der Blocks, die geladen wurden */

  COMRingBufferT<char, unsigned int> *_ring; /**< Ringpuffer zum Laden der Daten */
  COMCompressionT<T> *_compression;          /**< Zeiger auf ein Kompressionsobjekt */

  void _load_data(ViewDataT<T> *, const string &, const string &, COMTime, COMTime);
};

/*****************************************************************************/

/**
   Konstruktor
*/

template <class T>
ViewChunkT<T>::ViewChunkT()
{
  _ring = 0;
  _compression = 0;
}

/*****************************************************************************/

/**
   Destruktor
*/

template <class T>
ViewChunkT<T>::~ViewChunkT()
{
  clear();

  if (_ring) delete _ring;
  if (_compression) delete _compression;
}

/*****************************************************************************/

/**
   Lesezugriff auf die geladenen, generischen Daten

   \return Konstanter Zeiger auf die Datenliste
*/

template <class T>
inline const ViewData *ViewChunkT<T>::gen_data() const
{
  return &_gen_data;
}

/*****************************************************************************/

/**
   Lesezugriff auf die geladenen Minimumdaten

   \return Konstanter Zeiger auf die Datenliste
*/

template <class T>
inline const ViewData *ViewChunkT<T>::min_data() const
{
  return &_min_data;
}

/*****************************************************************************/

/**
   Lesezugriff auf die geladenen Maximumdaten

   \return Konstanter Zeiger auf die Datenliste
*/

template <class T>
inline const ViewData *ViewChunkT<T>::max_data() const
{
  return &_max_data;
}

/*****************************************************************************/

/**
   Liefert die Anzahl der geladenen Blöcke

   \return Anzahl der Blöcke
*/

template <class T>
inline unsigned int ViewChunkT<T>::blocks_fetched() const
{
  return _blocks_fetched;
}

/*****************************************************************************/

/**
   Gibt zurück, ob überhaupt Daten geladen wurden

   \return true, wenn Daten vorhanden
*/

template <class T>
bool ViewChunkT<T>::has_data() const
{
  return _max_data.size() || _min_data.size() || _gen_data.size();
}

/*****************************************************************************/

/**
   Lädt Daten zu einem bestimmten Zeitbereich in das Objekt

   \param start Anfang des Zeitbereiches
   \param end Ende des Zeitbereiches
   \param values_wanted Optimale Anzahl der zu ladenden Datenwerte
*/

template <class T>
void ViewChunkT<T>::fetch_data(COMTime start,
                               COMTime end,
                               unsigned int values_wanted)
{
  stringstream level_dir_name;
  double tpv;
  clear();

  // Optimalen Meta-Level errechnen
  _calc_optimal_level(start, end, values_wanted);

  tpv = _time_per_value();
  _gen_data.time_per_value(tpv);
  _min_data.time_per_value(tpv);
  _max_data.time_per_value(tpv);

  level_dir_name << _dir << "/level" << _level;

  if (_level == 0)
  {
    _load_data(&_gen_data, level_dir_name.str(), "gen", start, end);
  }
  else
  {
    _load_data(&_min_data, level_dir_name.str(), "min", start, end);
    _load_data(&_max_data, level_dir_name.str(), "max", start, end);
  }
}

/*****************************************************************************/

/**
   Lädt Daten zu einem bestimmten Zeitbereich in eine Liste

   \param data_list Zeiger auf die Liste, in die geladen werden soll
   \param level_dir_name Ebenenverzeichnis, aus dem geladen werden soll
   \param meta_type Meta-Typ, der geladen werden soll
   \param start Anfang des Zeitbereiches
   \param end Ende des Zeitbereiches
*/

template <class T>
void ViewChunkT<T>::_load_data(ViewDataT<T> *data_list,
                               const string &level_dir_name,
                               const string &meta_type,
                               COMTime start, COMTime end)
{
  string global_index_file_name;
  stringstream data_file_name;
  COMIndexT<COMGlobalIndexRecord> global_index;
  COMGlobalIndexRecord global_index_record;
  COMIndexT<COMIndexRecord> index;
  COMIndexRecord index_record;
  COMFile data_file;
  unsigned int i, write_len, blocks_read = 0;
  char *write_ptr;
  COMXMLParser xml;
  bool must_read_again;

  global_index_file_name = level_dir_name; 
  global_index_file_name += "/data_" + meta_type + ".idx";
  
  try
  {
    global_index.open_read(global_index_file_name);
  }
  catch (ECOMIndexT &e)
  {
    // Globaler Index in dieder Ebene nicht gefunden.
    // Keine Meldung und beenden!
    return;
  }

  try
  {
    // Wenn noch kein Ring-Lesespeicher reserviert wurde, dies jetzt tun
    if (!_ring) _ring = new COMRingBufferT<char, unsigned int>(READ_RING_SIZE);
  }
  catch (...)
  {
    cout << "ERROR: Could not allocate memory for ring buffer!" << endl;
    return;
  }

  // Ist bereits ein Kompressionsobjekt vorhanden?
  if (_compression)
  {
    // Dieses jetzt entfernen!
    delete _compression;
    _compression = 0;
  }

  // Ein neues Kompressionsobjekt erzeugen
  if (_format_index == DLS_FORMAT_ZLIB)
  {
    _compression = new COMCompressionT_ZLib<T>();
  }
  else if (_format_index == DLS_FORMAT_MDCT)
  {
    if (typeid(T) == typeid(float))
    {
      _compression = (COMCompressionT<T> *) new COMCompressionT_MDCT<float>(_mdct_block_size, 0);
    }
    else if (typeid(T) == typeid(double))
    {
      _compression = (COMCompressionT<T> *) new COMCompressionT_MDCT<double>(_mdct_block_size, 0);
    }
    else
    {
      cout << "ERROR: MDCT only for floating point types!" << endl;
      return;
    }
  }
  else if (_format_index == DLS_FORMAT_QUANT)
  {
    if (typeid(T) == typeid(float))
    {
      _compression = (COMCompressionT<T> *) new COMCompressionT_Quant<float>(0.0);
    }
    else if (typeid(T) == typeid(double))
    {
      _compression = (COMCompressionT<T> *) new COMCompressionT_Quant<double>(0.0);
    }
    else
    {
      cout << "ERROR: Quant only for floating point types!" << endl;
      return;
    }
  }
  else
  {
    cout << "ERROR: Unknown compression type index: " << _format_index << endl;
    return;
  }

  // Alle indizierten Datendateien durchlaufen
  for (i = 0; i < global_index.record_count(); i++)
  {
    try
    {
      // Einen Index-Record aus dem globalen Index lesen
      global_index_record = global_index[i];
    }
    catch (ECOMIndexT &e)
    {
      cout << "ERROR: Could not read record " << i << " from global index \"";
      cout << global_index_file_name << "\". Reason: " << e.msg << endl;
      return;
    }

    if (COMTime(global_index_record.end_time) < start && global_index_record.end_time != 0)
    {
      // Die vom Index referenzierte Datendatei liegt noch vor dem gesuchten
      // Zeitbereich. Die Nächste versuchen...
      continue;
    }

    if (COMTime(global_index_record.start_time) > end)
    {
      // Ab hier liegt alles nach dem gesuchten Zeitbereich. Suche beenden.
      break;
    }

    // Den Namen der Datendatei generieren
    data_file_name.str("");
    data_file_name.clear();
    data_file_name << level_dir_name;
    data_file_name << "/data" << global_index_record.start_time << "_" << meta_type;

    try
    {
      // Versuchen, den Index der Datendatei zu öffnen
      index.open_read(data_file_name.str() + ".idx");
    
      // ...und die Datendatei selber öffnen
      data_file.open_read(data_file_name.str().c_str());
    }
    catch (ECOMIndexT &e)
    {
      cout << "ERROR: Could not open index \"";
      cout << data_file_name.str() << ".idx\": " << e.msg << endl;
      return;
    }
    catch (ECOMFile &e)
    {
      cout << "ERROR: Could not open data file \"";
      cout << data_file_name.str() << "\": " << e.msg << endl;
      return;
    }

    // Alle Records im Index durchlaufen
    for (i = 0; i < index.record_count(); i++)
    {
      try
      {
        index_record = index[i];
      }
      catch (ECOMIndexT &e)
      {
        cout << "ERROR: Could not read from index: " << e.msg << endl;
        return;
      }

#ifdef DEBUG
      cout << "Trying record " << i << endl;
#endif

      // Der Block liegt noch vor der gesuchten Zeit. Den Nächsten versuchen!
      if (COMTime(index_record.end_time) < start) continue;

      // Jetzt liegen alle Blöcke hinter der gesuchten Zeit. Abbrechen!
      if (COMTime(index_record.start_time) >= end) break;
      
#ifdef DEBUG
      cout << "Record ok!" << endl;
#endif
      
      try
      {
        // In der Datendatei zur indizierten Position springen
        data_file.seek(index_record.position);
      }
      catch (ECOMFile &e)
      {
        cout << "ERROR: Could not seek in data file!" << endl;
        return;
      }
      
      _ring->clear();

      // Solange etwas in den Ring lesen, bis ein Tag vollständig ist.
      while (1)
      {
        // Schreibadresse des Ringes holen
        _ring->write_info(&write_ptr, &write_len);
        
        // Leselänge begrenzen
        if (write_len > 1024) write_len = 1024;
        
#ifdef DEBUG
        cout << "Trying to read " << write_len << " byte(s)..." << endl;
#endif

        try
        {
          data_file.read(write_ptr, write_len, &write_len);
        }
        catch (ECOMFile &e)
        { 
          cout << "ERROR: Could not read from data file: " << e.msg << endl;
          return;
        }
        
#ifdef DEBUG
        cout << write_len << " byte(s) read." << endl;
#endif
        
        if (write_len == 0) // Datei zuende! (Darf normalerweise nicht vorkommen)
        {
          cout << "ERROR: EOF in file \"" << data_file_name.str() << "\"";
          cout << " after searching position " << index_record.position << "!" << endl;
          return;
        }
        
        // Vermerken, dass Daten in den Ring geschrieben wurden
        _ring->written(write_len);
        
        try
        {
          // Versuchen, aus dem Gelesenen ein Tag zu parsen
          xml.parse(_ring);
        }
        catch (ECOMXMLParserEOF &e)
        {
          // Tag ist noch nicht vollständig. Weiterlesen!
          continue;
        }
        catch (ECOMXMLParser &e)
        {
          // Fehler beim Parsen! Abbrechen.
          cout << "parsing error: " << e.msg << endl;
          return;
        }
        
#ifdef DEBUG
        cout << "Tag parsed: " << xml.tag()->title() << endl;
#endif
     
        if (xml.tag()->title() == "d") // Ein Daten-Tag ist komplett
        {
          // Zeiten der Datenliste aktualisieren
          if (data_list->size() == 0)
          {
            data_list->start_time(index_record.start_time);
            data_list->end_time(index_record.end_time);
          }
          else
          {
            if (COMTime(index_record.start_time) < data_list->start_time())
            {
              data_list->start_time(index_record.start_time);
            }
            if (COMTime(index_record.end_time) > data_list->end_time())
            {
              data_list->end_time(index_record.end_time);
            }
          }
          
          // Chunk-Zeiten aktualisieren
          if (data_list->start_time() < _start) _start = data_list->start_time();
          if (data_list->end_time() > _end) _end = data_list->end_time();
          
          try
          {
            // Daten aus Tag laden und dekomprimieren
            data_list->load_data_tag(this,
                                     xml.tag()->att("d")->to_str().c_str(),
                                     xml.tag()->att("s")->to_int(),
                                     _compression);
          }
          catch (ECOMXMLTag &e)
          {
            cout << "ERROR: Could not read block: " << e.msg << endl;
            return;
          }

          blocks_read++;
          _blocks_fetched++;
        }
        
        break;
      }
      
      // Nächster Index-Record
    }
  }

  // Blocks geladen, Dateien noch offen

  if (blocks_read && _format_index == DLS_FORMAT_MDCT)
  {
    // Jetzt noch einen Block zusätzlich lesen
    
#ifdef DEBUG
    cout << "Loading one more record... Ring size: " << _ring->length() << endl;

    cout << "Ring content: " << endl;
    for (i = 0; i < _ring->length(); i++) cout << (*_ring)[i];
    cout << endl;
#endif

    try
    {
      // Versuchen, aus dem Gelesenen ein Tag zu parsen
      xml.parse(_ring);
      must_read_again = false;
    }
    catch (ECOMXMLParser &e)
    {
      // Fehler beim Parsen! Abbrechen.
      cout << "ERROR: While parsing: " << e.msg << endl;
      return;
    }
    catch (ECOMXMLParserEOF &e)
    {
      // Tag ist noch nicht vollständig. Weiterlesen!
      must_read_again = true;
    }

    // Solange etwas in den Ring lesen, bis ein Tag vollständig ist.
    while (must_read_again)
    {
      // Schreibadresse des Ringes holen
      _ring->write_info(&write_ptr, &write_len);
        
      // Leselänge begrenzen
      if (write_len > 1024) write_len = 1024;
        
#ifdef DEBUG
      cout << "Trying to read " << write_len << " byte(s)..." << endl;
#endif

      try
      {
        data_file.read(write_ptr, write_len, &write_len);
      }
      catch (ECOMFile &e)
      { 
        cout << "ERROR: Could not read data file: " << e.msg << endl;
        return;
      }
        
#ifdef DEBUG
      cout << write_len << " byte(s) read." << endl;
#endif
        
      if (write_len == 0) // Datei zuende!
      {
        // Dies kommt vor, wenn z. B. während der Erfassung einer MDCT noch kein
        // Folgeblock existiert. Wenn die Erfassung abgeschlossen ist, sollte
        // dies allerdings nicht auftreten!

        //cout << "WARNING: No succeding MDCT block available in \"" << data_file_name.str() << "\"" << endl;
        return;
      }
        
      // Vermerken, dass Daten in den Ring geschrieben wurden
      _ring->written(write_len);

      try
      {
        // Versuchen, aus dem Gelesenen ein Tag zu parsen
        xml.parse(_ring);
        must_read_again = false;
      }
      catch (ECOMXMLParser &e)
      {
        // Fehler beim Parsen! Abbrechen.
        cout << "ERROR: While parsing: " << e.msg << endl;
        return;
      }
      catch (ECOMXMLParserEOF &e)
      {
        // Tag ist noch nicht vollständig. Weiterlesen!
      }
    }
 
#ifdef DEBUG
    cout << "Tag parsed: " << xml.tag()->title() << endl;
#endif
     
    if (xml.tag()->title() == "d") // Ein Daten-Tag ist komplett
    {
      try
      {
        // Daten aus Tag laden und dekomprimieren
        data_list->load_data_tag(this,
                                 xml.tag()->att("d")->to_str().c_str(),
                                 xml.tag()->att("s")->to_int(),
                                 _compression);
      }
      catch (ECOMXMLTag &e)
      {
        cout << "ERROR: Could not read block!" << endl;
        return;
      }
          
      _blocks_fetched++;
    }
  }
}

/*****************************************************************************/

/**
   Entfernt alle geladenen Daten
*/

template <class T>
void ViewChunkT<T>::clear()
{
  _gen_data.clear();
  _min_data.clear();
  _max_data.clear();
  _blocks_fetched = 0;
}

/*****************************************************************************/

/**
   Berechnet den kleinsten und den größten Datenwert

   \param p_min Zeiger auf einen Speicher für den Minimalwert
   \param p_max Zeiger auf einen Speicher für den Maximalwert
*/

template <class T>
void ViewChunkT<T>::calc_min_max(double *p_min, double *p_max) const
{
  T data_min, data_max, current_min, current_max;
  bool first = true;

  current_min = 0;
  current_max = 0;

  if (_gen_data.size())
  {
#ifdef DEBUG
    cout << "Using gen data for min/max calc" << endl;
#endif

    current_min = _gen_data.min();
    current_max = _gen_data.max();
    first = false;
  }

  if (_min_data.size())
  {
#ifdef DEBUG
    cout << "Using min data for min/max calc" << endl;
#endif

    data_min = _min_data.min();
    data_max = _min_data.max();

    if (first)
    {
      current_min = data_min;
      current_max = data_max;
      first = false;
    }
    else
    {
      if (data_min < current_min) current_min = data_min;
      if (data_max > current_max) current_max = data_max;
    }
  }

  if (_max_data.size())
  {
#ifdef DEBUG
    cout << "Using max data for min/max calc" << endl;
#endif

    data_min = _max_data.min();
    data_max = _max_data.max();

    if (first)
    {
      current_min = data_min;
      current_max = data_max;
      first = false;
    }
    else
    {
      if (data_min < current_min) current_min = data_min;
      if (data_max > current_max) current_max = data_max;
    }
  }

  *p_min = current_min;
  *p_max = current_max;
}

/*****************************************************************************/

#ifdef DEBUG
#undef DEBUG
#endif

#endif
