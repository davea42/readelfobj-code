#!/bin/sh
n=test014m
base=$n.base
# This test should fail on all as we don't allow outpath
# in the dwarf_object_detector_path call.
o=kask2/dwarfdump_G4
#echo "START test $n "
./object_detector  $o  >junk.$n
dos2unix  junk.$n 2>/dev/null
diff $n.base junk.$n > junk.$n.out
if [ $? -ne 0 ]
then
  head -30 junk.$n.out
  echo "FAIL $n.sh $cmd, results differ $n.base vs junk.$n.out"
  echo "To update, mv junk.$n $n.base"
  exit 1
fi
exit 0

