/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include "view_globals.hpp"
#include "view_data.hpp"

/*****************************************************************************/

/**
   Konstruktor
*/

ViewData::ViewData()
{
}

/*****************************************************************************/

/**
   Destruktor
*/

ViewData::~ViewData()
{
}

/*****************************************************************************/

/**
   Setzt die Startzeit

   \param time Neue Startzeit
*/

void ViewData::start_time(COMTime time)
{
  _start_time = time;
}

/*****************************************************************************/

/**
   Setzt die Endzeit

   \param time Neue Endzeit
*/

void ViewData::end_time(COMTime time)
{
  _end_time = time;
}

/*****************************************************************************/

/**
   Setzt das Delta t

   \param tpv delta t
*/

void ViewData::time_per_value(double tpv)
{
  _time_per_value = tpv;
}

/*****************************************************************************/
