#!/bin/sh
n=test022
if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
srcdir=$top_srcdir/test
df=$srcdir/testdiff.py
base=$srcdir/$n.base
curdir=`pwd`

# gcc on mingw, windows 8.1
# Intel 80386 COFF object file, not valid PE
o=$srcdir/frame1-frame1.o
x="../src/readobjpe $o"
echo "START $n $x"
$x > junk.$n.tmp

# Emits junk.$n.tmpforbase
nin=junk.$n.tmp
nout=junk.$n.tmpforbase
$df $base $nin "$srcdir" > junk.$n.out
if [ $? -ne 0 ]
then
  head -30 junk.$n.out
  echo "FAIL $n.sh $cmd, results differ $base $nout"
  echo "To update, cp $curdir/$nout $base"
  exit 1
fi
exit 0

