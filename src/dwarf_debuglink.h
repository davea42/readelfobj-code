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



/* These make it easy to do some simple tests. */
int _dwarf_check_string_valid(
    void *areaptr,
    void *strptr,
    void *areaendptr,
    int suggested_error,
    int *errcode);

int _dwarf_pathjoinl(dwarfstring *target,dwarfstring * input);


void _dwarf_construct_linkedto_path(char *pathname,
   char * input_link_string, /* incoming link string */
   dwarfstring * debuglink_out);
int dwarf_gnu_debuglink(elf_filedata ep,
    char ** name_returned,  /* static storage, do not free */
    char ** crc_returned,   /* 32bit crc , do not free */
    char **  debuglink_path_returned, /* caller must free
        returned pointer */
    unsigned *debuglink_path_size_returned,/* Size of the
        debuglink path.  zero returned if no path known/found. */
    int*   errcode);

int dwarf_gnu_buildid(elf_filedata ep,
    Dwarf_Unsigned * type_returned,
    const char     **owner_name_returned,
    Dwarf_Unsigned * build_id_length_returned,
    const unsigned char  **build_id_returned,
    int*   errcode);
