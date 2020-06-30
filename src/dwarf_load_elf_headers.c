/*
Copyright 2018 David Anderson. All rights reserved.

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
#include <string.h> /* For memcpy etc */
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for exit(), C89 malloc */
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_MALLOC_H
/* Useful include for some Windows compilers. */
#include <malloc.h>
#endif /* HAVE_MALLOC_H */
#include <sys/types.h>   /* for open() */
#include <sys/stat.h>   /* for open() */
#include <fcntl.h>   /* for open() */
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_ELF_H
#include <elf.h>
#endif /* HAVE_ELF_H */
#include "dw_elfstructs.h"
#include "dwarf_reading.h"
#include "dwarf_object_detector.h"
#include "dwarf_object_read_common.h"
#include "readelfobj.h"
#include "sanitized.h"

#ifdef HAVE_UNUSED_ATTRIBUTE
#define  UNUSEDARG __attribute__ ((unused))
#else
#define  UNUSEDARG
#endif

#if 0
static void
dump_bytes(char * msg,Dwarf_Small * start, long len)
{
    Dwarf_Small *end = start + len;
    Dwarf_Small *cur = start;

    printf("%s len %ld ",msg,len);
    for (; cur < end; cur++) {
        printf("%02x ", *cur);
    }
    printf("\n");
}
#endif


static char buffer6[BUFFERSIZE];
static char buffer3[BUFFERSIZE];
static char buffer1[BUFFERSIZE];

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

static void
check_size(const char *name,
    Dwarf_Unsigned offset,
    size_t size,
    Dwarf_Unsigned filesize)
{
    Dwarf_Unsigned finaloff = offset + size;
    if (finaloff > filesize) {
        P("ERROR: An attempted read at offset "
            LONGESTUFMT " (" LONGESTXFMT ") %s\n",
            offset,offset,name);
        P("       of size "
            LONGESTSFMT " (" LONGESTXFMT ")\n",
            (Dwarf_Unsigned)size,(Dwarf_Unsigned)size);
        P("       exceeds the file size of "
            LONGESTUFMT " (" LONGESTXFMT ").\n",
            filesize,filesize);
    }
}

static int
is_empty_section(Dwarf_Unsigned type)
{
    if (type == SHT_NOBITS) {
        return TRUE;
    }
    if (type == SHT_NULL) {
        return TRUE;
    }
    return FALSE;
}

int
dwarf_construct_elf_access_path(const char *path,
    elf_filedata *mp,int *errcode)
{
    int fd = -1;
    int res = 0;
    elf_filedata mymp = 0;

    fd = open(path, O_RDONLY|O_BINARY);
    if (fd < 0) {
        *errcode = RO_ERR_PATH_SIZE;
        return DW_DLV_ERROR;
    }
    res = dwarf_construct_elf_access(fd,
        path,&mymp,errcode);
    if (res != DW_DLV_OK) {
        close(fd);
        return res;
    }
    mymp->f_destruct_close_fd = TRUE;
    *mp = mymp;
    return res;
}

