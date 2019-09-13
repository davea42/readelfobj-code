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

/*  A lightly generalized string buffer for libdwarf.

*/


struct dwstring_s {
   char *        s_data;
   unsigned long s_size;
   unsigned long s_avail;
   unsigned char s_malloc;
};

typedef struct dwstring_s dwstring; 

int dwstring_constructor(struct dwstring_s *g);
int dwstring_constructor_fixed(struct dwstring_s *g,
    unsigned long len);
int dwstring_constructor_static(struct dwstring_s *g,
    char * space,
    unsigned long len);
void dwstring_destructor(struct dwstring_s *g);

int dwstring_append(struct dwstring_s *g,char *str);

/*  When one wants the first 'len' caracters of str
    appended. NUL termination is provided by dwstrings. */
int dwstring_append_length(struct dwstring_s *g,
    char *str,unsigned long len);

char * dwstring_string(struct dwstring_s *g);
unsigned long dwstring_strlen(struct dwstring_s *g);

