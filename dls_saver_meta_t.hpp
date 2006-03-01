/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef DLSSaverMetaTHpp
#define DLSSaverMetaTHpp

/*****************************************************************************/

#include "com_time.hpp"
#include "dls_saver_t.hpp"

/*****************************************************************************/

// Beim Erweitern bitte auch die Behandlungszweige
// in "_meta_value()" und "_ending()" anpassen!

enum DLSMetaType {DLSMetaMean = 1, DLSMetaMin = 2, DLSMetaMax = 4};

/*****************************************************************************/

/**
   Saver-Objekt für Meta-Daten
*/

template <class T>
class DLSSaverMetaT : public DLSSaverT<T>
{
    using DLSSaverT<T>::_block_buf;
    using DLSSaverT<T>::_block_buf_index;
    using DLSSaverT<T>::_block_buf_size;
    using DLSSaverT<T>::_block_time;
    using DLSSaverT<T>::_meta_buf;
    using DLSSaverT<T>::_meta_buf_index;
    using DLSSaverT<T>::_meta_buf_size;
    using DLSSaverT<T>::_meta_time;
    using DLSSaverT<T>::_time_of_last;
    using DLSSaverT<T>::_parent_logger;
    using DLSSaverT<T>::_compression;
    using DLSSaverT<T>::_save_rest;
    using DLSSaverT<T>::_finish_files;
    using DLSSaverT<T>::_save_block;

public:
  DLSSaverMetaT(DLSLogger *, DLSMetaType, unsigned int);
  virtual ~DLSSaverMetaT();

  void generate_meta_data(COMTime, COMTime, unsigned int, const T *);
  void flush();

private:
  DLSSaverMetaT<T> *_next_saver; /**< Zeiger auf das Saver-Objekt der nächsten
				    Ebene */
  DLSMetaType _type;             /**< Typ dieses Saver-Objektes */
  bool _finished;                /**< true, wenn keine Daten mahr im
				    Speicher */
  unsigned int _level;           /**< Meta-Ebene dieses Saver-Objektes */

  void _pass_meta_data();
  T _meta_value(const T *, unsigned int);
  int _meta_level() const;
  string _meta_type() const;
};

/*****************************************************************************/

template <class T>
DLSSaverMetaT<T>::DLSSaverMetaT(DLSLogger *parent_logger,
                                DLSMetaType type,
                                unsigned int level)
  : DLSSaverT<T>(parent_logger)
{
  _next_saver = (DLSSaverMetaT<T> *) 0;
  _type = type;
  _finished = true;
  _level = level;
}

/*****************************************************************************/

template <class T>
DLSSaverMetaT<T>::~DLSSaverMetaT()
{
#if 0
  if (!_finished)
  {
    msg() << "Meta-Saver Level " << _level << ": not finished!";
    log(DLSWarning);
  }
#endif

  // Nächsten MetaSaver freigeben
  if (_next_saver) delete _next_saver;
}

/*****************************************************************************/

/**
   Generiert einen Meta-Wert aus den gegebenen Daten

   \param start_time Zeit des ersten Wertes im Quellpuffer
   \param end_time Zeit des letzten Wertes im Quellpuffer
   \param length Anzahl der Werte im Quellpuffer
   \param buffer Quellpuffer
*/

template <class T>
void DLSSaverMetaT<T>::generate_meta_data(COMTime start_time,
                                          COMTime end_time,
                                          unsigned int length,
                                          const T *buffer)
{
  if (length == 0) return;

  // Ab jetzt sind Daten im Speicher
  _finished = false;

  // Meta-Wert erzeugen
  T meta_value = _meta_value(buffer, length);

  // Zeit der Anfänge der ersten Werte in den Puffern vermerken
  if (_block_buf_index == 0) _block_time = start_time;
  if (_meta_buf_index == 0) _meta_time = start_time;

  // Zeit des Endwertes im letzten Metawert vermerken
  _time_of_last = end_time;

  // Wert in die Puffer übernehmen
  _block_buf[_block_buf_index++] = meta_value;
  _meta_buf[_meta_buf_index++] = meta_value;

  // Block-Puffer voll?
  if (_block_buf_index == _block_buf_size) _save_block();

  // Meta-Puffer voll?
  if (_meta_buf_index == _meta_buf_size) _pass_meta_data();

  if (_block_buf_index == 0 && _meta_buf_index == 0) _finished = true;
}

/*****************************************************************************/

template <class T>
T DLSSaverMetaT<T>::_meta_value(const T *buffer, unsigned int length)
{
  T meta_val;

  if (_type == DLSMetaMean)
  {
    double sum = 0;
    for (unsigned int i = 0; i < length; i++)
    {
      sum += buffer[i];
    }
    meta_val = (T) (sum / length);
  }

  else if (_type == DLSMetaMin)
  {
    T min = buffer[0];
    for (unsigned int i = 1; i < length; i++)
    {
      if (buffer[i] < min) min = buffer[i];
    }
    meta_val = min;
  }

  else if (_type == DLSMetaMax)
  {
    T max = buffer[0];
    for (unsigned int i = 1; i < length; i++)
    {
      if (buffer[i] > max) max = buffer[i];
    }
    meta_val = max;
  }

  return meta_val;
}

/*****************************************************************************/

template <class T>
void DLSSaverMetaT<T>::_pass_meta_data()
{
  // Wenn noch kein nächster Saver existiert - erzeugen!
  if (!_next_saver)
  {
    _next_saver = new DLSSaverMetaT<T>(_parent_logger,
                                       _type,
                                       _level + 1);
  }

  // Daten an nächsten Saver weiterreichen
  _next_saver->generate_meta_data(_meta_time, _time_of_last,
                                  _meta_buf_index, _meta_buf);

  _meta_buf_index = 0;
}

/*****************************************************************************/

template <class T>
void DLSSaverMetaT<T>::flush()
{
  // Zuerst Blockdaten speichern
  _save_block();

  // Eventuell restliche Daten des Kompressionsobjektes speichern
  _save_rest();

  // Dateien Beenden
  _finish_files();

  // Persistenten Speicher des Kompressionsobjekt leeren
  _compression->clear();

  // Puffer sind jetzt leer
  _meta_buf_index = 0;
  _finished = true;

  // Kinder sollen auch alle flush() aufrufen
  if (_next_saver) _next_saver->flush();
}

/*****************************************************************************/

template <class T>
inline int DLSSaverMetaT<T>::_meta_level() const
{
  return _level;
}

/*****************************************************************************/

template <class T>
string DLSSaverMetaT<T>::_meta_type() const
{
  switch (_type)
  {
    case DLSMetaMean: return "mean";
    case DLSMetaMin: return "min";
    case DLSMetaMax: return "max";
    default: return "undef";
  }
}

/*****************************************************************************/

#endif
