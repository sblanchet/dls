/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef COMMDCTTHpp
#define COMMDCTTHpp

#include <math.h>

/*****************************************************************************/

//#define MDCT_DEBUG

#define MDCT_MAX_BYTES 4 // Maximal 32 Bit für Quantisierung

/*****************************************************************************/

#ifdef MDCT_DEBUG
#include <iomanip>
using namespace std;
#endif

#include "com_globals.hpp"
#include "com_exception.hpp"
#include "mdct.hpp"

/*****************************************************************************/

/**
   Exception eines COMMDCTT-Objektes
*/

class ECOMMDCT : public COMException
{
public:
    ECOMMDCT(const string &pmsg) : COMException(pmsg) {};
};

/*****************************************************************************/

/**
   Überlappende, diskrete Cosinus-Transformation
*/

template <class T>
class COMMDCTT
{
public:
    COMMDCTT(unsigned int, double);
    ~COMMDCTT();

    void transform(const T *, unsigned int);
    void detransform(const char *, unsigned int);
    void flush_transform();
    void flush_detransform(const char *, unsigned int);
    void clear();
    unsigned int max_compressed_size(unsigned int) const;

    //@{
    const char *mdct_output() const;
    unsigned int mdct_output_size() const;
    const T *imdct_output() const;
    unsigned int imdct_output_length() const;
    //@}

    unsigned int block_size() const;

private:
    //@{
    unsigned int _dim;  /**< Dimension einer Einzel-DCT */
    unsigned int _exp2; /**< Exponent zur Basis 2 der Dimension */
    double _accuracy;   /**< Genauigkeit */
    //@}

    //@{
    char *_mdct_output; /**< Ausgabespeicher für die MDCT */
    unsigned int _mdct_output_size; /**< Bytes im Ausgabespeicher MDCT */
    T *_imdct_output;                  /**< Ausgabespeicher für IMDCT */
    unsigned int _imdct_output_length; /**< Werte im Ausgabespeicher IMDCT */
    //@}

    //@{
    T *_last_tail; /**< Die (_dim / 2) letzten Werte der letzten MDCT */
    bool _first; /**< true, wenn noch keine MDCT stattgefunden hat */
    unsigned int _last_length; /**< Anzahl Datenwerte der letzten MDCT */
    //@}

    //@{
    unsigned int _transform_all(const double *, unsigned int, char *);
    void _detransform_all(const char *, unsigned int, T *);
    //@}

    void _int_quant(const double *, unsigned char, int *, double *, double *);
    unsigned int _store_quant(unsigned char, int *, char *);

    COMMDCTT(); // Privater Default-Konstruktor (soll nicht aufgerufen werden!)
};

/*****************************************************************************/

/**
   Konstruktor

   Prüft zuerst die angegebene Dimension, da nur bestimmte
   Zweierpotenzen gültig sind. Initialisiert dann die MDCT.

   \param dim Dimension der Einzel-DCTs
   \param acc Genauigkeit der Einzel-DCTs
*/

template <class T>
COMMDCTT<T>::COMMDCTT(unsigned int dim, double acc)
{
    int init_ret;
    double log2;
    stringstream err;

    _dim = 0;
    _exp2 = 0;
    _mdct_output = 0;
    _imdct_output = 0;
    _last_tail = 0;
    _first = true;
    _last_length = 0;

    _accuracy = acc;

    // Dimension muss Potenz von 2 sein!
    log2 = log10((double) dim) / log10((double) 2);
    if (log2 != (unsigned int) log2)
    {
        err << "Invalid dimension " << dim;
        err << " (must be power of 2)!";
        throw ECOMMDCT(err.str());
    }

    if ((init_ret = mdct_init((unsigned int) log2)) < 0)
    {
        err << "Could not init MDCT! (Error code " << init_ret << ")";
        throw ECOMMDCT(err.str());
    }

    // Dimension ok
    _dim = dim;
    _exp2 = (unsigned int) log2;

    try
    {
        // Persistenten Speicher für letzte, halbe Dimension reservieren
        _last_tail = new T[_dim / 2];
    }
    catch (...)
    {
        throw ECOMMDCT("Could not allocate memory for tail buffer!");
    }

    // Persistenten Speicher leeren
    clear();
}

/*****************************************************************************/

