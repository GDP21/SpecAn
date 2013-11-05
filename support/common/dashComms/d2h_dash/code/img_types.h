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
	     MTX.

 @Version
	-	1.0	- Initial release.

******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log: img_types.h,v $
 Revision 1.1  2009/02/16 15:43:08  michael.melbourne
 Initial version

 Revision 1.1  2005/10/20 08:43:10  rml
 Copied and modifified slightly from Meta


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

typedef unsigned long long IMG_UINT64,	*IMG_PUINT64;
typedef long long         IMG_INT64,	*IMG_PINT64;
//typedef float			IMG_FLOAT,	*IMG_PFLOAT;    Don't allow float on for embedded systems

typedef unsigned char	IMG_BOOL,	*IMG_PBOOL;

typedef void			IMG_VOID,	*IMG_PVOID;

typedef void**			IMG_HVOID,	* IMG_PHVOID;

typedef IMG_INT32		IMG_RESULT;

typedef IMG_UINT32      IMG_UINTPTR_T;

typedef IMG_PVOID       IMG_HANDLE;

typedef IMG_UINT32      IMG_SIZE_T;

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
