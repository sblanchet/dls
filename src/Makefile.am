#------------------------------------------------------------------------------
# $Id$
#------------------------------------------------------------------------------

SVNREV = `$(top_srcdir)/svnrevision.sh $(top_srcdir)`

#------------------------------------------------------------------------------

bin_PROGRAMS = dlsd dls_ctl dls_view dls
noinst_LTLIBRARIES = libcomdls.la libdls.la

#------------------------------------------------------------------------------

libcomdls_la_CPPFLAGS = -I@FFTW3_DIR@/include -I@ZLIB_DIR@/include

libcomdls_la_SOURCES = \
	com_globals.cpp com_globals.hpp \
	com_exception.hpp \
	com_compression_t.hpp \
	com_index_t.hpp \
	com_mdct_t.hpp \
	com_quant_t.hpp \
	com_ring_buffer_t.hpp \
	com_time.cpp com_time.hpp \
	com_file.cpp com_file.hpp \
	com_zlib.cpp com_zlib.hpp \
	com_base64.cpp com_base64.hpp \
	com_xml_tag.cpp com_xml_tag.hpp \
	com_xml_parser.cpp com_xml_parser.hpp \
	com_channel_preset.cpp com_channel_preset.hpp \
	com_job_preset.cpp com_job_preset.hpp \
	mdct.cpp mdct.hpp

#------------------------------------------------------------------------------

libdls_la_SOURCES = \
	lib_dir.cpp lib_dir.hpp \
	lib_job.cpp lib_job.hpp \
	lib_channel.cpp lib_channel.hpp \
	lib_chunk.cpp lib_chunk.hpp \
	lib_data.cpp lib_data.hpp \
	lib_export.cpp lib_export.hpp

#------------------------------------------------------------------------------

dlsd_CPPFLAGS = -DSVNREV=$(SVNREV)
dlsd_LDFLAGS = -L@FFTW3_DIR@/lib -L@ZLIB_DIR@/lib
dlsd_LDADD = libcomdls.la -lfftw3 -lm -lz

dlsd_SOURCES = \
	dls_globals.hpp \
	dls_job_preset.cpp dls_job_preset.hpp \
	dls_logger.cpp dls_logger.hpp \
	dls_job.cpp dls_job.hpp \
	dls_saver_t.hpp \
	dls_saver_gen_t.hpp \
	dls_saver_meta_t.hpp \
	dls_proc_logger.cpp dls_proc_logger.hpp \
	dls_proc_mother.cpp dls_proc_mother.hpp \
	dls_main.cpp

#------------------------------------------------------------------------------

dls_ctl_CPPFLAGS = -DSVNREV=$(SVNREV) @FLTK_CPPFLAGS@
dls_ctl_LDADD = libcomdls.la @FLTK_LDFLAGS@

install-exec-hook:
	chmod +s $(bindir)/dls_ctl

dls_ctl_SOURCES = \
	fl_grid.cpp fl_grid.hpp \
	ctl_globals.hpp \
	ctl_dialog_msg.cpp ctl_dialog_msg.hpp \
	ctl_job_preset.cpp ctl_job_preset.hpp \
	ctl_dialog_job_edit.cpp ctl_dialog_job_edit.hpp \
	ctl_dialog_job.cpp ctl_dialog_job.hpp \
	ctl_dialog_channels.cpp ctl_dialog_channels.hpp \
	ctl_dialog_channel.cpp ctl_dialog_channel.hpp \
	ctl_dialog_main.cpp ctl_dialog_main.hpp \
	ctl_main.cpp

#------------------------------------------------------------------------------

dls_view_CPPFLAGS = -DSVNREV=$(SVNREV) @FLTK_CPPFLAGS@
dls_view_LDFLAGS = -L@FFTW3_DIR@/lib -L@ZLIB_DIR@/lib
dls_view_LDADD = libdls.la libcomdls.la @FLTK_LDFLAGS@ -lfftw3 -lm -lz

dls_view_SOURCES = \
	fl_grid.cpp fl_grid.hpp \
	fl_track_bar.cpp fl_track_bar.hpp \
	view_globals.hpp \
	view_channel.cpp view_channel.hpp \
	view_view_data.cpp view_view_data.hpp \
	view_view_msg.cpp view_view_msg.hpp \
	view_dialog_main.cpp view_dialog_main.hpp \
	view_dialog_export.cpp view_dialog_export.hpp \
	view_main.cpp

#------------------------------------------------------------------------------

dls_CPPFLAGS = -DSVNREV=$(SVNREV)
dls_LDFLAGS = -L@FFTW3_DIR@/lib -L@ZLIB_DIR@/lib
dls_LDADD = libdls.la libcomdls.la -lfftw3 -lm -lz

dls_SOURCES = \
	cmd_list.cpp cmd_export.cpp cmd_main.cpp

#------------------------------------------------------------------------------
