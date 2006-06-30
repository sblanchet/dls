/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef ViewDataTHpp
#define ViewDataTHpp

/*****************************************************************************/

#include <fstream>
#include <vector>
using namespace std;

/*****************************************************************************/

#include "com_xml_tag.hpp"
#include "com_compression_t.hpp"
#include "view_data.hpp"

//#define DEBUG

/*****************************************************************************/

/**
   Datenbereich. Template-Implementation von ViewData
*/

template <class T>
class ViewDataT : public ViewData
{
public:
  ViewDataT();
  ~ViewDataT();

  bool load_data_tag(const ViewChunk *,
                     const char *,
                     unsigned int,
                     COMCompressionT<T> *);
  void clear();

  T min() const;
  T max() const;

  double value(unsigned int) const; // virtuell
  double time(unsigned int) const;  // virtuell
  unsigned int size() const;        // virtuell

private:
  vector<T> _data; /**< Vektor mit den Datenwerten */
};

/*****************************************************************************/

/**
   Konstruktor
*/

template <class T>
ViewDataT<T>::ViewDataT() : ViewData()
{
}

/*****************************************************************************/

/**
   Destruktor
*/

template <class T>
ViewDataT<T>::~ViewDataT()
{
}

/*****************************************************************************/

/**
   Löscht die Daten
*/

template <class T>
void ViewDataT<T>::clear()
{
  _data.clear();
}

/*****************************************************************************/

/**
   Lädt Daten aus einem XML-Tag

   \param chunk Konstanter Zeiger auf den besitzenden Chunk
   \param block_data Zeiger auf die Daten im Tag
   \param block_size Vermerkte Anzahl der Datenwerte
   \param comp Zeiger auf das Kompressionsobjekt
*/

template <class T>
bool ViewDataT<T>::load_data_tag(const ViewChunk *chunk,
                                 const char *block_data,
                                 unsigned int block_size,
                                 COMCompressionT<T> *comp)
{
  unsigned int i;

  if (block_size)
  {
    try
    {
      comp->uncompress(block_data, strlen(block_data), block_size);
    }
    catch (ECOMCompression &e)
    {
      cout << "ERROR while uncompressing: " << e.msg << endl;
      return false;
    }
      
    for (i = 0; i < comp->decompressed_length(); i++)
    {
      _data.push_back(comp->decompression_output()[i]);
    }
    
    return true;
  }

  else if (chunk->format_index() == DLS_FORMAT_MDCT)
  {
#ifdef DEBUG
    cout << "reading overlapping mdct block. dim/2 = "
         << chunk->mdct_block_size() / 2 << endl;
#endif

    try
    {
      comp->flush_uncompress(block_data, strlen(block_data));
    }
    catch (ECOMCompression &e)
    {
      cout << "ERROR while uncompressing: " << e.msg << endl;
      return false;
    }

#ifdef DEBUG
    cout << "decompressed values: " << comp->decompressed_length() << endl;
#endif
      
    for (i = 0; i < comp->decompressed_length(); i++)
    {
      _data.push_back(comp->decompression_output()[i]);
    }

#ifdef DEBUG
    cout << "values: " << endl;
    for (i = 0; i < data.size(); i++)
    {
        cout << data[i] << ", ";
    }
    cout << endl;
#endif
    
    return true;
  }
  
  return false;
}

/*****************************************************************************/

/**
   Liefert das Minimum der aktuellen Datenwerte
   
   \return Kleinster Datenwert
*/

template <class T>
T ViewDataT<T>::min() const
{
  T current_min;
  unsigned int i;

  if (!_data.size()) return 0;

  current_min = _data[0];

  for (i = 1; i < _data.size(); i++)
  {
    if (_data[i] < current_min) current_min = _data[i];
  }

  return current_min;
}

/*****************************************************************************/

/**
   Liefert das Maximum der aktuellen Datenwerte
   
   \return Größter Datenwert
*/

template <class T>
T ViewDataT<T>::max() const
{
  T current_max;
  unsigned int i;

  if (!_data.size()) return 0;

  current_max = _data[0];

  for (i = 1; i < _data.size(); i++)
  {
    if (_data[i] > current_max) current_max = _data[i];
  }

  return current_max;
}

/*****************************************************************************/

/**
   Liefert die zeit des n-ten Datenwertes als double
   
   \param index Index des angefragten Datenwertes
   \return Zeit
*/

template <class T>
double ViewDataT<T>::time(unsigned int index) const
{
  if (index >= _data.size()) return 0;

  return (double) _start_time.to_dbl() + index * _time_per_value;
}

/*****************************************************************************/

/**
   Liefert den n-ten Datenwert als double
   
   \param index Index des angefragten Datenwertes
   \return Datenwert
*/

template <class T>
double ViewDataT<T>::value(unsigned int index) const
{
  if (index >= _data.size()) return 0;

  return (double) _data[index];
}

/*****************************************************************************/

/**
   Liefert die Anzahl der gespeicherten Datenwerte
   
   \return Anzahl
*/

template <class T>
unsigned int ViewDataT<T>::size() const
{
  return _data.size();
}

/*****************************************************************************/

#ifdef DEBUG
#undef DEBUG
#endif

#endif
