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

#include <iostream>
using namespace std;

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include "lib/LibDLS/globals.h"

#include "fl_grid.h"

/*****************************************************************************/

#define MIN_SCROLL_HEIGHT 10
#define LEFT_MARGIN 4
#define FRAME_WIDTH 3

/*****************************************************************************/

/**
   Constructor

   \param name Identifier of the new column name
   \param title Title to display
   \param width Relative width
*/

Fl_Grid_Column::Fl_Grid_Column(const string &name,
                               const string &title, int width)
{
    _name = name;
    _title = title;
    _width = width;
}

/*****************************************************************************/

/**
   Desctructor
*/

Fl_Grid_Column::~Fl_Grid_Column()
{
}

/*****************************************************************************/

/**
   Constructor

   \param xp Horizontal position of the upper left corner in pixel
   \param yp Vertical position of the upper left corrner in pixel
   \param wp Width of the widgets in pixels
   \param hp Height of the widget in pixels
   \param label Name of the widget in FLTK (not used)
*/

Fl_Grid::Fl_Grid(int xp, int yp, int wp, int hp, const char *label)
    : Fl_Widget(xp, yp, wp, hp, label)
{
    _cb = 0;
    _record_count = 0;
    _row_height = 21;
    _scroll_index = 0;
    _focused = false;
    _select_mode = flgSingleSelect;
    _scroll_tracking = false;
    _range_select_possible = false;
    _check_boxes = false;
}

/*****************************************************************************/

/**
   Destructor
*/

Fl_Grid::~Fl_Grid()
{
    // No deselect_all(), callbacks could fail!
}

/*****************************************************************************/

/**
   Add a column

   \param name Identifier name of the new column
   \param title Title to display
   \param width Relative width
*/

void Fl_Grid::add_column(const string &name,
                         const string &title, int width)
{
    Fl_Grid_Column col(name, title, width);
    _cols.push_back(col);
    redraw();
}

/*****************************************************************************/

/**
   Set the callback function

   The callback function is called on certain events.
   This includes:
   - Drawing a cell
   - Selecting / deselecting a record
   - Double-clicking on a record

   \param cb Pointer to the callback function to call
   \param data Data to tranfer when calling callback function
*/

void Fl_Grid::callback(void (*cb)(Fl_Widget *, void *), void *data)
{
    _cb = cb;
    _cb_data = data;
}

/*****************************************************************************/

/**
   Set the global line height

   All lines have the same line height in pixels.
   The line height must not be zero, because many calculation
   that divide by the line height would fail!

   \param height Line height
*/

void Fl_Grid::row_height(unsigned int height)
{
    if (height != _row_height && height > 0)
    {
        _row_height = height;
        redraw();
    }
}

/*****************************************************************************/

/**
   Set the content of the current cell

   Call only during a callback of type flgContent

   \param content Content to display
*/

void Fl_Grid::current_content(const string &content)
{
    _content = content;
}

/*****************************************************************************/

/**
   Set the color of the current cell content

   Call only during a callback of type flgContent

   Can be used with the FLTK color constants (FL_RED, FL_BLACK, etc.),
   or with the Fl_Color(R, G, B) constructor.

   \param col Text color
*/

void Fl_Grid::current_content_color(Fl_Color col)
{
    _content_color = col;
}

/*****************************************************************************/


void Fl_Grid::current_checked(bool checked)
{
    _checked = checked;
}

/*****************************************************************************/

/**
   Set the number of record to display

   If the new number of records is different from the current one,
   it causes

   - all currently selected records are deselected
   (the corresponding callbacks are executed)
   - the scroll position is set all the way up
   - the grid is redraw

   \param count Count
*/

void Fl_Grid::record_count(unsigned int count)
{
    if (count != _record_count)
    {
        deselect_all();

        _record_count = count;
        _scroll_index = 0;

        redraw();
    }
}

/*****************************************************************************/

/**
   Set the number of record to be displayed to 0

   \see record_count()
*/

void Fl_Grid::clear()
{
    record_count(0);
}

/*****************************************************************************/

/**
   Return the number of currently selected records

   \return Number of selected records
*/

unsigned int Fl_Grid::select_count() const
{
    return _selected.size();
}

/*****************************************************************************/

/**
   Return the index of the first selected record

   Should only be used in single-select mode

   \return Index of the selected record
   \throws char* No record is selected
*/

unsigned int Fl_Grid::selected_index() const
{
    if (_selected.size())
    {
        return *_selected.begin();
    }
    else throw "ERROR";
}

/*****************************************************************************/

/**
   Return the index of the record currently displayed
   in top tosition

   \return Record-Index
*/

