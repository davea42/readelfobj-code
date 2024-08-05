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
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for exit(), C89 malloc */
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_MALLOC_H
/* Useful include for some Windows compilers. */
#include <malloc.h>
#endif /* HAVE_MALLOC_H */
#include <string.h>
#include <sys/types.h> /* open() */
#include <sys/stat.h> /* open() */
#include <fcntl.h> /* open() */
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#include "dwarf_types.h"
#include "dwarf_reading.h"
#include "dwarf_object_read_common.h"
#include "dwarf_universal.h"
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

static int
dwarf_object_detector_universal_head_fd(
    int fd,
    Dwarf_Unsigned      dw_filesize,
    unsigned int      *dw_contentcount,
    Dwarf_Universal_Head * dw_head,
    int            *errcode);

#if 0
static void
dump_bytes(const char *msg,unsigned char * start, unsigned len)
{
    Dwarf_Small *end = start + len;
    Dwarf_Small *cur = start;
    printf("%s (0x%lx) ",msg,(unsigned long)start);
    for (; cur < end; cur++) {
        printf("%02x", *cur);
    }
    printf("\n");
}
#endif

static unsigned long
magic_copy(unsigned char *d, unsigned len)
{
    unsigned i = 0;
    unsigned long v = 0;

    v = d[0];
    for (i = 1 ; i < len; ++i) {
        v <<= 8;
        v |=  d[i];
    }
    return v;
}

int
dwarf_construct_macho_access_path(const char *path,
    unsigned int uninumber,
    struct macho_filedata_s **mp,
    int *errcode)
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
        path,
        uninumber,
        &mymp,errcode);
    if (res != DW_DLV_OK) {
        close(fd);
        return res;
    }
    mymp->mo_destruct_close_fd = TRUE;
    *mp = mymp;
    return res;
}

/* Reads universal binary headers, gets to
   the chosen inner binary, and returns the
   values from the inner binary.
   The filesize being that of the inner binary,
   and the fileoffset being the offset of the inner
   binary (so by definition > 0);
*/
   
