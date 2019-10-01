#!/bin/sh
n=test002
if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
srcdir=$top_srcdir/test

base=$n.base

o=$srcdir/libkrb5support.so.0.1.debug
#Version-readelfobj:
#echo "START test $n "
./readelfobj --version   >junk.$n
dos2unix  junk.$n 2>/dev/null
grep 'Version-readelfobj:' <junk.$n >junk.$n.out
if [ $? -eq 0 ]
then
  #Version string found
  exit 0
fi
echo "FAIL $srcdir/$n.sh found no version string in junk.$n"
exit 1

