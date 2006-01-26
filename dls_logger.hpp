//---------------------------------------------------------------
//
//  D L S _ L O G G E R . H P P
//
//---------------------------------------------------------------

#ifndef DLSLoggerHpp
#define DLSLoggerHpp

//---------------------------------------------------------------

#include <string>
using namespace std;

//---------------------------------------------------------------

#include "com_exception.hpp"
#include "com_channel_preset.hpp"
#include "dls_saver_gen_t.hpp"

//---------------------------------------------------------------

class DLSJob; // Nötig, da gegenseitige Referenzierung

//---------------------------------------------------------------

/**
   Allgemeine Exception eines Logger-Objektes
*/

class EDLSLogger : public COMException
{
public:
  EDLSLogger(string pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

/**
   Speichert Daten für einen Kanal entsprechend einer Vorgabe.

   Verwaltet selbständig Chunk-Verzeichnisse und kann Online-
   Änderungen in den Kanalvorgaben verarbeiten. Ein DLSLogger
   ist das prozessseitige Äquivalent zu einem Chunk.
   Die Größe der erzeugten Daten wird hier ebenfalls gespeichert.
   Für das eigentliche Speichern der Daten wird ein
   DLSSaverGen - Objekt vorgehalten. 
*/

class DLSLogger
{
public:

  DLSLogger(const DLSJob *, const COMChannelPreset *, const string &);
  ~DLSLogger();

  //@{
  void get_real_channel(const list<COMRealChannel> *);
  void check_presettings(const COMChannelPreset * = 0) const;
  void check_channel_info();
  void create_gen_saver();
  void process_data(const string &, COMTime);
  long long data_size() const;
  void finish();
  void discard_chunk();
  //@}

  //@{
  const COMChannelPreset *channel_preset() const;
  const COMRealChannel *real_channel() const;
  //@}

  //@{
  string start_tag(const COMChannelPreset *, const string & = "") const;
  string stop_tag() const;
  //@}

  //@{
  void set_change(const COMChannelPreset *, const string &);
  bool change_is(const string &) const;
  void do_change();
  //@}

  //@{
  bool chunk_created() const;
  void create_chunk(COMTime);
  const string &chunk_dir() const;
  //@}

  void bytes_written(unsigned int);

  //@{
  stringstream &msg() const;
  void log(DLSLogType) const;
  //@}

private:
  const DLSJob *_parent_job; /**< Zeiger auf das besitzende Auftragsobjekt */
  string _dls_dir;           /**< DLS-Datenverzeichnis */

  //@{
  COMChannelPreset _channel_preset; /**< Aktuelle Kanalvorgaben */
  COMRealChannel _real_channel;     /**< Informationen über den msrd-Kanal */
  //@}

  //@{
  DLSSaverGen *_gen_saver; /**< Zeiger auf das Objekt zur Speicherung
                                der generischen Daten */
  long long _data_size;    /**< Größe der bisher erzeugten Daten */
  //@}

  //@{
  bool _channel_dir_exists;  /**< Das Kanalverzeichnis existiert bereits */
  bool _channel_file_exists; /**< Die Kanal-Infodatei existiert bereits */
  bool _chunk_created;       /**< Das aktuelle Chunk-Verzeichnis wurde bereits erstellt */
  string _chunk_dir;         /**< Pfad des aktuellen Chunk-Verzeichnisses */
  //@}

  //@{
  bool _change_in_progress;         /**< Wartet eine Vorgabenänderung auf Bestätigung? */
  string _change_id;                /**< ID des Änderungsbefehls, auf dessen
                                         Bestätigung gewartet wird */
  COMChannelPreset _change_channel; /**< Neue Kanalvorgaben, die nach der
                                         Bestätigung aktiv werden */
  //@}

  bool _finished; /**< Keine Daten mehr im Speicher - kein Datenverlust bei "delete"  */
};

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf die aktuellen Kanalvorgaben

   \return Konstanter Zeiger auf die Kanalvorgaben
*/

inline const COMChannelPreset *DLSLogger::channel_preset() const
{
  return &_channel_preset;
}

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf die Eigenschaften des
   zu Grunde liegenden MSR-Kanals

   \return Konstanter Zeiger auf die Kanalvorgaben
*/

inline const COMRealChannel *DLSLogger::real_channel() const
{
  return &_real_channel;
}

//---------------------------------------------------------------

/**
   Prüft, ob ein aktuelles Chunk-Verzeichnis erstellt wurde

   Wenn ja, gibt chunk_dir() den Pfad zurück.

   \return true, wenn es ein aktuelles Chunk-Verzeichnis gibt
*/

inline bool DLSLogger::chunk_created() const
{
  return _chunk_created;
}

//---------------------------------------------------------------

/**
   Ermöglicht Auslesen des aktuellen Chunk-Verzeichnisses

   \return Pfad des Chunk-Verzeichnisses
*/

inline const string &DLSLogger::chunk_dir() const
{
  return _chunk_dir;
}

//---------------------------------------------------------------

/**
   Teilt dem Logger mit, dass Daten gespeichert wurden

   Dient dem Logger dazu, die Größe der bisher gespeicherten
   Daten mitzuführen und wird von den tieferliegenden
   DLSSaverT-Derivaten aufgerufen.
*/

inline void DLSLogger::bytes_written(unsigned int bytes)
{
  _data_size += bytes;
}

//---------------------------------------------------------------

/**
   Gibt die Größe des Chunks in Bytes zurück
*/

inline long long DLSLogger::data_size() const
{
  return _data_size;
}

//---------------------------------------------------------------

#endif
