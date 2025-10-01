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
#include "dwarf_macho_loader.h"
#include "dwarf_machoread.h"
#include "dwarf_object_detector.h"
#include "dwarf_macho_loader.h"
#include "dwarf_safe_arithmetic.h"

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

/* load_macho_header64(struct macho_filedata_s *mfp) */
int
_dwarf_load_macho_header64(struct macho_filedata_s *mfp,int *errcode)
{
    struct mach_header_64 mh64;
    int res = 0;
    Dwarf_Unsigned inner = mfp->mo_inner_offset;
    Dwarf_Unsigned totalcmds = 0;

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
    res  = _dwarf_uint64_mult(mfp->mo_header.ncmds,
        mfp->mo_header.sizeofcmds,&totalcmds);
    if (res == DW_DLV_ERROR) {
        printf("ERROR: header 64 cmd count*cmdsize "
            "overflows.Corrupt\n");
        *errcode = DW_DLE_MACHO_CORRUPT_HEADER;
        return DW_DLV_ERROR;
    }
    printf("  Total Commands count      : %lu\n",
        (unsigned long)mfp->mo_header.ncmds);
    printf("  Total Commands item size  : %lu\n",
        (unsigned long)mfp->mo_header.sizeofcmds);
    printf("  Total Commands space bytes: %lu\n",
        (unsigned long)totalcmds);
    if (totalcmds > MAX_COMMANDS_SIZE) {
        printf("ERROR: header 64 cmd count*cmdsize "
            "(%lu)"
            " exceeds limit (%lu).Corrupt\n",
            (unsigned long)totalcmds,
            (unsigned long)MAX_COMMANDS_SIZE);
        *errcode = DW_DLE_MACHO_CORRUPT_HEADER;
        return DW_DLV_ERROR;
    }
    if (totalcmds > mfp->mo_filesize ) {
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

int
_dwarf_load_segment_command_content64(struct macho_filedata_s *mfp,
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
    if (!_dwarf_is_known_segname(msp->segname)) {
        printf("Reading command segment 32: ,"
            "the segment name (%s) is Unknown. Corrupt\n",
            msp->segname);
        *errcode = DW_DLE_MACHO_CORRUPT_SEGMENT_NAME;
        return DW_DLV_ERROR;
    }
    ASNAR(mfp->mo_copy_word,msp->vmaddr,sc.vmaddr);
    ASNAR(mfp->mo_copy_word,msp->vmsize,sc.vmsize);
    ASNAR(mfp->mo_copy_word,msp->fileoff,sc.fileoff);
    ASNAR(mfp->mo_copy_word,msp->filesize,sc.filesize);
    if (msp->fileoff > filesize ||
        msp->filesize > filesize) {
        /* corrupt */
        printf("Reading command 64 segments fails,"
            " fileoffset %lu, "
            " filesize %lu\n",
            (unsigned long)msp->fileoff,
            (unsigned long)mfp->mo_filesize);
        *errcode = RO_ERR_FILEOFFSETBAD;
        return DW_DLV_ERROR;
    }
    if ((msp->fileoff+msp->filesize ) > filesize) {
        /* corrupt */
        printf("Reading command64 segment fails,"
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

int
_dwarf_macho_load_dwarf_section_details64(
    struct macho_filedata_s *mfp,
    struct generic_macho_segment_command *segp,
    Dwarf_Unsigned segi,
    int *errcode)
{
    Dwarf_Unsigned seci = 0;
    Dwarf_Unsigned seccount = segp->nsects;
    Dwarf_Unsigned secalloc = seccount+1;
    Dwarf_Unsigned curoff = segp->sectionsoffset; /* no inner here*/
    Dwarf_Unsigned shdrlen = sizeof(struct section_64);
    Dwarf_Unsigned newcount = 0;
    struct generic_macho_section *secs = 0;

    if (mfp->mo_dwarf_sections &&  !seccount) {
        printf("section_details64() section pointer "
            "and no sections. Odd\n");
        return DW_DLV_OK;
    }

    if (mfp->mo_dwarf_sections) {
        struct generic_macho_section * originalsections =
            mfp->mo_dwarf_sections;
        newcount = mfp->mo_dwarf_sectioncount + seccount;
        secs = (struct generic_macho_section *)calloc(
            newcount,
            sizeof(struct generic_macho_section));
        if (!secs) {
            *errcode = RO_ERR_MALLOC;
            return DW_DLV_OK;
        }
        memcpy(secs,mfp->mo_dwarf_sections,
            mfp->mo_dwarf_sectioncount *
            sizeof(struct generic_macho_section));
        mfp->mo_dwarf_sections = secs;
        seci =  mfp->mo_dwarf_sectioncount ;
        mfp->mo_dwarf_sectioncount = newcount;
        free(originalsections);
        secs += seci;
        secs->offset_of_sec_rec = curoff;
        secalloc = newcount;
    } else {
        secs = (struct generic_macho_section *)calloc(
            secalloc,
            sizeof(struct generic_macho_section));
        if (!secs) {
            *errcode = RO_ERR_MALLOC;
            return DW_DLV_OK;
        }
        newcount = secalloc;
        mfp->mo_dwarf_sections = secs;
        mfp->mo_dwarf_sectioncount = secalloc;
        if (!secalloc) {
            printf("section_details64() section count zero "
                "Impossible\n");
            *errcode = RO_ERR_MALLOC;
            return DW_DLV_ERROR;
        }
        secs->offset_of_sec_rec = curoff;
        /*  Leave 0 section all zeros except our offset,
        elf-like in a sense */
        secs->dwarfsectname = "";
        seci = 1;
        ++secs;
    }
    for (; seci < secalloc; ++seci,++secs,curoff += shdrlen ) {
        int res = 0;
        struct section_64 mosec;
        Dwarf_Unsigned endoffset = 0;
        Dwarf_Unsigned inner = mfp->mo_inner_offset;
        Dwarf_Unsigned offplussize = 0;
        Dwarf_Unsigned innercur = 0;

        res = _dwarf_uint64_add(curoff,sizeof(mosec),
            &endoffset);
        if (res == DW_DLV_ERROR) {
            printf("Reading sections fails,"
                "overflow adding offset (%lu) and "
                "sec-header-size %lu\n",
                (unsigned long)curoff,
                (unsigned long)sizeof(mosec));
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        if (curoff >=  mfp->mo_filesize ||
            endoffset > mfp->mo_filesize) {
            printf("Reading sections fails,"
                " fileoffset %lu,"
                " end-read offset %lu,"
                " filesize %lu\n",
                (unsigned long)curoff,
                (unsigned long)endoffset,
                (unsigned long)mfp->mo_filesize);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        res = _dwarf_uint64_add(inner,curoff,&innercur);
        if (res == DW_DLV_ERROR) {
            printf(" Error: overflow in section inner "
                "offset/curroff sum. Corrupt\n");
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        res = _dwarf_uint64_add(inner,mfp->mo_filesize,
            &offplussize);
        if (res == DW_DLV_ERROR) {
            printf(" Error: overflow in section %lu "
                "inneroffset (%lu) +filesize (%lu) sum. Corrupt\n",
                (unsigned long)seci,
                (unsigned long)inner,
                (unsigned long)mfp->mo_filesize);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        res = RRMOA(mfp->mo_fd, &mosec,
            innercur, sizeof(mosec),
            offplussize, errcode);
        if (res != DW_DLV_OK) {
            return res;
        }
        strncpy(secs->sectname,mosec.sectname,16);
        secs->sectname[16] = 0;
        if (_dwarf_not_ascii(secs->sectname)) {
            printf("A section name (%s) is not simple"
                " ascii characters. Corrupt DWARF.\n",
                secs->sectname);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }
        strncpy(secs->segname,mosec.segname,16);
        secs->segname[16] = 0;
        if (!_dwarf_is_known_segname(secs->segname)) {
            printf("Reading section details 64 fails,"
                "the segment name (%s) is unknown\n",
                secs->segname);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return DW_DLV_ERROR;
        }

        ASNAR(mfp->mo_copy_word,secs->addr,mosec.addr);
        ASNAR(mfp->mo_copy_word,secs->size,mosec.size);
        ASNAR(mfp->mo_copy_word,secs->offset,mosec.offset);
        ASNAR(mfp->mo_copy_word,secs->align,mosec.align);
        ASNAR(mfp->mo_copy_word,secs->reloff,mosec.reloff);
        ASNAR(mfp->mo_copy_word,secs->nreloc,mosec.nreloc);
        ASNAR(mfp->mo_copy_word,secs->flags,mosec.flags);
        if (0 == strcmp(secs->segname,"__DWARF")) {
            if (offplussize < secs->offset ||
                offplussize < secs->size){
                /* overflow in add */
                printf(" Error: overflow in section %s "
                    "offset (%lu)+sec size(%lu)  sum. Corrupt\n",
                    secs->sectname,
                    (unsigned long)secs->offset,
                    (unsigned long)secs->size);
                *errcode = RO_ERR_FILEOFFSETBAD;
                return DW_DLV_ERROR;
            }
        }

        /*  One section size apparently refers to executable,
            not here, so do not
            check here Same for segment __DATA */
        offplussize = secs->offset+secs->size;
        if (offplussize < secs->offset || offplussize< secs->size){
            /* overflow in add */
            return DW_DLV_ERROR;
        }
        if (0 == strcmp(secs->segname,"___DWARF"))  {
            if (secs->offset > mfp->mo_filesize ||
                secs->size > mfp->mo_filesize ||
                offplussize > mfp->mo_filesize) {
                printf("Reading sections64 fails,"
                    " fileoffset 0x%lx,"
                    " size 0x%lx,"
                    " filesize 0x%lx\n",
                    (unsigned long)curoff,
                    (unsigned long)secs->size,
                    (unsigned long)mfp->mo_filesize);
                *errcode = RO_ERR_FILEOFFSETBAD;
                return DW_DLV_ERROR;
            }
        }
        secs->reserved1 = 0;
        secs->reserved2 = 0;
        secs->reserved3 = 0;
        secs->offset_of_sec_rec = curoff;
        secs->generic_segment_num  = segi;
    }
    return DW_DLV_OK;
}

int
_dwarf_fill_in_uni_arch_64(
    struct fat_arch_64 * fa,
    struct Dwarf_Universal_Head_s *duhd,
    void *(*word_swap) (void *, const void *, size_t),
    int * errcode)
{
    Dwarf_Unsigned i = 0;
    struct Dwarf_Universal_Arch_s * dua = 0;

    dua = duhd->au_arches;
    for ( ; i < duhd->au_count; ++i,++fa,++dua) {
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
