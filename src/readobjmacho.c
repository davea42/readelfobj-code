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
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif /* HAVE_MALLOC_H */
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#include "dwarf_reading.h"
#include "dwarf_macho_loader.h"
#include "dwarf_object_detector.h"
#include "dwarf_object_read_common.h"
#include "dwarf_machoread.h"
#include "sanitized.h"
#include "readelfobj_version.h"

int printfilenames = FALSE;

char *Usage = "Usage: readobjmacho <options> file ...\n"
    "Options:\n"
    "--help     print this message\n"
    "--version  print version string\n";


static char tru_path_buffer[BUFFERSIZE];
static char buffer1[BUFFERSIZE];
static void do_one_file(const char *s);

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

static const char *
get_command_name(Dwarf_Unsigned v)
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
                P("Version-readobjmacho: %s\n",
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




static void
print_macho_segments(struct macho_filedata_s *mfp)
{
    Dwarf_Unsigned segmentcount = mfp->mo_segment_count;
    Dwarf_Unsigned i = 0;
    struct generic_macho_segment_command *cmdp =
        mfp->mo_segment_commands;

    P("  Segments count:" LONGESTUFMT " starting at "
        LONGESTXFMT8 ":\n",segmentcount,mfp->mo_command_start_offset);
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
print_macho_dwarf_sections(struct macho_filedata_s *mfp)
{
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned count = mfp->mo_dwarf_sectioncount;
    struct generic_macho_section * gsp = 0;

    gsp = mfp->mo_dwarf_sections;

    P(" Sections count: " LONGESTUFMT "  offset " LONGESTXFMT8 "\n",
        count,gsp->offset_of_sec_rec);
    P("                         offset size \n");
    for(i =0; i < count; ++i,++gsp) {
        P("  [" LONGESTUFMT "] %-16s"
            " " LONGESTXFMT8
            " " LONGESTXFMT8
            "\n",
            i,gsp->sectname,
            gsp->offset,
            gsp->size);
    }
}


static void
print_macho_commands(struct macho_filedata_s *mfp)
{
    Dwarf_Unsigned i = 0;
    struct generic_macho_command *cmdp = 0;

    cmdp = mfp->mo_commands;
    P(" Commands: at offset " LONGESTXFMT "\n",mfp->mo_command_start_offset);
    for ( ; i < mfp->mo_command_count; ++i, ++cmdp) {
        P("  [" LONGESTUFMT "] cmd: " LONGESTXFMT8 " %-14s"
            " cmdsize: " LONGESTUFMT " (" LONGESTXFMT8 ")\n",
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
    P("  cputype           : " LONGESTXFMT
        " cpusubtype: " LONGESTXFMT "\n",
        mfp->mo_header.cputype,
        mfp->mo_header.cpusubtype);
    P("  offset size       : %u\n",
        mfp->mo_offsetsize/8);
    P("  endian            : %s\n",
        (mfp->mo_endian ==  DW_ENDIAN_BIG)?"BIGENDIAN":
        ((mfp->mo_endian ==  DW_ENDIAN_LITTLE)?"LITTLEENDIAN":
        "Unknown-error"));
    P("  file size         : " LONGESTXFMT8 "\n",mfp->mo_filesize);
    P("  filetype          : " LONGESTXFMT
        " %s"    "\n",
        mfp->mo_header.filetype,
        mfp->mo_header.filetype == MH_DSYM?
            "DSYM (debug sections present)":
            "");
    P("  number of commands: " LONGESTXFMT  "\n",
        mfp->mo_header.ncmds);
    P("  size of commands  : " LONGESTXFMT  "\n",
        mfp->mo_header.sizeofcmds);
    P("  flags             : " LONGESTXFMT  "\n",
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
    struct macho_filedata_s *mfp = 0;
    int errcode = 0;

    res = dwarf_object_detector_path(s,tru_path_buffer,BUFFERSIZE,
        &ftype,&endian,&offsetsize,&filesize,&errcode);
    if (res != DW_DLV_OK) {
        P("ERROR: Unable to read \"%s\", ignoring file. "
            "Errcode %d\n", s,errcode);
        return;
    }
    if (printfilenames) {
        P("Reading: %s (%s)\n",s,tru_path_buffer);
    }
    if (ftype !=  DW_FTYPE_MACH_O) {
        P("File %s is not mach-o. Ignored.\n",tru_path_buffer);
        return;
    }
    res = dwarf_construct_macho_access_path(tru_path_buffer,
        &mfp,&errcode);
    if (res != RO_OK) {
        P("Warning: Unable to open %s for detailed reading. Err %d\n",
            s,errcode);
        return;
    }
#ifdef WORDS_BIGENDIAN
    if (endian == DW_ENDIAN_LITTLE || endian == DW_ENDIAN_OPPOSITE ) {
        mfp->mo_copy_word = ro_memcpy_swap_bytes;
        mfp->mo_endian = DW_ENDIAN_LITTLE;
    } else {
        mfp->mo_copy_word = memcpy;
        mfp->mo_endian = DW_ENDIAN_BIG;
    }
#else  /* LITTLE ENDIAN */
    if (endian == DW_ENDIAN_LITTLE || endian == DW_ENDIAN_SAME ) {
        mfp->mo_copy_word = memcpy;
        mfp->mo_endian = DW_ENDIAN_LITTLE;
    } else {
        mfp->mo_copy_word = dwarf_ro_memcpy_swap_bytes;
        mfp->mo_endian = DW_ENDIAN_BIG;
    }
#endif /* LITTLE- BIG-ENDIAN */

    mfp->mo_filesize = filesize;
    mfp->mo_offsetsize = offsetsize;
    res = dwarf_load_macho_header(mfp,&errcode);
    if (res != DW_DLV_OK) {
        P("Warning: %s macho-header not loaded giving up. Error %d",
            tru_path_buffer,errcode);
        dwarf_destruct_macho_access(mfp);
        return;
    }
    print_macho_header(mfp);
    res = dwarf_load_macho_commands(mfp,&errcode);
    print_macho_commands(mfp);
    print_macho_segments(mfp);
    print_macho_dwarf_sections(mfp);
    dwarf_destruct_macho_access(mfp);
}

int
cur_read_loc(FILE *fin_arg, long * fileoffset)
{
    long loc = 0;

    loc = ftell(fin_arg);
    if (loc < 0) {
        /* ERROR */
        return RO_ERROR;
    }
    *fileoffset = loc;
    return RO_OK;
}
