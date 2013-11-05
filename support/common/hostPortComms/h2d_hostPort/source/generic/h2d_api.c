/*!
******************************************************************************
 @file   : h2d_api.c

 @brief  MobileTV Host-to-Device API

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

/*============================================================================
====	I N C L U D E S
=============================================================================*/

#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include <MeOS.h>
#include <sys_util.h>

#include "dacomms_api.h"
#include "h2d_api.h"
#include "HPServer_api.h"

/*============================================================================
====	D E F I N E S
=============================================================================*/

// logging
#define VERBOSE_LOGGING
#ifdef VERBOSE_LOGGING
#define VerboseLog printf
#else
void dummyLog() {}
#define VerboseLog dummyLog
#endif

#define H2D_CALLBACK_QUEUE_SIZE			(20)
#define H2D_COMM_STACK_SIZE		2048

#define HP_POLLING_PERIOD		50
#define DEINIT_SLEEP_TIME 2000
#define NUM_OF_BUFFER_INFO_DESCRIPTORS (DACOMMS_NUM_HOST_ASYNC_QUEUES * DACOMMS_NUM_GROUPED_BUFFERS)

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
typedef struct H2D_tag_sContext
{
	IMG_BOOL					bInitialised;				// API initialised ?
	H2D_pfnEventCallback		pfnEventCallback;			// Application callback for async D2H transfers
	KRN_SEMAPHORE_T				hDisabledEvent;
	IMG_BOOL					bSendingCmd;
	IMG_BOOL					bDisabled;
} H2D_sContext;

typedef struct H2D_tag_sCmdRspContext
{
	IMG_UINT8				*	pui8CmdBuffer;
	IMG_UINT32					ui32CmdNumBytes;
	IMG_UINT8				*	pui8RspBuffer;
	IMG_UINT32					ui32RspMaxBytes;
	IMG_UINT32					ui32RspNumBytes;
	IMG_UINT32					ui32CmdBytesLeft;
	IMG_UINT32					ui32RspBytesLeft;
	H2D_eResult					eResult;
	IMG_BOOL					bTimedout;
	KRN_SEMAPHORE_T				sSemaphore;
	IMG_BOOL					bDisable;
} H2D_sCmdRspContext;

typedef struct H2D_tag_sAsyncContext
{
	IMG_UINT32					ui32NumBytes;
	//IMG_UINT32					ui32NumBytesLeft;
	H2D_eBuffType				eBuffType;
	H2D_eAsyncEvent				eAsyncEvent;
	IMG_UINT32					ui32CurrentRecv;
	IMG_BOOL					bError;
	IMG_UINT32					ui32DataBufferNum;
	IMG_UINT32					ui32NumGroups;
} H2D_sAsyncContext;

/*!
******************************************************************************
 This structure stores information about a buffer provided by the application
 which is used for asynchronous trasnfers
******************************************************************************/
typedef struct H2D_tag_sBuffer
{
	KRN_POOLLINK;
	struct H2D_tag_sBuffer	*	pNextBuffer;				// Next buffer in list
    IMG_UINT8				*	pui8Buff;					// Pointer to application buffer
	IMG_UINT32					ui32NumBytes;				// Number of bytes in the buffer
	IMG_UINT32					ui32BuffMaxBytes;			// Maximum number of bytes which buffer can hold
} H2D_sBuffer, *H2D_psBuffer;

typedef struct H2D_tag_sCallbackQueue
{
	H2D_sBuffer			aasH2DCallbackQueue[ H2D_CALLBACK_QUEUE_SIZE ][ DACOMMS_NUM_GROUPED_BUFFERS ];
	IMG_UINT32			aui32Front[ DACOMMS_NUM_GROUPED_BUFFERS ];
	IMG_UINT32			aui32Back[ DACOMMS_NUM_GROUPED_BUFFERS ];
	IMG_UINT32			aui32NumElements[ DACOMMS_NUM_GROUPED_BUFFERS ];
} H2D_sCallbackQueue;

/*============================================================================
====	D A T A
=============================================================================*/
static KRN_TASK_T			g_sWriteTask;
static IMG_UINT32			g_aui32WriteStack[ H2D_COMM_STACK_SIZE ];

static KRN_TASK_T			g_sReadTask;
static IMG_UINT32			g_aui32ReadStack[ H2D_COMM_STACK_SIZE ];

static KRN_TASKQ_T			H2D_deinit_sleep_queue;

/* Context info */
static H2D_sContext g_sH2DContext;

/* Queue pointers */
static H2D_sBuffer *		g_aapsH2DDataBufferFront[ DACOMMS_NUM_HOST_ASYNC_QUEUES ][ DACOMMS_NUM_GROUPED_BUFFERS ];
static H2D_sBuffer *		g_aapsH2DDataBufferBack[ DACOMMS_NUM_HOST_ASYNC_QUEUES ][ DACOMMS_NUM_GROUPED_BUFFERS ];

