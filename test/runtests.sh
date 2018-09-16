#!/bin/sh

ct=0
if [ ! -x readelfobj ]
then
   echo "Build readelfobj and install it here to test"
   echo "No test done"
   exit 1
fi

fail=0
for i in test[0-9]*.sh
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
