/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef ViewViewDataHpp
#define ViewViewDataHpp

/*****************************************************************************/

#include <list>
using namespace std;

#include <FL/Fl_Widget.h>

/*****************************************************************************/

#include "fl_track_bar.hpp"
#include "com_time.hpp"
#include "view_channel.hpp"

/*****************************************************************************/

/**
   Bereich eines Chunks zur Anzeige der nicht erfassten Bereiche
*/

struct ViewViewDataChunkRange
{
    COMTime start;
    COMTime end;
};

/*****************************************************************************/

/**
   Anzeige f�r Messdaten

   Diese Anzeige kann die Daten mehrerer Kan�le
   �bereinander darstellen. Diese werden auf die
   volle Widget-H�he verteilt. W�rden, die einzelnen
   Kan�le zu schmal werden, wird eine Mindesth�he
   f�r jeden Kanal verwendet und eine Track-Bar
   angezeigt. Das Widget unterst�tzt Interaktionen
   mit Maus und Keyboard
*/

class ViewViewData : public Fl_Widget
{
    public:
    ViewViewData(int, int, int, int, const char * = "");
    ~ViewViewData();

    // �ffentliche Methoden
    void set_job(const string &, unsigned int);
    void add_channel(const ViewChannel *);
    void rem_channel(const ViewChannel *);
    bool has_channel(const ViewChannel *) const;
    void clear();
    void full_range();
    void update();
    void range_callback(void (*)(COMTime, COMTime, void *), void *);

    COMTime start() const;
    COMTime end() const;
    const list<ViewChannel> *channels() const;

    private:
    // Daten
    string _dls_dir;             /**< DLS-Datenverzeichnis */
    unsigned int _job_id;        /**< Auftrags-ID */
    Fl_Track_Bar *_track_bar;    /**< Vertikale Track-Bar */
    list<ViewChannel> _channels; /**< Liste der zu Zeigenden Kan�le */
    COMTime _range_start;        /**< Startzeit der anzuzeigenden Zeitspanne */
    COMTime _range_end;          /**< Endzeit der anzuzeigenden Zeitspanne */
    bool _full_range;            /**< Gibt an, ob beim Hinzuf�gen eines
                                    Kanales die Zeitspanne auf die volle
                                    Zeitspanne ausgeweitet werden soll. */

    // Callbacks
    void (*_range_cb)(COMTime, COMTime, void *); /**< Funktionszeiger auf
                                                    Callback bei
                                                    Zeitbereichs�nderungen */
    void *_range_cb_data;                        /**< Daten zur �bergabe bei
                                                    Range-CB */

    // Widget-Zustand
    bool _focused;       /**< Das Widget h�lt momentan den Fokus */
    bool _zooming;       /**< Der Benutzer zieht einen Zoom-Bereich auf */
    bool _moving;        /**< Der Benutzer zieht einen Verschiebungs-Pfeil */
    bool _scanning;      /**< Der Benutzer l�sst die Scan-Linie anzeigen */
    int _scan_x;         /**< X-Position der Scan-Linie */
    int _start_x;        /**< X-Position des Cursors zu Beginn einer Aktion */
    int _start_y;        /**< Y-Position des Cursors zu Beginn einer Aktion */
    int _end_x;          /**< X-Position des Cursors zum Ende einer Aktion */
    int _end_y;          /**< Y-Position des Cursors zum Ende einer Aktion */
    bool _mouse_in;      /**< Der Mauscursor befindet sich auf dem Widget */
    bool _do_not_draw;   /**< Flag: Der Inhalt soll nicht neu
                            gezeichnet werden */

    // Tempor�re Gr��en zum Zeichnen
    int _channel_area_width;  /**< Breite des Zeichenbereiches
                                 f�r die Kan�le */
    int _channel_area_height; /**< H�he des zeichenbereiches f�r die Kan�le */
    int _channel_height;      /**< H�he einer Kanalzeile, incl. Textbox */
    double _channel_min;      /**< Kleinster Wert des aktuellen Kanals */
    double _channel_max;      /**< Gr��ter Wert des aktuellen Kanals */
    int _scroll_pos;          /**< Anzeige-Offset (abh�ngig von track-Bar) */
    double _scale_x;          /**< Faktor f�r horizontale Skalierung
                                 (in px/�s) */
    double _scale_y;          /**< Faktor f�r vertikale Skalierung
                                 (in px/�s) */
    bool _scan_found;         /**< Es gibt einen Schnittpunkt der Daten mit
                                 der Scan-Linie */
    double _scan_min;         /**< Minimaler Datenwert an der Scanlinie des
                                 aktuellen Kanals */
    double _scan_max;         /**< Maximaler Datenwert an der Scanlinie des
                                 aktuellen Kanals */
    int _scan_ch_x;           /**< X-Position der eingeschnappten Scanlinie
                                 eines Kanals */

    // Private Methoden
    void _load_data();
    void _calc_range();
    void _draw_gaps(const ViewChannel *, int, int, int, int);
    void _draw_time_scale(unsigned int, unsigned int,
                          unsigned int, unsigned int);
    void _draw_scroll_bar(unsigned int, unsigned int,
                          unsigned int, unsigned int);
    void _draw_channel(const ViewChannel *, int, int, int, int);
    void _draw_gen(const ViewChunk *, int, int, int, int);
    void _draw_min_max(const ViewChunk *, int, int, int, int);
    void _draw_interactions();
    void _draw_scan(const ViewChannel *, int, int, int, int);

    // Pr�dikatsfunktion zum Sortieren
    static bool range_before(const ViewViewDataChunkRange &,
                             const ViewViewDataChunkRange &);

    // FLTK
    virtual void draw();
    virtual int handle(int);
};

/*****************************************************************************/

inline COMTime ViewViewData::start() const
{
    return _range_start;
}

/*****************************************************************************/

inline COMTime ViewViewData::end() const
{
    return _range_end;
}

/*****************************************************************************/

inline const list<ViewChannel> *ViewViewData::channels() const
{
    return &_channels;
}

/*****************************************************************************/

#endif
