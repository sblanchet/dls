//---------------------------------------------------------------
//
//  C O M _ R E A L _ C H A N N E L . H P P
//
//---------------------------------------------------------------

#ifndef COMRealChannelHpp
#define COMRealChannelHpp

//---------------------------------------------------------------

#include <string>
using namespace std;

//---------------------------------------------------------------

#include "com_exception.hpp"
#include "com_xml_tag.hpp"

//---------------------------------------------------------------

enum COMRealChannelType {TCHAR, TUCHAR,
                         TINT, TUINT,
                         TLINT, TULINT,
                         TFLT,
                         TDBL};

//---------------------------------------------------------------

/**
   Exception eines COMRealChannel-Objektes
*/

class ECOMRealChannel : public COMException
{
public:
  ECOMRealChannel(const string &pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

class COMRealChannel
{
public:
  COMRealChannel();
  ~COMRealChannel();

  const string &name() const;
  const string &unit() const;
  unsigned int frequency() const;
  int index() const;
  unsigned int bufsize() const;
  COMRealChannelType type() const;
  const string &type_str() const;

  int type_size() const;
  void read_from_tag(const COMXMLTag *);

private:
  string _name;
  string _unit;
  int _index;
  COMRealChannelType _type;
  string _type_str;
  unsigned int _bufsize;
  unsigned int _frequency;
};

//---------------------------------------------------------------

inline const string &COMRealChannel::name() const
{
  return _name;
}

//---------------------------------------------------------------

inline const string &COMRealChannel::unit() const
{
  return _unit;
}

//---------------------------------------------------------------

inline int COMRealChannel::index() const
{
  return _index;
}

//---------------------------------------------------------------

inline unsigned int COMRealChannel::frequency() const
{
  return _frequency;
}

//---------------------------------------------------------------

inline COMRealChannelType COMRealChannel::type() const
{
  return _type;
}

//---------------------------------------------------------------

inline const string & COMRealChannel::type_str() const
{
  return _type_str;
}

//---------------------------------------------------------------

inline unsigned int COMRealChannel::bufsize() const
{
  return _bufsize;
}

//---------------------------------------------------------------

#endif
