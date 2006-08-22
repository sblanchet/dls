/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef COMXMLParserHpp
#define COMXMLParserHpp

/*****************************************************************************/

#include <string>
#include <istream>
using namespace std;

/*****************************************************************************/

#include "com_exception.hpp"
#include "com_xml_tag.hpp"
#include "com_ring_buffer_t.hpp"

/*****************************************************************************/

/**
   Exception eines COMXMLParser-Objektes

   Während des Parsings wurde ein Syntaxfehler festgestellt.
   Das fehlerhafte Tag wurde entfernt.
*/

class ECOMXMLParser : public COMException
{
public:
    ECOMXMLParser(const string &pmsg, string ptag = "") : COMException(pmsg) {
        tag = ptag;
    };
    string tag;
};

/*****************************************************************************/

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

/*****************************************************************************/

enum COMXMLParserType
{
    ptStream,
    ptRing
};

/*****************************************************************************/

/**
   Einfacher XML-Parser

   Dieser XML-Parser unterstützt nur Tags mit Attributen,
   kann allerdings zwischen öffnenden, einzelnen und schliessenden
   Tags unterscheiden. Daten, die zwischen den Tags stehen,
   werden ignoriert.

   Der Parser kann STL-Streams, noch besser aber Ringpuffer vom
   Typ COMRingBufferT verarbeiten.
*/

class COMXMLParser
{
public:
    COMXMLParser();
    ~COMXMLParser();

    const COMXMLTag *parse(istream *,
                           const string & = "",
                           COMXMLTagType = dxttSingle);
    const COMXMLTag *parse(COMRingBuffer *,
                           const string & = "",
                           COMXMLTagType = dxttSingle);

    const COMXMLTag *tag() const;

private:
    COMXMLTag _tag; /**< Zuletzt geparstes XML-Tag */
    string _current_tag;

    COMRingBuffer *_data_ring; /**< Zeiger auf zu
                                  parsenden Ring */

    istream *_data_stream; /**< Zeiger auf zu parsenden Stream */
    unsigned int _data_stream_start; /**< Ürsprüngliche Startposition
                                        im Stream */
    unsigned int _data_stream_pos; /**< Aktuelle Position im Stream */
    char _data_stream_char; /**< Aktuelles Zeichen im Stream */
    bool _data_stream_char_fetched; /**< Wurde das aktuelle Zeichen
                                       schon gelesen? */
    unsigned int _data_stream_char_index; /**< Index des aktuell
                                             gelesenen Zeichens im Stream */

    void _parse(COMXMLParserType, const string &, COMXMLTagType);
    char _data(COMXMLParserType, unsigned int);
    void _erase(COMXMLParserType, unsigned int);
    bool _alphanum(char);
};

/*****************************************************************************/

/**
   Liefert einen konstanten Zeiger auf das zuletzt geparste Tag

   \return Letztes Tag
*/

inline const COMXMLTag *COMXMLParser::tag() const
{
    return &_tag;
}

/*****************************************************************************/

#endif
