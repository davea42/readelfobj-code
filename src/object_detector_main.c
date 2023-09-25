/* Copyright (c) 2018-2018, David Anderson
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
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <string.h> /* memcpy, strcpy */
#include "dwarf_types.h"
#include "dwarf_reading.h"
#include "dwarf_universal.h"
#include "dwarf_object_detector.h"

/*  This is a main program reporting an overview of what
    an object is.

    object_detector [-z] path ...
 
    Other than a  required path name, its only option is
    -z, which turns off searching for MacOS dSYM
    DWARF objects and for debuglink objects.
*/

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif /* TRUE */

#ifndef DW_DLV_OK
#define DW_DLV_NO_ENTRY -1
#define DW_DLV_OK        0
#define DW_DLV_ERROR     1
#endif /* DW_DLV_OK */

const char *dwarf_file_type[7] = {
"file type unknown",
"file type elf",
"file type mach-o",
"file type pe",
"file type archive",
"file type Apple Universal Object",
0
};

const char *dwarf_endian_type[6] = {
"endian:unknown",
"big-endian",
"little-endian",
"same-endian",
"opposite-endian",
0
};
# if 0
int dwarf_object_detector_universal_head(
    char         *dw_path,
    unsigned      dw_filetype /* DW_FTYPE_APPLEUNIVERSAL */,
    Dwarf_Unsigned      dw_filesize,
    Dwarf_Unsigned     *dw_contentcount,
    Dwarf_Universal_Head * dw_head,
    int            *errcode);

int dwarf_object_detector_universal_instance(
    Dwarf_Universal_Head dw_head,
    Dwarf_Unsigned  dw_index_of,
    Dwarf_Unsigned *dw_cpu_type,
    Dwarf_Unsigned *dw_cpu_subtype,
    Dwarf_Unsigned *dw_offset,
    Dwarf_Unsigned *dw_size,
    Dwarf_Unsigned *dw_align,
    int         *errcode);
void dwarf_dealloc_universal_head(Dwarf_Universal_Head dw_head);
#endif


static int
report_universal(char *targpath,unsigned endian,
    unsigned offsetsize,size_t filesize,int *errcode)
{
    Dwarf_Universal_Head unihead = 0;
    unsigned int contentcount = 0;
    int res = 0;
    Dwarf_Unsigned i = 0;

    res = dwarf_object_detector_universal_head(targpath,
         filesize,&contentcount,&unihead,errcode);
    if (res != DW_DLV_OK) {
         return res;
    }
    printf("  Mach-O Universal binary\n");
    printf("                   count: %u\n", contentcount); 
    printf("              offsetsize: %u\n", offsetsize);
    printf("                filesize: 0x%lx\n", 
        (unsigned long)filesize);
    printf("                  endian: %s\n",
        dwarf_endian_type[endian]); 
    for ( ; i < contentcount; ++i) {
       Dwarf_Unsigned cpu_type = 0;
       Dwarf_Unsigned cpu_subtype = 0;
       Dwarf_Unsigned offset = 0;
       Dwarf_Unsigned size = 0;
       Dwarf_Unsigned align = 0;

       res = dwarf_object_detector_universal_instance(
           unihead,i,&cpu_type,&cpu_subtype,&offset,
           &size,&align,errcode);
       if (res == DW_DLV_NO_ENTRY) {
           printf(" FAIL: inner binary number "
               LONGESTUFMT
               " unavailable, does not exist\n",i);
           continue;
       }
       if (res == DW_DLV_NO_ENTRY) {
           printf(" FAIL: inner binary number "
               LONGESTUFMT
               " gets error, error code %d\n",i,*errcode);
           continue;
       }
       printf("    [%2llu] ",i);
	   printf(" cputype 0x%lx \n",(unsigned long)cpu_type);
       printf("          cpusubtype 0x%lx \n",
           (unsigned long)cpu_subtype);
       printf("          offset " LONGESTXFMT 
           " size " LONGESTXFMT 
           " align " LONGESTUFMT " \n",
           offset, size, align);
    }
    

    dwarf_dealloc_universal_head(unihead);
    (void)targpath;
    return DW_DLV_OK;
}

#define PATHSIZE 2000
static char finalpath[PATHSIZE];

int main(int argc, char **argv)
{
    int ct = 1;
    int errcode = 0;
    int zero_outpath = 0;
    int errct = 0;

    for ( ;ct < argc ; ct++) {
        char *path = argv[ct];
        int          res = 0;
        unsigned int ftype = 0;
        unsigned int endian = 0;
        unsigned int offsetsize = 0;
        Dwarf_Unsigned filesize = 0;
        char *finalpathp = finalpath;
        finalpath[0] = 0;

        if (!strcmp(path,"-z")) {
            zero_outpath = 1;
            continue;
        }
        if (zero_outpath) {
            finalpathp = 0;
        }

        res = dwarf_object_detector_path(path,
            finalpathp,PATHSIZE,
            &ftype, &endian, &offsetsize,&filesize,
            &errcode);
        if (res == DW_DLV_OK) {
            printf("%s (%s), type %u %s, endian "
                "%u %s, offsetsize %u, filesize %lu\n",
                path,finalpathp?finalpathp:"no-final-path",
                ftype,dwarf_file_type[ftype],
                endian,dwarf_endian_type[endian],
                offsetsize,
                (unsigned long)filesize);
            if (ftype == DW_FTYPE_APPLEUNIVERSAL) {
                char * targpath = finalpathp?finalpathp:path;
                res  = report_universal(targpath,endian,
                    offsetsize,filesize, &errcode);
                if (res == DW_DLV_ERROR) {
                    printf("%s FAIL: error accessing  "
                        " Apple Universal Binary "
                        "Errcode %s.\n",path,
                        dwarf_get_errname(errcode));
                    ++errct;
                }
            }
        } else if (res == DW_DLV_ERROR) {
            printf("%s FAIL: error opening file. "
                "Errcode %s.\n",path, dwarf_get_errname(errcode));
            ++errct;
        } else {
            /* DW_DLV_NO_ENTRY */
            printf("%s FAIL: no such file present/readable\n",path);
        }
    }
    return errct;
}
