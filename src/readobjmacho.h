/* Copyright (c) 2013-2018, David Anderson
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
#ifndef READOBJ_H
#define READOBJ_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern char *filename;
extern int printfilenames;
extern FILE *fin;

int cur_read_loc(FILE *fin, long* fileoffset);
#ifdef WORDS_BIGENDIAN
#define ASSIGNMO(gp,t,s)                             \
    do {                                        \
        unsigned tbyte = sizeof(t) - sizeof(s); \
        t = 0;                                  \
        gp->mo_copy_word(((char *)t)+tbyte ,&s,sizeof(s)); \
    } while (0)

#else /* LITTLE ENDIAN */
#define ASSIGNMO(gp,t,s)                             \
    do {                                        \
        t = 0;                                  \
        gp->mo_copy_word(&t,&s,sizeof(s));    \
    } while (0)
#endif /* end LITTLE- BIG-ENDIAN */

struct generic_macho_header {
    LONGESTUTYPE   magic;     
    LONGESTUTYPE   cputype;     
    LONGESTUTYPE   cpusubtype;     
    LONGESTUTYPE   filetype;  
    LONGESTUTYPE   ncmds;      /* number of load commands */
    LONGESTUTYPE   sizeofcmds; /* the size of all the load commands */
    LONGESTUTYPE   flags;   
    LONGESTUTYPE   reserved;  
};


struct macho_filedata_s {
    FILE *  mo_file;
    const char *mo_path;
    unsigned mo_endian;
    size_t mo_filesize;
    size_t mo_offsetsize; /* 32 or 64 */
    size_t mo_pointersize;
    void *(*mo_copy_word) (void *, const void *, size_t);

    /* Used to hold 32 and 64 header data */
    struct generic_macho_header mo_header;
};

struct macho_filedata_s macho_filedata;

int load_macho_header(struct macho_filedata_s *mfp);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* READOBJ_H */
