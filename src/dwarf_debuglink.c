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
    dwarfstring_constructor(&js.js_dirname);
    dwarfstring_constructor(&js.js_basename);
    dwarfstring_constructor(&js.js_cwd);
    dwarfstring_constructor(&js.js_originalfullpath);
    dwarfstring_constructor(&js.js_tmp);
    dwarfstring_constructor(&js.js_tmp2);
    dwarfstring_constructor(&js.js_tmpdeb);
    dwarfstring_constructor(&js.js_tmp3);
}
static void
destruct_js(struct joins_s * js)
{
    dwarfstring_destructor(&js.js_dirname);
    dwarfstring_destructor(&js.js_basename);
    dwarfstring_destructor(&js.js_cwd);
    dwarfstring_destructor(&js.js_originalfullpath);
    dwarfstring_destructor(&js.js_tmp);
    dwarfstring_destructor(&js.js_tmp2);
    dwarfstring_destructor(&js.js_tmpdeb);
    dwarfstring_destructor(&js.js_tmp3);
}

static char joinchar = '/';
static char* joinstr = "/";
static int
pathjoinl(dwarfstring *target,dwarfstring * input)
{
    char *inputs = dwarfstring_string(input);
    char *targ = dwarfstring_string(target);
    size_t targlen = 0;

    if (!dwarfstring_strlen(target)) {
        if (*inputs != joinchar) {
            dwarfstring_append(target,joinstr);
            dwarfstring_append(input);
        } else {
            dwarfstring_append(target,input);
        }
    }
    targlen = dwarf_strlen(target);
    targ = target[dwarf_strlen(
    if (targ[targlen-1] != joinchar) {
        if (*input != joinchar) {
            dwarfstring_append(target,joinstr);
            dwarfstring_append(input);
        } else {
            dwarfstring_append(target,input);
        }
    } else {
        if (*inputs != joinchar) {
            dwarfstring_append(target,input);
        } else {
            dwarfstring_append(target,input+1);
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
static void
construct_linkedto_path(char *pathname, /* incoming object path */
   char * input_link_string, /* incoming link string */
   dwarfstring * debuglink_out)
{
    char * depath = pathname,
    size_t depathlen = strlen(depath)+1;
    size_t maxlen = 0;
    char * tname = 0;
    int res = 0;
    unsigned buflen= 2000;
    char  buffer[2000];
    unsigned buflen= sizeof(buffer);
    struct joins_s joind;
    size_t dirnamelen = 0;

    construct_js(&joind);
    dirnamelen = mydirlen(depath);
    if (dirnamelen) {
        dwarfstring_append_len(&joind.js_dirname,depath,depathlen);
    }
    dwarfstring_append(&jjoind.js_basename,input_link_string); 

    if (depath[0] != joinchar) {
        char *wdret = 0;

        wdret = getcwd(buffer,buflen);
        if (!wdret) {
            destruct_js(&joind);
            *debuglink_length = 0;
            return;
        }
        dwarfstring_append(&joind.js_cwd,buffer);
        buffer[0] = 0;
    }

    {
        dwarfstring_append(&joind.js_originalfullpath,
            dwarfstring_string(&joind.js_cwd));
        res =  pathjoinl(&joind.js_originalfullpath,
            &joind.js_dirname);
        if (res != DW_DLV_OK) {
            destruct_js(&joind);
            *debuglink_length = 0;
            return;
        }
    }
    /*  We need to be sure there is no accidental
        match with the file we opened. */
    if (dwarfstring_strlen(&joind.js_dirname)) {
        res = pathjoinl(&joind.js_cwd,
            &joind.js_dirname);
    } else {
        res = DW_DLV_OK;
    }
    /* Now js_cwd is a leading / directory name. */
    joinbaselen = dwarfstring_strlen(&joind.js_cwd);
    if (res == DW_DLV_OK) {
        dwarfstring_reset(&joind.js_tmp);
        dwarfstring_append(&joind.js_tmp,&joind.js_cwd);
        /* If we add basename do we find what we look for? */
        res = pathjoinl(&joind.js_tmp,&joind.js_basename);
        if (!strcmp(dwarfstring_string(&joind.js_originalfullpath),
            dwarfstring_string(&joind.js_cwd))) {
            /* duplicated name. spurious match. */
        } else if (res == DW_DLV_OK) {
            res = does_file_exist(dwarfstring_string(&joind.js_cwd));
            if (res == DW_DLV_OK) {
                dwarfstring_append(dblink_out,
                     dwarfstring_string(&joind.js_tmp)); 
                destruct_js(&joind);
                return;
            }
        }
        dwarfstring_destructor(&tmp);
    }
    {

        dwarfstring_reset(&joind.js_tmp2);
        dwarfstring_reset(&joind.js_tmpdeb);
        dwarfstring_append(&joind.js_tmp2,&joind.js_cwd);
        dwarfstring_append(&joind.js_tmpdeb,".debug");
        res = pathjoinl(&tmp2,&tmpdeb);
        if (res == DW_DLV_OK) {
            res = pathjoinl(&tmp2,&joind.js_basename);
            if (!strcmp(dwarfstring_string(&joind.js_originalfullpath),
                dwarfstring_string(&joind.js_tmp2)) {
                /* duplicated name. spurious match. */
            } else if(res == DW_DLV_OK) {
                res = does_file_exist(dwarfstring_string(&joind.js_tmp2));
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
        dwarfstring_reset(&tmp3);
        dwarfstring_append(&tmp3, "/usr/lib/debug");
        res = pathjoinl(&joind.js_tmp3, &joind.js_cwd);
        if (res == DW_DLV_OK) {
            res = pathjoinl(&joind.js_tmp3,&joind.js_basename));
            if (!strcmp(joind.js_originalfullpath,joind.js_tmp3)) {
                /* duplicated name. spurious match. */
            } else if (res == DW_DLV_OK) {
                res = does_file_exist(&joind.js_tmp3);
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

FIXME
int
dwarf_gnu_debuglink(char * pathname_in,
    char ** name_returned,  /* static storage, do not free */
    char ** crc_returned, /* 32bit crc , do not free */
    char **  debuglink_path_returned, /* caller must free returned pointer */
    unsigned *debuglink_path_size_returned,/* Size of the debuglink path.
        zero returned if no path known/found. */
    Dwarf_Error*   error)
{
    char *ptr = 0;
    char *endptr = 0;
    unsigned namelen = 0;
    unsigned m = 0;
    unsigned incr = 0;
    char *crcptr = 0;
    int res = DW_DLV_ERROR;

    if (!dbg->de_gnu_debuglink.dss_data) {
        res = _dwarf_load_section(dbg,
            &dbg->de_gnu_debuglink,error);
        if (res != DW_DLV_OK) {
            return res;
        }
    }
    ptr = (char *)dbg->de_gnu_debuglink.dss_data;
    endptr = ptr + dbg->de_gnu_debuglink.dss_size;
    res = _dwarf_check_string_valid(dbg,ptr,
        ptr,
        endptr,
        DW_DLE_FORM_STRING_BAD_STRING,
        error);
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
        _dwarf_error(dbg,error,DW_DLE_CORRUPT_GNU_DEBUGLINK);
        return DW_DLV_ERROR;
    }
    if (dbg->de_path) {
        dwarfstring outpath;
        unsigned pathlength = 0;

        dwarfstring_constructor(&outpath);
        construct_linkedto_path(pathname_in,ptr,&outpath);
        pathlength = dwarfstring_strlen(&outpath);
        *debuglink_path_returned = malloc(pathlength +1);
        if( !*debuglink_path_returned) {
             FIXME OUT OF MEMORY
        }
        strcpy(*debuglink_path_returned, dwarfstring_string(&outpath);
        *debuglink_path_size_returned = dwarfstring_strlen(&outpath);
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
dwarf_gnu_buildid(Dwarf_Debug dbg,
    Dwarf_Unsigned * type_returned,
    const char     **owner_name_returned,
    Dwarf_Unsigned * build_id_length_returned,
    const unsigned char  **build_id_returned,
    Dwarf_Error*   error)
{
    Dwarf_Byte_Ptr ptr = 0;
    Dwarf_Byte_Ptr endptr = 0;
    int res = DW_DLV_ERROR;
    struct buildid_s *bu = 0;
    Dwarf_Unsigned namesize = 0;
    Dwarf_Unsigned descrsize = 0;
    Dwarf_Unsigned type = 0;

    if (!dbg->de_note_gnu_buildid.dss_data) {
        res = _dwarf_load_section(dbg,
            &dbg->de_note_gnu_buildid,error);
        if (res != DW_DLV_OK) {
            return res;
        }
    }
    ptr = (Dwarf_Byte_Ptr)dbg->de_note_gnu_buildid.dss_data;
    endptr = ptr + dbg->de_note_gnu_buildid.dss_size;

    if (dbg->de_note_gnu_buildid.dss_size <
        sizeof(struct buildid_s)) {
        _dwarf_error(dbg,error,
            DW_DLE_CORRUPT_NOTE_GNU_DEBUGID);
        return DW_DLV_ERROR;
    }

    bu = (struct buildid_s *)ptr;
    READ_UNALIGNED_CK(dbg,namesize,Dwarf_Unsigned,
        (Dwarf_Byte_Ptr)&bu->bu_ownernamesize[0], 4,
        error,endptr);
    READ_UNALIGNED_CK(dbg,descrsize,Dwarf_Unsigned,
        (Dwarf_Byte_Ptr)&bu->bu_buildidsize[0], 4,
        error,endptr);
    READ_UNALIGNED_CK(dbg,type,Dwarf_Unsigned,
        (Dwarf_Byte_Ptr)&bu->bu_type[0], 4,
        error,endptr);

    if (descrsize != 20) {
        _dwarf_error(dbg,error,DW_DLE_CORRUPT_NOTE_GNU_DEBUGID);
        return DW_DLV_ERROR;
    }
    res = _dwarf_check_string_valid(dbg,&bu->bu_owner[0],
        &bu->bu_owner[0],
        endptr,
        DW_DLE_CORRUPT_GNU_DEBUGID_STRING,
        error);
    if ( res != DW_DLV_OK) {
        return res;
    }
    if ((strlen(bu->bu_owner) +1) != namesize) {
        _dwarf_error(dbg,error,DW_DLE_CORRUPT_GNU_DEBUGID_STRING);
        return DW_DLV_ERROR;
    }

    if ((sizeof(struct buildid_s)-1 + namesize + descrsize) >
        dbg->de_note_gnu_buildid.dss_size) {
        _dwarf_error(dbg,error,DW_DLE_CORRUPT_GNU_DEBUGID_SIZE);
        return DW_DLV_ERROR;
    }
    *type_returned = type;
    *owner_name_returned = &bu->bu_owner[0];
    *build_id_length_returned = descrsize;
    *build_id_returned = ptr + sizeof(struct buildid_s)-1 + namesize;
    return DW_DLV_OK;
}
