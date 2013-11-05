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
	     Win32.

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

 This is major commit #1 of 3. There will be another major commit based
 upon the IMG Framework application and libraries, as well as another
 for scripts and tagging.

 -----> THIS IS NOT YET A USEABLE STRUCTURE <-----

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 


*****************************************************************************/

#include "../img_error.h"

#if !defined (__IMG_DEFS_H__)
#define __IMG_DEFS_H__

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <stddef.h>

#ifndef __IMG_TYPES_H__
#include "img_types.h"
#endif

#if (__cplusplus)
extern "C" {
#endif

#define IMG_LITTLE_ENDIAN	(0)
#define IMG_BIG_ENDIAN		(1)
/* Win32 is little endian */
#define	IMG_BYTE_ORDER		IMG_LITTLE_ENDIAN

typedef signed int		img_int32,	* img_pint32;
typedef int				img_result,	* img_presult;
typedef unsigned int	img_uint32, * img_puint32;
typedef void			img_void,	* img_pvoid;
typedef void*		    img_handle,	* img_phandle;
typedef void**			img_hvoid,	* img_phvoid;
typedef int				img_bool,	* img_pbool;
typedef unsigned char	img_uint8,	* img_puint8;
typedef float			img_float,	* img_pfloat;
typedef char			img_int8,	* img_pint8;
typedef char			img_char,	* img_pchar;
typedef unsigned char	img_byte,	* img_pbyte;
typedef unsigned short	img_uint16, * img_puint16;
typedef short			img_int16,  * img_pint16;

#if defined (CYGWIN)
typedef unsigned long long  img_uint64, * img_puint64;
typedef  long long      img_int64,  * img_pint64;
#else
typedef unsigned __int64  img_uint64, * img_puint64;
typedef  __int64      img_int64,  * img_pint64;
#endif

typedef uintptr_t		img_uintptr_t;

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
#define IMG_MEMCMP(A,B,C)	memcmp	(A,B,C)
#define IMG_MEMMOVE(A,B,C)	memmove	(A,B,C)
//#define IMG_STRDUP(A)		strdup(A)
#define IMG_STRDUP(A)		_strdup(A)
#define IMG_STRCMP(A,B)		strcmp(A,B)
#define IMG_STRCPY(A,B)		strcpy(A,B)

/* 64bit value prefix in printf - platform dependant		*/
#define IMG_INT64PRFX "I64"

/* Take a 64 bit unsigned int, check it is suitable and cast it as 32bit unsigned int */
#define IMG_UINT64_TO_UINT32(ui64Check) (IMG_ASSERT(((ui64Check) >> 32) == 0), (IMG_UINT32)(ui64Check))
/* Needed for historical reasons */
#define IMG_UINT64_TO_UNIT32 IMG_UINT64_TO_UINT32

/* Large File Seek and Tell functions */
#if defined _MSC_VER
#if _MSC_VER < 1400
int __cdecl _fseeki64(FILE *, __int64, int);
__int64 __cdecl _ftelli64(FILE *);
#ifdef _WINDLL
#define NO_FSEEK64
#endif
#endif
#else
int __cdecl _fseeki64(FILE *, __int64, int);
__int64 __cdecl _ftelli64(FILE *);
#endif
#define IMG_FTELL64 _ftelli64
#define IMG_FSEEK64 _fseeki64


#if (defined EXIT_ON_ASSERT) || (defined MSVDX_VIDDEC)

  #ifdef EXIT_ON_ASSERT
    #define IMG_ASSERT(exp) (void)((exp) || (fprintf(stderr, "ERROR: Assert %s; File %s; Line %d\n", #exp, __FILE__, __LINE__), _flushall(), exit(-1), 0))
  #else
    extern IMG_BOOL bExitOnAssert;
    #define IMG_ASSERT(exp) assert((exp) || (bExitOnAssert ? (fprintf(stderr, "ERROR: Assert %s; File %s; Line %d\n", #exp, __FILE__, __LINE__), _flushall(), exit(-1), 0) : 0))
  #endif  // EXIT_ON_ASSERT

  #define IMG_ASSERT_LIVE  // Defined when IMG_ASSERT() does something.

#else  // not EXIT_ON_ASSERT or MSVDX_VIDDEC

  #if defined __RELEASE_RELEASE__ || defined NO_ASSERT
    #define IMG_ASSERT(A) (void)0
  #else  // (__RELEASE_DEBUG__ or __RELEASE_TEST__) and not NO_ASSERT
    #define IMG_ASSERT(A) assert(A)
    #define IMG_ASSERT_LIVE  // Defined when IMG_ASSERT() does something.
  #endif

#endif  // EXIT_ON_ASSERT || MSVDX_VIDDEC


/* Takes any two signed values - return 'IMG_TRUE' if they have the same polarity */
#define	IMG_SAMESIGN(A,B)	(((((img_int32)A)^((img_int32)B))&0x80000000)==0)

#if defined (NO_INLINE_FUNCS)
	#define	INLINE
	#define	FORCE_INLINE
#else
#if defined(_UITRON_)
	#define	INLINE
	#define	FORCE_INLINE
	#define INLINE_IS_PRAGMA
#else
#if defined (__cplusplus)
	#define INLINE					inline
	#define	FORCE_INLINE			inline
#else
	#define	INLINE					__inline
#if defined(UNDER_CE) || defined(UNDER_XP) || defined(UNDER_VISTA)
	#define	FORCE_INLINE			__forceinline
#else
	#define	FORCE_INLINE			static __inline
#endif
#endif
#endif
#endif

/* Use this in any file, or use attributes under GCC - see below */
#ifndef PVR_UNREFERENCED_PARAMETER
#define	PVR_UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

/* The best way to supress unused parameter warnings using GCC is to use a
 * variable attribute.  Place the unref__ between the type and name of an
 * unused parameter in a function parameter list, eg `int unref__ var'. This
 * should only be used in GCC build environments, for example, in files that
 * compile only on Linux. Other files should use UNREFERENCED_PARAMETER */
#ifdef __GNUC__
#define unref__ __attribute__ ((unused))
#else
#define unref__
#endif

/*
	Wide character definitions
*/
#if defined(UNICODE)
	#ifdef WINCE
		typedef WCHAR			TCHAR, *PTCHAR, *PTSTR;
	#else
		typedef unsigned short	TCHAR, *PTCHAR, *PTSTR;
	#endif
#else	/* #if defined(UNICODE) */
	typedef char				TCHAR, *PTCHAR, *PTSTR;
#endif	/* #if defined(UNICODE) */

#define IMG_CALLCONV __stdcall
#define IMG_INTERNAL
#define	IMG_EXPORT	__declspec(dllexport)

/* IMG_IMPORT is defined as IMG_EXPORT so that headers and implementations match.
	* Some compilers require the header to be declared IMPORT, while the implementation is declared EXPORT
	*/
#define	IMG_IMPORT	IMG_EXPORT

#define IMG_MALLOC(A)       malloc(A)
#define IMG_CALLOC(A, B)    calloc(A, B)
#define IMG_REALLOC(A, B)   realloc(A, B)
#define IMG_FREE(A)         free(A)
#define IMG_MEMALIGN(A)		_aligned_malloc(A, SYS_MMU_PAGE_SIZE)

#define IMG_ALIGN(byte)
#define IMG_NORETURN

#if defined (__cplusplus)
	#define INLINE					inline
#else
#if !defined(INLINE)
	#define	INLINE					__inline
#endif
#endif

#define __TBILogF	consolePrintf

#define IMG_NORETURN

#define IMG_CONST const

#if (__cplusplus)
}
#endif

#endif // IMG_DEFS_H


