/*!
*****************************************************************************

 @file      uccLowPrim.h
 @brief     UCC low level primitives (private implementation)

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

/* @cond DONT_DOXYGEN */

#include <stdbool.h>

#if defined(__SIM__) && defined(__DEBUG__)
#include <stdio.h>
#endif

/*-------------------------------------------------------------------------*/

#if !defined(WITH_PDUMP) && defined(__DEBUG__)
#define WITH_PDUMP
#endif

#ifdef WITH_PDUMP
#include "uccPdump.h"
#endif

/*-------------------------------------------------------------------------*/

/*
 * Real target primitives.
 * Don't use these directly, go through the access primtives (e.g. READ32),
 * which either map onto these macros or the PDUMP functions.
 */

#ifndef __SIM__

/* HW version of UCC_READ32 macro */
#define UCC_HW_READ32(addr)        (*(volatile uint32_t *)(addr))

/* HW version of UCC_WRITE32 macro */
#define UCC_HW_WRITE32(addr, val)  (*(volatile uint32_t *)(addr)) = (val)

/* HW version of UCC_POLL32 */
bool UCC_HW_POLL32(uint32_t      addr,
                   uint32_t      reqVal,
                   uint32_t      enable,
                   UCC_POLL_OP_T op,
                   int           pollCount,
                   int           delay);

/* Output message */
#define UCC_OUTMSG __TBILogF

#else

/*
 * Dummy versions that do nothing.
 */

/* HW version of UCC_READ32 macro */
#define UCC_HW_READ32(addr) (0)

/* HW version of UCC_WRITE32 macro */
#define UCC_HW_WRITE32(addr, val)

/* HW version of UCC_POLL32 */
#define UCC_HW_POLL32(addr, reqVal, enable, op, pollCount, delay) (true)

/* Output message */
#define UCC_OUTMSG printf

#endif /* !__SIM__ */

/*-------------------------------------------------------------------------*/

/*
 * Access primtitives.
 * There are different versions for PDUMP vs non-PDUMP.
 */
#ifdef WITH_PDUMP

/*
 * PDUMP version.
 * Call the PDUMP functions, which in-turn invoke the non-PDUMP
 * macros, after they've done their PDUMP logging. The exception to this is
 * LOGMSG, which gets tricky due to the variable arguments. This macro
 * therefore strings together both the PDUMP function and _HW_LOGMSG.
 */
 
/* Read a 32-bit value from a memory mapped location */
#define UCC_LOW_READ32      UCC_PDUMP_READ32

/* Write a 32-bit value from a memory mapped location */
#define UCC_LOW_WRITE32     UCC_PDUMP_WRITE32

/* Acknowledge the presence of a buffer */
#define UCC_LOW_IN32        UCC_PDUMP_IN32

/* Acknowledge the presence of a buffer */
#define UCC_LOW_OUT32       UCC_PDUMP_OUT32

/* Acknowledge the presence of a buffer */
#define UCC_LOW_POLL32      UCC_PDUMP_POLL32

/* Log a message */
#define UCC_LOW_LOGMSG(...) do { UCC_PDUMP_LOGMSG(__VA_ARGS__); UCC_OUTMSG(__VA_ARGS__); } while (0)

#else

/*
 * Non-PDUMP version.
 */
 
/* Read a 32-bit value from a memory mapped location */
#define UCC_LOW_READ32      UCC_HW_READ32

/* Write a 32-bit value from a memory mapped location */
#define UCC_LOW_WRITE32     UCC_HW_WRITE32

/* Acknowledge the presence of a buffer */
#define UCC_LOW_IN32(buffer, count) do {((void)(buffer));((void)(count));} while (0)

/* Acknowledge the presence of a buffer */
#define UCC_LOW_OUT32(buffer, count) do {((void)(buffer));((void)(count));} while (0)

/* Polls a 32-bit value from a memory mapped location */
#define UCC_LOW_POLL32      UCC_HW_POLL32

/* Log a message */
#define UCC_LOW_LOGMSG      UCC_OUTMSG

#endif /* WITH_PDUMP */

/*-------------------------------------------------------------------------*/

/* @endcond */

/*-------------------------------------------------------------------------*/
