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

#include <math.h>
#include <dirent.h>
#include <fcntl.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
using namespace std;

#include <FL/Fl.H>
#include <FL/fl_draw.H>

/*****************************************************************************/

#include "lib/XmlParser.h"
#include "lib/RingBufferT.h"
#include "lib/IndexT.h"

#include "ViewGlobals.h"
#include "ViewChannel.h"
#include "ViewViewData.h"

/*****************************************************************************/

//#define DEBUG

#define DISTANCE 5                  // Distance from all four sides
#define SCALES_HEIGHT 10            // Height of the labeling of the time scale
#define INFO_HEIGHT 10              // Height of the status bar
#define MIN_CHANNEL_HEIGHT 50       // Minimum height of a channel line
#define CHANNEL_HEADER_HEIGHT 14    // Height of the channel text line
#define MIN_SCROLL_BUTTON_HEIGHT 10 // Minimum height of the scroll button
#define TRACK_BAR_WIDTH 20          // Width of the track bar

#define GEN_COLOR           FL_BLUE   // Blue
#define MIN_MAX_COLOR   0, 200,   0   // Dark green
#define SPACE_COLOR   255, 255, 220   // Light yellow
#define SCAN_COLOR       FL_MAGENTA   // Pink

#define STEP_FACTOR_COUNT 7
const int step_factors[] = {1, 2, 5, 10, 20, 50, 100};

/*****************************************************************************/

/**
   Constructor

   \param x X-Position of the left upper corner
   \param y Y-Position of the left upper corner
   \param w Width of the widget
   \param h Height of the widget
   \param label Title of the widget (FLTK, not used here)
*/

ViewViewData::ViewViewData(int x, int y, int w, int h, const char *label)
    : Fl_Widget(x, y, w, h, label)
{
    _focused = false;
    _zooming = false;
    _moving = false;
    _scanning = false;
    _mouse_in = false;
    _full_range = true;
    _do_not_draw = false;
    _range_cb = 0;

    _track_bar = new Fl_Track_Bar();

    _calc_range();
}

/*****************************************************************************/

/**
   Destructor
*/

ViewViewData::~ViewViewData()
{
    _channels.clear();

    delete _track_bar;
}

/*****************************************************************************/

/**
   Set the callback function for the time range

   This callback function is called whenever the time range of the
   widget changes. This is useful if other widgets should be
   synchronized with it in time.

   \param cb Pointer to the callback function to be called
   \param data Data to be transferred on callback
*/

void ViewViewData::range_callback(void (*cb)(LibDLS::Time,
            LibDLS::Time, void *), void *data)
{
    _range_cb = cb;
    _range_cb_data = data;
}

/*****************************************************************************/

/**
   Add a channel to the display

   The channel is inserted as a line at the bottom of the widget.
   The other channels then become narrower. If the channels were below
   the minimum height, a track bar will be created.

   \param channel Constant pointer to the channel
*/

void ViewViewData::add_channel(LibDLS::Channel *channel)
{
    ViewChannel viewchannel, *p_viewchannel;
    LibDLS::Time old_start, old_end;

    if (has_channel(channel)) return;

    viewchannel.set_channel(channel);
    _channels.push_back(viewchannel);
    p_viewchannel = &_channels.back();
    _channels.sort();

    fl_cursor(FL_CURSOR_WAIT, FL_BLACK, FL_WHITE);
    _do_not_draw = true;
    Fl::check();
    _do_not_draw = false;

    channel->fetch_chunks();

    fl_cursor(FL_CURSOR_DEFAULT);

    old_start = _range_start;
    old_end = _range_end;

    if (_full_range) _calc_range();

    if (_range_start != old_start || _range_end != old_end) {
        _load_data();
    }
    else {
        fl_cursor(FL_CURSOR_WAIT, FL_BLACK, FL_WHITE);
        _do_not_draw = true;
        Fl::check();
        _do_not_draw = false;

        p_viewchannel->fetch_data(_range_start, _range_end, w() - 2 * DISTANCE);

        fl_cursor(FL_CURSOR_DEFAULT);
        redraw();
    }
}

/*****************************************************************************/

/**
   Remove a channel from the display

   If the minimum channel height is not exceed, the other channel lines
   become higher after the removal.

   \param channel Constant pointer to a channel with the same name
*/

void ViewViewData::rem_channel(const LibDLS::Channel *channel)
{
    list<ViewChannel>::iterator channel_i;
    LibDLS::Time old_start, old_end;
    bool found;

    found = false;
    for (channel_i = _channels.begin();
         channel_i != _channels.end();
         channel_i++) {
        if (channel_i->channel() == channel) {
            found = true;
            break;
        }
    }

    if (!found) return;

    _channels.erase(channel_i);

    old_start = _range_start;
    old_end = _range_end;

    if (_full_range) {
        _calc_range();
    }

    if (_range_start != old_start || _range_end != old_end) {
        _load_data();
    }
    else {
        redraw();
    }
}

/*****************************************************************************/

/**
   Remove all channels
*/

void ViewViewData::clear()
{
    _channels.clear();
    redraw();
}

/*****************************************************************************/

/**
   Zoom on the overall data

   First the chunks of all channels are reloaded, then the entire time span
   is calculated and then the data is reloaded for this span.
*/