/**
   Destruktor

   Gibt alle reservierten Speicherbereiche frei.
*/

template <class T>
COMMDCTT<T>::~COMMDCTT()
{
    if (_last_tail) delete [] _last_tail;
    if (_mdct_output) delete [] _mdct_output;
    if (_imdct_output) delete [] _imdct_output;
}

/*****************************************************************************/

/**
   Persistenten Speicher leeren

   Sorgt dafür, dass der persistente, letzte Teil der letzten
   MDCT gelöscht (mit Nullen ersetzt) wird.
*/

template <class T>
void COMMDCTT<T>::clear()
{
    unsigned int i;

#ifdef MDCT_DEBUG
    msg() << "Clearing data of last MDCT!";
    log(DLSDebug);
#endif

    _first = true;
    _last_length = 0;

    if (!_last_tail) return;

    for (i = 0; i < _dim / 2; i++)
    {
        _last_tail[i] = 0;
    }
}

/*****************************************************************************/

/**
   Liefert die maximale Größe komprimierter MDCT-Daten

   Ist length 0, so wird die maximale Datengröße eines
   von flush_compress() komprimierten Blockes geliefert.

   \param length Anzahl komprimierter Datenwerte
   \return Maximale Größe in Bytes
*/

template <class T>
unsigned int COMMDCTT<T>::max_compressed_size(unsigned int length) const
{
    unsigned int blocks_of_dim;

    if (length)
    {
        // Anzahl der vollen Dimensionen ermitteln
        blocks_of_dim = length / _dim;
        if (length % _dim) blocks_of_dim++;
        return blocks_of_dim * 2 * (sizeof(T) + 1 + MDCT_MAX_BYTES * _dim / 2);
    }

    else // MDCT-Flushing
    {
        return sizeof(T) + 1 + MDCT_MAX_BYTES * _dim / 2;
    }
}

/*****************************************************************************/

/**
   Führt eine MDCT aus

   Prüft, ob die Eingabewerte auf eine ganze Anzahl von Blöcken
   der Dimension aufgefüllt werden muss. Reserviert dann den
   entsprechenden Speicher[, differenziert die Eingabewerte]
   und kopiert sie in diesen Puffer. Führt dann alle nötigen
   Einzel-DCTs aus und kopiert das Ergebnis in den Augabe-Puffer.

   \param input Konstanter Zeiger auf einen Speicher mit Werten,
   die transformiert werden sollen
   \param input_length Anzahl der Werte im Eingabepuffer
   \throw ECOMMDCT Es konnte nicht genug Speicher reserviert werden
*/

