#!/bin/sh
n=test003
if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
srcdir=$top_srcdir/test
base=$srcdir/$n.base
o=$srcdir/libkrb5support.so.0.1.debug
curdir=`pwd`

x="../src/readelfobj --print-dynamic $o"
echo "START $n $x"
$x > junk.$n.tmp

which dos2unix >/dev/null
if [ $? -eq 0 ]
then
  dos2unix  junk.$n.tmp 2>/dev/null
fi
rm -f junkz.$n
echo sx $srcdir xyyyxg | sed s/\ //g >junkz.$n
y=`cat junkz.$n`
# This next for windows under Mingw: c: becomes /c
sed 'sxc:/x/c/xg' < junk.$n.tmp >junk.$n.tmp2
# Now the following will strip away the sourcdir part
sed $y < junk.$n.tmp2 >junk.$n

diff $base junk.$n > junk.$n.out
if [ $? -ne 0 ]
then
  cat junk.$n.out
  echo "FAIL $n.sh results differ $base vs junk.$n"
  echo "To update, mv $curdir/junk.$n $base"
  exit 1
fi
exit 0

