#!/bin/sh
n=test021
base=$n.base
# 64bit SPARCV9
o=sparc64-64-tls.o
cmd="--all"
#echo "START test $n "
./readelfobj $cmd $o  >junk.$n
dos2unix -q junk.$n
diff $n.base junk.$n > junk.$n.out
if [ $? -ne 0 ]
then
  head -30 junk.$n.out
  echo "FAIL $n.sh $cmd, results differ $n.base junk.$n"
  echo "To update, mv junk.$n $n.base"
  exit 1
fi
exit 0