void ViewViewData::full_range()
{
    list<ViewChannel>::iterator channel_i;

    _full_range = true;

    fl_cursor(FL_CURSOR_WAIT, FL_BLACK, FL_WHITE);
    _do_not_draw = true;
    Fl::check();
    _do_not_draw = false;

    for (channel_i = _channels.begin();
         channel_i != _channels.end();
         channel_i++) {
        channel_i->channel()->fetch_chunks();
    }

    _calc_range();
    _load_data();
}

/*****************************************************************************/

/**
   Load news data for the current time range

   Update all chunks first and then load new data
*/

void ViewViewData::update()
{
    list<ViewChannel>::iterator channel_i;

    fl_cursor(FL_CURSOR_WAIT, FL_BLACK, FL_WHITE);
    _do_not_draw = true;
    Fl::check();
    _do_not_draw = false;

    for (channel_i = _channels.begin();
         channel_i != _channels.end();
         channel_i++) {
        channel_i->channel()->fetch_chunks();
        channel_i->fetch_data(_range_start, _range_end, w() - 2 * DISTANCE);
    }

    redraw();

    if (_range_cb) _range_cb(_range_start, _range_end, _range_cb_data);

    fl_cursor(FL_CURSOR_DEFAULT);
}

/*****************************************************************************/

/**
   Check if a specified channel is already displayed

   \param channel Constant pointer to a channel with the same name
   \return true, if the channel is already displayed
*/

bool ViewViewData::has_channel(const LibDLS::Channel *channel) const
{
    list<ViewChannel>::const_iterator channel_i;

    for (channel_i = _channels.begin();
         channel_i != _channels.end();
         channel_i++) {
        if (channel_i->channel() == channel) return true;
    }

    return false;
}

/*****************************************************************************/

/**
   Calculate the total time span of all channels
*/

void ViewViewData::_calc_range()
{
    list<ViewChannel>::const_iterator channel_i;
    LibDLS::Time start, end;

    // No channel available?
    if (!_channels.size()) {
        _range_start.set_null();
        _range_end.set_null();
        return;
    }

    // Here at least one channel available!
    channel_i = _channels.begin();

    _range_start = channel_i->channel()->start();
    _range_end = channel_i->channel()->end();

    // More channel available?
    while (++channel_i != _channels.end()) {
        start = channel_i->channel()->start();
        end = channel_i->channel()->end();
        if (start.is_null() || end.is_null()) continue;
        if (start < _range_start) _range_start = start;
        if (end > _range_end) _range_end = end;
    }
}

/*****************************************************************************/

/**
   Load all data at the current time
*/

void ViewViewData::_load_data()
{
    list<ViewChannel>::iterator channel_i;

    fl_cursor(FL_CURSOR_WAIT, FL_BLACK, FL_WHITE);
    _do_not_draw = true;
    Fl::check();
    _do_not_draw = false;

    for (channel_i = _channels.begin();
         channel_i != _channels.end();
         channel_i++) {
        channel_i->fetch_data(_range_start, _range_end, w() - 2 * DISTANCE);
    }

    redraw();

    if (_range_cb) _range_cb(_range_start, _range_end, _range_cb_data);

    fl_cursor(FL_CURSOR_DEFAULT);
}

/*****************************************************************************/

/**
   FLTK drawing function
*/

