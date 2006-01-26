#----------------------------------------------------------------
#
#  Makefile für DLS
#
#----------------------------------------------------------------

DLSD_OBJECTS = \
	com_time.o com_file.o com_globals.o \
	com_zlib.o com_base64.o \
	com_xml_tag.o com_xml_parser.o \
	com_channel_preset.o com_job_preset.o \
	com_real_channel.o \
	dls_job.o dls_logger.o \
	dls_proc_logger.o dls_proc_mother.o \
	dls_main.o

CTL_OBJECTS = \
	fl_grid.o \
	com_xml_tag.o com_time.o com_xml_parser.o \
	com_channel_preset.o com_job_preset.o \
	com_real_channel.o com_globals.o \
	ctl_dialog_job_edit.o ctl_dialog_job.o \
	ctl_dialog_channels.o ctl_dialog_channel.o \
	ctl_dialog_main.o ctl_main.o

VIEW_OBJECTS = \
	fl_grid.o \
	com_time.o com_file.o com_globals.o \
	com_base64.o com_zlib.o \
	com_xml_tag.o com_xml_parser.o \
	com_channel_preset.o com_job_preset.o \
	view_channel.o view_chunk.o view_view_data.o \
	view_dialog_main.o view_main.o

# DIRECTORIES

BIN = .
INSTALL = ../bin

# LIBRARIES

FLTK_INC = `fltk-config --cxxflags`
FLTK_LIB = `fltk-config --ldflags`

Z_INC = -I /usr/local/include
Z_LIB = -L /usr/local/lib -lz

# DLSD

DLSD_INC = $(Z_INC)
DLSD_LIB = $(Z_LIB)
DLSD_EXE = $(BIN)/dlsd

# CTL

CTL_INC = $(FLTK_INC)
CTL_LIB = $(FLTK_LIB) -pthread
CTL_EXE = $(BIN)/dls_ctl

# VIEW

VIEW_INC = $(FLTK_INC)
VIEW_LIB = $(FLTK_LIB) $(Z_LIB)
VIEW_EXE = $(BIN)/dls_view

#----------------------------------------------------------------

first: $(DLSD_EXE) $(CTL_EXE) $(VIEW_EXE)

all: mrproper depend $(DLSD_EXE) $(CTL_EXE) $(VIEW_EXE) doc

$(DLSD_EXE): $(DLSD_OBJECTS)
	g++ -Wall $(DLSD_OBJECTS) $(DLSD_LIB) -o $(DLSD_EXE)

$(CTL_EXE): $(CTL_OBJECTS)
	g++ -Wall $(CTL_OBJECTS) $(CTL_LIB) -o $(CTL_EXE)

$(VIEW_EXE): $(VIEW_OBJECTS)
	g++ -Wall $(VIEW_OBJECTS) $(VIEW_LIB) -o $(VIEW_EXE)

install:
	cp $(DLSD_EXE) $(CTL_EXE) $(VIEW_EXE) $(INSTALL)

doc:
	doxygen Doxyfile

# Compiler-Anweisungen ------------------------------------------

com_%.o: com_%.cpp
	g++ -c -g -Wall $< -o $@

dls_%.o: dls_%.cpp
	g++ -c -g -Wall $< -o $@

ctl_%.o: ctl_%.cpp
	g++ -c -g -Wall $(CTL_INC) $< -o $@

view_%.o: view_%.cpp
	g++ -c -g -Wall $(VIEW_INC) $< -o $@

fl_%.o: fl_%.cpp
	g++ -c -g -Wall $(FLTK_INC) $< -o $@

# Abhängigkeiten ------------------------------------------------

depend:
	(for file in *.cpp; do \
		g++ -M $(DLSD_INC) $(CTL_INC) $(VIEW_INC) $$file; \
	done) > .depend

include .depend

# Clean ---------------------------------------------------------

clean:
	rm -f *.o $(DLSD_EXE) $(CTL_EXE) $(VIEW_EXE)

mrproper: clean
	echo "" > .depend

#----------------------------------------------------------------
