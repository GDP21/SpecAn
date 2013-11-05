/*!
******************************************************************************
 @file   : d2h_api.c

 @brief  MobileTV Device-to-Host API

 @Author Imagination Technologies

 @date   17/05/2007

         <b>Copyright 2007 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to the third parties without the
         express written permission of Imagination Technologies
         Limited, Home Park Estate, Kings Langley, Hertfordshire,
         WD4 8LZ, U.K.

 \n<b>Description:</b>\n
        Contains an implementation of the Device to Host API.  This particular
        version uses SDIO-S for the command/response and data channels and
        for the interrupt mechanism.

 \n<b>Platform:</b>\n
         Platform Independent

 @Version
    -   1.0 1st Release

\n\n\n

******************************************************************************/

/*============================================================================
====	I N C L U D E S
=============================================================================*/

#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include <Meos.h>

//#include <system.h>				// For consumer_av build system
#include <hw.h>						// For ensigma build system
#include <sys_util.h>

#include "dacomms_api.h"
#include "d2h_api.h"

/*============================================================================
====	D E F I N E S
=============================================================================*/


#define D2H_COMM_STACK_SIZE		2048
/*============================================================================
====	E N U M S
=============================================================================*/


/*============================================================================
====	T Y P E D E F S
=============================================================================*/

/*!
******************************************************************************
 This structure is used to retain the api context
******************************************************************************/
typedef struct D2H_tag_sContext
{
	IMG_BOOL				    bInitialised;		/*!< IMG_TRUE if the sub-system is initialised	*/
	D2H_pfnEventCallback		pfnEventCallback;
} D2H_sContext;

typedef struct D2H_tag_sCmdContext
{
	IMG_UINT8				*	pui8Buffer;
	IMG_UINT32					ui32CmdNumBytes;
	IMG_UINT32					ui32CmdMaxBytes;
	IMG_UINT32					ui32CmdBytesLeft;
	D2H_eResult					eResult;
	KRN_SEMAPHORE_T				sSemaphore;
	IMG_BOOL					bRequested;
	IMG_BOOL					bBlocking;
} D2H_sCmdContext;

typedef struct D2H_tag_sRspContext
{
	IMG_UINT8				*	pui8Buffer;
	IMG_UINT32					ui32RspNumBytes;
	IMG_UINT32					ui32RspBytesLeft;
	D2H_eResult					eResult;
	KRN_SEMAPHORE_T				sSemaphore;
	IMG_BOOL					bRequested;
} D2H_sRspContext;

typedef struct D2H_tag_sAsyncContext
{
	D2H_sAsyncBufferDesc		sUserContext;
	IMG_UINT32					ui32NumGroups;
	IMG_UINT32					ui32CurrentSend;
	IMG_UINT32					aui32BufferNumBytesLeft[ DACOMMS_NUM_GROUPED_BUFFERS ];

	D2H_eResult					eResult;

	KRN_SEMAPHORE_T				sSemaphore;
	KRN_SEMAPHORE_T				sSemaphore2;
	IMG_BOOL					bRequested;
} D2H_sAsyncContext;

/*!
******************************************************************************
 This structure provides a context for asynchronous SDIOS reads/writes
******************************************************************************/
typedef struct D2H_tag_sCallBackContext
{
	IMG_UINT8		*	pui8Buffer;
	IMG_UINT32			ui32MaxBufferBytes;
	IMG_UINT32			ui32Status;
	IMG_UINT32			ui32OperationId;
} D2H_sCallbackContext;

/*============================================================================
====	D A T A
=============================================================================*/
static KRN_TASK_T			g_sCommTask;
static IMG_UINT32			g_aui32CommStack[ D2H_COMM_STACK_SIZE ];

/* Context info */
static D2H_sContext			g_sD2HContext = {
												IMG_FALSE,
												IMG_NULL
											};

static D2H_sCmdContext		g_sD2HCmdContext;
static D2H_sRspContext		g_sD2HRspContext;
static D2H_sAsyncContext	g_sD2HAsyncContext;

