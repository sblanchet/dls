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

#------------------------------------------------------------------------------

bin_PROGRAMS =

if ENABLE_TOOL
bin_PROGRAMS += dlsjson
endif

#------------------------------------------------------------------------------

dlsjson_CXXFLAGS = -Wall -DREVISION=\"$(REV)\" @FFTW3_CFLAGS@ @ZLIB_CFLAGS@

dlsjson_LDFLAGS = \
	-L$(top_builddir)/lib/.libs \
	@FFTW3_LDFLAGS@ \
	@ZLIB_LDFLAGS@ \
	-pthread

dlsjson_LDADD = \
	-ldls \
	@FFTW3_LIBS@ \
	-lm \
	@ZLIB_LIBS@ \
	$(XML_LIBS) \
	-lprotobuf \
	-ljson-c

dlsjson_DEPENDENCIES = $(top_builddir)/lib/libdls.la

dlsjson_SOURCES = \
	Data.cpp \
	List.cpp \
	ErrorCodes.cpp \
	main.cpp

#------------------------------------------------------------------------------
