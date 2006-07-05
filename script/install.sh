#!/bin/sh

#------------------------------------------------------------------------------
#
#  DLS install script
#
#  $Id$
#
#------------------------------------------------------------------------------

CONFIGFILE=/etc/sysconfig/dls
INITSCRIPT=/etc/init.d/dls
RCLINK=/usr/sbin/rcdls
DLSSCRIPT=/usr/local/bin/dls_status
QUOTASCRIPT=/usr/local/bin/dls_quota

if [ -s $CONFIGFILE ]; then
    echo "  Note: Using existing configuration file."
else
    echo "  Creating $CONFIGFILE"
    cp script/sysconfig $CONFIGFILE || exit 1
fi

echo "  Installing startup script"
cp script/rcdls.sh $INITSCRIPT || exit 1
chmod +x $INITSCRIPT || exit 1
if [ ! -L $RCLINK ]; then
    ln -s $INITSCRIPT $RCLINK || exit 1
fi

echo "  Installing tools"
cp script/dls_status.pl $DLSSCRIPT || exit 1
chmod +x $DLSSCRIPT || exit 1
cp script/dls_quota.pl $QUOTASCRIPT || exit 1
chmod +x $QUOTASCRIPT || exit 1

echo "  Installing binaries"
cp src/dlsd /usr/local/bin || exit 1
cp src/dls_ctl /usr/local/bin || exit 1
chmod +s /usr/local/bin/dls_ctl || exit 1
cp src/dls_view /usr/local/bin || exit 1

#------------------------------------------------------------------------------
