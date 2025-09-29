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
#ifdef HAVE_STDDEF_H
#include <stddef.h> /* ptrdiff_t */
#endif /* HAVE_STDDEF_H */
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for exit(), C89 malloc */
#endif /* HAVE_STDLIB_H */
#include <string.h>
#ifdef HAVE_ELF_H
#include <elf.h>
#endif /* HAVE_ELF_H */
#ifdef HAVE_STDINT_H
#include <stdint.h> /* for uintptr_t */
#endif /* HAVE_STDINT_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#include <sys/types.h> /* for open() */
#include <sys/stat.h> /* for open() */
#include <fcntl.h> /* for open() */
#include <errno.h>
#include "dwarf_types.h"
#include "dwarf_reading.h"
#include "dwarf_macho_loader.h" 
#include "dwarf_machoread.h" /* for Dwarf_Unsigned */
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

#ifdef HAVE_UNUSED_ATTRIBUTE
#define  UNUSEDARG __attribute__ ((unused))
#else
#define  UNUSEDARG
#endif

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

static int
extract_buildid(elf_filedata ep,
    struct generic_shdr* buildidshdr,
    Dwarf_Unsigned * type_returned,
    char     **owner_name_returned,
    Dwarf_Unsigned * build_id_length_returned,
    unsigned char  **build_id_returned,
    int*   errcode);

struct joins_s {
    char * js_fullpath;
    dwarfstring js_dirname;
    dwarfstring js_basepath;
    dwarfstring js_basename;
    dwarfstring js_cwd;
    dwarfstring js_originalfullpath;
    dwarfstring js_tmp;
    dwarfstring js_tmp2;
    dwarfstring js_tmpdeb;
    dwarfstring js_tmp3;
    dwarfstring js_buildid;
    dwarfstring js_buildid_filename;
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
    ptrdiff_t diff = 0;

    if (p < start) {
        diff = start - p;
        P("Error  string start  pointer error: loc "
            LONGESTSFMT
            " bytes before available area \n",(Dwarf_Signed)diff);
        *errcode = suggested_error;
        return DW_DLV_ERROR;
    }
    if (p >= end) {
        diff = p - start;
        P("ERROR: string end pointer error, argument error "
            "  Length:  "
            LONGESTSFMT  "\n",
            (Dwarf_Signed)diff);
        *errcode = suggested_error;
        return DW_DLV_ERROR;
    }
    while (p < end) {
        if (!*p) {
            return DW_DLV_OK;
        }
        ++p;
    }
    /*  ASSERT: p >= end, p >= strptr */
    diff = p - start;
    P("Error string not terminated error:  not ended after "
        LONGESTSFMT " bytes (past end of available bytes)\n",
        (Dwarf_Signed)diff);
    *errcode = suggested_error;
    return DW_DLV_ERROR;
}

#if 0
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
#endif

static void
construct_js(struct joins_s * js)
{
    memset(js,0,sizeof(struct joins_s));
    dwarfstring_constructor(&js->js_basename);
    dwarfstring_constructor(&js->js_dirname);
    dwarfstring_constructor(&js->js_basepath);
    dwarfstring_constructor(&js->js_cwd);
    dwarfstring_constructor(&js->js_originalfullpath);
    dwarfstring_constructor(&js->js_tmp);
    dwarfstring_constructor(&js->js_tmp2);
    dwarfstring_constructor(&js->js_tmpdeb);
    dwarfstring_constructor(&js->js_tmp3);
    dwarfstring_constructor(&js->js_buildid);
    dwarfstring_constructor(&js->js_buildid_filename);
}
static void
destruct_js(struct joins_s * js)
{
    dwarfstring_destructor(&js->js_dirname);
    dwarfstring_destructor(&js->js_basepath);
    dwarfstring_destructor(&js->js_basename);
    dwarfstring_destructor(&js->js_cwd);
    dwarfstring_destructor(&js->js_originalfullpath);
    dwarfstring_destructor(&js->js_tmp);
    dwarfstring_destructor(&js->js_tmp2);
    dwarfstring_destructor(&js->js_tmpdeb);
    dwarfstring_destructor(&js->js_tmp3);
    dwarfstring_destructor(&js->js_buildid);
    dwarfstring_destructor(&js->js_buildid_filename);
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
        dwarfstring_append(target,dwarfstring_string(input));
        return DW_DLV_OK;
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

    for (cp = s ; *cp ; ++cp,++count)  {
        if (*cp == joinchar) {
            lastjoinchar = cp;
        }
    }
    if (lastjoinchar) {
        /* we know diff is postive in all cases */
        ptrdiff_t diff =  lastjoinchar - s;
        /* count the last join as mydirlen. */
        return (size_t)(diff+1);
    }
    return 0;
}

