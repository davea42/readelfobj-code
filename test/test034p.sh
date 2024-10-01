#!/bin/sh
n=test034p
if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
srcdir=$top_srcdir/test
df=$srcdir/testdiff.py
base=$srcdir/$n.base

o=$srcdir/kask-dwarfdump_64.exe
curdir=`pwd`
x="../src/readobjpe --sections-by-name $o"  
echo "START $n $x"
$x > junk.$n.tmp
# Emits junk.$n.tmpforbase
nin=junk.$n.tmp
nout=junk.$n.tmpforbase
$df  $base $nin "$srcdir" > junk.$n.out
if [ $? -ne 0 ]
then
  cat junk.$n.out
  echo "FAIL $n.sh results differ $base vs $nout"
  echo "To update, cp $curdir/$nout $base"
  exit 1
fi
exit 0

