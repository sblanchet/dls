//---------------------------------------------------------------
//
//  C O M _ M D C T _ T. H P P
//
//---------------------------------------------------------------
 
#ifndef COMMDCTTHpp
#define COMMDCTTHpp

//---------------------------------------------------------------

//#define MDCT_DEBUG
//#define MDCT_DONT_CORRECT_BY_MEAN
//#define MDCT_NO_DIFF
//#define MDCT_OMEGA

#ifdef MDCT_DEBUG
#include <iostream>
using namespace std;
#endif

//---------------------------------------------------------------

#include "com_globals.hpp"
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
  COMMDCTT(unsigned int, double);
  ~COMMDCTT();

  void transform(const T *, unsigned int);
  void detransform(const T *, unsigned int);
  void flush_transform();
  void flush_detransform(const T *input);
  void clear();

  //@{
  const T *output() const;
  unsigned int output_length() const;
  //@}

  unsigned int block_size() const;

private:
  //@{
  unsigned int _dim;           /**< Dimension einer Einzel-DCT */
  unsigned int _exp2;          /**< Exponent zur Basis 2 der Dimension */
  double _accuracy;            /**< Genauigkeit: Alles kleinere wird auf 0 gesetzt. */
  T *_output;                  /**< Ausgabepuffer */
  unsigned int _output_length; /**< Anzahl der Werte im Ausgabepuffer */
  //@}

  T *_last_tail;               /**< Die (_dim / 2) letzten Werte der letzten MDCT */
  T _last_value;               /**< Letzter Wert der Differentierung/Integration */
  bool _first;                 /**< true, wenn noch keine MDCT stattgefunden hat */
  unsigned int _last_length;   /**< Anzahl Datenwerte der letzten MDCT */

  //@{
  static T *_sin_win_buffer[MDCT_MAX_EXP2_PLUS_ONE]; /**< Sinus-Fensterfunktionen für
                                                          verschiedene Zweierpotenzen */
  static T *_cos_buffer[MDCT_MAX_EXP2_PLUS_ONE];     /**< Cosinus-Transformationsfunktionen
                                                          für verschiedene Zweierpotenzen */
  static unsigned int _use_count;                    /**< Anzahl der aktuellen Objekte,
                                                          die diese statischen Puffer benutzen */
  //@}

  //@{
  void _transform_all(const T *, unsigned int, T *);
  void _detransform_all(const T *, unsigned int, T *);
  //@}

  COMMDCTT() {}; // Default-Konstruktor soll nicht aufgerufen werden!
};

//---------------------------------------------------------------

template <class T> T *COMMDCTT<T>::_sin_win_buffer[MDCT_MAX_EXP2_PLUS_ONE];
template <class T> T *COMMDCTT<T>::_cos_buffer[MDCT_MAX_EXP2_PLUS_ONE];
template <class T> unsigned int COMMDCTT<T>::_use_count = 0;

//---------------------------------------------------------------

/**
   Konstruktor

   Prüft zuerst die angegebene Dimension, da nur bestimmte
   Zweierpotenzen gültig sind. Dann wird geprüft, ob die nötigen
   Funktionspuffer bereits existieren. Wenn nicht, werden sie
   angelegt.
   
   \param dim Dimension der Einzel-DCTs
   \param acc Genauigkeit der Einzel-DCTs
*/

