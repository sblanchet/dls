//---------------------------------------------------------------
//
//  U S E R _ V I E W _ D A T A . C P P
//
//---------------------------------------------------------------

#include <math.h>
#include <dirent.h>
#include <fcntl.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
using namespace std;

#include <FL/Fl.h>
#include <FL/fl_draw.h>

//---------------------------------------------------------------

#include "com_xml_parser.hpp"
#include "com_ring_buffer_t.hpp"
#include "view_globals.hpp"
#include "view_view_data.hpp"
#include "view_channel.hpp"
#include "view_chunk.hpp"
#include "com_index_t.hpp"

//---------------------------------------------------------------

#define ABSTAND 5                   // Abstand von allen vier Seiten
#define SKALENHOEHE 10              // Höhe der Beschriftung der Zeitskala
#define INFOHOEHE 10                // Höhe der "Statusleiste"
#define MIN_KANAL_HOEHE 50          // Minimale Höhe einer Kanalzeile
#define KANAL_HEADER_HOEHE 14       // Höhe der Kanal-Textzeile
#define MIN_SCROLL_BUTTON_HEIGHT 10 // Minimale Höhe des Scroll-Buttons
#define SCROLL_BAR_WIDTH 20         // Breite der Scroll-Bar

#define GEN_COLOR       0,   0, 255   // Blau
#define MIN_MAX_COLOR   0, 200,   0   // Dunkelgrün
#define SPACE_COLOR   255, 255, 220   // Hellgelb

#define STEP_FACTOR_COUNT 7
const int step_factors[] = {1, 2, 5, 10, 20, 50, 100};

//---------------------------------------------------------------

ViewViewData::ViewViewData(int x, int y, int w, int h, const char *label)
  : Fl_Widget(x, y, w, h, label)
{
  _focused = false;
  _dragging_line = false;
  _moving = false;
  _scanning = false;
  _mouse_in = false;
  _full_range = true;
  _scroll_pos = 0;
  _scroll_button_tracking = false;
  _do_not_draw = false;

  _calc_range();
}

//---------------------------------------------------------------

ViewViewData::~ViewViewData()
{
  clear();
}

//---------------------------------------------------------------

void ViewViewData::set_job(const string &dls_dir, unsigned int job_id)
{
  clear();

  _dls_dir = dls_dir;
  _job_id = job_id;
}

//---------------------------------------------------------------

void ViewViewData::add_channel(const ViewChannel *channel)
{
  ViewChannel *ch;
  COMTime old_start, old_end;

  fl_cursor(FL_CURSOR_WAIT, FL_BLACK, FL_WHITE);
  _do_not_draw = true;
  Fl::check();
  _do_not_draw = false;

  // Abbrechen, wenn der Kanal bereits in der Liste ist
  if (has_channel(channel)) return;

  // Andernfalls hinzufügen
  _channels.push_back(*channel);
  ch = &_channels.back();

  // Chunk-Informationen laden
  ch->fetch_chunks(_dls_dir, _job_id);

  // Alten Zeitbereich merken
  old_start = _range_start;
  old_end = _range_end;

  // Wenn "alles" angezeigt werden soll
  if (_full_range)
  {
    // Zeitspanne neu berechnen
    _calc_range();
  }

  // Wenn sich die zeitspanne geändert hat
  if (_range_start != old_start || _range_end != old_end)
  {
    // Alle Daten neu laden und neu zeichnen
    _load_data();
  }
  else
  {
    // Sonst nur die Daten des eingefügten Kanals laden und zeichnen
    ch->load_data(_range_start, _range_end, h() - 2 * ABSTAND); // TODO: h() - xx richtig?
    redraw();
  }

  fl_cursor(FL_CURSOR_DEFAULT);

  _scroll_pos = 0;
}

//---------------------------------------------------------------

void ViewViewData::rem_channel(const ViewChannel *channel)
{
  list<ViewChannel>::iterator channel_i = _channels.begin();
  COMTime old_start, old_end;

  fl_cursor(FL_CURSOR_WAIT, FL_BLACK, FL_WHITE);
  _do_not_draw = true;
  Fl::check();
  _do_not_draw = false;
    
  // Alle Kanäle durchlaufen
  while (channel_i != _channels.end())
  {
    // Wenn der aktuelle Kanal der gesuchte ist
    if (channel_i->name() == channel->name())
    {
      // Diesen aus der Liste löschen
      _channels.erase(channel_i);

      // Wenn "alles" angezeigt werden soll
      if (_full_range)
      {
        // Zeitspanne neu berechnen
        _calc_range();
      }

      // Wenn sich die zeitspanne geändert hat
      if (_range_start != old_start || _range_end != old_end)
      {
        // Alle Daten neu laden und neu zeichnen
        _load_data();
      }
      else
      {
        // Sonst nur neu zeichnen
        redraw();
      }

      _scroll_pos = 0;

      fl_cursor(FL_CURSOR_DEFAULT);

      return;
    }

    channel_i++;
  }

  fl_cursor(FL_CURSOR_DEFAULT);
}

