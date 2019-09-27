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

int _dwarf_construct_linkedto_path(
   char         **global_prefixes_in,
   unsigned       length_global_prefixes_in,
   char          *pathname_in,
   char          *link_string_in, /* from debug link */
   unsigned char  *crc_in, /* from debug_link, 4 bytes */
   unsigned       builid_length, /* from gnu buildid */
   unsigned char *builid, /* from gnu buildid */
   char        ***paths_out,
   unsigned      *paths_out_length,
   int *errcode);

/*  Reads the gnu buildid and debuglink sections, if they
    exist. If neither, returns DW_DLV_NO_ENTRY */
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
    int*   errcode);

int dwarf_gnu_buildid(elf_filedata ep,
    Dwarf_Unsigned * type_returned,
    const char     **owner_name_returned,
    Dwarf_Unsigned * build_id_length_returned,
    const unsigned char  **build_id_returned,
    int*   errcode);

int dwarf_add_debuglink_global_path(elf_filedata ep,
    const char * pathname,
    int * errcode);
