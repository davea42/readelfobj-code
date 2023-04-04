#!/bin/sh
#binpatch is is a simple binary patch program.
#echo "Do not run this, it is already done"
#exit 1
#cp testobj64.baseobj testobj64.extend
#binpatch 0x3c 0x2300 0x00ff ./testobj64.extend
#binpatch 0x3e 0x2200 0xffff ./testobj64.extend
#hxdump -l 200 ./testobj64.extend
#binpatch 0x39b8 0x0000000000000000 0x2300000000000000 ./testobj64.extend
#binpatch 0x39c0 0x00000000 0x22000000 ./testobj64.extend
#
#hxdump -l 200 ./testobj64.extend  >junk64.new
#hxdump -l 200 -s 0x3990 ./testobj64.extend >>junk64.new

cp testobj.baseobj testobj.extend

# These patches only apply to testobj.baseobj
binpatch 0x30 0x2300 0x00ff ./testobj.extend
binpatch 0x32 0x2200 0xffff ./testobj.extend
binpatch 0x38e4 0x00000000 0x23000000 ./testobj.extend
binpatch 0x38e8 0x00000000 0x22000000 ./testobj.extend

hxdump -l 200 ./testobj.extend  >junk.new
hxdump -l 100 -s 0x3990 ./testobj.extend >>junk.new