static H2D_sBuffer *		g_apsH2DInfoBufferFront[ DACOMMS_NUM_GROUPED_BUFFERS ];
static H2D_sBuffer *		g_apsH2DInfoBufferBack[ DACOMMS_NUM_GROUPED_BUFFERS ];

static H2D_sCallbackQueue	g_sCallbackQueue;

static H2D_sCmdRspContext	g_sH2DCmdRspContext;
static H2D_sAsyncContext	g_sH2DAsyncContext;

static H2D_sBuffer			g_asBufferDesc[NUM_OF_BUFFER_INFO_DESCRIPTORS];
static KRN_POOL_T			H2D_asBuffInfoPool;

static KRN_SEMAPHORE_T				g_sReadSemaphore;
static KRN_SEMAPHORE_T				g_sWriteSemaphore;

IMG_UINT8 g_aui8CommandBufferStart[DACOMMS_CMD_DATA_SIZE + DACOMMS_CMD_PAYLOAD_START];
IMG_UINT8 g_aui8RspAsyncBufferStart[DACOMMS_RSP_ASYNC_DATA_SIZE + DACOMMS_RSP_ASYNC_PAYLOAD_START];

/*============================================================================
====	F U N C T I O N    P R O T O T Y P E S
=============================================================================*/

// Queuing functions
IMG_VOID h2d_QueueRemove(	H2D_eBuffType	eBuffType,
							IMG_UINT32		ui32DataBufferNum,
							IMG_UINT32		ui32GroupIndex );
H2D_sBuffer * h2d_QueueFront(	H2D_eBuffType	eBuffType,
								IMG_UINT32		ui32DataBufferNum,
								IMG_UINT32		ui32GroupIndex );
H2D_eResult h2d_QueueAdd( 	const H2D_sAsyncBufferDesc	*	psBufferDesc );

// Callback queue functions
IMG_VOID		h2d_CallbackQueueReset();
IMG_VOID		h2d_CallbackQueueAdd(	H2D_sBuffer	*	psBuffer,
										IMG_UINT32		ui32GroupIndex );
H2D_psBuffer	h2d_CallbackQueueFront( IMG_UINT32		ui32GroupIndex );
IMG_VOID		h2d_CallbackQueueRemove(IMG_UINT32		ui32GroupIndex );

static IMG_VOID h2d_readTask();
static IMG_VOID h2d_writeTask();

/*============================================================================
====	A P I    F U N C T I O N S
=============================================================================*/

/*!
******************************************************************************

 @Function              H2D_Initialise

******************************************************************************/

IMG_VOID H2D_Initialise( IMG_INT i32Priority )
{
	IMG_UINT32	i, j;

	// Check we're not already initialised
	assert( g_sH2DContext.bInitialised == IMG_FALSE );

	// Reset queue pointers
	for ( j = 0; j < DACOMMS_NUM_GROUPED_BUFFERS; ++j )
	{
		for ( i = 0; i < DACOMMS_NUM_HOST_ASYNC_QUEUES; ++i )
		{
			g_aapsH2DDataBufferFront[i][j] = IMG_NULL;
			g_aapsH2DDataBufferBack[i][j] = IMG_NULL;
		}
	}

	// Create the disabled event
//	g_sH2DContext.hDisabledEvent = CreateEvent( IMG_NULL, IMG_TRUE, IMG_FALSE, IMG_NULL );
//	KRN_initSemaphore( &g_sH2DContext.hDisabledEvent, 0 );

	g_sH2DContext.bSendingCmd = IMG_FALSE;
	g_sH2DContext.bInitialised = IMG_TRUE;
	g_sH2DAsyncContext.ui32NumBytes = 0;
	g_sH2DAsyncContext.ui32CurrentRecv = 0;
	g_sH2DAsyncContext.ui32NumGroups = 0;
	g_sH2DAsyncContext.bError = IMG_FALSE;
	KRN_initSemaphore( &g_sH2DCmdRspContext.sSemaphore, 0 );
	KRN_initSemaphore( &g_sWriteSemaphore, 0 );
	KRN_initSemaphore( &g_sReadSemaphore, 0 );

	KRN_initPool(&H2D_asBuffInfoPool, g_asBufferDesc, NUM_OF_BUFFER_INFO_DESCRIPTORS, sizeof(H2D_sBuffer));

	DQ_init(&H2D_deinit_sleep_queue);

	h2d_CallbackQueueReset();

	HPServer_init ( HP_POLLING_PERIOD );

	// Start up the comms tasks.
	KRN_startTask(	(KRN_TASKFUNC_T *)h2d_writeTask,
					&g_sWriteTask,
					g_aui32WriteStack,
					H2D_COMM_STACK_SIZE,
					i32Priority,
					IMG_NULL,
					"WriteTask" );

	KRN_startTask(	(KRN_TASKFUNC_T *)h2d_readTask,
					&g_sReadTask,
					g_aui32ReadStack,
					H2D_COMM_STACK_SIZE,
					i32Priority,
					IMG_NULL,
					"ReadTask" );

	return;
}

