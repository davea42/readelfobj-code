#!/bin/sh
# Verify the printed output of of test_linkedto

top_blddir=`pwd`/..
if [ x$DWTOPSRCDIR = "x" ]
then
  top_srcdir=$top_blddir
else
  top_srcdir=$DWTOPSRCDIR
fi
srcdir=$top_srcdir/src
curdir=$top_blddir/test

echo "Run test debuglink: ./test_linkedtopath"
./test_linkedtopath >junk.ltp
which dos2unix >/dev/null
if [ $? -eq 0 ]
then
  # It's only needed on Windows
  dos2unix junk.ltp
fi
diff $srcdir/baseline.ltp junk.ltp
if [ $? -ne 0 ] 
then
    echo "FAIL base test "
    echo "To update baseline: mv $curdir/junk.ltp $srcdir/baseline.ltp"
    exit 1
fi
exit 0
