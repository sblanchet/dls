//---------------------------------------------------------------
//
//  D L S _ S A V E R _ G E N _ T . H P P
//
//---------------------------------------------------------------

#ifndef DLSSaverGenTHpp
#define DLSSaverGenTHpp

//---------------------------------------------------------------

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <list>
#include <string>
using namespace std;

//---------------------------------------------------------------

#include "dls_saver_t.hpp"
#include "dls_saver_meta_t.hpp"

//---------------------------------------------------------------

//#define DEBUG

//---------------------------------------------------------------

/**
   Abstrakte Basisklasse von DLSSaverGenT

   Da DLSSaverGenT eine Template-Klasse ist und nicht mehrere
   unterschiedliche Instanzen dieser Template-Klasse in einer
   Liste verwaltet werden können, wird stattdessen eine Liste
   mit Zeigern auf diese abstrakte Basisklasse verwendet,
   die bereits alle wichtigen Methoden kennt.
*/

class DLSSaverGen
{
public:
  DLSSaverGen() {};
  virtual ~DLSSaverGen() {};

  /**
     Fügt einen Meta-Saver für einen bestimmten Meta-Typ hinzu

     \param type Meta-Typ
   */

  virtual void add_meta_saver(DLSMetaType type) = 0;

  /**
     Schreibt alle wartenden Daten ins Dateisystem
   */

  virtual void flush() = 0;

  /**
     Nimmt einen Puffer voller Binärdaten entgegen

     \param buffer Adresse des Puffers mit den zu speichernden Daten
     \param length Anzahl der Datenwerte
     \param time_of_last Zeit des letzten Datenwertes
     \throw EDLSSaver Fehler beim Verarbeiten der Daten
     \throw EDLSTimeTolerance Zeit-Toleranzfehler!
   */

  virtual void process_data(const void *buffer,
                            unsigned int length,
                            COMTime time_of_last) = 0;
};

//---------------------------------------------------------------

/**
   Speichern von generischen Daten

   \todo DOC
*/

template <class T>
class DLSSaverGenT : public DLSSaverGen, public DLSSaverT<T>
{
public:
  DLSSaverGenT(DLSLogger *);
  virtual ~DLSSaverGenT();

  void add_meta_saver(DLSMetaType type);
  void process_data(const void *, unsigned int, COMTime);
  void flush();

private:
  list<DLSMetaType> _meta_types;         /**< Liste der zu erfassenden Meta-Typen */
  list<DLSSaverMetaT<T> *> _meta_savers; /**< Liste der aktiven Meta-Saver */
  bool _savers_created;                  /**< Wurden bereits alle Meta-Saver erstellt? */
  bool _finished;                        /**< true, wenn keine Daten mehr im Speicher */

  void _fill_buffers(const T *, unsigned int, COMTime);

  //@{
  void _create_savers();
  void _generate_meta_data();
  void _flush_savers();
  void _clear_savers();
  //@}

  int _meta_level() const;
  string _meta_type() const;

  DLSSaverGenT(); // Default-Konstruktor privat: Darf nicht aufgerufen werden!
};

//---------------------------------------------------------------

/**
   Konstruktor

   \param parent_logger Zeiger auf das besitzende Logger-Objekt
*/

template <class T>
DLSSaverGenT<T>::DLSSaverGenT(DLSLogger *parent_logger)
  : DLSSaverT<T>(parent_logger)
{
  _savers_created = false;
  _finished = true;
}

//---------------------------------------------------------------

/**
   Destruktor

   Gibt eine Warnung aus, wenn noch Daten im Speicher waren
*/

template <class T>
DLSSaverGenT<T>::~DLSSaverGenT()
{
#if 0
  if (!_finished)
  {
    msg() << "saver_gen not finished!";
    log(DLSWarning);
  }
#endif

  // Saver löschen
  _clear_savers();
}

//---------------------------------------------------------------

/**
   Teilt dem Saver mit, dass Meta-Daten eines
   bestimmten Typs angelegt werden sollen.

   Fügt der Liste der zu erzeugenden Meta-Typen
   einen Typen hinzu. Die entsprechenden
   Meta-Saver werden bei Bedarf in der Methode
   _generate_meta_data() erstellt.

   \param type Typ der zu erstellenden Meta-Daten
*/

template <class T>
void DLSSaverGenT<T>::add_meta_saver(DLSMetaType type)
{
  _meta_types.push_back(type);
  _savers_created = false;
}

//---------------------------------------------------------------

