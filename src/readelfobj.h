/* Copyright (c) 2013-2019, David Anderson
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
#ifndef READELFOBJ_H
#define READELFOBJ_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define Dwarf_Unsigned LONGESTUTYPE
#define Dwarf_Signed   LONGESTSTYPE
#define Dwarf_Small    unsigned char

extern char *filename;
extern int printfilenames;

/* Standard Elf section types. */
#ifndef SHT_NULL
#define SHT_NULL 0
#endif
#ifndef SHT_PROGBITS
#define SHT_PROGBITS 1
#endif
#ifndef SHT_SYMTAB
#define SHT_SYMTAB 2
#endif
#ifndef SHT_STRTAB
#define SHT_STRTAB 3
#endif
#ifndef SHT_RELA
#define SHT_RELA 4
#endif

/* Symbol Types, Elf standard. */
#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_FILE    4

#ifndef DW_GROUPNUMBER_BASE
#define DW_GROUPNUMBER_BASE 1
#endif
#ifndef DW_GROUPNUMBER_DWO
#define DW_GROUPNUMBER_DWO  2
#endif

#ifndef SHF_GROUP
#define SHF_GROUP  (1 << 9)
#endif /* SHF_GROUP */

#ifndef STN_UNDEF
#define STN_UNDEF  0
#endif /* STN_UNDEF */

#ifndef SHT_HASH
#define SHT_HASH 5
#endif
#ifndef SHT_DYNAMIC
#define SHT_DYNAMIC 6
#endif
#ifndef SHT_NOTE
#define SHT_NOTE 7
#endif
#ifndef SHT_NOBITS
#define SHT_NOBITS 8
#endif
#ifndef SHT_REL
#define SHT_REL  9
#endif
#ifndef SHT_SHLIB
#define SHT_SHLIB 10
#endif
#ifndef SHT_DYNSYM
#define SHT_DYNSYM 11
#endif
#ifndef SHT_GROUP
#define SHT_GROUP  17
#endif /* SHT_GROUP */

#ifndef PT_NULL
#define PT_NULL 0
#endif
#ifndef PT_LOAD
#define PT_LOAD 1
#endif
#ifndef PT_DYNAMIC
#define PT_DYNAMIC 2
#endif
#ifndef PT_INTERP
#define PT_INTERP 3
#endif
#ifndef PT_NOTE
#define PT_NOTE 4
#endif
#ifndef PT_SHLIB
#define PT_SHLIB 5
#endif
#ifndef PT_PHDR
#define PT_PHDR 6
#endif
#ifndef PT_LOPROC
#define PT_LOPROC 0x70000000
#endif
#ifndef PT_HIPROC
#define PT_HIPROC 0x7fffffff
#endif

#ifndef PF_X
#define PF_X            (1 << 0)
#endif
#ifndef PF_W
#define PF_W            (1 << 1)
#endif
#ifndef PF_R
#define PF_R            (1 << 2)
#endif
#ifndef PF_MASKOS
#define PF_MASKOS       0x0ff00000
#endif
#ifndef PF_MASKPROC
#define PF_MASKPROC     0xf0000000
#endif


#ifndef ET_NONE
#define ET_NONE          0
#endif
#ifndef ET_REL
#define ET_REL           1
#endif
#ifndef ET_EXEC
#define ET_EXEC          2
#endif
#ifndef ET_DYN
#define ET_DYN           3
#endif
#ifndef ET_CORE
#define ET_CORE          4
#endif
#ifndef ET_NUM
#define ET_NUM           5
#endif
#ifndef ET_LOOS
#define ET_LOOS          0xfe00
#endif
#ifndef ET_HIOS
#define ET_HIOS          0xfeff
#endif
#ifndef ET_LOPROC
#define ET_LOPROC        0xff00
#endif
#ifndef ET_HIPROC
#define ET_HIPROC        0xffff
#endif

