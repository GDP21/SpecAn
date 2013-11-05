/*!****************************************************************************
 @File          ufwconfig.h

 @Title         Example Application dependent configuration file for the UCC framework

 @Date          16 Sep 2010

 @Copyright     Copyright (C) Imagination Technologies Limited 2010

 @Description   Defines various application-specific sizing and configuration
                constants for the UCC framework.

                This file is should copied into an application specific module
                as ufwconfig.h. It can then be edited as required for the particular
                application.

 ******************************************************************************/

#ifndef _UFWCONFIG_H_
#define _UFWCONFIG_H_


/* Pool sizes */
#define UFW_NORMAL_POOL_BYTES 0x2000
#define UFW_FAST_POOL_BYTES 0x2000
#define UFW_UNCACHED_POOL_BYTES 0x2000
#define UFW_GRAM_POOL_WORDS 0x10000

/* MeOS related sizing and configuration constants */

#define UFW_MAX_MEOS_PRIORITY 5
#define UFW_MAX_QIO_EXTINTNUM 96
#define UFW_MEOS_TRACEBUF_SIZE 0
#define UFW_MEOS_TIMER_STACK_SIZE 512
#define UFW_MEOS_STACK_INIT_VALUE 0xdeadbeef
#define UFW_MEOS_TICK_LENGTH 10000

#endif /* _UFWCONFIG_H_ */
