/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef DLSSaverTHpp
#define DLSSaverTHpp

/*****************************************************************************/

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <iostream>
#include <string>
#include <typeinfo>
using namespace std;

/*****************************************************************************/

#include "dls_globals.hpp"
#include "com_exception.hpp"
#include "com_zlib.hpp"
#include "com_base64.hpp"
#include "com_time.hpp"
#include "com_file.hpp"
#include "com_index_t.hpp"
#include "com_compression_t.hpp"
#include "dls_logger.hpp"

//#define DEBUG

/*****************************************************************************/

/**
   Allgemeine Exception eines Saver-Objekts
*/

class EDLSSaver : public COMException
{
public:
    EDLSSaver(const string &pmsg) : COMException(pmsg) {};
};

/*****************************************************************************/

/**
   Exception eines Saver-Objekts: Zeittoleranzfehler!

   Ein Zeit-Toleranzfehler tritt immer dann auf, wenn
   die Zeiten von zwei aufeinanderfolgenden Datenwerten
   nicht aufeinanderpassen, d. h. der relative Fehler
   einen festgelegten Grenzwert überschreitet.
*/

class EDLSTimeTolerance : public COMException
{
public:
    EDLSTimeTolerance(const string &pmsg) : COMException(pmsg) {};
};

/*****************************************************************************/

/**
   Abstrakte Basisklasse eines Saver-Objekts

   Vereint die Gemeinsamkeiten von DLSSaverGenT und
   DLSSaverMetaT. Diese sind: Verwaltung von Block- und
   Metapuffer, Codierung, Speichern von Blöcken,
   Dateiverwaltung und Indexverwaltung.
*/

template <class T>
class DLSSaverT
{
public:
    DLSSaverT(DLSLogger *);
    virtual ~DLSSaverT();

protected:
    DLSLogger *_parent_logger;        /**< Zeiger auf das besitzende
                                         Logger-Objekt */
    T *_block_buf;                    /**< Array von Datenwerten, die als Block
                                         in die entsprechende Datei gespeichert
                                         werden sollen */
    T *_meta_buf;                     /**< Array von Datenwerten, über die ein
                                         Meta-Wert erzeugt werden soll */
    unsigned int _block_buf_index;    /**< Index des ersten, freien Elementes
                                         im Block-Puffer */
    unsigned int _block_buf_size;     /**< Größe des Block-Puffers */
    unsigned int _meta_buf_index;     /**< Index des ersten, freien Elementes
                                         im Meta-Puffer */
    unsigned int _meta_buf_size;      /**< Größe des Meta-Puffers */
    COMTime _block_time;              /**< Zeit des ersten Datenwertes im
                                         Block-Puffer */
    COMTime _meta_time;               /**< Zeit des ersten Datenwertes im
                                         Meta-Puffer */
    COMTime _time_of_last;            /**< Zeit des letzten Datenwertes beider
                                         Puffer */
    COMCompressionT<T> *_compression; /**< Zeiger auf ein beliebiges
                                         Komprimierungs-Objekt */

    void _save_block();
    void _finish_files();
    void _save_rest();

/**
   Meta-Level ausgeben

   Jedes Kind von DLSSaverT muss diese Methode implementieren,
   damit bei der Verzeichniserstellung die Meta-Ebene
   mit in den Namen einfließen kann.

   \return Meta-Level
*/

    virtual int _meta_level() const = 0;

/**
   Meta-Typ ausgeben

   Jedes Kind von DLSSaverT muss diese Methode implementieren,
   damit bei der Dateierstellung der Meta-Typ
   mit in den Dateinamen einfließen kann.

   \return Meta-Level
*/

    virtual string _meta_type() const = 0;

private:
    COMFile _data_file;  /**< Datei-Objekt zum Speichern der Blöcke */
    COMFile _index_file; /**< Datei-Objekt zum Speichern der Block-Indizes */

    void _begin_files(COMTime);
};

/*****************************************************************************/

/**
   Konstruktor

   \param parent_logger Zeiger auf das besitzende Logger-Objekt
   \throw EDLSSaver Es konnte nicht genug Speicher allokiert werden
*/

