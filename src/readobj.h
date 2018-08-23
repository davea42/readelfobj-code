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
#ifndef READOBJ_H
#define READOBJ_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* readobj.h */
#if (SIZEOF_UNSIGNED_LONG < 8) && (SIZEOF_UNSIGNED_LONG_LONG == 8)
#define LONGESTXFMT  "0x%llx"
#define LONGESTXFMT8 "0x%08llx"
#define LONGESTUFMT  "%llu"
#define LONGESTSFMT  "%lld"
#define LONGESTUTYPE unsigned long long
#define LONGESTSTYPE long long
#else
#define LONGESTXFMT  "0x%lx"
#define LONGESTXFMT8 "0x%08lx"
#define LONGESTUFMT  "%lu"
#define LONGESTSFMT  "%ld"
#define LONGESTUTYPE unsigned long
#define LONGESTSTYPE long
#endif

extern char *filename;
extern int printfilenames;
extern FILE *fin;
extern char *getshstring(LONGESTUTYPE sectype);



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
};

struct generic_dynentry {
    LONGESTSTYPE  gd_tag;
    /*  gd_val stands in for d_ptr and d_val union,
        the union adds nothing in practice. */
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


/*  Use this for rel too. */
struct generic_rela {
    int          gr_isrela; /* 0 means rel, non-zero means rela */
    LONGESTUTYPE gr_offset;
    LONGESTUTYPE gr_info;
    LONGESTUTYPE gr_sym; /* From info */
    LONGESTUTYPE gr_type; /* From info */
    LONGESTSTYPE gr_addend;
};

int generic_ehdr_from_32(struct generic_ehdr *ehdr, Elf32_Ehdr *e);
int generic_ehdr_from_64(struct generic_ehdr *ehdr, Elf64_Ehdr *e);

int generic_phdr_from_phdr32(struct generic_phdr **phdr_out,
    LONGESTUTYPE * count_out,
    LONGESTUTYPE offset,
    LONGESTUTYPE entsize,
    LONGESTUTYPE count);
int
generic_phdr_from_phdr64(struct generic_phdr **phdr_out,
    LONGESTUTYPE * count_out,
    LONGESTUTYPE offset,
    LONGESTUTYPE entsize,
    LONGESTUTYPE count);

int generic_shdr_from_shdr32(struct generic_shdr **hdr_out,
    LONGESTUTYPE * count_out,
    LONGESTUTYPE offset,
    LONGESTUTYPE entsize,
    LONGESTUTYPE count);
int generic_shdr_from_shdr64(struct generic_shdr **hdr_out,
    LONGESTUTYPE * count_out,
    LONGESTUTYPE offset,
    LONGESTUTYPE entsize,
    LONGESTUTYPE count);

int generic_rel_from_rel32(struct generic_shdr * gsh,
    Elf32_Rel *relp,
    struct generic_rela *grel_out);
int generic_rel_from_rel64(struct generic_shdr * gsh,
    Elf64_Rel *relp,
    struct generic_rela *grel);
int generic_rel_from_rela32(struct generic_shdr * gsh,
    Elf32_Rela *relp,
    struct generic_rela *grel_out);
int generic_rel_from_rela64(struct generic_shdr * gsh,
    Elf64_Rela *relp,
    struct generic_rela *grel);




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

struct filedata_s {
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
extern struct filedata_s filedata;

int get_filedata(const char *name, int fd,struct filedata_s *fida);
int elf_load_dynamic32(LONGESTUTYPE offset,
    LONGESTUTYPE size);
int elf_load_dynamic64(LONGESTUTYPE offset,
    LONGESTUTYPE size);
int elf_print_dynamic(void);
char * get_dynstr_string(LONGESTUTYPE offset, LONGESTUTYPE index);
char * get_symstr_string(int is_symtab, LONGESTUTYPE offset);
char *get_shtypestring(LONGESTUTYPE x);
const char * get_em_machine_name(unsigned value);
const char * get_em_dynamic_table_name(LONGESTUTYPE value, char *buffer,
    unsigned buflen);
const char * get_program_header_type_name(unsigned value,
    char *buffer, unsigned buflen);
const char * get_section_header_flag_names(LONGESTUTYPE value,
    char *buffer, unsigned buflen);
const char * get_section_header_st_type(LONGESTUTYPE value, char *buffer,
    unsigned buflen);
int generic_elf_load_symbols32(int secnum,const char *secname,
    struct generic_symentry **gsym_out,LONGESTUTYPE offset,
    LONGESTUTYPE size, LONGESTUTYPE *count_out);
int generic_elf_load_symbols64(int secnum,const char *secname,
    struct generic_symentry **gsym_out,LONGESTUTYPE offset,LONGESTUTYPE size,
    LONGESTUTYPE *count_out);

void insert_in_use_entry(const char *description,LONGESTUTYPE offset,
    LONGESTUTYPE length,LONGESTUTYPE align);

const char * get_symbol_sto_type(LONGESTUTYPE value, char *buffer,
    unsigned buflen);
const char * get_symbol_shn_type(LONGESTUTYPE value, char *buffer,
    unsigned buflen);
const char * get_symbol_stb_string(LONGESTUTYPE val, char *buff,
    unsigned buflen);
const char * get_symbol_stt_type(LONGESTUTYPE value, char *buffer,
    unsigned buflen);
const char * get_osabi_name(LONGESTUTYPE value, char *buffer,
    unsigned buflen);



#define PREFIX "\t"
#define LUFMT "%lu"
#define UFMT "%u"
#define DFMT "%d"
#define XFMT "0x%x"

#define RO_OK         0
#define RO_ERR        1
#define RO_ERR_SEEK   2
#define RO_ERR_READ   3
#define RO_ERR_MALLOC 4
#define RO_ERR_OTHER  5

#define P printf
#define F fflush(stdout)
#define RR(buf,loc,siz)  ((fseek(fin,(long)loc,0)<0) ? RO_ERR_SEEK : \
    ((fread(buf,(long)siz,1,fin)!=1)?RO_ERR_READ:RO_OK))
#define RN(buf,siz)  ((fread(buf,siz,1,fin) != 1) ? RO_ERR_READ  : RO_OK)
/* #define CURLOC      ftell(fin)  */
#define SEEKTO(i)  ((fseek(fin,(long)(i),SEEK_SET) == 0)? RO_OK: RO_ERR_SEEK)

int cur_read_loc(FILE *fin, long* fileoffset);

/*  This will be altered to deal with endianness */
/* #define ASSIGN(t,s) (t = s) */
#ifdef WORDS_BIGENDIAN
#define ASSIGN(t,s)                             \
    do {                                        \
        unsigned tbyte = sizeof(t) - sizeof(s); \
        t = 0;                                  \
        filedata.f_copy_word(((char *)t)+tbyte ,&s,sizeof(s)); \
    } while (0)
#else /* LITTLE ENDIAN */
#define ASSIGN(t,s)                             \
    do {                                        \
        t = 0;                                  \
        filedata.f_copy_word(&t,&s,sizeof(s));    \
    } while (0)
#endif /* end LITTLE- BIG-ENDIAN */

#define BUFFERSIZE 1000  /* For santized() calls */

#ifndef EI_NIDENT
#define EI_NIDENT 16
#endif /* EI_NIDENT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* READOBJ_H */
