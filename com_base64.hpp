//---------------------------------------------------------------
//
//  C O M _ B A S E 6 4 . H P P
//
//---------------------------------------------------------------

#ifndef COMBase64Hpp
#define COMBase64Hpp

//---------------------------------------------------------------

#include "com_exception.hpp"

//---------------------------------------------------------------

/**
   Exception eines COMBase64-Objektes
*/

class ECOMBase64 : public COMException
{
public:
  ECOMBase64(const string &pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

/**
   Base64 Kodierung/Dekodierung

   Erm�glicht Base64 Kodierung und Dekodierung und speichert die
   Ergebnisse in einem internen Puffer.
*/

class COMBase64
{
public:
  COMBase64();
  ~COMBase64();

  void encode(const char *, unsigned int);
  void decode(const char *, unsigned int);

  const char *output() const;
  unsigned int length() const;

private:
  char *_out_buf; /**< Zeiger auf den Ergebnispuffer */
  unsigned int _out_len; /**< L�nge des Ergebnispuffers */
};

//---------------------------------------------------------------

/**
   Erm�glicht Lesezugriff auf den Ergebnispuffer

   \return Konstanter Zeiger auf den Ergebnispuffer
 */

inline const char *COMBase64::output() const
{
  return _out_buf;
}

//---------------------------------------------------------------

/**
   Ermittelt die L�nge der Daten im Ergebnispuffer

   \return L�nge in Bytes
 */

inline unsigned int COMBase64::length() const
{
  return _out_len;
}

//---------------------------------------------------------------

#endif
