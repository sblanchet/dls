/******************************************************************************
 *
 *  $Id$
 *
 *  This file is part of the Data Logging Service (DLS).
 *
 *  DLS is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  DLS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with DLS. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef COMXMLTagHpp
#define COMXMLTagHpp

/*****************************************************************************/

#include <stdint.h>

#include <string>
#include <list>
using namespace std;

/*****************************************************************************/

#include "com_exception.hpp"

/*****************************************************************************/

/**
   Exception eines COMXMLTag-Objektes

   I. A. bedeutet dies, das ein angefragtes Attribut nicht
   existiert, oder nicht in den gefragten Typ konvertiert
   werden kann.
*/

class ECOMXMLTag : public COMException
{
public:
    ECOMXMLTag(string pmsg, string ptag) : COMException(pmsg) {tag = ptag;};
    string tag;
};

/*****************************************************************************/

/**
   Attribut innerhalb eines XML-Tags
*/

class COMXMLAtt
{
public:
    COMXMLAtt(const string &, const string &);

    const string &name() const;
    const string &to_str() const;
    int to_int() const;
    double to_dbl() const;
    uint64_t to_uint64() const;

private:
    string _name;  /**< Attributname */
    string _value; /**< Attributwert */

    COMXMLAtt(); // Standardkonstruktor soll nicht aufgerufen werden
};

/*****************************************************************************/

enum COMXMLTagType {dxttBegin, dxttSingle, dxttEnd};

/*****************************************************************************/

/**
   XML-Tag
*/

class COMXMLTag
{
public:
    COMXMLTag();
    ~COMXMLTag();

    void clear();
    const string &title() const;
    void title(const string &);
    COMXMLTagType type() const;
    void type(COMXMLTagType);
    const COMXMLAtt *att(const string &) const;
    bool has_att(const string &) const;
    void push_att(const string &, const string &);
    void push_att(const string &, int);
    void push_att(const string &, unsigned int);
    void push_att(const string &, double);
    void push_att(const string &, uint64_t);
    int att_count() const;
    string tag() const;

private:
    string _title;         /**< Tag-Titel */
    COMXMLTagType _type;   /**< Tag-Art (Start, Single oder End) */
    list<COMXMLAtt> _atts; /**< Liste von Attributen */
};

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf den Tag-Titel

   \return Konstante referenz auf den Titel-String
*/

inline const string &COMXMLTag::title() const
{
    return _title;
}

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf den Tag-Typ

   \return Tag-Typ
*/

inline COMXMLTagType COMXMLTag::type() const
{
    return _type;
}

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf den Attributnamen

   \return Konstante Referenz auf den Namen
*/

inline const string &COMXMLAtt::name() const
{
    return _name;
}

/*****************************************************************************/

#endif
