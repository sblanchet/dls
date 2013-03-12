/******************************************************************************
 *
 *  $Id$
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

#include <sstream>
#include <cstdio>
using namespace std;

/*****************************************************************************/

#include "com_globals.hpp"
#include "com_time.hpp"

/*****************************************************************************/

/**
   Konstruktor
*/

COMTime::COMTime()
{
    set_null();
}

/*****************************************************************************/

/**
   Konstruktor mit int64-Parameter

   \param t Initialzeit
*/

COMTime::COMTime(int64_t t)
{
    *this = t;
}

/*****************************************************************************/

/**
   Konstruktor mit uint64-Parameter

   \param t Initialzeit
*/

COMTime::COMTime(uint64_t t)
{
    *this = (int64_t) t;
}

/*****************************************************************************/

/**
   Konstruktor mit Parameter "double"

   \param t Initialzeit
*/

COMTime::COMTime(double t)
{
    *this = t;
}

/*****************************************************************************/

/**
   Konstruktor mit Parameter "struct timeval"

   \param tv Initialzeit
*/

COMTime::COMTime(struct timeval *tv)
{
    *this = *tv;
}

/*****************************************************************************/

/**
 */

COMTime::COMTime(struct tm *t, unsigned int usec)
{
    struct timeval tv;
    tv.tv_sec = mktime(t);
    tv.tv_usec = usec;
    *this = tv;
}

/*****************************************************************************/

/**
   Zuweisungsoperator von "int64_t"

   \param t Neue Zeit
   \returns Referenz auf sich selber
*/

COMTime &COMTime::operator =(int64_t t)
{
    _time = t;
    return *this;
}

/*****************************************************************************/

/**
   Zuweisungsoperator von "uint64_t"

   \param t Neue Zeit
   \returns Referenz auf sich selber
*/

COMTime &COMTime::operator =(uint64_t t)
{
    _time = (int64_t) t;
    return *this;
}

/*****************************************************************************/

/**
   Zuweisungsoperator von "double"

   \param t Neue Zeit
   \returns Referenz auf sich selber
*/

COMTime &COMTime::operator =(double t)
{
    _time = (int64_t) t;
    return *this;
}

/*****************************************************************************/

/**
   Zuweisungsoperator von "struct timeval"

   \param tv Neue Zeit
   \returns Referenz auf sich selber
*/

COMTime &COMTime::operator =(struct timeval tv)
{
    _time = ((int64_t) tv.tv_sec) * 1000000 + (int64_t) tv.tv_usec;
    return *this;
}

/*****************************************************************************/

/**
   Zuweisung von einem double-Wert, der Sekunden
   und Sekundenbruchteile enthält

   Es findet eine Rundung statt. Diese funktioniert
   nur für positive Zeiten (>= 1.1.1970)!

   \param t Neue Zeit in Sekunden und Sekundenbruchteilen
*/

void COMTime::from_dbl_time(double t)
{
    _time = (int64_t) (t * 1000000.0 + 0.5);
}

/*****************************************************************************/

/**
   Setzt die Zeit auf Null
*/

void COMTime::set_null()
{
    _time = (int64_t) 0;
}

/*****************************************************************************/

/**
   Setzt die Zeit auf die aktuelle Zeit

   \returns Referenz auf die gesetzte Zeit
*/

void COMTime::set_now()
{
    *this = COMTime::now();
}

/*****************************************************************************/

/**
   Setzt die Zeit auf die aktuelle Zeit

   \returns 0 bei Erfolg, sonst ungleich 0
*/

int COMTime::set_date(int year, int month, int day,
        int hour, int min, int sec)
{
    struct tm tm = {};
    time_t t;

    tm.tm_sec = sec;
    tm.tm_min = min;
    tm.tm_hour = hour;
    tm.tm_mday = day;
    tm.tm_mon = month - 1;
    tm.tm_year = year - 1900;
    tm.tm_isdst = -1;

    t = mktime(&tm);
#if 0
    if (t < -2147483648 || t > 2147483647) { // 32 bit test
        std::cerr << __func__
            << "(" << t << "): Failed to set date "
                    << year << "-" << month << "-" << day << " "
                    << hour << ":" << min << ":" << sec << std::endl;
        t = -1;
    }
#endif
    if (t == -1) {
#if 0
        std::cerr << __func__
            << "(): Failed to set date "
                    << year << "-" << month << "-" << day << " "
                    << hour << ":" << min << ":" << sec << std::endl;
#endif
        return -1;
    }

    _time = t * 1000000LL;
    return 0;
}

/*****************************************************************************/

/**
   Prüft, ob die Zeit auf Null gesetzt ist

   \return true, wenn auf Null
*/

bool COMTime::is_null() const
{
    return _time == (int64_t) 0;
}