#ifndef EM_NONE
#define EM_NONE          0
#endif
#ifndef EM_M32
#define EM_M32           1
#endif
#ifndef EM_SPARC
#define EM_SPARC         2
#endif
#ifndef EM_386
#define EM_386           3
#endif
#ifndef EM_68K
#define EM_68K           4
#endif
#ifndef EM_88K
#define EM_88K           5
#endif
#ifndef EM_IAMCU
#define EM_IAMCU         6
#endif
#ifndef EM_860
#define EM_860           7
#endif
#ifndef EM_MIPS
#define EM_MIPS          8
#endif
#ifndef EM_S370
#define EM_S370          9
#endif
#ifndef EM_MIPS_RS3_LE
#define EM_MIPS_RS3_LE   10
#endif
#ifndef EM_PARISC
#define EM_PARISC        15
#endif
#ifndef EM_VPP500
#define EM_VPP500        17
#endif
#ifndef EM_SPARC32PLUS
#define EM_SPARC32PLUS   18
#endif
#ifndef EM_960
#define EM_960           19
#endif
#ifndef EM_PPC
#define EM_PPC           20
#endif
#ifndef EM_PPC64
#define EM_PPC64         21
#endif
#ifndef EM_S390
#define EM_S390          22
#endif
#ifndef EM_SPU
#define EM_SPU           23
#endif
#ifndef EM_V800
#define EM_V800          36
#endif
#ifndef EM_FR20
#define EM_FR20          37
#endif
#ifndef EM_RH32
#define EM_RH32          38
#endif
#ifndef EM_RCE
#define EM_RCE           39
#endif
#ifndef EM_ARM
#define EM_ARM           40
#endif
#ifndef EM_FAKE_ALPHA
#define EM_FAKE_ALPHA    41
#endif
#ifndef EM_SH
#define EM_SH            42
#endif
#ifndef EM_SPARCV9
#define EM_SPARCV9       43
#endif
#ifndef EM_TRICORE
#define EM_TRICORE       44
#endif
#ifndef EM_ARC
#define EM_ARC           45
#endif
#ifndef EM_H8_300
#define EM_H8_300        46
#endif
#ifndef EM_H8_300H
#define EM_H8_300H       47
#endif
#ifndef EM_H8S
#define EM_H8S           48
#endif
#ifndef EM_H8_500
#define EM_H8_500        49
#endif
#ifndef EM_IA_64
#define EM_IA_64         50
#endif
#ifndef EM_MIPS_X
#define EM_MIPS_X        51
#endif
#ifndef EM_COLDFIRE
#define EM_COLDFIRE      52
#endif
#ifndef EM_68HC12
#define EM_68HC12        53
#endif
#ifndef EM_MMA
#define EM_MMA           54
#endif
#ifndef EM_PCP
#define EM_PCP           55
#endif
#ifndef EM_NCPU
#define EM_NCPU          56
#endif
#ifndef EM_NDR1
#define EM_NDR1          57
#endif
#ifndef EM_STARCORE
#define EM_STARCORE      58
#endif
#ifndef EM_ME16
#define EM_ME16          59
#endif
#ifndef EM_ST100
#define EM_ST100         60
#endif
#ifndef EM_TINYJ
#define EM_TINYJ         61
#endif
#ifndef EM_X86_64
#define EM_X86_64        62
#endif
#ifndef EM_PDSP
#define EM_PDSP          63
#endif
#ifndef EM_PDP10
#define EM_PDP10         64
#endif
#ifndef EM_PDP11
#define EM_PDP11         65
#endif
#ifndef EM_FX66
#define EM_FX66          66
#endif
#ifndef EM_ST9PLUS
#define EM_ST9PLUS       67
#endif
#ifndef EM_ST7
#define EM_ST7           68
#endif
#ifndef EM_68HC16
#define EM_68HC16        69
#endif
#ifndef EM_68HC11
#define EM_68HC11        70
#endif
#ifndef EM_68HC08
#define EM_68HC08        71
#endif
#ifndef EM_68HC05
#define EM_68HC05        72
#endif
#ifndef EM_SVX
#define EM_SVX           73
#endif
#ifndef EM_ST19
#define EM_ST19          74
#endif
#ifndef EM_VAX
#define EM_VAX           75
#endif
#ifndef EM_CRIS
#define EM_CRIS          76
#endif
#ifndef EM_JAVELIN
#define EM_JAVELIN       77
#endif
#ifndef EM_FIREPATH
#define EM_FIREPATH      78
#endif
#ifndef EM_ZSP
#define EM_ZSP           79
#endif
#ifndef EM_MMIX
#define EM_MMIX          80
#endif
#ifndef EM_HUANY
#define EM_HUANY         81
#endif
#ifndef EM_PRISM
#define EM_PRISM         82
#endif
#ifndef EM_AVR
#define EM_AVR           83
#endif
#ifndef EM_FR30
#define EM_FR30          84
#endif
#ifndef EM_D10V
#define EM_D10V          85
#endif
#ifndef EM_D30V
#define EM_D30V          86
#endif
#ifndef EM_V850
#define EM_V850          87
#endif
#ifndef EM_M32R
#define EM_M32R          88
#endif
#ifndef EM_MN10300
#define EM_MN10300       89
#endif
#ifndef EM_MN10200
#define EM_MN10200       90
#endif
#ifndef EM_PJ
#define EM_PJ            91
#endif
#ifndef EM_OPENRISC
#define EM_OPENRISC      92
#endif
#ifndef EM_ARC_COMPACT
#define EM_ARC_COMPACT   93
#endif
#ifndef EM_XTENSA
#define EM_XTENSA        94
#endif
#ifndef EM_VIDEOCORE
#define EM_VIDEOCORE     95
#endif
#ifndef EM_TMM_GPP
#define EM_TMM_GPP       96
#endif
#ifndef EM_NS32K
#define EM_NS32K         97
#endif
#ifndef EM_TPC
#define EM_TPC           98
#endif
#ifndef EM_SNP1K
#define EM_SNP1K         99
#endif
#ifndef EM_ST200
#define EM_ST200         100
#endif
#ifndef EM_IP2K
#define EM_IP2K          101
#endif
#ifndef EM_MAX
#define EM_MAX           102
#endif
#ifndef EM_CR
#define EM_CR            103
#endif
#ifndef EM_F2MC16
#define EM_F2MC16        104
#endif
#ifndef EM_MSP430
#define EM_MSP430        105
#endif
#ifndef EM_BLACKFIN
#define EM_BLACKFIN      106
#endif
#ifndef EM_SE_C33
#define EM_SE_C33        107
#endif
#ifndef EM_SEP
#define EM_SEP           108
#endif
#ifndef EM_ARCA
#define EM_ARCA          109
#endif
#ifndef EM_UNICORE
#define EM_UNICORE       110
#endif
#ifndef EM_EXCESS
#define EM_EXCESS        111
#endif
#ifndef EM_DXP
#define EM_DXP           112
#endif
#ifndef EM_ALTERA_NIOS2
#define EM_ALTERA_NIOS2  113
#endif
#ifndef EM_CRX
#define EM_CRX           114
#endif
#ifndef EM_XGATE
#define EM_XGATE         115
#endif
#ifndef EM_C166
#define EM_C166          116
#endif
#ifndef EM_M16C
#define EM_M16C          117
#endif
#ifndef EM_DSPIC30F
#define EM_DSPIC30F      118
#endif
#ifndef EM_CE
#define EM_CE            119
#endif
#ifndef EM_M32C
#define EM_M32C          120
#endif
#ifndef EM_TSK3000
#define EM_TSK3000       131
#endif
#ifndef EM_RS08
#define EM_RS08          132
#endif
#ifndef EM_SHARC
#define EM_SHARC         133
#endif
#ifndef EM_ECOG2
#define EM_ECOG2         134
#endif
#ifndef EM_SCORE7
#define EM_SCORE7        135
#endif
#ifndef EM_DSP24
#define EM_DSP24         136
#endif
#ifndef EM_VIDEOCORE3
#define EM_VIDEOCORE3    137
#endif
#ifndef EM_LATTICEMICO32
#define EM_LATTICEMICO32 138
#endif
#ifndef EM_SE_C17
#define EM_SE_C17        139
#endif
#ifndef EM_TI_C6000
#define EM_TI_C6000      140
#endif
#ifndef EM_TI_C2000
#define EM_TI_C2000      141
#endif
#ifndef EM_TI_C5500
#define EM_TI_C5500      142
#endif
#ifndef EM_TI_ARP32
#define EM_TI_ARP32      143
#endif
#ifndef EM_TI_PRU
#define EM_TI_PRU        144
#endif
#ifndef EM_MMDSP_PLUS
#define EM_MMDSP_PLUS    160
#endif
#ifndef EM_CYPRESS_M8C
#define EM_CYPRESS_M8C   161
#endif
#ifndef EM_R32C
#define EM_R32C          162
#endif
#ifndef EM_TRIMEDIA
#define EM_TRIMEDIA      163
#endif
#ifndef EM_QDSP6
#define EM_QDSP6         164
#endif
#ifndef EM_8051
#define EM_8051          165
#endif
#ifndef EM_STXP7X
#define EM_STXP7X        166
#endif
#ifndef EM_NDS32
#define EM_NDS32         167
#endif
#ifndef EM_ECOG1X
#define EM_ECOG1X        168
#endif
#ifndef EM_MAXQ30
#define EM_MAXQ30        169
#endif
#ifndef EM_XIMO16
#define EM_XIMO16        170
#endif
#ifndef EM_MANIK
#define EM_MANIK         171
#endif
#ifndef EM_CRAYNV2
#define EM_CRAYNV2       172
#endif
#ifndef EM_RX
#define EM_RX            173
#endif
#ifndef EM_METAG
#define EM_METAG         174
#endif
#ifndef EM_MCST_ELBRUS
#define EM_MCST_ELBRUS   175
#endif
#ifndef EM_ECOG16
#define EM_ECOG16        176
#endif
#ifndef EM_CR16
#define EM_CR16          177
#endif
#ifndef EM_ETPU
#define EM_ETPU          178
#endif
#ifndef EM_SLE9X
#define EM_SLE9X         179
#endif
#ifndef EM_L10M
#define EM_L10M          180
#endif
#ifndef EM_K10M
#define EM_K10M          181
#endif
#ifndef EM_AARCH64
#define EM_AARCH64       183
#endif
#ifndef EM_AVR32
#define EM_AVR32         185
#endif
#ifndef EM_STM8
#define EM_STM8          186
#endif
#ifndef EM_TILE64
#define EM_TILE64        187
#endif
#ifndef EM_TILEPRO
#define EM_TILEPRO       188
#endif
#ifndef EM_MICROBLAZE
#define EM_MICROBLAZE    189
#endif
#ifndef EM_CUDA
#define EM_CUDA          190
#endif
#ifndef EM_TILEGX
#define EM_TILEGX        191
#endif
#ifndef EM_CLOUDSHIELD
#define EM_CLOUDSHIELD   192
#endif
#ifndef EM_COREA_1ST
#define EM_COREA_1ST     193
#endif
#ifndef EM_COREA_2ND
#define EM_COREA_2ND     194
#endif
#ifndef EM_ARC_COMPACT2
#define EM_ARC_COMPACT2  195
#endif
#ifndef EM_OPEN8
#define EM_OPEN8         196
#endif
#ifndef EM_RL78
#define EM_RL78          197
#endif
#ifndef EM_VIDEOCORE5
#define EM_VIDEOCORE5    198
#endif
#ifndef EM_78KOR
#define EM_78KOR         199
#endif
#ifndef EM_56800EX
#define EM_56800EX       200
#endif
#ifndef EM_BA1
#define EM_BA1           201
#endif
#ifndef EM_BA2
#define EM_BA2           202
#endif
#ifndef EM_XCORE
#define EM_XCORE         203
#endif
#ifndef EM_MCHP_PIC
#define EM_MCHP_PIC      204
#endif
#ifndef EM_KM32
#define EM_KM32          210
#endif
#ifndef EM_KMX32
#define EM_KMX32         211
#endif
#ifndef EM_EMX16
#define EM_EMX16         212
#endif
#ifndef EM_EMX8
#define EM_EMX8          213
#endif
#ifndef EM_KVARC
#define EM_KVARC         214
#endif
#ifndef EM_CDP
#define EM_CDP           215
#endif
#ifndef EM_COGE
#define EM_COGE          216
#endif
#ifndef EM_COOL
#define EM_COOL          217
#endif
#ifndef EM_NORC
#define EM_NORC          218
#endif
#ifndef EM_CSR_KALIMBA
#define EM_CSR_KALIMBA   219
#endif
#ifndef EM_Z80
#define EM_Z80           220
#endif
#ifndef EM_VISIUM
#define EM_VISIUM        221
#endif
#ifndef EM_FT32
#define EM_FT32          222
#endif
#ifndef EM_MOXIE
#define EM_MOXIE         223
#endif
#ifndef EM_AMDGPU
#define EM_AMDGPU        224
#endif
#ifndef EM_RISCV
#define EM_RISCV         243
#endif
#ifndef EM_BPF
#define EM_BPF           247
#endif

