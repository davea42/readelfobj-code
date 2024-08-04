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
    hasdos, olines = readin(origfile,False)
    hasdos, nlines = readin(newfile,srcdir)
    # diffs = difflib.unified_diff(olines,nlines,lineterm='')
    diffs = difflib.context_diff(
        olines, nlines, lineterm="", fromfile=origfile, tofile=newfile
    )
    used = False
    for s in diffs:
        print("There are differences.")
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
        sys.exit(1)
    sys.exit(0)
