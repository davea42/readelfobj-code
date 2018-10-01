/*
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
#ifndef READOBJMACHO_H
#define READOBJMACHO_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern char *filename;
extern int printfilenames;
extern FILE *fin;

int cur_read_loc(FILE *fin, long* fileoffset);
#ifdef WORDS_BIGENDIAN
#define ASSIGNMO(gp,t,s)                             \
    do {                                        \
        unsigned tbyte = sizeof(t) - sizeof(s); \
        t = 0;                                  \
        gp->mo_copy_word(((char *)t)+tbyte ,&s,sizeof(s)); \
    } while (0)

#else /* LITTLE ENDIAN */
#define ASSIGNMO(gp,t,s)                             \
    do {                                        \
        t = 0;                                  \
        gp->mo_copy_word(&t,&s,sizeof(s));    \
    } while (0)
#endif /* end LITTLE- BIG-ENDIAN */

struct generic_macho_header {
    LONGESTUTYPE   magic;     
    LONGESTUTYPE   cputype;     
    LONGESTUTYPE   cpusubtype;     
    LONGESTUTYPE   filetype;  
    LONGESTUTYPE   ncmds;      /* number of load commands */
    LONGESTUTYPE   sizeofcmds; /* the size of all the load commands */
    LONGESTUTYPE   flags;   
    LONGESTUTYPE   reserved;  
};
struct generic_macho_command {
    LONGESTUTYPE   cmd;
    LONGESTUTYPE   cmdsize;
    LONGESTUTYPE   offset_this_command;
};

struct generic_segment_command {
    LONGESTUTYPE   cmd;
    LONGESTUTYPE   cmdsize;
    char segname[16];
    LONGESTUTYPE   vmaddr;
    LONGESTUTYPE   vmsize;
    LONGESTUTYPE   fileoff;
    LONGESTUTYPE   filesize;
    LONGESTUTYPE   maxprot;
    LONGESTUTYPE   initprot;
    LONGESTUTYPE   nsects;
    LONGESTUTYPE   flags;
    LONGESTUTYPE   macho_command_index; /* our index into mo_commands */
    LONGESTUTYPE   sectionsoffset;
};

struct generic_section {
    /* Larger than in file, room for NUL guaranteed */
    char          sectname[24];
    char          segname[24];
    LONGESTUTYPE  addr;
    LONGESTUTYPE  size; 
    LONGESTUTYPE  offset;
    LONGESTUTYPE  align;
    LONGESTUTYPE  reloff;
    LONGESTUTYPE  nreloc; 
    LONGESTUTYPE  flags;   
    LONGESTUTYPE  reserved1;
    LONGESTUTYPE  reserved2;
    LONGESTUTYPE  reserved3;
    LONGESTUTYPE  generic_segment_num;
    LONGESTUTYPE  offset_of_sec_rec;
};


struct macho_filedata_s {
    FILE *  mo_file;
    const char *mo_path;
    unsigned mo_endian;
    LONGESTUTYPE mo_filesize;

    /*  32 or 64, this is the object offset size, not 
        DWARF offset size */
    LONGESTUTYPE mo_offsetsize; 

    void *(*mo_copy_word) (void *, const void *, size_t);
    /* Used to hold 32 and 64 header data */
    struct generic_macho_header mo_header;

    unsigned mo_command_count;
    LONGESTUTYPE  mo_command_start_offset;
    struct generic_macho_command *mo_commands;
    LONGESTUTYPE  mo_offset_after_commands; /* properly aligned value */

    LONGESTUTYPE mo_segment_count;
    struct generic_segment_command *mo_segment_commands;

    LONGESTUTYPE mo_dwarf_sectioncount;
    struct generic_section *mo_dwarf_sections;

 
};

struct macho_filedata_s macho_filedata;

int load_macho_header(struct macho_filedata_s *mfp);
int load_macho_commands(struct macho_filedata_s *mfp);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* READOBJMACHO_H */
