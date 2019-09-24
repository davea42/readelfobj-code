/*
Copyright (c) 2019, David Anderson
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
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  */

#include "config.h"
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif /* HAVE_MALLOC_H */
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_ELF_H
#include <elf.h>
#endif /* HAVE_ELF_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#include <sys/types.h> /* for open() */
#include <sys/stat.h> /* for open() */
#include <fcntl.h> /* for open() */
#include <errno.h>
#include "dwarf_reading.h"
#include "dwarf_object_detector.h"
#include "dwarf_object_read_common.h"
#include "readelfobj.h"
#include "dwarfstring.h"
#include "dwarf_debuglink.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif /* O_BINARY */

#define MINBUFLEN 1000
#define TRUE  1
#define FALSE 0

#if _WIN32
#define NULL_DEVICE_NAME "NUL"
#else
#define NULL_DEVICE_NAME "/dev/null"
#endif /* _WIN32 */

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


struct joins_s {
    char * js_fullpath;
    dwarfstring js_dirname;
    dwarfstring js_basename;
    dwarfstring js_cwd;
    dwarfstring js_originalfullpath;
    dwarfstring js_tmp;
    dwarfstring js_tmp2;
    dwarfstring js_tmpdeb;
    dwarfstring js_tmp3;
};

int
_dwarf_check_string_valid(
    void *areaptr,
    void *strptr, 
    void *areaendptr,
    int suggested_error,
    int *errcode)
{
    Dwarf_Small *start = areaptr;
    Dwarf_Small *p = strptr;
    Dwarf_Small *end = areaendptr;

    if (p < start) {
        P("Error  string start  pointer error: loc"
            LONGESTXFMT " Section pointer" LONGESTUFMT "\n",
            (Dwarf_Unsigned)p,
            (Dwarf_Unsigned)start);
        *errcode = suggested_error;
        return DW_DLV_ERROR;
    }
    if (p >= end) {
        P("Error  string end  pointer error: loc"
            LONGESTXFMT " Section pointer" LONGESTUFMT "\n",
            (Dwarf_Unsigned)p,
            (Dwarf_Unsigned)end);
        *errcode = suggested_error;
        return DW_DLV_ERROR;
    }
    while (p < end) {
        if (*p == 0) {
            return DW_DLV_OK;
        }
        ++p;
    }
    P("Error string not terminated error: loc"
         LONGESTXFMT " Section pointer" LONGESTUFMT "\n",
         (Dwarf_Unsigned)strptr,
         (Dwarf_Unsigned)end);
    *errcode = DW_DLE_STRING_NOT_TERMINATED;
    return DW_DLV_ERROR;
}


static int
does_file_exist(char *f)
{
    int fd = 0;

    fd = open(f,O_RDONLY|O_BINARY);
    if (fd < 0) {
        return DW_DLV_NO_ENTRY;
    }
    /* Here we could derive the crc to validate the file. */
    close(fd);
    return DW_DLV_OK;
}


static void
construct_js(struct joins_s * js)
{
    memset(js,0,sizeof(struct joins_s));
    dwarfstring_constructor(&js->js_dirname);
    dwarfstring_constructor(&js->js_basename);
    dwarfstring_constructor(&js->js_cwd);
    dwarfstring_constructor(&js->js_originalfullpath);
    dwarfstring_constructor(&js->js_tmp);
    dwarfstring_constructor(&js->js_tmp2);
    dwarfstring_constructor(&js->js_tmpdeb);
    dwarfstring_constructor(&js->js_tmp3);
}
static void
destruct_js(struct joins_s * js)
{
    dwarfstring_destructor(&js->js_dirname);
    dwarfstring_destructor(&js->js_basename);
    dwarfstring_destructor(&js->js_cwd);
    dwarfstring_destructor(&js->js_originalfullpath);
    dwarfstring_destructor(&js->js_tmp);
    dwarfstring_destructor(&js->js_tmp2);
    dwarfstring_destructor(&js->js_tmpdeb);
    dwarfstring_destructor(&js->js_tmp3);
}

