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

#include <stdio.h>
#include <sys/types.h> /* fstat */
#include <sys/stat.h> /* fstat */
#include <string.h> /* memcpy, strcpy */
#include "object_detector.h"

/*  For following MacOS file naming convention */
static const char * 
getseparator (const char *f)
{
    const char *p, *q;
    p = NULL;
    q = f;
    char c;
    do  {
        c = *q++;
        if (c == '\\' || c == '/' || c == ':') {
            p = q;
        }
    } while (c);
    return p;
}
static const char * 
getbasename (const char *f)
{
    const char *pseparator = getseparator (f);
    if (!pseparator) {
        return f;
    }
    return pseparator;
}

/*  Not a standard function, though part of GNU libc
    since 2008. */ 
static char *
dw_stpcpy(char *dest,const char *src)
{
     const char *cp = src;
     char *dp = dest;
     
     for ( ; *cp; ++cp,++dp) {
         *dp = *cp;
     }
     *dp = 0;
     return dp;
}



static int 
fill_in_elf_fields(struct elf_header *h,
    unsigned *endian,
    unsigned *offsetsize)
{
    unsigned locendian = 0;
    unsigned locoffsetsize = 0;

    if (h->e_version != 1 /* EV_CURRENT */) {
        return DW_DLV_ERROR;
    }
    switch(h->e_ident[EI_CLASS]) {
    case ELFCLASS32:
        locoffsetsize = 32;
        break;
    case ELFCLASS64:
        locoffsetsize = 64;
        break;
    default:
        return DW_DLV_ERROR;
    }
    switch(h->e_ident[EI_DATA]) {
    case ELFDATA2LSB:
        locendian = DW_ENDIAN_LITTLE;
        break;
    case ELFDATA2MSB:
        locendian = DW_ENDIAN_BIG;
        break;
    default:
        return DW_DLV_ERROR;
    }
    *endian = locendian;
    *offsetsize = locoffsetsize;
    return DW_DLV_OK;
}

static int 
is_mach_o_magic(struct elf_header *h,
    unsigned *endian,
    unsigned *offsetsize)
{
    t32 magicval = 0;
    unsigned locendian = 0;
    unsigned locoffsetsize = 0;

    memcpy(&magicval,h,sizeof(magicval));
    if (magicval == MH_MAGIC) {
        locendian = DW_ENDIAN_SAME;
        locoffsetsize = 32;
    } else if (magicval == MH_CIGAM) {
        locendian = DW_ENDIAN_OPPOSITE;
        locoffsetsize = 32;
    }else if (magicval == MH_MAGIC_64) {
        locendian = DW_ENDIAN_SAME;
        locoffsetsize = 64;
    } else if (magicval == MH_CIGAM_64) {
        locendian = DW_ENDIAN_OPPOSITE;
        locoffsetsize = 64;
    } else {
        return FALSE;
    }
    *endian = locendian;
    *offsetsize = locoffsetsize;
    return TRUE; 
}

int 
dwarf_object_detector_f(FILE *f,
    unsigned *ftype,
    unsigned *endian,
    unsigned *offsetsize,
    size_t   *filesize)
{
    size_t resf = 0;
    struct elf_header h;
    size_t readlen = sizeof(h);
    int res = 0;
    long fsize = 0;

    if (sizeof(t32) != 4 || sizeof(t16)!= 2) {
        return DW_DLV_ERROR;
    }
    res = fseek(f,0L,SEEK_END);
    if(res) {
        return DW_DLV_ERROR;
    }
    fsize = ftell(f);
    if (fsize < 0) {
        return DW_DLV_ERROR;
    }
    if (fsize <= readlen) { 
        /* Not a real object file */
        return DW_DLV_ERROR;
    }
    res = fseek(f,0L,SEEK_SET);
    if(res) {
        return DW_DLV_ERROR;
    }
    res = fread(&h,1,readlen,f);
    if (res != readlen) {
        return DW_DLV_ERROR;
    }
    if (h.e_ident[0] == 0x7f &&
        h.e_ident[1] == 'E' && 
        h.e_ident[2] == 'L' && 
        h.e_ident[3] == 'F') {
        /* is ELF */ 
        res = fill_in_elf_fields(&h,endian,offsetsize);
        if (res != DW_DLV_OK) {
            return res;
        }
        *ftype = DW_FTYPE_ELF;
        *filesize = (size_t)fsize;
        return DW_DLV_OK;
    }
    if (is_mach_o_magic(&h,endian,offsetsize)) {
        *ftype = DW_FTYPE_MACH_O;
        *filesize = (size_t)fsize;
        return DW_DLV_OK;
 
    }
    /* CHECK FOR  PE object. */
    return DW_DLV_NO_ENTRY;
}