struct dwarfstring_list_s {
    dwarfstring                dl_string;
    struct dwarfstring_list_s *dl_next;
};

static void
dwarfstring_list_constructor(struct dwarfstring_list_s *l)
{
    dwarfstring_constructor(&l->dl_string);
    l->dl_next = 0;
}

static int
dwarfstring_list_add_new(struct dwarfstring_list_s * base_entry,
    struct dwarfstring_list_s *prev,
    dwarfstring * input,
    struct dwarfstring_list_s ** new_out,
    int *errcode)
{
    struct dwarfstring_list_s *next = 0;
    if (prev) {
        next = ( struct dwarfstring_list_s *)
        malloc(sizeof(struct dwarfstring_list_s));
        if (!next) {
            *errcode = DW_DLE_ALLOC_FAIL;
            return DW_DLV_ERROR;
        }
        dwarfstring_list_constructor(next);
    } else {
        next = base_entry;
    }
    dwarfstring_append(&next->dl_string,
        dwarfstring_string(input));
    if (prev) {
        prev->dl_next = next;
    }
    *new_out = next;
    return DW_DLV_OK;
}

/*  destructs passed in entry (does not free it) and all
    those on the dl_next list (those are freed). */
static void
dwarfstring_list_destructor(struct dwarfstring_list_s *l)
{
    struct dwarfstring_list_s *curl = l;
    struct dwarfstring_list_s *nextl = l;

    nextl = curl->dl_next;
    dwarfstring_destructor(&curl->dl_string);
    curl->dl_next = 0;
    curl = nextl;
    for ( ; curl ; curl = nextl) {
        nextl = curl->dl_next;
        dwarfstring_destructor(&curl->dl_string);
        curl->dl_next = 0;
        free(curl);
    }
}

static void
build_buildid_filename(dwarfstring *target,
    unsigned buildid_length,
    unsigned char *buildid)
{
    dwarfstring tmp;
    unsigned bu = 0;
    unsigned char *cp  = 0;
    char lbuf[10];

    dwarfstring_constructor(&tmp);
    cp = buildid;
    for (bu = 0; bu < buildid_length; ++bu ,++cp) {
        sprintf(lbuf,"%02x",*cp);
        dwarfstring_append(&tmp,lbuf);
        if (bu == 0) {
            dwarfstring_append(&tmp,"/");
        }
    }
    dwarfstring_append(&tmp,".debug");
    _dwarf_pathjoinl(target,&tmp);
    dwarfstring_destructor(&tmp);
    return;
}

#if 0
static void
dump_bytes(const char *msg,unsigned char * start, unsigned len)
{
    Dwarf_Small *end = start + len;
    Dwarf_Small *cur = start;
    printf("%s (0x%lx) ",msg,(unsigned long)start);
    for (; cur < end; cur++) {
        printf("%02x", *cur);
    }
    printf("\n");
}
#endif

