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
  unsigned int output_size() const;

  void free();

private:
  char *_out_buf;         /**< Ausgabepuffer */
  unsigned int _out_size; /**< Länge der datem im Ausgabepuffer */
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

   \return Anzahl Zeichen im Ausgabepuffer
*/

inline unsigned int COMZLib::output_size() const
{
  return _out_size;
}

//---------------------------------------------------------------

#endif
