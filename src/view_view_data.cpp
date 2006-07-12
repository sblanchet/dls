/******************************************************************************
 *
 *  $Id$
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
using namespace std;

#include <FL/Fl.h>
#include <FL/fl_draw.h>

/*****************************************************************************/

#include "com_xml_parser.hpp"
#include "com_ring_buffer_t.hpp"
#include "com_index_t.hpp"
#include "view_globals.hpp"
#include "view_channel.hpp"
#include "view_chunk.hpp"
#include "view_view_data.hpp"

/*****************************************************************************/

//#define DEBUG

#define ABSTAND 5                   // Abstand von allen vier Seiten
#define SKALENHOEHE 10              // Höhe der Beschriftung der Zeitskala
#define INFOHOEHE 10                // Höhe der "Statusleiste"
#define MIN_KANAL_HOEHE 50          // Minimale Höhe einer Kanalzeile
#define KANAL_HEADER_HOEHE 14       // Höhe der Kanal-Textzeile
#define MIN_SCROLL_BUTTON_HEIGHT 10 // Minimale Höhe des Scroll-Buttons
#define TRACK_BAR_WIDTH 20          // Breite der Track-Bar

#define GEN_COLOR           FL_BLUE   // Blau
#define MIN_MAX_COLOR   0, 200,   0   // Dunkelgrün
#define SPACE_COLOR   255, 255, 220   // Hellgelb
#define SCAN_COLOR       FL_MAGENTA   // Pink

#define STEP_FACTOR_COUNT 7
const int step_factors[] = {1, 2, 5, 10, 20, 50, 100};

/*****************************************************************************/

/**
   Konstruktor

   \param x X-Position der linken, oberen Ecke
   \param y Y-Position der linken, oberen Ecke
   \param w Breite des Widgets
   \param h Höhe des Widgets
   \param label Titel des Widgets (FLTK, hier nicht verwendet)
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
   Destruktor
*/

ViewViewData::~ViewViewData()
{
    _channels.clear();

    delete _track_bar;
}

/*****************************************************************************/

/**
   Setzt das DLS-Datenverzeichnis und die Auftrags-ID

   Das Widget arbeitet immer in einem bestimmten
   Auftragsverzeichnis, das durch diese beiden Angaben
   eindeutig identifiziert wird.

   \param dls_dir Datenverzeichnis
   \param job_id Auftrags-ID
*/

void ViewViewData::set_job(const string &dls_dir, unsigned int job_id)
{
    clear();

    _dls_dir = dls_dir;
    _job_id = job_id;
}

/*****************************************************************************/

/**
   Setzt die Callback-Funktion für den Zeitbereich

   Diese Callback-Funktion wird immer aufgerufen, wenn sich der
   Zeitbereich des Widgets ändert. Das ist sinnvoll, wenn andere
   Widgets zeitlich mit diesem synchronisiert werden sollen.

   \param cb Zeiger auf die aufzurufende Callback-Funktion
   \param data Beim Callback zu übergebende Daten
*/

void ViewViewData::range_callback(void (*cb)(COMTime, COMTime, void *),
                                  void *data)
{
    _range_cb = cb;
    _range_cb_data = data;
}

/*****************************************************************************/

/**
   Fügt einen Kanal zur Anzeige hinzu

   Der Kanal wird als Zeile unten in das Widget eingefügt.
   Die anderen Kanäle werden dann schmaler. Wenn die Kanäle die
   Mindesthöhe unterschreiten würden, wird eine Track-Bar
   angezeigt.

   \param channel Konstanter Zeiger auf den Kanal
*/

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

    // Andernfalls kopieren und anfügen
    // Achtung: Kopieren ohne Copy-Konstruktor. Geht nur, weil
    // noch keine Chunks geladen wurden. Das passiert später.
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
        ch->load_data(_range_start, _range_end,
                      h() - 2 * ABSTAND); // TODO: h() - xx richtig?
        redraw();
    }

    fl_cursor(FL_CURSOR_DEFAULT);
}

/*****************************************************************************/

