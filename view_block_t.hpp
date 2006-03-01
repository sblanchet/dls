//---------------------------------------------------------------
//
//  V I E W _ B L O C K _ T . H P P
//
//---------------------------------------------------------------

#ifndef ViewBlockTHpp
#define ViewBlockTHpp

//---------------------------------------------------------------

#include <fstream>
using namespace std;

//---------------------------------------------------------------

#include "com_base64.hpp"
#include "com_zlib.hpp"
#include "com_time.hpp"
#include "com_xml_tag.hpp"
#include "com_compression_t.hpp"
#include "view_chunk.hpp"

//---------------------------------------------------------------

/**
   Kontinuierliche Folge von Datenwerten (Anstrakt)
*/

class ViewBlock
{
public:
  ViewBlock() {};
  virtual ~ViewBlock() {};

  virtual void start_time(COMTime) = 0;
  virtual void end_time(COMTime) = 0;

  virtual COMTime start_time() const = 0;
  virtual COMTime end_time() const = 0;
  virtual unsigned int size() const = 0;
  virtual double min() const = 0;
  virtual double max() const = 0;
  virtual double value(unsigned int) const = 0;
};

//---------------------------------------------------------------

/**
   Kontinuierliche Folge von Datenwerten für die Anzeige
*/

template <class T>
class ViewBlockT : public ViewBlock
{
public:
  ViewBlockT();
  virtual ~ViewBlockT();

  void start_time(COMTime);
  void end_time(COMTime);
  bool read_from_tag(const COMXMLTag *, COMCompressionT<T> *);

  COMTime start_time() const;
  COMTime end_time() const;
  unsigned int size() const;
  double min() const;
  double max() const;
  double value(unsigned int) const;

protected:
  T *_data;            /**< Array mit Datenwerten */
  COMTime _start_time; /**< Zeit des ersten Datenwertes */
  COMTime _end_time;   /**< Zeit des letzten Datenwertes */
  unsigned int _size;  /**< Anzahl der Datenwerte */
};

//---------------------------------------------------------------

/**
   Konstruktor
*/

template <class T>
ViewBlockT<T>::ViewBlockT() : ViewBlock()
{
  _data = 0;
  _size = 0;
}

//---------------------------------------------------------------

/**
   Destruktor

   Gibt den von den Datenwerten belegten Speicher frei
*/

template <class T>
ViewBlockT<T>::~ViewBlockT()
{
  if (_data) delete [] _data;
}

//---------------------------------------------------------------

/**
   Setzt die Zeit des ersten Datenwertes

   \param time Startzeit
*/

template <class T>
void ViewBlockT<T>::start_time(COMTime time)
{
  _start_time = time;
}

//---------------------------------------------------------------

/**
   Setzt die Zeit des letzten Datenwertes

   \param time Endzeit
*/

template <class T>
void ViewBlockT<T>::end_time(COMTime time)
{
  _end_time = time;
}

//---------------------------------------------------------------

/**
   Liefert die Zeit des ersten Datenwertes

   \return Startzeit
*/

template <class T>
inline COMTime ViewBlockT<T>::start_time() const
{
  return _start_time;
}

//---------------------------------------------------------------

/**
   Liefert die Zeit des letzten Datenwertes

   \return Startzeit
*/

template <class T>
inline COMTime ViewBlockT<T>::end_time() const
{
  return _end_time;
}

//---------------------------------------------------------------

/**
   Liefert die Anzahl der Datenwerte

   \return Anzahl
*/

template <class T>
inline unsigned int ViewBlockT<T>::size() const
{
  return _size;
}

//---------------------------------------------------------------

/**
   Liest Datenwerte aus einem XML-Tag ein

   \param tag Konstanter Zeiger auf ein XML-Tag mit Daten
   \param comp Zeiger auf das passende (De-)Kompressionsobjekt
   \return true, wenn Datenwerte eingelesen wurden
*/

template <class T>
bool ViewBlockT<T>::read_from_tag(const COMXMLTag *tag, COMCompressionT<T> *comp)
{
  const char *block_data;
  unsigned int block_data_length, block_size;

  try
  {
    block_data = tag->att("d")->to_str().c_str();
    block_data_length = tag->att("d")->to_str().length();
    block_size = tag->att("s")->to_int();
  }
  catch (ECOMXMLTag &e)
  {
    cout << "could not read from tag: " << e.msg << endl;
    return false;
  }

  // Bisherige Daten löschen
  if (_data) delete [] _data;
  _data = 0;
  _size = 0;

  try
  {
    _data = new T[block_size];
  }
  catch (...)
  {
    cout << "could not allocate " << block_size << " bytes of memory!" << endl;
    return false;
  }

  try
  {
    comp->uncompress(block_data, block_data_length, _data, block_size);
  }
  catch (ECOMCompression &e)
  {
    cout << "ERROR while uncompressing: " << e.msg << endl;
    return false;
  }

  _size = block_size;

  return true;
}

//---------------------------------------------------------------

/**
   Liefert den kleinsten Wert

   \return Kleinster Datenwert
*/

template <class T>
double ViewBlockT<T>::min() const
{
  double min;
  unsigned int i;

  if (!_size) return 0;

  min = (double) _data[0];

  for (i = 1; i < _size; i++)
  {
    if (_data[i] < min) min = _data[i];
  }

  return min;
}

//---------------------------------------------------------------

/**
   Liefert den größten Wert

   \return Größter Datenwert
*/

template <class T>
double ViewBlockT<T>::max() const
{
  double max;
  unsigned int i;

  if (!_size) return 0;

  max = (double) _data[0];

  for (i = 1; i < _size; i++)
  {
    if (_data[i] > max) max = _data[i];
  }

  return max;
}

//---------------------------------------------------------------

/**
   Liefert einen einzelnen Datenwert

   \param index Index des Datenwertes im Array
   \return Datenwert
*/

template <class T>
double ViewBlockT<T>::value(unsigned int index) const
{
  if (index >= _size) return 0;

  return (double) _data[index];
}

//---------------------------------------------------------------

#endif
