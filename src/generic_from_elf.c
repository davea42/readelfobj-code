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
#include <stdlib.h>
#include <elf.h>
#include "reading.h"
#include "readelfobj.h"
#include "sanitized.h"


static char buffer3[BUFFERSIZE];
static char buffer1[BUFFERSIZE];

int
generic_ehdr_from_32(struct generic_ehdr *ehdr, Elf32_Ehdr *e)
{
    int i = 0;

    for (i = 0; i < EI_NIDENT; ++i) {
        ehdr->ge_ident[i] = e->e_ident[i];
    }
    ASSIGN(ehdr->ge_type,e->e_type);
    ASSIGN(ehdr->ge_machine,e->e_machine);
    ASSIGN(ehdr->ge_version,e->e_version);
    ASSIGN(ehdr->ge_entry,e->e_entry);
    ASSIGN(ehdr->ge_phoff,e->e_phoff);
    ASSIGN(ehdr->ge_shoff,e->e_shoff);
    ASSIGN(ehdr->ge_flags,e->e_flags);
    ASSIGN(ehdr->ge_ehsize,e->e_ehsize);
    ASSIGN(ehdr->ge_phentsize,e->e_phentsize);
    ASSIGN(ehdr->ge_phnum,e->e_phnum);
    ASSIGN(ehdr->ge_shentsize,e->e_shentsize);
    ASSIGN(ehdr->ge_shnum,e->e_shnum);
    ASSIGN(ehdr->ge_shstrndx,e->e_shstrndx);
    filedata.f_ehdr = ehdr;
    filedata.f_loc_ehdr.g_name = "Elf File Header";
    filedata.f_loc_ehdr.g_offset = 0;
    filedata.f_loc_ehdr.g_count = 1;
    filedata.f_loc_ehdr.g_entrysize = sizeof(Elf32_Ehdr);
    filedata.f_loc_ehdr.g_totalsize = sizeof(Elf32_Ehdr);
    return RO_OK;
}
int
generic_ehdr_from_64(struct generic_ehdr *ehdr, Elf64_Ehdr *e)
{
    int i = 0;

    for (i = 0; i < EI_NIDENT; ++i) {
        ehdr->ge_ident[i] = e->e_ident[i];
    }
    ASSIGN(ehdr->ge_type,e->e_type);
    ASSIGN(ehdr->ge_machine,e->e_machine);
    ASSIGN(ehdr->ge_version,e->e_version);
    ASSIGN(ehdr->ge_entry,e->e_entry);
    ASSIGN(ehdr->ge_phoff,e->e_phoff);
    ASSIGN(ehdr->ge_shoff,e->e_shoff);
    ASSIGN(ehdr->ge_flags,e->e_flags);
    ASSIGN(ehdr->ge_ehsize,e->e_ehsize);
    ASSIGN(ehdr->ge_phentsize,e->e_phentsize);
    ASSIGN(ehdr->ge_phnum,e->e_phnum);
    ASSIGN(ehdr->ge_shentsize,e->e_shentsize);
    ASSIGN(ehdr->ge_shnum,e->e_shnum);
    ASSIGN(ehdr->ge_shstrndx,e->e_shstrndx);
    filedata.f_ehdr = ehdr;
    filedata.f_loc_ehdr.g_name = "Elf File Header";
    filedata.f_loc_ehdr.g_offset = 0;
    filedata.f_loc_ehdr.g_count = 1;
    filedata.f_loc_ehdr.g_entrysize = sizeof(Elf64_Ehdr);
    filedata.f_loc_ehdr.g_totalsize = sizeof(Elf64_Ehdr);
    return RO_OK;
}


int
generic_phdr_from_phdr32(struct generic_phdr **phdr_out,
    LONGESTUTYPE * count_out,
    LONGESTUTYPE offset,
    LONGESTUTYPE entsize,
    LONGESTUTYPE count)
{
    Elf32_Phdr *pph =0;
    Elf32_Phdr *orig_pph =0;
    struct generic_phdr *gphdr =0;
    struct generic_phdr *orig_gphdr =0;
    LONGESTUTYPE i = 0;
    int res = 0;

    *count_out = 0;
    res =(int)SEEKTO(offset);
    if (res != RO_OK) {
        P("Seek to " LONGESTUFMT " to read program headers failed\n",
            offset);
        return res;
    }
    pph = (Elf32_Phdr *)calloc(count , entsize);
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
    res = RN(pph,count*entsize);
    if(res != RO_OK) {
        P("Read  " LONGESTUFMT
            " bytes program headers failed\n",count*entsize);
        free(pph);
        free(gphdr);
        return res;
    }
    for( i = 0; i < count;
        ++i,  pph++,gphdr++) {
        ASSIGN(gphdr->gp_type,pph->p_type);
        ASSIGN(gphdr->gp_offset,pph->p_offset);
        ASSIGN(gphdr->gp_vaddr,pph->p_vaddr);
        ASSIGN(gphdr->gp_paddr,pph->p_paddr);
        ASSIGN(gphdr->gp_filesz,pph->p_filesz);
        ASSIGN(gphdr->gp_memsz,pph->p_memsz);
        ASSIGN(gphdr->gp_flags,pph->p_flags);
        ASSIGN(gphdr->gp_align,pph->p_align);
        insert_in_use_entry("Phdr target",gphdr->gp_offset,gphdr->gp_filesz,
           gphdr->gp_align);
    }
    free(orig_pph);
    *phdr_out = orig_gphdr;
    *count_out = count;
    filedata.f_phdr = orig_gphdr;
    filedata.f_loc_phdr.g_name = "Program Header";
    filedata.f_loc_phdr.g_offset = offset;
    filedata.f_loc_phdr.g_count = count;
    filedata.f_loc_phdr.g_entrysize = sizeof(Elf32_Phdr);
    filedata.f_loc_phdr.g_totalsize = sizeof(Elf32_Phdr)*count;
    return RO_OK;
}

