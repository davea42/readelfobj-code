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
struct dwstring_s {
   char *        s_data;
   unsigned long s_size;
   unsigned long s_avail;
   unsigned char s_malloc;
};
*/

#include <stdio.h> /* for malloc */
#include <stdlib.h> /* for malloc */
#include <string.h> /* for strlen */
#include "dwstring.h"
#ifndef TRUE
#define TRUE 1
#endif /* TRUE */
#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

static unsigned long minimumnewlen = 30;
int 
dwstring_constructor(struct dwstring_s *g)
{
    g->s_data = "";
    g->s_size = 0;
    g->s_avail = 0;
    g->s_malloc = FALSE;
    return TRUE;
}


static int
dwstring_resize_to(struct dwstring_s *g,unsigned long newlen)
{
    char *b = 0;
    unsigned long lastpos = 
        g->s_size - g->s_avail;
    unsigned long malloclen = newlen+1;
    
    if(malloclen < minimumnewlen) {
         malloclen = minimumnewlen;
    }
    b = malloc(malloclen);
    if (!b) {
        return FALSE;
    }
    if (lastpos > 0) {
        memcpy(b,g->s_data,lastpos);
    }
    if (g->s_malloc) {
        free(g->s_data);
        g->s_data = 0;
    }
    g->s_data = b;
    g->s_data[lastpos] = 0;
    g->s_size = newlen;
    g->s_avail = newlen - lastpos;
    g->s_malloc = TRUE;
    return TRUE;
}

int 
dwstring_constructor_fixed(struct dwstring_s *g,unsigned long len)
{
    char *b = 0;
    int r = 0;

    dwstring_constructor(g);
    if (len == 0) {
        return TRUE;
    }
    r = dwstring_resize_to(g,len);
    if (!r) {
        return FALSE;
    }
    return TRUE;
}
void 
dwstring_destructor(struct dwstring_s *g)
{
    if (g->s_malloc) {
        free(g->s_data);
        g->s_data = 0;
    }
    dwstring_constructor(g);
}

int 
dwstring_append(struct dwstring_s *g,char *str)
{
    unsigned long dlen = strlen(str);
    unsigned long lastpos = g->s_size - g->s_avail;
    int r = 0;

    if (dlen >= g->s_avail) {
        unsigned long newlen = 0;

        newlen = g->s_size + dlen+1;
        r = dwstring_resize_to(g,newlen);
        if (!r) {
            return FALSE;
        }
    }
    strcpy(g->s_data + lastpos,str);
    g->s_avail -= dlen;
    return TRUE;
}

char * 
dwstring_string(struct dwstring_s *g)
{
    return g->s_data;
}

unsigned long
dwstring_strlen(struct dwstring_s *g)
{
    return g->s_size - g->s_avail;
}