/**
   Entfernt einen Kanal aus der Anzeige

   Wenn die Mindest-Kanalhöhe nicht noch unterschritten wird,
   werden die anderen Kanalzeilen nach dem Entfernen höher.

   \param channel Konstanter Zeiger auf einen Kanal, der den
   selben Namen trägt
*/

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

            // Die "alte" Zeitspanne merken
            old_start = _range_start;
            old_end = _range_end;

            // Wenn "alles" angezeigt werden soll
            if (_full_range)
            {
                // Zeitspanne neu berechnen
                _calc_range();
            }

            // Wenn sich die Zeitspanne geändert hat
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

            fl_cursor(FL_CURSOR_DEFAULT);

            return;
        }

        channel_i++;
    }

    fl_cursor(FL_CURSOR_DEFAULT);
    _do_not_draw = true;
    Fl::check();
    _do_not_draw = false;
}

/*****************************************************************************/

/**
   Entfernt alle Kanäle
*/

void ViewViewData::clear()
{
    _channels.clear();
    redraw();
}

/*****************************************************************************/

/**
   Zoomt auf die Gesamtdaten

   Erst werden die Chunks aller Kanäle neu geladen, dann wird
   die gesamte Zeitspanne berechnet und dann werden für diese
   Spanne die Daten neu geladen.
*/

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

/*****************************************************************************/

/**
   Lädt neue Daten für den aktuellen Zeitbereich

   Aktualisiert erst alle Chunks und lädt dann neue Daten
*/

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

/*****************************************************************************/

/**
   Überprüft, ob ein bestimmter Kanal schon angezeigt wird

   \param channel Konstanter Zeiger auf einen Kanal mit
   dem selben Namen
   \return true, wenn der Kanal bereits angezeigt wird
*/

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

/*****************************************************************************/

/**
   Berechnet die gesamte Zeitspanne aller Kanäle
*/

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
        if (channel_i->chunks()->size() == 0) continue;

        if (channel_i->start() < _range_start)
            _range_start = channel_i->start();
        if (channel_i->end() > _range_end) _range_end = channel_i->end();
    }
}

/*****************************************************************************/

/**
   Lädt alle Daten zur aktuellen Zeitspanne
*/

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

    if (_range_cb) _range_cb(_range_start, _range_end, _range_cb_data);
}

/*****************************************************************************/

/**
   FLTK-Zeichenfunktion
*/

