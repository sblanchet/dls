//---------------------------------------------------------------
//
//  C O M _ X M L _ P A R S E R . H P P
//
//---------------------------------------------------------------

#ifndef COMXMLParserHpp
#define COMXMLParserHpp

//---------------------------------------------------------------

#include <string>
#include <istream>
using namespace std;

//---------------------------------------------------------------

#include "com_exception.hpp"
#include "com_xml_tag.hpp"
#include "com_ring_buffer_t.hpp"

//---------------------------------------------------------------

/**
   Exception eines COMXMLParser-Objektes

   Während des Parsings wurde ein Syntaxfehler festgestellt.
   Das fehlerhafte Tag wurde entfernt.
*/

class ECOMXMLParser : public COMException
{
public:
  ECOMXMLParser(const string &pmsg, string ptag = "") : COMException(pmsg) {tag = ptag;};
  string tag;
};

//---------------------------------------------------------------

/**
   Exception eines COMXMLParser-Objektes

   Während des Parsings wurde das Ende der Datenquelle erreicht.
   Der Lesezeiger wurde wieder auf den Anfang des
   angefangenen Tags gesetzt.
*/

class ECOMXMLParserEOF : public COMException
{
public:
  ECOMXMLParserEOF() : COMException("unexpected EOF!") {};
};

//---------------------------------------------------------------

enum COMXMLParserType {ptString, ptStream, ptRing};

//---------------------------------------------------------------

class COMXMLParser
{
public:
  COMXMLParser();
  ~COMXMLParser();

  /*
  const COMXMLTag *parse(string *,
                         const string & = "",
                         COMXMLTagType = dxttSingle);
  */
  const COMXMLTag *parse(istream *,
                         const string & = "",
                         COMXMLTagType = dxttSingle);
  const COMXMLTag *parse(COMRingBufferT<char, unsigned int> *,
                         const string & = "",
                         COMXMLTagType = dxttSingle);
  
  const COMXMLTag *last_tag() const;
  const string &last_parsed() const;

private:
  COMXMLTag _tag;
  string _force_tag;
  COMXMLTagType _force_type;
  COMXMLParserType _parse_type;

  COMRingBufferT<char, unsigned int> *_data_ring;
  unsigned int _data_ring_length;

  istream *_data_stream;
  unsigned int _data_stream_start;
  unsigned int _data_stream_pos;
  char _data_stream_char;
  bool _data_stream_char_fetched;
  unsigned int _data_stream_char_index;

  string *_data_string;
  unsigned int _data_string_length;

  void _parse();
  char _data(unsigned int);
  void _erase(unsigned int);
  bool _alphanum(char);
};

//---------------------------------------------------------------

inline const COMXMLTag *COMXMLParser::last_tag() const
{
  return &_tag;
}

//---------------------------------------------------------------

#endif
