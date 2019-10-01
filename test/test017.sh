#!/bin/sh
n=test017
if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
srcdir=$top_srcdir/test
base=$srcdir/$n.base
o=$srcdir/libdwarf.so.1.0.0
cmd="--all"
#echo "START test $n "
./readelfobj $cmd $o  >junk.$n.tmp
dos2unix  junk.$n.tmp 2>/dev/null

rm -f junkz 
echo sx $srcdir xyyyxg | sed s/\ //g >junkz
y=`cat junkz`
# This next for windows under Mingw: c: becomes /c
sed 'sxc:/x/c/xg' < junk.$n.tmp >junk.$n.tmp2
# Now the following will strip away the sourcdir part
sed $y < junk.$n.tmp2 >junk.$n
rm -f junkz 

diff $base junk.$n > junk.$n.out
if [ $? -ne 0 ]
then
  head -30 junk.$n.out
  echo "FAIL $n.sh $cmd, results differ $base junk.$n"
  echo "To update, mv junk.$n $base"
  exit 1
fi
exit 0