void ViewViewData::draw()
{
    stringstream str;
    list<ViewChannel>::const_iterator channel_i;
    int channel_pos;
    int clip_start, clip_height;
    double x_scale;

    if (_do_not_draw) return;

    // Set font and size
    fl_font(FL_HELVETICA, 10);

    // Drawing background
    draw_box(FL_DOWN_BOX, FL_WHITE);

    // Calculate temporary sizes
    _channel_area_height = h() - SCALES_HEIGHT - INFO_HEIGHT - 2 * DISTANCE - 2;
    if (_channels.size())
        _channel_height = _channel_area_height / _channels.size();
    else _channel_height = 0;
    if (_channel_height < MIN_CHANNEL_HEIGHT) _channel_height = MIN_CHANNEL_HEIGHT;
    _channel_area_width = w() - 2 * DISTANCE;

    // Draw track bar
    _track_bar->content_height(_channels.size() * _channel_height);
    _track_bar->view_height(_channel_area_height);
    _track_bar->draw(x() + w() - DISTANCE - TRACK_BAR_WIDTH,
                     y() + DISTANCE,
                     TRACK_BAR_WIDTH,
                     h() - 2 * DISTANCE);
    _scroll_pos = _track_bar->position();

    // Wenn the track bar is visible, the display width becomes narrower
    if (_track_bar->visible()) _channel_area_width -= TRACK_BAR_WIDTH + 1;

    // Draw boundary lines for the data area
    fl_color(0, 0, 0);
    fl_line(x() + DISTANCE,
            y() + DISTANCE + SCALES_HEIGHT,
            x() + DISTANCE + _channel_area_width,
            y() + DISTANCE + SCALES_HEIGHT);
    fl_line(x() + DISTANCE,
            y() + h() - DISTANCE - INFO_HEIGHT,
            x() + DISTANCE +  _channel_area_width,
            y() + h() - DISTANCE - INFO_HEIGHT);

    // If there are no channels, stop here
    if (_channels.size() == 0) return;

    // Draw general information
    fl_color(0, 0, 0);
    str.str("");
    str.clear();
    str << "Time range from " << _range_start.to_real_time()
        << " to " << _range_end.to_real_time();
    str << " (" << _range_start.diff_str_to(_range_end) << ")";
    if (_range_end <= _range_start) str << "  - INVALID TIME RANGE!";
    fl_draw(str.str().c_str(), x() + DISTANCE, y() + h() - DISTANCE);

    // Cancel if invalid sclare areas
    if (_range_end <= _range_start) return;

    // Determine scaling factor
    x_scale = _channel_area_width / (_range_end - _range_start).to_dbl();

    // Setu up clipping to draw the coerage gaps
    fl_push_clip(x() + DISTANCE,                   // X-Position
                 y() + DISTANCE + SCALES_HEIGHT + 2, // Y-Position
                 _channel_area_width,             // Width
                 _channel_area_height);           // Height

    // Define coverage gaps in color
    for (channel_i = _channels.begin(), channel_pos = 0;
         channel_i != _channels.end();
         channel_i++, channel_pos++) {
        if (channel_pos * _channel_height > _scroll_pos + _channel_area_height
            || (channel_pos + 1) * _channel_height < _scroll_pos) continue;

        _draw_gaps(&(*channel_i),                            // Channel
                   x() + DISTANCE,                            // X-Position
                   y() + DISTANCE + SCALES_HEIGHT - _scroll_pos
                   + channel_pos * _channel_height + 2,  // Y-Position
                   _channel_area_width,                      // Widgth
                   _channel_height,                         // Height
                   x_scale);
    }

    // Remove clipping for coverage gaps
    fl_pop_clip();

    // Draw time scale
    _draw_time_scale(x(), y(), _channel_area_width,
                     h() - INFO_HEIGHT - 2 * DISTANCE, x_scale);

    // Set up clipping for the entire channel area
    fl_push_clip(x() + DISTANCE,                   // X-Position
                 y() + DISTANCE + SCALES_HEIGHT + 2, // Y-Position
                 _channel_area_width,             // Width
                 _channel_area_height - 1);       // Height

    // Draw all channels that are visible
    for (channel_i = _channels.begin(), channel_pos = 0;
         channel_i != _channels.end();
         channel_i++, channel_pos++) {

        if (channel_pos * _channel_height > _scroll_pos + _channel_area_height
            || (channel_pos + 1) * _channel_height < _scroll_pos) continue;

        clip_start = channel_pos * _channel_height - _scroll_pos;
        if (clip_start < 0) clip_start = 0;

        clip_height = _channel_height;
        if ((channel_pos + 1) * _channel_height - 1 - _scroll_pos
            >= _channel_area_height) {
            clip_height = _channel_area_height
                - channel_pos * _channel_height + _scroll_pos;
        }

        // Setup channel clipping
        fl_push_clip(x() + DISTANCE, // X-Position
                     (y() + DISTANCE + SCALES_HEIGHT
                      + 2 + clip_start), // Y-Position
                     _channel_area_width, // Width
                     clip_height - 1); // Height

        // Draw a channel
        _draw_channel(&(*channel_i), // Channel
                      x() + DISTANCE, // X-Position
                      (y() + DISTANCE + SCALES_HEIGHT - _scroll_pos
                       + channel_pos * _channel_height + 2), // Y-Position
                      _channel_area_width, // Width
                      _channel_height, // Height
                      x_scale);

        // Remove channel clipping again
        fl_pop_clip();
    }

    // Remove the entire clipping
    fl_pop_clip();

    // Draw interaction objects (zoom lines, etc.)
    _draw_interactions(x_scale);
}

/*****************************************************************************/

/**
   Draw the gaps of the capture

   \param channel Constant pointer to the channel
   \param left X-Position of the left edge of the character area
   \param top Y-Position of the upper edge of the drawing area
   \param width Width of the drawing area
   \param height Height of the drawing area
*/

