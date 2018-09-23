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

/*  This file reads the parts of an Apple mach-o object
    file appropriate to reading DWARF debugging data.
*/

#include "config.h"
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> /* open() */
#include <sys/stat.h> /* open() */
#include <fcntl.h> /* open() */
#include <time.h>
#include <unistd.h>
#include "dwarf_reading.h"
#include "dwarf_object_read_common.h"
#include "dwarf_machoread.h"
#include "dwarf_object_detector.h"
#include "macho-loader.h"


static char tru_path_buffer[BUFFERSIZE];

static void destructmacho(struct macho_filedata_s *mp);
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

/* was get_command_name */
int
dwarf_get_macho_command_name(LONGESTUTYPE v,const char ** name,int *errcode)
{
    unsigned i = 0;

    for( ; commandname[i].name; i++) {
        if (v==commandname[i].val) {
            *name = commandname[i].name;
            return DW_DLV_OK;
        }
    }
    *name = "";
    return DW_DLV_NO_ENTRY;
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

int
dwarf_construct_macho_access_path(const char *path,
    struct macho_filedata_s **mp,int *errcode)
{
    int fd = -1;
    int res = 0;
    struct macho_filedata_s *mymp = 0;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        *errcode = RO_ERR_PATH_SIZE;
        return DW_DLV_ERROR;
    }
    res = dwarf_construct_macho_access(fd,
        path,&mymp,errcode);
    if (res != DW_DLV_OK) {
        close(fd);
        return res;
    }
    mymp->mo_destruct_close_fd = TRUE;
    *mp = mymp;
    return res;
}
  

/* Here path is not essential. Pass in with "" if unknown. */
int 
dwarf_construct_macho_access(int fd,
    const char *path,
    struct macho_filedata_s **mp,int *errcode)
{
    unsigned ftype = 0;
    unsigned endian = 0;
    unsigned offsetsize = 0;
    size_t   filesize;
    struct macho_filedata_s *mfp = 0;
    int      res = 0;

    res = dwarf_object_detector_fd(fd,
        &ftype,&endian,&offsetsize, &filesize, errcode);
    if (res != DW_DLV_OK) {
        return res;
    }

