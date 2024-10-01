#!/bin/sh
n=test013m
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

# This test should fail on all as we don't allow outpath
# in the dwarf_object_detector_path call.
o="-z $srcdir/mach-o-object32 $srcdir/mach-o-object64 $srcdir/libkrb5support.so.0.1.debug"
x="../src/object_detector $cmd $o"
echo "START $n $x"
$x > junk.$n.tmp

$df $base junk.$n.tmp "$srcdir" > junk.$n.out
if [ $? -ne 0 ]
then
  cat junk.$n.out
  echo "FAIL $n.sh $cmd, results differ $base vs junk.$n.tmp"
  echo "To update, cp $curdir/junk.$n.tmp $base"
  exit 1
fi
exit 0

