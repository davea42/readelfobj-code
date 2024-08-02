#!/bin/sh
n=test011

if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
srcdir=$top_srcdir/test
df=$srcdir/testdiff.py
base=$srcdir/$n.base
o="$srcdir/libexamine-0.dll"

curdir=`pwd`

cmd=""
#echo "START test $n "
x="../src/object_detector $cmd $o"
echo "START $n $x"
$x > junk.$n.tmp

$df $base junk.$n.tmp "$srcdir" > junk.$n.out
if [ $? -ne 0 ]
then
  cat junk.$n.out
  echo "FAIL $n.sh $cmd results differ $base vs junk.$n.tmp"
  echo "To update, mv $curdir/junk.$n.tmp $base"
  exit 1
fi
exit 0