int 
dwarf_object_detector_path(const char  *path,
    char *outpath,size_t outpath_len,
    unsigned *ftype,
    unsigned *endian,
    unsigned *offsetsize,
    size_t   *filesize)
{
    FILE *f = 0;
    size_t plen = strlen(path);
    size_t dsprefixlen = sizeof(DSYM_SUFFIX);
    struct stat statbuf;

    /* Malloc this instead? */
    char dwarf_finalpath[PATHSIZE];
    int res = 0;

#if !defined(S_ISREG)
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISDIR)
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

    res = stat(path,&statbuf);
    if(res) {
        return DW_DLV_NO_ENTRY;
    }
    if(S_ISDIR(statbuf.st_mode)) {
        size_t finallen = 2*plen + dsprefixlen + 2;
        if (finallen < PATHSIZE) {
            /* path + suffix + basenameofpath */
            char * p = 0;

            p = dw_stpcpy(dwarf_finalpath,path); 
            p = dw_stpcpy(p,DSYM_SUFFIX);
            dw_stpcpy(p,getbasename(path));
        } else {
            return DW_DLV_ERROR;
        }
        
        f = fopen(dwarf_finalpath,"r");
        if (!f) {
            return DW_DLV_NO_ENTRY;
        }
        res = dwarf_object_detector_f(f,
            ftype,endian,offsetsize,filesize);
        if (res == DW_DLV_OK) {
            if (finallen >= outpath_len) {
                fclose(f);
                return DW_DLV_ERROR;
            }
            strcpy(outpath,dwarf_finalpath);
        }
        fclose(f);
        return res;
    } 
    f = fopen(path,"r");
    if (!f) {
        return DW_DLV_NO_ENTRY;
    }
    res = dwarf_object_detector_f(f,
        ftype,endian,offsetsize,filesize);
    if (res == DW_DLV_OK) {
        if (plen >= outpath_len) {
                fclose(f);
                return DW_DLV_ERROR;
        }
        strcpy(outpath,path);
    }
    fclose(f);
    return res;
}


#ifdef TESTING
const char *dwarf_file_type[5] = {
"file type unknown",
"file type elf",
"file type mach-o",
"file type pe",
0
};

const char *dwarf_endian_type[6] = {
"endian:unknown",
"big-endian",
"little-endian",
"same-endian",
"opposite-endian",
0
};


static char finalpath[PATHSIZE];

int main(int argc, char **argv)
{
    int ct = 1;

    for( ;ct < argc ; ct++) {
        char *path = argv[ct];
        int res = 0;
        unsigned ftype = 0;
        unsigned endian = 0;
        unsigned offsetsize = 0;
        size_t filesize = 0;
         
        res = dwarf_object_detector_path(path,
            finalpath,PATHSIZE,
            &ftype, &endian, &offsetsize,&filesize);
        if (res == DW_DLV_OK) {
            printf("%s (%s), type %u %s, endian "
                "%u %s, offsetsize %u, filesize %lu\n",
                path,finalpath,
                ftype,dwarf_file_type[ftype],
                endian,dwarf_endian_type[endian],
                offsetsize,
                (unsigned long)filesize);
        } else if (res == DW_DLV_ERROR) {
            printf("%s type is unknown\n",path); 
        } else if (res == DW_DLV_NO_ENTRY) {
            printf("%s cannot be opened\n",path); 
        } else {
            printf("%s FAIL. LOGIC ERROR\n",path);
        }
    }
}
#endif /* TESTING */
