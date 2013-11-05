/*!
******************************************************************************
 @file   : img_error.h

 @brief

 @Author Ray Livesley

 @date   16/09/2008

         <b>Copyright 2008 by Imagination Technologies Limited.</b>\n
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
         This file contains the IMG error definitions.

 \n<b>Platform:</b>\n
         Platform Independent

******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log: img_error.h,v $

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 


*****************************************************************************/

#if !defined (__IMG_ERROR_H__)
#define __IMG_ERROR_H__

#if defined (__cplusplus)
extern "C" {
#endif

#define	IMG_SUCCESS								(0)		/* !< Success	*/
#define IMG_TIMEOUT								(1)		/* !< Timeout	*/
#define IMG_MALLOC_FAILED						(2)		/* !< MALLOC failed	*/
#define IMG_FATAL_ERROR							(3)		/* !< Unspecified fatal error	*/
#define IMG_ERROR_OUT_OF_MEMORY					(4)		/* !< Memory allocation failed	*/
#define IMG_ERROR_DEVICE_NOT_FOUND				(5)		/* !< Device is not found	*/
#define IMG_ERROR_DEVICE_UNAVAILABLE			(6)		/* !< Device is not available/in use	*/
#define IMG_ERROR_GENERIC_FAILURE				(7)		/* !< Generic/unspecified failure	*/
#define IMG_INTERRUPTED							(8)		/* !< Operation was interrupted - retry	*/
#define IMG_ERROR_INVALID_ID					(9)		/* !< Invalid id	*/
#define IMG_ERROR_SIGNATURE_INCORRECT			(10)	/* !< A signature value was found to be incorrect	*/
#define IMG_ERROR_INVALID_PARAMETERS			(11)	/* !< The provided parameters were inconsistent/incorrect	*/
#define IMG_ERROR_STORAGE_TYPE_EMPTY			(12)	/* !< A list/pool has run dry	*/
#define IMG_ERROR_STORAGE_TYPE_FULL				(13)	/* !< A list is full	*/
#define IMG_ERROR_ALREADY_COMPLETE				(14)	/* !< Something has already occurred which the code thinks has not	*/
#define IMG_ERROR_UNEXPECTED_STATE				(15)	/* !< A state machine is in an unexpected/illegal state	*/
#define IMG_ERROR_COULD_NOT_OBTAIN_RESOURCE		(16)	/* !< A required resource could not be created/locked	*/
#define IMG_ERROR_NOT_INITIALISED				(17)	/* !< An attempt to access a structure/resource was made before it was initialised	*/
#define	IMG_ERROR_ALREADY_INITIALISED			(18)	/* !< An attempt to initialise a structure/resource was made when it has already been initialised	*/
#define	IMG_ERROR_VALUE_OUT_OF_RANGE			(19)	/* !< A provided value exceeded stated bounds				*/
#define IMG_ERROR_CANCELLED						(20)	/* !< The operation has been cancelled */
#define	IMG_ERROR_MINIMUM_LIMIT_NOT_MET			(21)	/* !< A specified minimum has not been met */
#define IMG_ERROR_NOT_SUPPORTED					(22)	/* !< The requested feature or mode is not supported */
#define IMG_ERROR_IDLE							(23)	/* !< A device or process was idle */
#define IMG_ERROR_BUSY							(24)	/* !< A device or process was busy */
#define IMG_ERROR_DISABLED						(25)	/* !< The device or resource has been disabled */
#define IMG_ERROR_OPERATION_PROHIBITED			(26)	/* !< The requested operation is not permitted at this time */
#define IMG_ERROR_MMU_PAGE_DIRECTORY_FAULT		(27)	/* !< The entry read from the MMU page directory is invalid */
#define IMG_ERROR_MMU_PAGE_TABLE_FAULT			(28)	/* !< The entry read from an MMU page table is invalid */

#if defined (__cplusplus)
}
#endif

#endif /* __IMG_ERROR_H__  */
