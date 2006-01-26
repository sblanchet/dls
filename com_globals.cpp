//---------------------------------------------------------------
//
//  C O M _ G L O B A L S . C P P
//
//---------------------------------------------------------------

#include "com_globals.hpp"
#include "com_exception.hpp"

//---------------------------------------------------------------

RCS_ID("$Header: /home/fp/dls/src/RCS/com_globals.cpp,v 1.6 2004/12/09 10:15:16 fp Exp $");

//---------------------------------------------------------------

const char *dls_format_strings[DLS_FORMAT_COUNT] =
{
    "ZLib/Base64"

#if DLS_FORMAT_COUNT > 1
    , "MDCT/ZLib/Base64"
#endif
};

//---------------------------------------------------------------

COMChannelType dls_str_to_channel_type(const string &str)
{
  if (str == "TCHAR")  return TCHAR;
  if (str == "TUCHAR") return TUCHAR;
  if (str == "TINT")   return TINT;
  if (str == "TUINT")  return TUINT;
  if (str == "TLINT")  return TLINT;
  if (str == "TULINT") return TULINT;
  if (str == "TFLT")   return TFLT;
  if (str == "TDBL")   return TDBL;

  throw COMException("unknown channel type \"" + str + "\"!");
}

//---------------------------------------------------------------

char *dls_channel_type_to_str(COMChannelType type)
{
  switch (type)
  {
    case TCHAR:  return "TCHAR";
    case TUCHAR: return "TUCHAR";
    case TINT:   return "TINT";
    case TUINT:  return "TUINT";
    case TLINT:  return "TLINT";
    case TULINT: return "TULINT";
    case TFLT:   return "TFLT";
    case TDBL:   return "TDBL";

    default:
      throw COMException("unknown channel type!");
  }
}

//---------------------------------------------------------------
