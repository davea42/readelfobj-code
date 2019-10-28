#!/bin/sh
n=test010
if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
srcdir=$top_srcdir/test
base=$srcdir/$n.base
o="$srcdir/dd-irix-n32 $srcdir/libc.so.6 $srcdir/libkrb5support.so.0.1.debug"
cmd=""
#echo "START test $n "
./object_detector  $cmd $o  >junk.$n.tmp
which dos2unix >/dev/null
if [ $? -eq 0 ]
then
  dos2unix  junk.$n.tmp 2>/dev/null
fi
rm -f junkz
echo sx $srcdir xyyyxg | sed s/\ //g >junkz
y=`cat junkz`
# This next for windows under Mingw: c: becomes /c
sed 'sxc:/x/c/xg' < junk.$n.tmp >junk.$n.tmp2
# Now the following will strip away the sourcdir part
sed $y < junk.$n.tmp2 >junk.$n.tmp
rm -f junkz 

diff $base junk.$n.tmp > junk.$n.out
if [ $? -ne 0 ]
then
  cat junk.$n.out
  echo "FAIL $n.sh $cmd results differ $base vs junk.$n.tmp"
  echo "To update, mv junk.$n.tmp $base"
  exit 1
fi
exit 0

