#!/bin/sh
n=test001
if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
srcdir=$top_srcdir/test
base=$n.base
curdir=`pwd`
o=$srcdir/libkrb5support.so.0.1.debug
echo "START $n test ./readelfobj --help $o "
./readelfobj --help $o >junk.$n.tmp
which dos2unix >/dev/null
if [ $? -eq 0 ]
then
  dos2unix  junk.$n.tmp >/dev/null
fi
diff  $srcdir/$n.base junk.$n.tmp > junk.$n.out
if [ $? -ne 0 ]
then
  cat junk.$n.out
  echo "FAIL $n.sh results differ $n.base vs junk.$n.tmp"
  echo "To update, mv $curdir/test/junk.$n.tmp $n.base"
  exit 1
fi
exit 0