static int
dwarf_macho_inner_object_fd(int fd,
    unsigned int uninumber,
    Dwarf_Unsigned outer_filesize,
    unsigned int *ftype,
    unsigned int *unibinarycount,
    unsigned int *endian,
    unsigned int *offsetsize,
    Dwarf_Unsigned *fileoffset,
    Dwarf_Unsigned *filesize,
    int *errcode)
{
    int res = 0;
    Dwarf_Universal_Head  head = 0;
    Dwarf_Unsigned innerbase = 0;
    Dwarf_Unsigned innersize = 0;

    res =  dwarf_object_detector_universal_head_fd(
        fd, outer_filesize, unibinarycount,
        &head, errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    if (uninumber >= *unibinarycount) {
         printf("FAIL: Requested inner binary number invalid\n");
         *errcode = DW_DLE_UNIVERSAL_BINARY_ERROR;
         dwarf_dealloc_universal_head(head);
         return DW_DLV_ERROR;
    }
    /*  Now find the precise details of uninumber 
        instance we want */
    
    innerbase = head->au_arches[uninumber].au_offset;
    innersize = head->au_arches[uninumber].au_size;
    if (innersize >= outer_filesize || innerbase >= outer_filesize) {
        printf("FAIL: inner object impossible. Corrupt Dwarf\n");
        *errcode = DW_DLE_UNIVERSAL_BINARY_ERROR;
        dwarf_dealloc_universal_head(head);
        return DW_DLV_ERROR;
    }
    /* Now access inner to return its specs */
    { 
        /*  But ignore the size this returns! 
            we determined that above. the following call
            does not get the inner size, we got that 
            just above here! */
        Dwarf_Unsigned fake_size = 0;

        res = dwarf_object_detector_fd_a(fd,
            ftype,endian,offsetsize,innerbase,&fake_size,
            errcode);
    }
    if (res != DW_DLV_OK) {       
        dwarf_dealloc_universal_head(head);
        return res;
    }
    *fileoffset = innerbase;
    *filesize = innersize;
    dwarf_dealloc_universal_head(head);
    return DW_DLV_OK;
}


/* Here path is not essential. Pass in with "" if unknown. */
int
dwarf_construct_macho_access(int fd,
    const char *path,
    unsigned int uninumber,
    struct macho_filedata_s **mp,int *errcode)
{
    unsigned       ftype = 0;
    unsigned       endian = 0;
    unsigned       offsetsize = 0;
    Dwarf_Unsigned filesize = 0;
    struct macho_filedata_s *mfp = 0;
    int            res = 0;
    unsigned int   ftypei = 0;
    unsigned int   endiani = 0;
    unsigned int   offsetsizei = 0;
    Dwarf_Unsigned filesizei = 0;
    Dwarf_Unsigned fileoffseti = 0;
    unsigned int   unibinarycounti = 0;

    /*  This for outer object, normal or universal.
        Hence base is 0 */
    res = dwarf_object_detector_fd_a(fd,
        &ftype,&endian,&offsetsize,0, &filesize,errcode);
    if (res != DW_DLV_OK) {
        printf("FAIL: object_detector failed on Mach-O object\n");
        return res;
    }
    if (ftype == DW_FTYPE_APPLEUNIVERSAL) {
        res = dwarf_macho_inner_object_fd(fd,
            uninumber,
            filesize,
            &ftypei,&unibinarycounti,&endiani,
            &offsetsizei,&fileoffseti,&filesizei,errcode);
        if (res != DW_DLV_OK) {
            printf("FAIL: Macho access to universal inner binary %u"
                "failed\n",uninumber);
            return res;
        }
        filesize = filesizei;
        endian = endiani;
        offsetsize = offsetsizei;
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
#ifdef WORDS_BIGENDIAN
    if (endian == DW_ENDIAN_LITTLE || endian == DW_ENDIAN_OPPOSITE ) {
        mfp->mo_copy_word = dwarf_ro_memcpy_swap_bytes;
        mfp->mo_endian = DW_ENDIAN_LITTLE;
    } else {
        mfp->mo_copy_word = memcpy;
        mfp->mo_endian = DW_ENDIAN_BIG;
    }
#else  /* LITTLE ENDIAN */
    if (endian == DW_ENDIAN_LITTLE || endian == DW_ENDIAN_SAME ) {
        mfp->mo_copy_word = memcpy;
        mfp->mo_endian = DW_ENDIAN_LITTLE;
    } else {
        mfp->mo_copy_word = dwarf_ro_memcpy_swap_bytes;
        mfp->mo_endian = DW_ENDIAN_BIG;
    }
#endif /* LITTLE- BIG-ENDIAN */

    /* for binary inside universal obj */
    mfp->mo_universal_count = unibinarycounti;
    mfp->mo_inner_offset = fileoffseti; 

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
        for (i=0; i < mp->mo_dwarf_sectioncount; ++i,++sp) {
            if (sp->loaded_data) {
                free(sp->loaded_data);
                sp->loaded_data = 0;
            }
        }
        free(mp->mo_dwarf_sections);
        mp->mo_dwarf_sections = 0;
    }
    memset(mp,0,sizeof(*mp));
    free(mp);
    return;
}

/* load_macho_header32(struct macho_filedata_s *mfp)*/
static int
load_macho_header32(struct macho_filedata_s *mfp, int *errcode)
{
    struct mach_header mh32;
    int res = 0;
    Dwarf_Unsigned inner = mfp->mo_inner_offset;

    res = RRMOA(mfp->mo_fd,&mh32,inner,sizeof(mh32),
        inner+mfp->mo_filesize,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    /* Do not adjust endianness of magic, leave as-is. */
    ASNAR(memcpy,mfp->mo_header.magic,mh32.magic);
    /* We will print the swapped one. */
    ASNAR(mfp->mo_copy_word,mfp->mo_header.swappedmagic,mh32.magic);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.cputype,mh32.cputype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.cpusubtype,
        mh32.cpusubtype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.filetype,mh32.filetype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.ncmds,mh32.ncmds);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.sizeofcmds,
        mh32.sizeofcmds);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.flags,mh32.flags);
    if (mfp->mo_header.sizeofcmds >= mfp->mo_filesize ||
        mfp->mo_header.ncmds >= mfp->mo_filesize ||
        mfp->mo_header.ncmds >= mfp->mo_header.sizeofcmds)  {
        printf("ERROR: %s header32 size fields bogus "
            "filesize is %lu,"
            "number of commands is %lu,"
            "size of commands is %lu. \n",
            dwarf_get_errname(DW_DLE_MACHO_CORRUPT_HEADER),
            (unsigned long)mfp->mo_filesize,
            (unsigned long)mfp->mo_header.ncmds,
            (unsigned long)mfp->mo_header.sizeofcmds);
        *errcode = DW_DLE_MACHO_CORRUPT_HEADER;
        return DW_DLV_ERROR;
    }

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
    Dwarf_Unsigned inner = mfp->mo_inner_offset;

    res = RRMOA(mfp->mo_fd,&mh64,inner,sizeof(mh64),
        inner+mfp->mo_filesize,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    /* Do not adjust endianness of magic, leave as-is. */
    ASNAR(memcpy,mfp->mo_header.magic,mh64.magic);
    /* We will print the swapped one. */
    ASNAR(mfp->mo_copy_word,mfp->mo_header.swappedmagic,mh64.magic);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.cputype,mh64.cputype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.cpusubtype,
        mh64.cpusubtype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.filetype,mh64.filetype);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.ncmds,mh64.ncmds);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.sizeofcmds,
        mh64.sizeofcmds);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.flags,mh64.flags);
    ASNAR(mfp->mo_copy_word,mfp->mo_header.reserved,mh64.reserved);
    if (mfp->mo_header.sizeofcmds >= mfp->mo_filesize ||
        mfp->mo_header.ncmds >= mfp->mo_filesize ||
        mfp->mo_header.ncmds >= mfp->mo_header.sizeofcmds)  {
        printf("ERROR: %s header32 size fields bogus "
            "filesize is %lu,"
            "number of commands is %lu,"
            "size of commands is %lu. \n",
            dwarf_get_errname(DW_DLE_MACHO_CORRUPT_HEADER),
            (unsigned long)mfp->mo_filesize,
            (unsigned long)mfp->mo_header.ncmds,
            (unsigned long)mfp->mo_header.sizeofcmds);
        *errcode = DW_DLE_MACHO_CORRUPT_HEADER;
        return DW_DLV_ERROR;
    }
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
    Dwarf_Unsigned inner = mfp->mo_inner_offset;

    if (segoffset > filesize ||
        mmp->cmdsize > filesize ||
        (mmp->cmdsize + segoffset) > filesize ) {
        printf("Reading command_content32 fails,"
            " segoffset %lu,"
            " commandsize %lu,"
            " filesize %lu\n",
            (unsigned long)segoffset,
            (unsigned long)mmp->cmdsize,
            (unsigned long)mfp->mo_filesize);
        *errcode = RO_ERR_LOADSEGOFFSETBAD;
        return DW_DLV_ERROR;
    }
    res = RRMOA(mfp->mo_fd,&sc,
        inner+segoffset,sizeof(sc),
        inner+filesize,errcode);
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
        printf("Reading command sections fails,"
            " fileoffset %lu, "
            " filesize %lu\n",
            (unsigned long)msp->fileoff,
            (unsigned long)mfp->mo_filesize);
        *errcode = RO_ERR_FILEOFFSETBAD;
        return DW_DLV_ERROR;
    }
    if ((msp->fileoff+msp->filesize ) > filesize) {
        /* corrupt */
        printf("Reading command32 sections fails,"
            " fileoffset of end of sec %lu, "
            " filesize %lu\n",
            (unsigned long)(msp->fileoff+msp->filesize ),
            (unsigned long)mfp->mo_filesize);
        *errcode = RO_ERR_FILEOFFSETBAD;
        return DW_DLV_ERROR;
    }
    ASNAR(mfp->mo_copy_word,msp->maxprot,sc.maxprot);
    ASNAR(mfp->mo_copy_word,msp->initprot,sc.initprot);
    ASNAR(mfp->mo_copy_word,msp->nsects,sc.nsects);
    if (msp->nsects >= mfp->mo_filesize) {
        printf("Reading command32 sections fails,"
            " mmp_offset %lu,"
            " number of sections %lu,"
            " filesize %lu\n",
            (unsigned long)segoffset,
            (unsigned long)msp->nsects,
            (unsigned long)mfp->mo_filesize);
        *errcode  = RO_ERR_FILEOFFSETBAD;
        return DW_DLV_ERROR;
    }
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
    Dwarf_Unsigned inner = mfp->mo_inner_offset;

    if (segoffset > filesize ||
        mmp->cmdsize > filesize ||
        (mmp->cmdsize + segoffset) > filesize ) {
        printf("Reading command_content64 fails,"
            " segoffset %lu,"
            " commandsize %lu,"
            " filesize %lu\n",
            (unsigned long)segoffset,
            (unsigned long)mmp->cmdsize,
            (unsigned long)mfp->mo_filesize);
        *errcode = RO_ERR_FILEOFFSETBAD;
        return DW_DLV_ERROR;
    }
    res = RRMOA(mfp->mo_fd,&sc,
        inner+segoffset,sizeof(sc),
        inner+mfp->mo_filesize,errcode);
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
        printf("Reading command 64 sections fails,"
            " fileoffset %lu, "
            " filesize %lu\n",
            (unsigned long)msp->fileoff,
            (unsigned long)mfp->mo_filesize);
        *errcode = RO_ERR_FILEOFFSETBAD;
        return DW_DLV_ERROR;
    }
    if ((msp->fileoff+msp->filesize ) > filesize) {
        /* corrupt */
        printf("Reading command64 sections fails,"
            " fileoffset of end of sec %lu, "
            " filesize %lu\n",
            (unsigned long)(msp->fileoff+msp->filesize ),
            (unsigned long)mfp->mo_filesize);
        *errcode = RO_ERR_FILEOFFSETBAD;
        return DW_DLV_ERROR;
    }
    ASNAR(mfp->mo_copy_word,msp->maxprot,sc.maxprot);
    ASNAR(mfp->mo_copy_word,msp->initprot,sc.initprot);
    ASNAR(mfp->mo_copy_word,msp->nsects,sc.nsects);
    if (msp->nsects >= mfp->mo_filesize) {
        printf("Reading command64 sections fails,"
            " mmp_offset %lu,"
            " number of sections %lu,"
            " filesize %lu\n",
            (unsigned long)segoffset,
            (unsigned long)msp->nsects,
            (unsigned long)mfp->mo_filesize);
        *errcode  = RO_ERR_FILEOFFSETBAD;
        return DW_DLV_ERROR;
    }
    ASNAR(mfp->mo_copy_word,msp->flags,sc.flags);
    msp->macho_command_index = mmpindex;
    msp->sectionsoffset = afterseghdr;
    return DW_DLV_OK;
}

