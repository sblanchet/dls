//---------------------------------------------------------------
//
//  D L S _ J O B _ C H A N N E L . H P P
//
//---------------------------------------------------------------

#ifndef DLSJobChannelHpp
#define DLSJobChannelHpp

//---------------------------------------------------------------

#include <string>
using namespace std;

//---------------------------------------------------------------

#include "dls_globals.hpp"
#include "com_exception.hpp"
#include "com_xml_tag.hpp"

//---------------------------------------------------------------

/**
   Exception eines COMChannelPreset-Objektes
*/

class ECOMChannelPreset : public COMException
{
public:
  ECOMChannelPreset(const string &pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

/**
   Kanalvorgabe

   Enthält Kanalname, Abtastrate, Blockgröße, Meta-Vorgaben
   und das Format, in dem die Daten gespeichert werden sollen.
*/

class COMChannelPreset
{
public:
  COMChannelPreset();
  ~COMChannelPreset();

  bool operator!=(const COMChannelPreset &) const;

  void read_from_tag(const COMXMLTag *);
  void write_to_tag(COMXMLTag *) const;

  void clear();

  string name;                   /**< Kanalname */
  unsigned int sample_frequency; /**< Abtastrate, mit der aufgezeichnet werden soll */
  unsigned int block_size;       /**< Blockgröße, mit der aufgezeichnet werden soll */
  unsigned int meta_mask;        /**< Bitmaske mit den aufzuzeichnenden Meta-Typen */
  unsigned int meta_reduction;   /**< Meta-Untersetzung */
  int format_index;              /**< Index des Formates zum Speichern der Daten */
  unsigned int mdct_block_size;  /**< Blockgröße für MDCT */
  double accuracy;               /**< Genauigkeit von verlustbehafteten Kompressionen */

  COMChannelType type;          /**< Datentyp des Kanals (nur für MDCT-Prüfung) */
};

//---------------------------------------------------------------

#endif


