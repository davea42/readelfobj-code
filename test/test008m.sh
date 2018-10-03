#!/bin/sh
n=test008m
base=$n.base
o="mach-o-object32 mach-o-object64 libkrb5support.so.0.1.debug"
#echo "START test $n "
./object_detector  $o  >junk.$n
diff $n.base junk.$n > junk.$n.out
if [ $? -ne 0 ]
then
  head -30 junk.$n.out
  echo "FAIL $n.sh $cmd, results differ $n.base vs junk.$n.out"
  echo "To update, mv junk.$n $n.base"
  exit 1
fi
exit 0

