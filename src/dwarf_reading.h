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
#ifndef DWARF_READING_H
#define DWARF_READING_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef DW_DLV_OK
/* DW_DLV_OK  must match RO_OK */
/* DW_DLV_NO_ENTRY  must match FIXME */
#define DW_DLV_OK 0
#define DW_DLV_NO_ENTRY -1
#define DW_DLV_ERROR 1
#endif /* DW_DLV_OK */

#if defined(_WIN32) && defined(HAVE_NONSTANDARD_PRINTF_64_FORMAT)
#define LONGESTXFMT  "0x%I64x"
#define LONGESTXFMT8 "0x%08I64x"
#define LONGESTUFMT  "%I64u"
#define LONGESTUFMT2  "%2I64u"
#define LONGESTUFMT8 "%08I64u"
#define LONGESTSFMT  "%I64d"
#define LONGESTUTYPE unsigned long long
#define LONGESTSTYPE long long
#else
#define LONGESTXFMT  "0x%llx"
#define LONGESTXFMT8 "0x%08llx"
#define LONGESTUFMT  "%llu"
#define LONGESTUFMT2  "%2llu"
#define LONGESTUFMT8 "%08llu"
#define LONGESTSFMT  "%lld"
#define LONGESTUTYPE unsigned long long
#define LONGESTSTYPE long long
#endif

#ifndef O_BINARY
/*  Will be non-zero on MinGW */
#define O_BINARY 0
#endif

#define DWARF_32BIT_SIZE 4
#define DWARF_64BIT_SIZE 8

#define TRUE 1
#define FALSE 0

#define ALIGN4 4
#define ALIGN8 8

#define PREFIX "\t"
#define LUFMT "%lu"
#define UFMT "%u"
#define DFMT "%d"
#define XFMT "0x%x"

/* Return values, these match the DW_DLV* of libdwarf.  */
#define RO_NO_ENTRY  -1
#define RO_OK         0
#define RO_ERROR      1

/* even if already seen, values must match, so no #ifdef needed. */
#define DW_DLV_NO_ENTRY  -1
#define DW_DLV_OK         0
#define DW_DLV_ERROR      1


/*  Error codes returned via pointer,specific to object-reading */
#define RO_ERR_SEEK   2

#define RO_ERR_READ             3
#define RO_ERR_MALLOC           4
#define RO_ERR_OTHER            5
#define RO_ERR_BADOFFSETSIZE    6
#define RO_ERR_LOADSEGOFFSETBAD 7
#define RO_ERR_FILEOFFSETBAD    8
#define RO_ERR_BADTYPESIZE      9
#define RO_ERR_TOOSMALL        10
#define RO_ERR_ELF_VERSION     11
#define RO_ERR_ELF_CLASS       12
#define RO_ERR_ELF_ENDIAN      13
#define RO_ERR_OPEN_FAIL       14
#define RO_ERR_PATH_SIZE       15
#define RO_ERR_INTEGERTOOSMALL 16
#define RO_ERR_SYMBOLSECTIONSIZE  17
#define RO_ERR_RELSECTIONSIZE     18
#define RO_ERR_STRINGOFFSETBIG    19
#define RO_ERR_DYNAMICSECTIONSIZE 20
#define RO_ERR_UNEXPECTEDZERO     21
#define RO_ERR_PHDRCOUNTMISMATCH  22
#define RO_ERR_SHDRCOUNTMISMATCH  23
#define RO_ERR_RELCOUNTMISMATCH   24
#define RO_ERR_NULL_ELF_POINTER   25
#define RO_ERR_NOT_A_KNOWN_TYPE   26
#define RO_ERR_SIZE_SMALL         27
#define RO_ERR_FILE_WRONG_TYPE    28
#define RO_ERR_ELF_STRING_SECT    29
#define RO_ERR_GROUP_ERROR        30
#define RO_SEEK_OFF_END           31
#define RO_READ_OFF_END           32
#define RO_SEEK_ERROR             33
#define RO_READ_ERROR             34
#define RO_ERR_ELF_STRING_LINK_ERROR    35
#define RO_ERR_SECTION_SIZE       36
#define RO_ERR_INVALID_STRING     37
#define DW_DLE_STRING_NOT_TERMINATED      38
#define DW_DLE_FORM_STRING_BAD_STRING     39
#define DW_DLE_CORRUPT_GNU_DEBUGLINK      40
#define DW_DLE_CORRUPT_NOTE_GNU_DEBUGID   41
#define DW_DLE_CORRUPT_GNU_DEBUGID_STRING 42
#define DW_DLE_CORRUPT_GNU_DEBUGID_SIZE   43
#define DW_DLE_ALLOC_FAIL                 44
#define DW_DLE_ERROR_NO_DOS_HEADER        45
#define DW_DLE_DW_DLE_ERROR_NO_NT_SIGNATURE 46
#define DW_DLE_MACHO_CORRUPT_HEADER       47

const char *dwarf_get_errname(int i);
#define P printf
#define F fflush(stdout)

#define RRMOA(f,buf,loc,siz,fsiz,errc) dwarf_object_read_random(f, \
    (char *)buf,loc,siz,fsiz,errc);

#ifdef WORDS_BIGENDIAN
#define SIGN_EXTEND(dest, length)           \
    do {                                    \
        if (*(signed char *)((char *)&dest +\
            sizeof(dest) - length) < 0) {   \
            memcpy((char *)&dest, "\xff\xff\xff\xff\xff\xff\xff\xff", \
                sizeof(dest) - length);     \
        }                                   \
    } while (0)

#else /* LITTLE ENDIAN */
#define SIGN_EXTEND(dest, length)                               \
    do {                                                        \
        if (*(signed char *)((char *)&dest + (length-1)) < 0) { \
            memcpy((char *)&dest+length,                        \
                "\xff\xff\xff\xff\xff\xff\xff\xff",             \
                sizeof(dest) - length);                         \
        }                                                       \
    } while (0)
#endif /* end LITTLE- BIG-ENDIAN */

#define BUFFERSIZE 1000  /* For sanitized() calls */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DWARF_READING_H */
