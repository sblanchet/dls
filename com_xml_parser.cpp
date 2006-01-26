//---------------------------------------------------------------
//
//  C O M _ X M L _ P A R S E R . C P P
//
//---------------------------------------------------------------

//#define DEBUG_XML_PARSER

#if DEBUG_XML_PARSER
#include <iostream>
#endif
#include <sstream>
using namespace std;

//---------------------------------------------------------------

#include "com_globals.hpp"
#include "com_xml_parser.hpp"
#include "com_ring_buffer_t.hpp"

//---------------------------------------------------------------

RCS_ID("$Header: /home/fp/dls/src/RCS/com_xml_parser.cpp,v 1.6 2004/12/15 14:43:16 fp Exp $");

//---------------------------------------------------------------

/**
   Konstruktor
*/

COMXMLParser::COMXMLParser()
{
}

//---------------------------------------------------------------

/**
   Destruktor
*/

COMXMLParser::~COMXMLParser()
{
}

//---------------------------------------------------------------

/**
   Holt das nächste XML-Tag aus einem STL-Stream
*/

const COMXMLTag *COMXMLParser::parse(istream *in,
                                     const string &force_tag,
                                     COMXMLTagType force_type)
{
  _data_stream = in;
  _data_stream_start = in->tellg();
  _data_stream_pos = 0;
  _data_stream_char_fetched = false;

  _parse(ptStream, force_tag, force_type);

  return &_tag;
}

//---------------------------------------------------------------

/**
   Holt das nächste XML-Tag aus Ringpuffer
*/

const COMXMLTag *COMXMLParser::parse(COMRingBufferT<char, unsigned int> *ring,
                                     const string &force_tag,
                                     COMXMLTagType force_type)
{
  _data_ring = ring;

  _parse(ptRing, force_tag, force_type);

  return &_tag;
}

//---------------------------------------------------------------

/**
   Sucht Taganfang und -ende und parst den Inhalt

   \param parse_type Stream oder Ring
   \param force_tag Tag-Prüfung nach Parsing
   \param force_type Tag-Typ-Prüfung
   \throw ECOMXMLParser Fehler beim Parsing
   \throw ECOMXMLParserEOF EOF beim Parsing
*/

void COMXMLParser::_parse(COMXMLParserType parse_type,
                          const string &force_tag,
                          COMXMLTagType force_type)
{
  bool escaped, escaped2;
  unsigned int i = 0, tag_length;
  string title, name, value;
  stringstream err;
  char c;

  // Bis zum ersten '<' gehen
  while (_data(parse_type, i) != '<') i++;

  // Alles vor dem ersten '<' löschen
  if (i > 0)
  {
    if (parse_type == ptRing)
    {
      _erase(parse_type, i);
      i = 0;
    }
    else // Stream
    {
      _data_stream_start += i;
    }
  }
    
  _current_tag = "";

  escaped = false;
  escaped2 = false;
  while (1)
  {
    c = _data(parse_type, i);

    _current_tag += c;

    if (c == '>' && !escaped) break;

    if (escaped) // Wenn mit " escaped
    {
      if (escaped2) // Wenn dazu mit \ escaped
      {
        escaped2 = false; // Ein Zeichen ignorieren
      }
      else
      {
        if (_data(parse_type, i) == '\\') escaped2 = true;
        else if (_data(parse_type, i) == '\"') escaped = false;
      }
    }
    else // Nicht escaped
    {
      if (_data(parse_type, i) == '\"') escaped = true;
    }
    i++;
  }

  // Ein Tag (von 0 bis i) ist komplett!
  tag_length = i + 1;

  _tag.clear();
  _tag.type(dxttBegin);

  i = 1; // '<' überspringen

  // End-Tag
  if (_data(parse_type, i) == '/')
  {
    _tag.type(dxttEnd);
    i++;
  }

  // Titel holen

  while (_alphanum(_data(parse_type, i)))
  {
    title += _data(parse_type, i++);
  }

  if (title == "")
  {
    err << "expected title, got char " << (int) _data(parse_type, i) << "...";

    // Falsches Tag löschen
    _erase(parse_type, tag_length);

    throw ECOMXMLParser(err.str(), _current_tag);
  }

  _tag.title(title);

  // Leerzeichen überspringen
  while (_data(parse_type, i) == ' ' || _data(parse_type, i) == '\n') i++;

  // Attribut einlesen

  while (_alphanum(_data(parse_type, i)))
  {
    name = "";
    value = "";
    
    // Attributnamen einlesen
    while (_alphanum(_data(parse_type, i)))
    {
      name += _data(parse_type, i++);
    }

    // Leerzeichen ignorieren
    if (_data(parse_type, i) == ' ' || _data(parse_type, i) == '\n')
    {
      // Attribut ohne Wert
      _tag.push_att(name, "");

      i++;
      while (_data(parse_type, i) == ' ' || _data(parse_type, i) == '\n') i++;
      continue;
    }
    else if (_data(parse_type, i) == '/' || _data(parse_type, i) == '>')
    {
      // Attribut ohne Wert
      _tag.push_att(name, "");

      break;
    }

    if (_data(parse_type, i++) != '=')
    {
      // Falsches Tag löschen
      _erase(parse_type, tag_length);

      throw ECOMXMLParser("expected \'=\'", _current_tag);
    }
      
    if (_data(parse_type, i++) != '\"')
    {
      // Falsches Tag löschen
      _erase(parse_type, tag_length);

      throw ECOMXMLParser("expected: '\"'!", _current_tag);
    }

    // Attributwert einlesen

    escaped = false;
    while (1)
    {
      if (escaped) // Letztes Zeichen war Escape-Zeichen.
      {
        value += _data(parse_type, i++); // "Blind" alles einlesen
        escaped = false;    // Escape zurücksetzen
      }
      else if (_data(parse_type, i) == '\\') // Escape-Zeichen
      {
        escaped = true;
        i++;
      }
      else if (_data(parse_type, i) == '\"') // Nicht 'escape'tes '"': Fertig!
      {
        i++;
        break;
      }
      else
      {
        value += _data(parse_type, i++);
      }
    }
      
    // Fertiges Attribut ins Tag speichern
    _tag.push_att(name, value);

    // Leerzeichen ignorieren
    while (_data(parse_type, i) == ' ' || _data(parse_type, i) == '\n') i++;
  }

  if (_data(parse_type, i) == '/')
  {
    if (_tag.type() == dxttEnd) // Tag hatte bereits / vor dem Titel
    {
      throw ECOMXMLParser("double \'/\' detected", _current_tag);
    }

    _tag.type(dxttSingle);
    i++;
  }

  if (_data(parse_type, i) != '>')
  {
    err << "expected \'>\' in tag " << _tag.title();
    throw ECOMXMLParser(err.str(), _current_tag);
  }

  // Tag aus der Quelle entfernen
  _erase(parse_type, tag_length);

  //_last_tag_length = tag_length;

  if (force_tag != "")
  {
    if (title != force_tag)
    {
      throw ECOMXMLParser("expected tag <" + force_tag + ">, got <" + title + ">...", _current_tag);
    }
    if (_tag.type() != force_type)
    {
      err << "wrong tag type of <" << force_tag << "> ";
      err << "expected " << force_type << ", got " << _tag.type();
      throw ECOMXMLParser(err.str(), _current_tag);
    }
  }
}

