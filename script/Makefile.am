#------------------------------------------------------------------------------
# $Id$
#------------------------------------------------------------------------------

initdir = $(sysconfdir)/init.d
profiledir = $(sysconfdir)/profile.d
sysdir = $(sysconfdir)/sysconfig

noinst_SCRIPTS = dls_quota.pl dls_status.pl init.sh profile.sh sysconfig
EXTRA_DIST = $(noinst_SCRIPTS)

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	$(mkinstalldirs) $(DESTDIR)$(initdir)
	$(mkinstalldirs) $(DESTDIR)$(profiledir)
	$(mkinstalldirs) $(DESTDIR)$(sysdir)
	$(INSTALL_SCRIPT) $(srcdir)/dls_quota.pl $(DESTDIR)$(bindir)/dls_quota
	$(INSTALL_SCRIPT) $(srcdir)/dls_status.pl $(DESTDIR)$(bindir)/dls_status
	$(INSTALL_SCRIPT) $(srcdir)/init.sh $(DESTDIR)$(initdir)/dls
	$(INSTALL_SCRIPT) $(srcdir)/profile.sh $(DESTDIR)$(profiledir)/dls.sh
	$(INSTALL_DATA) $(srcdir)/sysconfig $(DESTDIR)$(sysdir)/dls

#------------------------------------------------------------------------------
