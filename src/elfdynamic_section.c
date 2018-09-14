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

#include "config.h"
#include <stdio.h>
#include <a.out.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <elf.h>
#include "reading.h"
#include "readelfobj.h"
#include "sanitized.h"

static char buffer6[BUFFERSIZE];

static int
generic_dyn_from_dyn32(struct generic_dynentry **gbuffer_io,
    LONGESTUTYPE *bufcount_out,
    LONGESTUTYPE offset,
    LONGESTUTYPE size,
    LONGESTUTYPE ecount)
{
    Elf32_Dyn *ebuf = 0;
    Elf32_Dyn *orig_ebuf = 0;
    struct generic_dynentry * gbuffer = 0;
    struct generic_dynentry * orig_gbuffer = 0;
    LONGESTUTYPE i = 0;
    LONGESTUTYPE trueoff = 0;
    int res = 0;


    ebuf = malloc(size);
    *bufcount_out = 0;
    if (!ebuf) {
        P("Out Of Memory, cannot malloc dynamic section space for "
            LONGESTUFMT " bytes\n",size);
        return RO_ERR_MALLOC;
    }
    orig_ebuf = ebuf;
    gbuffer= malloc(sizeof(struct generic_dynentry)*ecount);
    if(!gbuffer) {
        free(ebuf);
        P("Out Of Memory, cannot malloc generic dynamic space for "
            LONGESTUFMT " bytes\n",
            (sizeof(struct generic_dynentry)*ecount));
        return RO_ERR_MALLOC;
    }
    orig_gbuffer = gbuffer;
    trueoff = offset;
    res = RR(ebuf,offset,size);
    if(res != RO_OK) {
        P("could not read whole dynamic section of %s "
        "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
        sanitized(filename,buffer6,BUFFERSIZE),
        offset,size);
        free(gbuffer);
        free(ebuf);
        return res;
    }
    for(i = 0; i < ecount;
        ++i,++gbuffer,++ebuf,trueoff += sizeof(Elf32_Dyn)) {
        ASSIGN(gbuffer->gd_tag,ebuf->d_tag);
        ASSIGN(gbuffer->gd_val,ebuf->d_un.d_val);
        /* Assigning the file offset, not sec offset */
        ASSIGN(gbuffer->gd_dyn_file_offset, trueoff);
        if (gbuffer->gd_tag == 0) {
            filedata.f_wasted_dynamic_count++;
            filedata.f_wasted_dynamic_space += sizeof(Elf32_Dyn);
        }
    }
    *bufcount_out = ecount;
    *gbuffer_io = orig_gbuffer;
    free(orig_ebuf);
    return RO_OK;
}


static int
generic_dyn_from_dyn64(struct generic_dynentry **gbuffer_io,
    LONGESTUTYPE *bufcount_out,
    LONGESTUTYPE offset,
    LONGESTUTYPE size,
    LONGESTUTYPE ecount)
{
    Elf64_Dyn *ebuf = 0;
    Elf64_Dyn *orig_ebuf = 0;
    struct generic_dynentry * gbuffer = 0;
    struct generic_dynentry * orig_gbuffer = 0;
    LONGESTUTYPE i = 0;
    LONGESTUTYPE trueoff = 0;
    int res = 0;


    ebuf = malloc(size);
    *bufcount_out = 0;
    if (!ebuf) {
        P("Out Of Memory, cannot malloc dynamic section space for "
            LONGESTUFMT " bytes\n",size);
        return RO_ERR_MALLOC;
    }
    orig_ebuf = ebuf;
    gbuffer= malloc(sizeof(struct generic_dynentry)*ecount);
    if(!gbuffer) {
        free(ebuf);
        P("Out Of Memory, cannot malloc generic dynamic space for "
            LONGESTUFMT " bytes\n",
            (sizeof(struct generic_dynentry)*ecount));
        return RO_ERR_MALLOC;
    }
    orig_gbuffer = gbuffer;
    trueoff = offset;
    res = RR(ebuf,offset,size);
    if(res != RO_OK) {
        P("could not read whole dynamic section of %s "
        "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
        filename,
        offset,size);
        free(gbuffer);
        free(ebuf);
        return res;
    }
    for(i = 0; i < ecount;
        ++i,++gbuffer,++ebuf,trueoff += sizeof(Elf64_Dyn)) {
        ASSIGN(gbuffer->gd_tag,ebuf->d_tag);
        ASSIGN(gbuffer->gd_val,ebuf->d_un.d_val);
        /* Assigning the file offset, not sec offset */
        ASSIGN(gbuffer->gd_dyn_file_offset, trueoff);
        if (gbuffer->gd_tag == 0) {
            filedata.f_wasted_dynamic_count++;
            filedata.f_wasted_dynamic_space += sizeof(Elf64_Dyn);
        }
    }
    *bufcount_out = ecount;
    *gbuffer_io = orig_gbuffer;
    free(orig_ebuf);
    return RO_OK;
}


