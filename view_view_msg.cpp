//---------------------------------------------------------------
//
//  V I E W _ V I E W _ M S G . C P P
//
//---------------------------------------------------------------

#include <iostream>
#include <sstream>
using namespace std;

#include <FL/Fl.h>
#include <FL/fl_draw.h>

//---------------------------------------------------------------

#include "com_xml_parser.hpp"
#include "com_index_t.hpp"
#include "com_file.hpp"
#include "view_globals.hpp"
#include "view_view_msg.hpp"

//#define DEBUG

#define FRAME_WIDTH 3
#define LEVEL_HEIGHT 15
#define TRACK_BAR_WIDTH 20

//---------------------------------------------------------------

const Fl_Color msg_colors[MSG_COUNT] =
{
  FL_BLUE,           // Info
  FL_DARK_YELLOW,    // Warning
  FL_RED,            // Error
  FL_MAGENTA,        // Critical Error
  fl_darker(FL_GRAY) // Broadcast
};

//---------------------------------------------------------------

RCS_ID("$Header: /home/fp/dls/src/RCS/view_view_msg.cpp,v 1.9 2005/02/23 13:22:28 fp Exp $");

//---------------------------------------------------------------

/**
   Konstruktor

   \param x X-Position des linken Randes
   \param y Y-Position des oberen Randes
   \param w Breite des Widgets
   \param h Höhe des Widgets
   \param label Label des Widgets
*/

ViewViewMsg::ViewViewMsg(int x, int y, int w, int h, const char *label)
  : Fl_Widget(x, y, w, h, label)
{
  _focused = false;
  _track_bar = new Fl_Track_Bar();
}

//---------------------------------------------------------------

/**
   Destruktor
*/

ViewViewMsg::~ViewViewMsg()
{
  _messages.clear();
  delete _track_bar;
}

//---------------------------------------------------------------

/**
   Gibt den Auftrag an, zu dem die Anzeige erfolgen soll

   \param dls_dir DLS-Datenverzeichnis
   \param job_id Auftrags-ID
*/

void ViewViewMsg::set_job(const string &dls_dir, unsigned int job_id)
{
  clear();

  _dls_dir = dls_dir;
  _job_id = job_id;
}

//---------------------------------------------------------------

/**
   Löscht alle Nachrichten
*/

void ViewViewMsg::clear()
{
  _messages.clear();
  redraw();
}

//---------------------------------------------------------------

/**
   Lädt Nachrichten im angegebenen Zeitbereich

   \param start Anfangszeit des Bereiches
   \param end Endzeit des Bereiches
*/

