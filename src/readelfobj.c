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
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <elf.h>
#include <unistd.h>
#include "reading.h"
#include "readelfobj.h"
#include "sanitized.h"
#include "readelfobj_version.h"

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
static void elf_find_sym_sections(void);
static void elf_print_elf_header(void);
static void elf_print_progheaders(void);
static void elf_print_sectstrings(LONGESTUTYPE);
static void elf_print_sectheaders(void);
static void elf_print_relocations( struct generic_shdr * gsh,
    struct generic_rela *grela, LONGESTUTYPE count);
static void elf_print_relocation32(int isrela,LONGESTUTYPE secnum,
    struct generic_shdr *sec);
static void elf_print_relocation64(int isrela,LONGESTUTYPE secnum,
    struct generic_shdr *sec);
static void elf_print_symbols32(int is_symtab,int secnum,
    const char *secname,
    LONGESTUTYPE offset,
    LONGESTUTYPE size);
static void elf_print_symbols64(int is_symtab,int secnum,
    const char *secname,
    LONGESTUTYPE offset,
    LONGESTUTYPE size);
static void report_wasted_space(void);

int print_symtab_sections = 0; /* symtab dynsym */
int print_reloc_sections  = 0; /* .rel .rela */
int print_dynamic_sections  = 0; /* .dynamic */
int print_wasted  = 0; /* prints space use details */
int only_wasted_summary = 0; /* suppress standard printing */

/* All the data for one file. */
struct filedata_s filedata;

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


static void
clean_filedata(void)
{
    free(filedata.f_ehdr);
    free(filedata.f_shdr);
    free(filedata.f_phdr);
    free(filedata.f_elf_shstrings_data);
    free(filedata.f_dynamic);
    free(filedata.f_symtab_sect_strings);
    free(filedata.f_dynsym_sect_strings);
    memset(&filedata, 0, sizeof(filedata));
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
            if((strcmp(argv[0],"--help") == 0) ||
                (strcmp(argv[0],"-h") == 0)) {
                P("%s",Usage);
                exit(0);
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
            do_one_file(filename);
            clean_filedata();
            fclose(fin);
        }
        if (!filecount && !printed_version) {
            printf("%s\n",Usage);
            exit(1);
        }
    }
    return RO_OK;
}