/* Here path is not essential. Pass in with "" if unknown. */
int
dwarf_construct_elf_access(int fd,
    const char *path,
    elf_filedata *mp,int *errcode)
{
    unsigned ftype = 0;
    unsigned endian = 0;
    unsigned offsetsize = 0;
    size_t   filesize;
    elf_filedata mfp = 0;
    int      res = 0;

    res = dwarf_object_detector_fd(fd,
        &ftype,&endian,&offsetsize, &filesize, errcode);
    if (res != DW_DLV_OK) {
        return res;
    }

    mfp = calloc(1,sizeof(struct elf_filedata_s));
    if (!mfp) {
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    mfp->f_fd = fd;
    mfp->f_ident[0] = 'E';
    mfp->f_ident[1] = 1;
    mfp->f_offsetsize = offsetsize;
    mfp->f_filesize = filesize;
    mfp->f_endian = endian;
    mfp->f_path = strdup(path);
    mfp->f_destruct_close_fd = FALSE;
    *mp = mfp;
    mfp->f_gnu_global_path_count = 1;
    mfp->f_gnu_global_paths = (char **)malloc(sizeof(void *));
    if (!mfp->f_gnu_global_paths) {
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    mfp->f_gnu_global_paths[0] = strdup("/usr/lib/debug");
    return DW_DLV_OK;
}

/*  Caller must zero the passed in pointer
    after this returns to remind
    the caller to avoid use of the pointer. */
int
dwarf_destruct_elf_access(elf_filedata ep,
    UNUSEDARG int *errcode)
{
    struct generic_shdr *shp = 0;
    Dwarf_Unsigned shcount = 0;
    Dwarf_Unsigned i = 0;

    for (i = 0; i < ep->f_gnu_global_path_count;++i) {
        char *d = ep->f_gnu_global_paths[i];
        free(d);
    }
    free(ep->f_gnu_global_paths);
    ep->f_gnu_global_paths = 0;
    ep->f_gnu_global_path_count = 0;
    free(ep->f_ehdr);
    shp = ep->f_shdr;
    shcount = ep->f_loc_shdr.g_count;
    for(i = 0; i < shcount; ++i,++shp) {
        free(shp->gh_rels);
        shp->gh_rels = 0;
        free(shp->gh_content);
        shp->gh_content = 0;
        free(shp->gh_sht_group_array);
        shp->gh_sht_group_array = 0;
        shp->gh_sht_group_array_count = 0;
    }
    free(ep->f_shdr);
    free(ep->f_phdr);
    free(ep->f_elf_shstrings_data);
    free(ep->f_dynamic);
    free(ep->f_symtab_sect_strings);
    free(ep->f_dynsym_sect_strings);
    free(ep->f_symtab);
    free(ep->f_dynsym);

    /* if TRUE close f_fd on destruct.*/
    if (ep->f_destruct_close_fd) {
        close(ep->f_fd);
    }
    ep->f_ident[0] = 'X';
    free(ep->f_path);
    free(ep);
    return DW_DLV_OK;
}




static int
generic_ehdr_from_32(elf_filedata ep,
    struct generic_ehdr *ehdr, dw_elf32_ehdr *e,
    UNUSEDARG int *errcode)
{
    int i = 0;

    for (i = 0; i < EI_NIDENT; ++i) {
        ehdr->ge_ident[i] = e->e_ident[i];
    }
    ASNAR(ep->f_copy_word,ehdr->ge_type,e->e_type);
    ASNAR(ep->f_copy_word,ehdr->ge_machine,e->e_machine);
    ASNAR(ep->f_copy_word,ehdr->ge_version,e->e_version);
    ASNAR(ep->f_copy_word,ehdr->ge_entry,e->e_entry);
    ASNAR(ep->f_copy_word,ehdr->ge_phoff,e->e_phoff);
    ASNAR(ep->f_copy_word,ehdr->ge_shoff,e->e_shoff);
    ASNAR(ep->f_copy_word,ehdr->ge_flags,e->e_flags);
    ASNAR(ep->f_copy_word,ehdr->ge_ehsize,e->e_ehsize);
    ASNAR(ep->f_copy_word,ehdr->ge_phentsize,e->e_phentsize);
    ASNAR(ep->f_copy_word,ehdr->ge_phnum,e->e_phnum);
    ASNAR(ep->f_copy_word,ehdr->ge_shentsize,e->e_shentsize);
    ASNAR(ep->f_copy_word,ehdr->ge_shnum,e->e_shnum);
    ASNAR(ep->f_copy_word,ehdr->ge_shstrndx,e->e_shstrndx);
    ep->f_ehdr = ehdr;
    ep->f_machine = ehdr->ge_machine;
    ep->f_loc_ehdr.g_name = "Elf File Header";
    ep->f_loc_ehdr.g_offset = 0;
    ep->f_loc_ehdr.g_count = 1;
    ep->f_loc_ehdr.g_entrysize = sizeof(dw_elf32_ehdr);
    ep->f_loc_ehdr.g_totalsize = sizeof(dw_elf32_ehdr);
    return RO_OK;
}

static int
generic_ehdr_from_64(elf_filedata ep,
    struct generic_ehdr *ehdr, dw_elf64_ehdr *e,
    UNUSEDARG int *errcode)
{
    int i = 0;

    for (i = 0; i < EI_NIDENT; ++i) {
        ehdr->ge_ident[i] = e->e_ident[i];
    }
    ASNAR(ep->f_copy_word,ehdr->ge_type,e->e_type);
    ASNAR(ep->f_copy_word,ehdr->ge_machine,e->e_machine);
    ASNAR(ep->f_copy_word,ehdr->ge_version,e->e_version);
    ASNAR(ep->f_copy_word,ehdr->ge_entry,e->e_entry);
    ASNAR(ep->f_copy_word,ehdr->ge_phoff,e->e_phoff);
    ASNAR(ep->f_copy_word,ehdr->ge_shoff,e->e_shoff);
    ASNAR(ep->f_copy_word,ehdr->ge_flags,e->e_flags);
    ASNAR(ep->f_copy_word,ehdr->ge_ehsize,e->e_ehsize);
    ASNAR(ep->f_copy_word,ehdr->ge_phentsize,e->e_phentsize);
    ASNAR(ep->f_copy_word,ehdr->ge_phnum,e->e_phnum);
    ASNAR(ep->f_copy_word,ehdr->ge_shentsize,e->e_shentsize);
    ASNAR(ep->f_copy_word,ehdr->ge_shnum,e->e_shnum);
    ASNAR(ep->f_copy_word,ehdr->ge_shstrndx,e->e_shstrndx);
    ep->f_ehdr = ehdr;
    ep->f_machine = ehdr->ge_machine;
    ep->f_loc_ehdr.g_name = "Elf File Header";
    ep->f_loc_ehdr.g_offset = 0;
    ep->f_loc_ehdr.g_count = 1;
    ep->f_loc_ehdr.g_entrysize = sizeof(dw_elf64_ehdr);
    ep->f_loc_ehdr.g_totalsize = sizeof(dw_elf64_ehdr);
    return RO_OK;
}


static int
generic_phdr_from_phdr32(elf_filedata ep,
    struct generic_phdr **phdr_out,
    Dwarf_Unsigned * count_out,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned entsize,
    Dwarf_Unsigned count,
    int *errcode)
{
    dw_elf32_phdr *pph =0;
    dw_elf32_phdr *orig_pph =0;
    struct generic_phdr *gphdr =0;
    struct generic_phdr *orig_gphdr =0;
    Dwarf_Unsigned i = 0;
    int res = 0;

    *count_out = 0;
    pph = (dw_elf32_phdr *)calloc(count , entsize);
    if(pph == 0) {
        P("malloc of " LONGESTUFMT
            " bytes of program header space failed\n",count *entsize);
        *errcode =  RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    gphdr = (struct generic_phdr *)calloc(count,sizeof(*gphdr));
    if(gphdr == 0) {
        free(pph);
        P("malloc of " LONGESTUFMT
            " bytes of generic program header space failed\n",
            count *entsize);
        *errcode =  RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }

    orig_pph = pph;
    orig_gphdr = gphdr;
    res = RRMOA(ep->f_fd,pph,offset,count*entsize,ep->f_filesize,errcode);
    if(res != RO_OK) {
        P("Read  " LONGESTUFMT
            " bytes program headers failed\n",count*entsize);
        free(pph);
        free(gphdr);
        return res;
    }
    for( i = 0; i < count;
        ++i,  pph++,gphdr++) {
        ASNAR(ep->f_copy_word,gphdr->gp_type,pph->p_type);
        ASNAR(ep->f_copy_word,gphdr->gp_offset,pph->p_offset);
        ASNAR(ep->f_copy_word,gphdr->gp_vaddr,pph->p_vaddr);
        ASNAR(ep->f_copy_word,gphdr->gp_paddr,pph->p_paddr);
        ASNAR(ep->f_copy_word,gphdr->gp_filesz,pph->p_filesz);
        ASNAR(ep->f_copy_word,gphdr->gp_memsz,pph->p_memsz);
        ASNAR(ep->f_copy_word,gphdr->gp_flags,pph->p_flags);
        ASNAR(ep->f_copy_word,gphdr->gp_align,pph->p_align);
        if (gphdr->gp_offset ||gphdr->gp_filesz) {
            dwarf_insert_in_use_entry(ep,"Phdr target",
                gphdr->gp_offset,
                gphdr->gp_filesz,
                gphdr->gp_align);
        }
    }
    free(orig_pph);
    *phdr_out = orig_gphdr;
    *count_out = count;
    ep->f_phdr = orig_gphdr;
    ep->f_loc_phdr.g_name = "Program Header";
    ep->f_loc_phdr.g_offset = offset;
    ep->f_loc_phdr.g_count = count;
    ep->f_loc_phdr.g_entrysize = sizeof(dw_elf32_phdr);
    ep->f_loc_phdr.g_totalsize = sizeof(dw_elf32_phdr)*count;
    return RO_OK;
}

static int
generic_phdr_from_phdr64(elf_filedata ep,
    struct generic_phdr **phdr_out,
    Dwarf_Unsigned * count_out,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned entsize,
    Dwarf_Unsigned count,
    int *errcode)
{
    dw_elf64_phdr *pph =0;
    dw_elf64_phdr *orig_pph =0;
    struct generic_phdr *gphdr =0;
    struct generic_phdr *orig_gphdr =0;
    int res = 0;
    Dwarf_Unsigned i = 0;

    *count_out = 0;
    pph = (dw_elf64_phdr *)calloc(count , entsize);
    if(pph == 0) {
        P("malloc of " LONGESTUFMT
            " bytes of program header space failed\n",count *entsize);
        return RO_ERR_MALLOC;
    }
    gphdr = (struct generic_phdr *)calloc(count,sizeof(*gphdr));
    if(gphdr == 0) {
        free(pph);
        P("malloc of " LONGESTUFMT
            " bytes of generic program header space failed\n",
            count *entsize);
        return RO_ERR_MALLOC;
    }

    orig_pph = pph;
    orig_gphdr = gphdr;
    res = RRMOA(ep->f_fd,pph,offset,count*entsize,ep->f_filesize,errcode);
    if(res != RO_OK) {
        P("Read  " LONGESTUFMT
            " bytes program headers failed\n",count*entsize);
        free(pph);
        free(gphdr);
        return res;
    }
    for( i = 0; i < count;
        ++i,  pph++,gphdr++) {
        ASNAR(ep->f_copy_word,gphdr->gp_type,pph->p_type);
        ASNAR(ep->f_copy_word,gphdr->gp_offset,pph->p_offset);
        ASNAR(ep->f_copy_word,gphdr->gp_vaddr,pph->p_vaddr);
        ASNAR(ep->f_copy_word,gphdr->gp_paddr,pph->p_paddr);
        ASNAR(ep->f_copy_word,gphdr->gp_filesz,pph->p_filesz);
        ASNAR(ep->f_copy_word,gphdr->gp_memsz,pph->p_memsz);
        ASNAR(ep->f_copy_word,gphdr->gp_flags,pph->p_flags);
        ASNAR(ep->f_copy_word,gphdr->gp_align,pph->p_align);
        if (gphdr->gp_offset ||gphdr->gp_filesz) {
            dwarf_insert_in_use_entry(ep,"Phdr target",
                gphdr->gp_offset,
                gphdr->gp_filesz,
                gphdr->gp_align);
        }
    }
    free(orig_pph);
    *phdr_out = orig_gphdr;
    *count_out = count;
    ep->f_phdr = orig_gphdr;
    ep->f_loc_phdr.g_name = "Program Header";
    ep->f_loc_phdr.g_offset = offset;
    ep->f_loc_phdr.g_count = count;
    ep->f_loc_phdr.g_entrysize = sizeof(dw_elf64_phdr);
    ep->f_loc_phdr.g_totalsize = sizeof(dw_elf64_phdr)*count;
    return RO_OK;
}

static int
generic_shdr_from_shdr32(elf_filedata ep,
    Dwarf_Unsigned * count_out,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned entsize,
    Dwarf_Unsigned count,
    int *errcode)
{
    dw_elf32_shdr          *psh =0;
    dw_elf32_shdr          *orig_psh =0;
    struct generic_shdr *gshdr =0;
    struct generic_shdr *orig_gshdr =0;
    Dwarf_Unsigned i = 0;
    int res = 0;

    *count_out = 0;
    psh = (dw_elf32_shdr *)calloc(count , entsize);
    if(!psh) {
        P("malloc of " LONGESTUFMT
            " bytes of section header space failed\n",count *entsize);
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    gshdr = (struct generic_shdr *)calloc(count,sizeof(*gshdr));
    if(!gshdr) {
        free(psh);
        P("malloc of " LONGESTUFMT
            " bytes of generic section header space failed\n",
            count *entsize);
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }

    orig_psh = psh;
    orig_gshdr = gshdr;
    res = RRMOA(ep->f_fd,psh,offset,count*entsize,ep->f_filesize,errcode);
    if(res != RO_OK) {
        P("Read  " LONGESTUFMT
            " bytes section headers failed\n",count*entsize);
        free(psh);
        free(gshdr);
        return res;
    }
    for(i = 0; i < count;
        ++i,  psh++,gshdr++) {
        gshdr->gh_secnum = i;
        ASNAR(ep->f_copy_word,gshdr->gh_name,psh->sh_name);
        ASNAR(ep->f_copy_word,gshdr->gh_type,psh->sh_type);
        ASNAR(ep->f_copy_word,gshdr->gh_flags,psh->sh_flags);
        ASNAR(ep->f_copy_word,gshdr->gh_addr,psh->sh_addr);
        ASNAR(ep->f_copy_word,gshdr->gh_offset,psh->sh_offset);
        ASNAR(ep->f_copy_word,gshdr->gh_size,psh->sh_size);
        ASNAR(ep->f_copy_word,gshdr->gh_link,psh->sh_link);
        ASNAR(ep->f_copy_word,gshdr->gh_info,psh->sh_info);
        ASNAR(ep->f_copy_word,gshdr->gh_addralign,psh->sh_addralign);
        ASNAR(ep->f_copy_word,gshdr->gh_entsize,psh->sh_entsize);
        if (!is_empty_section(gshdr->gh_type)) {
            dwarf_insert_in_use_entry(ep,"Shdr target",
                gshdr->gh_offset,gshdr->gh_size,ALIGN4);
        }
    }
    free(orig_psh);
    *count_out = count;
    ep->f_shdr = orig_gshdr;
    ep->f_loc_shdr.g_name = "Section Header";
    ep->f_loc_shdr.g_count = count;
    ep->f_loc_shdr.g_offset = offset;
    ep->f_loc_shdr.g_entrysize = sizeof(dw_elf32_shdr);
    ep->f_loc_shdr.g_totalsize = sizeof(dw_elf32_shdr)*count;
    return RO_OK;
}

static int
generic_shdr_from_shdr64(elf_filedata ep,
    Dwarf_Unsigned * count_out,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned entsize,
    Dwarf_Unsigned count,
    int *errcode)
{
    dw_elf64_shdr          *psh =0;
    dw_elf64_shdr          *orig_psh =0;
    struct generic_shdr *gshdr =0;
    struct generic_shdr *orig_gshdr =0;
    Dwarf_Unsigned i = 0;
    int res = 0;

    *count_out = 0;
    psh = (dw_elf64_shdr *)calloc(count , entsize);
    if(!psh) {
        P("malloc of " LONGESTUFMT
            " bytes of section header space failed\n",count *entsize);
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    gshdr = (struct generic_shdr *)calloc(count,sizeof(*gshdr));
    if(gshdr == 0) {
        free(psh);
        P("malloc of " LONGESTUFMT
            " bytes of generic section header space failed\n",
            count *entsize);
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }

    orig_psh = psh;
    orig_gshdr = gshdr;
    res = RRMOA(ep->f_fd,psh,offset,count*entsize,ep->f_filesize,errcode);
    if(res != RO_OK) {
        P("Read  " LONGESTUFMT
            " bytes section headers failed\n",count*entsize);
        P("Read  offset " LONGESTXFMT " length "
            LONGESTXFMT " off+len " LONGESTXFMT "\n",
            offset,count*entsize,offset + (count*entsize));
        free(psh);
        free(gshdr);
        return res;
    }
    for( i = 0; i < count;
        ++i,  psh++,gshdr++) {
        ASNAR(ep->f_copy_word,gshdr->gh_name,psh->sh_name);
        ASNAR(ep->f_copy_word,gshdr->gh_type,psh->sh_type);
        ASNAR(ep->f_copy_word,gshdr->gh_flags,psh->sh_flags);
        ASNAR(ep->f_copy_word,gshdr->gh_addr,psh->sh_addr);
        ASNAR(ep->f_copy_word,gshdr->gh_offset,psh->sh_offset);
        ASNAR(ep->f_copy_word,gshdr->gh_size,psh->sh_size);
        ASNAR(ep->f_copy_word,gshdr->gh_link,psh->sh_link);
        ASNAR(ep->f_copy_word,gshdr->gh_info,psh->sh_info);
        ASNAR(ep->f_copy_word,gshdr->gh_addralign,psh->sh_addralign);
        ASNAR(ep->f_copy_word,gshdr->gh_entsize,psh->sh_entsize);
        if (!is_empty_section(gshdr->gh_type)) {
            dwarf_insert_in_use_entry(ep,"Shdr target",
                gshdr->gh_offset,gshdr->gh_size,ALIGN8);
        }
    }
    free(orig_psh);
    *count_out = count;
    ep->f_shdr = orig_gshdr;
    ep->f_loc_shdr.g_name = "Section Header";
    ep->f_loc_shdr.g_count = count;
    ep->f_loc_shdr.g_offset = offset;
    ep->f_loc_shdr.g_entrysize = sizeof(dw_elf64_shdr);
    ep->f_loc_shdr.g_totalsize = sizeof(dw_elf64_shdr)*count;
    return RO_OK;
}



static int
dwarf_generic_elf_load_symbols32(elf_filedata  ep,
    int secnum,const char *secname,
    struct generic_symentry **gsym_out,
    Dwarf_Unsigned offset,Dwarf_Unsigned size,
    Dwarf_Unsigned *count_out,int *errcode)
{
    Dwarf_Unsigned ecount = 0;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned i = 0;
    dw_elf32_sym *psym = 0;
    dw_elf32_sym *orig_psym = 0;
    struct generic_symentry * gsym = 0;
    struct generic_symentry * orig_gsym = 0;
    int res = 0;

    ecount = (long)(size/sizeof(dw_elf32_sym));
    size2 = ecount * sizeof(dw_elf32_sym);
    if(size != size2) {
        P("ERROR: Bogus size of symbols. "
            LONGESTUFMT " not divisible by %lu\n",
            size,(unsigned long)sizeof(dw_elf32_sym));
        *errcode = RO_ERR_SYMBOLSECTIONSIZE;
        return RO_ERROR;
    }
    psym = calloc(ecount,sizeof(dw_elf32_sym));
    if (!psym) {
        P("ERROR:  Unable to malloc Elf32_Sym strings for section %d (%s) "
            "at offset " LONGESTXFMT "\n",
            secnum,sanitized(secname,buffer3,BUFFERSIZE),offset);
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    gsym = calloc(ecount,sizeof(struct generic_symentry));
    if (!gsym) {
        free(psym);
        P("ERROR:  Unable to malloc generic_symentry "
            "strings for section %d (%s) "
            "at offset " LONGESTXFMT "\n",
            secnum,sanitized(secname,buffer3,BUFFERSIZE),offset);
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    res = RRMOA(ep->f_fd,psym,offset,size,ep->f_filesize,errcode);
    if(res!= RO_OK) {
        free(psym);
        free(gsym);
        P("ERROR: could not read whole symbol section of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            sanitized(filename,buffer3,BUFFERSIZE),
            offset,size);
        return res;
    }
    orig_psym = psym;
    orig_gsym = gsym;
    for ( i = 0; i < ecount; ++i,++psym,++gsym) {
        Dwarf_Unsigned bind = 0;
        Dwarf_Unsigned type = 0;

        ASNAR(ep->f_copy_word,gsym->gs_name,psym->st_name);
        ASNAR(ep->f_copy_word,gsym->gs_value,psym->st_value);
        ASNAR(ep->f_copy_word,gsym->gs_size,psym->st_size);
        ASNAR(ep->f_copy_word,gsym->gs_info,psym->st_info);
        ASNAR(ep->f_copy_word,gsym->gs_other,psym->st_other);
        ASNAR(ep->f_copy_word,gsym->gs_shndx,psym->st_shndx);
        bind = gsym->gs_info >> 4;
        type = gsym->gs_info & 0xf;
        gsym->gs_bind = bind;
        gsym->gs_type = type;
    }
    *count_out = ecount;
    *gsym_out = orig_gsym;
    free(orig_psym);
    return RO_OK;
}


static int
dwarf_generic_elf_load_symbols64(elf_filedata ep,
    int secnum,const char *secname,
    struct generic_symentry **gsym_out,
    Dwarf_Unsigned offset,Dwarf_Unsigned size,
    Dwarf_Unsigned *count_out,int *errcode)
{
    Dwarf_Unsigned ecount = 0;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned i = 0;
    dw_elf64_sym*psym = 0;
    dw_elf64_sym *orig_psym = 0;
    struct generic_symentry * gsym = 0;
    struct generic_symentry * orig_gsym = 0;
    int res = 0;

    ecount = (long)(size/sizeof(dw_elf64_sym));
    size2 = ecount * sizeof(dw_elf64_sym);
    if(size != size2) {
        P("ERROR: Bogus size of symbols. "
            LONGESTUFMT " not divisible by %lu\n",
            size,(unsigned long)sizeof(dw_elf64_sym));
        *errcode = RO_ERR_SYMBOLSECTIONSIZE;
        return RO_ERROR;
    }
    psym = calloc(ecount,sizeof(dw_elf64_sym));
    if (!psym) {
        P("ERROR:  Unable to malloc Elf64_Sym strings "
            "for section %d (%s) "
            "at offset " LONGESTXFMT "\n",
            secnum,sanitized(secname,buffer3,BUFFERSIZE),offset);
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    gsym = calloc(ecount,sizeof(struct generic_symentry));
    if (!gsym) {
        free(psym);
        P("ERROR:  Unable to malloc generic_symentry "
            "strings for section %d (%s) "
            "at offset " LONGESTXFMT "\n",
            secnum,
            sanitized(secname,buffer3,BUFFERSIZE),offset);
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    res = RRMOA(ep->f_fd,psym,offset,size,ep->f_filesize,errcode);
    if(res!= RO_OK) {
        free(psym);
        free(gsym);
        check_size("symbol section",offset,size,ep->f_filesize);
        P("ERROR: Could not read whole symbol section of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            sanitized(filename,buffer3,BUFFERSIZE),
            offset,size);
        return res;
    }
    orig_psym = psym;
    orig_gsym = gsym;
    for ( i = 0; i < ecount; ++i,++psym,++gsym) {
        Dwarf_Unsigned bind = 0;
        Dwarf_Unsigned type = 0;

        ASNAR(ep->f_copy_word,gsym->gs_name,psym->st_name);
        ASNAR(ep->f_copy_word,gsym->gs_value,psym->st_value);
        ASNAR(ep->f_copy_word,gsym->gs_size,psym->st_size);
        ASNAR(ep->f_copy_word,gsym->gs_info,psym->st_info);
        ASNAR(ep->f_copy_word,gsym->gs_other,psym->st_other);
        ASNAR(ep->f_copy_word,gsym->gs_shndx,psym->st_shndx);
        bind = gsym->gs_info >> 4;
        type = gsym->gs_info & 0xf;
        gsym->gs_bind = bind;
        gsym->gs_type = type;
    }
    *count_out = ecount;
    *gsym_out = orig_gsym;
    free(orig_psym);
    return RO_OK;
}

static int
dwarf_generic_elf_load_symbols(elf_filedata  ep,
    int secnum,const char *secname,
    struct generic_shdr *psh,
    struct generic_symentry **gsym_out,
    Dwarf_Unsigned *count_out,int *errcode)
{
    int res = 0;
    struct generic_symentry *gsym = 0;
    Dwarf_Unsigned count = 0;
    const char *namestr = 0;

    if(!secnum) {
        return DW_DLV_NO_ENTRY;
    }
    namestr = secname;
    if (ep->f_offsetsize == 32) {
        res = dwarf_generic_elf_load_symbols32(ep,
            secnum,namestr,
            &gsym,
            psh->gh_offset,psh->gh_size,
            &count,errcode);
    } else if (ep->f_offsetsize == 64) {
        res = dwarf_generic_elf_load_symbols64(ep,
            secnum,namestr,
            &gsym,
            psh->gh_offset,psh->gh_size,
            &count,errcode);
    } else {
        *errcode = RO_ERR_BADOFFSETSIZE;
        return DW_DLV_ERROR;
    }
    if (res == DW_DLV_OK) {
        *gsym_out = gsym;
        *count_out = count;
    }
    return res;
}
int
dwarf_load_elf_dynsym_symbols(elf_filedata ep, int*errcode)
{
    int res = 0;
    struct generic_symentry *gsym = 0;
    Dwarf_Unsigned count = 0;
    Dwarf_Unsigned secnum = ep->f_dynsym_sect_index;
    struct generic_shdr * psh = 0;
    const char *namestr = 0;

    if(!secnum) {
        return DW_DLV_NO_ENTRY;
    }
    psh = ep->f_shdr + secnum;
    namestr = psh->gh_namestring;
    res = dwarf_generic_elf_load_symbols(ep,
        secnum,namestr,
        psh,
        &gsym,
        &count,errcode);
    if (res == DW_DLV_OK) {
        ep->f_dynsym = gsym;
        ep->f_loc_dynsym.g_count = count;
    }
    return res;
}
int
dwarf_load_elf_symtab_symbols(elf_filedata ep, int*errcode)
{
    int res = 0;
    struct generic_symentry *gsym = 0;
    Dwarf_Unsigned count = 0;
    Dwarf_Unsigned secnum = ep->f_symtab_sect_index;
    struct generic_shdr * psh = 0;
    const char *namestr = 0;

    if(!secnum) {
        return DW_DLV_NO_ENTRY;
    }
    psh = ep->f_shdr + secnum;
    namestr = psh->gh_namestring;

    res = dwarf_generic_elf_load_symbols(ep,
        secnum,namestr,
        psh,
        &gsym,
        &count,errcode);
    if (res == DW_DLV_OK) {
        ep->f_symtab = gsym;
        ep->f_loc_symtab.g_count = count;
    }
    return res;
}

static int
generic_rel_from_rela32(elf_filedata ep,
    struct generic_shdr * gsh,
    dw_elf32_rela *relp,
    struct generic_rela *grel,
    int *errcode)
{
    Dwarf_Unsigned ecount = 0;
    Dwarf_Unsigned size = gsh->gh_size;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned i = 0;

    ecount = size/sizeof(dw_elf32_rela);
    size2 = ecount * sizeof(dw_elf32_rela);
    if(size != size2) {
        P("ERROR: Bogus size of relocations section "
            LONGESTUFMT ". "
            " not divisible by %u\n",
            size,(unsigned)sizeof(dw_elf32_rela));
        *errcode = RO_ERR_RELSECTIONSIZE;
        return  DW_DLV_ERROR;
    }
    for ( i = 0; i < ecount; ++i,++relp,++grel) {
        ASNAR(ep->f_copy_word,grel->gr_offset,relp->r_offset);
        ASNAR(ep->f_copy_word,grel->gr_info,relp->r_info);
        /* addend signed */
        ASNAR(ep->f_copy_word,grel->gr_addend,relp->r_addend);
        SIGN_EXTEND(grel->gr_addend,sizeof(relp->r_addend));
        grel->gr_isrela = TRUE;
        grel->gr_sym  = grel->gr_info >> 8; /* ELF32_R_SYM */
        grel->gr_type = grel->gr_info & 0xff; /* ELF32_R_TYPE */
    }
    return DW_DLV_OK;
}
#if 0
/*  EM_MIPS where the file is littleendian ELF 64.
    ELF64_MIPS_R_SSYM(i) (((i) >> 24) & 0xff)
    ELF64_MIPS_R_TYPE3(i) (((i) >> 16) & 0xff)
    ELF64_MIPS_R_TYPE2(i) (((i) >> 8) & 0xff)
    ELF64_MIPS_R_TYPE(i) ((i) & 0xff)
    EM_SPARCV9 is also special 64bit reloc type
*/
#endif


static int
generic_rel_from_rela64(elf_filedata ep,
    struct generic_shdr * gsh,
    dw_elf64_rela *relp,
    struct generic_rela *grel, int *errcode)
{
    Dwarf_Unsigned ecount = 0;
    Dwarf_Unsigned size = gsh->gh_size;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned i = 0;
    int objlittleendian = (ep->f_endian == DW_ENDIAN_LITTLE);

    ecount = size/sizeof(dw_elf64_rela);
    size2 = ecount * sizeof(dw_elf64_rela);
    if(size != size2) {
        P("ERROR: Bogus size of relocations section "
            LONGESTUFMT ". "
            " not divisible by %u\n",
            size,(unsigned)sizeof(dw_elf64_rela));
        *errcode = RO_ERR_RELSECTIONSIZE;
        return  DW_DLV_ERROR;
    }
    for ( i = 0; i < ecount; ++i,++relp,++grel) {
        ASNAR(ep->f_copy_word,grel->gr_offset,relp->r_offset);
        ASNAR(ep->f_copy_word,grel->gr_info,relp->r_info);
        ASNAR(ep->f_copy_word,grel->gr_addend,relp->r_addend);
        SIGN_EXTEND(grel->gr_addend,sizeof(relp->r_addend));
        if (ep->f_machine == EM_MIPS && objlittleendian ) {
            char realsym[4];

            memcpy(realsym,&relp->r_info,sizeof(realsym));
            ASNAR(ep->f_copy_word,grel->gr_sym,realsym);
            grel->gr_isrela = TRUE;
            grel->gr_type  = relp->r_info[7];
            grel->gr_type2 = relp->r_info[6];
            grel->gr_type3 = relp->r_info[5];
        } else if (ep->f_machine == EM_SPARCV9) {
            /*  Always Big Endian?  */
            char realsym[4];

            memcpy(realsym,&relp->r_info,sizeof(realsym));
            ASNAR(ep->f_copy_word,grel->gr_sym,realsym);
            grel->gr_type  = relp->r_info[7];
        } else {
            grel->gr_sym  = grel->gr_info >>32; /* ELF64_R_SYM */
            grel->gr_isrela = TRUE;
            grel->gr_type = grel->gr_info  & 0xffffffff; /* ELF64_R_TYPE */
        }
    }
    return DW_DLV_OK;
}

static int
generic_rel_from_rel32(elf_filedata ep,
    struct generic_shdr * gsh,
    dw_elf32_rel *relp,
    struct generic_rela *grel,int *errcode)
{
    Dwarf_Unsigned ecount = 0;
    Dwarf_Unsigned size = gsh->gh_size;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned i = 0;

    ecount = size/sizeof(dw_elf32_rel);
    size2 = ecount * sizeof(dw_elf32_rel);
    if(size != size2) {
        P("ERROR: Bogus size of relocations section "
            LONGESTUFMT ". "
            " not divisible by %lu\n",
            size,(unsigned long)sizeof(dw_elf32_rel));
        *errcode = RO_ERR_RELSECTIONSIZE;
        return  DW_DLV_ERROR;
    }
    for ( i = 0; i < ecount; ++i,++relp,++grel) {
        grel->gr_isrela = 0;
        ASNAR(ep->f_copy_word,grel->gr_offset,relp->r_offset);
        ASNAR(ep->f_copy_word,grel->gr_info,relp->r_info);
        grel->gr_addend  = 0; /* Unused for plain .rel */
        grel->gr_sym  = grel->gr_info >> 8; /* ELF32_R_SYM */
        grel->gr_isrela = FALSE;
        grel->gr_type = grel->gr_info & 0xff; /* ELF32_R_TYPE */
    }
    return DW_DLV_OK;
}

static int
generic_rel_from_rel64(elf_filedata ep,
    struct generic_shdr * gsh,
    dw_elf64_rel *relp,
    struct generic_rela *grel,int *errcode)
{
    Dwarf_Unsigned ecount = 0;
    Dwarf_Unsigned size = gsh->gh_size;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned i = 0;
    int objlittleendian = (ep->f_endian == DW_ENDIAN_LITTLE);

    ecount = size/sizeof(dw_elf64_rel);
    size2 = ecount * sizeof(dw_elf64_rel);
    if(size != size2) {
        P("ERROR: Bogus size of relocations section "
            LONGESTUFMT ". "
            " not divisible by %lu RO_ERR_RELCOUNTMISMATCH\n",
            size,(unsigned long)sizeof(dw_elf64_rel));
        *errcode = RO_ERR_RELCOUNTMISMATCH;
        return RO_ERROR;
    }
    for ( i = 0; i < ecount; ++i,++relp,++grel) {
        grel->gr_isrela = 0;
        ASNAR(ep->f_copy_word,grel->gr_offset,relp->r_offset);
        ASNAR(ep->f_copy_word,grel->gr_info,relp->r_info);
        grel->gr_addend  = 0; /* Unused for plain .rel */

        if (ep->f_machine == EM_MIPS && objlittleendian ) {
            char realsym[4];

            memcpy(realsym,&relp->r_info,sizeof(realsym));
            ASNAR(ep->f_copy_word,grel->gr_sym,realsym);
            grel->gr_type  = relp->r_info[7];
            grel->gr_type2 = relp->r_info[6];
            grel->gr_type3 = relp->r_info[5];
        } else {
            grel->gr_sym  = grel->gr_info >>32; /* ELF64_R_SYM */
            grel->gr_type = grel->gr_info & 0xffffffff; /* ELF64_R_TYPE */
        }
        grel->gr_isrela = FALSE;
    }
    return RO_OK;
}


int
dwarf_load_elf_dynstr(elf_filedata ep, int *errcode)
{
    struct generic_shdr *strpsh = 0;
    int res = 0;
    Dwarf_Unsigned strsectindex  =0;
    Dwarf_Unsigned strsectlength = 0;

        if (!ep->f_dynsym_sect_strings_sect_index) {
            return DW_DLV_NO_ENTRY;
        }
        strsectindex = ep->f_dynsym_sect_strings_sect_index;
        strsectlength = ep->f_dynsym_sect_strings_max;
        strpsh = ep->f_shdr + strsectindex;
        /*  Alloc an extra byte as a guaranteed NUL byte
            at the end of the strings in case the section
            is corrupted and lacks a NUL at end. */
        ep->f_dynsym_sect_strings = calloc(1,strsectlength+1);
        if(!ep->f_dynsym_sect_strings) {
            P("ERROR: Unable to malloc " LONGESTXFMT " bytes for "
                "dynsym strings table\n",strsectlength);
            ep->f_dynsym_sect_strings = 0;
            ep->f_dynsym_sect_strings_max = 0;
            ep->f_dynsym_sect_strings_sect_index = 0;
            return RO_ERROR;
        }
        res = RRMOA(ep->f_fd,ep->f_dynsym_sect_strings,
            strpsh->gh_offset,
            strsectlength,ep->f_filesize,errcode);
        if(res != RO_OK) {
            check_size("dynsym section strings",strpsh->gh_offset,
                strsectlength,ep->f_filesize);
            P("ERROR: Could not read dynsym section strings at "
                LONGESTXFMT " (" LONGESTUFMT  ")\n",
                strpsh->gh_offset,strpsh->gh_offset);
            ep->f_dynsym_sect_strings = 0;
            ep->f_dynsym_sect_strings_max = 0;
            ep->f_dynsym_sect_strings_sect_index = 0;
            return res;
        }
    return DW_DLV_OK;
}
int
dwarf_load_elf_symstr(elf_filedata ep, int *errcode)
{
    struct generic_shdr *strpsh = 0;
    int res = 0;
    Dwarf_Unsigned strsectindex  =0;
    Dwarf_Unsigned strsectlength = 0;

    if (!ep->f_symtab_sect_strings_sect_index) {
        return DW_DLV_NO_ENTRY;
    }
    strsectindex = ep->f_symtab_sect_strings_sect_index;
    strsectlength = ep->f_symtab_sect_strings_max;
    strpsh = ep->f_shdr + strsectindex;
    /*  Alloc an extra byte as a guaranteed NUL byte
        at the end of the strings in case the section
        is corrupted and lacks a NUL at end. */
    ep->f_symtab_sect_strings = calloc(1,strsectlength+1);
    if(!ep->f_symtab_sect_strings) {
        P("ERROR: Unable to malloc " LONGESTXFMT " bytes for "
            "symtab strings table\n",strsectlength);
        ep->f_symtab_sect_strings = 0;
        ep->f_symtab_sect_strings_max = 0;
        ep->f_symtab_sect_strings_sect_index = 0;
        return res;
    }
    res = RRMOA(ep->f_fd,ep->f_symtab_sect_strings,
        strpsh->gh_offset,
        strsectlength,ep->f_filesize,errcode);
    if(res != RO_OK) {
        check_size("symtab section strings",strpsh->gh_offset,
            strsectlength,ep->f_filesize);
        P("ERROR: Could not read symtab section strings at "
            LONGESTXFMT " (" LONGESTUFMT  ")\n",
            strpsh->gh_offset,strpsh->gh_offset);
        ep->f_symtab_sect_strings = 0;
        ep->f_symtab_sect_strings_max = 0;
        ep->f_symtab_sect_strings_sect_index = 0;
        return res;
    }
    return DW_DLV_OK;
}


int
dwarf_get_elf_symstr_string(elf_filedata ep,
    int is_symtab,Dwarf_Unsigned index,
    const char **str_out,
    int*errcode)
{
    if (is_symtab) {
        if(index >= ep->f_symtab_sect_strings_max) {
            *errcode = RO_ERR_STRINGOFFSETBIG;
            return DW_DLV_ERROR;
        }
        *str_out = ep->f_symtab_sect_strings + index;
        return DW_DLV_OK;
    }
    if(index >= ep->f_dynsym_sect_strings_max) {
        *errcode = RO_ERR_STRINGOFFSETBIG;
        return DW_DLV_ERROR;
    }
    *str_out = ep->f_dynsym_sect_strings + index;
    return DW_DLV_OK;
}


static int
elf_load_sectstrings(elf_filedata ep,Dwarf_Unsigned stringsection,
    int *errcode)
{
    int i = 0;
    struct generic_shdr *psh = 0;
    Dwarf_Unsigned secoffset = 0;

    ep->f_elf_shstrings_length = 0;
    if (stringsection >= ep->f_ehdr->ge_shnum) {
        printf("String section " LONGESTUFMT " invalid. Ignored.",
            stringsection);
        return RO_ERROR;
    }
    psh = ep->f_shdr + stringsection;
    secoffset = psh->gh_offset;
    if(is_empty_section(psh->gh_type)) {
        P("String section type SHT_NULL or SHT_NOBITS!!. "
            "No section string section!\n");
        return RO_ERROR;
    }
    if(psh->gh_size > ep->f_elf_shstrings_max) {
        free(ep->f_elf_shstrings_data);
        ep->f_elf_shstrings_data = (char *)malloc(psh->gh_size);
        ep->f_elf_shstrings_max = psh->gh_size;
        if(!ep->f_elf_shstrings_data) {
            ep->f_elf_shstrings_max = 0;
            P("Unable to malloc %ld bytes for strings\n",
                (long)psh->gh_size);
            *errcode = RO_ERR_MALLOC;
            return DW_DLV_ERROR;
        }
    }
    ep->f_elf_shstrings_length = psh->gh_size;
    i = RRMOA(ep->f_fd,ep->f_elf_shstrings_data,secoffset,
        psh->gh_size,ep->f_filesize,errcode);
    if(i != RO_OK) {
        P("Read  " LONGESTUFMT" bytes of shstrings section string "
            "data failed\n",ep->f_elf_shstrings_length);
        ep->f_elf_shstrings_length = 0;
        return i;
    }
    return RO_OK;
}

static int
elf_load_progheaders32(elf_filedata ep,
    Dwarf_Unsigned offset,Dwarf_Unsigned entsize,Dwarf_Unsigned count,
    int *errcode)
{
    struct generic_phdr *gphdr =0;
    Dwarf_Unsigned generic_count = 0;
    int res = 0;

    if(count == 0) {
        const char *p = "";
        if (printfilenames) {
            p = sanitized(filename,buffer1,BUFFERSIZE);
        }
        P("No program headers %s\n",p);
        return DW_DLV_NO_ENTRY;
    }
    if(entsize < sizeof(dw_elf32_phdr)) {
        P("ERROR: Elf Program header too small? "
            LONGESTUFMT  " vs "
            LONGESTUFMT "\n",
            entsize,(Dwarf_Unsigned)sizeof(dw_elf32_phdr));
        *errcode = RO_ERR_TOOSMALL;
        return RO_ERROR;
    }
    if ((offset > ep->f_filesize)||
        (entsize > 200)||
        (count > ep->f_filesize) ||
        (((count *entsize) +offset) > ep->f_filesize)) {
            P("ERROR: Something badly wrong with elf header"
                " references to program headers"
                " filesize " LONGESTUFMT
                " sectionentrysize " LONGESTUFMT
                " sectionentrycount " LONGESTUFMT
                "\n", ep->f_filesize,entsize,count);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return RO_ERROR;
    }
    res = generic_phdr_from_phdr32(ep,&gphdr,&generic_count,
        offset,entsize,count,errcode);
    if (res != RO_OK) {
        return res;
    }
    if (count != generic_count) {
        P("ERROR: Something badly wrong reading program headers"
            " Header count expected " LONGESTUFMT
            " while actual count " LONGESTUFMT
            " RO_ERR_PHDRCOUNTMISMATCH\n",
            count,generic_count);
        *errcode = RO_ERR_PHDRCOUNTMISMATCH;
        return RO_ERROR;
    }
    ep->f_phdr = gphdr;
    ep->f_loc_phdr.g_count = generic_count;
    dwarf_insert_in_use_entry(ep,"Elf32_Phdr block",
        offset,entsize*count,ALIGN4);
    return RO_OK;
}

static int
elf_load_progheaders64(elf_filedata ep,
    Dwarf_Unsigned offset,Dwarf_Unsigned entsize,Dwarf_Unsigned count,
    int *errcode)
{
    struct generic_phdr *gphdr =0;
    Dwarf_Unsigned generic_count = 0;
    int res = 0;

    if(count == 0) {
        const char *p = "";
        if (printfilenames) {
            p = sanitized(filename,buffer1,BUFFERSIZE);
        }
        P("No program headers %s\n",p);
        return DW_DLV_NO_ENTRY;
    }
    if(entsize < sizeof(dw_elf64_phdr)) {
        P("ERROR: Elf Program header too small? "
            LONGESTUFMT  " vs "
            LONGESTUFMT "\n",
            entsize,(Dwarf_Unsigned)sizeof(dw_elf64_phdr));
        *errcode = RO_ERR_TOOSMALL;
        return RO_ERROR;
    }
    if ((offset > ep->f_filesize)||
        (entsize > 200)||
        (count > ep->f_filesize) ||
        (((count *entsize) +offset) > ep->f_filesize)) {
            P("ERROR: Something badly wrong with elf header"
                " references to program headers"
                " filesize " LONGESTUFMT
                " sectionentrysize " LONGESTUFMT
                " sectionentrycount " LONGESTUFMT
                "\n", ep->f_filesize,entsize,count);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return RO_ERROR;
    }
    res = generic_phdr_from_phdr64(ep,&gphdr,&generic_count,
        offset,entsize,count,errcode);
    if (res != RO_OK) {
        return res;
    }
    if (count != generic_count) {
        P("ERROR: Something badly wrong reading program headers"
            " Header count expected " LONGESTUFMT
            " while actual count " LONGESTUFMT
            " RO_ERR_PHDRCOUNTMISMATCH\n",
            count,generic_count);
        *errcode = RO_ERR_PHDRCOUNTMISMATCH;
        return RO_ERROR;
    }
    ep->f_phdr = gphdr;
    ep->f_loc_phdr.g_count = generic_count;
    dwarf_insert_in_use_entry(ep,"Elf64_Phdr block",
        offset,entsize*count,ALIGN4);
    return RO_OK;
}


static int
elf_load_sectheaders32(elf_filedata ep,
    Dwarf_Unsigned offset,Dwarf_Unsigned entsize,
    Dwarf_Unsigned count,int *errcode)
{
    Dwarf_Unsigned generic_count = 0;
    int res = 0;


    if(count == 0) {
        P("No section headers\n");
        return DW_DLV_NO_ENTRY;
    }
    if(entsize < sizeof(dw_elf32_shdr)) {
        P("Elf Section header too small? "
            LONGESTUFMT " vs %u\n",
            entsize,(unsigned )sizeof(dw_elf32_shdr));
        *errcode = RO_ERR_TOOSMALL;
        return RO_ERROR;
    }
    if ((offset > ep->f_filesize)||
        (entsize > 200)||
        (count > ep->f_filesize) ||
        ((count *entsize +offset) > ep->f_filesize)) {
            P("ERROR: Something badly wrong with elf header"
                " references to section headers "
                " filesize " LONGESTUFMT
                " sectionentrysize " LONGESTUFMT
                " sectionentrycount " LONGESTUFMT
                "\n", ep->f_filesize,entsize,count);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return RO_ERROR;
    }
    res = generic_shdr_from_shdr32(ep,&generic_count,
        offset,entsize,count,errcode);
    if (res != RO_OK) {
        return res;
    }
    if (generic_count != count) {
        P("ERROR: Something badly wrong reading section headers"
            " Header count expected " LONGESTUFMT
            " while actual count " LONGESTUFMT
            " RO_ERR_SHDRCOUNTMISMATCH\n",
            count,generic_count);
        *errcode = RO_ERR_SHDRCOUNTMISMATCH;
        return RO_ERROR;
    }
    dwarf_insert_in_use_entry(ep,"Elf32_Shdr block",
        offset,entsize*count,ALIGN4);
    return RO_OK;
}

static int
elf_load_sectheaders64(elf_filedata ep,
    Dwarf_Unsigned offset,Dwarf_Unsigned entsize,
    Dwarf_Unsigned count,int*errcode)
{
    Dwarf_Unsigned generic_count = 0;
    int res = 0;


    if(count == 0) {
        P("No section headers\n");
        return DW_DLV_NO_ENTRY;
    }
    if(entsize < sizeof(dw_elf64_shdr)) {
        P("Elf Section header too small? "
            LONGESTUFMT " vs %u\n",
            entsize,(unsigned )sizeof(dw_elf64_shdr));
        *errcode = RO_ERR_TOOSMALL;
        return RO_ERROR;
    }
    if ((offset > ep->f_filesize)||
        (entsize > 200)||
        (count > ep->f_filesize) ||
        ((count *entsize +offset) > ep->f_filesize)) {
            P("ERROR: Something badly wrong with elf header"
                " references to section headers "
                " filesize " LONGESTUFMT
                " sectionentrysize " LONGESTUFMT
                " sectionentrycount " LONGESTUFMT
                "\n", ep->f_filesize,entsize,count);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return RO_ERROR;
    }
    res = generic_shdr_from_shdr64(ep,&generic_count,
        offset,entsize,count,errcode);
    if (res != RO_OK) {
        return res;
    }
    if (generic_count != count) {
        P("ERROR: Something badly wrong reading section headers"
            " Header count expected " LONGESTUFMT
            " while actual count " LONGESTUFMT
            " RO_ERR_SHDRCOUNTMISMATCH\n",
            count,generic_count);
        *errcode = RO_ERR_SHDRCOUNTMISMATCH;
        return RO_ERROR;
    }
    dwarf_insert_in_use_entry(ep,"Elf64_Shdr block",
        offset,entsize*count,ALIGN8);
    return RO_OK;
}


static int
dwarf_elf_load_rela_32(elf_filedata ep,
    Dwarf_Unsigned secnum,
    struct generic_shdr * gsh,struct generic_rela ** grel_out,
    Dwarf_Unsigned *count_out, int *errcode)
{
    Dwarf_Unsigned count = 0;
    Dwarf_Unsigned size = 0;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned sizeg = 0;
    Dwarf_Unsigned offset = 0;
    int res = 0;
    dw_elf32_rela *relp = 0;
    Dwarf_Unsigned object_reclen = sizeof(dw_elf32_rela);
    struct generic_rela *grel = 0;

    offset = gsh->gh_offset;
    size = gsh->gh_size;
    if(size == 0) {
        P("Warning: zero-size relocation section: " LONGESTUFMT
            ", offset "
            LONGESTXFMT ", size " LONGESTXFMT "\n",
            secnum,
            offset, size);
        return RO_OK;
    }
    if ((offset > ep->f_filesize)||
        (size > ep->f_filesize) ||
        ((size +offset) > ep->f_filesize)) {
            P("ERROR: Something badly wrong with"
                " relocation section %s "
                " filesize " LONGESTUFMT
                " offset " LONGESTUFMT
                " size " LONGESTUFMT
                "\n",
                sanitized(gsh->gh_namestring,buffer1,BUFFERSIZE),
                ep->f_filesize,offset,size);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return RO_ERROR;
    }

    count = (long)(size/object_reclen);
    size2 = count * object_reclen;
    if(size != size2) {
        P("ERROR: Bogus size of relocations. Section " LONGESTUFMT
            ": " LONGESTUFMT
            " not divisible by "
            LONGESTUFMT "\n",
            secnum, size,object_reclen);
        *errcode = RO_ERR_RELSECTIONSIZE;
        return RO_ERROR;
    }
    relp = (dw_elf32_rela *)malloc(size);
    if(!relp) {
        P("ERROR: Could not malloc whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    res = RRMOA(ep->f_fd,relp,offset,size,ep->f_filesize,errcode);
    if(res != RO_OK) {
        free(relp);
        check_size("relocation section",offset,size,ep->f_filesize);
        P("ERROR: Could not read whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return res;
    }
    sizeg = count*sizeof(struct generic_rela);
    grel = (struct generic_rela *)malloc(sizeg);
    if (!grel) {
        free(relp);
        P("ERROR: Could not malloc whole generic reloc section "
            LONGESTUFMT " of %s "
            " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            sizeg);
        *errcode = RO_ERR_MALLOC;
        return RO_ERROR;
    }
    res = generic_rel_from_rela32(ep,gsh,relp,grel,errcode);
    free(relp);
    if (res == DW_DLV_OK) {
        gsh->gh_relcount = count;
        gsh->gh_rels = grel;
        *count_out = count;
        *grel_out = grel;
        return RO_OK;
    }
    /* Some sort of issue */
    count_out = 0;
    free(grel);
    return res;
}


static int
dwarf_elf_load_rel_32(elf_filedata ep,
    Dwarf_Unsigned secnum,
    struct generic_shdr * gsh,struct generic_rela ** grel_out,
    Dwarf_Unsigned *count_out,int *errcode)
{
    Dwarf_Unsigned count = 0;
    Dwarf_Unsigned size = 0;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned sizeg = 0;
    Dwarf_Unsigned offset = 0;
    int res = 0;
    dw_elf32_rel* relp = 0;
    Dwarf_Unsigned object_reclen = sizeof(dw_elf32_rel);
    struct generic_rela *grel = 0;

    offset = gsh->gh_offset;
    size = gsh->gh_size;
    if(size == 0) {
        P("Warning: Empty relocation section: " LONGESTUFMT
            ", offset "
            LONGESTXFMT ", size " LONGESTXFMT "\n",
            secnum, offset, size);
        return RO_ERROR;
    }
    if ((offset > ep->f_filesize)||
        (size > ep->f_filesize) ||
        ((size +offset) > ep->f_filesize)) {
            P("ERROR: Something badly wrong with relocation section "
                LONGESTUFMT " %s "
                " filesize " LONGESTUFMT
                " offset " LONGESTUFMT
                " size " LONGESTUFMT
                "\n",
                secnum,
                sanitized(gsh->gh_namestring,buffer1,BUFFERSIZE),
                ep->f_filesize,offset,size);
            return RO_ERROR;
    }

    count = size/object_reclen;
    size2 = count * object_reclen;
    if(size != size2) {
        P("Bogus size of relocations. Section " LONGESTUFMT
            ": " LONGESTUFMT
            " not divisible by "
            LONGESTUFMT "\n",
            secnum, size,object_reclen);
        return RO_ERROR;
    }
    relp = (dw_elf32_rel *)malloc(size);
    if(!relp) {
        P("ERROR: Could not malloc whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return RO_ERR_MALLOC;
    }
    res = RRMOA(ep->f_fd,relp,offset,size,ep->f_filesize,errcode);
    if(res != RO_OK) {
        free(relp);
        check_size("relocation section",offset,size,ep->f_filesize);
        P("could not read whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return res;
    }
    sizeg = count *sizeof(struct generic_rela);
    grel = (struct generic_rela *)malloc(sizeg);
    if (!grel) {
        free(relp);
        P("ERROR: Could not malloc whole generic reloc section "
            LONGESTUFMT " of %s "
            " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            sizeg);
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    res = generic_rel_from_rel32(ep,gsh,relp,grel,errcode);
    free(relp);
    if (res == DW_DLV_OK) {
        *count_out = count;
        *grel_out = grel;
        return RO_OK;
    }
    /* Some sort of error */
    count_out = 0;
    free (grel);
    return res;
}


static int
dwarf_elf_load_rel_64(elf_filedata ep,
    Dwarf_Unsigned secnum,
    struct generic_shdr * gsh,struct generic_rela ** grel_out,
    Dwarf_Unsigned *count_out,int *errcode)
{
    Dwarf_Unsigned count = 0;
    Dwarf_Unsigned size = 0;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned sizeg = 0;
    Dwarf_Unsigned offset = 0;
    int res = 0;
    dw_elf64_rel* relp = 0;
    Dwarf_Unsigned object_reclen = sizeof(dw_elf64_rel);
    struct generic_rela *grel = 0;

    offset = gsh->gh_offset;
    size = gsh->gh_size;
    if(size == 0) {
        P("Warning: Empty relocation section: " LONGESTUFMT
            ", offset "
            LONGESTXFMT ", size " LONGESTXFMT "\n",
            secnum, offset, size);
        *errcode = RO_ERR_RELSECTIONSIZE;
        return RO_ERROR;
    }
    if ((offset > ep->f_filesize)||
        (size > ep->f_filesize) ||
        ((size +offset) > ep->f_filesize)) {
            P("ERROR: Something badly wrong with relocation section "
                LONGESTUFMT " %s "
                " filesize " LONGESTUFMT
                " offset " LONGESTUFMT
                " size " LONGESTUFMT
                "\n",
                secnum,
                sanitized(gsh->gh_namestring,buffer1,BUFFERSIZE),
                ep->f_filesize,offset,size);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return RO_ERROR;
    }

    count = size/object_reclen;
    size2 = count * object_reclen;
    if(size != size2) {
        P("Bogus size of relocations. Section " LONGESTUFMT
            ": " LONGESTUFMT
            " not divisible by "
            LONGESTUFMT " RO_ERR_RELCOUNTMISMATCH\n",
            secnum, size,object_reclen);
        *errcode = RO_ERR_RELCOUNTMISMATCH;
        return RO_ERROR;
    }
    relp = (dw_elf64_rel *)malloc(size);
    if(!relp) {
        P("ERROR: Could not malloc whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    res = RRMOA(ep->f_fd,relp,offset,size,ep->f_filesize,errcode);
    if(res != RO_OK) {
        free(relp);
        check_size("relocation section",offset,size,ep->f_filesize);
        P("could not read whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return res;
    }
    sizeg = count*sizeof(struct generic_rela);
    grel = (struct generic_rela *)malloc(sizeg);
    if (!grel) {
        free(relp);
        P("ERROR: Could not malloc whole generic reloc section "
            LONGESTUFMT " of %s "
            " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            sizeg);
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    res = generic_rel_from_rel64(ep,gsh,relp,grel,errcode);
    free(relp);
    if (res == DW_DLV_OK) {
        *count_out = count;
        *grel_out = grel;
        return res;
    }
    /* Some sort of error */
    count_out = 0;
    free (grel);
    return RO_ERROR;
}


static int
dwarf_elf_load_rela_64(elf_filedata ep,Dwarf_Unsigned secnum,
    struct generic_shdr * gsh,struct generic_rela ** grel_out,
    Dwarf_Unsigned *count_out,int *errcode)
{
    Dwarf_Unsigned count = 0;
    Dwarf_Unsigned size = 0;
    Dwarf_Unsigned size2 = 0;
    Dwarf_Unsigned sizeg = 0;
    Dwarf_Unsigned offset = 0;
    int res = 0;
    dw_elf64_rela *relp = 0;
    Dwarf_Unsigned object_reclen = sizeof(dw_elf64_rela);
    struct generic_rela *grel = 0;

    offset = gsh->gh_offset;
    size = gsh->gh_size;
    if(size == 0) {
        P("Warning: zero-size relocation section: " LONGESTUFMT
            ", offset "
            LONGESTXFMT ", size " LONGESTXFMT "\n",
            secnum,
            offset, size);
        *errcode = RO_ERR_RELSECTIONSIZE;
        return RO_ERROR;
    }
    if ((offset > ep->f_filesize)||
        (size > ep->f_filesize) ||
        ((size +offset) > ep->f_filesize)) {
            P("ERROR: Something badly wrong with"
                " relocation section "
                LONGESTUFMT " %s "
                " filesize " LONGESTUFMT
                " offset " LONGESTUFMT
                " size " LONGESTUFMT
                "\n",
                secnum,
                sanitized(gsh->gh_namestring,buffer1,BUFFERSIZE),
                ep->f_filesize,offset,size);
            *errcode = RO_ERR_FILEOFFSETBAD;
            return RO_ERROR;
    }
    count = (long)(size/object_reclen);
    size2 = count * object_reclen;
    if(size != size2) {
        P("ERROR: Bogus size of relocations. Section " LONGESTUFMT
            ": " LONGESTUFMT
            " not divisible by "
            LONGESTUFMT " RO_ERR_RELCOUNTMISMATCH\n",
            secnum, size,object_reclen);
        *errcode = RO_ERR_RELCOUNTMISMATCH;
        return RO_ERROR;
    }
    /* Here want native rela size from the file */
    relp = (dw_elf64_rela *)malloc(size);
    if(!relp) {
        P("ERROR: Could not malloc whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    res = RRMOA(ep->f_fd,relp,offset,size,ep->f_filesize,errcode);
    if(res != RO_OK) {
        free(relp);
        check_size("relocation section",offset,size,ep->f_filesize);
        P("ERROR: Could not read whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return res;
    }
    sizeg = count*sizeof(struct generic_rela);
    /* Here want generic-record size from the file */
    grel = (struct generic_rela *)malloc(sizeg);
    if (!grel) {
        free(relp);
        P("ERROR: Could not malloc whole generic reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        *errcode = RO_ERR_MALLOC;
        return RO_ERROR;
    }
    res = generic_rel_from_rela64(ep,gsh,relp,grel,errcode);
    free(relp);
    if (res == DW_DLV_OK) {
        *count_out = count;
        *grel_out = grel;
        return RO_OK;
    }
    /* Some sort of error */
    count_out = 0;
    free (grel);
    return res;
}

int
dwarf_load_elf_rela(elf_filedata ep,
    Dwarf_Unsigned secnum, int *errcode)
{
    struct generic_shdr *gshdr = 0;
    unsigned offsetsize = 0;
    struct generic_rela *grp = 0;
    Dwarf_Unsigned count_read = 0;
    int res = 0;

    if (!ep) {
        *errcode = RO_ERR_NULL_ELF_POINTER;
        return DW_DLV_ERROR;
    }
    offsetsize = ep->f_offsetsize;
    gshdr = ep->f_shdr +secnum;
    if (is_empty_section(gshdr->gh_type)) {
        return DW_DLV_NO_ENTRY;
    }
    if (offsetsize == 32) {
        res = dwarf_elf_load_rela_32(ep,
            secnum,gshdr,&grp,&count_read,errcode);
    } else if (offsetsize == 64) {
        res = dwarf_elf_load_rela_64(ep,
            secnum,gshdr,&grp,&count_read,errcode);
    } else {
        *errcode = RO_ERR_BADOFFSETSIZE;
        return RO_ERROR;
    }
    if (res != RO_OK) {
        return res;
    }
    gshdr->gh_rels = grp;
    gshdr->gh_relcount = count_read;
    return DW_DLV_OK;
}
int
dwarf_load_elf_rel(elf_filedata ep,
    Dwarf_Unsigned secnum, int *errcode)
{
    struct generic_shdr *gshdr = 0;
    unsigned offsetsize = 0;
    struct generic_rela *grp = 0;
    Dwarf_Unsigned count_read = 0;
    int res = 0;

    if (!ep) {
        *errcode = RO_ERR_NULL_ELF_POINTER;
        return DW_DLV_ERROR;
    }
    offsetsize = ep->f_offsetsize;
    gshdr = ep->f_shdr +secnum;
    if (is_empty_section(gshdr->gh_type)) {
        return DW_DLV_NO_ENTRY;
    }
    if (offsetsize == 32) {
        res = dwarf_elf_load_rel_32(ep,
            secnum,gshdr,&grp,&count_read,errcode);
    } else if (offsetsize == 64) {
        res = dwarf_elf_load_rel_64(ep,
            secnum,gshdr,&grp,&count_read,errcode);
    } else {
        *errcode = RO_ERR_BADOFFSETSIZE;
        return DW_DLV_ERROR;
    }
    if (res == DW_DLV_ERROR) {
        return res;
    }
    if (res == DW_DLV_NO_ENTRY) {
        return res;
    }
    gshdr->gh_rels = grp;
    gshdr->gh_relcount = count_read;
    return DW_DLV_OK;
}

static void
elf_check_phdr_sizes(elf_filedata ep)
{
    struct generic_phdr *gphdr = 0;
    Dwarf_Unsigned generic_count = 0;
    Dwarf_Unsigned i = 1;
    Dwarf_Unsigned filesize = ep->f_filesize;

    gphdr = ep->f_phdr;
    generic_count = ep->f_loc_phdr.g_count;
    for(i = 0; i < generic_count; i++, ++gphdr) {
        Dwarf_Unsigned offset = 0;
        Dwarf_Unsigned size = 0;
        const char *type = "";

        switch(gphdr->gp_type) {
        case PT_NULL:
            type = "PT_NULL";
            break;
        case PT_SHLIB:
            type = "PT_SHLIB";
            break;
        case PT_LOAD:
            type = "PT_LOAD";
            size   = gphdr->gp_filesz;
            offset = gphdr->gp_offset;
            break;
        case PT_DYNAMIC:
            type = "PT_DYNAMIC";
            size   = gphdr->gp_filesz;
            offset = gphdr->gp_offset;
            break;
        case PT_NOTE:
            type = "PT_NOTE";
            size   = gphdr->gp_filesz;
            offset = gphdr->gp_offset;
            break;
        case PT_INTERP:
            type = "PT_INTERP";
            size   = gphdr->gp_filesz;
            offset = gphdr->gp_offset;
            break;
        case PT_PHDR:
            type = "PT_PHDR";
            size   = gphdr->gp_filesz;
            offset = gphdr->gp_offset;
            break;
        case PT_GNU_EH_FRAME:
            type = "PT_GNU_EH_FRAME";
            /*  Location of exception handling information
                as defined by .eh_frame_hdr section. */
            size   = gphdr->gp_filesz;
            offset = gphdr->gp_offset;
            break;
        case PT_GNU_STACK:
            /*  Indicates, by its existence, that
                an executable stack is not needed. */
            type = "PT_GNU_STACK";
            break;
        case PT_GNU_RELRO:
            /*  The location and size of a segment
                that may be made read-only after
                relocations have been processed. */
            type = "PT_GNU_RELRO";
            size   = gphdr->gp_filesz;
            offset = gphdr->gp_offset;
            break;
        case PT_PAX_FLAGS:
            /*  This is likely going to go away in favor
                of XATTR_PAX according to gentoo documentation.
                As of glibc-2.16 this progam header will
                not be used any longer.(?)
                Apparently extended file attributes will
                be used instead (XATTR_PAX) */
            type = "PT_PAX_FLAGS";
            break;
        default:
            type = "<PT_unknown>";
            break;
        }
        if (size >= filesize ||
            offset >= filesize ||
            (size+offset) > filesize) {
            P("ERROR: ProgramHeader size error. "
                " headernumber " LONGESTUFMT " (%s)"
                " offset " LONGESTUFMT
                ", size " LONGESTUFMT
                ", endpoint " LONGESTUFMT
                ", filesize " LONGESTUFMT "\n",
                i, type,offset, size,
                offset+size, filesize);
        }
        if (gphdr->gp_filesz > gphdr->gp_memsz) {
            P("Warning: ProgramHeader p_filesz > p_memsz "
                " headernumber " LONGESTUFMT " (%s)"
                " p_filesz " LONGESTUFMT
                ", p_memsz " LONGESTUFMT "\n",
                i,type, gphdr->gp_filesz, gphdr->gp_memsz);
        }
    }
}

static void
elf_check_sect_sizes(elf_filedata ep)
{
    struct generic_shdr *gshdr = 0;
    Dwarf_Unsigned generic_count = 0;
    Dwarf_Unsigned i = 1;
    Dwarf_Unsigned filesize = ep->f_filesize;

    gshdr = ep->f_shdr;
    generic_count = ep->f_loc_shdr.g_count;
    for(i = 0; i < generic_count; i++, ++gshdr) {
        if (is_empty_section(gshdr->gh_type)) {
            continue;
        }
        if (gshdr->gh_offset >= filesize ||
            gshdr->gh_size >= filesize ||
            (gshdr->gh_size+ gshdr->gh_offset) > filesize) {
            P("ERROR: Section size error. "
                " section " LONGESTUFMT " %s "
                " offset " LONGESTUFMT
                ", size " LONGESTUFMT
                ", endpoint " LONGESTUFMT
                ", filesize " LONGESTUFMT "\n",
                i,
                sanitized(gshdr->gh_namestring,
                    buffer1,BUFFERSIZE),
                gshdr->gh_offset,
                gshdr->gh_size,
                gshdr->gh_size + gshdr->gh_offset,
                filesize);
        }
    }
}

static int
validate_section_name_string(Dwarf_Unsigned section_length,
  Dwarf_Unsigned section_number,
  Dwarf_Unsigned string_loc_index,
  const char * strings_start,
  int  * errcode)
{
    const char *endpoint = strings_start + section_length;
    const char *cur = 0;

    if (section_length <= string_loc_index) {
        P("ERROR: Section " LONGESTUFMT
            ": section name offset "
            LONGESTUFMT " in section strings is too large.",
            section_number,string_loc_index);
        *errcode = RO_ERR_STRINGOFFSETBIG;
        return DW_DLV_ERROR;
    }
    cur = string_loc_index+strings_start;
    for(  ; cur < endpoint;++cur) {
        if (!*cur) {
            return DW_DLV_OK;
        }
    }
    P("ERROR: Section " LONGESTUFMT
        ": section name offset "
        LONGESTUFMT " in section strings"
        " is not terminated properly.",
        section_number,string_loc_index);
    *errcode = RO_ERR_STRINGOFFSETBIG;
    return DW_DLV_ERROR;
}

static int
elf_load_sect_namestring(elf_filedata ep, int *errcode)
{
    struct generic_shdr *gshdr = 0;
    Dwarf_Unsigned generic_count = 0;
    Dwarf_Unsigned i = 1;
    const char *stringsecbase = 0;

    stringsecbase = ep->f_elf_shstrings_data;
    gshdr = ep->f_shdr;
    generic_count = ep->f_loc_shdr.g_count;
    if (generic_count> ep->f_filesize) {
        P("ERROR: Something badly wrong with "
            "namestring section "
            " %s "
            " count " LONGESTUFMT
            " filesize " LONGESTUFMT
            "\n",
            sanitized(gshdr->gh_namestring,buffer1,BUFFERSIZE),
            generic_count,ep->f_filesize);
        return RO_ERROR;
    }
    for(i = 0; i < generic_count; i++, ++gshdr) {
        const char *namestr =
            "<Invalid sh_name value. Corrupt Elf.>";
        int res = 0;

        res = validate_section_name_string(ep->f_elf_shstrings_length,
            i, gshdr->gh_name, stringsecbase,
            errcode);
        if (res != DW_DLV_OK) {
            gshdr->gh_namestring = namestr;
            return res;
        }
        gshdr->gh_namestring = stringsecbase + gshdr->gh_name;
    }
    return DW_DLV_OK;
}


static int
elf_load_elf_header32(elf_filedata ep,int *errcode)
{
    int res = 0;
    dw_elf32_ehdr ehdr32;
    struct generic_ehdr *ehdr = 0;

    res = RRMOA(ep->f_fd,&ehdr32,0,sizeof(ehdr32),ep->f_filesize,errcode);
    if(res != RO_OK) {
        P("ERROR: could not read whole ELF file header of %s\n",
            sanitized(filename,buffer1,BUFFERSIZE));
        return res;
    }
    ehdr = (struct generic_ehdr *)calloc(1,sizeof(struct generic_ehdr));
    if (!ehdr) {
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    res  = generic_ehdr_from_32(ep,ehdr,&ehdr32,errcode);
    if (res == RO_OK) {
        dwarf_insert_in_use_entry(ep,"Elf32_Ehdr",0,
            sizeof(dw_elf32_ehdr),ALIGN4);
    }
    return res;
}
static int
elf_load_elf_header64(elf_filedata ep,int *errcode)
{
    int res = 0;
    dw_elf64_ehdr ehdr64;
    struct generic_ehdr *ehdr = 0;

    res = RRMOA(ep->f_fd,&ehdr64,0,sizeof(ehdr64),ep->f_filesize,errcode);
    if(res != RO_OK) {
        P("ERROR: could not read whole ELF file header of %s\n",
            sanitized(filename,buffer1,BUFFERSIZE));
        return res;
    }
    ehdr = (struct generic_ehdr *)calloc(1,sizeof(struct generic_ehdr));
    if (!ehdr) {
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    res  = generic_ehdr_from_64(ep,ehdr,&ehdr64,errcode);
    if (res == RO_OK) {
        dwarf_insert_in_use_entry(ep,"Elf64_Ehdr",
            0,sizeof(dw_elf64_ehdr),ALIGN8);
    }
    return res;
}

static int
generic_dyn_from_dyn32(elf_filedata ep,
    struct generic_dynentry **gbuffer_io,
    Dwarf_Unsigned *bufcount_out,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned size,
    Dwarf_Unsigned ecount, int *errcode)
{
    dw_elf32_dyn *ebuf = 0;
    dw_elf32_dyn *orig_ebuf = 0;
    struct generic_dynentry * gbuffer = 0;
    struct generic_dynentry * orig_gbuffer = 0;
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned trueoff = 0;
    int res = 0;


    ebuf = malloc(size);
    *bufcount_out = 0;
    if (!ebuf) {
        P("Out Of Memory, cannot malloc dynamic section space for "
            LONGESTUFMT " bytes\n",size);
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    orig_ebuf = ebuf;
    gbuffer= malloc(sizeof(struct generic_dynentry)*ecount);
    if(!gbuffer) {
        free(ebuf);
        P("Out Of Memory, cannot malloc generic dynamic space for "
            LONGESTUFMT " bytes\n",
            (sizeof(struct generic_dynentry)*ecount));
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    orig_gbuffer = gbuffer;
    trueoff = offset;
    res = RRMOA(ep->f_fd,ebuf,offset,size,ep->f_filesize,errcode);
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
        ++i,++gbuffer,++ebuf,trueoff += sizeof(dw_elf32_dyn)) {
        /* SIGNED VALUE */
        ASNAR(ep->f_copy_word,gbuffer->gd_tag,ebuf->d_tag);
        SIGN_EXTEND(gbuffer->gd_tag,sizeof(ebuf->d_tag));
        ASNAR(ep->f_copy_word,gbuffer->gd_val,ebuf->d_val);
        /* Assigning the file offset, not sec offset */
        gbuffer->gd_dyn_file_offset =  trueoff;
        if (gbuffer->gd_tag == 0) {
            ep->f_wasted_dynamic_count++;
            ep->f_wasted_dynamic_space += sizeof(dw_elf32_dyn);
        }
    }
    *bufcount_out = ecount;
    *gbuffer_io = orig_gbuffer;
    free(orig_ebuf);
    return RO_OK;
}

static int
generic_dyn_from_dyn64(elf_filedata ep,
    struct generic_dynentry **gbuffer_io,
    Dwarf_Unsigned *bufcount_out,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned size,
    Dwarf_Unsigned ecount, int *errcode)
{
    dw_elf64_dyn *ebuf = 0;
    dw_elf64_dyn *orig_ebuf = 0;
    struct generic_dynentry * gbuffer = 0;
    struct generic_dynentry * orig_gbuffer = 0;
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned trueoff = 0;
    int res = 0;


    ebuf = malloc(size);
    *bufcount_out = 0;
    if (!ebuf) {
        P("Out Of Memory, cannot malloc dynamic section space for "
            LONGESTUFMT " bytes\n",size);
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    orig_ebuf = ebuf;
    gbuffer= malloc(sizeof(struct generic_dynentry)*ecount);
    if(!gbuffer) {
        free(ebuf);
        P("Out Of Memory, cannot malloc generic dynamic space for "
            LONGESTUFMT " bytes\n",
            (sizeof(struct generic_dynentry)*ecount));
        *errcode = RO_ERR_MALLOC;
        return DW_DLV_ERROR;
    }
    orig_gbuffer = gbuffer;
    trueoff = offset;
    res = RRMOA(ep->f_fd,ebuf,offset,size,ep->f_filesize,errcode);
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
        ++i,++gbuffer,++ebuf,trueoff += sizeof(dw_elf64_dyn)) {
        /* SIGNED VALUE */
        ASNAR(ep->f_copy_word,gbuffer->gd_tag,ebuf->d_tag);
        SIGN_EXTEND(gbuffer->gd_tag,sizeof(ebuf->d_tag));
        ASNAR(ep->f_copy_word,gbuffer->gd_val,ebuf->d_val);
        /* Assigning the file offset, not sec offset */
        gbuffer->gd_dyn_file_offset = trueoff;
        if (gbuffer->gd_tag == 0) {
            ep->f_wasted_dynamic_count++;
            ep->f_wasted_dynamic_space += sizeof(dw_elf64_dyn);
        }
    }
    *bufcount_out = ecount;
    *gbuffer_io = orig_gbuffer;
    free(orig_ebuf);
    return RO_OK;
}


static int
elf_load_dynamic32(elf_filedata ep,
    Dwarf_Unsigned offset,Dwarf_Unsigned size, int*errcode)
{
    Dwarf_Unsigned ecount = 0;
    Dwarf_Unsigned size2 = 0;
    struct generic_dynentry *gbuffer;
    Dwarf_Unsigned bufcount = 0;
    int res = 0;

    if ((offset > ep->f_filesize)||
        (size > ep->f_filesize) ) {
        P("ERROR: Something badly wrong with dynamic section "
            " filesize " LONGESTUFMT
            " section content size " LONGESTUFMT
            "\n", ep->f_filesize,size);
        *errcode = RO_ERR_FILEOFFSETBAD;
        return DW_DLV_ERROR;
    }

    ecount = size/(Dwarf_Unsigned)sizeof(dw_elf32_dyn);
    size2 = ecount * sizeof(dw_elf32_dyn);
    if(size != size2) {
        P("Bogus size of dynamic. "
            LONGESTUFMT " not divisible by %lu\n",
            size,(unsigned long)sizeof(dw_elf32_dyn));
        *errcode = RO_ERR_DYNAMICSECTIONSIZE;
        return DW_DLV_ERROR;
    }
    res = generic_dyn_from_dyn32(ep,
        &gbuffer,&bufcount,offset,size,ecount,errcode);
    if (res != RO_OK) {
        return res;
    }
    if(!bufcount) {
        /* impossible? */
        free(gbuffer);
        *errcode = RO_ERR_UNEXPECTEDZERO;
        return RO_ERROR;
    }
    ep->f_dynamic = gbuffer;
    ep->f_loc_dynamic.g_name = ".dynamic";
    ep->f_loc_dynamic.g_offset = offset;
    ep->f_loc_dynamic.g_entrysize = sizeof(dw_elf32_dyn);
    ep->f_loc_dynamic.g_count = ecount;
    ep->f_loc_dynamic.g_totalsize = ecount *sizeof(dw_elf32_dyn);
    return RO_OK;
}

static int
elf_load_dynamic64(elf_filedata ep,
    Dwarf_Unsigned offset,Dwarf_Unsigned size,int *errcode)
{
    Dwarf_Unsigned ecount = 0;
    Dwarf_Unsigned size2 = 0;
    struct generic_dynentry *gbuffer;
    Dwarf_Unsigned bufcount = 0;
    int res = 0;

    if ((offset > ep->f_filesize)||
        (size > ep->f_filesize) ) {
        P("ERROR: Something badly wrong with dynamic section "
            " filesize " LONGESTUFMT
            " section content size " LONGESTUFMT
            "\n", ep->f_filesize,size);
        *errcode = RO_ERR_FILEOFFSETBAD;
        return DW_DLV_ERROR;
    }

    ecount = size/(Dwarf_Unsigned)sizeof(dw_elf64_dyn);
    size2 = ecount * sizeof(dw_elf64_dyn);
    if(size != size2) {
        P("Bogus size of dynamic. "
            LONGESTUFMT " not divisible by %lu\n",
            size,(unsigned long)sizeof(dw_elf64_dyn));
        *errcode = RO_ERR_DYNAMICSECTIONSIZE;
        return DW_DLV_ERROR;
    }
    res = generic_dyn_from_dyn64(ep,
        &gbuffer,&bufcount,offset,size,ecount,errcode);
    if (res != RO_OK) {
        return res;
    }
    if(!bufcount) {
        /* impossible? */
        free(gbuffer);
        *errcode = RO_ERR_UNEXPECTEDZERO;
        return RO_ERROR;
    }
    ep->f_dynamic = gbuffer;
    ep->f_loc_dynamic.g_name = ".dynamic";
    ep->f_loc_dynamic.g_offset = offset;
    ep->f_loc_dynamic.g_entrysize = sizeof(dw_elf64_dyn);
    ep->f_loc_dynamic.g_count = ecount;
    ep->f_loc_dynamic.g_totalsize = ecount *sizeof(dw_elf64_dyn);
    return RO_OK;
}

static int validate_struct_sizes(UNUSEDARG int*errcode)
{
#ifdef HAVE_ELF_H
    /*  This is a sanity check when we have an elf.h
        to check against. */
    if (sizeof(Elf32_Ehdr) != sizeof(dw_elf32_ehdr)) {
        *errcode = RO_ERR_BADTYPESIZE;
        return DW_DLV_ERROR;
    }
    if (sizeof(Elf64_Ehdr) != sizeof(dw_elf64_ehdr)) {
        *errcode = RO_ERR_BADTYPESIZE;
        return DW_DLV_ERROR;
    }
    if (sizeof(Elf32_Shdr) != sizeof(dw_elf32_shdr)) {
        *errcode = RO_ERR_BADTYPESIZE;
        return DW_DLV_ERROR;
    }
    if (sizeof(Elf64_Shdr) != sizeof(dw_elf64_shdr)) {
        *errcode = RO_ERR_BADTYPESIZE;
        return DW_DLV_ERROR;
    }
    if (sizeof(Elf32_Phdr) != sizeof(dw_elf32_phdr)) {
        *errcode = RO_ERR_BADTYPESIZE;
        return DW_DLV_ERROR;
    }
    if (sizeof(Elf64_Phdr) != sizeof(dw_elf64_phdr)) {
        *errcode = RO_ERR_BADTYPESIZE;
        return DW_DLV_ERROR;
    }
    if (sizeof(Elf32_Rel) != sizeof(dw_elf32_rel)) {
        *errcode = RO_ERR_BADTYPESIZE;
        return DW_DLV_ERROR;
    }
    if (sizeof(Elf64_Rel) != sizeof(dw_elf64_rel)) {
        *errcode = RO_ERR_BADTYPESIZE;
        return DW_DLV_ERROR;
    }
    if (sizeof(Elf32_Rela) != sizeof(dw_elf32_rela)) {
        *errcode = RO_ERR_BADTYPESIZE;
        return DW_DLV_ERROR;
    }
    if (sizeof(Elf64_Rela) != sizeof(dw_elf64_rela)) {
        *errcode = RO_ERR_BADTYPESIZE;
        return DW_DLV_ERROR;
    }
    if (sizeof(Elf32_Sym) != sizeof(dw_elf32_sym)) {
        *errcode = RO_ERR_BADTYPESIZE;
        return DW_DLV_ERROR;
    }
    if (sizeof(Elf64_Sym) != sizeof(dw_elf64_sym)) {
        *errcode = RO_ERR_BADTYPESIZE;
        return DW_DLV_ERROR;
    }
#endif /* HAVE_ELF_H */
    return DW_DLV_OK;
}

int
dwarf_load_elf_header(elf_filedata ep,int*errcode)
{
    unsigned offsetsize = ep->f_offsetsize;
    int res = 0;

    res = validate_struct_sizes(errcode);
    if (res != DW_DLV_OK) {
        return res;
    }

    if (offsetsize == 32) {
        res = elf_load_elf_header32(ep,errcode);
    } else if (offsetsize == 64) {
        if (sizeof(Dwarf_Unsigned) < 8) {
            P("Cannot read Elf64 from %s as the longest available "
                " integer is just %u bytes\n",
                sanitized(filename,buffer1,BUFFERSIZE),
                (unsigned)sizeof(Dwarf_Unsigned));
            *errcode =  RO_ERR_INTEGERTOOSMALL;
            return DW_DLV_ERROR;
        }
        res = elf_load_elf_header64(ep,errcode);
    } else {
        P("Cannot read Elf from %s as the elf class is"
            " %u, which is not a valid class value.\n",
            sanitized(filename,buffer1,BUFFERSIZE),
            (unsigned)offsetsize);
        *errcode = RO_ERR_ELF_CLASS;
        return DW_DLV_ERROR;
    }
    return res;
}

/*  We assume that  the sect numbers are valid,
    if non-zero: we set them!  */
static int
validate_links(const char * area,
    elf_filedata ep,
    Dwarf_Unsigned knownsect,
    Dwarf_Unsigned string_sect,
    int *errcode)
{
    struct generic_shdr* pshk = 0;
    struct generic_shdr* pshs = 0;

    if (string_sect) {
        pshs = ep->f_shdr + string_sect;
    }
    if (!knownsect) {
        if (string_sect) {
            P("Warning: Links %s: Have string sect " LONGESTUFMT
                " %s but the apppropriate symbol table"
                " section missing. \n",
                area,string_sect,
                sanitized(pshs->gh_namestring,
                    buffer1,BUFFERSIZE));
        }
        return DW_DLV_OK;
    }
    pshk = ep->f_shdr + knownsect;
    if (!string_sect) {
        P("Warning: Links %s: Have (sec " LONGESTUFMT
            " %s) but no string section found. \n",
            area,knownsect,
            sanitized(pshk->gh_namestring,
                buffer1,BUFFERSIZE));
        *errcode = RO_ERR_ELF_STRING_SECT;
        return DW_DLV_ERROR;
    }
    if (string_sect != pshk->gh_link) {
        P("Warning: Links  %s: (" LONGESTUFMT
            " %s) expected to have link "
            LONGESTUFMT "(%s) but has link "
            LONGESTUFMT " instead \n",
            area,
            knownsect,
            sanitized(pshk->gh_namestring, buffer3,BUFFERSIZE),
            string_sect,
            sanitized(pshs->gh_namestring, buffer3,BUFFERSIZE),
            pshk->gh_link);
        *errcode =  RO_ERR_ELF_STRING_LINK_ERROR;
        return DW_DLV_ERROR;
    }
    return DW_DLV_OK;
}


static int
string_endswith(const char *n,const char *q)
{
    unsigned long len = strlen(n);
    unsigned long qlen = strlen(q);
    const char *startpt = 0;

    if ( len < qlen) {
        return FALSE;
    }
    startpt = n + (len-qlen);
    if (strcmp(startpt,q)) {
        return FALSE;
    }
    return TRUE;
}

/*  We are allowing either SHT_GROUP or .group to indicate
    a group section, but really one should have both
    or neither! */
static int
elf_sht_groupsec(Dwarf_Unsigned type, const char *sname)
{
    /*  ARM compilers name SHT group "__ARM_grp<long name here>"
        not .group */
    if ((type == SHT_GROUP) || (!strcmp(sname,".group"))){
        return TRUE;
    }
    return FALSE;
}

static int
elf_flagmatches(Dwarf_Unsigned flagsword,Dwarf_Unsigned flag)
{
    if ((flagsword&flag) == flag) {
        return TRUE;
    }
    return FALSE;
}

/*  For SHT_GROUP sections. */
static int
read_gs_section_group(elf_filedata ep,
    struct generic_shdr* psh,
    int *errcode)
{
    Dwarf_Unsigned i = 0;
    int res = 0;

    if (!psh->gh_sht_group_array) {
        Dwarf_Unsigned seclen = psh->gh_size;
        char *data = 0;
        char *dp = 0;
        Dwarf_Unsigned* grouparray = 0;
        char dblock[4];
        Dwarf_Unsigned va = 0;
        Dwarf_Unsigned count = 0;
        int foundone = 0;

        if (seclen < DWARF_32BIT_SIZE) {
            *errcode = RO_ERR_TOOSMALL;
            return DW_DLV_ERROR;
        }
        data = malloc(seclen);
        if (!data) {
            P("Group list count malloc  " LONGESTUFMT
                " bytes fails.\n",seclen);
            *errcode = RO_ERR_MALLOC;
            return DW_DLV_ERROR;
        }
        dp = data;
        count = seclen/psh->gh_entsize;
        if (count > ep->f_loc_shdr.g_count) {
            /* Impossible */
            P("Group list count  " LONGESTUFMT
                " is larger than the section count. Impossible.\n",
                count);
            free(data);
            *errcode = RO_ERR_GROUP_ERROR;
            return DW_DLV_ERROR;
        }

        if (psh->gh_entsize != DWARF_32BIT_SIZE) {
            P("Group list entry size  " LONGESTUFMT
                " is larger than 4. Corrupt Elf.\n",
                psh->gh_entsize);
            *errcode = RO_ERR_BADTYPESIZE;
            free(data);
            return DW_DLV_ERROR;
        }
        res = RRMOA(ep->f_fd,data,psh->gh_offset,seclen,ep->f_filesize,errcode);
        if(res != RO_OK) {
            P("Read  " LONGESTUFMT
                " bytes of .group section failed\n",seclen);
            free(data);
            return res;
        }
        grouparray = malloc(count * sizeof(Dwarf_Unsigned));
        if (!grouparray) {
            P("Group array count malloc  " LONGESTUFMT
                " bytes fails.\n",count*sizeof(Dwarf_Unsigned));
            free(data);
            *errcode = RO_ERR_MALLOC;
            return DW_DLV_ERROR;
        }

        memcpy(dblock,dp,DWARF_32BIT_SIZE);
        ASNAR(memcpy,va,dblock);
        /* There is ambiguity on the endianness of this stuff. */
        if (va != 1 && va != 0x1000000) {
            /*  Could be corrupted elf object. */
            *errcode = RO_ERR_GROUP_ERROR;
            free(data);
            free(grouparray);
            return DW_DLV_ERROR;
        }
        grouparray[0] = 1;
        dp = dp + DWARF_32BIT_SIZE;
        for( i = 1; i < count; ++i,dp += DWARF_32BIT_SIZE) {
            Dwarf_Unsigned gseca = 0;
            Dwarf_Unsigned gsecb = 0;
            struct generic_shdr* targpsh = 0;

            memcpy(dblock,dp,DWARF_32BIT_SIZE);
            ASNAR(memcpy,gseca,dblock);
            ASNAR(dwarf_ro_memcpy_swap_bytes,gsecb,dblock);
            if (!gseca) {
                free(data);
                free(grouparray);
                *errcode = RO_ERR_GROUP_ERROR;
                return DW_DLV_ERROR;
            }
            grouparray[i] = gseca;
            if (gseca > ep->f_loc_shdr.g_count) {
                /*  Might be confused endianness by
                    the compiler generating the SHT_GROUP.
                    This is pretty horrible. */

                if (gsecb > ep->f_loc_shdr.g_count) {
                    *errcode = RO_ERR_GROUP_ERROR;
                    free(data);
                    free(grouparray);
                    return DW_DLV_ERROR;
                }
                /* Ok. Yes, ugly. */
                gseca = gsecb;
                grouparray[i] = gseca;
            }
            targpsh = ep->f_shdr + gseca;
            if (targpsh->gh_section_group_number) {
                /* multi-assignment to groups. Oops. */
                free(data);
                free(grouparray);
                *errcode = RO_ERR_GROUP_ERROR;
                return DW_DLV_ERROR;
            }
            targpsh->gh_section_group_number =
                ep->f_sg_next_group_number;
            foundone = 1;
        }
        if (foundone) {
            ++ep->f_sg_next_group_number;
            ++ep->f_sht_group_type_section_count;
        }
        free(data);
        psh->gh_sht_group_array = grouparray;
        psh->gh_sht_group_array_count = count;
    }
    return DW_DLV_OK;
}
/*  Does related things.
    A)  Counts the number of SHT_GROUP
        and for each builds an array of the sections in the group
        (which we expect are all DWARF-related)
        and sets the group number in each mentioned section.
    B)  Counts the number of SHF_GROUP flags.
    C)  If gnu groups:
        ensure all the DWARF sections marked with right group
        based on A(we will mark unmarked as group 1,
        DW_GROUPNUMBER_BASE).
    D)  If arm groups (SHT_GROUP zero, SHF_GROUP non-zero):
        Check the relocations of all SHF_GROUP section
        FIXME: algorithm needed.


    If SHT_GROUP and SHF_GROUP this is GNU groups.
    If no SHT_GROUP and have SHF_GROUP this is
    arm cc groups and we must use relocation information
    to identify the group members.

    It seems(?) impossible for an object to have both
    dwo sections and (SHF_GROUP or SHT_GROUP), but
    we do not rule that out here.  */
static int
elf_setup_all_section_groups(elf_filedata ep,
    int *errcode)
{
    struct generic_shdr* psh = 0;
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned count = 0;
    int res = 0;

    count = ep->f_loc_shdr.g_count;
    psh = ep->f_shdr;

    /* Does step A and step B */
    for (i = 0; i < count; ++psh,++i) {
        const char *name = psh->gh_namestring;
        if (is_empty_section(psh->gh_type)) {
            /*  No data here. */
            continue;
        }
        if (!elf_sht_groupsec(psh->gh_type,name)) {
            /* Step B */
            if (elf_flagmatches(psh->gh_flags,SHF_GROUP)) {
                ep->f_shf_group_flag_section_count++;
            }
            continue;
        }
        /* Looks like a section group. Do Step A. */
        res  =read_gs_section_group(ep,psh,errcode);
        if (res != DW_DLV_OK) {
            return res;
        }
    }
    /*  Any sections not marked above or here are in
        grep DW_GROUPNUMBER_BASE (1).
        Section C. */
    psh = ep->f_shdr;
    for (i = 0; i < count; ++psh,++i) {
        const char *name = psh->gh_namestring;

        if (is_empty_section(psh->gh_type)) {
            /*  No data here. */
            continue;
        }
        if (elf_sht_groupsec(psh->gh_type,name)) {
            continue;
        }
        /* Not a section group */
        if(string_endswith(name,".dwo")) {
            if (psh->gh_section_group_number) {
                /* multi-assignment to groups. Oops. */
                *errcode = RO_ERR_GROUP_ERROR;
                return DW_DLV_ERROR;
            }
            psh->gh_is_dwarf = TRUE;
            psh->gh_section_group_number = DW_GROUPNUMBER_DWO;
            ep->f_dwo_group_section_count++;
        } else if (dwarf_load_elf_section_is_dwarf(name)) {
            if(!psh->gh_section_group_number) {
                psh->gh_section_group_number = DW_GROUPNUMBER_BASE;
            }
            psh->gh_is_dwarf = TRUE;
        } else {
            /* Do nothing. */
        }
    }
    if (ep->f_sht_group_type_section_count) {
        /*  Not ARM. Done. */
    }
    if (!ep->f_shf_group_flag_section_count) {
        /*  Nothing more to do. */
        return DW_DLV_OK;
    }
    return DW_DLV_OK;
}

static int
elf_find_sym_sections(elf_filedata ep,
    int *errcode)
{
    struct generic_shdr* psh = 0;
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned count = 0;
    int res = 0;

    count = ep->f_loc_shdr.g_count;
    psh = ep->f_shdr;
    for (i = 0; i < count; ++psh,++i) {
        const char *name = psh->gh_namestring;
        if (is_empty_section(psh->gh_type)) {
            /*  No data here.  With a corrupted object
                the name could be important, like .symtab,
                but with no content we ignore the empty. */
            continue;
        }
        if (!strcmp(name,".dynsym")) {
            ep->f_dynsym_sect_index = i;
            ep->f_loc_dynsym.g_offset = psh->gh_offset;
        } else if (!strcmp(name,".dynstr")) {
            ep->f_dynsym_sect_strings_sect_index = i;
            ep->f_dynsym_sect_strings_max = psh->gh_size;
        } else if (!strcmp(name,".symtab")) {
            ep->f_symtab_sect_index = i;
            ep->f_loc_symtab.g_offset = psh->gh_offset;
        } else if (!strcmp(name,".strtab")) {
            ep->f_symtab_sect_strings_sect_index = i;
            ep->f_symtab_sect_strings_max = psh->gh_size;
        } else if (!strcmp(name,".dynamic")) {
            ep->f_dynamic_sect_index = i;
            ep->f_loc_dynamic.g_offset = psh->gh_offset;
        }
    }

    res = validate_links(".dynsym",ep,ep->f_dynsym_sect_index,
        ep->f_dynsym_sect_strings_sect_index,errcode);
    if (res!= DW_DLV_OK) {
        return res;
    }
    res = validate_links(".symtab",ep,ep->f_symtab_sect_index,
        ep->f_symtab_sect_strings_sect_index,errcode);
    if (res!= DW_DLV_OK) {
        return res;
    }
    res = validate_links(".dynamic",ep,ep->f_dynamic_sect_index,
        ep->f_dynsym_sect_strings_sect_index,errcode);
    if (res!= DW_DLV_OK) {
        return res;
    }
    return DW_DLV_OK;
}


int
dwarf_load_elf_sectheaders(elf_filedata ep,int*errcode)
{
    int res = 0;

    if (ep->f_offsetsize == 32) {
        res  = elf_load_sectheaders32(ep,ep->f_ehdr->ge_shoff,
            ep->f_ehdr->ge_shentsize,
            ep->f_ehdr->ge_shnum,errcode);
    } else {
        res  = elf_load_sectheaders64(ep,ep->f_ehdr->ge_shoff,
            ep->f_ehdr->ge_shentsize,
            ep->f_ehdr->ge_shnum,errcode);
    }
    if (res != DW_DLV_OK) {
        return res;
    }
    res  = elf_load_sectstrings(ep,ep->f_ehdr->ge_shstrndx,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    res  = elf_load_sect_namestring(ep,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    elf_check_sect_sizes(ep);
    res  = elf_find_sym_sections(ep,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    res = elf_setup_all_section_groups(ep,errcode);

    return res;
}
int
dwarf_load_elf_progheaders(elf_filedata ep,int*errcode)
{
    int res =  DW_DLV_NO_ENTRY;
    if (ep->f_offsetsize == 32) {
        if (ep->f_ehdr->ge_phnum) {
            res = elf_load_progheaders32(ep, ep->f_ehdr->ge_phoff,
                ep->f_ehdr->ge_phentsize,
                ep->f_ehdr->ge_phnum,errcode);
        }
    } else if (ep->f_offsetsize == 64) {
        if (ep->f_ehdr->ge_phnum) {
            res = elf_load_progheaders64(ep, ep->f_ehdr->ge_phoff,
                ep->f_ehdr->ge_phentsize,
                ep->f_ehdr->ge_phnum,errcode);
        }
    } else {
        *errcode = RO_ERR_BADOFFSETSIZE;
        return DW_DLV_ERROR;
    }
    elf_check_phdr_sizes(ep);
    return res;
}

int
dwarf_load_elf_dynamic(elf_filedata ep, int *errcode)
{
    struct generic_shdr *sp = 0;
    int res = 0;
    unsigned offsetsize = ep->f_offsetsize;

    if (!ep->f_dynamic_sect_index) {
        return DW_DLV_NO_ENTRY;
    }
    sp = ep->f_shdr + ep->f_dynamic_sect_index;
    if (offsetsize == 32) {
        res = elf_load_dynamic32(ep,sp->gh_offset, sp->gh_size,errcode);
    } else {
        res = elf_load_dynamic64(ep,sp->gh_offset, sp->gh_size,errcode);
    }
    return res;
}

int
dwarf_load_elf_section_is_dwarf(const char *sname)
{
    if (!strncmp(sname,".rel",4)) {
        return FALSE;
    }
    if (!strncmp(sname,".debug_",7)) {
        return TRUE;
    }
    if (!strncmp(sname,".zdebug_",8)) {
        return TRUE;
    }
    if (!strcmp(sname,".eh_frame")) {
        return TRUE;
    }
    if (!strncmp(sname,".gdb_index",10)) {
        return TRUE;
    }
    return FALSE;
}


/*  Used in object checkers. */
void
dwarf_insert_in_use_entry(elf_filedata ep,
    const char *description,Dwarf_Unsigned offset,
    Dwarf_Unsigned length,Dwarf_Unsigned align)
{
    struct in_use_s *e = 0;

    e = (struct in_use_s *)calloc(1,sizeof(struct in_use_s));
    if(!e) {
        P("ERROR: Out of memory creating in-use entry " LONGESTUFMT
            " Giving up.\n",ep->f_in_use_count);
        exit(1);
    }
    e->u_next = 0;
    e->u_name = description;
    e->u_offset = offset;
    e->u_align = align;
    e->u_length = length;
    e->u_lastbyte = offset+length;
    ++ep->f_in_use_count;
    if (ep->f_in_use) {
        ep->f_in_use_tail->u_next = e;
        ep->f_in_use_tail = e;
        return;
    }
    ep->f_in_use = e;
    ep->f_in_use_tail = e;
}