void ViewViewData::_draw_gaps(const ViewChannel *channel,
                              int left, int top,
                              int width, int height,
                              double x_scale) const
{
    double xp, old_xp;
    int offset_drawing, height_drawing;
    LibDLS::Channel::ChunkMap::const_iterator chunk_i;
    ViewViewDataChunkRange chunk_range;
    vector<ViewViewDataChunkRange> chunk_ranges, relevant_chunk_ranges;
    vector<ViewViewDataChunkRange>::iterator chunk_range_i;
    LibDLS::Time last_end;

    offset_drawing = top + height - 2;
    height_drawing = height - CHANNEL_HEADER_HEIGHT - 4;

    // Remember chunk time ranges
    for (chunk_i = channel->channel()->chunks().begin();
         chunk_i != channel->channel()->chunks().end();
         chunk_i++) {
        chunk_range.start = chunk_i->second.start();
        chunk_range.end = chunk_i->second.end();
        chunk_ranges.push_back(chunk_range);
    }

    // Sort chunk time ranges by start time
    sort(chunk_ranges.begin(), chunk_ranges.end(), range_before);

    // Check if chunks overlap
    last_end.set_null();
    for (chunk_range_i = chunk_ranges.begin();
         chunk_range_i != chunk_ranges.end();
         chunk_range_i++) {
        if (chunk_range_i->start <= last_end) {
            cerr << "WARNING: Chunks overlapping in channel \""
                 << channel->channel()->name() << "\"!" << endl;
            return;
        }
        last_end = chunk_range_i->end;
    }

    // Copy all chunks that have a share in the time slot
    for (chunk_range_i = chunk_ranges.begin();
         chunk_range_i != chunk_ranges.end();
         chunk_range_i++) {
        if (chunk_range_i->end < _range_start) continue;
        if (chunk_range_i->start > _range_end) break;
        relevant_chunk_ranges.push_back(*chunk_range_i);
    }

    old_xp = -1;
    fl_color(SPACE_COLOR);

    for (chunk_range_i = relevant_chunk_ranges.begin();
         chunk_range_i != relevant_chunk_ranges.end();
         chunk_range_i++) {
        xp = (chunk_range_i->start - _range_start).to_dbl() * x_scale;

        if (xp > old_xp + 1) { // Gap exists?
            fl_rectf(left + (int) (old_xp + 1.5),
                     offset_drawing - height_drawing,
                     (int) (xp - old_xp - 1),
                     height_drawing);
        }

        old_xp = (chunk_range_i->end - _range_start).to_dbl() * x_scale;
    }

    if (width > old_xp + 1) {
        fl_rectf(left + (int) (old_xp + 1.5),
                 offset_drawing - height_drawing,
                 (int) (width - old_xp - 1),
                 height_drawing);
    }
}

/*****************************************************************************/

/**
   Predicate fucntion, used by _draw_gaps in sorting.

   The sort() algorithmus needs a pointer to a predicated function
   that specifies the ordering of two elements.

   \param range1 Constant reference to the first time span
   \param range2 Constant reference to the second time span
   \return true, if first is before the second time span
*/

bool ViewViewData::range_before(const ViewViewDataChunkRange &range1,
                                const ViewViewDataChunkRange &range2)
{
    return range1.start < range2.start;
}

/*****************************************************************************/

/**
   Draw the time scale

   \param left X-Position of the left edge of the characer area
   \param top Y-Position of the upper edge of th drawing area
   \param width Width of the drawing area
   \param height Height of the drawing area
*/

void ViewViewData::_draw_time_scale(unsigned int left, unsigned int top,
                                    unsigned int width, unsigned int height,
                                    double x_scale
                                    ) const
{
    int pot, step_factor_index, last_pixel, xp, text_width, text_height;
    double time_step, step;
    bool scale_fits;
    LibDLS::Time time_start, time_end;
    stringstream str;

    // Determine the power of then of the time span to be displayed
    pot = (int) log10((_range_end - _range_start).to_dbl());
    time_step = pow(10.0, pot - 1); // A power of ten smaller

    // Now, try to find a "reasonable" scale
    step_factor_index = 0;
    scale_fits = false;
    while (!scale_fits && step_factor_index < STEP_FACTOR_COUNT)
    {
        step = time_step * step_factors[step_factor_index];

        last_pixel = 0;
        time_start = ceil(_range_start.to_dbl() / step) * step;
        time_end = floor(_range_end.to_dbl() / step) * step;
        scale_fits = true;

        // Simulate drawing
        for (LibDLS::Time t = time_start; t <= time_end; t += step) {
            xp = (int) ((t - _range_start).to_dbl() * x_scale);

            if (xp <= last_pixel) {
                // Impossible to draw. Try next step!
                step_factor_index++;
                scale_fits = false;
                break;
            }

            // Generate scale label text
            str.str("");
            str.clear();
            str << t.to_real_time();

            // Calculate the dimensions of the scale label
            text_width = 0;
            fl_measure(str.str().c_str(), text_width, text_height);
            last_pixel = xp + text_width + 5;
        }
    }

    if (!scale_fits) return; // Only draw when a meaninful
                             // scale has been found

    for (LibDLS::Time t = time_start; t <= time_end; t += step) {
        xp = (int) ((t - _range_start).to_dbl() * x_scale);

        // Draw scale line
        fl_color(150, 150, 150);
        fl_line(left + xp, top + SCALES_HEIGHT - 5,
                left + xp, top + height);

        // Generate scale label text
        str.str("");
        str.clear();
        str << t.to_real_time();

        // If the scale label still fits in the window
        if (xp + text_width + 2 < (int) width)
        {
            // Draw scale label
            fl_color(0, 0, 0);
            fl_draw(str.str().c_str(), left + xp + 2, top + 12);
        }
    }
}

/*****************************************************************************/

/**
   Draw a channel line

   \param channel Constant pointer to the channel to draw
   \param left X-Position of the left edge of the character area
   \param top Y-Position of the upper edge of the drawing area
   \param width Width of the drawing area
   \param height Height of the drawing area
*/

