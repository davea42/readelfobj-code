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
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for exit(), C89 malloc */
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_MALLOC_H
/* Useful include for some Windows compilers. */
#include <malloc.h>
#endif /* HAVE_MALLOC_H */
#include <string.h>
#include <stdlib.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#include "dwarf_types.h"
#include "dwarf_reading.h"
#include "dwarf_macho_loader.h"
#include "dwarf_object_detector.h"
#include "dwarf_object_read_common.h"
#include "dwarf_machoread.h"
#include "sanitized.h"
#include "common_options.h"

unsigned int unibinarynumber = 0;
unsigned char skip_dsym_check = FALSE;

char *Usage = "Usage: readobjmacho <options> file ...\n"
    "Options:\n"
    "--help     print this message\n"
    "--version  print version string\n"
    "--universalnumber=<n>  If a Universal Binary\n"
    "--printfilenames print the name of the file being read\n"
    "--sections-by-size sort sections by section size\n"
    "--sections-by-name sort_sections by name\n"
    "  print binary number n. Defaults to zero\n"
    "--skip-dsym-check accept name as is\n";

static char tru_path_buffer[BUFFERSIZE];
static char buffer1[BUFFERSIZE];

static void do_one_file(const char *s,sec_options *options);

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
{"LC_DATA_IN_CODE", 0x29},
{"LC_SOURCE_VERSION", 0x2a},
{"LC_DYLIB_CODE_SIGN_DRS", 0x2b},
{"LC_ENCRYPTION_INFO_64", 0x2c},
{"LC_LINKER_OPTION", 0x2d},
{"LC_LINKER_OPTIMIZATION_HINT", 0x2e},
{"LC_VERSION_MIN_TVOS", 0x2f},
{"LC_VERSION_MIN_WATCHOS", 0x30},
{"LC_NOTE", 0x31},
{"LC_BUILD_VERSION", 0x32},
{"LC_DYLD_EXPORTS_TRIE", (0x33 | LC_REQ_DYLD)},
{"LC_DYLD_CHAINED_FIXUPS", (0x34 | LC_REQ_DYLD)},
{"LC_FILESET_ENTRY", (0x35 | LC_REQ_DYLD)},
{"LC_ATOM_INFO", 0x36},
{0,0}
};

static struct commands_text_s
    /*const char *name;
    unsigned long val; */
filetypename [] = {
{"MH_OBJECT",    0x1},
{"MH_EXECUTE",    0x2},
{"MH_FVMLIB",    0x3 },
{"MH_CORE",        0x4},
{"MH_PRELOAD",    0x5 },
{"MH_DYLIB",    0x6   },
{"MH_DYLINKER",    0x7},
{"MH_BUNDLE",    0x8 },
{"MH_DYLIB_STUB",    0x9},
{"MH_DSYM",        0xa },
{"MH_KEXT_BUNDLE",    0xb},
{0,0}
};
static struct commands_text_s
    /*const char *name;
    unsigned long val; */
flagsnamesbits [] = {
{"MH_NOUNDEFS",    0x1 },
{"MH_INCRLINK",0x2 },
{"MH_DYLDLINK",0x4},
{"MH_BINDATLOAD",0x8},
{"MH_PREBOUND",0x10 },
{"MH_SPLIT_SEGS",0x20 },
{"MH_LAZY_INIT",0x40},
{"MH_TWOLEVEL",0x80 },
{"MH_FORCE_FLAT",0x100},
{"MH_NOMULTIDEFS",0x200 },
{"MH_NOFIXPREBINDING",0x400  },
{"MH_PREBINDABLE",0x800   },
{"MH_ALLMODSBOUND",0x1000   },
{"MH_SUBSECTIONS_VIA_SYMBOLS",0x2000  },
{"MH_CANONICAL",0x4000   },
{"MH_WEAK_DEFINES",0x8000   },
{"MH_BINDS_TO_WEAK",0x10000  },
{"MH_ALLOW_STACK_EXECUTION",0x20000  },
{"MH_ROOT_SAFE",0x40000  },
{"MH_SETUID_SAFE",0x80000    },
{"MH_NO_REEXPORTED_DYLIBS",0x100000   },
{"MH_PIE",0x200000    },
{"MH_DEAD_STRIPPABLE_DYLIB",0x400000   },
{"MH_HAS_TLV_DESCRIPTORS",0x800000   },
{"MH_NO_HEAP_EXECUTION",0x1000000    },
{"MH_APP_EXTENSION_SAFE",0x02000000  },
{0,0}
};

