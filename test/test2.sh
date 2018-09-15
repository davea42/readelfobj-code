#!/bin/sh
n=test2
base=$n.base
o=libkrb5support.so.0.1.debug
#Version-readelfobj:
#echo "START test $n "
./readelfobj --version   >junk.$n
grep 'Version-readelfobj:' <junk.$n >junk.$n.out
if [ $? -eq 0 ]
then
  #Version string found
  exit 0
fi
echo "FAIL $n.sh found no version string in junk.$n"
exit 1