void ViewViewData::_draw_channel(const ViewChannel *channel,
                                 int left, int top,
                                 int width, int height, double x_scale) const
{
    stringstream str;
    int drawing_top, drawing_height;
    ScanInfo scan;
    double channel_min, channel_max;
    double y_scale = 0.0;

    // Draw box for caption
    fl_color(FL_WHITE);
    fl_rectf(left + 1, top + 1, width - 2, CHANNEL_HEADER_HEIGHT - 2);
    fl_color(FL_BLACK);
    fl_rect(left, top, width, CHANNEL_HEADER_HEIGHT);

    // Draw caption text
    str << channel->channel()->name();
    str << " | ";
    str << channel->min() << channel->channel()->unit();
    str << " to ";
    str << channel->max() << channel->channel()->unit();
    if (channel->max() < channel->min()) str << " (INVALID VALUE RANGE)";

    if (channel->min_level() == channel->max_level()) {
        str << " | level " << channel->min_level();
    }
    else {
        str << " | levels " << channel->min_level();
        str << " - " << channel->max_level();
    }

    fl_draw(str.str().c_str(), left + 5, top + 10);

    // Assume that there is no intersection of the data
    // with any scan line
    scan.found = false;

    drawing_top = top + CHANNEL_HEADER_HEIGHT + 2;
    drawing_height = height - CHANNEL_HEADER_HEIGHT - 4;

    channel_min = channel->min();
    channel_max = channel->max();

    // Only draw channel data if meaning value scale
    if (channel_max >= channel_min) {
        if (channel_min == channel_max) {
            y_scale = 0.0;
        }
        else {
            y_scale = drawing_height / (channel_max - channel_min);
        }

        if (channel->gen_data().size()) {
            _draw_gen(channel, &scan, left, drawing_top,
                      width, drawing_height, x_scale, y_scale,
                      channel_min);
        }
        if (channel->min_data().size() && channel->max_data().size()) {
            _draw_min_max(channel, &scan, left, drawing_top,
                          width, drawing_height, x_scale, y_scale,
                          channel_min);
        }
    }

    // If necessary, draw scan line with data values
    _draw_scan(channel, &scan, left, drawing_top, width, drawing_height,
               x_scale, y_scale, channel_min);
}

/*****************************************************************************/

/**
   Draw generic data values

   \param chunk Constant pointer to the chunk whose data is to be drawn
   \param left X-Position of the left edge of the character area
   \param top Y-Position of the upper edge of the drawing area
   \param width Width of the drawing area
   \param height Height of the drawing area
*/

void ViewViewData::_draw_gen(const ViewChannel *channel,
                             ScanInfo *scan,
                             int left, int top,
                             int width, int height,
                             double x_scale, double y_scale,
                             double channel_min) const
{
    list<LibDLS::Data>::const_iterator data_i;
    double xv, yv, value, old_value = 0.0;
    int xp, yp, old_xp = 0, old_yp = 0, dx, dy;
    bool first_in_chunk = true, first_drawn = true;
    unsigned int i;

    fl_color(GEN_COLOR);

    for (data_i = channel->gen_data().begin();
         data_i != channel->gen_data().end();
         data_i++) {

        for (i = 0; i < data_i->size(); i++) {
            value = data_i->value(i);
            xv = (data_i->time(i) - _range_start).to_dbl() * x_scale;
            yv = (value - channel_min) * y_scale;

            // Round
            if (xv >= 0.0) xp = (int) (xv + 0.5);
            else xp = (int) (xv - 0.5);
            if (yv >= 0.0) yp = (int) (yv + 0.5);
            else yp = (int) (yv - 0.5);

            // Only draw if the current point is
            // within the time window
            if (xp >= 0) {
                if (first_in_chunk) {
                    fl_point(left + xp, top + height - yp);
                }
                else {
                    dx = xp - old_xp;
                    dy = yp - old_yp;

                    // If the current pixel is more than one pixe away
                    // from the last one
                    if ((float) dx * (float) dx
                        + (float) dy * (float) dy > 0) {
                        // Draw line
                        fl_line(left + old_xp, top + height - old_yp,
                                left + xp, top + height - yp);
                    }
                }

                if (xp >= width) break;

                if (_scanning) {
                    if (xp == _scan_x) {
                        // Point falls exactly on the scan line
                        if (scan->found) {
                            if (value < scan->min_value)
                                scan->min_value = value;
                            if (value > scan->max_value)
                                scan->max_value = value;
                        }
                        else {
                            scan->min_value = value;
                            scan->max_value = value;
                            scan->snapped_x = _scan_x;
                            scan->found = true;
                        }
                    }

                    // The scan line is between two points
                    else if (xp > _scan_x && old_xp
                             < _scan_x && !first_drawn) {
                        if (_scan_x - old_xp < xp - _scan_x) {
                            scan->min_value = old_value;
                            scan->max_value = old_value;
                            scan->snapped_x = old_xp;
                        }
                        else {
                            scan->min_value = value;
                            scan->max_value = value;
                            scan->snapped_x = xp;
                        }

                        scan->found = true;
                    }
                }

                first_drawn = false;
            }

            // All following values of the block go beyond
            // the drawing window. Abort.
            if (xp >= width) break;

            old_xp = xp;
            old_yp = yp;
            old_value = value;
            first_in_chunk = false;
        }
    }
}

/*****************************************************************************/

/**
   Draw meta data values

   \param chunk Constant pointer to the chunk to draw
   \param left X-Position of the left edge of the character area
   \param top Y-Position of the upper edge of the drawing area
   \param width Width of the character area
   \param height Height of the drawing area
*/