template <class T>
DLSSaverT<T>::DLSSaverT(DLSLogger *parent_logger)
{
    stringstream err;
    unsigned int dim;
    double acc;

    _parent_logger = parent_logger;

    _block_buf_size = _parent_logger->channel_preset()->block_size;
    _block_buf_index = 0;
    _meta_buf_size = _parent_logger->channel_preset()->meta_reduction;
    _meta_buf_index = 0;

    _block_buf = 0;
    _meta_buf = 0;
    _compression = 0;

    try
    {
        _block_buf = new T[_block_buf_size];
        _meta_buf = new T[_meta_buf_size];
    }
    catch (...)
    {
        throw EDLSSaver("Could not allocate memory for buffers!");
    }

    try
    {
        if (_parent_logger->channel_preset()->format_index == DLS_FORMAT_ZLIB)
        {
            _compression = new COMCompressionT_ZLib<T>();
        }
        else if (_parent_logger->channel_preset()->format_index
                 == DLS_FORMAT_MDCT)
        {
            dim = _parent_logger->channel_preset()->mdct_block_size;
            acc = _parent_logger->channel_preset()->accuracy;

            if (typeid(T) == typeid(float))
            {
                _compression = (COMCompressionT<T> *)
                    new COMCompressionT_MDCT<float>(dim, acc);
            }
            else if (typeid(T) == typeid(double))
            {
                _compression = (COMCompressionT<T> *)
                    new COMCompressionT_MDCT<double>(dim, acc);
            }
            else
            {
                err << "MDCT only suitable for";
                err << " floating point types, not for "
                    << typeid(T).name() << "!";
            }
        }
        else if (_parent_logger->channel_preset()->format_index
                 == DLS_FORMAT_QUANT)
        {
            acc = _parent_logger->channel_preset()->accuracy;

            if (typeid(T) == typeid(float))
            {
                _compression = (COMCompressionT<T> *)
                    new COMCompressionT_Quant<float>(acc);
            }
            else if (typeid(T) == typeid(double))
            {
                _compression = (COMCompressionT<T> *)
                    new COMCompressionT_Quant<double>(acc);
            }
            else
            {
                err << "Quantization only suitable for";
                err << " floating point types, not for "
                    << typeid(T).name() << "!";
            }
        }
        else
        {
            err << "Unknown channel format index "
                << _parent_logger->channel_preset()->format_index;
        }
    }
    catch (ECOMCompression &e)
    {
        throw EDLSSaver(e.msg);
    }
    catch (...)
    {
        throw EDLSSaver("Could not allocate memory for compression object!");
    }

    if (err.str() != "") throw EDLSSaver(err.str());
}

/*****************************************************************************/

/**
   Destruktor

   Schließt die Daten- und Indexdateien, allerdings ohne
   Fehlerverarbeitung! Gibt die Puffer frei.
*/

template <class T>
DLSSaverT<T>::~DLSSaverT()
{
    if (_compression) delete _compression;
    if (_block_buf) delete [] _block_buf;
    if (_meta_buf) delete [] _meta_buf;
}

/*****************************************************************************/

/**
   Speichert einen Datenblock als XML-Tag in die Ausgabedatei

   Konstruiert zuerst das komplette XML-Tag und prüft dann, ob
   zusammen mit dem bisherigen Dateiinhalt die maximale Dateigröße
   überschritten werden würde. Bei Bedarf wird dann eine neue
   Datei geöffnet.
   Dann wird das XML-Tag in die aktuell offene Datei gespeichert
   und dessen Größe auf die bisherige Dateigröße addiert.

   Achtung! Beim Ändern der Methode bitte auch _save_carry()
   beachten, die ähnlich aufgebaut ist.

   \throw EDLSSaver Datenlänge des Einzelblocks überschreitet
   bereits maximale Dateigröße oder Block
   konnte nicht gespeichert werden
*/

