/*
Copyright 2026 David Anderson. All rights reserved.

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

#include "config.h"
#include <stdio.h>
#include <string.h> /* For memcpy etc */
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for exit(), C89 malloc */
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_MALLOC_H
/* Useful include for some Windows compilers. */
#include <malloc.h>
#endif /* HAVE_MALLOC_H */
#include <sys/types.h>   /* for open() */
#include <sys/stat.h>   /* for open() */
#include <fcntl.h>   /* for open() */
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_ELF_H
#include <elf.h>
#endif /* HAVE_ELF_H */
#ifdef HAVE_ZLIB_H
#include "zlib.h"
#endif /* ZLIB */
#ifdef HAVE_ZSTD_H
#include "zstd.h"
#endif /* ZSTD */
#include "dwarf_types.h"
#include "dw_elfstructs.h"
#include "dwarf_reading.h"
#include "dwarf_universal.h"
#include "dwarf_object_detector.h"
#include "dwarf_object_read_common.h"
#include "readelfobj.h"
#include "dwarf_elf_decompress.h"
#include "sanitized.h"
#include "common_options.h"

#ifdef HAVE_UNUSED_ATTRIBUTE
#define  UNUSEDARG __attribute__ ((unused))
#else
#define  UNUSEDARG
#endif

#if 0 /* dump_bytes */
static void
dump_bytes(char * msg,Dwarf_Small * start, long len)
{
    Dwarf_Small *end = start + len;
    Dwarf_Small *cur = start;

    printf("%s ",msg);
    for (; cur < end; cur++) {
        printf("%02x ", *cur);
    }
    printf("\n");
}
#endif /* 0 */