void ViewViewData::_draw_min_max(const ViewChannel *channel,
                                 ScanInfo *scan,
                                 int left, int top,
                                 int width, int height,
                                 double x_scale, double y_scale,
                                 double channel_min) const
{
    list<LibDLS::Data>::const_iterator data_i;
    double xv, yv, value;
    int xp, yp, i;
    unsigned int j;
    int *min_px, *max_px;

    try {
        min_px = new int[width];
    }
    catch (...) {
        cerr << "ERROR: Failed to allocate drawing memory!" << endl;
        return;
    }

    try {
        max_px = new int[width];
    }
    catch (...) {
        cerr << "ERROR: Failed to allocate drawing memory!" << endl;
        delete [] min_px;
        return;
    }

    for (i = 0; i < width; i++) {
        min_px[i] = -1;
        max_px[i] = -1;
    }

    fl_color(MIN_MAX_COLOR);

    for (data_i = channel->min_data().begin();
         data_i != channel->min_data().end();
         data_i++) {

        for (j = 0; j < data_i->size(); j++) {
            value = data_i->value(j);
            xv = (data_i->time(j) - _range_start).to_dbl() * x_scale;
            yv = (value - channel_min) * y_scale;

            // Round
            if (xv >= 0.0) xp = (int) (xv + 0.5);
            else xp = (int) (xv - 0.5);
            if (yv >= 0.0) yp = (int) (yv + 0.5);
            else yp = (int) (yv - 0.5);

            if (xp >= 0 && xp < width) {
                if (min_px[xp] == -1
                    || (min_px[xp] != -1 && yp < min_px[xp]))
                    min_px[xp] = yp;

                if (_scanning) {
                    if (xp == _scan_x) {
                        if (scan->found) {
                            if (value < scan->min_value)
                                scan->min_value = value;
                            if (value > scan->max_value)
                                scan->max_value = value;
                        }
                        else {
                            scan->min_value = value;
                            scan->max_value = value;
                            scan->snapped_x = _scan_x;
                            scan->found = true;
                        }
                    }
                }
            }

            // All following values of the block go beyond
            // the drawing window. Abort.
            else if (xp >= width) break;
        }
    }

    for (data_i = channel->max_data().begin();
         data_i != channel->max_data().end();
         data_i++) {

        for (j = 0; j < data_i->size(); j++) {
            value = data_i->value(j);
            xv = (data_i->time(j) - _range_start).to_dbl() * x_scale;
            yv = (value - channel_min) * y_scale;

            // Round
            if (xv >= 0.0) xp = (int) (xv + 0.5);
            else xp = (int) (xv - 0.5);
            if (yv >= 0.0) yp = (int) (yv + 0.5);
            else yp = (int) (yv - 0.5);

            if (xp >= 0 && xp < width) {
                if (max_px[xp] == -1
                    || (max_px[xp] != -1 && yp > max_px[xp])) {
                    max_px[xp] = yp;
                }

                if (_scanning) {
                    if (xp == _scan_x) {
                        if (scan->found) {
                            if (value < scan->min_value)
                                scan->min_value = value;
                            if (value > scan->max_value)
                                scan->max_value = value;
                        }
                        else {
                            scan->min_value = value;
                            scan->max_value = value;
                            scan->snapped_x = _scan_x;
                            scan->found = true;
                        }
                    }
                }
            }

            // All following values of the block go beyond
            // the drawing window. Abort.
            else if (xp >= width) break;
        }
    }

    // Draw values
    for (i = 0; i < width; i++) {
        if (min_px[i] != -1 && max_px[i] != -1) {
            fl_line(left + i,
                    top + height - min_px[i],
                    left + i,
                    top + height - max_px[i]);
        }
        else {
            if (min_px[i] != -1) {
                fl_point(left + i, top + height - min_px[i]);
            }
            if (max_px[i] != -1) {
                fl_point(left + i, top + height - max_px[i]);
            }
        }
    }

    delete [] min_px;
    delete [] max_px;
}

/*****************************************************************************/

/**
   Draw the scan line with cut lines and caption

   \param channel Constant pointer to the channel
   \param left X-Position of the left edge of the character area
   \param top Y-Position of the upper edge of the drawing area
   \param width Width of the drawing area
   \param height Height of the drawing area
*/

