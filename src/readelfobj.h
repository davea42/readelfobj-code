/* Copyright (c) 2013-2018, David Anderson
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
#ifndef READELFOBJ_H
#define READELFOBJ_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern char *filename;
extern int printfilenames;

/*  Use this for rel too. */
struct generic_rela {
    int          gr_isrela; /* 0 means rel, non-zero means rela */
    LONGESTUTYPE gr_offset;
    LONGESTUTYPE gr_info;
    LONGESTUTYPE gr_sym; /* From info */
    LONGESTUTYPE gr_type; /* From info */
    LONGESTSTYPE gr_addend;
};

/*  The following are generic to simplify handling
    Elf32 and Elf64.  Some fields added where
    the two sizes have different extraction code. */
struct generic_ehdr {
    unsigned char ge_ident[EI_NIDENT];
    LONGESTUTYPE ge_type;
    LONGESTUTYPE ge_machine;
    LONGESTUTYPE ge_version;
    LONGESTUTYPE ge_entry;
    LONGESTUTYPE ge_phoff;
    LONGESTUTYPE ge_shoff;
    LONGESTUTYPE ge_flags;
    LONGESTUTYPE ge_ehsize;
    LONGESTUTYPE ge_phentsize;
    LONGESTUTYPE ge_phnum;
    LONGESTUTYPE ge_shentsize;
    LONGESTUTYPE ge_shnum;
    LONGESTUTYPE ge_shstrndx;
};
struct generic_phdr {
    LONGESTUTYPE gp_type;
    LONGESTUTYPE gp_flags;
    LONGESTUTYPE gp_offset;
    LONGESTUTYPE gp_vaddr;
    LONGESTUTYPE gp_paddr;
    LONGESTUTYPE gp_filesz;
    LONGESTUTYPE gp_memsz;
    LONGESTUTYPE gp_align;
};
struct generic_shdr {
    LONGESTUTYPE gh_secnum;
    LONGESTUTYPE gh_name;
    const char * gh_namestring;
    LONGESTUTYPE gh_type;
    const char * gh_typestring;
    LONGESTUTYPE gh_flags;
    LONGESTUTYPE gh_addr;
    LONGESTUTYPE gh_offset;
    LONGESTUTYPE gh_size;
    LONGESTUTYPE gh_link;
    LONGESTUTYPE gh_info;
    LONGESTUTYPE gh_addralign;
    LONGESTUTYPE gh_entsize;

    /*  Zero unless content read in. Malloc space
        of size gh_size,  in bytes. For dwarf
        and strings mainly. free() this if not null*/
    char *       gh_content;
    /*  If a .rel or .rela section this will point
        to generic relocation records if such
        have been loaded.
        free() this if not null. */
    LONGESTUTYPE          gh_relcount;
    struct generic_rela * gh_rels;
};

struct generic_dynentry {
    LONGESTSTYPE  gd_tag;
    /*  gd_val stands in for d_ptr and d_val union,
        the union adds nothing in practice since
        we expect ptrsize <= ulongest. */
    LONGESTUTYPE  gd_val;
    LONGESTUTYPE  gd_dyn_file_offset;
};

struct generic_symentry {
    LONGESTUTYPE gs_name;
    LONGESTUTYPE gs_value;
    LONGESTUTYPE gs_size;
    LONGESTUTYPE gs_info;
    LONGESTUTYPE gs_other;
    LONGESTUTYPE gs_shndx;
    /* derived */
    LONGESTUTYPE gs_bind;
    LONGESTUTYPE gs_type;
};

struct location {
    const char *g_name;
    LONGESTUTYPE g_offset;
    LONGESTUTYPE g_count;
    LONGESTUTYPE g_entrysize;
    LONGESTUTYPE g_totalsize;
};

struct in_use_s {
    struct in_use_s *u_next;
    const char *u_name;
    LONGESTUTYPE u_offset;
    LONGESTUTYPE u_align;
    LONGESTUTYPE u_length;
    LONGESTUTYPE u_lastbyte;
};

struct elf_filedata_s {
    /*  f_ident[0] == 'E' means it is elf and
        elf_filedata_s is the struct involved.
        Other means error/corruption of some kind.
        f_ident[1] is a version number.
        Only version 1 is defined. */
    char         f_ident[8];
    int          f_fd;
    int          f_printf_on_error;

    /* If TRUE close f_fd on destruct. */
    int          f_destruct_close_fd;

    unsigned	 f_endian;
    unsigned     f_offsetsize; /* Elf offset size, not DWARF. 32 or 64 */
    LONGESTUTYPE f_filesize;
    LONGESTUTYPE f_max_secdata_offset;
    LONGESTUTYPE f_max_progdata_offset;

    LONGESTUTYPE f_wasted_dynamic_count;
    LONGESTUTYPE f_wasted_dynamic_space;

    LONGESTUTYPE f_wasted_content_space;
    LONGESTUTYPE f_wasted_content_count;

    LONGESTUTYPE f_wasted_align_space;
    LONGESTUTYPE f_wasted_align_count;
    void *(*f_copy_word) (void *, const void *, size_t);

