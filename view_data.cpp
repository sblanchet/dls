//---------------------------------------------------------------
//
//  V I E W _ D A T A . C P P
//
//---------------------------------------------------------------

#include "view_globals.hpp"
#include "view_data.hpp"

//---------------------------------------------------------------

RCS_ID("$Header: /home/fp/dls/src/RCS/view_data.cpp,v 1.6 2004/12/21 14:55:44 fp Exp $");

//---------------------------------------------------------------

/**
   Konstruktor
*/

ViewData::ViewData()
{
}

//---------------------------------------------------------------

/**
   Destruktor
*/

ViewData::~ViewData()
{
}

//---------------------------------------------------------------

/**
   Setzt die Startzeit

   \param time Neue Startzeit
*/

void ViewData::start_time(COMTime time)
{
  _start_time = time;
}

//---------------------------------------------------------------

/**
   Setzt die Endzeit

   \param time Neue Endzeit
*/

void ViewData::end_time(COMTime time)
{
  _end_time = time;
}

//---------------------------------------------------------------

/**
   Setzt das Delta t

   \param tpv delta t
*/

void ViewData::time_per_value(double tpv)
{
  _time_per_value = tpv;
}

//---------------------------------------------------------------
