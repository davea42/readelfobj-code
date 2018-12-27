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
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif /* HAVE_MALLOC_H */
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#ifdef HAVE_ELF_H
#include <elf.h>
#endif /* HAVE_ELF_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#include <unistd.h>
#include "dwarf_reading.h"
#include "dwarf_object_detector.h"
#include "dwarf_object_read_common.h"
#include "readelfobj.h"
#include "sanitized.h"
#include "readelfobj_version.h"
#include "dwarf_elf_reloc_386.h" 
#include "dwarf_elf_reloc_mips.h"	
#include "dwarf_elf_reloc_ppc.h"
#include "dwarf_elf_reloc_arm.h"  
#include "dwarf_elf_reloc_ppc64.h"	
#include "dwarf_elf_reloc_x86_64.h"

#define TRUE 1
#define FALSE 0

#ifndef IS_ELF
#define IS_ELF(e) (e.e_ident[0] == 0x7f && \
    e.e_ident[1]== 'E' && \
    e.e_ident[2]== 'L' && \
    e.e_ident[3]== 'F' )
#endif

#define ALIGN4 4
#define ALIGN8 8

static void do_one_file(const char *filename);
static void elf_print_elf_header(elf_filedata ep);
static void elf_print_progheaders(elf_filedata ep);
static void elf_print_sectstrings(elf_filedata ep,LONGESTUTYPE);
static void elf_print_sectheaders(elf_filedata ep);
static void elf_print_relocation_details(elf_filedata ep,int isrela,
    struct generic_shdr * gsh);
static void elf_print_symbols(elf_filedata ep,int is_symtab,
    struct generic_symentry * gsym,
    LONGESTUTYPE ecount,
    const char *secname);

static void report_wasted_space(elf_filedata ep);
static int elf_print_dynamic(elf_filedata ep);

int print_symtab_sections = 0; /* symtab dynsym */
int print_reloc_sections  = 0; /* .rel .rela */
int print_dynamic_sections  = 0; /* .dynamic */
int print_wasted  = 0; /* prints space use details */
int only_wasted_summary = 0; /* suppress standard printing */

static char buffer1[BUFFERSIZE];
static char buffer2[BUFFERSIZE];

char *filename;
int printfilenames;
FILE *fin;

char *Usage = "Usage: readelfobj <options> file ...\n"
    "Options:\n"
    "--print-dynamic print the .dynamic section (DT_ stuff)\n"
    "--print-relocs print relocation entries (.rela & .rel)\n"
    "--print-symtabs print out all elf symbols (.symtab & .dynsym)\n"
    "--print-wasted print out details about file space use\n"
    "               beyond just the total wasted.\n"
    "--only-wasted-summary  Skip printing section/segment data.\n"
    "--help         print this message\n"
    "--version      print version string\n";


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
            if((strcmp(argv[0],"--help") == 0) ||
                (strcmp(argv[0],"-h") == 0)) {
                P("%s",Usage);
                exit(0);
            }
            if(strcmp(argv[0],"--all") == 0) {
                print_symtab_sections= 1;
                print_wasted= 1;
                print_reloc_sections= 1;
                print_dynamic_sections= 1;
                continue;
            }
            if(strcmp(argv[0],"--print-symtabs") == 0) {
                print_symtab_sections= 1;
                continue;
            }
            if(strcmp(argv[0],"--only-wasted-summary") == 0) {
                only_wasted_summary= 1;
                continue;
            }
            if(strcmp(argv[0],"--print-wasted") == 0) {
                print_wasted= 1;
                continue;
            }
            if(strcmp(argv[0],"--print-relocs") == 0) {
                print_reloc_sections= 1;
                continue;
            }
            if(strcmp(argv[0],"--print-dynamic") == 0) {
                print_dynamic_sections= 1;
                continue;
            }
            if((strcmp(argv[0],"--version") == 0) ||
                (strcmp(argv[0],"-v") == 0 )) {
                P("Version-readelfobj: %s\n",
                    READELFOBJ_VERSION_DATE_STR);
                printed_version = TRUE;
                continue;
            }
            if ( (i+1) < argc) {
                printfilenames = 1;
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
            ++filecount;
            fclose(fin);
            do_one_file(filename);
        }
        if (!filecount && !printed_version) {
            printf("%s\n",Usage);
            exit(1);
        }
    }
    return RO_OK;
}

