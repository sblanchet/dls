//---------------------------------------------------------------
//
//  C O M _ X M L _ P A R S E R . C P P
//
//---------------------------------------------------------------

#include <sstream>
using namespace std;

#include "com_xml_parser.hpp"
#include "com_ring_buffer_t.hpp"

//---------------------------------------------------------------

COMXMLParser::COMXMLParser()
{
}

//---------------------------------------------------------------

COMXMLParser::~COMXMLParser()
{
}

//---------------------------------------------------------------

const COMXMLTag *COMXMLParser::parse(istream *in,
                                     const string &force_tag,
                                     COMXMLTagType force_type)
{
  _force_tag = force_tag;
  _force_type = force_type;
  _parse_type = ptStream;
  _data_stream = in;
  _data_stream_start = in->tellg();
  _data_stream_pos = 0;
  _data_stream_char_fetched = false;

  _parse();

  return &_tag;
}

//---------------------------------------------------------------

const COMXMLTag *COMXMLParser::parse(COMRingBufferT<char, unsigned int> *ring,
                                     const string &force_tag,
                                     COMXMLTagType force_type)
{
  _force_tag = force_tag;
  _force_type = force_type;
  _parse_type = ptRing;
  _data_ring = ring;
  _data_ring_length = ring->length();

  _parse();

  return &_tag;
}

//---------------------------------------------------------------

void COMXMLParser::_parse()
{
  bool escaped, escaped2;
  unsigned int i = 0, tag_length;
  string title, name, value;
  stringstream err;

  // Bis zum ersten '<' gehen
  while (_data(i) != '<') i++;

  // Alles vor dem ersten '<' löschen
  if (i > 0)
  {
    if (_parse_type == ptString || _parse_type == ptRing)
    {
      _erase(i);
      i = 0;
    }
    else // Stream
    {
      _data_stream_start += i;
    }
  }
    
  escaped = false;
  escaped2 = false;
  while (_data(i) != '>' || escaped)
  {
    if (escaped) // Wenn mit " escaped
    {
      if (escaped2) // Wenn dazu mit \ escaped
      {
        escaped2 = false; // Ein Zeichen ignorieren
      }
      else
      {
        if (_data(i) == '\\') escaped2 = true;
        else if (_data(i) == '\"') escaped = false;
      }
    }
    else // Nicht escaped
    {
      if (_data(i) == '\"') escaped = true;
    }
    i++;
  }

  // Ein Tag (von 0 bis i) ist komplett!
  tag_length = i + 1;

  _tag.clear();
  _tag.type(dxttBegin);

  i = 1; // '<' überspringen

  // End-Tag
  if (_data(i) == '/')
  {
    _tag.type(dxttEnd);
    i++;
  }

  // Titel holen

  while (_alphanum(_data(i)))
  {
    title += _data(i++);
  }

  if (title == "")
  {
    err << "expected title, got char " << (int) _data(i) << "...";

    // Falsches Tag löschen
    _erase(tag_length);

    throw ECOMXMLParser(err.str());
  }

  _tag.title(title);

  // Leerzeichen überspringen
  while (_data(i) == ' ') i++;

  // Attribut einlesen

  while (_alphanum(_data(i)))
  {
    name = "";
    value = "";
    
    // Attributnamen einlesen
    while (_alphanum(_data(i)))
    {
      name += _data(i++);
    }

    // Leerzeichen ignorieren
    if (_data(i) == ' ')
    {
      // Attribut ohne Wert
      _tag.push_att(name, "");

      i++;
      while (_data(i) == ' ') i++;
      continue;
    }
    else if (_data(i) == '/' || _data(i) == '>')
    {
      // Attribut ohne Wert
      _tag.push_att(name, "");

      break;
    }

    if (_data(i++) != '=')
    {
      // Falsches Tag löschen
      _erase(tag_length);

      throw ECOMXMLParser("expected \'=\'");
    }
      
    if (_data(i++) != '\"')
    {
      // Falsches Tag löschen
      _erase(tag_length);

      throw ECOMXMLParser("expected: '\"'!");
    }

    // Attributwert einlesen

    escaped = false;
    while (1)
    {
      if (escaped) // Letztes Zeichen war Escape-Zeichen.
      {
        value += _data(i++); // "Blind" alles einlesen
        escaped = false;    // Escape zurücksetzen
      }
      else if (_data(i) == '\\') // Escape-Zeichen
      {
        escaped = true;
        i++;
      }
      else if (_data(i) == '\"') // Nicht 'escape'tes '"': Fertig!
      {
        i++;
        break;
      }
      else
      {
        value += _data(i++);
      }
    }
      
    // Fertiges Attribut ins Tag speichern
    _tag.push_att(name, value);

    // Leerzeichen ignorieren
    while (_data(i) == ' ') i++;
  }

  if (_data(i) == '/')
  {
    if (_tag.type() == dxttEnd) // Tag hatte bereits / vor dem Titel
    {
      throw ECOMXMLParser("double \'/\' detected");
    }

    _tag.type(dxttSingle);
    i++;
  }

  if (_data(i) != '>')
  {
    throw ECOMXMLParser("expected \'>\'");
  }

  // Tag aus der Quelle entfernen
  _erase(tag_length);

  if (_force_tag != "")
  {
    if (title != _force_tag)
    {
      throw ECOMXMLParser("expected tag <" + _force_tag + ">, got <" + title + ">...");
    }
    if (_tag.type() != _force_type)
    {
      err << "wrong tag type of <" << _force_tag << "> ";
      err << "expected " << _force_type << ", got " << _tag.type();
       throw ECOMXMLParser(err.str());
    }
  }
}

//---------------------------------------------------------------

char COMXMLParser::_data(unsigned int index)
{
  if (_parse_type == ptRing)
  {
    if (index >= _data_ring_length)
    {
      throw ECOMXMLParserEOF();
    } 
    return (*_data_ring)[index];
  }

  else if (_parse_type == ptStream)
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
      
      return _data_stream_char;
    }
  }

  else //if (_parse_type == ptString)
  {
    if (index >= _data_string_length)
    {
      throw ECOMXMLParserEOF();
    } 
    return (*_data_string)[index];
  }
}

//---------------------------------------------------------------

void COMXMLParser::_erase(unsigned int length)
{
  if (_parse_type == ptRing)
  {
    _data_ring->erase_first(length);
  }
  else if (_parse_type == ptString)
  {
    _data_string->erase(0, length);
  }
}

//---------------------------------------------------------------

bool COMXMLParser::_alphanum(char c)
{
  return (c >= 'a' && c <= 'z')
    || (c >= 'A' && c <= 'Z')
    || (c >= '0' && c <= '9')
    || c == '_';
}

//---------------------------------------------------------------
