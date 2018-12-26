/* Created by build_access.py */
#include "dwarf_elf_reloc_mips.h"

/* returns string of length 0 if invalid arg */
const char * 
dwarf_get_elf_relocname_mips(unsigned long val)
{
    switch(val) {
    case R_MIPS_NONE : return "R_MIPS_NONE";
    case R_MIPS_16 : return "R_MIPS_16";
    case R_MIPS_32 : return "R_MIPS_32";
    case R_MIPS_REL : return "R_MIPS_REL";
    case R_MIPS_26 : return "R_MIPS_26";
    case R_MIPS_HI16 : return "R_MIPS_HI16";
    case R_MIPS_LO16 : return "R_MIPS_LO16";
    case R_MIPS_GPREL : return "R_MIPS_GPREL";
    case R_MIPS_LITERAL : return "R_MIPS_LITERAL";
    case R_MIPS_GOT : return "R_MIPS_GOT";
    case R_MIPS_PC16 : return "R_MIPS_PC16";
    case R_MIPS_CALL : return "R_MIPS_CALL";
    case R_MIPS_GPREL32 : return "R_MIPS_GPREL32";
    case R_MIPS_UNUSED1 : return "R_MIPS_UNUSED1";
    case R_MIPS_UNUSED2 : return "R_MIPS_UNUSED2";
    case R_MIPS_UNUSED3 : return "R_MIPS_UNUSED3";
    case R_MIPS_SHIFT5 : return "R_MIPS_SHIFT5";
    case R_MIPS_SHIFT6 : return "R_MIPS_SHIFT6";
    case R_MIPS_64 : return "R_MIPS_64";
    case R_MIPS_GOT_DISP : return "R_MIPS_GOT_DISP";
    case R_MIPS_GOT_PAGE : return "R_MIPS_GOT_PAGE";
    case R_MIPS_GOT_OFST : return "R_MIPS_GOT_OFST";
    case R_MIPS_GOT_HI16 : return "R_MIPS_GOT_HI16";
    case R_MIPS_GOT_LO16 : return "R_MIPS_GOT_LO16";
    case R_MIPS_SUB : return "R_MIPS_SUB";
    case R_MIPS_INSERT_A : return "R_MIPS_INSERT_A";
    case R_MIPS_INSERT_B : return "R_MIPS_INSERT_B";
    case R_MIPS_DELETE : return "R_MIPS_DELETE";
    case R_MIPS_HIGHER : return "R_MIPS_HIGHER";
    case R_MIPS_HIGHEST : return "R_MIPS_HIGHEST";
    case R_MIPS_CALL_HI16 : return "R_MIPS_CALL_HI16";
    case R_MIPS_CALL_LO16 : return "R_MIPS_CALL_LO16";
    case R_MIPS_SCN_DISP : return "R_MIPS_SCN_DISP";
    case R_MIPS_REL16 : return "R_MIPS_REL16";
    case R_MIPS_ADD_IMMEDIATE : return "R_MIPS_ADD_IMMEDIATE";
    }
return "";
}
