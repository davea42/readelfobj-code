/*
Copyright (c) 2019, David Anderson
All rights reserved.

Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:

    Redistributions of source code must retain the above
    copyright notice, this list of conditions and the following
    disclaimer.

    Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  */

#include "config.h"
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif /* HAVE_MALLOC_H */
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_ELF_H
#include <elf.h>
#endif /* HAVE_ELF_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#include <sys/types.h> /* for open() */
#include <sys/stat.h> /* for open() */
#include <fcntl.h> /* for open() */
#include <errno.h>
#include "dwarf_reading.h"
#include "dwarf_object_detector.h"
#include "dwarf_object_read_common.h"
#include "readelfobj.h"
#include "dwarfstring.h"
#include "dwarf_debuglink.h"


static int errcount = 0;

static void
check_svalid(int expret,int gotret,int experr,int goterr,int line,
    char *filename)
{

   if (expret != gotret) {
       errcount++;
       printf("ERROR expected return %d, got %d line %d %s\n",
           expret,gotret,line,filename);
   }
   if (experr != goterr) {
       errcount++;
       printf("ERROR expected errcode %d, got %d line %d %s\n",
           experr,goterr,line,filename);
   }
}

static void
test1(void) 
{
      char testbuffer[1000];
      char *area = testbuffer;
      char * str = testbuffer;
      const char *msg = "This is a simple string for testing.";
      int res = 0;
      char *end = testbuffer +100;
      testbuffer[0] = 0;
      int errcode = 0;


      strcpy(testbuffer,msg);
      /* The error value is arbitrary, not realistic. */
      res = _dwarf_check_string_valid(area,str,
         end,DW_DLE_CORRUPT_GNU_DEBUGID_STRING,
         &errcode);
      check_svalid(DW_DLV_OK,res,
          0,errcode,
          __LINE__,__FILE__);

      end = testbuffer +10;
      res = _dwarf_check_string_valid(area,str,
         end,DW_DLE_STRING_NOT_TERMINATED,
         &errcode);
      check_svalid(DW_DLV_ERROR, res, 
         DW_DLE_STRING_NOT_TERMINATED, errcode,
         __LINE__,__FILE__);

      end = testbuffer +10;
      area = end +2;
      res = _dwarf_check_string_valid(area,str,
         end,DW_DLE_CORRUPT_GNU_DEBUGID_STRING,
         &errcode);
      check_svalid(DW_DLV_ERROR,res,
         DW_DLE_CORRUPT_GNU_DEBUGID_STRING, errcode,
         __LINE__,__FILE__);

}

static void
checkjoin(int expret,int gotret,char*expstr,char*gotstr,
    int line,
    const char *filename)
{
   if (expret != gotret) {
       errcount++;
       printf("ERROR expected return %d, got %d line %d %s\n",
           expret,gotret,line,filename);
   }
   if (strcmp(expstr,gotstr)) {
       errcount++;
       printf("ERROR expected string \"%s\", got \"%s\" line %d %s\n",
           expstr,gotstr,line,filename);
   }
}


#if 0
int
_dwarf_pathjoinl(dwarfstring *target,dwarfstring * input);
#endif
static void
test2(void)
{
     dwarfstring targ;
     dwarfstring inp;
     int res = 0;

     dwarfstring_constructor(&targ);
     dwarfstring_constructor(&inp);

     dwarfstring_append(&targ,"/a/b");
     dwarfstring_append(&inp,"foo");
     res = _dwarf_pathjoinl(&targ,&inp);
     checkjoin(DW_DLV_OK,res,"/a/b/foo",dwarfstring_string(&targ),
          __LINE__,__FILE__);
    
     dwarfstring_reset(&targ);
     dwarfstring_append(&targ,"gef");
     res = _dwarf_pathjoinl(&targ,&inp);
     checkjoin(DW_DLV_OK,res,"gef/foo",dwarfstring_string(&targ),
          __LINE__,__FILE__);

     dwarfstring_reset(&targ);
     dwarfstring_reset(&inp);
     dwarfstring_append(&targ,"gef/");
     dwarfstring_append(&inp,"/jkl/");
     res = _dwarf_pathjoinl(&targ,&inp);
     checkjoin(DW_DLV_OK,res,"gef/jkl/",dwarfstring_string(&targ),
          __LINE__,__FILE__);

     dwarfstring_reset(&targ);
     dwarfstring_reset(&inp);
     dwarfstring_append(&targ,"gef/");
     dwarfstring_append(&inp,"jkl/");
     res = _dwarf_pathjoinl(&targ,&inp);
     checkjoin(DW_DLV_OK,res,"gef/jkl/",dwarfstring_string(&targ),
          __LINE__,__FILE__);

     dwarfstring_reset(&targ);
     dwarfstring_reset(&inp);
     dwarfstring_append(&targ,"gef");
     dwarfstring_append(&inp,"jkl/");
     res = _dwarf_pathjoinl(&targ,&inp);
     checkjoin(DW_DLV_OK,res,"gef/jkl/",dwarfstring_string(&targ),
          __LINE__,__FILE__);

     dwarfstring_reset(&targ);
     dwarfstring_reset(&inp);
     dwarfstring_append(&inp,"/jkl/");
     res = _dwarf_pathjoinl(&targ,&inp);
     checkjoin(DW_DLV_OK,res,"/jkl/",dwarfstring_string(&targ),
          __LINE__,__FILE__);

     dwarfstring_reset(&targ);
     dwarfstring_reset(&inp);
     dwarfstring_append(&inp,"jkl/");
     res = _dwarf_pathjoinl(&targ,&inp);
     checkjoin(DW_DLV_OK,res,"jkl/",dwarfstring_string(&targ),
          __LINE__,__FILE__);

     dwarfstring_reset(&targ);
     dwarfstring_reset(&inp);
     dwarfstring_append(&targ,"jkl");
     dwarfstring_append(&inp,"pqr/");
     res = _dwarf_pathjoinl(&targ,&inp);
     checkjoin(DW_DLV_OK,res,"jkl/pqr/",dwarfstring_string(&targ),
          __LINE__,__FILE__);

     dwarfstring_reset(&targ);
     dwarfstring_reset(&inp);
     dwarfstring_append(&targ,"/");
     dwarfstring_append(&inp,"/");
     res = _dwarf_pathjoinl(&targ,&inp);
     checkjoin(DW_DLV_OK,res,"/",dwarfstring_string(&targ),
          __LINE__,__FILE__);


     dwarfstring_destructor(&targ);
     dwarfstring_destructor(&inp);
}


#if 0
void
_dwarf_construct_linkedto_path(char *pathname,
   char * input_link_string, /* incoming link string */
   dwarfstring * debuglink_out);
#endif



int main()
{

    test1();

    test2();

    if (errcount) { 
        return 1;
    }
    return 0;
}
