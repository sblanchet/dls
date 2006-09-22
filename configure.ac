#------------------------------------------------------------------------------
# $Id$
#------------------------------------------------------------------------------

AC_INIT([dls],[1.0],[fp@igh-essen.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign dist-bzip2])
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_CONFIG_HEADERS([config.h])
AC_PREFIX_DEFAULT([/opt/etherlab])
AC_CONFIG_FILES([Makefile src/Makefile script/Makefile])

#------------------------------------------------------------------------------
# Subversion
#------------------------------------------------------------------------------

AM_CONDITIONAL(SVNVERSION, which svnversion >/dev/null 2>&1)

#------------------------------------------------------------------------------
# FLTK
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
# FFTW3
#------------------------------------------------------------------------------

AC_ARG_WITH([fftw3-dir],
    AC_HELP_STRING(
        [--with-fftw3-dir=<fftw3-dir>],
        [FFTW3 directory [[/usr]]]),
    [ ],
    [withval=/usr])
AC_MSG_CHECKING(FFTW3 path)
test -r "$withval"/include/fftw3.h ||\
    AC_MSG_ERROR([FFTW3 not installed in $withval])
AC_SUBST(FFTW3_DIR,[$withval])
AC_MSG_RESULT($FFTW3_DIR)

#------------------------------------------------------------------------------

AC_OUTPUT

#------------------------------------------------------------------------------
