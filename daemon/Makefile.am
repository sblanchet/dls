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

REV = `$(top_srcdir)/revision.sh $(top_srcdir)`

AM_CXXFLAGS = -Wall

#------------------------------------------------------------------------------

bin_PROGRAMS =

if ENABLE_DAEMON
bin_PROGRAMS += dlsd
endif

#------------------------------------------------------------------------------

dlsd_CXXFLAGS = -DREVISION=\"$(REV)\" @FFTW3_CFLAGS@ @ZLIB_CFLAGS@
dlsd_CXXFLAGS += -I$(topsrcdir)
dlsd_LDFLAGS = -L$(builddir)/../lib/.libs @FFTW3_LDFLAGS@ @ZLIB_LDFLAGS@
dlsd_LDADD = @FFTW3_LIBS@ -ldls @ZLIB_LIBS@ -lm -lpthread -lprotobuf
dlsd_DEPENDENCIES = $(builddir)/../lib/libdls.la

dlsd_SOURCES = \
	Connection.cpp \
	Job.cpp \
	JobPreset.cpp \
	Logger.cpp \
	ProcLogger.cpp \
	ProcMother.cpp \
	globals.cpp \
	$(builddir)/../proto/dls.pb.cc \
	main.cpp

#------------------------------------------------------------------------------

noinst_HEADERS = \
	Connection.h \
	Job.h \
	JobPreset.h \
	Logger.h \
	ProcLogger.h \
	ProcMother.h \
	SaverGenT.h \
	SaverMetaT.h \
	SaverT.h \
	globals.h

#------------------------------------------------------------------------------