static char joinchar = '/';
static char* joinstr = "/";

int
_dwarf_pathjoinl(dwarfstring *target,dwarfstring * input)
{
    char *inputs = dwarfstring_string(input);
    char *targ = dwarfstring_string(target);
    size_t targlen = 0;

    if (!dwarfstring_strlen(target)) {
        if (*inputs != joinchar) {
            dwarfstring_append(target,joinstr);
            dwarfstring_append(target,dwarfstring_string(input));
        } else {
            dwarfstring_append(target,dwarfstring_string(input));
        }
    }
    targlen = dwarfstring_strlen(target);
    targ = dwarfstring_string(target);
    if (targ[targlen-1] != joinchar) {
        if (*inputs != joinchar) {
            dwarfstring_append(target,joinstr);
            dwarfstring_append(target,inputs);
        } else {
            dwarfstring_append(target,inputs);
        }
    } else {
        if (*inputs != joinchar) {
            dwarfstring_append(target,inputs);
        } else {
            dwarfstring_append(target,inputs+1);
        }
    }
    return DW_DLV_OK;
}
/*  ASSERT: the last character in s is not a /  */
static size_t
mydirlen(char *s)
{
    char *cp = 0;
    char *lastjoinchar = 0;
    size_t count =0;

    for(cp = s ; *cp ; ++cp,++count)  {
        if (*cp == joinchar) {
            lastjoinchar = cp;
        }
    }
    if (lastjoinchar) {
        /* we know diff is postive in all cases */
        ptrdiff_t diff =  lastjoinchar - s;
        /* count the last join charn mydirlen. */
        return (size_t)(diff+1);
    }
    return 0;
}