/*!
******************************************************************************

 @Function              D2H_Deinitialise

******************************************************************************/

IMG_VOID H2D_Deinitialise(	IMG_VOID	)
{
	IMG_UINT32		i, j;

	H2D_sBuffer *psPrevBuffer, *psBuffer = IMG_NULL;

	assert( g_sH2DContext.bInitialised );

	// Make bInitialised false so that the polling thread will exit.
	g_sH2DContext.bInitialised = IMG_FALSE;

	// Wait for the blocking time before we close the handle in case its blocked
//	Sleep( 20 );
	KRN_hibernate(&H2D_deinit_sleep_queue, DEINIT_SLEEP_TIME);
	// Close the semaphore
//	CloseHandle( g_sH2DContext.hDisabledEvent );

	h2d_CallbackQueueReset();

	/* Clear the data queue */
	for ( j = 0; j < DACOMMS_NUM_GROUPED_BUFFERS; ++j )
	{
		for ( i = 0; i < DACOMMS_NUM_HOST_ASYNC_QUEUES; ++i )
		{
			psBuffer = g_aapsH2DDataBufferFront[i][j];
			while ( psBuffer )
			{
				psPrevBuffer = psBuffer;
				psBuffer = psBuffer->pNextBuffer;
				//IMG_FREE ( psPrevBuffer );
				KRN_returnPool( psPrevBuffer );
			}
		}
	}

	/* Clear the message queue */
	for ( i = 0; i < DACOMMS_NUM_GROUPED_BUFFERS; ++i )
	{
		psBuffer = g_apsH2DInfoBufferFront[i];
		while ( psBuffer )
		{
			psPrevBuffer = psBuffer;
			psBuffer = psBuffer->pNextBuffer;
			//IMG_FREE ( psPrevBuffer );
			KRN_returnPool( psPrevBuffer );
		}
	}

	return;
}

/*!
******************************************************************************

 @Function              H2D_Disable

******************************************************************************/
IMG_VOID H2D_Disable(	IMG_VOID	)
{
//	ResetEvent( g_sH2DContext.hDisabledEvent );
	// Signal the comms thread to wait
	g_sH2DContext.bDisabled = IMG_TRUE;
	// Wait for the comms thread to get into its disabled state
//	WaitForSingleObject( g_sH2DContext.hDisabledEvent, INFINITE );
}

/*!
******************************************************************************

 @Function              H2D_Enable

******************************************************************************/
IMG_VOID H2D_Enable(	IMG_VOID	)
{
	g_sH2DContext.bDisabled = IMG_FALSE;
}

/*!
******************************************************************************

 @Function              H2D_SendCmdGetRsp

******************************************************************************/

H2D_eResult H2D_SendCmdGetRsp	(	IMG_UINT8	*	pui8CmdBuff,
									IMG_UINT32		ui32CmdNumBytes,
									IMG_UINT8	*	pui8RspBuff,
									IMG_UINT32		ui32RspBuffMaxBytes,
									IMG_UINT32		ui32Timeout,
									IMG_UINT32	*	pui32RspNumBytes
								)
{
	assert( g_sH2DContext.bInitialised );
	// Check if this has mistakenly been called from another thread
	assert( g_sH2DContext.bSendingCmd == IMG_FALSE );

	*pui32RspNumBytes = 0;

	// Check the cmd size is less than the maximum
	if ( ui32CmdNumBytes > DACOMMS_CMD_DATA_SIZE )
	{
		return H2D_ILLEGAL_PARAM;
	}

	g_sH2DCmdRspContext.pui8CmdBuffer = pui8CmdBuff;
	g_sH2DCmdRspContext.ui32CmdNumBytes = ui32CmdNumBytes;
	g_sH2DCmdRspContext.ui32CmdBytesLeft = g_sH2DCmdRspContext.ui32CmdNumBytes;
	g_sH2DCmdRspContext.pui8RspBuffer = pui8RspBuff;
	g_sH2DCmdRspContext.ui32RspNumBytes = 0;
	g_sH2DCmdRspContext.ui32RspMaxBytes = ui32RspBuffMaxBytes;
	g_sH2DCmdRspContext.bTimedout = IMG_FALSE;
	g_sH2DContext.bSendingCmd = IMG_TRUE;
	g_sH2DCmdRspContext.bDisable = IMG_FALSE;

	// Wait for the transaction to complete
	if ( ui32Timeout == 0 )
		ui32Timeout = KRN_INFWAIT;

	//start the transfer
	KRN_setSemaphore( &g_sWriteSemaphore, 1 );
	KRN_setSemaphore( &g_sH2DCmdRspContext.sSemaphore, 0 );

	//and wait for it to complete
	KRN_testSemaphore(&g_sH2DCmdRspContext.sSemaphore, 1, KRN_INFWAIT);

	g_sH2DContext.bSendingCmd = IMG_FALSE;
/*	if ( dwResult == WAIT_TIMEOUT )
	{
		// Timeout occurred, cancel the command
		g_sH2DCmdRspContext.bTimedout = IMG_TRUE;
		return H2D_TIMEOUT;
	}*/

	if ( g_sH2DCmdRspContext.eResult != H2D_SUCCESS )
	{
		return g_sH2DCmdRspContext.eResult;
	}

	*pui32RspNumBytes = g_sH2DCmdRspContext.ui32RspNumBytes;

	return H2D_SUCCESS;
}