/* Standard Elf dynamic tags. */
#ifndef DT_NULL
#define DT_NULL 0
#endif
#ifndef DT_NEEDED
#define DT_NEEDED 1
#endif
#ifndef DT_PLTRELSZ
#define DT_PLTRELSZ 2
#endif
#ifndef DT_PLTGOT
#define DT_PLTGOT 3
#endif
#ifndef DT_HASH
#define DT_HASH 4
#endif
#ifndef DT_STRTAB
#define DT_STRTAB 5
#endif
#ifndef DT_SYMTAB
#define DT_SYMTAB 6
#endif
#ifndef DT_RELA
#define DT_RELA 7
#endif
#ifndef DT_RELASZ
#define DT_RELASZ 8
#endif
#ifndef DT_RELAENT
#define DT_RELAENT 9
#endif
#ifndef DT_STRSZ
#define DT_STRSZ  10
#endif

#ifndef DT_SYMENT
#define DT_SYMENT 11
#endif

#ifndef DT_INIT
#define DT_INIT 12
#endif

#ifndef DT_FINI
#define DT_FINI 13
#endif

#ifndef DT_SONAME
#define DT_SONAME 14
#endif

#ifndef DT_RPATH
#define DT_RPATH 15
#endif

#ifndef DT_SYMBOLIC
#define DT_SYMBOLIC 16
#endif

