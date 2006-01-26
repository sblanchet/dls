#!/bin/sh

perl -i.bak -pe 'if (/^(\#.*define.*CTL_BUILD\s+)(\d*)(.*)$/) {$n = $2 + 1; $_ = "$1$n$3\n"}' ctl_build.cpp
rm ctl_build.cpp.bak
