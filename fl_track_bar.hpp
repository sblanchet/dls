//---------------------------------------------------------------
//
//  F L _ T R A C K _ B A R . H P P
//
//---------------------------------------------------------------

#ifndef FlTrackBarHpp
#define FlTrackBarHpp

//---------------------------------------------------------------

/**
   Track-Bar-Klasse f�r FLTK

   Diese Track-Bar ist kein eigenes Widget, sondern eine
   Hilfsklasse, die die Berechnung, das Zeichnen und die
   Interaktion mit dem Benutzer �bernimmt.
*/

class Fl_Track_Bar
{
public:
  Fl_Track_Bar();
  ~Fl_Track_Bar();

  void content_height(int);
  void view_height(int);
  void draw(int, int, int, int);
  bool handle(int, int, int);

  bool visible() const;
  int position() const;

private:
  int _last_width;  /**< Breite beim letzten Zeichnen */
  int _last_height; /**< H�he beim letzten Zeichnen */

  int _content_height; /**< Virtuelle H�he des zu scrollenden Inhaltes */
  int _view_height;    /**< H�he des Anzeigebereiches f�r den Inhalt */

  bool _visible;          /**< Die Trackbar ist momentan sichtbar */
  bool _pushed_on_button; /**< Der Klick vor einem Drag-Event war auf den Track-Button */
  bool _tracking;         /**< Der Benutzer zieht gerade den Track-Button */

  int _position;        /**< Offset des scrollenden Inhaltes in Pixel */
  int _button_position; /**< Position des track-Buttons */
  int _button_height;   /**< H�he des Track-Buttons */
  int _grip;            /**< Position des Cursors auf dem Track-Button beim Ziehen */
};

//---------------------------------------------------------------

/**
   Gibt zur�ck, ob der Track-Bar momentan sichtbar ist

   \return true, wenn sichtbar
*/

inline bool Fl_Track_Bar::visible() const
{
  return _visible;
}

//---------------------------------------------------------------

/**
   Gibt das Offset der scrollenden Daten f�r das Zeichnen zur�ck
   
   \return Offset in Pixeln
*/

inline int Fl_Track_Bar::position() const
{
  return _position;
}

//---------------------------------------------------------------

#endif