template <class T>
void COMMDCTT<T>::transform(const T *input, unsigned int input_length)
{
    unsigned int i, blocks_of_dim;
    double *mdct_buffer;

#ifdef MDCT_DEBUG
    msg() << "MDCT::transform() len=" << input_length << " dim=" << _dim;
    log(DLSDebug);
#endif

    _mdct_output_size = 0;

    if (!_dim) return; // Division durch 0 verhindern
    if (!input_length) return; // Keine Daten - Keine MDCT!

    // Anzahl der vollen Dimensionen ermitteln
    blocks_of_dim = input_length / _dim;

    // Anzahl Werte kein Vielfaches der Dimension. Die nächstgrößere,
    // ganzzahlige Blockanzahl benutzen, um später mit Nullen aufzufüllen
    if (input_length % _dim) blocks_of_dim++;

    if (_mdct_output)
    {
        // Den alten Ausgabepuffer freigeben
        delete [] _mdct_output;
        _mdct_output = 0;
    }

    try
    {
        // Speicher für die Ausgabe reservieren
        // Größe: blocks_of_dim * 2 DCTs
        _mdct_output = new char[blocks_of_dim * 2 *
                                (sizeof(T) + 1 + MDCT_MAX_BYTES * _dim / 2)];

        // Speicher für den MDCT-Puffer reservieren.
        // Dabei eine Halbe Dimension mehr für den "Übertrag"
        // der letzten MDCT einplanen
        mdct_buffer = new double[_dim / 2 + blocks_of_dim * _dim];
    }
    catch (...)
    {
        throw ECOMMDCT("Could not allocate memory for buffers!");
    }

    if (_first)
    {
        // Vorne mit erstem Wert der Daten anfüllen
        for (i = 0; i < _dim / 2; i++) mdct_buffer[i] = (double) input[0];
    }
    else
    {
        // Rest des letzten Blockes vorne anfügen
        for (i = 0; i < _dim / 2; i++) mdct_buffer[i] = (double) _last_tail[i];
    }

    // Daten in den MDCT-Puffer kopieren
    for (i = 0; i < input_length; i++)
        mdct_buffer[_dim / 2 + i] = (double) input[i];

#ifdef MDCT_DEBUG
#if 0
    msg() << "Input buffer:";
    log(DLSDebug);
    for (i = 0; i < input_length; i++)
    {
        msg() << convert_to_bin(&input[i], 8, -8) << " "
              << setw(15) << input[i];
        log(DLSDebug);
    }
#endif
#endif

    // Eventuell hinten auffüllen (mit dem letzten Datenwert)
    for (i = input_length; i < blocks_of_dim * _dim; i++)
    {
        mdct_buffer[_dim / 2 + i] = (double) input[input_length - 1];
    }

    // Die letzte, halbe Dimension von Werten (undifferenziert) speichern
    for (i = 0; i < _dim / 2; i++)
    {
        _last_tail[i] = (T) mdct_buffer[blocks_of_dim * _dim + i];
    }

#ifdef MDCT_DEBUG
#if 0
    msg() << "MDCT buffer before transform:";
    log(DLSDebug);
    for (i = 0; i < blocks_of_dim * _dim; i++) msg() << mdct_buffer[i] << ", ";
    log(DLSDebug);
#endif
#endif

    // DCT's ausführen
    _mdct_output_size = _transform_all(mdct_buffer, blocks_of_dim * 2,
                                       _mdct_output);

#ifdef MDCT_DEBUG
#if 1
    msg() << "Output buffer after transform:";
    log(DLSDebug);
    msg() << convert_to_bin(_mdct_output, _mdct_output_size, 8);
    log(DLSDebug);
#endif
#endif

    _first = false;
    _last_length = input_length;

    // MDCT-Puffer wieder freigeben
    delete [] mdct_buffer;
}

/*****************************************************************************/

/**
   Führt alle nötigen Einzel-DCTs aus

   Die Ergebnisvektoren werden sortiert in den Ausgabepuffer
   gespeichert, und zwar so, dass alle ersten Werte vorne
   stehen, dann alle zweiten Werte, usw.

   \param input Konstanter Zeiger auf den Puffer mit den
   zu transformiernden Werten
   \param dct_count Anzahl auszuführender DCTs
   \param output Zeiger auf den Ausgabespeicher
   \return Größe der Daten im Ausgabespeicher
*/

