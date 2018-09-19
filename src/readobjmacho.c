/*
Copyright (c) 2013-2018, David Anderson
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

/*  This program attempts to read and print the headers of an
    Apple Mach-o object file.

    It prints as much as possible as early as possible in case the
    object is malformed.
*/

#include "config.h"
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "reading.h"
#include "object_detector.h"
#include "readobjmacho.h"
#include "macho-loader.h"
#include "sanitized.h"
#include "readelfobj_version.h"

int printfilenames = FALSE;
struct macho_filedata_s macho_filedata;

char *Usage = "Usage: readobjmacho <options> file ...\n"
    "Options:\n"
    "--help     print this message\n"
    "--version  print version string\n";


static char tru_path_buffer[BUFFERSIZE];
static char buffer1[BUFFERSIZE];
static void do_one_file(const char *s);

int
main(int argc,char **argv)
{
    int i = 0;
    int filecount = 0;
    int printed_version = FALSE;

    if( argc == 1) {
        printf("%s\n",Usage);
        exit(1);
    } else {
        argv++;
        for(i =1; i<argc; i++,argv++) {
            const char * filename = 0;
            FILE *fin = 0;

            if((strcmp(argv[0],"--help") == 0) ||
                (strcmp(argv[0],"-h") == 0)) {
                P("%s",Usage);
                exit(0);
            }
            if((strcmp(argv[0],"--version") == 0) ||
                (strcmp(argv[0],"-v") == 0 )) {
                P("Version-readelfobj: %s\n",
                    READELFOBJ_VERSION_DATE_STR);
                printed_version = TRUE;
                continue;
            }
            if ( (i+1) < argc) {
                printfilenames = TRUE;
            }
            filename = argv[0];
            if (printfilenames) {
                P("File: %s\n",sanitized(filename,buffer1,BUFFERSIZE));
            }
            fin = fopen(filename,"r");
            if(fin == NULL) {
                P("No such file as %s\n",argv[0]);
                continue;
            }
            fclose(fin);
            ++filecount;
            do_one_file(filename);
        }
        if (!filecount && !printed_version) {
            printf("%s\n",Usage);
            exit(1);
        }
    }
    return RO_OK;
}


/*
  A byte-swapping version of memcpy
  for cross-endian use.
  Only 2,4,8 should be lengths passed in.
*/
static void *
ro_memcpy_swap_bytes(void *s1, const void *s2, size_t len)
{
    void *orig_s1 = s1;
    unsigned char *targ = (unsigned char *) s1;
    const unsigned char *src = (const unsigned char *) s2;

    if (len == 4) {
        targ[3] = src[0];
        targ[2] = src[1];
        targ[1] = src[2];
        targ[0] = src[3];
    } else if (len == 8) {
        targ[7] = src[0];
        targ[6] = src[1];
        targ[5] = src[2];
        targ[4] = src[3];
        targ[3] = src[4];
        targ[2] = src[5];
        targ[1] = src[6];
        targ[0] = src[7];
    } else if (len == 2) {
        targ[1] = src[0];
        targ[0] = src[1];
    }
/* should NOT get below here: is not the intended use */
    else if (len == 1) {
        targ[0] = src[0];
    } else {
        memcpy(s1, s2, len);
    }

    return orig_s1;
}

static void
destructmacho(struct macho_filedata_s *mp)
{
    if (mp->mo_file) {
        fclose(mp->mo_file);
        mp->mo_file = 0;
    }
    memset(mp,0,sizeof(*mp));
}

static void
print_macho_header(struct macho_filedata_s *mfp)
{
    P("Mach-o Magic:  " LONGESTXFMT "\n",mfp->mo_header.magic);
    P("  cputype           :  " LONGESTXFMT   
        " cpusubtype: " LONGESTXFMT "\n",
        mfp->mo_header.cputype,mfp->mo_header.cpusubtype);
    P("  filetype          :  " LONGESTXFMT  
        " %s"    "\n",
        mfp->mo_header.filetype,
        mfp->mo_header.filetype == MH_DSYM? 
            "DSYM (debug sections present)":
            "");
    P("  number of commands:  " LONGESTXFMT  "\n",
        mfp->mo_header.ncmds);
    P("  size of commands  :  " LONGESTXFMT  "\n",
        mfp->mo_header.sizeofcmds);
    P("  flags             :  " LONGESTXFMT  "\n",
        mfp->mo_header.flags);
}

