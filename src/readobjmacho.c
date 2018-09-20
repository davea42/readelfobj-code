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

static void destructmacho(struct macho_filedata_s *mp);
static struct commands_text_s {
   const char *name;
   unsigned long val;
} commandname [] = {
{"LC_SEGMENT",    0x1},
{"LC_SYMTAB",     0x2},
{"LC_SYMSEG",     0x3}, 
{"LC_THREAD",     0x4 }, 
{"LC_UNIXTHREAD", 0x5}, 
{"LC_LOADFVMLIB", 0x6},
{"LC_IDFVMLIB",   0x7  },
{"LC_IDENT",      0x8  },
{"LC_FVMFILE",    0x9  },
{"LC_PREPAGE",    0xa  },
{"LC_DYSYMTAB",   0xb  },
{"LC_LOAD_DYLIB", 0xc  },
{"LC_ID_DYLIB",   0xd  },
{"LC_LOAD_DYLINKER", 0xe },
{"LC_ID_DYLINKER",0xf },
{"LC_PREBOUND_DYLIB", 0x10 },
{"LC_ROUTINES",   0x11 },
{"LC_SUB_FRAMEWORK", 0x12 },
{"LC_SUB_UMBRELLA", 0x13  },
{"LC_SUB_CLIENT", 0x14  },
{"LC_SUB_LIBRARY",0x15  },
{"LC_TWOLEVEL_HINTS", 0x16 },
{"LC_PREBIND_CKSUM",  0x17 },
{"LC_LOAD_WEAK_DYLIB", (0x18 | LC_REQ_DYLD)},
{"LC_SEGMENT_64",   0x19 },
{"LC_ROUTINES_64",  0x1a    },
{"LC_UUID",         0x1b    },
{"LC_RPATH",       (0x1c | LC_REQ_DYLD) },
{"LC_CODE_SIGNATURE", 0x1d }, 
{"LC_SEGMENT_SPLIT_INFO", 0x1e}, 
{"LC_REEXPORT_DYLIB", (0x1f | LC_REQ_DYLD)}, 
{"LC_LAZY_LOAD_DYLIB", 0x20}, 
{"LC_ENCRYPTION_INFO", 0x21}, 
{"LC_DYLD_INFO",    0x22   }, 
{"LC_DYLD_INFO_ONLY", (0x22|LC_REQ_DYLD)}, 
{"LC_LOAD_UPWARD_DYLIB", (0x23 | LC_REQ_DYLD) },
{"LC_VERSION_MIN_MACOSX", 0x24   },
{"LC_VERSION_MIN_IPHONEOS", 0x25}, 
{"LC_FUNCTION_STARTS", 0x26 },
{"LC_DYLD_ENVIRONMENT", 0x27 },
{"LC_MAIN", (0x28|LC_REQ_DYLD)},
{0,0}
};

const char *get_command_name(LONGESTUTYPE v)
{
    unsigned i = 0;

    for( ; commandname[i].name; i++) {
        if (v==commandname[i].val) { 
            return commandname[i].name;
        }
    }
    return ("Unknown");
}


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
            destructmacho(&macho_filedata);
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
    if (mp->mo_commands){
        free(mp->mo_commands);
        mp->mo_commands = 0;
    }
    if (mp->mo_segment_commands){
        free(mp->mo_segment_commands);
        mp->mo_segment_commands = 0;
    }
    memset(mp,0,sizeof(*mp));
}

static void
print_macho_segments(struct macho_filedata_s *mfp)
{
    LONGESTUTYPE segmentcount = mfp->mo_segment_count;
    LONGESTUTYPE i = 0;
    struct generic_segment_command *cmdp = mfp->mo_segment_commands;

    P("  Segments 0-" LONGESTUFMT ":\n",segmentcount); 
    P("    command                 segname      fileoff   filesize\n");
    for ( ; i < segmentcount; ++i, ++cmdp) {
       P("  [" LONGESTUFMT "] " 
           LONGESTXFMT " %-15s"
           " %-17s"
           " " LONGESTXFMT8
           " " LONGESTXFMT8 "\n",
           i,
           cmdp->cmd, cmdp->cmd?get_command_name(cmdp->cmd):"",
           cmdp->segname,
           cmdp->fileoff,
           cmdp->filesize);
    }
}