void ViewViewMsg::load_msg(COMTime start, COMTime end)
{
  COMIndexT<COMMessageIndexRecord> index;
  COMMessageIndexRecord index_record;
  COMFile file;
  COMRingBufferT<char, unsigned int> ring(10000);
  COMXMLParser xml;
  ViewMSRMessage msg;
  stringstream str;
  char *write_ptr;
  unsigned int i, write_len;
  
  clear();

  _range_start = start;
  _range_end = end;

#ifdef DEBUG
  cout << "loading messages from " << start << " to " << end << endl;
#endif

  str << _dls_dir << "/job" << _job_id << "/messages";

  try
  {
    index.open_read((str.str() + ".idx").c_str());
    file.open_read((str.str() + ".xml").c_str());

    for (i = 0; i < index.record_count(); i++)
    {
      index_record = index[i];

      if (COMTime(index_record.time) < _range_start) continue;
      if (COMTime(index_record.time) > _range_end) break;

      file.seek(index_record.position);
      ring.clear();

      // Solange lesen, bis ein Tag komplett ist
      while (1)
      {
        ring.write_info(&write_ptr, &write_len);
        
        if (!write_len)
        {
          cout << "ERROR: ring buffer full!" << endl;
          return;
        }

        if (write_len > 300) write_len = 300;
        
        file.read(write_ptr, write_len, &write_len);

        if (!write_len)
        {
          cout << "ERROR: EOF in message file!" << endl;
          return;
        }

        ring.written(write_len);

        try
        {
          xml.parse(&ring);
        }
        catch (ECOMXMLParserEOF &e)
        {
          // Noch nicht genug Daten. Mehr einlesen!
          continue;
        }

        break;
      } 

      msg.time = index_record.time;

      try
      {
        msg.text = xml.tag()->att("text")->to_str();
      }
      catch (ECOMXMLTag &e)
      {
        cout << "WARNING: no text attribute in message tag: " << e.msg << " tag: " << e.tag << endl;
        msg.text = "??? (no text in tag!)";
      }

      if (xml.tag()->title() == "info")            msg.type = MSG_INFO;
      else if (xml.tag()->title() == "warning")    msg.type = MSG_WARNING;
      else if (xml.tag()->title() == "error")      msg.type = MSG_ERROR;
      else if (xml.tag()->title() == "crit_error") msg.type = MSG_CRITICAL;
      else if (xml.tag()->title() == "broadcast")  msg.type = MSG_BROADCAST;
      else msg.type = MSG_UNKNOWN;
      
      _messages.push_back(msg);
    }
  }
  catch (ECOMIndexT &e)
  {
    cout << "ERROR in message index: " << e.msg << endl;
    return;
  }
  catch (ECOMFile &e)
  {
    cout << "ERROR in message file: " << e.msg << endl;
    return;
  }
  catch (ECOMXMLParser &e)
  {
    cout << "ERROR while parsing of message file: " << e.msg << endl;
    return;
  }

#ifdef DEBUG
  cout << _messages.size() << " messages loaded." << endl;
#endif

  redraw();
}

//---------------------------------------------------------------

/**
   FLTK-Zeichenfunktion
*/

void ViewViewMsg::draw()
{
  list<ViewMSRMessage>::iterator msg_i;
  int i, text_width, text_height;
  double scale_x;
  int xp, scroll_pos;

  // Schriftart und -größe setzen
  fl_font(FL_HELVETICA, 10);

  // Hintergrund zeichnen
  draw_box(FL_DOWN_BOX, FL_WHITE);

  if (_range_end <= _range_start) return;

  // Skalierung berechnen
  scale_x = (w() - 2 * FRAME_WIDTH) / (_range_end - _range_start).to_dbl();

  // Ebenen berechnen, auf denen die einzelnen Nachrichten gezeichnet werden sollen
  _calc_msg_levels();

  // Scroll-Bar zeichnen
  _track_bar->content_height(_level_count * LEVEL_HEIGHT);
  _track_bar->view_height(h() - 2 * FRAME_WIDTH);
  _track_bar->draw(x() + w() - FRAME_WIDTH - TRACK_BAR_WIDTH,
                   y() + FRAME_WIDTH,
                   TRACK_BAR_WIDTH,
                   h() - 2 * FRAME_WIDTH);

  scroll_pos = _track_bar->position();

  // Clipping einrichten
  if (_track_bar->visible())
  {
    fl_push_clip(x() + FRAME_WIDTH, y() + FRAME_WIDTH,
                 w() - 2 * FRAME_WIDTH - TRACK_BAR_WIDTH - 1, h() - 2 * FRAME_WIDTH);
  }
  else
  {
     fl_push_clip(x() + FRAME_WIDTH, y() + FRAME_WIDTH,
                  w() - 2 * FRAME_WIDTH, h() - 2 * FRAME_WIDTH);
  }

  // Level von unten nach oben zeichnen
  for (i = _level_count - 1; i >= 0; i--)
  {
    msg_i = _messages.begin();
    while (msg_i != _messages.end())
    {
#ifdef DEBUG
      fl_color(FL_GRAY);
      fl_line(x() + FRAME_WIDTH,
              y() + FRAME_WIDTH + i * LINE_HEIGHT,
              x() + h() - 2 * FRAME_WIDTH,
              y() + FRAME_WIDTH + i * LINE_HEIGHT);
#endif

      if (msg_i->level == i)
      {
        xp = (int) ((msg_i->time - _range_start).to_dbl() * scale_x);

        // Text ausmessen
        text_width = 0;
        fl_measure(msg_i->text.c_str(), text_width, text_height, 0);

#ifdef DEBUG
        cout << text_width << " " << text_height << " " << msg_i->text << endl;
#endif

        // Hintergrund hinter dem Text weiss zeichnen
#ifdef DEBUG
        fl_color(FL_RED);
#else
        fl_color(FL_WHITE);
#endif
        fl_rectf(x() + FRAME_WIDTH + xp,
                 y() + FRAME_WIDTH + i * LEVEL_HEIGHT - scroll_pos,
                 text_width + 5,
                 LEVEL_HEIGHT);

        // Zeitlinie einzeichnen
        fl_color(FL_BLACK);
        fl_line(x() + FRAME_WIDTH + xp,
                y() + FRAME_WIDTH - scroll_pos,
                x() + FRAME_WIDTH + xp,
                y() + FRAME_WIDTH + i * LEVEL_HEIGHT + LEVEL_HEIGHT / 2 - scroll_pos);
        fl_line(x() + FRAME_WIDTH + xp,
                y() + FRAME_WIDTH + i * LEVEL_HEIGHT + LEVEL_HEIGHT / 2 - scroll_pos,
                x() + FRAME_WIDTH + xp + 2,
                y() + FRAME_WIDTH + i * LEVEL_HEIGHT + LEVEL_HEIGHT / 2 - scroll_pos);
                 
        // Text zeichnen
        if (msg_i->type >= 0 && msg_i->type < MSG_COUNT) fl_color(msg_colors[msg_i->type]);
        else fl_color(FL_BLACK);
        fl_draw(msg_i->text.c_str(),
                x() + FRAME_WIDTH + xp + 4,
                y() + FRAME_WIDTH + i * LEVEL_HEIGHT + (LEVEL_HEIGHT - text_height) / 2 + text_height - fl_descent() - scroll_pos);
      }
      
      msg_i++;
    }
  }

  // Clipping wieder entfernen
  fl_pop_clip();
}

