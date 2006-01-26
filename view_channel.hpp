//---------------------------------------------------------------
//
//  V I E W _ C H A N N E L . H P P
//
//---------------------------------------------------------------

#ifndef ViewChannelHpp
#define ViewChannelHpp

//---------------------------------------------------------------

#include <string>
#include <list>
using namespace std;

#include "com_exception.hpp"
#include "com_time.hpp"
#include "com_ring_buffer_t.hpp"
#include "view_globals.hpp"
#include "view_chunk.hpp"

//---------------------------------------------------------------

/**
   Exception eines ViewChannel-Objektes
*/

class EViewChannel : public COMException
{
public:
  EViewChannel(const string &pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

/**
   Darstellung eines Kanals in der Anzeige
*/

class ViewChannel
{
public:
  ViewChannel();
  ~ViewChannel();
  
  void import(const string &, unsigned int, unsigned int);
  void fetch_chunks(const string &, unsigned int);
  void load_data(COMTime, COMTime, unsigned int);
  void clear();

  unsigned int index() const;
  const string &name() const;
  const string &unit() const;
  COMChannelType type() const;

  COMTime start() const;
  COMTime end() const;

  const list<ViewChunk *> *chunks() const;
  double min() const;
  double max() const;
  unsigned int blocks_fetched() const;
  unsigned int min_level_fetched() const;
  unsigned int max_level_fetched() const;

private:
  // Kanal
  unsigned int _index;  /**< MSR-Kanal-Index */
  string _name;         /**< Kanalname */
  string _unit;         /**< Einheit */
  COMChannelType _type; /**< Kanaltyp (TUINT, TDBL, usw...) */

  // Chunks
  list<ViewChunk *> _chunks; /**< Liste der Chunks in diesem Kanal */
  COMTime _range_start;      /**< Startzeit des gesamten Kanal-Zeitbereichs */
  COMTime _range_end;        /**< Endzeit des gesamten Kanal-Zeitbereiches */

  // Daten
  double _min;             /**< Kleinster, geladener Datenwert */
  double _max;             /**< Größter, geladener Datenwert */
  unsigned int _min_level; /**< Niedrigste Meta-Ebene, aus der geladen wurde */
  unsigned int _max_level; /**< Höchste Meta-Ebene, aus der geladen wurde */

  void _clear_data();
  void _calc_min_max();
};

//---------------------------------------------------------------

/**
   Liefert den MSR-Index des Kanals

   \return Index
*/

inline unsigned int ViewChannel::index() const
{
  return _index;
}

//---------------------------------------------------------------

/**
   Liefert den Namen des Kanals

   \return Kanalname
*/

inline const string &ViewChannel::name() const
{
  return _name;
}

//---------------------------------------------------------------

/**
   Liefert die Einheit des Kanals

   \return Einheit
*/

inline const string &ViewChannel::unit() const
{
  return _unit;
}

//---------------------------------------------------------------

/**
   Liefert den Typ des Kanals

   Beispiel: TUINT, definiert in com_globals.hpp

   \return Kanaltyp
*/

inline COMChannelType ViewChannel::type() const
{
  return _type;
}

//---------------------------------------------------------------

/**
   Liefert die Zeit der ersten Erfassung in diesem Kanal

   Diese wird allein duch die Informationen der existierenden
   Chunks bestimmt. Wird der erste Chunk gelöscht, ändert sie sich.
   Die Startzeit ist also mehr oder minder dynamisch.

   \return Startzeit
*/

inline COMTime ViewChannel::start() const
{
  return _range_start;
}

//---------------------------------------------------------------

/**
   Liefert die Zeit der letzten Erfassung in diesem Kanal

   Siehe start()

   \return Endzeit
*/

inline COMTime ViewChannel::end() const
{
  return _range_end;
}

//---------------------------------------------------------------

/**
   Liefert einen Zeiger auf die Liste der geladenen Chunks

   \return Konstanter Zeiger auf Chunkliste
*/

inline const list<ViewChunk *> *ViewChannel::chunks() const
{
  return &_chunks;
}

//---------------------------------------------------------------

/**
   Gibt den kleinsten, geladenen Wert zurück

   \return Kleinster Wert
*/

inline double ViewChannel::min() const
{
  return _min;
}

//---------------------------------------------------------------

/**
   Gibt den größten, geladenen Wert zurück

   \return Größter Wert
*/

inline double ViewChannel::max() const
{
  return _max;
}

//---------------------------------------------------------------

/**
   Gibt die niedrigste Meta-Ebene zurück, aus der geladen wurde

   \return Meta-Ebene
*/

inline unsigned int ViewChannel::min_level_fetched() const
{
  return _min_level;
}

//---------------------------------------------------------------

/**
   Gibt die höchste Meta-Ebene zurück, aus der geladen wurde

   \return Meta-Ebene
*/

inline unsigned int ViewChannel::max_level_fetched() const
{
  return _max_level;
}

//---------------------------------------------------------------

#endif
