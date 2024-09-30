#!/bin/sh
n=test026

#if [ x$DWTOPSRCDIR = "x" ]
#then
#  top_srcdir=$top_blddir
#else
#  top_srcdir=$DWTOPSRCDIR
#fi
top_srcdir=$DWTOPSRCDIR
srcdir=$top_srcdir/test
df=$srcdir/testdiff.py
base=$srcdir/$n.base
echo "srcdir $srcdir"

o=$srcdir/elfextended/testobjgnu.extend
curdir=`pwd`
cmd="--only-wasted-summary"
#echo "START test $n "
x="../src/readelfobj $cmd  $o"
echo "START $n $x"
$x > junk.$n.tmp

if [ ! -f  $base ]
then
  echo junk > $base
fi
$df  $base junk.$n.tmp "$srcdir" > junk.$n.out
if [ $? -ne 0 ]
then
  head -30 junk.$n.out
  echo "FAIL $n.sh $cmd, results differ $base vs junk.$n.tmp"
  echo "To update, cp $curdir/junk.$n.tmp $base"
  exit 1
fi
exit 0

