/******************************************************************************
 *
 *  $Id$
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

#include <sys/types.h>
#include <dirent.h>

#include <iostream>
#include <sstream>
using namespace std;

#include <FL/Fl.H>
#include <FL/fl_draw.H>

/*****************************************************************************/

#include "ViewGlobals.h"
#include "ViewViewMsg.h"

//#define DEBUG

#define FRAME_WIDTH 3
#define LEVEL_HEIGHT 15
#define TRACK_BAR_WIDTH 20

/*****************************************************************************/

const Fl_Color msg_colors[LibDLS::Job::Message::TypeCount] =
{
    FL_BLUE,           // Info
    FL_DARK_YELLOW,    // Warning
    FL_RED,            // Error
    FL_MAGENTA,        // Critical Error
    fl_darker(FL_GRAY) // Broadcast
};

/*****************************************************************************/

/**
   Constructor

   \param x X-Position of the left edge
   \param y Y-Position of the upper edge
   \param w Width of the widget
   \param h Height of the widget
   \param label Label of the widget
*/

ViewViewMsg::ViewViewMsg(int x, int y, int w, int h, const char *label)
    : Fl_Widget(x, y, w, h, label)
{
    _focused = false;
    _track_bar = new Fl_Track_Bar();
}

/*****************************************************************************/

/**
   Destructor
*/

ViewViewMsg::~ViewViewMsg()
{
    _messages.clear();
    delete _track_bar;
}

/*****************************************************************************/

/**
   Delete all messages
*/

void ViewViewMsg::clear()
{
    _messages.clear();
    redraw();
}

/*****************************************************************************/

/**
   Load messages in the specified time range

   \param start Start time of the area
   \param end End time of the range
*/

void ViewViewMsg::load_msg(const LibDLS::Job *job,
        LibDLS::Time start, LibDLS::Time end)
{
    clear();

    _range_start = start;
    _range_end = end;

    list<LibDLS::Job::Message> msgs = job->load_msg(start, end);

    for (list<LibDLS::Job::Message>::const_iterator i = msgs.begin();
            i != msgs.end(); i++) {
        Message msg;
        msg.message = *i;
        msg.level = 0;
        _messages.push_back(msg);
    }

    redraw();
}

/*****************************************************************************/

/**
   FLTK character function
*/

