/*!
******************************************************************************
 @file   : img_defs.h

 @brief

 @Author Various

 @date   03/06/2003

         <b>Copyright 2003 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to third parties without the
         express written permission of Imagination Technologies
         Limited, Unit 8, HomePark Industrial Estate,
         King's Langley, Hertfordshire, WD4 8LZ, U.K.

 <b>Description:</b>\n
         This header file contains the Imagination Technologies platform
		 specific defintions.

 <b>Platform:</b>\n
	     MTX.

 @Version
	     1.0

******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log: img_defs.h,v $
 Revision 1.1  2009/02/16 15:43:08  michael.melbourne
 Initial version

 Revision 1.13  2007/04/23 09:21:54  jmg
 Added IMG_NORETURN.

 Revision 1.12  2007/04/11 14:45:55  tsb
 Add IMG_NORETURN

 Revision 1.11  2007/01/04 15:08:22  jmg
 Improved DO_NOT_USE macros. Added fprintf().

 Revision 1.10  2007/01/03 14:49:49  jmg
 Added checks preventing non-debug mtx builds from using printf() and fopen().

 Revision 1.9  2006/12/08 12:42:34  jmg
 Added NO_ASSERT directive.

 Revision 1.8  2006/10/31 16:24:11  jmg
 Enabled IMG_ASSERT() for test builds.

 Revision 1.7  2006/06/01 12:56:27  jmg
 Fixed IMG_ASSERT() for use in macros with test build.

 Revision 1.6  2006/04/21 17:14:11  grc
 Changed TAL memory functions to use IMG_HANDLE rather than IMG_HVOID.
 Also changed all appropriate occurences of IMG_HVOID to IMG_HANDLE.

 Revision 1.5  2005/12/12 16:17:21  jmg
 Fixed 64-bit types.

 Revision 1.4  2005/11/03 11:30:07  ra
 Pickup on new img_def headers.
 Add tal support for Msvdx
 Further work on vec local ram buffers

 Revision 1.3  2005/10/21 15:59:26  rml
 Fix align macro

 Revision 1.2  2005/10/21 15:56:55  rml
 Fix align macro

 Revision 1.1  2005/10/20 08:43:10  rml
 Copied and modifified slightly from Meta


*****************************************************************************/


#if !defined (__IMG_DEFS_H__)
#define __IMG_DEFS_H__

#include <assert.h>

#if (__cplusplus)
extern "C" {
#endif

typedef signed int		img_int32,	* img_pint32;
typedef int				img_result,	* img_presult;
typedef unsigned int	img_uint32, * img_puint32;
typedef void			img_void,	* img_pvoid;
typedef void*			img_handle,	* img_phandle;
typedef void**			img_hvoid,	* img_phvoid;
typedef unsigned char	img_bool,	* img_pbool;
typedef unsigned char	img_uint8,	* img_puint8;
typedef float			img_float,	* img_pfloat;
typedef char			img_int8,	* img_pint8;
typedef char			img_char,	* img_pchar;
typedef unsigned char	img_byte,	* img_pbyte;
typedef unsigned short	img_uint16, * img_puint16;
typedef short			img_int16,  * img_pint16;
typedef unsigned long long img_uint64, * img_puint64;
typedef long long       img_int64,  * img_pint64;
typedef unsigned long   img_uintptr_t;


#define	IMG_SUCCESS					(0)
#define IMG_TIMEOUT					(1)
#define IMG_MALLOC_FAILED			(2)
#define IMG_FATAL_ERROR				(3)

typedef		enum	img_tag_TriStateSwitch
{
	IMG_ON		=	0x00,
	IMG_OFF,
	IMG_IGNORE

} img_TriStateSwitch, * img_pTriStateSwitch;


#define	IMG_NULL 0
#define IMG_TRUE 1
#define IMG_FALSE 0

/* Maths functions - platform dependant						*/
#define		IMG_COS					cos
#define		IMG_SIN					sin
#define		IMG_POW(A, B)			pow		(A,B)
#define		IMG_SQRT(A)				sqrt	(A)
#define		IMG_FABS(A)				fabs	(A)
#define		IMG_EXP(A)				exp		(A)

/* General functions										*/
#define	IMG_MEMCPY(A,B,C)	memcpy	(A,B,C)
#define	IMG_MEMSET(A,B,C)	memset	(A,B,C)
#define	IMG_MEMCMP(A,B,C)	memcmp	(A,B,C)

/* Takes any two signed values - return 'IMG_TRUE' if they have the same polarity */
#define	IMG_SAMESIGN(A,B)	(((((img_int32)A)^((img_int32)B))&0x80000000)==0)

#if defined __RELEASE_RELEASE__ || defined NO_ASSERT
#define IMG_ASSERT(A) (void)0
#else  // (__RELEASE_DEBUG__ or __RELEASE_TEST__) and not NO_ASSERT
  #define IMG_ASSERT(A)		assert	(A)
  #define IMG_ASSERT_LIVE  // Defined when IMG_ASSERT() does something.
#endif

#define HARD_ASSERT(e) ((e) ? (void)0 : __TBIAssert(__FILE__, __LINE__, #e))

#define IMG_CALLCONV
#define IMG_EXPORT

#define IMG_MALLOC(A)		IMG_MALLOC_DO_NOT_USE
#define IMG_FREE(A)			IMG_FREE_DO_NOT_USE

#ifndef __RELEASE_DEBUG__
#define fprintf(...)   fprintf_DO_NOT_USE()
#define printf(...)    printf_DO_NOT_USE()
#define fopen(...)     fopen_DO_NOT_USE()
#endif

#define INLINE inline

#define IMG_ALIGN(byte)       __attribute__ ((aligned (byte)))
#define IMG_NORETURN          __attribute__ ((noreturn))

#define IMG_NORETURN		__attribute__ ((noreturn))

#if (__cplusplus)
}
#endif

#if !defined (__IMG_TYPES_H__)
#include "img_types.h"
#endif

#endif // IMG_DEFS_H