//---------------------------------------------------------------

/**
   Berechnet für jede Nachricht die Anzeigeebene
*/

void ViewViewMsg::_calc_msg_levels()
{
  double scale_x;
  int level, xp;
  list<ViewMSRMessage>::iterator msg_i;
  list<int> levels;
  list<int>::iterator level_i;
  bool found;

  _level_count = 0;

  if (_range_end <= _range_start) return;
  
  // Skalierung berechnen
  scale_x = (w() - 2 * FRAME_WIDTH) / (_range_end - _range_start).to_dbl();

  msg_i = _messages.begin();
  while (msg_i != _messages.end())
  {
    xp = (int) ((msg_i->time - _range_start).to_dbl() * scale_x);

    // Zeile finden, in der die Nachricht gezeichnet werden kann
    level = 0;
    found = false;
    level_i = levels.begin();
    while (level_i != levels.end())
    {
      if (*level_i < xp) // Nachricht würde in diese Zeile passen
      {
        found = true;
        msg_i->level = level;

        // Endposition in Zeile vermerken
        *level_i = (int) (xp + fl_width(msg_i->text.c_str())) + 5;

        break;
      }

      level_i++;
      level++;
    }
    
    if (!found)
    {
      msg_i->level = level;

      // Alle Zeilen voll. Neue erstellen.
      levels.push_back((int) (xp + fl_width(msg_i->text.c_str())) + 5);
      _level_count++;
    }

    msg_i++;
  }
}

//---------------------------------------------------------------

/**
   FLTK-Ereignisfunktion

   \param event Ereignistyp
   \return 1, wenn Ereignis verarbeitet wurde
*/

int ViewViewMsg::handle(int event)
{
  int xp, yp;

  xp = Fl::event_x() - x();
  yp = Fl::event_y() - y();

  if (_track_bar->handle(event, xp - w() + TRACK_BAR_WIDTH, yp - FRAME_WIDTH))
  {
    redraw();
    return 1;
  }

  switch (event)
  {
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

//---------------------------------------------------------------



