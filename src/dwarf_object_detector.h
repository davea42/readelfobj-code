/* Copyright (c) 2018, David Anderson
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

#ifndef DWARF_OBJECT_DETECTOR_H
#define DWARF_OBJECT_DETECTOR_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*  Declares the interface function.
    outpath is a place you provide, of a length outpath_len
    you consider reasonable,
    where the final path used is recorded.
    outpath_len must be larger than strlen(path);
    
    This matters as for mach-o if the path is a directory
    name the function will look in the standard macho-place
    for the object file (useful for dSYM) and return the
    constructed path in oupath.  
    returns DW_DLV_OK, DW_DLV_ERROR, or DW_DLV_NO_ENTRY */

#ifndef DW_FTYPE_UNKNOWN
#define DW_FTYPE_UNKNOWN 0
#define DW_FTYPE_ELF     1
#define DW_FTYPE_MACH_O  2
#define DW_FTYPE_PE      3
#endif /* DW_FTYPE_UNKNOWN */

#ifndef DW_ENDIAN_UNKNOWN
#define DW_ENDIAN_UNKNOWN 0
#define DW_ENDIAN_BIG     1
#define DW_ENDIAN_LITTLE  2
#define DW_ENDIAN_SAME    3
#define DW_ENDIAN_OPPOSITE 4
#endif /* DW_ENDIAN_UNKNOWN */

/* offsetsize refers to the object-file-format.
    Elf 32 or macho- 32. Not to dwarf. */
int dwarf_object_detector_path(const char  *path,
    char *outpath,size_t outpath_len,
    unsigned *ftype,
    unsigned *endian,
    unsigned *offsetsize,
    size_t   *filesize,
    int * errcode;);

int dwarf_object_detector_f(FILE *f,
    unsigned *ftype,
    unsigned *endian,
    unsigned *offsetsize,
    size_t   *filesize,
    int *errcode);

int dwarf_object_detector_fd(int fd,
    unsigned *ftype,
    unsigned *endian,
    unsigned *offsetsize,
    size_t   *filesize,
    int * errcode);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* DWARF_OBJECT_DETECTOR_H */