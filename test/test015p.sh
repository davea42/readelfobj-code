#!/bin/sh
n=test015p
base=$n.base
o=libexamine-0.dll
#echo "START test $n "
./object_detector  $o  >junk.$n
diff $n.base junk.$n > junk.$n.out
dos2unix  junk.$n 2>/dev/null
if [ $? -ne 0 ]
then
  head -30 junk.$n.out
  echo "FAIL $n.sh $cmd, results differ $n.base vs junk.$n.out"
  echo "To update, mv junk.$n $n.base"
  exit 1
fi
exit 0

