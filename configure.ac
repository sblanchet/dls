#------------------------------------------------------------------------------
# $Id$
#------------------------------------------------------------------------------

AC_INIT([dls],[1.0],[fp@igh-essen.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign dist-bzip2])
AC_PROG_CXX
AC_PROG_RANLIB
AC_CONFIG_HEADERS([config.h])
AC_PREFIX_DEFAULT([/opt/etherlab])
AC_CONFIG_FILES([Makefile src/Makefile])

#------------------------------------------------------------------------------
# EtherCAT
#------------------------------------------------------------------------------

AC_ARG_WITH([fltk-dir],
    AC_HELP_STRING(
        [--with-fltk-dir=<fltk-dir>],
        [FLTK directory [[/usr/local]]]),
    [ ],
    [
        withval=/usr/local
    ])
AC_MSG_CHECKING(FLTK path)
test -x "$withval"/bin/fltk-config ||\
    AC_MSG_ERROR([FLTK not installed in $withval])
AC_SUBST(FLTK_DIR,[$withval])
AC_MSG_RESULT($FLTK_DIR)
FLTK_CPPFLAGS=$($FLTK_DIR/bin/fltk-config --cxxflags)
AC_SUBST(FLTK_CPPFLAGS)
FLTK_LDFLAGS=$($FLTK_DIR/bin/fltk-config --ldflags)
AC_SUBST(FLTK_LDFLAGS)

#------------------------------------------------------------------------------
# FFTW
#------------------------------------------------------------------------------

AC_ARG_WITH([fftw-dir],
    AC_HELP_STRING(
        [--with-fftw-dir=<fftw-dir>],
        [FFTW3 directory [[/usr/local]]]),
    [ ],
    [
        withval=/usr/local
    ])
AC_MSG_CHECKING(FFTW path)
test -r "$withval"/include/fftw3.h ||\
    AC_MSG_ERROR([FFTW3 not installed in $withval])
AC_SUBST(FFTW_DIR,[$withval])
AC_MSG_RESULT($FFTW_DIR)

#------------------------------------------------------------------------------

AC_OUTPUT

#------------------------------------------------------------------------------
