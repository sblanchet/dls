#------------------------------------------------------------------------------
# $Id$
#------------------------------------------------------------------------------

SUBDIRS = src script

EXTRA_DIST = doku/dls_doku.pdf

mydist:
	@SVNREV=`svnversion $(srcdir)` && \
		$(MAKE) dist-bzip2 distdir=$(PACKAGE)-$(VERSION)-r$${SVNREV}

dist-hook:
	if which svnversion >/dev/null 2>&1; then \
		svnversion $(srcdir) 2>/dev/null >$(distdir)/svnrevision; \
	fi

#------------------------------------------------------------------------------