#ifndef DT_REL
#define DT_REL 17
#endif
#ifndef DT_RELSZ
#define DT_RELSZ 18
#endif

#ifndef DT_RELENT
#define DT_RELENT 19
#endif

#ifndef DT_PLTREL
#define DT_PLTREL 20
#endif

#ifndef DT_DEBUG
#define DT_DEBUG 21
#endif

#ifndef DT_TEXTREL
#define DT_TEXTREL 22
#endif

#ifndef DT_JMPREL
#define DT_JMPREL 23
#endif

#ifndef SHN_UNDEF
#define SHN_UNDEF 0
#endif
#ifndef SHN_LORESERVE
#define SHN_LORESERVE 0xff00
#endif
#ifndef SHN_LOPROC
#define SHN_LOPROC 0xff00
#endif
#ifndef SHN_HIPROC
#define SHN_HIPROC 0xff1f
#endif
#ifndef SHN_ABS
#define SHN_ABS 0xfff1
#endif
#ifndef SHN_COMMON
#define SHN_COMMON 0xfff2
#endif
#ifndef SHN_HIRESERVE
#define SHN_HIRESERVE 0xffff
#endif

#ifndef EV_CURRENT
#define EV_CURRENT       1
#endif
#ifndef EV_NONE
#define EV_NONE          0
#endif