int
generic_phdr_from_phdr64(struct generic_phdr **phdr_out,
    LONGESTUTYPE * count_out,
    LONGESTUTYPE offset,
    LONGESTUTYPE entsize,
    LONGESTUTYPE count)
{
    Elf64_Phdr *pph =0;
    Elf64_Phdr *orig_pph =0;
    struct generic_phdr *gphdr =0;
    struct generic_phdr *orig_gphdr =0;
    int res = 0;
    LONGESTUTYPE i = 0;

    *count_out = 0;
    res =(int)SEEKTO(offset);
    if (res != RO_OK) {
        P("Seek to " LONGESTUFMT " to read program headers failed\n",offset);
        return res;
    }
    pph = (Elf64_Phdr *)calloc(count , entsize);
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
    res = RN(pph,count*entsize);
    if(res != RO_OK) {
        P("Read  " LONGESTUFMT
            " bytes program headers failed\n",count*entsize);
        free(pph);
        free(gphdr);
        return res;
    }
    for( i = 0; i < count;
        ++i,  pph++,gphdr++) {
        ASSIGN(gphdr->gp_type,pph->p_type);
        ASSIGN(gphdr->gp_offset,pph->p_offset);
        ASSIGN(gphdr->gp_vaddr,pph->p_vaddr);
        ASSIGN(gphdr->gp_paddr,pph->p_paddr);
        ASSIGN(gphdr->gp_filesz,pph->p_filesz);
        ASSIGN(gphdr->gp_memsz,pph->p_memsz);
        ASSIGN(gphdr->gp_flags,pph->p_flags);
        ASSIGN(gphdr->gp_align,pph->p_align);
        insert_in_use_entry("Phdr target",gphdr->gp_offset,gphdr->gp_filesz,
           gphdr->gp_align);
    }
    free(orig_pph);
    *phdr_out = orig_gphdr;
    *count_out = count;
    filedata.f_phdr = orig_gphdr;
    filedata.f_loc_phdr.g_name = "Program Header";
    filedata.f_loc_phdr.g_offset = offset;
    filedata.f_loc_phdr.g_count = count;
    filedata.f_loc_phdr.g_entrysize = sizeof(Elf64_Phdr);
    filedata.f_loc_phdr.g_totalsize = sizeof(Elf64_Phdr)*count;
    return RO_OK;
}

int
generic_shdr_from_shdr32(struct generic_shdr **hdr_out,
    LONGESTUTYPE * count_out,
    LONGESTUTYPE offset,
    LONGESTUTYPE entsize,
    LONGESTUTYPE count)
{
    Elf32_Shdr          *psh =0;
    Elf32_Shdr          *orig_psh =0;
    struct generic_shdr *gshdr =0;
    struct generic_shdr *orig_gshdr =0;
    LONGESTUTYPE i = 0;
    int res = 0;

    *count_out = 0;
    res =(int)SEEKTO(offset);
    if (res != RO_OK) {
        P("Seek to " LONGESTUFMT " to read setion headers failed\n",offset);
        return res;
    }
    psh = (Elf32_Shdr *)calloc(count , entsize);
    if(psh == 0) {
        P("malloc of " LONGESTUFMT
            " bytes of section header space failed\n",count *entsize);
        return RO_ERR_MALLOC;
    }
    gshdr = (struct generic_shdr *)calloc(count,sizeof(*gshdr));
    if(gshdr == 0) {
        free(psh);
        P("malloc of " LONGESTUFMT
            " bytes of generic section header space failed\n",
            count *entsize);
        return RO_ERR_MALLOC;
    }

    orig_psh = psh;
    orig_gshdr = gshdr;
    res = RN(psh,count*entsize);
    if(res != RO_OK) {
        P("Read  " LONGESTUFMT
            " bytes section headers failed\n",count*entsize);
        free(psh);
        free(gshdr);
        return res;
    }
    for( i = 0; i < count;
        ++i,  psh++,gshdr++) {
        gshdr->gh_secnum = i;
        ASSIGN(gshdr->gh_name,psh->sh_name);
        ASSIGN(gshdr->gh_type,psh->sh_type);
        ASSIGN(gshdr->gh_flags,psh->sh_flags);
        ASSIGN(gshdr->gh_addr,psh->sh_addr);
        ASSIGN(gshdr->gh_offset,psh->sh_offset);
        ASSIGN(gshdr->gh_size,psh->sh_size);
        ASSIGN(gshdr->gh_link,psh->sh_link);
        ASSIGN(gshdr->gh_info,psh->sh_info);
        ASSIGN(gshdr->gh_addralign,psh->sh_addralign);
        ASSIGN(gshdr->gh_entsize,psh->sh_entsize);
        if (gshdr->gh_type != SHT_NOBITS) {
            insert_in_use_entry("Shdr target",gshdr->gh_offset,gshdr->gh_size,1);
        }
    }
    free(orig_psh);
    *hdr_out = orig_gshdr;
    *count_out = count;
    filedata.f_shdr = orig_gshdr;
    filedata.f_loc_shdr.g_name = "Section Header";
    filedata.f_loc_shdr.g_count = count;
    filedata.f_loc_shdr.g_offset = offset;
    filedata.f_loc_shdr.g_entrysize = sizeof(Elf32_Shdr);
    filedata.f_loc_shdr.g_totalsize = sizeof(Elf32_Shdr)*count;
    return RO_OK;
}

