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
#include <sys/types.h> /* fstat */
#include <sys/stat.h> /* fstat */
#include <string.h> /* memcpy, strcpy */
#include "dwarf_reading.h"
#include "dwarf_object_detector.h"

/* This is library code to actually detect the type of object. */

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif /* TRUE */


#ifndef DW_DLV_OK
#define DW_DLV_NO_ENTRY -1
#define DW_DLV_OK        0
#define DW_DLV_ERROR     1
#endif /* DW_DLV_OK */


const char *dwarf_file_type[6] = {
"file type unknown",
"file type elf",
"file type mach-o",
"file type pe",
"file type archive",
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


#define PATHSIZE 2000
static char finalpath[PATHSIZE];

int main(int argc, char **argv)
{
    int ct = 1;
    int errcode = 0;
    int zero_outpath = 0;

    for( ;ct < argc ; ct++) {
        char *path = argv[ct];
        int res = 0;
        unsigned ftype = 0;
        unsigned endian = 0;
        unsigned offsetsize = 0;
        size_t filesize = 0;
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
        } else if (res == DW_DLV_ERROR) {
            if (errcode == RO_ERR_NOT_A_KNOWN_TYPE) {
                printf("%s FAIL: file type not a valid object-file "
                    "Errcode %d.\n",path, errcode);
            } else {
                printf("%s FAIL: error opening file. "
                    "Errcode %d.\n",path, errcode);
            }
        } else {
            /* DW_DLV_NO_ENTRY */
            printf("%s FAIL: no such file present/readable\n",path);
        }
    }
}