/*!
******************************************************************************

 @Function              H2D_SendCmdDisable

******************************************************************************/
H2D_eResult H2D_SendCmdDisable	(	IMG_UINT8	*	pui8CmdBuff,
									IMG_UINT32		ui32CmdNumBytes,
									IMG_UINT32		ui32Timeout
								)
{
	assert( g_sH2DContext.bInitialised );
	// Check if this has mistakenly been called from another thread
	assert( g_sH2DContext.bSendingCmd == IMG_FALSE );

	// Check the cmd size is less than the maximum
	if ( ui32CmdNumBytes > DACOMMS_CMD_DATA_SIZE )
	{
		return H2D_ILLEGAL_PARAM;
	}

	g_sH2DCmdRspContext.pui8CmdBuffer = pui8CmdBuff;
	g_sH2DCmdRspContext.ui32CmdNumBytes = ui32CmdNumBytes;
	g_sH2DCmdRspContext.ui32CmdBytesLeft = g_sH2DCmdRspContext.ui32CmdNumBytes;

	g_sH2DCmdRspContext.bTimedout = IMG_FALSE;
	g_sH2DContext.bSendingCmd = IMG_TRUE;
	g_sH2DCmdRspContext.bDisable = IMG_TRUE;

	// Wait for the transaction to complete
	if ( ui32Timeout == 0 )
		ui32Timeout = KRN_INFWAIT;

	//start the transfer
	KRN_setSemaphore( &g_sWriteSemaphore, 1 );
	KRN_setSemaphore( &g_sH2DCmdRspContext.sSemaphore, 0 );

	//and wait for it to complete
	KRN_testSemaphore(&g_sH2DCmdRspContext.sSemaphore, 1, KRN_INFWAIT);

	g_sH2DContext.bSendingCmd = IMG_FALSE;
/*	if ( dwResult == WAIT_TIMEOUT )
	{
		// Timeout occurred, cancel the command
		g_sH2DCmdRspContext.bTimedout = IMG_TRUE;
		return H2D_TIMEOUT;
	}*/

	if ( g_sH2DCmdRspContext.eResult != H2D_SUCCESS )
	{
		return g_sH2DCmdRspContext.eResult;
	}

	return H2D_SUCCESS;
}

/*!
******************************************************************************

 @Function              H2D_QueueBuff

******************************************************************************/
H2D_eResult H2D_QueueBuff	(	const H2D_sAsyncBufferDesc	*	psBufferDesc	)
{
	assert( g_sH2DContext.bInitialised );

	return h2d_QueueAdd( psBufferDesc );
}

/*!
******************************************************************************

 @Function              H2D_AddCallback

******************************************************************************/
H2D_eResult H2D_AddCallback	(	H2D_pfnEventCallback	pfnEventCallback )
{
	assert( g_sH2DContext.bInitialised );

	if ( g_sH2DContext.pfnEventCallback )
	{
		/* Error - already defined */
		assert( g_sH2DContext.pfnEventCallback == IMG_NULL );
		return H2D_CB_ALREADY_DEFINED;
	}
	else
	{
		g_sH2DContext.pfnEventCallback = pfnEventCallback;
	}

	return H2D_SUCCESS;
}

/*!
******************************************************************************

 @Function              H2D_RemoveCallback

******************************************************************************/
H2D_eResult H2D_RemoveCallback	(	IMG_VOID	)
{

	assert( g_sH2DContext.bInitialised );

	/* Actually defined ? */
	if ( g_sH2DContext.pfnEventCallback == IMG_NULL )
	{
		/* Error - not defined */
		assert( g_sH2DContext.pfnEventCallback != IMG_NULL );
		return H2D_CB_NOT_DEFINED;
	}
	else
	{
		g_sH2DContext.pfnEventCallback = IMG_NULL;
	}

	return H2D_SUCCESS;
}

/*============================================================================
	I N T E R N A L   F U N C T I O N S
=============================================================================*/

