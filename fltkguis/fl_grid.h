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

#ifndef FlGridHpp
#define FlGridHpp

/*****************************************************************************/

#include <string>
#include <list>
using namespace std;

/*****************************************************************************/

#include <FL/Fl_Widget.H>

/*****************************************************************************/

enum Fl_Grid_Event
{
    flgContent,
    flgChecked,
    flgSelect,
    flgDeSelect,
    flgDoubleClick,
    flgCheck
};

enum Fl_Grid_Select_Mode
{
    flgNoSelect,
    flgSingleSelect,
    flgMultiSelect
};

/*****************************************************************************/

/**
   Column to display in a Fl_Grid
*/

class Fl_Grid_Column
{
public:
    Fl_Grid_Column(const string &, const string &, int);
    ~Fl_Grid_Column();

    const string &name() const;
    const string &title() const;
    unsigned int width() const;

private:
    string _name;        /**< Identifier for column name */
    string _title;       /**< Column title */
    unsigned int _width; /**< Proportional column width */

    Fl_Grid_Column();
};

/*****************************************************************************/

/**
   Table object for für FLTK

    Displays a table with data in columns and rows.
    Supports selection of lines, automatic vertical
    scrolling bar and color labeling of cell values.
*/

class Fl_Grid : public Fl_Widget
{
public:
    Fl_Grid(int, int, int, int, const char * = "");
    ~Fl_Grid();

    // column management
    void add_column(const string &, const string & = "", int = 100);

    // content
    void record_count(unsigned int);
    unsigned int record_count() const;
    void clear();

    // Callback
    void callback(void (*)(Fl_Widget *, void *), void *);

    // Appearance
    void row_height(unsigned int);

    // Event handling
    Fl_Grid_Event current_event() const;
    unsigned int current_record() const;
    const string &current_col() const;
    bool current_selected() const;
    void current_content(const string &);
    void current_content_color(Fl_Color);
    void current_checked(bool);

    // Selection
    unsigned int select_count() const;
    unsigned int selected_index() const;
    const list<unsigned int> *selected_list() const;
    void select_mode(Fl_Grid_Select_Mode);
    void select(unsigned int);
    void select_add(unsigned int);
    void deselect(unsigned int);
    void deselect_all();
    bool selected(unsigned int) const;

    // Scrolling
    unsigned int top_index() const;
    void scroll(unsigned int);

    // Checkboxes
    void check_boxes(bool);

protected:
    bool _focused; /**< true, if the grid has currently the window focus */
    list<Fl_Grid_Column> _cols; /**< List of columns to display */
    unsigned int _record_count; /**< Number of current lines */
    void (*_cb)(Fl_Widget *, void *); /**< Pointer to the callback function */
    void *_cb_data; /**< Data passed in a callback */
    string _content; /**< cell content to draw */
    Fl_Color _content_color; /**< Color to draw cell content */
    bool _checked;
    Fl_Grid_Event _event; /**< Type of callback event */
    unsigned int _event_record; /**< Record index of the current event */
    string _event_col; /**< Column identifier of the current event */
    bool _event_sel; /**< true, if the line of the current event is selected */
    Fl_Grid_Select_Mode _select_mode; /**< Selection mode: None, only one
                                         or several lines */
    unsigned int _row_height; /**< Height of all lines in pixels */
    unsigned int _scroll_index; /**< Index of the line displayed at the top */
    list<unsigned int> _selected; /**< Seleted records indexes */
    int _push_x, _push_y; /**< Position of the last mouse click on the grid */
    bool _scroll_tracking; /**< true, if the user is currently
                              dragging the scroll bar */
    int _scroll_grip; /**< Vertical position of the mouse cursor
                         on the scroll bar */
    bool _range_select_possible; /**< true, if a range selection
                                    is currently possible */
    unsigned int _range_select_partner; /**< Start or end index of the
                                           currently possible range
                                           selection */
    bool _check_boxes; /**< true, if a checkbox should appear
                          before each line */

    void _range_select(unsigned int);

    virtual void draw();
    virtual int handle(int);
};

/*****************************************************************************/

/**
   Return the proportionate width of a column

   \return Proportional width
*/

inline unsigned int Fl_Grid_Column::width() const
{
    return _width;
}

/*****************************************************************************/

/**
   Return the title of a column

   \return column title
*/

inline const string &Fl_Grid_Column::title() const
{
    return _title;
}

/*****************************************************************************/

/**
   Return identifier name of a column

   \return Name
*/

inline const string &Fl_Grid_Column::name() const
{
    return _name;
}

/*****************************************************************************/

/**
   Return the event type

   During a callback, returns the type of event that has
   triggered the callback. This can be the following::

   - flgContent:     The callback expects a call to current_content(),
   to get the cell contents
   - flgSelect:      The callback informs that a record has been selected
   selektiert wurde
   - flgDeSelect:    The callback informs that a record is no longer selected
   - flgDoubleClick: The callback informs that a record has received a
   double-click

   \return event type
*/

inline Fl_Grid_Event Fl_Grid::current_event() const
{
    return _event;
}

/*****************************************************************************/

/**
   Specify the index of the record of a callback

   \return Record-Index
*/

inline unsigned int Fl_Grid::current_record() const
{
    return _event_record;
}

/*****************************************************************************/

/**
   Return the column in a callback

   Returns the identifyier name of the column in question
   during a callback with the event type flgContent

   \return Column identifier
*/

inline const string &Fl_Grid::current_col() const
{
    return _event_col;
}

/*****************************************************************************/

/**
   Specifies during a callback with the event type flgContent
   whether the corresponding record is currently selected

   \return Record-Index
*/

inline bool Fl_Grid::current_selected() const
{
    return _event_sel;
}

/*****************************************************************************/

/**
   Return a constant pointer to the list of indexes
   of the selected records.

   \return Record-Index
*/

inline const list<unsigned int> *Fl_Grid::selected_list() const
{
    return &_selected;
}

/*****************************************************************************/

#endif
