#!/bin/sh

perl -i.bak -pe 'if (/^(\#.*define.*VIEW_BUILD\s+)(\d*)(.*)$/) {$n = $2 + 1; $_ = "$1$n$3\n";}' view_build.cpp
rm view_build.cpp.bak