/*****************************************************************************/

/**
   Vergleichsoperator

   \param right Zeit, mit der verglichen wird
   \returns true, wenn die Zeiten gleich sind
*/

bool COMTime::operator ==(const COMTime &right) const
{
    return _time == right._time;
}

/*****************************************************************************/

/**
   Ungleich-Operator

   \param right Zeit, mit der verglichen wird
   \returns true, wenn die Zeiten ungleich sind
*/

bool COMTime::operator !=(const COMTime &right) const
{
    return _time != right._time;
}

/*****************************************************************************/

/**
   Kleiner-Operator

   \param right Zeit, mit der verglichen wird
   \returns true, wenn die Zeit des linken Operanden kleiner ist
*/

bool COMTime::operator <(const COMTime &right) const
{
    return _time < right._time;
}

/*****************************************************************************/

/**
   Größer-Operator

   \param right Zeit, mit der verglichen wird
   \returns true, wenn die Zeit des linken Operanden größer ist
*/

bool COMTime::operator >(const COMTime &right) const
{
    return _time > right._time;
}

/*****************************************************************************/

/**
   Kleiner-Gleich-Operator

   \param right Zeit, mit der verglichen wird
   \returns true, wenn die Zeit des linken Operanden kleiner
   oder gleich der Zeit des rechten Operanden ist
*/

bool COMTime::operator <=(const COMTime &right) const
{
    return _time <= right._time;
}

/*****************************************************************************/

/**
   Größer-Gleich-Operator

   \param right Zeit, mit der verglichen wird
   \returns true, wenn die Zeit des linken Operanden größer
   oder gleich der Zeit des rechten Operanden ist
*/

bool COMTime::operator >=(const COMTime &right) const
{
    return _time >= right._time;
}

/*****************************************************************************/

/**
   Additionsoperator

   \param right Zeit, die aufaddiert wird
   \returns Summe der Zeiten des linken und rechten Operanden
*/

COMTime COMTime::operator +(const COMTime &right) const
{
    return _time + right._time;
}

/*****************************************************************************/

/**
   Subtraktionsoperator

   \param right Zeit, die abgezogen wird
   \returns Differenz der Zeiten des linken und rechten Operanden
*/

COMTime COMTime::operator -(const COMTime &right) const
{
    return _time - right._time;
}

/*****************************************************************************/

/**
   Additionsoperator mit Zuweisung

   Setzt die Zeit des linken Operanden auf die Summe

   \param right Zeit, die aufaddiert wird
   \returns Referenz auf die neue Zeit
*/

COMTime &COMTime::operator +=(const COMTime &right)
{
    _time += right._time;

    return *this;
}

/*****************************************************************************/

/**
   Multiplikationsoperator

   \param factor Faktor, mit dem multipliziert wird
   \returns Produkt der Zeit des linken Operanden
   mit dem Faktor
*/

COMTime COMTime::operator *(int64_t factor) const
{
    return _time * factor;
}

/*****************************************************************************/

/**
   Stream-Ausgabeoperator

   \param o ostream-Objekt, auf dem die Zeit ausgegeben werden soll
   \param time Zeit, die auf den Stream geschrieben werden soll
   \returns Referenz auf den veränderten ostream
*/

ostream &operator <<(ostream &o, const COMTime &time)
{
    o << fixed << time._time;

    return o;
}

/*****************************************************************************/

/**
   Konvertierung nach "double"

   \returns Anzahl der Mikrosekunden als double
*/

double COMTime::to_dbl() const
{
    return (double) _time;
}

/*****************************************************************************/

/**
   Konvertierung zu einem "double"-Wert, der Sekunden
   und Sekundenbruchteile enthält

   \returns Anzahl der Sekunden als double
*/

double COMTime::to_dbl_time() const
{
    return (double) _time / 1000000.0;
}

/*****************************************************************************/

/**
   Konvertierung nach "int64_t"

   \returns Anzahl der Mikrosekunden als int64_t
*/

int64_t COMTime::to_int64() const
{
    return _time;
}

/*****************************************************************************/

/**
   Konvertierung nach "uint64_t"

   \returns Anzahl der Mikrosekunden als uint64_t
*/

uint64_t COMTime::to_uint64() const
{
    return (uint64_t) _time;
}

/*****************************************************************************/

/**
   Konvertierung nach "time_t"

   \returns Anzahl der Sekunden seit Epoch
*/

time_t COMTime::to_time_t() const
{
    return (time_t) _time / 1000000;
}

/*****************************************************************************/

/**
   Konvertierung nach "struct timeval"

   \returns Zeit als "struct timeval"
*/

struct timeval COMTime::to_tv() const
{
    struct timeval tv;

    tv.tv_sec = _time / 1000000;
    tv.tv_usec = _time % 1000000;

