//---------------------------------------------------------------
//
//  C O M _ Z L I B . H P P
//
//---------------------------------------------------------------

#ifndef COMZLibHpp
#define COMZLibHpp

//---------------------------------------------------------------

#include "com_exception.hpp"

//---------------------------------------------------------------

/**
   Exception eines COMZlib-Objektes

   Tritt meistens auf, wenn der Augabepuffer zu
   klein ist (Fehler 5).
*/

class ECOMZLib : public COMException
{
public:
  ECOMZLib(const string &pmsg) : COMException(pmsg) {};
};

//---------------------------------------------------------------

/**
   ZLib-Kompressionsklasse

   Stellt alle nötigen Funktionen bereit, um Daten mit der
   ZLib zu komprimieren und zu dekomprimieren.
*/

class COMZLib
{
public:
  COMZLib();
  ~COMZLib();

  void compress(const char *, unsigned int);
  void uncompress(const char *, unsigned int, unsigned int);

  const char *output() const;
  unsigned int output_length() const;

private:
  char *_out_buf;        /**< Ausgabepuffer */
  unsigned int _out_len; /**< Länge der datem im Ausgabepuffer */

  void _realloc(unsigned int);
};

//---------------------------------------------------------------

/**
   Ermöglicht lesenden Zugriff auf den Ausgabepuffer

   \return Konstanter Zeiger auf den Ausgabepuffer
*/

inline const char *COMZLib::output() const
{
  return _out_buf;
}

//---------------------------------------------------------------

/**
   Liefert die Länge der ausgegebenen Daten

   \return Anzahl zeichen im Ausgabepuffer
*/

inline unsigned int COMZLib::output_length() const
{
  return _out_len;
}

//---------------------------------------------------------------

#endif