    struct in_use_s * f_in_use;
    struct in_use_s * f_in_use_tail;
    LONGESTUTYPE f_in_use_count;

    struct location      f_loc_ehdr;
    struct generic_ehdr* f_ehdr;

    struct location      f_loc_shdr;
    struct generic_shdr* f_shdr;

    struct location      f_loc_phdr;
    struct generic_phdr* f_phdr;

    char *f_elf_shstrings_data; /* section name strings */
    /* length of currentsection.  Might be zero..*/
    LONGESTUTYPE  f_elf_shstrings_length;
    /* size of malloc-d space */
    LONGESTUTYPE  f_elf_shstrings_max;

    /* This is the .dynamic section */
    struct location      f_loc_dynamic;
    struct generic_dynentry * f_dynamic;
    LONGESTUTYPE f_dynamic_sect_index;

    /* .dynsym, .dynstr */
    struct location      f_loc_dynsym;
    struct generic_symentry* f_dynsym;
    char  *f_dynsym_sect_strings;
    LONGESTUTYPE f_dynsym_sect_strings_max;
    LONGESTUTYPE f_dynsym_sect_strings_sect_index;
    LONGESTUTYPE f_dynsym_sect_index;

    /* .symtab .strtab */
    struct location      f_loc_symtab;
    struct generic_symentry* f_symtab;
    char * f_symtab_sect_strings;
    LONGESTUTYPE f_symtab_sect_strings_max;
    LONGESTUTYPE f_symtab_sect_strings_sect_index;
    LONGESTUTYPE f_symtab_sect_index;


};
typedef struct elf_filedata_s * elf_filedata;

int dwarf_construct_elf_access(int fd,
    const char *path,
    elf_filedata *ep,int *errcode);
int dwarf_construct_elf_access_path(const char *path,
    elf_filedata *ep,int *errcode);
int dwarf_destruct_elf_access(elf_filedata ep,int *errcode);
int dwarf_load_elf_header(elf_filedata ep,int *errcode);
int dwarf_load_elf_sectheaders(elf_filedata ep,int *errcode);
int dwarf_load_elf_progheaders(elf_filedata ep,int *errcode);

int dwarf_load_elf_dynamic(elf_filedata ep, int *errcode);
int dwarf_load_elf_symstr(elf_filedata ep, int *errcode);
int dwarf_load_elf_dynstr(elf_filedata ep, int *errcode);
int dwarf_load_elf_symtab_symbols(elf_filedata ep,int *errcode);
int dwarf_load_elf_dynsym_symbols(elf_filedata ep,int *errcode);

int dwarf_load_elf_rela(elf_filedata ep,
    LONGESTUTYPE secnum, int *errcode);
int dwarf_load_elf_rel(elf_filedata ep,
    LONGESTUTYPE secnum, int *errcode);

int dwarf_get_elf_symstr_string(elf_filedata ep,
    int is_symtab,LONGESTUTYPE index,
    char *buffer, LONGESTUTYPE bufferlen,
    int *errcode);

/*  The following for an elf checker/dumper. */
const char * dwarf_get_elf_machine_name(unsigned value);
const char * dwarf_get_elf_dynamic_table_name(
    LONGESTUTYPE value,
    char *buffer, unsigned buflen);
const char * dwarf_get_elf_program_header_type_name(
    LONGESTUTYPE value,
    char *buffer, unsigned buflen);
const char * dwarf_get_elf_section_header_flag_names(
    LONGESTUTYPE value,
    char *buffer, unsigned buflen);
const char * dwarf_get_elf_section_header_st_type(
    LONGESTUTYPE value,
    char *buffer, unsigned buflen);
void dwarf_insert_in_use_entry(elf_filedata ep,
    const char *description,LONGESTUTYPE offset,
    LONGESTUTYPE length,LONGESTUTYPE align);
const char * dwarf_get_elf_symbol_sto_type(
    LONGESTUTYPE value, char *buffer,
    unsigned buflen);
const char * dwarf_get_elf_symbol_shn_type(
    LONGESTUTYPE value, char *buffer, unsigned buflen);
const char * dwarf_get_elf_symbol_stb_string(
    LONGESTUTYPE val, char *buff, unsigned buflen);
const char * dwarf_get_elf_symbol_stt_type( LONGESTUTYPE value,
    char *buffer, unsigned buflen);
const char * dwarf_get_elf_osabi_name( LONGESTUTYPE value,
    char *buffer, unsigned buflen);
const char * dwarf_get_elf_machine_name(unsigned value);
const char * dwarf_get_elf_dynamic_table_name(
    LONGESTUTYPE value, char *buffer, unsigned buflen);
const char * dwarf_get_elf_section_header_flag_names(
    LONGESTUTYPE value, char *buffer, unsigned buflen);
const char * dwarf_get_elf_section_header_st_type_name(
    LONGESTUTYPE value, char *buffer, unsigned buflen);



int cur_read_loc(FILE *fin, long* fileoffset);

#ifndef EI_NIDENT
#define EI_NIDENT 16
#endif /* EI_NIDENT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* READELFOBJ_H */
