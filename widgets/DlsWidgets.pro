#-----------------------------------------------------------------------------
#
# $Id$
#
# Copyright (C) 2012  Florian Pose <fp@igh-essen.com>
#
# This file is part of the DLS widget library.
#
# The DLS widget library is free software: you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the License,
# or (at your option) any later version.
#
# The DLS widget library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
# General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with the DLS widget library. If not, see
# <http://www.gnu.org/licenses/>.
#
# vim: syntax=config
#
#-----------------------------------------------------------------------------

TEMPLATE = lib
TARGET = DlsWidgets

# On release:
# - Change version in Doxyfile
# - Change version in .spec file
# - Add a NEWS entry
VERSION = 0.9.0

CONFIG += designer plugin dll
DEPENDPATH += .
MOC_DIR = .moc
OBJECTS_DIR = .obj
QT += svg

include(updateqm.pri)

isEmpty(PREFIX) {
    unix:PREFIX = /vol/opt/etherlab
    win32:PREFIX = "c:/msys/1.0/local"
}

LIBEXT=""
unix {
    HARDWARE_PLATFORM = $$system(uname -i)
    contains(HARDWARE_PLATFORM, x86_64) {
        LIBEXT="64"
    }
}

INCLUDEPATH += . $$PWD/../src
INCLUDEPATH += $$PWD/DlsWidgets

win32 {
    QMAKE_LFLAGS += -shared
}

LIBS += -L$$OUT_PWD/../src/.libs -ldls -lfftw3 -lz

target.path = $$[QT_INSTALL_PLUGINS]/designer
INSTALLS += target

unix {
    libraries.path = $${PREFIX}/lib$${LIBEXT}
    libraries.files = libDlsWidgets.so

    INSTALLS += libraries
}

unix:inst_headers.path = $${PREFIX}/include/DlsWidgets
win32:inst_headers.path = "$${PREFIX}/include/DlsWidgets"
inst_headers.files = \
    DlsWidgets/Graph.h \
    DlsWidgets/Layer.h \
    DlsWidgets/Model.h \
    DlsWidgets/Scale.h \
    DlsWidgets/Section.h \
    DlsWidgets/Translator.h \
    DlsWidgets/ValueScale.h

INSTALLS += inst_headers

HEADERS += \
    $${inst_headers.files} \
    Channel.h \
    ColorDelegate.h \
    DatePickerDialog.h \
    Dir.h \
    ExportDialog.h \
    Job.h \
    Node.h \
    Plugin.h \
    SectionDialog.h \
    SectionModel.h \
    WidgetCollection.h

SOURCES += \
    Channel.cpp \
    ColorDelegate.cpp \
    DatePickerDialog.cpp \
    Dir.cpp \
    ExportDialog.cpp \
    Graph.cpp \
    Job.cpp \
    Layer.cpp \
    Model.cpp \
    Node.cpp \
    Plugin.cpp \
    Scale.cpp \
    Section.cpp \
    SectionDialog.cpp \
    SectionModel.cpp \
    Translator.cpp \
    ValueScale.cpp \
    WidgetCollection.cpp

FORMS = \
    DatePickerDialog.ui \
    ExportDialog.ui \
    SectionDialog.ui

RESOURCES = DlsWidgets.qrc

TRANSLATIONS = DlsWidgets_de.ts

ADDITIONAL_DISTFILES =

DISTFILES += $${ADDITIONAL_DISTFILES}

unix {
    tags.target = tags
    tags.commands = etags --members $${SOURCES} $${HEADERS}
    QMAKE_EXTRA_TARGETS += tags
}

#-----------------------------------------------------------------------------
