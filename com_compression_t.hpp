//---------------------------------------------------------------
//
//  C O M _ C O M P R E S S I O N _ T. H P P
//
//---------------------------------------------------------------

#ifndef COMCompressionTHpp
#define COMCompressionTHpp

//---------------------------------------------------------------

#include "com_zlib.hpp"
#include "com_base64.hpp"
#include "com_mdct_t.hpp"
#include "com_exception.hpp"

//---------------------------------------------------------------

/**
   Exception eines DLSCompression-Objektes
*/

class ECOMCompression : public COMException
{
public:
  ECOMCompression(const string &pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

/**
   Abstrakte Basisklasse eines Kompressionsobjektes
*/

template <class T>
class COMCompressionT
{
public:
  COMCompressionT() {};
  virtual ~COMCompressionT() {};

  /**
     Komprimiert ein Array von Datenwerten beliebigen, aber gleichen Typs

     \param input Konstanter Zeiger auf ein Array von Werten
     \param input_length Anzahl der Werte im Input-Array
     \param output Zeiger auf einen Speicherbereich zum Ablegen
                   der komprimierten Daten
     \param output_size Größe des reservierten Ausgabespeichers in Bytes
     \return Größe der komprimierten Daten in Bytes
     \throw ECOMCompression Fehler beim Komprimieren
  */

  virtual unsigned int compress(const T *input,
                                unsigned int input_length,
                                char *output,
                                unsigned int output_size) = 0;

  /**
     Wandelt komprimierte Binärdaten in ein Array von Datenwerten um

     \param input Konstanter Zeiger auf den Speicherbereich mit
                  den komprimierten Binärdaten
     \param input_length Größe der komprimierten Daten in Bytes
     \param output Zeiger auf einen Speicherbereich zum Ablegen
                   der dekomprimierten Datenwerte
     \param output_size Maximale Anzahl der zu dekomprimierenden Datenwerte
     \return Anzahl tatsächlich dekomprimierter Datenwerte
     \throw ECOMCompression Fehler beim Dekomprimieren
  */

  virtual unsigned int uncompress(const char *input,
                                  unsigned int input_length,
                                  T *output,
                                  unsigned int output_size) = 0;

  /**
     Leert den persistenten Speicher des Komprimierungsvorganges
     und liefert die restlichen, komprimierten Daten zurück

     \param output Zeiger auf einen Speicherbereich zum Ablegen
                   der komprimierten Datenwerte
     \param output_size Größe des Ausgabespeichers
     \return Größe der komprimierten Daten in Bytes
     \throw ECOMCompression Fehler beim Komprimieren
  */

  virtual unsigned int flush_compress(char *output,
                                      unsigned int output_size) = 0;

  /**
     Leert den persistenten Speicher des Dekomprimierungsvorganges
     und liefert die restlichen, dekomprimierten Daten

     Die Größe des Ausgabespeichers liegt in diesem Fall fest.

     \param input Konstanter Zeiger auf den Speicherbereich mit
                  den zuvor von flush_compress() gelieferten Binärdaten
     \param input_length Größe der komprimierten Daten in Bytes
     \param output Zeiger auf einen Speicherbereich zum Ablegen
                   der dekomprimierten Datenwerte
     \return Anzahl tatsächlich dekomprimierter Datenwerte
     \throw ECOMCompression Fehler beim Dekomprimieren
  */

  virtual unsigned int flush_uncompress(const char *input,
                                        unsigned int input_length,
                                        T *output) = 0;

  /**
     Löscht alle Komprimierungsoperationen, die von vorherigen Daten abhängig sind
  */

  virtual void clear() = 0;
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
class COMCompressionT_ZLib : public COMCompressionT<T>
{
public:
  virtual unsigned int compress(const T *input,
                                unsigned int input_length,
                                char *output,
                                unsigned int output_size);
  virtual unsigned int uncompress(const char *input,
                                  unsigned int input_length,
                                  T *output,
                                  unsigned int output_size);
  virtual void clear();
  virtual unsigned int flush_compress(char *output,
                                      unsigned int output_size);
  virtual unsigned int flush_uncompress(const char *input,
                                        unsigned int input_length,
                                        T *output);

protected:
  COMZLib _zlib;     /**< ZLib-Objekt zum Komprimieren */
  COMBase64 _base64; /**< Base64-Objekt zum Kodieren */
};

//---------------------------------------------------------------

template <class T>
unsigned int COMCompressionT_ZLib<T>::compress(const T *input,
                                               unsigned int input_length,
                                               char *output,
                                               unsigned int output_size)
{
  stringstream err;
  unsigned int i;

  try
  {
    _zlib.compress((char *) input, input_length * sizeof(T));
    _base64.encode(_zlib.output(), _zlib.output_length());
  }
  catch (ECOMZLib &e)
  {
    err << "ZLib: " << e.msg;
    throw ECOMCompression(err.str());
  }
  catch (ECOMBase64 &e)
  {
    err << "Base64: " << e.msg;
    throw ECOMCompression(err.str());
  }

  if (_base64.output_length() + 1 > output_size)
  {
    throw ECOMCompression("output buffer too small!");
  }

  // Komprimierte Daten in Ausgabepuffer kopieren
  for (i = 0; i < _base64.output_length(); i++)
  {
    output[i] = _base64.output()[i];
  }

  output[_base64.output_length()] = 0;

  return _base64.output_length();
}

//---------------------------------------------------------------

template <class T>
unsigned int COMCompressionT_ZLib<T>::uncompress(const char *input,
                                                 unsigned int input_length,
                                                 T *output,
                                                 unsigned int output_size)
{
  stringstream err;
  unsigned int i;

  try
  {
    _base64.decode(input, input_length);
    _zlib.uncompress(_base64.output(), _base64.output_length(), output_size * sizeof(T));
  }
  catch (ECOMBase64 &e)
  {
    err << "error while base64-decoding: " << e.msg << endl;
    throw ECOMCompression(err.str());
  }
  catch (ECOMZLib &e)
  {
    err << "error while zlib-uncompressing: " << e.msg << endl;
    throw ECOMCompression(err.str());
  }

  if (_zlib.output_length() % sizeof(T) != 0
      || _zlib.output_length() / sizeof(T) != output_size)
  {
    err << "zlib output (" << _zlib.output_length();
    err << ") does not fit to expected values (" << output_size;
    err << ")!" << endl;
    throw ECOMCompression(err.str());
  }

  for (i = 0; i < output_size; i++)
  {
    output[i] = ((T *) _zlib.output())[i];
  }

  return output_size;
}

//---------------------------------------------------------------

template <class T>
void COMCompressionT_ZLib<T>::clear()
{
}

//---------------------------------------------------------------

template <class T>
unsigned int COMCompressionT_ZLib<T>::flush_compress(char *output,
                                                     unsigned int output_size)
{
  return 0;
}

//---------------------------------------------------------------

template <class T>
unsigned int COMCompressionT_ZLib<T>::flush_uncompress(const char *input,
                                                       unsigned int input_length,
                                                       T *output)
{
  return 0;
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
class COMCompressionT_MDCT : public COMCompressionT_ZLib<T>
{
public:
  COMCompressionT_MDCT(unsigned int, double);
  ~COMCompressionT_MDCT();

  unsigned int compress(const T *input,
                        unsigned int input_length,
                        char *output,
                        unsigned int output_size);
  unsigned int uncompress(const char *input,
                          unsigned int input_length,
                          T *output,
                          unsigned int output_size);
  void clear();
  unsigned int flush_compress(char *output,
                              unsigned int output_size);
  unsigned int flush_uncompress(const char *input,
                                unsigned int input_length,
                                T *output);

private:
  COMMDCTT<T> *_mdct; /**< MDCT-Objekt zum Transformieren */

  COMCompressionT_MDCT() {}; // privat!
};

//---------------------------------------------------------------

template <class T>
COMCompressionT_MDCT<T>::COMCompressionT_MDCT(unsigned int dim,
                                              double acc)
{
  _mdct = 0;

  try
  {
    _mdct = new COMMDCTT<T>(dim, acc);
  }
  catch (ECOMMDCT &e)
  {
    throw ECOMCompression(e.msg);
  }
  catch (...)
  {
    throw ECOMCompression("could not allocate memory for MDCT object!");
  }
}

//---------------------------------------------------------------

template <class T>
COMCompressionT_MDCT<T>::~COMCompressionT_MDCT()
{
  if (_mdct) delete _mdct;
}

//---------------------------------------------------------------

template <class T>
unsigned int COMCompressionT_MDCT<T>::compress(const T *input,
                                               unsigned int input_length,
                                               char *output,
                                               unsigned int output_size)
{
  stringstream err;

  try
  {
    _mdct->transform(input, input_length);
  }
  catch (ECOMMDCT &e)
  {
    err << "MDCT: " << e.msg;
    throw ECOMCompression(err.str());
  }

  return COMCompressionT_ZLib<T>::compress(_mdct->output(),
                                           _mdct->output_length(),
                                           output,
                                           output_size);
}

//---------------------------------------------------------------

template <class T>
unsigned int COMCompressionT_MDCT<T>::uncompress(const char *input,
                                                 unsigned int input_length,
                                                 T *output,
                                                 unsigned int output_size)
{
  stringstream err;
  unsigned int i, real_size;

  if (output_size % _mdct->block_size() == 0)
  {
    real_size = output_size;
  }
  else
  {
    real_size = (output_size / _mdct->block_size() + 1) * _mdct->block_size();
  }

  real_size++; // Einen Wert mehr für MDCT-Mittelwert

  try
  {
    _base64.decode(input, input_length);
    _zlib.uncompress(_base64.output(), _base64.output_length(), real_size * sizeof(T));
    _mdct->detransform((T *) _zlib.output(), output_size + 1);
  }
  catch (ECOMBase64 &e)
  {
    err << "error while base64-decoding: " << e.msg << endl;
    throw ECOMCompression(err.str());
  }
  catch (ECOMZLib &e)
  {
    err << "error while zlib-uncompressing: " << e.msg << endl;
    throw ECOMCompression(err.str());
  }
  catch (ECOMMDCT &e)
  {
    err << "error while MDCT-detransforming: " << e.msg << endl;
    throw ECOMCompression(err.str());
  }

  for (i = 0; i < _mdct->output_length(); i++)
  {
    output[i] = _mdct->output()[i];
  }

  return _mdct->output_length();
}

//---------------------------------------------------------------

template <class T>
void COMCompressionT_MDCT<T>::clear()
{
  _mdct->clear();
}

//---------------------------------------------------------------

template <class T>
unsigned int COMCompressionT_MDCT<T>::flush_compress(char *output,
                                                     unsigned int output_size)
{
  stringstream err;

  try
  {
    _mdct->flush_transform();
  }
  catch (ECOMMDCT &e)
  {
    err << "MDCT flush: " << e.msg;
    throw ECOMCompression(err.str());
  }

  return COMCompressionT_ZLib<T>::compress(_mdct->output(),
                                           _mdct->output_length(),
                                           output,
                                           output_size);
}

//---------------------------------------------------------------

template <class T>
unsigned int COMCompressionT_MDCT<T>::flush_uncompress(const char *input,
                                                       unsigned int input_length,
                                                       T *output)
{
  stringstream err;
  unsigned int i;

  try
  {
    _base64.decode(input, input_length);
    _zlib.uncompress(_base64.output(), _base64.output_length(),
                     (_mdct->block_size() / 2 + 1) * sizeof(T));
    _mdct->flush_detransform((T *) _zlib.output());
  }
  catch (ECOMBase64 &e)
  {
    err << "error while base64-decoding: " << e.msg << endl;
    throw ECOMCompression(err.str());
  }
  catch (ECOMZLib &e)
  {
    err << "error while zlib-uncompressing: " << e.msg << endl;
    throw ECOMCompression(err.str());
  }
  catch (ECOMMDCT &e)
  {
    err << "error while MDCT-detransforming: " << e.msg << endl;
    throw ECOMCompression(err.str());
  }

  for (i = 0; i < _mdct->output_length(); i++)
  {
    output[i] = _mdct->output()[i];
  }

  return _mdct->output_length();
}

//---------------------------------------------------------------

#endif
