//---------------------------------------------------------------
//
//  C O M _ Z L I B . C P P
//
//---------------------------------------------------------------

#include <zlib.h>

#include <sstream>
using namespace std;

//---------------------------------------------------------------

#include "com_zlib.hpp"

//---------------------------------------------------------------

COMZLib::COMZLib()
{
  _out_buf = 0;
}

//---------------------------------------------------------------

COMZLib::~COMZLib()
{
  if (_out_buf) delete [] _out_buf;
}

//---------------------------------------------------------------

void COMZLib::compress(const char *src, unsigned int src_len)
{
  unsigned int out_len = src_len + 1024;
  int comp_ret;
  
  _realloc(out_len);

  comp_ret = ::compress((Bytef *) _out_buf, (uLongf *) &out_len,
                        (const Bytef *) src, src_len);

  if (comp_ret != Z_OK)
  {
    stringstream err;
    err << "error " << comp_ret;
    err << ", _out_buf=" << (unsigned long) _out_buf;
    err << ", out_len=" << out_len;
    err << ", src=" << (unsigned long) src;
    err << ", src_len=" << src_len;

    throw ECOMZLib(err.str());
  }

  _out_len = out_len;
}

//---------------------------------------------------------------

void COMZLib::uncompress(const char *src, unsigned int src_len,
                         unsigned int out_len)
{
  int uncomp_ret;

  _realloc(out_len);

  uncomp_ret = ::uncompress((Bytef *) _out_buf, (uLongf *) &out_len,
                            (const Bytef *) src, src_len);

  if (uncomp_ret != Z_OK)
  {
    stringstream err;
    err << "error " << uncomp_ret;
    throw ECOMZLib(err.str());
  }

  _out_len = out_len;
}

//---------------------------------------------------------------

void COMZLib::_realloc(unsigned int size)
{
  stringstream err;

  if (_out_buf) delete [] _out_buf;
  _out_buf = 0;

  try
  {
    _out_buf = new char[size];
  }
  catch (...)
  {
    err << "could not allocate " << size << " bytes of memory!";
    throw ECOMZLib(err.str());
  }
}

//---------------------------------------------------------------