template <class T>
unsigned int COMMDCTT<T>::_transform_all(const double *input,
                                         unsigned int dct_count,
                                         char *output)
{
    unsigned int d, i, data_size = 0;
    double coeff[_dim / 2];
    int intquant[_dim / 2];
    double quant[_dim / 2];
    double coeff_imdct[_dim];
    double quant_imdct[_dim];
    double max_error, error;
    double scale;
    unsigned char bits, use_bits, start, end;

    // Alle DCTs durchführen
    for (d = 0; d < dct_count; d++)
    {
#ifdef MDCT_DEBUG
        msg() << "Values:";
        log(DLSDebug);
        msg() << "SEEEEEEE EEEEMMMM MMMMMMMM MMMMMMMM "
              << "MMMMMMMM MMMMMMMM MMMMMMMM MMMMMMMM";
        log(DLSDebug);
        for (i = 0; i < _dim; i++)
        {
            msg() << convert_to_bin(&(input + (d * _dim / 2))[i], 8, -8);
            msg() << " " << input[d * _dim / 2 + i];
            log(DLSDebug);
        }
#endif

        // Transformieren
        mdct(_exp2, input + (d * _dim / 2), coeff);

#ifdef MDCT_DEBUG
        msg() << "Coeffs:";
        log(DLSDebug);
        for (i = 0; i < _dim / 2; i++) {
            msg() << setw(15) << coeff[i];
            log(DLSDebug);
        }
#endif

        // Rücktransformieren
        imdct(_exp2, coeff, coeff_imdct);

        // Start- und Endpunkt für Bisektion setzen
        start = 2;
        end = MDCT_MAX_BYTES * 8 - 1;

        // Noch keine verwendbare Bitanzahl gefunden
        use_bits = 0;

        while (start <= end)
        {
            bits = (end - start + 1) / 2 + start;

            // Koeffizienten quantisieren
            _int_quant(coeff, bits, intquant, quant, &scale);

            // IMDCT mit quantisierten Koeffizienten
            imdct(_exp2, quant, quant_imdct);

            // Abweichung der Halbkurve berechnen
            max_error = 0;
            for (i = 0; i < _dim; i++)
            {
                error = fabs(quant_imdct[i] - coeff_imdct[i]);
                if (error > max_error) max_error = error;
            }

#ifdef MDCT_DEBUG
            msg() << "Quant with " << (int) bits << " bits. IMDCT error "
                  << max_error;
            log(DLSDebug);
#endif

            if (max_error < _accuracy / 2) // Fehler unter Toleranz
            {
                // Diese Anzahl Bits könnte zum Quantisieren genutzt werden
                use_bits = bits;

                // Probieren, ob weniger Bits genügen würden
                end = bits - 1;
            }
            else // Fehler zu groß!
            {
                // Mehr Bits verwenden
                start = bits + 1;
            }
        }

        if (!use_bits) // Der Fehler war immer zu groß
        {
            // Maximale Anzahl Bits zum Quantisieren verwenden
            use_bits = MDCT_MAX_BYTES * 8 - 1;

            // Warnung ausgeben!
            msg() << "MDCT - Could not reach maximal error of "
                  << _accuracy / 2;
            msg() << ". Quantizing with " << (int) bits
                  << " bits. Error is " << max_error;
            log(DLSWarning);
        }

#ifdef MDCT_DEBUG
        msg() << "Using quant " << (int) use_bits;
        log(DLSDebug);
        for (i = 0; i < _dim / 2; i++)
        {
            msg() << convert_to_bin(&intquant[i], sizeof(int), -sizeof(int))
                  << " " << setw(15) << intquant[i];
            log(DLSDebug);
        }
#endif

        // Skalierungsfaktor speichern
        *((T *) (output + data_size)) = scale;
        data_size += sizeof(T);

        // Anzahl Quantisierungsbits speichern
        *(output + data_size) = use_bits;
        data_size++;

        // Koeffizienten in den Ausgabepuffer kopieren
        data_size += _store_quant(use_bits, intquant, output + data_size);

#ifdef MDCT_DEBUG
        msg() << "Total packed (" << data_size << " bytes):";
        log(DLSDebug);
        msg() << convert_to_bin(output, data_size, 8);
        log(DLSDebug);
#endif
    } // DCTs

    return data_size;
}

/*****************************************************************************/

/**
   Absolute Quantisierung

   Nimmt eine absolute Quantisierung der double-Koeffizienten
   vor. D. h. es wird ein Raster über den gesamten Wertebereich
   gelegt und ähnliche Werte mit dem selben Integer-Wert
   repräsentiert.

   \param koeff Array mit _dim / 2 Koeffizienten
   \param bits Anzahl Bits, auf die Quantisiert werden soll
   \param intquant Array mit _dim / 2 Integern zur Ablage
   der Integer-Koeffizienten
   \param quant Array mit _dim / 2 doubles für die Ablage der
   modifizierten Koeffizienten
   \param scale Zeiger auf den Skalierungsfaktor (wird während
   der Verarbeitung beschrieben)
*/

template <class T>
void COMMDCTT<T>::_int_quant(const double *koeff,
                             unsigned char bits,
                             int *intquant,
                             double *quant,
                             double *scale)
{
    unsigned int i;
    double abs_value, max = 0;

    // Division durch 0 verhindern
    if (bits < 2) return;

    // Maximum ermitteln
    for (i = 0; i < _dim / 2; i++)
    {
        abs_value = fabs(koeff[i]);
        if (abs_value > max) max = abs_value;
    }

    // Skalierung berechnen
    *scale = 2 * max / ((1 << bits) - 1);

    // Alle Koeffizienten skalieren
    for (i = 0; i < _dim / 2; i++)
    {
        // Skalierte Koeffizienten im Integer-Array ablegen
        intquant[i] = (int) round(koeff[i] / (*scale));
        //HM 26.02.2005 geaendert auf round statt floor

        // Integer wieder rück-skalieren und im double-Array speichern
        quant[i] = intquant[i] * (*scale);
    }
}