//---------------------------------------------------------------

void ViewViewData::clear()
{
  _channels.clear();

  _scroll_pos = 0;

  redraw();
}

//---------------------------------------------------------------

void ViewViewData::full_range()
{
  list<ViewChannel>::iterator channel_i = _channels.begin();

  fl_cursor(FL_CURSOR_WAIT, FL_BLACK, FL_WHITE);
  _do_not_draw = true;
  Fl::check();
  _do_not_draw = false;

  _full_range = true;

  while (channel_i != _channels.end())
  {
    // Bisherige Daten entfernen
    channel_i->clear();

    // Chunks holen
    channel_i->fetch_chunks(_dls_dir, _job_id);

    channel_i++;
  }
  
  // Zeitspanne errechnen
  _calc_range();

  // Alle Daten holen und neu zeichnen
  _load_data();

  fl_cursor(FL_CURSOR_DEFAULT);
}

//---------------------------------------------------------------

void ViewViewData::update()
{
  list<ViewChannel>::iterator channel_i = _channels.begin();

  fl_cursor(FL_CURSOR_WAIT, FL_BLACK, FL_WHITE);
  _do_not_draw = true;
  Fl::check();
  _do_not_draw = false;

  while (channel_i != _channels.end())
  {
    // Bisherige Daten entfernen
    channel_i->clear();

    // Chunks holen
    channel_i->fetch_chunks(_dls_dir, _job_id);

    channel_i++;
  }

  // Daten holen und neu zeichnen
  _load_data();

  fl_cursor(FL_CURSOR_DEFAULT);
}

//---------------------------------------------------------------

bool ViewViewData::has_channel(const ViewChannel *channel) const
{
  list<ViewChannel>::const_iterator channel_i = _channels.begin();

  while (channel_i != _channels.end())
  {
    if (channel_i->name() == channel->name()) return true;
    
    channel_i++;
  }

  return false;
}

//---------------------------------------------------------------

void ViewViewData::_calc_range()
{
  list<ViewChannel>::const_iterator channel_i;

  // Kein Channel vorhanden?
  if (_channels.size() == 0)
  {
    _range_start = (long long) 0;
    _range_end = (long long) 0;
    return;
  }

  // Hier mindestens ein Channel vorhanden!
  channel_i = _channels.begin();

  _range_start = channel_i->start();
  _range_end = channel_i->end();

  // Weitere Channels vorhanden?
  while (++channel_i != _channels.end())
  {
    if (channel_i->start() < _range_start) _range_start = channel_i->start();
    if (channel_i->end() > _range_end) _range_end = channel_i->end();
  }
}

//---------------------------------------------------------------

void ViewViewData::_load_data()
{
  list<ViewChannel>::iterator channel_i;

  channel_i = _channels.begin();
  while (channel_i != _channels.end())
  {
    channel_i->load_data(_range_start, _range_end, w() - 2 * ABSTAND);
    channel_i++;
  }

  redraw();
}

//---------------------------------------------------------------

