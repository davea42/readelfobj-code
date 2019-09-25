#!/bin/sh
# Verify the printed output of of test_linkedto

./test_linkedtopath >junk.ltp
diff baseline.ltp junk.ltp
if [ $? -ne 0 ] 
then
    echo "FAIL base test "
    echo "To update baseline: mv junk.ltp baseline.ltp"
    exit 1
fi
exit 0