unsigned int Fl_Grid::top_index() const
{
    return _scroll_index;
}

/*****************************************************************************/

/**
   Scroll to the specified record index

   It tries to get the specified record to appear in
   the top line.

   \param index Record-Index
*/

void Fl_Grid::scroll(unsigned int index)
{
    int row_count = h() / _row_height - 1;

    if (index >= _record_count - row_count)
    {
        index = _record_count - row_count;
    }

    _scroll_index = index;

    redraw();
}

/*****************************************************************************/

/**
   Set the selection mode

   The following values are possible:

   - flgNoSelect:     Selecting records is no possible
   - flgSingleSelect: Only one record can ben selected at a time
   - flgMultiSelect:  Several records can be selected

   \param mode Selection mode
*/

void Fl_Grid::select_mode(Fl_Grid_Select_Mode mode)
{
    deselect_all();
    _select_mode = mode;
}

/*****************************************************************************/

/**
   Deselect all currently selected records

   The corresponding callbacks are called!
*/

void Fl_Grid::deselect_all()
{
    list<unsigned int>::iterator sel_i;
    unsigned int index;

    _range_select_possible = false;

    while (_selected.begin() != _selected.end())
    {
        index = *_selected.begin();

        _selected.erase(_selected.begin());

        if (_cb)
        {
            _event = flgDeSelect;
            _event_record = index;
            _cb(this, _cb_data);
        }
    }
}

/*****************************************************************************/

/**
   Select a specific record

   Before all others are deselected.
   To select several records, please use select_add!

   \param index Record-Index
*/

void Fl_Grid::select(unsigned int index)
{
    deselect_all();
    select_add(index);
}

/*****************************************************************************/

/**
   Select anohter record

   Add the specified record to the list of currently
   selected records. This only works if

   - Selecting skin is possible
   - The specified record index makes sense
   - The specified record is not yet selected

   If the selection mode flgSingleSelect is active
   and a record has already been selected, it will
   be deselected.

   \param index Record-Index
*/

void Fl_Grid::select_add(unsigned int index)
{
    if (_select_mode == flgNoSelect || index >= _record_count) return;

    if (selected(index)) return;

    if (_select_mode == flgSingleSelect) deselect_all();

    _selected.push_back(index);

    _range_select_partner = index;
    _range_select_possible = true;

    if (_cb)
    {
        _event = flgSelect;
        _event_record = index;
        _cb(this, _cb_data);
    }

    redraw();
}

/*****************************************************************************/

/**
   Check if a specified record is selected

   \param index Record-Index
   \return true, if the specified record is selected
*/

bool Fl_Grid::selected(unsigned int index) const
{
    list<unsigned int>::const_iterator sel_i;

    if (_select_mode == flgNoSelect || index >= _record_count)
    {
        return false;
    }

    sel_i = _selected.begin();
    while (sel_i != _selected.end())
    {
        if (*sel_i == index) return true;
        sel_i++;
    }

    return false;
}

/*****************************************************************************/

/**
   Deselect the specified record

   \param index Record-Index
*/

void Fl_Grid::deselect(unsigned int index)
{
    list<unsigned int>::iterator sel_i;

    _range_select_possible = false;

    sel_i = _selected.begin();
    while (sel_i != _selected.end())
    {
        if (*sel_i == index)
        {
            _selected.erase(sel_i);

            if (_cb)
            {
                _event = flgDeSelect;
                _event_record = index;
                _cb(this, _cb_data);
            }

            return;
        }
        sel_i++;
    }
}

/*****************************************************************************/

/**
   Select a range or records

   The range is selected from the last selected record to
   the current one. If there was no selection, or was
   previousy deselected, this is not possible.

   \param index Record-Index of the clicked record
*/

void Fl_Grid::_range_select(unsigned int index)
{
    unsigned int start, end;

    if (!_range_select_possible) return;

    if (index < _range_select_partner)
    {
        start = index;
        end = _range_select_partner;
    }
    else
    {
        start = _range_select_partner;
        end = index;
    }

    for (unsigned int i = start; i <= end; i++)
    {
        select_add(i);
    }

    _range_select_partner = index;
}

/*****************************************************************************/

/**
   Determine whether there should be a checkbox to the left of each line

   \param check true, if checkboxes should be displayed
*/

void Fl_Grid::check_boxes(bool check)
{
    if (_check_boxes != check)
    {
        _check_boxes = check;
        redraw();
    }
}

/*****************************************************************************/

/**
   Draw the grid

   This function is called by FLTK as soon as the widget is redrawn.
   It can not be called "by hand" because the drawing functions only
   work in a certain context. To force a redraw, so please use redraw ()!
*/

