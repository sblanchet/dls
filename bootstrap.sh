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
automake --add-missing
autoconf

#------------------------------------------------------------------------------
