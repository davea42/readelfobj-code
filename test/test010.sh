#!/bin/sh
n=test010
base=$n.base
o="dd-irix-n32 libc.so.6  libkrb5support.so.0.1.debug"
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

