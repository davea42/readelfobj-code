#!/usr/bin/env python3
import os
import sys

# Translate the int32_t int16_t int64_t
# declarations
#
#  int32_t x; (etc)
#  cpu_type x;
#  cpu_subtype x;
#  vm_prot_t x

#becomes
#  TYP(x,4);
# and similar for int16_t and int64_t.
# using the macro in the output to be introduced
# by hand:
#   define TYP(n,l) char n[l]
#
# Replacing those macro-loader integral types.

# Our very limited use of th headers makes this
# possible and useful.

typedict = {"uint32_t":4,"uint64_t":8,
"uint16_t":2,"cpu_type_t":4,"cpu_subtype_t":4,
"vm_prot_t":4}

def countlead(l):
  ct = 0
  for c in l:
    if c == ' ':
      ct = int(ct) +1
    else:
      break
  return ct;

def do_line(fname,l):
  leadingspacecount = countlead(l)
  t = l.rstrip()
  wds = t.split()
  if len(wds) < 2:
    # blank or uninteresting
    print (t)
    return
  if not wds[0] in typedict: 
    print (t)
    return
  else:
    fldlen = typedict[wds[0]]
  n = wds[1]
  vname = n.split(";")
  if len(vname) != 2:
    print (t)
    return
  if vname[1] != '':
    print (t)
    return
  varname = vname[0]
  if len(varname) < 1:
    print (t)
    return
  for i in range(leadingspacecount):
    print("",end=" ")
  if len(wds) == 2:
    print("TYP(%s,%d);"%(varname,int(fldlen)))
  else: 
    # len > 2
    t=wds[2:]
    rest= ' '.join(t)
    print("TYP(%s,%d); %s;"%(varname,int(fldlen),rest))

def print_output(fname,lines):
  for l in lines:
     do_line(fname,l)



if __name__ == '__main__':
  if len(sys.argv) != 2:
    print("File name required")
    sys.exit(1)
  fname = sys.argv[1]  
  try:
    file = open(fname,"r")
  except IOError as message:
    print("File could not be opened: ", message)
    sys.exit(1)
  lines = file.readlines()
  file.close()
  print_output(fname,lines)

 



