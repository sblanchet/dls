//---------------------------------------------------------------
//
//  C O M _ T I M E . H P P
//
//---------------------------------------------------------------

#ifndef COMTimeHpp
#define COMTimeHpp

//---------------------------------------------------------------

#include <sys/time.h>

#include <ostream>
using namespace std;

//---------------------------------------------------------------

/**
   Datentyp zur Speicherung der Zeit in Mikrosekunden

   Dieser Datentyp verwaltet einen long long integer
   zur Speicherung der Mikrosekunden nach epoch.
*/

class COMTime
{
  friend ostream &operator <<(ostream &, const COMTime &);

public:
  COMTime();
  COMTime(long long);
  COMTime(double);
  COMTime(struct timeval);

  void from_dbl_time(double);

  //  COMTime &operator =(int);
  COMTime &operator =(long long);
  COMTime &operator =(double);
  COMTime &operator =(struct timeval);

  bool operator ==(const COMTime &) const;
  bool operator !=(const COMTime &) const;
  bool operator <(const COMTime &) const;
  bool operator >(const COMTime &) const;
  bool operator <=(const COMTime &) const;
  bool operator >=(const COMTime &) const;

  COMTime operator +(const COMTime &) const;
  COMTime &operator +=(const COMTime &);
  COMTime operator -(const COMTime &) const;
  COMTime operator *(long long) const;
  
  double to_dbl() const;
  long long to_ll() const;
  string to_str() const;
  struct timeval to_tv() const;
  string to_real_time() const;

  const COMTime &set_now();
  static COMTime now();

private:
  long long _time; /**< Mikrosekunden nach epoch */
};

//---------------------------------------------------------------

ostream &operator <<(ostream &, const COMTime &);

//---------------------------------------------------------------

#endif
