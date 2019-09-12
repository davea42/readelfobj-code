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

#ifndef GBUF_H
#define GBUF_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */




struct gbuf_s {
   char *gb_data;
   unsigned long gb_size;
   unsigned long gb_avail;
   unsigned char gb_malloc;
};

/*  each that has an int  return value
    returns TRUE if the action taken, or FALSE if unable. 
    any data present remains present.  */
int gbuf_constructor(struct gbuf_s *g);
int gbuf_constructor_static(struct gbuf_s *g,unsigned long len);
void gbuf_destructor(struct gbuf_s *g);
int gbuf_append(struct gbuf_s *g,char *data);

char * gbuf_get_data(struct gbuf_s *g);
unsigned long  gbuf_get_data_len(struct gbuf_s *g);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GBUF_H */
