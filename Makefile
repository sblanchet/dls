#----------------------------------------------------------------
#
#  Makefile f�r DLS
#
#----------------------------------------------------------------

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

BIN = .
INSTALL = ../bin

# LIBRARIES

# FLTK-Configure:
# ./configure --enable-threads --enable-xft --prefix=/vol/projekte/dls_data_logging_server/soft/fltk-1.1-install

FLTK_INC = `fltk-config --cxxflags`
FLTK_LIB = `fltk-config --ldflags`

Z_INC = -I /usr/local/include
Z_LIB = -L /usr/local/lib -lz

FFTW_DIR = /vol/projekte/dls_data_logging_server/soft/fftw-install

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

COMPILER_FLAGS := -Wall
LINKER_FLAGS := -Wall
BUILD_FLAGS := -D BUILDER=$(USER)

ifneq ($(DIST), true)
COMPILER_FLAGS += -g
BUILD_FLAGS += -D DEBUG_INFO
endif

#----------------------------------------------------------------

first: normal

normal: $(BIN)/$(DLSD_EXE) $(BIN)/$(CTL_EXE) $(BIN)/$(VIEW_EXE)

all: mrproper depend install doc

dist: 
	$(MAKE) DIST=true all

backup: clean
	tar -cjf ../backup/dls`date +%y%m%d`.tar.bz2 *

install: $(INSTALL)/$(DLSD_EXE) $(INSTALL)/$(CTL_EXE) $(INSTALL)/$(VIEW_EXE)

uninstall:
	rm -f $(INSTALL)/$(DLSD_EXE) $(INSTALL)/$(CTL_EXE) $(INSTALL)/$(VIEW_EXE)

$(BIN)/$(DLSD_EXE): $(DLSD_OBJECTS)
	./dls_build_inc
	g++ -c $(BUILD_FLAGS) dls_build.cpp -o dls_build.o
	g++ $(LINKER_FLAGS) $(DLSD_OBJECTS) dls_build.o $(DLSD_LIB) -o $(BIN)/$(DLSD_EXE)

$(INSTALL)/$(DLSD_EXE): $(BIN)/$(DLSD_EXE)
	cp $(BIN)/$(DLSD_EXE) $(INSTALL)

$(BIN)/$(CTL_EXE): $(CTL_OBJECTS)
	./ctl_build_inc
	g++ -c $(BUILD_FLAGS) ctl_build.cpp -o ctl_build.o
	g++ $(LINKER_FLAGS) $(CTL_OBJECTS) ctl_build.o $(CTL_LIB) -o $(BIN)/$(CTL_EXE)

$(INSTALL)/$(CTL_EXE): $(BIN)/$(CTL_EXE)
	cp $(BIN)/$(CTL_EXE) $(INSTALL)

$(BIN)/$(VIEW_EXE): $(VIEW_OBJECTS)
	./view_build_inc
	g++ -c $(BUILD_FLAGS) view_build.cpp -o view_build.o
	g++ $(LINKER_FLAGS) $(VIEW_OBJECTS) view_build.o $(VIEW_LIB) -o $(BIN)/$(VIEW_EXE)

$(INSTALL)/$(VIEW_EXE): $(BIN)/$(VIEW_EXE)
	cp $(BIN)/$(VIEW_EXE) $(INSTALL)

doc:
	doxygen Doxyfile

# Compiler-Anweisungen ------------------------------------------

com_%.o: com_%.cpp
	g++ -c $(COMPILER_FLAGS) $< -o $@

dls_%.o: dls_%.cpp
	g++ -c $(COMPILER_FLAGS) $< -o $@

ctl_%.o: ctl_%.cpp
	g++ -c $(COMPILER_FLAGS) $(CTL_INC) $< -o $@

view_%.o: view_%.cpp
	g++ -c $(COMPILER_FLAGS) $(VIEW_INC) $< -o $@

fl_%.o: fl_%.cpp
	g++ -c $(COMPILER_FLAGS) $(FLTK_INC) $< -o $@

mdct.o: mdct.c
	g++ -c $(COMPILER_FLAGS) $(FFTW_INC) $< -o $@

# Abh�ngigkeiten ------------------------------------------------

depend:
	(for file in *.cpp mdct.c; \
	   do g++ -M $(DLSD_INC) $(CTL_INC) $(VIEW_INC) $(FFTW_INC) $$file; \
	done) > .depend

include .depend

# Clean ---------------------------------------------------------

clean:
	rm -f *.o $(DLSD_EXE) $(CTL_EXE) $(VIEW_EXE)

mrproper: clean
	echo "" > .depend

#----------------------------------------------------------------
