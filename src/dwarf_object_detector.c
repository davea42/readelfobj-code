/* Copyright (c) 2018-2019, David Anderson
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
#include <sys/types.h> /* open */
#include <sys/stat.h> /* open */
#include <fcntl.h> /* O_RDONLY */
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_STRING_H
#include <string.h> /* memcpy, strcpy */
#endif /* HAVE_STRING_H */
#include "dwarf_types.h"
#include "dwarf_machoread.h" /* for Dwarf_Unsigned */
#include "dwarf_object_detector.h"

/* This is the main() program for the object_detector executable. */

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif /* TRUE */

#ifndef O_RDONLY
#define O_RDONLY 0
#endif


/*  TYP, SIZEOFT32 and ASNAR
    mean we can use correctly-sized arrays of char for the
    struct members instead of determing a proper integer
    that size.

    We are dealing with carefully constructed structs
    that do not have any alignment-forced (hidden)
    unused bytes so reading lengths from the real structs
    works for each variable.  */
#ifndef TYP
#define TYP(n,l) char n[l]
#endif /* TYPE */
#define SIZEOFT32 4

#define DW_DLV_NO_ENTRY -1
#define DW_DLV_OK        0
#define DW_DLV_ERROR     1

/* Must match dwarf_reading.h list */
#define RO_ERR_SEEK             2
#define RO_ERR_READ             3
#define RO_ERR_MALLOC           4
#define RO_ERR_OTHER            5
#define RO_ERR_BADOFFSETSIZE    6
#define RO_ERR_LOADSEGOFFSETBAD 7
#define RO_ERR_FILEOFFSETBAD    8
#define RO_ERR_BADTYPESIZE      9
#define RO_ERR_TOOSMALL        10
#define RO_ERR_ELF_VERSION     11
#define RO_ERR_ELF_CLASS       12
#define RO_ERR_ELF_ENDIAN      13
#define RO_ERR_OPEN_FAIL       14
#define RO_ERR_PATH_SIZE       15
#define RO_ERR_INTEGERTOOSMALL 16
#define RO_ERR_SYMBOLSECTIONSIZE  17
#define RO_ERR_RELSECTIONSIZE     18
#define RO_ERR_STRINGOFFSETBIG    19
#define RO_ERR_DYNAMICSECTIONSIZE 20
#define RO_ERR_UNEXPECTEDZERO     21
#define RO_ERR_PHDRCOUNTMISMATCH  22
#define RO_ERR_SHDRCOUNTMISMATCH  23
#define RO_ERR_RELCOUNTMISMATCH   24
#define RO_ERR_NULL_ELF_POINTER   25
#define RO_ERR_NOT_A_KNOWN_TYPE   26
#define RO_ERR_SIZE_SMALL         27
#define RO_ERR_FILE_WRONG_TYPE    28
#define RO_ERR_ELF_STRING_SECT    29
#define RO_ERR_GROUP_ERROR        30
#define RO_SEEK_OFF_END           31
#define RO_READ_OFF_END           32
#define RO_SEEK_ERROR             33
#define RO_READ_ERROR             34
#define RO_ERR_ELF_STRING_LINK_ERROR    35
#define RO_ERR_SECTION_SIZE       36
#define RO_ERR_INVALID_STRING     37
#define DW_DLE_STRING_NOT_TERMINATED      38
#define DW_DLE_FORM_STRING_BAD_STRING     39
#define DW_DLE_CORRUPT_GNU_DEBUGLINK      40
#define DW_DLE_CORRUPT_NOTE_GNU_DEBUGID   41
#define DW_DLE_CORRUPT_GNU_DEBUGID_STRING 42
#define DW_DLE_CORRUPT_GNU_DEBUGID_SIZE   43
#define DW_DLE_ALLOC_FAIL                 44
#define DW_DLE_ERROR_NO_DOS_HEADER        45
#define DW_DLE_DW_DLE_ERROR_NO_NT_SIGNATURE 46