    return tv;
}

/*****************************************************************************/

/**
   Konvertierung nach "string"

   \returns Anzahl der Mikrosekunden in einem String
*/

string COMTime::to_str() const
{
    stringstream str;

    str << fixed << *this;

    return str.str();
}

/*****************************************************************************/

string COMTime::to_real_time() const
{
    struct timeval tv;
    struct tm local_time;
    char str[100];
    string ret;
    time_t t;

    tv = to_tv();
    t = tv.tv_sec;
    local_time = *localtime(&t);
    strftime(str, sizeof(str), "%d.%m.%Y %H:%M:%S", &local_time);
    ret = str;
    sprintf(str, ".%06u", (unsigned int) tv.tv_usec);
    return ret + str;
}

/*****************************************************************************/

string COMTime::format_time(const char *fmt) const
{
    struct timeval tv;
    struct tm local_time;
    char str[100];
    string ret;
    time_t t;

    tv = to_tv();
    t = tv.tv_sec;
    local_time = *localtime(&t);
    strftime(str, sizeof(str), fmt, &local_time);
    return str;
}

/*****************************************************************************/

string COMTime::to_rfc811_time() const
{
    return format_time("%a, %d %b %Y %H:%M:%S %z");
}

/*****************************************************************************/

string COMTime::diff_str_to(const COMTime &other) const
{
    stringstream str;
    int64_t diff, part;

    if (other._time > _time) {
	diff = other._time - _time;
    }
    else {
	diff = _time - other._time;
        str << "-";
    }

    part = diff / ((int64_t) 1000000 * 60 * 60 * 24); // Tage
    if (part) str << part << "d ";
    diff -= part * ((int64_t) 1000000 * 60 * 60 * 24);

    part = diff / ((int64_t) 1000000 * 60 * 60); // Stunden
    if (part) str << part << "h ";
    diff -= part * ((int64_t) 1000000 * 60 * 60);

    part = diff / ((int64_t) 1000000 * 60); // Minuten
    if (part) str << part << "m ";
    diff -= part * ((int64_t) 1000000 * 60);

    part = diff / 1000000; // Sekunden
    if (part) str << part << "s ";
    diff -= part * 1000000;

    if (diff) str << diff << "us ";

    // return string without last character
    return str.str().substr(0, str.str().size() - 1);
}

/*****************************************************************************/

int COMTime::year() const
{
    struct timeval tv;
    struct tm local_time;
    time_t t;

    tv = to_tv();
    t = tv.tv_sec;
    local_time = *localtime(&t);
    return local_time.tm_year + 1900;
}

/*****************************************************************************/

int COMTime::month() const
{
    struct timeval tv;
    struct tm local_time;
    time_t t;

    tv = to_tv();
    t = tv.tv_sec;
    local_time = *localtime(&t);
    return local_time.tm_mon + 1;
}

/*****************************************************************************/

int COMTime::hour() const
{
    struct timeval tv;
    struct tm local_time;
    time_t t;

    tv = to_tv();
    t = tv.tv_sec;
    local_time = *localtime(&t);
    return local_time.tm_hour;
}

/*****************************************************************************/

int COMTime::min() const
{
    struct timeval tv;
    struct tm local_time;
    time_t t;

    tv = to_tv();
    t = tv.tv_sec;
    local_time = *localtime(&t);
    return local_time.tm_min;
}

/*****************************************************************************/

int COMTime::sec() const
{
    struct timeval tv;
    struct tm local_time;
    time_t t;

    tv = to_tv();
    t = tv.tv_sec;
    local_time = *localtime(&t);
    return local_time.tm_sec;
}

/*****************************************************************************/

int COMTime::day() const
{
    struct timeval tv;
    struct tm local_time;
    time_t t;

    tv = to_tv();
    t = tv.tv_sec;
    local_time = *localtime(&t);
    return local_time.tm_mday;
}


/*****************************************************************************/

int COMTime::day_of_week() const
{
    struct timeval tv;
    struct tm local_time;
    time_t t;

    tv = to_tv();
    t = tv.tv_sec;
    local_time = *localtime(&t);
    return local_time.tm_wday;
}

/*****************************************************************************/

bool COMTime::is_leap_year() const
{
    int y = year();

    return (!(y % 4) && (y % 100)) || !(y % 400);
}

/*****************************************************************************/

int COMTime::month_days() const
{
    static int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int m = month();

    return days[m - 1] + (m == 2 ? (int) is_leap_year() : 0);
}

/*****************************************************************************/

/**
   Gibt die aktuelle Zeit zurück

   \returns Aktuelle Zeit als COMTime
*/

COMTime COMTime::now()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return &tv; // Nutzt Konstruktor mit Parameter "struct timeval *"
}

/*****************************************************************************/