static const char *
get_command_name(Dwarf_Unsigned v)
{
    unsigned i = 0;

    for ( ; commandname[i].name; i++) {
        if (v==commandname[i].val) {
            return commandname[i].name;
        }
    }
    return ("Unknown");
}
static const char *
get_filetype_name(Dwarf_Unsigned v)
{
    unsigned i = 0;

    for ( ; filetypename[i].name; i++) {
        if (v==filetypename[i].val) {
            return filetypename[i].name;
        }
    }
    return ("Unknown");
}

/*  Add an ending newline here after flagsnamesbits dealt with */
static void
print_all_flags(unsigned long flag)
{
    unsigned long bitmask = 1;
    unsigned long i = 0;
    unsigned long maxindex = 32;
    int printedcount = 0;

    for (  ; i < maxindex; ++i, bitmask <<=1) {
        unsigned long j = 0;
        unsigned long v = flag&bitmask;
        for ( ; flagsnamesbits[j].name; j++) {
            if (v == flagsnamesbits[j].val) {
                if (!printedcount) {
                    printf(" %s",flagsnamesbits[j].name);
                }else {
                    printf("|\n             %s",
                        flagsnamesbits[j].name);
                }
                ++printedcount;
                break; /* ready for next bit */
            }
        }
    }
    printf("\n");
}

static int
findnumber(char *s)
{
    int newv = 0;
    char *nptr = 0;

    if (strlen(s) < 1) {
        printf("Error --unarybinarynumber= missing value. "
            " Giving up.\n");
        exit(1);
    }
    newv = strtoul((const char *)s,&nptr,10);
    if ( nptr && !*nptr) {
        return newv;
    }
    printf("Error --unarybinarynumber=%s "
        "does not have a valid number. "
        " Giving up.\n",s);
    exit(1);
}

