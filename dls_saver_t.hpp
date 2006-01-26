//---------------------------------------------------------------
//
//  D L S _ S A V E R _ T . H P P
//
//---------------------------------------------------------------

#ifndef DLSSaverTHpp
#define DLSSaverTHpp

//---------------------------------------------------------------

#include "string.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>
#include <string>
using namespace std;

//---------------------------------------------------------------

#include "com_exception.hpp"
#include "com_zlib.hpp"
#include "com_base64.hpp"
#include "com_time.hpp"
#include "com_file.hpp"
#include "com_index_t.hpp"
#include "dls_compression_t.hpp"

//---------------------------------------------------------------

class DLSLogger; // Nötig, da gegenseitige Referenzierung
class EDLSLogger;

//---------------------------------------------------------------

/**
   Allgemeine Exception eines Saver-Objekts
*/

class EDLSSaver : public COMException
{
public:
  EDLSSaver(const string &pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

/**
   Exception eines Saver-Objekts: Zeittoleranzfehler!
*/

class EDLSTimeTolerance : public COMException
{
public:
  EDLSTimeTolerance(const string &pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

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
  DLSLogger *_parent_logger;     /**< Zeiger auf das besitzende Logger-Objekt */
  T *_block_buf;                 /**< Array von Datenwerten, die als Block in die
                                      entsprechende Datei gespeichert werden sollen */
  T *_meta_buf;                  /**< Array von Datenwerten, über die ein Meta-Wert
                                      erzeugt werden soll */
  unsigned int _block_buf_index; /**< Index des ersten, freien Elementes im Block-Puffer */
  unsigned int _block_buf_size;  /**< Größe des Block-Puffers */
  unsigned int _meta_buf_index;  /**< Index des ersten, freien Elementes im Meta-Puffer */
  unsigned int _meta_buf_size;   /**< Größe des Meta-Puffers */
  COMTime _block_time;           /**< Zeit des ersten Datenwertes im Block-Puffer */
  COMTime _meta_time;            /**< Zeit des ersten Datenwertes im Meta-Puffer */
  COMTime _time_of_last;         /**< Zeit des letzten Datenwertes beider Puffer */

  void _save_block();
  void _save_carry(COMTime, COMTime, unsigned int, T);
  void _finish_files();
  stringstream &msg();
  void log(DLSLogType);

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
  DLSCompressionT<T> *_compression; /**< Zeiger auf ein beliebiges Komprimierungs-Objekt */
  COMFile _data_file;               /**< Datei-Objekt zum Speichern der Blöcke */
  COMFile _index_file;              /**< Datei-Objekt zum Speichern der Block-Indizes */
  char *_write_buf;                 /**< Schreibpuffer zum Zwischenspeichern vor der Ausgabe */

  void _begin_files(COMTime);
};

//---------------------------------------------------------------

/**
   Konstruktor

   \param parent_logger Zeiger auf das besitzende Logger-Objekt
   \throw EDLSSaver Es konnte nicht genug Speicher allokiert werden
*/

template <class T>
DLSSaverT<T>::DLSSaverT(DLSLogger *parent_logger)
{
  stringstream err;

  _parent_logger = parent_logger;

  _block_buf_size = _parent_logger->channel_preset()->block_size;
  _block_buf_index = 0;
  _meta_buf_size = _parent_logger->channel_preset()->meta_reduction;
  _meta_buf_index = 0;

  _block_buf = 0;
  _meta_buf = 0;
  _write_buf = 0;
  _compression = 0;

  try
  {
    _block_buf = new T[_block_buf_size];
    _meta_buf = new T[_meta_buf_size];
    _write_buf = new char[SAVER_BUF_SIZE];
  }
  catch (...)
  {
    throw EDLSSaver("could not allocate memory for buffers!");
  }

  try
  {
    if (_parent_logger->channel_preset()->format_index == DLS_FORMAT_ZLIB)
    {
      _compression = new DLSCompressionT_ZLib<T>();
    }
    else if (_parent_logger->channel_preset()->format_index == DLS_FORMAT_MDCT)
    {
      if (typeid(T) == typeid(float))
      {
        _compression = (DLSCompressionT<T> *) new DLSCompressionT_MDCT<float>(_parent_logger->channel_preset()->mdct_block_size);
      }
      else if (typeid(T) == typeid(double))
      {
        _compression = (DLSCompressionT<T> *) new DLSCompressionT_MDCT<double>(_parent_logger->channel_preset()->mdct_block_size);
      }
      else 
      {
        err << "MDCT only suitable for floating point types, not for " << typeid(T).name() << "!";
      }
    }
    else
    {
      err << "unknown channel format index " << _parent_logger->channel_preset()->format_index;
    }
  }
  catch (EDLSCompression &e)
  {
    throw EDLSSaver(e.msg);
  }
  catch (...)
  {
    throw EDLSSaver("could not allocate memory for compression object!");
  }

  if (err.str() != "") throw EDLSSaver(err.str());
}

//---------------------------------------------------------------

/**
   Destruktor

   Schließt die Daten- und Indexdateien, allerdings ohne
   Fehlerverarbeitung! Gibt die Puffer frei.
*/

template <class T>
DLSSaverT<T>::~DLSSaverT()
{
  // Dateien schliessen
  _data_file.close();
  _index_file.close();

  // ggfs. Kompressionsobjekt freigeben
  if (_compression) delete _compression;

  // Puffer freigeben
  if (_write_buf) delete [] _write_buf;
  if (_block_buf) delete [] _block_buf;
  if (_meta_buf) delete [] _meta_buf;
}

//---------------------------------------------------------------

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
  unsigned int data_length;
  COMIndexRecord index_record;
  stringstream str, err;
  COMTime start_time, end_time;

  // Wenn keine Daten im Puffer sind, beenden.
  if (_block_buf_index == 0) return;

  // Tag-Anfang in einen Stream schreiben
  str << "<d t=\"" << _block_time << "\"";
  str << " s=\"" << _block_buf_index << "\"";
  str << " d=\"";
  
  // Stream in Schreibpuffer schieben
  strcpy(_write_buf, str.str().c_str());

  // Länge des bisherigen Tags ermitteln
  data_length = strlen(_write_buf);

  if (!_compression) throw EDLSSaver("no compression object!"); // Sollte nie passieren...

  //cout << "BLOCK" << endl;

  try
  {
    data_length += _compression->compress(_block_buf,                      // Adresse des Input-Puffers
                                          _block_buf_index,                // Größe des Input-Puffers
                                          _write_buf + data_length,        // Adresse des Ausgabepuffers
                                          SAVER_BUF_SIZE - data_length - 1 // Größe des Ausgabepuffers
                                          );
  }
  catch (EDLSCompression &e)
  {
    err << "block compression: " << e.msg;
    throw EDLSSaver(err.str());
  }

  //cout << "BLOCK END" << endl;

  // Tag-Ende anhängen
  strcpy(_write_buf + data_length, "\"/>\n");
  data_length += 4;

  // Prüfen ob die reine Datenlänge bereits größer ist, als erlaubt
  if (data_length >= SAVER_MAX_FILE_SIZE - 1)
  {
    throw EDLSSaver("data length exceeds maximum file size!");
  }

  if (_data_file.open()) // Wenn die Datendatei bereits geöffnet ist
  {
    // Würde die aktuelle Dateigröße plus den hinzukommenden
    // Daten die Maximalgröße überschreiten?
    if (_data_file.size() + data_length >= SAVER_MAX_FILE_SIZE - 1)
    {
      // Dann neue Dateien beginnen
      _begin_files(_block_time);
    }
  }
  else // Dateien sind noch nicht offen
  {
    // Neue Dateien beginnen
    _begin_files(_block_time);
  }

  // Daten für neuen Indexeintrag erfassen
  index_record.start_time = _block_time.to_ll();
  index_record.end_time = _time_of_last.to_ll();
  index_record.position = _data_file.size();

  start_time.set_now();

  try
  {
    // Buffer in die Datei schreiben
    _data_file.write(_write_buf, data_length);
  }
  catch (ECOMFile &e)
  {
    err << "could not write to file! (disk full?): " << e.msg;
    throw EDLSSaver(err.str());
  }

  end_time.set_now();

  if (end_time - start_time > (long long) 1000000)
  {
    msg() << "write() took " << (end_time - start_time) << "!";
    log(DLSWarning);
  }

  try
  {
    // Index aktualisieren
    _index_file.write((char *) &index_record, sizeof(COMIndexRecord));
  }
  catch (ECOMFile &e)
  {
    err << "could not add index record! (disk full?): " << e.msg;
    throw EDLSSaver(err.str());
  }
  
  _block_buf_index = 0;
}

//---------------------------------------------------------------

/**
   Speichert einen Carryblock als XML-Tag in die Ausgabedatei

   Konstruiert zuerst das komplette XML-Tag und prüft dann, ob
   zusammen mit dem bisherigen Dateiinhalt die maximale Dateigröße
   überschritten werden würde. Bei Bedarf wird dann eine neue
   Datei geöffnet.
   Dann wird das XML-Tag in die aktuell offene Datei gespeichert
   und dessen Größe auf die bisherige Dateigröße addiert.

   \throw EDLSSaver Datenlänge des Einzelblocks überschreitet
                    bereits maximale Dateigröße oder Block
                    konnte nicht gespeichert werden
 */

template <class T>
void DLSSaverT<T>::_save_carry(COMTime start_time,
                               COMTime end_time,
                               unsigned int length,
                               T carry_value)
{
  unsigned int data_length;
  COMIndexRecord index_record;
  stringstream str, err;

  // Wenn keine Daten: Beenden!
  if (length == 0) return;

  // Tag-Anfang in einen Stream schreiben
  str.str("");
  str.clear();
  str << "<c t=\"" << start_time << "\" n=\"" << length << "\"";
  str << " d=\"";
  
  // Stream in Schreibpuffer schieben
  strcpy(_write_buf, str.str().c_str());

  // Länge des bisherigen Tags ermitteln
  data_length = strlen(_write_buf);

  if (!_compression) throw EDLSSaver("no compression object!"); // Sollte nie passieren...

  //cout << "CARRY" << endl;

  try
  {
    data_length += _compression->compress(&carry_value,                    // Adresse des Input-Puffers
                                          1,                               // Größe des Input-Puffers
                                          _write_buf + data_length,        // Adresse des Ausgabepuffers
                                          SAVER_BUF_SIZE - data_length - 1 // Größe des Ausgabepuffers
                                          );
  }
  catch (EDLSCompression &e)
  {
    err << "carry compression: " << e.msg;
    throw EDLSSaver(err.str());
  }

  //cout << "CARRY END" << endl;

  // Tag-Ende anhängen
  strcpy(_write_buf + data_length, "\"/>\n");
  data_length += 4;

  // Prüfen ob die reine Datenlänge bereits größer ist, als erlaubt
  if (data_length >= SAVER_MAX_FILE_SIZE - 1)
  {
    throw EDLSSaver("data length exceeds maximum file size!");
  }

  if (_data_file.open()) // Wenn die Dateien bereits geöffnet sind
  {
    // Würde die aktuelle Dateigröße plus den hinzukommenden
    // Daten die Maximalgröße überschreiten?
    if (_data_file.size() + data_length >= SAVER_MAX_FILE_SIZE - 1)
    {
      _begin_files(start_time); // Neue Dateien beginnen
    }
  }
  else // Dateien sind noch nicht offen
  {
    _begin_files(start_time); // Neue Dateien beginnen
  }

  // Daten für Index-Eintrag erfassen
  index_record.start_time = start_time.to_ll();
  index_record.end_time = end_time.to_ll();
  index_record.position = _data_file.size();

  try
  {
    // Puffer in die Datei schreiben
    _data_file.write(_write_buf, data_length);
  }
  catch (ECOMFile &e)
  {
    err << "could not write to file! (disk full?): " << e.msg;
    throw EDLSSaver(err.str());
  }

  try
  {
    // Index aktualisieren
    _index_file.write((char *) &index_record, sizeof(COMIndexRecord));
  }
  catch (ECOMFile &e)
  {
    err << "could not write to index file! (disk full?): " << e.msg;
    throw EDLSSaver(err.str());
  }
}

//---------------------------------------------------------------

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
  dir_name << _parent_logger->chunk_dir() << "/level" << _meta_level();

  if (mkdir(dir_name.str().c_str(), 0755) != 0)
  {
    if (errno != EEXIST)
    {
      err << "could not create \"" << dir_name.str() << "\" (errno " << errno << ")!";
      throw EDLSSaver(err.str());     
    }
  }

  // Alte Dateien Schließen
  _finish_files();

  file_name << dir_name.str();
  file_name << "/data" << time_of_first;
  file_name << "_" << _meta_type();

  try
  {
    _data_file.open_write(file_name.str().c_str());
  }
  catch (ECOMFile &e)
  {
    err << "could not open file \"" << file_name.str();
    err << "\": " << e.msg;
    throw EDLSSaver(err.str());
  }

  file_name << ".idx";

  try
  {
    _index_file.open_write(file_name.str().c_str());
  }
  catch (ECOMFile &e)
  {
    err << "could not open index file \"" << file_name.str();
    err << "\": " << e.msg;
    throw EDLSSaver(err.str());
  }

  // Globalen Index updaten
  file_name.str("");
  file_name.clear();
  file_name << _parent_logger->chunk_dir();
  file_name << "/level" << _meta_level();
  file_name << "/data_" << _meta_type() << ".idx";

  global_index_record.start_time = time_of_first.to_ll();
  global_index_record.end_time = 0; // Datei noch nicht beendet

  try
  {
    global_index.open_rw(file_name.str());
    global_index.append_record(&global_index_record);
    global_index.close();
  }
  catch (ECOMIndexT &e)
  {
    err << "could not write to global index file \"" << file_name.str() << "\": " << e.msg;
    throw EDLSSaver(err.str());
  }
}

//---------------------------------------------------------------

/**
   Schließt Daten- und Index-Dateien

   \todo Erste Bedingung korrekt? Daten?

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
    msg() << "could not close data file: " << e.msg;
    log(DLSWarning);
  }

  try
  {
    _index_file.close();
  }
  catch (ECOMFile &e)
  {
    msg() << "could not close index file: " << e.msg;
    log(DLSWarning);
  }

  // Wenn Dateien geöffnet waren und Daten hineingeschrieben wurden
  if (was_open && _time_of_last.to_ll() != 0)
  {
    // Dateinamen des globalen Index` bestimmen
    file_name << _parent_logger->chunk_dir();
    file_name << "/level" << _meta_level();
    file_name << "/data_" << _meta_type() << ".idx";

    try
    {
      // Globalen Index öffnen
      global_index.open_rw(file_name.str());

      if (global_index.record_count() == 0) // Keine Records im Index?
      {
        // Das darf nicht passieren, da doch beim Anlagen der Datendateien
        // ein globaler Index-Record angelegt wurde...
        err << "global index file has no entries!";
        throw EDLSSaver(err.str());
      }

      // Record auslesen
      index_of_last = global_index.record_count() - 1;
      global_index_record = global_index[index_of_last];

      if (global_index_record.end_time != 0)
      {
        err << "end time of last record in global index is not 0!";
        throw EDLSSaver(err.str());
      }

      // Letzten Record im Index updaten
      global_index_record.end_time = _time_of_last.to_ll();
      global_index.change_record(index_of_last, &global_index_record);
      global_index.close();
    }
    catch (ECOMIndexT &e)
    {
      err << "updating global index: " << e.msg;
      throw EDLSSaver(err.str());
    }
  }
}

//---------------------------------------------------------------

/**
   Speichert eine später zu loggende Nachricht

   \return Referenz auf den msg-Stream des Logging-Prozesses
*/

template <class T>
stringstream &DLSSaverT<T>::msg()
{
  return _parent_logger->msg();
}

//---------------------------------------------------------------

/**
   Loggt eine vorher zwischengespeicherte Nachricht.

   \param type Type der Nachricht
*/

template <class T>
void DLSSaverT<T>::log(DLSLogType type)
{
  _parent_logger->log(type);
}

//---------------------------------------------------------------

#endif


