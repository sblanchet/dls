/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <zlib.h>

#include <sstream>
using namespace std;

/*****************************************************************************/

#include "com_globals.hpp"
#include "com_zlib.hpp"

//#define DEBUG

/*****************************************************************************/

/**
   Konstruktor
*/

COMZLib::COMZLib()
{
    _out_buf = 0;
    _out_size = 0;
}

/*****************************************************************************/

/**
   Destruktor
*/

COMZLib::~COMZLib()
{
    free();
}

/*****************************************************************************/

/**
   Gibt alle Speicherbereiche vorübergehend frei
*/

void COMZLib::free()
{
    _out_size = 0;

    if (_out_buf)
    {
        delete [] _out_buf;
        _out_buf = 0;
    }
}

/*****************************************************************************/

/**
   Komprimieren von Daten

   Komprimiert die Daten im Eingabepuffer und speichert das
   Ergebnis im internen Ausgabepuffer.

   \param src Konstanter zeiger auf die zu komprimierenden Daten
   \param src_size Anzahl der zu komprimierenden Zeichen
   \throw ECOMZLib Fehler beim Komprimieren
*/

void COMZLib::compress(const char *src, unsigned int src_size)
{
    uLongf out_size;
    int comp_ret;
    stringstream err;

    free();

    // Keine Daten - Nichts zu komprimieren
    if (!src_size) {
        return;
    }

    // Die komprimierten Daten können in sehr ungünstigen Fällen
    // größer sein, als die Quelldaten. Siehe ZLib-Doku:
    // http://www.gzip.org/zlib/manual.html#compress
    out_size = (uLongf) (src_size * 1.01 + 12 + 0.5);

    try
    {
        _out_buf = new char[out_size];
    }
    catch (...)
    {
        err << "Could not allocate " << out_size << " bytes!";
        throw ECOMZLib(err.str());
    }

    // Komprimieren
    comp_ret = ::compress((Bytef *) _out_buf, &out_size,
                          (const Bytef *) src, src_size);

    if (comp_ret != Z_OK) // Fehler beim Komprimieren
    {
        err << "compress() returned " << comp_ret;
        if (comp_ret == Z_BUF_ERROR) err << " (BUFFER ERROR)";
        err << ", out_size=" << out_size;
        err << ", src_size=" << src_size;
        throw ECOMZLib(err.str());
    }

#ifdef DEBUG
    msg() << "ZLib compression: input=" << src_size;
    msg() << " output=" << out_size;
    msg() << " quote=" << (float) out_size / src_size * 100 << "%";
    log(DLSDebug);
#endif

    _out_size = out_size;
}

/*****************************************************************************/

/**
   Dekomprimieren

   \param src Konstanter Zeiger auf die komprimierten Daten
   \param src_size Größe der komprimierten Daten in Bytes
   \param out_size Erwartete Größe der dekomprimierten Daten in Bytes
   \throw ECOMZLib Fehler beim Dekomprimieren
*/

void COMZLib::uncompress(const char *src, unsigned int src_size,
                         unsigned int out_size)
{
    int uncomp_ret;
    stringstream err;
    uLongf zlib_out_size = out_size;

    _out_size = 0;

    // Keine Eingabedaten - keine Dekompression
    if (!src_size) return;

    try
    {
        _out_buf = new char[out_size];
    }
    catch (...)
    {
        err << "Could not allocate " << out_size << " bytes!";
        throw ECOMZLib(err.str());
    }

    // Dekomprimieren
    uncomp_ret = ::uncompress((Bytef *) _out_buf, &zlib_out_size,
                              (const Bytef *) src, src_size);

    if (uncomp_ret != Z_OK) // Fehler beim Dekomprimieren
    {
        err << "uncompress() returned " << uncomp_ret;
        if (uncomp_ret == Z_BUF_ERROR) {
            err << " (BUFFER ERROR)";
        }
        err << ", out_size=" << out_size;
        err << ", src_size=" << src_size;
        throw ECOMZLib(err.str());
    }

    _out_size = out_size;
}

/*****************************************************************************/
