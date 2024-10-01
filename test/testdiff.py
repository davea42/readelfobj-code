#!/usr/bin/env python3

# Run as:
# dwdiff.py baseline newfile testsrcdir

import os
import sys
import difflib

def removeccolon(linein,srcdir):
    # This is for msys2, sometimes names look like Windows
    l2= linein.replace("c:/","/c")
    # This is to strip away parts of pathnames that differ 
    # and replace with the simple "yyy"
    l3= l2.replace(srcdir,"yyy")
    return l3
def writebackfile(text,name):
    newn =  ''.join([name,"forbase"])
    try:
        f = open(newn, "w")
    except:
        print("Unable to open newn ", newn, " giving up")
        sys.exit(1)
    for t in text:
        f.write(''.join([t,'\n']))
    f.close()

def readin(srcfile,srcdir):
    hasdos = False
    try:
        f = open(srcfile, "r")
    except:
        print("Unable to open input conf ", srcfile, " giving up")
        sys.exit(1)
    y = f.readlines()
    out = []
    for l in y:
        if l.endswith("\r\n"):
            hasdos = True
        if srcdir:
            # is the test here slower than
            # just doing the remove/replace?
            l2=removeccolon(l,srcdir)
        else:
            l2 = l
        out += [l2.rstrip()]
    f.close()
    return hasdos, out



if __name__ == "__main__":
    origfile = False
    newfile = False
    srcdir = False
    if len(sys.argv) > 3:
        origfile = sys.argv[1]
        newfile = sys.argv[2]
        srcdir=   sys.argv[3]
    else:
        print("dwdiff.py args required: baseline newfile testsrcdir")
        exit(1)
    #  The origfile should have yyy instances, not full paths.
    hasdos, olines = readin(origfile,False)
    hasdos, nlines = readin(newfile,srcdir)
    # diffs = difflib.unified_diff(olines,nlines,lineterm='')
    diffs = difflib.context_diff(
        olines, nlines, lineterm="", fromfile=origfile, tofile=newfile
    )
    used = False
    for s in diffs:
        print("There are differences origfile:", origfile)
        print("                              :", newfile)
        used = True
        break
    if used:
        print(
            "Line Count Base=",
            len(olines),
            " Line Count Test=",
            len(nlines),
        )
        for s in diffs:
            print(s)
        writebackfile(nlines,newfile)
        sys.exit(1)
    sys.exit(0)
