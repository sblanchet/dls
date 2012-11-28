#-----------------------------------------------------------------------------
#
# $Id$
#
# Copyright (C) 2012  Florian Pose <fp@igh-essen.com>
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

INCLUDEPATH += ../widgets ../src
CONFIG += debug

unix {
    LIBS += -L$$PWD/../widgets -lDlsWidgets
    LIBS += -L$$PWD/../src/.libs -ldls
    LIBS += -lfftw3 -lm -lz
    QMAKE_LFLAGS += -Wl,--rpath -Wl,"$$OUT_PWD/../src/.libs"
    QMAKE_LFLAGS += -Wl,--rpath -Wl,"$$OUT_PWD/../widgets"
}
win32 {
    LIBS += -L"$$OUT_PWD\..\widgets\release"
}

HEADERS += MainWindow.h
SOURCES += main.cpp MainWindow.cpp
FORMS += MainWindow.ui

RESOURCES += dlsgui.qrc

HEADERS += modeltest.h
SOURCES += modeltest.cpp

#-----------------------------------------------------------------------------
