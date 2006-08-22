/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef ViewViewMsgHpp
#define ViewViewMsgHpp

/*****************************************************************************/

#include <list>
using namespace std;

#include <FL/Fl_Widget.h>

/*****************************************************************************/

#include "com_time.hpp"

#include "lib_job.hpp"
using namespace LibDLS;

#include "fl_track_bar.hpp"

/*****************************************************************************/

/**
   Widget zum Anzeigen der MSR-Nachrichten
*/

class ViewViewMsg : public Fl_Widget
{
public:
    ViewViewMsg(int, int, int, int, const char * = "");
    ~ViewViewMsg();

    void load_msg(const Job *, COMTime, COMTime);
    void clear();

private:
    Fl_Track_Bar *_track_bar; /**< Vertikale Scroll-Leiste */

    // Daten
    list<ViewMSRMessage> _messages; /**< Liste der geladenen Nachrichten */
    COMTime _range_start; /**< Startzeit der anzuzeigenden Zeitspanne */
    COMTime _range_end; /**< Endzeit der anzuzeigenden Zeitspanne */
    int _level_count; /**< Aktuelle Anzahl der anzuzeigenden Ebenen */

    // Widget-Zustand
    bool _focused; /**< Das Widget hat gerade den Fokus */

    virtual void draw();
    virtual int handle(int);

    void _calc_msg_levels();
};

/*****************************************************************************/

#endif