#ifndef EI_MAG0
#define EI_MAG0          0
#endif
#ifndef EI_MAG1
#define EI_MAG1          1
#endif
#ifndef EI_MAG2
#define EI_MAG2          2
#endif
#ifndef EI_MAG3
#define EI_MAG3          3
#endif
#ifndef EI_CLASS
#define EI_CLASS         4
#endif
#ifndef EI_DATA
#define EI_DATA          5
#endif
#ifndef EI_VERSION
#define EI_VERSION       6
#endif
#ifndef EI_PAD
#define EI_PAD           7
#endif
#ifndef EI_OSABI
#define EI_OSABI         7
#endif
#ifndef EI_NIDENT
#define EI_NIDENT        16
#endif
#ifndef EI_ABIVERSION
#define EI_ABIVERSION       8
#endif

#ifndef ELFMAG0
#define ELFMAG0          0x7f
#endif
#ifndef ELFMAG1
#define ELFMAG1          'E'
#endif
#ifndef ELFMAG2
#define ELFMAG2          'L'
#endif
#ifndef ELFMAG3
#define ELFMAG3          'F'
#endif
#ifndef ELFCLASSNONE
#define ELFCLASSNONE     0
#endif
#ifndef ELFCLASS32
#define ELFCLASS32       1
#endif
#ifndef ELFCLASS64
#define ELFCLASS64       2
#endif
#ifndef ELFDATANONE
#define ELFDATANONE      0
#endif
#ifndef ELFDATA2LSB
#define ELFDATA2LSB      1
#endif
#ifndef ELFDATA2MSB
#define ELFDATA2MSB      2
#endif

