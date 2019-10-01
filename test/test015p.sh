#!/bin/sh
n=test015p
if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
srcdir=$top_srcdir/test
base=$srcdir/$n.base

o=$srcdir/libexamine-0.dll
#echo "START test $n "
./object_detector  $o  >junk.$n.tmp
diff $base junk.$n.tmp > junk.$n.out

rm -f junkz
echo sx $srcdir xyyyx | sed s/\ //g >junkz
y=`cat junkz`
sed $y < junk.$n.tmp >junk.$n
rm -f junkz

dos2unix  junk.$n 2>/dev/null
if [ $? -ne 0 ]
then
  head -30 junk.$n.out
  echo "FAIL $n.sh $cmd, results differ $base vs junk.$n.out"
  echo "To update, mv junk.$n $base"
  exit 1
fi
exit 0

