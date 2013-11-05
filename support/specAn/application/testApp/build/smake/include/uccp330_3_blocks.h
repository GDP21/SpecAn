/* @cond DONT_DOXYGEN */
/**********************************************************************\

UCCP330_3 register-block and memory-block definitions

This is an automatically generated file: DO NOT EDIT

------------------------------------------------------------------------

Copyright (c) Imagination Technologies Limited
All rights reserved
Strictly confidential

\**********************************************************************/

#ifndef _UCCP330_3_BLOCKS_H_
#define _UCCP330_3_BLOCKS_H_

/*--------------------------------------------------------------------*/

/* System bus registers */

#define REGSYSBUS   0x02000000
#define REGMTXUCCP  0x02000300
#define REGSYSREG   0x02000400
#define REGSOCFAB   0x02000600
#define REGTSDMUX   0x02000800
#define REGMCPREG   0x02001000
#define REGWLANTMR  0x02002200
#define REGREDSOL   0x02002400
#define REGSTCONT   0x02002C00
#define REGCRYPTO   0x02002D00
#define REGPHYWAY   0x02002E00
#define REGMCP2REG  0x02003000
#define REGMCP3REG  0x02004000
#define REGMCP4REG  0x02005000
#define REGEDCREG   0x02006000
#define REGMCP5REG  0x02007000
#define REGMCP6REG  0x02008000
#define REGEXTREG   0x02009000
#define REGTSCREG   0x02009300
#define REG_SLVDBG  0x0203C000
#define REG_META_SD 0x0203C000
#define REGPMRREG   0x02040000

/*--------------------------------------------------------------------*/

/* Peripheral bus registers */

#define PMR_MCP              0x02040000
#define PMR_SCP              0x02041000
#define PMR_SCP5             0x02041800
#define PMR_SCP6             0x02042000
#define PMR_LDPC2            0x02042800
#define PMR_TURBO_DEC        0x02043000
#define PMR_WLAN_ED          0x02043800
#define PMR_WLAN_AGC         0x02043C00
#define PMR_WLAN_ED2         0x02044000
#define PMR_WLAN_AGC2        0x02044400
#define PMR_UCC_VITERBI2     0x02044800
#define PMR_UCC_TSO2         0x02045000
#define PMR_TS_SYNC2         0x02045800
#define PMR_SCP2             0x02046000
#define PMR_SCP3             0x02046800
#define PMR_SCP4             0x02047000
#define PMR_SCP_WO           0x02047800
#define PMR_UCC_VITERBI      0x02048000
#define PMR_UCC_DERANDOMISER 0x02048800
#define PMR_UCC_TSO          0x02049000
#define PMR_WLAN_DEAGG       0x02049800
#define PMR_WLAN_TXP         0x0204A000
#define PMR_WLAN_TXC         0x0204A800
#define PMR_DFE              0x0204B000
#define PMR_DFE2             0x0204B800
#define PMR_OD_SYND          0x0204C000
#define PMR_OD_LAMBDA        0x0204C800
#define PMR_OD_CHIEN         0x0204D000
#define PMR_OD_ERROR_MAGS    0x0204D800
#define PMR_TS_SYNC          0x0204E000
#define PMR_UCC_GRAM         0x0204E800
#define PMR_LDPC             0x0204F000
#define PMR_DEMAPPER         0x0204F800
#define PMR_MCP2             0x02050000
#define PMR_MCP3             0x02051000
#define PMR_MCP4             0x02052000
#define PMR_EDC              0x02053000
#define PMR_UCC_EFS          0x02054000
#define PMR_MCP5             0x02056000
#define PMR_MCP6             0x02057000
#define PMR_RESAMPLER        0x02058000
#define PMR_DVBT2_OUT_PROC   0x02058800
#define PMR_HSBDI_B          0x02059000
#define PMR_BIT_PACKER       0x02059800
#define PMR_DVBT2_OUT_PROC2  0x0205A000
#define PMR_RESAMPLER2       0x0205A800
#define PMR_RESAMPLER3       0x0205B000
#define PMR_RESAMPLER4       0x0205B800
#define PMR_RESAMPLER5       0x0205C000
#define PMR_SCP7             0x0205C800
#define PMR_SCP8             0x0205D000
#define PMR_SCP9             0x0205D800
#define PMR_SCP10            0x0205E000
#define PMR_SCP11            0x0205E800
#define PMR_SCP12            0x0205F000
#define PMR_BIT_PACKER2      0x0205F800
#define PMR_BLUETOOTH        0x02060000
#define PMR_OD2_SYND         0x02060800
#define PMR_OD2_LAMBDA       0x02061000
#define PMR_OD2_CHIEN        0x02061800
#define PMR_OD2_ERROR_MAGS   0x02062000
#define PMR_WLAN_TIMERS      0x02062800
#define PMR_PHYWAY           0x02063000
#define PMR_UCC_QM           0x0207F000

/*--------------------------------------------------------------------*/

/* Memory */

#define MEMSYSMEM  0xB0000000
#define MEMGBL_DBL 0xB4000000
#define MEMGBL_SXT 0xB5000000
#define MEMGBL_CPX 0xB6000000
#define MEMGBL_PKD 0xB7000000

/*--------------------------------------------------------------------*/

/* Miscellaneous regions */

#define REGSIMCODE 0xFFF00000
#define REGMTXCODE 0xFFF00000
#define REGMTXDATA 0xFFF00000

#define REG_JD     0x00000000
#define REG_JTAG   0x00000000

/*--------------------------------------------------------------------*/

/* MCP data memory regions */

#define MCP_A_START         0x000000
#define MCP_A_SIZE          0x100000
#define MCP_B_START         0x100000
#define MCP_B_SIZE          0x100000
#define MCP_L_START         0x7F0000
#define MCP_L_SIZE          0x100000

#define MCP310_WIDE_START   0x00000
#define MCP310_NARROW_START 0x18000

/*--------------------------------------------------------------------*/

#endif /* _UCCP330_3_BLOCKS_H_ */

/*--------------------------------------------------------------------*/
/* @endcond */