static void
do_one_file(const char *s)
{
    unsigned ftype = 0;
    unsigned endian = 0;
    unsigned offsetsize = 0;
    size_t filesize = 0;
    int res = 0;

    if (printfilenames) {
        P("Reading: %s\n",s);
    }
    res = dwarf_object_detector_path(s,tru_path_buffer,BUFFERSIZE,
       &ftype,&endian,&offsetsize,&filesize);
    if (res != DW_DLV_OK) {
        P("ERROR: Unable to read \"%s\", ignoring file\n",
            s); 
        return;
    }
    if (ftype !=  DW_FTYPE_MACH_O) {
        P("File %s is not mach-o. Ignored.\n",tru_path_buffer);
        return;
    }

    macho_filedata.mo_file = fopen(tru_path_buffer,"r");
    if (!macho_filedata.mo_file) {
        P("Warning: Unable to open %s for detailed reading.\n",
            s);
        return;
    }

       
#ifdef WORDS_BIGENDIAN
    if (endian == DW_ENDIAN_LITTLE || endian == DW_ENDIAN_OPPOSITE ) {
        macho_filedata.mo_copy_word = ro_memcpy_swap_bytes;
        macho_filedata.mo_endian = DW_ENDIAN_LITTLE;
    } else {
        macho_filedata.mo_copy_word = memcpy;
        macho_filedata.mo_endian = DW_ENDIAN_BIG;
    }
#else  /* LITTLE ENDIAN */
    if (endian == DW_ENDIAN_LITTLE || endian == DW_ENDIAN_SAME ) {
        macho_filedata.mo_copy_word = memcpy;
        macho_filedata.mo_endian = DW_ENDIAN_LITTLE;
    } else {
        macho_filedata.mo_copy_word = ro_memcpy_swap_bytes;
        macho_filedata.mo_endian = DW_ENDIAN_BIG;
    }
#endif /* LITTLE- BIG-ENDIAN */

    macho_filedata.mo_filesize = filesize;
    macho_filedata.mo_offsetsize = offsetsize;
    macho_filedata.mo_pointersize = 0; /* Not known yet */
    macho_filedata.mo_path = tru_path_buffer;
    res = load_macho_header(&macho_filedata);
    if (res != DW_DLV_OK) {
        P("Warning: %s macho-header not loaded giving up",
            macho_filedata.mo_path);
        destructmacho(&macho_filedata);
        return;
    }
    print_macho_header(&macho_filedata);


}

int
cur_read_loc(FILE *fin_arg, long * fileoffset)
{
    long loc = 0;

    loc = ftell(fin_arg);
    if (loc < 0) {
        /* ERROR */
        return RO_ERR;
    }
    *fileoffset = loc;
    return RO_OK;
}

static int
load_macho_header32(struct macho_filedata_s *mfp)
{
     struct mach_header mh32;
     int res = 0;

     res = RRMO(mfp->mo_file,&mh32,0,sizeof(mh32));
     if (res != RO_OK) {
         return res;
     }
     /* Do not adjust endianness of magic, leave as-is. */
     mfp->mo_header.magic = mh32.magic;
     ASSIGNMO(mfp,mfp->mo_header.cputype,mh32.cputype);
     ASSIGNMO(mfp,mfp->mo_header.cpusubtype,mh32.cpusubtype);
     ASSIGNMO(mfp,mfp->mo_header.filetype,mh32.filetype);
     ASSIGNMO(mfp,mfp->mo_header.ncmds,mh32.ncmds);
     ASSIGNMO(mfp,mfp->mo_header.sizeofcmds,mh32.sizeofcmds);
     ASSIGNMO(mfp,mfp->mo_header.flags,mh32.flags);
     mfp->mo_header.reserved = 0;
     return RO_OK;  
}

static int
load_macho_header64(struct macho_filedata_s *mfp)
{
     struct mach_header_64 mh64;
     int res = 0;

     res = RRMO(mfp->mo_file,&mh64,0,sizeof(mh64));
     if (res != RO_OK) {
         return res;
     }
     /* Do not adjust endianness of magic, leave as-is. */
     mfp->mo_header.magic = mh64.magic;
     ASSIGNMO(mfp,mfp->mo_header.cputype,mh64.cputype);
     ASSIGNMO(mfp,mfp->mo_header.cpusubtype,mh64.cpusubtype);
     ASSIGNMO(mfp,mfp->mo_header.filetype,mh64.filetype);
     ASSIGNMO(mfp,mfp->mo_header.ncmds,mh64.ncmds);
     ASSIGNMO(mfp,mfp->mo_header.sizeofcmds,mh64.sizeofcmds);
     ASSIGNMO(mfp,mfp->mo_header.flags,mh64.flags);
     ASSIGNMO(mfp,mfp->mo_header.reserved,mh64.reserved);
     return RO_OK;  
}

int
load_macho_header(struct macho_filedata_s *mfp)
{
     int res = 0;

     if (mfp->mo_offsetsize == 32) {
         res = load_macho_header32(mfp);
     } else if (mfp->mo_offsetsize == 64) {
         res = load_macho_header64(mfp);
     } else {
         P("Warning: Unknown offset size %u\n",(unsigned)mfp->mo_offsetsize);
         return RO_ERR;
     }
     if (res != RO_OK) {
         P("Warning: unable to read object header\n");
         return res;
     }
     return res;
}


