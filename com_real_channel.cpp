//---------------------------------------------------------------
//
//  C O M _ R E A L _ C H A N N E L . C P P
//
//---------------------------------------------------------------

#include <sstream>
using namespace std;

//---------------------------------------------------------------

#include "com_real_channel.hpp"
#include "com_xml_tag.hpp"

//---------------------------------------------------------------

COMRealChannel::COMRealChannel()
{
}

//---------------------------------------------------------------

COMRealChannel::~COMRealChannel()
{
} 

//---------------------------------------------------------------

int COMRealChannel::type_size() const
{
  if (_type == TUCHAR) return 1; // TODO Schön? Ist das so?
  if (_type == TINT) return 4;
  if (_type == TUINT) return 4;
  if (_type == TULINT) return 4;
  else return 8; // TDBL
}

//---------------------------------------------------------------

void COMRealChannel::read_from_tag(const COMXMLTag *tag)
{
  string type;

  try
  {
    _name = tag->att("name")->to_str();
    _unit = tag->att("unit")->to_str();
    _index = tag->att("index")->to_int();
    _frequency = tag->att("HZ")->to_int();
    _bufsize = tag->att("bufsize")->to_int();
    _type_str = tag->att("typ")->to_str();
  }
  catch (ECOMXMLTag &e)
  {
    throw ECOMRealChannel("tag error: " + e.msg + " tag: " + e.tag);
  }
  
  if (_type_str == "TUCHAR") _type = TUCHAR;
  else if (_type_str == "TINT") _type = TINT;
  else if (_type_str == "TUINT") _type = TUINT;
  else if (_type_str == "TULINT") _type = TULINT;
  else if (_type_str == "TFLT") _type = TFLT;
  else if (_type_str == "TDBL") _type = TDBL;
  else
  {
    throw ECOMRealChannel("unknown channel type \"" + _type_str + "\"");
  }
}

//---------------------------------------------------------------
