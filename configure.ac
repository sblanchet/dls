#------------------------------------------------------------------------------
# $Id$
#------------------------------------------------------------------------------

AC_INIT([dls],[1.0],[fp@igh-essen.com])
AM_INIT_AUTOMAKE([-Wall -Werror dist-bzip2])
AC_PROG_CXX
AC_PROG_RANLIB
AC_PREFIX_DEFAULT([/opt/etherlab])

#------------------------------------------------------------------------------
# FLTK
#------------------------------------------------------------------------------

AC_ARG_WITH([fltk-dir],
    AC_HELP_STRING(
        [--with-fltk-dir=<fltk-dir>],
        [FLTK directory [[/usr/local]]]),
    [
	FLTKDIR=$withval
	if test -n "$FLTKDIR"; then
            AC_MSG_CHECKING(for FLTK path)
            test -x "$FLTKDIR/bin/fltk-config" ||\
	        AC_MSG_ERROR([FLTK not installed in $FLTKDIR])
            AC_MSG_RESULT($FLTKDIR)
	fi
    ],
    [
        FLTKDIR=""
    ])

AC_PATH_PROG([FLTKCONF], [fltk-config],, [$FLTKDIR/bin:$PATH])

if test -z "$FLTKCONF"; then
    AC_MSG_WARN([fltk-config was not found.\
                 Please use --with-fltk-dir.]);
fi

FLTK_CPPFLAGS=$($FLTKCONF --cxxflags)
AC_SUBST(FLTK_CPPFLAGS)

FLTK_LDFLAGS=$($FLTKCONF --ldflags)
AC_SUBST(FLTK_LDFLAGS)

#------------------------------------------------------------------------------
# FFTW3
#------------------------------------------------------------------------------

AC_MSG_CHECKING(FFTW3 path)

AC_ARG_WITH([fftw3-dir],
    AC_HELP_STRING(
        [--with-fftw3-dir=<fftw3-dir>],
        [FFTW3 directory [[/usr/local:/usr]]]),
    [
        test -r "$withval"/include/fftw3.h ||\
            AC_MSG_ERROR([FFTW3 not installed in $withval])
    ],
    [
	if test -r /usr/local/include/fftw3.h; then
	    withval=/usr/local
	elif test -r /usr/include/fftw3.h; then
            withval=/usr
        else
            AC_MSG_ERROR([FFTW3 not installed in /usr/local or /usr!])
        fi
    ])

AC_SUBST(FFTW3_DIR,[$withval])
AC_MSG_RESULT($FFTW3_DIR)

#------------------------------------------------------------------------------
# ZLib
#------------------------------------------------------------------------------

AC_ARG_WITH([zlib-dir],
    AC_HELP_STRING(
        [--with-zlib-dir=<zlib-dir>],
        [ZLib directory [[/usr]]]),
    [
	    ZLIB_DIR=$withval
    ],
    [
        ZLIB_DIR="/usr"
    ])

AC_MSG_CHECKING(for ZLib)
test -r "${ZLIB_DIR}/include/zlib.h" ||
    AC_MSG_ERROR([ZLib header not found in ${ZLIB_DIR}])

AC_SUBST(ZLIB_DIR)
AC_MSG_RESULT(${ZLIB_DIR})

#------------------------------------------------------------------------------

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	script/Makefile
])
AC_OUTPUT

#------------------------------------------------------------------------------