template <class T>
COMMDCTT<T>::COMMDCTT(unsigned int dim, double acc)
{
  unsigned int i, j;
  double log2;
  stringstream err;

  _dim = 0;
  _exp2 = 0;
  _output = 0;
  _last_tail = 0;
  _first = true;
  _last_length = 0;

  _accuracy = acc;

  // Dimension muss Potenz von 2 sein!
  log2 = logb(dim) / logb(2);
  if (log2 != (unsigned int) log2)
  {
    err << "invalid dimension " << dim;
    err << " (must be power of 2)!";
    throw ECOMMDCT(err.str());
  }

  // Dimension muss im gültigen Bereich liegen!
  if ((unsigned int) log2 < MDCT_MIN_EXP2 ||
      (unsigned int) log2 >= MDCT_MAX_EXP2_PLUS_ONE)
  {
    err << "invalid dimension " << dim;
    err << " (out of valid range)!";
    throw ECOMMDCT(err.str());
  }

  // Dimension ok
  _dim = dim;
  _exp2 = (unsigned int) log2;
  
  try
  {
    // Persistenten Speicher für letzte, halbe Dimension allokieren
    _last_tail = new T[_dim / 2];
  }
  catch (...)
  {
    throw ECOMMDCT("could not allocate memory for tail buffer!");
  }

  // Persistenten Speicher leeren
  clear();

  // Erster Aufruf: Globale Puffer initialisieren
  if (_use_count++ == 0)
  {
#ifdef MDCT_DEBUG
    cout << "init global buffers" << endl;
#endif

    for (i = MDCT_MIN_EXP2; i < MDCT_MAX_EXP2_PLUS_ONE; i++)
    {
      _sin_win_buffer[i] = 0;
      _cos_buffer[i] = 0;
    }
  }

  if (!_sin_win_buffer[_exp2]) // Fenster- und Cos-Funktion noch nicht erzeugt
  {
#ifdef MDCT_DEBUG
    cout << "creating buffers for dimension " << _dim << endl;
#endif

    // Puffer für Sinus-Fensterfunktion erzeugen und initialisieren
    try
    {
      _sin_win_buffer[_exp2] = new T[_dim];
    }
    catch (...)
    {
      throw ECOMMDCT("could not allocate memory for sine window buffer!");
    }

    for (i = 0; i < _dim; i++)
    {
      _sin_win_buffer[_exp2][i] = sin(M_PI * (i + 0.5) / _dim);
    }

    // Puffer mit Cosinus-Koeffizienten erzeugen und initialisieren
    try
    {
      _cos_buffer[_exp2] = new T[_dim * _dim / 2];
    }
    catch (...)
    {
      throw ECOMMDCT("could not allocate memory for cosine buffer!");
    }

    for (i = 0; i < _dim / 2; i++)
    { 
      for (j = 0; j < _dim; j++)
      {
        _cos_buffer[_exp2][i * _dim + j] = cos(M_PI * (2.0 * j + _dim / 2 + 1.0)
                                               * (2.0 * i + 1.0) / 2.0 / _dim);
      }
    }
#ifdef MDCT_DEBUG
    cout << "buffers created and initialized" << endl;
#endif
  }
}

//---------------------------------------------------------------

/**
   Destruktor

   Löscht die allokierten Speicherbereiche.

   Wenn das aktuelle Objekt das "letzte seiner Art" ist,
   löscht es auch alle allokierten Funktionspuffer.
*/

template <class T>
COMMDCTT<T>::~COMMDCTT()
{
  unsigned int i;

  if (--_use_count == 0)
  {
#ifdef MDCT_DEBUG
    cout << "cleaning global buffers" << endl;
#endif

    for (i = MDCT_MIN_EXP2; i < MDCT_MAX_EXP2_PLUS_ONE; i++)
    {
      if (_sin_win_buffer[i])
      {
#ifdef MDCT_DEBUG
        cout << "cleaning sin " << i << endl;
#endif

        delete [] _sin_win_buffer[i];
        _sin_win_buffer[i] = (T *) 0;
      }

      if (_cos_buffer[i])
      {
#ifdef MDCT_DEBUG
        cout << "cleaning cos " << i << endl;
#endif

        delete [] _cos_buffer[i];
        _cos_buffer[i] = (T *) 0;
      }
    }
  }

  if (_last_tail) delete [] _last_tail;
  if (_output) delete [] _output;
}

//---------------------------------------------------------------

/**
   Sorgt dafür, dass der persistente, letzte Teil der letzten
   MDCT gelöscht (mit Nullen ersetzt) wird.
*/

