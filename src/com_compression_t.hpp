/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef COMCompressionTHpp
#define COMCompressionTHpp

/*****************************************************************************/

#include "com_zlib.hpp"
#include "com_base64.hpp"
#include "com_mdct_t.hpp"
#include "com_quant_t.hpp"
#include "com_exception.hpp"

//#define DEBUG

/*****************************************************************************/

/**
   Exception eines DLSCompression-Objektes
*/

class ECOMCompression : public COMException
{
public:
    ECOMCompression(const string &pmsg) : COMException(pmsg) {};
};

/*****************************************************************************/

/**
   Abstrakte Basisklasse eines Kompressionsobjektes
*/

template <class T>
class COMCompressionT
{
public:
    COMCompressionT() {};
    virtual ~COMCompressionT() {};

    /**
       Gibt alle persistenten Speicher frei.

       Die Speicher werden bei der nächsten Anforderung immer
       neu angelegt. Durch den Aufruf von free() wird der
       Speicher in der Zwischenzeit nicht unnötig belegt.
    */

    virtual void free() = 0;

    /**
       Komprimiert ein Array von Datenwerten beliebigen Typs

       \param input Konstanter Zeiger auf ein Array von Werten
       \param length Anzahl der Werte im Input-Array
       \throw ECOMCompression Fehler beim Komprimieren
    */

    virtual void compress(const T *input,
                          unsigned int length) = 0;

    /**
       Wandelt komprimierte Binärdaten in ein Array von Datenwerten um

       \param input Konstanter Zeiger auf den Speicherbereich mit
       den komprimierten Binärdaten
       \param size Größe der komprimierten Daten in Bytes
       \param length Erwartete Anzahl von Datenwerten
       \throw ECOMCompression Fehler beim Dekomprimieren
    */

    virtual void uncompress(const char *input,
                            unsigned int size,
                            unsigned int length) = 0;

    /**
       Leert den persistenten Speicher des Komprimierungsvorganges
       und liefert die restlichen, komprimierten Daten zurück

       \throw ECOMCompression Fehler beim Komprimieren
    */

    virtual void flush_compress() = 0;

    /**
       Leert den persistenten Speicher des Dekomprimierungsvorganges
       und liefert die restlichen, dekomprimierten Daten

       \param input Konstanter Zeiger auf den Speicherbereich mit
       den zuvor von flush_compress() gelieferten Binärdaten
       \param size Größe der komprimierten Daten in Bytes
       \throw ECOMCompression Fehler beim Dekomprimieren
    */

    virtual void flush_uncompress(const char *input,
                                  unsigned int size) = 0;

    /**
       Löscht alle Persistenzen, die von vorherigen Daten abhängig sind
    */

    virtual void clear() = 0;

    /**
       Liefert die Komprimierten Daten

       \return Konstanter Zeiger auf die komprimierten Daten
    */

    virtual const char *compression_output() const = 0;

    /**
       Liefert die Größe der komprimierten Daten

       \return Größe in Bytes
    */

    virtual unsigned int compressed_size() const = 0;

    /**
       Liefert die Dekomprimierten Daten

       \return Konstanter Zeiger auf die dekomprimierten Daten
    */

    virtual const T *decompression_output() const = 0;

    /**
       Liefert die Anzahl der dekomprimierten Datenwerte

       \return Anzahl Datenwerte
    */

    virtual unsigned int decompressed_length() const = 0;
};

/*****************************************************************************/
//
//  ZLib / Base64
//
/*****************************************************************************/

/**
   Kompressionsobjekt: Erst ZLib, dann Base64
*/

template <class T>
class COMCompressionT_ZLib : public COMCompressionT<T>
{
public:
    COMCompressionT_ZLib();
    ~COMCompressionT_ZLib();

    void compress(const T *input,
                  unsigned int length);
    void uncompress(const char *input,
                    unsigned int size,
                    unsigned int length);
    void clear();
    void flush_compress();
    void flush_uncompress(const char *input,
                          unsigned int size);

    void free();

    const char *compression_output() const;
    unsigned int compressed_size() const;
    const T *decompression_output() const;
    unsigned int decompressed_length() const;

private:
    COMZLib _zlib;            /**< ZLib-Objekt zum Komprimieren */
    COMBase64 _base64;        /**< Base64-Objekt zum Kodieren */
};

/*****************************************************************************/

template <class T>
COMCompressionT_ZLib<T>::COMCompressionT_ZLib()
{
}

/*****************************************************************************/

template <class T>
COMCompressionT_ZLib<T>::~COMCompressionT_ZLib()
{
    free();
}

/*****************************************************************************/