static int
this_is_maybe_elf(int *class, int *is_little_endian)
{
    Elf32_Ehdr ehdr32;
    int res = 0;
    int lclass = 0;
    int isle = 0;

    res = RR(&ehdr32,0,sizeof(ehdr32));
    if(res != RO_OK) {
        P("Warning: Could not read whole file header of %s\n",
            sanitized(filename,buffer1,BUFFERSIZE));
        return FALSE;
    }
    if (filedata.f_filesize < sizeof(ehdr32)) {
        P("Warning: The file %s is shorter than any ELF file header\n",
            sanitized(filename,buffer1,BUFFERSIZE));
        return FALSE;
    }
    if (! IS_ELF(ehdr32)) {
        P("Warning: The file %s is not Elf.\n",
            sanitized(filename,buffer1,BUFFERSIZE));
        return FALSE;
    }
    lclass = ehdr32.e_ident[EI_CLASS];
    if (lclass == ELFCLASS32) {
        *class = 32;
    } else if (lclass == ELFCLASS64) {
        *class = 64;
    } else {
        P("Warning: The file %s has an unknown elf class."
            " Ignoring the file.\n",
            sanitized(filename,buffer1,BUFFERSIZE));
        return FALSE;
    }
    isle = ehdr32.e_ident[EI_DATA];
    if (isle == ELFDATA2LSB) {
        *is_little_endian = TRUE;
    } else if (isle == ELFDATA2MSB) {
        *is_little_endian = FALSE;
    } else {
        P("Warning: The file %s has an unknown endianness code."
            " Ignoring the file.\n",
            sanitized(filename,buffer1,BUFFERSIZE));
        return FALSE;
    }
    return TRUE;
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
check_dynamic_section(void)
{
    LONGESTUTYPE pcount = filedata.f_loc_phdr.g_count;
    struct generic_phdr *gphdr = filedata.f_phdr;
    struct generic_shdr *gshdr = 0;
    LONGESTUTYPE scount = filedata.f_loc_shdr.g_count;
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
        return;
    }
    if (!scount) {
        /* nothing to do. */
        return;
    }

    /*  In case of error reading headers count might now be zero */
    for( i = 0; i < pcount; ++i,  gphdr++) {
        const char *typename = get_program_header_type_name(
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

    gshdr = filedata.f_shdr;
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
        return;
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
        return;
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
        return;
    }
    return;
}


static void
do_one_file(const char *s)
{
    int res = 0;
    int eclass = 0;
    int obj_is_little_endian = FALSE;

    res = get_filedata(s,fileno(fin),&filedata);
    if (res != RO_OK) {
        return;
    }
    if (!this_is_maybe_elf( &eclass, &obj_is_little_endian)) {
        return;
    }
#ifdef WORDS_BIGENDIAN
    if (obj_is_little_endian) {
        filedata.f_copy_word = ro_memcpy_swap_bytes;
    } else {
        filedata.f_copy_word = memcpy;
    }
#else  /* LITTLE ENDIAN */
    if (obj_is_little_endian) {
        filedata.f_copy_word = memcpy;
    } else {
        filedata.f_copy_word = ro_memcpy_swap_bytes;
    }
#endif /* LITTLE- BIG-ENDIAN */
    if (eclass == 32) {
        elf_load_elf_header32();
    } else if (eclass == 64) {
        if (sizeof(LONGESTUTYPE) < 8) {
            P("Cannot read Elf64 from %s as the longest available "
                " integer is just %u bytes\n",
                sanitized(filename,buffer1,BUFFERSIZE),
                (unsigned)sizeof(LONGESTUTYPE));
            return;
        }
        elf_load_elf_header64();
    } else {
        P("Cannot read Elf from %s as the elf class is"
            " %u, which is not a valid class value.\n",
            sanitized(filename,buffer1,BUFFERSIZE),
            (unsigned)eclass);
        return;
    }
    if (!only_wasted_summary) {
        elf_print_elf_header();
    }
    if (eclass == 32) {
        elf_load_sectheaders32(filedata.f_ehdr->ge_shoff,
            filedata.f_ehdr->ge_shentsize,
            filedata.f_ehdr->ge_shnum);
    } else {
        elf_load_sectheaders64(filedata.f_ehdr->ge_shoff,
            filedata.f_ehdr->ge_shentsize,
            filedata.f_ehdr->ge_shnum);
    }
    elf_load_sectstrings(filedata.f_ehdr->ge_shstrndx);
    elf_load_sect_namestring();
    elf_find_sym_sections();
    if (eclass == 32) {
        if (filedata.f_ehdr->ge_phnum) {
            elf_load_progheaders32( filedata.f_ehdr->ge_phoff,
                filedata.f_ehdr->ge_phentsize,
                filedata.f_ehdr->ge_phnum);
        }
    } else {
        if (filedata.f_ehdr->ge_phnum) {
            elf_load_progheaders64( filedata.f_ehdr->ge_phoff,
                filedata.f_ehdr->ge_phentsize,
                filedata.f_ehdr->ge_phnum);
        }
    }
    if (filedata.f_dynsym_sect_index) {
        /* There is such a section. */
        res = elf_load_dynstr(TRUE,
            filedata.f_dynsym_sect_strings_sect_index,
            filedata.f_dynsym_sect_strings_max);
    }
    if (filedata.f_symtab_sect_index) {
        /* There is such a section. */
        elf_load_dynstr(FALSE,
            filedata.f_symtab_sect_strings_sect_index,
            filedata.f_symtab_sect_strings_max);
    }
    if (!only_wasted_summary) {
        elf_print_progheaders();
        elf_print_sectstrings(filedata.f_ehdr->ge_shstrndx);
        elf_print_sectheaders();
    }
    if(filedata.f_dynamic_sect_index) {
        struct generic_shdr *sp = filedata.f_shdr +
            filedata.f_dynamic_sect_index;
        if (eclass == 32) {
            elf_load_dynamic32(sp->gh_offset,
                sp->gh_size);
        } else {
            elf_load_dynamic64(sp->gh_offset,
                sp->gh_size);
        }
        if (print_dynamic_sections) {
            elf_print_dynamic();
        }
    } else {
        if (print_dynamic_sections) {
            P("No .dynamic section in  %s\n",
                sanitized(filename,buffer1,BUFFERSIZE));
        }
    }
    if(print_symtab_sections ) {
        if (filedata.f_symtab_sect_index) {
            struct generic_shdr * psh = filedata.f_shdr +
                filedata.f_symtab_sect_index;
            const char *namestr = psh->gh_namestring;
            LONGESTUTYPE link = psh->gh_link;
            if ( link != filedata.f_symtab_sect_strings_sect_index){
                P("ERROR: symtab link section " LONGESTUFMT " mismatch with "
                    LONGESTUFMT " section\n",
                    link,
                    filedata.f_symtab_sect_strings_sect_index);
                return;
            }
            if (eclass == 32) {
                elf_print_symbols32(TRUE,filedata.f_symtab_sect_index,
                    namestr,psh->gh_offset,psh->gh_size);
            } else {
                elf_print_symbols64(TRUE,filedata.f_symtab_sect_index,
                    namestr,psh->gh_offset,psh->gh_size);
            }
        }
        if(filedata.f_dynsym_sect_index) {
            struct generic_shdr * psh = filedata.f_shdr +
                filedata.f_dynsym_sect_index;
            const char *namestr = psh->gh_namestring;
            LONGESTUTYPE link = psh->gh_link;
            if ( link != filedata.f_dynsym_sect_strings_sect_index){
                P("ERROR: dynsym link section " LONGESTUFMT " mismatch with "
                    LONGESTUFMT " section\n",
                    link,
                    filedata.f_dynsym_sect_strings_sect_index);
                return;
            }
            if (eclass == 32) {
                elf_print_symbols32(FALSE,filedata.f_symtab_sect_index,
                    namestr,psh->gh_offset,psh->gh_size);
            } else {
                elf_print_symbols64(FALSE,filedata.f_symtab_sect_index,
                    namestr,psh->gh_offset,psh->gh_size);
            }
        }
        if (!filedata.f_dynsym_sect_index  &&
            !filedata.f_symtab_sect_index) {
            P("No .symtab or .dynsym section in %s.\n",
                sanitized(filename,buffer1,BUFFERSIZE));
        }
    }
    if(print_reloc_sections) {
        unsigned reloc_count = 0;
        LONGESTUTYPE i = 0;
        struct generic_shdr * psh = filedata.f_shdr;
        for (i = 0;i < filedata.f_loc_shdr.g_count; ++i,++psh) {
            const char *namestr = filedata.f_shdr->gh_namestring;
            if(!strncmp(namestr,".rel.",5)) {
                ++reloc_count;
                if (eclass == 32) {
                    elf_print_relocation32(FALSE,i,psh);
                } else {
                    elf_print_relocation64(FALSE,i,psh);
                }
            } else if (!strncmp(namestr,".rela.",6)) {
                ++reloc_count;
                if (eclass == 32) {
                    elf_print_relocation32(TRUE,i,psh);
                } else {
                    elf_print_relocation64(TRUE,i,psh);
                }
            }
        }
        if (reloc_count == 0) {
            P("No .rel* sections were found in %s\n",
                sanitized(filename,buffer1,BUFFERSIZE));
        }
    }
    if (filedata.f_dynamic_sect_index &&
        filedata.f_wasted_dynamic_space) {
        const char *p = "";
        if (printfilenames) {
            p = sanitized(filename,buffer1,BUFFERSIZE);
        }
        P("Warning %s: Wasted .dynamic section entries: "
            LONGESTXFMT " (" LONGESTUFMT ")\n",
            p,
            filedata.f_wasted_dynamic_count,
            filedata.f_wasted_dynamic_count);

        P("Warning %s: Wasted .dynamic section space  : "
            LONGESTXFMT " (" LONGESTUFMT ")\n",
            p,
            filedata.f_wasted_dynamic_space,
            filedata.f_wasted_dynamic_space);
    }
    check_dynamic_section();
    report_wasted_space();
}
static void
elf_print_sectstrings(LONGESTUTYPE stringsection)
{
    struct generic_shdr *psh = 0;
    if (!stringsection) {
        P("Section strings never found.\n");
        return;
    }
    if (stringsection >= filedata.f_ehdr->ge_shnum) {
        printf("String section " LONGESTUFMT " invalid. Ignored.",
            stringsection);
        return;
    }

    psh = filedata.f_shdr + stringsection;;
    P("String section data at " LONGESTXFMT " (" LONGESTUFMT ")"
        " length " LONGESTXFMT " (" LONGESTUFMT ")" "\n",
        psh->gh_offset, psh->gh_offset,
        psh->gh_size,psh->gh_size);
}
static void
elf_print_interp(LONGESTUTYPE offset, LONGESTUTYPE size)
{
    long cloc = 0;
    long j = 0;
    long res = 0;
    char *buf = 0;

    res = cur_read_loc(fin,&cloc);
    if(res != RO_OK) {
        P("\tcurrent loc unknown?. No interpreter string.\n");
        return;
    }
    if ((offset > filedata.f_filesize)||
        (size > filedata.f_filesize) ||
        ((size +offset) > filedata.f_filesize)) {
            P("ERROR: Something badly wrong with interpreter name "
                " filesize " LONGESTUFMT
                " offset " LONGESTUFMT
                " size " LONGESTUFMT
                "\n", filedata.f_filesize,offset,size);
            return;
    }
    j = (long)SEEKTO(offset);
    if(j != RO_OK){
        P("ERROR: seekto %lx failed reading interpreter data\n",(long)offset);
    }
    buf = malloc(size);
    if(buf == 0) {
        SEEKTO(cloc);
        P("ERROR: malloc failed reading interpreter data\n");
        return;
    }
    res = RN(buf,size);
    if(res != RO_OK) {
        P("ERROR: Read interp string failed\n");
        SEEKTO(cloc);
        return;
    }
    P("    Interpreter:  %s\n",sanitized(buf,buffer1,BUFFERSIZE));
    if(SEEKTO(cloc) != RO_OK) {
        P("ERROR: Seek back to %lx after reading interpreter data failed\n",(long)cloc);
    }
    free(buf);
    return;
}


static void
elf_print_progheaders(void)
{
    LONGESTUTYPE count = filedata.f_loc_phdr.g_count;
    struct generic_phdr *gphdr = filedata.f_phdr;
    LONGESTUTYPE i = 0;

    /*  In case of error reading headers count might now be zero */
    P("\n");
    P("Program header count " LONGESTUFMT "\n",count);
    P("{\n");
    for( i = 0; i < count; ++i,  gphdr++) {
        P("Program header " LONGESTUFMT ,i);
        P("  type %s " LONGESTXFMT,
            get_program_header_type_name(gphdr->gp_type,
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
            elf_print_interp(gphdr->gp_offset,
                gphdr->gp_filesz);
        }
    }
    P("}\n");
}

static void
elf_print_sectheaders(void)
{
    struct generic_shdr *gshdr = 0;
    LONGESTUTYPE generic_count = 0;
    LONGESTUTYPE i = 0;

    gshdr = filedata.f_shdr;
    generic_count = filedata.f_loc_shdr.g_count;
    P("\n");
    P("Section count: " LONGESTUFMT "\n",generic_count);
    P("{\n");
    for(i = 0; i < generic_count; i++, ++gshdr) {
        const char *namestr = sanitized(gshdr->gh_namestring,
            buffer1,BUFFERSIZE);

        P("Section " LONGESTUFMT ", name " LONGESTUFMT " %s\n",
            i,gshdr->gh_name, namestr);
        P("  type " LONGESTXFMT " %s",gshdr->gh_type,
            get_section_header_st_type(gshdr->gh_type,buffer2,
                BUFFERSIZE));
        if(gshdr->gh_flags == 0) {
            P(", flags " LONGESTXFMT ,gshdr->gh_flags);
        } else {
            P(", flags "  LONGESTXFMT,gshdr->gh_flags);
            P(" %s",get_section_header_flag_names(gshdr->gh_flags,
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
elf_print_symbols(struct generic_symentry * gsym, LONGESTUTYPE ecount,
   const char *secname, int is_symtab)
{
    LONGESTUTYPE i = 0;

    P("\n");
    P("Symbols from %s: " LONGESTUFMT "\n",
        sanitized(secname,buffer1,BUFFERSIZE),ecount);
    P("{\n");
    if(ecount > 0) {
        P("[Index] Value    Size    Type              "
            "Bind       Other          Shndx   Name\n");
    }

    for(i = 0; i < ecount; ++i,++gsym) {
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
            get_symbol_stt_type(gsym->gs_type, buffer2, BUFFERSIZE));
        P(", bind "
            LONGESTXFMT " (" LONGESTUFMT ") %s",
            gsym->gs_bind,gsym->gs_bind,
            get_symbol_stb_string(gsym->gs_bind, buffer2,BUFFERSIZE));
        P("\n");

        P("  st_other "
            LONGESTXFMT " (" LONGESTUFMT ") %s",
            gsym->gs_other,
            gsym->gs_other,
            get_symbol_sto_type(gsym->gs_other,buffer2, BUFFERSIZE));
        P(", st_shndx " LONGESTUFMT " %s",
            gsym->gs_shndx,
            get_symbol_shn_type(gsym->gs_shndx, buffer2,BUFFERSIZE));
        P("\n");

        P("  st_name  (" LONGESTUFMT  ") %s",gsym->gs_name,
            sanitized(get_symstr_string(is_symtab,gsym->gs_name),
            buffer2,BUFFERSIZE));
        P("\n");
    }
    P("}\n");
    return;
}

static void
elf_print_symbols32(int is_symtab,int secnum,const char *secname,
    LONGESTUTYPE offset,LONGESTUTYPE size)
{
    struct generic_symentry *gsym = 0;
    LONGESTUTYPE ecount = 0;
    int res = 0;


    res = generic_elf_load_symbols32(secnum,secname,&gsym,
        offset,size,&ecount);
    if(res != RO_OK) {
        P("ERROR: not printing symbols, sec %d\n",secnum);
        return;
    }
    elf_print_symbols(gsym,ecount,secname,is_symtab);
    free(gsym);
    return;
}

static void
elf_print_symbols64(int is_symtab,int secnum,const char *secname,
    LONGESTUTYPE offset,LONGESTUTYPE size)
{
    struct generic_symentry *gsym = 0;
    LONGESTUTYPE ecount = 0;
    int res = 0;


    res = generic_elf_load_symbols64(secnum,secname,&gsym,
        offset,size,&ecount);
    if(res != RO_OK) {
        P("ERROR: not printing symbols, sec %d\n",secnum);
        return;
    }
    elf_print_symbols(gsym,ecount,secname,is_symtab);
    free(gsym);
    return;
}


static void
elf_print_relocation32(int isrela,LONGESTUTYPE secnum,
    struct generic_shdr * gsh)
{
    struct generic_rela *grela = 0;

    LONGESTUTYPE count  = 0;
    int res = 0;

    /*  print true file offset. Need to store it so available here. */
    if (isrela) {
        res = elf_load_rela_32(secnum,gsh,&grela,&count);
    } else {
        res = elf_load_rel_32(secnum,gsh,&grela,&count);
    } /* or load 64 */
    if (!res) {
        P("ERROR: probable loading relocation section\n");
        return;
    }
    elf_print_relocations(gsh,grela,count);
    free(grela);
}
static void
elf_print_relocation64(int isrela,LONGESTUTYPE secnum,
    struct generic_shdr * gsh)
{
    struct generic_rela *grela = 0;
    LONGESTUTYPE count  = 0;
    int res = 0;

    /*  print true file offset. Need to store it so available here. */
    if (isrela) {
        res = elf_load_rela_64(secnum,gsh,&grela,&count);
    } else {
        res = elf_load_rel_64(secnum,gsh,&grela,&count);
    } /* or load 64 */
    if (!res) {
        P("ERROR: probable loading relocation section\n");
        return;
    }
    elf_print_relocations(gsh,grela,count);
    free(grela);
}


static void
elf_print_relocations( struct generic_shdr * gsh,
    struct generic_rela *grela, LONGESTUTYPE count)
{
    LONGESTUTYPE i = 0;

    P("\n");
    P("Section " LONGESTUFMT ": %s\n",
        gsh->gh_secnum,
        sanitized(gsh->gh_namestring,buffer1,BUFFERSIZE));

    for(i = 0; i < count; ++i,grela++) {
        P("[" LONGESTUFMT "] ",i);
        P(" targ_offset "
            LONGESTXFMT " (" LONGESTUFMT ")",
            grela->gr_offset,
            grela->gr_offset);
        P(", info "
            LONGESTXFMT " (" LONGESTUFMT ")",
            grela->gr_info,
            grela->gr_info);
        P(", sym "
            LONGESTXFMT " (" LONGESTUFMT ")",
            grela->gr_sym,
            grela->gr_sym);
        P(", type "
            LONGESTXFMT " (" LONGESTUFMT ")",
            grela->gr_type,
            grela->gr_type);
        P("\n");
    }
    return;
}
static void
elf_print_elf_header(void)
{
    LONGESTUTYPE i = 0;
    int c = 0;

    P("Elf object file %s\n",sanitized(filename,buffer1,BUFFERSIZE));
    P("  ident bytes: ");
    for(i = 0; i < EI_NIDENT; i++) {
        c =  filedata.f_ehdr->ge_ident[i];
        P(" %02x",c);
    }
    P("\n");
    i = filedata.f_ehdr->ge_ident[EI_CLASS];
    P("  ident[class] " LONGESTXFMT "   %s\n",i,
        (i == ELFCLASSNONE)? "ELFCLASSNONE":
        (i == ELFCLASS32) ? "ELFCLASS32" :
        (i == ELFCLASS64) ? "ELFCLASS64" :
        "unknown ");
    c = filedata.f_ehdr->ge_ident[EI_DATA];
    P("  ident[data] %#x   %s\n",c,(c == ELFDATANONE)? "ELFDATANONE":
        (c == ELFDATA2MSB)? "ELFDATA2MSB":
        (c == ELFDATA2LSB) ? "ELFDATA2LSB":
        "Invalid object encoding");
    i = filedata.f_ehdr->ge_ident[EI_VERSION];
    P("  file version " LONGESTXFMT "\n", i);
    i = filedata.f_ehdr->ge_ident[EI_OSABI];
    P("  osabi        " LONGESTXFMT " %s\n",
        i,
        get_osabi_name(i,buffer1,BUFFERSIZE));

    i = filedata.f_ehdr->ge_ident[EI_ABIVERSION];
    P("  ident[version] " LONGESTXFMT "   %s\n",i,
        (i == EV_CURRENT)? "EV_CURRENT":
        "unknown");
    i = filedata.f_ehdr->ge_type;
    P("  type " LONGESTXFMT " %s\n",i,(i == ET_NONE)? "ET_NONE No file type":
        (i == ET_REL)? "ET_REL Relocatable file":
        (i == ET_EXEC)? "ET_EXEC Executable file":
        (i == ET_DYN)? "ET_DYN Shared object file":
        (i == ET_CORE) ? "ET_CORE Core file":
        (i >= 0xff00 && i < 0xffff)? "Processor-specific type":
        "unknown");
    /* See http://www.uxsglobal.com/developers/gabi/latest/ch4.eheader.html  */
    P("  machine " LONGESTXFMT" %s\n",filedata.f_ehdr->ge_machine,
        get_em_machine_name(filedata.f_ehdr->ge_machine));
    P("  Entry " LONGESTXFMT "  prog hdr off: "
        LONGESTXFMT "   sec hdr off "
        LONGESTXFMT "\n",
        filedata.f_ehdr->ge_entry,
        filedata.f_ehdr->ge_phoff,
        filedata.f_ehdr->ge_shoff);
    P("  Flags " LONGESTXFMT,filedata.f_ehdr->ge_flags);
    /* FIXME print flags */
    P("\tEhdrsize " LONGESTUFMT "  Proghdrsize "
        LONGESTUFMT "  Sechdrsize "
        LONGESTUFMT "\n",
        filedata.f_ehdr->ge_ehsize,
        filedata.f_ehdr->ge_phentsize,
        filedata.f_ehdr->ge_shentsize);
    P("              P-hdrcount  "
        LONGESTUFMT "  S-hdrcount "
        LONGESTUFMT "\n",
        filedata.f_ehdr->ge_phnum,
        filedata.f_ehdr->ge_shnum);
    if(filedata.f_ehdr->ge_shstrndx == SHN_UNDEF) {
        P("  Section strings are not present e_shstrndx ==SHN_UNDEF\n");
    } else {
        P("  Section strings are in section "
            LONGESTUFMT "\n",filedata.f_ehdr->ge_shstrndx);
    }
    if(filedata.f_ehdr->ge_shstrndx > filedata.f_ehdr->ge_shnum) {
        P("String section index is wrong: "
            LONGESTUFMT " vs only "
            LONGESTUFMT " sections."
            " Consider it 0\n",
            filedata.f_ehdr->ge_shstrndx,
            filedata.f_ehdr->ge_shnum);
        filedata.f_ehdr->ge_shstrndx = 0;
    }
}

static void
elf_find_sym_sections(void)
{
    struct generic_shdr* psh = 0;
    LONGESTUTYPE i = 0;
    LONGESTUTYPE count = 0;

    count = filedata.f_loc_shdr.g_count;
    psh = filedata.f_shdr;
    for (i = 0; i < count; ++psh,++i) {
        const char *name = psh->gh_namestring;
        if (!strcmp(name,".dynsym")) {
            filedata.f_dynsym_sect_index = i;
        } else if (!strcmp(name,".dynstr")) {
            filedata.f_dynsym_sect_strings_sect_index = i;
            filedata.f_dynsym_sect_strings_max = psh->gh_size;
        } else if (!strcmp(name,".symtab")) {
            filedata.f_symtab_sect_index = i;
        } else if (!strcmp(name,".strtab")) {
            filedata.f_symtab_sect_strings_sect_index = i;
            filedata.f_symtab_sect_strings_max = psh->gh_size;
        } else if (!strcmp(name,".dynamic")) {
            filedata.f_dynamic_sect_index = i;
        }
    }
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

void
insert_in_use_entry(const char *description,LONGESTUTYPE offset,
    LONGESTUTYPE length,LONGESTUTYPE align)
{
    struct in_use_s *e = 0;

    e = (struct in_use_s *)calloc(1,sizeof(struct in_use_s));
    if(!e) {
        P("ERROR: Out of memory creating in-use entry " LONGESTUFMT
            " Giving up.\n",filedata.f_in_use_count);
        exit(1);
    }
    e->u_next = 0;
    e->u_name = description;
    e->u_offset = offset;
    e->u_align = align;
    e->u_length = length;
    e->u_lastbyte = offset+length;
    ++filedata.f_in_use_count;
    if (filedata.f_in_use) {
        filedata.f_in_use_tail->u_next = e;
        filedata.f_in_use_tail = e;
        return;
    }
    filedata.f_in_use = e;
    filedata.f_in_use_tail = e;
}

#define MAXWBLOCK 100000

static int
is_wasted_space_zero(LONGESTUTYPE offset,
    LONGESTUTYPE length,
    int *wasted_space_zero)
{
    LONGESTUTYPE remaining = length;
    char *allocspace = 0;
    LONGESTUTYPE alloclen = length;
    LONGESTUTYPE checklen = length;

    if (length > MAXWBLOCK) {
        alloclen = MAXWBLOCK;
        checklen = MAXWBLOCK;
    }
    allocspace = (char *)malloc(alloclen);
    if(!allocspace) {
        P("Unable to malloc " LONGESTUFMT "bytes for zero checking.\n",
            alloclen);
        return RO_ERR;
    }
    while (remaining) {
        LONGESTUTYPE i = 0;
        int res = 0;

        if (remaining < checklen) {
            checklen = remaining;
        }
        res = RR(allocspace,offset,checklen);
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
    return 0;
}


static void
report_wasted_space(void)
{
    LONGESTUTYPE filesize = filedata.f_filesize;
    LONGESTUTYPE iucount = filedata.f_in_use_count;
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
    iupl = filedata.f_in_use;
    for (i = 0  ; i < iucount; iupa++, iupl = nxt,++i) {
        nxt = iupl->u_next;
        *iupa = *iupl;
        if (iupa->u_lastbyte > highoffset) {
            highoffset = iupa->u_lastbyte;
        }
        free(iupl);
    }
    filedata.f_in_use = 0;
    filedata.f_in_use_tail = 0;
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
                    filedata.f_wasted_align_count++;
                    filedata.f_wasted_align_space += distance;
                    if (print_wasted) {
                        P("Warning: A gap of " LONGESTUFMT
                            " forced by alignment "
                            LONGESTUFMT
                            " from " LONGESTXFMT8 " to " LONGESTXFMT8
                            "\n",
                            distance,iupa->u_align,
                            low_instance.u_lastbyte,iupa->u_offset);
                        res = is_wasted_space_zero(
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
                res = is_wasted_space_zero(low_instance.u_lastbyte,
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
            filedata.f_wasted_content_count++;
            filedata.f_wasted_content_space += diff;
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
    if (filedata.f_wasted_content_count) {
        const char *p = "";
        if (printfilenames) {
            p = sanitized(filename,buffer1,BUFFERSIZE);
        }
        P("Warning %s: " LONGESTUFMT
            " instances of wasted section space exist "
            "and total " LONGESTUFMT " bytes wasted.\n",
            p,
            filedata.f_wasted_content_count,
            filedata.f_wasted_content_space);
    }
    if (filedata.f_wasted_align_count) {
        const char *p = "";
        if (printfilenames) {
            p = sanitized(filename,buffer1,BUFFERSIZE);
        }
        P("Warning %s: " LONGESTUFMT
            " instances of unused alignment space exist "
            "and total " LONGESTUFMT " bytes of alignment.\n",
            p,
            filedata.f_wasted_align_count,
            filedata.f_wasted_align_space);
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