static DQ_T					g_taskQueue;                // queue for hibernating tasks

KRN_SEMAPHORE_T				g_sSemaphore;

__attribute__ ((__section__(".dacommsbuffers"))) IMG_UINT8			g_aui8CommandBufferStart[DACOMMS_CMD_DATA_SIZE + DACOMMS_CMD_PAYLOAD_START];
__attribute__ ((__section__(".dacommsbuffers"))) IMG_UINT8			g_aui8ResponseBufferStart[DACOMMS_RSP_DATA_SIZE + DACOMMS_RSP_PAYLOAD_START];
__attribute__ ((__section__(".dacommsbuffers"))) IMG_UINT8			g_aui8AsyncBufferStart[DACOMMS_ASYNC_DATA_SIZE + DACOMMS_ASYNC_PAYLOAD_START];

/*============================================================================
====	F U N C T I O N S
=============================================================================*/

IMG_VOID d2h_CommsTask();

/*!
******************************************************************************

 @Function              D2H_Initialise

******************************************************************************/
IMG_VOID D2H_Initialise	(	IMG_INT		i32Priority,
							IMG_UINT32	ui32DmaChannel	)
{
    (void)ui32DmaChannel;     /* Remove warnings about unused parameters */

	/* Check we're not already initialised */
	IMG_ASSERT( g_sD2HContext.bInitialised == IMG_FALSE );

	/* Initialise the task queue for hibernating tasks */
    DQ_init(&g_taskQueue);

	KRN_initSemaphore( &g_sD2HCmdContext.sSemaphore, 0 );
	KRN_initSemaphore( &g_sD2HRspContext.sSemaphore, 0 );
	KRN_initSemaphore( &g_sD2HAsyncContext.sSemaphore, 0 );
	KRN_initSemaphore( &g_sD2HAsyncContext.sSemaphore2, 1 );
	KRN_initSemaphore( &g_sSemaphore, 0 );

	g_sD2HContext.bInitialised = IMG_TRUE;

	// Start up the comm task. Run it at max priority
	KRN_startTask(	(KRN_TASKFUNC_T *)d2h_CommsTask,
					&g_sCommTask,
					g_aui32CommStack,
					D2H_COMM_STACK_SIZE,
					i32Priority,
					IMG_NULL,
					"CommTask" );

	return;
}


/*!
******************************************************************************

 @Function              D2H_Deinitialise

******************************************************************************/
IMG_VOID D2H_Deinitialise	(	IMG_VOID	)
{
	IMG_ASSERT( g_sD2HContext.bInitialised == IMG_TRUE );

	g_sD2HContext.bInitialised = IMG_FALSE;

	return;
}


/*!
******************************************************************************

 @Function              D2H_ReadCmdBlocking

******************************************************************************/
D2H_eResult D2H_ReadCmdBlocking	(	IMG_UINT8	*	pui8CmdBuff,
									IMG_UINT32		ui32CmdBuffMaxBytes,
									IMG_UINT32	*	pui32CmdNumBytes)
{
	IMG_ASSERT( g_sD2HContext.bInitialised == IMG_TRUE );
	// Check if we're already busy with another ReadCmdNonBlocking
	IMG_ASSERT( g_sD2HCmdContext.bRequested == IMG_FALSE );

	g_sD2HCmdContext.pui8Buffer = pui8CmdBuff;
	g_sD2HCmdContext.ui32CmdMaxBytes = ui32CmdBuffMaxBytes;
	g_sD2HCmdContext.ui32CmdNumBytes = 0;
	g_sD2HCmdContext.bBlocking = IMG_TRUE;

	g_sD2HCmdContext.bRequested = IMG_TRUE;

	KRN_setSemaphore( &g_sSemaphore, 1 );

	KRN_setSemaphore( &g_sD2HCmdContext.sSemaphore, 0 );

	// Wait for a command to be read
	KRN_testSemaphore( &g_sD2HCmdContext.sSemaphore, 1, KRN_INFWAIT );

	// Check the status of the transaction
	if ( g_sD2HCmdContext.eResult != D2H_SUCCESS )
	{
		*pui32CmdNumBytes = 0;
		return g_sD2HCmdContext.eResult;
	}

	*pui32CmdNumBytes = g_sD2HCmdContext.ui32CmdNumBytes;

	return D2H_SUCCESS;
}