template <class T>
void COMCompressionT_ZLib<T>::free()
{
    _zlib.free();
    _base64.free();
}

/*****************************************************************************/

template <class T>
void COMCompressionT_ZLib<T>::compress(const T *input,
                                       unsigned int length)
{
    stringstream err;

    try
    {
        _zlib.compress((char *) input, length * sizeof(T));
        _base64.encode(_zlib.output(), _zlib.output_size());
    }
    catch (ECOMZLib &e)
    {
        err << "ZLib: " << e.msg;
        throw ECOMCompression(err.str());
    }
    catch (ECOMBase64 &e)
    {
        err << "Base64: " << e.msg;
        throw ECOMCompression(err.str());
    }
}

/*****************************************************************************/

template <class T>
void COMCompressionT_ZLib<T>::uncompress(const char *input,
                                         unsigned int size,
                                         unsigned int length)
{
    stringstream err;

    free();

    try
    {
        _base64.decode(input, size);
        _zlib.uncompress(_base64.output(), _base64.output_size(),
                         length * sizeof(T));
    }
    catch (ECOMBase64 &e)
    {
        err << "While Base64-decoding: " << e.msg << endl;
        throw ECOMCompression(err.str());
    }
    catch (ECOMZLib &e)
    {
        err << "While ZLib-uncompressing: " << e.msg << endl;
        throw ECOMCompression(err.str());
    }

    if (_zlib.output_size() != length * sizeof(T))
    {
        err << "ZLib output does not have expected size: ";
        err << _zlib.output_size() << " / " << length * sizeof(T);
        throw ECOMCompression(err.str());
    }
}

/*****************************************************************************/

template <class T>
void COMCompressionT_ZLib<T>::clear()
{
}

/*****************************************************************************/

template <class T>
void COMCompressionT_ZLib<T>::flush_compress()
{
    free();
}

/*****************************************************************************/

template <class T>
void COMCompressionT_ZLib<T>::flush_uncompress(const char *input,
                                               unsigned int size)
{
    free();
}

/*****************************************************************************/

template<class T>
const char *COMCompressionT_ZLib<T>::compression_output() const
{
    return _base64.output();
}

/*****************************************************************************/

template<class T>
unsigned int COMCompressionT_ZLib<T>::compressed_size() const
{
    return _base64.output_size();
}

/*****************************************************************************/

template<class T>
const T *COMCompressionT_ZLib<T>::decompression_output() const
{
    return (T *) _zlib.output();
}

/*****************************************************************************/

template<class T>
unsigned int COMCompressionT_ZLib<T>::decompressed_length() const
{
    return _zlib.output_size() / sizeof(T);
}

/*****************************************************************************/
//
//  MDCT / ZLib / Base64
//
/*****************************************************************************/

/**
   Kompressionsobjekt: Erst MDCT, dann ZLib und dann Base64
*/

template <class T>
class COMCompressionT_MDCT : public COMCompressionT<T>
{
public:
    COMCompressionT_MDCT(unsigned int, double);
    ~COMCompressionT_MDCT();

    void compress(const T *input,
                  unsigned int length);
    void uncompress(const char *input,
                    unsigned int size,
                    unsigned int length);
    void clear();
    void flush_compress();
    void flush_uncompress(const char *input,
                          unsigned int size);

    void free();

    const char *compression_output() const;
    unsigned int compressed_size() const;
    const T *decompression_output() const;
    unsigned int decompressed_length() const;

private:
    COMBase64 _base64;  /**< Base64-Objekt zum Kodieren */
    COMZLib _zlib;      /**< ZLib-Objekt zum Komprimieren */
    COMMDCTT<T> *_mdct; /**< MDCT-Objekt zum Transformieren */

    COMCompressionT_MDCT() {}; // privat!
};

/*****************************************************************************/

template <class T>
COMCompressionT_MDCT<T>::COMCompressionT_MDCT(unsigned int dim,
                                              double acc)
{
    _mdct = 0;

    try
    {
        _mdct = new COMMDCTT<T>(dim, acc);
    }
    catch (ECOMMDCT &e)
    {
        throw ECOMCompression(e.msg);
    }
    catch (...)
    {
        throw ECOMCompression("Could not allocate memory for MDCT object!");
    }
}

/*****************************************************************************/

template <class T>
COMCompressionT_MDCT<T>::~COMCompressionT_MDCT()
{
    if (_mdct) delete _mdct;
}

/*****************************************************************************/

template <class T>
void COMCompressionT_MDCT<T>::free()
{
    _zlib.free();
    _base64.free();
}

/*****************************************************************************/

