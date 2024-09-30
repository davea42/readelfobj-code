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

/*  This program attempts to read and print the headers of n
    Elf object or  a.out.

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
#include <time.h>
#ifdef HAVE_ELF_H
#include <elf.h>
#endif /* HAVE_ELF_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#include <unistd.h>
#include <errno.h>
#include "dwarf_types.h"
#include "dwarf_reading.h"
#include "dwarf_object_detector.h"
#include "dwarf_object_read_common.h"
#include "readelfobj.h"
#include "sanitized.h"
#include "dwarf_elf_reloc_aarch64.h"
#include "dwarf_elf_reloc_arm.h"
#include "dwarf_elf_reloc_386.h"
#include "dwarf_elf_reloc_mips.h"
#include "dwarf_elf_reloc_ppc.h"
#include "dwarf_elf_reloc_ppc64.h"
#include "dwarf_elf_reloc_sparc.h"
#include "dwarf_elf_reloc_x86_64.h"
#include "dwarfstring.h"
#include "dwarf_debuglink.h"
#include "common_options.h"

#define TRUE 1
#define FALSE 0

#ifdef HAVE_UNUSED_ATTRIBUTE
#define  UNUSEDARG __attribute__ ((unused))
#else
#define  UNUSEDARG
#endif

#ifndef IS_ELF
#define IS_ELF(e) (e.e_ident[0] == 0x7f && \
    e.e_ident[1]== 'E' && \
    e.e_ident[2]== 'L' && \
    e.e_ident[3]== 'F' )
#endif

#define ALIGN4 4
#define ALIGN8 8

static void do_one_file(const char *filename,sec_options*options);
static int elf_print_elf_header(elf_filedata ep);
static int elf_print_progheaders(elf_filedata ep);
static int elf_print_sectstrings(elf_filedata ep,Dwarf_Unsigned);
static int elf_print_sectheaders(elf_filedata ep,sec_options*options);
static int elf_print_gnu_debuglink(elf_filedata ep);
static int elf_print_sg_groups(elf_filedata ep);
static int elf_print_relocation_details(elf_filedata ep,int isrela,
    struct generic_shdr * gsh);
static int elf_print_symbols(elf_filedata ep,int is_symtab,
    struct generic_symentry * gsym,
    Dwarf_Unsigned ecount,
    const char *secname);

static void report_wasted_space(elf_filedata ep,sec_options*options);
static int elf_print_dynamic(elf_filedata ep);

int print_symtab_sections = 0; /* symtab dynsym */
int print_reloc_sections  = 0; /* .rel .rela */
int print_dynamic_sections  = 0; /* .dynamic */
int print_wasted  = 0; /* prints space use details */
int only_wasted_summary = 0; /* suppress standard printing */
int print_groups = 0; /* print group information. */
int print_sec_extra = 0; /* print address, inputfile offset */

static char buffer1[BUFFERSIZE];
static char buffer2[BUFFERSIZE];

char *filename;
FILE *fin;

char *Usage = "Usage: readelfobj <options> file ...\n"
    "Options:\n"
    "--print-dynamic print the .dynamic section (DT_ stuff)\n"
    "--print-groups  print the section group of each DWARF section\n"
    "--print-relocs  print relocation entries (.rela & .rel)\n"
    "--print-symtabs print out all elf symbols (.symtab & .dynsym)\n"
    "--print-wasted  print out details about file space use\n"
    "                beyond just the total wasted.\n"
    "--print-sec-extra print out section header address field\n"
    "                and the input file offset of the Shdr\n"
    "--sections-by-size sort sections by section size\n"
    "--sections-by-name sort_sections by name\n"
    "--only-wasted-summary  Skip printing section/segment data.\n"
    "--all           Enables all the above options\n"
    "--help          print this message\n"
    "--version       print version string\n";

int
main(int argc,char **argv)
{
    int i = 0;
    int filecount = 0;
    int printed_version = FALSE;

    if (argc == 1) {
        printf("%s\n",Usage);
        exit(1);
    } else {
        int myerr = 0;

        argv++;
        for (i =1; i<argc; i++,argv++) {
            if ((strcmp(argv[0],"--help") == 0) ||
                (strcmp(argv[0],"-h") == 0)) {
                P("%s",Usage);
                exit(0);
            }
            if (strcmp(argv[0],"--all") == 0) {
                print_symtab_sections= 1;
                print_wasted= 1;
                print_reloc_sections= 1;
                print_dynamic_sections= 1;
                print_groups= 1;
                print_sec_extra = 1;
                continue;
            }
            if (strcmp(argv[0],"--print-sec-extra") == 0) {
                print_sec_extra= 1;
                continue;
            }
            if (strcmp(argv[0],"--print-groups") == 0) {
                print_groups= 1;
                continue;
            }
            if (strcmp(argv[0],"--print-symtabs") == 0) {
                print_symtab_sections= 1;
                continue;
            }
            if (strcmp(argv[0],"--only-wasted-summary") == 0) {
                only_wasted_summary= 1;
                continue;
            }
            if (strcmp(argv[0],"--print-wasted") == 0) {
                print_wasted= 1;
                continue;
            }
            if (strcmp(argv[0],"--print-relocs") == 0) {
                print_reloc_sections= 1;
                continue;
            }
            if (strcmp(argv[0],"--print-dynamic") == 0) {
                print_dynamic_sections= 1;
                continue;
            }
            if (strcmp(argv[0],"--sections-by-size") == 0) {
                secoptionsdata.co_sort_section_by_size = TRUE;
                secoptionsdata.co_sort_section_by_name = FALSE;
                continue;
            }
            if (strcmp(argv[0],"--sections-by-name") == 0) {
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
                P("Version-readelfobj: %s\n",
                    PACKAGE_VERSION);
                printed_version = TRUE;
                continue;
            }
            filename = argv[0];
            if (secoptionsdata.co_printfilenames) {
                P("File: %s\n",sanitized(filename,buffer1,
                    BUFFERSIZE));
            }
            errno = 0;
            fin = fopen(filename,"r");
            if (fin == NULL) {
                myerr = errno;
                if (myerr) {
                    P("Cannot open %s. Errno %d %s\n",argv[0],
                        myerr,strerror(myerr));
                } else {
                    P("Cannot open %s\n",argv[0]);
                }
                continue;
            }
            ++filecount;
            fclose(fin);
            do_one_file(filename,&secoptionsdata);
        }
        if (!filecount && !printed_version) {
            printf("%s\n",Usage);
            exit(1);
        }
    }
    return RO_OK;
}

