#!/bin/sh
#
# Output information about parent revision to TeX-includable file.
#

temp='\\def\\revision\{{node|short}}
\\def\\isodate#1-#2-#3 #4:#5 #6x\{
  \\day  = #3
  \\month= #2
  \\year = #1
}
\\isodate {date|isodate}x
'

OUT=hg.tex

LANG=C hg parent --template "$temp" > $OUT

echo Updated $OUT
