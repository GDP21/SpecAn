/*!
******************************************************************************
 @file   : h2d_api.h

 @brief    MobileTV Host-to-Device API

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

#ifndef __H2D_API_H__
#define __H2D_API_H__

#include "img_defs.h"
#include "dacomms_api.h"

#ifdef __cplusplus
extern "C" {
#endif


/*============================================================================
====	D E F I N E S
=============================================================================*/


/*============================================================================
====	E N U M S
=============================================================================*/

/*!
******************************************************************************
 This type defines the H2D function return codes.
******************************************************************************/
typedef enum
{
    H2D_SUCCESS,						//!< Success

    H2D_ILLEGAL_PARAM,					//!< Function call has illegal param(s)

    H2D_TIMEOUT,						//!< Timeout has occurred

    H2D_RSP_BUFF_OVERFLOW,				//!< Response buffer overflow

    H2D_CB_ALREADY_DEFINED,				//!< Callback function already defined
    H2D_CB_NOT_DEFINED,					//!< Callback function not defined

    H2D_BUFF_QUEUE_OVERFLOW,			//!< Buffer queue overflow

	H2D_COMMS_ERROR,

#if DACOMMS_VARIANT == DACOMMS_DASH
	H2D_DASH_ERROR,						//!< Dash Error
#endif

	H2D_INVALID_MSG

} H2D_eResult;


/*!
******************************************************************************
 This type defines the H2D callback function return codes.
******************************************************************************/
typedef enum
{
    H2D_CB_SUCCESS,								 //!< Success
    H2D_CB_ERROR1,								 //!< TBD, etc
} H2D_eCBResult;


/*!
******************************************************************************
 This type defines the H2D callback function event codes.
******************************************************************************/
typedef enum
{
    H2D_AE_MSG,									 //!< Info/event/message
    H2D_AE_DATA,								 //!< Data
    H2D_AE_COMMS_ERROR,								 //!< Error has occurred
	H2D_AE_BUFFER_UNAVAILABLE

} H2D_eAsyncEvent;


/*!
******************************************************************************
 This type defines the H2D buffer type.
******************************************************************************/
typedef enum
{
    H2D_BT_MSG,									 //!< Info/event/message
    H2D_BT_DATA,								 //!< Data
} H2D_eBuffType;


/*============================================================================
====	T Y P E D E F S
=============================================================================*/

/*!
******************************************************************************
 This struct defines a group of buffers to be queued for receipt of async msgs
******************************************************************************/
typedef struct H2D_tag_sAsyncBufferDescriptor
{
	H2D_eBuffType				eBuffType;
	IMG_UINT32					ui32DataBufferNum;
	IMG_UINT8				*	apui8Buffer[ DACOMMS_NUM_GROUPED_BUFFERS ];
	IMG_UINT32					aui32BufferMaxBytes[ DACOMMS_NUM_GROUPED_BUFFERS ];
} H2D_sAsyncBufferDesc;


/*!
******************************************************************************
 This struct defines the type of async message received as well as buffers
 storing the data
******************************************************************************/
typedef struct H2D_tag_sAsyncCallbackDescriptor
{
	H2D_eAsyncEvent				eEvent;
	IMG_UINT32					ui32DataBufferNum;
	IMG_UINT8				*	apui8Buff[ DACOMMS_NUM_GROUPED_BUFFERS ];
	IMG_UINT32					aui32BuffNumBytes[ DACOMMS_NUM_GROUPED_BUFFERS ];
} H2D_sAsyncCallbackDesc;


/*!
******************************************************************************

 @Function              H2D_pfnEventCallback

 @Description

 This is the prototype for the asynchronous callback function.

 @Input    psAsyncDesc		: Structure holding received message type and buffer information

 @Return   H2D_eCBResult	: This function returns either H2D_CB_SUCCESS or an
							  H2D_eCBResult error code.

******************************************************************************/
typedef H2D_eCBResult ( * H2D_pfnEventCallback) ( const H2D_sAsyncCallbackDesc * psAsyncDesc );


/*============================================================================
====	F U N C T I O N   P R O T O T Y P E S
=============================================================================*/

/*!
******************************************************************************

 @Function              H2D_Initialise

 @Description

 This function initialises the host-to-device communications sub-system.

 @Input     None.

 @Return    None.

******************************************************************************/
extern IMG_VOID H2D_Initialise	(	IMG_INT		i32Priority	);


