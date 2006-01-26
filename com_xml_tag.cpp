//---------------------------------------------------------------
//
//  C O M _ X M L _ T A G . C P P
//
//---------------------------------------------------------------

#include <sstream>
using namespace std;

//---------------------------------------------------------------

#include "com_xml_tag.hpp"

//---------------------------------------------------------------

COMXMLTag::COMXMLTag()
{
  clear();
}

//---------------------------------------------------------------

COMXMLTag::~COMXMLTag()
{
}

//---------------------------------------------------------------

void COMXMLTag::clear()
{
  _title.clear();
  _atts.clear();
  _type = dxttSingle;
}

//---------------------------------------------------------------

void COMXMLTag::title(const string &title)
{
  _title = title;
}

//---------------------------------------------------------------

void COMXMLTag::type(COMXMLTagType type)
{
  _type = type;
}

//---------------------------------------------------------------

const COMXMLAtt *COMXMLTag::att(const string &name) const
{
  list<COMXMLAtt>::const_iterator iter = _atts.begin();

  // Liste der Attribute durchsuchen
  while (iter != _atts.end())
  { 
    if (iter->name() == name) return &(*iter);
    iter++;
  }

  throw ECOMXMLTag("attribute \"" + name + "\" does not exist!", tag());
}

//---------------------------------------------------------------

bool COMXMLTag::has_att(const string &name) const
{
  list<COMXMLAtt>::const_iterator iter = _atts.begin();

  // Liste der Attribute durchsuchen
  while (iter != _atts.end())
  { 
    if (iter->name() == name) return true;
    iter++;
  }

  return false;
}

//---------------------------------------------------------------

void COMXMLTag::push_att(const string &name, const string &value)
{
  _atts.push_back(COMXMLAtt(name, value));
}

//---------------------------------------------------------------

void COMXMLTag::push_att(const string &name, int value)
{
  stringstream str;
  str << value;
  _atts.push_back(COMXMLAtt(name, str.str()));
}

//---------------------------------------------------------------

int COMXMLTag::att_count() const
{
  return _atts.size();
}

//---------------------------------------------------------------

string COMXMLTag::tag() const
{
  string str;
  list<COMXMLAtt>::const_iterator iter;

  str = "<";

  if (_type == dxttEnd) str += "/";

  str += _title;

  iter = _atts.begin();
  while (iter != _atts.end())
  {
    str += " " + iter->name() + "=\"" + iter->to_str() + "\"";
    iter++;
  }

  if (_type == dxttSingle) str += "/";

  str += ">";

  return str;
}

//---------------------------------------------------------------

const string & COMXMLAtt::to_str() const
{
  return _value;
}

//---------------------------------------------------------------

int COMXMLAtt::to_int() const
{
  int i;
  stringstream str;
  str.exceptions(stringstream::failbit | stringstream::badbit);
  str << _value;

  try
  {
    str >> i;
  }
  catch (...)
  {
    throw ECOMXMLTag("\"" + _value + "\" is no integer!", "");
  }

  return i;
}

//---------------------------------------------------------------

double COMXMLAtt::to_dbl() const
{
  double d;
  stringstream str;
  str.exceptions(stringstream::failbit | stringstream::badbit);
  str << _value;

  try
  {
    str >> d;
  }
  catch (...)
  {
    throw ECOMXMLTag("\"" + _value + "\" is no double!", "");
  }

  return d;
}

//---------------------------------------------------------------

long long COMXMLAtt::to_ll() const
{
  long long value;
  stringstream str;
  str.exceptions(stringstream::failbit | stringstream::badbit);
  str << _value;

  try
  {
    str >> value;
  }
  catch (...)
  {
    throw ECOMXMLTag("\"" + _value + "\" is no double!", "");
  }

  return value;
}

//---------------------------------------------------------------

COMXMLAtt::COMXMLAtt(const string &name, const string &value)
{
  _name = name;
  _value = value;
}

//---------------------------------------------------------------



