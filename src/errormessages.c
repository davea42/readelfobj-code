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
#include "config.h"
#include "dwarf_reading.h"

const char *ro_errname[] = {
"DW_DLV_OK",
"DW_DLV_ERROR",
"RO_ERR_SEEK",
"RO_ERR_READ",/*              3 */
"RO_ERR_MALLOC(4) malloc failed",/*            4 */
"RO_ERR_OTHER",/*             5 */
"RO_ERR_BADOFFSETSIZE",/*     6 */
"RO_ERR_LOADSEGOFFSETBAD",/*  7 */
"RO_ERR_FILEOFFSETBAD",/*     8 */
"RO_ERR_BADTYPESIZE",/*       9 */
"RO_ERR_TOOSMALL",/*         10 */
"RO_ERR_ELF_VERSION",/*      11 */
"RO_ERR_ELF_CLASS",/*        12 */
"RO_ERR_ELF_ENDIAN",/*       13 */
"RO_ERR_OPEN_FAIL",/*        14 */
"RO_ERR_PATH_SIZE(15)",/*        15 */
"RO_ERR_INTEGERTOOSMALL(16)",/*  16 */
"RO_ERR_SYMBOLSECTIONSIZE(17)",/*   17 */
"RO_ERR_RELSECTIONSIZE(18)",/*      18 */
"RO_ERR_STRINGOFFSETBIG(19)",/*     19 */
"RO_ERR_DYNAMICSECTIONSIZE",/*  20 */
"RO_ERR_UNEXPECTEDZERO",/*      21 */
"RO_ERR_PHDRCOUNTMISMATCH",/*   22 */
"RO_ERR_SHDRCOUNTMISMATCH",/*   23 */
"RO_ERR_RELCOUNTMISMATCH",/*    24 */
"RO_ERR_NULL_ELF_POINTER",/*    25 */
"RO_ERR_NOT_A_KNOWN_TYPE(26)",/*    26 */
    /*          27 */
"RO_ERR_SIZE_SMALL(27) a pe file is too small to be an object file.",
"RO_ERR_FILE_WRONG_TYPE",/*     28 */
"RO_ERR_ELF_STRING_SECT",/*     29 */
"RO_ERR_GROUP_ERROR",/*         30 */
"RO_SEEK_OFF_END(31)",/*            31 */
"RO_READ_OFF_END(32)",/*            32 */
"RO_SEEK_ERROR",    /*         33 */
"RO_READ_ERROR",     /*        34 */
"RO_ERR_ELF_STRING_LINK_ERROR", /*   35 */
"RO_ERR_SECTION_SIZE",     /*  36 */
"RO_ERR_INVALID_STRING",   /*  37 */
"DW_DLE_STRING_NOT_TERMINATED",   /*   38 */
"DW_DLE_FORM_STRING_BAD_STRING",  /*   39 */
"DW_DLE_CORRUPT_GNU_DEBUGLINK",   /*   40 */
"DW_DLE_CORRUPT_NOTE_GNU_DEBUGID Corrupt .note.gnu.build-id section",/*   41 */
"DW_DLE_CORRUPT_GNU_DEBUGID_STRING",/* 42 */
"DW_DLE_CORRUPT_GNU_DEBUGID_SIZE",/*   43 */
"DW_DLE_ALLOC_FAIL(44) malloc failed",             /*    44 */
    /*    45 */
"DW_DLE_ERROR_NO_DOS_HEADER(45) so we do not accept the file as pe",
"DW_DLE_DW_DLE_ERROR_NO_NT_SIGNATURE(46) so we do not "
    "accept the file as pe",/* 46 */
"DW_DLE_MACHO_CORRUPT_HEADER(47) Corrupt object code",/* 47 */
"DW_DLE_PE_NO_SECTION_NAME(48) Corrupt section header?",
"DW_DLE_NO_SECT_STRINGS(49) Corrupt Sections",
"DW_DLE_TOO_FEW_SECTIONS(50) Corrupt object file",
"DW_DLE_BUILD_ID_DESCRIPTION_SIZE(51) Corrupt .gnu.note/build-id section",
"DW_DLE_BAD_SECTION_FLAGS(52) Corrupt section flag bits",
"DW_DLE_IMPROPER_SECTION_ZERO(53) Section 0 is not empty but must be",
0
};

const char *
dwarf_get_errname(int i)
{
    int size = sizeof(ro_errname)/sizeof(char *);

    if (i >= 0 && i < size) {
        return ro_errname[i];
    }
    return "<unknown>";
}