void ViewViewMsg::draw()
{
    list<Message>::iterator msg_i;
    int i, text_width, text_height;
    double scale_x;
    int xp, scroll_pos;

    // Set font and size
    fl_font(FL_HELVETICA, 10);

    // Drawing background
    draw_box(FL_DOWN_BOX, FL_WHITE);

    if (_range_end <= _range_start) return;

    // Calculate scaling
    scale_x = (w() - 2 * FRAME_WIDTH) / (_range_end - _range_start).to_dbl();

    // Calculate levels on which to draw
    // each message
    _calc_msg_levels();

    // Draw a scroll bar
    _track_bar->content_height(_level_count * LEVEL_HEIGHT);
    _track_bar->view_height(h() - 2 * FRAME_WIDTH);
    _track_bar->draw(x() + w() - FRAME_WIDTH - TRACK_BAR_WIDTH,
                     y() + FRAME_WIDTH,
                     TRACK_BAR_WIDTH,
                     h() - 2 * FRAME_WIDTH);

    scroll_pos = _track_bar->position();

    // Set up clipping
    if (_track_bar->visible()) {
        fl_push_clip(x() + FRAME_WIDTH, y() + FRAME_WIDTH,
                     w() - 2 * FRAME_WIDTH - TRACK_BAR_WIDTH - 1,
                     h() - 2 * FRAME_WIDTH);
    }
    else {
        fl_push_clip(x() + FRAME_WIDTH, y() + FRAME_WIDTH,
                     w() - 2 * FRAME_WIDTH, h() - 2 * FRAME_WIDTH);
    }

    // Draw level from bottom to top
    for (i = _level_count - 1; i >= 0; i--) {
        msg_i = _messages.begin();
        while (msg_i != _messages.end()) {
            if (msg_i->level == i) {
                xp = (int)
                    ((msg_i->message.time - _range_start).to_dbl() * scale_x);

                // Measure text
                text_width = 0;
                fl_measure(msg_i->message.text.c_str(),
                        text_width, text_height, 0);

#ifdef DEBUG
                cout << text_width << " " << text_height << " "
                     << msg_i->message.text << endl;
#endif

                // Drawing background behing the text white
#ifdef DEBUG
                fl_color(FL_RED);
#else
                fl_color(FL_WHITE);
#endif
                fl_rectf(x() + FRAME_WIDTH + xp,
                         y() + FRAME_WIDTH + i * LEVEL_HEIGHT - scroll_pos,
                         text_width + 5,
                         LEVEL_HEIGHT);

                // Timeline draw
                fl_color(FL_BLACK);
                fl_line(x() + FRAME_WIDTH + xp,
                        y() + FRAME_WIDTH - scroll_pos,
                        x() + FRAME_WIDTH + xp,
                        y() + FRAME_WIDTH + i * LEVEL_HEIGHT
                        + LEVEL_HEIGHT / 2 - scroll_pos);
                fl_line(x() + FRAME_WIDTH + xp,
                        y() + FRAME_WIDTH + i * LEVEL_HEIGHT
                        + LEVEL_HEIGHT / 2 - scroll_pos,
                        x() + FRAME_WIDTH + xp + 2,
                        y() + FRAME_WIDTH + i * LEVEL_HEIGHT
                        + LEVEL_HEIGHT / 2 - scroll_pos);

                // Drawing text
                if (msg_i->message.type >= 0 &&
                        msg_i->message.type
                        < LibDLS::Job::Message::TypeCount) {
                    fl_color(msg_colors[msg_i->message.type]);
                }
                else {
                    fl_color(FL_BLACK);
                }
                fl_draw(msg_i->message.text.c_str(),
                        x() + FRAME_WIDTH + xp + 4,
                        y() + FRAME_WIDTH + i * LEVEL_HEIGHT
                        + (LEVEL_HEIGHT - text_height) / 2
                        + text_height - fl_descent() - scroll_pos);
            }

            msg_i++;
        }
    }

    // Remove clipping again
    fl_pop_clip();
}

/*****************************************************************************/

/**
   Calculate the display level for each message
*/

void ViewViewMsg::_calc_msg_levels()
{
    double scale_x;
    int level, xp;
    list<Message>::iterator msg_i;
    list<int> levels;
    list<int>::iterator level_i;
    bool found;

    _level_count = 0;

    if (_range_end <= _range_start) return;

    // Calculate scaling
    scale_x = (w() - 2 * FRAME_WIDTH) / (_range_end - _range_start).to_dbl();

    msg_i = _messages.begin();
    while (msg_i != _messages.end()) {
        xp = (int) ((msg_i->message.time - _range_start).to_dbl() * scale_x);

        // Find a line where the message can be drawn
        level = 0;
        found = false;
        level_i = levels.begin();
        while (level_i != levels.end()) {
            if (*level_i < xp) { // Message would fit in this line
                found = true;
                msg_i->level = level;

                // Note end position in line
                *level_i = (int)
                    (xp + fl_width(msg_i->message.text.c_str())) + 5;
                break;
            }

            level_i++;
            level++;
        }

        if (!found) {
            msg_i->level = level;

            // All lines full. Create new ones.
            levels.push_back((int)
                    (xp + fl_width(msg_i->message.text.c_str())) + 5);
            _level_count++;
        }

        msg_i++;
    }
}

/*****************************************************************************/

/**
   FLTK event function

   \param event event type
   \return 1, wenn Event was processed
*/

int ViewViewMsg::handle(int event)
{
    int xp, yp;

    xp = Fl::event_x() - x();
    yp = Fl::event_y() - y();

    if (_track_bar->handle(event, xp - w() + TRACK_BAR_WIDTH,
                           yp - FRAME_WIDTH)) {
        redraw();
        return 1;
    }

    switch (event) {
        case FL_PUSH:
            take_focus();
            return 1;

        case FL_FOCUS:
            _focused = true;
            redraw();
            return 1;

        case FL_UNFOCUS:
            _focused = false;
            redraw();
            return 1;

        default:
            return 0;
    }

    return 0;
}

/*****************************************************************************/
