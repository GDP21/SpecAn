/*!
*****************************************************************************

 @file      uccPdump.h
 @brief     PDUMP version of the low level primitives

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

/* @cond DONT_DOXYGEN */

/*-------------------------------------------------------------------------*/

#ifndef _UCC_PDUMP_H_
#define _UCC_PDUMP_H_

/*-------------------------------------------------------------------------*/

/* Read/write a 32-bit value from a memory mapped location */
uint32_t UCC_PDUMP_READ32(uint32_t addr);
void UCC_PDUMP_WRITE32(uint32_t addr, uint32_t val);

/* Acknowledge the presence of a 32-bit-aligned buffer */
void UCC_PDUMP_IN32(uint32_t *buffer, uint32_t count);
void UCC_PDUMP_OUT32(uint32_t *buffer, uint32_t count);

/* Polls a 32-bit value from a memory mapped location */
bool UCC_PDUMP_POLL32(uint32_t      addr,
                      uint32_t      reqVal,
                      uint32_t      enable,
                      UCC_POLL_OP_T op,
                      int           pollCount,
                      int           delay);

/* Log a message */
void UCC_PDUMP_LOGMSG(char *fmt, ...);

/*-------------------------------------------------------------------------*/

#endif /* _UCC_PDUMP_H_ */

/*-------------------------------------------------------------------------*/

/* @endcond */

/*-------------------------------------------------------------------------*/