void ViewViewData::_draw_scan(const ViewChannel *channel, ScanInfo *scan,
                              int left, int top, int width, int height,
                              double x_scale, double y_scale,
                              double channel_min) const
{
    stringstream text_time, text_value;
    int text_time_width, text_value_width;
    int text_time_x, text_value_x;
    int text_height, yp1 = 0, yp2, y_pos, scan_x;
    LibDLS::Time scan_time;

    // If you do not want to scan or
    // if the scan is out of range, cancel
    if (!_scanning || _scan_x < 0 || _scan_x >= width) return;

    if (scan->found) {
        scan_x = scan->snapped_x;
        fl_color(SCAN_COLOR);
    }
    else {
        scan_x = _scan_x;
        fl_color(fl_darker(SCAN_COLOR));
    }

    // Draw a scan line
    fl_line(left + scan_x, top, left + scan_x, top + height);

    // Calculate the time of the scan line
    scan_time = _range_start + LibDLS::Time((scan_x - DISTANCE) / x_scale);

    // Set text widths to 0
    // Also important because fl_measure calculates the text
    // otherwise broken to the width
    text_time_width = 0;
    text_value_width = 0;

    text_time << scan_time.to_real_time();
    fl_measure(text_time.str().c_str(), text_time_width, text_height);

    if (scan->found) {
        text_value << scan->min_value << channel->channel()->unit();

        if (scan->min_value != scan->max_value) {
            text_value << " to " << scan->max_value
                       << channel->channel()->unit();
        }

        fl_measure(text_value.str().c_str(), text_value_width, text_height);
    }

    // if one of the texts no longer fits to the right of the scan line
    if (scan_x + 10 + text_time_width >= left + width
        || scan_x + 10 + text_value_width >= left + width) {
        // Draw all texts on the left side of the scan line
        text_time_x = scan_x - 10 - text_time_width;
        text_value_x = scan_x - 10 - text_value_width;
    }
    else {
        // Draw all texts on the right side of the scan line
        text_time_x = scan_x + 10;
        text_value_x = scan_x + 10;
    }

    if (scan->found) {
        // Show intersections
        yp1 = (int) ((scan->min_value - channel_min) * y_scale + 0.5);
        yp2 = (int) ((scan->max_value - channel_min) * y_scale + 0.5);

        fl_color(SCAN_COLOR);
        fl_line(left, top + height - yp1,
                left + width, top + height - yp1);

        if (yp2 != yp1)
            fl_line(left, top + height - yp2,
                    left + width, top + height - yp2);
    }

    // Draw time text
    fl_color(FL_WHITE);
    fl_rectf(left + text_time_x - 2, top, text_time_width + 4, text_height);
    if (scan->found) fl_color(SCAN_COLOR);
    else fl_color(fl_darker(SCAN_COLOR));
    fl_draw(text_time.str().c_str(), left + text_time_x,
            top + text_height / 2 + fl_descent());
    y_pos = text_height;

    if (scan->found) {
        if (height - yp1 - 5 - text_height < y_pos)
            yp1 = height - y_pos - text_height - 5;

        // Traw text for value(s)
        fl_color(FL_WHITE);
        fl_rectf(left + text_value_x - 2,
                 top + height - yp1 - 5 - text_height,
                 text_value_width + 4,
                 text_height);
        fl_color(SCAN_COLOR);
        fl_draw(text_value.str().c_str(),
                left + text_value_x,
                top + height - yp1 - text_height / 2 - fl_descent());
    }
}

/*****************************************************************************/

/**
   Draw the zoom lines or the shift arrow
*/

void ViewViewData::_draw_interactions(double x_scale) const
{
    stringstream str;
    int text_width, text_height, x_pos;
    LibDLS::Time start_time, end_time;

    // "Zooming line"
    if (_zooming) {
        fl_color(FL_RED);

        fl_line(x() + _start_x, y() + DISTANCE + SCALES_HEIGHT + 1,
                x() + _start_x, y() + h() - DISTANCE - INFO_HEIGHT - 1);
        fl_line(x() + _end_x, y() + DISTANCE + SCALES_HEIGHT + 1,
                x() + _end_x, y() + h() - DISTANCE - INFO_HEIGHT - 1);

        start_time = _range_start.to_dbl() + _start_x / x_scale;

        str.str("");
        str.clear();
        str << start_time.to_real_time();

        text_width = 0;
        fl_measure(str.str().c_str(), text_width, text_height);
        x_pos = _start_x;

        if (x_pos + text_width >= w() - DISTANCE)
            x_pos = w() - DISTANCE - text_width;
        if (x_pos < 0) x_pos = 0;

        fl_color(255, 255, 255);
        fl_rectf(x() + x_pos + 1,
                 y() + DISTANCE + SCALES_HEIGHT + 1,
                 text_width,
                 text_height);
        fl_color(255, 0, 0);
        fl_draw(str.str().c_str(),
                x() + x_pos + 2,
                y() + DISTANCE + SCALES_HEIGHT + 10);

        end_time = _range_start + LibDLS::Time(_end_x / x_scale);

        str.str("");
        str.clear();
        str << end_time.to_real_time();

        text_width = 0;
        fl_measure(str.str().c_str(), text_width, text_height);
        x_pos = _end_x;

        if (x_pos + text_width >= w() - DISTANCE)
            x_pos = w() - DISTANCE - text_width;
        if (x_pos < 0) x_pos = 0;

        fl_color(255, 255, 255);
        fl_rectf(x() + x_pos + 1,
                 y() + DISTANCE + SCALES_HEIGHT + 10,
                 text_width,
                 text_height);
        fl_color(255, 0, 0);
        fl_draw(str.str().c_str(),
                x() + x_pos + 2,
                y() + DISTANCE + SCALES_HEIGHT + 20);

        str.str("");
        str.clear();
        str << start_time.diff_str_to(end_time);

        text_width = 0;
        fl_measure(str.str().c_str(), text_width, text_height);
        x_pos = _end_x;

        if (x_pos + text_width >= w() - DISTANCE)
            x_pos = w() - DISTANCE - text_width;
        if (x_pos < 0) x_pos = 0;

        fl_color(255, 255, 255);
        fl_rectf(x() + x_pos + 1,
                 y() + DISTANCE + SCALES_HEIGHT + 23,
                 text_width,
                 text_height);
        fl_color(255, 0, 0);
        fl_draw(str.str().c_str(),
                x() + x_pos + 2,
                y() + DISTANCE + SCALES_HEIGHT + 33);
    }

    // "Shift arrow"
    else if (_moving) {
        fl_color(150, 150, 0);
        fl_line(x() + _start_x, y() + _start_y, x() + _end_x, y() + _start_y);

        if (_start_x < _end_x)
        {
            fl_line(x() + _end_x, y() + _start_y,
                    x() + _end_x - 5, y() + _start_y - 5);
            fl_line(x() + _end_x, y() + _start_y,
                    x() + _end_x - 5, y() + _start_y + 5);
        }
        else if (_start_x > _end_x)
        {
            fl_line(x() + _end_x, y() + _start_y,
                    x() + _end_x + 5, y() + _start_y - 5);
            fl_line(x() + _end_x, y() + _start_y,
                    x() + _end_x + 5, y() + _start_y + 5);
        }
    }
}

