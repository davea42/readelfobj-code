/* Created by build_access.py */
#include "dwarf_elf_reloc_ppc64.h"

/* returns string of length 0 if invalid arg */
const char *
dwarf_get_elf_relocname_ppc64(unsigned long val)
{
    switch(val) {
    case R_PPC64_ADDR30 : return "R_PPC64_ADDR30";
    case R_PPC64_ADDR64 : return "R_PPC64_ADDR64";
    case R_PPC64_ADDR16_HIGHER : return "R_PPC64_ADDR16_HIGHER";
    case R_PPC64_ADDR16_HIGHERA : return "R_PPC64_ADDR16_HIGHERA";
    case R_PPC64_ADDR16_HIGHEST : return "R_PPC64_ADDR16_HIGHEST";
    case R_PPC64_ADDR16_HIGHESTA : return "R_PPC64_ADDR16_HIGHESTA";
    case R_PPC64_UADDR64 : return "R_PPC64_UADDR64";
    case R_PPC64_REL64 : return "R_PPC64_REL64";
    case R_PPC64_PLT64 : return "R_PPC64_PLT64";
    case R_PPC64_PLTREL64 : return "R_PPC64_PLTREL64";
    case R_PPC64_TOC16 : return "R_PPC64_TOC16";
    case R_PPC64_TOC16_LO : return "R_PPC64_TOC16_LO";
    case R_PPC64_TOC16_HI : return "R_PPC64_TOC16_HI";
    case R_PPC64_TOC16_HA : return "R_PPC64_TOC16_HA";
    case R_PPC64_TOC : return "R_PPC64_TOC";
    case R_PPC64_PLTGOT16 : return "R_PPC64_PLTGOT16";
    case R_PPC64_PLTGOT16_LO : return "R_PPC64_PLTGOT16_LO";
    case R_PPC64_PLTGOT16_HI : return "R_PPC64_PLTGOT16_HI";
    case R_PPC64_PLTGOT16_HA : return "R_PPC64_PLTGOT16_HA";
    case R_PPC64_ADDR16_DS : return "R_PPC64_ADDR16_DS";
    case R_PPC64_ADDR16_LO_DS : return "R_PPC64_ADDR16_LO_DS";
    case R_PPC64_GOT16_DS : return "R_PPC64_GOT16_DS";
    case R_PPC64_GOT16_LO_DS : return "R_PPC64_GOT16_LO_DS";
    case R_PPC64_PLT16_LO_DS : return "R_PPC64_PLT16_LO_DS";
    case R_PPC64_SECTOFF_DS : return "R_PPC64_SECTOFF_DS";
    case R_PPC64_SECTOFF_LO_DS : return "R_PPC64_SECTOFF_LO_DS";
    case R_PPC64_TOC16_DS : return "R_PPC64_TOC16_DS";
    case R_PPC64_TOC16_LO_DS : return "R_PPC64_TOC16_LO_DS";
    case R_PPC64_PLTGOT16_DS : return "R_PPC64_PLTGOT16_DS";
    case R_PPC64_PLTGOT16_LO_DS : return "R_PPC64_PLTGOT16_LO_DS";
    case R_PPC64_TLS : return "R_PPC64_TLS";
    case R_PPC64_DTPMOD64 : return "R_PPC64_DTPMOD64";
    case R_PPC64_TPREL16 : return "R_PPC64_TPREL16";
    case R_PPC64_TPREL16_LO : return "R_PPC64_TPREL16_LO";
    case R_PPC64_TPREL16_HI : return "R_PPC64_TPREL16_HI";
    case R_PPC64_TPREL16_HA : return "R_PPC64_TPREL16_HA";
    case R_PPC64_TPREL64 : return "R_PPC64_TPREL64";
    case R_PPC64_DTPREL16 : return "R_PPC64_DTPREL16";
    case R_PPC64_DTPREL16_LO : return "R_PPC64_DTPREL16_LO";
    case R_PPC64_DTPREL16_HI : return "R_PPC64_DTPREL16_HI";
    case R_PPC64_DTPREL16_HA : return "R_PPC64_DTPREL16_HA";
    case R_PPC64_DTPREL64 : return "R_PPC64_DTPREL64";
    case R_PPC64_GOT_TLSGD16 : return "R_PPC64_GOT_TLSGD16";
    case R_PPC64_GOT_TLSGD16_LO : return "R_PPC64_GOT_TLSGD16_LO";
    case R_PPC64_GOT_TLSGD16_HI : return "R_PPC64_GOT_TLSGD16_HI";
    case R_PPC64_GOT_TLSGD16_HA : return "R_PPC64_GOT_TLSGD16_HA";
    case R_PPC64_GOT_TLSLD16 : return "R_PPC64_GOT_TLSLD16";
    case R_PPC64_GOT_TLSLD16_LO : return "R_PPC64_GOT_TLSLD16_LO";
    case R_PPC64_GOT_TLSLD16_HI : return "R_PPC64_GOT_TLSLD16_HI";
    case R_PPC64_GOT_TLSLD16_HA : return "R_PPC64_GOT_TLSLD16_HA";
    case R_PPC64_GOT_TPREL16_DS : return "R_PPC64_GOT_TPREL16_DS";
    case R_PPC64_GOT_TPREL16_LO_DS :
        return "R_PPC64_GOT_TPREL16_LO_DS";
    case R_PPC64_GOT_TPREL16_HI : return "R_PPC64_GOT_TPREL16_HI";
    case R_PPC64_GOT_TPREL16_HA : return "R_PPC64_GOT_TPREL16_HA";
    case R_PPC64_GOT_DTPREL16_DS : return "R_PPC64_GOT_DTPREL16_DS";
    case R_PPC64_GOT_DTPREL16_LO_DS :
        return "R_PPC64_GOT_DTPREL16_LO_DS";
    case R_PPC64_GOT_DTPREL16_HI : return "R_PPC64_GOT_DTPREL16_HI";
    case R_PPC64_GOT_DTPREL16_HA : return "R_PPC64_GOT_DTPREL16_HA";
    case R_PPC64_TPREL16_DS : return "R_PPC64_TPREL16_DS";
    case R_PPC64_TPREL16_LO_DS : return "R_PPC64_TPREL16_LO_DS";
    case R_PPC64_TPREL16_HIGHER : return "R_PPC64_TPREL16_HIGHER";
    case R_PPC64_TPREL16_HIGHERA : return "R_PPC64_TPREL16_HIGHERA";
    case R_PPC64_TPREL16_HIGHEST : return "R_PPC64_TPREL16_HIGHEST";
    case R_PPC64_TPREL16_HIGHESTA : return "R_PPC64_TPREL16_HIGHESTA";
    case R_PPC64_DTPREL16_DS : return "R_PPC64_DTPREL16_DS";
    case R_PPC64_DTPREL16_LO_DS : return "R_PPC64_DTPREL16_LO_DS";
    case R_PPC64_DTPREL16_HIGHER : return "R_PPC64_DTPREL16_HIGHER";
    case R_PPC64_DTPREL16_HIGHERA : return "R_PPC64_DTPREL16_HIGHERA";
    case R_PPC64_DTPREL16_HIGHEST : return "R_PPC64_DTPREL16_HIGHEST";
    case R_PPC64_DTPREL16_HIGHESTA :
        return "R_PPC64_DTPREL16_HIGHESTA";
    case R_PPC64_TOC32 : return "R_PPC64_TOC32";
    case R_PPC64_DTPMOD32 : return "R_PPC64_DTPMOD32";
    case R_PPC64_TPREL32 : return "R_PPC64_TPREL32";
    case R_PPC64_DTPREL32 : return "R_PPC64_DTPREL32";
    case R_PPC64_ADDR16_HIGHA : return "R_PPC64_ADDR16_HIGHA";
    case R_PPC64_TPREL16_HIGH : return "R_PPC64_TPREL16_HIGH";
    case R_PPC64_TPREL16_HIGHA : return "R_PPC64_TPREL16_HIGHA";
    case R_PPC64_DTPREL16_HIGH : return "R_PPC64_DTPREL16_HIGH";
    case R_PPC64_DTPREL16_HIGHA : return "R_PPC64_DTPREL16_HIGHA";
    case R_PPC64_JMP_IREL : return "R_PPC64_JMP_IREL";
    case R_PPC64_IRELATIVE : return "R_PPC64_IRELATIVE";
    case R_PPC64_REL16 : return "R_PPC64_REL16";
    case R_PPC64_REL16_LO : return "R_PPC64_REL16_LO";
    case R_PPC64_REL16_HI : return "R_PPC64_REL16_HI";
    case R_PPC64_REL16_HA : return "R_PPC64_REL16_HA";
    }
return "";
}
