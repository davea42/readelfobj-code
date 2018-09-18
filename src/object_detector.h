/* Copyright (c) 2018-2018, David Anderson
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

/*  Provides a small number of Elf/Mach-o
    object file declarations/definitions. */

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif /* TRUE */


#ifndef DW_DLV_OK
#define DW_DLV_NO_ENTRY -1
#define DW_DLV_OK        0
#define DW_DLV_ERROR     1
#endif /* DW_DLV_OK */

#define DW_FTYPE_UNKNOWN 0
#define DW_FTYPE_ELF     1
#define DW_FTYPE_MACH_O  2
#define DW_FTYPE_PE      3


#define DW_ENDIAN_UNKNOWN 0
#define DW_ENDIAN_BIG     1
#define DW_ENDIAN_LITTLE  2
#define DW_ENDIAN_SAME    3
#define DW_ENDIAN_OPPOSITE 4

#ifndef EI_NIDENT
#define EI_NIDENT 16 
#define EI_CLASS  4
#define EI_DATA   5
#define EI_VERSION 6 
#define ELFCLASS32 1
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2
#endif /* EI_NIDENT */

#define DSYM_SUFFIX "/.dSYM/Contents/Resources/DWARF/"
#define PATHSIZE 2000

/*  Assuming short 16 bits, unsigned 32 bits */
typedef unsigned short t16;
typedef unsigned t32;

#ifndef  MH_MAGIC
/* mach-o 32bit */
#define MH_MAGIC        0xfeedface
#define MH_CIGAM        0xcefaedfe
#endif /*  MH_MAGIC */
#ifndef  MH_MAGIC_64
/* mach-o 64bit */
#define MH_MAGIC_64 0xfeedfacf
#define MH_CIGAM_64 0xcffaedfe
#endif /*  MH_MAGIC_64 */

#define EI_NIDENT 16
/* An incomplete elf header, good for 32 and 64bit elf */
struct elf_header {
    unsigned char  e_ident[EI_NIDENT];
    t16 e_type;
    t16 e_machine;
    t32 e_version; 
};

/*  outpath is a place you provide, of a length outpath_len
    you consider reasonable,
    where the final path used is recorded.
    This matters as for mach-o if the path is a directory
    name the function will look in the standard macho-place
    for the object file (useful for dSYM) and return the
    constructed path in oupath.  
    returns DW_DLV_OK, DW_DLV_ERROR, or DW_DLV_NO_ENTRY */
int dwarf_object_detector_path(const char  *path,
    char *outpath,size_t outpath_len,
    unsigned *ftype,
    unsigned *endian,
    unsigned *offsetsize,
    size_t   *filesize);