/*****************************************************************************/

/**
   Gepacktes Speichern der quantisierten Koeffizienten

   \return Größe des Benötigten Speichers in Bytes
*/

template <class T>
unsigned int COMMDCTT<T>::_store_quant(unsigned char bits,
                                       int *intquant,
                                       char *output)
{
    unsigned int current_byte, bits_free, i;
    unsigned char b;

    current_byte = 0;
    bits_free = 8;
    output[current_byte] = 0;

#ifdef MDCT_DEBUG
    msg() << "Abs quant";
    log(DLSDebug);
#endif

    // Zuerst die Vorzeichenbits hintereinander an den Anfang schreiben
    for (i = 0; i < _dim / 2; i++)
    {
        if (intquant[i] < 0)
        {
            output[current_byte] |= 1 << (7 - (i % 8));
            intquant[i] *= -1;
        }

#ifdef MDCT_DEBUG
        msg() << convert_to_bin(&intquant[i], sizeof(int), -sizeof(int))
              << " " << intquant[i];
        log(DLSDebug);
#endif

        if (--bits_free == 0)
        {
            current_byte++;
            output[current_byte] = 0;
            bits_free = 8;
        }
    }

    // Nun die quantisierten Koeffizienten "transponiert" speichern
    for (b = bits; b > 0; b--)
    {
        for (i = 0; i < _dim / 2; i++)
        {
            if (!bits_free)
            {
                current_byte++;
                output[current_byte] = 0;
                bits_free = 8;
            }

            if (intquant[i] & (1 << (b - 1)))
                output[current_byte] |= 1 << (bits_free - 1);

            bits_free--;
        }
    }

    return current_byte + 1;
}

/*****************************************************************************/

/**
   Führt die letzte DCT aus

   \throw ECOMMDCT Es konnte nicht genug Speicher allokiert werden
*/

template <class T>
void COMMDCTT<T>::flush_transform()
{
    unsigned int i;
    double *mdct_buffer;

#ifdef MDCT_DEBUG
    msg() << "MDCT::flush_transform() dim=" << _dim
          << " last_len=" << _last_length;
    log(DLSDebug);
#endif

    _mdct_output_size = 0;

    if (!_dim) return; // Division durch 0 verhindern

    // Wenn die letzte MDCT über weniger als _dim/2
    // Werte ging, ist ein Überhangblock unnötig.
    if ((_last_length % _dim) <= _dim / 2) return;

    if (_mdct_output)
    {
        delete [] _mdct_output;
        _mdct_output = 0;
    }

    try
    {
        // Speicher für die Ausgabe reservieren
        _mdct_output = new char[sizeof(T) + 1 + MDCT_MAX_BYTES * _dim / 2];

        // Speicher für den MDCT-Puffer reservieren.
        mdct_buffer = new double[_dim];
    }
    catch (...)
    {
        throw ECOMMDCT("Could not allocate memory for buffers!");
    }

    // Rest des letzten Blockes vorne anfügen
    for (i = 0; i < _dim / 2; i++) mdct_buffer[i] = (double) _last_tail[i];

    // Rest des Speichers mit dem letzten Wert der ersten Hälfte auffüllen
    for (i = _dim / 2; i < _dim; i++)
        mdct_buffer[i] = mdct_buffer[_dim / 2 - 1];

    // Transformieren
    _mdct_output_size = _transform_all(mdct_buffer, 1, _mdct_output);

    // MDCT-Puffer wieder freigeben
    delete [] mdct_buffer;
}

/*****************************************************************************/

/**
   Führt eine MDCT-Rücktransformation aus

   \param input Konstanter Zeiger auf ein Array mit
   geordneten MDCT-Koeffizienten
   \param input_length Anzahl der Werte im input-Array
*/