static int
dwarf_macho_load_segment_commands(struct macho_filedata_s *mfp,
    int *errcode)
{
    Dwarf_Unsigned i = 0;
    struct generic_macho_command *mmp = 0;
    struct generic_macho_segment_command *msp = 0;

    if (mfp->mo_segment_count < 1) {
        return DW_DLV_OK;
    }
    mfp->mo_segment_commands =
        (struct generic_macho_segment_command *)
        calloc( mfp->mo_segment_count,
            sizeof(struct generic_macho_segment_command));
    if (!mfp->mo_segment_commands) {
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }

    mmp = mfp->mo_commands;
    msp = mfp->mo_segment_commands;
    for (i = 0 ; i < mfp->mo_command_count; ++i,++mmp) {
        unsigned cmd = mmp->cmd;
        int res = 0;

        if (cmd == LC_SEGMENT) {
            res = load_segment_command_content32(mfp,mmp,msp,i,
                errcode);
            ++msp;
        } else if (cmd == LC_SEGMENT_64) {
            res = load_segment_command_content64(mfp,mmp,msp,i,
                errcode);
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
    Dwarf_Unsigned curoff = segp->sectionsoffset; /* no inner here*/
    Dwarf_Unsigned shdrlen = sizeof(struct section);

    struct generic_macho_section *secs = 0;

    secs = (struct generic_macho_section *)calloc(
        secalloc,
        sizeof(struct generic_macho_section));
    if (!secs) {
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
        Dwarf_Unsigned endoffset = 0;
        Dwarf_Unsigned inner = mfp->mo_inner_offset;

        endoffset = curoff + sizeof(mosec);
        if (curoff >=  mfp->mo_filesize ||
            endoffset > mfp->mo_filesize) {
            printf("Reading sections fails,"
                " fileoffset %lu,"
                " end-read offset %lu,"
                " filesize %lu\n",
                (unsigned long)curoff,
                (unsigned long)endoffset,
                (unsigned long)mfp->mo_filesize);
            *errcode  = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        res = RRMOA(mfp->mo_fd,&mosec,
            inner+curoff,sizeof(mosec),
            inner+mfp->mo_filesize,errcode);
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
            printf("Reading sections32 fails,"
                " fileoffset %lu,"
                " size %lu,"
                " filesize %lu\n",
                (unsigned long)curoff,
                (unsigned long)secs->size,
                (unsigned long)mfp->mo_filesize);
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
    Dwarf_Unsigned curoff = segp->sectionsoffset; /* no inner here*/
    Dwarf_Unsigned shdrlen = sizeof(struct section_64);
    struct generic_macho_section *secs = 0;

    secs = (struct generic_macho_section *)
        calloc(secalloc,
        sizeof(struct generic_macho_section));
    if (!secs) {
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
        Dwarf_Unsigned endoffset = 0;
        Dwarf_Unsigned inner = mfp->mo_inner_offset;

        endoffset = curoff + sizeof(mosec);
        if (curoff >=  mfp->mo_filesize ||
            endoffset > mfp->mo_filesize) {
            printf("Reading sections fails,"
                " fileoffset %lu,"
                " end-read offset %lu,"
                " filesize %lu\n",
                (unsigned long)curoff,
                (unsigned long)endoffset,
                (unsigned long)mfp->mo_filesize);
            *errcode  = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        res = RRMOA(mfp->mo_fd,&mosec,inner+curoff,sizeof(mosec),
            inner+mfp->mo_filesize,errcode);
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
            printf("Reading sections64 fails,"
                " fileoffset %lu,"
                " size %lu,"
                " filesize %lu\n",
                (unsigned long)curoff,
                (unsigned long)secs->size,
                (unsigned long)mfp->mo_filesize);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
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

    struct generic_macho_segment_command *segp =
        mfp->mo_segment_commands;
    for ( ; segi < mfp->mo_segment_count; ++segi,++segp) {
        int res = 0;

        /* Check for __DWARF in DSYM, not other file types. */
        if (mfp->mo_header.filetype == MH_DSYM &&
            strcmp(segp->segname,"__DWARF")) {
            continue;
        }

        res = dwarf_macho_load_dwarf_section_details(mfp,segp,
            segi,errcode);
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
    Dwarf_Unsigned inner = mfp->mo_inner_offset;

    if ((curoff + mfp->mo_command_count * sizeof(mc)) >=
        mfp->mo_filesize) {
        /* corrupt object. */
        *errcode = RO_ERR_LOADSEGOFFSETBAD;
        return DW_DLV_ERROR;
    }

    mfp->mo_commands = (struct generic_macho_command *) calloc(
        mfp->mo_command_count,sizeof(struct generic_macho_command));
    if (!mfp->mo_commands) {
        /* out of memory */
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    mcp = mfp->mo_commands;
    for ( ; cmdi < mfp->mo_header.ncmds; ++cmdi,++mcp ) {
        res = RRMOA(mfp->mo_fd,&mc,inner+curoff,sizeof(mc),
            inner+mfp->mo_filesize,errcode);
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

static int 
fill_in_uni_arch_32(
    struct fat_arch * fa, 
    struct Dwarf_Universal_Head_s *duhd,
    void *(*word_swap) (void *, const void *, size_t),
    int * errcode)
{
    Dwarf_Unsigned i = 0;
    struct Dwarf_Universal_Arch_s * dua = 0;

    dua = duhd->au_arches;
    for( ; i < duhd->au_count; ++i,++fa,++dua) {
        ASNAR(word_swap,dua->au_cputype,fa->cputype);
        ASNAR(word_swap,dua->au_cpusubtype,fa->cpusubtype);
        ASNAR(word_swap,dua->au_offset,fa->offset);
        if (dua->au_offset >= duhd->au_filesize) {
            printf(" Reading Univ Binary item %lu "
                "the offset value is "
                LONGESTXFMT
                ", impossible\n",
                (unsigned long)i,dua->au_offset);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        ASNAR(word_swap,dua->au_size,fa->size);
        if (dua->au_size >= duhd->au_filesize) {
            printf(" Reading Univ Binary item %lu "
                "the size value is "
                LONGESTXFMT
                ", impossible\n",
                (unsigned long)i,dua->au_size);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        if ((dua->au_size+dua->au_offset) > duhd->au_filesize) {
            printf(" Reading Univ Binary item %lu "
                "the size+offset value is "
                LONGESTXFMT
                ", impossible\n",
                (unsigned long)i,dua->au_size+dua->au_offset);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        ASNAR(word_swap,dua->au_align,fa->align);
        if (dua->au_align >= 32) {
            printf(" Reading Univ Binary item %lu "
                "the align value is "
                LONGESTXFMT
                ", impossible\n",
                (unsigned long)i,dua->au_align);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        dua->au_reserved = 0;
    }

    return DW_DLV_OK;
}

static int
fill_in_uni_arch_64(
    struct fat_arch_64 * fa, 
    struct Dwarf_Universal_Head_s *duhd,
    void *(*word_swap) (void *, const void *, size_t),
    int * errcode)
{
    Dwarf_Unsigned i = 0;
    struct Dwarf_Universal_Arch_s * dua = 0;

    dua = duhd->au_arches;
    for( ; i < duhd->au_count; ++i,++fa,++dua) {
        ASNAR(word_swap,dua->au_cputype,fa->cputype);
        ASNAR(word_swap,dua->au_cpusubtype,fa->cpusubtype);
        ASNAR(word_swap,dua->au_offset,fa->offset);
        if (dua->au_offset >= duhd->au_filesize) {
            printf(" Reading Univ Binary item %lu "
                "the offset value is "
                LONGESTXFMT
                ", impossible\n",
                (unsigned long)i,dua->au_offset);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        ASNAR(word_swap,dua->au_size,fa->size);
        if (dua->au_size >= duhd->au_filesize) {
            printf(" Reading Univ Binary item %lu "
                "the size value is "
                LONGESTXFMT
                ", impossible\n",
                (unsigned long)i,dua->au_size);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        if ((dua->au_size+dua->au_offset) > duhd->au_filesize) {
            printf(" Reading Univ Binary item %lu "
                "the size+offset value is "
                LONGESTXFMT
                ", impossible\n",
                (unsigned long)i,dua->au_size+dua->au_offset);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        ASNAR(word_swap,dua->au_align,fa->align);
        if (dua->au_align >= 64) {
            printf(" Reading Univ Binary item %lu "
                "the align value is "
                LONGESTXFMT
                ", impossible\n",
                (unsigned long)i,dua->au_align);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        ASNAR(word_swap,dua->au_reserved,fa->reserved);
    }

    return DW_DLV_OK;
}

static const struct Dwarf_Universal_Head_s duhzero; 
static const struct fat_header fhzero; 
int dwarf_object_detector_universal_head(
    char         *dw_path, 
    Dwarf_Unsigned      dw_filesize,
    unsigned int     *dw_contentcount, 
    Dwarf_Universal_Head * dw_head,
    int            *errcode)
{
    int fd = -1;
    int res = 0;

    fd = open(dw_path, O_RDONLY|O_BINARY);
    if (fd < 0) {
        printf("Opening path %s for universal binary failed\n",
           dw_path);
        *errcode = RO_ERR_PATH_SIZE;
        return DW_DLV_ERROR;
    }
    res = dwarf_object_detector_universal_head_fd(fd,
        dw_filesize, dw_contentcount, dw_head, errcode);
    close(fd);
    return res;
}
static int 
dwarf_object_detector_universal_head_fd(
    int fd,
    Dwarf_Unsigned      dw_filesize,
    unsigned int      *dw_contentcount, 
    Dwarf_Universal_Head * dw_head,
    int                *errcode)
{
    struct Dwarf_Universal_Head_s  duhd;
    struct Dwarf_Universal_Head_s *duhdp = 0;
    struct  fat_header fh;
    int     res = 0;
    void *(*word_swap) (void *, const void *, size_t);
    int     locendian = 0;
    int     locoffsetsize = 0;

    duhd = duhzero;
    fh = fhzero;
    /*  A universal head is always at offset zero. */
    res = RRMOA(fd,&fh,0,sizeof(fh), dw_filesize,errcode);
    if (res != DW_DLV_OK) {
        printf("Reading struct for universal binary "
            "header failed\n");
        return res;
    }
    duhd.au_filesize = dw_filesize;
    duhd.au_magic = magic_copy((unsigned char *)&fh.magic[0],4);
    if (duhd.au_magic == FAT_MAGIC) {
        locendian = DW_ENDIAN_BIG;
        locoffsetsize = 32;
    } else if (duhd.au_magic == FAT_CIGAM) {
        locendian = DW_ENDIAN_LITTLE;
        locoffsetsize = 32;
    }else if (duhd.au_magic == FAT_MAGIC_64) {
        locendian = DW_ENDIAN_BIG;
        locoffsetsize = 64;
    } else if (duhd.au_magic == FAT_CIGAM_64) {
        locendian = DW_ENDIAN_LITTLE;
        locoffsetsize = 64;
    } else {
        printf("Reading magic number universal compare failed "
            "Inconsistent\n");
        *errcode = RO_ERR_FILE_WRONG_TYPE;
        return DW_DLV_ERROR;
    }
#ifdef WORDS_BIGENDIAN
    if (locendian == DW_ENDIAN_LITTLE 
        || locendian == DW_ENDIAN_OPPOSITE ) {
        word_swap = dwarf_ro_memcpy_swap_bytes;
    } else {
        word_swap = memcpy;
    }
#else  /* LITTLE ENDIAN */
    if (locendian == DW_ENDIAN_LITTLE 
        || locendian == DW_ENDIAN_SAME ) {
        word_swap = memcpy;
    } else {
        word_swap = dwarf_ro_memcpy_swap_bytes;
    }
#endif /* LITTLE- BIG-ENDIAN */


    ASNAR(word_swap,duhd.au_count,fh.nfat_arch);
    /*  The limit is a first-cut safe heuristic. */
    if (duhd.au_count >= (dw_filesize/2) ) {
        printf("Universal Binary header count impossible: "
           LONGESTUFMT "\n",
            duhd.au_count);
        *errcode = DW_DLE_UNIVERSAL_BINARY_ERROR ;
        return DW_DLV_ERROR;
    }
    duhd.au_arches = (struct  Dwarf_Universal_Arch_s*)
        calloc(duhd.au_count, sizeof(struct Dwarf_Universal_Arch_s));
    if (!duhd.au_arches) {
        printf("Universal Binary au_arches alloc fail line %d\n",
            __LINE__);
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    if (locoffsetsize == 32) {
        struct fat_arch * fa = 0;
        fa = (struct fat_arch *)calloc(duhd.au_count,
            sizeof(struct fat_arch));
        if (!fa) {
            printf("Universal Binary 32bit au_arches "
                "alloc fail line %d\n",
                __LINE__);
            *errcode = DW_DLE_ALLOC_FAIL;
            free(duhd.au_arches);
            duhd.au_arches = 0;
            free(fa);
            return res;
        }
        if ((sizeof(fh) +  duhd.au_count*sizeof(*fa)) >
            duhd.au_filesize) {
            printf("Undersized Mach-O 32 universal header!"
                " Corrupt object file.\n");
            *errcode = DW_DLE_UNIVERSAL_BINARY_ERROR;
            free(duhd.au_arches);
            duhd.au_arches = 0;
            free(fa);
            return DW_DLV_ERROR;
        }
        res = RRMOA(fd,fa,/*offset=*/sizeof(fh),
            duhd.au_count*sizeof(*fa),
            dw_filesize,errcode);
        if (res != DW_DLV_OK) {
            printf("Universal Binary value read fail line %d\n",
               __LINE__);
            free(duhd.au_arches);
            duhd.au_arches = 0;
            free(fa);
            return res;
        }
        res = fill_in_uni_arch_32(fa,&duhd,word_swap,
            errcode);
        if (res != DW_DLV_OK) {
            free(duhd.au_arches);
            printf("Universal Binary value read fail line %d\n",
               __LINE__);
            duhd.au_arches = 0;
            free(fa);
            return res;
        }
        free(fa);
        fa = 0;
    } else { /* 64 */
        struct fat_arch_64 * fa = 0;
        fa = (struct fat_arch_64 *)calloc(duhd.au_count,
            sizeof(struct fat_arch_64));
        if (!fa) {
            printf("Universal Binary au_arches alloc fail line %d\n",
               __LINE__);
            *errcode = DW_DLE_ALLOC_FAIL;
            free(duhd.au_arches);
            duhd.au_arches = 0;
            return res;
        }
        if ((sizeof(fh) +  duhd.au_count*sizeof(*fa)) >
            duhd.au_filesize) {
            printf("Undersized Mach-O 64 universal header!"
                " Corrupt object file.\n");
            *errcode = DW_DLE_UNIVERSAL_BINARY_ERROR;
            free(duhd.au_arches);
            duhd.au_arches = 0;
            free(fa);
            return DW_DLV_ERROR;
        }
        res = RRMOA(fd,fa,/*offset*/sizeof(fh),
            duhd.au_count*sizeof(fa),
            dw_filesize,errcode);
        if (res == DW_DLV_ERROR) {
            printf("Universal Binary value read fail line %d\n",
               __LINE__);
            free(duhd.au_arches);
            duhd.au_arches = 0;
            free(fa);
            return res;
        }
        res = fill_in_uni_arch_64(fa,&duhd,word_swap,
            errcode);
        if (res != DW_DLV_OK) {
            printf("Universal Binary value read fail line %d\n",
               __LINE__);
            free(duhd.au_arches);
            duhd.au_arches = 0;
            free(fa);
            return res;
        }
        free(fa);
        fa = 0;
    }

    duhdp = malloc(sizeof(*duhdp));
    if (!duhdp) {
        printf("Universal Binary au_arches alloc fail line %d\n",
            __LINE__);
        free(duhd.au_arches);
        duhd.au_arches = 0;
        *errcode = DW_DLE_ALLOC_FAIL;
        return res;
    }
    memcpy(duhdp,&duhd,sizeof(duhd));
    *dw_contentcount = duhd.au_count;
    duhdp->au_arches = duhd.au_arches;
    *dw_head = duhdp;
    return res;
}

#if 0
static void
print_arch_item(unsigned int i,
    struct  Dwarf_Universal_Arch_s* arch)
{
    printf(" Universal Binary Index " LONGESTUFMT "\n",i);
    printf("   cpu     " LONGESTXFMT "\n",arch->au_cputype);
    printf("   cpusubt " LONGESTXFMT "\n",arch->au_cpusubtype);
    printf("   offset  " LONGESTXFMT "\n",arch->au_offset);
    printf("   size    " LONGESTXFMT "\n",arch->au_size);
    printf("   align   " LONGESTXFMT "\n",arch->au_align);
}
#endif

int dwarf_object_detector_universal_instance(
    Dwarf_Universal_Head dw_head,
    Dwarf_Unsigned  dw_index_of,
    Dwarf_Unsigned *dw_cpu_type,
    Dwarf_Unsigned *dw_cpusubtype,
    Dwarf_Unsigned *dw_offset,
    Dwarf_Unsigned *dw_size,
    Dwarf_Unsigned *dw_align,
    int         *errcode)
{
    struct  Dwarf_Universal_Arch_s* arch = 0;

    if (!dw_head) {
         printf("Missing argument to "
            "dwarf_object_detector_universal_instance");
         *errcode = DW_DLE_UNIVERSAL_BINARY_ERROR;
         return DW_DLV_ERROR;
    }
    if (dw_index_of >= dw_head->au_count){
         printf("Requested index "
            LONGESTUFMT
            " to specific binary "
            "is too larg: valid: 0 to "
            LONGESTUFMT "-1\n",
            dw_index_of, dw_head->au_count);
         return DW_DLV_NO_ENTRY;
    }
    arch =  dw_head->au_arches +dw_index_of;
    *dw_cpu_type = arch->au_cputype;
    *dw_cpusubtype = arch->au_cpusubtype; 
    *dw_offset = arch->au_offset;
    *dw_size = arch->au_size;
    *dw_align = arch->au_align;
#if 0
    print_arch_item(dw_index_of,arch);
#endif
    return DW_DLV_OK;
}

void dwarf_dealloc_universal_head(Dwarf_Universal_Head dw_head)
{
    if (!dw_head) {
        return;
    }
    free(dw_head->au_arches);
    dw_head->au_arches = 0;
    free(dw_head);
}