static int
check_dynamic_section(elf_filedata ep)
{
    LONGESTUTYPE pcount = ep->f_loc_phdr.g_count;
    struct generic_phdr *gphdr = ep->f_phdr;
    struct generic_shdr *gshdr = 0;
    LONGESTUTYPE scount = ep->f_loc_shdr.g_count;
    LONGESTUTYPE i = 0;
    LONGESTUTYPE dynamic_p_offset = 0;
    LONGESTUTYPE dynamic_p_length = 0;
    LONGESTUTYPE dynamic_p_pnum = 0;
    LONGESTUTYPE dynamic_s_offset = 0;
    LONGESTUTYPE dynamic_s_length = 0;
    LONGESTUTYPE dynamic_s_snum = 0;
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
    for( i = 0; i < pcount; ++i,  gphdr++) {
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
    for(i = 0; i < scount; i++, ++gshdr) {
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


char namebuffer[BUFFERSIZE*4];
static void
do_one_file(const char *s)
{
    int res = 0;
    unsigned ftype = 0;
    unsigned endian = 0;
    unsigned offsetsize = 0;
    size_t filesize = 0;
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
        P("Unable to open %s. Err code %d\n",namebuffer,errcode);
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

    res = dwarf_load_elf_header(ep,&errcode);
    if (res == DW_DLV_ERROR) {
        P("Error: unable to load elf header. errcode %d\n",errcode);
        dwarf_destruct_elf_access(ep,&errcode);
        return;
    }
    if (res == DW_DLV_NO_ENTRY) {
        dwarf_destruct_elf_access(ep,&errcode);
        P("Error: unable to find elf header.\n");
        return;
    }

    if (!only_wasted_summary) {
        elf_print_elf_header(ep);
    }

    res = dwarf_load_elf_sectheaders(ep,&errcode);

    res = dwarf_load_elf_progheaders(ep,&errcode);

    res = dwarf_load_elf_symstr(ep,&errcode);
    if (res == DW_DLV_ERROR) {
        P("Error: unable to load symstr. errcode %d\n",errcode);
        dwarf_destruct_elf_access(ep,&errcode);
        return;
    }
    res = dwarf_load_elf_dynstr(ep,&errcode);
    if (res == DW_DLV_ERROR) {
        P("Error: unable to load dynstr. errcode %d\n",errcode);
        dwarf_destruct_elf_access(ep,&errcode);
        return;
    }
    if (!only_wasted_summary) {
        elf_print_progheaders(ep);
        elf_print_sectstrings(ep,ep->f_ehdr->ge_shstrndx);
        elf_print_sectheaders(ep);
    }
    res = dwarf_load_elf_dynamic(ep,&errcode);
    if (res == DW_DLV_ERROR) {
        P("Error: Unable to load dynamic section");
    } else if (res == DW_DLV_OK) {
        if (print_dynamic_sections) {
            elf_print_dynamic(ep);
        }
    } else {
        if (print_dynamic_sections) {
            P("No .dynamic section in  %s\n",
                sanitized(filename,buffer1,BUFFERSIZE));
        }
    }
    res = dwarf_load_elf_dynsym_symbols(ep,&errcode);
    if( res == DW_DLV_ERROR) {
        P("Error: Unable to load .dynsym section. Errcode %d\n",
            errcode);
    }
    res  =dwarf_load_elf_symtab_symbols(ep,&errcode);
    if( res == DW_DLV_ERROR) {
        P("Error: Unable to load .symtab section. Errcode %d\n",
            errcode);
    }
    if(print_symtab_sections ) {
        if (ep->f_symtab_sect_index) {
            struct generic_shdr * psh = ep->f_shdr +
                ep->f_symtab_sect_index;
            const char *namestr = psh->gh_namestring;
            LONGESTUTYPE link = psh->gh_link;
            if ( link != ep->f_symtab_sect_strings_sect_index){
                P("ERROR: symtab link section " LONGESTUFMT
                    " mismatch with "
                    LONGESTUFMT " section\n",
                    link,
                    ep->f_symtab_sect_strings_sect_index);
                dwarf_destruct_elf_access(ep,&errcode);
                return;
            }
            elf_print_symbols(ep,TRUE,ep->f_symtab,
                ep->f_loc_symtab.g_count,namestr);
        }
        if(ep->f_dynsym_sect_index) {
            struct generic_shdr * psh = ep->f_shdr +
                ep->f_dynsym_sect_index;
            const char *namestr = psh->gh_namestring;
            LONGESTUTYPE link = psh->gh_link;
            if ( link != ep->f_dynsym_sect_strings_sect_index){
                P("ERROR: dynsym link section " LONGESTUFMT
                    " mismatch with "
                    LONGESTUFMT " section\n",
                    link,
                    ep->f_dynsym_sect_strings_sect_index);
                dwarf_destruct_elf_access(ep,&errcode);
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
    if(print_reloc_sections) {
        unsigned reloc_count = 0;
        LONGESTUTYPE i = 0;
        struct generic_shdr *psh = ep->f_shdr;

        for (i = 0;i < ep->f_loc_shdr.g_count; ++i,++psh) {
            const char *namestr = psh->gh_namestring;
            if(!strncmp(namestr,".rel.",5)) {
                res = dwarf_load_elf_rel(ep,i,&errcode);
                if(res == DW_DLV_ERROR) {
                    P("ERROR reading .rel section "
                        LONGESTUFMT " Error code %d file:%s \n",
                        i,errcode,
                        sanitized(filename,buffer1,BUFFERSIZE));
                    dwarf_destruct_elf_access(ep,&errcode);
                    return;
                } 
            } else if (!strncmp(namestr,".rela.",6)) {
                res = dwarf_load_elf_rela(ep,i,&errcode);
                if(res == DW_DLV_ERROR) {
                    P("ERROR reading .rela section "
                        LONGESTUFMT " Error code %d file:%s \n",
                        i,errcode,
                        sanitized(filename,buffer1,BUFFERSIZE));
                    dwarf_destruct_elf_access(ep,&errcode);
                    return;
                }
            }
        } 
    } 
    if(print_reloc_sections) {
        unsigned reloc_count = 0;
        LONGESTUTYPE i = 0;
        struct generic_shdr * psh = 0;

        psh = ep->f_shdr;
        for (i = 0;i < ep->f_loc_shdr.g_count; ++i,++psh) {
            const char *namestr = psh->gh_namestring;
            if(!strncmp(namestr,".rel.",5)) {
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
    }
    if (ep->f_dynamic_sect_index &&
        ep->f_wasted_dynamic_space) {
        const char *p = "";
        if (printfilenames) {
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
    report_wasted_space(ep);
    dwarf_destruct_elf_access(ep,&errcode);
}
static void
elf_print_sectstrings(elf_filedata ep,LONGESTUTYPE stringsection)
{
    struct generic_shdr *psh = 0;
    if (!stringsection) {
        P("Section strings never found.\n");
        return;
    }
    if (stringsection >= ep->f_ehdr->ge_shnum) {
        printf("String section " LONGESTUFMT " invalid. Ignored.",
            stringsection);
        return;
    }

    psh = ep->f_shdr + stringsection;;
    P("String section data at " LONGESTXFMT " (" LONGESTUFMT ")"
        " length " LONGESTXFMT " (" LONGESTUFMT ")" "\n",
        psh->gh_offset, psh->gh_offset,
        psh->gh_size,psh->gh_size);
}

/*  We're not creating a generic function for this,
    such is not needed for normal reading DWARF.  */
static void
elf_load_print_interp(elf_filedata ep,
    LONGESTUTYPE offset,
    LONGESTUTYPE size)
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
    buf = malloc(size);
    if(buf == 0) {
        P("ERROR: malloc failed reading interpreter data\n");
        return;
    }
    res = RRMOA(ep->f_fd,buf,offset,size,&errcode);
    if(res != RO_OK) {
        P("ERROR: Read interp string failed\n");
        return;
    }
    P("    Interpreter:  %s\n",sanitized(buf,buffer1,BUFFERSIZE));
    free(buf);
    return;
}


static void
elf_print_progheaders(elf_filedata ep)
{
    LONGESTUTYPE count = ep->f_loc_phdr.g_count;
    struct generic_phdr *gphdr = ep->f_phdr;
    LONGESTUTYPE i = 0;

    /*  In case of error reading headers count might now be zero */
    P("\n");
    P("Program header count " LONGESTUFMT "\n",count);
    P("{\n");
    for( i = 0; i < count; ++i,  gphdr++) {
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
        if(gphdr->gp_flags & PF_X) {
            P(" PF_X");
        }
        if(gphdr->gp_flags & PF_W) {
            P(" PF_W");
        }
        if(gphdr->gp_flags & PF_R) {
            P(" PF_R");
        }
        P(", align " LONGESTXFMT " (" LONGESTUFMT ")",
            gphdr->gp_align,gphdr->gp_align);
        P("\n");
        if(gphdr->gp_type == PT_INTERP) {
            elf_load_print_interp(ep,gphdr->gp_offset,
                gphdr->gp_filesz);
        }
    }
    P("}\n");
}

static void
elf_print_sectheaders(elf_filedata ep)
{
    struct generic_shdr *gshdr = 0;
    LONGESTUTYPE generic_count = 0;
    LONGESTUTYPE i = 0;

    gshdr = ep->f_shdr;
    generic_count = ep->f_loc_shdr.g_count;
    P("\n");
    P("Section count: " LONGESTUFMT "\n",generic_count);
    P("{\n");
    for(i = 0; i < generic_count; i++, ++gshdr) {
        const char *namestr = sanitized(gshdr->gh_namestring,
            buffer1,BUFFERSIZE);

        P("Section " LONGESTUFMT ", name " LONGESTUFMT " %s\n",
            i,gshdr->gh_name, namestr);
        P("  type " LONGESTXFMT " %s",gshdr->gh_type,
            dwarf_get_elf_section_header_st_type(gshdr->gh_type,buffer2,
                BUFFERSIZE));
        if(gshdr->gh_flags == 0) {
            P(", flags " LONGESTXFMT ,gshdr->gh_flags);
        } else {
            P(", flags "  LONGESTXFMT,gshdr->gh_flags);
            P(" %s",
                dwarf_get_elf_section_header_flag_names(
                    gshdr->gh_flags,
                    buffer2,BUFFERSIZE));
        }
        P("\n");

        P("  addr " LONGESTXFMT " (" LONGESTUFMT ")",
            gshdr->gh_addr,gshdr->gh_addr);
        P(" offset " LONGESTXFMT " (" LONGESTUFMT ")"  ,
            gshdr->gh_offset,gshdr->gh_offset);
        P(", size " LONGESTUFMT ,gshdr->gh_size);
        P("\n");

        P("  link " LONGESTUFMT ,gshdr->gh_link);
        P(", info " LONGESTUFMT ,gshdr->gh_info);
        P(", align " LONGESTUFMT ,gshdr->gh_addralign);
        P(", entsize " LONGESTUFMT  ,gshdr->gh_entsize);
        P("\n");
    }
    P("}\n");
}

static void
elf_print_symbols(elf_filedata ep,
    int is_symtab,
    struct generic_symentry * gsym,
    LONGESTUTYPE ecount,
    const char *secname)
{
    LONGESTUTYPE i = 0;
    struct location *locp = 0;

    if(is_symtab) {
        locp = &ep->f_loc_symtab;
    } else {
        locp = &ep->f_loc_dynsym;
    }
    if (!locp) {
        P("ERROR: the %s symbols section missing, not printable\n",
            secname);
        return;
    }
    P("\n");
    P("Symbols from %s: " LONGESTUFMT
        " at offset " LONGESTXFMT "\n",
        sanitized(secname,buffer1,BUFFERSIZE),ecount,
        locp->g_offset);
    P("{\n");
    if(ecount > 0) {
        P("[Index] Value    Size    Type              "
            "Bind       Other          Shndx   Name\n");
    }

    for(i = 0; i < ecount; ++i,++gsym) {
        int errcode = 0;
        int res;
        char *localstr = 0;


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
        P(", st_shndx " LONGESTUFMT " %s",
            gsym->gs_shndx,
            dwarf_get_elf_symbol_shn_type(gsym->gs_shndx,
                buffer2,BUFFERSIZE));
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
            P("  st_name  (" LONGESTUFMT  ") %s",gsym->gs_name,
                sanitized(localstr,buffer1,BUFFERSIZE));
            P("\n");
        }
    }
    P("}\n");
    return;
}

static int
get_elf_symtab_symbol_name( elf_filedata ep,
    unsigned long symnum,
    char **localstr_out, 
    int *errcode)
{
    int is_symtab = TRUE;
    int res = 0;

    struct generic_symentry *gsym = 0;
    if (symnum >= ep->f_loc_symtab.g_count) {
        return DW_DLV_NO_ENTRY;
    }
    gsym = ep->f_symtab + symnum;
    res = dwarf_get_elf_symstr_string(ep,
            is_symtab,gsym->gs_name,
            localstr_out,errcode);
    return res;
}

static int
get_elf_reloc_name(
    LONGESTUTYPE machine,
    LONGESTUTYPE type,
    const char **typename_out,
    int *errcode)
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
    case EM_MIPS:
    case EM_MIPS_RS3_LE:
    case EM_MIPS_X:
        tname= dwarf_get_elf_relocname_mips(type);
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
    struct generic_rela *grela, LONGESTUTYPE count)
{
    LONGESTUTYPE i = 0;

    P("\n");
    P("Section " LONGESTUFMT ": %s reloccount: " LONGESTUFMT "\n",
        gsh->gh_secnum,
        sanitized(gsh->gh_namestring,buffer1,BUFFERSIZE),
        count);
    P(" [i]   offset   info        type symbol %s\n",isrela?"    addend":""); 
    for(i = 0; i < count; ++i,grela++) {
        char *localstr = 0;
        unsigned long symnum = grela->gr_sym;
        int errcode = 0;
        char *symname = "";
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
           get_elf_reloc_name(ep->f_ehdr->ge_machine,grela->gr_type,&typename,&errcode);
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
            LONGESTUFMT,
            typename,
            grela->gr_type);

        P(" "
            LONGESTUFMT " %s",
            grela->gr_sym,
            symname?sanitized(symname,buffer1,BUFFERSIZE):"");
        if (isrela) {
            P(" "
                LONGESTSFMT ,
                grela->gr_addend);
        }
        P("\n");
    }
    return;
}

static void
elf_print_relocation_details(
    elf_filedata ep,
    int isrela,
    struct generic_shdr * gsh)
{
    struct generic_rela *grela = 0;
    LONGESTUTYPE count  = 0;

    count = gsh->gh_relcount;
    grela = gsh->gh_rels;
    elf_print_relocation_content(ep,isrela,gsh,grela,count);
}

static void
elf_print_elf_header(elf_filedata ep)
{
    LONGESTUTYPE i = 0;
    int c = 0;

    P("Elf object file %s\n",sanitized(filename,buffer1,BUFFERSIZE));
    P("  ident bytes: ");
    for(i = 0; i < EI_NIDENT; i++) {
        c = ep->f_ehdr->ge_ident[i];
        P(" %02x",c);
    }
    P("\n");
    i = ep->f_ehdr->ge_ident[EI_CLASS];
    P("  ident[class] " LONGESTXFMT "   %s\n",i,
        (i == ELFCLASSNONE)? "ELFCLASSNONE":
        (i == ELFCLASS32) ? "ELFCLASS32" :
        (i == ELFCLASS64) ? "ELFCLASS64" :
        "unknown ");
    c = ep->f_ehdr->ge_ident[EI_DATA];
    P("  ident[data] %#x   %s\n",c,(c == ELFDATANONE)? "ELFDATANONE":
        (c == ELFDATA2MSB)? "ELFDATA2MSB":
        (c == ELFDATA2LSB) ? "ELFDATA2LSB":
        "Invalid object encoding");
    i = ep->f_ehdr->ge_ident[EI_VERSION];
    P("  file version " LONGESTXFMT "\n", i);
    i = ep->f_ehdr->ge_ident[EI_OSABI];
    P("  osabi        " LONGESTXFMT " %s\n",
        i,
        dwarf_get_elf_osabi_name(i,buffer1,BUFFERSIZE));

    i = ep->f_ehdr->ge_ident[EI_ABIVERSION];
    P("  ident[version] " LONGESTXFMT "   %s\n",i,
        (i == EV_CURRENT)? "EV_CURRENT":
        "unknown");
    i = ep->f_ehdr->ge_type;
    P("  type " LONGESTXFMT " %s\n",i,(i == ET_NONE)? "ET_NONE No file type":
        (i == ET_REL)? "ET_REL Relocatable file":
        (i == ET_EXEC)? "ET_EXEC Executable file":
        (i == ET_DYN)? "ET_DYN Shared object file":
        (i == ET_CORE) ? "ET_CORE Core file":
        (i >= 0xff00 && i < 0xffff)? "Processor-specific type":
        "unknown");
    /* See http://www.uxsglobal.com/developers/gabi/latest/ch4.eheader.html  */
    P("  machine " LONGESTXFMT" %s\n",ep->f_ehdr->ge_machine,
        dwarf_get_elf_machine_name(ep->f_ehdr->ge_machine));
    P("  Entry " LONGESTXFMT "  prog hdr off: "
        LONGESTXFMT "   sec hdr off "
        LONGESTXFMT "\n",
        ep->f_ehdr->ge_entry,
        ep->f_ehdr->ge_phoff,
        ep->f_ehdr->ge_shoff);
    P("  Flags " LONGESTXFMT,ep->f_ehdr->ge_flags);
    /* FIXME print flags */
    P("\tEhdrsize " LONGESTUFMT "  Proghdrsize "
        LONGESTUFMT "  Sechdrsize "
        LONGESTUFMT "\n",
        ep->f_ehdr->ge_ehsize,
        ep->f_ehdr->ge_phentsize,
        ep->f_ehdr->ge_shentsize);
    P("              P-hdrcount  "
        LONGESTUFMT "  S-hdrcount "
        LONGESTUFMT "\n",
        ep->f_ehdr->ge_phnum,
        ep->f_ehdr->ge_shnum);
    if(ep->f_ehdr->ge_shstrndx == SHN_UNDEF) {
        P("  Section strings are not present e_shstrndx ==SHN_UNDEF\n");
    } else {
        P("  Section strings are in section "
            LONGESTUFMT "\n",ep->f_ehdr->ge_shstrndx);
    }
    if(ep->f_ehdr->ge_shstrndx > ep->f_ehdr->ge_shnum) {
        P("String section index is wrong: "
            LONGESTUFMT " vs only "
            LONGESTUFMT " sections."
            " Consider it 0\n",
            ep->f_ehdr->ge_shstrndx,
            ep->f_ehdr->ge_shnum);
        ep->f_ehdr->ge_shstrndx = 0;
    }
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

#define MAXWBLOCK 100000

static int
is_wasted_space_zero(elf_filedata ep,
    LONGESTUTYPE offset,
    LONGESTUTYPE length,
    int *wasted_space_zero)
{
    LONGESTUTYPE remaining = length;
    char *allocspace = 0;
    LONGESTUTYPE alloclen = length;
    LONGESTUTYPE checklen = length;
    int errcode = 0;

    if (length > MAXWBLOCK) {
        alloclen = MAXWBLOCK;
        checklen = MAXWBLOCK;
    }
    allocspace = (char *)malloc(alloclen);
    if(!allocspace) {
        P("Unable to malloc " LONGESTUFMT "bytes for zero checking.\n",
            alloclen);
        return RO_ERROR;
    }
    while (remaining) {
        LONGESTUTYPE i = 0;
        int res = 0;

        if (remaining < checklen) {
            checklen = remaining;
        }
        res = RRMOA(ep->f_fd,allocspace,offset,checklen,&errcode);
        if (res != RO_OK) {
            free(allocspace);
            P("ERROR: could not read wasted space at offset "
                LONGESTXFMT " properly\n",
                offset);
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

    if( l->u_offset < r->u_offset) {
        return -1;
    }
    if( l->u_offset > r->u_offset) {
        return 1;
    }
    if( l->u_lastbyte < r->u_lastbyte) {
        return -1;
    }
    if( l->u_lastbyte > r->u_lastbyte) {
        return 1;
    }
    /*  When shdr and phdr (etc) use a specific area
        we want a consistent ordering of the report
        so regression tests work right. */
    strfield = strcmp(l->u_name,r->u_name);
    return strfield;
}


static void
report_wasted_space(elf_filedata  ep)
{
    LONGESTUTYPE filesize = ep->f_filesize;
    LONGESTUTYPE iucount = ep->f_in_use_count;
    LONGESTUTYPE i = 0;
    int res = 0;

    struct in_use_s *iuarray = 0;
    struct in_use_s *iupa = 0;
    struct in_use_s *iupl = 0;
    struct in_use_s *nxt = 0;
    LONGESTUTYPE highoffset = 0;
    struct in_use_s low_instance;
    int firstinstance = TRUE;

    if (!iucount) {
        P("No content found in %s\n",
            sanitized(filename,buffer1,BUFFERSIZE));
        return;
    }
    memset(&low_instance,0,sizeof(low_instance));
    iuarray = ( struct in_use_s *)malloc(iucount*sizeof( struct in_use_s));
    if(!iuarray) {
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
            LONGESTUTYPE diff = 0;
            if(iupa->u_align > 1) {
                LONGESTUTYPE misaligned = low_instance.u_lastbyte %
                    iupa->u_align;
                LONGESTUTYPE newlast = low_instance.u_lastbyte;
                LONGESTUTYPE distance = 0;
                int wasted_space_zero = FALSE;

                if (misaligned) {
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
                            if(!wasted_space_zero) {
                                P("  Wasted space at "
                                    LONGESTXFMT8 " of length "
                                    LONGESTUFMT " bytes is not all zero\n",
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
                    /*  ERROR in object: alignment. */
                    P("Warning: A gap of " LONGESTUFMT
                        " forced by alignment "
                        LONGESTUFMT
                        " would get into next area, something wrong. "
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
                res = is_wasted_space_zero(ep,low_instance.u_lastbyte,
                    diff,&wasted_space_zero);
                if (res == RO_OK) {
                    if(!wasted_space_zero) {
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
        if (printfilenames) {
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
        if (printfilenames) {
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
        LONGESTUTYPE diffh = filesize - highoffset;
        if (printfilenames) {
            p = sanitized(filename,buffer1,BUFFERSIZE);
        }
        P("Warning %s: There are " LONGESTUFMT " bytes at the end of file"
            " not appearing in any section or header\n",
            p,
            diffh);
    } else if (highoffset > filesize) {
        LONGESTUTYPE diffo=  highoffset - filesize;
        const char *p = "";

        if (printfilenames) {
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
    LONGESTUTYPE bufcount = 0;
    LONGESTUTYPE i = 0;
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
    if(bufcount) {
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
    for(i = 0; i < bufcount; ++i,++gbuffer) {
        const char *name = 0;
        char *targname = "";

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
