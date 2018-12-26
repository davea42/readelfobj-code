/* Created by build_access.py */
#include "dwarf_elf_reloc_arm.h"

/* returns string of length 0 if invalid arg */
const char * 
dwarf_get_elf_relocname_arm(unsigned long val)
{
    switch(val) {
    case R_ARM_NONE : return "R_ARM_NONE";
    case R_ARM_PC24 : return "R_ARM_PC24";
    case R_ARM_ABS32 : return "R_ARM_ABS32";
    case R_ARM_REL32 : return "R_ARM_REL32";
    case R_ARM_LDR_PC_G0 : return "R_ARM_LDR_PC_G0";
    case R_ARM_ABS16 : return "R_ARM_ABS16";
    case R_ARM_ABS12 : return "R_ARM_ABS12";
    case R_ARM_THM_ABS5 : return "R_ARM_THM_ABS5";
    case R_ARM_ABS8 : return "R_ARM_ABS8";
    case R_ARM_SBREL32 : return "R_ARM_SBREL32";
    case R_ARM_THM_CALL : return "R_ARM_THM_CALL";
    case R_ARM_THM_PC8 : return "R_ARM_THM_PC8";
    case R_ARM_BREL_ADJ : return "R_ARM_BREL_ADJ";
    case R_ARM_TLS_DESC : return "R_ARM_TLS_DESC";
    case R_ARM_THM_SWI8 : return "R_ARM_THM_SWI8";
    case R_ARM_XPC25 : return "R_ARM_XPC25";
    case R_ARM_THM_XPC22 : return "R_ARM_THM_XPC22";
    case R_ARM_TLS_DTPMOD32 : return "R_ARM_TLS_DTPMOD32";
    case R_ARM_TLS_DTPOFF32 : return "R_ARM_TLS_DTPOFF32";
    case R_ARM_TLS_TPOFF32 : return "R_ARM_TLS_TPOFF32";
    case R_ARM_COPY : return "R_ARM_COPY";
    case R_ARM_GLOB_DAT : return "R_ARM_GLOB_DAT";
    case R_ARM_JUMP_SLOT : return "R_ARM_JUMP_SLOT";
    case R_ARM_RELATIVE : return "R_ARM_RELATIVE";
    case R_ARM_GOTOFF32 : return "R_ARM_GOTOFF32";
    case R_ARM_BASE_PREL : return "R_ARM_BASE_PREL";
    case R_ARM_GOT_BREL : return "R_ARM_GOT_BREL";
    case R_ARM_PLT32 : return "R_ARM_PLT32";
    case R_ARM_CALL : return "R_ARM_CALL";
    case R_ARM_JUMP24 : return "R_ARM_JUMP24";
    case R_ARM_THM_JUMP24 : return "R_ARM_THM_JUMP24";
    case R_ARM_BASE_ABS : return "R_ARM_BASE_ABS";
    case R_ARM_ALU_PCREL_7_0 : return "R_ARM_ALU_PCREL_7_0";
    case R_ARM_ALU_PCREL_15_8 : return "R_ARM_ALU_PCREL_15_8";
    case R_ARM_ALU_PCREL_23_15 : return "R_ARM_ALU_PCREL_23_15";
    case R_ARM_LDR_SBREL_11_0_NC : return "R_ARM_LDR_SBREL_11_0_NC";
    case R_ARM_ALU_SBREL_19_12_NC : return "R_ARM_ALU_SBREL_19_12_NC";
    case R_ARM_ALU_SBREL_27_20_CK : return "R_ARM_ALU_SBREL_27_20_CK";
    case R_ARM_TARGET1 : return "R_ARM_TARGET1";
    case R_ARM_SBREL31 : return "R_ARM_SBREL31";
    case R_ARM_V4BX : return "R_ARM_V4BX";
    case R_ARM_TARGET2 : return "R_ARM_TARGET2";
    case R_ARM_PREL31 : return "R_ARM_PREL31";
    case R_ARM_MOVW_ABS_NC : return "R_ARM_MOVW_ABS_NC";
    case R_ARM_MOVT_ABS : return "R_ARM_MOVT_ABS";
    case R_ARM_MOVW_PREL_NC : return "R_ARM_MOVW_PREL_NC";
    case R_ARM_MOVT_PREL : return "R_ARM_MOVT_PREL";
    case R_ARM_THM_MOVW_ABS_NC : return "R_ARM_THM_MOVW_ABS_NC";
    case R_ARM_THM_MOVT_ABS : return "R_ARM_THM_MOVT_ABS";
    case R_ARM_THM_MOVW_PREL_NC : return "R_ARM_THM_MOVW_PREL_NC";
    case R_ARM_THM_MOVT_PREL : return "R_ARM_THM_MOVT_PREL";
    case R_ARM_THM_JUMP19 : return "R_ARM_THM_JUMP19";
    case R_ARM_THM_JUMP6 : return "R_ARM_THM_JUMP6";
    case R_ARM_THM_ALU_PREL_11_0 : return "R_ARM_THM_ALU_PREL_11_0";
    case R_ARM_THM_PC12 : return "R_ARM_THM_PC12";
    case R_ARM_ABS32_NOI : return "R_ARM_ABS32_NOI";
    case R_ARM_REL32_NOI : return "R_ARM_REL32_NOI";
    case R_ARM_ALU_PC_G0_NC : return "R_ARM_ALU_PC_G0_NC";
    case R_ARM_ALU_PC_G0 : return "R_ARM_ALU_PC_G0";
    case R_ARM_ALU_PC_G1_NC : return "R_ARM_ALU_PC_G1_NC";
    case R_ARM_ALU_PC_G1 : return "R_ARM_ALU_PC_G1";
    case R_ARM_ALU_PC_G2 : return "R_ARM_ALU_PC_G2";
    case R_ARM_LDR_PC_G1 : return "R_ARM_LDR_PC_G1";
    case R_ARM_LDR_PC_G2 : return "R_ARM_LDR_PC_G2";
    case R_ARM_LDRS_PC_G0 : return "R_ARM_LDRS_PC_G0";
    case R_ARM_LDRS_PC_G1 : return "R_ARM_LDRS_PC_G1";
    case R_ARM_LDRS_PC_G2 : return "R_ARM_LDRS_PC_G2";
    case R_ARM_LDC_PC_G0 : return "R_ARM_LDC_PC_G0";
    case R_ARM_LDC_PC_G1 : return "R_ARM_LDC_PC_G1";
    case R_ARM_LDC_PC_G2 : return "R_ARM_LDC_PC_G2";
    case R_ARM_ALU_SB_G0_NC : return "R_ARM_ALU_SB_G0_NC";
    case R_ARM_ALU_SB_G0 : return "R_ARM_ALU_SB_G0";
    case R_ARM_ALU_SB_G1_NC : return "R_ARM_ALU_SB_G1_NC";
    case R_ARM_ALU_SB_G1 : return "R_ARM_ALU_SB_G1";
    case R_ARM_ALU_SB_G2 : return "R_ARM_ALU_SB_G2";
    case R_ARM_LDR_SB_G0 : return "R_ARM_LDR_SB_G0";
    case R_ARM_LDR_SB_G1 : return "R_ARM_LDR_SB_G1";
    case R_ARM_LDR_SB_G2 : return "R_ARM_LDR_SB_G2";
    case R_ARM_LDRS_SB_G0 : return "R_ARM_LDRS_SB_G0";
    case R_ARM_LDRS_SB_G1 : return "R_ARM_LDRS_SB_G1";
    case R_ARM_LDRS_SB_G2 : return "R_ARM_LDRS_SB_G2";
    case R_ARM_LDC_SB_G0 : return "R_ARM_LDC_SB_G0";
    case R_ARM_LDC_SB_G1 : return "R_ARM_LDC_SB_G1";
    case R_ARM_LDC_SB_G2 : return "R_ARM_LDC_SB_G2";
    case R_ARM_MOVW_BREL_NC : return "R_ARM_MOVW_BREL_NC";
    case R_ARM_MOVT_BREL : return "R_ARM_MOVT_BREL";
    case R_ARM_MOVW_BREL : return "R_ARM_MOVW_BREL";
    case R_ARM_THM_MOVW_BREL_NC : return "R_ARM_THM_MOVW_BREL_NC";
    case R_ARM_THM_MOVT_BREL : return "R_ARM_THM_MOVT_BREL";
    case R_ARM_THM_MOVW_BREL : return "R_ARM_THM_MOVW_BREL";
    case R_ARM_TLS_GOTDESC : return "R_ARM_TLS_GOTDESC";
    case R_ARM_TLS_CALL : return "R_ARM_TLS_CALL";
    case R_ARM_TLS_DESCSEQ : return "R_ARM_TLS_DESCSEQ";
    case R_ARM_THM_TLS_CALL : return "R_ARM_THM_TLS_CALL";
    case R_ARM_PLT32_ABS : return "R_ARM_PLT32_ABS";
    case R_ARM_GOT_ABS : return "R_ARM_GOT_ABS";
    case R_ARM_GOT_PREL : return "R_ARM_GOT_PREL";
    case R_ARM_GOT_BREL12 : return "R_ARM_GOT_BREL12";
    case R_ARM_GOTOFF12 : return "R_ARM_GOTOFF12";
    case R_ARM_GOTRELAX : return "R_ARM_GOTRELAX";
    case R_ARM_GNU_VTENTRY : return "R_ARM_GNU_VTENTRY";
    case R_ARM_GNU_VTINHERIT : return "R_ARM_GNU_VTINHERIT";
    case R_ARM_THM_JUMP11 : return "R_ARM_THM_JUMP11";
    case R_ARM_THM_JUMP8 : return "R_ARM_THM_JUMP8";
    case R_ARM_TLS_GD32 : return "R_ARM_TLS_GD32";
    case R_ARM_TLS_LDM32 : return "R_ARM_TLS_LDM32";
    case R_ARM_TLS_LDO32 : return "R_ARM_TLS_LDO32";
    case R_ARM_TLS_IE32 : return "R_ARM_TLS_IE32";
    case R_ARM_TLS_LE32 : return "R_ARM_TLS_LE32";
    case R_ARM_TLS_LDO12 : return "R_ARM_TLS_LDO12";
    case R_ARM_TLS_LE12 : return "R_ARM_TLS_LE12";
    case R_ARM_TLS_IE12GP : return "R_ARM_TLS_IE12GP";
    case R_ARM_ME_TOO : return "R_ARM_ME_TOO";
    case R_ARM_THM_TLS_DESCSEQ16 : return "R_ARM_THM_TLS_DESCSEQ16";
    case R_ARM_THM_TLS_DESCSEQ32 : return "R_ARM_THM_TLS_DESCSEQ32";
    case R_ARM_RXPC25 : return "R_ARM_RXPC25";
    case R_ARM_RSBREL32 : return "R_ARM_RSBREL32";
    case R_ARM_THM_RPC22 : return "R_ARM_THM_RPC22";
    case R_ARM_RREL32 : return "R_ARM_RREL32";
    case R_ARM_RABS32 : return "R_ARM_RABS32";
    case R_ARM_RPC24 : return "R_ARM_RPC24";
    case R_ARM_RBASE : return "R_ARM_RBASE";
    case R_ARM_NUM : return "R_ARM_NUM";
    case R_AARCH64_ABS64 : return "R_AARCH64_ABS64";
    case R_AARCH64_ABS32 : return "R_AARCH64_ABS32";
    }
return "";
}