/*!
******************************************************************************

 @Function              h2d_PerformCallback

******************************************************************************/
IMG_VOID h2d_PerformCallback( H2D_eCBResult		*	peResult )
{
	H2D_sAsyncCallbackDesc	sCallbackDesc;
	H2D_sBuffer			*	psBuffer = IMG_NULL;
	IMG_UINT32				i;

	// Check if all the group elements have come through
	if ( (g_sH2DAsyncContext.ui32NumGroups == g_sH2DAsyncContext.ui32CurrentRecv + 1 ) &&
		!(g_sH2DAsyncContext.bError) )
	{
		// We're going to pull some buffers from the callback queue now
		sCallbackDesc.eEvent = g_sH2DAsyncContext.eAsyncEvent;
		sCallbackDesc.ui32DataBufferNum = g_sH2DAsyncContext.ui32DataBufferNum;

		// make sure all elements in the callback descriptor have been sensibly initialised
		for ( i = 0; i < DACOMMS_NUM_GROUPED_BUFFERS; i++ )
		{
				sCallbackDesc.apui8Buff[i] = IMG_NULL;
				sCallbackDesc.aui32BuffNumBytes[i] = 0;
		}

		for ( i = 0; i < g_sH2DAsyncContext.ui32NumGroups; i++ )
		{
			// Try get a buffer from the callback queue
			psBuffer = h2d_CallbackQueueFront( i );
			if ( !psBuffer )
			{
				// If there is no buffer, it means there was an overflow of the previous buffers, and we have one or more lone buffers,
				// so we need to get the partner buffer from the application queue and return it straitght to the application, empty.
				psBuffer = h2d_QueueFront( g_sH2DAsyncContext.eBuffType, g_sH2DAsyncContext.ui32DataBufferNum, i );
				if ( !psBuffer )
				{
					// If there is no partner buffer in the application queue, then things are really b0rked.
					assert( 0 );
				}
				sCallbackDesc.apui8Buff[i] = psBuffer->pui8Buff;
				sCallbackDesc.aui32BuffNumBytes[i] = 0;
				h2d_QueueRemove( g_sH2DAsyncContext.eBuffType, g_sH2DAsyncContext.ui32DataBufferNum, i );
			}
			else
			{
				sCallbackDesc.apui8Buff[i] = psBuffer->pui8Buff;
				sCallbackDesc.aui32BuffNumBytes[i] = psBuffer->ui32NumBytes;
				h2d_CallbackQueueRemove( i );
			}
		}
		*peResult = (*g_sH2DContext.pfnEventCallback)( &sCallbackDesc );
	}
}

