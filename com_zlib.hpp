//---------------------------------------------------------------
//
//  C O M _ Z L I B . H P P
//
//---------------------------------------------------------------

#ifndef COMZLibHpp
#define COMZLibHpp

//---------------------------------------------------------------

#include "com_exception.hpp"

//---------------------------------------------------------------

/**
   Exception eines COMZlib-Objektes
*/

class ECOMZLib : public COMException
{
public:
  ECOMZLib(const string &pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

class COMZLib
{
public:
  COMZLib();
  ~COMZLib();

  void compress(const char *, unsigned int);
  void uncompress(const char *, unsigned int, unsigned int);

  const char *output() const;
  unsigned int length() const;

private:
  char *_out_buf;
  unsigned int _out_len;

  void _realloc(unsigned int);
};

//---------------------------------------------------------------

inline const char *COMZLib::output() const
{
  return _out_buf;
}

//---------------------------------------------------------------

inline unsigned int COMZLib::length() const
{
  return _out_len;
}

//---------------------------------------------------------------

#endif