/*  New September 2019.  Access to the GNU section named
    .gnu_debuglink
    See
    https://sourceware.org/gdb/onlinedocs/gdb/
    Separate-Debug-Files.html
*/
int _dwarf_construct_linkedto_path(
    char         **global_prefixes_in,
    unsigned       length_global_prefixes_in,
    char          *pathname_in,
    char          *link_string_in, /* from debug link */
    UNUSEDARG unsigned char *crc_in, /* from debug_link, 4 bytes */
    unsigned       buildid_length, /* from gnu buildid */
    unsigned char *buildid, /* from gnu buildid */
    char        ***paths_out,
    unsigned      *paths_out_length,
    int *errcode)
{
    char * depath = pathname_in;
    int res = 0;
    struct joins_s joind;
    size_t dirnamelen = 0;
    struct dwarfstring_list_s base_dwlist;
    struct dwarfstring_list_s *last_entry = 0;
    unsigned global_prefix_number = 0;

    dwarfstring_list_constructor(&base_dwlist);
    construct_js(&joind);
    build_buildid_filename(&joind.js_buildid_filename,
        buildid_length, buildid);
    dirnamelen = mydirlen(depath);
    if (dirnamelen) {
        dwarfstring_append_length(&joind.js_dirname,
            depath,dirnamelen);
    }
    dwarfstring_append(&joind.js_basepath,depath+dirnamelen);
    dwarfstring_append(&joind.js_basename,link_string_in);
    if (depath[0] != joinchar) {
        char  buffer[2000];
#ifdef TESTING
        buffer[0] = 0;
        /*  For testing lets use a fake (consistent)
            base dir.  */
        strcpy(buffer,"/testing/base/dir");
#else
        unsigned buflen= sizeof(buffer);
        char *wdret = 0;

        buffer[0] = 0;
        wdret = getcwd(buffer,buflen);
        if (!wdret) {
            printf("getcwd() issue. Do nothing. "
                " line  %d %s\n",__LINE__,__FILE__);
            dwarfstring_list_destructor(&base_dwlist);
            destruct_js(&joind);
            *errcode = DW_DLE_ALLOC_FAIL;
            return DW_DLV_ERROR;
        }
#endif /* TESTING */
        dwarfstring_append(&joind.js_cwd,buffer);
        buffer[0] = 0;
    }

    {
        /*  Builds the full path to the original
            executable, but absent executable name. */
        dwarfstring_append(&joind.js_originalfullpath,
            dwarfstring_string(&joind.js_cwd));
        _dwarf_pathjoinl(&joind.js_originalfullpath,
            &joind.js_dirname);
        _dwarf_pathjoinl(&joind.js_originalfullpath,
            &joind.js_basepath);
#ifdef TESTING
        printf("originalfullpath is %s\n",
            dwarfstring_string(&joind.js_originalfullpath));
#endif
    }
    {
        /*  There is perhaps a directory prefix in the
            incoming pathname.
            So we add that to js_cwd. */
        res = _dwarf_pathjoinl(&joind.js_cwd,
            &joind.js_dirname);
        /* This is used in a couple search paths. */
    }
    for (global_prefix_number = 0;
        buildid_length &&
        (global_prefix_number < length_global_prefixes_in);
        ++global_prefix_number) {
        char * prefix = 0;

        prefix = global_prefixes_in[global_prefix_number];
        dwarfstring_reset(&joind.js_buildid);
        dwarfstring_append(&joind.js_buildid,prefix);
        _dwarf_pathjoinl(&joind.js_buildid,
            &joind.js_buildid_filename);
        if (!strcmp(dwarfstring_string(&joind.js_originalfullpath),
            dwarfstring_string(&joind.js_buildid))) {
#ifdef TESTING
            printf("duplicated output string %s\n",
                dwarfstring_string(&joind.js_buildid));
#endif /* TESTING */
            /* duplicated name. spurious match. */
        } else {
            struct dwarfstring_list_s *now_last = 0;
            res = dwarfstring_list_add_new(
                &base_dwlist,
                last_entry,&joind.js_buildid,
                &now_last,errcode);
            if (res != DW_DLV_OK) {
                dwarfstring_list_destructor(&base_dwlist);
                destruct_js(&joind);
                return res;
            }
            last_entry = now_last;
        }
    }
    if (link_string_in) {
        /* js_cwd is a leading / directory name. */
        {
            dwarfstring_reset(&joind.js_tmp);
            dwarfstring_append(&joind.js_tmp,
                dwarfstring_string(&joind.js_cwd));
            /* If we add basename do we find what we look for? */
            res = _dwarf_pathjoinl(&joind.js_tmp,&joind.js_basename);
            if (!strcmp(dwarfstring_string(
                &joind.js_originalfullpath),
                dwarfstring_string(&joind.js_tmp))) {
#ifdef TESTING
                printf("duplicated output string %s\n",
                    dwarfstring_string(&joind.js_tmp));
#endif /* TESTING */
                /* duplicated name. spurious match. */
            } else if (res == DW_DLV_OK) {
                struct dwarfstring_list_s *now_last = 0;
                res = dwarfstring_list_add_new(
                    &base_dwlist,
                    last_entry,&joind.js_tmp,
                    &now_last,errcode);
                if (res != DW_DLV_OK) {
                    dwarfstring_list_destructor(&base_dwlist);
                    destruct_js(&joind);
                    return res;
                }
                last_entry = now_last;
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
                res = _dwarf_pathjoinl(&joind.js_tmp2,
                    &joind.js_basename);
                /*  this the second search path
                    after global directories
                    search for nn/nnnnn....debug.   */
                if (!strcmp(dwarfstring_string(
                    &joind.js_originalfullpath),
                    dwarfstring_string(&joind.js_tmp2))) {
#ifdef TESTING
                printf("duplicated output string %s\n",
                    dwarfstring_string(&joind.js_tmp2));
#endif /* TESTING */
                    /* duplicated name. spurious match. */
                } else if (res == DW_DLV_OK) {
                    struct dwarfstring_list_s *now_last = 0;
                    res = dwarfstring_list_add_new(
                        &base_dwlist,
                        last_entry,&joind.js_tmp2,
                        &now_last,errcode);
                    if (res != DW_DLV_OK) {
                        dwarfstring_list_destructor(&base_dwlist);
                        destruct_js(&joind);
                        return res;
                    }
                    last_entry = now_last;
                }
            }
        }
        /*  Not found above, now look in the global locations. */
        for (global_prefix_number = 0;
            global_prefix_number < length_global_prefixes_in;
            ++global_prefix_number) {
            char * prefix = global_prefixes_in[global_prefix_number];

            dwarfstring_reset(&joind.js_tmp3);
            dwarfstring_append(&joind.js_tmp3, prefix);
            res = _dwarf_pathjoinl(&joind.js_tmp3, &joind.js_cwd);
            if (res == DW_DLV_OK) {
                res = _dwarf_pathjoinl(&joind.js_tmp3,
                    &joind.js_basename);
                if (!strcmp(dwarfstring_string(
                    &joind.js_originalfullpath),
                    dwarfstring_string(&joind.js_tmp3))) {
                    /* duplicated name. spurious match. */
#ifdef TESTING
                    printf("duplicated output string %s\n",
                        dwarfstring_string(&joind.js_tmp3));
#endif /* TESTING */
                } else if (res == DW_DLV_OK) {
                    struct dwarfstring_list_s *now_last = 0;
                    res = dwarfstring_list_add_new(
                        &base_dwlist,
                        last_entry,&joind.js_tmp3,
                        &now_last,errcode);
                    if (res != DW_DLV_OK) {
                        dwarfstring_list_destructor(&base_dwlist);
                        destruct_js(&joind);
                        return res;
                    }
                    last_entry = now_last;
                }
            }
        }
    }

    /* Now use the list, if any, to make the output area. */
    {
        struct dwarfstring_list_s *cur = 0;
        char **resultfullstring = 0;

        unsigned long count = 0;
        unsigned long pointerarraysize = 0;
        unsigned long sumstringlengths = 0;
        unsigned long totalareasize = 0;
        unsigned long setptrindex = 0;
        unsigned long setstrindex = 0;

        cur = &base_dwlist;
        for ( ; cur ; cur = cur->dl_next) {
            ++count;
            pointerarraysize += sizeof(void *);
            sumstringlengths +=
                dwarfstring_strlen(&cur->dl_string) +1;
        }
        /*  Make a final null pointer in the pointer array. */
        pointerarraysize += sizeof(void *);
        totalareasize = pointerarraysize + sumstringlengths +8;
        resultfullstring = (char **)malloc(totalareasize);
        setstrindex = pointerarraysize;
        if (!resultfullstring) {
#ifdef TESTING
            printf("Malloc fail making final paths. Length "
                LONGESTUFMT " bytes.\n",
                (Dwarf_Unsigned)totalareasize);
#endif /* TESTING */
            dwarfstring_list_destructor(&base_dwlist);
            destruct_js(&joind);
            *errcode = DW_DLE_ALLOC_FAIL;
            return DW_DLV_ERROR;
        }
        memset(resultfullstring,0,totalareasize);
        cur = &base_dwlist;

        for ( ; cur ; cur = cur->dl_next,++setptrindex) {
            char **iptr = (char **)((char *)resultfullstring +
                setptrindex*sizeof(void *));
            char *sptr = (char*)resultfullstring + setstrindex;

            strcpy(sptr,dwarfstring_string(&cur->dl_string));
            setstrindex += dwarfstring_strlen(&cur->dl_string)+1;
            *iptr = sptr;
        }
        *paths_out = resultfullstring;
        *paths_out_length = count;
    }
    dwarfstring_list_destructor(&base_dwlist);
    destruct_js(&joind);
    return DW_DLV_OK;
}