static int
compare_by_secsize(const void *lin,const void *rin)
{
    sort_section_element * lsel = (sort_section_element *)lin;
    Dwarf_Unsigned lsecindex = lsel->od_originalindex;
    struct generic_shdr *lgshdr =
        (struct generic_shdr *)lsel->od_sec_desc;
    Dwarf_Unsigned lsecsize = lgshdr->gh_size;

    sort_section_element * rsel = (sort_section_element *)rin;
    Dwarf_Unsigned rsecindex = rsel->od_originalindex;
    struct generic_shdr *rgshdr = 
        (struct generic_shdr *)rsel->od_sec_desc;
    Dwarf_Unsigned rsecsize = rgshdr->gh_size;

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
    struct generic_shdr *lgshdr =
        (struct generic_shdr *)lsel->od_sec_desc;
    const char *lname = lgshdr->gh_namestring;

    sort_section_element * rsel = (sort_section_element *)rin;
    Dwarf_Unsigned rsecindex = rsel->od_originalindex;
    struct generic_shdr *rgshdr =
        (struct generic_shdr *)rsel->od_sec_desc;
    const char *rname = rgshdr->gh_namestring;
 
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

static int
check_dynamic_section(elf_filedata ep)
{
    Dwarf_Unsigned pcount = ep->f_loc_phdr.g_count;
    struct generic_phdr *gphdr = ep->f_phdr;
    struct generic_shdr *gshdr = 0;
    Dwarf_Unsigned scount = ep->f_loc_shdr.g_count;
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned dynamic_p_offset = 0;
    Dwarf_Unsigned dynamic_p_length = 0;
    Dwarf_Unsigned dynamic_p_pnum = 0;
    Dwarf_Unsigned dynamic_s_offset = 0;
    Dwarf_Unsigned dynamic_s_length = 0;
    Dwarf_Unsigned dynamic_s_snum = 0;
    int foundp = FALSE;
    int founds = FALSE;

    if (!pcount) {
        /* nothing to do. */
        return DW_DLV_NO_ENTRY;
    }
    if (!scount) {
        /* nothing to do. */
        return DW_DLV_NO_ENTRY;
    }

    /*  In case of error reading headers count might now be zero */
    for ( i = 0; i < pcount; ++i,  gphdr++) {
        const char *typename =
            dwarf_get_elf_program_header_type_name(
            gphdr->gp_type, buffer1,BUFFERSIZE);

        /* The type name returned is in ( ) */
        if (!strcmp(typename, "(PT_DYNAMIC )")) {
            dynamic_p_offset = gphdr->gp_offset;
            dynamic_p_length = gphdr->gp_filesz;
            dynamic_p_pnum = i;
            foundp = TRUE;
            break;
        }
    }

    gshdr = ep->f_shdr;
    for (i = 0; i < scount; i++, ++gshdr) {
        const char *namestr = sanitized(gshdr->gh_namestring,
            buffer1,BUFFERSIZE);
        if (!strcmp(namestr,".dynamic")) {
            dynamic_s_offset = gshdr->gh_offset;
            dynamic_s_length = gshdr->gh_size;
            dynamic_s_snum = i;
            founds = TRUE;
            break;
        }
    }
    if (!foundp || !founds) {
        /* Nothing to do */
        return DW_DLV_NO_ENTRY;
    }
    if (dynamic_p_offset !=  dynamic_s_offset) {
        P("Warning: dynamic section error: ProgHeader  "
            LONGESTUFMT " PT_DYNAMIC"
            " offset " LONGESTXFMT " (" LONGESTUFMT ") "
            " but section " LONGESTUFMT " .dynamic"
            " offset " LONGESTXFMT " (" LONGESTUFMT ")\n",
            dynamic_p_pnum,
            dynamic_p_offset,
            dynamic_p_offset,
            dynamic_s_snum,
            dynamic_s_offset,
            dynamic_s_offset);
        return DW_DLV_ERROR;
    }
    if (dynamic_p_length !=  dynamic_s_length) {
        P("Warning: dynamic section error:  ProgHeader "
            LONGESTUFMT " PT_DYNAMIC"
            " length " LONGESTXFMT " (" LONGESTUFMT ") "
            " but section " LONGESTUFMT " .dynamic"
            " length " LONGESTXFMT " (" LONGESTUFMT ")\n",
            dynamic_p_pnum,
            dynamic_p_length,
            dynamic_p_length,
            dynamic_s_snum,
            dynamic_s_length,
            dynamic_s_length);
        return DW_DLV_ERROR;
    }
    return DW_DLV_OK;
}

static void
print_minimum(elf_filedata ep,sec_options *options)
{
    int res = 0;
    res = elf_print_elf_header(ep);
    if (res != DW_DLV_OK) {
        return;
    }
    res = elf_print_progheaders(ep);
    if (res != DW_DLV_OK) {
        return;
    }
    res = elf_print_sectstrings(ep, ep->f_ehdr->ge_shstrndx);
    if (res != DW_DLV_OK) {
        return;
    }
    res = elf_print_sectheaders(ep,options);
    if (res != DW_DLV_OK) {
        return;
    }
    elf_print_gnu_debuglink(ep);
}

static void
print_requested(elf_filedata ep,sec_options *options)
{
    int res = 0;

    if (!only_wasted_summary) {
        res = elf_print_elf_header(ep);
        if (res != DW_DLV_OK) {
            return;
        }
        res = elf_print_progheaders(ep);
        if (res != DW_DLV_OK) {
            return;
        }
        res = elf_print_sectstrings(ep,ep->f_ehdr->ge_shstrndx);
        if (res != DW_DLV_OK) {
            return;
        }
        res = elf_print_sectheaders(ep,options);
        if (res != DW_DLV_OK) {
            return;
        }
        res = elf_print_gnu_debuglink(ep);
    }
    if (print_groups) {
        elf_print_sg_groups(ep);
    }
    if (print_dynamic_sections) {
        elf_print_dynamic(ep);
    }
    if (print_symtab_sections ) {
        if (ep->f_symtab_sect_index) {
            struct generic_shdr * psh = ep->f_shdr +
                ep->f_symtab_sect_index;
            const char *namestr = psh->gh_namestring;
            Dwarf_Unsigned link = psh->gh_link;
            if ( link != ep->f_symtab_sect_strings_sect_index){
                P("ERROR: symtab link section " LONGESTUFMT
                    " mismatch with "
                    LONGESTUFMT " section\n",
                    link,
                    ep->f_symtab_sect_strings_sect_index);
                return;
            }
            elf_print_symbols(ep,TRUE,ep->f_symtab,
                ep->f_loc_symtab.g_count,namestr);
        }
        if (ep->f_dynsym_sect_index) {
            struct generic_shdr * psh = ep->f_shdr +
                ep->f_dynsym_sect_index;
            const char *namestr = psh->gh_namestring;
            Dwarf_Unsigned link = psh->gh_link;
            if ( link != ep->f_dynsym_sect_strings_sect_index){
                P("ERROR: dynsym link section " LONGESTUFMT
                    " mismatch with "
                    LONGESTUFMT " section\n",
                    link,
                    ep->f_dynsym_sect_strings_sect_index);
                return;
            }
            elf_print_symbols(ep,FALSE,ep->f_dynsym,
                ep->f_loc_dynsym.g_count,namestr);
        }
        if (!ep->f_dynsym_sect_index  &&
            !ep->f_symtab_sect_index) {
            P("No .symtab or .dynsym section in %s.\n",
                sanitized(filename,buffer1,BUFFERSIZE));
        }
    }
    if (print_reloc_sections) {
        unsigned reloc_count = 0;
        Dwarf_Unsigned i = 0;
        struct generic_shdr * psh = 0;

        P("Relocation Sections\n");
        P("{\n");
        psh = ep->f_shdr;
        for (i = 0;i < ep->f_loc_shdr.g_count; ++i,++psh) {
            const char *namestr = psh->gh_namestring;
            if (!strncmp(namestr,".rel.",5)) {
                ++reloc_count;
                elf_print_relocation_details(ep,FALSE,psh);
            } else if (!strncmp(namestr,".rela.",6)) {
                ++reloc_count;
                elf_print_relocation_details(ep,TRUE,psh);
            }
        }
        if (reloc_count == 0) {
            P("No .rel or .rela sections were found in %s\n",
                sanitized(filename,buffer1,BUFFERSIZE));
        }
        P("}\n");
    }
    if (ep->f_dynamic_sect_index &&
        ep->f_wasted_dynamic_space) {
        const char *p = "";
        if (options->co_printfilenames) {
            p = sanitized(filename,buffer1,BUFFERSIZE);
        }
        P("Warning %s: Wasted .dynamic section entries: "
            LONGESTXFMT " (" LONGESTUFMT ")\n",
            p,
            ep->f_wasted_dynamic_count,
            ep->f_wasted_dynamic_count);

        P("Warning %s: Wasted .dynamic section space  : "
            LONGESTXFMT " (" LONGESTUFMT ")\n",
            p,
            ep->f_wasted_dynamic_space,
            ep->f_wasted_dynamic_space);
    }
    check_dynamic_section(ep);
    report_wasted_space(ep,options);
}

char namebuffer[BUFFERSIZE*4];
static void
do_one_file(const char *s,sec_options *options)
{
    int res = 0;
    unsigned ftype = 0;
    unsigned endian = 0;
    unsigned offsetsize = 0;
    Dwarf_Unsigned filesize = 0;
    int errcode = 0;
    elf_filedata ep = 0;

    res = dwarf_object_detector_path(s,
        namebuffer,BUFFERSIZE*4, &ftype,
        &endian, &offsetsize, &filesize,
        &errcode);
    if (res != DW_DLV_OK) {
        printf("Not valid Elf: %s\n",s);
        return;
    }
    if (ftype != DW_FTYPE_ELF) {
        printf("Not Elf: %s\n",s);
        return;
    }

    res = dwarf_construct_elf_access_path(namebuffer, &ep,&errcode);
    if (res == DW_DLV_NO_ENTRY) {
        P("Unable to find %s. Something odd here. \n",namebuffer);
        return;
    } else if (res == DW_DLV_ERROR) {
        P("Unable to open %s. Err code %d (%s)\n",namebuffer,
            errcode,dwarf_get_errname(errcode));
        return;
    }

#ifdef WORDS_BIGENDIAN
    if (endian == DW_ENDIAN_LITTLE) {
        ep->f_copy_word = dwarf_ro_memcpy_swap_bytes;
    } else {
        ep->f_copy_word = memcpy;
    }
#else  /* LITTLE ENDIAN */
    if (endian == DW_ENDIAN_LITTLE) {
        ep->f_copy_word = memcpy;
    } else {
        ep->f_copy_word = dwarf_ro_memcpy_swap_bytes;
    }
#endif /* LITTLE- BIG-ENDIAN */
    ep->f_filesize = filesize;
    ep->f_offsetsize = offsetsize;

    /* If there are no .group sections this will remain at 3. */
    ep->f_sg_next_group_number = 3;

    res = dwarf_load_elf_header(ep,&errcode);
    if (res == DW_DLV_ERROR) {
        P("ERROR: unable to load elf header. errcode %d (%s)\n",
            errcode,dwarf_get_errname(errcode));
        dwarf_destruct_elf_access(ep,&errcode);
        return;
    }
    if (res == DW_DLV_NO_ENTRY) {
        print_minimum(ep,options);
        P("ERROR: unable to find elf header.\n");
        dwarf_destruct_elf_access(ep,&errcode);
        return;
    }

    res = dwarf_load_elf_sectheaders(ep,&errcode);
    if (res == DW_DLV_ERROR) {
        print_minimum(ep,options);
        P("ERROR: unable to load section headers, errcode %d (%s)\n",
            errcode,dwarf_get_errname(errcode));
        dwarf_destruct_elf_access(ep,&errcode);
        return;
    }

    res = dwarf_load_elf_progheaders(ep,&errcode);
    if (res == DW_DLV_ERROR) {
        print_minimum(ep,options);
        P("ERROR: unable to load program headers. errcode %d (%s)\n",
            errcode,dwarf_get_errname(errcode));
        dwarf_destruct_elf_access(ep,&errcode);
        return;
    }

    res = dwarf_load_elf_symstr(ep,&errcode);
    if (res == DW_DLV_ERROR) {
        print_minimum(ep,options);
        P("ERROR: unable to load symbol table strings."
            " errcode %d (%s)\n",
            errcode,dwarf_get_errname(errcode));
        dwarf_destruct_elf_access(ep,&errcode);
        return;
    }
    res = dwarf_load_elf_dynstr(ep,&errcode);
    if (res == DW_DLV_ERROR) {
        print_minimum(ep,options);
        P("ERROR: unable to load dynamic section strings."
            " errcode %d (%s)\n",
            errcode,dwarf_get_errname(errcode));
        dwarf_destruct_elf_access(ep,&errcode);
        return;
    }
    if (print_groups) {
        elf_print_sg_groups(ep);
    }
    res = dwarf_load_elf_dynamic(ep,&errcode);
    if (res == DW_DLV_ERROR) {
        print_minimum(ep,options);
        P("ERROR: Unable to load dynamic section,"
            " errcode %d (%s)\n",
            errcode,dwarf_get_errname(errcode));
    }
    res = dwarf_load_elf_dynsym_symbols(ep,&errcode);
    if (res == DW_DLV_ERROR) {
        print_minimum(ep,options);
        P("ERROR: Unable to load .dynsym section."
            " errcode %d (%s)\n",
            errcode,dwarf_get_errname(errcode));
    }
    res  =dwarf_load_elf_symtab_symbols(ep,&errcode);
    if (res == DW_DLV_ERROR) {
        print_minimum(ep,options);
        P("ERROR: Unable to load .symtab section."
            " errcode %d (%s)\n",
            errcode,dwarf_get_errname(errcode));
    }
    {
        Dwarf_Unsigned i = 0;
        struct generic_shdr *psh = ep->f_shdr;

        for (i = 0;i < ep->f_loc_shdr.g_count; ++i,++psh) {
            const char *namestr = psh->gh_namestring;
            if (!strncmp(namestr,".rel.",5)) {
                res = dwarf_load_elf_rel(ep,i,&errcode);
                if (res == DW_DLV_ERROR) {
                    print_minimum(ep,options);
                    P("ERROR reading .rel section "
                        LONGESTUFMT " Error code %d (%s) file:%s \n",
                        i,errcode,dwarf_get_errname(errcode),
                        sanitized(filename,buffer1,BUFFERSIZE));
                    P("ERROR attempting to continue\n");
                    dwarf_destruct_elf_access(ep,&errcode);
                    return;
                }
            } else if (!strncmp(namestr,".rela.",6)) {
                res = dwarf_load_elf_rela(ep,i,&errcode);
                if (res == DW_DLV_ERROR) {
                    print_minimum(ep,options);
                    P("ERROR reading .rela section "
                        LONGESTUFMT
                        " %s Error code %d (%s) file:%s \n",
                        i,
                        sanitized(namestr,buffer2,BUFFERSIZE),
                        errcode,dwarf_get_errname(errcode),
                        sanitized(filename,buffer1,BUFFERSIZE));
                    dwarf_destruct_elf_access(ep,&errcode);
                    return;
                }
            }
        }
    }
    print_requested(ep,options);
    dwarf_destruct_elf_access(ep,&errcode);
}
static int
elf_print_sectstrings(elf_filedata ep,Dwarf_Unsigned stringsection)
{
    struct generic_shdr *psh = 0;
    if (!stringsection) {
        P("Section strings missing String section number %lu\n",
            (unsigned long)stringsection);
        return DW_DLV_OK;
    }
    if (!ep->f_shdr) {
        P("Section strings never found. String section number %lu\n",
            (unsigned long)stringsection);
        return DW_DLV_OK;
    }
    if (stringsection >= ep->f_ehdr->ge_shnum) {
        printf("String section " LONGESTUFMT " invalid. Ignored.",
            stringsection);
        return DW_DLV_OK;
    }

    psh = ep->f_shdr + stringsection;;
    P("String section data at " LONGESTXFMT " (" LONGESTUFMT ")"
        " length " LONGESTXFMT " (" LONGESTUFMT ")" "\n",
        psh->gh_offset, psh->gh_offset,
        psh->gh_size,psh->gh_size);
    return DW_DLV_OK;
}

/*  We're not creating a generic function for this,
    such is not needed for normal reading DWARF.  */
static void
elf_load_print_interp(elf_filedata ep,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned size)
{
    long res = 0;
    char *buf = 0;
    int errcode = 0;

    if ((offset > ep->f_filesize)||
        (size > ep->f_filesize) ||
        ((size +offset) > ep->f_filesize)) {
            P("ERROR: Something badly wrong with interpreter name "
                " filesize " LONGESTUFMT
                " offset " LONGESTUFMT
                " size " LONGESTUFMT
                "\n", ep->f_filesize,offset,size);
            return;
    }
    buf = malloc(size+1);
    if (buf == 0) {
        P("ERROR: malloc failed reading interpreter data\n");
        return;
    }
    res = RRMOA(ep->f_fd,buf,offset,size,
        ep->f_filesize,&errcode);
    if (res != RO_OK) {
        free(buf);
        P("ERROR: Read interp string failed\n");
        return;
    }
    /* forcing null terminator */
    buf[size] = 0;
    res = _dwarf_check_string_valid(buf,buf,buf+size,
        DW_DLE_FORM_STRING_BAD_STRING,
        &errcode);
    if (res != RO_OK) {
        free(buf);
        P("ERROR: Interp string is not null terminated\n");
        return;
    }
    P("    Interpreter:  %s\n",sanitized(buf,buffer1,BUFFERSIZE));
    free(buf);
    return;
}

static int
elf_print_progheaders(elf_filedata ep)
{
    Dwarf_Unsigned count = ep->f_loc_phdr.g_count;
    struct generic_phdr *gphdr = ep->f_phdr;
    Dwarf_Unsigned i = 0;

    if (!ep->f_phdr) {
        return DW_DLV_OK;
    }
    /*  In case of error reading headers count might now be zero */
    P("\n");
    P("Program header count: " LONGESTUFMT "\n",count);
    if (!count) {
        P("\n");
        return DW_DLV_OK;
    }
    P("{\n");
    for ( i = 0; i < count; ++i,  gphdr++) {
        P("Program header " LONGESTUFMT ,i);
        P("  type %s " LONGESTXFMT,
            dwarf_get_elf_program_header_type_name(gphdr->gp_type,
                buffer1,BUFFERSIZE),
            gphdr->gp_type);
        P("\n");
        P("  offset " LONGESTXFMT " (" LONGESTUFMT ")",
            gphdr->gp_offset,gphdr->gp_offset);
        P(", vaddr " LONGESTXFMT " (" LONGESTUFMT ")",
            gphdr->gp_vaddr, gphdr->gp_vaddr);
        P(", paddr " LONGESTXFMT " (" LONGESTUFMT ")",
            gphdr->gp_paddr,gphdr->gp_paddr);
        P("\n");
        P("  filesz " LONGESTXFMT " (" LONGESTUFMT ")",
            gphdr->gp_filesz,gphdr->gp_filesz);
        P(", memsz " LONGESTXFMT " (" LONGESTUFMT ")",
            gphdr->gp_memsz,gphdr->gp_memsz);
        P("\n");

        P("  flags " LONGESTXFMT ,gphdr->gp_flags);
        if (gphdr->gp_flags & PF_X) {
            P(" PF_X");
        }
        if (gphdr->gp_flags & PF_W) {
            P(" PF_W");
        }
        if (gphdr->gp_flags & PF_R) {
            P(" PF_R");
        }
        P(", align " LONGESTXFMT " (" LONGESTUFMT ")",
            gphdr->gp_align,gphdr->gp_align);
        P("\n");
        if (gphdr->gp_type == PT_INTERP) {
            elf_load_print_interp(ep,gphdr->gp_offset,
                gphdr->gp_filesz);
        }
    }
    P("}\n");
    return DW_DLV_OK;
}

static int
elf_print_sectheaders(elf_filedata ep,sec_options *options)
{
    struct generic_shdr *gshdr = 0;
    Dwarf_Unsigned generic_count = 0;
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned debug_sect_count = 0;
    Dwarf_Unsigned debug_sect_size = 0;
    sort_section_element * sort_el = 0;

    gshdr = ep->f_shdr;
    if (!gshdr) {
        return DW_DLV_OK;
    }
    generic_count = ep->f_loc_shdr.g_count;
    P("\n");
    P("Section count: " LONGESTUFMT "\n",generic_count);
    if (!generic_count) {
        P("\n");
        return DW_DLV_OK;
    }
    P(" [i] offset      size        name         "
        "addr     (flags)(type)(link,info,align)\n");
    P("{\n");
    sort_el = calloc(generic_count,sizeof(sort_section_element));
    if (!sort_el) {
        P("ERROR: unable to allocate " LONGESTUFMT 
            " section elements, cannot print section\n",
            generic_count);
            return DW_DLV_OK;
    }
    for (i = 0; i < generic_count; i++) {
         sort_el[i].od_originalindex = i;
         sort_el[i].od_sec_desc = (void *)(gshdr+i);
    }
    if (options->co_sort_section_by_size) {
        qsort((void *)sort_el,generic_count,
            sizeof(sort_section_element),
            compare_by_secsize);
    } else if (options->co_sort_section_by_name) {
        qsort((void *)sort_el,generic_count,
            sizeof(sort_section_element),
            compare_by_secname);
    } /* else no sort, print as is */
    for (i = 0; i < generic_count; i++) {
        const char *namestr = 0;
        sort_section_element *sel = 0 ;
        Dwarf_Unsigned origindex = 0;
        

        sel = &sort_el[i];
        gshdr = (struct generic_shdr *)sel->od_sec_desc;
        origindex = sel->od_originalindex;
        namestr = sanitized(gshdr->gh_namestring,
            buffer1,BUFFERSIZE);
        if (dwarf_load_elf_section_is_dwarf(namestr)) {
            debug_sect_count++;
            debug_sect_size += gshdr->gh_size;
        }
        P("[" LONGESTUFMT2 "]", origindex);
        P(" " LONGESTXFMT8,gshdr->gh_offset);
        P(" " LONGESTXFMT8,gshdr->gh_size);
        /*P(" (" LONGESTUFMT8 ") ",gshdr->gh_size); */
        P(" %-14s",namestr);
        /*P(" "  LONGESTXFMT,gshdr->gh_flags); */
        if (!gshdr->gh_flags) {
            P(" (0)");
        } else {
            P(" %s",
                dwarf_get_elf_section_header_flag_names(
                    gshdr->gh_flags,
                    buffer2,BUFFERSIZE));
        }
        P("%s",dwarf_get_elf_section_header_st_type(gshdr->gh_type,
            buffer2,BUFFERSIZE));
        if (gshdr->gh_link || gshdr->gh_info || gshdr->gh_addralign) {
            P("(" LONGESTUFMT ,gshdr->gh_link);
            P("," LONGESTXFMT ,gshdr->gh_info);
            P("," LONGESTXFMT ")" ,gshdr->gh_addralign);
        }
        P("\n");
        if (print_sec_extra) {
            P("    Hdroffset: " LONGESTXFMT8,gshdr->gh_fdoffset);
            P("\n");
            P("    Addr     : " LONGESTXFMT8,gshdr->gh_addr);
            P("\n");
        }
        if ( gshdr->gh_type == SHT_REL ||
            gshdr->gh_type == SHT_RELA) {

            if ( gshdr->gh_type == SHT_REL &&
                strncmp(namestr,".rel.",5) ) {
                P("Warning: Section " LONGESTUFMT " %s"
                    " is an SHT_REL relocation section but its name "
                    " does not start with \".rel.\"\n",i,namestr);

            } else if ( gshdr->gh_type == SHT_RELA &&
                strncmp(namestr,".rela.",6) ) {
                P("Warning: Section " LONGESTUFMT " %s"
                    " is an SHT_RELA relocation section but its name "
                    " does not start with \".rela.\"\n",i,namestr);
            }

            if (gshdr->gh_link >= generic_count) {
                P("Warning: Section " LONGESTUFMT " %s"
                    " is a relocation section but sh_link"
                    " is not a valid section index\n",i,namestr);
            } else {
                struct generic_shdr *tshdr = 0;
                tshdr = ep->f_shdr + gshdr->gh_link;
                if (tshdr->gh_type != SHT_SYMTAB &&
                    tshdr->gh_type != SHT_DYNSYM ) {
                    P("Warning: Section " LONGESTUFMT " %s"
                        " is a relocation section but sh_link"
                        " does not index a symtab\n",i,namestr);
                }
            }
            if (gshdr->gh_info >= generic_count) {
                P("Warning: Section " LONGESTUFMT " %s"
                    " is a relocation section but sh_info"
                    " is not a valid section index\n",i,namestr);
            }
        } else if (gshdr->gh_type == SHT_SYMTAB  ||
            gshdr->gh_type == SHT_DYNSYM ) {

            if (gshdr->gh_link >= generic_count) {
                P("Warning: Section " LONGESTUFMT " %s"
                    " is a symtab/dynsym section but sh_link"
                    " is not a valid string table index\n",i,namestr);
            } else {
                struct generic_shdr *tshdr = 0;

                tshdr = ep->f_shdr + gshdr->gh_link;
                if (tshdr->gh_type != SHT_STRTAB) {
                    P("Warning: Section " LONGESTUFMT " %s"
                        " is a symtab/dynsym section but sh_info"
                        " is not a valid string table index\n",
                        i,namestr);
                }
            }
        }
        if (gshdr->gh_relcount && !ep->f_symtab_sect_index) {
            P("Warning: Section " LONGESTUFMT " %s is a "
                "relocation section"
                " but there is no .symtab. "
                "Possibly invalid relocations.\n",
                i,namestr);
        }
    }
    P("Summary: " LONGESTUFMT " bytes for "
        LONGESTUFMT " debug sections\n",
        debug_sect_size,debug_sect_count);
    P("}\n");
    free(sort_el);
    return DW_DLV_OK;
}

static int
elf_print_symbols(elf_filedata ep,
    int is_symtab,
    struct generic_symentry * gsym,
    Dwarf_Unsigned ecount,
    const char *secname)
{
    Dwarf_Unsigned i = 0;
    struct location *locp = 0;

    if (is_symtab) {
        locp = &ep->f_loc_symtab;
    } else {
        locp = &ep->f_loc_dynsym;
    }
    if (!locp) {
        P("ERROR: the %s symbols section missing, not printable\n",
            secname);
        return DW_DLV_OK;
    }
    P("\n");
    P("Symbols from %s: " LONGESTUFMT
        " at offset " LONGESTXFMT "\n",
        sanitized(secname,buffer1,BUFFERSIZE),ecount,
        locp->g_offset);
    P("{\n");
    if (ecount > 0) {
        P("[Index] Value    Size    Type              "
            "Bind       Other          Shndx   Name\n");
    }

    for (i = 0; i < ecount; ++i,++gsym) {
        int errcode = 0;
        int res;
        const char *localstr = 0;
        struct generic_shdr *shp = 0;
        const char *targetsecname = "";

        P("[%3d]",(int)i);
        P("  st_value "
            LONGESTXFMT " (" LONGESTUFMT ")",
            gsym->gs_value,
            gsym->gs_value);
        P("\n");

        P("  st_size  "
            LONGESTXFMT " (" LONGESTUFMT ")",
            gsym->gs_size,
            gsym->gs_size);
        P(", st_info "
            LONGESTXFMT " (" LONGESTUFMT ")",
            gsym->gs_info,
            gsym->gs_info);
        P("\n");

        P("  type "
            LONGESTXFMT " (" LONGESTUFMT ") %s",
            gsym->gs_type,gsym->gs_type,
            dwarf_get_elf_symbol_stt_type(gsym->gs_type,
                buffer2, BUFFERSIZE));
        P(", bind "
            LONGESTXFMT " (" LONGESTUFMT ") %s",
            gsym->gs_bind,gsym->gs_bind,
            dwarf_get_elf_symbol_stb_string(gsym->gs_bind,
                buffer2,BUFFERSIZE));
        P("\n");

        P("  st_other "
            LONGESTXFMT " (" LONGESTUFMT ") %s",
            gsym->gs_other,
            gsym->gs_other,
            dwarf_get_elf_symbol_sto_type(gsym->gs_other,
                buffer2, BUFFERSIZE));
        P(", st_shndx " LONGESTUFMT, gsym->gs_shndx);
        if (gsym->gs_shndx < ep->f_loc_shdr.g_count) {
            shp = ep->f_shdr + gsym->gs_shndx;
            targetsecname = shp->gh_namestring;
        } else {
            targetsecname = dwarf_get_elf_symbol_shn_type(
                gsym->gs_shndx,buffer2,BUFFERSIZE);
        }
        P(" %s",targetsecname?targetsecname:"<no-name!>");
        P("\n");
        res = dwarf_get_elf_symstr_string(ep,
            is_symtab,gsym->gs_name,
            &localstr,&errcode);
        if (res != DW_DLV_OK ) {
            P("  ERROR: st_name access %s "
                " entry " LONGESTUFMT
                " with index " LONGESTUFMT
                " (" LONGESTXFMT ")"
                " fails with err code %d\n",
                is_symtab?".symtab":".dynsym",
                i, gsym->gs_name, gsym->gs_name,
                errcode);
        } else {
            unsigned long slen = 0;
            if ( ! localstr[0] ) {
                localstr = "<no-name>";
            } else {
                slen = (unsigned long)strlen(localstr);
            }
            P("  st_name  (" LONGESTUFMT ") name-length %2lu: %s",
                gsym->gs_name,
                slen,
                sanitized(localstr,buffer1,BUFFERSIZE));
            P("\n");
        }
    }
    P("}\n");
    return DW_DLV_OK;
}

static int
get_elf_symtab_symbol_name( elf_filedata ep,
    unsigned long symnum,
    const char **localstr_out,
    int *errcode)
{
    int is_symtab = TRUE;
    int res = 0;

    struct generic_symentry *gsym = 0;
    if (symnum >= ep->f_loc_symtab.g_count) {
        return DW_DLV_NO_ENTRY;
    }
    gsym = ep->f_symtab + symnum;
    if (gsym->gs_type == STT_SECTION) {
        Dwarf_Unsigned secnum = gsym->gs_shndx;
        struct generic_shdr *shdr = 0;
        if (secnum < ep->f_ehdr->ge_shnum) {
            shdr = ep->f_shdr + secnum;
            *localstr_out = shdr->gh_namestring;
            return DW_DLV_OK;
        }
    }
    res = dwarf_get_elf_symstr_string(ep,
        is_symtab,gsym->gs_name,
        localstr_out,errcode);
    return res;
}

static int
get_elf_reloc_name(
    Dwarf_Unsigned machine,
    Dwarf_Unsigned type,
    const char **typename_out)
{
    const char *tname = 0;

    switch(machine) {
    case EM_386:
        tname = dwarf_get_elf_relocname_386(type);
        break;
    case EM_X86_64:
        tname = dwarf_get_elf_relocname_x86_64(type);
        break;
    case EM_PPC:
        tname =dwarf_get_elf_relocname_ppc(type);
        break;
    case EM_PPC64:
        tname = dwarf_get_elf_relocname_ppc64(type);
        break;
    case EM_ARM:
        tname = dwarf_get_elf_relocname_arm(type);
        break;
    case EM_AARCH64:
        tname = dwarf_get_elf_relocname_aarch64(type);
        break;
    case EM_MIPS:
    case EM_MIPS_RS3_LE:
    case EM_MIPS_X:
        tname= dwarf_get_elf_relocname_mips(type);
        break;
    case EM_SPARCV9:
    case EM_SPARC:
    case EM_SPARC32PLUS:
        tname= dwarf_get_elf_relocname_sparc(type);
        break;
    default:
        tname = "(name uncertain)";
        break;
    }
    *typename_out = tname;
    return DW_DLV_OK;
}

static void
elf_print_relocation_content(
    elf_filedata ep,
    int isrela,
    struct generic_shdr * gsh,
    struct generic_rela *grela, Dwarf_Unsigned count)
{
    Dwarf_Unsigned i = 0;
    int is64bit = (ep->f_offsetsize == 64);
    int ismips = (ep->f_machine == EM_MIPS);

    P("\n");
    P("Section " LONGESTUFMT ": %s reloccount: " LONGESTUFMT
        " links-sec: " LONGESTUFMT
        " symtabsec: " LONGESTUFMT "\n",
        gsh->gh_secnum,
        sanitized(gsh->gh_namestring,buffer1,BUFFERSIZE),
        count,gsh->gh_info,
        gsh->gh_link);

    P(" [i]   offset   info           type.              "
        "symbol %s\n",isrela?
        " addend":"");
    for (i = 0; i < count; ++i,grela++) {
        int errcode = 0;
        const char *symname = "";
        const char *typename = "";

        if (grela->gr_sym != STN_UNDEF) {
            get_elf_symtab_symbol_name(
                ep,
                grela->gr_sym,
                &symname,
                &errcode);
        } else {
            symname = (char *)"STN_UNDEF";
        }
        if (grela->gr_type) {
            get_elf_reloc_name(ep->f_ehdr->ge_machine,
                grela->gr_type,&typename);
        }

        P("[" LONGESTUFMT "] ",i);
        P(" "
            LONGESTXFMT8,
            grela->gr_offset);
        P(" "
            LONGESTXFMT8,
            grela->gr_info);
        P(" "
            "%-14s "
            LONGESTUFMT ".",
            typename,
            grela->gr_type);
        P(" "
            LONGESTUFMT " %s",
            grela->gr_sym,
            symname?sanitized(symname,buffer1,BUFFERSIZE):"<noname>");
        if (isrela) {
            if (grela->gr_addend < 0) {
                P(" "
                    LONGESTSFMT,
                    grela->gr_addend);
            } else {
                P(" +"
                    LONGESTSFMT,
                    grela->gr_addend);
            }
        }
        P("\n");
        if (ismips && is64bit) {
            const char *typenx =0;
            if (grela->gr_type2) {
                get_elf_reloc_name(ep->f_ehdr->ge_machine,
                    grela->gr_type2,&typenx);
                P("         Type2: %u %s\n",grela->gr_type2,typenx);
            }
            typenx = 0;
            if (grela->gr_type3) {
                get_elf_reloc_name(ep->f_ehdr->ge_machine,
                    grela->gr_type3,&typenx);
                P("         Type3: %u %s\n",grela->gr_type3,typenx);
            }
        }
    }
    return;
}

static int
elf_print_relocation_details(
    elf_filedata ep,
    int isrela,
    struct generic_shdr * gsh)
{
    struct generic_rela *grela = 0;
    Dwarf_Unsigned count  = 0;

    count = gsh->gh_relcount;
    grela = gsh->gh_rels;
    elf_print_relocation_content(ep,isrela,gsh,grela,count);
    return DW_DLV_OK;
}

static int
elf_print_elf_header(elf_filedata ep)
{
    Dwarf_Unsigned i = 0;
    int c = 0;

    if (secoptionsdata.co_printfilenames) {
        P("Elf object file %s\n",
            sanitized(filename,buffer1,BUFFERSIZE));
    } else {
        P("Elf object file.\n");
    }
    if (!ep->f_ehdr) {
        P("No header data available\n");
        return DW_DLV_NO_ENTRY;
    }
    P(" Elf Header ident bytes: ");
    for (i = 0; i < EI_NIDENT; i++) {
        if (!(i%4)) {
            P(" ");
        }
        c = ep->f_ehdr->ge_ident[i];
        P("%02x",c);
    }
    P("\n");
    i = ep->f_ehdr->ge_ident[EI_CLASS];
    P("  File class    = " LONGESTXFMT " %s\n",i,
        (i == ELFCLASSNONE)? "(ELFCLASSNONE)":
        (i == ELFCLASS32) ? "(ELFCLASS32)" :
        (i == ELFCLASS64) ? "(ELFCLASS64" :
        "(unknown)");
    c = ep->f_ehdr->ge_ident[EI_DATA];
    P("  Data encoding = %#x %s\n",c,(c == ELFDATANONE)?
        "(ELFDATANONE)":
        (c == ELFDATA2MSB)? "(ELFDATA2MSB)":
        (c == ELFDATA2LSB) ? "(ELFDATA2LSB)":
        "(Invalid object encoding)");
    i = ep->f_ehdr->ge_ident[EI_VERSION];
    P("  file version  = " LONGESTXFMT " %s\n", i,
        (i == EV_CURRENT)? "(EV_CURRENT)":
        "(unknown)");
    i = ep->f_ehdr->ge_ident[EI_OSABI];
    P("  OS ABI        = " LONGESTXFMT " %s\n",
        i,
        dwarf_get_elf_osabi_name(i,buffer1,BUFFERSIZE));

    i = ep->f_ehdr->ge_ident[EI_ABIVERSION];
    P("  ABI version   = " LONGESTXFMT "\n",i);
    i = ep->f_ehdr->ge_type;
    P("  e_type     : " LONGESTXFMT " (%s)\n",i,(i == ET_NONE)?
        "ET_NONE No file type":
        (i == ET_REL)? "ET_REL Relocatable file":
        (i == ET_EXEC)? "ET_EXEC Executable file":
        (i == ET_DYN)? "ET_DYN Shared object file":
        (i == ET_CORE) ? "ET_CORE Core file":
        (i >= 0xff00 && i < 0xffff)? "Processor-specific type":
        "unknown");
    /*  See http://www.uxsglobal.com/developers/gabi/latest/
        ch4.eheader.html  */
    P("  e_machine  : " LONGESTXFMT" (%s)\n",ep->f_ehdr->ge_machine,
        dwarf_get_elf_machine_name(ep->f_ehdr->ge_machine));
    P("  e_version  : " LONGESTXFMT  "\n", ep->f_ehdr->ge_version);
    P("  e_entry    : " LONGESTXFMT8 "\n", ep->f_ehdr->ge_entry);
    P("  e_phoff    : " LONGESTXFMT8 "\n", ep->f_ehdr->ge_phoff);
    P("  e_shoff    : " LONGESTXFMT8 "\n", ep->f_ehdr->ge_shoff);
    P("  e_flags    : " LONGESTXFMT  "\n", ep->f_ehdr->ge_flags);
    P("  e_ehsize   : " LONGESTXFMT  "\n", ep->f_ehdr->ge_ehsize);
    P("  e_phentsize: " LONGESTXFMT  "\n", ep->f_ehdr->ge_phentsize);
    P("  e_phnum    : " LONGESTXFMT  "\n", ep->f_ehdr->ge_phnum);
    P("  e_shentsize: " LONGESTXFMT  "\n", ep->f_ehdr->ge_shentsize);
    if (ep->f_ehdr->ge_shnum_extended) {
        P("  e_shnum    :  is in extended form, value from "
            "section zero\n");
    }
    if (ep->f_ehdr->ge_shnum_in_shnum) {
        printf("  e_shnum    : " LONGESTXFMT " ("
            LONGESTUFMT") Number of sections\n",
            ep->f_ehdr->ge_shnum,
            ep->f_ehdr->ge_shnum);
    } else {
        P("ERROR:  e_shnum    : never set properly\n");
    }
    if (ep->f_ehdr->ge_strndx_extended) {
        P("  e_shstrndx : is in extended form, value from "
            "section zero\n");
    }
    if (ep->f_ehdr->ge_strndx_in_strndx) {
        printf("  e_shstrndx : " LONGESTXFMT " ("
            LONGESTUFMT ") Section strings section\n",
            ep->f_ehdr->ge_shstrndx,
            ep->f_ehdr->ge_shstrndx);
    } else {
        P("ERROR  e_shstrndx : never set properly\n");
    }
    if (ep->f_ehdr->ge_shstrndx == SHN_UNDEF) {
        P("  Section strings are not present e_shstrndx"
            "==SHN_UNDEF\n");
    }
    if (ep->f_ehdr->ge_shstrndx > ep->f_ehdr->ge_shnum) {
        P("String section index is wrong: "
            LONGESTUFMT " vs only "
            LONGESTUFMT " sections."
            " Consider it 0\n",
            ep->f_ehdr->ge_shstrndx,
            ep->f_ehdr->ge_shnum);
        ep->f_ehdr->ge_shstrndx = 0;
    }
    return DW_DLV_OK;
}

#define MAXWBLOCK 100000

static int
is_wasted_space_zero(elf_filedata ep,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned length,
    int *wasted_space_zero)
{
    Dwarf_Unsigned remaining = length;
    char *allocspace = 0;
    Dwarf_Unsigned alloclen = length;
    Dwarf_Unsigned checklen = length;
    int errcode = 0;

    if (length > MAXWBLOCK) {
        alloclen = MAXWBLOCK;
        checklen = MAXWBLOCK;
    }
    allocspace = (char *)malloc(alloclen);
    if (!allocspace) {
        P("Unable to malloc " LONGESTUFMT
            "bytes for zero checking.\n",
            alloclen);
        return RO_ERROR;
    }
    while (remaining) {
        Dwarf_Unsigned i = 0;
        int res = 0;

        if (remaining < checklen) {
            checklen = remaining;
        }
        res = RRMOA(ep->f_fd,allocspace,offset,checklen,
            ep->f_filesize,&errcode);
        if (res == RO_ERROR) {
            free(allocspace);
            P("ERROR: could not read wasted space at offset "
                LONGESTXFMT " properly. errcode %d (%s)\n",
                offset,errcode,dwarf_get_errname(errcode));
            return res;
        }
        for (i = 0; i < checklen; ++i) {
            if (allocspace[i]) {
                free(allocspace);
                *wasted_space_zero = FALSE;
                return RO_OK;;
            }
        }
        remaining -= checklen;
    }
    *wasted_space_zero = TRUE;
    free(allocspace);
    return RO_OK;
}

static int
comproffset(const void *l_in, const void *r_in)
{
    const struct in_use_s *l = l_in;
    const struct in_use_s *r = r_in;
    int strfield = 0;

    if (l->u_offset < r->u_offset) {
        return -1;
    }
    if (l->u_offset > r->u_offset) {
        return 1;
    }
    if (l->u_lastbyte < r->u_lastbyte) {
        return -1;
    }
    if (l->u_lastbyte > r->u_lastbyte) {
        return 1;
    }
    /*  When shdr and phdr (etc) use a specific area
        we want a consistent ordering of the report
        so regression tests work right. */
    strfield = strcmp(l->u_name,r->u_name);
    return strfield;
}

static void
report_wasted_space(elf_filedata  ep,sec_options *options)
{
    Dwarf_Unsigned filesize = ep->f_filesize;
    Dwarf_Unsigned iucount = ep->f_in_use_count;
    Dwarf_Unsigned i = 0;
    int res = 0;

    struct in_use_s *iuarray = 0;
    struct in_use_s *iupa = 0;
    struct in_use_s *iupl = 0;
    struct in_use_s *nxt = 0;
    Dwarf_Unsigned highoffset = 0;
    struct in_use_s low_instance;
    int firstinstance = TRUE;

    if (!iucount) {
        P("No content found in %s\n",
            sanitized(filename,buffer1,BUFFERSIZE));
        return;
    }
    memset(&low_instance,0,sizeof(low_instance));
    iuarray = ( struct in_use_s *)malloc(iucount*
        sizeof( struct in_use_s));
    if (!iuarray) {
        P("ERROR: Cannot malloc array for space calculations\n");
        return;
    }
    iupa = iuarray;
    iupl = ep->f_in_use;
    for (i = 0  ; i < iucount; iupa++, iupl = nxt,++i) {
        nxt = iupl->u_next;
        *iupa = *iupl;
        if (iupa->u_lastbyte > highoffset) {
            highoffset = iupa->u_lastbyte;
        }
        free(iupl);
    }
    ep->f_in_use = 0;
    ep->f_in_use_tail = 0;
    qsort(iuarray,iucount,sizeof(struct in_use_s),
        comproffset);
    iupa = iuarray;
    if (print_wasted) {
        P("Listing Used Areas\n");
        P("[]        offset       length  finaloffset    name\n");
    }
    for (i = 0; i < iucount; ++i,++iupa) {
        if (print_wasted) {
            P("[%3d] " LONGESTXFMT8 " " LONGESTXFMT8 " " LONGESTXFMT8
                " %s\n",
                (int)i, iupa->u_offset, iupa->u_length,
                iupa->u_lastbyte,iupa->u_name);
        }
        if (!iupa->u_length) {
            /* Not interesting. */
            continue;
        }
        if (TRUE == firstinstance) {
            low_instance = *iupa;
            firstinstance = FALSE;
            continue;
        }
        if (iupa->u_offset == low_instance.u_offset) {
            /* Longer block at same offset appears later. */
            low_instance = *iupa;
            continue;
        }
        /* ASSERT: iupa->u_offset > low_instance.u_offset  */
        if (iupa->u_offset == low_instance.u_lastbyte) {
            /*  New follows the old with no wasted space. */
            if (iupa->u_length) {
                low_instance = *iupa;
            }
            continue;
        }
        if (iupa->u_offset > low_instance.u_lastbyte) {
            Dwarf_Unsigned diff = 0;
            if (iupa->u_align > 1) {
                Dwarf_Unsigned misaligned = low_instance.u_lastbyte %
                    iupa->u_align;
                Dwarf_Unsigned newlast = low_instance.u_lastbyte;
                Dwarf_Unsigned distance = 0;
                int wasted_space_zero = FALSE;

                if (misaligned) {
                    /*  Section alignment (u_align) is set
                        from the Elf header. For Elf32
                        it is set 4, for Elf64 it is set 8
                        This is not a requirement of Elf,
                        but is usually what is done. */
                    distance =  iupa->u_align - misaligned;
                    newlast += distance;
                }
                if (iupa->u_offset == newlast) {
                    /* alignment space. No waste. */
                    ep->f_wasted_align_count++;
                    ep->f_wasted_align_space += distance;
                    if (print_wasted) {
                        P("Warning: A gap of " LONGESTUFMT
                            " forced by alignment "
                            LONGESTUFMT
                            " from " LONGESTXFMT8 " to " LONGESTXFMT8
                            "\n",
                            distance,iupa->u_align,
                            low_instance.u_lastbyte,iupa->u_offset);
                        res = is_wasted_space_zero(ep,
                            low_instance.u_lastbyte,
                            distance,&wasted_space_zero);
                        if (res == RO_OK) {
                            if (!wasted_space_zero) {
                                P("  Wasted space at "
                                    LONGESTXFMT8 " of length "
                                    LONGESTUFMT
                                    " bytes is not all zero\n",
                                    low_instance.u_lastbyte,
                                    distance);
                            }
                        }
                    }
                    if (iupa->u_length) {
                        low_instance = *iupa;
                    }
                    continue;
                }
                if (iupa->u_offset > newlast) {
                    /* A gap after alignment */
                    /* FALL thru */
                } else {
                    /*  object: alignment. */
                    /*  Section alignment (u_align) is set
                        from the Elf header. For Elf32
                        it is set 4, for Elf64 it is set 8
                        This is not a requirement of Elf,
                        but is usually what is done. */
                    P("Warning: A gap of " LONGESTUFMT
                        " suggested by Elf32/64 alignment "
                        LONGESTUFMT
                        " would get into the next area, "
                        "which could possibly be an error. "
                        LONGESTXFMT " > "  LONGESTXFMT
                        "\n",
                        distance,iupa->u_align,
                        newlast , iupa->u_offset);
                    /* FALL thru */
                }
            }
            /* A gap  */
            diff = iupa->u_offset - low_instance.u_lastbyte;
            if (print_wasted) {
                int wasted_space_zero = FALSE;
                P("Warning: A gap of " LONGESTXFMT
                    " bytes at offset " LONGESTXFMT
                    " through " LONGESTXFMT "\n",
                    diff,low_instance.u_lastbyte,iupa->u_offset);
                res = is_wasted_space_zero(ep,
                    low_instance.u_lastbyte,
                    diff,&wasted_space_zero);
                if (res == RO_OK) {
                    if (!wasted_space_zero) {
                        P("Warning:  Wasted space at "
                            LONGESTXFMT8 " of length "
                            LONGESTUFMT " bytes is not all zero\n",
                            low_instance.u_lastbyte,
                            diff);
                    }
                }
            }
            ep->f_wasted_content_count++;
            ep->f_wasted_content_space += diff;
            low_instance = *iupa;
            continue;
        }
        /* iupa->u_offset < low_instance.u_lastbyte  */
        /*  Containment or overlap */
        if (iupa->u_lastbyte <= low_instance.u_lastbyte) {
            /* Containment. Normal. */
            continue;
        }
        P("Warning: Odd overlap of  "
            LONGESTXFMT8 "..." LONGESTXFMT8 " %s  with "
            LONGESTXFMT8 "..." LONGESTXFMT8 " %s\n",
            low_instance.u_offset,low_instance.u_lastbyte,
            low_instance.u_name,
            iupa->u_offset,iupa->u_lastbyte,iupa->u_name);
        low_instance = *iupa;
    }
    if (ep->f_wasted_content_count) {
        const char *p = "";
        if (options->co_printfilenames) {
            p = sanitized(filename,buffer1,BUFFERSIZE);
        }
        P("Warning %s: " LONGESTUFMT
            " instances of wasted section space exist "
            "and total " LONGESTUFMT " bytes wasted.\n",
            p,
            ep->f_wasted_content_count,
            ep->f_wasted_content_space);
    }
    if (ep->f_wasted_align_count) {
        const char *p = "";
        if (options->co_printfilenames) {
            p = sanitized(filename,buffer1,BUFFERSIZE);
        }
        P("Warning %s: " LONGESTUFMT
            " instances of unused alignment space exist "
            "and total " LONGESTUFMT " bytes of alignment.\n",
            p,
            ep->f_wasted_align_count,
            ep->f_wasted_align_space);
    }
    if (highoffset < filesize) {
        const char *p = "";
        Dwarf_Unsigned diffh = filesize - highoffset;
        if (options->co_printfilenames) {
            p = sanitized(filename,buffer1,BUFFERSIZE);
        }
        P("Warning %s: There are " LONGESTUFMT
            " bytes at the end of file"
            " not appearing in any section or header\n",
            p,
            diffh);
    } else if (highoffset > filesize) {
        Dwarf_Unsigned diffo=  highoffset - filesize;
        const char *p = "";

        if (options->co_printfilenames) {
            p = sanitized(filename,buffer1,BUFFERSIZE);
        }
        P("Warning %s: There are " LONGESTUFMT " bytes after "
            LONGESTXFMT " (" LONGESTUFMT ") referred to by headers "
            "but not part of the file\n",
            p,
            diffo,
            highoffset,highoffset);
    }
    free(iuarray);
}

static char buffer6[BUFFERSIZE];
static int
elf_print_dynamic(elf_filedata ep)
{
    Dwarf_Unsigned bufcount = 0;
    Dwarf_Unsigned i = 0;
    struct generic_dynentry *gbuffer = 0;
    struct generic_shdr *dynamicsect = 0;
    int errcode = 0;

    if (!ep->f_dynamic_sect_index) {
        P("No .dynamic section exists in %s\n",
            sanitized(filename,buffer6,BUFFERSIZE));
        return RO_OK;
    }
    if (ep->f_dynamic_sect_index >= ep->f_ehdr->ge_shnum) {
        P("Section Number of .dynamic section is bogus in %s\n",
            sanitized(filename,buffer6,BUFFERSIZE));
        return RO_ERROR;
    }
    dynamicsect = ep->f_shdr + ep->f_dynamic_sect_index;
    bufcount = ep->f_loc_dynamic.g_count;
    if (bufcount) {
        const char *name = sanitized(dynamicsect->gh_namestring,
            buffer6,BUFFERSIZE);

        P("\n");
        P("Section %s (" LONGESTUFMT "):"
            " Entries:" LONGESTUFMT " Offset:"
            LONGESTXFMT8  "\n",
            name,
            ep->f_dynamic_sect_index,
            bufcount,
            ep->f_loc_dynamic.g_offset);
    } else {
        P("No content exists in %s\n",
            sanitized(dynamicsect->gh_namestring,buffer6,BUFFERSIZE));
        return RO_ERROR;
    }
    gbuffer = ep->f_dynamic;
    printf(" Tag          Name             Value\n");
    for (i = 0; i < bufcount; ++i,++gbuffer) {
        const char *name = 0;
        const char *targname = "";

        name = dwarf_get_elf_dynamic_table_name(gbuffer->gd_tag,
            buffer6,BUFFERSIZE);

        switch(gbuffer->gd_tag) {
        case DT_NULL:
            break;
        case DT_NEEDED: {
            int res = 0;

            res = dwarf_get_elf_symstr_string(ep,
                FALSE,gbuffer->gd_val,
                &targname,&errcode);
            if (res != DW_DLV_OK) {
                targname = "Cannot access string";
            }
            }
            break;
        case DT_PLTRELSZ:
            break;
        case DT_PLTGOT:
            break;
        case DT_HASH:
            targname = "DT_HASH";
            break;
        case DT_STRTAB:
            /* offset of string table */
        {
            struct generic_shdr *hstr = 0;
            hstr = ep->f_shdr +
                ep->f_dynsym_sect_strings_sect_index;
            if (gbuffer->gd_val != hstr->gh_offset) {
                targname = "Does not match section header offset";
            }
        }
            break;
        case DT_SYMTAB:
            break;
        case DT_RELA:
            break;
        case DT_RELASZ:
            break;
        case DT_RELAENT:
            break;
        case DT_STRSZ:
            break;
        case DT_SYMENT:
            break;
        case DT_INIT:
            break;
        case DT_FINI:
            break;
        case DT_SONAME: {
            int res = 0;

            res = dwarf_get_elf_symstr_string(ep,
                FALSE,gbuffer->gd_val,&targname,&errcode);
            if (res != DW_DLV_OK) {
                targname = "Cannot access string";
            }
            }
            break;
        case DT_RPATH: {
            int res = 0;

            res = dwarf_get_elf_symstr_string(ep,
                FALSE,gbuffer->gd_val,&targname,&errcode);
            if (res != DW_DLV_OK) {
                targname = "Cannot access string";
            }
            }
            break;
        case DT_SYMBOLIC:
            break;
        case DT_REL:
            break;
        case DT_RELSZ:
            break;
        case DT_RELENT:
            break;
        case DT_PLTREL:
            break;
        case DT_DEBUG:
            break;
        case DT_TEXTREL:
            break;
        case DT_JMPREL:
            break;
        }

        P(" "
            LONGESTXFMT8 " %-16s "
            LONGESTXFMT8 " (" LONGESTUFMT ") %s\n",
            gbuffer->gd_tag,
            name,
            gbuffer->gd_val,
            gbuffer->gd_val,
            targname);
    }
    return RO_OK;
}

static int
elf_print_sg_groups(elf_filedata ep)
{
    Dwarf_Unsigned i = 0;
    struct generic_shdr *psh = ep->f_shdr;

    if (!ep->f_sht_group_type_section_count &&
        !ep->f_shf_group_flag_section_count &&
        !ep->f_dwo_group_section_count ) {
        P("Section Groups: No section groups or "
            ".dwo sections present. ");
        return DW_DLV_OK;
    }
    P("Section Group arrays\n");
    P(" section  name      groupsections\n");
    for (i = 0;i < ep->f_loc_shdr.g_count; ++i,++psh) {
        const char *namestr = psh->gh_namestring;
        Dwarf_Unsigned a = 1;
        Dwarf_Unsigned count = psh->gh_sht_group_array_count;

        if (!psh->gh_sht_group_array_count) {
            continue;
        }
        P("[" LONGESTUFMT "]  %-20s",psh->gh_secnum ,namestr);
        for ( ; a < count; ++a) {
            P(" " LONGESTUFMT,psh->gh_sht_group_array[a]);
        }
        P("\n");
    }
    P("\n");
    psh = ep->f_shdr;
    P("Section Group by dwarf section\n");
    P("  section          groupnumber  sectionnumber\n");
    for (i = 0;i < ep->f_loc_shdr.g_count; ++i,++psh) {
        const char *namestr = psh->gh_namestring;
        int isdw = FALSE;

        isdw = dwarf_load_elf_section_is_dwarf(namestr);
        if (!isdw) {
            continue;
        }
        P("  %-20s " LONGESTUFMT
            "          " LONGESTUFMT "\n",
            namestr,psh->gh_section_group_number,psh->gh_secnum);
    }
    if (ep->f_sht_group_type_section_count) {
        P("  SHT_GROUP count  : " LONGESTUFMT "\n",
            ep->f_sht_group_type_section_count);
    }
    if (ep->f_shf_group_flag_section_count) {
        P("  SHF_GROUP count  : " LONGESTUFMT "\n",
            ep->f_shf_group_flag_section_count);
    }
    if (ep->f_dwo_group_section_count ) {
        P("  .dwo group count : " LONGESTUFMT "\n",
            ep->f_dwo_group_section_count);
    }
    return DW_DLV_OK;
}

static void
byte_string_to_hex(dwarfstring *bi,
    unsigned char *buildid,
    unsigned buildid_length)
{
    unsigned i = 0;
    char buf[10];
    for ( ; i < buildid_length;++i){
        sprintf(buf,"%02x",buildid[i]);
        dwarfstring_append(bi,buf);
    }
}

static int
elf_print_gnu_debuglink(elf_filedata ep)
{
    char          *debuglinkname = 0;
    unsigned char *crc = 0;
    Dwarf_Unsigned buildidtype = 0;
    char          *buildidowner = 0;
    Dwarf_Unsigned buildid_length = 0;
    unsigned char *buildid = 0;
    char         **debuglink_paths = 0;
    unsigned       debuglink_path_count = 0;
    int res = 0;
    int errcode = 0;
    unsigned i = 0;

    res = dwarf_gnu_debuglink(ep,
        &debuglinkname,&crc,
        &buildidtype,&buildidowner,
        &buildid_length, &buildid,
        &debuglink_paths, &debuglink_path_count,
        &errcode);
    if (res == DW_DLV_NO_ENTRY) {
        return res;
    }
    if (res == DW_DLV_ERROR) {
        printf("ERROR: dwarf_gnu_debuglink failed. Errcode %d"
            " reading %s\n",errcode,ep->f_path);
        return DW_DLV_NO_ENTRY;
    }
    printf("GNU .gnu_debuglink and .note.gnu.buildid\n");
    printf("{\n");
    if (crc) {
        unsigned char *cp = crc;

        printf("  Section .gnu_debuglink\n");
        printf("    link              :   \"%s\"\n",
            sanitized(debuglinkname,
            buffer6,sizeof(buffer6)));
        printf("    compiler crc value:    0x");
        for (i = 0; i < 4; ++i) {
            printf("%02x ",cp[i]);
        }
        printf("\n");
    }
    if (buildidowner) {
        dwarfstring bi;
        char id[41];

        dwarfstring_constructor_static(&bi,id,sizeof(id));
        byte_string_to_hex(&bi,buildid,buildid_length);
        printf("  Section .note.gnu.build-id");
        printf("    type: " LONGESTUFMT "  owner: \"%s\"\n",
            buildidtype,sanitized(buildidowner,
                buffer6,sizeof(buffer6)));
        printf("    buildid bytes: " LONGESTUFMT "\n",
            buildid_length);
        printf("    buildid (hex): %s\n",dwarfstring_string(&bi));
        dwarfstring_destructor(&bi);
    }
    printf("  Paths list count:  %u\n",debuglink_path_count);
    for (i = 0; i <debuglink_path_count; ++i) {
        printf("  [%u] %s\n",i,sanitized(debuglink_paths[i],
            buffer6,sizeof(buffer6)));
    }
    printf("}\n");
    free(debuglink_paths);
    return DW_DLV_NO_ENTRY;
}
