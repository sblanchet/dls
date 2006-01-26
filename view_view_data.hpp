//---------------------------------------------------------------
//
//  V I E W _ V I E W _ D A T A . H P P
//
//---------------------------------------------------------------

#ifndef ViewViewDataHpp
#define ViewViewDataHpp

//---------------------------------------------------------------

#include <list>
using namespace std;

#include <FL/Fl_Widget.h>

//---------------------------------------------------------------

#include "com_time.hpp"
#include "view_channel.hpp"

//---------------------------------------------------------------

struct ViewViewDataChunkRange
{
  COMTime start;
  COMTime end;
};

bool range_before(const ViewViewDataChunkRange &,
                  const ViewViewDataChunkRange &);

//---------------------------------------------------------------

class ViewViewData : public Fl_Widget
{
public:
  ViewViewData(int, int, int, int, const char * = "");
  ~ViewViewData();

  void set_job(const string &, unsigned int);
  void add_channel(const ViewChannel *);
  void rem_channel(const ViewChannel *);
  bool has_channel(const ViewChannel *) const;
  void clear();
  void full_range();
  void update();

private:
  // Daten
  list<ViewChannel> _channels;
  COMTime _range_start;                      /**< Startzeit der anzuzeigenden Zeitspanne */
  COMTime _range_end;                        /**< Endzeit der anzuzeigenden Zeitspanne */
  bool _full_range;                          /**< Gibt an, ob beim Hinzufügen eines
                                                  Kanales die Zeitspanne auf die volle
                                                  Zeitspanne ausgeweitet werden soll. */

  string _dls_dir;
  unsigned int _job_id;

  // Widget-Zustand
  bool _focused;
  bool _dragging_line;
  bool _moving;
  bool _scanning;
  int _start_x, _start_y, _end_x, _end_y;
  bool _mouse_in;
  bool _do_not_draw;

  // Scrollbar
  bool _scroll_bar_visible;
  int _scroll_pos;
  int _scroll_button_pos, _scroll_button_height, _scroll_grip;
  bool _scroll_button_tracking;
  
  void _load_data();
  void _calc_range();

  virtual void draw();
  virtual int handle(int);

  void _draw_gaps(const ViewChannel *, int, int, int, int);
  void _draw_time_scale(unsigned int, unsigned int, unsigned int, unsigned int);
  void _draw_scroll_bar(unsigned int, unsigned int, unsigned int, unsigned int);
  void _draw_channel(const ViewChannel *, int, int, int, int);
  void _draw_gen(const ViewChannel *, const ViewChunk *, int, int, int, int);
  void _draw_min_max(const ViewChannel *, const ViewChunk *, int, int, int, int);
  void _draw_interactions();
};

//---------------------------------------------------------------

#endif