void ViewViewData::draw()
{
  stringstream str;
  list<ViewChannel>::iterator channel_i;
  int channel_pos, channel_height;
  int channel_area_width, channel_area_height;
  int clip_start, clip_height;

  if (_do_not_draw) return;

  // Schriftart und -größe setzen
  fl_font(FL_HELVETICA, 10);

  // Hintergrund zeichnen
  fl_color(255, 255, 255);
  fl_rectf(x(), y(), w(), h());

  // Höhe für einen Channel berechnen
  channel_area_height = h() - SKALENHOEHE - INFOHOEHE - 2 * ABSTAND - 2;
  if (_channels.size()) channel_height = channel_area_height / _channels.size();
  else channel_height = 0;
  channel_area_width = w() - 2 * ABSTAND;

  // Passen alle Kanäle in den Zeichenbereich?
  if (channel_height < MIN_KANAL_HOEHE)
  {
    channel_height = MIN_KANAL_HOEHE;
    channel_area_width -= SCROLL_BAR_WIDTH;

    _scroll_bar_visible = true;

    // Scroll-Bar zeichnen
    _draw_scroll_bar(x() + w() - SCROLL_BAR_WIDTH, y(), SCROLL_BAR_WIDTH, h());
  }
  else
  {
    _scroll_bar_visible = false;
    _scroll_pos = 0;
  }

  // Begrenzungslinien für den Datenbereich zeichnen
  fl_color(0, 0, 0);
  fl_line(x() + ABSTAND,
          y() + ABSTAND + SKALENHOEHE,
          x() + ABSTAND + channel_area_width,
          y() + ABSTAND + SKALENHOEHE);
  fl_line(x() + ABSTAND,
          y() + h() - ABSTAND - INFOHOEHE,
          x() + ABSTAND +  channel_area_width,
          y() + h() - ABSTAND - INFOHOEHE);

  // Allgemeine Infos zeichnen
  fl_color(0, 0, 0);
  str.str("");
  str.clear();
  str << "Time range from " << _range_start.to_real_time() << " to " << _range_end.to_real_time();
  if (_range_end <= _range_start) str << "  - ILLEGAL RANGE!";
  fl_draw(str.str().c_str(), x() + ABSTAND, y() + h() - ABSTAND);

  // Abbruch, wenn ungültige Skalenbereiche
  if (_range_end <= _range_start) return;

  // Erfassungslücken farblich hinterlegen
  channel_pos = 0;
  channel_i = _channels.begin();
  while (channel_i != _channels.end())
  {
    if (channel_pos * channel_height <= _scroll_pos + channel_area_height
        && (channel_pos + 1) * channel_height >= _scroll_pos)
    {
      _draw_gaps(&(*channel_i),                            // Kanal
                 x() + ABSTAND,                            // X-Position
                 y() + ABSTAND + SKALENHOEHE - _scroll_pos
                 + channel_pos * channel_height + 2,       // Y-Position
                 channel_area_width,                       // Breite
                 channel_height);                          // Höhe
    }

    channel_i++;
    channel_pos++;
  }

  // Zeitskala zeichnen
  _draw_time_scale(x(), y(), channel_area_width, h() - INFOHOEHE - 2 * ABSTAND);

  // Wenn keine Channels vorhanden, hier beenden
  if (_channels.size() == 0) return;

  // Gesamt-Clipping einrichten
  fl_push_clip(x() + ABSTAND,                   // X-Position
               y() + ABSTAND + SKALENHOEHE + 2, // Y-Position
               channel_area_width,              // Breite
               channel_area_height);            // Höhe

  // Alle Kanäle zeichnen, die sichtbar sind
  channel_pos = 0;
  channel_i = _channels.begin();
  while (channel_i != _channels.end())
  {
    if (channel_pos * channel_height <= _scroll_pos + channel_area_height
        && (channel_pos + 1) * channel_height >= _scroll_pos)
    {
      clip_start = channel_pos * channel_height - _scroll_pos;

      if (clip_start < 0)
      {
        clip_start = 0;
      }

      clip_height = channel_height;

      if ((channel_pos + 1) * channel_height - 1 - _scroll_pos >= channel_area_height)
      {
        clip_height = channel_area_height - channel_pos * channel_height + _scroll_pos;
      }

      // Kanal-Clipping einrichten
      fl_push_clip(x() + ABSTAND,                                // X-Position
                   y() + ABSTAND + SKALENHOEHE + 2 + clip_start, // Y-Position
                   channel_area_width,                           // Breite
                   clip_height);                                 // Höhe

      // Kanal zeichnen
      _draw_channel(&(*channel_i),                                                  // Kanal
                    x() + ABSTAND,                                                  // X-Position
                    y() + ABSTAND + SKALENHOEHE - _scroll_pos + channel_pos * channel_height + 2, // Y-Position
                    channel_area_width,                                             // Breite
                    channel_height);                                                // Höhe
                   
      // Kanal-Clipping wieder entfernen
      fl_pop_clip();
    }

    channel_i++;
    channel_pos++;
  }

  // Gesamt-Clipping wieder entfernen
  fl_pop_clip();

  // Interaktionsobjekte (Zoom-Linien etc.) zeichnen
  _draw_interactions();
}

//---------------------------------------------------------------

