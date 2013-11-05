/*!
******************************************************************************
 @file   : img_types.h

 @brief

 @Author Imagination Technologies

 @date   01/06/2005

         <b>Copyright 2005 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to third parties without the
         express written permission of Imagination Technologies
         Limited, Unit 8, HomePark Industrial Estate,
         King's Langley, Hertfordshire, WD4 8LZ, U.K.

 \n<b>Description:</b>\n
         This file contains the IMG Type Definitions.

 \n<b>Platform:</b>\n
	     Win32.

 @Version
	-	1.0	- Initial release.

******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log: img_types.h,v $
 Revision 1.1  2009/02/16 15:43:09  michael.melbourne
 Initial version

 Revision 1.1  2007/12/11 15:36:41  michael.melbourne
 Initial versions for DVBH Remote API

 Revision 1.11  2007/07/04 13:38:27  mah
 Added void* typedef for IMG_INT128 to allow SAVI to build.

 Revision 1.10  2007/01/25 10:32:50  erf
 Merged changes from Eurasia files.

 Revision 1.9  2006/05/30 09:54:26  grc
 Redefined typedef for IMG_HANDLE to be img_void *.

 Revision 1.8  2006/05/12 08:25:17  jer
 Modifications made to make the emulation closer to Vista

 Revision 1.7  2006/01/27 04:09:45  ra
 First cut at intergration of Mpeg4 entdec and firmware.

 Revision 1.6  2005/11/22 11:25:48  erf
 Removed last checkin.

 Revision 1.5  2005/11/21 18:16:24  erf
 Use "long long" for int64 definition.

 Revision 1.4  2005/11/01 17:24:27  ra
 Update to work with the latest TAL

 Revision 1.3  2005/10/20 08:30:17  rml
 Change platform

 Revision 1.2  2005/10/20 08:26:35  rml
 Slight re-oganisation

 Revision 1.1  2005/10/19 16:58:23  rml
 Cross link to get upper and lower type defs

 Revision 1.5  2005/10/07 11:35:46  ra
 Full system compile and run.

 Need to add VDEB and VDMC. At that moment vec_test.cpp serves as  VDEB and VDMC.
 Missing implementation of unlockCommsMutex function was faked.

 Revision 1.4  2005/09/14 08:42:59  rml
 Add IMG_ALIGN()

 Revision 1.3  2005/09/01 11:38:26  rml
 no message

 Revision 1.2  2005/08/23 09:40:23  rml
 Add ALIGN and INLINE

 Revision 1.1  2005/08/12 14:35:06  rml
 Added to CVS


*****************************************************************************/

#if !defined(__IMG_TYPES_H__)
#define __IMG_TYPES_H__

#if (__cplusplus)
extern "C" {
#endif

typedef unsigned int	IMG_UINT,	*IMG_PUINT;
typedef signed int		IMG_INT,	*IMG_PINT;

typedef unsigned char	IMG_UINT8,	*IMG_PUINT8;
typedef unsigned char	IMG_BYTE,	*IMG_PBYTE;
typedef signed char		IMG_INT8,	*IMG_PINT8;
typedef char			IMG_CHAR,	*IMG_PCHAR;

typedef unsigned short	IMG_UINT16,	*IMG_PUINT16;
typedef signed short	IMG_INT16,	*IMG_PINT16;

typedef unsigned int	IMG_UINT32,	*IMG_PUINT32;
typedef signed int		IMG_INT32,	*IMG_PINT32;

typedef unsigned __int64 IMG_UINT64, * IMG_PUINT64;
typedef __int64          IMG_INT64,  * IMG_PINT64;
typedef float			IMG_FLOAT,	*IMG_PFLOAT;
typedef double			IMG_DOUBLE, *IMG_PDOUBLE;

typedef int				IMG_BOOL,	*IMG_PBOOL;

typedef void			IMG_VOID,	*IMG_PVOID;

typedef void**			IMG_HVOID,	* IMG_PHVOID;

typedef IMG_INT32		IMG_RESULT;

typedef IMG_UINT32      IMG_UINTPTR_T;

typedef void*			IMG_HANDLE;

typedef IMG_UINT32      IMG_SIZE_T;

typedef void*			IMG_INT128;

/*
 * Address types.
 * All types used to refer to a block of memory are wrapped in structures
 * to enforce some type degree of type safety, i.e. a IMG_DEV_VIRTADDR cannot
 * be assigned to a variable of type IMG_DEV_PHYSADDR because they are not the
 * same thing.
 *
 * There is an assumption that the system contains at most one non-cpu mmu,
 * and a memory block is only mapped by the MMU once.
 *
 * Different devices could have offset views of the physical address space.
 * 
 */


/*
 *
 * +------------+    +------------+      +------------+        +------------+
 * |    CPU     |    |    DEV     |      |    DEV     |        |    DEV     |
 * +------------+    +------------+      +------------+        +------------+
 *       |                 |                   |                     |
 *       | PVOID           |IMG_DEV_VIRTADDR   |IMG_DEV_VIRTADDR     |
 *       |                 \-------------------/                     |
 *       |                          |                                |
 * +------------+             +------------+                         |     
 * |    MMU     |             |    MMU     |                         |
 * +------------+             +------------+                         | 
 *       |                          |                                | 
 *       |                          |                                |
 *       |                          |                                |
 *   +--------+                +---------+                      +--------+
 *   | Offset |                | (Offset)|                      | Offset |
 *   +--------+                +---------+                      +--------+    
 *       |                          |                IMG_DEV_PHYADDR | 
 *       |                          |                                |
 *       |                          | IMG_DEV_PHYADDR                |
 * +---------------------------------------------------------------------+ 
 * |                         System Address bus                          |
 * +---------------------------------------------------------------------+
 *
 */

typedef IMG_PVOID IMG_CPU_VIRTADDR;

/* cpu physical address */
typedef struct {IMG_UINT32 uiAddr;} IMG_CPU_PHYADDR;

/* device virtual address */
typedef struct {IMG_UINT32 uiAddr;} IMG_DEV_VIRTADDR;

/* device physical address */
typedef struct {IMG_UINT32 uiAddr;} IMG_DEV_PHYADDR;

/* system physical address */
typedef struct {IMG_UINT32 uiAddr;} IMG_SYS_PHYADDR;

/* 
	system physical structure.
	specifies contiguous and non-contiguous system physical addresses
*/
typedef struct _SYSTEM_ADDR_
{
	/* if zero this is contiguous */
	IMG_UINT32	ui32PageCount;
	union
	{
		/*	
			contiguous address: 
			basic system address  
		*/
		IMG_SYS_PHYADDR	sContig;		

		/* 
			non-contiguous address:
			multiple system page addresses representing system pages 
			of which a single allocation is composed
			Note: if non-contiguous allocations don't always start at a
			page boundary then a page offset word is also required.
		*/
		IMG_SYS_PHYADDR	asNonContig[1];
	} u;
} SYSTEM_ADDR;

#if (__cplusplus)
}
#endif

#if !defined (__IMG_DEFS_H__)
#include "img_defs.h"
#endif

#endif	/* __IMG_TYPES_H__ */
/*****************************************************************************
 End of file (img_types.h)
*****************************************************************************/
