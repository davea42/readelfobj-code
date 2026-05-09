#!/bin/sh
n=test035comp
if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
pass=0
fail=0
for t in comprelad32.x comprelaf32.x comprelal32.x comprelar32.x \
  comprelat32.x compshstrtab32.x compsymtab32.x comprelad.x \
  comprelaf.x comprelal.x comprelar.x comprelat.x \
  compshstrtab.x compsymtab.x
do
  srcdir=$top_srcdir/test
  df=$srcdir/testdiff.py
  base=$srcdir/$t.base
  curdir=`pwd`
  o=$srcdir/$t
  cmd="--all"
  x="../src/readelfobj $cmd $o"
  echo "START testo35comp $t $x"
  $x > junk.$t.tmp
  if [ ! -f $base ]
  then
     echo junk >$base
  fi
  $df  $base junk.$t.tmp "$srcdif" > junk.$t.out
  if [ $? -ne 0 ]
  then
    cat junk.$t.out
    echo "FAIL test on $n $o :  results differ $t.base vs junk.$t.tmp"
    echo "To update, cp $curdir/junk.$t.tmp $base"
    fail=`expr $fail + 1`
  else
    good=`expr $good + 1`
  fi
done
echo "compress tests counts good $good, fail $fail"
if [ $fail -gt 0 ]
then
  echo "Had a FAIL. in decompression"
  exit 1
fi
exit 0