void Fl_Grid::draw()
{
    int drawing_width, drawing_height;
    list<Fl_Grid_Column>::iterator col_i;
    unsigned int width_sum;
    float width_factor;
    unsigned int rec_index;
    bool rec_selected;
    unsigned int row_count;
    int scroll_height, scroll_pos, left, top;

    drawing_width = w() - 2 * FRAME_WIDTH;
    drawing_height = h() - 2 * FRAME_WIDTH;

    // Drawing backgroup
    draw_box(FL_DOWN_BOX, FL_WHITE);

    // Draw focus lines
    if (_focused) draw_focus();

    // Add column widths
    col_i = _cols.begin();
    width_sum = 0;
    while (col_i != _cols.end())
    {
        width_sum += col_i->width();
        col_i++;
    }

    if (width_sum == 0) return;

    // Force clipping
    fl_push_clip(x() + FRAME_WIDTH, y() + FRAME_WIDTH,
                 drawing_width, drawing_height);

    row_count = drawing_height / _row_height - 1;

    if (_record_count <= row_count)
    {
        width_factor = (_check_boxes
                        ? drawing_width - _row_height
                        : drawing_width)
            / (float) width_sum;
        _scroll_index = 0;
    }
    else // Is it a scrollbar needed?
    {
        width_factor = ((_check_boxes
                         ? drawing_width - _row_height
                         : drawing_width) - 20)
            / (float) width_sum;

        if (_scroll_index > _record_count - row_count)
        {
            _scroll_index = _record_count - row_count;
        }

        // Draw scrollbar
        fl_color(150, 150, 150);
        fl_rectf(x() + FRAME_WIDTH + drawing_width - 19,
                 y() + FRAME_WIDTH + 1, 18, 18);
        fl_rectf(x() + FRAME_WIDTH + drawing_width - 19,
                 y() + FRAME_WIDTH + drawing_height - 19, 18, 18);
        fl_color(200, 200, 200);
        fl_polygon(x() + FRAME_WIDTH + drawing_width - 17,
                   y() + FRAME_WIDTH + 14,
                   x() + FRAME_WIDTH + drawing_width - 5,
                   y() + FRAME_WIDTH + 14,
                   x() + FRAME_WIDTH + drawing_width - 11,
                   y() + FRAME_WIDTH + 5);
        fl_polygon(x() + FRAME_WIDTH + drawing_width - 17,
                   y() + FRAME_WIDTH + drawing_height - 14,
                   x() + FRAME_WIDTH + drawing_width - 5,
                   y() + FRAME_WIDTH + drawing_height - 14,
                   x() + FRAME_WIDTH + drawing_width - 11,
                   y() + FRAME_WIDTH + drawing_height - 5);

        scroll_height = (int) (row_count
                               / (double) _record_count // _record_count >0, da
                               * (drawing_height - 38)); // _record_count
                                                         //    > _row_count
        if (scroll_height < MIN_SCROLL_HEIGHT)
            scroll_height = MIN_SCROLL_HEIGHT;

        scroll_pos = (int) (_scroll_index
                            / (double) (_record_count
                                        - row_count) // Definitely >0
                            * ((drawing_height - 38) - scroll_height));

        fl_rectf(x() + FRAME_WIDTH + drawing_width - 19,
                 y() + FRAME_WIDTH + 19 + scroll_pos, 18, scroll_height);
    }

    // Draw headers
    col_i = _cols.begin();
    width_sum = 0;
    while (col_i != _cols.end())
    {
        left = (int) (width_sum * width_factor)
            + (_check_boxes ? _row_height : 0) + 1;

        // Header background
        fl_color(200, 200, 200);
        fl_rectf(x() + FRAME_WIDTH + left,
                 y() + FRAME_WIDTH + 1,
                 (int) (col_i->width() * width_factor) - 2,
                 _row_height - 2);

        // Header text
        fl_color(0, 0, 0);
        fl_font(FL_HELVETICA | FL_BOLD, 12);
        fl_push_clip(x() + FRAME_WIDTH + left + 1,
                     y() + FRAME_WIDTH + 2,
                     (int) (col_i->width() * width_factor) - 4,
                     _row_height - 4);
        fl_draw(col_i->title().c_str(),
                x() + FRAME_WIDTH + left + LEFT_MARGIN,
                y() + FRAME_WIDTH + (int) (0.5 * _row_height + fl_descent()));
        fl_pop_clip();

        width_sum += col_i->width();
        col_i++;
    }

    for (unsigned int i = 0; i < _record_count; i++)
    {
        if ((i + 2) * _row_height > (unsigned int) drawing_height) break;

        if (i + _scroll_index < 0 || i + _scroll_index >= _record_count)
            continue;

        rec_index = i + _scroll_index;
        rec_selected = selected(rec_index);
        top = (i + 1) * _row_height + 1;

        if (_check_boxes)
        {
            // cell background
            if (rec_selected)
            {
                fl_color(82, 133, 156);
            }
            else
            {
                fl_color(230, 230, 230);
            }

            fl_rectf(x() + FRAME_WIDTH + 1, y() + FRAME_WIDTH + top,
                     _row_height, _row_height - 2);

            fl_color(0, 0, 0);
            fl_rect(x() + FRAME_WIDTH + 3, y() + FRAME_WIDTH + top + 2,
                    _row_height - 6, _row_height - 6);
            fl_color(255, 255, 255);
            fl_rectf(x() + FRAME_WIDTH + 4, y() + FRAME_WIDTH + top + 3,
                     _row_height - 8, _row_height - 8);

            // check 'Checked' status
            _checked = false;
            if (_cb)
            {
                _event = flgChecked;
                _event_record = rec_index;
                _cb(this, _cb_data);
            }

            if (_checked)
            {
                fl_color(0, 0, 0);
                fl_line(x() + FRAME_WIDTH + 4,
                        y() + FRAME_WIDTH + top + 3,
                        x() + FRAME_WIDTH + _row_height - 5,
                        y() + FRAME_WIDTH + top + _row_height - 6);
                fl_line(x() + FRAME_WIDTH + 4,
                        y() + FRAME_WIDTH + top + _row_height - 6,
                        x() + FRAME_WIDTH + _row_height - 5,
                        y() + FRAME_WIDTH + top + 3);
            }
        }

        col_i = _cols.begin();
        width_sum = 0;
        while (col_i != _cols.end())
        {
            // Cells background
            if (rec_selected)
            {
                fl_color(82, 133, 156);
            }
            else
            {
                fl_color(230, 230, 230);
            }

            left = (int) (width_sum * width_factor)
                + (_check_boxes ? _row_height : 0) + 1;

            fl_rectf(x() + FRAME_WIDTH + left,
                     y() + FRAME_WIDTH + (i + 1) * _row_height + 1,
                     (int) (col_i->width() * width_factor) - 2,
                     _row_height - 2);

            // Cells text
            if (rec_selected)
            {
                _content_color = fl_rgb_color(255, 255, 255);
            }
            else
            {
                _content_color = fl_rgb_color(0, 0, 0);
            }

            // Request cell contents
            _content = "";
            if (_cb)
            {
                _event = flgContent;
                _event_record = rec_index;
                _event_col = col_i->name();
                _event_sel = rec_selected;
                _cb(this, _cb_data);
            }

            fl_font(FL_HELVETICA, 12);
            fl_color(_content_color);
            fl_push_clip(x() + FRAME_WIDTH + left + 1,
                         y() + FRAME_WIDTH + (i + 1) * _row_height + 2,
                         (int) (col_i->width() * width_factor) - 4,
                         _row_height - 4);
            fl_draw(_content.c_str(),
                    x() + FRAME_WIDTH + left + LEFT_MARGIN,
                    y() + FRAME_WIDTH + (int) ((i + 1.5)
                                               * _row_height + fl_descent()));
            fl_pop_clip();

            width_sum += col_i->width();
            col_i++;
        }
    }

    // Remove clipping
    fl_pop_clip();
}