template <class T>
void COMMDCTT<T>::detransform(const char *input,
                              unsigned int input_length)
{
    unsigned int blocks_of_dim, i;
    T *mdct_buffer;
    stringstream err;

#ifdef MDCT_DEBUG
    msg() << "MDCT::detransform() len=" << input_length << " dim=" << _dim;
    if (_first) msg() << " FIRST";
    log(DLSDebug);
#endif

    _imdct_output_length = 0;

    if (!_dim) return; // Bei Fehler: Division durch 0 verhindern!
    if (input_length < 2) return; // Keine Daten - keine MDCT!

    // Anzahl der nötigen Blocks ermitteln
    blocks_of_dim = input_length / _dim;

    // Wenn nötig, auf ganze Blockzahl auffüllen
    if (input_length % _dim) blocks_of_dim++;

#ifdef MDCT_DEBUG
    msg() << "blocks_of_dim=" << blocks_of_dim;
    log(DLSDebug);
#endif

    if (_imdct_output)
    {
        // Den alten Ausgabepuffer freigeben
        delete [] _imdct_output;
        _imdct_output = 0;
    }

    try
    {
        _imdct_output = new T[blocks_of_dim * _dim];
        mdct_buffer = new T[_dim / 2 + blocks_of_dim * _dim];
    }
    catch (...)
    {
        throw ECOMMDCT("Could not allocate memory for buffers!");
    }

    // Die letzte, halbe Dimension der letzten
    // Rücktransformation in den Puffer kopieren
    for (i = 0; i < _dim / 2; i++)
    {
        mdct_buffer[i] = _last_tail[i];
    }

    // Den Rest auf 0 setzen
    for (i = 0; i < blocks_of_dim * _dim; i++)
    {
        mdct_buffer[_dim / 2 + i] = 0;
    }

    // Alle inversen Transformationen ausführen
    _detransform_all(input, blocks_of_dim * 2, mdct_buffer);

    _imdct_output_length = blocks_of_dim * _dim;
    if ((input_length % _dim) > 0 && (input_length % _dim) < _dim / 2)
    {
        _imdct_output_length -= _dim / 2 - (input_length % _dim);
    }

    // Daten in den Ausgabepuffer kopieren
    if (_first)
    {
        _imdct_output_length -= _dim / 2;

        for (i = 0; i < _imdct_output_length; i++)
        {
            _imdct_output[i] = mdct_buffer[_dim / 2 + i];
        }
    }
    else
    {
        for (i = 0; i < _imdct_output_length; i++)
        {
            _imdct_output[i] = mdct_buffer[i];
        }
    }

#ifdef MDCT_DEBUG
    msg() << "output_length=" << _imdct_output_length;
    log(DLSDebug);
#endif

    // Daten der letzten, halben Dimension speichern
    for (i = 0; i < _dim / 2; i++)
    {
        _last_tail[i] = mdct_buffer[blocks_of_dim * _dim + i];
    }

#ifdef MDCT_DEBUG
    msg() << "Deleting mdct buffer.";
    log(DLSDebug);
#endif

    delete [] mdct_buffer;

    _first = false;
    _last_length = input_length;
}

/*****************************************************************************/

template <class T>
void COMMDCTT<T>::_detransform_all(const char *input,
                                   unsigned int dct_count,
                                   T *output)
{
    unsigned int d, i, current_byte, current_bit;
    char signs[_dim / 2];
    int int_coeff[_dim / 2];
    double coeff[_dim / 2];
    double coeff_imdct[_dim];
    double scale;
    unsigned char bits, b;

    current_byte = 0;
    current_bit = 8;

    // Alle inversen DCTs durchführen
    for (d = 0; d < dct_count; d++)
    {
        // Wenn nötig zum nächsten, vollen Byte wechseln
        if (current_bit != 8)
        {
            current_byte++;
            current_bit = 8;
        }

#ifdef MDCT_DEBUG
        msg() << "OFFSET " << current_byte;
        log(DLSDebug);
#endif

        // Integer-Koeffizienten mit Nullen vorbelegen
        for (i = 0; i < _dim / 2; i++) int_coeff[i] = 0;

        // Skalierungksfaktor aus den komprimierten Daten lesen
        scale = (double) *((T *) (input + current_byte));
        current_byte += sizeof(T);

        // Anzahl der Quantisierungsbits lesen
        bits = (unsigned char) *(input + current_byte);
        current_byte++;

#ifdef MDCT_DEBUG
        msg() << "DCT " << d << ": scale=" << scale << ", bits=" << (int) bits;
        log(DLSDebug);
#endif

        // Vorzeichenbits auslesen
        for (i = 0; i < _dim / 2; i++)
        {
            if (*(input + current_byte) & (1 << (current_bit - 1)))
                signs[i] = -1;
            else signs[i] = 1;

            if (--current_bit == 0)
            {
                current_byte++;
                current_bit = 8;
            }
        }

#ifdef MDCT_DEBUG
        msg() << "Signs:";
        log(DLSDebug);
        for (i = 0; i < _dim / 2; i++) msg() << (int) signs[i] << ", ";
        log(DLSDebug);
#endif

        for (b = bits; b > 0; b--)
        {
            for (i = 0; i < _dim / 2; i++)
            {
                if (input[current_byte] & (1 << current_bit - 1))
                    int_coeff[i] |= 1 << (b - 1);

                if (--current_bit == 0)
                {
                    current_byte++;
                    current_bit = 8;
                }
            }
        }

        // Koeffizienten skalieren und mit richtigem Vorzeichen behaften
        for (i = 0; i < _dim / 2; i++)
            coeff[i] = int_coeff[i] * signs[i] * scale;

        // Inverse MDCT ausführen
        imdct(_exp2, coeff, coeff_imdct);

        // Wiederhergestellte Werte in den Ausgabepuffer kopieren
        for (i = 0; i < _dim; i++)
        {
            output[d * _dim / 2 + i] += (T) coeff_imdct[i];
        }
    }
}

