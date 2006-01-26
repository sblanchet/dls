//---------------------------------------------------------------
//
//  C O M _ E X C E P T I O N . H P P
//
//---------------------------------------------------------------

#ifndef COMExceptionHpp
#define COMExceptionHpp

//---------------------------------------------------------------

#include <string>
using namespace std;

//---------------------------------------------------------------

/**
   Basisklasse aller Exceptions der DLS-Klassen
*/

class COMException
{
public:

  /**
     Konstruktor

     \param pmsg Nachricht der zu werfenden Exception
  */

  COMException(const string &pmsg)
  {
    msg = pmsg;
  };

  string msg; /**< Nachricht der Exception */

private:

  /**
     Standardkonstruktor

     Privat, da er nicht aufgerufen werden soll.
  */

  COMException();
};

//---------------------------------------------------------------

#endif