#if defined(HAVE_ZLIB) && defined(HAVE_ZSTD)
/*  For decompressing any section.
    Knows nothing about what the compressed bytes
    mean.

    PRECONDITION:
    This assumes 
    psh->gh_content has the compressed  bytes,
    psh->gh_size is the size of the compressed section, and
    psh->gh_flags has the SHF_COMPRESSED bit set.

    POSTCONDITION:
    The original psh->gh_content was freed and replaced by
    new malloc space which now contains the decompressed data.
    psh->gh_size is set to the size of the decompressed data.
*/
int
dwarf_elf_do_decompress(elf_filedata ep,
    struct generic_shdr *psh,
    int* error)
{
    Dwarf_Small *dest = 0;
    Dwarf_Small *src = psh->gh_content;
    Dwarf_Unsigned destlen = 0;
    Dwarf_Unsigned srclen = psh->gh_size;
    Dwarf_Unsigned flags = psh->gh_flags;
    Dwarf_Small *endsection = 0;
    int zstdcompress = FALSE;
    Dwarf_Unsigned uncompressed_len = 0;
#if 0
    Dwarf_Unsigned compressed_length = srclen;
    int is_shstrings = FALSE;
    const char *namestring = psh->gh_namestring;
#endif
    printf("Decompress section %ld\n",
        (unsigned long)psh->gh_secnum);F;

    endsection = src + srclen;
    if ((src + 12) > endsection) {
        *error = DW_DLE_ZLIB_SECTION_SHORT;
        P(" DW_DLE_ZLIB_SECTION_SHORT"
            "The section is too short to be zlib or zstd related");
#if 0
        /*_dwarf_error_string(dbg, error,DW_DLE_ZLIB_SECTION_SHORT,
            "DW_DLE_ZLIB_SECTION_SHORT"
            "Section too short to be either zlib or zstd related"); */
#endif /* 0 */
        return DW_DLV_ERROR;
    }
#if 0
    if (psh->gh_secnum == ep->f_elf_shstrings_secnumber) {
        is_shstrings = TRUE;
    }
#endif
#if 0
dump_bytes("Looking for ZLIB ",src,4);F;
#endif
    if (!strncmp("ZLIB",(const char *)src,4)) {
        /*  Checking if section content (not
        section name) starts with ZLIB */
        unsigned i = 0;
        unsigned l = 8;
        unsigned char *c = src+4;
#if 0
printf("dadebug entry ZLIB\n"); F;
#endif
        for ( ; i < l; ++i,c++) {
            uncompressed_len <<= 8;
            uncompressed_len += *c;
        }
        src = src + 12;
        srclen -= 12;
#if 0
        section->dss_ZLIB_compressed = TRUE;
#endif
    } else  if (flags & SHF_COMPRESSED) {
        /*  The prefix is a struct:
            unsigned int type; followed by pad if following are 64bit!
            size-of-target-address size
            size-of-target-address
        */
        Dwarf_Small *ptr    = (Dwarf_Small *)src;
        Dwarf_Unsigned type = 0;
        Dwarf_Unsigned size = 0;
        /* Dwarf_Unsigned addralign = 0; */
        unsigned fldsize    = ep->f_pointersize/8;
        unsigned structsize = 3* fldsize;

#if 0
printf("dadebug entry SHF_COMPRESSED fldsize %u\n",fldsize); F;
dump_bytes("SHF_COMPRESSED bytes",ptr,24);
#endif
        ASNARLRAW(ep->f_copy_word,error,type,ptr,DWARF_32BIT_SIZE);
        if (*error) {
            return DW_DLV_ERROR;
        }
        ptr += fldsize;
        ASNARLRAW(ep->f_copy_word,error,size,ptr,fldsize);
        if (*error) {
            return DW_DLV_ERROR;
        }
        switch(type) {
        case ELFCOMPRESS_ZLIB:
            P(" ELFCOMPRESS_ZLIB: uncompressed size %lu\n",
                (unsigned long)size);
            break;
        case ELFCOMPRESS_ZSTD:
            zstdcompress = TRUE;
            P(" ELFCOMPRESS_ZSTD: uncompressed size %lu\n",
                (unsigned long)size);
            break;
        default: {
            P("DW_DLE_ZDEBUG_INPUT_FORMAT_ODD "
                "  The SHF_COMPRESSED type field is 0x%lx, neither"
                " zlib (1) or zstd(2). Corrupt dwarf.",
                (unsigned long)type);
            *error = DW_DLE_ZDEBUG_INPUT_FORMAT_ODD;
            return DW_DLV_ERROR;
#if 0
            char buf[100];
            dwarfstring m;

            dwarfstring_constructor_static(&m,buf,sizeof(buf));
            dwarfstring_append_printf_u(&m,
                "DW_DLE_ZDEBUG_INPUT_FORMAT_ODD"
                " The SHF_COMPRESSED type field is 0x%lx, neither"
                " zlib (1) or zstd(2). Corrupt dwarf.",type);
            _dwarf_error_string(dbg, error,
                DW_DLE_ZDEBUG_INPUT_FORMAT_ODD,
                dwarfstring_string(&m));
            dwarfstring_destructor(&m);
            return DW_DLV_ERROR;
#endif
        }
        }
        uncompressed_len = size;

#if 0
        section->dss_uncompressed_length = uncompressed_len;
#endif
        src    += structsize;
        srclen -= structsize;
#if 0
        section->dss_shf_compressed = TRUE;
#endif
    } else {
        P(" DW_DLE_ZDEBUG_INPUT_FORMAT_ODD the compressed section "
            " is somehow not properly formatted");
        *error = DW_DLE_ZDEBUG_INPUT_FORMAT_ODD;
        return DW_DLV_ERROR;
#if 0
        _dwarf_error_string(dbg, error,
            DW_DLE_ZDEBUG_INPUT_FORMAT_ODD,
            "DW_DLE_ZDEBUG_INPUT_FORMAT_ODD"
            " The compressed section is not properly formatted");
#endif
    }
    /*  Dropped heuristic of excess compress inflation.
        Not reliable. */
    if ((src +srclen) > endsection) {
        P("DW_DLE_ZLIB_SECTION_SHORT "
            " The zstd or zlib compressed section  is"
            " longer than the section"
            " length. So corrupt Elf");
#if 0
        _dwarf_error_string(dbg, error,
            DW_DLE_ZLIB_SECTION_SHORT,
            "DW_DLE_ZDEBUG_ZLIB_SECTION_SHORT"
            " The zstd or zlib compressed section  is"
            " longer than the section"
            " length. So corrupt Elf");
#endif
        return DW_DLV_ERROR;
    }
    destlen = uncompressed_len;
    dest = malloc(destlen);
#if 0
    printf("dadebug uncompressed len 0x%lx\n",
    (unsigned long)destlen); F;
#endif
    if (!dest) {
        P("DW_DLE_ALLOC_FAIL in allocating %lu bytes for"
            "decompressing a section\n",(unsigned long)destlen);
        return DW_DLV_ERROR;
    }
    /*  uncompress is a zlib function. */
    if (!zstdcompress) {
        int res = 0;
        uLongf dlen = destlen;

#if 0
printf("dadebug uncompress (zlib) now\n"); F;
#endif
        res = uncompress(dest,&dlen,src,srclen);
        if (res == Z_BUF_ERROR) {
            free(dest);
            P(" DW_DLE_ZLIB_BUF_ERROR, uncompress failed \n");
            *error = DW_DLE_ZLIB_BUF_ERROR;
            return DW_DLV_ERROR;
        } else if (res == Z_MEM_ERROR) {
            free(dest);
            P(" DW_DLE_ALLOC_FAIL, uncompress failed \n");
            *error = DW_DLE_ALLOC_FAIL;
            return DW_DLV_ERROR;
        } else if (res != Z_OK) {
            /* Probably Z_DATA_ERROR. */
            free(dest);
            P(" DW_DLE_ZLIB_DATA_ERROR, uncompress failed \n");
            *error = DW_DLE_ZLIB_DATA_ERROR;
            return DW_DLV_ERROR;
        }
    } else {
        size_t zsize = 0;
#if 0
printf("dadebug uncompress (zstd) now\n"); F;
#endif
        zsize = ZSTD_decompress(dest,destlen,src,srclen);
        if (zsize != destlen) {
            free(dest);
            *error = DW_DLE_ZLIB_DATA_ERROR;
            return DW_DLV_ERROR;
        }
    }
    /* Z_OK */
    free(psh->gh_content);
    psh->gh_content = dest;
    psh->gh_size = destlen;
    psh->gh_compressed_len = srclen;
#if 0
    psh->gh_is_malloc = TRUE;
printf("dadebug done decompress\n"); F;
#endif
#if 0
    _dwarf_malloc_section_free(section);
    section->dss_data = dest;
    section->dss_size = destlen;
    section->dss_was_alloc= TRUE;
    section->dss_actual_load_type= Dwarf_Alloc_Malloc;
    section->dss_did_decompress = TRUE;
#endif
    return DW_DLV_OK;
}
#endif /*defined(HAVE_ZLIB) && defined(HAVE_ZSTD)*/
