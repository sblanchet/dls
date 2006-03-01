//---------------------------------------------------------------
//
//  C O M _ C H A N N E L _ P R E S E T . C P P
//
//---------------------------------------------------------------

#include <sstream>
using namespace std;

#include "com_globals.hpp"
#include "com_channel_preset.hpp"
#include "com_xml_tag.hpp"

//---------------------------------------------------------------

RCS_ID("$Header: /home/fp/dls/src/RCS/com_channel_preset.cpp,v 1.8 2005/03/11 10:43:00 fp Exp $");

//---------------------------------------------------------------

/**
   Konstruktor
*/

COMChannelPreset::COMChannelPreset()
{
  clear();
}

//---------------------------------------------------------------

/**
   Destruktor
*/

COMChannelPreset::~COMChannelPreset()
{
} 

//---------------------------------------------------------------

/**
   Pr�ft zwei Kanalvorgaben auf Ungleichheit

   Wird ben�tigt, um bei �nderung der Auftragsvorgaben zu
   bestimmen, ob sich eine Kanalvorgabe ge�ndert hat.

   \param other The "other" channel preset to compare with.
   \return true, if something has changed
   \see DLSJob::sync_loggers()
*/

bool COMChannelPreset::operator!=(const COMChannelPreset &other) const
{
  return name != other.name ||
    sample_frequency != other.sample_frequency ||
    block_size != other.block_size ||
    meta_mask != other.meta_mask ||
    meta_reduction != other.meta_reduction ||
    format_index != other.format_index ||
    mdct_block_size != other.mdct_block_size ||
    mdct_accuracy != other.mdct_accuracy;
}

//---------------------------------------------------------------

/**
   Importiert eine Kanalvorgabe aus einem XML-Tag

   Importiert aus einem <channel>-Tag aus der
   Auftragsvorgabendatei.

   \param tag Konstanter Zeiger auf ein XML-Tag,
   aus dem gelesen werden soll
   \throw ECOMChannelPreset Fehler w�hrend des Importierens
*/

void COMChannelPreset::read_from_tag(const COMXMLTag *tag)
{
  string format_string;
  stringstream err;

  clear();

  try
  {
    name = tag->att("name")->to_str();
    sample_frequency = tag->att("frequency")->to_int();
    block_size = tag->att("block_size")->to_int();
    meta_mask = tag->att("meta_mask")->to_int();
    meta_reduction = tag->att("meta_reduction")->to_int();
    format_string = tag->att("format")->to_str();

    for (int i = 0; i < DLS_FORMAT_COUNT; i++)
    {
      if (format_string == dls_format_strings[i])
      {
        format_index = i;
        break;
      }
    }

    if (format_index == DLS_FORMAT_INVALID)
    {
      clear();
      err << "Unknown channel format \"" << format_string << "\"!";
      throw ECOMChannelPreset(err.str());
    }

    if (format_index == DLS_FORMAT_MDCT)
    {
      mdct_block_size = tag->att("mdct_block_size")->to_int();
      mdct_accuracy = tag->att("mdct_accuracy")->to_dbl();
    }

    if (tag->has_att("type"))
    {
      type = dls_str_to_channel_type(tag->att("type")->to_str());
    }
    else
    {
      type = TUNKNOWN;
    }
  }
  catch (ECOMXMLTag &e)
  {
    clear();
    throw ECOMChannelPreset(e.msg);
  }
}
 
//---------------------------------------------------------------

/**
   Exportiert eine Kanalvorgabe in ein XML-Tag

   Erstellt ein von read_from_tag() lesbares XML-Tag
   f�r die Auftragsvorgabendatei.

   \param tag Zeiger auf ein XML-Tag, in das
   geschrieben werden soll
   \throw ECOMChannelPreset Ung�ltiges Format
*/

void COMChannelPreset::write_to_tag(COMXMLTag *tag) const
{
  if (format_index < 0 || format_index >= DLS_FORMAT_COUNT)
  {
    throw ECOMChannelPreset("Invalid channel format!");
  }

  tag->clear();
  tag->title("channel");
  tag->push_att("name", name);
  tag->push_att("frequency", sample_frequency);
  tag->push_att("block_size", block_size);
  tag->push_att("meta_mask", meta_mask);
  tag->push_att("meta_reduction", meta_reduction);
  tag->push_att("format", dls_format_strings[format_index]);

  if (format_index == DLS_FORMAT_MDCT)
  {
    tag->push_att("mdct_block_size", mdct_block_size);
    tag->push_att("mdct_accuracy", mdct_accuracy);
  }

  if (type != TUNKNOWN)
  {
    tag->push_att("type", dls_channel_type_to_str(type));
  }
}

//---------------------------------------------------------------

/**
   L�scht die aktuellen Kanalvorgaben
*/

void COMChannelPreset::clear()
{
  name = "";
  sample_frequency = 0;
  block_size = 0;
  meta_mask = 0;
  meta_reduction = 0;
  format_index = DLS_FORMAT_INVALID;
  mdct_block_size = 0;
  mdct_accuracy = 0;
  type = TUNKNOWN;
}

//---------------------------------------------------------------