void ViewViewData::draw()
{
    stringstream str;
    list<ViewChannel>::iterator channel_i;
    int channel_pos;
    int clip_start, clip_height;

    if (_do_not_draw) return;

    // Schriftart und -größe setzen
    fl_font(FL_HELVETICA, 10);

    // Hintergrund zeichnen
    draw_box(FL_DOWN_BOX, FL_WHITE);

    // Temporäre Größen berechnen
    _channel_area_height = h() - SKALENHOEHE - INFOHOEHE - 2 * ABSTAND - 2;
    if (_channels.size())
        _channel_height = _channel_area_height / _channels.size();
    else _channel_height = 0;
    if (_channel_height < MIN_KANAL_HOEHE) _channel_height = MIN_KANAL_HOEHE;
    _channel_area_width = w() - 2 * ABSTAND;

    // Track-Bar zeichnen
    _track_bar->content_height(_channels.size() * _channel_height);
    _track_bar->view_height(_channel_area_height);
    _track_bar->draw(x() + w() - ABSTAND - TRACK_BAR_WIDTH,
                     y() + ABSTAND,
                     TRACK_BAR_WIDTH,
                     h() - 2 * ABSTAND);
    _scroll_pos = _track_bar->position();

    // Wenn die Track-Bar sichtbar ist, wird die Anzeigebreite schmaler
    if (_track_bar->visible()) _channel_area_width -= TRACK_BAR_WIDTH + 1;

    // Begrenzungslinien für den Datenbereich zeichnen
    fl_color(0, 0, 0);
    fl_line(x() + ABSTAND,
            y() + ABSTAND + SKALENHOEHE,
            x() + ABSTAND + _channel_area_width,
            y() + ABSTAND + SKALENHOEHE);
    fl_line(x() + ABSTAND,
            y() + h() - ABSTAND - INFOHOEHE,
            x() + ABSTAND +  _channel_area_width,
            y() + h() - ABSTAND - INFOHOEHE);

    // Wenn keine Channels vorhanden, hier beenden
    if (_channels.size() == 0) return;

    // Allgemeine Infos zeichnen
    fl_color(0, 0, 0);
    str.str("");
    str.clear();
    str << "Time range from " << _range_start.to_real_time()
        << " to " << _range_end.to_real_time();
    if (_range_end <= _range_start) str << "  - ILLEGAL RANGE!";
    fl_draw(str.str().c_str(), x() + ABSTAND, y() + h() - ABSTAND);

    // Abbruch, wenn ungültige Skalenbereiche
    if (_range_end <= _range_start) return;

    // Skalierungsfaktor bestimmen
    _scale_x = _channel_area_width / (_range_end - _range_start).to_dbl();

    // Clipping für das Zeichnen der Erfassungslücken einrichten
    fl_push_clip(x() + ABSTAND,                   // X-Position
                 y() + ABSTAND + SKALENHOEHE + 2, // Y-Position
                 _channel_area_width,             // Breite
                 _channel_area_height);           // Höhe

    // Erfassungslücken farblich hinterlegen
    channel_pos = 0;
    channel_i = _channels.begin();
    while (channel_i != _channels.end())
    {
        if (channel_pos * _channel_height <= _scroll_pos + _channel_area_height
            && (channel_pos + 1) * _channel_height >= _scroll_pos)
        {
            _draw_gaps(&(*channel_i),                            // Kanal
                       x() + ABSTAND,                            // X-Position
                       y() + ABSTAND + SKALENHOEHE - _scroll_pos
                       + channel_pos * _channel_height + 2,  // Y-Position
                       _channel_area_width,                      // Breite
                       _channel_height);                         // Höhe
        }

        channel_i++;
        channel_pos++;
    }

    // Clipping für Erfassungslücken wieder entfernen
    fl_pop_clip();

    // Zeitskala zeichnen
    _draw_time_scale(x(), y(), _channel_area_width,
                     h() - INFOHOEHE - 2 * ABSTAND);

    // Clipping für den gesamten Kanal-Bereich einrichten
    fl_push_clip(x() + ABSTAND,                   // X-Position
                 y() + ABSTAND + SKALENHOEHE + 2, // Y-Position
                 _channel_area_width,             // Breite
                 _channel_area_height - 1);       // Höhe

    // Alle Kanäle zeichnen, die sichtbar sind
    channel_pos = 0;
    channel_i = _channels.begin();
    while (channel_i != _channels.end())
    {
        if (channel_pos * _channel_height <= _scroll_pos + _channel_area_height
            && (channel_pos + 1) * _channel_height >= _scroll_pos)
        {
            clip_start = channel_pos * _channel_height - _scroll_pos;
            if (clip_start < 0) clip_start = 0;

            clip_height = _channel_height;
            if ((channel_pos + 1) * _channel_height - 1 - _scroll_pos
                >= _channel_area_height)
            {
                clip_height = _channel_area_height
                    - channel_pos * _channel_height + _scroll_pos;
            }

            // Kanal-Clipping einrichten
            fl_push_clip(x() + ABSTAND, // X-Position
                         (y() + ABSTAND + SKALENHOEHE
                          + 2 + clip_start), // Y-Position
                         _channel_area_width, // Breite
                         clip_height - 1); // Höhe

            // Kanal zeichnen
            _draw_channel(&(*channel_i), // Kanal
                          x() + ABSTAND, // X-Position
                          (y() + ABSTAND + SKALENHOEHE - _scroll_pos
                           + channel_pos * _channel_height + 2), // Y-Position
                          _channel_area_width, // Breite
                          _channel_height); // Höhe

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

/*****************************************************************************/

/**
   Zeichnet die Lücken der Erfassung

   \param channel Konstanter Zeiger auf den Kanal
   \param left X-Position des linken Randes des Zeichenbereiches
   \param top Y-Position des oberen Randes des Zeichenbereiches
   \param width Breite des Zeichenbereiches
   \param height Höhe des Zeichenbereiches
*/

void ViewViewData::_draw_gaps(const ViewChannel *channel,
                              int left, int top,
                              int width, int height)
{
    double xp, old_xp;
    int offset_drawing, height_drawing;
    list<ViewChunk *>::const_iterator chunk_i;
    ViewViewDataChunkRange chunk_range;
    vector<ViewViewDataChunkRange> chunk_ranges, relevant_chunk_ranges;
    vector<ViewViewDataChunkRange>::iterator chunk_range_i;
    COMTime last_end;

    offset_drawing = top + height - 2;
    height_drawing = height - KANAL_HEADER_HOEHE - 4;

    // Chunk-Zeitbereiche merken
    chunk_i = channel->chunks()->begin();
    while (chunk_i != channel->chunks()->end())
    {
        chunk_range.start = (*chunk_i)->start();
        chunk_range.end = (*chunk_i)->end();
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
            cout << "WARNING: chunks overlapping in channel \""
                 << channel->name() << "\"!" << endl;
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
        xp = (chunk_range_i->start.to_dbl() - _range_start.to_dbl())
            * _scale_x;

        if (xp > old_xp + 1) // Lücke vorhanden?
        {
            fl_rectf(left + (int) (old_xp + 1.5),
                     offset_drawing - height_drawing,
                     (int) (xp - old_xp - 1),
                     height_drawing);
        }

        old_xp = (chunk_range_i->end.to_dbl() - _range_start.to_dbl())
            * _scale_x;

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

/*****************************************************************************/

/**
   Prädikatsfunktion, wird von _draw_gaps beim Sortieren verwendet.

   Der sort()-Algorithmus benötigt einen Zeiger auf eine
   Prädikatisfunktion, die die Ordnung zweier Elemente
   angibt.

   \param range1 Konstante Referenz auf die erste Zeitspanne
   \param range2 Konstante Referenz auf die zweite Zeitspanne
   \return true, wenn die Erste vor der zweiten Zeitspanne liegt
*/

bool ViewViewData::range_before(const ViewViewDataChunkRange &range1,
                                const ViewViewDataChunkRange &range2)
{
    return range1.start < range2.start;
}

/*****************************************************************************/

/**
   Zeichnet die Zeitskala

   \param left X-Position des linken Randes des Zeichenbereiches
   \param top Y-Position des oberen Randes des Zeichenbereiches
   \param width Breite des Zeichenbereiches
   \param height Höhe des Zeichenbereiches
*/

void ViewViewData::_draw_time_scale(unsigned int left, unsigned int top,
                                    unsigned int width, unsigned int height)
{
    int pot, step_factor_index, last_pixel, xp, text_width, text_height;
    double time_step, step;
    bool scale_fits;
    COMTime time_start, time_end;
    stringstream str;

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
            xp = (int) ((t - _range_start).to_dbl() * _scale_x);

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
            text_width = 0;
            fl_measure(str.str().c_str(), text_width, text_height);
            last_pixel = xp + text_width + 5;
        }
    }

    if (!scale_fits) return; // Nur zeichnen, wenn eine
                             // sinnvolle Skala gefunden wurde

    for (COMTime t = time_start; t <= time_end; t += step)
    {
        xp = (int) ((t - _range_start).to_dbl() * _scale_x);

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
            fl_draw(str.str().c_str(), left + xp + 2, top + 12);
        }
    }
}

/*****************************************************************************/

/**
   Zeichnet eine Kanalzeile

   \param channel Konstanter Zeiger auf den Kanal, der
   gezeichnet werden soll
   \param left X-Position des linken Randes des Zeichenbereiches
   \param top Y-Position des oberen Randes des Zeichenbereiches
   \param width Breite des Zeichenbereiches
   \param height Höhe des Zeichenbereiches
*/

void ViewViewData::_draw_channel(const ViewChannel *channel,
                                 int left, int top,
                                 int width, int height)
{
    stringstream str;
    list<ViewChunk *>::const_iterator chunk_i;
    int drawing_top, drawing_height;

    // Box für Beschriftung zeichnen
    fl_color(FL_WHITE);
    fl_rectf(left + 1, top + 1, width - 2, KANAL_HEADER_HOEHE - 2);
    fl_color(FL_BLACK);
    fl_rect(left, top, width, KANAL_HEADER_HOEHE);

    // Beschriftungstext zeichnen
    str << channel->name();
    str << " | ";
    str << channel->min() << channel->unit();
    str << " - ";
    str << channel->max() << channel->unit();
    if (channel->max() < channel->min()) str << " (ILLEGAL RANGE)";
    str << " | ";
    str << channel->blocks_fetched() << " block(s)";

    if (channel->min_level_fetched() == channel->max_level_fetched())
    {
        str << " | level " << channel->min_level_fetched();
    }
    else
    {
        str << " | levels " << channel->min_level_fetched();
        str << " - " << channel->max_level_fetched();
    }

    fl_draw(str.str().c_str(), left + 5, top + 10);

    // Annehmen, dass es keinen Schnittpunkt der Daten mit einer
    // eventuellen Scanlinie gibt
    _scan_found = false;

    drawing_top = top + KANAL_HEADER_HOEHE + 2;
    drawing_height = height - KANAL_HEADER_HOEHE - 4;

    // Kanaldaten zeichnen, wenn sinnvolle Werteskala
    if (channel->max() >= channel->min())
    {
        _channel_min = channel->min();
        _channel_max = channel->max();

        // Wenn die Werteskala nur einen Wert umfasst,
        // Bereich zur Anzeige erweitern
        if (_channel_min == _channel_max)
        {
            _channel_min--;
            _channel_max++;
        }

        _scale_y = drawing_height / (_channel_max - _channel_min);

        // Daten zeichnen
        chunk_i = channel->chunks()->begin();
        while (chunk_i != channel->chunks()->end())
        {
            if ((*chunk_i)->current_level() == 0)
            {
                _draw_gen(*chunk_i, left, drawing_top, width, drawing_height);
            }
            else
            {
                _draw_min_max(*chunk_i, left, drawing_top,
                              width, drawing_height);
            }

            chunk_i++;
        }
    }

    // Bei Bedarf Scan-Linie mit Datenwerten zeichnen
    _draw_scan(channel, left, drawing_top, width, drawing_height);
}

/*****************************************************************************/

/**
   Zeichnet generische Datenwerte

   \param chunk Konstanter Zeiger auf den Chunk, dessen
   Daten gezeichnet werden sollen
   \param left X-Position des linken Randes des Zeichenbereiches
   \param top Y-Position des oberen Randes des Zeichenbereiches
   \param width Breite des Zeichenbereiches
   \param height Höhe des Zeichenbereiches
*/

void ViewViewData::_draw_gen(const ViewChunk *chunk,
                             int left, int top,
                             int width, int height)
{
    double xv, yv, time, value, old_value;
    int xp, yp, old_xp, old_yp, dx, dy;
    bool first_in_chunk = true, first_drawn = true;
    unsigned int i;

    fl_color(GEN_COLOR);

    for (i = 0; i < chunk->gen_data()->size(); i++)
    {
        time = chunk->gen_data()->time(i);
        value = chunk->gen_data()->value(i);
        xv = (time - _range_start.to_dbl()) * _scale_x;
        yv = (value - _channel_min) * _scale_y;

        // Runden
        if (xv >= 0) xp = (int) (xv + 0.5);
        else xp = (int) (xv - 0.5);
        if (yv >= 0) yp = (int) (yv + 0.5);
        else yp = (int) (yv - 0.5);

        // Nur zeichnen, wenn aktueller Punkt auch
        // innerhalb des Zeitfensters liegt
        if (xp >= 0)
        {
            if (first_in_chunk)
            {
                fl_point(left + xp, top + height - yp);
            }
            else
            {
                dx = xp - old_xp;
                dy = yp - old_yp;

                // Wenn der aktuelle Pixel mehr als einen Pixel
                // vom Letzten entfernt liegt
                if ((float) dx * (float) dx + (float) dy * (float) dy > 0)
                {
                    // Linie zeichnen
                    fl_line(left + old_xp, top + height - old_yp,
                            left + xp, top + height - yp);
                }
            }

            if (xp >= width) break;

            if (_scanning)
            {
                if (xp == _scan_x) // Punkt fällt genau auf die Scan-Linie
                {
                    if (_scan_found)
                    {
                        if (value < _scan_min) _scan_min = value;
                        if (value > _scan_max) _scan_max = value;
                    }
                    else
                    {
                        _scan_min = value;
                        _scan_max = value;
                        _scan_ch_x = _scan_x;
                        _scan_found = true;
                    }
                }

                // Die Scan-Linie ist zwischen zwei Punkten
                else if (xp > _scan_x && old_xp < _scan_x && !first_drawn)
                {
                    if (_scan_x - old_xp < xp - _scan_x)
                    {
                        _scan_min = old_value;
                        _scan_max = old_value;
                        _scan_ch_x = old_xp;
                    }
                    else
                    {
                        _scan_min = value;
                        _scan_max = value;
                        _scan_ch_x = xp;
                    }

                    _scan_found = true;
                }
            }

            first_drawn = false;
        }

        // Alle folgenden Werte des Blockes gehen über das
        // Zeichenfenster hinaus. Abbrechen.
        if (xp >= width) break;

        old_xp = xp;
        old_yp = yp;
        old_value = value;
        first_in_chunk = false;
    }
}

/*****************************************************************************/

/**
   Zeichnet Meta-Datenwerte

   \param chunk Konstanter Zeiger auf den Chunk, der
   gezeichnet werden soll
   \param left X-Position des linken Randes des Zeichenbereiches
   \param top Y-Position des oberen Randes des Zeichenbereiches
   \param width Breite des Zeichenbereiches
   \param height Höhe des Zeichenbereiches
*/

void ViewViewData::_draw_min_max(const ViewChunk *chunk,
                                 int left, int top,
                                 int width, int height)
{
    double xv, yv, time, value;
    int xp, yp, i;
    unsigned int j;
    list<ViewBlock *>::const_iterator block_i;
    int *min_px, *max_px;

    try
    {
        min_px = new int[width];
    }
    catch (...)
    {
        cout << "ERROR: could not allocate drawing memory!" << endl;
        return;
    }

    try
    {
        max_px = new int[width];
    }
    catch (...)
    {
        cout << "ERROR: could not allocate drawing memory!" << endl;
        delete [] min_px;
        return;
    }

    for (i = 0; i < width; i++)
    {
        min_px[i] = -1;
        max_px[i] = -1;
    }

    fl_color(MIN_MAX_COLOR);

#ifdef DEBUG
    //cout << "min: " << chunk->min_data()->size() << " values." << endl;
#endif

    for (j = 0; j < chunk->min_data()->size(); j++)
    {
        time = chunk->min_data()->time(j);
        value = chunk->min_data()->value(j);
        xv = (time - _range_start.to_dbl()) * _scale_x;
        yv = (value - _channel_min) * _scale_y;

        // Runden
        if (xv >= 0) xp = (int) (xv + 0.5);
        else xp = (int) (xv - 0.5);
        if (yv >= 0) yp = (int) (yv + 0.5);
        else yp = (int) (yv - 0.5);

        if (xp >= 0 && xp < width)
        {
            if (min_px[xp] == -1 || (min_px[xp] != -1 && yp < min_px[xp]))
            {
                min_px[xp] = yp;
            }

            if (_scanning)
            {
                if (xp == _scan_x)
                {
                    if (_scan_found)
                    {
                        if (value < _scan_min) _scan_min = value;
                        if (value > _scan_max) _scan_max = value;
                    }
                    else
                    {
                        _scan_min = value;
                        _scan_max = value;
                        _scan_ch_x = _scan_x;
                        _scan_found = true;
                    }
                }
            }
        }

        // Alle folgenden Werte des Blockes gehen über das
        // Zeichenfenster hinaus. Abbrechen.
        else if (xp >= width) break;
    }

    for (j = 0; j < chunk->max_data()->size(); j++)
    {
        time = chunk->max_data()->time(j);
        value = chunk->max_data()->value(j);
        xv = (time - _range_start.to_dbl()) * _scale_x;
        yv = (value - _channel_min) * _scale_y;

        // Runden
        if (xv >= 0) xp = (int) (xv + 0.5);
        else xp = (int) (xv - 0.5);
        if (yv >= 0) yp = (int) (yv + 0.5);
        else yp = (int) (yv - 0.5);

        if (xp >= 0 && xp < width)
        {
            if (max_px[xp] == -1 || (max_px[xp] != -1 && yp > max_px[xp]))
            {
                max_px[xp] = yp;
            }

            if (_scanning)
            {
                if (xp == _scan_x)
                {
                    if (_scan_found)
                    {
                        if (value < _scan_min) _scan_min = value;
                        if (value > _scan_max) _scan_max = value;
                    }
                    else
                    {
                        _scan_min = value;
                        _scan_max = value;
                        _scan_ch_x = _scan_x;
                        _scan_found = true;
                    }
                }
            }
        }

        // Alle folgenden Werte des Blockes gehen über das
        // Zeichenfenster hinaus. Abbrechen.
        else if (xp >= width) break;
    }

    // Werte zeichnen
    for (i = 0; i < width; i++)
    {
        if (min_px[i] != -1 && max_px[i] != -1)
        {
            fl_line(left + i,
                    top + height - min_px[i],
                    left + i,
                    top + height - max_px[i]);
        }
        else
        {
            if (min_px[i] != -1)
            {
                fl_point(left + i, top + height - min_px[i]);
            }
            if (max_px[i] != -1)
            {
                fl_point(left + i, top + height - max_px[i]);
            }
        }
    }

    delete [] min_px;
    delete [] max_px;
}

/*****************************************************************************/

/**
   Zeichnet die Scanlinie mit Schnittlinien und Beschriftung

   \param channel Konstanter Zeiger auf den Kanal
   \param left X-Position des linken Randes des Zeichenbereiches
   \param top Y-Position des oberen Randes des Zeichenbereiches
   \param width Breite des Zeichenbereiches
   \param height Höhe des Zeichenbereiches
*/

void ViewViewData::_draw_scan(const ViewChannel *channel,
                              int left, int top, int width, int height)
{
    stringstream text_time, text_value;
    int text_time_width, text_value_width;
    int text_time_x, text_value_x;
    int text_height, yp1, yp2, y_pos, scan_x;
    COMTime scan_time;

    // Wenn nicht gescannt werden soll, oder die
    // Scan-Linie nicht im gültigen Bereich liegt, abbrechen.
    if (!_scanning || _scan_x < 0 || _scan_x >= width) return;

    if (_scan_found)
    {
        scan_x = _scan_ch_x;
        fl_color(SCAN_COLOR);
    }
    else
    {
        scan_x = _scan_x;
        fl_color(fl_darker(SCAN_COLOR));
    }

    // Scan-Linie zeichnen
    fl_line(left + scan_x, top, left + scan_x, top + height);

    // Zeit der Scanlinie berechnen
    scan_time = _range_start.to_dbl() + (scan_x - ABSTAND) / _scale_x;

    // Textbreiten auf 0 setzen
    // Auch wichtig, da fl_measure den Text sonst
    // auf die Breite gebrochen berechnet
    text_time_width = 0;
    text_value_width = 0;

    text_time << scan_time.to_real_time();
    fl_measure(text_time.str().c_str(), text_time_width, text_height);

    if (_scan_found)
    {
        text_value << _scan_min << channel->unit();

        if (_scan_min != _scan_max)
        {
            text_value << " - " << _scan_max << channel->unit();
        }

        fl_measure(text_value.str().c_str(), text_value_width, text_height);
    }

    // Wenn einer der Texte nicht mehr rechts neben die Scanlinie passt
    if (scan_x + 10 + text_time_width >= left + width
        || scan_x + 10 + text_value_width >= left + width)
    {
        // Alle Texte auf die linke Seite der Scanlinie zeichnen
        text_time_x = scan_x - 10 - text_time_width;
        text_value_x = scan_x - 10 - text_value_width;
    }
    else
    {
        // Alle Texte auf die rechte Seite der Scanlinie zeichnen
        text_time_x = scan_x + 10;
        text_value_x = scan_x + 10;
    }

    if (_scan_found)
    {
        // Schnittpunkte anzeigen
        yp1 = (int) ((_scan_max - _channel_min) * _scale_y + 0.5);

        fl_color(SCAN_COLOR);
        fl_line(left, top + height - yp1, left + width, top + height - yp1);

        if (_scan_min != _scan_max)
        {
            yp2 = (int) ((_scan_min - _channel_min) * _scale_y + 0.5);

            if (yp2 != yp1)
            {
                fl_line(left, top + height - yp2, left + width,
                        top + height - yp2);
            }
        }
    }

    // Zeit-Text zeichnen
    fl_color(FL_WHITE);
    fl_rectf(left + text_time_x - 2, top, text_time_width + 4, text_height);
    if (_scan_found) fl_color(SCAN_COLOR);
    else fl_color(fl_darker(SCAN_COLOR));
    fl_draw(text_time.str().c_str(), left + text_time_x,
            top + text_height / 2 + fl_descent());
    y_pos = text_height;

    if (_scan_found)
    {
        if (height - yp1 - 5 - text_height < y_pos)
            yp1 = height - y_pos - text_height - 5;

        // Text für Wert(e) zeichnen
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
   Zeichnet die Zoom-Linien oder den Verschiebungs-Pfeil
*/

void ViewViewData::_draw_interactions()
{
    stringstream str;
    int text_width, text_height, x_pos;
    COMTime time;

    // "Zooming-Linie"
    if (_zooming)
    {
        fl_color(FL_RED);

        fl_line(x() + _start_x, y() + ABSTAND + SKALENHOEHE + 1,
                x() + _start_x, y() + h() - ABSTAND - INFOHOEHE - 1);
        fl_line(x() + _end_x, y() + ABSTAND + SKALENHOEHE + 1,
                x() + _end_x, y() + h() - ABSTAND - INFOHOEHE - 1);

        time = _range_start.to_dbl() + _start_x / _scale_x;

        str.str("");
        str.clear();
        str << time.to_real_time();

        text_width = 0;
        fl_measure(str.str().c_str(), text_width, text_height);
        x_pos = _start_x;

        if (x_pos + text_width >= w() - ABSTAND)
            x_pos = w() - ABSTAND - text_width;
        if (x_pos < 0) x_pos = 0;

        fl_color(255, 255, 255);
        fl_rectf(x() + x_pos + 1,
                 y() + ABSTAND + SKALENHOEHE + 1,
                 text_width,
                 text_height);
        fl_color(255, 0, 0);
        fl_draw(str.str().c_str(),
                x() + x_pos + 2,
                y() + ABSTAND + SKALENHOEHE + 10);

        time = _range_start.to_dbl() + _end_x / _scale_x;

        str.str("");
        str.clear();
        str << time.to_real_time();

        text_width = 0;
        fl_measure(str.str().c_str(), text_width, text_height);
        x_pos = _end_x;

        if (x_pos + text_width >= w() - ABSTAND)
            x_pos = w() - ABSTAND - text_width;
        if (x_pos < 0) x_pos = 0;

        fl_color(255, 255, 255);
        fl_rectf(x() + x_pos + 1,
                 y() + ABSTAND + SKALENHOEHE + 10,
                 text_width,
                 text_height);
        fl_color(255, 0, 0);
        fl_draw(str.str().c_str(),
                x() + x_pos + 2,
                y() + ABSTAND + SKALENHOEHE + 20);
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
}

/*****************************************************************************/

/**
   FLTK-Ereignisfunktion

   \param event FLTK-Eventnummer
   \return 1, wenn Ereignis bearbeitet, sonst 0
*/

int ViewViewData::handle(int event)
{
    int xp, yp, dx, dy, dw, dh, zoom_out_factor;
    double scale_x;
    COMTime new_start, new_end, time_diff;
    unsigned int channel_area_width;
    COMTime time_range;

    // Cursorposition relativ zum Widget errechnen
    xp = Fl::event_x() - x();
    yp = Fl::event_y() - y();

    // Ist das Ereignis für die Track-Bar?
    if (_track_bar->handle(event,
                           xp - w() + TRACK_BAR_WIDTH + ABSTAND,
                           yp - ABSTAND))
    {
        redraw();
        return 1;
    }

    // Skalierungsfaktor berechnen
    channel_area_width = w() - 2 * ABSTAND;
    if (_track_bar->visible()) channel_area_width -= TRACK_BAR_WIDTH + 1;
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
                    _range_start = new_start.to_dbl()
                        - time_range.to_dbl() * 0.5;
                    _range_end = new_start.to_dbl()
                        + time_range.to_dbl() * 0.5;
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

            if (Fl::event_state(FL_BUTTON1)) // Linke Maustaste
            {
                _zooming = true;
            }
            else if (Fl::event_state(FL_BUTTON3)) // Rechte Maustaste
            {
                _moving = true;
            }

            _end_x = xp;
            _end_y = yp;

            redraw();

            return 1;

        case FL_RELEASE:

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

            if (_zooming)
            {
                _zooming = false;

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
                _scan_x = xp - ABSTAND;
                _scanning = true;
                redraw();
            }

            return 1;

        case FL_MOVE:

            if (_scanning)
            {
                _scan_x = xp - ABSTAND;
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
                _scan_x = xp - ABSTAND;
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

/*****************************************************************************/