template <class T>
void COMCompressionT_MDCT<T>::compress(const T *input,
                                       unsigned int length)
{
    stringstream err;

    try
    {
        _mdct->transform(input, length);
        _zlib.compress((char *) _mdct->mdct_output(),
                       _mdct->mdct_output_size());
        _base64.encode(_zlib.output(), _zlib.output_size());
    }
    catch (ECOMMDCT &e)
    {
        err << "MDCT: " << e.msg;
        throw ECOMCompression(err.str());
    }
    catch (ECOMZLib &e)
    {
        err << "ZLib: " << e.msg;
        throw ECOMCompression(err.str());
    }
    catch (ECOMBase64 &e)
    {
        err << "Base64: " << e.msg;
        throw ECOMCompression(err.str());
    }
}

/*****************************************************************************/

template <class T>
void COMCompressionT_MDCT<T>::uncompress(const char *input,
                                         unsigned int size,
                                         unsigned int length)
{
    stringstream err;
    unsigned int max_size;

    max_size = _mdct->max_compressed_size(length);

    try
    {
        _base64.decode(input, size);
        _zlib.uncompress(_base64.output(), _base64.output_size(), max_size);
        _mdct->detransform(_zlib.output(), length);
    }
    catch (ECOMBase64 &e)
    {
        err << "While Base64-decoding: " << e.msg << endl;
        throw ECOMCompression(err.str());
    }
    catch (ECOMZLib &e)
    {
        err << "While ZLib-uncompressing: " << e.msg << endl;
        throw ECOMCompression(err.str());
    }
    catch (ECOMMDCT &e)
    {
        err << "While MDCT-detransforming: " << e.msg << endl;
        throw ECOMCompression(err.str());
    }
}

/*****************************************************************************/

template <class T>
void COMCompressionT_MDCT<T>::clear()
{
    _mdct->clear();
}

/*****************************************************************************/

template <class T>
void COMCompressionT_MDCT<T>::flush_compress()
{
    stringstream err;

    try
    {
        _mdct->flush_transform();
        _zlib.compress(_mdct->mdct_output(), _mdct->mdct_output_size());
        _base64.encode(_zlib.output(), _zlib.output_size());
    }
    catch (ECOMMDCT &e)
    {
        err << "MDCT flush: " << e.msg;
        throw ECOMCompression(err.str());
    }
    catch (ECOMZLib &e)
    {
        err << "ZLib: " << e.msg;
        throw ECOMCompression(err.str());
    }
    catch (ECOMBase64 &e)
    {
        err << "Base64: " << e.msg;
        throw ECOMCompression(err.str());
    }
}

/*****************************************************************************/

template <class T>
void COMCompressionT_MDCT<T>::flush_uncompress(const char *input,
                                               unsigned int size)
{
    stringstream err;
    unsigned int max_size;

    // Die maximale Datengröße vor ZLib ermitteln
    max_size = _mdct->max_compressed_size(0);

    try
    {
        _base64.decode(input, size);
        _zlib.uncompress(_base64.output(), _base64.output_size(), max_size);
        _mdct->flush_detransform(_zlib.output(), _zlib.output_size());
    }
    catch (ECOMBase64 &e)
    {
        err << "While Base64-decoding: " << e.msg << endl;
        throw ECOMCompression(err.str());
    }
    catch (ECOMZLib &e)
    {
        err << "While ZLib-uncompressing: " << e.msg << endl;
        throw ECOMCompression(err.str());
    }
    catch (ECOMMDCT &e)
    {
        err << "While MDCT-detransforming: " << e.msg << endl;
        throw ECOMCompression(err.str());
    }
}

/*****************************************************************************/

template<class T>
const char *COMCompressionT_MDCT<T>::compression_output() const
{
    return _base64.output();
}

/*****************************************************************************/

template<class T>
unsigned int COMCompressionT_MDCT<T>::compressed_size() const
{
    return _base64.output_size();
}

/*****************************************************************************/

template<class T>
const T *COMCompressionT_MDCT<T>::decompression_output() const
{
    return _mdct->imdct_output();
}

/*****************************************************************************/

template<class T>
unsigned int COMCompressionT_MDCT<T>::decompressed_length() const
{
    return _mdct->imdct_output_length();
}

/*****************************************************************************/
//
//  Quant / ZLib / Base64
//
/*****************************************************************************/

/**
   Kompressionsobjekt: Erst Quantisierung, dann ZLib, dann Base64
*/

template <class T>
class COMCompressionT_Quant : public COMCompressionT<T>
{
public:
    COMCompressionT_Quant(double);
    ~COMCompressionT_Quant();

