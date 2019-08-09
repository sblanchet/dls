# About

DLS is a nice Data Logging Service from Etherlab
https://www.etherlab.org/en/dls

This work is based on the original mecurial repository
http://hg.code.sf.net/p/dls/code

* The original mercurial repository has been cloned and converted to git
with `hg-fast-export`
```bash
git clone https://github.com/frej/fast-export.git hg-fast-export
hg clone http://hg.code.sf.net/p/dls/code dls-hg
git init dls-git && cd dls-git
../hg-fast-export.sh -r ../dls-hg && git checkout HEAD
```
* Then I have started to develop my own improvements in a new branch.
* Sometimes my patches may be accepted and merged in the main mercurial repository
* I will try to synchronize regularly with the original code.
* The last synchronisation occurs on 2019-08-09 with mecurial revision 874:a55aae466861