#ifndef EI_NIDENT
#define EI_NIDENT 16
#define EI_CLASS  4
#define EI_DATA   5
#define EI_VERSION 6
#define ELFCLASS32 1
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2
#endif /* EI_NIDENT */

#define DSYM_SUFFIX ".dSYM/Contents/Resources/DWARF/"
#define PATHSIZE 2000

#ifndef FAT_MAGIC
#define FAT_MAGIC   0xcafebabe
#define FAT_CIGAM   0xbebafeca  /* NXSwapLong(FAT_MAGIC) */
#define FAT_MAGIC_64    0xcafebabf
#define FAT_CIGAM_64    0xbfbafeca  /* NXSwapLong(FAT_MAGIC_64) */
#endif /* FAT_MAGIC */

#ifndef  MH_MAGIC
/* mach-o 32bit */
#define MH_MAGIC        0xfeedface
#define MH_CIGAM        0xcefaedfe
#endif /*  MH_MAGIC */
#ifndef  MH_MAGIC_64
/* mach-o 64bit */
#define MH_MAGIC_64 0xfeedfacf
#define MH_CIGAM_64 0xcffaedfe
#endif /*  MH_MAGIC_64 */

static unsigned long
magic_copy(unsigned char *d, unsigned len)
{
    unsigned i = 0;
    unsigned long v = 0;

    v = d[0];
    for (i = 1 ; i < len; ++i) {
        v <<= 8;
        v |=  d[i];
    }
    return v;
}

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

#define EI_NIDENT 16
/* An incomplete elf header, good for 32 and 64bit elf */
struct elf_header {
    unsigned char  e_ident[EI_NIDENT];
    TYP(e_type,2);
    TYP(e_machine,2);
    TYP(e_version,4);
};

/*  Windows. Certain PE objects.
The following references may be of interest.
https://msdn.microsoft.com/library/windows/desktop/
ms680547(v=vs.85).aspx
#PE format overview and various machine magic numbers

https://msdn.microsoft.com/en-us/library/ms809762.aspx
# describes some details of PE headers, basically an overview

https://msdn.microsoft.com/en-us/library/windows/
desktop/aa383751(v=vs.85).aspx #defines sizes of various types

https://msdn.microsoft.com/fr-fr/library/windows/
desktop/ms680313(v=vs.85).aspx
#defines IMAGE_FILE_HEADER and Machine fields (32/64)

https://msdn.microsoft.com/fr-fr/library/windows/
desktop/ms680305(v=vs.85).aspx #defines IMAGE_DATA_DIRECTORY

https://msdn.microsoft.com/en-us/library/windows/
desktop/ms680339(v=vs.85).aspx
#Defines IMAGE_OPTIONAL_HEADER and some magic numbers

https://msdn.microsoft.com/fr-fr/library/windows/
desktop/ms680336(v=vs.85).aspx # defines _IMAGE_NT_HEADERS 32 64

https://msdn.microsoft.com/en-us/library/windows/
desktop/ms680341(v=vs.85).aspx # defines _IMAGE_SECTION_HEADER
*/

/* ===== START pe structures */

struct dos_header {
    TYP(dh_mz,2);
    TYP(dh_dos_data,58);
    TYP(dh_image_offset,4);
};

#define IMAGE_DOS_SIGNATURE      0x5A4D
#define IMAGE_DOS_REVSIGNATURE   0x4D5A
#define IMAGE_NT_SIGNATURE       0x00004550
#define IMAGE_FILE_MACHINE_I386  0x14c
#define IMAGE_FILE_MACHINE_IA64  0x200
#define IMAGE_FILE_MACHINE_AMD64 0x8664

struct pe_image_file_header {
    TYP(im_machine,2);
    TYP(im_sectioncount,2);
    TYP(im_ignoring,(3*4));
    TYP(im_opt_header_size,2);
    TYP(im_ignoringb,2);
};

/* ===== END pe structures */

