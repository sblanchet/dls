//---------------------------------------------------------------
//
//  C O M _ X M L _ T A G . H P P
//
//---------------------------------------------------------------

#ifndef COMXMLTagHpp
#define COMXMLTagHpp

//---------------------------------------------------------------

#include <string>
#include <list>
using namespace std;

//---------------------------------------------------------------

#include "com_exception.hpp"

//---------------------------------------------------------------

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

//---------------------------------------------------------------

/**
   Attribut innerhalb eines XML-Tags
*/

class COMXMLAtt
{
public:
  COMXMLAtt(const string &, const string &);

  const string &name() const;
  void name(const string &);
  const string &to_str() const;
  int to_int() const;
  double to_dbl() const;
  long long to_ll() const;
  void value(const string &);

private:
  string _name;  /**< Attributname */
  string _value; /**< Attributwert */

  COMXMLAtt(); // Standardkonstruktor soll nicht aufgerufen werden
};

//---------------------------------------------------------------

enum COMXMLTagType {dxttBegin, dxttSingle, dxttEnd};

//---------------------------------------------------------------

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
  int att_count() const;
  string tag() const;

private:
  string _title;         /**< Tag-Titel */
  COMXMLTagType _type;   /**< Tag-Art (Start, Single oder End) */
  list<COMXMLAtt> _atts; /**< Liste von Attributen */
};

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf den Tag-Titel

   \return Konstante referenz auf den Titel-String
*/

inline const string &COMXMLTag::title() const
{
  return _title;
}

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf den Tag-Typ

   \return Tag-Typ
*/

inline COMXMLTagType COMXMLTag::type() const
{
  return _type;
}

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf den Attributnamen

   \return Konstante Referenz auf den Namen
*/

inline const string &COMXMLAtt::name() const
{
  return _name;
}

//---------------------------------------------------------------

#endif
