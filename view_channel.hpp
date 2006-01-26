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
#include "view_chunk.hpp"
#include "view_block_t.hpp"

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
  const string &type() const;

  COMTime start() const;
  COMTime end() const;

  const list<ViewChunk> *chunks() const;
  double min() const;
  double max() const;
  unsigned int blocks_fetched() const;
  unsigned int min_level_fetched() const;
  unsigned int max_level_fetched() const;

private:
  // Kanal
  unsigned int _index;
  string _name;
  string _unit;
  string _type;

  // Chunks
  list<ViewChunk> _chunks;
  COMTime _range_start;
  COMTime _range_end;

  // Daten
  double _min, _max;
  unsigned int _min_level, _max_level;

  void _clear_data();
  void _calc_min_max();
};

//---------------------------------------------------------------

inline unsigned int ViewChannel::index() const
{
  return _index;
}

//---------------------------------------------------------------

inline const string &ViewChannel::name() const
{
  return _name;
}

//---------------------------------------------------------------

inline const string &ViewChannel::unit() const
{
  return _unit;
}

//---------------------------------------------------------------

inline const string &ViewChannel::type() const
{
  return _type;
}

//---------------------------------------------------------------

inline COMTime ViewChannel::start() const
{
  return _range_start;
}

//---------------------------------------------------------------

inline COMTime ViewChannel::end() const
{
  return _range_end;
}

//---------------------------------------------------------------

inline const list<ViewChunk> *ViewChannel::chunks() const
{
  return &_chunks;;
}

//---------------------------------------------------------------

inline double ViewChannel::min() const
{
  return _min;
}

//---------------------------------------------------------------

inline double ViewChannel::max() const
{
  return _max;
}

//---------------------------------------------------------------

inline unsigned int ViewChannel::min_level_fetched() const
{
  return _min_level;
}

//---------------------------------------------------------------

inline unsigned int ViewChannel::max_level_fetched() const
{
  return _max_level;
}

//---------------------------------------------------------------

#endif