/*!
******************************************************************************

 @Function              H2D_Deinitialise

 @Description

 This function de-initialises the host-to-device communications sub-system.

 @Input     None.

 @Return    None.

******************************************************************************/
extern IMG_VOID H2D_Deinitialise	(	IMG_VOID	);

/*!
******************************************************************************

 @Function              H2D_Disable

 @Description

 This function disables the host communications sub-system and frees the SDIO bus
 for other applications to use.

 @Input     None.

 @Return    None.

******************************************************************************/
extern IMG_VOID H2D_Disable	(	IMG_VOID	);

/*!
******************************************************************************

 @Function              H2D_Enable

 @Description

 This function enables a disable comms sub-system

 @Input     None.

 @Return    None.

******************************************************************************/
extern IMG_VOID H2D_Enable	(	IMG_VOID	);


/*!
******************************************************************************

 @Function              H2D_SendCmdGetRsp

 @Description

 This function sends a command from the host to the device and obtains a response
 with an optional timeout.

 @Input     pui8CmdBuff				: The pointer to the command buffer.

 @Input     ui32CmdNumBytes			: The number of bytes in the command buffer.

 @Input     pui8RspBuff				: The pointer to the response buffer.

 @Input     ui32RspBuffMaxBytes		: The maximum size of the response buffer.

 @Input     ui32Timeout				: The command/response timeout value (in milliseconds,
									  or zero for infinite timeout period).

 @Output    pui32RspNumBytes		: The actual number of response bytes received (may be greater than
									  ui32RspBuffMaxBytes, but in this case the extra bytes will not have
									  been saved in the buffer and an error code - H2D_RSP_BUFF_OVERFLOW -
									  will be returned).

 @Return    H2D_eResult				: Either H2D_SUCCESS or an H2D_eResult error code

******************************************************************************/
extern H2D_eResult H2D_SendCmdGetRsp	(
											IMG_UINT8	*	pui8CmdBuff,
											IMG_UINT32		ui32CmdNumBytes,
											IMG_UINT8	*	pui8RspBuff,
											IMG_UINT32		ui32RspBuffMaxBytes,
											IMG_UINT32		ui32Timeout,
											IMG_UINT32	*	pui32RspNumBytes
										);

/*!
******************************************************************************

 @Function              H2D_SendCmdDisable

 @Description

 This function sends a command from the host to the device and disables the host communications.

 @Input     pui8CmdBuff				: The pointer to the command buffer.

 @Input     ui32CmdNumBytes			: The number of bytes in the command buffer.

 @Input     ui32Timeout				: The command/response timeout value (in milliseconds,
									  or zero for infinite timeout period).

 @Return    H2D_eResult				: Either H2D_SUCCESS or an H2D_eResult error code

******************************************************************************/
extern H2D_eResult H2D_SendCmdDisable	(
											IMG_UINT8	*	pui8CmdBuff,
											IMG_UINT32		ui32CmdNumBytes,
											IMG_UINT32		ui32Timeout
										);


/*!
******************************************************************************

 @Function              H2D_QueueBuff

 @Description

 This function queues a buffer for storing info/event/messages or data from the
 device.

 @Input     psBufferDesc			: A structure describing the groups of buffers to queue.

 @Return    H2D_eResult				: Either H2D_SUCCESS or an H2D_eResult error code

******************************************************************************/
extern H2D_eResult H2D_QueueBuff	(	const H2D_sAsyncBufferDesc	*	psBufferDesc	);


/*!
******************************************************************************

 @Function              H2D_AddCallback

 @Description

 This function adds an asynchronous callback.  The caller provides the address of a
 function that will be called when an asynchronous event occurs.  The callback is
 persistent and is not removed when the application callback function is invoked.

 Only one callback function may be defined at any given time.  The callback
 function is removed using H2D_RemoveCallback().

 @Input     pfnEventCallback    : A pointer to the application callback function

 @Return    H2D_eResult			: Either H2D_SUCCESS or an H2D_eResult error code

******************************************************************************/
extern H2D_eResult H2D_AddCallback	(	H2D_pfnEventCallback        pfnEventCallback	);


/*!
******************************************************************************

 @Function              H2D_RemoveCallback

 @Description

 This function removes the defined event callback.  If the callback is already
 in progress when this function is called then it will complete normally;
 otherwise, the callback will be cancelled and no user callback will be made.

 @Return    H2D_eResult			: Either H2D_SUCCESS or an H2D_eResult error code

******************************************************************************/
extern H2D_eResult H2D_RemoveCallback	(	IMG_VOID	);


/*============================================================================
	E N D
=============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* __H2D_API_H__   */

