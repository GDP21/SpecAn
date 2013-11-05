/*!
******************************************************************************
 @file   HPServer_api.h

 @brief  Host Port Interface, Server side

 @Author Imagination Technologies

 @date   28/09/2010

         <b>Copyright 2010 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to the third parties without the
         express written permission of Imagination Technologies
         Limited, Home Park Estate, Kings Langley, Hertfordshire,
         WD4 8LZ, U.K.

\n\n\n

******************************************************************************/

#ifndef __HPSERVER_API_H__
#define __HPSERVER_API_H__

#ifdef __cplusplus
extern "C" {
#endif


/*============================================================================
====	D E F I N E S
=============================================================================*/
#define HPSERVER_MAX_DATA_BYTES (0xFFFFFF)

/*============================================================================
====	E N U M S
=============================================================================*/

/*!
******************************************************************************
 This type defines the Host Port (client) function return codes.
******************************************************************************/
typedef enum
{
    HPSERVER_SUCCESS,						//!< Success
    HPSERVER_ERROR,							//!< Error

    HPSERVER_ILLEGAL_PARAM,					//!< Function call has illegal param(s)

    HPSERVER_CB_ALREADY_DEFINED,			//!< Callback function already defined
    HPSERVER_CB_NOT_DEFINED,				//!< Callback function not defined

	HPSERVER_READ_CANCEL,
	HPSERVER_READ_TIMEOUT,					//!< HPServer_ReadClient timeout

	HPSERVER_WRITE_TIMEOUT,					//!< HPServer_WriteToClient timeout

} HPServer_eResult;

/*!
******************************************************************************
 This type defines the D2H callback function return codes.
******************************************************************************/
typedef enum
{
    HPSERVER_CB_SUCCESS,					 //!< Success

    HPSERVER_CB_ERROR1,						 //!< TBD, etc

} HPServer_eCBResult;

/*============================================================================
====	T Y P E D E F S
=============================================================================*/


/*============================================================================
====	F U N C T I O N   P R O T O T Y P E S
=============================================================================*/

/*!
******************************************************************************

 @Function              HPServer_init

 @Description

 This function initialises the Host Port Interface driver.

 @Input     ui32pollingPeriod		: Hibernation time inbetween attempts to write
 									  to the client (in scheduler clock ticks).

 @Return    HPServer_eResult		: Either HPSERVER_SUCCESS or an HPServer_eResult error code

******************************************************************************/
HPServer_eResult HPServer_init (
    IMG_UINT32			ui32pollingPeriod
    );

/*!
******************************************************************************

 @Function              HPServer_ReadClient

 @Description

 This function reads from the client.

 @Input     pui8buffer				: The pointer to the buffer to store read.

 @Input     ui32bufferSizeInBytes	: The maximum size of the buffer.

 @Input     timeout					: Timeout (in scheduler clock ticks).
                                      KRN_INFWAIT for infinate wait, otherwise should be >0

 @Output    pui32numberOfBytesRead	: The actual number of bytes received.

 @Return    HPServer_eResult		: Either HPSERVER_SUCCESS or an HPServer_eResult error code

******************************************************************************/
HPServer_eResult HPServer_ReadClient (
    IMG_UINT8			*pui8buffer,
    IMG_UINT32 			ui32bufferSizeInBytes,
    IMG_UINT32			*pui32numberOfBytesRead,
    IMG_INT32			timeout
    );

/*!
******************************************************************************

 @Function              HPServer_WriteToClient

 @Description

 This function write to the client.

 @Input     pui8buffer				: The pointer to the buffer to store read.

 @Input     ui32bufferSizeInBytes	: The maximum size of the buffer.

 @Input     timeout					: Timeout (in scheduler clock ticks).
                                      KRN_INFWAIT for infinate wait, otherwise should be >0

 @Output    pui32numberBytesTransferred	: The actual number of bytes written.

 @Return    HPServer_eResult		: Either HPSERVER_SUCCESS or an HPServer_eResult error code

******************************************************************************/
HPServer_eResult HPServer_WriteToClient (
    IMG_UINT8			*pui8buffer,
    IMG_UINT32 			ui32bufferSizeInBytes,
    IMG_UINT32			*pui32numberBytesTransferred,
    IMG_INT32			timeout
    );

/*============================================================================
	E N D
=============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* __HPSERVER_API_H__   */
