#------------------------------------------------------------------------------
# $Id$
#------------------------------------------------------------------------------

initdir = $(sysconfdir)/init.d
profiledir = $(sysconfdir)/profile.d
sysdir = $(sysconfdir)/sysconfig

noinst_SCRIPTS = dls_quota.pl dls_status.pl init.sh profile.sh sysconfig

EXTRA_DIST = $(noinst_SCRIPTS)

install-data-local:
	$(mkinstalldirs) $(bindir)
	$(mkinstalldirs) $(initdir)
	$(mkinstalldirs) $(profiledir)
	$(mkinstalldirs) $(sysdir)
	$(INSTALL_SCRIPT) $(srcdir)/dls_quota.pl $(bindir)/dls_quota
	$(INSTALL_SCRIPT) $(srcdir)/dls_status.pl $(bindir)/dls_status
	$(INSTALL_SCRIPT) $(srcdir)/init.sh $(initdir)/dls
	$(INSTALL_SCRIPT) $(srcdir)/profile.sh $(profiledir)/dls.sh
	$(INSTALL_DATA) $(srcdir)/sysconfig $(sysdir)/dls

#------------------------------------------------------------------------------