static void
print_macho_commands(struct macho_filedata_s *mfp)
{
    LONGESTUTYPE i = 0;
    struct generic_macho_command *cmdp = 0;

    cmdp = mfp->mo_commands;
    P(" Commands: at offset " LONGESTXFMT "\n",mfp->mo_command_start_offset);
    for ( ; i < mfp->mo_command_count; ++i, ++cmdp) {
       P("  [" LONGESTUFMT "] cmd: " LONGESTXFMT " %s"
           " cmdsize: " LONGESTUFMT " (" LONGESTXFMT ")\n",
           i,
           cmdp->cmd, get_command_name(cmdp->cmd),
           cmdp->cmdsize,
           cmdp->cmdsize);
    }

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

    res = load_macho_commands(&macho_filedata);

    print_macho_commands(&macho_filedata);
    print_macho_segments(&macho_filedata);
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
     mfp->mo_command_count = mfp->mo_header.ncmds;
     mfp->mo_command_start_offset = sizeof(mh32);
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
     mfp->mo_command_count = mfp->mo_header.ncmds;
     mfp->mo_command_start_offset = sizeof(mh64);
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


static int
load_segment_command_content32(struct macho_filedata_s *mfp,
    struct generic_macho_command *mmp,
    struct generic_segment_command *msp,
    LONGESTUTYPE mmpindex)
{
    struct segment_command sc;
    int res = 0;
    LONGESTUTYPE filesize = mfp->mo_filesize;

    if (mmp->offset_this_command > filesize ||
        mmp->cmdsize > filesize ||
        (mmp->cmdsize + mmp->offset_this_command) > filesize ) {

        return RO_ERR;
    }   
    res = RRMO(mfp->mo_file,&sc,mmp->offset_this_command,sizeof(sc));
    if (res != RO_OK) {
        return res;
    }
    ASSIGNMO(mfp,msp->cmd,sc.cmd);
    ASSIGNMO(mfp,msp->cmdsize,sc.cmdsize);
    strncpy(msp->segname,sc.segname,16);
    msp->segname[16] =0;
    ASSIGNMO(mfp,msp->vmaddr,sc.vmaddr);
    ASSIGNMO(mfp,msp->vmsize,sc.vmsize);
    ASSIGNMO(mfp,msp->fileoff,sc.fileoff);
    ASSIGNMO(mfp,msp->filesize,sc.filesize);
    if (msp->fileoff > mfp->mo_filesize ||
        msp->filesize > mfp->mo_filesize) {
        /* corrupt */
        return RO_ERR;
    }
    if ((msp->fileoff+msp->filesize ) > filesize) {
        /* corrupt */
        return RO_ERR;
    }
    ASSIGNMO(mfp,msp->maxprot,sc.maxprot);
    ASSIGNMO(mfp,msp->initprot,sc.initprot);
    ASSIGNMO(mfp,msp->nsects,sc.nsects);
    ASSIGNMO(mfp,msp->flags,sc.flags);
    msp->macho_command_index = mmpindex;
    return RO_OK;
}
static int
load_segment_command_content64(struct macho_filedata_s *mfp,
    struct generic_macho_command *mmp,
    struct generic_segment_command *msp,
    LONGESTUTYPE mmpindex)
{
    struct segment_command_64 sc;
    int res = 0;
    LONGESTUTYPE filesize = mfp->mo_filesize;

    if (mmp->offset_this_command > filesize ||
        mmp->cmdsize > filesize ||
        (mmp->cmdsize + mmp->offset_this_command) > filesize ) {
        return RO_ERR;
    }   
    res = RRMO(mfp->mo_file,&sc,mmp->offset_this_command,sizeof(sc));
    if (res != RO_OK) {
        return res;
    }
    ASSIGNMO(mfp,msp->cmd,sc.cmd);
    ASSIGNMO(mfp,msp->cmdsize,sc.cmdsize);
    strncpy(msp->segname,sc.segname,16);
    msp->segname[16] =0;
    ASSIGNMO(mfp,msp->vmaddr,sc.vmaddr);
    ASSIGNMO(mfp,msp->vmsize,sc.vmsize);
    ASSIGNMO(mfp,msp->fileoff,sc.fileoff);
    ASSIGNMO(mfp,msp->filesize,sc.filesize);
    if (msp->fileoff > filesize ||
        msp->filesize > filesize) {
        /* corrupt */
        return RO_ERR;
    }
    if ((msp->fileoff+msp->filesize ) > filesize) {
        /* corrupt */
        return RO_ERR;
    }
    ASSIGNMO(mfp,msp->maxprot,sc.maxprot);
    ASSIGNMO(mfp,msp->initprot,sc.initprot);
    ASSIGNMO(mfp,msp->nsects,sc.nsects);
    ASSIGNMO(mfp,msp->flags,sc.flags);
    msp->macho_command_index = mmpindex;
    return RO_OK;
}       




int 
load_segment_commands(struct macho_filedata_s *mfp)
{
    LONGESTUTYPE i = 0;  
    struct generic_macho_command *mmp = 0;
    struct generic_segment_command *msp = 0;

    if(mfp->mo_segment_count < 1) {
        return RO_OK;
    }
    /* Add room for zero segment */
    ++mfp->mo_segment_count;

    mfp->mo_segment_commands = (struct generic_segment_command *)
        calloc(sizeof(struct generic_segment_command),mfp->mo_segment_count);
    if(!mfp->mo_segment_commands) {
        return RO_ERR;
    }

    mmp = mfp->mo_commands;
    msp = mfp->mo_segment_commands;
    /* leave zero segment all zeros */
    msp++;
    for (i = 0 ; i < mfp->mo_command_count; ++i,++mmp) {
        unsigned cmd = mmp->cmd;
        int res = 0;

        if (cmd == LC_SEGMENT) {
            res = load_segment_command_content32(mfp,mmp,msp,i);
            ++msp;
        } else if (cmd == LC_SEGMENT_64) {
            res = load_segment_command_content64(mfp,mmp,msp,i);
            ++msp;
        }
        if (res != RO_OK) {
            return res;
        }
        
    }
}

/* Works the same, 32 or 64 bit */
int 
load_macho_commands(struct macho_filedata_s *mfp)
{
    LONGESTUTYPE cmdi = 0;
    LONGESTUTYPE curoff = mfp->mo_command_start_offset;
    LONGESTUTYPE cmdspace = 0;
    struct load_command mc;
    struct generic_macho_command *mcp = 0;
    unsigned segment_command_count = 0;
    int res = 0;

    if ((curoff + mfp->mo_command_count * sizeof(mc)) >= 
        mfp->mo_filesize) {
        /* corrupt object. */
        return RO_ERR;
    }

    mfp->mo_commands = (struct generic_macho_command *) calloc(
        mfp->mo_command_count,sizeof(struct generic_macho_command));
    if( !mfp->mo_commands) {
        /* out of memory */
        return RO_ERR;
    }
     
    mcp = mfp->mo_commands;
    for ( ; cmdi < mfp->mo_header.ncmds; ++cmdi,++mcp ) {
        int res = 0;

        res = RRMO(mfp->mo_file,&mc,curoff,sizeof(mc));
        if (res != RO_OK) {
            return res;
        }
        ASSIGNMO(mfp,mcp->cmd,mc.cmd);
        ASSIGNMO(mfp,mcp->cmdsize,mc.cmdsize);
        mcp->offset_this_command = curoff;
        curoff += mcp->cmdsize;
        cmdspace += mcp->cmdsize;
        if (mcp->cmdsize > mfp->mo_filesize ||
            curoff > mfp->mo_filesize) {
            /* corrupt object */
            return RO_ERR;
        }
        if (mcp->cmd == LC_SEGMENT || mcp->cmd == LC_SEGMENT_64) {
            segment_command_count++;
        }
    }
    mfp->mo_segment_count = segment_command_count;
    res = load_segment_commands(mfp);
    if (res != RO_OK) {
        return res;
    }
    return RO_OK;
}
