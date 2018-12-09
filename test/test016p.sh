#!/bin/sh
n=test016p
base=$n.base
o=libexamine-0.dll
#echo "START test $n "
./readobjpe  $o  >junk.$n
diff $n.base junk.$n > junk.$n.out
if [ $? -ne 0 ]
then
  head -30 junk.$n.out
  echo "FAIL $n.sh $cmd, results differ $n.base vs junk.$n.out"
  echo "To update, mv junk.$n $n.base"
  exit 1
fi
exit 0