/**
   Nimmt Binärdaten zum Speichern entgegen

   Diese Methode führt Plausiblitätsprüfungen (Anzahl der
   Werte / Größe des Puffers / vergangene Zeit seit dem
   letzten Datenwert) durch, um die Daten
   schliesslich als Array vom Typ T an die Methode
   _fill_buffers() weiterzuleiten.

   Wird der zeitliche Toleranzbereich verletzt,
   wird eine Exception geworfen. Der Prozess sollte
   dann beendet werden.

   \param buffer Adresse des Datenpuffers
   \param size Anzahl der Bytes im Puffer
   \param time_of_last Zeit des letzten Datenwertes im Puffer
   \throw EDLSSaver Fehler beim Speichern der Daten
   \throw EDLSTimeTolerance Toleranzfehler! Prozess beenden!
*/

template <class T>
void DLSSaverGenT<T>::process_data(const void *buffer,
                                   unsigned int size,
                                   COMTime time_of_last)
{
  COMTime diff_time, time_of_first, actual_diff, target_diff;
  float error_percent;
  double freq = _parent_logger->channel_preset()->sample_frequency;
  unsigned int values_in_buffer;
  stringstream err;

  if (size == 0) return;

  // Die Länge des Datenblocks muss ein Vielfaches der Datengröße sein!
  if (size % sizeof(T) != 0)
  {
    throw EDLSSaver("illegal data size!");
  }

  values_in_buffer = size / sizeof(T);

  diff_time.from_dbl_time((values_in_buffer - 1) / freq);
  time_of_first = time_of_last - diff_time; // Zeit des ersten neuen Wertes

  // Wenn Werte in den Puffern sind
  if (_block_buf_index || _meta_buf_index)
  {
    // Zeitabstände errechnen
    target_diff.from_dbl_time(1 / freq);         // Erwarteter Zeitabstand
    actual_diff = time_of_first - _time_of_last; // Tatsächlicher Zeitabstand

    // Relativen Fehler errechnen
    error_percent = (actual_diff.to_dbl() - target_diff.to_dbl()) / target_diff.to_dbl() * 100;
    if (error_percent < 0) error_percent *= -1;

    // Toleranzbereich verletzt?
    if (error_percent > ALLOWED_TIME_VARIANCE)
    {
      // Fehler! Prozess beenden!
      err << "time diff of " << actual_diff;
      err << " (expected: " << target_diff << ", error: " << error_percent << "%)";
      err << " channel \"" << _parent_logger->channel_preset()->name << "\".";
      throw EDLSTimeTolerance(err.str());
    }
  }

  // Daten speichern
  _fill_buffers((T *) buffer, values_in_buffer, time_of_first);
}

//---------------------------------------------------------------

/**
   Speichern der Daten im Block- und Meta-Puffer

   Die Datenwerte werden einzeln in den Block-
   und Meta-Puffer geschoben. Läuft dabei der
   Block-Puffer voll, so wird ein kompletter Block
   in das Dateisystem geschrieben und der Puffer geleert.
   Läuft der Meta-Puffer voll, so werden alle
   Meta-Saver angewiesen, aus den vorhandenen Daten ihre
   Meta-Werte zu generieren. Danach wird der Puffer
   geleert.

   \param buffer Adresse des Datenpuffers
   \param length Anzahl der Datenwerte im Puffer
   \param time_of_first Zeit des ERSTEN Datenwertes im Puffer
*/

template <class T>
void DLSSaverGenT<T>::_fill_buffers(const T *buffer,
                                    unsigned int length,
                                    COMTime time_of_first)
{
  COMTime time_of_one;
  double freq = _parent_logger->channel_preset()->sample_frequency;

  time_of_one.from_dbl_time(1 / freq); // Zeit eines Wertes

  // Ab jetzt sind Werte im Speicher!
  _finished = false;

  // Alle Werte übernehmen
  for (unsigned int i = 0; i < length; i++)
  {
    // Zeit des zuletzt eingefügten Wertes setzen
    _time_of_last = time_of_first + time_of_one * i;

    // Bei Blockanfang, Zeiten vermerken
    if (_block_buf_index == 0) _block_time = _time_of_last;
    if (_meta_buf_index == 0) _meta_time = _time_of_last;

    // Wert in die Puffer übernehmen
    _block_buf[_block_buf_index++] = buffer[i];
    _meta_buf[_meta_buf_index++] = buffer[i];

    // Block-Puffer voll?
    if (_block_buf_index == _block_buf_size) _save_block();

    // Meta-Puffer voll?
    if (_meta_buf_index == _meta_buf_size) _generate_meta_data();
  }
}

//---------------------------------------------------------------
 
/**
   Alle wartenden Daten in's Dateisystem schreiben

   Auch wenn noch nicht genug Daten für einen
   vollständigen Block im Block-Puffer sind, werden
   diese in's Dateisystem geschrieben und der Puffer
   anschließend geleert. Das gleiche passiert mit den wartenden
   Meta-Daten: Carries werden erstellt und die Meta-Puffer geleert.

   \throws EDLSSaver Fehler beim Speichern
*/

