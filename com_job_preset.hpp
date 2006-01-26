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

   Enth�lt Beschreibung, Zustand, Datenquelle, Trigger, usw.
*/

class COMJobPreset
{
public:
  COMJobPreset();
  ~COMJobPreset();

  void import(const string &, int);

  int id() const;
  const string &description() const;
  string id_desc() const;
  const string &owner() const;
  bool running() const;
  long long quota_time() const;
  long long quota_size() const;
  const string &source() const;
  const string &trigger() const;
  const vector<COMChannelPreset> *channels() const;
  bool channel_exists(const string &) const;

  // prototypen
  void description(const string &);

protected:
  int _id;                            /**< Auftrags-ID */
  string _description;                /**< Beschreibender Name des Auftrages */
  string _owner;                      /**< Besitzer des Auftrages \todo Nicht genutzt */
  bool _running;                      /**< Soll erfasst werden? */
  long long _quota_time;              /**< Auftrags-Quota nach Zeit */
  long long _quota_size;              /**< Auftrags-Quota nach Datengr��e */
  string _source;                     /**< IP-Adresse oder Hostname der Datenquelle */
  string _trigger;                    /**< Name des Trigger-Parameters, andernfalls leer */  
  vector<COMChannelPreset> _channels; /**< Liste der Kanalvorgaben */
};

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf die ID

   \returns Auftrags-ID
   \see _id
*/

inline int COMJobPreset::id() const
{
  return _id;
}

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf die Beschreibung

   \returns Beschreibung
   \see _description
*/

inline const string &COMJobPreset::description() const
{
  return _description;
}

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf das Besitzer-Attribut

   \returns Besitzername
   \see _owner
*/

inline const string &COMJobPreset::owner() const
{
  return _owner;
}

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf den Sollzustand

   \returns Sollzustand
   \see _running
*/

inline bool COMJobPreset::running() const
{
  return _running;
}

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf die Adresse der Datenquelle

   \returns IP-Adresse oder Hostname der Datenquelle
   \see _source
*/

inline const string &COMJobPreset::source() const
{
  return _source;
}

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf den Trigger-Attribut

   \returns Name des triggerparameters, oder "", wenn kein Trigger
   \see _trigger
*/

inline const string &COMJobPreset::trigger() const
{
  return _trigger;
}

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf das Zeit-Quota-Attribut

   \returns Quota-Gr��e
   \see _quota_time
*/

inline long long COMJobPreset::quota_time() const
{
  return _quota_time;
}

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf das Daten-Quota-Attribut

   \returns Quota-Gr��e
   \see _quota_size
*/

inline long long COMJobPreset::quota_size() const
{
  return _quota_size;
}

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf den Vektor der Kanalvorgaben

   \returns Konstanter Zeiger auf den Vektor der Kanalvorgaben
   \see _channels
*/

inline const vector<COMChannelPreset> *COMJobPreset::channels() const
{
  return &_channels;
}

//---------------------------------------------------------------

#endif