#ifndef ELFOSABI_NONE
#define ELFOSABI_NONE       0
#endif
#ifndef ELFOSABI_SYSV
#define ELFOSABI_SYSV       0
#endif
#ifndef ELFOSABI_HPUX
#define ELFOSABI_HPUX       1
#endif
#ifndef ELFOSABI_NETBSD
#define ELFOSABI_NETBSD     2
#endif
#ifndef ELFOSABI_GNU
#define ELFOSABI_GNU        3
#endif
#ifndef ELFOSABI_LINUX
#define ELFOSABI_LINUX      ELFOSABI_GNU
#endif
#ifndef ELFOSABI_SOLARIS
#define ELFOSABI_SOLARIS    6
#endif
#ifndef ELFOSABI_AIX
#define ELFOSABI_AIX        7
#endif
#ifndef ELFOSABI_IRIX
#define ELFOSABI_IRIX       8
#endif
#ifndef ELFOSABI_FREEBSD
#define ELFOSABI_FREEBSD    9
#endif
#ifndef ELFOSABI_TRU64
#define ELFOSABI_TRU64      10
#endif
#ifndef ELFOSABI_MODESTO
#define ELFOSABI_MODESTO    11
#endif
#ifndef ELFOSABI_OPENBSD
#define ELFOSABI_OPENBSD    12
#endif
#ifndef ELFOSABI_ARM_AEABI
#define ELFOSABI_ARM_AEABI  64
#endif
#ifndef ELFOSABI_ARM
#define ELFOSABI_ARM        97
#endif
#ifndef ELFOSABI_STANDALONE
#define ELFOSABI_STANDALONE 255
#endif


/*  Use this for rel too. */
struct generic_rela {
    int          gr_isrela; /* 0 means rel, non-zero means rela */
    Dwarf_Unsigned gr_offset;
    Dwarf_Unsigned gr_info;
    Dwarf_Unsigned gr_sym; /* From info */
    Dwarf_Unsigned gr_type; /* From info */
    Dwarf_Signed   gr_addend;
    unsigned char  gr_type2; /* MIPS64 */
    unsigned char  gr_type3; /* MIPS64 */
};

/*  The following are generic to simplify handling
    Elf32 and Elf64.  Some fields added where
    the two sizes have different extraction code. */
struct generic_ehdr {
    unsigned char ge_ident[EI_NIDENT];
    Dwarf_Unsigned ge_type;
    Dwarf_Unsigned ge_machine;
    Dwarf_Unsigned ge_version;
    Dwarf_Unsigned ge_entry;
    Dwarf_Unsigned ge_phoff;
    Dwarf_Unsigned ge_shoff;
    Dwarf_Unsigned ge_flags;
    Dwarf_Unsigned ge_ehsize;
    Dwarf_Unsigned ge_phentsize;
    Dwarf_Unsigned ge_phnum;
    Dwarf_Unsigned ge_shentsize;
    Dwarf_Unsigned ge_shnum;
    Dwarf_Unsigned ge_shstrndx;
};
struct generic_phdr {
    Dwarf_Unsigned gp_type;
    Dwarf_Unsigned gp_flags;
    Dwarf_Unsigned gp_offset;
    Dwarf_Unsigned gp_vaddr;
    Dwarf_Unsigned gp_paddr;
    Dwarf_Unsigned gp_filesz;
    Dwarf_Unsigned gp_memsz;
    Dwarf_Unsigned gp_align;
};
struct generic_shdr {
    Dwarf_Unsigned gh_secnum;
    Dwarf_Unsigned gh_name;
    const char * gh_namestring;
    Dwarf_Unsigned gh_type;
    const char * gh_typestring;
    Dwarf_Unsigned gh_flags;
    Dwarf_Unsigned gh_addr;
    Dwarf_Unsigned gh_offset;
    Dwarf_Unsigned gh_size;
    Dwarf_Unsigned gh_link;
    Dwarf_Unsigned gh_info;
    Dwarf_Unsigned gh_addralign;
    Dwarf_Unsigned gh_entsize;

    /*  Zero unless content read in. Malloc space
        of size gh_size,  in bytes. For dwarf
        and strings mainly. free() this if not null*/
    char *       gh_content;

    /*  If a .rel or .rela section this will point
        to generic relocation records if such
        have been loaded.
        free() this if not null. */
    Dwarf_Unsigned          gh_relcount;
    struct generic_rela * gh_rels;

    /*  For SHT_GROUP based  grouping, which
        group is this section in. 0 unknown,
        1 DW_GROUP_NUMBER_BASE base DWARF,
        2 DW_GROUPNUMBER_DWO  dwo sections, 3+
        are in an SHT_GROUP. GNU uses this.
        set with group number (3+) from SHT_GROUP
        and the flags should have SHF_GROUP set
        if in SHT_GROUP. Must only be in one group? */
    Dwarf_Unsigned gh_section_group_number;