void ViewViewData::_draw_time_scale(unsigned int left, unsigned int top,
                                    unsigned int width, unsigned int height)
{
  int pot, step_factor_index, last_pixel, xp, text_width, text_height;
  double scale_x, time_step, step;
  bool scale_fits;
  COMTime time_start, time_end;
  stringstream str;

  // Skalierungsfaktor bestimmen
  scale_x = width / (_range_end - _range_start).to_dbl();

  // Zehnerpotenz der anzuzeigenden Zeitspanne bestimmen
  pot = (int) log10((_range_end - _range_start).to_dbl());
  time_step = pow(10.0, pot - 1); // Eine Zehnerpotenz kleiner

  // Jetzt versuchen, eine "hinreichend vernünftige" Skala zu finden
  step_factor_index = 0;
  scale_fits = false;
  while (!scale_fits && step_factor_index < STEP_FACTOR_COUNT)
  {
    step = time_step * step_factors[step_factor_index];

    last_pixel = 0;
    time_start = ceil(_range_start.to_dbl() / step) * step;
    time_end = floor(_range_end.to_dbl() / step) * step;
    scale_fits = true;

    // Zeichnen simulieren
    for (COMTime t = time_start; t <= time_end; t += step)
    {
      xp = (int) ((t - _range_start).to_dbl() * scale_x);

      if (xp <= last_pixel)
      {
        // Zeichnen unmöglich. Nächsten Schritt versuchen!
        step_factor_index++;
        scale_fits = false;
        break;
      }

      // Skalenbeschriftungstext generieren
      str.str("");
      str.clear();
      str << t.to_real_time();
      
      // Abmessungen der Skalenbeschriftung berechnen
      fl_measure(str.str().c_str(), text_width, text_height);
      last_pixel = xp + text_width + 5;
    }
  }

  if (!scale_fits) return; // Nur zeichnen, wenn eine sinnvolle Skala gefunden wurde

  for (COMTime t = time_start; t <= time_end; t += step)
  {
    xp = (int) ((t - _range_start).to_dbl() * scale_x);
      
    // Skalenlinie zeichnen
    fl_color(150, 150, 150);
    fl_line(left + xp, top + SKALENHOEHE - 5,
            left + xp, top + height);
      
    // Skalenbeschriftungstext generieren
    str.str("");
    str.clear();
    str << t.to_real_time();
      
    // Wenn die Skalenbeschriftung noch ins Fenster passt
    if (xp + text_width + 2 < (int) width)
    {
      // Skalenbeschriftung zeichnen
      fl_color(0, 0, 0);
      fl_draw(str.str().c_str(), left + xp + 2, top + 10);
    }
  }
}

//---------------------------------------------------------------

void ViewViewData::_draw_scroll_bar(unsigned int left,
                                    unsigned int top,
                                    unsigned int width,
                                    unsigned int height)
{
  unsigned int content_height, channel_height, channel_area_height;

  if (_channels.size() == 0) return;

  // Höhe für einen Channel berechnen
  channel_area_height = h() - SKALENHOEHE - INFOHOEHE - 2 * ABSTAND - 4;

  if (_channels.size()) channel_height = channel_area_height / _channels.size();
  else channel_height = 0;

  if (channel_height < MIN_KANAL_HOEHE) channel_height = MIN_KANAL_HOEHE;

  content_height = channel_height * _channels.size();

  _scroll_button_height = (unsigned int) (channel_area_height / (double) content_height * (height - 2));

  if (_scroll_button_height < MIN_SCROLL_BUTTON_HEIGHT)
  {
    _scroll_button_height = MIN_SCROLL_BUTTON_HEIGHT;
  }

  _scroll_button_pos = (unsigned int) (_scroll_pos / (double) (content_height - channel_area_height)
                                       * (height - _scroll_button_height - 2));

  fl_color(150, 150, 150);
  fl_rect(left + 1, top + _scroll_button_pos + 1, width - 2, _scroll_button_height);
}

//---------------------------------------------------------------

void ViewViewData::_draw_channel(const ViewChannel *channel,
                                 int left, int top,
                                 int width, int height)
{
  stringstream str;
  list<ViewChunk>::const_iterator chunk_i;

  // Box für Beschriftung zeichnen
  fl_color(FL_WHITE);
  fl_rectf(left + 1, top + 1, width - 2, KANAL_HEADER_HOEHE - 2);
  fl_color(FL_BLACK);
  fl_rect(left, top, width, KANAL_HEADER_HOEHE);

  // Beschriftungstext zeichnen
  str << channel->name();
  str << " - value range: " << channel->min() << " to " << channel->max();
  if (channel->max() <= channel->min()) str << " (ILLEGAL RANGE)";
  str << " - " << channel->blocks_fetched() << " blocks" << endl;

  if (channel->min_level_fetched() == channel->max_level_fetched())
  {
    str << " - level " << channel->min_level_fetched();
  }
  else
  {
    str << " - levels " << channel->min_level_fetched();
    str << " to " << channel->max_level_fetched();
  }

  fl_draw(str.str().c_str(), left + 5, top + 10);

  // Abbrechen, wenn keine sinnvolle Werteskala
  if (channel->max() <= channel->min()) return;

  // Daten zeichnen
  chunk_i = channel->chunks()->begin();
  while (chunk_i != channel->chunks()->end())
  {
    if (chunk_i->current_level() == 0)
    {
      _draw_gen(channel, &(*chunk_i), left, top, width, height);
    }
    else
    {
      _draw_min_max(channel, &(*chunk_i), left, top, width, height);
    }

    chunk_i++;
  }
}

//---------------------------------------------------------------