static void *
memcpy_swap_bytes(void *s1, const void *s2, size_t len)
{
    void *orig_s1 = s1;
    unsigned char *targ = (unsigned char *) s1;
    const unsigned char *src = (const unsigned char *) s2;

    if (len == 4) {
        targ[3] = src[0];
        targ[2] = src[1];
        targ[1] = src[2];
        targ[0] = src[3];
    } else if (len == 8) {
        targ[7] = src[0];
        targ[6] = src[1];
        targ[5] = src[2];
        targ[4] = src[3];
        targ[3] = src[4];
        targ[2] = src[5];
        targ[1] = src[6];
        targ[0] = src[7];
    } else if (len == 2) {
        targ[1] = src[0];
        targ[0] = src[1];
    }
/* should NOT get below here: is not the intended use */
    else if (len == 1) {
        targ[0] = src[0];
    } else {
        memcpy(s1, s2, len);
    }
    return orig_s1;
}

static int
object_read_random(int fd,char *buf,off_t loc,
    size_t size,Dwarf_Unsigned filesize,int *errc)
{
    off_t scode = 0;
    ssize_t rcode = 0;
    off_t endpoint = 0;

    if ((Dwarf_Unsigned)loc >= filesize) {
        /*  Seek can seek off the end. Lets not allow that.
            The object is corrupt. */
        *errc = RO_SEEK_OFF_END;
        return DW_DLV_ERROR;
    }
    endpoint = loc+size;
    if ((Dwarf_Unsigned)endpoint > filesize) {
        /*  Let us -not- try to read past end of object.
            The object is corrupt. */
        *errc = RO_READ_OFF_END;
        return DW_DLV_ERROR;
    }
    scode = lseek(fd,loc,SEEK_SET);
    if (scode == (off_t)-1) {
        *errc = RO_ERR_SEEK;
        return DW_DLV_ERROR;
    }
    rcode = read(fd,buf,size);
    if (rcode == -1 ||
        (size_t)rcode != size) {
        *errc = RO_ERR_READ;
        return DW_DLV_ERROR;
    }
    return DW_DLV_OK;
}

/*  For following MacOS file naming convention */
static const char *
getseparator (const char *f)
{
    const char *p = 0;
    const char *q = 0;
    char c = 0;;

    p = NULL;
    q = f;
    do  {
        c = *q++;
        if (c == '\\' || c == '/' || c == ':') {
            p = q;
        }
    } while (c);
    return p;
}

static const char *
getbasename (const char *f)
{
    const char *pseparator = getseparator (f);
    if (!pseparator) {
        return f;
    }
    return pseparator;
}

/*  Not a standard function, though part of GNU libc
    since 2008 (I have never examined the GNU version).  */
static char *
dw_stpcpy(char *dest,const char *src)
{
    const char *cp = src;
    char *dp = dest;

    for ( ; *cp; ++cp,++dp) {
        *dp = *cp;
    }
    *dp = 0;
    return dp;
}

/* This started like Elf, so check initial fields. */
static int
fill_in_elf_fields(struct elf_header *h,
    unsigned *endian,
    /*  Size of the object file offsets, not DWARF offset
        size. */
    unsigned *objoffsetsize,
    int *errcode)
{
    unsigned locendian = 0;
    unsigned locoffsetsize = 0;

    switch(h->e_ident[EI_CLASS]) {
    case ELFCLASS32:
        locoffsetsize = 32;
        break;
    case ELFCLASS64:
        locoffsetsize = 64;
        break;
    default:
        *errcode = RO_ERR_ELF_CLASS;
        return DW_DLV_ERROR;
    }
    switch(h->e_ident[EI_DATA]) {
    case ELFDATA2LSB:
        locendian = DW_ENDIAN_LITTLE;
        break;
    case ELFDATA2MSB:
        locendian = DW_ENDIAN_BIG;
        break;
    default:
        *errcode = RO_ERR_ELF_ENDIAN;
        return DW_DLV_ERROR;
    }
    if (h->e_ident[EI_VERSION] != 1 /* EV_CURRENT */) {
        *errcode = RO_ERR_ELF_VERSION;
        return DW_DLV_ERROR;
    }

    *endian = locendian;
    *objoffsetsize = locoffsetsize;
    return DW_DLV_OK;
}