/*****************************************************************************/

/**
   Führt die letzte Rück-DCT über dem Blockrest aus

   \param input _dim / 2 Werte für die letzte Rück-DCT
   \param input_size Größe der Restdaten in Bytes
*/

template <class T>
void COMMDCTT<T>::flush_detransform(const char *input, unsigned int input_size)
{
    unsigned int i;
    T *mdct_buffer;

#ifdef MDCT_DEBUG
    msg() << "MDCT::flush_detransform() dim=" << _dim
          << " last_len=" << _last_length;
    log(DLSDebug);
#endif

    _imdct_output_length = 0;

    if (!_dim) return; // Bei Fehler: Division durch 0 verhindern!

    // Wenn die letzte DCT über weniger als _dim / 2 Werte ging,
    // gibt es keinen Überhangblock.
    if ((_last_length % _dim) <= _dim / 2) return;

    if (_imdct_output)
    {
        // Den alten Ausgabepuffer freigeben
        delete [] _imdct_output;
        _imdct_output = 0;
    }

    try
    {
        _imdct_output = new T[_dim / 2];
        mdct_buffer = new T[_dim];
    }
    catch (...)
    {
        throw ECOMMDCT("Could not allocate memory for buffers!");
    }

    // Die letzte, halbe Dimension der letzten
    // Rücktransformation in den Speicher kopieren
    for (i = 0; i < _dim / 2; i++) mdct_buffer[i] = _last_tail[i];

    // Den Rest auf 0 setzen
    for (i = _dim / 2; i < _dim; i++) mdct_buffer[i] = 0;

    // Alle inversen Transformationen ausführen
    _detransform_all(input, 1, mdct_buffer);

    // Daten in den Ausgabepuffer kopieren
    _imdct_output_length = (_last_length % _dim) - _dim / 2;
    for (i = 0; i < _imdct_output_length; i++)
    {
        _imdct_output[i] = mdct_buffer[i];
    }

    // Daten in den Ausgabepuffer kopieren
    for (i = 0; i < _imdct_output_length; i++)
    {
        _imdct_output[i] = mdct_buffer[i];
    }

#ifdef MDCT_DEBUG
    msg() << "Flush output_len=" << _imdct_output_length;
    log(DLSDebug);
    msg() << "Deleting MDCT buffer.";
    log(DLSDebug);
#endif

    delete [] mdct_buffer;
}

/*****************************************************************************/

template <class T>
const char *COMMDCTT<T>::mdct_output() const
{
    return _mdct_output;
}

/*****************************************************************************/

template <class T>
unsigned int COMMDCTT<T>::mdct_output_size() const
{
    return _mdct_output_size;
}

/*****************************************************************************/

template <class T>
const T *COMMDCTT<T>::imdct_output() const
{
    return _imdct_output;
}

/*****************************************************************************/

template <class T>
unsigned int COMMDCTT<T>::imdct_output_length() const
{
    return _imdct_output_length;
}

/*****************************************************************************/

template <class T>
unsigned int COMMDCTT<T>::block_size() const
{
    return _dim;
}

/*****************************************************************************/

#endif