int
generic_shdr_from_shdr64(struct generic_shdr **hdr_out,
    LONGESTUTYPE * count_out,
    LONGESTUTYPE offset,
    LONGESTUTYPE entsize,
    LONGESTUTYPE count)
{
    Elf64_Shdr          *psh =0;
    Elf64_Shdr          *orig_psh =0;
    struct generic_shdr *gshdr =0;
    struct generic_shdr *orig_gshdr =0;
    LONGESTUTYPE i = 0;
    int res = 0;

    *count_out = 0;
    res =(int)SEEKTO(offset);
    if (res != RO_OK) {
        P("Seek to " LONGESTUFMT " to read setion headers failed\n",offset);
        return res;
    }
    psh = (Elf64_Shdr *)calloc(count , entsize);
    if(psh == 0) {
        P("malloc of " LONGESTUFMT
            " bytes of section header space failed\n",count *entsize);
        return RO_ERR_MALLOC;
    }
    gshdr = (struct generic_shdr *)calloc(count,sizeof(*gshdr));
    if(gshdr == 0) {
        free(psh);
        P("malloc of " LONGESTUFMT
            " bytes of generic section header space failed\n",
            count *entsize);
        return RO_ERR_MALLOC;
    }

    orig_psh = psh;
    orig_gshdr = gshdr;
    res = RN(psh,count*entsize);
    if(res != RO_OK) {
        P("Read  " LONGESTUFMT
            " bytes section headers failed\n",count*entsize);
        free(psh);
        free(gshdr);
        return res;
    }
    for( i = 0; i < count;
        ++i,  psh++,gshdr++) {
        ASSIGN(gshdr->gh_name,psh->sh_name);
        ASSIGN(gshdr->gh_type,psh->sh_type);
        ASSIGN(gshdr->gh_flags,psh->sh_flags);
        ASSIGN(gshdr->gh_addr,psh->sh_addr);
        ASSIGN(gshdr->gh_offset,psh->sh_offset);
        ASSIGN(gshdr->gh_size,psh->sh_size);
        ASSIGN(gshdr->gh_link,psh->sh_link);
        ASSIGN(gshdr->gh_info,psh->sh_info);
        ASSIGN(gshdr->gh_addralign,psh->sh_addralign);
        ASSIGN(gshdr->gh_entsize,psh->sh_entsize);
        if (gshdr->gh_type != SHT_NOBITS) {
            insert_in_use_entry("Shdr target",gshdr->gh_offset,gshdr->gh_size,1);
        }
    }
    free(orig_psh);
    *hdr_out = orig_gshdr;
    *count_out = count;
    filedata.f_shdr = orig_gshdr;
    filedata.f_loc_shdr.g_name = "Section Header";
    filedata.f_loc_shdr.g_count = count;
    filedata.f_loc_shdr.g_offset = offset;
    filedata.f_loc_shdr.g_entrysize = sizeof(Elf64_Shdr);
    filedata.f_loc_shdr.g_totalsize = sizeof(Elf64_Shdr)*count;
    return RO_OK;
}


