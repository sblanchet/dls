/******************************************************************************
 *
 *  $Id$
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
   und Sekundenbruchteile enth�lt

   Es findet eine Rundung statt. Diese funktioniert
   nur f�r positive Zeiten (>= 1.1.1970)!

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
   Pr�ft, ob die Zeit auf Null gesetzt ist

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
   Gr��er-Operator

   \param right Zeit, mit der verglichen wird
   \returns true, wenn die Zeit des linken Operanden gr��er ist
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
   Gr��er-Gleich-Operator

   \param right Zeit, mit der verglichen wird
   \returns true, wenn die Zeit des linken Operanden gr��er
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
   \returns Referenz auf den ver�nderten ostream
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
   und Sekundenbruchteile enth�lt

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

    tv = to_tv();
    local_time = *localtime(&tv.tv_sec);
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

    tv = to_tv();
    local_time = *localtime(&tv.tv_sec);
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
   Gibt die aktuelle Zeit zur�ck

   \returns Aktuelle Zeit als COMTime
*/

COMTime COMTime::now()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return &tv; // Nutzt Konstruktor mit Parameter "struct timeval *"
}

/*****************************************************************************/
