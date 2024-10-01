#!/bin/sh
df=srcdir/testdiff.py
n=test004
if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
srcdir=$top_srcdir/test
df=$srcdir/testdiff.py
base=$srcdir/$n.base
o=$srcdir/libkrb5support.so.0.1.debug
curdir=`pwd`

cmd="--print-relocs"
x="../src/readelfobj $cmd $o"
echo "START $n $x"
$x > junk.$n.tmp

# Emits junk.$n.tmpforbase
nin=junk.$n.tmp
nout=junk.$n.tmpforbase
$df $base $nin "$srcdir" > junk.$n.out
if [ $? -ne 0 ]
then
  cat junk.$n.out
  echo "FAIL $n.sh $cmd results differ $base vs $nout"
  echo "To update, cp $curdir/$nout $base"
  exit 1
fi
exit 0