static char archive_magic[8] = {
'!','<','a','r','c','h','>',0x0a
};
static int
is_archive_magic(struct elf_header *h)
{
    int i = 0;
    /* h is much longer than the 8 we check here. */
    int len = sizeof(archive_magic);
    const char *cp = (const char *)h;

    for ( ; i < len; ++i) {
        if (cp[i] != archive_magic[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

/*  A bit unusual in that it always sets *is_pe_flag
    Return of DW_DLV_OK  it is a PE file we recognize. */
static int
is_pe_object(int fd,
    Dwarf_Unsigned filesize,
    unsigned *endian,
    unsigned *offsetsize,
    int *errcode)
{
    unsigned dos_sig;
    unsigned locendian = 0;
    void *(*word_swap) (void *, const void *, size_t);
    unsigned long nt_address = 0;
    struct dos_header dhinmem;
    char nt_sig_array[4];
    unsigned long nt_sig = 0;
    struct pe_image_file_header ifh;
    int res = 0;

    if (filesize < (sizeof (struct dos_header) +
        SIZEOFT32 +
        sizeof(struct pe_image_file_header))) {
        *errcode = RO_ERR_TOOSMALL;
        return DW_DLV_ERROR;
    }
    res = object_read_random(fd,(char *)&dhinmem,
        0,sizeof(dhinmem),filesize,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    /*  No swap here, want it as in the file, we don't yet
        know what a deref will do to the value.  */
    dos_sig = magic_copy((unsigned char *)dhinmem.dh_mz,
        sizeof(dhinmem.dh_mz));
    if (dos_sig == IMAGE_DOS_SIGNATURE) {
        /*  IMAGE_DOS_SIGNATURE assumes bytes
            reversed by little-endian
            load, so we intrepet a match the other way. */
        /*  BIG ENDIAN. From looking at hex characters in object  */
#ifdef WORDS_BIGENDIAN
        word_swap = memcpy;
#else  /* LITTLE ENDIAN */
        word_swap = memcpy_swap_bytes;
#endif /* LITTLE- BIG-ENDIAN */
        locendian = DW_ENDIAN_BIG;
    } else if (dos_sig == IMAGE_DOS_REVSIGNATURE) {
        /* raw load, so  intrepet a match the other way. */
        /* LITTLE ENDIAN */
#ifdef WORDS_BIGENDIAN
        word_swap = memcpy_swap_bytes;
#else  /* LITTLE ENDIAN */
        word_swap = memcpy;
#endif /* LITTLE- BIG-ENDIAN */
        locendian = DW_ENDIAN_LITTLE;
    } else {
        /* Not dos header not a PE file we recognize */
        *errcode = DW_DLE_ERROR_NO_DOS_HEADER;
        return DW_DLV_ERROR;
    }
    ASNAR(word_swap,nt_address, dhinmem.dh_image_offset);
    if (filesize < (Dwarf_Unsigned)nt_address) {
        /* Not dos header not a PE file we recognize */
        *errcode = RO_ERR_TOOSMALL;
        return DW_DLV_ERROR;
    }
    if (filesize < (Dwarf_Unsigned)(nt_address +
        SIZEOFT32 +
        sizeof(struct pe_image_file_header))) {
        *errcode = RO_ERR_TOOSMALL;
        /* Not dos header not a PE file we recognize */
        return DW_DLV_ERROR;
    }
    res =  object_read_random(fd,(char *)&nt_sig_array[0],
        nt_address, sizeof(nt_sig_array),filesize,errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    {   unsigned long lsig = 0;
        ASNAR(word_swap,lsig,nt_sig_array);
        nt_sig = lsig;
    }
    if (nt_sig != IMAGE_NT_SIGNATURE) {
        *errcode = DW_DLE_DW_DLE_ERROR_NO_NT_SIGNATURE;
        return DW_DLV_ERROR;
    }
    res = object_read_random(fd,(char *)&ifh,
        nt_address + SIZEOFT32,
        sizeof(struct pe_image_file_header),
        filesize,
        errcode);
    if (res != DW_DLV_OK) {
        return res;
    }
    {
        unsigned long machine = 0;

        ASNAR(word_swap,machine,ifh.im_machine);
        switch(machine) {
        case IMAGE_FILE_MACHINE_I386:
            *offsetsize = 32;
            *endian = locendian;
            return DW_DLV_OK;
        case IMAGE_FILE_MACHINE_IA64:
        case IMAGE_FILE_MACHINE_AMD64:
            *offsetsize = 64;
            *endian = locendian;
            return DW_DLV_OK;
        }
    }
    /*  There are lots more machines,
        we are unsure which are of interest. */
    *errcode = RO_ERR_NOT_A_KNOWN_TYPE;
    return DW_DLV_ERROR;
}

static int
is_mach_o_universal(struct elf_header *h,
    unsigned *endian,
    unsigned *offsetsize)
{
    unsigned long magicval = 0;
    unsigned locendian = 0;
    unsigned locoffsetsize = 0;

    /*  No swapping here. Need to match size of
        the universal-object  magic field. */
    magicval = magic_copy(h->e_ident,4);
    if (magicval == FAT_MAGIC) {
        locendian = DW_ENDIAN_BIG;
        locoffsetsize = 32;
    } else if (magicval == FAT_CIGAM) {
        locendian = DW_ENDIAN_LITTLE;
        locoffsetsize = 32;
    }else if (magicval == FAT_MAGIC_64) {
        locendian = DW_ENDIAN_BIG;
        locoffsetsize = 64;
    } else if (magicval == FAT_CIGAM_64) {
        locendian = DW_ENDIAN_LITTLE;
        locoffsetsize = 64;
    } else {
        return FALSE;
    }
    *endian = locendian;
    *offsetsize = locoffsetsize;
    return TRUE;

}

static int
is_mach_o_magic(struct elf_header *h,
    unsigned *endian,
    unsigned *offsetsize)
{
    unsigned long magicval = 0;
    unsigned locendian = 0;
    unsigned locoffsetsize = 0;

    /*  No swapping here. Need to match size of
        Mach-o magic field. */
    magicval = magic_copy(h->e_ident,4);
    if (magicval == MH_MAGIC) {
        locendian = DW_ENDIAN_BIG;
        locoffsetsize = 32;
    } else if (magicval == MH_CIGAM) {
        locendian = DW_ENDIAN_LITTLE;
        locoffsetsize = 32;
    }else if (magicval == MH_MAGIC_64) {
        locendian = DW_ENDIAN_BIG;
        locoffsetsize = 64;
    } else if (magicval == MH_CIGAM_64) {
        locendian = DW_ENDIAN_LITTLE;
        locoffsetsize = 64;
    } else {
        return FALSE;
    }
    *endian = locendian;
    *offsetsize = locoffsetsize;
    return TRUE;
}

int
dwarf_object_detector_fd(int fd,
    unsigned *ftype,
    unsigned *endian,
    unsigned *offsetsize,
    Dwarf_Unsigned *filesize,
    int *errcode)
{
    Dwarf_Unsigned fileoffsetbase = 0;
 
    int res = 0;

    res = dwarf_object_detector_fd_a(fd,
        ftype,endian,offsetsize,
        fileoffsetbase,filesize,
        errcode);
    return res;
}

int
dwarf_object_detector_fd_a(int fd,
    unsigned *ftype,
    unsigned *endian,
    unsigned *offsetsize,
    Dwarf_Unsigned fileoffsetbase,
    Dwarf_Unsigned *filesize,
    int *errcode)
{
    struct elf_header h;
    size_t readlen = sizeof(h);
    int    res = 0;
    off_t  fsize = 0;
    off_t  lsval = 0;
    ssize_t readval = 0;
    Dwarf_Unsigned remaininglen  = 0;

    fsize = lseek(fd,0L,SEEK_END);
    if (fsize < 0) {
        printf("FAIL: fsize < 0 impossible\n");
        *errcode = RO_ERR_SEEK;
        return DW_DLV_ERROR;
    }
    remaininglen = fsize - fileoffsetbase;
    if ((Dwarf_Unsigned)fsize <= fileoffsetbase) {
        printf("FAIL: fsize <= offsetbase impossible\n");
        *errcode = RO_ERR_SEEK;
        return DW_DLV_ERROR;
    }
    if (remaininglen <= readlen) {
        /* Not a real object file */
        *errcode = RO_ERR_TOOSMALL;
        return DW_DLV_ERROR;
    }
    lsval  = lseek(fd,fileoffsetbase,SEEK_SET);
    if (lsval < 0) {
        *errcode = RO_ERR_SEEK;
        return DW_DLV_ERROR;
    }
    readval = read(fd,&h,readlen);
    if (readval != (ssize_t)readlen) {
        *errcode = RO_ERR_READ;
        return DW_DLV_ERROR;
    }
    if (h.e_ident[0] == 0x7f &&
        h.e_ident[1] == 'E' &&
        h.e_ident[2] == 'L' &&
        h.e_ident[3] == 'F') {
        /* is ELF */

        res = fill_in_elf_fields(&h,endian,offsetsize,errcode);
        if (res != DW_DLV_OK) {
            return res;
        }
        *ftype = DW_FTYPE_ELF;
        *filesize = (Dwarf_Unsigned)fsize;
        return DW_DLV_OK;
    }
    if (is_mach_o_universal(&h,endian,offsetsize)) {
        *ftype = DW_FTYPE_APPLEUNIVERSAL;
        *filesize = (Dwarf_Unsigned)fsize;
        return DW_DLV_OK;
    }
    if (is_mach_o_magic(&h,endian,offsetsize)) {
        *ftype = DW_FTYPE_MACH_O;
        *filesize = (Dwarf_Unsigned)fsize;
        return DW_DLV_OK;
    }
    if (is_archive_magic(&h)) {
        *ftype = DW_FTYPE_ARCHIVE;
        *filesize = (Dwarf_Unsigned)fsize;
        return DW_DLV_OK;
    }
    res = is_pe_object(fd,fsize,endian,offsetsize,errcode);
    if (res == DW_DLV_OK ) {
        *ftype = DW_FTYPE_PE;
        *filesize = (Dwarf_Unsigned)fsize;
        return DW_DLV_OK;
    }
    /* errcode already set., DW_DLV_NO_ENTRY impossible */
    return DW_DLV_ERROR;
}

int
dwarf_object_detector_path(const char  *path,
    char *outpath,size_t outpath_len,
    unsigned *ftype,
    unsigned *endian,
    unsigned *offsetsize,
    Dwarf_Unsigned *filesize,
    int *errcode)
{
    char *cp = 0;
    size_t plen = strlen(path);
    size_t dsprefixlen = sizeof(DSYM_SUFFIX);
    int fd = -1;
    int res = 0;
    int have_outpath = outpath && outpath_len;

#if !defined(S_ISREG)
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISDIR)
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

    if (have_outpath) {
        if ((2*plen + dsprefixlen +2) >= outpath_len) {
            *errcode =  RO_ERR_PATH_SIZE;
            return DW_DLV_ERROR;
        }
        cp = dw_stpcpy(outpath,path);
        cp = dw_stpcpy(cp,DSYM_SUFFIX);
        dw_stpcpy(cp,getbasename(path));
        fd = open(outpath,O_RDONLY|O_BINARY);
        if (fd < 0) {
            *outpath = 0;
            fd = open(path,O_RDONLY|O_BINARY);
            dw_stpcpy(outpath,path);
        }
    } else {
        fd = open(path,O_RDONLY|O_BINARY);
    }
    if (fd < 0) {
        if (have_outpath) {
            *outpath = 0;
        }
        return DW_DLV_NO_ENTRY;
    }
    res = dwarf_object_detector_fd(fd,
        ftype,endian,offsetsize,filesize, errcode);
    if (res != DW_DLV_OK && have_outpath) {
        *outpath = 0;
    }
    close(fd);
    return res;
}
