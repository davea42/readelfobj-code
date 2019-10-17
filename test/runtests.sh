#!/bin/sh

top_blddir=`pwd`/..
if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
srcdir=$top_srcdir/test
loc=`pwd`
ct=0
if [ ! -x $top_blddir/src/readelfobj ]
then
   echo "Build readelfobj and install it as $loc/../src/readelfobj to test"
   echo "No test done"
   exit 1
fi
sh $top_srcdir/test/PICKUP $top_blddir
if [ $? -ne 0 ]
then
  echo "FAIL executing $top_srcdir/test/PICKUP in test/runtest.sh"
  exit 1
fi
. $top_srcdir/SHALIAS.sh
fail=0
for i in $srcdir/test[0-9]*.sh
do
  sh $i
  if [ $? -ne 0 ]
  then
     ct=`expr $ct + 1`  
  fi
done

if [ $ct -gt 0 ]
then
  echo "FAIL. COUNT: $ct"
  exit 1
fi
echo "PASS." 
exit 0