    void compress(const T *input,
                  unsigned int length);
    void uncompress(const char *input,
                    unsigned int size,
                    unsigned int length);
    void clear();
    void flush_compress();
    void flush_uncompress(const char *input,
                          unsigned int size);

    void free();

    const char *compression_output() const;
    unsigned int compressed_size() const;
    const T *decompression_output() const;
    unsigned int decompressed_length() const;

private:
    COMQuantT<T> *_quant;      /**< Quantisierungs-Objekt */
    COMZLib _zlib;            /**< ZLib-Objekt zum Komprimieren */
    COMBase64 _base64;        /**< Base64-Objekt zum Kodieren */

    COMCompressionT_Quant();
};

/*****************************************************************************/

template <class T>
COMCompressionT_Quant<T>::COMCompressionT_Quant(double acc)
{
    _quant = 0;

    try
    {
        _quant = new COMQuantT<T>(acc);
    }
    catch (ECOMQuant &e)
    {
        throw ECOMCompression(e.msg);
    }
    catch (...)
    {
        throw ECOMCompression("Could not allocate memory for"
                              " quantization object!");
    }
}

/*****************************************************************************/

template <class T>
COMCompressionT_Quant<T>::~COMCompressionT_Quant()
{
    free();

    if (_quant) delete _quant;
}

/*****************************************************************************/

template <class T>
void COMCompressionT_Quant<T>::free()
{
    if (_quant) _quant->free();

    _zlib.free();
    _base64.free();
}

/*****************************************************************************/

template <class T>
void COMCompressionT_Quant<T>::compress(const T *input,
                                        unsigned int length)
{
    stringstream err;

    if (!_quant) throw COMException("No quantization object!");

    try
    {
        _quant->quantize(input, length);
        _zlib.compress(_quant->quant_output(), _quant->quant_output_size());
        _base64.encode(_zlib.output(), _zlib.output_size());
    }
    catch (ECOMQuant &e)
    {
        err << "Quant: " << e.msg;
        throw ECOMCompression(err.str());
    }
    catch (ECOMZLib &e)
    {
        err << "ZLib: " << e.msg;
        throw ECOMCompression(err.str());
    }
    catch (ECOMBase64 &e)
    {
        err << "Base64: " << e.msg;
        throw ECOMCompression(err.str());
    }
}

/*****************************************************************************/

template <class T>
void COMCompressionT_Quant<T>::uncompress(const char *input,
                                          unsigned int size,
                                          unsigned int length)
{
    stringstream err;

    if (!_quant) throw COMException("No quantization object!");

    free();

    try
    {
        _base64.decode(input, size);
        _zlib.uncompress(_base64.output(),
                         _base64.output_size(),
                         length * sizeof(T));
        _quant->dequantize(_zlib.output(), _zlib.output_size(), length);
    }
    catch (ECOMBase64 &e)
    {
        err << "While Base64-decoding: " << e.msg << endl;
        throw ECOMCompression(err.str());
    }
    catch (ECOMZLib &e)
    {
        err << "While ZLib-uncompressing: " << e.msg << endl;
        throw ECOMCompression(err.str());
    }
    catch (ECOMQuant &e)
    {
        err << "While de-quantizing: " << e.msg << endl;
        throw ECOMCompression(err.str());
    }

    if (_quant->dequant_output_length() != length)
    {
        err << "Quantization output does not have expected length: ";
        err << _quant->dequant_output_length() << " / " << length;
        throw ECOMCompression(err.str());
    }
}

/*****************************************************************************/

template <class T>
void COMCompressionT_Quant<T>::clear()
{
}

/*****************************************************************************/

template <class T>
void COMCompressionT_Quant<T>::flush_compress()
{
    free();
}

/*****************************************************************************/

template <class T>
void COMCompressionT_Quant<T>::flush_uncompress(const char *input,
                                                unsigned int size)
{
    free();
}

/*****************************************************************************/

template<class T>
const char *COMCompressionT_Quant<T>::compression_output() const
{
    return _base64.output();
}

/*****************************************************************************/

template<class T>
unsigned int COMCompressionT_Quant<T>::compressed_size() const
{
    return _base64.output_size();
}

/*****************************************************************************/

template<class T>
const T *COMCompressionT_Quant<T>::decompression_output() const
{
    if (!_quant) throw COMException("No quantization object!");

    return _quant->dequant_output();
}

/*****************************************************************************/

template<class T>
unsigned int COMCompressionT_Quant<T>::decompressed_length() const
{
    if (!_quant) throw COMException("No quantization object!");

    return _quant->dequant_output_length();
}

/*****************************************************************************/

#ifdef DEBUG
#undef DEBUG
#endif

#endif