/*****************************************************************************/

/**
   Let the grid process FLTK events


    This function is only called by FLTK and should not be called manually!

   \param e Event code
   \return 1, if the event was processed, otherwise 0
*/

int Fl_Grid::handle(int e)
{
    int drawing_width, drawing_height;
    unsigned int record_index, row_count;
    int row_index;
    int xp, yp;
    int scroll_height, scroll_pos;

    drawing_width = w() - 2 * FRAME_WIDTH;
    drawing_height = h() - 2 * FRAME_WIDTH;

    xp = Fl::event_x() - x() - FRAME_WIDTH;
    yp = Fl::event_y() - y() - FRAME_WIDTH;

    row_count = drawing_height / _row_height - 1;

    switch (e)
    {
        case FL_PUSH:

            take_focus();

            _push_x = xp;
            _push_y = yp;

            if (_record_count > row_count && xp > drawing_width - 20)
            {
                if (yp > drawing_height - 20) // Lower button
                {
                    if (_scroll_index < _record_count - row_count)
                    {
                        _scroll_index++;
                        redraw();
                    }
                }
                else if (yp < 20) // Upper button
                {
                    if (_scroll_index > 0)
                    {
                        _scroll_index--;
                        redraw();
                    }
                }
            }

            else // row area clicked
            {
                row_index = yp / _row_height - 1;
                record_index = row_index + _scroll_index;

                if (row_index >= 0
                    && row_index < (int) row_count
                    && record_index < _record_count) // Record clicked
                {
                    if (_select_mode != flgNoSelect)
                    {
                        if (Fl::event_key(FL_Control_L)
                            || Fl::event_key(FL_Control_R))
                        {
                            if (selected(record_index)) deselect(record_index);
                            else select_add(record_index);
                        }
                        else if (Fl::event_key(FL_Shift_L)
                                 || Fl::event_key(FL_Shift_R))
                        {
                            _range_select(record_index);
                        }
                        else
                        {
                            select(record_index);
                        }
                    }

                    // Impossible selection, but checkboxes, then
                    // a "mark" leads to the check
                    else if (_check_boxes && _cb)
                    {
                        _event = flgCheck;
                        _event_record = record_index;
                        _cb(this, _cb_data);
                        redraw();
                    }

                    if (Fl::event_clicks() == 1 && _cb)
                    {
                        Fl::event_clicks(0);

                        _event = flgDoubleClick;
                        _event_record = record_index;
                        _cb(this, _cb_data);
                    }
                }
                else
                {
                    deselect_all();
                }
            }

            return 1;

        case FL_DRAG:

            if (_record_count > row_count) // available scrollbar
            {
                // Calculate the current height and position of the scroll bar
                scroll_height = (int) (row_count
                                       / (double) _record_count
                                       * (drawing_height - 38));
                if (scroll_height < MIN_SCROLL_HEIGHT)
                    scroll_height = MIN_SCROLL_HEIGHT;
                scroll_pos = (int) (_scroll_index
                                    / (double) (_record_count - row_count)
                                    * ((drawing_height - 38) - scroll_height));

                if (_scroll_tracking)
                {
                    int new_scroll_pos = yp - 19 - _scroll_grip;

                    if (new_scroll_pos < 0)
                    {
                        new_scroll_pos = 0;
                    }
                    else if (new_scroll_pos >= drawing_height - 19)
                    {
                        new_scroll_pos = drawing_height - 19;
                    }

                    _scroll_index = (int) (new_scroll_pos
                                           / (double) (drawing_height
                                                       - 38 - scroll_height)
                                           * (_record_count - row_count));

                    redraw();
                }

                // Clicked on the scrollbar area?
                else if (_push_x > drawing_width - 20)
                {
                    if (_push_y >= 19 + scroll_pos
                        && _push_y < 19 + scroll_pos + scroll_height)
                    {
                        _scroll_tracking = true;
                        _scroll_grip = _push_y - scroll_pos - 19;
                    }
                }
            }

            return 1;

        case FL_RELEASE:
            _scroll_tracking = false;
            return 1;

        case FL_MOUSEWHEEL:

            take_focus();

            if (Fl::event_dy() > 0) // Lower button
            {
                if (_scroll_index < _record_count - row_count)
                {
                    _scroll_index++;
                    redraw();
                }
            }
            else if (Fl::event_dy() < 0) // Upper button
            {
                if (_scroll_index > 0)
                {
                    _scroll_index--;
                    redraw();
                }
            }
            return 1;

        case FL_KEYDOWN:

            if (Fl::event_key() == FL_Down)
            {
                if (_scroll_index < _record_count - row_count)
                {
                    _scroll_index++;
                    redraw();
                }
                return 1;
            }
            else if (Fl::event_key() == FL_Up)
            {
                if (_scroll_index > 0)
                {
                    _scroll_index--;
                    redraw();
                }
                return 1;
            }
            else if (Fl::event_key() == FL_Page_Down)
            {
                if (_scroll_index
                    < _record_count - row_count) // not finished yet
                {
                    if (row_count > _record_count - _scroll_index - row_count)
                    {
                        _scroll_index = _record_count - row_count;
                    }
                    else
                    {
                        _scroll_index += row_count;
                    }

                    redraw();
                }
                return 1;
            }
            else if (Fl::event_key() == FL_Page_Up)
            {
                if (_scroll_index > 0)
                {
                    if (_scroll_index > row_count)
                    {
                        _scroll_index -= row_count;
                    }
                    else
                    {
                        _scroll_index = 0;
                    }

                    redraw();
                }

                return 1;
            }

            return 0;

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
}

/*****************************************************************************/
