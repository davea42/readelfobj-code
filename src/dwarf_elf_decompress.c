
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
#include "dwarf_types.h"
#include "dw_elfstructs.h"
#include "dwarf_reading.h"
#include "dwarf_universal.h"
#include "dwarf_object_detector.h"
#include "dwarf_object_read_common.h"
#include "readelfobj.h"
#include "sanitized.h"
#include "common_options.h"

#ifdef HAVE_UNUSED_ATTRIBUTE
#define  UNUSEDARG __attribute__ ((unused))
#else
#define  UNUSEDARG
#endif

#if defined(HAVE_ZLIB) && defined(HAVE_ZSTD)
/*  This is exclusively for reading .symtab and .symstr
    sections. See dwarf_elf_init() for decompressing all
    other sections. We need decompress to do relocations (if any
    relocations and if either of these sections compressed).  */
static int
dwarf_elf_do_decompress(dwarf_elf_object_access_internals_t *ep,
    struct generic_shdr *psh,
    int* error)
{
    Dwarf_Small *basesrc = psh->gh_content;
    Dwarf_Small *src = basesrc;
    Dwarf_Small *dest = 0;
    Dwarf_Unsigned destlen = 0;
    Dwarf_Unsigned srclen = psh->gh_size;
    Dwarf_Unsigned flags = psh->gh_flags;
    Dwarf_Small *endsection = 0;
    int zstdcompress = FALSE;
    Dwarf_Unsigned uncompressed_len = 0;
    Dwarf_Unsigned compressed_length = 0;

    endsection = basesrc + srclen;
    if ((basesrc + 12) > endsection) {
        *error = DW_DLE_ZLIB_SECTION_SHORT;
        /*_dwarf_error_string(dbg, error,DW_DLE_ZLIB_SECTION_SHORT,
            "DW_DLE_ZLIB_SECTION_SHORT"
            "Section too short to be either zlib or zstd related"); */
        return DW_DLV_ERROR;
    }
    compressed_length = srclen;
    if (!strncmp("ZLIB",(const char *)src,4)) {
        /*  This should be impossible */
        unsigned i = 0;
        unsigned l = 8;
        unsigned char *c = src+4;
        for ( ; i < l; ++i,c++) {
            uncompressed_len <<= 8;
            uncompressed_len += *c;
        }
        src = src + 12;
        srclen -= 12;
        section->dss_uncompressed_length = uncompressed_len;
        section->dss_ZLIB_compressed = TRUE;
        *error = DW_DLE_ZLIB_SECTION_SHORT;
        return DW_DLV_OK;
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
        unsigned fldsize    = ep->f_pointersize;
        unsigned structsize = 3* fldsize;
      FIXME 
#if 0
        READ_UNALIGNED_CK(dbg,type,Dwarf_Unsigned,ptr,
            DWARF_32BIT_SIZE,
            error,endsection);
        ptr += fldsize;
        READ_UNALIGNED_CK(dbg,size,Dwarf_Unsigned,ptr,fldsize,
            error,endsection);
#endif
        switch(type) {
        case ELFCOMPRESS_ZLIB:
            break;
        case ELFCOMPRESS_ZSTD:
            zstdcompress = TRUE;
            break;
        default: {
            char buf[100];
            dwarfstring m;

            dwarfstring_constructor_static(&m,buf,sizeof(buf));
            dwarfstring_append_printf_u(&m,
                "DW_DLE_ZDEBUG_INPUT_FORMAT_ODD"
                " The SHF_COMPRESSED type field is 0x%x, neither"
                " zlib (1) or zstd(2). Corrupt dwarf.", type);
            _dwarf_error_string(dbg, error,
                DW_DLE_ZDEBUG_INPUT_FORMAT_ODD,
                dwarfstring_string(&m));
            dwarfstring_destructor(&m);
            return DW_DLV_ERROR;
        }
        }
        uncompressed_len = size;
        section->dss_uncompressed_length = uncompressed_len;
        src    += structsize;
        srclen -= structsize;
        section->dss_shf_compressed = TRUE;
    } else {
        _dwarf_error_string(dbg, error,
            DW_DLE_ZDEBUG_INPUT_FORMAT_ODD,
            "DW_DLE_ZDEBUG_INPUT_FORMAT_ODD"
            " The compressed section is not properly formatted");
        return DW_DLV_ERROR;
    }
    /* Dropped heuristic of excess compress inflation. Not reliable. */
    if ((src +srclen) > endsection) {
        _dwarf_error_string(dbg, error,
            DW_DLE_ZLIB_SECTION_SHORT,
            "DW_DLE_ZDEBUG_ZLIB_SECTION_SHORT"
            " The zstd or zlib compressed section  is"
            " longer than the section"
            " length. So corrupt dwarf");
        return DW_DLV_ERROR;
    }
    destlen = uncompressed_len;
    dest = malloc(destlen);
    if (!dest) {
        _dwarf_error_string(dbg, error,
            DW_DLE_ALLOC_FAIL,
            "DW_DLE_ALLOC_FAIL"
            " The zstd or zlib uncompressed space"
            " malloc failed: out of memory");
        return DW_DLV_ERROR;
    }
    /*  uncompress is a zlib function. */
    if (!zstdcompress) {
        int res = 0;
        uLongf dlen = destlen;

        res = uncompress(dest,&dlen,src,srclen);
        if (res == Z_BUF_ERROR) {
            free(dest);
            DWARF_DBG_ERROR(dbg, DW_DLE_ZLIB_BUF_ERROR, DW_DLV_ERROR);
        } else if (res == Z_MEM_ERROR) {
            free(dest);
            DWARF_DBG_ERROR(dbg, DW_DLE_ALLOC_FAIL, DW_DLV_ERROR);
        } else if (res != Z_OK) {
            free(dest);
            /* Probably Z_DATA_ERROR. */
            DWARF_DBG_ERROR(dbg, DW_DLE_ZLIB_DATA_ERROR,
                DW_DLV_ERROR);
        }
    }
    if (zstdcompress) {
        size_t zsize =
            ZSTD_decompress(dest,destlen,src,srclen);
        if (zsize != destlen) {
            free(dest);
            _dwarf_error_string(dbg, error,
                DW_DLE_ZLIB_DATA_ERROR,
                "DW_DLE_ZLIB_DATA_ERROR"
                " The zstd ZSTD_decompress() failed.");
            return DW_DLV_ERROR;
        }
    }
    /* Z_OK */
    _dwarf_malloc_section_free(section);
    section->dss_data = dest;
    section->dss_size = destlen;
    section->dss_was_alloc= TRUE;
    section->dss_actual_load_type= Dwarf_Alloc_Malloc;
    section->dss_did_decompress = TRUE;
    return DW_DLV_OK;
}
#endif /*defined(HAVE_ZLIB) && defined(HAVE_ZSTD)*/