template <class T>
void COMMDCTT<T>::clear()
{
  unsigned int i;

#ifdef MDCT_DEBUG
  cout << "LAST TAIL CLEAR!" << endl;
#endif

  _first = true;
  _last_length = 0;
  _last_value = 0;

  if (!_last_tail) return;

  for (i = 0; i < _dim / 2; i++)
  {
    _last_tail[i] = 0;
  }
}

//---------------------------------------------------------------

/**
   Führt eine MDCT aus

   Prüft, ob die Eingabewerte auf eine ganze Anzahl von Blöcken
   der Dimension aufgefüllt werden muss. Allokiert dann den
   entsprechenden Puffer, differenziert die Eingabewerte
   und kopiert sie in diesen Puffer. Führt dann alle nötigen
   Einzel-DCTs aus und kopiert das Ergebnis in den Augabe-Puffer.

   \param input Konstanter Zeiger auf einen Puffer mit Werten,
   die transformiert werden sollen
   \param input_length Anzahl der Werte im Eingabepuffer
   \throw ECOMMDCT Es konnte nicht genug Speicher allokiert werden
*/

template <class T>
void COMMDCTT<T>::transform(const T *input, unsigned int input_length)
{
  unsigned int i, blocks_of_dim;
  T *mdct_buffer;
  T mean;
#ifndef MDCT_NO_DIFF
  T diff_value;
  T tmp_value;
#endif

#ifdef MDCT_DEBUG
  cout << "mdct::transform() len=" << input_length << " dim=" << _dim << endl;
#endif

  _output_length = 0;

  if (!_dim) return; // Division durch 0 verhindern
  if (!input_length) return; // Keine Daten - Keine MDCT!

  // Ist die Anzahl der Eingabewerte ein Vielfaches der Dimension
  if (input_length % _dim == 0)
  {
    // Prima! Alles geht auf.
    blocks_of_dim = input_length / _dim;
  }
  else
  {
    // Nein: Die nächstgrößere ganzzahlige Blockgröße benutzen,
    // um später mit Nullen aufzufüllen
    blocks_of_dim = (input_length / _dim) + 1;
  }

  if (_output)
  {
    // Den alten Ausgabepuffer freigeben
    delete [] _output;
    _output = 0;
  }
    
  try
  {
    // Speicher für den Ausgabepuffer allokieren
    // Größe: blocks_of_dim * 2 DCTs, die jeweils _dim / 2 Koeffizienten liefern
    // plus ein Mittelwert am Anfang
    _output = new T[1 + blocks_of_dim * _dim];

    // Speicher für den MDCT-Puffer allokieren.
    // Dabei eine Halbe Dimension mehr für den "Übertrag"
    // der letzten MDCT einplanen
    mdct_buffer = new T[_dim / 2 + blocks_of_dim * _dim];
  }
  catch (...)
  {
    throw ECOMMDCT("could not allocate memory for buffers!");
  }

  if (_first)
  {
    // Vorne mit erstem Wert der Daten anfüllen
    for (i = 0; i < _dim / 2; i++)
    {
      mdct_buffer[i] = input[0];
    }
  }
  else
  {
    // Rest des letzten Blockes vorne anfügen
    for (i = 0; i < _dim / 2; i++)
    {
      mdct_buffer[i] = _last_tail[i];
    }
  }

  // Daten in den MDCT-Puffer kopieren
  for (i = 0; i < input_length; i++)
  {
    mdct_buffer[_dim / 2 + i] = input[i];
  }

  // Eventuell hinten auffüllen (mit dem letzten Datenwert)
  for (i = input_length; i < blocks_of_dim * _dim; i++)
  {
    mdct_buffer[_dim / 2 + i] = input[input_length - 1];
  }

  // Mittelwert berechnen
  mean = 0;
  for (i = 0; i < blocks_of_dim * _dim; i++)
  {
    mean += mdct_buffer[i];
  }
  mean /= (blocks_of_dim * _dim);

  // Die letzte, halbe Dimension von Werten (undifferenziert) speichern
  for (i = 0; i < _dim / 2; i++)
  {
    _last_tail[i] = mdct_buffer[blocks_of_dim * _dim + i];
  }

#ifndef MDCT_NO_DIFF
  if (_first)
  {
    diff_value = mdct_buffer[i];
  }
  else
  {
    diff_value = _last_value;
  }

#if 0
#ifdef MDCT_DEBUG
  cout << "buffer before diff:" << endl;
  for (i = 0; i < blocks_of_dim * _dim; i++) cout << mdct_buffer[i] << ", ";
  cout << endl;
#endif
#endif

  // Differentieren
  for (i = 0; i < blocks_of_dim * _dim; i++)
  {
    tmp_value = mdct_buffer[i];
    mdct_buffer[i] -= diff_value;
    diff_value = tmp_value;
  }

#if 0
#ifdef MDCT_DEBUG
  cout << "buffer after diff:" << endl;
  for (i = 0; i < blocks_of_dim * _dim; i++) cout << mdct_buffer[i] << ", ";
  cout << endl;
#endif
#endif

  // Den Differentierungswert dieser Stelle für die Nachwelt speichern
  _last_value = diff_value;

  // Weiter differentieren
  for (i = 0; i < _dim / 2; i++)
  {
    tmp_value = mdct_buffer[blocks_of_dim * _dim + i];
    mdct_buffer[blocks_of_dim * _dim + i] -= diff_value;
    diff_value = tmp_value;
  }
#endif

  // Den Mittelwert an der ersten Stelle des Ausgabepuffers speichern
  _output[0] = mean;

#ifdef MDCT_DEBUG
  cout << "mdct buffer before transform:" << endl;
  for (i = 0; i < blocks_of_dim * _dim; i++) cout << mdct_buffer[i] << ", ";
  cout << endl;
#endif

  // DCT's ausführen
  _transform_all(mdct_buffer, blocks_of_dim * 2, _output + 1);

#ifdef MDCT_DEBUG
  cout << "output buffer after transform:" << endl;
  for (i = 0; i < 1 + blocks_of_dim * _dim; i++) cout << _output[i] << ", ";
  cout << endl;
#endif

  // Alles ok, der Ergebnispuffer ist jetzt gefüllt.
  _output_length = 1 + blocks_of_dim * _dim;

  _first = false;
  _last_length = input_length;

  // MDCT-Puffer wieder freigeben
  delete [] mdct_buffer;
}

