/*
Copyright (c) 2018, David Anderson
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

#include "config.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for exit(), C89 malloc */
#endif /* HAVE_STDLIB_H */
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#include "dwarf_types.h"
#include "dwarf_reading.h"
#include "dwarf_pe_descr.h" /* for flags */
#include "dwarf_object_detector.h"
#include "dwarf_object_read_common.h"
#include "dwarf_peread.h"
#include "sanitized.h"
#include "common_options.h"

#define BUFFERSIZE 1000
static char buffer1[BUFFERSIZE];
#define BUFFERSIZE 1000
static char buffer2[BUFFERSIZE];

#define TRUE 1
#define FALSE 0

char *Usage = "Usage: readobjpe <options> file ...\n"
    "Options:\n"
    "--sections-by-size sort sections by section size\n"
    "--sections-by-name sort sections by name\n"
    "--printfilenames print the name of the file being read\n"
    "--help     print this message\n"
    "--version  print version string\n";

static int
compare_by_secsize(const void *lin,const void *rin)
{
    sort_section_element * lsel = (sort_section_element *)lin;
    Dwarf_Unsigned lsecindex = lsel->od_originalindex;
    struct dwarf_pe_generic_image_section_header * lgsp =
        (struct dwarf_pe_generic_image_section_header *)
        lsel->od_sec_desc;
    Dwarf_Unsigned lsecsize = lgsp->VirtualSize;

    sort_section_element * rsel = (sort_section_element *)rin;
    Dwarf_Unsigned rsecindex = rsel->od_originalindex;
    struct dwarf_pe_generic_image_section_header * rgsp =
        (struct dwarf_pe_generic_image_section_header *)
        rsel->od_sec_desc;
    Dwarf_Unsigned rsecsize = rgsp->VirtualSize;

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
    struct dwarf_pe_generic_image_section_header * lgsp =
        (struct dwarf_pe_generic_image_section_header *)
        lsel->od_sec_desc;
    const char *lname = lgsp->name; /* or dwarfsectname? */

    sort_section_element * rsel = (sort_section_element *)rin;
    Dwarf_Unsigned rsecindex = rsel->od_originalindex;
    struct dwarf_pe_generic_image_section_header *rgsp =
        (struct dwarf_pe_generic_image_section_header *)
        rsel->od_sec_desc;
    const char *rname = rgsp->name; /* or dwarfsectname? */

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
pe_sections_display(dwarf_pe_object_access_internals_t *pep,
    sec_options *options)
{

    Dwarf_Unsigned count = pep->pe_section_count;
    sort_section_element * sort_el = 0;
    struct dwarf_pe_generic_image_section_header *gsp = 0;
    Dwarf_Unsigned i = 0;

    gsp = pep->pe_sectionptr;

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

    printf("Display " LONGESTUFMT " sections.\n",
        count);
    for (i = 0 ; i < count; ++i) {
        struct dwarf_pe_generic_image_section_header *sp = 0;
        sort_section_element *sel = 0 ;
        Dwarf_Unsigned origindex = 0;

        sel = &sort_el[i];
        origindex = sel->od_originalindex;
        sp = (struct dwarf_pe_generic_image_section_header *)
            sel->od_sec_desc;
        printf("Section " LONGESTUFMT " fileoff: "
            LONGESTXFMT8 "\n",
            origindex,sp->SecHeaderOffset);
        printf("  Name:  %20s   : %20s \n",
            sanitized(sp->name,buffer1,BUFFERSIZE),
            sanitized(sp->dwarfsectname,buffer2,BUFFERSIZE));
        printf("  VirtSize       : " LONGESTXFMT8
            "  (" LONGESTUFMT ")\n",
            sp->VirtualSize,sp->VirtualSize);
        if (sp->VirtualSize >= pep->pe_filesize) {
            printf("Error in PE section %lu: "
                "Section VirtualSize is 0x%lx but file size "
                "is 0x%lx\n",
                (unsigned long)origindex,
                (unsigned long)sp->VirtualSize,
                (unsigned long)pep->pe_filesize);
        }
        printf("  VirtAddr       : " LONGESTXFMT8
            "  (" LONGESTUFMT ")\n",
            sp->VirtualAddress,sp->VirtualAddress);
        printf("  Raw size       : " LONGESTXFMT8
            "  (" LONGESTUFMT ")\n",
            sp->SizeOfRawData,sp->SizeOfRawData);
        printf("  Ptr To Rawdata : " LONGESTXFMT8
            "  (" LONGESTUFMT ")\n",
            sp->PointerToRawData,sp->PointerToRawData);
        printf("  Characteristics: " LONGESTXFMT8
            "  (" LONGESTUFMT ")\n",
            sp->Characteristics,sp->Characteristics);
    }
    free(sort_el);
}
static void
pe_headers_display(dwarf_pe_object_access_internals_t *pe)
{
    struct dwarf_pe_generic_file_header *gfh = 0;
    struct dwarf_pe_generic_optional_header *ofh = 0;

    if (!pe->pe_path) {
        pe->pe_path = strdup("<null path name>");
    }
    printf("File: %s  Object type letter: %c version: %d\n",
        sanitized(pe->pe_path,buffer1,BUFFERSIZE),
        pe->pe_ident[0],
        pe->pe_ident[0]);
    printf(" 64bit?         : %s\n",pe->pe_is_64bit?"yes":"no");
    printf(" filesize       : " LONGESTUFMT "\n",
        pe->pe_filesize);
    printf(" offsetsize     : %d\n",pe->pe_offsetsize);
    printf(" pointersize    : %d\n",pe->pe_pointersize);
    printf(" ftype          : %d\n",pe->pe_ftype);
    printf(" endianness     : %d\n",(int)pe->pe_endian);
    printf(" nthdroffset    : " LONGESTXFMT8 "\n",
        pe->pe_nt_header_offset);
    printf(" Opt-hdr-offset : " LONGESTXFMT8 "\n",
        pe->pe_optional_header_offset);
    printf(" Opt-hdr-size   : " LONGESTXFMT8 "\n",
        pe->pe_optional_header_size);
    printf(" symtab-offset  : " LONGESTXFMT8 "\n",
        pe->pe_symbol_table_offset);
    printf(" strtab-offset  : " LONGESTXFMT8 "\n",
        pe->pe_string_table_offset);
    printf(" strtab-size    : " LONGESTXFMT8 "\n",
        pe->pe_string_table_size);
    printf(" sectable-offset: " LONGESTXFMT8 "\n",
        pe->pe_section_table_offset);

    printf("nt header\n");
    gfh = & pe->pe_FileHeader;
    printf("  machine       : " LONGESTXFMT8 "\n",
        gfh->Machine);
    printf("  NumberOfSects : " LONGESTXFMT8 "\n",
        gfh->NumberOfSections);
    printf("  PtrToSymTab   : " LONGESTXFMT8 "\n",
        gfh->PointerToSymbolTable);
    printf("  NumberofSyms  : " LONGESTXFMT8 "\n",
        gfh->NumberOfSymbols);
    printf("  OptHdrSize    : " LONGESTXFMT8 "\n",
        gfh->SizeOfOptionalHeader);
    printf("  Charctrstics  : " LONGESTXFMT8 "\n",
        gfh->Characteristics);

    printf("Optional Header\n");
    ofh = & pe->pe_OptionalHeader;
    printf("  Magic         : " LONGESTXFMT8 "\n",
        ofh->Magic);
    printf("  SectionAlign  : " LONGESTXFMT8 "\n",
        ofh->SectionAlignment);
    printf("  SizeOfHeadrs  : " LONGESTXFMT8 "\n",
        ofh->SizeOfHeaders);
    printf("  ImageBase     : " LONGESTXFMT8 "\n",
        ofh->ImageBase);
}

static void
do_one_file(const char *name, sec_options *options)
{
    unsigned ftype = 0;
    unsigned endian = 0;
    unsigned offsetsize = 0;
    dwarf_pe_object_access_internals_t *pep = 0;
    int      errcode = 0;
    int      res = 0;
    Dwarf_Unsigned filesize = 0;

    res = dwarf_object_detector_path(name,0,0,
        &ftype,&endian,&offsetsize,&filesize,&errcode);
    if (res != DW_DLV_OK) {
        printf("ERROR: Unable to read \"%s\", ignoring file. "
            "Errcode %s\n", name,dwarf_get_errname(errcode));
        return;
    }
    printf("Reading: %s\n",name);
    if (ftype !=  DW_FTYPE_PE) {
        printf("File %s is not pe. Ignored.\n",name);
        return;
    }
    res = dwarf_construct_pe_access_path(name,
        &pep,&errcode);
    if (res != RO_OK) {
        P("Warning: Unable to open %s for detailed reading."
            " errcode %d (%s)\n",
            name,errcode,dwarf_get_errname(errcode));
        return;
    }
#ifdef WORDS_BIGENDIAN
    if (endian == DW_ENDIAN_LITTLE ) {
        pep->pe_copy_word = dwarf_ro_memcpy_swap_bytes;
        pep->pe_endian = DW_ENDIAN_LITTLE;
    } else {
        pep->pe_copy_word = memcpy;
        pep->pe_endian = DW_ENDIAN_BIG;
    }
#else  /* LITTLE ENDIAN */
    if (endian == DW_ENDIAN_LITTLE) {
        pep->pe_copy_word = memcpy;
        pep->pe_endian = DW_ENDIAN_LITTLE;
    } else {
        pep->pe_copy_word = dwarf_ro_memcpy_swap_bytes;
        pep->pe_endian = DW_ENDIAN_BIG;
    }
#endif /* LITTLE- BIG-ENDIAN */
    pep->pe_filesize = filesize;
    pep->pe_offsetsize = offsetsize;

    res = dwarf_load_pe_sections(pep,&errcode);
    if (res != DW_DLV_OK) {
        P("Warning: %s pe-header not loaded giving up."
            " errcode %d (%s)\n",
            name,errcode,dwarf_get_errname(errcode));
        dwarf_destruct_pe_access(pep);
        return;
    }
    pe_headers_display(pep);
    pe_sections_display(pep,options);
    dwarf_destruct_pe_access(pep);
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

            if ((strcmp(argv[0],"--version") == 0) ||
                (strcmp(argv[0],"-v") == 0 )) {
                P("Version-readobjpe: %s\n",
                    PACKAGE_VERSION);
                printed_version = TRUE;
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
                P("No such file as %s\n",
                    sanitized(filename,buffer1,BUFFERSIZE));
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
