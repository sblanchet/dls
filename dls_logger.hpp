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
#include "com_real_channel.hpp"
#include "com_channel_preset.hpp"
#include "dls_saver_gen_t.hpp"

//---------------------------------------------------------------

class DLSJob; // N�tig, da gegenseitige Referenzierung

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
   Speichert Daten f�r einen Kanal entsprechend einer Vorgabe.

   Verwaltet selbst�ndig Chunk-Verzeichnisse und kann Online-
   �nderungen in den Kanalvorgaben verarbeiten. F�r das
   eigentliche Speichern der Daten wird ein DLSSaverGen - Objekt
   vorgehalten.
*/

class DLSLogger
{
public:

  DLSLogger(const DLSJob *, const COMChannelPreset *, const string &);
  ~DLSLogger();

  //@{
  void get_real_channel(const list<COMRealChannel> *);
  bool presettings_valid(const COMChannelPreset * = 0) const;
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

  void process_data(const string &, COMTime);
  void finish();

  //@{
  bool chunk_created() const;
  void create_chunk(COMTime);
  const string &chunk_dir() const;
  //@}

  //@{
  stringstream &msg() const;
  void log(DLSLogType) const;
  //@}

private:
  const DLSJob *_parent_job; /**< Zeiger auf das besitzende Auftragsobjekt */
  string _dls_dir;           /**< DLS-Datenverzeichnis */

  //@{
  COMChannelPreset _channel_preset; /**< Aktuelle Kanalvorgaben */
  COMRealChannel _real_channel;     /**< Informationen �ber den msrd-Kanal */
  //@}

  DLSSaverGen *_saver;          /**< Zeiger auf das Objekt zur Speicherung
                                     der generischen Daten */

  //@{
  bool _chunk_created; /**< Wurde das aktuelle Chunk-Verzeichnis
                            bereits erstellt? */
  string _chunk_dir;   /**< Pfad des Chunk-Verzeichnisses */
  //@}

  //@{
  bool _change_in_progress;         /**< Wartet eine Vorgaben�nderung auf Best�tigung? */
  string _change_id;                /**< ID des �nderungsbefehls, auf dessen
                                         Best�tigung gewartet wird */
  COMChannelPreset _change_channel; /**< Neue Kanalvorgaben, die nach der
                                         Best�tigung aktiv werden */
  //@}

  bool _finished; /**< Keine Daten mehr im Speicher - kein Datenverlust bei "delete"  */
};

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf die aktuellen Kanalvorgaben

   \return Konstanter Zeiger auf die Kanalvorgaben
*/

inline const COMChannelPreset *DLSLogger::channel_preset() const
{
  return &_channel_preset;
}

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf die Eigenschaften des
   zu Grunde liegenden MSR-Kanals

   \return Konstanter Zeiger auf die Kanalvorgaben
*/

inline const COMRealChannel *DLSLogger::real_channel() const
{
  return &_real_channel;
}

//---------------------------------------------------------------

/**
   Pr�ft, ob ein aktuelles Chunk-Verzeichnis erstellt wurde

   Wenn ja, gibt chunk_dir() den Pfad zur�ck.

   \return true, wenn es ein aktuelles Chunk-Verzeichnis gibt
*/

inline bool DLSLogger::chunk_created() const
{
  return _chunk_created;
}

//---------------------------------------------------------------

/**
   Erm�glicht Auslesen des aktuellen Chunk-Verzeichnisses

   \return Pfad des Chunk-Verzeichnisses
*/

inline const string &DLSLogger::chunk_dir() const
{
  return _chunk_dir;
}

//---------------------------------------------------------------

#endif
