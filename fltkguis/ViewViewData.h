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

#ifndef ViewViewDataHpp
#define ViewViewDataHpp

/*****************************************************************************/

#include <list>
using namespace std;

#include <FL/Fl_Widget.H>

/*****************************************************************************/

#include "lib/LibDLS/Time.h"
#include "lib/LibDLS/Job.h"

#include "fl_track_bar.h"
#include "ViewChannel.h"

/*****************************************************************************/

/**
   Area of a chunk for displaying the unrecorded areas
*/

struct ViewViewDataChunkRange
{
    LibDLS::Time start;
    LibDLS::Time end;
};

/*****************************************************************************/

/**
 */

struct ScanInfo
{
    bool found;
    double min_value;
    double max_value;
    int snapped_x;
};

/*****************************************************************************/

/**
   Display for measured data

   This display can display the data of several channels on top of
   each other. These are distributed to the full widget height.
   If the individual channels become too narrow, a minimum height
   will be used for each channel and a track bar  will be displayed.
   The widget supports mouse and keyboard interactions.
*/

class ViewViewData : public Fl_Widget
{
public:
    ViewViewData(int, int, int, int, const char * = "");
    ~ViewViewData();

    void add_channel(LibDLS::Channel *);
    void rem_channel(const LibDLS::Channel *);
    bool has_channel(const LibDLS::Channel *) const;
    void clear();
    void full_range();
    void update();
    void range_callback(void (*)(LibDLS::Time, LibDLS::Time, void *), void *);

    LibDLS::Time start() const;
    LibDLS::Time end() const;

private:
    Fl_Track_Bar *_track_bar;    /**< Vertical track bar */
    list<ViewChannel> _channels; /**< channels list to show */
    LibDLS::Time _range_start;   /**< Start time of the time span to display */
    LibDLS::Time _range_end;     /**< End time of the time span to display */
    bool _full_range;            /**< Indicate whether adding a channel
                                    should extend the time span to the
                                    full amount of time. */

    // Callbacks
    void (*_range_cb)(LibDLS::Time, LibDLS::Time, void *); /**<
                                                             Function pointer
                                                             for time domain
                                                             changes */
    void *_range_cb_data;                        /**< Data for transfer at
                                                    Range-CB */

    // Widget condition
    bool _focused;       /**< The widget currenlty holds the focus */
    bool _zooming;       /**< The user picks up a zoom area */
    bool _moving;        /**< The user pulls a shift arrow */
    bool _scanning;      /**< The user displays the scan line */
    int _scan_x;         /**< X-Position of the scan line */
    int _start_x;        /**< cursor X-Position at the action beginning */
    int _start_y;        /**< cursor Y-Position at the action beginning */
    int _end_x;          /**< cursor X-Position at the action ending */
    int _end_y;          /**< cursor Y-Position at the action ending */
    bool _mouse_in;      /**< The mouse cursor is on the widget */
    bool _do_not_draw;   /**< Flag: the content should not be redraw */

    // Temporary size for drawing
    int _channel_area_width;  /**< Width of the character area
                                 for the channels */
    int _channel_area_height; /**< Height of the character area
                                 for the channels */
    int _channel_height;      /**< Height of a channel line, incl. Textbox */
    int _scroll_pos;          /**< Display offset (depending on track bar) */

    // Private methods
    void _load_data();
    void _calc_range();
    void _draw_gaps(const ViewChannel *, int, int, int, int, double) const;
    void _draw_time_scale(unsigned int, unsigned int,
                          unsigned int, unsigned int, double) const;
    void _draw_scroll_bar(unsigned int, unsigned int,
                          unsigned int, unsigned int) const;
    void _draw_channel(const ViewChannel *, int, int, int, int, double) const;
    void _draw_gen(const ViewChannel *, ScanInfo *, int, int, int, int,
                   double, double, double) const;
    void _draw_min_max(const ViewChannel *, ScanInfo *,
                       int, int, int, int, double, double, double) const;
    void _draw_interactions(double) const;
    void _draw_scan(const ViewChannel *, ScanInfo *, int, int, int, int,
                    double, double, double) const;

    // Predicate function for sorting
    static bool range_before(const ViewViewDataChunkRange &,
                             const ViewViewDataChunkRange &);

    // FLTK
    virtual void draw();
    virtual int handle(int);
};

/*****************************************************************************/

inline LibDLS::Time ViewViewData::start() const
{
    return _range_start;
}

/*****************************************************************************/

inline LibDLS::Time ViewViewData::end() const
{
    return _range_end;
}

/*****************************************************************************/

#endif