//---------------------------------------------------------------

/**
   Führt alle nötigen Einzel-DCTs aus

   Die Ergebnisvektoren werden sortiert in den Ausgabepuffer
   gespeichert, und zwar so, dass alle ersten Werte vorne
   stehen, dann alle zweiten Werte, usw.

   \param input Konstanter Zeiger auf den Puffer mit den
   zu transformiernden Werten
   \param dct_count Anzahl auszuführender DCTs
   \param output Zeiger auf den Ausgabepuffer
*/

template <class T>
void COMMDCTT<T>::_transform_all(const T *input,
                                 unsigned int dct_count,
                                 T *output)
{
  unsigned int i, j, d;
  double abs;
  T value;
  
#ifdef MDCT_DEBUG
  unsigned int elim_count = 0;
#endif

#ifdef MDCT_OMEGA
  double error_sum;
  bool found_first_non_zero;
  T smallest, omega;
  unsigned int index_of_smallest;
#endif
  
  // Alle DCTs durchführen
  for (d = 0; d < dct_count; d++)
  {
    // Jeden Einzelwert berechnen
    for (i = 0; i < _dim / 2; i++)
    {
      value = 0;
      
      for (j = 0; j < _dim; j++)
      {
        value += _sin_win_buffer[_exp2][j]
          * input[_dim / 2 * d + j] * _cos_buffer[_exp2][i * _dim + j];
      }

      if (i > 1)
      {
        // Alles kleiner als _accuracy zu 0 setzen
        abs = (double) value;
        if (abs < 0) abs *= -1;
        if (abs <= _accuracy)
        {
          value = 0;
#ifdef MDCT_DEBUG
          elim_count++;
#endif
        }
      }

      output[d + i * dct_count] = value;
    }

#ifdef MDCT_OMEGA
    if (_accuracy < 0)
    {
      error_sum = 0;

      while (1)
      {
        i = 3;
        found_first_non_zero = false;

        while (!found_first_non_zero && i < _dim / 2)
        {
          value = output[d + i * dct_count];

          if (value != 0)
          {
            smallest = value;
            index_of_smallest = i;
            found_first_non_zero = true;
          }

          i++;
        }

        if (!found_first_non_zero) break;

        while (i < _dim / 2)
        {
          value = output[d + i * dct_count];

          if (value != 0 && value < smallest)
          {
            smallest = value;
            index_of_smallest = i;
          }

          i++;
        }

        omega = M_PI * (2 * index_of_smallest + 1) / (double) _dim;

        if (error_sum + smallest / omega > -_accuracy) break;

        error_sum += smallest / omega;
        output[d + index_of_smallest * dct_count] = 0;

#ifdef MDCT_DEBUG
        elim_count++;
#endif
      }
    }
#endif
  }

#ifdef MDCT_DEBUG
  cout << "compression efficency: " << elim_count;
  cout << " / " << dct_count * _dim / 2;
  cout << " (" << elim_count / (double) dct_count / _dim * 2 * 100 << "%)";
  cout << " with accuracy " << _accuracy << endl;
#endif
}

