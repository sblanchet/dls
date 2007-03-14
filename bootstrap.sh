#!/bin/bash

#------------------------------------------------------------------------------
#
#  Bootstrap script for autotools
#
#  $Id$
#
#------------------------------------------------------------------------------

set -x
mkdir -p autoconf
touch ChangeLog
aclocal -I autoconf
libtoolize
#autoheader
automake --add-missing
autoconf

#------------------------------------------------------------------------------
