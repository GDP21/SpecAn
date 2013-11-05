/*!
******************************************************************************
 @file   : d2h_api.h

 @brief  MobileTV Device-to-Host API


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
/*
******************************************************************************
 Modifications :- see CVS history

******************************************************************************/

#ifndef __D2D_API_H__
#define __D2H_API_H__

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
 This type defines the D2H function return codes.
******************************************************************************/
typedef enum
{
    D2H_SUCCESS,						//!< Success

    D2H_ILLEGAL_PARAM,					//!< Function call has illegal param(s)

    D2H_CB_ALREADY_DEFINED,				//!< Callback function already defined
    D2H_CB_NOT_DEFINED,					//!< Callback function not defined

	D2H_CMD_BUFF_OVERFLOW,
	D2H_COMMS_ERROR,
	D2H_INVALID_MSG,
} D2H_eResult;


/*!
******************************************************************************
 This type defines the D2H callback function return codes.
******************************************************************************/
typedef enum
{
    D2H_CB_SUCCESS,								 //!< Success

    D2H_CB_ERROR1,								 //!< error, etc

} D2H_eCBResult;


/*!
******************************************************************************
 This type defines the D2H callback function event codes.
******************************************************************************/
typedef enum
{
    D2H_AE_CMD,									 //!< Info/event/message
    D2H_AE_ERROR,								 //!< Error has occurred

} D2H_eAsyncEvent;


/*!
******************************************************************************
 This type defines the D2H buffer type.
******************************************************************************/
typedef enum
{
    D2H_BT_MSG,									 //!< Info/event/message
    D2H_BT_DATA,								 //!< Data

} D2H_eBuffType;


/*============================================================================
====	T Y P E D E F S
=============================================================================*/

typedef struct D2H_tag_sAsyncBufferDescriptor
{
	// Async info or async data
	D2H_eBuffType				eBuffType;
	// If async data, then the index of the host side queue which the buffers are intended for
	// If async info, this is ignored
	IMG_UINT32					ui32DataBufferNum;
	// An array of pointers which specify the buffers which are grouped together for a transfer.
	// If a group buffer is not being used, set to IMG_NULL
	IMG_UINT8				*	apui8Buffer[ DACOMMS_NUM_GROUPED_BUFFERS ];
	// An array of integers which specify the size of each group buffer to transfer.
	// If a group buffer is not being used, set to 0.
	IMG_UINT32					aui32BufferNumBytes[ DACOMMS_NUM_GROUPED_BUFFERS ];
} D2H_sAsyncBufferDesc;


/*!
******************************************************************************

 @Function              D2H_pfnEventCallback

 @Description

 This is the prototype for the asynchronous callback function.

 @Input    eEvent			: Indicates the event which has occurred.

 @Input    pui8Buff			: Pointer to message/data buffer.

 @Input    ui32BuffNumBytes	: Number of valid bytes in the message/data buffer.

 @Return   D2H_eCBResult	: This function returns either D2H_CB_SUCCESS or an
							  D2H_eCBResult error code.

******************************************************************************/
typedef D2H_eCBResult ( * D2H_pfnEventCallback) (
    D2H_eAsyncEvent				eEvent,
    img_uint8	*				pui8Buff,
    img_uint32					ui32BuffNumBytes
);


/*============================================================================
====	F U N C T I O N   P R O T O T Y P E S
=============================================================================*/

/*!
******************************************************************************

 @Function              D2H_Initialise

 @Description

 This function initialises the device-to-host communications sub-system.

 @Input     i32Priority		: The priority which the COMMS task runs at.

 @Input		ui32DmaChannel	: The SDIOS DMA channel to use.

 @Return    None.

******************************************************************************/
extern IMG_VOID D2H_Initialise	(	IMG_INT		i32Priority,
									IMG_UINT32	ui32DmaChannel	);


/*!
******************************************************************************

 @Function              D2H_Deinitialise

 @Description

 This function de-initialises the device-to-host communications sub-system.

 NOTE: Only used for test purposes.

 @Input     None.

 @Return    None.

******************************************************************************/
extern IMG_VOID D2H_Deinitialise	(	IMG_VOID	);


