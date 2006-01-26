//---------------------------------------------------------------
//
//  C O M _ B A S E 6 4 . C P P
//
//---------------------------------------------------------------

#include <string.h>

//---------------------------------------------------------------

#include "com_base64.hpp"

//---------------------------------------------------------------

static const char base64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char pad64 = '=';

//---------------------------------------------------------------

COMBase64::COMBase64()
{
  _out_buf = 0;
}

//---------------------------------------------------------------

COMBase64::~COMBase64()
{
  if (_out_buf) delete [] _out_buf;
}

//---------------------------------------------------------------

/**
   Kodiert beliebige Binärdaten in Base64

   \param src Zeiger auf den Puffer mit den Binärdaten
   \param src_len Länge der Binärdaten
   \throw ECOMBase64 Zu wenig Speicher beim Kodieren
 */

void COMBase64::encode(const char *src, unsigned int src_len)
{
  unsigned int datalength = 0, out_size = (int) (src_len * 4.0 / 3 + 4);
  unsigned char input[3];
  unsigned char output[4];
  unsigned int i;

  if (_out_buf) delete [] _out_buf;
  _out_buf = 0;

  if (!src_len) return;

  try
  {
    _out_buf = new char[out_size];
  }
  catch (...)
  {
    throw ECOMBase64("could not allocate enough memory!");
  }

  while (2 < src_len)
  {
    input[0] = *src++;
    input[1] = *src++;
    input[2] = *src++;
    src_len -= 3;

    output[0] = input[0] >> 2;
    output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
    output[2] = ((input[1] & 0x0F) << 2) + (input[2] >> 6);
    output[3] = input[2] & 0x3F;

    if (datalength + 4 > out_size)
    {
      throw ECOMBase64("output buffer full!");
    }

    _out_buf[datalength++] = base64[output[0]];
    _out_buf[datalength++] = base64[output[1]];
    _out_buf[datalength++] = base64[output[2]];
    _out_buf[datalength++] = base64[output[3]];
  }

  if (0 != src_len)
  {
    input[0] = input[1] = input[2] = '\0';
    for (i = 0; i < src_len; i++) input[i] = *src++;

    output[0] = input[0] >> 2;
    output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
    output[2] = ((input[1] & 0x0F) << 2) + (input[2] >> 6);

    if (datalength + 4 > out_size)
    {
      throw ECOMBase64("output buffer full!");
    }

    _out_buf[datalength++] = base64[output[0]];
    _out_buf[datalength++] = base64[output[1]];

    if (src_len == 1) _out_buf[datalength++] = pad64;
    else _out_buf[datalength++] = base64[output[2]];

    _out_buf[datalength++] = pad64;
  }

  if (datalength >= out_size)
  {
    throw ECOMBase64("output buffer full!");
  }

  _out_buf[datalength] = '\0';

  _out_len = datalength;
}

//---------------------------------------------------------------

/**
   Dekodiert Base64-Daten

   \param src Zeiger auf einen Puffer mit Base64-Daten
   \param src_len Länge der Base64-Daten
   \throw ECOMBase64 Zu wenig Speicher oder Formatfehler
 */

void COMBase64::decode(const char *src, unsigned int src_len)
{
  int tarindex, state, ch;
  unsigned int out_size = src_len + 1;
  char *pos;

  if (_out_buf) delete [] _out_buf;
  _out_buf = 0;

  if (!src_len) return;

  try
  {
    _out_buf = new char[out_size];
  }
  catch (...)
  {
    throw ECOMBase64("could not allocate enough memory!");
  }

  state = 0;
  tarindex = 0;

  while ((ch = *src++) != '\0')
  {
    if (ch == ' ') continue;
    if (ch == pad64) break;

    pos = strchr(base64, ch);
    if (pos == 0)
    {
      throw ECOMBase64("found illegal character while decoding!");
    }

    switch (state)
    {
      case 0:
        if (_out_buf)
        {
          if ((size_t) tarindex >= out_size)
          {
            throw ECOMBase64("output buffer full!");
          }

          _out_buf[tarindex] = (pos - base64) << 2;
        }
        state = 1;
        break;

      case 1:
        if (_out_buf)
        {
          if ((size_t) tarindex + 1 >= out_size)
          {
            throw ECOMBase64("output buffer full!");
          }

          _out_buf[tarindex] |= (pos - base64) >> 4;
          _out_buf[tarindex + 1] = ((pos - base64) & 0x0f) << 4;
        }
        tarindex++;
        state = 2;
        break;

      case 2:
        if (_out_buf)
        {
          if ((size_t) tarindex + 1 >= out_size)
          {
            throw ECOMBase64("output buffer full!");
          }

          _out_buf[tarindex] |= (pos - base64) >> 2;
          _out_buf[tarindex + 1] = ((pos - base64) & 0x03) << 6;
        }
        tarindex++;
        state = 3;
        break;

      case 3:
        if (_out_buf)
        {
          if ((size_t) tarindex >= out_size)
          {
            throw ECOMBase64("output buffer full!");
          }

          _out_buf[tarindex] |= (pos - base64);
        }
        tarindex++;
        state = 0;
        break;

      default: throw ECOMBase64("unknown state!");
    }
  }

  if (ch == pad64)
  {          
    ch = *src++;

    switch (state)
    {
      case 0:
      case 1: throw ECOMBase64("unknown state (padding)!");

      case 2:
        for ((void) NULL; ch != '\0'; ch = *src++)
        {
          if (ch != ' ') break;
        }

        if (ch != pad64) throw ECOMBase64("unexpected character!");
        ch = *src++;

      case 3:
        for ((void) NULL; ch != '\0'; ch = *src++)
        {
          if (ch != ' ') throw ECOMBase64("unexpected character!");
        }

        if (_out_buf && _out_buf[tarindex] != 0) throw ECOMBase64("error ???");
    }
  }
  else if (state != 0) throw ECOMBase64("unknown ending state!");

  _out_len = tarindex;
}

//---------------------------------------------------------------
