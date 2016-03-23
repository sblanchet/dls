#-----------------------------------------------------------------------------
#
# $Id$
#
# Copyright (C) 2012 - 2013  Florian Pose <fp@igh-essen.com>
#
# This file is part of the data logging service (DLS).
#
# DLS is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# DLS is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
# more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with the DLS. If not, see <http://www.gnu.org/licenses/>.
#
# vim: tw=78 syntax=config
#
#-----------------------------------------------------------------------------

TEMPLATE = app
TARGET = dlsgui
DEPENDPATH += .

INCLUDEPATH += $$PWD/../widgets $$PWD/../lib
DEPENDPATH += $$PWD/../widgets $$PWD/../lib
QT += svg

include(../widgets/updateqm.pri)

isEmpty(PREFIX) {
    unix:PREFIX = /vol/opt/etherlab
    win32:PREFIX = "c:/msys/1.0/local"
}

unix {
    CONFIG += debug
    LIBS += $$PWD/../widgets/libDlsWidgets.so
    LIBS += $$PWD/../lib/.libs/libdls.so
    LIBS += -lfftw3 -lxml2 -lm -lz
    QMAKE_LFLAGS += -Wl,--rpath -Wl,"../lib/.libs"
    QMAKE_LFLAGS += -Wl,--rpath -Wl,"../widgets"
}
win32 {
    CONFIG += release
    LIBS += -L$$PWD/../widgets/release -lDlsWidgets0
    LIBS += -L$$PWD/../lib/.libs -ldls
    LIBS += -lfftw3 -lxml2 -lm -lz
}

target.path = $$PREFIX/bin
INSTALLS += target

HEADERS += \
    AboutDialog.h \
    LogWindow.h \
    MainWindow.h \
    SettingsDialog.h \
    UriDialog.h \

SOURCES += \
    AboutDialog.cpp \
    LogWindow.cpp \
    MainWindow.cpp \
    SettingsDialog.cpp \
    UriDialog.cpp \
    main.cpp

FORMS += \
    AboutDialog.ui \
    LogWindow.ui \
    MainWindow.ui \
    SettingsDialog.ui \
    UriDialog.ui

RESOURCES += dlsgui.qrc

RC_FILE = dlsgui.rc

TRANSLATIONS = dlsgui_de.ts

mimetype.path = $$PREFIX/share/mime/packages
mimetype.files = dlsgui-mime.xml

desktop.path = $$PREFIX/share/applications
desktop.files = dlsgui.desktop

icons_svg.path = $$PREFIX/share/icons/hicolor/scalable/apps
icons_svg.files = images/dlsgui.svg images/dlsgui-view.svg

icons_png.path = $$PREFIX/share/icons/hicolor/32x32/apps
icons_png.files = images/dlsgui.png images/dlsgui-view.png

INSTALLS += mimetype desktop icons_svg icons_png

#HEADERS += modeltest.h
#SOURCES += modeltest.cpp

#-----------------------------------------------------------------------------
