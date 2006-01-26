//---------------------------------------------------------------
//
//  D L S _ C O M P R E S S I O N _ T. H P P
//
//---------------------------------------------------------------

#ifndef DLSCompressionTHpp
#define DLSCompressionTHpp

//---------------------------------------------------------------

#include <iostream> // debug
using namespace std;

//---------------------------------------------------------------

#include "com_zlib.hpp"
#include "com_base64.hpp"
#include "com_mdct_t.hpp"
#include "com_exception.hpp"

//---------------------------------------------------------------

/**
   Exception eines DLSCompression-Objektes
*/

class EDLSCompression : public COMException
{
public:
  EDLSCompression(const string &pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

/**
   Abstrakte Basisklasse eines Kompressionsobjektes
*/

template <class T>
class DLSCompressionT
{
public:
  DLSCompressionT() {};
  virtual ~DLSCompressionT() {};

  virtual unsigned int compress(const T *, unsigned int,
                                char *, unsigned int) = 0;
};

//---------------------------------------------------------------
//
//  ZLib / Base64
//
//---------------------------------------------------------------

/**
   Kompressionsobjekt: Erst ZLib, dann Base64
*/

template <class T>
class DLSCompressionT_ZLib : public DLSCompressionT<T>
{
public:
  virtual unsigned int compress(const T *, unsigned int, char *, unsigned int);

private:
  COMZLib _zlib;
  COMBase64 _base64;
};

//---------------------------------------------------------------

/**
   Komprimieren

   Komprimiert die Eingabedaten Byteweise mit Hilfe der ZLib
   und kodiert die komprimierten Daten dann mit Base64.

   \param input Konstanter Zeiger auf ein Array von Werten
   \param input_length Anzahl der Elemente im Eingabe-Array
   \param output Zeiger auf den Speicher für die Ausgabe
   \param output_size Größe des Ausgabespeichers in Bytes
*/

template <class T>
unsigned int DLSCompressionT_ZLib<T>::compress(const T *input,
                                               unsigned int input_length,
                                               char *output,
                                               unsigned int output_size)
{
  stringstream err;

  try
  {
    _zlib.compress((char *) input, input_length * sizeof(T));
    _base64.encode(_zlib.output(), _zlib.length());
  }
  catch (ECOMZLib &e)
  {
    err << "ZLib: " << e.msg;
    throw EDLSCompression(err.str());
  }
  catch (ECOMBase64 &e)
  {
    err << "Base64: " << e.msg;
    throw EDLSCompression(err.str());
  }

  if (_base64.length() > output_size)
  {
    throw EDLSCompression("output buffer too small!");
  }

  strcpy(output, _base64.output());
  return _base64.length();
}

//---------------------------------------------------------------
//
//  MDCT / ZLib / Base64
//
//---------------------------------------------------------------

/**
   Kompressionsobjekt: Erst MDCT, dann ZLib und dann Base64
*/

template <class T>
class DLSCompressionT_MDCT : public DLSCompressionT_ZLib<T>
{
public:
  DLSCompressionT_MDCT(unsigned int);
  ~DLSCompressionT_MDCT();

  unsigned int compress(const T *, unsigned int, char *, unsigned int);

private:
  COMMDCTT<T> *_mdct;

  DLSCompressionT_MDCT() {}; // privat!
};

//---------------------------------------------------------------

template <class T>
DLSCompressionT_MDCT<T>::DLSCompressionT_MDCT(unsigned int mdct_block_size)
{
  _mdct = 0;

  try
  {
    _mdct = new COMMDCTT<T>(mdct_block_size);
  }
  catch (ECOMMDCT &e)
  {
    throw EDLSCompression(e.msg);
  }
  catch (...)
  {
    throw EDLSCompression("could not allocate memory for MDCT object!");
  }
}

//---------------------------------------------------------------

template <class T>
DLSCompressionT_MDCT<T>::~DLSCompressionT_MDCT()
{
  if (_mdct) delete _mdct;
}

//---------------------------------------------------------------

/**
   Komprimieren

   Führt erst die MDCT-Transformation aus und übergibt die Daten
   dann an DLSCompressionT_ZLib_Base64<T>::compress().

   \param input Konstanter Zeiger auf ein Array von Werten
   \param input_length Anzahl der Elemente im Eingabe-Array
   \param output Zeiger auf den Speicher für die Ausgabe
   \param output_size Größe des Ausgabespeichers in Bytes
*/

template <class T>
unsigned int DLSCompressionT_MDCT<T>::compress(const T *input,
                                               unsigned int input_length,
                                               char *output,
                                               unsigned int output_size)
{
  stringstream err;

  //cout << "comp_mdct::compress called" << endl;

  try
  {
    _mdct->compress(input, input_length);
  }
  catch (ECOMMDCT &e)
  {
    err << "MDCT: " << e.msg;
    throw EDLSCompression(err.str());
  }

  //cout << "comp_mdct::compress returned" << endl;

  /*  return DLSCompressionT_ZLib<T>::compress(_mdct->output(),
                                           _mdct->output_length(),
                                           output,
                                           output_size);
  */

  return 0;
}

//---------------------------------------------------------------

#endif