template <class T>
void DLSSaverT<T>::_save_block()
{
    COMIndexRecord index_record;
    stringstream pre, post, err;
    COMTime start_time, end_time;

    // Wenn keine Daten im Puffer sind, beenden.
    if (_block_buf_index == 0) return;

    if (!_compression)
        throw EDLSSaver("FATAL: No compression object!");

#ifdef DEBUG
    msg() << "Compressing data for channel "
          << _parent_logger->channel_preset()->name;
    log(DLSDebug);
#endif

    // Bei Bedarf neue Dateien beginnen
    if (!_data_file.open() || _data_file.calc_size() >= SAVER_MAX_FILE_SIZE)
    {
        _begin_files(_block_time);
    }

    // Daten für neuen Indexeintrag erfassen
    index_record.start_time = _block_time.to_ll();
    index_record.end_time = _time_of_last.to_ll();
    index_record.position = _data_file.calc_size();

    try
    {
        // Daten komprimieren
        _compression->compress(_block_buf, _block_buf_index);
    }
    catch (ECOMCompression &e)
    {
        err << "Block compression: " << e.msg;
        throw EDLSSaver(err.str());
    }

    start_time.set_now(); // Zeiterfassung

    try
    {
        // Tag-Anfang in die Datei schreiben
        pre << "<d t=\"" << _block_time << "\"";
        pre << " s=\"" << _block_buf_index << "\"";
        pre << " d=\"";
        _data_file.append(pre.str().c_str(), pre.str().length());

        // Komprimierte Daten in die Datei schreiben
        _data_file.append(_compression->compression_output(),
                          _compression->compressed_size());

        // Tag-Ende in die Datei schreiben
        post << "\"/>" << endl;
        _data_file.append(post.str().c_str(), post.str().length());
    }
    catch (ECOMFile &e)
    {
        _compression->free();
        err << "Could not write to file! (disk full?): " << e.msg;
        throw EDLSSaver(err.str());
    }

    end_time.set_now(); // Zeiterfassung

    // Warnen, wenn Aufruf von write() sehr lange gebraucht hat
    if (end_time - start_time > (long long) (WRITE_TIME_WARNING * 1000000))
    {
        msg() << "Writing to disk took "
              << (end_time - start_time).to_dbl_time() << " seconds!";
        log(DLSWarning);
    }

    _compression->free();

    // Dem Logger mitteilen, dass Daten gespeichert wurden
    _parent_logger->bytes_written(pre.str().length()
                                  + _compression->compressed_size()
                                  + post.str().length());

    try
    {
        // Index aktualisieren
        _index_file.append((char *) &index_record, sizeof(COMIndexRecord));
    }
    catch (ECOMFile &e)
    {
        err << "Could not add index record! (disk full?): " << e.msg;
        throw EDLSSaver(err.str());
    }

    // Dem Logger mitteilen, dass Daten gespeichert wurden
    _parent_logger->bytes_written(sizeof(COMIndexRecord));

    _block_buf_index = 0;
}

/*****************************************************************************/

/**
   Speichert einen Überhangblock als XML-Tag in die Ausgabedatei

   Ähnlich wie _save_block(), schreibt aber _immer_ in die
   noch offene Datei.

   \throw EDLSSaver Block konnte nicht gespeichert werden
*/

template <class T>
void DLSSaverT<T>::_save_rest()
{
    stringstream pre, post, err;
    COMTime start_time, end_time;

#ifdef DEBUG
    msg() << "Saving rest";
    log(DLSDebug);
#endif

    if (!_compression)
        throw EDLSSaver("FATAL: No compression object!");

    try
    {
        _compression->flush_compress();
    }
    catch (ECOMCompression &e)
    {
        err << "Block flush compression: " << e.msg;
        throw EDLSSaver(err.str());
    }

#ifdef DEBUG
    msg() << "Saving " << _compression->compressed_size() << " bytes of rest";
    log(DLSDebug);
#endif

    if (_compression->compressed_size())
    {
        try
        {
            // Tag-Anfang in Datei schreiben
            pre << "<d t=\"0\" s=\"0\" d=\"";
            _data_file.append(pre.str().c_str(), pre.str().length());

            // Komprimierte Daten in die Datei schreiben
            _data_file.append(_compression->compression_output(),
                              _compression->compressed_size());

            // Tag-Ende anhängen
            post << "\"/>" << endl;
            _data_file.append(post.str().c_str(), post.str().length());
        }
        catch (ECOMFile &e)
        {
            _compression->free();
            err << "Could not write to file! (Disk full?): " << e.msg;
            throw EDLSSaver(err.str());
        }

        _compression->free();

        // Dem Logger mitteilen, dass Daten gespeichert wurden
        _parent_logger->bytes_written(pre.str().length()
                                      + _compression->compressed_size()
                                      + post.str().length());
    }

#ifdef DEBUG
    msg() << "Saving rest finished";
    log(DLSDebug);
#endif
}

/*****************************************************************************/

/**
   Öffnet neue Daten- und Indexdateien

   Prüft, ob der besitzende Logger bereits das Chunk-Verzeichnis
   erstellt hat und weist diesen bei Bedarf an, dies zu tun.
   Erstellt dann das benötigte Ebenen-Verzeichnis, falls es noch
   nicht existiert.
   Erstellt dann eine neue Daten- und eine neue Indexdatei
   und öffnet diese.

   \throw EDLSSaver Chunk-Verzeichnis, Ebenen-Verzeichnis
   oder die Dateien konnten nicht
   erstellt werden.
*/

