#------------------------------------------------------------------------------
# $Id$
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