int
elf_load_dynamic32(LONGESTUTYPE offset,LONGESTUTYPE size)
{
    LONGESTUTYPE ecount = 0;
    LONGESTUTYPE size2 = 0;
    struct generic_dynentry *gbuffer;
    LONGESTUTYPE bufcount = 0;
    int res = 0;

    if ((offset > filedata.f_filesize)||
        (size > filedata.f_filesize) ) {
        P("Something badly wrong with dynamic section "
            " filesize " LONGESTUFMT
            " section content size " LONGESTUFMT
            "\n", filedata.f_filesize,size);
        return RO_ERR;
    }

    ecount = size/(LONGESTUTYPE)sizeof(Elf32_Dyn);
    size2 = ecount * sizeof(Elf32_Dyn);
    if(size != size2) {
        P("Bogus size of dynamic. "
            LONGESTUFMT " not divisible by %lu\n",
            size,(unsigned long)sizeof(Elf32_Dyn));
        return RO_ERR;
    }
    res = generic_dyn_from_dyn32(&gbuffer,&bufcount,offset,size,ecount);
    if (res != RO_OK) {
        return res;
    }
    if(!bufcount) {
        return RO_ERR;
    }
    filedata.f_dynamic = gbuffer;
    filedata.f_loc_dynamic.g_name = ".dynamic";
    filedata.f_loc_dynamic.g_offset = offset;
    filedata.f_loc_dynamic.g_entrysize = sizeof(Elf32_Dyn);
    filedata.f_loc_dynamic.g_count = ecount;
    filedata.f_loc_dynamic.g_totalsize = ecount *sizeof(Elf32_Dyn);
    return RO_OK;
}

int
elf_load_dynamic64(LONGESTUTYPE offset,LONGESTUTYPE size)
{
    LONGESTUTYPE ecount = 0;
    LONGESTUTYPE size2 = 0;
    struct generic_dynentry *gbuffer;
    LONGESTUTYPE bufcount = 0;
    int res = 0;

    if ((offset > filedata.f_filesize)||
        (size > filedata.f_filesize) ) {
        P("Something badly wrong with dynamic section "
            " filesize " LONGESTUFMT
            " section content size " LONGESTUFMT
            "\n", filedata.f_filesize,size);
        return RO_ERR;
    }

    ecount = size/(LONGESTUTYPE)sizeof(Elf64_Dyn);
    size2 = ecount * sizeof(Elf64_Dyn);
    if(size != size2) {
        P("Bogus size of dynamic. "
            LONGESTUFMT " not divisible by %lu\n",
            size,(unsigned long)sizeof(Elf64_Dyn));
        return RO_ERR;
    }
    res = generic_dyn_from_dyn64(&gbuffer,&bufcount,offset,size,ecount);
    if (res != RO_OK) {
        return res;
    }
    if(!bufcount) {
        return RO_ERR;
    }
    filedata.f_dynamic = gbuffer;
    filedata.f_loc_dynamic.g_name = ".dynamic";
    filedata.f_loc_dynamic.g_offset = offset;
    filedata.f_loc_dynamic.g_entrysize = sizeof(Elf64_Dyn);
    filedata.f_loc_dynamic.g_count = ecount;
    filedata.f_loc_dynamic.g_totalsize = ecount *sizeof(Elf64_Dyn);
    return RO_OK;
}


int
elf_print_dynamic(void)
{
    LONGESTUTYPE bufcount = 0;
    LONGESTUTYPE i = 0;
    struct generic_dynentry *gbuffer = 0;
    struct generic_shdr *dynamicsect = 0;

    if (!filedata.f_dynamic_sect_index) {
        P("No .dynamic section exists in %s\n",
            sanitized(filename,buffer6,BUFFERSIZE));
        return RO_OK;
    }
    if (filedata.f_dynamic_sect_index >= filedata.f_ehdr->ge_shnum) {
        P("Section Number of .dynamic section is bogus in %s\n",
            sanitized(filename,buffer6,BUFFERSIZE));
        return RO_ERR;
    }
    dynamicsect = filedata.f_shdr + filedata.f_dynamic_sect_index;
    bufcount = filedata.f_loc_dynamic.g_count;
    if(bufcount) {
        const char *name = sanitized(dynamicsect->gh_namestring,
            buffer6,BUFFERSIZE);

        P("\n");
        P("Section %s (" LONGESTUFMT "):"
            " Entries:" LONGESTUFMT " Offset:"
            LONGESTXFMT8  "\n",
            name,
            filedata.f_dynamic_sect_index,
            bufcount,
            filedata.f_loc_dynamic.g_offset);
        P("  name                             value\n");
    } else {
        P("No content exists in %s\n",
            sanitized(dynamicsect->gh_namestring,buffer6,BUFFERSIZE));
        return RO_ERR;
    }
    gbuffer = filedata.f_dynamic;
    for(i = 0; i < bufcount; ++i,++gbuffer) {
        const char *name = 0;

        name = get_em_dynamic_table_name(gbuffer->gd_tag,
            buffer6,BUFFERSIZE);
        P("  Tag: "
            LONGESTXFMT8 " %-15s "
            LONGESTXFMT8 " (" LONGESTUFMT ")\n",
            gbuffer->gd_tag,
            name,
            gbuffer->gd_val,
            gbuffer->gd_val);
    }
    return RO_OK;
}
