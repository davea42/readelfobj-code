#!/bin/sh
n=test011

if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
srcdir=$top_srcdir/test
base=$srcdir/$n.base
o="$srcdir/libexamine-0.dll"

cmd=""
#echo "START test $n "
./object_detector  $cmd $o  >junk.$n.tmp
dos2unix  junk.$n.tmp 2>/dev/null

rm -f junkz
echo sx $srcdir xyyyx | sed s/\ //g >junkz
y=`cat junkz`
sed $y < junk.$n.tmp >junk.$n
rm -f junkz

diff $base junk.$n > junk.$n.out
if [ $? -ne 0 ]
then
  cat junk.$n.out
  echo "FAIL $n.sh $cmd results differ $base vs junk.$n.out"
  echo "To update, mv junk.$n $base"
  exit 1
fi
exit 0

