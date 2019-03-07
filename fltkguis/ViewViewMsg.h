/******************************************************************************
 *
 *  This file is part of the Data Logging Service (DLS).
 *
 *  DLS is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  DLS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with DLS. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef ViewViewMsgHpp
#define ViewViewMsgHpp

/*****************************************************************************/

#include <list>
using namespace std;

#include <FL/Fl_Widget.H>

/*****************************************************************************/

#include "lib/LibDLS/Time.h"
#include "lib/LibDLS/Job.h"

#include "fl_track_bar.h"

/*****************************************************************************/

/**
   Widget to display MSR messages
*/

class ViewViewMsg : public Fl_Widget
{
public:
    ViewViewMsg(int, int, int, int, const char * = "");
    ~ViewViewMsg();

    void load_msg(const LibDLS::Job *, LibDLS::Time, LibDLS::Time);
    void clear();

private:
    Fl_Track_Bar *_track_bar; /**< Verticale scroll bar */

    // Data
    struct Message {
        LibDLS::Job::Message message;
        int level;
    };
    list<Message> _messages; /**< List of loaded messages */
    LibDLS::Time _range_start; /**< Start of the time span to be displayed */
    LibDLS::Time _range_end; /**< End of the time span to be displayed */
    int _level_count; /**< Current number of levels to be displayed */

    // Widget condition
    bool _focused; /**< The widget has currently the focus */

    virtual void draw();
    virtual int handle(int);

    void _calc_msg_levels();
};

/*****************************************************************************/

#endif
