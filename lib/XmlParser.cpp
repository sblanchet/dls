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

//#define DEBUG_XML_PARSER

#if DEBUG_XML_PARSER
#include <iostream>
#endif
#include <sstream>
#include <fstream>
using namespace std;

#include "LibDLS/globals.h"

#include "XmlParser.h"
#include "RingBufferT.h"

using namespace LibDLS;

/*****************************************************************************/

/**
   Konstruktor
*/

XmlParser::XmlParser()
{
}

/*****************************************************************************/

/**
   Destruktor
*/

XmlParser::~XmlParser()
{
}

/*****************************************************************************/

/**
   Holt das nächste XML-Tag aus einem STL-Stream
*/

const XmlTag *XmlParser::parse(
        istream *in,
        const string &force_tag,
        XmlTagType force_type
        )
{
    _data_stream = in;
    _data_stream_start = in->tellg();
    _data_stream_pos = 0;
    _data_stream_char_fetched = false;

    _parse(force_tag, force_type);

    return &_tag;
}

/*****************************************************************************/

/**
   Sucht Taganfang und -ende und parst den Inhalt

   \param force_tag Tag-Prüfung nach Parsing
   \param force_type Tag-Typ-Prüfung
   \throw EXmlParser Fehler beim Parsing
   \throw EXmlParserEOF EOF beim Parsing
*/

void XmlParser::_parse(const string &force_tag, XmlTagType force_type)
{
    bool escaped, escaped2;
    unsigned int i = 0;
    string title, name, value;
    stringstream err;
    char c;

    // Bis zum ersten '<' gehen
    while (_data(i) != '<') {
        i++;
    }

    // Alles vor dem ersten '<' löschen
    if (i > 0) {
        _data_stream_start += i;
    }

    _current_tag = "";

    escaped = false;
    escaped2 = false;
    while (1) {
        c = _data(i);

        _current_tag += c;

        if (c == '>' && !escaped) {
            break;
        }

        if (escaped) { // Wenn mit " escaped
            if (escaped2) { // Wenn dazu mit \ escaped
                escaped2 = false; // Ein Zeichen ignorieren
            }
            else {
                if (_data(i) == '\\') {
                    escaped2 = true;
                }
                else if (_data(i) == '\"') {
                    escaped = false;
                }
            }
        }
        else { // Nicht escaped
            if (_data(i) == '\"') {
                escaped = true;
            }
        }
        i++;
    }

    _tag.clear();
    _tag.type(dxttBegin);

    i = 1; // '<' überspringen

    // End-Tag
    if (_data(i) == '/') {
        _tag.type(dxttEnd);
        i++;
    }

    // Titel holen

    while (_alphanum(_data(i))) {
        title += _data(i++);
    }

    if (title == "") {
        err << "Expected title, got char " << (int) _data(i) << "...";
        throw EXmlParser(err.str(), _current_tag);
    }

    _tag.title(title);

    // Leerzeichen überspringen
    while (_data(i) == ' ' || _data(i) == '\n') {
        i++;
    }

    // Attribut einlesen

    while (_alphanum(_data(i))) {
        name = "";
        value = "";

        // Attributnamen einlesen
        while (_alphanum(_data(i))) {
            name += _data(i++);
        }

        // Leerzeichen ignorieren
        if (_data(i) == ' ' || _data(i) == '\n') {
            // Attribut ohne Wert
            _tag.push_att(name, "");

            i++;
            while (_data(i) == ' ' || _data(i) == '\n') {
                i++;
            }
            continue;
        }
        else if (_data(i) == '/' || _data(i) == '>') {
            // Attribut ohne Wert
            _tag.push_att(name, "");

            break;
        }

        if (_data(i++) != '=') {
            throw EXmlParser("Expected \'=\'", _current_tag);
        }

        if (_data(i++) != '\"') {
            throw EXmlParser("Expected: '\"'!", _current_tag);
        }

        // Attributwert einlesen

        escaped = false;
        while (1) {
            if (escaped) { // Letztes Zeichen war Escape-Zeichen.
                value += _data(i++); // "Blind" alles einlesen
                escaped = false;    // Escape zurücksetzen
            }
            else if (_data(i) == '\\') { // Escape-Zeichen
                escaped = true;
                i++;
            }
            else if (_data(i) == '\"') {
                // Nicht 'escape'tes '"': Fertig!
                i++;
                break;
            }
            else {
                value += _data(i++);
            }
        }

        // Fertiges Attribut ins Tag speichern
        _tag.push_att(name, value);

        // Leerzeichen ignorieren
        while (_data(i) == ' ' || _data(i) == '\n') {
            i++;
        }
    }

    if (_data(i) == '/') {
        if (_tag.type() == dxttEnd) { // Tag hatte bereits / vor dem Titel
            throw EXmlParser("Double \'/\' detected", _current_tag);
        }

        _tag.type(dxttSingle);
        i++;
    }

    if (_data(i) != '>') {
        err << "Expected \'>\' in tag " << _tag.title();
        throw EXmlParser(err.str(), _current_tag);
    }

    if (force_tag != "") {
        if (title != force_tag) {
            throw EXmlParser("Expected tag <" + force_tag +
                                ">, got <" + title + ">...", _current_tag);
        }
        if (_tag.type() != force_type) {
            err << "Wrong tag type of <" << force_tag << ">.";
            err << " Expected " << force_type << ", got " << _tag.type();
            throw EXmlParser(err.str(), _current_tag);
        }
    }
}

/*****************************************************************************/

/**
   Liefert ein Zeichen an der gegebenen Stelle

   \param index Index des Zeichens
   \return Zeichen
   \throw EXmlParserEOF EOF!
*/

char XmlParser::_data(unsigned int index)
{
    if (_data_stream_char_fetched && index == _data_stream_char_index) {
        return _data_stream_char;
    }
    else {
        if (_data_stream_pos != index) {
            _data_stream->seekg(_data_stream_start + index);
        }

        _data_stream_char = _data_stream->get();
        _data_stream_char_fetched = true;
        _data_stream_char_index = index;
        _data_stream_pos = index + 1;

        if (_data_stream_char == EOF) {
            _data_stream->seekg(_data_stream_start);
            _data_stream_pos = 0;

            throw EXmlParserEOF();
        }

#ifdef DEBUG_XML_PARSER
        msg() << _data_stream_char;
        log(DLSDebug);
#endif

        return _data_stream_char;
    }
}

/*****************************************************************************/

/**
   Bestimmt, ob das angegebene Zeichen alphanumerisch ist

   \param c Zeichen
   \return true, wenn Zeichen alphanumerisch
*/

bool XmlParser::_alphanum(char c)
{
    return ((c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9')
            || c == '_');
}

/*****************************************************************************/
