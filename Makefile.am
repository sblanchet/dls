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

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src script

EXTRA_DIST = \
	bootstrap.sh \
	dls.spec \
	dls.spec.in \
	gui/LogWindow.cpp \
	gui/LogWindow.h \
	gui/LogWindow.ui \
	gui/MainWindow.cpp \
	gui/MainWindow.h \
	gui/MainWindow.ui \
	gui/SettingsDialog.cpp \
	gui/SettingsDialog.h \
	gui/SettingsDialog.ui \
	gui/dlsgui.pro \
	gui/dlsgui.qrc \
	gui/dlsgui_de.ts \
	gui/images/document-new.svg \
	gui/images/document-open.svg \
	gui/images/document-save-as.svg \
	gui/images/document-save.svg \
	gui/images/system-log-out.svg \
	gui/main.cpp \
	gui/modeltest.cpp \
	gui/modeltest.h \
	revision.sh \
	widgets/Channel.cpp \
	widgets/Channel.h \
	widgets/ColorDelegate.cpp \
	widgets/ColorDelegate.h \
	widgets/DatePickerDialog.cpp \
	widgets/DatePickerDialog.h \
	widgets/DatePickerDialog.ui \
	widgets/Dir.cpp \
	widgets/Dir.h \
	widgets/DlsWidgets.pro \
	widgets/DlsWidgets.qrc \
	widgets/DlsWidgets/Graph.h \
	widgets/DlsWidgets/Layer.h \
	widgets/DlsWidgets/Model.h \
	widgets/DlsWidgets/Scale.h \
	widgets/DlsWidgets/Section.h \
	widgets/DlsWidgets/Translator.h \
	widgets/DlsWidgets/ValueScale.h \
	widgets/DlsWidgets_de.ts \
	widgets/ExportDialog.cpp \
	widgets/ExportDialog.h \
	widgets/ExportDialog.ui \
	widgets/Graph.cpp \
	widgets/Job.cpp \
	widgets/Job.h \
	widgets/Layer.cpp \
	widgets/Model.cpp \
	widgets/Node.cpp \
	widgets/Node.h \
	widgets/Plugin.cpp \
	widgets/Plugin.h \
	widgets/Scale.cpp \
	widgets/Section.cpp \
	widgets/SectionDialog.cpp \
	widgets/SectionDialog.h \
	widgets/SectionDialog.ui \
	widgets/SectionModel.cpp \
	widgets/SectionModel.h \
	widgets/TODO \
	widgets/Translator.cpp \
	widgets/ValueScale.cpp \
	widgets/WidgetCollection.cpp \
	widgets/WidgetCollection.h \
	widgets/images/dialog-error.svg \
	widgets/images/dialog-information.svg \
	widgets/images/dialog-warning.svg \
	widgets/images/document-print.svg \
	widgets/images/document-properties.svg \
	widgets/images/document-save.svg \
	widgets/images/drive-harddisk.svg \
	widgets/images/edit-redo.svg \
	widgets/images/edit-undo.svg \
	widgets/images/go-next.svg \
	widgets/images/list-remove.svg \
	widgets/images/measure.svg \
	widgets/images/messages.svg \
	widgets/images/system-search.svg \
	widgets/images/utilities-system-monitor.svg \
	widgets/images/view-fullscreen.svg \
	widgets/images/view-refresh.svg \
	widgets/updateqm.pri

BUILT_SOURCES = \
	dls.spec

appdir=$(datarootdir)/applications
dist_app_DATA = \
	dls_ctl.desktop \
	dls_view.desktop

mydist:
	@REV=`hg id $(srcdir)` && \
		$(MAKE) dist-bzip2 distdir=$(PACKAGE)-$(VERSION)-$${REV}

dist-hook:
	if which hg >/dev/null 2>&1; then \
		hg id -i $(srcdir) 2>/dev/null >$(distdir)/revision; \
	fi

#------------------------------------------------------------------------------
