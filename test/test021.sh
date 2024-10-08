#!/bin/sh
n=test021
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

# 64bit SPARCV9
o=$srcdir/sparc64-64-tls.o
cmd="--all"
x="../src/readelfobj $cmd  $o"
echo "START $n $x"
$x > junk.$n.tmp

nin=junk.$n.tmp
nout=junk.$n.tmpforbase
$df  $base $nin "$srcdir" > junk.$n.out
if [ $? -ne 0 ]
then
  head -30 junk.$n.out
  echo "FAIL $n.sh results differ $base vs $nout"
  echo "To update, cp $curdir/$nout $base"
  exit 1
fi
exit 0

