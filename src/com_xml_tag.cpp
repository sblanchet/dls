/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <sstream>
using namespace std;

/*****************************************************************************/

#include "com_globals.hpp"
#include "com_xml_tag.hpp"

/*****************************************************************************/

/**
   Konstruktor
*/

COMXMLTag::COMXMLTag()
{
    clear();
}

/*****************************************************************************/

/**
   Destruktor
*/

COMXMLTag::~COMXMLTag()
{
}

/*****************************************************************************/

/**
   Leert das XMl-Tag
*/

void COMXMLTag::clear()
{
    _title.clear();
    _atts.clear();
    _type = dxttSingle;
}

/*****************************************************************************/

/**
   Setzt den Titel
*/

void COMXMLTag::title(const string &title)
{
    _title = title;
}

/*****************************************************************************/

/**
   Setzt den Typ (Start-Tag/Einzeltag/End-Tag)

   \param type Neuer Typ
*/

void COMXMLTag::type(COMXMLTagType type)
{
    _type = type;
}

/*****************************************************************************/

/**
   Liefert einen konstanten Zeiger auf ein bestimmtes Attribut

   \param name Name des gewünschten Attrubutes
   \return Konstanter Zeiger auf das Attribut
   \throw ECOMXMLTag Ein Attribut mit diesem Namen existiert nicht
*/

const COMXMLAtt *COMXMLTag::att(const string &name) const
{
    list<COMXMLAtt>::const_iterator iter = _atts.begin();

    // Liste der Attribute durchsuchen
    while (iter != _atts.end())
    {
        if (iter->name() == name) return &(*iter);
        iter++;
    }

    throw ECOMXMLTag("Attribute \"" + name + "\" does not exist!", tag());
}

/*****************************************************************************/

/**
   Prüft, ob das Tag ein Attribute mit einem bestimmten Namen hat

   \param name Name des gesuchten Attributes
   \return true, wenn das tag ein solches Attribut besitzt
*/

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

/*****************************************************************************/

/**
   Fügt dem Tag ein Attribut mit Namen und Wert (String) hinzu

   \param name name des Attributes
   \param value Wert des Attributes
*/

void COMXMLTag::push_att(const string &name, const string &value)
{
    _atts.push_back(COMXMLAtt(name, value));
}

/*****************************************************************************/

/**
   Fügt dem Tag ein Attribut mit Namen und Wert hinzu (int)

   Dies ist die (int)-Version von
   push_att(const string &, const string &).

   \param name name des Attributes
   \param value Wert des Attributes
*/

void COMXMLTag::push_att(const string &name, int value)
{
    stringstream str;
    str << value;
    _atts.push_back(COMXMLAtt(name, str.str()));
}

/*****************************************************************************/

/**
   Fügt dem Tag ein Attribut mit Namen und Wert hinzu (unsigned int)

   Dies ist die (unsigned int)-Version von
   push_att(const string &, const string &).

   \param name name des Attributes
   \param value Wert des Attributes
*/

void COMXMLTag::push_att(const string &name, unsigned int value)
{
    stringstream str;
    str << value;
    _atts.push_back(COMXMLAtt(name, str.str()));
}

/*****************************************************************************/

/**
   Fügt dem Tag ein Attribut mit Namen und Wert hinzu (double)

   Dies ist die (double)-Version von
   push_att(const string &, const string &).

   \param name name des Attributes
   \param value Wert des Attributes
*/

void COMXMLTag::push_att(const string &name, double value)
{
    stringstream str;
    str << fixed << value;
    _atts.push_back(COMXMLAtt(name, str.str()));
}

/*****************************************************************************/

/**
   Fügt dem Tag ein Attribut mit Namen und Wert hinzu (long long)

   Dies ist die (long long)-Version von
   push_att(const string &, const string &).

   \param name name des Attributes
   \param value Wert des Attributes
*/

void COMXMLTag::push_att(const string &name, long long value)
{
    stringstream str;
    str << value;
    _atts.push_back(COMXMLAtt(name, str.str()));
}

/*****************************************************************************/

/**
   Zählt die Attribute des Tags

   \return Anzahl der Attribute
*/

int COMXMLTag::att_count() const
{
    return _atts.size();
}

/*****************************************************************************/

/**
   Gibt das gesamte Tag als String zurück

   Erst diese Methode baut das eigentliche Tag zusammen.

   \return XML-Tag als String
*/

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

/*****************************************************************************/
//
//  COMXMLAtt-Methoden
//
/*****************************************************************************/

/**
   Konstruktor
*/

COMXMLAtt::COMXMLAtt(const string &name, const string &value)
{
    _name = name;
    _value = value;
}

/*****************************************************************************/

/**
   Gibt den Wert des Attributes als String zurück

   \return Attributwert
*/

const string & COMXMLAtt::to_str() const
{
    return _value;
}

/*****************************************************************************/

/**
   Gibt den Wert des Attributes als (int) zurück

   \return Attributwert
*/

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

/*****************************************************************************/

/**
   Gibt den Wert des Attributes als (double) zurück

   \return Attributwert
*/

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

/*****************************************************************************/

/**
   Gibt den Wert des Attributes als (long long) zurück

   \return Attributwert
*/

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
        throw ECOMXMLTag("\"" + _value + "\" is no long long!", "");
    }

    return value;
}

/*****************************************************************************/
