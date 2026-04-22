
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

#if defined(HAVE_ZLIB) && defined(HAVE_ZSTD)
/*  This is exclusively for reading .symtab and .symstr
    sections. See dwarf_elf_init() for decompressing all
    other sections. We need decompress to do relocations (if any
    relocations and if either of these sections compressed).  */
int
dwarf_elf_do_decompress(elf_filedata ep,
    struct generic_shdr *psh,
    int* error);
#ifdef WORDS_BIGENDIAN
#define ASNARLRAW(func,ec,t,s,l)                     \
    do {                                  \
        unsigned tbyte = sizeof(t) - (l); \
        if (sizeof(t) < (l)) {            \
            *ec = DW_DLE_ZLIB_UNCOMPRESS_ERROR; \
        }                                 \
        (t) = 0;                          \
        func(((char *)&(t))+tbyte ,&(s)[0],(l));\
    } while (0)
#else /* LITTLE ENDIAN */
#define ASNARLRAW(func,ec,t,s,l)                 \
    do {                              \
        (t) = 0;                      \
        if (sizeof(t) < (l)) {        \
            *ec = DW_DLE_ZLIB_UNCOMPRESS_ERROR; \
        }                             \
        func(&(t),&(s)[0],(l));  \
    } while (0)
#endif /* end LITTLE- BIG-ENDIAN */

#endif /* ZLIB ZSTD */
