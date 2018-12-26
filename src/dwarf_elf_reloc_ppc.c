/* Created by build_access.py */
#include "dwarf_elf_reloc_ppc.h"

/* returns string of length 0 if invalid arg */
const char * 
dwarf_get_elf_relocname_ppc(unsigned long val)
{
    switch(val) {
    case R_PPC_NONE : return "R_PPC_NONE";
    case R_PPC_ADDR32 : return "R_PPC_ADDR32";
    case R_PPC_ADDR24 : return "R_PPC_ADDR24";
    case R_PPC_ADDR16 : return "R_PPC_ADDR16";
    case R_PPC_ADDR16_LO : return "R_PPC_ADDR16_LO";
    case R_PPC_ADDR16_HI : return "R_PPC_ADDR16_HI";
    case R_PPC_ADDR16_HA : return "R_PPC_ADDR16_HA";
    case R_PPC_ADDR14 : return "R_PPC_ADDR14";
    case R_PPC_ADDR14_BRTAKEN : return "R_PPC_ADDR14_BRTAKEN";
    case R_PPC_ADDR14_BRNTAKEN : return "R_PPC_ADDR14_BRNTAKEN";
    case R_PPC_REL24 : return "R_PPC_REL24";
    case R_PPC_REL14 : return "R_PPC_REL14";
    case R_PPC_REL14_BRTAKEN : return "R_PPC_REL14_BRTAKEN";
    case R_PPC_REL14_BRNTAKEN : return "R_PPC_REL14_BRNTAKEN";
    case R_PPC_GOT16 : return "R_PPC_GOT16";
    case R_PPC_GOT16_LO : return "R_PPC_GOT16_LO";
    case R_PPC_GOT16_HI : return "R_PPC_GOT16_HI";
    case R_PPC_GOT16_HA : return "R_PPC_GOT16_HA";
    case R_PPC_PLTREL24 : return "R_PPC_PLTREL24";
    case R_PPC_COPY : return "R_PPC_COPY";
    case R_PPC_GLOB_DAT : return "R_PPC_GLOB_DAT";
    case R_PPC_JMP_SLOT : return "R_PPC_JMP_SLOT";
    case R_PPC_RELATIVE : return "R_PPC_RELATIVE";
    case R_PPC_LOCAL24PC : return "R_PPC_LOCAL24PC";
    case R_PPC_UADDR32 : return "R_PPC_UADDR32";
    case R_PPC_UADDR16 : return "R_PPC_UADDR16";
    case R_PPC_REL32 : return "R_PPC_REL32";
    case R_PPC_PLT32 : return "R_PPC_PLT32";
    case R_PPC_PLTREL32 : return "R_PPC_PLTREL32";
    case R_PPC_PLT16_LO : return "R_PPC_PLT16_LO";
    case R_PPC_PLT16_HI : return "R_PPC_PLT16_HI";
    case R_PPC_PLT16_HA : return "R_PPC_PLT16_HA";
    case R_PPC_SDAREL16 : return "R_PPC_SDAREL16";
    case R_PPC_SECTOFF : return "R_PPC_SECTOFF";
    case R_PPC_SECTOFF_LO : return "R_PPC_SECTOFF_LO";
    case R_PPC_SECTOFF_HI : return "R_PPC_SECTOFF_HI";
    case R_PPC_SECTOFF_HA : return "R_PPC_SECTOFF_HA";
    case R_PPC_37 : return "R_PPC_37";
    case R_PPC_38 : return "R_PPC_38";
    case R_PPC_39 : return "R_PPC_39";
    case R_PPC_40 : return "R_PPC_40";
    case R_PPC_41 : return "R_PPC_41";
    case R_PPC_42 : return "R_PPC_42";
    case R_PPC_43 : return "R_PPC_43";
    case R_PPC_44 : return "R_PPC_44";
    case R_PPC_45 : return "R_PPC_45";
    case R_PPC_46 : return "R_PPC_46";
    case R_PPC_47 : return "R_PPC_47";
    case R_PPC_48 : return "R_PPC_48";
    case R_PPC_49 : return "R_PPC_49";
    case R_PPC_50 : return "R_PPC_50";
    case R_PPC_51 : return "R_PPC_51";
    case R_PPC_52 : return "R_PPC_52";
    case R_PPC_53 : return "R_PPC_53";
    case R_PPC_54 : return "R_PPC_54";
    case R_PPC_55 : return "R_PPC_55";
    case R_PPC_56 : return "R_PPC_56";
    case R_PPC_57 : return "R_PPC_57";
    case R_PPC_58 : return "R_PPC_58";
    case R_PPC_59 : return "R_PPC_59";
    case R_PPC_60 : return "R_PPC_60";
    case R_PPC_61 : return "R_PPC_61";
    case R_PPC_62 : return "R_PPC_62";
    case R_PPC_63 : return "R_PPC_63";
    case R_PPC_64 : return "R_PPC_64";
    case R_PPC_65 : return "R_PPC_65";
    case R_PPC_66 : return "R_PPC_66";
    case R_PPC_TLS : return "R_PPC_TLS";
    case R_PPC_DTPMOD32 : return "R_PPC_DTPMOD32";
    case R_PPC_TPREL16 : return "R_PPC_TPREL16";
    case R_PPC_TPREL16_LO : return "R_PPC_TPREL16_LO";
    case R_PPC_TPREL16_HI : return "R_PPC_TPREL16_HI";
    case R_PPC_TPREL16_HA : return "R_PPC_TPREL16_HA";
    case R_PPC_TPREL32 : return "R_PPC_TPREL32";
    case R_PPC_DTPREL16 : return "R_PPC_DTPREL16";
    case R_PPC_DTPREL16_LO : return "R_PPC_DTPREL16_LO";
    case R_PPC_DTPREL16_HI : return "R_PPC_DTPREL16_HI";
    case R_PPC_DTPREL16_HA : return "R_PPC_DTPREL16_HA";
    case R_PPC_DTPREL32 : return "R_PPC_DTPREL32";
    case R_PPC_GOT_TLSGD16 : return "R_PPC_GOT_TLSGD16";
    case R_PPC_GOT_TLSGD16_LO : return "R_PPC_GOT_TLSGD16_LO";
    case R_PPC_GOT_TLSGD16_HI : return "R_PPC_GOT_TLSGD16_HI";
    case R_PPC_GOT_TLSGD16_HA : return "R_PPC_GOT_TLSGD16_HA";
    case R_PPC_GOT_TLSLD16 : return "R_PPC_GOT_TLSLD16";
    case R_PPC_GOT_TLSLD16_LO : return "R_PPC_GOT_TLSLD16_LO";
    case R_PPC_GOT_TLSLD16_HI : return "R_PPC_GOT_TLSLD16_HI";
    case R_PPC_GOT_TLSLD16_HA : return "R_PPC_GOT_TLSLD16_HA";
    case R_PPC_GOT_TPREL16 : return "R_PPC_GOT_TPREL16";
    case R_PPC_GOT_TPREL16_LO : return "R_PPC_GOT_TPREL16_LO";
    case R_PPC_GOT_TPREL16_HI : return "R_PPC_GOT_TPREL16_HI";
    case R_PPC_GOT_TPREL16_HA : return "R_PPC_GOT_TPREL16_HA";
    case R_PPC_GOT_DTPREL16 : return "R_PPC_GOT_DTPREL16";
    case R_PPC_GOT_DTPREL16_LO : return "R_PPC_GOT_DTPREL16_LO";
    case R_PPC_GOT_DTPREL16_HI : return "R_PPC_GOT_DTPREL16_HI";
    case R_PPC_GOT_DTPREL16_HA : return "R_PPC_GOT_DTPREL16_HA";
    }
return "";
}
