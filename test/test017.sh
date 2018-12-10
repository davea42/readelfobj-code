#!/bin/sh
n=test017
base=$n.base
o=libdwarf.so.1.0.0
cmd="--all"
#echo "START test $n "
./readelfobj $cmd $o  >junk.$n
diff $n.base junk.$n > junk.$n.out
if [ $? -ne 0 ]
then
  head -30 junk.$n.out
  echo "FAIL $n.sh $cmd, results differ $n.base vs junk.$n.out"
  echo "To update, mv junk.$n $n.base"
  exit 1
fi
exit 0

