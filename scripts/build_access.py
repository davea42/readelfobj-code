#!/usr/bin/env python3
import os
import sys

# Turn a set of  #define X 12
# and so on into
# an array of
#ifndef ELF_EM_VALUES_H
#endif
#struct rel_values {
#   const char *rel_name;
#   unsigned long rel_number;
#};
# with functions to get the strings from the vals
# written to a .c file
#  dwarf_elf_rel_xxx.c
# with function dwarf_get_elf_rel_xxx

#and a set of protected defines (put in .h)
##ifndef X
##define X 12
##endif /* X */
#  dwarf_elf_rel_xxx.h
# declare function dwarf_get_elf_rel_xxx()


def alldig(s):
  for c in s:
    if ord(c) < ord('0') or ord(c) > ord('9'):
       return "n" 
  return "y"


def do_line(fname,l,hfile,cfile):
  t = l.rstrip()
  if t.startswith("#define") == 0:
     return;
  wds = t.split()
  if len(wds) != 3:
    print("ERROR. Improper define input",wds);
    sys.exit(1)
    return
  
  n=wds[1]
  v=wds[2]
  if alldig(v) == "n":
     print("dadebug skipping t")
     # redef. We just use one name, not the alternates
     return

  print("#ifndef",n,file=hfile) 
  fmt1= "#define %-20s %s"%(n,v)
  print(fmt1,file=hfile) 
  fmt2= "#endif /* %s */"%n
  print(fmt2,file=hfile)


  rstr = 'return "%s";'%n
  print("    case",n,": "+rstr,file=cfile) 
  print(rstr,file=cfile)




def write_output(fname,lines,type,outprefix):
  outhname = outprefix+"_"+type+".h"
  outcname = outprefix+"_"+type+".c"
  print("dadebug ",outhname,outcname)
  try:
    hfile = open(outhname,"w")
  except IOError as message:
    print("File could not be opened for write: ", message)
    sys.exit(1)
  try:
    cfile = open(outcname,"w")
  except IOError as message:
    print("File could not be opened for write: ", message)
    sys.exit(1)

  print("/* Created by build_access.py */",file=hfile)
  funcname="dwarf_get_elf_relocname_" + type
  print("const char * "+funcname+ "(unsigned long);",file=hfile)

  print("/* Created by build_access.py */",file=cfile)
  print('#include "%s"'%outhname,file=cfile)
  print("",file=cfile)

  print("const char * ",file=cfile)
  print(funcname + "(unsigned long val)",file=cfile)
  print("{",file=cfile)
  print("    switch(val) {",file=cfile)

  ct = 0
  for l in lines:
     ct = int(ct) +1
     do_line(fname,l,hfile,cfile)
     if int(ct) > 3:
       print(" Terminating on count",file=hfile)
       print(" Terminating on count",file=cfile)
       hfile.close()
       cfile.close()
       print(" Terminating on count")
       sys.exit(1)
  print("    }",file=cfile)
  print('return "";',file=cfile)

  print("}",file=cfile)
  cfile.close()
  hfile.close()


# usage  ./build_access.py <input.h> <typename> <filename>
# as in 
# ./build_access.py  raw_elf_definesdwarf_reloc_ppc.h ppc dwarf_elf_reloc_ppc
if __name__ == '__main__':
  if len(sys.argv) != 4:
    print("infile, type, outprefix required")
    sys.exit(1)
  fname = sys.argv[1] 
  type = sys.argv[2]
  outprefix = sys.argv[3]
  
  try:
    file = open(fname,"r")
  except IOError as message:
    print("File could not be opened: ", message)
    sys.exit(1)
  lines = file.readlines()
  file.close()
  write_output(fname,lines,type,outprefix)

 



