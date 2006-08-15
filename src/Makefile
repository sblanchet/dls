#------------------------------------------------------------------------------
#
#  DLS-Makefile
#
#  $Id$
#
#------------------------------------------------------------------------------

DLSD_OBJECTS := \
	com_globals.o com_time.o com_file.o \
	com_zlib.o com_base64.o \
	com_xml_tag.o com_xml_parser.o \
	com_channel_preset.o com_job_preset.o \
	mdct.o \
	dls_job_preset.o dls_logger.o dls_job.o \
	dls_proc_logger.o dls_proc_mother.o \
	dls_main.o

CTL_OBJECTS := \
	fl_grid.o \
	com_globals.o com_time.o \
	com_xml_tag.o  com_xml_parser.o \
	com_channel_preset.o com_job_preset.o \
	ctl_dialog_msg.o ctl_job_preset.o \
	ctl_dialog_job_edit.o ctl_dialog_job.o \
	ctl_dialog_channels.o ctl_dialog_channel.o \
	ctl_dialog_main.o \
	ctl_main.o

VIEW_OBJECTS := \
	fl_grid.o fl_track_bar.o \
	com_globals.o com_time.o com_file.o  \
	com_zlib.o com_base64.o \
	com_xml_tag.o com_xml_parser.o \
	com_channel_preset.o com_job_preset.o \
	mdct.o \
	view_data.o view_chunk.o view_channel.o \
	view_view_data.o view_view_msg.o \
	view_dialog_main.o view_dialog_export.o \
	view_main.o

FLTK_INC := $(shell ../../soft/fltk-1.1-install/bin/fltk-config --cxxflags)
FLTK_LIB := $(shell ../../soft/fltk-1.1-install/bin/fltk-config --ldflags)
Z_INC := -I/usr/local/include
Z_LIB := -L/usr/local/lib -lz
FFTW_INC := -I../../soft/fftw-install/include
FFTW_LIB := -L../../soft/fftw-install/lib -lfftw3 -lm

# DLSD
DLSD_INC = $(Z_INC) $(FFTW_INC)
DLSD_LIB = $(Z_LIB) $(FFTW_LIB)
DLSD_EXE = dlsd

# CTL
CTL_INC = $(FLTK_INC)
CTL_LIB = $(FLTK_LIB) -pthread
CTL_EXE = dls_ctl

# VIEW
VIEW_INC = $(FLTK_INC)
VIEW_LIB = $(FLTK_LIB) $(Z_LIB) $(FFTW_LIB)
VIEW_EXE = dls_view

# FLAGS
SVNREV := $(shell svnversion . || echo "unknown")

CFLAGS += -Wall -DSVNREV="$(SVNREV)"
LDFLAGS += -Wall

#------------------------------------------------------------------------------

all: .depend $(DLSD_EXE) $(CTL_EXE) $(VIEW_EXE)

$(DLSD_EXE): $(DLSD_OBJECTS)
	g++ $(LDFLAGS) $(DLSD_OBJECTS) $(DLSD_LIB) -o $(DLSD_EXE)

$(CTL_EXE): $(CTL_OBJECTS)
	g++ $(LDFLAGS) $(CTL_OBJECTS) $(CTL_LIB) -o $(CTL_EXE)

$(VIEW_EXE): $(VIEW_OBJECTS)
	g++ $(LDFLAGS) $(VIEW_OBJECTS) $(VIEW_LIB) -o $(VIEW_EXE)

doc:
	doxygen Doxyfile

# Compiler-Anweisungen --------------------------------------------------------

com_%.o: com_%.cpp
	g++ -c $(CFLAGS) $< -o $@

dls_%.o: dls_%.cpp
	g++ -c $(CFLAGS) $< -o $@

ctl_%.o: ctl_%.cpp
	g++ -c $(CFLAGS) $(CTL_INC) $< -o $@

view_%.o: view_%.cpp
	g++ -c $(CFLAGS) $(VIEW_INC) $< -o $@

fl_%.o: fl_%.cpp
	g++ -c $(CFLAGS) $(FLTK_INC) $< -o $@

mdct.o: mdct.c
	g++ -c $(CFLAGS) $(FFTW_INC) $< -o $@

# Abhängigkeiten --------------------------------------------------------------

.depend dep:
	(for file in *.cpp mdct.c; \
		do g++ -M $(FFTW_INC) $(FLTK_INC) $(Z_INC) $$file; \
	done) > .depend

ifneq ($(wildcard .depend),)
include .depend
endif

# Clean -----------------------------------------------------------------------

clean:
	@rm -f *.o *~ $(DLSD_EXE) $(CTL_EXE) $(VIEW_EXE) .depend core

#------------------------------------------------------------------------------
