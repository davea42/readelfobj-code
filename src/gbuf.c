/*
Copyright (c) 2019-2019, David Anderson
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

/*  A lighly generalized data buffer.
    Works for more than just strings,
    but has features (such as ensuring
    data always has a NUL byte following
    the data area used) most useful for C strings.
*/
/*
struct gbuf_s {
   char *gb_data;
   unsigned long gb_size;
   unsigned long gb_avail;
   unsigned char gb_malloc;
size does not count the NUL byte terminating the data.
avail is available bytes following the data.
};
*/

#include <stdio.h> /* for malloc */
#include <stdlib.h> /* for malloc */
#include <string.h> /* for strlen */
#include "gbuf.h"
#ifndef TRUE
#define TRUE 1
#endif /* TRUE */
#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

static unsigned long minimumnewlen = 30;
int 
gbuf_constructor(struct gbuf_s *g)
{
    g->gb_data = "";
    g->gb_size = 0;
    g->gb_avail = 0;
    g->gb_malloc = FALSE;
    return TRUE;
}


static int
gbuf_resize_to(struct gbuf_s *g,unsigned long newlen)
{
    char *b = 0;
    unsigned long lastpos = 
        g->gb_size - g->gb_avail;
    unsigned long malloclen = newlen+1;
    
    if(malloclen < minimumnewlen) {
         malloclen = minimumnewlen;
    }
    b = malloc(malloclen);
    if (!b) {
        return FALSE;
    }
    if (lastpos > 0) {
        memcpy(b,g->gb_data,lastpos);
    }
    if (g->gb_malloc) {
        free(g->gb_data);
        g->gb_data = 0;
    }
    g->gb_data = b;
    g->gb_data[lastpos] = 0;
    g->gb_size = newlen;
    g->gb_avail = newlen - lastpos;
    g->gb_malloc = TRUE;
    return TRUE;
}

int 
gbuf_constructor_fixed(struct gbuf_s *g,unsigned long len)
{
    char *b = 0;
    int r = 0;

    gbuf_constructor(g);
    if (len == 0) {
        return TRUE;
    }
    r = gbuf_resize_to(g,len);
    if (!r) {
        return FALSE;
    }
    return TRUE;
}
void 
gbuf_destructor(struct gbuf_s *g)
{
    if (g->gb_malloc) {
        free(g->gb_data);
        g->gb_data = 0;
    }
    gbuf_constructor(g);
}

int 
gbuf_append(struct gbuf_s *g,char *str)
{
    unsigned long dlen = strlen(str);
    unsigned long lastpos = g->gb_size - g->gb_avail;
    int r = 0;

    if (dlen >= g->gb_avail) {
        unsigned long newlen = 0;

        newlen = g->gb_size + dlen+1;
        r = gbuf_resize_to(g,newlen);
        if (!r) {
            return FALSE;
        }
    }
    strcpy(g->gb_data + lastpos,str);
    g->gb_avail -= dlen;
    return TRUE;
}

char * 
gbuf_get_data(struct gbuf_s *g)
{
    return g->gb_data;
}

unsigned long
gbuf_get_data_len(struct gbuf_s *g)
{
    return g->gb_size - g->gb_avail;
}