void ViewViewData::_draw_gen(const ViewChannel *channel,
                             const ViewChunk *chunk,
                             int left, int top,
                             int width, int height)
{
  double scale_x, scale_y;
  double xp, yp, old_xp, old_yp, dx, dy;
  bool first_in_chunk = true;
  list<ViewBlock *>::const_iterator block_i;
  int offset_drawing, height_drawing;

    // Skalierungsfaktor bestimmen
  scale_x = width / (_range_end - _range_start).to_dbl();

  offset_drawing = top + height - 2;
  height_drawing = height - KANAL_HEADER_HOEHE - 4;

  scale_y = height_drawing / (channel->max() - channel->min());

  fl_color(GEN_COLOR);

  // Alle geladenen Blöcke zu diesem Chunk zeichnen
  block_i = chunk->blocks()->begin();
  while (block_i != chunk->blocks()->end())
  {
    // Sind überhaupt Daten im Block?
    if ((*block_i)->size() == 0)
    {
      // Nächsten Block versuchen
      block_i++;
      continue;
    }

    // Werte zeichnen
    for (unsigned int i = 0; i < (*block_i)->size(); i++)
    {
      xp = ((*block_i)->start_time().to_dbl()
            + i * (*block_i)->time_per_value()
            - _range_start.to_dbl()) * scale_x;
      yp = ((*block_i)->value(i) - channel->min()) * scale_y;

      // Nur zeichnen, wenn aktueller Punkt auch innerhalb des Zeitfensters liegt
      if (xp >= 0)
      {
        if (first_in_chunk)
        {
          fl_point(left + (int) (xp + 0.5), offset_drawing - (int) (yp + 0.5));
        }
        else
        {
          dx = xp - old_xp;
          dy = yp - old_yp;
          
          // Wenn der aktuelle Pixel mehr als einen Pixel vom Letzten entfernt liegt
          if (dx * dx + dy * dy > 0)
          {
            // Linie zeichnen
            fl_line(left + (int) (old_xp + 0.5),
                    offset_drawing - (int) (old_yp + 0.5),
                    left + (int) (xp + 0.5),
                    offset_drawing - (int) (yp + 0.5));
          }
        }
      }

      // Alle folgenden Werte des Blockes gehen über das Zeichenfenster hinaus. Abbrechen.
      if ((int) (xp + 0.5) >= width) break;

      old_xp = xp;
      old_yp = yp;
      first_in_chunk = false;
    }

    block_i++;
  }
}

//---------------------------------------------------------------

void ViewViewData::_draw_min_max(const ViewChannel *channel,
                                 const ViewChunk *chunk,
                                 int left, int top,
                                 int width, int height)
{
  double scale_x, scale_y;
  int xp, yp;
  list<ViewBlock *>::const_iterator block_i;
  int offset_drawing, height_drawing;
  int *min_values, *max_values;

  try
  {
    min_values = new int[width];
  }
  catch (...)
  {
    cout << "ERROR: could not allocate drawing memory!" << endl;
    return;
  }

  try
  {
    max_values = new int[width];
  }
  catch (...)
  {
    cout << "ERROR: could not allocate drawing memory!" << endl;
    delete [] min_values;
    return;
  }

  for (int i = 0; i < width; i++)
  {
    min_values[i] = -1;
    max_values[i] = -1;
  }

    // Skalierungsfaktor bestimmen
  scale_x = width / (_range_end - _range_start).to_dbl();

  offset_drawing = top + height - 2;
  height_drawing = height - KANAL_HEADER_HOEHE - 4;

  scale_y = height_drawing / (channel->max() - channel->min());

  fl_color(MIN_MAX_COLOR);

  block_i = chunk->min_blocks()->begin();
  while (block_i != chunk->min_blocks()->end())
  {
    for (unsigned int i = 0; i < (*block_i)->size(); i++)
    {
      xp = (int) (((*block_i)->start_time().to_dbl()
                   + i * (*block_i)->time_per_value()
                   - _range_start.to_dbl()) * scale_x);
      yp = (int) (((*block_i)->value(i) - channel->min()) * scale_y);

      if (xp >= 0 && xp < width)
      {
        if (min_values[xp] == -1 || (min_values[xp] != -1 && yp < min_values[xp]))
        {
          min_values[xp] = yp;
        }
      }

      // Alle folgenden Werte des Blockes gehen über das Zeichenfenster hinaus. Abbrechen.
      else if ((int) (xp + 0.5) >= width) break;
    }

    block_i++;
  }

  block_i = chunk->max_blocks()->begin();
  while (block_i != chunk->max_blocks()->end())
  {
    for (unsigned int i = 0; i < (*block_i)->size(); i++)
    {
      xp = (int) (((*block_i)->start_time().to_dbl()
                   + i * (*block_i)->time_per_value()
                   - _range_start.to_dbl()) * scale_x);
      yp = (int) (((*block_i)->value(i) - channel->min()) * scale_y);

      if (xp >= 0 && xp < width)
      {
        if (max_values[xp] == -1 || (max_values[xp] != -1 && yp > max_values[xp]))
        {
          max_values[xp] = yp;
        }
      }

      // Alle folgenden Werte des Blockes gehen über das Zeichenfenster hinaus. Abbrechen.
      else if ((int) (xp + 0.5) >= width) break;
    }

    block_i++;
  }

  // Werte zeichnen
  for (int i = 0; i < width; i++)
  {
    if (min_values[i] != -1 && max_values[i] != -1)
    {
      fl_line(left + i,
              offset_drawing - min_values[i],
              left + i,
              offset_drawing - max_values[i]);
    }
    else
    {
      if (min_values[i] != -1)
      {
        fl_point(left + i, offset_drawing - min_values[i]);
      }
      if (max_values[i] != -1)
      {
        fl_point(left + i, offset_drawing - max_values[i]);
      }
    }
  }

  delete [] min_values;
  delete [] max_values;
}

