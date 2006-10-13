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
aclocal -I autoconf
libtoolize
#autoheader
automake --add-missing
autoconf

#------------------------------------------------------------------------------
