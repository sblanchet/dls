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
	@SVNREV=`svnversion $(srcdir)` && \
		$(MAKE) dist-bzip2 distdir=$(PACKAGE)-$(VERSION)-r$${SVNREV}

dist-hook:
	if which svnversion >/dev/null 2>&1; then \
		svnversion $(srcdir) 2>/dev/null >$(distdir)/svnrevision; \
	fi

#------------------------------------------------------------------------------