/*!
******************************************************************************

 @Function              D2H_ReadCmdNonBlocking

******************************************************************************/
D2H_eResult D2H_ReadCmdNonBlocking	(	IMG_UINT8	*	pui8CmdBuff,
										IMG_UINT32		ui32CmdBuffMaxBytes)
{
	IMG_ASSERT( g_sD2HContext.bInitialised == IMG_TRUE );
	// Check if we're already busy with another ReadCmdNonBlocking
	IMG_ASSERT( g_sD2HCmdContext.bRequested == IMG_FALSE );

	g_sD2HCmdContext.pui8Buffer = pui8CmdBuff;
	g_sD2HCmdContext.ui32CmdMaxBytes = ui32CmdBuffMaxBytes;
	g_sD2HCmdContext.ui32CmdNumBytes = 0;
	g_sD2HCmdContext.bBlocking = IMG_FALSE;
	g_sD2HCmdContext.bRequested = IMG_TRUE;

	KRN_setSemaphore( &g_sSemaphore, 1 );

	KRN_setSemaphore( &g_sD2HCmdContext.sSemaphore, 0 );

	// Return now
	return D2H_SUCCESS;
}


/*!
******************************************************************************

 @Function              D2H_SendRsp

******************************************************************************/

D2H_eResult D2H_SendRsp	(	IMG_UINT8	*	pui8RspBuff,
							IMG_UINT32		ui32RspNumBytes	)
{
	IMG_ASSERT( g_sD2HContext.bInitialised == IMG_TRUE );
	IMG_ASSERT( g_sD2HRspContext.bRequested == IMG_FALSE );

	g_sD2HRspContext.pui8Buffer = pui8RspBuff;
	g_sD2HRspContext.ui32RspNumBytes = ui32RspNumBytes;
	g_sD2HRspContext.ui32RspBytesLeft = ui32RspNumBytes;
	g_sD2HRspContext.bRequested = IMG_TRUE;

	KRN_setSemaphore( &g_sSemaphore, 1 );

	KRN_setSemaphore( &g_sD2HRspContext.sSemaphore, 0 );

	// Block here until its done
	KRN_testSemaphore( &g_sD2HRspContext.sSemaphore, 1, KRN_INFWAIT );

	g_sD2HRspContext.bRequested = IMG_FALSE;

	return g_sD2HRspContext.eResult;
}


/*!
******************************************************************************

 @Function              D2H_SendAsyncBuff

******************************************************************************/