/*!
******************************************************************************

 @Function              h2d_ReadData

******************************************************************************/
IMG_BOOL h2d_ReadData()
{
	unsigned int i;
	HPServer_eResult eResult;
	H2D_eCBResult			eCBResult;
	IMG_UINT8 *pui8Payload;
	IMG_UINT32 ui32numberOfBytesRead;
	DACOMMS_sHPRspAsyncMsg *psMessageHeader = (DACOMMS_sHPRspAsyncMsg *) g_aui8RspAsyncBufferStart;
	H2D_sBuffer			*	psBuffer = IMG_NULL;
	H2D_sAsyncCallbackDesc	sCallbackDesc;

	//read command (blocking call)
	eResult = HPServer_ReadClient(
							g_aui8RspAsyncBufferStart,
							DACOMMS_RSP_ASYNC_DATA_SIZE + DACOMMS_RSP_ASYNC_PAYLOAD_START,
							&ui32numberOfBytesRead,
							KRN_INFWAIT);

	//is it a response message or async message
	if (psMessageHeader->ui8MessageType == COMMS_MSG_RSP)
	{
		//it's a response

		if ( (eResult != HPSERVER_SUCCESS) ||
		     (psMessageHeader->ui32Length[0] != (ui32numberOfBytesRead - sizeof(DACOMMS_sHPRspAsyncMsg))) )
		{
			g_sH2DCmdRspContext.eResult = H2D_COMMS_ERROR;
			g_sH2DContext.bSendingCmd = IMG_FALSE;
			KRN_setSemaphore( &g_sH2DCmdRspContext.sSemaphore, 1 );
			return IMG_FALSE;
		}

		// Check if there is enough space in the user buffer
		if ( psMessageHeader->ui32Length[0] > g_sH2DCmdRspContext.ui32RspMaxBytes )
		{
			g_sH2DCmdRspContext.eResult = H2D_RSP_BUFF_OVERFLOW;
			g_sH2DContext.bSendingCmd = IMG_FALSE;
			KRN_setSemaphore( &g_sH2DCmdRspContext.sSemaphore, 1 );
			return IMG_FALSE;
		}

		// Copy the data to the user buffer
		pui8Payload = g_aui8RspAsyncBufferStart + DACOMMS_RSP_ASYNC_PAYLOAD_START;
		IMG_MEMCPY(	g_sH2DCmdRspContext.pui8RspBuffer, pui8Payload, psMessageHeader->ui32Length[0] );

		g_sH2DCmdRspContext.ui32RspNumBytes = psMessageHeader->ui32Length[0];
		g_sH2DCmdRspContext.ui32RspBytesLeft = 0;
		g_sH2DCmdRspContext.eResult = H2D_SUCCESS;

		// Signal that we have finished this command/response transaction
		KRN_setSemaphore( &g_sH2DCmdRspContext.sSemaphore, 1 );

		return IMG_TRUE;
	}
	else
	{
		//it's an aync msg
		g_sH2DAsyncContext.ui32NumGroups = 0;
		g_sH2DAsyncContext.ui32CurrentRecv = 0;
		g_sH2DAsyncContext.ui32DataBufferNum = 0;

		//for each group, see if there is data
		for ( i = 0; i < DACOMMS_NUM_GROUPED_BUFFERS; i++ )
		{
			if ( psMessageHeader->ui32Length[i] != 0 )
				g_sH2DAsyncContext.ui32NumGroups++;
		}

		//setup payload initially to be end of header
		pui8Payload = g_aui8RspAsyncBufferStart + DACOMMS_RSP_ASYNC_PAYLOAD_START;

		//for each group (of which there is data
		for ( i = 0; i < g_sH2DAsyncContext.ui32NumGroups; i++ )
		{
			if ( psMessageHeader->ui32Length[i] != 0 )
			{
				//if there is get a buffer from that group
				psBuffer = h2d_QueueFront( g_sH2DAsyncContext.eBuffType, g_sH2DAsyncContext.ui32DataBufferNum, i );
				if ( !psBuffer )
				{
					sCallbackDesc.eEvent = H2D_AE_BUFFER_UNAVAILABLE;
					// If there are no available, appropriate queues, ditch the data and generate async event
					eCBResult = (*g_sH2DContext.pfnEventCallback)( &sCallbackDesc );
					// Flag an error to read the rest of the data
					g_sH2DAsyncContext.bError = IMG_TRUE;
					break;
				}

				// check if there is enough space in the application buffer
				if ( psMessageHeader->ui32Length[i] > psBuffer->ui32BuffMaxBytes )
				{
					sCallbackDesc.eEvent = H2D_AE_BUFFER_UNAVAILABLE;
					// If there are no available, appropriate queues, ditch the data and generate async event
					eCBResult = (*g_sH2DContext.pfnEventCallback)( &sCallbackDesc );
					// Flag an error to read the rest of the data
					g_sH2DAsyncContext.bError = IMG_TRUE;
					break;
				}

				//copy data into it
				IMG_MEMCPY(	psBuffer->pui8Buff, pui8Payload, psMessageHeader->ui32Length[i] );
				pui8Payload += psMessageHeader->ui32Length[i];
				psBuffer->ui32NumBytes = psMessageHeader->ui32Length[i];

				// Add it to the callback queue
				h2d_CallbackQueueAdd( psBuffer, i );
				// Remove the buffer from the buffer queue
				h2d_QueueRemove( g_sH2DAsyncContext.eBuffType, g_sH2DAsyncContext.ui32DataBufferNum, i );
				// Do a callback if we're ready
				h2d_PerformCallback( &eCBResult );
				// Reset the buffer pointer to we can retrieve a new one in the next pass
				psBuffer = IMG_NULL;
				//inc counter
				g_sH2DAsyncContext.ui32CurrentRecv++;
			}
		}

		g_sH2DAsyncContext.ui32NumBytes = 0;
		g_sH2DAsyncContext.bError = IMG_FALSE;
	}

	return IMG_TRUE;
}

/*!
******************************************************************************

 @Function              h2d_SendCommandData

******************************************************************************/
IMG_VOID h2d_SendCommandData()
{
	HPServer_eResult eResult;
	IMG_UINT32 ui32numberBytesTransferred;
	IMG_UINT8				*	pui8Payload;
	IMG_UINT32					ui32NumBytes;
	DACOMMS_sHPCmdMsg *psMessageHeader = (DACOMMS_sHPCmdMsg *) g_aui8CommandBufferStart;

	ui32NumBytes = g_sH2DCmdRspContext.ui32CmdNumBytes;

	//check number of bytes is not bigger than the max size
	if ( ui32NumBytes > DACOMMS_CMD_DATA_SIZE )
	{
		g_sH2DCmdRspContext.eResult = H2D_COMMS_ERROR;
		KRN_setSemaphore( &g_sH2DCmdRspContext.sSemaphore, 1 );
		return;
	}

	psMessageHeader->ui32Length = ui32NumBytes;
	pui8Payload = g_aui8CommandBufferStart + DACOMMS_CMD_PAYLOAD_START;
	IMG_MEMCPY(	pui8Payload, g_sH2DCmdRspContext.pui8CmdBuffer, ui32NumBytes );

	eResult = HPServer_WriteToClient(
									g_aui8CommandBufferStart,
									ui32NumBytes + sizeof(DACOMMS_sHPCmdMsg),
									&ui32numberBytesTransferred,
									KRN_INFWAIT);

	g_sH2DCmdRspContext.ui32CmdBytesLeft = 0;

}

