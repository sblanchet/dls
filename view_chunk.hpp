//---------------------------------------------------------------
//
//  V I E W _ C H U N K . H P P
//
//---------------------------------------------------------------

#ifndef ViewChunkHpp
#define ViewChunkHpp

//---------------------------------------------------------------

#include "com_exception.hpp"
#include "com_time.hpp"

//---------------------------------------------------------------

class ViewBlock;
class ViewChannel;

//---------------------------------------------------------------

/**
   Exception eines ViewChunk-Objektes
*/

class EViewChunk : public COMException
{
public:
  EViewChunk(const string &pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

class ViewChunk
{
public:
  ViewChunk();
  ~ViewChunk();
  
  void set_dir(const string &);
  void import();
  void fetch_range();
  bool fetch_data(ViewChannel *, COMTime, COMTime, unsigned int);
  void clear();

  COMTime start() const;
  COMTime end() const;

  const string &dir() const;
  unsigned int sample_frequency() const;
  unsigned int meta_reduction() const;

  const list<ViewBlock *> *blocks() const;
  const list<ViewBlock *> *min_blocks() const;
  const list<ViewBlock *> *max_blocks() const;

  unsigned int current_level() const;

private:
  string _dir;
  unsigned int _sample_frequency;
  unsigned int _meta_mask;
  unsigned int _meta_reduction;
  string _format;
  COMTime _start;
  COMTime _end;

  list<ViewBlock *> _blocks;
  list<ViewBlock *> _min_blocks;
  list<ViewBlock *> _max_blocks;
  unsigned int _level;

  void _calc_optimal_level(COMTime, COMTime, unsigned int);
  bool _load_blocks(list<ViewBlock*> *, const string &,
                    const string &,
                    ViewChannel *, COMTime, COMTime);
};
 
//---------------------------------------------------------------

inline const string &ViewChunk::dir() const
{
  return _dir;
}

//---------------------------------------------------------------

inline COMTime ViewChunk::start() const
{
  return _start;
}

//---------------------------------------------------------------

inline COMTime ViewChunk::end() const
{
  return _end;
}

//---------------------------------------------------------------

inline const list<ViewBlock *> *ViewChunk::blocks() const
{
  return &_blocks;
}

//---------------------------------------------------------------

inline const list<ViewBlock *> *ViewChunk::min_blocks() const
{
  return &_min_blocks;
}

//---------------------------------------------------------------

inline const list<ViewBlock *> *ViewChunk::max_blocks() const
{
  return &_max_blocks;
}

//---------------------------------------------------------------

inline unsigned int ViewChunk::current_level() const
{
  return _level;
}

//---------------------------------------------------------------

#endif
