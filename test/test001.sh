#!/bin/sh
n=test001
base=$n.base
o=libkrb5support.so.0.1.debug
#echo "START test $n "
./readelfobj --help $o >junk.$n
dos2unix junk.$n
diff  $n.base junk.$n  > junk.$n.out
if [ $? -ne 0 ]
then
  cat junk.$n.out
  echo "FAIL $n.sh results differ $n.base vs junk.$n.out"
  echo "To update, mv junk.$n $n.base"
  exit 1
fi
exit 0