D2H_eResult D2H_SendAsyncBuff	(	const D2H_sAsyncBufferDesc	*	psBufferDescriptor )
{
	img_uint32		i;

	IMG_ASSERT( g_sD2HContext.bInitialised == IMG_TRUE );
	IMG_ASSERT( !( ( psBufferDescriptor->eBuffType == D2H_BT_DATA )								&&
				   ( psBufferDescriptor->ui32DataBufferNum >= DACOMMS_NUM_HOST_ASYNC_QUEUES )		) );

	// Block here if we're doing this in another thread.
	KRN_testSemaphore( &g_sD2HAsyncContext.sSemaphore2, 1, KRN_INFWAIT );

	/*
	** If the user tries to send (proper) data over the dash, we'll silently ignore it
	*/
	if (psBufferDescriptor->eBuffType == D2H_BT_DATA)
	{
		KRN_setSemaphore( &g_sD2HAsyncContext.sSemaphore2, 1 );
		return D2H_SUCCESS;
	}

	// User context
	g_sD2HAsyncContext.sUserContext.eBuffType = psBufferDescriptor->eBuffType;
	g_sD2HAsyncContext.sUserContext.ui32DataBufferNum = psBufferDescriptor->ui32DataBufferNum;
	g_sD2HAsyncContext.ui32NumGroups = 0;
	for ( i = 0; i < DACOMMS_NUM_GROUPED_BUFFERS; i++ )
	{
		if ( psBufferDescriptor->apui8Buffer[i] &&
			 psBufferDescriptor->aui32BufferNumBytes[i] )
		{
			g_sD2HAsyncContext.sUserContext.apui8Buffer[i] = psBufferDescriptor->apui8Buffer[i];
			g_sD2HAsyncContext.sUserContext.aui32BufferNumBytes[i] = psBufferDescriptor->aui32BufferNumBytes[i];
			g_sD2HAsyncContext.aui32BufferNumBytesLeft[i] = g_sD2HAsyncContext.sUserContext.aui32BufferNumBytes[i];
			++g_sD2HAsyncContext.ui32NumGroups;
		}
		else
		{
			g_sD2HAsyncContext.sUserContext.apui8Buffer[i] = IMG_NULL;
			g_sD2HAsyncContext.sUserContext.aui32BufferNumBytes[i] = 0;
		}
	}

	// General context
	g_sD2HAsyncContext.ui32CurrentSend = 0;
	g_sD2HAsyncContext.bRequested = IMG_TRUE;

	KRN_setSemaphore( &g_sSemaphore, 1 );

	KRN_setSemaphore( &g_sD2HAsyncContext.sSemaphore, 0 );

	// Block here until its done
	KRN_testSemaphore( &g_sD2HAsyncContext.sSemaphore, 1, KRN_INFWAIT );

	g_sD2HAsyncContext.bRequested = IMG_FALSE;

	KRN_setSemaphore( &g_sD2HAsyncContext.sSemaphore2, 1 );

	return g_sD2HAsyncContext.eResult;
}


/*!
******************************************************************************

 @Function              D2H_AddCallback

******************************************************************************/
D2H_eResult D2H_AddCallback	( D2H_pfnEventCallback        pfnEventCallback )
{
	D2H_eResult				eResult = D2H_SUCCESS;

	IMG_ASSERT( g_sD2HContext.bInitialised );

	/* Already defined ? */
	if ( g_sD2HContext.pfnEventCallback )
	{
		/* Error - already defined */
		IMG_ASSERT( g_sD2HContext.pfnEventCallback == IMG_NULL );
		eResult = D2H_CB_ALREADY_DEFINED;
	}
	else
	{
		g_sD2HContext.pfnEventCallback = pfnEventCallback;
	}

	return ( eResult );
}


/*!
******************************************************************************

 @Function              D2H_RemoveCallback

******************************************************************************/
D2H_eResult D2H_RemoveCallback	(	IMG_VOID	)
{
	D2H_eResult				eResult = D2H_SUCCESS;
	IMG_ASSERT( g_sD2HContext.bInitialised );

	/* Already defined ? */
	if ( g_sD2HContext.pfnEventCallback == IMG_NULL)
	{
		/* Error - already defined */
		IMG_ASSERT( g_sD2HContext.pfnEventCallback != IMG_NULL );
		eResult = D2H_CB_NOT_DEFINED;
	}
	else
	{
		g_sD2HContext.pfnEventCallback = IMG_NULL;
	}

	return ( eResult );
}

/*============================================================================
	I N T E R N A L   F U N C T I O N S
=============================================================================*/

