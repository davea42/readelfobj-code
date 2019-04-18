/* Created by build_access.py */
#include "dwarf_elf_reloc_sparc.h"

/* returns string of length 0 if invalid arg */
const char *
dwarf_get_elf_relocname_sparc(unsigned long val)
{
    switch(val) {
    case R_SPARC_NONE : return "R_SPARC_NONE";
    case R_SPARC_8 : return "R_SPARC_8";
    case R_SPARC_16 : return "R_SPARC_16";
    case R_SPARC_32 : return "R_SPARC_32";
    case R_SPARC_DISP8 : return "R_SPARC_DISP8";
    case R_SPARC_DISP16 : return "R_SPARC_DISP16";
    case R_SPARC_DISP32 : return "R_SPARC_DISP32";
    case R_SPARC_WDISP30 : return "R_SPARC_WDISP30";
    case R_SPARC_WDISP22 : return "R_SPARC_WDISP22";
    case R_SPARC_HI22 : return "R_SPARC_HI22";
    case R_SPARC_22 : return "R_SPARC_22";
    case R_SPARC_13 : return "R_SPARC_13";
    case R_SPARC_LO10 : return "R_SPARC_LO10";
    case R_SPARC_GOT10 : return "R_SPARC_GOT10";
    case R_SPARC_GOT13 : return "R_SPARC_GOT13";
    case R_SPARC_GOT22 : return "R_SPARC_GOT22";
    case R_SPARC_PC10 : return "R_SPARC_PC10";
    case R_SPARC_PC22 : return "R_SPARC_PC22";
    case R_SPARC_WPLT30 : return "R_SPARC_WPLT30";
    case R_SPARC_COPY : return "R_SPARC_COPY";
    case R_SPARC_GLOB_DAT : return "R_SPARC_GLOB_DAT";
    case R_SPARC_JMP_SLOT : return "R_SPARC_JMP_SLOT";
    case R_SPARC_RELATIVE : return "R_SPARC_RELATIVE";
    case R_SPARC_UA32 : return "R_SPARC_UA32";
    case R_SPARC_PLT32 : return "R_SPARC_PLT32";
    case R_SPARC_HIPLT22 : return "R_SPARC_HIPLT22";
    case R_SPARC_LOPLT10 : return "R_SPARC_LOPLT10";
    case R_SPARC_PCPLT32 : return "R_SPARC_PCPLT32";
    case R_SPARC_PCPLT22 : return "R_SPARC_PCPLT22";
    case R_SPARC_PCPLT10 : return "R_SPARC_PCPLT10";
    case R_SPARC_10 : return "R_SPARC_10";
    case R_SPARC_11 : return "R_SPARC_11";
    case R_SPARC_64 : return "R_SPARC_64";
    case R_SPARC_OLO10 : return "R_SPARC_OLO10";
    case R_SPARC_HH22 : return "R_SPARC_HH22";
    case R_SPARC_HM10 : return "R_SPARC_HM10";
    case R_SPARC_LM22 : return "R_SPARC_LM22";
    case R_SPARC_PC_HH22 : return "R_SPARC_PC_HH22";
    case R_SPARC_PC_HM10 : return "R_SPARC_PC_HM10";
    case R_SPARC_PC_LM22 : return "R_SPARC_PC_LM22";
    case R_SPARC_WDISP16 : return "R_SPARC_WDISP16";
    case R_SPARC_WDISP19 : return "R_SPARC_WDISP19";
    case R_SPARC_GLOB_JMP : return "R_SPARC_GLOB_JMP";
    case R_SPARC_7 : return "R_SPARC_7";
    case R_SPARC_5 : return "R_SPARC_5";
    case R_SPARC_6 : return "R_SPARC_6";
    case R_SPARC_DISP64 : return "R_SPARC_DISP64";
    case R_SPARC_PLT64 : return "R_SPARC_PLT64";
    case R_SPARC_HIX22 : return "R_SPARC_HIX22";
    case R_SPARC_LOX10 : return "R_SPARC_LOX10";
    case R_SPARC_H44 : return "R_SPARC_H44";
    case R_SPARC_M44 : return "R_SPARC_M44";
    case R_SPARC_L44 : return "R_SPARC_L44";
    case R_SPARC_REGISTER : return "R_SPARC_REGISTER";
    case R_SPARC_UA64 : return "R_SPARC_UA64";
    case R_SPARC_UA16 : return "R_SPARC_UA16";
    case R_SPARC_TLS_GD_HI22 : return "R_SPARC_TLS_GD_HI22";
    case R_SPARC_TLS_GD_LO10 : return "R_SPARC_TLS_GD_LO10";
    case R_SPARC_TLS_GD_ADD : return "R_SPARC_TLS_GD_ADD";
    case R_SPARC_TLS_GD_CALL : return "R_SPARC_TLS_GD_CALL";
    case R_SPARC_TLS_LDM_HI22 : return "R_SPARC_TLS_LDM_HI22";
    case R_SPARC_TLS_LDM_LO10 : return "R_SPARC_TLS_LDM_LO10";
    case R_SPARC_TLS_LDM_ADD : return "R_SPARC_TLS_LDM_ADD";
    case R_SPARC_TLS_LDM_CALL : return "R_SPARC_TLS_LDM_CALL";
    case R_SPARC_TLS_LDO_HIX22 : return "R_SPARC_TLS_LDO_HIX22";
    case R_SPARC_TLS_LDO_LOX10 : return "R_SPARC_TLS_LDO_LOX10";
    case R_SPARC_TLS_LDO_ADD : return "R_SPARC_TLS_LDO_ADD";
    case R_SPARC_TLS_IE_HI22 : return "R_SPARC_TLS_IE_HI22";
    case R_SPARC_TLS_IE_LO10 : return "R_SPARC_TLS_IE_LO10";
    case R_SPARC_TLS_IE_LD : return "R_SPARC_TLS_IE_LD";
    case R_SPARC_TLS_IE_LDX : return "R_SPARC_TLS_IE_LDX";
    case R_SPARC_TLS_IE_ADD : return "R_SPARC_TLS_IE_ADD";
    case R_SPARC_TLS_LE_HIX22 : return "R_SPARC_TLS_LE_HIX22";
    case R_SPARC_TLS_LE_LOX10 : return "R_SPARC_TLS_LE_LOX10";
    case R_SPARC_TLS_DTPMOD32 : return "R_SPARC_TLS_DTPMOD32";
    case R_SPARC_TLS_DTPMOD64 : return "R_SPARC_TLS_DTPMOD64";
    case R_SPARC_TLS_DTPOFF32 : return "R_SPARC_TLS_DTPOFF32";
    case R_SPARC_TLS_DTPOFF64 : return "R_SPARC_TLS_DTPOFF64";
    case R_SPARC_TLS_TPOFF32 : return "R_SPARC_TLS_TPOFF32";
    case R_SPARC_TLS_TPOFF64 : return "R_SPARC_TLS_TPOFF64";
    case R_SPARC_GOTDATA_HIX22 : return "R_SPARC_GOTDATA_HIX22";
    case R_SPARC_GOTDATA_LOX10 : return "R_SPARC_GOTDATA_LOX10";
    case R_SPARC_GOTDATA_OP_HIX22 : return "R_SPARC_GOTDATA_OP_HIX22";
    case R_SPARC_GOTDATA_OP_LOX10 : return "R_SPARC_GOTDATA_OP_LOX10";
    case R_SPARC_GOTDATA_OP : return "R_SPARC_GOTDATA_OP";
    case R_SPARC_H34 : return "R_SPARC_H34";
    case R_SPARC_SIZE32 : return "R_SPARC_SIZE32";
    case R_SPARC_SIZE64 : return "R_SPARC_SIZE64";
    case R_SPARC_WDISP10 : return "R_SPARC_WDISP10";
    case R_SPARC_JMP_IREL : return "R_SPARC_JMP_IREL";
    case R_SPARC_IRELATIVE : return "R_SPARC_IRELATIVE";
    case R_SPARC_GNU_VTINHERIT : return "R_SPARC_GNU_VTINHERIT";
    case R_SPARC_GNU_VTENTRY : return "R_SPARC_GNU_VTENTRY";
    case R_SPARC_REV32 : return "R_SPARC_REV32";
    }
return "";
}