/*  New September 2019.  Access to the GNU section named
    .gnu_debuglink
    See
    https://sourceware.org/gdb/onlinedocs/gdb/Separate-Debug-Files.html

*/
void
_dwarf_construct_linkedto_path(char *pathname,
   char * input_link_string, /* incoming link string */
   dwarfstring * debuglink_out)
{
    char * depath = pathname;
    size_t depathlen = strlen(depath)+1;
    int res = 0;
    char  buffer[2000];
    unsigned buflen= sizeof(buffer);
    struct joins_s joind;
    size_t dirnamelen = 0;

    construct_js(&joind);
    dirnamelen = mydirlen(depath);
    if (dirnamelen) {
        dwarfstring_append_length(&joind.js_dirname,depath,depathlen);
    }
    dwarfstring_append(&joind.js_basename,input_link_string); 

    if (depath[0] != joinchar) {
        char *wdret = 0;

        wdret = getcwd(buffer,buflen);
        if (!wdret) {
            destruct_js(&joind);
            return;
        }
        dwarfstring_append(&joind.js_cwd,buffer);
        buffer[0] = 0;
    }

    {
        dwarfstring_append(&joind.js_originalfullpath,
            dwarfstring_string(&joind.js_cwd));
        res =  _dwarf_pathjoinl(&joind.js_originalfullpath,
            &joind.js_dirname);
        if (res != DW_DLV_OK) {
            destruct_js(&joind);
            return;
        }
    }
    /*  We need to be sure there is no accidental
        match with the file we opened. */
    if (dwarfstring_strlen(&joind.js_dirname)) {
        res = _dwarf_pathjoinl(&joind.js_cwd,
            &joind.js_dirname);
    } else {
        res = DW_DLV_OK;
    }
    /* Now js_cwd is a leading / directory name. */
    if (res == DW_DLV_OK) {
        dwarfstring_reset(&joind.js_tmp);
        dwarfstring_append(&joind.js_tmp,
            dwarfstring_string(&joind.js_cwd));
        /* If we add basename do we find what we look for? */
        res = _dwarf_pathjoinl(&joind.js_tmp,&joind.js_basename);
        if (!strcmp(dwarfstring_string(&joind.js_originalfullpath),
            dwarfstring_string(&joind.js_cwd))) {
            /* duplicated name. spurious match. */
        } else if (res == DW_DLV_OK) {
            res = does_file_exist(dwarfstring_string(&joind.js_cwd));
            if (res == DW_DLV_OK) {
                dwarfstring_append(debuglink_out,
                     dwarfstring_string(&joind.js_tmp)); 
                destruct_js(&joind);
                return;
            }
        }
    }
    {

        dwarfstring_reset(&joind.js_tmp2);
        dwarfstring_reset(&joind.js_tmpdeb);

        dwarfstring_append(&joind.js_tmp2,
            dwarfstring_string(&joind.js_cwd));
        dwarfstring_append(&joind.js_tmpdeb,".debug");
        res = _dwarf_pathjoinl(&joind.js_tmp2,&joind.js_tmpdeb);
        if (res == DW_DLV_OK) {
            res = _dwarf_pathjoinl(&joind.js_tmp2,&joind.js_basename);
            if (!strcmp(dwarfstring_string(&joind.js_originalfullpath),
                dwarfstring_string(&joind.js_tmp2))) {
                /* duplicated name. spurious match. */
            } else if(res == DW_DLV_OK) {
                res = does_file_exist(dwarfstring_string(
                    &joind.js_tmp2));
                if (res == DW_DLV_OK) {
                    dwarfstring_append(debuglink_out,
                       dwarfstring_string(&joind.js_tmp2));
                    destruct_js(&joind);
                    return;
                }
            }
        }
    }
    /* Not found above */
    {
        dwarfstring_reset(&joind.js_tmp3);
        dwarfstring_append(&joind.js_tmp3, "/usr/lib/debug");
        res = _dwarf_pathjoinl(&joind.js_tmp3, &joind.js_cwd);
        if (res == DW_DLV_OK) {
            res = _dwarf_pathjoinl(&joind.js_tmp3,&joind.js_basename);
            if (!strcmp(dwarfstring_string(&joind.js_originalfullpath),
                dwarfstring_string(&joind.js_tmp3))) {
                /* duplicated name. spurious match. */
            } else if (res == DW_DLV_OK) {
                res = does_file_exist(
                    dwarfstring_string(&joind.js_tmp3));
                if (res == DW_DLV_OK) {
                    dwarfstring_append(debuglink_out,
                       dwarfstring_string(&joind.js_tmp3));
                    destruct_js(&joind);
                    return;
                }
            }
        }
    }
    destruct_js(&joind);
    return;
}