int
main(int argc,char **argv)
{
    int i = 0;
    int filecount = 0;
    int printed_version = FALSE;

    if ( argc == 1) {
        printf("%s\n",Usage);
        exit(1);
    } else {
        argv++;
        for (i =1; i<argc; i++,argv++) {
            const char * filename = 0;
            FILE *fin = 0;

            if ((strcmp(argv[0],"--help") == 0) ||
                (strcmp(argv[0],"-h") == 0)) {
                P("%s",Usage);
                exit(0);
            }
            if ((strcmp(argv[0],"--version") == 0) ||
                (strcmp(argv[0],"-v") == 0 )) {
                P("Version-readobjmacho: %s\n",
                    PACKAGE_VERSION);
                printed_version = TRUE;
            }
            if (strcmp(argv[0],"--skip-dsym-check") == 0) {
                printf("Skip dsym check\n");
                skip_dsym_check = TRUE;
                continue;
            }
            if (!strncmp(argv[0],"--unibinarynumber=",18)) {
                unibinarynumber=findnumber(argv[0]+18);
                printf("If universal binary, use inner binary "
                    "number %d\n",unibinarynumber);
                continue;
            }
            if ((strcmp(argv[0],"--sections-by-size") == 0) ||
                (strcmp(argv[0],"--section-by-size") == 0)) {
                secoptionsdata.co_sort_section_by_size = TRUE;
                secoptionsdata.co_sort_section_by_name = FALSE;
                continue;
            }
            if ((strcmp(argv[0],"--sections-by-name") == 0)||
                (strcmp(argv[0],"--section-by-name") == 0)) {
                secoptionsdata.co_sort_section_by_name = TRUE;
                secoptionsdata.co_sort_section_by_size = FALSE;
                continue;
            }
            if (strcmp(argv[0],"--printfilenames") == 0) {
                secoptionsdata.co_printfilenames = TRUE;
                continue;
            }

            if ( argv[0][0] == '-') {
                printf("Argument %s unknown, ignored\n",argv[0]);
                continue;
            }

            filename = argv[0];
            if (secoptionsdata.co_printfilenames) {
                P("File: %s\n",sanitized(filename,buffer1,
                    BUFFERSIZE));
            }
            fin = fopen(filename,"r");
            if (fin == NULL) {
                P("No such file as %s\n",argv[0]);
                continue;
            }
            fclose(fin);
            ++filecount;
            do_one_file(filename,&secoptionsdata);
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
    P("    command                 segname      "
        "fileoff   filesize\n");
    for ( ; i < segmentcount; ++i, ++cmdp) {
        P("  [" LONGESTUFMT "] "
            LONGESTXFMT " %-16s"
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
static int
compare_by_secsize(const void *lin,const void *rin)
{
    sort_section_element * lsel = (sort_section_element *)lin;
    Dwarf_Unsigned lsecindex = lsel->od_originalindex;
    struct generic_macho_section * lgsp =
        (struct generic_macho_section *)lsel->od_sec_desc;
    Dwarf_Unsigned lsecsize = lgsp->size;

    sort_section_element * rsel = (sort_section_element *)rin;
    Dwarf_Unsigned rsecindex = rsel->od_originalindex;
    struct generic_macho_section *rgsp =
        (struct generic_macho_section *)rsel->od_sec_desc;
    Dwarf_Unsigned rsecsize = rgsp->size;

    if (lsecsize < rsecsize) {
        return 1;
    }
    if (lsecsize > rsecsize) {
        return -1;
    }
    if (lsecindex < rsecindex) {
        return -1;
    }
    if (lsecindex > rsecindex) {
        return 1;
    }
    /*  impossible */
    return 0;
}

static int
compare_by_secname(const void *lin,const void *rin)
{
    sort_section_element * lsel = (sort_section_element *)lin;
    Dwarf_Unsigned lsecindex = lsel->od_originalindex;
    struct generic_macho_section * lgsp =
        (struct generic_macho_section *)lsel->od_sec_desc;
    const char *lname = lgsp->sectname;

    sort_section_element * rsel = (sort_section_element *)rin;
    Dwarf_Unsigned rsecindex = rsel->od_originalindex;
    struct generic_macho_section *rgsp =
        (struct generic_macho_section *)rsel->od_sec_desc;
    const char *rname = rgsp->sectname;

    int res = 0;

    res = strcmp(lname,rname);
    if (res) {
        return res;
    }
    if (lsecindex < rsecindex) {
        return -1;
    }
    if (lsecindex > rsecindex) {
        return 1;
    }
    /*  impossible */
    return 0;

}

static void
print_macho_dwarf_sections(struct macho_filedata_s *mfp,
    sec_options *options)
{
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned count = mfp->mo_dwarf_sectioncount;
    struct generic_macho_section * gsp = 0;
    sort_section_element * sort_el = 0;

    gsp = mfp->mo_dwarf_sections;

    sort_el = calloc(count,sizeof(sort_section_element));
    if (!sort_el) {
        P("ERROR: unable to allocate " LONGESTUFMT
            " section elements, cannot print sections\n",
            count);
        return;
    }
    for (i = 0; i < count; i++) {
        sort_el[i].od_originalindex = i;
        sort_el[i].od_sec_desc = (void *)(gsp+i);
    }
    if (options->co_sort_section_by_size) {
        qsort((void *)sort_el,count,
            sizeof(sort_section_element),
            compare_by_secsize);
    } else if (options->co_sort_section_by_name) {
        qsort((void *)sort_el,count,
            sizeof(sort_section_element),
            compare_by_secname);
    } /* else no sort, print as is */

    P(" Sections count: " LONGESTUFMT "  offset " LONGESTXFMT8 "\n",
        count,gsp->offset_of_sec_rec);
    P("                         addr      offset     size \n");

    for (i =0; i < count; ++i) {
        sort_section_element *sel = 0 ;
        Dwarf_Unsigned origindex = 0;
        Dwarf_Unsigned addr = 0;
        const char    *namestr = 0;

        sel = &sort_el[i];
        gsp = (struct generic_macho_section *)sel->od_sec_desc;
        origindex = sel->od_originalindex;
        namestr = gsp->sectname;
        addr = gsp->addr;

        P("  [" LONGESTUFMT2 "] %-16s"
            " " LONGESTXFMT8
            " " LONGESTXFMT8
            " " LONGESTXFMT8
            "\n",
            origindex,namestr,
            addr,
            gsp->offset,
            gsp->size);
    }
    free(sort_el);
}

static void
print_macho_commands(struct macho_filedata_s *mfp)
{
    Dwarf_Unsigned i = 0;
    struct generic_macho_command *cmdp = 0;

    cmdp = mfp->mo_commands;
    P(" Commands: at offset " LONGESTXFMT "\n",
        mfp->mo_command_start_offset);
    for ( ; i < mfp->mo_command_count; ++i, ++cmdp) {
        P("  [" LONGESTUFMT "] cmd: " LONGESTXFMT8
            " %-16s"
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
    P("Mach-o Magic:  " LONGESTXFMT "\n",mfp->mo_header.swappedmagic);
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
        get_filetype_name(mfp->mo_header.filetype));
    P("  number of commands: " LONGESTXFMT
        " ("LONGESTUFMT ")\n",
        mfp->mo_header.ncmds,mfp->mo_header.ncmds);
    P("  size of commands  : " LONGESTXFMT
        " ("LONGESTUFMT ")\n",
        mfp->mo_header.sizeofcmds, mfp->mo_header.sizeofcmds);
    P("  flags             : " LONGESTXFMT  ,
        (LONGESTUTYPE)mfp->mo_header.flags);
    print_all_flags(mfp->mo_header.flags);
}

static void
do_one_file(const char *s, sec_options *options)
{
    unsigned ftype = 0;
    unsigned endian = 0;
    unsigned offsetsize = 0;
    Dwarf_Unsigned filesize = 0;
    int res = 0;
    struct macho_filedata_s *mfp = 0;
    int errcode = 0;
    const char *local_path = 0;

    if (skip_dsym_check) {
        res = dwarf_object_detector_path(s,
            0,0,
            &ftype,&endian,&offsetsize,&filesize,&errcode);
        local_path = s;
        P("Reading name provided %s\n",local_path);
    } else {
        res = dwarf_object_detector_path(s,
            tru_path_buffer,BUFFERSIZE,
            &ftype,&endian,&offsetsize,&filesize,&errcode);
        local_path = (const char *)tru_path_buffer;
        if (strcmp(s,local_path)) {
            P("Reading object at %s rather than name provided.\n",
            local_path);
        }
    }
    if (res != DW_DLV_OK) {
        P("ERROR: Unable to read \"%s\", ignoring file. "
            "Errcode %d\n", s,errcode);
        return;
    }
    if (secoptionsdata.co_printfilenames) {
        P("Reading: %s (%s)\n",s,tru_path_buffer);
    }
    if (ftype != DW_FTYPE_MACH_O &&
        ftype != DW_FTYPE_APPLEUNIVERSAL) {
        P("File %s is not mach-o. Ignored.\n",tru_path_buffer);
        return;
    }
    res = dwarf_construct_macho_access_path(local_path,
        unibinarynumber,
        &mfp,&errcode);
    if (res != RO_OK) {
        P("Warning: Unable to open %s for detailed reading. "
            "Err %d\n",
            s,errcode);
        return;
    }
    res = dwarf_load_macho_header(mfp,&errcode);
    if (res != DW_DLV_OK) {
        P("Warning: %s macho-header not loaded giving up."
            "Error %d (%s)\n",
            local_path,errcode,dwarf_get_errname(errcode));
        dwarf_destruct_macho_access(mfp);
        return;
    }
    print_macho_header(mfp);
    res = dwarf_load_macho_commands(mfp,&errcode);
    if (res == DW_DLV_ERROR) {
        P("ERROR: load Macho data failed."
            "Error %d (%s)\n",
            errcode,dwarf_get_errname(errcode));
        dwarf_destruct_macho_access(mfp);
        exit(1);
    }
    if (res == DW_DLV_NO_ENTRY) {
        P("No dwarf sections exist in \"%s\"\n",local_path);
        dwarf_destruct_macho_access(mfp);
        exit(1);
    }
    print_macho_commands(mfp);
    print_macho_segments(mfp);
    print_macho_dwarf_sections(mfp,options);
    dwarf_destruct_macho_access(mfp);
}
