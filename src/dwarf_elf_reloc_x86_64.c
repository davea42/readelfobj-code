/* Created by build_access.py */
#include "dwarf_elf_reloc_x86_64.h"

/* returns string of length 0 if invalid arg */
const char * 
dwarf_get_elf_relocname_x86_64(unsigned long val)
{
    switch(val) {
    case R_X86_64_NONE : return "R_X86_64_NONE";
    case R_X86_64_64 : return "R_X86_64_64";
    case R_X86_64_PC32 : return "R_X86_64_PC32";
    case R_X86_64_GOT32 : return "R_X86_64_GOT32";
    case R_X86_64_PLT32 : return "R_X86_64_PLT32";
    case R_X86_64_COPY : return "R_X86_64_COPY";
    case R_X86_64_GLOB_DAT : return "R_X86_64_GLOB_DAT";
    case R_X86_64_JUMP_SLOT : return "R_X86_64_JUMP_SLOT";
    case R_X86_64_RELATIVE : return "R_X86_64_RELATIVE";
    case R_X86_64_GOTPCREL : return "R_X86_64_GOTPCREL";
    case R_X86_64_32 : return "R_X86_64_32";
    case R_X86_64_32S : return "R_X86_64_32S";
    case R_X86_64_16 : return "R_X86_64_16";
    case R_X86_64_PC16 : return "R_X86_64_PC16";
    case R_X86_64_8 : return "R_X86_64_8";
    case R_X86_64_PC8 : return "R_X86_64_PC8";
    case R_X86_64_DTPMOD64 : return "R_X86_64_DTPMOD64";
    case R_X86_64_DTPOFF64 : return "R_X86_64_DTPOFF64";
    case R_X86_64_TPOFF64 : return "R_X86_64_TPOFF64";
    case R_X86_64_TLSGD : return "R_X86_64_TLSGD";
    case R_X86_64_TLSLD : return "R_X86_64_TLSLD";
    case R_X86_64_DTPOFF32 : return "R_X86_64_DTPOFF32";
    case R_X86_64_GOTTPOFF : return "R_X86_64_GOTTPOFF";
    case R_X86_64_TPOFF32 : return "R_X86_64_TPOFF32";
    case R_X86_64_PC64 : return "R_X86_64_PC64";
    case R_X86_64_GOTOFF64 : return "R_X86_64_GOTOFF64";
    case R_X86_64_GOTPC32 : return "R_X86_64_GOTPC32";
    case R_X86_64_GOT64 : return "R_X86_64_GOT64";
    case R_X86_64_GOTPCREL64 : return "R_X86_64_GOTPCREL64";
    case R_X86_64_GOTPC64 : return "R_X86_64_GOTPC64";
    case R_X86_64_GOTPLT64 : return "R_X86_64_GOTPLT64";
    case R_X86_64_PLTOFF64 : return "R_X86_64_PLTOFF64";
    case R_X86_64_SIZE32 : return "R_X86_64_SIZE32";
    case R_X86_64_SIZE64 : return "R_X86_64_SIZE64";
    case R_X86_64_GOTPC32_TLSDESC : return "R_X86_64_GOTPC32_TLSDESC";
    case R_X86_64_TLSDESC_CALL : return "R_X86_64_TLSDESC_CALL";
    case R_X86_64_TLSDESC : return "R_X86_64_TLSDESC";
    case R_X86_64_IRELATIVE : return "R_X86_64_IRELATIVE";
    case R_X86_64_RELATIVE64 : return "R_X86_64_RELATIVE64";
    case R_X86_64_GOTPCRELX : return "R_X86_64_GOTPCRELX";
    case R_X86_64_REX_GOTPCRELX : return "R_X86_64_REX_GOTPCRELX";
    }
return "";
}
