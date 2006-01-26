//---------------------------------------------------------------
//
//  C O M _ M D C T _ T. H P P
//
//---------------------------------------------------------------
 
#ifndef COMMDCTTHpp
#define COMMDCTTHpp

//---------------------------------------------------------------

#define DEBUG_MDCT false

#if DEBUG_MDCT
#include <iostream>
using namespace std;
#endif

//---------------------------------------------------------------

#include "com_exception.hpp"

//---------------------------------------------------------------

/**
   Exception eines COMMDCTT-Objektes
*/

class ECOMMDCT : public COMException
{
public:
  ECOMMDCT(const string &pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

/**
   Überlappende, diskrete Cosinus-Transformation
*/

template <class T>
class COMMDCTT
{
public:
  COMMDCTT(unsigned int);
  ~COMMDCTT();

  void clear();
  void compress(const T *, unsigned int);
  const T *output() const;
  unsigned int output_length() const;

private:
  unsigned int _dim;
  unsigned int _exp;
  T *_output;
  unsigned int _output_length;
  unsigned int _output_size;
  T *_last_tail;

  static T *_sin_win_buffer[MDCT_MAX_EXP_PLUS_ONE];
  static T *_cos_buffer[MDCT_MAX_EXP_PLUS_ONE];
  static unsigned int _use_count;

  void _transform_all(const T *, unsigned int, T *);

  COMMDCTT() {};
};

//---------------------------------------------------------------

template <class T> T *COMMDCTT<T>::_sin_win_buffer[MDCT_MAX_EXP_PLUS_ONE];
template <class T> T *COMMDCTT<T>::_cos_buffer[MDCT_MAX_EXP_PLUS_ONE];
template <class T> unsigned int COMMDCTT<T>::_use_count = 0;

//---------------------------------------------------------------

template <class T>
COMMDCTT<T>::COMMDCTT(unsigned int dim)
{
  unsigned int i, j;
  double log2;
  stringstream err;

  _dim = 0;
  _exp = 0;
  _output = 0;
  _last_tail = 0;

  // Dimension muss Potenz von 2 sein!
  log2 = logb(dim) / logb(2);
  if (log2 != (unsigned int) log2 || log2 < MDCT_MIN_EXP || log2 >= MDCT_MAX_EXP_PLUS_ONE)
  {
    err << "invalid dimension: " << dim;
    throw ECOMMDCT(err.str());
  }

  _dim = dim;
  _exp = (unsigned int) log2;
  
  try
  {
    _last_tail = new T[_dim / 2];
    clear();
  }
  catch (...)
  {
    throw ECOMMDCT("could not allocate memory for tail buffer!");
  }

  // Erster Aufruf: Globale Puffer initialisieren
  if (_use_count++ == 0)
  {
#if DEBUG_MDCT
    cout << "init global buffers" << endl;
#endif

    for (i = 0; i < MDCT_MAX_EXP_PLUS_ONE; i++)
    {
      _sin_win_buffer[i] = 0;
      _cos_buffer[i] = 0;
    }
  }

  if (!_sin_win_buffer[_exp]) // Fenster- und Cos-Funktion noch nicht erzeugt
  {
#if DEBUG_MDCT
    cout << "creating buffers for exp " << _exp << endl;
#endif

    // Puffer für Sinus-Fensterfunktion erzeugen und initialisieren
    try
    {
      _sin_win_buffer[_exp] = new T[_dim];
    }
    catch (...)
    {
      throw ECOMMDCT("could not allocate memory for sine window buffer!");
    }

    for (i = 0; i < _dim; i++)
    {
      _sin_win_buffer[_exp][i] = sin(M_PI * (i + 0.5) / _dim);
    }

    // Puffer mit Cosinus-Koeffizienten erzeugen und initialisieren
    try
    {
      _cos_buffer[_exp] = new T[_dim * _dim / 2];
    }
    catch (...)
    {
      throw ECOMMDCT("could not allocate memory for cosine buffer!");
    }

    for (i = 0; i < _dim / 2; i++)
    { 
      for (j = 0; j < _dim; j++)
      { 
        _cos_buffer[_exp][i * _dim + j] =
          cos(M_PI * (2.0 * j + _dim / 2 + 1.0) * (2.0 * i + 1.0) / 2.0 / _dim);
      }
    }
  }
}

//---------------------------------------------------------------

template <class T>
COMMDCTT<T>::~COMMDCTT()
{
  unsigned int i;

  if (--_use_count == 0)
  {
#if DEBUG_MDCT
    cout << "cleaning global buffers" << endl;
#endif

    for (i = 0; i < MDCT_MAX_EXP_PLUS_ONE; i++)
    {
      if (_sin_win_buffer[i])
      {
#if DEBUG_MDCT
        cout << "cleaning sin " << i << endl;
#endif

        delete [] _sin_win_buffer[i];
        _sin_win_buffer[i] = 0;
      }

      if (_cos_buffer[i])
      {
#if DEBUG_MDCT
        cout << "cleaning cos " << i << endl;
#endif

        delete [] _cos_buffer[i];
        _cos_buffer[i] = 0;
      }
    }
  }

  if (_last_tail) delete [] _last_tail;
  if (_output) delete [] _output;
}

//---------------------------------------------------------------

template <class T>
void COMMDCTT<T>::clear()
{
  if (!_last_tail) return;

  for (unsigned int i = 0; i < _dim; i++)
  {
    _last_tail[i] = 0;
  }
}

//---------------------------------------------------------------

template <class T>
void COMMDCTT<T>::compress(const T *input, unsigned int input_length)
{
  unsigned int i, offset, blocks_of_dim, size;
  unsigned char dct_count;
  T *padded;

#if DEBUG_MDCT
  cout << "mdct::compress len=" << input_length << " dim=" << _dim << endl;
#endif

  if (!_dim) return; // Bei Fehler: Division durch 0 verhindern!

  // Für Flushing bitte flush() aufrufen!
  if (!input_length) return;

  if (input_length % _dim == 0)
  {
    blocks_of_dim = input_length / _dim;
  }
  else
  {
    blocks_of_dim = input_length / _dim + 1;
  }

#if DEBUG_MDCT
  cout << "blocks of dim: " << blocks_of_dim << endl;
#endif

  if (blocks_of_dim * 2 > 255)
  {
    throw ECOMMDCT("only 255 DCTs allowed per block!");
  }

  dct_count = blocks_of_dim * 2;
  size = 1 + dct_count * _dim / 2;

  // Prüfen, ob Ausgabepuffer groß genug, sonst reallokieren
  if (!_output || _output_size < size)
  {
    if (_output)
    {
      delete [] _output;
      _output = 0;
    }
    
    try
    {
      _output = new T[size];
      _output_size = size;
    }
    catch (...)
    {
      throw ECOMMDCT("could not allocate memory for output buffer!");
    }
  }

  try
  {
    // Eine Halbe Länge mehr für den "Übertrag"
    padded = new T[_dim / 2 + blocks_of_dim * _dim];
  }
  catch (...)
  {
    throw ECOMMDCT("could not allocate memory for padded buffer!");
  }

  // Rest des letzten Blockes vorne anfügen
  for (i = 0; i < _dim / 2; i++)
  {
    padded[i] = _last_tail[i];
  }

  // Daten in den Padded-Puffer kopieren
  offset = _dim / 2;
  for (i = 0; i < input_length; i++)
  {
    padded[offset + i] = input[i];
  }

  // Eventuell hinten auffüllen
  if (input_length < blocks_of_dim * _dim)
  {
    offset = _dim / 2 + input_length;
    for (i = 0; i < blocks_of_dim * _dim - input_length; i++)
    {
      padded[offset + i] = 0;
    }
  }

#if DEBUG_MDCT
  cout << "input buffer: " << endl;
  
  for (i = 0; i < blocks_of_dim * _dim + _dim / 2; i++)
  {
    cout << padded[i] << ", ";
  }
  
  cout << endl;
#endif

  _output[0] = dct_count;

  // DCT's ausführen
  _transform_all(padded, dct_count, _output + 1);
  
#if DEBUG_MDCT
  cout << "transforming end" << endl;
#endif

  delete [] padded;

  _output_length = 0;
}

//---------------------------------------------------------------

template <class T>
void COMMDCTT<T>::_transform_all(const T *input,
                                 unsigned int dct_count,
                                 T *output)
{
  unsigned int i, j, d;
  
#if DEBUG_MDCT
  cout << "transforming " << dct_count << " DCTs..." << endl;
#endif

  // Alle DCTs durchführen
  for (d = 0; d < dct_count; d++)
  {
    // Jeden Einzelwert berechnen
    for (i = 0; i < _dim / 2; i++)
    {
      _output[i] = 0;
      
      for (j = 0; j < _dim; j++)
      {
        _output[d + i * dct_count] +=
          _sin_win_buffer[_exp][j] * input[_dim / 2 * d + j] * _cos_buffer[_exp][i * _dim + j];
      }
    }
  }
}

//---------------------------------------------------------------

template <class T>
const T *COMMDCTT<T>::output() const
{
  return _output;
}

//---------------------------------------------------------------

template <class T>
unsigned int COMMDCTT<T>::output_length() const
{
  return _output_length;
}

//---------------------------------------------------------------

#endif

#ifdef NEVER

int msr_mdct(unsigned int x,double *in,double *out) {

    int i,j,dim;

    if(x>MAX_MDCT_DIM-1) 
	return -1;   

    if(mdct_buffers[x].refcount <=0)
	return -2;

    dim = mdct_buffers[x].dim;

    for(i=0;i<(dim/2);i++) {
	out[i]=0.0;
	for(j=0;j<dim;j++) {
	    out[i]=out[i]+mdct_buffers[x].swinbuf[j]*in[j]*mdct_buffers[x].cosbuf[i*dim+j];
	}
    }
    return 0; //alles ok
}

int msr_imdct(unsigned int x,double *in,double *out) {

    int i,j,dim;

    if(x>MAX_MDCT_DIM-1) 
	return -1;   

    if(mdct_buffers[x].refcount <=0)
	return -2;

    dim = mdct_buffers[x].dim;

    for(i=0;i<dim;i++) {
	out[i]=0.0;
	for(j=0;j<dim/2;j++) {
	    out[i]=out[i]+in[j]*mdct_buffers[x].cosbuf[j*dim+i];
	}
	out[i]*=mdct_buffers[x].swinbuf[i]*4/dim;
    }
    return 0; //alles ok
}

#endif