//---------------------------------------------------------------

void ViewViewData::_draw_gaps(const ViewChannel *channel,
                              int left, int top,
                              int width, int height)
{
  double scale_x;
  double xp, old_xp;
  int offset_drawing, height_drawing;
  list<ViewChunk>::const_iterator chunk_i;
  ViewViewDataChunkRange chunk_range;
  vector<ViewViewDataChunkRange> chunk_ranges, relevant_chunk_ranges;
  vector<ViewViewDataChunkRange>::iterator chunk_range_i;
  COMTime last_end;

  // Skalierungsfaktor bestimmen
  scale_x = width / (_range_end - _range_start).to_dbl();

  offset_drawing = top + height - 2;
  height_drawing = height - KANAL_HEADER_HOEHE - 4;

  // Chunk-Zeitbereiche merken
  chunk_i = channel->chunks()->begin();
  while (chunk_i != channel->chunks()->end())
  {
    chunk_range.start = chunk_i->start();
    chunk_range.end = chunk_i->end();
    chunk_ranges.push_back(chunk_range);

    chunk_i++;
  }

  // Chunk-Zeitbereiche nach Anfangszeit sortieren
  sort(chunk_ranges.begin(), chunk_ranges.end(), range_before);

  // Prüfen, ob Chunks überlappen
  last_end = (long long) 0;
  chunk_range_i = chunk_ranges.begin();
  while (chunk_range_i != chunk_ranges.end())
  {
    if (chunk_range_i->start <= last_end)
    {
      cout << "WARNING: chunks overlapping in channel \"" << channel->name() << "\"!" << endl;
      cout << "cannot draw gaps!" << endl;
      return;
    }

    last_end = chunk_range_i->end;
    chunk_range_i++;
  }

  // Alle Chunks kopieren, die Anteil am Zeitfenster haben
  chunk_range_i = chunk_ranges.begin();
  while (chunk_range_i != chunk_ranges.end())
  {
    if (chunk_range_i->end >= _range_start)
    {
      if (chunk_range_i->start > _range_end) break;
      relevant_chunk_ranges.push_back(*chunk_range_i);
    }
    chunk_range_i++;
  }

  old_xp = -1;
  fl_color(SPACE_COLOR);

  chunk_range_i = relevant_chunk_ranges.begin();
  while (chunk_range_i != relevant_chunk_ranges.end())
  {
    xp = (chunk_range_i->start.to_dbl() - _range_start.to_dbl()) * scale_x;

    if (xp > old_xp + 1) // Lücke vorhanden?
    {
      fl_rectf(left + (int) (old_xp + 1.5),
               offset_drawing - height_drawing,
               (int) (xp - old_xp - 1),
               height_drawing);
    }

    old_xp = (chunk_range_i->end.to_dbl() - _range_start.to_dbl()) * scale_x;

    chunk_range_i++;
  }

  if (width > old_xp + 1)
  {
    fl_rectf(left + (int) (old_xp + 1.5),
             offset_drawing - height_drawing,
             (int) (width - old_xp - 1),
             height_drawing);
  }
}

//---------------------------------------------------------------

bool range_before(const ViewViewDataChunkRange &range1,
                  const ViewViewDataChunkRange &range2)
{
  return range1.start < range2.start;
}

//---------------------------------------------------------------

bool range_outside(const ViewViewDataChunkRange &range1,
                  const ViewViewDataChunkRange &range2)
{
  return range1.start < range2.start;
}

//---------------------------------------------------------------