static int
extract_debuglink(UNUSEDARG elf_filedata ep,
    struct generic_shdr* linkshdr,
    char ** name_returned,  /* static storage, do not free */
    unsigned char ** crc_returned,   /* 32bit crc , do not free */
    int *errcode)
{
    char *ptr = 0;
    char *endptr = 0;
    unsigned namelen = 0;
    unsigned m = 0;
    unsigned incr = 0;
    unsigned char *crcptr = 0;
    int res = DW_DLV_ERROR;
    Dwarf_Unsigned secsize = 0;
    Dwarf_Unsigned secoffset = 0;

    secsize = linkshdr->gh_size;
    if (!linkshdr->gh_content) {
        char *secdata = 0;

        secdata = malloc(secsize+1);
        if (!secdata) {
            P("Error  malloc fail:  size "
                LONGESTXFMT " line %d %s\n",secsize,
                __LINE__,__FILE__);
            *errcode = DW_DLE_ALLOC_FAIL;
            return DW_DLV_ERROR;
        }
        linkshdr->gh_content = secdata;
    }
    ptr = linkshdr->gh_content;
    secoffset = linkshdr->gh_offset;
    res = RRMOA(ep->f_fd,ptr,secoffset,secsize,
        ep->f_filesize,errcode);
    ptr[secsize] = 0;
    if (!secsize) {
        P("Section size of .gnu.debuglink is zero");
        return DW_DLV_NO_ENTRY;
    }
    if (res != DW_DLV_OK) {
        P("Read  " LONGESTUFMT
            " bytes .gnu_debuglink section failed\n",
            secsize);
        P("Read  offset " LONGESTXFMT " length "
            LONGESTXFMT " off+len " LONGESTXFMT "\n",
            secoffset,secsize,secoffset + secsize);
        return res;
    }
    endptr = ptr + secsize;
    res = _dwarf_check_string_valid(ptr, ptr,
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
    crcptr = (unsigned char *)ptr +namelen +1 +incr;
    if ((crcptr +4) != (unsigned char*)endptr) {
        P("ERROR: .gnu_debuglink is not the right length: "
            " expected end " LONGESTXFMT
            " found end at  " LONGESTXFMT ")\n",
            (Dwarf_Unsigned)(uintptr_t)(crcptr+4),
            (Dwarf_Unsigned)(uintptr_t)endptr);
        *errcode = DW_DLE_CORRUPT_GNU_DEBUGLINK;
        return DW_DLV_ERROR;
    }
    *name_returned = ptr;
    *crc_returned = crcptr;
    return DW_DLV_OK;
}

int
dwarf_gnu_debuglink(elf_filedata ep,
    char ** name_returned,  /* static storage, do not free */
    unsigned char ** crc_returned,   /* 32bit crc , do not free */
    Dwarf_Unsigned *buildidtype,
    char **buildid_owner,
    Dwarf_Unsigned  *buildid_length,
    unsigned char **buildid,
    char ***  debuglink_paths_returned,
    unsigned *debuglink_paths_count,
    int*   errcode)
{
    struct generic_shdr* shdr = ep->f_shdr;
    Dwarf_Unsigned seccount = ep->f_loc_shdr.g_count;
    char * pathname = ep->f_path;
    struct generic_shdr* linkshdr = 0;
    struct generic_shdr* buildidshdr = 0;
    int buildidres = 0;
    Dwarf_Unsigned i = 0;

    for (i = 0;i < seccount; ++i,shdr++) {
        if (!shdr->gh_namestring) {
            /*  something is badly wrong!  Corrupt object. */
            P("ERROR For section " LONGESTUFMT
                " the namestring field is a null-pointer\n",i);
            continue;
        }
        if (!strcmp("<No valid Elf section strings exist>",
            shdr->gh_namestring)) {
            P("ERROR For section " LONGESTUFMT
                " the namestring field is %s"
                " so no debuglink or debug-id section"
                " can be found in any section.\n",
                i, shdr->gh_namestring);
            return DW_DLV_NO_ENTRY;
        }
        if (!strcmp(".gnu_debuglink",shdr->gh_namestring)) {
            linkshdr = shdr;
        } else if (!strcmp(".note.gnu.build-id",
            shdr->gh_namestring)) {
            buildidshdr = shdr;
        }
        if (linkshdr && buildidshdr) {
            break;
        }
    }
    if (!linkshdr && !buildidshdr) {
        return DW_DLV_NO_ENTRY;
    }
    if (linkshdr) {
        int linkres = 0;

        linkres = extract_debuglink(ep,linkshdr,
            name_returned,crc_returned,errcode);
        if (linkres == DW_DLV_ERROR) {
            return linkres;
        }
    }
    if (buildidshdr) {
        buildidres = extract_buildid(ep,
            buildidshdr,
            buildidtype,
            buildid_owner,
            buildid_length,
            buildid,
            errcode);
        if (buildidres == DW_DLV_ERROR) {
            return buildidres;
        }
    }

    if (pathname) {
        int res = 0;
        res =  _dwarf_construct_linkedto_path(
            ep->f_gnu_global_paths,
            ep->f_gnu_global_path_count,
            pathname,
            *name_returned,
            *crc_returned,
            *buildid_length,
            *buildid,
            debuglink_paths_returned,
            debuglink_paths_count,
            errcode);
        if (res != DW_DLV_OK) {
            return res;
        }
    } else {
        *debuglink_paths_count = 0;
    }
    return DW_DLV_OK;
}

/*  The definition of .note.gnu.buildid contents (also
    used for other GNU .note.gnu.  sections too. */
struct buildid_s {
    char bu_ownernamesize[4];
    char bu_buildidsize[4];
    char bu_type[4];
    /*  Owner name usually "GNU"
        length faked here. */
};

static int
extract_buildid(elf_filedata ep,
    struct generic_shdr* buildidshdr,
    Dwarf_Unsigned * type_returned,
    char     **owner_name_returned,
    Dwarf_Unsigned * build_id_length_returned,
    unsigned char  **build_id_returned,
    int*   errcode)
{
    char * ptr = 0;
    int res = DW_DLV_ERROR;
    struct buildid_s *bu = 0;
    char *nameptr = 0;
    char *nameend = 0;
    Dwarf_Unsigned namesize = 0;
    Dwarf_Unsigned descrsize = 0;
    Dwarf_Unsigned type = 0;
    Dwarf_Unsigned finalsize;
    Dwarf_Unsigned secsize = 0;
    Dwarf_Unsigned secoffset = 0;

    secsize = buildidshdr->gh_size;
    if (!buildidshdr->gh_content) {
        char *secdata = 0;

        /*  Adding extra byte so we can force a NUL
            in the extra byte. */
        secdata = (char *)malloc(secsize+1);
        if (!secdata) {
            P("Error  malloc fail:  size "
                LONGESTXFMT " line %d %s\n",secsize,
                __LINE__,__FILE__);
            *errcode = DW_DLE_ALLOC_FAIL;
            return DW_DLV_ERROR;
        }
        buildidshdr->gh_content = secdata;
    }
    /*  4 is arbitrary, but the size has to be at least this
        and should be more. */
    if (secsize < (sizeof(struct buildid_s)+4)) {
        P("ERROR section .note.gnu.build-id too small: "
            " section length: " LONGESTXFMT
            " minimum struct size " LONGESTXFMT  "\n",
            secsize,(Dwarf_Unsigned)sizeof(struct buildid_s));
        *errcode =  DW_DLE_CORRUPT_NOTE_GNU_DEBUGID;
        return DW_DLV_ERROR;
    }
    ptr = buildidshdr->gh_content;
    secoffset = buildidshdr->gh_offset;
    res = RRMOA(ep->f_fd,ptr,secoffset,secsize,
        ep->f_filesize,errcode);
    if (res != DW_DLV_OK) {
        P("Read  " LONGESTUFMT
            " bytes .note.gnu.build-id section failed\n",
            secsize);
        P("Read  offset " LONGESTXFMT " length "
            LONGESTXFMT " off+len " LONGESTXFMT "\n",
            secoffset,secsize,secoffset + secsize);
        return res;
    }
    /*  Forcing null terminator in the extra byte not touched
        by the read. */
    ptr[secsize] = 0;
    /*  We hold gh_content till all is closed
        as we return pointers into it
        if all goes well. */
    bu = (struct buildid_s *)ptr;
    ASNAR(ep->f_copy_word,namesize, bu->bu_ownernamesize);
    ASNAR(ep->f_copy_word,descrsize,bu->bu_buildidsize);
    ASNAR(ep->f_copy_word,type,     bu->bu_type);
    if (namesize >= secsize) {
        P("ERROR section .note.gnu.build-id name size "
            "size too big: " LONGESTUFMT "\n",
            namesize);
#if 0
        *errcode = DW_DLE_CORRUPT_NOTE_GNU_DEBUGID;
#endif
        return DW_DLV_ERROR;
    }
    if (descrsize >= secsize) { /* In case value insanely large */
        P("ERROR section .note.gnu.build-id description "
            "size error : "
            " description length : "
            " length " LONGESTUFMT "  greater than section size\n",
            descrsize);
#if 0
        *errcode = DW_DLE_CORRUPT_NOTE_GNU_DEBUGID;
        return DW_DLV_ERROR;
#endif
    }
    if ((descrsize+8) >= secsize) { /* In case a bit too large */
        P("ERROR section .note.gnu.build-id description "
            "size error : "
            " description length : "
            " length " LONGESTUFMT " greater than will "
            "fit in section\n",
            descrsize);
#if 0
        *errcode = DW_DLE_CORRUPT_NOTE_GNU_DEBUGID;
        return DW_DLV_ERROR;
#endif
    }
    if (descrsize >= DW_BUILDID_SANE_SIZE) {
        P("ERROR section .note.gnu.build-id description "
            " length : "
            " length " LONGESTUFMT
            " is unreasonable.\n",
            descrsize);
    }
    nameptr = ptr + sizeof(struct buildid_s);
    if (nameptr[namesize-1]) {
        P("ERROR section .note.gnu.build-id owner "
            "not null-terminated. "
            "Required length " LONGESTUFMT " plus NUL terminator"
            ". Forcing NUL\n",
            namesize);
        nameptr[namesize-1] = 0;
    }
    nameend = nameptr + namesize;
    res = _dwarf_check_string_valid(nameptr,
        nameptr,
        nameend,
        DW_DLE_CORRUPT_GNU_DEBUGID_STRING,
        errcode);
    if ( res != DW_DLV_OK) {
        return res;
    }
    if ((strlen(nameptr) +1) != namesize) {
        P("ERROR section .note.gnu.build-id owner "
            "string size wrong: "
            " size " LONGESTUFMT
            " required length " LONGESTUFMT "\n",
            descrsize,namesize);
        *errcode = DW_DLE_CORRUPT_GNU_DEBUGID_STRING;
        return DW_DLV_ERROR;
    }

    finalsize =   sizeof(struct buildid_s) + namesize + descrsize;
    if (finalsize > secsize) {
        P("ERROR section .note.gnu.build-id owner final "
            "size wrong: "
            "size " LONGESTUFMT
            " required length " LONGESTUFMT "\n",
            descrsize,
            finalsize);
        *errcode = DW_DLE_CORRUPT_GNU_DEBUGID_SIZE;
        return DW_DLV_ERROR;
    }
    *type_returned = type;
    *owner_name_returned = nameptr;
    *build_id_length_returned = descrsize;
    *build_id_returned = (unsigned char *)ptr +
        sizeof(struct buildid_s) + namesize;
    return DW_DLV_OK;
}