    /*  Content of an SHT_GROUP section as an array
        of integers. [0] is the version, which
        can only be one(1) . */
    Dwarf_Unsigned * gh_sht_group_array;
    /*  Number of elements in the gh_sht_group_array. */
    Dwarf_Unsigned   gh_sht_group_array_count;

    /*   TRUE if .debug_info .eh_frame etc. */
    char  gh_is_dwarf;
};

struct generic_dynentry {
    LONGESTSTYPE  gd_tag;
    /*  gd_val stands in for d_ptr and d_val union,
        the union adds nothing in practice since
        we expect ptrsize <= ulongest. */
    Dwarf_Unsigned  gd_val;
    Dwarf_Unsigned  gd_dyn_file_offset;
};

struct generic_symentry {
    Dwarf_Unsigned gs_name;
    Dwarf_Unsigned gs_value;
    Dwarf_Unsigned gs_size;
    Dwarf_Unsigned gs_info;
    Dwarf_Unsigned gs_other;
    Dwarf_Unsigned gs_shndx;
    /* derived */
    Dwarf_Unsigned gs_bind;
    Dwarf_Unsigned gs_type;
};

struct location {
    const char *g_name;
    Dwarf_Unsigned g_offset;
    Dwarf_Unsigned g_count;
    Dwarf_Unsigned g_entrysize;
    Dwarf_Unsigned g_totalsize;
};

struct in_use_s {
    struct in_use_s *u_next;
    const char *u_name;
    Dwarf_Unsigned u_offset;
    Dwarf_Unsigned u_align;
    Dwarf_Unsigned u_length;
    Dwarf_Unsigned u_lastbyte;
};

struct elf_filedata_s {
    /*  f_ident[0] == 'E' means it is elf and
        elf_filedata_s is the struct involved.
        Other means error/corruption of some kind.
        f_ident[1] is a version number.
        Only version 1 is defined. */
    char         f_ident[8];
    int          f_fd;
    int          f_printf_on_error;
    int          f_machine; /* from f_ident(EI_MACHINE) */
    char *       f_path; /* non-null if known. Must be freed */

    /* If TRUE close f_fd on destruct. */
    int          f_destruct_close_fd;

    unsigned	 f_endian;
    unsigned     f_offsetsize; /* Elf offset size, not DWARF. 32 or 64 */
    Dwarf_Unsigned f_filesize;
    Dwarf_Unsigned f_max_secdata_offset;
    Dwarf_Unsigned f_max_progdata_offset;

    Dwarf_Unsigned f_wasted_dynamic_count;
    Dwarf_Unsigned f_wasted_dynamic_space;

    Dwarf_Unsigned f_wasted_content_space;
    Dwarf_Unsigned f_wasted_content_count;

    Dwarf_Unsigned f_wasted_align_space;
    Dwarf_Unsigned f_wasted_align_count;
    void *(*f_copy_word) (void *, const void *, size_t);

    struct in_use_s * f_in_use;
    struct in_use_s * f_in_use_tail;
    Dwarf_Unsigned f_in_use_count;

    struct location      f_loc_ehdr;
    struct generic_ehdr* f_ehdr;

    struct location      f_loc_shdr;
    struct generic_shdr* f_shdr;

    struct location      f_loc_phdr;
    struct generic_phdr* f_phdr;

    char *f_elf_shstrings_data; /* section name strings */
    /* length of currentsection.  Might be zero..*/
    Dwarf_Unsigned  f_elf_shstrings_length;
    /* size of malloc-d space */
    Dwarf_Unsigned  f_elf_shstrings_max;

    /* This is the .dynamic section */
    struct location      f_loc_dynamic;
    struct generic_dynentry * f_dynamic;
    Dwarf_Unsigned f_dynamic_sect_index;

    /* .dynsym, .dynstr */
    struct location      f_loc_dynsym;
    struct generic_symentry* f_dynsym;
    char  *f_dynsym_sect_strings;
    Dwarf_Unsigned f_dynsym_sect_strings_max;
    Dwarf_Unsigned f_dynsym_sect_strings_sect_index;
    Dwarf_Unsigned f_dynsym_sect_index;