//---------------------------------------------------------------

/**
   Liefert ein zeichen an der gegebenen Stelle

   \param parse_type Stream oder Ring
   \param index Index des Zeichens
   \return Zeichen
   \throw ECOMXMLParserEOF EOF!
*/

char COMXMLParser::_data(COMXMLParserType parse_type, unsigned int index)
{
  if (parse_type == ptRing)
  {
    if (index >= _data_ring->length())
    {
      throw ECOMXMLParserEOF();
    } 
    return (*_data_ring)[index];
  }

  else if (parse_type == ptStream)
  {
    if (_data_stream_char_fetched && index == _data_stream_char_index)
    {
      return _data_stream_char;
    }
    else
    {
      if (_data_stream_pos != index)
      {
        _data_stream->seekg(_data_stream_start + index);
      }

      _data_stream_char = _data_stream->get();
      _data_stream_char_fetched = true;
      _data_stream_char_index = index;
      _data_stream_pos = index + 1;

      if (_data_stream_char == EOF)
      {
        _data_stream->seekg(_data_stream_start);
        _data_stream_pos = 0;
        
        throw ECOMXMLParserEOF();
      }

#ifdef DEBUG_XML_PARSER
      cout << _data_stream_char;
#endif

      return _data_stream_char;
    }
  }

  /*
  else
  {
    if (index >= _data_string_length)
    {
      throw ECOMXMLParserEOF();
    } 
    return (*_data_string)[index];
  }
  */

  throw ECOMXMLParser("unkown parser type!");
}

//---------------------------------------------------------------

/**
   Löscht die geparsten Daten aus der Quelle

   \param parse_type Stream oder Ring
   \param length Anzahl der zu löschenden Zeichen
*/

void COMXMLParser::_erase(COMXMLParserType parse_type, unsigned int length)
{
  if (parse_type == ptRing)
  {
    _data_ring->erase_first(length);
  }
}

//---------------------------------------------------------------

/**
   Bestimmt, ob das angegebene Zeichen alphanumerisch ist

   \param c Zeichen
   \return true, wenn Zeichen alphanumerisch
*/

bool COMXMLParser::_alphanum(char c)
{
  return ((c >= 'a' && c <= 'z')
          || (c >= 'A' && c <= 'Z')
          || (c >= '0' && c <= '9')
          || c == '_');
}

//---------------------------------------------------------------
