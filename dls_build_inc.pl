#!/bin/sh

perl -i.bak -pe 'if (/^(\#.*define.*DLS_BUILD\s+)(\d*)(.*)$/) { $n = $2 + 1; $_ = "$1$n$3\n" }' dls_build.cpp
rm dls_build.cpp.bak