template <class T>
void DLSSaverGenT<T>::flush()
{
  // Blockdaten speichern
  _save_block();

  // Eventuell restliche Daten des Kompressionsobjektes speichern
  _save_rest();

#ifdef DEBUG
  cout << "DLSSaverGenT: _finish_files() for channel " << _parent_logger->channel_preset()->name << endl;
#endif

  // Dateien beenden
  _finish_files();

#ifdef DEBUG
  cout << "DLSSaverGenT: _compression_clear()" << endl;
#endif

  // Persistenten Speicher des Kompressionsobjekt leeren
  _compression->clear();

#ifdef DEBUG
  cout << "DLSSaverGenT: _flush_savers()" << endl;
#endif

  // Metadaten speichern
  _flush_savers();

#ifdef DEBUG
  cout << "DLSSaverGenT: _clear_savers()" << endl;
#endif

  // Alle Saver beenden
  _clear_savers();

#ifdef DEBUG
  cout << "DLSSaverGenT: flush finished!" << endl;
#endif

  // Jetzt ist nichts mehr im Speicher
  _finished = true;
}

//---------------------------------------------------------------

/**
   Meta-Daten generieren

   Diese Methode wird aufgerufen, wenn der Meta-Puffer
   voll ist. Wenn noch keine Meta-Saver erstellt wurden,
   passiert dies nun (vorrausgesetzt es sind welche gewünscht).
   Alle Meta-Saver bekommen Gelegenheit, aus dem vollen
   Meta-Puffer ihre Meta-Werte zu generieren.
   Danach wird dieser geleert.

   \throw EDLSSaver Fehler beim Speichern
*/

template <class T>
void DLSSaverGenT<T>::_generate_meta_data()
{
  typename list<DLSSaverMetaT<T> *>::iterator meta_i;

  // Wenn Meta-Saver noch nicht existieren - erzeugen
  if (!_savers_created) _create_savers();

  // Meta-Daten generieren
  meta_i = _meta_savers.begin();
  while (meta_i != _meta_savers.end())
  {
    (*meta_i)->generate_meta_data(_meta_time, _time_of_last,
                                  _meta_buf_index, _meta_buf);
    meta_i++;
  }

  _meta_buf_index = 0;
}

//---------------------------------------------------------------

/**
*/

template <class T>
void DLSSaverGenT<T>::_flush_savers()
{
  typename list<DLSSaverMetaT<T> *>::iterator meta_i;

  meta_i = _meta_savers.begin();
  while (meta_i != _meta_savers.end())
  {
    // Alle Daten im Speicher auf die Festplatte schreiben
    (*meta_i)->flush();
    meta_i++;
  }

  // Alle Saver haben die Restdaten verwertet. Meta-Puffer leeren.
  _meta_buf_index = 0;
}

//---------------------------------------------------------------

/**
   Meta-Saver aus den Vorgaben erzeugen

   Alle erzeugten Meta-Saver haben Meta-Level 1.

   \throw EDLSSaver Saver konnte nicht erzeugt werden
*/

template <class T>
void DLSSaverGenT<T>::_create_savers()
{
  list<DLSMetaType>::iterator meta_i;
  DLSSaverMetaT<T> *_new_saver;

  _clear_savers();

  meta_i = _meta_types.begin();
  while (meta_i != _meta_types.end())
  {
    try
    {
      _new_saver = new DLSSaverMetaT<T>(_parent_logger, *meta_i, 1);
    }
    catch (EDLSSaver &e)
    {
      throw EDLSSaver(e);
    }

    _meta_savers.push_back(_new_saver);
    meta_i++;
  }

  _savers_created = true;
}

//---------------------------------------------------------------

/**
   Alle Meta-Saver entfernen

   Entfernt nacheinander alle Meta-Saver und deren Kinder
*/

template <class T>
void DLSSaverGenT<T>::_clear_savers()
{
  typename list<DLSSaverMetaT<T> *>::iterator meta_i;

  // Meta-Saver löschen
  meta_i = _meta_savers.begin();
  while (meta_i != _meta_savers.end())
  {
    delete *meta_i;
    meta_i++;
  }

  _meta_savers.clear();
  _savers_created = false;
}

//---------------------------------------------------------------

// Doku: Siehe dls_saver_t.hpp

template <class T>
inline int DLSSaverGenT<T>::_meta_level() const
{
  return 0;
}

//---------------------------------------------------------------

// Doku: Siehe dls_saver_t.hpp

template <class T>
inline string DLSSaverGenT<T>::_meta_type() const
{
  return "gen";
}

//---------------------------------------------------------------

#endif
