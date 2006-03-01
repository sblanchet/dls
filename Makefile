#------------------------------------------------------------------------------
#
#  DLS-Makefile
#
#  $Id$
#
#------------------------------------------------------------------------------

DLSD_OBJECTS = \
	com_globals.o com_time.o com_file.o \
	com_zlib.o com_base64.o \
	com_xml_tag.o com_xml_parser.o \
	com_channel_preset.o com_job_preset.o \
	mdct.o \
	dls_job_preset.o dls_logger.o dls_job.o \
	dls_proc_logger.o dls_proc_mother.o \
	dls_main.o

CTL_OBJECTS = \
	fl_grid.o \
	com_globals.o com_time.o \
	com_xml_tag.o  com_xml_parser.o \
	com_channel_preset.o com_job_preset.o \
	ctl_dialog_msg.o ctl_job_preset.o \
	ctl_dialog_job_edit.o ctl_dialog_job.o \
	ctl_dialog_channels.o ctl_dialog_channel.o \
	ctl_dialog_main.o \
	ctl_main.o

VIEW_OBJECTS = \
	fl_grid.o fl_track_bar.o \
	com_globals.o com_time.o com_file.o  \
	com_zlib.o com_base64.o \
	com_xml_tag.o com_xml_parser.o \
	com_channel_preset.o com_job_preset.o \
	mdct.o \
	view_data.o view_chunk.o view_channel.o \
	view_view_data.o view_view_msg.o \
	view_dialog_main.o \
	view_main.o

# DIRECTORIES

INST_DIR = /usr/local/bin
FFTW_DIR = /vol/projekte/dls_data_logging_server/soft/fftw-install

# LIBRARIES

# FLTK-Configure:
# ./configure --enable-threads --enable-xft \
#             --prefix=/vol/projekte/dls_data_logging_server/soft/fltk-1.1-install

FLTK_INC = `fltk-config --cxxflags`
FLTK_LIB = `fltk-config --ldflags`

Z_INC = -I /usr/local/include
Z_LIB = -L /usr/local/lib -lz

FFTW_INC = -I $(FFTW_DIR)/include
FFTW_LIB = -L $(FFTW_DIR)/lib -lfftw3 -lm

# DLSD

DLSD_INC = $(Z_INC)
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

SVNREV := $(shell svnversion .)

CFLAGS += -Wall -DSVNREV="$(SVNREV)"
LDFLAGS += -Wall

ifneq ($(DIST), true)
CFLAGS += -g
endif

#------------------------------------------------------------------------------

first: $(DLSD_EXE) $(CTL_EXE) $(VIEW_EXE)

all: mrproper depend install doc

dist:
	$(MAKE) DIST=true all

backup: clean
	tar -cjf ../backup/dls`date +%y%m%d`.tar.bz2 *

install: $(INST_DIR)/$(DLSD_EXE) $(INST_DIR)/$(CTL_EXE) $(INST_DIR)/$(VIEW_EXE)

uninstall:
	rm -f $(INST_DIR)/$(DLSD_EXE) $(INST_DIR)/$(CTL_EXE) $(INST_DIR)/$(VIEW_EXE)

$(DLSD_EXE): $(DLSD_OBJECTS)
	g++ $(LINKER_FLAGS) $(DLSD_OBJECTS) $(DLSD_LIB) -o $(DLSD_EXE)

$(INST_DIR)/$(DLSD_EXE): $(DLSD_EXE)
	cp $(DLSD_EXE) $(INST_DIR)

$(CTL_EXE): $(CTL_OBJECTS)
	g++ $(LINKER_FLAGS) $(CTL_OBJECTS) $(CTL_LIB) -o $(CTL_EXE)

$(INST_DIR)/$(CTL_EXE): $(CTL_EXE)
	cp $(CTL_EXE) $(INST_DIR)

$(VIEW_EXE): $(VIEW_OBJECTS)
	g++ $(LINKER_FLAGS) $(VIEW_OBJECTS) $(VIEW_LIB) -o $(VIEW_EXE)

$(INST_DIR)/$(VIEW_EXE): $(VIEW_EXE)
	cp $(VIEW_EXE) $(INST_DIR)

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

depend:
	(for file in *.cpp mdct.c; \
	   do g++ -M $(DLSD_INC) $(CTL_INC) $(VIEW_INC) $(FFTW_INC) $$file; \
	done) > .depend

ifneq ($(wildcard .depend),)
include .depend
endif

# Clean -----------------------------------------------------------------------

clean:
	rm -f *.o *~ $(DLSD_EXE) $(CTL_EXE) $(VIEW_EXE)

mrproper: clean
	echo "" > .depend

#------------------------------------------------------------------------------
