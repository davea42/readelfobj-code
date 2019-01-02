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
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif /* HAVE_MALLOC_H */
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> /* open() */
#include <sys/stat.h> /* open() */
#include <fcntl.h> /* open() */
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#include "dwarf_reading.h"
#include "dwarf_object_read_common.h"
#include "dwarf_machoread.h"
#include "dwarf_object_detector.h"
#include "dwarf_macho_loader.h"

#ifndef TYP
#define TYP(n,l) char n[l]
#endif /* TYPE */

#ifdef WORDS_BIGENDIAN
#define ASNAR(func,t,s)                         \
    do {                                        \
        unsigned tbyte = sizeof(t) - sizeof(s); \
        t = 0;                                  \
        func(((char *)&t)+tbyte ,&s[0],sizeof(s));  \
    } while (0)
#else /* LITTLE ENDIAN */
#define ASNAR(func,t,s)                         \
    do {                                        \
        t = 0;                                  \
        func(&t,&s[0],sizeof(s));               \
    } while (0)
#endif /* end LITTLE- BIG-ENDIAN */

int
dwarf_construct_macho_access_path(const char *path,
    struct macho_filedata_s **mp,int *errcode)
{
    int fd = -1;
    int res = 0;
    struct macho_filedata_s *mymp = 0;

    fd = open(path, O_RDONLY|O_BINARY);
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
    mfp->mo_path = strdup(path);
    mfp->mo_filesize = filesize;
    mfp->mo_byteorder = endian;
    mfp->mo_destruct_close_fd = FALSE;
    *mp = mfp;
    return DW_DLV_OK;
}


void
dwarf_destruct_macho_access(struct macho_filedata_s *mp)
{
    if (mp->mo_destruct_close_fd) {
        close(mp->mo_fd);
        mp->mo_fd = -1;
    }
    if (mp->mo_commands){
        free(mp->mo_commands);
        mp->mo_commands = 0;
    }
    if (mp->mo_segment_commands){
        free(mp->mo_segment_commands);
        mp->mo_segment_commands = 0;
    }
    free((char *)mp->mo_path);
    if (mp->mo_dwarf_sections) {
        struct generic_macho_section *sp = 0;
        Dwarf_Unsigned i = 0;

        sp = mp->mo_dwarf_sections;
        for( i=0; i < mp->mo_dwarf_sectioncount; ++i,++sp) {
            if (sp->loaded_data) {
                free(sp->loaded_data);
                sp->loaded_data = 0;
            }
        }
        free(mp->mo_dwarf_sections);
        mp->mo_dwarf_sections = 0;
    }
    memset(mp,0,sizeof(*mp));
    return;
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
    ASNAR(memcpy,mfp->mo_header.magic,mh32.magic);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.cputype,mh32.cputype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.cpusubtype,mh32.cpusubtype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.filetype,mh32.filetype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.ncmds,mh32.ncmds);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.sizeofcmds,mh32.sizeofcmds);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.flags,mh32.flags);
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
    ASNAR(memcpy,mfp->mo_header.magic,mh64.magic);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.cputype,mh64.cputype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.cpusubtype,mh64.cpusubtype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.filetype,mh64.filetype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.ncmds,mh64.ncmds);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.sizeofcmds,mh64.sizeofcmds);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.flags,mh64.flags);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.reserved,mh64.reserved);
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
    struct generic_macho_segment_command *msp,
    Dwarf_Unsigned mmpindex,
    int *errcode)
{
    struct segment_command sc;
    int res = 0;
    Dwarf_Unsigned filesize = mfp->mo_filesize;
    Dwarf_Unsigned segoffset = mmp->offset_this_command;
    Dwarf_Unsigned afterseghdr = segoffset + sizeof(sc);

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
    ASNAR(mfp->mo_copy_word,msp->cmd,sc.cmd);
    ASNAR(mfp->mo_copy_word,msp->cmdsize,sc.cmdsize);
    strncpy(msp->segname,sc.segname,16);
    msp->segname[16] =0;
    ASNAR(mfp->mo_copy_word,msp->vmaddr,sc.vmaddr);
    ASNAR(mfp->mo_copy_word,msp->vmsize,sc.vmsize);
    ASNAR(mfp->mo_copy_word,msp->fileoff,sc.fileoff);
    ASNAR(mfp->mo_copy_word,msp->filesize,sc.filesize);
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
    ASNAR(mfp->mo_copy_word,msp->maxprot,sc.maxprot);
    ASNAR(mfp->mo_copy_word,msp->initprot,sc.initprot);
    ASNAR(mfp->mo_copy_word,msp->nsects,sc.nsects);
    ASNAR(mfp->mo_copy_word,msp->flags,sc.flags);
    msp->macho_command_index = mmpindex;
    msp->sectionsoffset = afterseghdr;
    return DW_DLV_OK;
}
static int
load_segment_command_content64(struct macho_filedata_s *mfp,
    struct generic_macho_command *mmp,
    struct generic_macho_segment_command *msp,
    Dwarf_Unsigned mmpindex,int *errcode)
{
    struct segment_command_64 sc;
    int res = 0;
    Dwarf_Unsigned filesize = mfp->mo_filesize;
    Dwarf_Unsigned segoffset = mmp->offset_this_command;
    Dwarf_Unsigned afterseghdr = segoffset + sizeof(sc);

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
    ASNAR(mfp->mo_copy_word,msp->cmd,sc.cmd);
    ASNAR(mfp->mo_copy_word,msp->cmdsize,sc.cmdsize);
    strncpy(msp->segname,sc.segname,16);
    msp->segname[16] =0;
    ASNAR(mfp->mo_copy_word,msp->vmaddr,sc.vmaddr);
    ASNAR(mfp->mo_copy_word,msp->vmsize,sc.vmsize);
    ASNAR(mfp->mo_copy_word,msp->fileoff,sc.fileoff);
    ASNAR(mfp->mo_copy_word,msp->filesize,sc.filesize);
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
    ASNAR(mfp->mo_copy_word,msp->maxprot,sc.maxprot);
    ASNAR(mfp->mo_copy_word,msp->initprot,sc.initprot);
    ASNAR(mfp->mo_copy_word,msp->nsects,sc.nsects);
    ASNAR(mfp->mo_copy_word,msp->flags,sc.flags);
    msp->macho_command_index = mmpindex;
    msp->sectionsoffset = afterseghdr;
    return DW_DLV_OK;
}

