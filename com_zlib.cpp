//---------------------------------------------------------------
//
//  C O M _ Z L I B . C P P
//
//---------------------------------------------------------------

#include <zlib.h>

#include <sstream>
using namespace std;

//---------------------------------------------------------------

#include "com_globals.hpp"
#include "com_zlib.hpp"

//---------------------------------------------------------------

RCS_ID("$Header: /home/fp/dls/src/RCS/com_zlib.cpp,v 1.6 2004/12/17 10:07:43 fp Exp $");

//---------------------------------------------------------------

/**
   Konstruktor
*/

COMZLib::COMZLib()
{
  _out_buf = 0;
}

//---------------------------------------------------------------

/**
   Destruktor
*/

COMZLib::~COMZLib()
{
  if (_out_buf) delete [] _out_buf;
}

//---------------------------------------------------------------

/**
   Komprimieren von Daten

   Komprimiert die Daten im Eingabepuffer und speichert das
   Ergebnis im internen Ausgabepuffer.

   \param src Konstanter zeiger auf die zu komprimierenden Daten
   \param src_len Anzahl der zu komprimierenden Zeichen
   \throw ECOMZLib Fehler beim Komprimieren
*/

void COMZLib::compress(const char *src, unsigned int src_len)
{
  unsigned int out_len;
  int comp_ret;
  stringstream err;

  _out_len = 0;

  // Keine Daten - Nichts zu komprimieren
  if (!src_len) return;
  
  // Die komprimierten Daten können in sehr ungünstigen Fällen
  // größer sein, als die Quelldaten. 1K mehr allokieren.
  out_len = src_len + 1024;
  _realloc(out_len);

  // Komprimieren
  comp_ret = ::compress((Bytef *) _out_buf, (uLongf *) &out_len,
                        (const Bytef *) src, src_len);

  if (comp_ret != Z_OK) // Fehler beim Komprimieren
  {
    err << "error " << comp_ret;
    if (comp_ret == Z_BUF_ERROR) err << " (BUFFER ERROR)";
    err << ", out_len=" << out_len;
    err << ", src_len=" << src_len;
    throw ECOMZLib(err.str());
  }

  _out_len = out_len;
}

//---------------------------------------------------------------

/**
   Dekomprimieren

   \param src Konstanter Zeiger auf die komprimierten Daten
   \param src_len Größe der komprimierten Daten
   \param out_len Erwartete Größe der dekomprimierten Daten
   \throw ECOMZLib Fehler beim Dekomprimieren
*/

void COMZLib::uncompress(const char *src, unsigned int src_len,
                         unsigned int out_len)
{
  int uncomp_ret;
  stringstream err;

  _out_len = 0;

  // Keine Eingabedaten - keine Dekompression
  if (!src_len) return;

  // Speicher für die Ausgabe reservieren
  _realloc(out_len);

  // Dekomprimieren
  uncomp_ret = ::uncompress((Bytef *) _out_buf, (uLongf *) &out_len,
                            (const Bytef *) src, src_len);

  if (uncomp_ret != Z_OK) // Fehler beim Dekomprimieren
  {
    err << "error " << uncomp_ret;
    if (uncomp_ret == Z_BUF_ERROR) err << " (BUFFER ERROR)";
    err << ", out_len=" << out_len;
    err << ", src_len=" << src_len;
    throw ECOMZLib(err.str());
  }

  _out_len = out_len;
}

//---------------------------------------------------------------

/**
   Reserviert Speicher für die Ausgabe

   \param size Größe des Ausgabespeichers
   \throw ECOMZLib Es konnte kein Speicher reserviert werden
*/

void COMZLib::_realloc(unsigned int size)
{
  stringstream err;

  // Alten Speicher freigeben
  if (_out_buf) delete [] _out_buf;
  _out_buf = 0;

  try
  {
    // Neuen Speicher allokieren
    _out_buf = new char[size];
  }
  catch (...)
  {
    err << "could not allocate " << size << " bytes of memory!";
    throw ECOMZLib(err.str());
  }
}

//---------------------------------------------------------------