//---------------------------------------------------------------

/**
   Führt die letzte DCT aus

   \throw ECOMMDCT Es konnte nicht genug Speicher allokiert werden
*/

template <class T>
void COMMDCTT<T>::flush_transform()
{
  unsigned int i;
  T *mdct_buffer;
  T mean;
#ifndef MDCT_NO_DIFF
  T diff_value, tmp_value;
#endif

#ifdef MDCT_DEBUG
  cout << "mdct::flush_transform() dim=" << _dim << " last_len=" << _last_length << endl;
#endif

  _output_length = 0;

  if (!_dim) return; // Division durch 0 verhindern

  // Wenn die letzte MDCT über weniger als _dim/2
  // Werte ging, ist ein Überhangblock unnötig.
  if ((_last_length % _dim) <= _dim / 2) return;

  if (_output)
  {
    delete [] _output;
    _output = 0;
  }
    
  try
  {
    // Speicher für den Ausgabepuffer allokieren
    _output = new T[1 + _dim / 2];

    // Speicher für den MDCT-Puffer allokieren.
    mdct_buffer = new T[_dim];
  }
  catch (...)
  {
    throw ECOMMDCT("could not allocate memory for buffers!");
  }

  // Rest des letzten Blockes vorne anfügen
  for (i = 0; i < _dim / 2; i++)
  {
    mdct_buffer[i] = _last_tail[i];
  }

  // Zweite Hälfte mit dem letzten Wert der ersten Hälfte auffüllen
  for (i = _dim / 2; i < _dim; i++)
  {
    mdct_buffer[i] = mdct_buffer[_dim / 2 - 1];
  }

  // Mittelwert berechnen
  mean = 0;
  for (i = 0; i < _dim / 2; i++)
  {
    mean += mdct_buffer[i];
  }
  mean /= (_dim / 2);

  _output[0] = mean;

#ifndef MDCT_NO_DIFF
  // Differentieren
  diff_value = _last_value;
  for (i = 0; i < _dim / 2; i++)
  {
    tmp_value = mdct_buffer[i];
    mdct_buffer[i] -= diff_value;
    diff_value = tmp_value;
  }
#endif

  // DCT ausführen
  _transform_all(mdct_buffer, 1, _output + 1);

  // Alles ok, der Ergebnispuffer ist jetzt gefüllt.
  _output_length = 1 + _dim / 2;

  // MDCT-Puffer wieder freigeben
  delete [] mdct_buffer;
}

