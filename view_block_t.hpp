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

//---------------------------------------------------------------

class ViewBlock
{
public:
  ViewBlock() {};
  virtual ~ViewBlock() {};

  virtual void start_time(COMTime) = 0;
  virtual void end_time(COMTime) = 0;
  virtual void size(unsigned int) = 0;
  virtual void time_per_value(double) = 0;
  virtual bool read(const string &) = 0;

  virtual COMTime start_time() const = 0;
  virtual COMTime end_time() const = 0;
  virtual const string &meta() const = 0;
  virtual unsigned int size() const = 0;
  virtual double time_per_value() const = 0;
  virtual double min() const = 0;
  virtual double max() const = 0;
  virtual double value(unsigned int) const = 0;
};

//---------------------------------------------------------------

template <class T>
class ViewBlockT : public ViewBlock
{
public:
  ViewBlockT();
  virtual ~ViewBlockT();

  void start_time(COMTime);
  void end_time(COMTime);
  void size(unsigned int);
  void time_per_value(double);
  bool read(const string &);

  COMTime start_time() const;
  COMTime end_time() const;
  const string &meta() const;
  unsigned int size() const;
  double time_per_value() const;
  double min() const;
  double max() const;
  double value(unsigned int) const;

protected:
  T *_data;
  unsigned int _data_size;
  COMTime _start_time;
  COMTime _end_time;
  string _meta;
  unsigned int _size;
  double _time_per_value;
};

//---------------------------------------------------------------

template <class T>
ViewBlockT<T>::ViewBlockT() : ViewBlock()
{
  _start_time = (long long) 0;
  _meta = "UNDEFINED";
  _size = 0;
  _time_per_value = 0;
  _data_size = 0;
}

//---------------------------------------------------------------

template <class T>
ViewBlockT<T>::~ViewBlockT()
{
  if (_data_size)
  {
    delete [] _data;
  }
}

//---------------------------------------------------------------

template <class T>
void ViewBlockT<T>::start_time(COMTime time)
{
  _start_time = time;
}

//---------------------------------------------------------------

template <class T>
void ViewBlockT<T>::end_time(COMTime time)
{
  _end_time = time;
}

//---------------------------------------------------------------

template <class T>
void ViewBlockT<T>::size(unsigned int size)
{
  _size = size;
}

//---------------------------------------------------------------

template <class T>
void ViewBlockT<T>::time_per_value(double tpv)
{
  _time_per_value = tpv;
}

//---------------------------------------------------------------

template <class T>
inline COMTime ViewBlockT<T>::start_time() const
{
  return _start_time;
}

//---------------------------------------------------------------

template <class T>
inline COMTime ViewBlockT<T>::end_time() const
{
  return _end_time;
}

//---------------------------------------------------------------

template <class T>
inline const string &ViewBlockT<T>::meta() const
{
  return _meta;
}

//---------------------------------------------------------------

template <class T>
inline double ViewBlockT<T>::time_per_value() const
{
  return _time_per_value;
}

//---------------------------------------------------------------

template <class T>
inline unsigned int ViewBlockT<T>::size() const
{
  return _size;
}

//---------------------------------------------------------------

template <class T>
bool ViewBlockT<T>::read(const string &block_data)
{
  COMBase64 base64;
  COMZLib zlib;

  if (_data_size)
  {
    delete [] _data;
    _data_size = 0;
  }

  try
  {
    base64.decode(block_data.c_str(), block_data.length());
    zlib.uncompress(base64.output(), base64.length(), _size * sizeof(T));
  }
  catch (ECOMBase64 &e)
  {
    cout << "error while base64-decoding: " << e.msg << endl;
    return false;
  }
  catch (ECOMZLib &e)
  {
    cout << "error while zlib-uncompressing: " << e.msg << endl;
    return false;
  }

  try
  {
    _data = new T[_size];
    _data_size = _size;
  }
  catch (...)
  {
    cout << "could not allocate " << _size << " bytes of memory!" << endl;
    return false;
  }

  // Daten kopieren
  for (unsigned int i = 0; i < _data_size; i++)
  {
    _data[i] = ((T *) zlib.output())[i];
  }

  return true;
}

//---------------------------------------------------------------

template <class T>
double ViewBlockT<T>::min() const
{
  double min;
  unsigned int i;

  if (_size == 0) return 0;

  min = (double) _data[0];

  i = 1;
  while (i < _size)
  {
    if (_data[i] < min) min = _data[i];
    i++;
  }

  return min;
}

//---------------------------------------------------------------

template <class T>
double ViewBlockT<T>::max() const
{
  double max;
  unsigned int i;

  if (_size == 0) return 0;

  max = (double) _data[0];

  i = 1;
  while (i < _size)
  {
    if (_data[i] > max) max = _data[i];
    i++;
  }

  return max;
}

//---------------------------------------------------------------

template <class T>
double ViewBlockT<T>::value(unsigned int index) const
{
  if (index >= _size) return 0;

  return (double) _data[index];
}

//---------------------------------------------------------------

#endif
