#!/bin/sh
n=test001
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
o=$srcdir/libkrb5support.so.0.1.debug
echo "START $n test ./readelfobj --help $o "
../src/readelfobj --help $o >junk.$n.tmp
$df  $srcdir/$n.base junk.$n.tmp "$srcdif" > junk.$n.out
if [ $? -ne 0 ]
then
  cat junk.$n.out
  echo "FAIL $n.sh results differ $n.base vs junk.$n.tmp"
  echo "To update, cp $curdir/junk.$n.tmp $base"
  exit 1
fi
exit 0