/*!
******************************************************************************

 @Function              h2d_readTask

******************************************************************************/
IMG_VOID h2d_readTask()
{
	while ( g_sH2DContext.bInitialised )
	{
		// Block here until we have something to do
		//KRN_testSemaphore( &g_sReadSemaphore, 1, KRN_INFWAIT );

		h2d_ReadData();
	}

	KRN_removeTask( IMG_NULL );
}

/*!
******************************************************************************

 @Function              h2d_writeTask

******************************************************************************/
IMG_VOID h2d_writeTask()
{
	while ( g_sH2DContext.bInitialised )
	{
		// Block here until we have something to do
		KRN_testSemaphore( &g_sWriteSemaphore, 1, KRN_INFWAIT );

		h2d_SendCommandData();
	}

	KRN_removeTask( IMG_NULL );
}

/*!
******************************************************************************

 @Function              h2d_CallbackQueueReset

******************************************************************************/
IMG_VOID h2d_CallbackQueueReset()
{
	IMG_UINT32	i;

	for ( i = 0; i < DACOMMS_NUM_GROUPED_BUFFERS; ++i )
	{
		g_sCallbackQueue.aui32NumElements[i] = 0;
		g_sCallbackQueue.aui32Front[i] = 0;
		g_sCallbackQueue.aui32Back[i] = 0;
	}
}

/*!
******************************************************************************

 @Function              h2d_CallbackQueueAdd

******************************************************************************/
IMG_VOID h2d_CallbackQueueAdd(	H2D_sBuffer	*	psBuffer,
								IMG_UINT32		ui32GroupIndex )
{
	assert( ui32GroupIndex < DACOMMS_NUM_GROUPED_BUFFERS );

	// Check if the queue is full
	assert( g_sCallbackQueue.aui32NumElements[ ui32GroupIndex ] < H2D_CALLBACK_QUEUE_SIZE );

	// Add the buffer to the queue
	g_sCallbackQueue.aasH2DCallbackQueue[ g_sCallbackQueue.aui32Back[ ui32GroupIndex ]++ ][ ui32GroupIndex ] = *psBuffer;
	// Check if we must move the index to the beginning of the circular queue
	if ( g_sCallbackQueue.aui32Back[ ui32GroupIndex ] >= H2D_CALLBACK_QUEUE_SIZE )
	{
		g_sCallbackQueue.aui32Back[ ui32GroupIndex ] = 0;
	}
	++g_sCallbackQueue.aui32NumElements[ ui32GroupIndex ];

#ifdef DEBUG_PRINT_CALLBACK_QUEUE
	printf("***INSERT***\n");
	h2d_CallbackQueuePrint();
#endif
}

/*!
******************************************************************************

 @Function              h2d_CallbackQueueFront

******************************************************************************/
H2D_psBuffer h2d_CallbackQueueFront( IMG_UINT32	ui32GroupIndex )
{
	assert( ui32GroupIndex < DACOMMS_NUM_GROUPED_BUFFERS );

	if ( g_sCallbackQueue.aui32NumElements[ ui32GroupIndex ] )
	{
		return &g_sCallbackQueue.aasH2DCallbackQueue[ g_sCallbackQueue.aui32Front[ ui32GroupIndex ] ][ ui32GroupIndex ];
	}
	else
	{
		return IMG_NULL;
	}
}

/*!
******************************************************************************

 @Function              h2d_CallbackQueueRemove

******************************************************************************/
IMG_VOID h2d_CallbackQueueRemove( IMG_UINT32	ui32GroupIndex )
{
	assert( ui32GroupIndex < DACOMMS_NUM_GROUPED_BUFFERS );

	if ( ++g_sCallbackQueue.aui32Front[ ui32GroupIndex ] >= H2D_CALLBACK_QUEUE_SIZE )
	{
		g_sCallbackQueue.aui32Front[ ui32GroupIndex] = 0;
	}
	--g_sCallbackQueue.aui32NumElements[ ui32GroupIndex ];

#ifdef DEBUG_PRINT_CALLBACK_QUEUE
	printf("***REMOVE***\n");
	h2d_CallbackQueuePrint();
#endif
}

