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
	revision.sh

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
