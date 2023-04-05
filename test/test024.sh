#!/bin/sh
n=test024
if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
top_srcdir=/home/davea/dwarf/readelfobj/code
srcdir=$top_srcdir/test
base=$srcdir/$n.base
curdir=`pwd`

o=$srcdir/elfextended/testobj.extend
cmd="--only-wasted-summary"
#echo "START test $n "
#./readelfobj $cmd $o  >junk.$n.tmp
#echo "show junk.$n.tmp"
#cat junk.$n.tmp

x="./readelfobj $cmd  $o"
echo "START $n $x"
$x > junk.$n.tmp


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
  head -30 junk.$n.out
  echo "FAIL $n.sh $cmd, results differ $base vs junk.$n.tmp"
  echo "To update, mv $curdir/junk.$n.tmp $base"
  exit 1
fi
exit 0
