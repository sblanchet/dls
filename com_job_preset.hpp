//---------------------------------------------------------------
//
//  C O M _ J O B _ P R E S E T . H P P
//
//---------------------------------------------------------------

#ifndef COMJobPresetHpp
#define COMJobPresetHpp

//---------------------------------------------------------------

#include <string>
#include <vector>
using namespace std;

//---------------------------------------------------------------

#include "com_exception.hpp"
#include "com_time.hpp"
#include "com_channel_preset.hpp"

//---------------------------------------------------------------

/**
   Exception eines COMJobPreset-Objektes
*/

class ECOMJobPreset : public COMException
{
public:
  ECOMJobPreset(const string &pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

/**
   Auftragsvorgaben mit Liste der Kanalvorgaben

   Enthält Beschreibung, Zustand, Datenquelle, Trigger, usw.
*/

class COMJobPreset
{
public:
  COMJobPreset();
  ~COMJobPreset();

  void import(const string &, int);
  void write(const string &);
  void notify_new(const string &);
  void notify_changed(const string &);
  void notify_deleted(const string &);

  void id(int);
  void description(const string &);
  void running(bool);
  void source(const string &);
  void trigger(const string &);

  void toggle_running();
  void add_channel(const COMChannelPreset *);
  void change_channel(const COMChannelPreset *);
  void remove_channel(const string &);

  int id() const;
  const string &description() const;
  string id_desc() const;
  const string &owner() const;
  bool running() const;
  unsigned int quota() const;
  const string &source() const;
  const string &trigger() const;
  const vector<COMChannelPreset> *channels() const;
  bool channel_exists(const string &) const;

  //--- Nur für User-Bereich
  //@{
  time_t process_watchdog;        /**< Zeitstempel der Watchdog-Datei (NUR USER) */
  unsigned int process_bad_count; /**< Anzahl letzter Watchdog-Prüfungen,
                                       ohne dass sich etwas geändert hat (NUR USER) */
  bool process_watch_determined;  /**< Watchdog-Information steht fest (NUR USER) */
  time_t logging_watchdog;        /**< Zeitstempel der Logging-Watchdog-Datei (NUR USER) */
  unsigned int logging_bad_count; /**< Anzahl letzter Logging-Watchdog-Prüfungen,
                                       ohne dass sich etwas geändert hat (NUR USER) */
  bool logging_watch_determined;  /**< Logging-Watchdog-Information steht fest (NUR USER) */
  //@}
  //---

  //--- Nur für Mother
  //@{
  int pid;            /**< PID des ge'fork'ten Kindprozesses (NUR MOTHER) */
  int last_exit_code; /**< Exitcode des letzten Prozesses (NUR MOTHER) */
  COMTime exit_time;  /**< Beendigungszeit des letzten Prozesses */
  //@}
  //---

private:
  int _id;                         /**< Auftrags-ID */
  string _description;             /**< Beschreibender Name des Auftrages */
  string _owner;                   /**< Besitzer des Auftrages \todo Nicht genutzt */
  bool _running;                   /**< Soll erfasst werden? */
  unsigned int _quota;             /**< Auftrags-Quota \todo Nicht genutzt */
  string _source;                  /**< IP-Adresse oder Hostname der Datenquelle */
  string _trigger;                 /**< Name des Trigger-Parameters, andernfalls leer */  
  vector<COMChannelPreset> _channels; /**< Liste der Kanalvorgaben */

  void _write_spooling_file(const string &, const string &);
};

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf die ID

   \returns Auftrags-ID
   \see _id
*/

inline int COMJobPreset::id() const
{
  return _id;
}

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf die Beschreibung

   \returns Beschreibung
   \see _description
*/

inline const string &COMJobPreset::description() const
{
  return _description;
}

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf das Besitzer-Attribut

   \returns Besitzername
   \see _owner
*/

inline const string &COMJobPreset::owner() const
{
  return _owner;
}

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf den Sollzustand

   \returns Sollzustand
   \see _running
*/

inline bool COMJobPreset::running() const
{
  return _running;
}

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf die Adresse der Datenquelle

   \returns IP-Adresse oder Hostname der Datenquelle
   \see _source
*/

inline const string &COMJobPreset::source() const
{
  return _source;
}

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf den Trigger-Attribut

   \returns Name des triggerparameters, oder "", wenn kein Trigger
   \see _trigger
*/

inline const string &COMJobPreset::trigger() const
{
  return _trigger;
}

//---------------------------------------------------------------

/**
   Ermöglicht Lesezugriff auf den Vektor der Kanalvorgaben

   \returns Konstanter Zeiger auf den Vektor der Kanalvorgaben
   \see _channels
*/

inline const vector<COMChannelPreset> *COMJobPreset::channels() const
{
  return &_channels;
}

//---------------------------------------------------------------

#endif