//---------------------------------------------------------------

/**
   Führt eine MDCT-Rücktransformation aus

   \param input Konstanter Zeiger auf ein Array mit
                geordneten MDCT-Koeffizienten
   \param input_length Anzahl der Werte im input-Array
*/

template <class T>
void COMMDCTT<T>::detransform(const T *input,
                              unsigned int input_length)
{
  unsigned int blocks_of_dim, i;
  T *mdct_buffer;
  T mean, mean_diff;
#ifndef MDCT_NO_DIFF
  T int_value;
#endif
  stringstream err;

#ifdef MDCT_DEBUG
  cout << "mdct::detransform() len=" << input_length << " dim=" << _dim;
  if (_first) cout << " FIRST";
  cout << endl;
#endif

  _output_length = 0;

  if (!_dim) return; // Bei Fehler: Division durch 0 verhindern!
  if (input_length < 2) return; // Keine Daten - keine MDCT!

  // Den MDCT-Mittelwert jetzt nicht beachten
  input_length--;

  // Wenn die Datenlänge nicht durch die Dimension teilbar ist
  if (input_length % _dim)
  {
    // Auf ganze Blockzahl auffüllen
    blocks_of_dim = input_length / _dim + 1;
  }
  else
  {
    blocks_of_dim = input_length / _dim;
  }

#ifdef MDCT_DEBUG
  cout << "blocks_of_dim=" << blocks_of_dim << endl;
#endif

  if (_output)
  {
    // Den alten Ausgabepuffer freigeben
    delete [] _output;
    _output = 0;
  }
   
  try
  {
    _output = new T[blocks_of_dim * _dim];
    mdct_buffer = new T[_dim / 2 + blocks_of_dim * _dim];
  }
  catch (...)
  {
    throw ECOMMDCT("could not allocate memory for buffers!");
  }

  // Die letzte, halbe Dimension der letzten
  // Rücktransformation in den Puffer kopieren
  for (i = 0; i < _dim / 2; i++)
  {
    mdct_buffer[i] = _last_tail[i];
  }

  // Den Rest auf 0 setzen
  for (i = 0; i < blocks_of_dim * _dim; i++)
  {
    mdct_buffer[_dim / 2 + i] = 0;
  }

  // Alle inversen Transformationen ausführen
  _detransform_all(input + 1, blocks_of_dim * 2, mdct_buffer);

#ifndef MDCT_NO_DIFF
  // Integrieren
  int_value = _last_value;
  for (i = 0; i < blocks_of_dim * _dim; i++)
  {
    mdct_buffer[i] += int_value;
    int_value = mdct_buffer[i];
  }  

  _last_value = int_value;
#endif

  // Mittelwert berechnen
  mean = 0;
  for (i = 0; i < blocks_of_dim * _dim; i++)
  {
    mean += mdct_buffer[i];
  }
  mean /= blocks_of_dim * _dim;

#ifndef MDCT_DONT_CORRECT_BY_MEAN
  if (_first)
  {
    mean_diff = input[0] - mean;
    _last_value += mean_diff;
  }
  else
  {
    mean_diff = 0;
  }
#else
  mean_diff = 0;
#endif

#ifdef MDCT_DEBUG
  cout << "calculated mean: " << mean << " handed mean: " << input[0] << endl;
#endif

  _output_length = blocks_of_dim * _dim;

  if ((input_length % _dim) > 0 && (input_length % _dim) < _dim / 2)
  {
    _output_length -= _dim / 2 - (input_length % _dim);
  }

  // Daten in den Ausgabepuffer kopieren
  if (_first)
  {
    _output_length -= _dim / 2;

    for (i = 0; i < _output_length; i++)
    {
      _output[i] = mdct_buffer[_dim / 2 + i] +  mean_diff;
    }
  }
  else
  {
    for (i = 0; i < _output_length; i++)
    {
      _output[i] = mdct_buffer[i] + mean_diff;
    }
  }

#ifdef MDCT_DEBUG
  //for (i = 0; i < _output_length; i++) if ((i % (_dim / 2)) == 0) _output[i] = 300;
  cout << "output_length=" << _output_length << endl;
#endif

  // Daten der letzten, halben Dimension speichern
  for (i = 0; i < _dim / 2; i++)
  {
    _last_tail[i] = mdct_buffer[blocks_of_dim * _dim + i];
  }

#ifdef MDCT_DEBUG
  cout << "deleting mdct buffer." << endl;
#endif

  delete [] mdct_buffer;

  _first = false;
  _last_length = input_length;
}