int
dwarf_gnu_debuglink(elf_filedata ep,
    char ** name_returned,  /* static storage, do not free */
    char ** crc_returned, /* 32bit crc , do not free */
    char **  debuglink_path_returned, /* caller must free returned pointer */
    unsigned *debuglink_path_size_returned,/* Size of the debuglink path.
        zero returned if no path known/found. */
    int*   errcode)
{
    char *ptr = 0;
    char *endptr = 0;
    unsigned namelen = 0;
    unsigned m = 0;
    unsigned incr = 0;
    char *crcptr = 0;
    int res = DW_DLV_ERROR;
    struct generic_shdr* shdr = ep->f_shdr;
    Dwarf_Unsigned seccount = ep->f_loc_shdr.g_count;
    Dwarf_Unsigned secsize = 0;
    Dwarf_Unsigned secoffset =0;
    char *secdata = 0;
    char * pathname = ep->f_path;
    Dwarf_Unsigned i = 0;

    for (i = 0;i < seccount; ++i,shdr++) {
        if (strcmp(".gnu_debuglink",shdr->gh_namestring)) {
            continue;
        }
    }
    if (i >= seccount) {
        return DW_DLV_NO_ENTRY;
    }
    secsize = shdr->gh_size;
    secdata = malloc(secsize);
    if (!secdata) {
        P("Error  malloc fail:  size "
            LONGESTXFMT " line %d %s\n",secsize,
            __LINE__,__FILE__);
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    shdr->gh_content = secdata;
    ptr = secdata;
    endptr = ptr + secsize;

    res = _dwarf_check_string_valid(ptr,
        ptr,
        endptr,
        DW_DLE_FORM_STRING_BAD_STRING,
        errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    namelen = (unsigned)strlen((const char*)ptr);
    m = (namelen+1) %4;
    if (m) {
        incr = 4 - m;
    }
    crcptr = ptr +namelen +1 +incr;
    if ((crcptr +4) != endptr) {
        P("ERROR: .gnu_debuglink is not the right length: "
            " expected end " LONGESTXFMT 
            " found end at  " LONGESTUFMT ")\n",
            (Dwarf_Unsigned)(crcptr+4),
            (Dwarf_Unsigned)endptr);
        *errcode = DW_DLE_CORRUPT_GNU_DEBUGLINK;
        return DW_DLV_ERROR;
    }
    if (pathname) {
        dwarfstring outpath;
        unsigned pathlength = 0;

        dwarfstring_constructor(&outpath);
        _dwarf_construct_linkedto_path(pathname,ptr,&outpath);
        pathlength = dwarfstring_strlen(&outpath);
        *debuglink_path_returned = malloc(pathlength +1);
        if( !*debuglink_path_returned) {
            P("Error  malloc fail:  size "
                LONGESTXFMT " line %d %s\n",
                (Dwarf_Unsigned)(pathlength+1),
                __LINE__,__FILE__);
            *errcode = DW_DLE_ALLOC_FAIL;
            return DW_DLV_ERROR;
        }
        strcpy(*debuglink_path_returned, 
            dwarfstring_string(&outpath));
        *debuglink_path_size_returned = 
            dwarfstring_strlen(&outpath);
        dwarfstring_destructor(&outpath);
    } else {
        *debuglink_path_size_returned  = 0;
    }
    *name_returned = ptr;
    *crc_returned = crcptr;
    return DW_DLV_OK;
}

/*  The definition of .note.gnu.buildid contents (also
    used for other GNU .note.gnu.  sections too. */
struct buildid_s {
    char bu_ownernamesize[4];
    char bu_buildidsize[4];
    char bu_type[4];
    char bu_owner[1];
};

int
dwarf_gnu_buildid(elf_filedata ep,
    Dwarf_Unsigned * type_returned,
    const char     **owner_name_returned,
    Dwarf_Unsigned * build_id_length_returned,
    const unsigned char  **build_id_returned,
    int*   errcode)
{
    char * ptr = 0;
    char * endptr = 0;
    int res = DW_DLV_ERROR;
    struct buildid_s *bu = 0;
    Dwarf_Unsigned namesize = 0;
    Dwarf_Unsigned descrsize = 0;
    Dwarf_Unsigned type = 0;
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned finalsize;
    struct generic_shdr* shdr = ep->f_shdr;
    Dwarf_Unsigned seccount = ep->f_loc_shdr.g_count;
    Dwarf_Unsigned secsize = 0;
    Dwarf_Unsigned secoffset =0;
    char *secdata = 0;

    for (i = 0;i < seccount; ++i,shdr++) {
        if (strcmp(".note.gnu.build-id",shdr->gh_namestring)) {
            continue;
        }
    }
    if (i >= seccount) {
        return DW_DLV_NO_ENTRY;
    }
    secsize = shdr->gh_size;
    secdata = (char *)malloc(secsize);
    if (!secdata) {
        P("Error  malloc fail:  size "
            LONGESTXFMT " line %d %s\n",secsize,
            __LINE__,__FILE__);
        *errcode = DW_DLE_ALLOC_FAIL;
        return DW_DLV_ERROR;
    }
    shdr->gh_content = secdata;
    /*  We gh_content till all is closed 
        as we return pointers into it
        if all goes well. */
    secoffset = shdr->gh_offset;
    res = RRMOA(ep->f_fd,shdr,secoffset,
         secsize,ep->f_filesize,errcode);
    if(res != RO_OK) {
        P("Read  " LONGESTUFMT
            " bytes .note.gnu.build-id section failed\n",secsize);
        P("Read  offset " LONGESTXFMT " length "
            LONGESTXFMT " off+len " LONGESTXFMT "\n",
            secoffset,secsize,secoffset + secsize);
        free(secdata);
        return res;
    }
    ptr = secdata;
    endptr = ptr + secsize;
    if (secsize < sizeof(struct buildid_s)) {
        P("ERROR section .note.gnu.build-id too small: " 
            " section length: " LONGESTXFMT 
            " minimum struct size " LONGESTXFMT  "\n",
            secsize,(Dwarf_Unsigned) sizeof(struct buildid_s));
        *errcode =  DW_DLE_CORRUPT_NOTE_GNU_DEBUGID;
        free(secdata);
        return DW_DLV_ERROR;
    }

    bu = (struct buildid_s *)ptr;

    
    ASNAR(ep->f_copy_word,namesize, bu->bu_ownernamesize);
    ASNAR(ep->f_copy_word,descrsize, bu->bu_buildidsize);
    ASNAR(ep->f_copy_word,type, bu->bu_type);

#if 0 /* dadebug */
    READ_UNALIGNED_CK(dbg,namesize,Dwarf_Unsigned,
        (Dwarf_Byte_Ptr)&bu->bu_ownernamesize[0], 4,
        errcode,endptr);
    READ_UNALIGNED_CK(dbg,descrsize,Dwarf_Unsigned,
        (Dwarf_Byte_Ptr)&bu->bu_buildidsize[0], 4,
        errcode,endptr);
    READ_UNALIGNED_CK(dbg,type,Dwarf_Unsigned,
        (Dwarf_Byte_Ptr)&bu->bu_type[0], 4,
        errcode,endptr);
#endif

    if (descrsize != 20) {
        P("ERROR section .note.gnu.build-id description size small: " 
            " description length : " LONGESTUFMT 
            " required length %u \n",
            descrsize,20);
        *errcode = DW_DLE_CORRUPT_NOTE_GNU_DEBUGID;
        free(secdata);
        return DW_DLV_ERROR;
    }
    res = _dwarf_check_string_valid(&bu->bu_owner[0],
        &bu->bu_owner[0],
        endptr,
        DW_DLE_CORRUPT_GNU_DEBUGID_STRING,
        errcode);
    if ( res != DW_DLV_OK) {
        return res;
    }
    if ((strlen(bu->bu_owner) +1) != namesize) {
        P("ERROR section .note.gnu.build-id owner string size wrong: " 
            " size " LONGESTUFMT 
            " required length " LONGESTUFMT "\n",
            descrsize,namesize);
        *errcode = DW_DLE_CORRUPT_GNU_DEBUGID_STRING;
        return DW_DLV_ERROR;
    }

    finalsize = sizeof(struct buildid_s)-1 + namesize + descrsize;
    if (finalsize > secsize) {
        P("ERROR section .note.gnu.build-id owner final size wrong: " 
            " size " LONGESTUFMT 
            " required length " LONGESTUFMT "\n",
            descrsize,
            finalsize);
        *errcode = DW_DLE_CORRUPT_GNU_DEBUGID_SIZE;
        return DW_DLV_ERROR;
    }
    *type_returned = type;
    *owner_name_returned = &bu->bu_owner[0];
    *build_id_length_returned = descrsize;
    *build_id_returned = ptr + sizeof(struct buildid_s)-1 + namesize;
    return DW_DLV_OK;
}
