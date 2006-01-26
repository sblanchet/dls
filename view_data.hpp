//---------------------------------------------------------------
//
//  V I E W _ D A T A . H P P
//
//---------------------------------------------------------------

#ifndef ViewDataHpp
#define ViewDataHpp

//---------------------------------------------------------------

#include "com_time.hpp"

//---------------------------------------------------------------

//---------------------------------------------------------------

/**
   Datenliste für die Anzeige
*/

class ViewData
{
public:
  ViewData();
  virtual ~ViewData();

  void start_time(COMTime);
  void end_time(COMTime);
  void time_per_value(double);

  COMTime start_time() const;
  COMTime end_time() const;

  virtual unsigned int size() const = 0;
  virtual double time(unsigned int) const = 0;
  virtual double value(unsigned int) const = 0;

protected:
  COMTime _start_time;   /**< Startzeit der geladenen Daten */
  COMTime _end_time;     /**< Endzeit der geladenen Daten */
  double _time_per_value;
};

//---------------------------------------------------------------

/**
   Gibt die Startzeit der geladenen Daten zurück

   \return Startzeit
*/

inline COMTime ViewData::start_time() const
{
  return _start_time;
}

//---------------------------------------------------------------

/**
   Gibt die Endzeit der geladenen Daten zurück

   \return Endzeit
*/

inline COMTime ViewData::end_time() const
{
  return _end_time;
}

//---------------------------------------------------------------

#endif
