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

lib_LTLIBRARIES = libdls.la


#------------------------------------------------------------------------------

libdls_la_CXXFLAGS = -Wall @FFTW3_CFLAGS@ @ZLIB_CFLAGS@
libdls_la_LDFLAGS = @FFTW3_LDFLAGS@ @ZLIB_LDFLAGS@ \
	-version-info @LIBDLS_VERSION@
libdls_la_LIBADD = @FFTW3_LIBS@ @ZLIB_LIBS@ -lm

libdls_la_SOURCES = \
	Base64.cpp \
	Channel.cpp \
	ChannelPreset.cpp \
	Chunk.cpp \
	Data.cpp \
	Dir.cpp \
	Export.cpp \
	File.cpp \
	Job.cpp \
	JobPreset.cpp \
	Time.cpp \
	XmlParser.cpp \
	XmlTag.cpp \
	ZLib.cpp \
	globals.cpp \
	mdct.cpp

dlsincludedir=$(includedir)/LibDLS

dlsinclude_HEADERS = \
	Channel.h \
	Chunk.h \
	Data.h \
	Dir.h \
	Exception.h \
	Export.h \
	Job.h \
	Time.h \
	globals.h


#------------------------------------------------------------------------------

noinst_HEADERS = \
	Base64.h \
	ChannelPreset.h \
	CompressionT.h \
	File.h \
	IndexT.h \
	JobPreset.h \
	MdctT.h \
	QuantT.h \
	RingBufferT.h \
	XmlParser.h \
	XmlTag.h \
	ZLib.h \
	mdct.h

#------------------------------------------------------------------------------
