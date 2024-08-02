#!/bin/sh
n=test003
# on  the s390, the letter x appears
# in the pathname so use z as the sed marker, not x.
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

x="../src/readelfobj --print-dynamic $o"
echo "START $n $x"
$x > junk.$n.tmp

$df  $base junk.$n.tmp "$srcdir" > junk.$n.out
if [ $? -ne 0 ]
then
  cat junk.$n.out
  echo "FAIL $n.sh results differ $base vs junk.$n"
  echo "To update, mv $curdir/junk.$n $base"
  exit 1
fi
exit 0

