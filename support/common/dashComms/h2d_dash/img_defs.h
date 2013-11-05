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

 @Version
	     1.0

******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log: img_defs.h,v $
 Revision 1.1  2009/02/16 15:43:09  michael.melbourne
 Initial version

 Revision 1.1  2007/12/11 15:36:41  michael.melbourne
 Initial versions for DVBH Remote API

 Revision 1.22  2007/06/28 13:35:03  thw
 Added 'IMG_MEMCMP' macro.

 Revision 1.21  2007/04/23 09:22:13  jmg
 Added IMG_NORETURN.

 Revision 1.20  2007/04/11 14:45:55  tsb
 Add IMG_NORETURN

 Revision 1.19  2007/01/31 15:20:04  tsb
 changed def of IMG_ASSERT() to write message to stderr

 Revision 1.18  2007/01/25 10:32:50  erf
 Merged changes from Eurasia files.

 Revision 1.17  2006/12/05 15:14:33  bal
 Added #includes of stdio.h and stdlib.h when EXIT_ON_ASSERT is defined

 Revision 1.16  2006/12/05 12:49:55  rml
 Fix EXIT_ON_ASSERT

 Revision 1.15  2006/12/05 12:23:50  rml
 Minor change to EXIT_ON_ASSERT

 Revision 1.14  2006/12/05 12:19:45  rml
 Add EXIT_ON_ASSERT

 Revision 1.13  2006/04/21 17:14:12  grc
 Changed TAL memory functions to use IMG_HANDLE rather than IMG_HVOID.
 Also changed all appropriate occurences of IMG_HVOID to IMG_HANDLE.

 Revision 1.12  2005/11/22 11:25:48  erf
 Removed last checkin.

 Revision 1.11  2005/11/21 18:12:47  erf
 Use "long long" for int64 definition.

 Revision 1.10  2005/11/03 11:30:07  ra
 Pickup on new img_def headers.
 Add tal support for Msvdx
 Further work on vec local ram buffers

 Revision 1.9  2005/10/20 08:30:17  rml
 Change platform

 Revision 1.8  2005/10/20 08:26:35  rml
 Slight re-oganisation

 Revision 1.7  2005/10/19 16:58:23  rml
 Cross link to get upper and lower type defs

 Revision 1.6  2005/07/05 12:57:58  jmg
 Added IMG_ASSERT_LIVE.

 Revision 1.5  2005/01/28 10:33:13  sjn
 Fixed problem with uint64 definition on WIN32 (isolated to VC++ 6, possibly?)

 Revision 1.4  2005/01/27 15:07:39  jmg
 Fixed img_uint64.

 Revision 1.3  2004/09/07 14:36:57  jmg
 Added img_int64 type

 Revision 1.2  2004/07/22 12:22:07  thw
 Added macro 'IMG_SAMESIGN'.

 Revision 1.1.1.1  2004/02/11 16:58:02  rml
 Directory restructuring

 Revision 1.12  2004/01/20 09:31:28  rml
 Put back error #defines

 Revision 1.11  2004/01/19 16:44:51  rml
 Remove un used return codes.

 Revision 1.10  2003/12/04 11:01:58  rml
 no message

 Revision 1.9  2003/12/03 16:13:31  rml
 Some additional types added

 Revision 1.8  2003/11/28 11:34:45  thw
 Readded the mysteriously vanished img_pint8

 Revision 1.7  2003/11/27 13:28:00  mxb
 Changes for Nucleus/Meos Builds

 Revision 1.6  2003/11/24 10:32:39  rml
 no message

 Revision 1.5  2003/10/17 15:13:10  hs
 added img_pint8

 Revision 1.4  2003/08/06 15:18:55  rml
 Add TAL_MemSpaceGetInterruptMask

 Revision 1.3  2003/08/05 13:57:47  rml
 MeOs Changes

 Revision 1.2  2003/07/21 09:39:01  hs
 Added IMG_MEMSET

 Revision 1.1  2003/07/01 16:03:25  hs
 Initial checkin of the new 'consumerav' CVS tree structure.

 This is major commit #1 of 3. There will be another major commit based
 upon the IMG Framework application and libraries, as well as another
 for scripts and tagging.

 -----> THIS IS NOT YET A USEABLE STRUCTURE <-----

 Revision 1.4  2003/06/24 10:36:29  hs
 Updated types to include types required by PDP.

 Revision 1.3  2003/06/16 12:31:43  rml
 Types added for BMA

 Revision 1.2  2003/06/12 15:08:47  rml
 First commit of CxImage, RegTool and bug fixes to the MWC APIs

 Revision 1.1.1.4  2003/06/04 14:37:23  rml
 no message


*****************************************************************************/


#if !defined (__IMG_DEFS_H__)
#define __IMG_DEFS_H__

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>

#if (__cplusplus)
extern "C" {
#endif

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
typedef unsigned __int64 img_uint64, * img_puint64;
typedef __int64          img_int64,  * img_pint64;
typedef unsigned long    img_uintptr_t;


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
#define IMG_MEMCMP(A,B,C)	memcmp	(A,B,C)

#ifndef EXIT_ON_ASSERT
#define IMG_ASSERT(exp) (void)( (exp) || (fprintf(stderr, "ERROR: Assert %s; File %s; Line %d\n", #exp, __FILE__, __LINE__), assert(exp), 0) )
//#define IMG_ASSERT(A)		assert	(A)
#else
#define IMG_ASSERT(exp) (void)( (exp) || (fprintf(stderr, "ERROR: Assert %s; File %s; Line %d\n", #exp, __FILE__, __LINE__), exit(-1), 0) )
#endif

#define IMG_ASSERT_LIVE  // Defined when IMG_ASSERT() does something.

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
typedef unsigned short		TCHAR, *PTCHAR, *PTSTR;
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

#define IMG_MALLOC(A)		malloc	(A)
#define IMG_FREE(A)			free	(A)

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

#if (__cplusplus)
}
#endif

#if !defined (__IMG_TYPES_H__)
#include "img_types.h"
#endif

#endif // IMG_DEFS_H