/*!
******************************************************************************
~
 @Function              d2h_ReadCommand

******************************************************************************/
IMG_VOID d2h_ReadCommand( IMG_UINT32	ui32NumBytes )
{
	DACOMMS_sDashCmdRspMsg	*	psDashCmdRspMsg = (DACOMMS_sDashCmdRspMsg *)g_aui8CommandBufferStart;
	IMG_UINT8				*	pui8Payload;

	// Check if there is a ReadCmd(Non)Blocking has been called. If there hasnt send back a NACK
	if ( g_sD2HCmdContext.ui32CmdNumBytes != ui32NumBytes )
	{
		// Check if there is enough space in the application buffer
		if ( ui32NumBytes > g_sD2HCmdContext.ui32CmdMaxBytes )
		{
			g_sD2HCmdContext.eResult = D2H_CMD_BUFF_OVERFLOW;
			g_sD2HCmdContext.bRequested = IMG_FALSE;

			if ( g_sD2HCmdContext.bBlocking == IMG_FALSE )
			{
				IMG_ASSERT( g_sD2HContext.pfnEventCallback != IMG_NULL );
				(*g_sD2HContext.pfnEventCallback)( D2H_AE_ERROR, IMG_NULL, 0 );
			}
			else
			{
				KRN_setSemaphore( &g_sD2HCmdContext.sSemaphore, 1 );
			}

			//done with the command buffer. allow the host to use it again
			psDashCmdRspMsg->ui8Status = DASH_BUFFER_EMPTY;
			return;
		}

		g_sD2HCmdContext.ui32CmdNumBytes = ui32NumBytes;
		g_sD2HCmdContext.ui32CmdBytesLeft = g_sD2HCmdContext.ui32CmdNumBytes;
	}

	// Copy the data to the user buffer
	pui8Payload = g_aui8CommandBufferStart + DACOMMS_CMD_PAYLOAD_START;
	IMG_MEMCPY(	g_sD2HCmdContext.pui8Buffer, pui8Payload, ui32NumBytes );

	g_sD2HCmdContext.ui32CmdBytesLeft = 0;
	g_sD2HCmdContext.bRequested = IMG_FALSE;
	g_sD2HCmdContext.eResult = D2H_SUCCESS;

	// Check if it is a non blocking read
	if ( g_sD2HCmdContext.bBlocking == IMG_FALSE )
	{
		// Check if there is an app callback
		IMG_ASSERT( g_sD2HContext.pfnEventCallback != IMG_NULL );
		(*g_sD2HContext.pfnEventCallback)( D2H_AE_CMD, g_sD2HCmdContext.pui8Buffer, g_sD2HCmdContext.ui32CmdNumBytes );
	}
	else
	{
		KRN_setSemaphore( &g_sD2HCmdContext.sSemaphore, 1 );
	}
	//done with the command buffer. allow the host to use it again
	psDashCmdRspMsg->ui8Status = DASH_BUFFER_EMPTY;
}

/*!
******************************************************************************

 @Function              d2h_SendResponse

******************************************************************************/
IMG_VOID d2h_SendRspData()
{
	DACOMMS_sDashCmdRspMsg	*	psDashCmdRspMsg = (DACOMMS_sDashCmdRspMsg *)g_aui8ResponseBufferStart;
	IMG_UINT8				*	pui8Payload;
	IMG_UINT32					ui32NumBytes;

	ui32NumBytes = g_sD2HRspContext.ui32RspNumBytes;

	//check number of bytes is not bigger than the max size
	if ( ui32NumBytes > DACOMMS_CMD_DATA_SIZE )
	{
		g_sD2HRspContext.eResult = D2H_COMMS_ERROR;
		KRN_setSemaphore( &g_sD2HRspContext.sSemaphore, 1 );
		return;
	}

	//check buffer is empty
	if ( psDashCmdRspMsg->ui8Status == DASH_BUFFER_FULL)
	{
		//buffer is still being used by the host, so we cannot send a new response yet
		// We're not finished yet
		KRN_setSemaphore( &g_sSemaphore, 1 );
		return;
	}

	//fill it
	psDashCmdRspMsg->ui32Length = ui32NumBytes;
	pui8Payload = g_aui8ResponseBufferStart + DACOMMS_RSP_PAYLOAD_START;
	IMG_MEMCPY( pui8Payload, g_sD2HRspContext.pui8Buffer, ui32NumBytes );

	g_sD2HRspContext.ui32RspBytesLeft = 0;
	g_sD2HRspContext.eResult = D2H_SUCCESS;
	g_sD2HRspContext.bRequested = IMG_FALSE;
	KRN_setSemaphore( &g_sD2HRspContext.sSemaphore, 1 );
	psDashCmdRspMsg->ui8Status = DASH_BUFFER_FULL;
}