/*!
******************************************************************************

 @Function              h2d_QueueAdd

******************************************************************************/
H2D_eResult h2d_QueueAdd(	const H2D_sAsyncBufferDesc	*	psBufferDesc	)
{
	H2D_psBuffer	*	ppsBufferFront, *ppsBufferBack;
	H2D_sBuffer *psBuffer;
	IMG_UINT32			i;

	for ( i = 0; i < DACOMMS_NUM_GROUPED_BUFFERS; ++i )
	{
		//if there is no buffer for this group, then don't add one
		if ( (psBufferDesc->apui8Buffer[i] == IMG_NULL) ||
		     (psBufferDesc->aui32BufferMaxBytes[i] == 0) )
		     continue;

		/* Allocate the queue element */
		//H2D_sBuffer *psBuffer = (H2D_sBuffer *)IMG_MALLOC( sizeof( H2D_sBuffer ) );
		psBuffer = (H2D_sBuffer *)KRN_takePool(&H2D_asBuffInfoPool, 0);
		if ( !psBuffer )
		{
			return H2D_BUFF_QUEUE_OVERFLOW;
		}
		// Copy the data
		psBuffer->pNextBuffer = IMG_NULL;
		psBuffer->pui8Buff = (IMG_UINT8 *)psBufferDesc->apui8Buffer[i];
		psBuffer->ui32BuffMaxBytes = psBufferDesc->aui32BufferMaxBytes[i];
		psBuffer->ui32NumBytes = 0;

		/* Set the queue pointers according to the buffer type */
		switch( psBufferDesc->eBuffType )
		{
			case H2D_BT_DATA:
			{
				ppsBufferFront = &(g_aapsH2DDataBufferFront[ psBufferDesc->ui32DataBufferNum ][i]);
				ppsBufferBack = &(g_aapsH2DDataBufferBack[ psBufferDesc->ui32DataBufferNum ][i]);
				break;
			}
			case H2D_BT_MSG:
			{
				ppsBufferFront = &(g_apsH2DInfoBufferFront[i]);
				ppsBufferBack = &(g_apsH2DInfoBufferBack[i]);
				break;
			}
			default:
			{
				assert( ( psBufferDesc->eBuffType == H2D_BT_DATA ) ||
							( psBufferDesc->eBuffType == H2D_BT_MSG ) );
				break;
			}
		}

		/* Check if theres already a queue and add the element to the back */
		if ( (*ppsBufferBack) != IMG_NULL )
		{
			(*ppsBufferBack)->pNextBuffer = psBuffer;
			*ppsBufferBack = psBuffer;
		}
		else
		{
			(*ppsBufferFront) = psBuffer;
			*ppsBufferBack = psBuffer;
		}
	}

	return H2D_SUCCESS;
}

/*!
******************************************************************************

 @Function              h2d_QueueRemove

******************************************************************************/
IMG_VOID h2d_QueueRemove( H2D_eBuffType		eBuffType,
						  IMG_UINT32		ui32DataBufferNum,
						  IMG_UINT32		ui32GroupIndex )
{
	H2D_sBuffer		*psBuffer;

	assert( ui32GroupIndex < DACOMMS_NUM_GROUPED_BUFFERS );
	switch ( eBuffType )
	{
		case H2D_BT_DATA:
		{
			assert( ui32DataBufferNum < DACOMMS_NUM_HOST_ASYNC_QUEUES );
			psBuffer = g_aapsH2DDataBufferFront[ ui32DataBufferNum ][ ui32GroupIndex ];
			if ( !psBuffer )
				return;
			g_aapsH2DDataBufferFront[ ui32DataBufferNum ][ ui32GroupIndex ] = g_aapsH2DDataBufferFront[ ui32DataBufferNum ][ ui32GroupIndex ]->pNextBuffer;
			// Check if the queue is empty to reset the back pointer
			if ( !g_aapsH2DDataBufferFront[ ui32DataBufferNum ][ ui32GroupIndex ] )
			{
				g_aapsH2DDataBufferBack[ ui32DataBufferNum ][ ui32GroupIndex ] = IMG_NULL;
			}

			break;
		}

		case H2D_BT_MSG:
		{
			psBuffer = g_apsH2DInfoBufferFront[ ui32GroupIndex ];
			if ( !psBuffer )
				return;
			g_apsH2DInfoBufferFront[ ui32GroupIndex ] = g_apsH2DInfoBufferFront[ ui32GroupIndex ]->pNextBuffer;
			// Check if the queue is empty to reset the back pointer
			if ( !g_apsH2DInfoBufferFront[ ui32GroupIndex ] )
			{
				g_apsH2DInfoBufferBack[ ui32GroupIndex ] = IMG_NULL;
			}
			break;
		}
	}

	/* Free up allocated memory */
	//IMG_FREE( psBuffer );
	KRN_returnPool( psBuffer );
	psBuffer = IMG_NULL;

	return;
}

/*!
******************************************************************************

 @Function              h2d_QueueFront

******************************************************************************/
H2D_sBuffer *h2d_QueueFront(H2D_eBuffType	eBuffType,
							IMG_UINT32		ui32DataBufferNum,
							IMG_UINT32		ui32GroupIndex )
{
	assert( ui32GroupIndex < DACOMMS_NUM_GROUPED_BUFFERS );
	switch ( eBuffType )
	{
		case H2D_BT_DATA:
		{
			assert( ui32DataBufferNum < DACOMMS_NUM_HOST_ASYNC_QUEUES );
			return g_aapsH2DDataBufferFront[ ui32DataBufferNum ][ ui32GroupIndex ];
		}
		case H2D_BT_MSG:
		{
			return g_apsH2DInfoBufferFront[ ui32GroupIndex ];
		}
	}

	assert( 0 );
	return IMG_NULL;
}