/*****************************************************************************/

/**
   FLTK event function

   \param event FLTK event number
   \return 1, if event edited, otherwise 0
*/

int ViewViewData::handle(int event)
{
    int xp, yp, dx, dw, zoom_out_factor;
    double scale_x;
    LibDLS::Time new_start, new_end, time_diff;
    unsigned int channel_area_width;
    LibDLS::Time time_range;

    // Calculate relative cursor position to the widget
    xp = Fl::event_x() - x();
    yp = Fl::event_y() - y();

    // Is this event for the track bar?
    if (_track_bar->handle(event,
                           xp - w() + TRACK_BAR_WIDTH + DISTANCE,
                           yp - DISTANCE)) {
        redraw();
        return 1;
    }

    // Calculate scaling factor
    channel_area_width = w() - 2 * DISTANCE;
    if (_track_bar->visible()) channel_area_width -= TRACK_BAR_WIDTH + 1;
    scale_x = (_range_end - _range_start).to_dbl() / channel_area_width;

    switch (event) {
        case FL_PUSH:
            take_focus();

            _start_x = xp;
            _start_y = yp;

            if (Fl::event_clicks() == 1) { // Double-click
                Fl::event_clicks(0);

                if (xp > DISTANCE
                    && xp < (int) (channel_area_width + DISTANCE)) {
                    if (Fl::event_state(FL_SHIFT)) zoom_out_factor = 10;
                    else zoom_out_factor = 2;

                    // Zoom out at the clicked time
                    new_start = _range_start + (int64_t) (xp * scale_x);
                    time_range = _range_end - _range_start;
                    time_range = time_range * zoom_out_factor;
                    _range_start = new_start.to_dbl()
                        - time_range.to_dbl() * 0.5;
                    _range_end = new_start.to_dbl()
                        + time_range.to_dbl() * 0.5;
                    _full_range = false;
                    _load_data();
                }
            }

            return 1;

        case FL_DRAG:
            if (Fl::event_state(FL_BUTTON1)) { // Left mouse button
                _zooming = true;
            }
            else if (Fl::event_state(FL_BUTTON3)) { // Right mouse key
                _moving = true;
            }

            _end_x = xp;
            _end_y = yp;

            redraw();

            return 1;

        case FL_RELEASE:
            _end_x = xp;
            _end_y = yp;

            _start_x -= DISTANCE;
            _end_x -= DISTANCE;

            if (_start_x < _end_x) {
                dx = _start_x;
                dw = _end_x - _start_x;
            }
            else {
                dx = _end_x;
                dw = _start_x - _end_x;
            }

            if (_zooming) {
                _zooming = false;

                new_start = _range_start + (int64_t) (dx * scale_x);
                new_end = _range_start + (int64_t) ((dx + dw) * scale_x);

                if (new_start < new_end) {
                    _range_start = new_start;
                    _range_end = new_end;
                    _full_range = false;
                    _load_data();
                }
                else {
                    redraw();
                }
            }

            if (_moving) {
                _moving = false;

                time_diff = (int64_t) ((_end_x - _start_x) * scale_x);

                new_start = _range_start + time_diff;
                new_end = _range_end + time_diff;

                if (new_start < new_end) {
                    _range_start = new_start;
                    _range_end = new_end;
                    _full_range = false;
                    _load_data();
                }
                else {
                    redraw();
                }
            }

            return 1;

        case FL_ENTER:
            _mouse_in = true;

            if (Fl::event_state(FL_CTRL)) {
                _scan_x = xp - DISTANCE;
                _scanning = true;
                redraw();
            }

            return 1;

        case FL_MOVE:
            if (_scanning) {
                _scan_x = xp - DISTANCE;
                redraw();
            }

            return 1;

        case FL_LEAVE:
            _mouse_in = false;

            if (_scanning) {
                _scanning = false;
                redraw();
            }

            return 1;

        case FL_KEYDOWN:
            if (_mouse_in && Fl::event_key() == FL_Control_L) {
                _scan_x = xp - DISTANCE;
                _scanning = true;
                redraw();
                return 1;
            }

            return 0;

        case FL_KEYUP:
            if (Fl::event_key() == FL_Control_L) {
                if (_scanning) {
                    _scanning = false;
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

    return 0;
}

/*****************************************************************************/
