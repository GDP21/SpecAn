/*!****************************************************************************
 @File          ufwconfig.h

 @Title         Spectrum Analyser specific configuration file for the UCC framework

 @Date          29 Nov 2012

 @Copyright     Copyright (C) Imagination Technologies Limited 2010

 @Description   Defines various application-specific sizing and configuration
                constants for the UCC framework.

 ******************************************************************************/

#ifndef _UFWCONFIG_H_
#define _UFWCONFIG_H_


/* Pool sizes */
/* Normal pool holds our context space and some stacks */
#if (defined(EXAMPLE_TUNER) || defined (DUMMY_TUNER))
#define UFW_NORMAL_POOL_BYTES 0x1C300	/* Need to reduce pool size to squeeze into core on emulator builds without tuner support */
#else
#define UFW_NORMAL_POOL_BYTES 0x1D600
#endif
/* 0x3200 + 0xE00 (global heap) = 0x4000 which is 8 pages, leaving other pages for code */
#define UFW_FAST_POOL_BYTES 0x1500
/* Uncached pool not currently used for anything on DVB-S2.  (If not working with the TSO,
it could be used to hold a large transport stream output buffer). */
#define UFW_UNCACHED_POOL_BYTES 0
//#define UFW_UNCACHED_POOL_BYTES 0x200000
/* Size of GRAM on config 2 is 0x51600 words.  Need to allow some additional space
for DCP and RCD images which are not included in the pool. */
#ifdef MCP_LOGGING_ENABLE
#define UFW_GRAM_POOL_WORDS 0x40000
#else
/* Size of GRAM on config 16 is 0x1cF40 words.  Add 0x800 words to the MCP GRAM size (to allow for PD buffer and a few other
odds and ends taken from this pool). */
#define UFW_GRAM_POOL_WORDS 0x18800
#endif

/* MeOS related sizing and configuration constants */

#define UFW_MAX_MEOS_PRIORITY 5
#define UFW_MAX_QIO_EXTINTNUM 96
#define UFW_MEOS_TRACEBUF_SIZE 10
#define UFW_MEOS_TIMER_STACK_SIZE 0x200
#define UFW_MEOS_STACK_INIT_VALUE 0xdeadbeef
#define UFW_MEOS_TICK_LENGTH 1000 /* 1ms MeOS tick */

#endif /* _UFWCONFIG_H_ */
