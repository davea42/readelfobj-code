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
#ifndef DWARF_MACHOREAD_H
#define DWARF_MACHOREAD_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*  There are reports that this limit of the number of bytes of
     MAX_COMMANDS_SIZE  16464
 
    Macho object commands is a hard kernel limit in iOS.
    But our testcase /home/davea/dwarf/regressiontests/macuniv/demo 
    uses 17K or so lets raise this, we are not using iOS */
#define MAX_COMMANDS_SIZE  30000

struct Dwarf_Universal_Arch_s;
struct Dwarf_Universal_Head_s {
    Dwarf_Unsigned au_magic;
    Dwarf_Unsigned au_count;
    Dwarf_Unsigned au_filesize; /* of physical file */
    struct Dwarf_Universal_Arch_s * au_arches;
};
struct Dwarf_Universal_Arch_s {
    Dwarf_Unsigned au_cputype;
    Dwarf_Unsigned au_cpusubtype;
    Dwarf_Unsigned au_offset;
    Dwarf_Unsigned au_size;
    Dwarf_Unsigned au_align;
    Dwarf_Unsigned au_reserved;
};



struct generic_macho_header {
    Dwarf_Unsigned   magic;
    Dwarf_Unsigned   swappedmagic;
    Dwarf_Unsigned   cputype;
    Dwarf_Unsigned   cpusubtype;
    Dwarf_Unsigned   filetype;
    Dwarf_Unsigned   ncmds;      /* number of load commands */
    Dwarf_Unsigned   sizeofcmds; /* the size of all the load commands */
    Dwarf_Unsigned   flags;
    Dwarf_Unsigned   reserved;

};
struct generic_macho_command {
    Dwarf_Unsigned   cmd;
    Dwarf_Unsigned   cmdsize;
    Dwarf_Unsigned   offset_this_command;
};

struct generic_macho_segment_command {
    Dwarf_Unsigned   cmd;
    Dwarf_Unsigned   cmdsize;
    char             segname[24];
    Dwarf_Unsigned   vmaddr;
    Dwarf_Unsigned   vmsize;
    Dwarf_Unsigned   fileoff;
    Dwarf_Unsigned   filesize;
    Dwarf_Unsigned   maxprot;
    Dwarf_Unsigned   initprot;
    Dwarf_Unsigned   nsects;
    Dwarf_Unsigned   flags;

    /* our index into mo_commands */
    Dwarf_Unsigned   macho_command_index;
    Dwarf_Unsigned   sectionsoffset;
};

struct generic_macho_section {
    /* Larger than in file, room for NUL guaranteed */
    char          sectname[24];
    char          segname[24];
    const char *  dwarfsectname;
    Dwarf_Unsigned  addr;
    Dwarf_Unsigned  size;
    Dwarf_Unsigned  offset;
    Dwarf_Unsigned  align;
    Dwarf_Unsigned  reloff;
    Dwarf_Unsigned  nreloc;
    Dwarf_Unsigned  flags;
    Dwarf_Unsigned  reserved1;
    Dwarf_Unsigned  reserved2;
    Dwarf_Unsigned  reserved3;
    Dwarf_Unsigned  generic_segment_num;
    Dwarf_Unsigned  offset_of_sec_rec;
    Dwarf_Small*  loaded_data;
};

/*  ident[0] == 'M' means this is a macho header.
    ident[1] will be 1 indicating version 1.
    Other bytes in ident not defined, should be zero. */
struct macho_filedata_s {
    char             mo_ident[8];
    const char *     mo_path; /* libdwarf must free.*/
    int              mo_fd;
    int              mo_destruct_close_fd; /*aka: lib owns fd */
    int              mo_is_64bit;
    Dwarf_Unsigned   mo_filesize; /* object file size */
    Dwarf_Unsigned   mo_inner_offset; /* for universal inner */
    Dwarf_Small      mo_offsetsize; /* 32 or 64 section data */
    Dwarf_Small      mo_pointersize;
    int              mo_ftype;
    unsigned         mo_endian;
    size_t           mo_universal_count;
    /*Dwarf_Small    mo_machine; */
    void *(*mo_copy_word) (void *, const void *, size_t);

    /* Used to hold 32 and 64 header data */
    struct generic_macho_header mo_header;

    unsigned mo_command_count;
    Dwarf_Unsigned  mo_command_start_offset;
    struct generic_macho_command *mo_commands;
    Dwarf_Unsigned  mo_offset_after_commands;

    Dwarf_Unsigned mo_segment_count;
    Dwarf_Unsigned mo_segment_size_total; 
    struct generic_macho_segment_command *mo_segment_commands;
    /* We are also adding __TEXT sections */
    Dwarf_Unsigned mo_dwarf_sectioncount;
    struct generic_macho_section *mo_dwarf_sections;

};

typedef struct macho_filedata_s * macho_filedata;

int dwarf_construct_macho_access_path(const char *path,
    unsigned int uninumber,
    macho_filedata *mp, int *errcode);
int dwarf_construct_macho_access(int fd,const char *path,
    unsigned int uninumber,
    macho_filedata *mp, int *errcode);
int dwarf_load_macho_header(macho_filedata mfp,int *errcode);
int dwarf_load_macho_commands(macho_filedata mfp,int *errcode);
void dwarf_destruct_macho_access(macho_filedata mp);
int _dwarf_fill_in_uni_arch_64(
    struct fat_arch_64 * fa,
    struct Dwarf_Universal_Head_s *duhd,
    void *(*word_swap) (void *, const void *, size_t),
    int * errcode);
int _dwarf_macho_load_dwarf_section_details64(
    struct macho_filedata_s *mfp,
    struct generic_macho_segment_command *segp,
    Dwarf_Unsigned segi,
    int *errcode);
int _dwarf_load_segment_command_content64(struct macho_filedata_s *mfp,
    struct generic_macho_command *mmp,
    struct generic_macho_segment_command *msp,
    Dwarf_Unsigned mmpindex,int *errcode);
int _dwarf_load_macho_header64(struct macho_filedata_s *mfp,int *errcode);
int _dwarf_not_ascii(const char *s);
int _dwarf_is_known_segname(char *sname);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* DWARF_MACHOREAD_H */
