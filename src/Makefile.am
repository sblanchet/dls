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

SUBDIRS = images

REV = `$(top_srcdir)/revision.sh $(top_srcdir)`

AM_CXXFLAGS = -Wall

appdir=$(datarootdir)/applications
dist_app_DATA =

#------------------------------------------------------------------------------

bin_PROGRAMS =

if ENABLE_DAEMON
bin_PROGRAMS += dlsd
endif

if ENABLE_FLTK
bin_PROGRAMS += dls_ctl dls_view
endif

if ENABLE_TOOL
bin_PROGRAMS += dls
endif

lib_LTLIBRARIES = libdls.la

#------------------------------------------------------------------------------

libdls_la_CXXFLAGS = @FFTW3_CFLAGS@ @ZLIB_CFLAGS@
libdls_la_LDFLAGS = @FFTW3_LDFLAGS@ @ZLIB_LDFLAGS@ \
	-version-info @LIBDLS_VERSION@
libdls_la_LIBADD = @FFTW3_LIBS@ @ZLIB_LIBS@ -lm

libdls_la_SOURCES = \
	com_base64.cpp \
	com_channel_preset.cpp \
	com_file.cpp \
	com_globals.cpp \
	com_job_preset.cpp \
	com_time.cpp \
	com_xml_parser.cpp \
	com_xml_tag.cpp \
	com_zlib.cpp \
	lib_channel.cpp \
	lib_chunk.cpp \
	lib_common.cpp \
	lib_data.cpp \
	lib_dir.cpp \
	lib_export.cpp \
	lib_job.cpp \
	mdct.cpp

include_HEADERS = \
	com_exception.hpp \
	com_time.hpp \
	com_job_preset.hpp \
	com_channel_preset.hpp \
	lib_channel.hpp \
	lib_chunk.hpp \
	lib_data.hpp \
	lib_dir.hpp \
	lib_export.hpp \
	lib_globals.hpp \
	lib_job.hpp

#------------------------------------------------------------------------------

dlsd_CXXFLAGS = -DREVISION=\"$(REV)\" @FFTW3_CFLAGS@ @ZLIB_CFLAGS@
dlsd_LDFLAGS = -L$(builddir)/.libs @FFTW3_LDFLAGS@ @ZLIB_LDFLAGS@
dlsd_LDADD = @FFTW3_LIBS@ -ldls @ZLIB_LIBS@ -lm
dlsd_DEPENDENCIES = libdls.la

dlsd_SOURCES = \
	dls_globals.cpp \
	dls_job.cpp \
	dls_job_preset.cpp \
	dls_logger.cpp \
	dls_main.cpp \
	dls_proc_logger.cpp \
	dls_proc_mother.cpp

#------------------------------------------------------------------------------

dls_ctl_CXXFLAGS = -fPIE -DREVISION=\"$(REV)\" @FLTK_CPPFLAGS@
dls_ctl_LDFLAGS = -pie -L$(builddir)/.libs -pthread
dls_ctl_LDADD = @FLTK_LDFLAGS@ -ldls
dls_ctl_DEPENDENCIES = libdls.la

if ENABLE_FLTK
install-exec-hook:
	chmod +s $(DESTDIR)$(bindir)/dls_ctl
endif

dls_ctl_SOURCES = \
	ctl_dialog_channel.cpp \
	ctl_dialog_channels.cpp \
	ctl_dialog_job.cpp \
	ctl_dialog_job_edit.cpp \
	ctl_dialog_main.cpp \
	ctl_dialog_msg.cpp \
	ctl_job_preset.cpp \
	ctl_main.cpp \
	fl_grid.cpp

dist_app_DATA += \
	dls_ctl.desktop

#------------------------------------------------------------------------------

dls_view_CXXFLAGS = -DREVISION=\"$(REV)\" @FLTK_CPPFLAGS@ \
	@FFTW3_CFLAGS@ @ZLIB_CFLAGS@
dls_view_LDFLAGS = -L$(builddir)/.libs @FFTW3_LDFLAGS@ @ZLIB_LDFLAGS@ -pthread
dls_view_LDADD = @FLTK_LDFLAGS@ -ldls @FFTW3_LIBS@ -lm @ZLIB_LIBS@
dls_view_DEPENDENCIES = libdls.la

dls_view_SOURCES = \
	fl_grid.cpp \
	fl_track_bar.cpp \
	view_channel.cpp \
	view_dialog_export.cpp \
	view_dialog_main.cpp \
	view_main.cpp \
	view_view_data.cpp \
	view_view_msg.cpp

dist_app_DATA += \
	dls_view.desktop

#------------------------------------------------------------------------------

dls_CXXFLAGS = -DREVISION=\"$(REV)\" @FFTW3_CFLAGS@ @ZLIB_CFLAGS@
dls_LDFLAGS = -L$(builddir)/.libs @FFTW3_LDFLAGS@ @ZLIB_LDFLAGS@ -pthread
dls_LDADD = -ldls @FFTW3_LIBS@ -lm @ZLIB_LIBS@
dls_DEPENDENCIES = libdls.la

dls_SOURCES = \
	cmd_export.cpp \
	cmd_list.cpp \
	cmd_main.cpp

#------------------------------------------------------------------------------

noinst_HEADERS = \
	com_base64.hpp \
	com_compression_t.hpp \
	com_file.hpp \
	com_globals.hpp \
	com_index_t.hpp \
	com_mdct_t.hpp \
	com_quant_t.hpp \
	com_ring_buffer_t.hpp \
	com_xml_parser.hpp \
	com_xml_tag.hpp \
	com_zlib.hpp \
	ctl_dialog_channel.hpp \
	ctl_dialog_channels.hpp \
	ctl_dialog_job.hpp \
	ctl_dialog_job_edit.hpp \
	ctl_dialog_main.hpp \
	ctl_dialog_msg.hpp \
	ctl_globals.hpp \
	ctl_job_preset.hpp \
	dls_globals.hpp \
	dls_job.hpp \
	dls_job_preset.hpp \
	dls_logger.hpp \
	dls_proc_logger.hpp \
	dls_proc_mother.hpp \
	dls_saver_gen_t.hpp \
	dls_saver_meta_t.hpp \
	dls_saver_t.hpp \
	fl_grid.hpp \
	fl_grid.hpp \
	fl_track_bar.hpp \
	mdct.hpp \
	view_channel.hpp \
	view_dialog_export.hpp \
	view_dialog_main.hpp \
	view_globals.hpp \
	view_view_data.hpp \
	view_view_msg.hpp

#------------------------------------------------------------------------------