    mfp = calloc(1,sizeof(struct macho_filedata_s));
    if (!mfp) {
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    mfp->mo_fd = fd;
    mfp->mo_ident[0] = 'M';
    mfp->mo_ident[1] = 1;
    mfp->mo_offsetsize = offsetsize;
    mfp->mo_filesize = filesize;
    mfp->mo_endian = endian;
    mfp->mo_destruct_close_fd = FALSE;
    *mp = mfp;
    return DW_DLV_OK;
}


/* destructmacho(struct macho_filedata_s *mp) */
int 
dwarf_destruct_macho_access(struct macho_filedata_s *mp,int *errcode)
{
    if (mp->mo_destruct_close_fd) {
        close(mp->mo_fd);
        mp->mo_fd = -1;
    }
#if 0
    if (mp->mo_file) {
        fclose(mp->mo_file);
        mp->mo_file = 0;
    }
#endif
    if (mp->mo_commands){
        free(mp->mo_commands);
        mp->mo_commands = 0;
    }
    if (mp->mo_segment_commands){
        free(mp->mo_segment_commands);
        mp->mo_segment_commands = 0;
    }
    memset(mp,0,sizeof(*mp));
    return DW_DLV_OK;
}

/* load_macho_header32(struct macho_filedata_s *mfp)*/
static int
load_macho_header32(struct macho_filedata_s *mfp, int *errcode)
{
    struct mach_header mh32;
    int res = 0;

    res = RRMOA(mfp->mo_fd,&mh32,0,sizeof(mh32),errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    /* Do not adjust endianness of magic, leave as-is. */
    mfp->mo_header.magic = mh32.magic;
    ASSIGNMO(mfp->mo_copy_word,mfp->mo_header.cputype,mh32.cputype);
    ASSIGNMO(mfp->mo_copy_word,mfp->mo_header.cpusubtype,mh32.cpusubtype);
    ASSIGNMO(mfp->mo_copy_word,mfp->mo_header.filetype,mh32.filetype);
    ASSIGNMO(mfp->mo_copy_word,mfp->mo_header.ncmds,mh32.ncmds);
    ASSIGNMO(mfp->mo_copy_word,mfp->mo_header.sizeofcmds,mh32.sizeofcmds);
    ASSIGNMO(mfp->mo_copy_word,mfp->mo_header.flags,mh32.flags);
    mfp->mo_header.reserved = 0;
    mfp->mo_command_count = mfp->mo_header.ncmds;
    mfp->mo_command_start_offset = sizeof(mh32);
    return DW_DLV_OK;
}

/* load_macho_header64(struct macho_filedata_s *mfp) */
static int
load_macho_header64(struct macho_filedata_s *mfp,int *errcode)
{
    struct mach_header_64 mh64;
    int res = 0;

    res = RRMOA(mfp->mo_fd,&mh64,0,sizeof(mh64),errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    /* Do not adjust endianness of magic, leave as-is. */
    mfp->mo_header.magic = mh64.magic;
    ASSIGNMO(mfp->mo_copy_word,mfp->mo_header.cputype,mh64.cputype);
    ASSIGNMO(mfp->mo_copy_word,mfp->mo_header.cpusubtype,mh64.cpusubtype);
    ASSIGNMO(mfp->mo_copy_word,mfp->mo_header.filetype,mh64.filetype);
    ASSIGNMO(mfp->mo_copy_word,mfp->mo_header.ncmds,mh64.ncmds);
    ASSIGNMO(mfp->mo_copy_word,mfp->mo_header.sizeofcmds,mh64.sizeofcmds);
    ASSIGNMO(mfp->mo_copy_word,mfp->mo_header.flags,mh64.flags);
    ASSIGNMO(mfp->mo_copy_word,mfp->mo_header.reserved,mh64.reserved);
    mfp->mo_command_count = mfp->mo_header.ncmds;
    mfp->mo_command_start_offset = sizeof(mh64);
    return DW_DLV_OK;
}

/* int load_macho_header(struct macho_filedata_s *mfp) */
int
dwarf_load_macho_header(struct macho_filedata_s *mfp, int *errcode)
{
    int res = 0;

    if (mfp->mo_offsetsize == 32) {
        res = load_macho_header32(mfp,errcode);
    } else if (mfp->mo_offsetsize == 64) {
        res = load_macho_header64(mfp,errcode);
    } else {
        *errcode = RO_ERR_BADOFFSETSIZE;
        return DW_DLV_ERROR;
    }
    return res;
}


static int
load_segment_command_content32(struct macho_filedata_s *mfp,
    struct generic_macho_command *mmp,
    struct generic_segment_command *msp,
    LONGESTUTYPE mmpindex,
    int *errcode)
{
    struct segment_command sc;
    int res = 0;
    LONGESTUTYPE filesize = mfp->mo_filesize;
    LONGESTUTYPE segoffset = mmp->offset_this_command;
    LONGESTUTYPE afterseghdr = segoffset + sizeof(sc);

    if (mmp->offset_this_command > filesize ||
        mmp->cmdsize > filesize ||
        (mmp->cmdsize + mmp->offset_this_command) > filesize ) {
        *errcode = RO_ERR_LOADSEGOFFSETBAD;
        return DW_DLV_ERROR;
    }
    res = RRMOA(mfp->mo_fd,&sc,mmp->offset_this_command,sizeof(sc),errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    ASSIGNMO(mfp->mo_copy_word,msp->cmd,sc.cmd);
    ASSIGNMO(mfp->mo_copy_word,msp->cmdsize,sc.cmdsize);
    strncpy(msp->segname,sc.segname,16);
    msp->segname[16] =0;
    ASSIGNMO(mfp->mo_copy_word,msp->vmaddr,sc.vmaddr);
    ASSIGNMO(mfp->mo_copy_word,msp->vmsize,sc.vmsize);
    ASSIGNMO(mfp->mo_copy_word,msp->fileoff,sc.fileoff);
    ASSIGNMO(mfp->mo_copy_word,msp->filesize,sc.filesize);
    if (msp->fileoff > mfp->mo_filesize ||
        msp->filesize > mfp->mo_filesize) {
        /* corrupt */
        *errcode = RO_ERR_FILEOFFSETBAD;
        return DW_DLV_ERROR;
    }
    if ((msp->fileoff+msp->filesize ) > filesize) {
        /* corrupt */
        *errcode = RO_ERR_FILEOFFSETBAD;
        return DW_DLV_ERROR;
    }
    ASSIGNMO(mfp->mo_copy_word,msp->maxprot,sc.maxprot);
    ASSIGNMO(mfp->mo_copy_word,msp->initprot,sc.initprot);
    ASSIGNMO(mfp->mo_copy_word,msp->nsects,sc.nsects);
    ASSIGNMO(mfp->mo_copy_word,msp->flags,sc.flags);
    msp->macho_command_index = mmpindex;
    msp->sectionsoffset = afterseghdr;
    return DW_DLV_OK;
}
static int
load_segment_command_content64(struct macho_filedata_s *mfp,
    struct generic_macho_command *mmp,
    struct generic_segment_command *msp,
    LONGESTUTYPE mmpindex,int *errcode)
{
    struct segment_command_64 sc;
    int res = 0;
    LONGESTUTYPE filesize = mfp->mo_filesize;
    LONGESTUTYPE segoffset = mmp->offset_this_command;
    LONGESTUTYPE afterseghdr = segoffset + sizeof(sc);

    if (mmp->offset_this_command > filesize ||
        mmp->cmdsize > filesize ||
        (mmp->cmdsize + mmp->offset_this_command) > filesize ) {
        *errcode = RO_ERR_FILEOFFSETBAD;
        return DW_DLV_ERROR;
    }
    res = RRMOA(mfp->mo_fd,&sc,mmp->offset_this_command,sizeof(sc),errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    ASSIGNMO(mfp->mo_copy_word,msp->cmd,sc.cmd);
    ASSIGNMO(mfp->mo_copy_word,msp->cmdsize,sc.cmdsize);
    strncpy(msp->segname,sc.segname,16);
    msp->segname[16] =0;
    ASSIGNMO(mfp->mo_copy_word,msp->vmaddr,sc.vmaddr);
    ASSIGNMO(mfp->mo_copy_word,msp->vmsize,sc.vmsize);
    ASSIGNMO(mfp->mo_copy_word,msp->fileoff,sc.fileoff);
    ASSIGNMO(mfp->mo_copy_word,msp->filesize,sc.filesize);
    if (msp->fileoff > filesize ||
        msp->filesize > filesize) {
        /* corrupt */
        return DW_DLV_ERROR;
    }
    if ((msp->fileoff+msp->filesize ) > filesize) {
        /* corrupt */
        *errcode = RO_ERR_FILEOFFSETBAD;
        return DW_DLV_ERROR;
    }
    ASSIGNMO(mfp->mo_copy_word,msp->maxprot,sc.maxprot);
    ASSIGNMO(mfp->mo_copy_word,msp->initprot,sc.initprot);
    ASSIGNMO(mfp->mo_copy_word,msp->nsects,sc.nsects);
    ASSIGNMO(mfp->mo_copy_word,msp->flags,sc.flags);
    msp->macho_command_index = mmpindex;
    msp->sectionsoffset = afterseghdr;
    return DW_DLV_OK;
}

static int
dwarf_macho_load_segment_commands(struct macho_filedata_s *mfp,int *errcode)
{
    LONGESTUTYPE i = 0;
    struct generic_macho_command *mmp = 0;
    struct generic_segment_command *msp = 0;

    if(mfp->mo_segment_count < 1) {
        return DW_DLV_OK;
    }
    mfp->mo_segment_commands = (struct generic_segment_command *)
        calloc(sizeof(struct generic_segment_command),mfp->mo_segment_count);
    if(!mfp->mo_segment_commands) {
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }

    mmp = mfp->mo_commands;
    msp = mfp->mo_segment_commands;
    for (i = 0 ; i < mfp->mo_command_count; ++i,++mmp) {
        unsigned cmd = mmp->cmd;
        int res = 0;

        if (cmd == LC_SEGMENT) {
            res = load_segment_command_content32(mfp,mmp,msp,i,errcode);
            ++msp;
        } else if (cmd == LC_SEGMENT_64) {
            res = load_segment_command_content64(mfp,mmp,msp,i,errcode);
            ++msp;
        }
        if (res != DW_DLV_OK) {
            return res;
        }
    }
    return DW_DLV_OK;
}

static int
dwarf_macho_load_dwarf_section_details32(struct macho_filedata_s *mfp,
    struct generic_segment_command *segp,
    LONGESTUTYPE segi,int *errcode)
{
    LONGESTUTYPE seci = 0;
    LONGESTUTYPE seccount = segp->nsects;
    struct generic_section * secp = 0;
    LONGESTUTYPE secalloc = seccount+1;
    LONGESTUTYPE curoff = segp->sectionsoffset;
    LONGESTUTYPE shdrlen = sizeof(struct section);

    struct generic_section *secs = 0;

    secs = (struct generic_section *)calloc(sizeof(struct generic_section),
        secalloc);
    if(!secs) {
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_OK;
    }
    mfp->mo_dwarf_sections = secs;
    mfp->mo_dwarf_sectioncount = secalloc;
    secs->offset_of_sec_rec = curoff;
    /*  Leave 0 section all zeros except our offset,
        elf-like in a sense */
    ++secs;
    for (; seci < seccount; ++seci,++secs,curoff += shdrlen ) {
        struct section mosec;
        int res = 0;

        res = RRMOA(mfp->mo_fd,&mosec,curoff,sizeof(mosec),errcode);
        if (res != DW_DLV_OK) {
            return res;
        }
        strncpy(secs->sectname,mosec.sectname,16);
        secs->sectname[16] = 0;
        strncpy(secs->segname,mosec.segname,16);
        secs->segname[16] = 0;
        ASSIGNMO(mfp->mo_copy_word,secs->addr,mosec.addr);
        ASSIGNMO(mfp->mo_copy_word,secs->size,mosec.size);
        ASSIGNMO(mfp->mo_copy_word,secs->offset,mosec.offset);
        ASSIGNMO(mfp->mo_copy_word,secs->align,mosec.align);
        ASSIGNMO(mfp->mo_copy_word,secs->reloff,mosec.reloff);
        ASSIGNMO(mfp->mo_copy_word,secs->nreloc,mosec.nreloc);
        ASSIGNMO(mfp->mo_copy_word,secs->flags,mosec.flags);
        if (secs->offset > mfp->mo_filesize ||
            secs->size > mfp->mo_filesize ||
            (secs->offset+secs->size) > mfp->mo_filesize) {
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        secs->reserved1 = 0;
        secs->reserved2 = 0;
        secs->reserved3 = 0;
        secs->generic_segment_num  = segi;
        secs->offset_of_sec_rec = curoff;
    }
    return DW_DLV_OK;
}
static int
dwarf_macho_load_dwarf_section_details64(struct macho_filedata_s *mfp,
    struct generic_segment_command *segp,
    LONGESTUTYPE segi,
    int *errcode)
{
    LONGESTUTYPE seci = 0;
    LONGESTUTYPE seccount = segp->nsects;
    struct generic_section * secp = 0;
    LONGESTUTYPE secalloc = seccount+1;
    LONGESTUTYPE curoff = segp->sectionsoffset;
    LONGESTUTYPE shdrlen = sizeof(struct section_64);
    struct generic_section *secs = 0;

    secs = (struct generic_section *)calloc(sizeof(struct generic_section),
        secalloc);
    if(!secs) {
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    mfp->mo_dwarf_sections = secs;
    mfp->mo_dwarf_sectioncount = secalloc;
    secs->offset_of_sec_rec = curoff;
    /*  Leave 0 section all zeros except our offset,
        elf-like in a sense */
    ++secs;
    for (; seci < seccount; ++seci,++secs,curoff += shdrlen ) {
        int res = 0;
        struct section_64 mosec;

        res = RRMOA(mfp->mo_fd,&mosec,curoff,sizeof(mosec),errcode);
        if (res != DW_DLV_OK) {
            return res;
        }
        strncpy(secs->sectname,mosec.sectname,16);
        secs->sectname[16] = 0;
        strncpy(secs->segname,mosec.segname,16);
        secs->segname[16] = 0;
        ASSIGNMO(mfp->mo_copy_word,secs->addr,mosec.addr);
        ASSIGNMO(mfp->mo_copy_word,secs->size,mosec.size);
        ASSIGNMO(mfp->mo_copy_word,secs->offset,mosec.offset);
        ASSIGNMO(mfp->mo_copy_word,secs->align,mosec.align);
        ASSIGNMO(mfp->mo_copy_word,secs->reloff,mosec.reloff);
        ASSIGNMO(mfp->mo_copy_word,secs->nreloc,mosec.nreloc);
        ASSIGNMO(mfp->mo_copy_word,secs->flags,mosec.flags);
        if (secs->offset > mfp->mo_filesize ||
            secs->size > mfp->mo_filesize ||
            (secs->offset+secs->size) > mfp->mo_filesize) {
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_OK;
        }
        secs->reserved1 = 0;
        secs->reserved2 = 0;
        secs->reserved3 = 0;
        secs->offset_of_sec_rec = curoff;
        secs->generic_segment_num  = segi;
    }
    return DW_DLV_OK;
}

static int
dwarf_macho_load_dwarf_section_details(struct macho_filedata_s *mfp,
    struct generic_segment_command *segp,
    LONGESTUTYPE segi,int *errcode)
{
    LONGESTUTYPE seci = 0;
    LONGESTUTYPE seccount = segp->nsects;
    struct generic_section * secp = 0;
    LONGESTUTYPE secalloc = seccount+1;
    int res = 0;

    if (mfp->mo_offsetsize == 32) {
        res = dwarf_macho_load_dwarf_section_details32(mfp,segp,segi,errcode);
    } else if (mfp->mo_offsetsize == 64) {
        res = dwarf_macho_load_dwarf_section_details64(mfp,segp,segi,errcode);
    } else {
        *errcode = RO_ERR_BADOFFSETSIZE;
        return DW_DLV_ERROR;
    }
    return DW_DLV_OK;
}

static int
dwarf_macho_load_dwarf_sections(struct macho_filedata_s *mfp,int *errcode)
{
    LONGESTUTYPE segi = 0;

    struct generic_segment_command *segp = mfp->mo_segment_commands;
    struct generic_section * secp = 0;
    for ( ; segi < mfp->mo_segment_count; ++segi,++segp) {
        int res = 0;

        if (strcmp(segp->segname,"__DWARF")) {
            continue;
        }
        /* Found DWARF, for now assume only one such. */
        res = dwarf_macho_load_dwarf_section_details(mfp,segp,segi,errcode);
        return res;
    }
    return RO_NO_ENTRY;
}

/* Works the same, 32 or 64 bit */
int
dwarf_load_macho_commands(struct macho_filedata_s *mfp,int *errcode)
{
    LONGESTUTYPE cmdi = 0;
    LONGESTUTYPE curoff = mfp->mo_command_start_offset;
    LONGESTUTYPE cmdspace = 0;
    struct load_command mc;
    struct generic_macho_command *mcp = 0;
    unsigned segment_command_count = 0;
    int res = 0;

    if ((curoff + mfp->mo_command_count * sizeof(mc)) >=
        mfp->mo_filesize) {
        /* corrupt object. */
        *errcode = RO_ERR_LOADSEGOFFSETBAD;
        return DW_DLV_ERROR;
    }

    mfp->mo_commands = (struct generic_macho_command *) calloc(
        mfp->mo_command_count,sizeof(struct generic_macho_command));
    if( !mfp->mo_commands) {
        /* out of memory */
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    mcp = mfp->mo_commands;
    for ( ; cmdi < mfp->mo_header.ncmds; ++cmdi,++mcp ) {
        int res = 0;

        res = RRMOA(mfp->mo_fd,&mc,curoff,sizeof(mc),errcode);
        if (res != RO_OK) {
            return res;
        }
        ASSIGNMO(mfp->mo_copy_word,mcp->cmd,mc.cmd);
        ASSIGNMO(mfp->mo_copy_word,mcp->cmdsize,mc.cmdsize);
        mcp->offset_this_command = curoff;
        curoff += mcp->cmdsize;
        cmdspace += mcp->cmdsize;
        if (mcp->cmdsize > mfp->mo_filesize ||
            curoff > mfp->mo_filesize) {
            /* corrupt object */
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        if (mcp->cmd == LC_SEGMENT || mcp->cmd == LC_SEGMENT_64) {
            segment_command_count++;
        }
    }
    mfp->mo_segment_count = segment_command_count;
    res = dwarf_macho_load_segment_commands(mfp,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    res = dwarf_macho_load_dwarf_sections(mfp,errcode);
    return res;
}
