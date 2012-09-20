#-----------------------------------------------------------------------------
#
# $Id$
#
# Copyright (C) 2012  Florian Pose <fp@igh-essen.com>
#
# This file is part of the data logging service (DLS).
#
# The DLS is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# The DLS is distributed in the hope that it will be useful, but WITHOUT ANY
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

INCLUDEPATH += ../widgets
CONFIG += debug

unix {
    LIBS += -L../widgets -lDlsWidgets
    QMAKE_LFLAGS += -Wl,--rpath -Wl,"../widgets"
}
win32 {
    LIBS += -L"..\release"
}

HEADERS += MainWindow.h
SOURCES += main.cpp MainWindow.cpp
FORMS += MainWindow.ui

#-----------------------------------------------------------------------------
