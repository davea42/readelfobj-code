#!/bin/sh
n=test023
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
o=$srcdir/frame1-frame1.o

cmd="--all"
x="../src/object_detector $cmd  $o"
echo "START $n $x"
$x > junk.$n.tmp

$df $base junk.$n.tmp "$srcdir" > junk.$n.out
if [ $? -ne 0 ]
then
  head -30 junk.$n.out
  echo "FAIL $n.sh $cmd, results differ $base junk.$n.tmp"
  echo "To update, mv $curdir/junk.$n.tmp $base"
  exit 1
fi
exit 0