int
generic_elf_load_symbols32(int secnum,const char *secname,
    struct generic_symentry **gsym_out,LONGESTUTYPE offset,LONGESTUTYPE size,
    LONGESTUTYPE *count_out)
{
    LONGESTUTYPE ecount = 0;
    LONGESTUTYPE size2 = 0;
    LONGESTUTYPE i = 0;
    Elf32_Sym *psym = 0;
    Elf32_Sym *orig_psym = 0;
    struct generic_symentry * gsym = 0;
    struct generic_symentry * orig_gsym = 0;
    int res = 0;

    ecount = (long)(size/sizeof(Elf32_Sym));
    size2 = ecount * sizeof(Elf32_Sym);
    if(size != size2) {
        P("ERROR: Bogus size of symbols. "
            LONGESTUFMT " not divisible by %lu\n",
            size,(unsigned long)sizeof(Elf32_Sym));
        return RO_ERR;
    }
    psym = calloc(ecount,sizeof(Elf32_Sym));
    if (!psym) {
        P("ERROR:  Unable to malloc Elf32_Sym strings for section %d (%s) "
            "at offset " LONGESTXFMT "\n",
            secnum,sanitized(secname,buffer3,BUFFERSIZE),offset);
        return RO_ERR_MALLOC;
    }
    gsym = calloc(ecount,sizeof(struct generic_symentry));
    if (!gsym) {
        free(psym);
        P("ERROR:  Unable to malloc generic_symentry "
            "strings for section %d (%s) "
            "at offset " LONGESTXFMT "\n",
            secnum,sanitized(secname,buffer3,BUFFERSIZE),offset);
        return RO_ERR_MALLOC;
    }
    res = RR(psym,offset,size);
    if(res!= RO_OK) {
        P("ERROR: could not read whole symbol section of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            sanitized(filename,buffer3,BUFFERSIZE),
            offset,size);
        return res;
    }
    orig_psym = psym;
    orig_gsym = gsym;
    for ( i = 0; i < ecount; ++i,++psym,++gsym) {
        LONGESTUTYPE bind = 0;
        LONGESTUTYPE type = 0;

        ASSIGN(gsym->gs_name,psym->st_name);
        ASSIGN(gsym->gs_value,psym->st_value);
        ASSIGN(gsym->gs_size,psym->st_size);
        ASSIGN(gsym->gs_info,psym->st_info);
        ASSIGN(gsym->gs_other,psym->st_other);
        ASSIGN(gsym->gs_shndx,psym->st_shndx);
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


int
generic_elf_load_symbols64(int secnum,const char *secname,
    struct generic_symentry **gsym_out,LONGESTUTYPE offset,LONGESTUTYPE size,
    LONGESTUTYPE *count_out)
{
    LONGESTUTYPE ecount = 0;
    LONGESTUTYPE size2 = 0;
    LONGESTUTYPE i = 0;
    Elf64_Sym *psym = 0;
    Elf64_Sym *orig_psym = 0;
    struct generic_symentry * gsym = 0;
    struct generic_symentry * orig_gsym = 0;
    int res = 0;

    ecount = (long)(size/sizeof(Elf64_Sym));
    size2 = ecount * sizeof(Elf64_Sym);
    if(size != size2) {
        P("ERROR: Bogus size of symbols. "
            LONGESTUFMT " not divisible by %lu\n",
            size,(unsigned long)sizeof(Elf64_Sym));
        return RO_ERR;
    }
    psym = calloc(ecount,sizeof(Elf64_Sym));
    if (!psym) {
        P("ERROR:  Unable to malloc Elf64_Sym strings "
            "for section %d (%s) "
            "at offset " LONGESTXFMT "\n",
            secnum,sanitized(secname,buffer3,BUFFERSIZE),offset);
        return RO_ERR_MALLOC;
    }
    gsym = calloc(ecount,sizeof(struct generic_symentry));
    if (!gsym) {
        free(psym);
        P("ERROR:  Unable to malloc generic_symentry "
            "strings for section %d (%s) "
            "at offset " LONGESTXFMT "\n",
            secnum,
            sanitized(secname,buffer3,BUFFERSIZE),offset);
        return RO_ERR_MALLOC;
    }
    res = RR(psym,offset,size);
    if(res!= RO_OK) {
        P("ERROR: Could not read whole symbol section of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            sanitized(filename,buffer3,BUFFERSIZE),
            offset,size);
        return res;
    }
    orig_psym = psym;
    orig_gsym = gsym;
    for ( i = 0; i < ecount; ++i,++psym,++gsym) {
        LONGESTUTYPE bind = 0;
        LONGESTUTYPE type = 0;

        ASSIGN(gsym->gs_name,psym->st_name);
        ASSIGN(gsym->gs_value,psym->st_value);
        ASSIGN(gsym->gs_size,psym->st_size);
        ASSIGN(gsym->gs_info,psym->st_info);
        ASSIGN(gsym->gs_other,psym->st_other);
        ASSIGN(gsym->gs_shndx,psym->st_shndx);
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
generic_rel_from_rela32(struct generic_shdr * gsh,
    Elf32_Rela *relp,
    struct generic_rela *grel)
{
    LONGESTUTYPE ecount = 0;
    LONGESTUTYPE size = gsh->gh_size;
    LONGESTUTYPE size2 = 0;
    LONGESTUTYPE i = 0;

    ecount = size/sizeof(Elf32_Rela);
    size2 = ecount * sizeof(Elf32_Rela);
    if(size != size2) {
        P("ERROR: Bogus size of relocations section "
            LONGESTUFMT ". "
            " not divisible by %u\n",
            size,(unsigned)sizeof(Elf32_Rela));
        return  RO_ERR;
    }
    for ( i = 0; i < ecount; ++i,++relp,++grel) {
        ASSIGN(grel->gr_offset,relp->r_offset);
        ASSIGN(grel->gr_info,relp->r_info);
        ASSIGN(grel->gr_addend,relp->r_addend);
        grel->gr_sym  = relp->r_info >>8; /* ELF32_R_SYM */
        grel->gr_type = relp->r_info  & 0xff; /* ELF32_R_TYPE */
    }
    return RO_OK;
}

static int
generic_rel_from_rela64(struct generic_shdr * gsh,
    Elf64_Rela *relp,
    struct generic_rela *grel)
{
    LONGESTUTYPE ecount = 0;
    LONGESTUTYPE size = gsh->gh_size;
    LONGESTUTYPE size2 = 0;
    LONGESTUTYPE i = 0;

    ecount = size/sizeof(Elf64_Rela);
    size2 = ecount * sizeof(Elf64_Rela);
    if(size != size2) {
        P("ERROR: Bogus size of relocations section "
            LONGESTUFMT ". "
            " not divisible by %u\n",
            size,(unsigned)sizeof(Elf64_Rela));
        return  RO_ERR;
    }
    for ( i = 0; i < ecount; ++i,++relp,++grel) {
        ASSIGN(grel->gr_offset,relp->r_offset);
        ASSIGN(grel->gr_info,relp->r_info);
        ASSIGN(grel->gr_addend,relp->r_addend);
        grel->gr_sym  = relp->r_info >>8; /* ELF64_R_SYM */
        grel->gr_type = relp->r_info  & 0xff; /* ELF64_R_TYPE */
    }
    return RO_OK;
}

int
generic_rel_from_rel32(struct generic_shdr * gsh,
    Elf32_Rel *relp,
    struct generic_rela *grel)
{
    LONGESTUTYPE ecount = 0;
    LONGESTUTYPE size = gsh->gh_size;
    LONGESTUTYPE size2 = 0;
    LONGESTUTYPE i = 0;

    ecount = size/sizeof(Elf32_Rel);
    size2 = ecount * sizeof(Elf32_Rel);
    if(size != size2) {
        P("ERROR: Bogus size of relocations section "
            LONGESTUFMT ". "
            " not divisible by %lu\n",
            size,(unsigned long)sizeof(Elf32_Rel));
        return RO_ERR;
    }
    for ( i = 0; i < ecount; ++i,++relp,++grel) {
        grel->gr_isrela = 0;
        ASSIGN(grel->gr_offset,relp->r_offset);
        ASSIGN(grel->gr_info,relp->r_info);
        grel->gr_addend  = 0; /* Unused for plain .rel */
        grel->gr_sym  = relp->r_info >>8; /* ELF32_R_SYM */
        grel->gr_type = relp->r_info  & 0xff; /* ELF32_R_TYPE */
    }
    return RO_OK;
}

int
generic_rel_from_rel64(struct generic_shdr * gsh,
    Elf64_Rel *relp,
    struct generic_rela *grel)
{
    LONGESTUTYPE ecount = 0;
    LONGESTUTYPE size = gsh->gh_size;
    LONGESTUTYPE size2 = 0;
    LONGESTUTYPE i = 0;

    ecount = size/sizeof(Elf64_Rel);
    size2 = ecount * sizeof(Elf64_Rel);
    if(size != size2) {
        P("ERROR: Bogus size of relocations section "
            LONGESTUFMT ". "
            " not divisible by %lu\n",
            size,(unsigned long)sizeof(Elf64_Rel));
        return RO_ERR;
    }
    for ( i = 0; i < ecount; ++i,++relp,++grel) {
        grel->gr_isrela = 0;
        ASSIGN(grel->gr_offset,relp->r_offset);
        ASSIGN(grel->gr_info,relp->r_info);
        grel->gr_addend  = 0; /* Unused for plain .rel */
        grel->gr_sym  = relp->r_info >>8; /* ELF64_R_SYM */
        grel->gr_type = relp->r_info  & 0xff; /* ELF64_R_TYPE */
    }
    return RO_OK;
}


int
elf_load_dynstr(int isdynsym,
    LONGESTUTYPE strsectindex,
    LONGESTUTYPE strsectlength)
{
    struct generic_shdr *strpsh = 0;
    int res = 0;

    strpsh = filedata.f_shdr + strsectindex;
    if (isdynsym) {
        filedata.f_dynsym_sect_strings = malloc(strsectlength);
        if(!filedata.f_dynsym_sect_strings) {
            P("ERROR: Unable to malloc " LONGESTXFMT " bytes for "
                "dynsym strings table\n",strsectlength);
            filedata.f_dynsym_sect_strings = 0;
            filedata.f_dynsym_sect_strings_max = 0;
            filedata.f_dynsym_sect_strings_sect_index = 0;
            return RO_ERR;
        }
        res = RR(filedata.f_dynsym_sect_strings,
            strpsh->gh_offset,
            strsectlength);
        if(res != RO_OK) {
            P("ERROR: Could not read dynsym section strings at "
                LONGESTXFMT " (" LONGESTUFMT  ")\n",
                strpsh->gh_offset,strpsh->gh_offset);
            filedata.f_dynsym_sect_strings = 0;
            filedata.f_dynsym_sect_strings_max = 0;
            filedata.f_dynsym_sect_strings_sect_index = 0;
            return res;
        }
    } else {
        filedata.f_symtab_sect_strings = malloc(strsectlength);
        if(!filedata.f_symtab_sect_strings) {
            P("ERROR: Unable to malloc " LONGESTXFMT " bytes for "
                "symtab strings table\n",strsectlength);
            filedata.f_symtab_sect_strings = 0;
            filedata.f_symtab_sect_strings_max = 0;
            filedata.f_symtab_sect_strings_sect_index = 0;
            return res;
        }
        res = RR(filedata.f_symtab_sect_strings,
            strpsh->gh_offset,
            strsectlength);
        if(res != RO_OK) {
            P("ERROR: Could not read symtab section strings at "
                LONGESTXFMT " (" LONGESTUFMT  ")\n",
                strpsh->gh_offset,strpsh->gh_offset);
            filedata.f_symtab_sect_strings = 0;
            filedata.f_symtab_sect_strings_max = 0;
            filedata.f_symtab_sect_strings_sect_index = 0;
            return res;
        }
    }
    return RO_OK;
}


char *
get_dynstr_string(LONGESTUTYPE offset, LONGESTUTYPE index)
{
    if(index != filedata.f_dynsym_sect_strings_sect_index) {
        P("link is " LONGESTUFMT ", sect found was " LONGESTUFMT  "\n",
            index,
            filedata.f_dynsym_sect_strings_sect_index);
        return "dynsym section link does not match section of dynamic strings";
    }
    if(offset >= filedata.f_dynsym_sect_strings_max) {
        return "offset beyond end of dynsym sect strings";
    }
    return filedata.f_dynsym_sect_strings + offset;
}

char *
get_symstr_string(int is_symtab,LONGESTUTYPE offset)
{
    if (is_symtab) {
        if(offset >= filedata.f_symtab_sect_strings_max) {
            return "offset beyond end of .strtab strings";
        }
        return filedata.f_symtab_sect_strings + offset;
    }
    if(offset >= filedata.f_dynsym_sect_strings_max) {
        return "offset beyond end of .dynstr strings";
    }
    return filedata.f_dynsym_sect_strings + offset;
}



int
elf_load_sectstrings(LONGESTUTYPE stringsection)
{
    int i = 0;
    struct generic_shdr *psh = 0;
    LONGESTUTYPE secoffset = 0;

    filedata.f_elf_shstrings_length = 0;
    if (stringsection >= filedata.f_ehdr->ge_shnum) {
        printf("String section " LONGESTUFMT " invalid. Ignored.",
            stringsection);
        return RO_ERR;
    }
    psh = filedata.f_shdr + stringsection;
    secoffset = psh->gh_offset;
    if(psh->gh_type == SHT_NULL) {
        P("String section type SHT_NULL!!. "
            "No section string section!\n");
        return RO_ERR;
    }
    if(psh->gh_size > filedata.f_elf_shstrings_max) {
        free(filedata.f_elf_shstrings_data);
        filedata.f_elf_shstrings_data = (char *)malloc(psh->gh_size);
        filedata.f_elf_shstrings_max = psh->gh_size;
        if(filedata.f_elf_shstrings_data == 0) {
            filedata.f_elf_shstrings_max = 0;
            P("Unable to malloc %ld bytes for strings\n",
                (long)psh->gh_size);
            return RO_ERR_MALLOC;
        }
    }
    filedata.f_elf_shstrings_length = psh->gh_size;
    i =(int)SEEKTO(secoffset);
    if(i != RO_OK) {
        P("Seek to " LONGESTUFMT " loading shstrings failed\n",secoffset);
        return i;
    }
    i = RN(filedata.f_elf_shstrings_data,psh->gh_size);
    if(i != RO_OK) {
        P("Read  " LONGESTUFMT" bytes of shstrings section string "
            "data failed\n",filedata.f_elf_shstrings_length);
        filedata.f_elf_shstrings_length = 0;
        return i;
    }
    return RO_OK;
}

int
elf_load_progheaders32(LONGESTUTYPE offset,LONGESTUTYPE entsize,LONGESTUTYPE count)
{
    struct generic_phdr *gphdr =0;
    LONGESTUTYPE generic_count = 0;
    int res = 0;

    if(count == 0) {
        const char *p = "";
        if (printfilenames) {
            p = sanitized(filename,buffer1,BUFFERSIZE);
        }
        P("No program headers %s\n",p);
        return RO_OK;
    }
    if(entsize < sizeof(Elf32_Phdr)) {
        P("ERROR: Elf Program header too small? "
            LONGESTUFMT  " vs "
            LONGESTUFMT "\n",
            entsize,(LONGESTUTYPE)sizeof(Elf32_Phdr));
        return RO_ERR;
    }
    if ((offset > filedata.f_filesize)||
        (entsize > 200)||
        (count > filedata.f_filesize) ||
        (((count *entsize) +offset) > filedata.f_filesize)) {
            P("ERROR: Something badly wrong with program header "
                " filesize " LONGESTUFMT
                " sectionentrysize " LONGESTUFMT
                " sectionentrycount " LONGESTUFMT
                "\n", filedata.f_filesize,entsize,count);
            return RO_ERR;
    }
    res = generic_phdr_from_phdr32(&gphdr,&generic_count,
        offset,entsize,count);
    if (res != RO_OK) {
        return res;
    }
    if (count != generic_count) {
        P("Something badly wrong reading program headers");
        return RO_ERR;
    }
    insert_in_use_entry("Elf32_Phdr block",offset,entsize*count,ALIGN4);
    return RO_OK;
}

int
elf_load_progheaders64(LONGESTUTYPE offset,LONGESTUTYPE entsize,LONGESTUTYPE count)
{
    struct generic_phdr *gphdr =0;
    LONGESTUTYPE generic_count = 0;
    int res = 0;

    if(count == 0) {
        P("No program headers\n");
        return RO_OK;
    }
    if(entsize < sizeof(Elf64_Phdr)) {
        P("ERROR: Elf Program header too small? "
            LONGESTUFMT  " vs "
            LONGESTUFMT "\n",
            entsize,(LONGESTUTYPE)sizeof(Elf64_Phdr));
        return RO_ERR;
    }
    if ((offset > filedata.f_filesize)||
        (entsize > 200)||
        (count > filedata.f_filesize) ||
        (((count *entsize) +offset) > filedata.f_filesize)) {
            P("Something badly wrong with program header "
                " filesize " LONGESTUFMT
                " sectionentrysize " LONGESTUFMT
                " sectionentrycount " LONGESTUFMT
                "\n", filedata.f_filesize,entsize,count);
            return RO_ERR;
    }
    res = generic_phdr_from_phdr64(&gphdr,&generic_count,
        offset,entsize,count);
    if (res != RO_OK) {
        return res;
    }
    if (count != generic_count) {
        P("ERROR: Something badly wrong reading program headers");
        return RO_ERR;
    }
    insert_in_use_entry("Elf64_Phdr block",offset,entsize*count,ALIGN8);
    return RO_OK;
}



int
elf_load_sectheaders32(LONGESTUTYPE offset,LONGESTUTYPE entsize,
    LONGESTUTYPE count)
{
    struct generic_shdr *gshdr = 0;
    LONGESTUTYPE generic_count = 0;
    int res = 0;


    if(entsize < sizeof(Elf32_Shdr)) {
        P("Elf Section header too small? "
            LONGESTUFMT " vs %u\n",
            entsize,(unsigned )sizeof(Elf32_Shdr));
        return RO_ERR;
    }
    if(count == 0) {
        P("No section headers\n");
        return RO_OK;
    }
    if ((offset > filedata.f_filesize)||
        (entsize > 200)||
        (count > filedata.f_filesize) ||
        ((count *entsize +offset) > filedata.f_filesize)) {
            P("Something badly wrong with section header "
                " filesize " LONGESTUFMT
                " sectionentrysize " LONGESTUFMT
                " sectionentrycount " LONGESTUFMT
                "\n", filedata.f_filesize,entsize,count);
            return RO_ERR;
    }
    res = generic_shdr_from_shdr32(&gshdr,&generic_count,
        offset,entsize,count);
    if (res != RO_OK) {
        return res;
    }
    if (generic_count != count) {
        P("Something wrong reading section headers\n");
        return RO_ERR;
    }
    insert_in_use_entry("Elf32_Shdr block",offset,entsize*count,ALIGN4);
    return RO_OK;
}


int
elf_load_sectheaders64(LONGESTUTYPE offset,LONGESTUTYPE entsize,
    LONGESTUTYPE count)
{
    struct generic_shdr *gshdr = 0;
    LONGESTUTYPE generic_count = 0;
    int res = 0;


    if(entsize < sizeof(Elf64_Shdr)) {
        P("Elf Section header too small? "
            LONGESTUFMT " vs %u\n",
            entsize,(unsigned )sizeof(Elf64_Shdr));
        return RO_ERR;
    }
    if(count == 0) {
        P("No section headers\n");
        return RO_OK;
    }
    if ((offset > filedata.f_filesize)||
        (entsize > 200)||
        (count > filedata.f_filesize) ||
        ((count *entsize +offset) > filedata.f_filesize)) {
            P("Something badly wrong with section header "
                " filesize " LONGESTUFMT
                " sectionentrysize " LONGESTUFMT
                " sectionentrycount " LONGESTUFMT
                "\n", filedata.f_filesize,entsize,count);
            return RO_ERR;
    }
    res = generic_shdr_from_shdr64(&gshdr,&generic_count,
        offset,entsize,count);
    if (res != RO_OK) {
        return res;
    }
    if (generic_count != count) {
        P("Something wrong reading section headers\n");
        return RO_ERR;
    }
    insert_in_use_entry("Elf64_Shdr block",offset,entsize*count,ALIGN8);
    return RO_OK;
}


int
elf_load_rela_32(LONGESTUTYPE secnum,
    struct generic_shdr * gsh,struct generic_rela ** grel_out,
    LONGESTUTYPE *count_out)
{
    LONGESTUTYPE count = 0;
    LONGESTUTYPE size = 0;
    LONGESTUTYPE size2 = 0;
    LONGESTUTYPE offset = 0;
    int res = 0;
    Elf32_Rela *relp = 0;
    LONGESTUTYPE reclen = sizeof(Elf32_Rela);
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
    if ((offset > filedata.f_filesize)||
        (size > filedata.f_filesize) ||
        ((size +offset) > filedata.f_filesize)) {
            P("ERROR: Something badly wrong with relocation section %s "
                " filesize " LONGESTUFMT
                " offset " LONGESTUFMT
                " size " LONGESTUFMT
                "\n",
                sanitized(gsh->gh_namestring,buffer1,BUFFERSIZE),
                filedata.f_filesize,offset,size);
            return RO_ERR;
    }

    count = (long)(size/reclen);
    size2 = count * reclen;
    if(size != size2) {
        P("ERROR: Bogus size of relocations. Section " LONGESTUFMT
            ": " LONGESTUFMT
            " not divisible by "
            LONGESTUFMT "\n",
            secnum, size,reclen);
        return RO_ERR;
    }
    relp = (Elf32_Rela *)malloc(size);
    if(!relp) {
        P("ERROR: Could not malloc whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return RO_ERR_MALLOC;
    }
    res = RR(relp,offset,size);
    if(res != RO_OK) {
        free(relp);
        P("ERROR: Could not read whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return res;
    }
    grel = (struct generic_rela *)malloc(size2);
    if (!grel) {
        P("ERROR: Could not malloc whole generic reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return RO_ERR_MALLOC;
    }
    res = generic_rel_from_rela32(gsh,relp,grel);
    free(relp);
    if (res) {
        *count_out = count;
        *grel_out = grel;
        return RO_OK;
    }
    /* Some sort of error */
    count_out = 0;
    free (grel);
    return RO_ERR;
}

int
elf_load_rel_32(LONGESTUTYPE secnum,
    struct generic_shdr * gsh,struct generic_rela ** grel_out,
    LONGESTUTYPE *count_out)
{
    LONGESTUTYPE count = 0;
    LONGESTUTYPE size = 0;
    LONGESTUTYPE size2 = 0;
    LONGESTUTYPE offset = 0;
    int res = 0;
    Elf32_Rel* relp = 0;
    LONGESTUTYPE reclen = sizeof(Elf32_Rel);
    struct generic_rela *grel = 0;

    offset = gsh->gh_offset;
    size = gsh->gh_size;
    if(size == 0) {
        P("Warning: Empty relocation section: " LONGESTUFMT
            ", offset "
            LONGESTXFMT ", size " LONGESTXFMT "\n",
            secnum, offset, size);
        return RO_ERR;
    }
    if ((offset > filedata.f_filesize)||
        (size > filedata.f_filesize) ||
        ((size +offset) > filedata.f_filesize)) {
            P("ERROR: Something badly wrong with relocation section %s "
                " filesize " LONGESTUFMT
                " offset " LONGESTUFMT
                " size " LONGESTUFMT
                "\n",
                sanitized(gsh->gh_namestring,buffer1,BUFFERSIZE),
                filedata.f_filesize,offset,size);
            return RO_ERR;
    }

    count = size/reclen;
    size2 = count * reclen;
    if(size != size2) {
        P("Bogus size of relocations. Section " LONGESTUFMT
            ": " LONGESTUFMT
            " not divisible by "
            LONGESTUFMT "\n",
            secnum, size,reclen);
        return RO_ERR;
    }
    relp = (Elf32_Rel *)malloc(size);
    if(!relp) {
        P("ERROR: Could not malloc whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return RO_ERR_MALLOC;
    }
    res = RR(relp,offset,size);
    if(res != RO_OK) {
        free(relp);
        P("could not read whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return res;
    }
    grel = (struct generic_rela *)malloc(size2);
    if (!grel) {
        P("ERROR: Could not malloc whole generic reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return RO_ERR_MALLOC;
    }
    res = generic_rel_from_rel32(gsh,relp,grel);
    free(relp);
    if (res) {
        *count_out = count;
        *grel_out = grel;
        return RO_OK;
    }
    /* Some sort of error */
    count_out = 0;
    free (grel);
    return RO_ERR;
}


int
elf_load_rel_64(LONGESTUTYPE secnum,
    struct generic_shdr * gsh,struct generic_rela ** grel_out,
    LONGESTUTYPE *count_out)
{
    LONGESTUTYPE count = 0;
    LONGESTUTYPE size = 0;
    LONGESTUTYPE size2 = 0;
    LONGESTUTYPE offset = 0;
    int res = 0;
    Elf64_Rel* relp = 0;
    LONGESTUTYPE reclen = sizeof(Elf64_Rel);
    struct generic_rela *grel = 0;

    offset = gsh->gh_offset;
    size = gsh->gh_size;
    if(size == 0) {
        P("Warning: Empty relocation section: " LONGESTUFMT
            ", offset "
            LONGESTXFMT ", size " LONGESTXFMT "\n",
            secnum, offset, size);
        return RO_ERR;
    }
    if ((offset > filedata.f_filesize)||
        (size > filedata.f_filesize) ||
        ((size +offset) > filedata.f_filesize)) {
            P("ERROR: Something badly wrong with relocation section %s "
                " filesize " LONGESTUFMT
                " offset " LONGESTUFMT
                " size " LONGESTUFMT
                "\n",
                sanitized(gsh->gh_namestring,buffer1,BUFFERSIZE),
                filedata.f_filesize,offset,size);
            return RO_ERR;
    }

    count = size/reclen;
    size2 = count * reclen;
    if(size != size2) {
        P("Bogus size of relocations. Section " LONGESTUFMT
            ": " LONGESTUFMT
            " not divisible by "
            LONGESTUFMT "\n",
            secnum, size,reclen);
        return RO_ERR;
    }
    relp = (Elf64_Rel *)malloc(size);
    if(!relp) {
        P("ERROR: Could not malloc whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return RO_ERR_MALLOC;
    }
    res = RR(relp,offset,size);
    if(res != RO_OK) {
        free(relp);
        P("could not read whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return res;
    }
    grel = (struct generic_rela *)malloc(size2);
    if (!grel) {
        P("ERROR: Could not malloc whole generic reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return RO_ERR_MALLOC;
    }
    res = generic_rel_from_rel64(gsh,relp,grel);
    free(relp);
    if (res) {
        *count_out = count;
        *grel_out = grel;
        return RO_OK;
    }
    /* Some sort of error */
    count_out = 0;
    free (grel);
    return RO_ERR;
}


int
elf_load_rela_64(LONGESTUTYPE secnum,
    struct generic_shdr * gsh,struct generic_rela ** grel_out,
    LONGESTUTYPE *count_out)
{
    LONGESTUTYPE count = 0;
    LONGESTUTYPE size = 0;
    LONGESTUTYPE size2 = 0;
    LONGESTUTYPE offset = 0;
    int res = 0;
    Elf64_Rela *relp = 0;
    LONGESTUTYPE reclen = sizeof(Elf64_Rela);
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
    if ((offset > filedata.f_filesize)||
        (size > filedata.f_filesize) ||
        ((size +offset) > filedata.f_filesize)) {
            P("ERROR: Something badly wrong with relocation section %s "
                " filesize " LONGESTUFMT
                " offset " LONGESTUFMT
                " size " LONGESTUFMT
                "\n",
                sanitized(gsh->gh_namestring,buffer1,BUFFERSIZE),
                filedata.f_filesize,offset,size);
            return RO_ERR;
    }

    count = (long)(size/reclen);
    size2 = count * reclen;
    if(size != size2) {
        P("ERROR: Bogus size of relocations. Section " LONGESTUFMT
            ": " LONGESTUFMT
            " not divisible by "
            LONGESTUFMT "\n",
            secnum, size,reclen);
        return RO_ERR;
    }
    relp = (Elf64_Rela *)malloc(size);
    if(!relp) {
        P("ERROR: Could not malloc whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return RO_ERR_MALLOC;
    }
    res = RR(relp,offset,size);
    if(res != RO_OK) {
        free(relp);
        P("ERROR: Could not read whole reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return res;
    }
    grel = (struct generic_rela *)malloc(size2);
    if (!grel) {
        P("ERROR: Could not malloc whole generic reloc section "
            LONGESTUFMT " of %s "
            "at offset " LONGESTUFMT " size " LONGESTUFMT "\n",
            secnum,
            sanitized(filename,buffer1,BUFFERSIZE),
            offset,size);
        return RO_ERR_MALLOC;
    }
    res = generic_rel_from_rela64(gsh,relp,grel);
    free(relp);
    if (res) {
        *count_out = count;
        *grel_out = grel;
        return RO_OK;
    }
    /* Some sort of error */
    count_out = 0;
    free (grel);
    return RO_ERR;
}

void 
elf_load_sect_namestring(void)
{
    struct generic_shdr *gshdr = 0;
    LONGESTUTYPE generic_count = 0;
    LONGESTUTYPE i = 0;

    gshdr = filedata.f_shdr;
    generic_count = filedata.f_loc_shdr.g_count;
    for(i = 0; i < generic_count; i++, ++gshdr) {
        const char *namestr = "Invalid sh_name value";

        if (filedata.f_elf_shstrings_length <= gshdr->gh_name) {
            P("ERROR: Section " LONGESTUFMT
                ": section name offset "
                LONGESTUFMT " in section strings is too large.",
                i,gshdr->gh_name);
        } else {
            namestr = gshdr->gh_name + filedata.f_elf_shstrings_data;
        }
        gshdr->gh_namestring = namestr;
    }
}


int
elf_load_elf_header32(void)
{
    int res = 0;
    Elf32_Ehdr ehdr32;
    struct generic_ehdr *ehdr = 0;

    res = SEEKTO(0);
    if(res != RO_OK) {
        P("ERROR: could not seek to zero  for ELF file header of %s\n",
            sanitized(filename,buffer1,BUFFERSIZE));
        return res;
    }
    res = RR(&ehdr32,0,sizeof(ehdr32));
    if(res != RO_OK) {
        P("ERROR: could not read whole ELF file header of %s\n",
            sanitized(filename,buffer1,BUFFERSIZE));
        return res;
    }
    ehdr = (struct generic_ehdr *)calloc(1,sizeof(struct generic_ehdr));
    res  = generic_ehdr_from_32(ehdr,&ehdr32);
    if (res == RO_OK) {
        insert_in_use_entry("Elf32_Ehdr",0,sizeof(Elf32_Ehdr),ALIGN4);
    }
    return res;
}
int
elf_load_elf_header64(void)
{
    int res = 0;
    Elf64_Ehdr ehdr64;
    struct generic_ehdr *ehdr = 0;

    res = SEEKTO(0);
    if(res != RO_OK) {
        P("ERROR: could not seek to zero  for ELF file header of %s\n",
            sanitized(filename,buffer1,BUFFERSIZE));
        return res;
    }
    res = RR(&ehdr64,0,sizeof(ehdr64));
    if(res != RO_OK) {
        P("ERROR: could not read whole ELF file header of %s\n",
            sanitized(filename,buffer1,BUFFERSIZE));
        return res;
    }
    ehdr = (struct generic_ehdr *)calloc(1,sizeof(struct generic_ehdr));
    res  = generic_ehdr_from_64(ehdr,&ehdr64);
    if (res == RO_OK) {
        insert_in_use_entry("Elf64_Ehdr",0,sizeof(Elf64_Ehdr),ALIGN8);
    }
    return res;
}

