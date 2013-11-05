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
	     Meta.

 @Version
	     1.0

******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log: img_defs.h,v $

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 


*****************************************************************************/

#include "../img_error.h"

#if !defined (__IMG_DEFS_H__)
#define __IMG_DEFS_H__

#include <assert.h>
#include <string.h>			/* Needed for memset/memcpy etc in 2.4.0b1 toolkit - got away with it for 2.3.3b4 */

#if (__cplusplus)
extern "C" {
#endif

#define IMG_LITTLE_ENDIAN	(0)
#define IMG_BIG_ENDIAN		(1)
/* Meta is little endian */
#define	IMG_BYTE_ORDER		IMG_LITTLE_ENDIAN

/* Keywords used by remote procedure call code */
#define	_IN
#define	_OUT
#define _SIZE
#define _OPAQUE
#define _TRANSLATE

typedef signed int		img_int32,	* img_pint32;
typedef int				img_result,	* img_presult;
typedef unsigned int	img_uint32, * img_puint32;
typedef void			img_void,	* img_pvoid;
typedef void**			img_hvoid,	* img_phvoid;

#if MESON_LC
	typedef void**			img_handle,	* img_phandle;
#else
	typedef void*			img_handle,	* img_phandle;
#endif

typedef int				img_bool,	* img_pbool;
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

#if defined __RELEASE_DEBUG__  || defined __RELEASE_TEST__
  #define IMG_ASSERT(A)		assert	(A)
  #define IMG_ASSERT_LIVE  // Defined when IMG_ASSERT() does something.
#else			// __RELEASE_RELEASE__
  #define IMG_ASSERT(A)
#endif

#if defined IMG_ASSERT_LIVE
	/* Take a 64 bit unsigned int, check it is suitable and cast it as 32bit unsigned int */
	#define IMG_UINT64_TO_UINT32(ui64Check) (IMG_ASSERT((ui64Check >> 32) == 0), (IMG_UINT32)ui64Check)
#else
	/* Take a 64 bit unsigned int, and cast it as 32bit unsigned int */
	#define IMG_UINT64_TO_UINT32(ui64Check) ((IMG_UINT32)ui64Check)
#endif


#define HARD_ASSERT(e) ((e) ? (void)0 : __TBIAssert(__FILE__, __LINE__, #e))

#define IMG_CALLCONV
#define IMG_EXPORT

#if defined __META_MEOS__ || defined __META_NUCLEUS_PLUS__
#define IMG_MALLOC(A)		IMG_MALLOC_DO_NOT_USE
#define IMG_FREE(A)			IMG_FREE_DO_NOT_USE
#define INLINE inline
#endif

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