/*!
******************************************************************************

 @Function              D2H_ReadCmdBlocking

 @Description

 This function reads a command from the host.  This function is blocking and will
 not return until a command is received.

 @Input     pui8CmdBuff				: The pointer to the buffer to store the command.

 @Input     ui32CmdBuffMaxBytes		: The maximum size of the command buffer.

 @Output    pui32CmdNumBytes		: The actual number of command bytes received (may be greater than
									  ui32CmdBuffMaxBytes, but in this case the extra bytes will not have
									  been saved in the buffer and an error code - H2D_CMD_BUFF_OVERFLOW -
									  will be returned).

 @Return    D2H_eResult				: Either D2H_SUCCESS or an D2H_eResult error code

******************************************************************************/
extern D2H_eResult D2H_ReadCmdBlocking	(
											IMG_UINT8	*	pui8CmdBuff,
											IMG_UINT32		ui32CmdBuffMaxBytes,
											IMG_UINT32	*	pui32CmdNumBytes
										);


/*!
******************************************************************************

 @Function              D2H_ReadCmdNonBlocking

 @Description

 This function reads a command from the host.  This function is non-blocking and will
 return immediately.  A callback will be made when the requested command has been
 received (or an error occurs).

 @Input     pui8CmdBuff				: The pointer to the buffer to store the command.

 @Input     ui32CmdBuffMaxBytes		: The maximum size of the command buffer.

 @Return    D2H_eResult				: Either D2H_SUCCESS or an D2H_eResult error code

******************************************************************************/
extern D2H_eResult D2H_ReadCmdNonBlocking	(
												IMG_UINT8	*	pui8CmdBuff,
												IMG_UINT32		ui32CmdBuffMaxBytes
											);


/*!
******************************************************************************

 @Function              D2H_SendRsp

 @Description

 This function sends a response from the device to the host.

 @Input     pui8RspBuff				: The pointer to the response buffer.

 @Output    pui32RspNumBytes		: The number of response bytes to send.

 @Return    D2H_eResult				: Either D2H_SUCCESS or an D2H_eResult error code

******************************************************************************/
extern D2H_eResult D2H_SendRsp	(
											IMG_UINT8	*	pui8RspBuff,
											IMG_UINT32		ui32RspNumBytes
								);


/*!
******************************************************************************

 @Function              D2H_SendAsyncBuff

 @Description

 This function sends a group of one of more buffers to the host.

 @Input     psBufferDescriptor		: A structure describing the group of buffers to send.
									  See the description of D2H_sAsyncBufferDesc

 @Return    D2H_eResult				: Either D2H_SUCCESS or an D2H_eResult error code

******************************************************************************/
extern D2H_eResult D2H_SendAsyncBuff	(	const D2H_sAsyncBufferDesc	*	psBufferDescriptor );


/*!
******************************************************************************

 @Function              D2H_AddCallback

 @Description

 This function adds an asynchronous callback.  The caller provides the address of a
 function that will be called when an asynchronous event occurs.  The callback is
 persistent and is not removed when the application callback function is invoked.

 Only one callback function may be defined at any given time.  The callback
 function is removed using D2H_RemoveCallback().

 @Input     pfnEventCallback    : A pointer to the application callback function

 @Return    D2H_eResult			: Either D2H_SUCCESS or an D2H_eResult error code

******************************************************************************/
extern D2H_eResult D2H_AddCallback	(
										D2H_pfnEventCallback        pfnEventCallback
									);


/*!
******************************************************************************

 @Function              D2H_RemoveCallback

 @Description

 This function removes the defined event callback.  If the callback is already
 in progress when this function is called then it will complete normally;
 otherwise, the callback will be cancelled and no user callback will be made.

 @Return    D2H_eResult			: Either D2H_SUCCESS or an D2H_eResult error code

******************************************************************************/
extern D2H_eResult D2H_RemoveCallback	(	IMG_VOID	);


/*============================================================================
	E N D
=============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* __D2H_API_H__   */