//---------------------------------------------------------------

template <class T>
void COMMDCTT<T>::_detransform_all(const T *input,
                                   unsigned int dct_count,
                                   T *output)
{
  unsigned int i, m, d;
  T value;

  // Alle inversen DCTs durchführen
  for (d = 0; d < dct_count; d++)
  {
    for (i = 0; i < _dim; i++)
    {
      value = 0;

      for (m = 0; m < _dim / 2; m++)
      {
        value += input[m * dct_count + d] * _cos_buffer[_exp2][i + m * _dim];
      }

      output[d * _dim / 2 + i] += value * _sin_win_buffer[_exp2][i] * 4 / _dim;
      
#ifdef MDCT_DEBUG
      //if (i == 0) output[d * _dim / 2 + i] = 0;
#endif
    }
  }
}

//---------------------------------------------------------------

/**
   Führt die letzte Rück-DCT über dem Blockrest aus

   \param input _dim / 2 Werte für die letzte Rück-DCT
*/

template <class T>
void COMMDCTT<T>::flush_detransform(const T *input)
{
  unsigned int i;
  T *mdct_buffer;
  stringstream err;
#ifndef MDCT_NO_DIFF
  T int_value;
#endif

#ifdef MDCT_DEBUG
  cout << "mdct::flush_detransform() dim=" << _dim << " last_len=" << _last_length << endl;
#endif

  _output_length = 0;

  if (!_dim) return; // Bei Fehler: Division durch 0 verhindern!

  // Wenn die letzte DCT über weniger als _dim / 2 Werte ging,
  // gibt es keinen Überhangblock.
  if ((_last_length % _dim) <= _dim / 2) return;

  if (_output)
  {
    delete [] _output;
    _output = 0;
  }
   
  try
  {
    _output = new T[_dim / 2];
    mdct_buffer = new T[_dim];
  }
  catch (...)
  {
    throw ECOMMDCT("could not allocate memory for buffers!");
  }

  // Die letzte, halbe Dimension der letzten
  // Rücktransformation in den Puffer kopieren
  for (i = 0; i < _dim / 2; i++)
  {
    mdct_buffer[i] = _last_tail[i];
  }

  // Den Rest mit Nullen auffüllen
  for (i = 0; i < _dim / 2; i++)
  {
    mdct_buffer[_dim / 2 + i] = 0;
  }

  // Die letzte Rücktransformation ausführen
  _detransform_all(input + 1, 1, mdct_buffer);

#ifndef MDCT_NO_DIFF
  int_value = _last_value;
  for (i = 0; i < _dim / 2; i++)
  {
    mdct_buffer[i] += int_value;
    int_value = mdct_buffer[i];
  } 
#endif

  // Daten in den Ausgabepuffer kopieren
  _output_length = (_last_length % _dim) - _dim / 2;
  for (i = 0; i < _output_length; i++)
  {
    _output[i] = mdct_buffer[i];

#ifdef MDCT_DEBUG
    //if (i == 0) _output[i] = 200;
#endif
  }

#ifdef MDCT_DEBUG
  cout << "flush output_len=" << _output_length << endl;
#endif
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

template <class T>
unsigned int COMMDCTT<T>::block_size() const
{
  return _dim;
}

//---------------------------------------------------------------

#endif
