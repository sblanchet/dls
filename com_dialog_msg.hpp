//---------------------------------------------------------------
//
//  C O M _ D I A L O G _ M S G . H P P
//
//---------------------------------------------------------------

#ifndef ComDialogMsgHpp
#define ComDialogMsgHpp

//---------------------------------------------------------------

#include <vector>
#include <sstream>
using namespace std;

#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Button.h>

//---------------------------------------------------------------

#include "fl_grid.hpp"

//---------------------------------------------------------------

/**
   Nachricht mit Text und Typ f�r COMDialogMsg
*/

struct COMMsg
{
  int type;
  string text;
};

//---------------------------------------------------------------

/**
   Allgemeiner Dialog zum Anzeigen von Fehlern und Warnungen
*/

class COMDialogMsg
{
public:
  COMDialogMsg();
  ~COMDialogMsg();

  stringstream &str();
  void error();
  void warning();

private:
  Fl_Double_Window *_wnd; /**< Dialogfenster */
  Fl_Grid *_grid_msg;     /**< Grid zum Anzeigen der Meldungen */
  Fl_Button *_button_ok;  /**< "OK"-Button */

  vector<COMMsg> _messages; /**< Vektor mit den aktuell angezeigten Meldungen */
  stringstream _str;        /**< Stream zum einfachen Hinzuf�gen von Meldungen */
  
  static void _callback(Fl_Widget *, void *);
  void _button_ok_clicked();
  void _grid_msg_callback();
};

//---------------------------------------------------------------

inline stringstream &COMDialogMsg::str()
{
  return _str;
}

//---------------------------------------------------------------

#endif