template <class T>
void DLSSaverT<T>::_begin_files(COMTime time_of_first)
{
    stringstream dir_name, file_name, err;
    COMIndexT<COMGlobalIndexRecord> global_index;
    COMGlobalIndexRecord global_index_record;

    if (!_parent_logger->chunk_created())
    {
        try
        {
            _parent_logger->create_chunk(time_of_first);
        }
        catch (EDLSLogger &e)
        {
            throw EDLSSaver(e.msg);
        }
    }

    // Pfad des Ebenenverzeichnisses konstruieren
    dir_name << _parent_logger->chunk_dir_name() << "/level" << _meta_level();

    if (mkdir(dir_name.str().c_str(), 0755)) {
        if (errno != EEXIST) {
            err << "Could not create \"" << dir_name.str()
                << "\" (errno " << errno << ")!";
            throw EDLSSaver(err.str());
        }
    }

    // Alte Dateien Schließen
    _finish_files();

    file_name << dir_name.str();
    file_name << "/data" << time_of_first;
    file_name << "_" << _meta_type();

    try {
        _data_file.open_read_append(file_name.str().c_str());
    }
    catch (ECOMFile &e) {
        err << "Failed to open file \"" << file_name.str();
        err << "\": " << e.msg;
        throw EDLSSaver(err.str());
    }

    file_name << ".idx";

    try {
        _index_file.open_read_append(file_name.str().c_str());
    }
    catch (ECOMFile &e) {
        err << "Failed to open index file \"" << file_name.str();
        err << "\": " << e.msg;
        throw EDLSSaver(err.str());
    }

    // Globalen Index updaten
    file_name.str("");
    file_name.clear();
    file_name << _parent_logger->chunk_dir_name();
    file_name << "/level" << _meta_level();
    file_name << "/data_" << _meta_type() << ".idx";

    global_index_record.start_time = time_of_first.to_ll();
    global_index_record.end_time = 0; // Datei noch nicht beendet

    try
    {
        global_index.open_read_append(file_name.str());
        global_index.append_record(&global_index_record);
        global_index.close();
    }
    catch (ECOMIndexT &e)
    {
        err << "Could not write to global index file \""
            << file_name.str() << "\": " << e.msg;
        throw EDLSSaver(err.str());
    }

    // Dem Logger mitteilen, dass Daten gespeichert wurden
    _parent_logger->bytes_written(sizeof(COMGlobalIndexRecord));
}

/*****************************************************************************/

/**
   Schließt Daten- und Index-Dateien

   \throw EDLSSaver Ebenen-Verzeichnis oder Datendatei konnte
   nicht erstellt werden
*/

template <class T>
void DLSSaverT<T>::_finish_files()
{
    COMIndexT<COMGlobalIndexRecord> global_index;
    COMGlobalIndexRecord global_index_record;
    stringstream file_name, err;
    unsigned int index_of_last;
    bool was_open = _data_file.open();

    try
    {
        _data_file.close();
    }
    catch (ECOMFile &e)
    {
        msg() << "Could not close data file: " << e.msg;
        log(DLSWarning);
    }

    try
    {
        _index_file.close();
    }
    catch (ECOMFile &e)
    {
        msg() << "Could not close index file: " << e.msg;
        log(DLSWarning);
    }

    // Wenn Dateien geöffnet waren und Daten hineingeschrieben wurden
    if (was_open && _time_of_last.to_ll() != 0)
    {
        // Dateinamen des globalen Index` bestimmen
        file_name << _parent_logger->chunk_dir_name();
        file_name << "/level" << _meta_level();
        file_name << "/data_" << _meta_type() << ".idx";

        try
        {
            // Globalen Index öffnen
            global_index.open_read_write(file_name.str());

            if (global_index.record_count() == 0) // Keine Records im Index?
            {
                // Das darf nicht passieren, da doch
                // beim Anlagen der Datendateien
                // ein globaler Index-Record angelegt wurde...
                err << "Global index file has no entries!";
                throw EDLSSaver(err.str());
            }

            // Record auslesen
            index_of_last = global_index.record_count() - 1;
            global_index_record = global_index[index_of_last];

            if (global_index_record.end_time != 0)
            {
                err << "End time of last record in global index is not 0!";
                throw EDLSSaver(err.str());
            }

            // Letzten Record im Index updaten
            global_index_record.end_time = _time_of_last.to_ll();
            global_index.change_record(index_of_last, &global_index_record);
            global_index.close();
        }
        catch (ECOMIndexT &e)
        {
            err << "Updating global index: " << e.msg;
            throw EDLSSaver(err.str());
        }
    }
}

/*****************************************************************************/

#ifdef DEBUG
#undef DEBUG
#endif

#endif
