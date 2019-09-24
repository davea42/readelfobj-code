#!/bin/sh
n=test011
base=$n.base
o="libexamine-0.dll"
cmd=""
#echo "START test $n "
./object_detector  $cmd $o  >junk.$n
dos2unix  junk.$n 2>/dev/null
diff $n.base junk.$n > junk.$n.out
if [ $? -ne 0 ]
then
  cat junk.$n.out
  echo "FAIL $n.sh $cmd results differ $n.base vs junk.$n.out"
  echo "To update, mv junk.$n $n.base"
  exit 1
fi
exit 0

