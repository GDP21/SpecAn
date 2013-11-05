/*!
*****************************************************************************

 @file      uccRegDefs.h
 @brief     Top level header file for UCC register definitions

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#ifndef _UCC_REG_DEFS_H_
#define _UCC_REG_DEFS_H_

#ifdef __UCCP_REG_DEFS_H__
#include __UCCP_REG_DEFS_H__

#ifdef __UCCP320_2__
/*
 * Uniquely for a META based configuration, this has a daisy-chained interrupt logic block.
 * We set this up in place of the standard HWSTATEXT4, thus making it the third interrupt block,
 * servicing interrupts 64..95.
 *
 * This is a hack, based on the belief that such configurations will never again be
 * used in future UCCPs
 */
#ifdef HWSTATEXT4
#undef HWSTATEXT4
#endif
#ifdef HWLEVELEXT4
#undef HWLEVELEXT4
#endif
#ifdef HWVEC40EXT
#undef HWVEC40EXT
#endif
#ifdef HWMASKEXT4
#undef HWMASKEXT4
#endif
#define HWSTATEXT4  0x02014090 /* Saturn CR_PERIP_HWSTATEXT */
#define HWLEVELEXT4 0x02014094 /* Saturn CR_PERIP_HWLEVELEXT */
#define HWVEC40EXT  0x02014300 /* Saturn CR_PERIP_SCB0_HWVECEXT */
/* No HWMASKEXT4 - it doen't exist! */
#endif
#else
#error UCCP definitions not included. Is UCCP specified?
#endif

#endif /* _UCC_REG_DEFS_H_ */
