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
    char *filename_in)
{

    if (expret != gotret) {
        errcount++;
        printf("ERROR expected return %d, got %d line %d %s\n",
            expret,gotret,line,filename_in);
    }
    if (experr != goterr) {
        errcount++;
        printf("ERROR expected errcode %d, got %d line %d %s\n",
            experr,goterr,line,filename_in);
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
    int errcode = 0;


    testbuffer[0] = 0;
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
    const char *filename_in)
{
    if (expret != gotret) {
        errcount++;
        printf("ERROR expected return %d, got %d line %d %s\n",
            expret,gotret,line,filename_in);
    }
    if (strcmp(expstr,gotstr)) {
        errcount++;
        printf("ERROR expected string \"%s\", got \"%s\" line %d %s\n",
            expstr,gotstr,line,filename_in);
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
    checkjoin(DW_DLV_OK,res,"/a/b/foo",
        dwarfstring_string(&targ),
        __LINE__,__FILE__);

    dwarfstring_reset(&targ);
    dwarfstring_append(&targ,"gef");
    res = _dwarf_pathjoinl(&targ,&inp);
    checkjoin(DW_DLV_OK,res,"gef/foo",
        dwarfstring_string(&targ),
        __LINE__,__FILE__);

    dwarfstring_reset(&targ);
    dwarfstring_reset(&inp);
    dwarfstring_append(&targ,"gef/");
    dwarfstring_append(&inp,"/jkl/");
    res = _dwarf_pathjoinl(&targ,&inp);
    checkjoin(DW_DLV_OK,res,"gef/jkl/",
        dwarfstring_string(&targ),
        __LINE__,__FILE__);

    dwarfstring_reset(&targ);
    dwarfstring_reset(&inp);
    dwarfstring_append(&targ,"gef/");
    dwarfstring_append(&inp,"jkl/");
    res = _dwarf_pathjoinl(&targ,&inp);
    checkjoin(DW_DLV_OK,res,"gef/jkl/",
        dwarfstring_string(&targ),
        __LINE__,__FILE__);

    dwarfstring_reset(&targ);
    dwarfstring_reset(&inp);
    dwarfstring_append(&targ,"gef");
    dwarfstring_append(&inp,"jkl/");
    res = _dwarf_pathjoinl(&targ,&inp);
    checkjoin(DW_DLV_OK,res,"gef/jkl/",
        dwarfstring_string(&targ),
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


static void
checklinkedto(int expret,int gotret,
    int expcount,int gotcount,int line, char *filename_in)
{
    if (expret != gotret) {
        errcount++;
        printf("ERROR expected return %d, got %d line %d %s\n",
            expret,gotret,line,filename_in);
    }
    if (expcount != gotcount) {
        errcount++;
        printf("ERROR expected return %d, got %d line %d %s\n",
            expcount,gotcount,line,filename_in);
    }
}

static void
printpaths(unsigned count,char **array)
{
    unsigned i = 0;
   
    printf("    Paths:\n");
    for(i = 0 ; i < count ; ++i) {
        char *s = array[i];

        printf("    [%2d] \"%s\"\n",i,s);
    }
    printf("\n");
}

#if 0
int _dwarf_construct_linkedto_path(
   char         **global_prefixes_in,
   unsigned       length_global_prefixes_in,
   char          *pathname_in,
   char          *link_string_in, /* from debug link */
   unsigned char  *crc_in, /* from debug_link, 4 bytes */
   unsigned       builid_length, /* from gnu buildid */
   unsigned char *builid, /* from gnu buildid */
   char        ***paths_out,
   unsigned      *paths_out_length,
   int *errcode);
#endif

static unsigned char buildid[20] = {
    0x11,0x22,0x33, 0x44,
    0x21,0x22,0x23, 0x44,
    0xa1,0xa2,0xa3, 0xa4,
    0xb1,0xb2,0xb3, 0xb4,
    0xc1,0xc2,0xc3, 0xc4 };
/*  Since we don't find the files here this
    is not a good test. However, the program
    is used by rundebuglink.sh */
static void
test3(void)
{
    char * executablepath = "a/b";
    char * linkstring = "de";
    dwarfstring result;
    char ** global_prefix = 0;
    unsigned global_prefix_count = 2;
    unsigned global_prefix_len = 0;
    unsigned char crc[4];
    unsigned buildid_length = 20;
    char **paths_returned = 0;
    unsigned paths_returned_count = 0;
    int errcode = 0;
    int res = 0;

    crc[0] = 0x12;
    crc[1] = 0x34;
    crc[2] = 0x56;
    crc[3] = 0xab;
    global_prefix_len =  (global_prefix_count+1)
        * sizeof(void *);
    global_prefix = (char **)malloc(global_prefix_len);
    global_prefix[0] = "/usr/lib/debug";
    global_prefix[1] = "/fake/lib/debug";


    dwarfstring_constructor(&result);
    res =_dwarf_construct_linkedto_path(global_prefix,
        global_prefix_count,
        executablepath,linkstring,
        crc,
        buildid_length,buildid,
        &paths_returned,&paths_returned_count,
        &errcode);
    checklinkedto(DW_DLV_OK,res,6,paths_returned_count,
        __LINE__,__FILE__);
    printpaths(paths_returned_count,paths_returned);
    free(paths_returned);
    paths_returned = 0;
    paths_returned_count = 0;
    errcode = 0;

    dwarfstring_reset(&result);
    executablepath = "ge";
    linkstring = "h/i";
    res =_dwarf_construct_linkedto_path(global_prefix,
        global_prefix_count,
        executablepath,linkstring,
        crc,
        buildid_length,buildid,
        &paths_returned,&paths_returned_count,
        &errcode);
    checklinkedto(DW_DLV_OK,res,6,paths_returned_count,
        __LINE__,__FILE__);
    printpaths(paths_returned_count,paths_returned);
    free(paths_returned);
    paths_returned = 0;
    paths_returned_count = 0;
    errcode = 0;

    dwarfstring_reset(&result);
    executablepath = "/somewherespecial/a/b/ge";
    linkstring = "h/i";
    res =_dwarf_construct_linkedto_path(global_prefix,
        global_prefix_count,
        executablepath,linkstring,
        crc,
        buildid_length,buildid,
        &paths_returned,&paths_returned_count,
        &errcode);
    checklinkedto(DW_DLV_OK,res,6,paths_returned_count,
        __LINE__,__FILE__);
    printpaths(paths_returned_count,paths_returned);
    free(paths_returned);
    free(global_prefix);
    paths_returned = 0;
    paths_returned_count = 0;
    errcode = 0;
}



int main()
{

    test1();
    test2();
    test3();

    if (errcount) {
        return 1;
    }
    return 0;
}