    /* .symtab .strtab */
    struct location      f_loc_symtab;
    struct generic_symentry* f_symtab;
    char * f_symtab_sect_strings;
    Dwarf_Unsigned f_symtab_sect_strings_max;
    Dwarf_Unsigned f_symtab_sect_strings_sect_index;
    Dwarf_Unsigned f_symtab_sect_index;

    /* Starts at 3. 0,1,2 used specially. */
    Dwarf_Unsigned f_sg_next_group_number;
    /*  Both the following will be zero unless there
        are explicit Elf groups. */
    Dwarf_Unsigned f_sht_group_type_section_count;
    Dwarf_Unsigned f_shf_group_flag_section_count;
    Dwarf_Unsigned f_dwo_group_section_count;


};
typedef struct elf_filedata_s * elf_filedata;

int dwarf_construct_elf_access(int fd,
    const char *path,
    elf_filedata *ep,int *errcode);
int dwarf_construct_elf_access_path(const char *path,
    elf_filedata *ep,int *errcode);
int dwarf_destruct_elf_access(elf_filedata ep,int *errcode);
int dwarf_load_elf_header(elf_filedata ep,int *errcode);
int dwarf_load_elf_sectheaders(elf_filedata ep,int *errcode);
int dwarf_load_elf_progheaders(elf_filedata ep,int *errcode);

int dwarf_load_elf_dynamic(elf_filedata ep, int *errcode);
int dwarf_load_elf_symstr(elf_filedata ep, int *errcode);
int dwarf_load_elf_dynstr(elf_filedata ep, int *errcode);
int dwarf_load_elf_symtab_symbols(elf_filedata ep,int *errcode);
int dwarf_load_elf_dynsym_symbols(elf_filedata ep,int *errcode);
int dwarf_load_elf_section_is_dwarf(const char *name);

int dwarf_load_elf_rela(elf_filedata ep,
    Dwarf_Unsigned secnum, int *errcode);
int dwarf_load_elf_rel(elf_filedata ep,
    Dwarf_Unsigned secnum, int *errcode);

/*  Gets sh_strtab if is_symtab TRUE.
    Gets sh_dynstr if is_symtab FALSE.
    Returns pointer to the in-mem string
    through strptr.
*/
int dwarf_get_elf_symstr_string(elf_filedata ep,
    int is_symtab,
    Dwarf_Unsigned index,
    const char **strptr,
    int *errcode);

/*  The following for an elf checker/dumper. */
const char * dwarf_get_elf_machine_name(unsigned value);
const char * dwarf_get_elf_dynamic_table_name(
    Dwarf_Unsigned value,
    char *buffer, unsigned buflen);
const char * dwarf_get_elf_program_header_type_name(
    Dwarf_Unsigned value,
    char *buffer, unsigned buflen);
const char * dwarf_get_elf_section_header_flag_names(
    Dwarf_Unsigned value,
    char *buffer, unsigned buflen);
const char * dwarf_get_elf_section_header_st_type(
    Dwarf_Unsigned value,
    char *buffer, unsigned buflen);
void dwarf_insert_in_use_entry(elf_filedata ep,
    const char *description,Dwarf_Unsigned offset,
    Dwarf_Unsigned length,Dwarf_Unsigned align);
const char * dwarf_get_elf_symbol_sto_type(
    Dwarf_Unsigned value, char *buffer,
    unsigned buflen);
const char * dwarf_get_elf_symbol_shn_type(
    Dwarf_Unsigned value, char *buffer, unsigned buflen);
const char * dwarf_get_elf_symbol_stb_string(
    Dwarf_Unsigned val, char *buff, unsigned buflen);
const char * dwarf_get_elf_symbol_stt_type( Dwarf_Unsigned value,
    char *buffer, unsigned buflen);
const char * dwarf_get_elf_osabi_name( Dwarf_Unsigned value,
    char *buffer, unsigned buflen);
const char * dwarf_get_elf_machine_name(unsigned value);
const char * dwarf_get_elf_dynamic_table_name(
    Dwarf_Unsigned value, char *buffer, unsigned buflen);
const char * dwarf_get_elf_section_header_flag_names(
    Dwarf_Unsigned value, char *buffer, unsigned buflen);
const char * dwarf_get_elf_section_header_st_type_name(
    Dwarf_Unsigned value, char *buffer, unsigned buflen);



int cur_read_loc(FILE *fin, long* fileoffset);

#ifndef EI_NIDENT
#define EI_NIDENT 16
#endif /* EI_NIDENT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* READELFOBJ_H */