void ViewViewData::_draw_interactions()
{
  stringstream str;
  double scale_x;
  int text_width, text_height, x_pos;
  COMTime time;

  // Skalierungsfaktor bestimmen
  scale_x = (w() - 2 * ABSTAND) / (_range_end - _range_start).to_dbl();

  // "Zooming-Linie"
  if (_dragging_line)
  {
    fl_color(FL_RED);

    fl_line(x() + _start_x, y() + ABSTAND + SKALENHOEHE + 1,
            x() + _start_x, y() + h() - ABSTAND - INFOHOEHE - 1);
    fl_line(x() + _end_x, y() + ABSTAND + SKALENHOEHE + 1,
            x() + _end_x, y() + h() - ABSTAND - INFOHOEHE - 1);

    time = _range_start.to_dbl() + _start_x / scale_x;

    str.str("");
    str.clear();
    str << time.to_real_time();
      
    text_width = 0;
    fl_measure(str.str().c_str(), text_width, text_height);
    x_pos = _start_x;

    if (x_pos + text_width >= w() - ABSTAND) x_pos = w() - ABSTAND - text_width;
    if (x_pos < 0) x_pos = 0;

    fl_color(255, 255, 255);
    fl_rectf(x() + x_pos + 1, y() + ABSTAND + SKALENHOEHE + 1, text_width, text_height);
    fl_color(255, 0, 0);
    fl_draw(str.str().c_str(), x() + x_pos + 2, y() + ABSTAND + SKALENHOEHE + 10);

    time = _range_start.to_dbl() + _end_x / scale_x;

    str.str("");
    str.clear();
    str << time.to_real_time();
      
    text_width = 0;
    fl_measure(str.str().c_str(), text_width, text_height);
    x_pos = _end_x;

    if (x_pos + text_width >= w() - ABSTAND) x_pos = w() - ABSTAND - text_width;
    if (x_pos < 0) x_pos = 0;

    fl_color(255, 255, 255);
    fl_rectf(x() + x_pos + 1, y() + ABSTAND + SKALENHOEHE + 10, text_width, text_height);
    fl_color(255, 0, 0);
    fl_draw(str.str().c_str(), x() + x_pos + 2, y() + ABSTAND + SKALENHOEHE + 20);
  }

  // "Verschiebungs-Pfeil"
  else if (_moving)
  {
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

  // Vertikale Scanlinie
  else if (_scanning)
  {
    fl_color(FL_RED);

    if (_start_x > ABSTAND && _start_x < w() - ABSTAND)
    {
      fl_line(x() + _start_x, y() + ABSTAND + SKALENHOEHE + 1,
              x() + _start_x, y() + h() - ABSTAND - INFOHOEHE - 1);

      time = _range_start.to_dbl() + _start_x / scale_x;

      str.str("");
      str.clear();
      str << time.to_real_time();
      
      text_width = 0;
      fl_measure(str.str().c_str(), text_width, text_height);
      x_pos = _start_x;

      if (x_pos + text_width >= w() - ABSTAND) x_pos = w() - ABSTAND - text_width;
      if (x_pos < 0) x_pos = 0;

      fl_color(255, 255, 255);
      fl_rectf(x() + x_pos + 1, y() + ABSTAND + SKALENHOEHE + 1, text_width, text_height);
      fl_color(255, 0, 0);
      fl_draw(str.str().c_str(), x() + x_pos + 2, y() + ABSTAND + SKALENHOEHE + 10);
    }
  }
}

//---------------------------------------------------------------

int ViewViewData::handle(int event)
{
  int xp, yp, dx, dy, dw, dh, zoom_out_factor;
  double scale_x;
  COMTime new_start, new_end, time_diff;
  unsigned int channel_area_height, channel_height, channel_area_width, content_height;
  COMTime time_range;

  xp = Fl::event_x() - x();
  yp = Fl::event_y() - y();

  // Höhe für einen Channel berechnen
  channel_area_height = h() - SKALENHOEHE - INFOHOEHE - 2 * ABSTAND - 4;
  if (_channels.size()) channel_height = channel_area_height / _channels.size();
  else channel_height = 0;
  channel_area_width = w() - 2 * ABSTAND;

  // Passen alle Kanäle in den Zeichenbereich?
  if (channel_height < MIN_KANAL_HOEHE)
  {
    channel_height = MIN_KANAL_HOEHE;
    channel_area_width -= SCROLL_BAR_WIDTH;
    content_height = channel_height * _channels.size();
  }

  // Skalierungsfaktor berechnen
  scale_x = (_range_end - _range_start).to_dbl() / channel_area_width;

  switch (event)
  {
    case FL_PUSH:

      take_focus();

      _start_x = xp;
      _start_y = yp;

      if (Fl::event_clicks() == 1) // Doubleclick
      {
        Fl::event_clicks(0);

        if (xp > ABSTAND && xp < (int) (channel_area_width + ABSTAND))
        {
          if (Fl::event_state(FL_SHIFT)) zoom_out_factor = 10;
          else zoom_out_factor = 2;

          // Herauszoomen um den geklickten Zeitpunkt
          new_start = _range_start + (long long) (xp * scale_x);
          time_range = _range_end - _range_start;
          time_range = time_range * zoom_out_factor;
          _range_start = new_start.to_dbl() - time_range.to_dbl() * 0.5;
          _range_end = new_start.to_dbl() + time_range.to_dbl() * 0.5;
          _full_range = false;

          fl_cursor(FL_CURSOR_WAIT, FL_BLACK, FL_WHITE);
          _do_not_draw = true;
          Fl::check();
          _do_not_draw = false;

          _load_data();

          fl_cursor(FL_CURSOR_DEFAULT, FL_BLACK, FL_WHITE);
        }
      }
        
      return 1;

    case FL_DRAG:

      if (_scroll_button_tracking)
      {
        int new_scroll_pos = yp - _scroll_grip;
        
        if (new_scroll_pos < 0)
        {
          new_scroll_pos = 0;
        }
        else if (new_scroll_pos > h() - _scroll_button_height - 1)
        {
          new_scroll_pos = h() - _scroll_button_height - 1;
        }
        
        _scroll_pos = (int) (new_scroll_pos / (double) (h() - _scroll_button_height - 2)
                             * (content_height - channel_area_height));
        
        redraw();
      }
      else
      {
        if (Fl::event_state(FL_BUTTON1)) // Linke Maustaste
        {
          // Auf die Scrollbar geklickt
          if (_scroll_bar_visible &&
              _start_x >= w() - SCROLL_BAR_WIDTH + 1 &&
              _start_x <= w() - 1 &&
              _start_y >= _scroll_button_pos + 1 &&
              _start_y <= _scroll_button_pos + _scroll_button_height + 1)
          {    
            _scroll_grip = _start_y - _scroll_button_pos - 1;
            _scroll_button_tracking = true;
          }
          else // Nicht auf die Scrollbar
          {
            _dragging_line = true;
          }
        }
        else if (Fl::event_state(FL_BUTTON3)) // Rechte Maustaste
        {
          _moving = true;
        }
      }

      _end_x = xp;
      _end_y = yp;

      redraw();

      return 1;

    case FL_RELEASE:

      if (_scroll_button_tracking)
      {
        _scroll_button_tracking = false;
        return 1;
      }

      _end_x = xp;
      _end_y = yp;

      if (_start_x < _end_x)
      {
        dx = _start_x;
        dw = _end_x - _start_x;
      }
      else
      {
        dx = _end_x;
        dw = _start_x - _end_x;
      }

      if (_start_y < _end_y)
      {
        dy = _start_y;
        dh = _end_y - _start_y;
      }
      else
      {
        dy = _end_y;
        dh = _start_y - _end_y;
      }

      if (_dragging_line)
      {
        _dragging_line = false;

        new_start = _range_start + (long long) (dx * scale_x);
        new_end = _range_start + (long long) ((dx + dw) * scale_x);

        if (new_start < new_end)
        {
          _range_start = new_start;
          _range_end = new_end;
          _full_range = false;
          
          fl_cursor(FL_CURSOR_WAIT, FL_BLACK, FL_WHITE);
          _do_not_draw = true;
          Fl::check();
          _do_not_draw = false;

          _load_data();

          fl_cursor(FL_CURSOR_DEFAULT, FL_BLACK, FL_WHITE);
        }
        else
        {
          redraw();
        }
      }

      if (_moving)
      {
        _moving = false;
      
        time_diff = (long long) ((_end_x - _start_x) * scale_x);

        new_start = _range_start + time_diff;
        new_end = _range_end + time_diff;

        if (new_start < new_end)
        {
          _range_start = new_start;
          _range_end = new_end;
          _full_range = false;

          fl_cursor(FL_CURSOR_WAIT, FL_BLACK, FL_WHITE);
          _do_not_draw = true;
          Fl::check();
          _do_not_draw = false;

          _load_data();

          fl_cursor(FL_CURSOR_DEFAULT, FL_BLACK, FL_WHITE);
        }
        else
        {
          redraw();
        }
      }

      return 1;

    case FL_ENTER:

      _mouse_in = true;

      if (Fl::event_state(FL_CTRL))
      {
        _start_x = xp;
        _start_y = yp;
        _scanning = true;
        redraw();
      }

      return 1;

    case FL_MOVE:

      if (_scanning)
      {
        _start_x = xp;
        _start_y = yp;
        redraw();
      }

      return 1;

    case FL_LEAVE:

      _mouse_in = false;

      if (_scanning)
      {
        _scanning = false;
        redraw();
      }

      return 1;

    case FL_KEYDOWN:

      if (_mouse_in && Fl::event_key() == FL_Control_L)
      {
        _start_x = xp;
        _start_y = yp;
        _scanning = true;
        redraw();

        return 1;
      }

      return 0;

    case FL_KEYUP:

      if (Fl::event_key() == FL_Control_L)
      {
        if (_scanning)
        {
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

//---------------------------------------------------------------



