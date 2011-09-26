#------------------------------------------------------------------------------
# $Id$
#------------------------------------------------------------------------------

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src script

EXTRA_DIST = \
	bootstrap.sh \
	dls.spec \
	dls.spec.in \
	svnrevision.sh

BUILT_SOURCES = \
	dls.spec

mydist:
	@REV=`hg id $(srcdir)` && \
		$(MAKE) dist-bzip2 distdir=$(PACKAGE)-$(VERSION)-$${REV}

dist-hook:
	if which hg >/dev/null 2>&1; then \
		hg id $(srcdir) 2>/dev/null >$(distdir)/revision; \
	fi

#------------------------------------------------------------------------------