static int
dwarf_macho_load_segment_commands(struct macho_filedata_s *mfp,int *errcode)
{
    Dwarf_Unsigned i = 0;
    struct generic_macho_command *mmp = 0;
    struct generic_macho_segment_command *msp = 0;

    if(mfp->mo_segment_count < 1) {
        return DW_DLV_OK;
    }
    mfp->mo_segment_commands = (struct generic_macho_segment_command *)
        calloc(sizeof(struct generic_macho_segment_command),
        mfp->mo_segment_count);
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
    struct generic_macho_segment_command *segp,
    Dwarf_Unsigned segi,int *errcode)
{
    Dwarf_Unsigned seci = 0;
    Dwarf_Unsigned seccount = segp->nsects;
    Dwarf_Unsigned secalloc = seccount+1;
    Dwarf_Unsigned curoff = segp->sectionsoffset;
    Dwarf_Unsigned shdrlen = sizeof(struct section);

    struct generic_macho_section *secs = 0;

    secs = (struct generic_macho_section *)calloc(
        sizeof(struct generic_macho_section),
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
    secs->dwarfsectname = "";
    ++secs;
    seci = 1;
    for (; seci < secalloc; ++seci,++secs,curoff += shdrlen ) {
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
        ASNAR(mfp->mo_copy_word,secs->addr,mosec.addr);
        ASNAR(mfp->mo_copy_word,secs->size,mosec.size);
        ASNAR(mfp->mo_copy_word,secs->offset,mosec.offset);
        ASNAR(mfp->mo_copy_word,secs->align,mosec.align);
        ASNAR(mfp->mo_copy_word,secs->reloff,mosec.reloff);
        ASNAR(mfp->mo_copy_word,secs->nreloc,mosec.nreloc);
        ASNAR(mfp->mo_copy_word,secs->flags,mosec.flags);
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
    struct generic_macho_segment_command *segp,
    Dwarf_Unsigned segi,
    int *errcode)
{
    Dwarf_Unsigned seci = 0;
    Dwarf_Unsigned seccount = segp->nsects;
    Dwarf_Unsigned secalloc = seccount+1;
    Dwarf_Unsigned curoff = segp->sectionsoffset;
    Dwarf_Unsigned shdrlen = sizeof(struct section_64);
    struct generic_macho_section *secs = 0;

    secs = (struct generic_macho_section *)
        calloc(sizeof(struct generic_macho_section), secalloc);
    if(!secs) {
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    mfp->mo_dwarf_sections = secs;
    mfp->mo_dwarf_sectioncount = secalloc;
    secs->offset_of_sec_rec = curoff;
    /*  Leave 0 section all zeros except our offset,
        elf-like in a sense */
    secs->dwarfsectname = "";
    seci = 1;
    ++secs;
    for (; seci < secalloc; ++seci,++secs,curoff += shdrlen ) {
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
        ASNAR(mfp->mo_copy_word,secs->addr,mosec.addr);
        ASNAR(mfp->mo_copy_word,secs->size,mosec.size);
        ASNAR(mfp->mo_copy_word,secs->offset,mosec.offset);
        ASNAR(mfp->mo_copy_word,secs->align,mosec.align);
        ASNAR(mfp->mo_copy_word,secs->reloff,mosec.reloff);
        ASNAR(mfp->mo_copy_word,secs->nreloc,mosec.nreloc);
        ASNAR(mfp->mo_copy_word,secs->flags,mosec.flags);
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
    struct generic_macho_segment_command *segp,
    Dwarf_Unsigned segi,int *errcode)
{
    int res = 0;

    if (mfp->mo_offsetsize == 32) {
        res = dwarf_macho_load_dwarf_section_details32(mfp,
            segp,segi,errcode);
    } else if (mfp->mo_offsetsize == 64) {
        res = dwarf_macho_load_dwarf_section_details64(mfp,
            segp,segi,errcode);
    } else {
        *errcode = RO_ERR_BADOFFSETSIZE;
        return DW_DLV_ERROR;
    }
    return res;
}

static int
dwarf_macho_load_dwarf_sections(struct macho_filedata_s *mfp,
    int *errcode)
{
    Dwarf_Unsigned segi = 0;

    struct generic_macho_segment_command *segp = mfp->mo_segment_commands;
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
    Dwarf_Unsigned cmdi = 0;
    Dwarf_Unsigned curoff = mfp->mo_command_start_offset;
    Dwarf_Unsigned cmdspace = 0;
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
        res = RRMOA(mfp->mo_fd,&mc,curoff,sizeof(mc),errcode);
        if (res != RO_OK) {
            return res;
        }
        ASNAR(mfp->mo_copy_word,mcp->cmd,mc.cmd);
        ASNAR(mfp->mo_copy_word,mcp->cmdsize,mc.cmdsize);
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
