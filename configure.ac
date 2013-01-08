#------------------------------------------------------------------------------
#
# $Id$
#
# This file is part of the Data Logging Service (DLS).
#
# DLS is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# DLS is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along
# with DLS. If not, see <http://www.gnu.org/licenses/>.
#
#------------------------------------------------------------------------------

AC_INIT([dls],[1.2.7],[fp@igh-essen.com])
AC_CONFIG_AUX_DIR([autoconf])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror dist-bzip2])

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_CXX
AC_PROG_RANLIB
AC_PREFIX_DEFAULT([/opt/etherlab])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([config.h.in])

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
    AC_MSG_ERROR([fltk-config was not found.\
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

AC_DEFINE([BINDIR], [], [Fully expanded binary directory.])
AX_DEFINE_DIR([BINDIR], [bindir])

#------------------------------------------------------------------------------

AC_CONFIG_FILES([
    Makefile
    dls.spec
    script/Makefile
    script/init.sh
    src/Makefile
])
AC_OUTPUT

#------------------------------------------------------------------------------