/*!
******************************************************************************

 @Function              d2h_SendAsyncData

******************************************************************************/
IMG_VOID d2h_SendAsyncData()
{
	DACOMMS_sDashAsyncMsg	*	psDashAsyncMsg = (DACOMMS_sDashAsyncMsg *)g_aui8AsyncBufferStart;
	IMG_UINT8				*	pui8Payload;
	IMG_UINT32					ui32NumBytes;
	IMG_UINT32					ui32TotalNumBytes = 0;
	IMG_UINT32					i;

	//check number of bytes is not bigger than the max size
	for ( i = 0; i < DACOMMS_NUM_GROUPED_BUFFERS; i++ )
	{
		ui32TotalNumBytes += g_sD2HAsyncContext.sUserContext.aui32BufferNumBytes[i];
	}

	if ( ui32TotalNumBytes > DACOMMS_ASYNC_DATA_SIZE )
	{
		g_sD2HAsyncContext.eResult = D2H_COMMS_ERROR;
		KRN_setSemaphore( &g_sD2HAsyncContext.sSemaphore, 1 );
		return;
	}

	//check buffer is empty
	if ( psDashAsyncMsg->ui8Status == DASH_BUFFER_FULL)
	{
		//buffer is still being used by the host, so we cannot send a new message yet
		// We're not finished yet
		KRN_setSemaphore( &g_sSemaphore, 1 );
		return;
	}

	//fill it
	pui8Payload = g_aui8AsyncBufferStart + DACOMMS_ASYNC_PAYLOAD_START;
	for ( i = 0; i < DACOMMS_NUM_GROUPED_BUFFERS; i++ )
	{
		ui32NumBytes = g_sD2HAsyncContext.sUserContext.aui32BufferNumBytes[i];
		psDashAsyncMsg->ui32Length[i] = ui32NumBytes;
		IMG_MEMCPY(	pui8Payload, g_sD2HAsyncContext.sUserContext.apui8Buffer[i], ui32NumBytes );
		pui8Payload += ui32NumBytes;

		g_sD2HAsyncContext.aui32BufferNumBytesLeft[i] = 0;
	}

	g_sD2HAsyncContext.eResult = D2H_SUCCESS;
	KRN_setSemaphore( &g_sD2HAsyncContext.sSemaphore, 1 );
	psDashAsyncMsg->ui8Status = DASH_BUFFER_FULL;
}

/*!
******************************************************************************

 @Function              d2h_CommsTask

******************************************************************************/
IMG_VOID d2h_CommsTask()
{
	DACOMMS_sDashCmdRspMsg	*	psDashCmdRspMsg = (DACOMMS_sDashCmdRspMsg *)g_aui8CommandBufferStart;

	while ( g_sD2HContext.bInitialised )
	{
		// Block here until we have something to do
		KRN_testSemaphore( &g_sSemaphore, 1, KRN_INFWAIT );

		/*
		** Check if we have a command to respond to, or if we have a reponse
		** ready, or any async messages ready
		*/

		// check correct memory addr for new command
		if ( psDashCmdRspMsg->ui8Status == DASH_BUFFER_FULL)
		{
			//got a new command
			d2h_ReadCommand( psDashCmdRspMsg->ui32Length );
		}
		// we have an async ready
		else if ( g_sD2HAsyncContext.bRequested && g_sD2HAsyncContext.sUserContext.eBuffType == D2H_BT_MSG )
		{
			d2h_SendAsyncData();
		}
		// we have a response ready
		else if ( g_sD2HRspContext.bRequested )
		{
			d2h_SendRspData();
		}
		//
		else  //we have had D2H_ReadCmdBlocking called, but don't have a command yet
		{
			// Keep the comms task alive
			KRN_setSemaphore( &g_sSemaphore, 1 );
		}

		KRN_release();
	}

	KRN_removeTask( IMG_NULL );
}

/*============================================================================
	E N D
=============================================================================*/
