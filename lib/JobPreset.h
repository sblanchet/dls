/******************************************************************************
 *
 *  $Id$
 *
 *  This file is part of the Data Logging Service (DLS).
 *
 *  DLS is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  DLS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with DLS. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef LibDLSJobPresetH
#define LibDLSJobPresetH

/*****************************************************************************/

#include <stdint.h>

#include <string>
#include <vector>
using namespace std;

/*****************************************************************************/

#include "Exception.h"
#include "Time.h"
#include "ChannelPreset.h"

/*****************************************************************************/

namespace LibDLS {

/*****************************************************************************/

/**
   Exception eines JobPreset-Objektes
*/

class EJobPreset : public Exception
{
public:
    EJobPreset(const string &pmsg) : Exception(pmsg) {};
};

/*****************************************************************************/

/**
   Auftragsvorgaben mit Liste der Kanalvorgaben

   Enthält Beschreibung, Zustand, Datenquelle, Trigger, usw.
*/

class JobPreset
{
public:
    JobPreset();
    ~JobPreset();

    void import(const string &, unsigned int);

    unsigned int id() const;
    const string &description() const;
    string id_desc() const;
    const string &owner() const;
    bool running() const;
    uint64_t quota_time() const;
    uint64_t quota_size() const;
    const string &source() const;
    uint16_t port() const;
    const string &trigger() const;
    const vector<ChannelPreset> *channels() const;
    bool channel_exists(const string &) const;

protected:
    unsigned int _id; /**< Auftrags-ID */
    string _description; /**< Beschreibender Name des Auftrages */
    string _owner; /**< Besitzer des Auftrages \todo Nicht genutzt */
    bool _running; /**< Soll erfasst werden? */
    uint64_t _quota_time; /**< Auftrags-Quota nach Zeit */
    uint64_t _quota_size; /**< Auftrags-Quota nach Datengröße */
    string _source; /**< IP-Adresse oder Hostname der Datenquelle */
    uint16_t _port; /**< Port der Datenquelle. */
    string _trigger; /**< Name des Trigger-Parameters, andernfalls leer */
    vector<ChannelPreset> _channels; /**< Liste der Kanalvorgaben */
};

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf die ID

   \returns Auftrags-ID
   \see _id
*/

inline unsigned int JobPreset::id() const
{
    return _id;
}

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf die Beschreibung

   \returns Beschreibung
   \see _description
*/

inline const string &JobPreset::description() const
{
    return _description;
}

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf das Besitzer-Attribut

   \returns Besitzername
   \see _owner
*/

inline const string &JobPreset::owner() const
{
    return _owner;
}

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf den Sollzustand

   \returns Sollzustand
   \see _running
*/

inline bool JobPreset::running() const
{
    return _running;
}

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf die Adresse der Datenquelle

   \returns IP-Adresse oder Hostname der Datenquelle
   \see _source
*/

inline const string &JobPreset::source() const
{
    return _source;
}

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf den Port der Datenquelle

   \returns Port der Datenquelle
   \see _port
*/

inline uint16_t JobPreset::port() const
{
    return _port;
}

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf den Trigger-Attribut

   \returns Name des triggerparameters, oder "", wenn kein Trigger
   \see _trigger
*/

inline const string &JobPreset::trigger() const
{
    return _trigger;
}

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf das Zeit-Quota-Attribut

   \returns Quota-Größe
   \see _quota_time
*/

inline uint64_t JobPreset::quota_time() const
{
    return _quota_time;
}

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf das Daten-Quota-Attribut

   \returns Quota-Größe
   \see _quota_size
*/

inline uint64_t JobPreset::quota_size() const
{
    return _quota_size;
}

/*****************************************************************************/

/**
   Ermöglicht Lesezugriff auf den Vektor der Kanalvorgaben

   \returns Konstanter Zeiger auf den Vektor der Kanalvorgaben
   \see _channels
*/

inline const vector<ChannelPreset> *JobPreset::channels() const
{
    return &_channels;
}

/*****************************************************************************/

} // namespace

/*****************************************************************************/

#endif
