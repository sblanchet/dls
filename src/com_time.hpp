/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef COMTimeHpp
#define COMTimeHpp

/*****************************************************************************/

#include <sys/time.h>
#include <stdint.h>

#include <ostream>
using namespace std;

/*****************************************************************************/

/**
   Datentyp zur Speicherung der Zeit in Mikrosekunden

   Dieser Datentyp verwaltet einen uint64_t integer
   zur Speicherung der Mikrosekunden nach epoch.
*/

class COMTime
{
    friend ostream &operator <<(ostream &, const COMTime &);

public:
    COMTime();
    COMTime(uint64_t);
    COMTime(double);
    COMTime(struct timeval *);
    COMTime(struct tm *, unsigned int);

    void from_dbl_time(double);
    void set_null();
    void set_now();

    //  COMTime &operator =(int);
    COMTime &operator =(uint64_t);
    COMTime &operator =(double);
    COMTime &operator =(struct timeval);

    bool operator ==(const COMTime &) const;
    bool operator !=(const COMTime &) const;
    bool operator <(const COMTime &) const;
    bool operator >(const COMTime &) const;
    bool operator <=(const COMTime &) const;
    bool operator >=(const COMTime &) const;
    bool is_null() const;

    COMTime operator +(const COMTime &) const;
    COMTime &operator +=(const COMTime &);
    COMTime operator -(const COMTime &) const;
    COMTime operator *(uint64_t) const;

    double to_dbl() const;
    double to_dbl_time() const;
    uint64_t to_uint64() const;
    string to_str() const;
    struct timeval to_tv() const;
    string to_real_time() const;
    string format_time(const char *) const;
    string to_rfc811_time() const;
    string diff_str_to(const COMTime &) const;

    static COMTime now();

private:
    uint64_t _time; /**< Mikrosekunden nach epoch */
};

/*****************************************************************************/

ostream &operator <<(ostream &, const COMTime &);

/*****************************************************************************/

#endif
