/*!
******************************************************************************
 @file   : h2d_api.c

 @brief  MobileTV Host-to-Device API

 @Author Imagination Technologies

 @date   31/05/2007

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
        Contains an implementation of the Host to Device API.  This particular
        version uses SDIO for the command/response and data channels, and the
		interrupt mechanism.

 \n<b>Platform:</b>\n
         Platform Independent

 @Version
    -   1.0 1st Release

\n\n\n

******************************************************************************/
/*
******************************************************************************

/*============================================================================
====	I N C L U D E S
=============================================================================*/

// We are building for Windows XP
#define _WIN32_WINNT	(0x0501)

#include <memory.h>
#include <windows.h>
#include <setupapi.h>
#include <winioctl.h>
#include <strsafe.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <initguid.h>

#include "stdafx.h"
#include "CDash.h"
#include "h2d_dash.h"

#include "dacomms_api.h"
#include "h2d_api.h"

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
	HANDLE						hCommThread;
	HANDLE						hDisabledEvent;
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
	HANDLE						hSemaphore;
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
/* Context info */
static H2D_sContext g_sH2DContext =	{	IMG_FALSE,
										IMG_NULL,
										0,
										0,
										IMG_FALSE,
										IMG_FALSE
									};

/* Queue pointers */
static H2D_sBuffer *		g_aapsH2DDataBufferFront[ DACOMMS_NUM_HOST_ASYNC_QUEUES ][ DACOMMS_NUM_GROUPED_BUFFERS ];
static H2D_sBuffer *		g_aapsH2DDataBufferBack[ DACOMMS_NUM_HOST_ASYNC_QUEUES ][ DACOMMS_NUM_GROUPED_BUFFERS ];

static H2D_sBuffer *		g_apsH2DInfoBufferFront[ DACOMMS_NUM_GROUPED_BUFFERS ];
static H2D_sBuffer *		g_apsH2DInfoBufferBack[ DACOMMS_NUM_GROUPED_BUFFERS ];

static H2D_sCallbackQueue	g_sCallbackQueue;

static H2D_sCmdRspContext	g_sH2DCmdRspContext;
static H2D_sAsyncContext	g_sH2DAsyncContext;

static IMG_UINT32 ADDR_CmdBufferStart;
static IMG_UINT32 ADDR_RspBufferStart;
static IMG_UINT32 ADDR_AsyncBufferStart;

/*============================================================================
====	F U N C T I O N    P R O T O T Y P E S
=============================================================================*/

DWORD WINAPI h2d_CommThread( LPVOID lParam );

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


/*============================================================================
====	A P I    F U N C T I O N S
=============================================================================*/

/*!
******************************************************************************

 @Function              H2D_InitDashBuffers

******************************************************************************/

IMG_VOID H2D_InitDashBuffers(	IMG_UINT32 ui32CmdBufferStart,
								IMG_UINT32 ui32RspBufferStart,
								IMG_UINT32 ui32AsyncBufferStart
							)
{
	ADDR_CmdBufferStart = ui32CmdBufferStart;
	ADDR_RspBufferStart = ui32RspBufferStart;
	ADDR_AsyncBufferStart = ui32AsyncBufferStart;
}

/*!
******************************************************************************

 @Function              H2D_DashBoot

******************************************************************************/

H2D_eResult H2D_DashBoot(	IMG_CHAR elfFile[]	)
{
	int status = 0;

	/* Quick check we have comms with target! */
	VerboseLog("DACOMMS: Communicating with DASH... ");
	if (MDTV_SUPPORT_IsCommsWorking() == MDTV_ERROR_NO_COMMS)
	{
		VerboseLog("FAIL!\n");
		return H2D_DASH_ERROR;
	}
	else
		VerboseLog("PASS\n");

	/* Test the support functions for DASH control */
	VerboseLog("DACOMMS: Stopping target... ");
	if (MDTV_SUPPORT_Stop() == MDTV_ERROR_NO_COMMS)
	{
		VerboseLog("FAIL!\n");
		return H2D_DASH_ERROR;
	}
	VerboseLog("PASS\n");

	VerboseLog("DACOMMS: Testing if target running... ");
	if ((status = MDTV_SUPPORT_IsRunning()) == MDTV_ERROR_NO_COMMS)
	{
		VerboseLog("FAIL!\n");
		return H2D_DASH_ERROR;
	}
	if (status == 1)
		VerboseLog("target is running\n");
	else
		VerboseLog("target is NOT running\n");

	VerboseLog("DACOMMS: HW reset... ");
	if (MDTV_SUPPORT_HwReset() == MDTV_ERROR_NO_COMMS)
	{
		VerboseLog("FAIL!\n");
		return H2D_DASH_ERROR;
	}
	VerboseLog("PASS\n");

	VerboseLog("DACOMMS: SW reset... ");
	if (MDTV_SUPPORT_SwReset() == MDTV_ERROR_NO_COMMS)
	{
		VerboseLog("FAIL!\n");
		return H2D_DASH_ERROR;
	}
	VerboseLog("PASS\n");

	DashScriptInitialize();
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{	/* return dash/comms error if fail */
		DashScriptUninitialize();
        return H2D_DASH_ERROR;
	}
	try
	{
		BeforeInit();
		PreLoadConfig();	/* NB currently does nothing! */

		/* Download the hardcoded filename for the MTX binary elf */
		VerboseLog("DACOMMS: Loading program... \n");
		VerboseLog("          %s\n", elfFile);
		if ((status = DashScriptLoadProgramFile(elfFile,0)) != 1)
		{	/* return error */
			DashScriptUninitialize();
			VerboseLog("           FAIL!\n");
			return H2D_DASH_ERROR;
		}

		/* Post load setup & start target running */
		PostLoadConfig();	/* NB currently does nothing! */
		if ((status = dash.Run()) != 1)
		{	/* return error: 0 means it was alrady running */
			DashScriptUninitialize();
			VerboseLog("           FAIL!\n");
			return H2D_DASH_ERROR;
		}

		Sleep(500); /* ? not sure why we need to wait? */
		DashScriptUninitialize();
		VerboseLog("           PASS\n");

		/* Quick check we have comms with target! */
		VerboseLog("DACOMMS: Communicating with DASH... ");
		if (MDTV_SUPPORT_IsCommsWorking() == MDTV_ERROR_NO_COMMS)
		{
			VerboseLog("FAIL!\n");
			return H2D_DASH_ERROR;
		}
		VerboseLog("PASS\n");

		return H2D_SUCCESS;
	}

	/* exception case when no comms with dash/target */
	catch (COleDispatchException* e)
    {
		e->Delete();
		DashScriptUninitialize();
		VerboseLog("FAIL!\n");
        return H2D_DASH_ERROR;
    }
}


/*!
******************************************************************************

 @Function              H2D_Initialise

******************************************************************************/

IMG_VOID H2D_Initialise(	IMG_VOID	)
{
	IMG_UINT32	i, j;

#ifdef DACOMMS_EVENT_LOGGING
	QueryPerformanceFrequency( &g_ui64Freq );
#endif
	// Check we're not already initialised
	IMG_ASSERT( g_sH2DContext.bInitialised == IMG_FALSE );

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
	g_sH2DContext.hDisabledEvent = CreateEvent( IMG_NULL, IMG_TRUE, IMG_FALSE, IMG_NULL );

#ifdef DEBUG_PRINT_TO_FILE
	g_fLog = fopen("log.txt", "w");
#else
#if defined DEBUG_PRINT || defined DEBUG_PRINT_AARDVARK
	g_fLog = stdout;
#endif
#endif

	g_sH2DContext.bSendingCmd = IMG_FALSE;
	g_sH2DContext.bInitialised = IMG_TRUE;
	g_sH2DAsyncContext.ui32NumBytes = 0;
	g_sH2DAsyncContext.ui32CurrentRecv = 0;
	g_sH2DAsyncContext.bError = IMG_FALSE;
	g_sH2DCmdRspContext.hSemaphore = CreateSemaphore( NULL, 0, 1, NULL );
	IMG_ASSERT( g_sH2DCmdRspContext.hSemaphore );

	h2d_CallbackQueueReset();

	// Start the comms thread
	g_sH2DContext.hCommThread = CreateThread( NULL,
											0,
											h2d_CommThread,
											NULL,
											0,
											NULL );

	IMG_ASSERT( g_sH2DContext.hCommThread );

	SetThreadPriority( g_sH2DContext.hCommThread, THREAD_PRIORITY_ABOVE_NORMAL );

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

	IMG_ASSERT( g_sH2DContext.bInitialised );

	// Make bInitialised false so that the polling thread will exit.
	g_sH2DContext.bInitialised = IMG_FALSE;

	// Wait for the blocking time before we close the handle in case its blocked
	Sleep( 20 );
	// Close the comm thread
	CloseHandle( g_sH2DContext.hCommThread );
	// Close the semaphore
	CloseHandle( g_sH2DContext.hDisabledEvent );

	CloseHandle( g_sH2DCmdRspContext.hSemaphore );

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
				IMG_FREE ( psPrevBuffer );
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
			IMG_FREE ( psPrevBuffer );
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
	ResetEvent( g_sH2DContext.hDisabledEvent );
	// Signal the comms thread to wait
	g_sH2DContext.bDisabled = IMG_TRUE;
	// Wait for the comms thread to get into its disabled state
	WaitForSingleObject( g_sH2DContext.hDisabledEvent, INFINITE );
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
	DWORD		dwResult;

	IMG_ASSERT( g_sH2DContext.bInitialised );
	// Check if this has mistakenly been called from another thread
	IMG_ASSERT( g_sH2DContext.bSendingCmd == IMG_FALSE );

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
		ui32Timeout = INFINITE;

	dwResult = WaitForSingleObject( g_sH2DCmdRspContext.hSemaphore, ui32Timeout );

	g_sH2DContext.bSendingCmd = IMG_FALSE;
	if ( dwResult == WAIT_TIMEOUT )
	{
		// Timeout occurred, cancel the command
		g_sH2DCmdRspContext.bTimedout = IMG_TRUE;
		return H2D_TIMEOUT;
	}

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
	DWORD		dwResult;

	IMG_ASSERT( g_sH2DContext.bInitialised );
	// Check if this has mistakenly been called from another thread
	IMG_ASSERT( g_sH2DContext.bSendingCmd == IMG_FALSE );

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
		ui32Timeout = INFINITE;

	dwResult = WaitForSingleObject( g_sH2DCmdRspContext.hSemaphore, ui32Timeout );

	g_sH2DContext.bSendingCmd = IMG_FALSE;
	if ( dwResult == WAIT_TIMEOUT )
	{
		// Timeout occurred, cancel the command
		g_sH2DCmdRspContext.bTimedout = IMG_TRUE;
		return H2D_TIMEOUT;
	}

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
	IMG_ASSERT( g_sH2DContext.bInitialised );

	return h2d_QueueAdd( psBufferDesc );
}

/*!
******************************************************************************

 @Function              H2D_AddCallback

******************************************************************************/
H2D_eResult H2D_AddCallback	(	H2D_pfnEventCallback	pfnEventCallback )
{
	IMG_ASSERT( g_sH2DContext.bInitialised );

	if ( g_sH2DContext.pfnEventCallback )
	{
		/* Error - already defined */
		IMG_ASSERT( g_sH2DContext.pfnEventCallback == IMG_NULL );
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

	IMG_ASSERT( g_sH2DContext.bInitialised );

	/* Actually defined ? */
	if ( g_sH2DContext.pfnEventCallback == IMG_NULL )
	{
		/* Error - not defined */
		IMG_ASSERT( g_sH2DContext.pfnEventCallback != IMG_NULL );
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

 @Function              h2d_ReadRspData

******************************************************************************/
IMG_BOOL h2d_ReadRspData()
{
	DACOMMS_sDashCmdRspMsg	sDashCmdRspMsg;
	unsigned int i;

	//VerboseLog("DACOMMS: Reading response...\n");

	DashScriptInitialize();
	sDashCmdRspMsg.ui32Length = ReadLong(dash, ADDR_RspBufferStart + DACOMMS_RSP_LENGTH_START);
	DashScriptUninitialize();

	// Check if there is enough space in the application buffer
	if ( sDashCmdRspMsg.ui32Length > g_sH2DCmdRspContext.ui32RspMaxBytes )
	{
		//VerboseLog("DACOMMS ERROR: Length larger than available buffer!\n");
		g_sH2DContext.bSendingCmd = IMG_FALSE;
		g_sH2DCmdRspContext.eResult = H2D_COMMS_ERROR;
		ReleaseSemaphore( g_sH2DCmdRspContext.hSemaphore, 1, NULL );
		return IMG_FALSE;
	}

	// Copy the data to the user buffer
	DashScriptInitialize();
	for ( i = 0; i < sDashCmdRspMsg.ui32Length; i++ )
	{
		g_sH2DCmdRspContext.pui8RspBuffer[i] = ReadByte(dash, ADDR_RspBufferStart + DACOMMS_RSP_PAYLOAD_START + i);
	}
	DashScriptUninitialize();

	g_sH2DCmdRspContext.ui32RspBytesLeft = 0;
	g_sH2DCmdRspContext.ui32RspNumBytes = sDashCmdRspMsg.ui32Length;

	// Signal that we have finished this command/response transaction
	ReleaseSemaphore( g_sH2DCmdRspContext.hSemaphore, 1, NULL );

	//done with the response buffer. allow the host to use it again
	//VerboseLog("DACOMMS: Reading response done!\n");
	DashScriptInitialize();
	WriteByte(dash, ADDR_RspBufferStart, DASH_BUFFER_EMPTY);
	DashScriptUninitialize();

	return IMG_TRUE;
}

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
					IMG_ASSERT( 0 );
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

 @Function              h2d_ReadAsyncData

******************************************************************************/
IMG_BOOL h2d_ReadAsyncData()
{
	H2D_eCBResult			eCBResult;
	H2D_sBuffer			*	psBuffer = IMG_NULL;
	H2D_sAsyncCallbackDesc	sCallbackDesc;
	DACOMMS_sDashAsyncMsg	sDashAsyncMsg;
	unsigned int			i, j;
	IMG_UINT8			*	pui8TargetBuffer;
	unsigned int			ui32SourceBufferAddr = ADDR_AsyncBufferStart + DACOMMS_ASYNC_PAYLOAD_START;

	IMG_ASSERT( g_sH2DContext.pfnEventCallback != IMG_NULL );

	g_sH2DAsyncContext.ui32NumGroups = 0;
	g_sH2DAsyncContext.ui32CurrentRecv = 0;
	g_sH2DAsyncContext.eBuffType = H2D_BT_MSG;
	g_sH2DAsyncContext.eAsyncEvent = H2D_AE_MSG;
	g_sH2DAsyncContext.ui32DataBufferNum = 0;

	//for each group
	for ( i = 0; i < DACOMMS_NUM_GROUPED_BUFFERS; i++ )
	{
		//see if there is some data
		//VerboseLog("DACOMMS: Reading async, group %d length... ", i);
		DashScriptInitialize();
		sDashAsyncMsg.ui32Length[i] = ReadLong(dash, ADDR_AsyncBufferStart + DACOMMS_ASYNC_LENGTH_START + (4*i));
		DashScriptUninitialize();
		//VerboseLog("length = %d\n", sDashAsyncMsg.ui32Length[i]);

		if ( sDashAsyncMsg.ui32Length[i] != 0 )
			g_sH2DAsyncContext.ui32NumGroups++;
	}

	//for each group
	for ( i = 0; i < g_sH2DAsyncContext.ui32NumGroups; i++ )
	{
		if ( sDashAsyncMsg.ui32Length[i] != 0 )
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
			if ( sDashAsyncMsg.ui32Length[i] > psBuffer->ui32BuffMaxBytes )
			{
				sCallbackDesc.eEvent = H2D_AE_BUFFER_UNAVAILABLE;
				// If there are no available, appropriate queues, ditch the data and generate async event
				eCBResult = (*g_sH2DContext.pfnEventCallback)( &sCallbackDesc );
				// Flag an error to read the rest of the data
				g_sH2DAsyncContext.bError = IMG_TRUE;
				break;
			}

			//copy data into it
			pui8TargetBuffer = psBuffer->pui8Buff;
			DashScriptInitialize();
			for ( j = 0; j < sDashAsyncMsg.ui32Length[i]; j++ )
			{
				*pui8TargetBuffer = ReadByte(dash, ui32SourceBufferAddr);
				pui8TargetBuffer++;
				ui32SourceBufferAddr += 1;
			}
			DashScriptUninitialize();
			psBuffer->ui32NumBytes += sDashAsyncMsg.ui32Length[i];

			// Add it to the callback queue
			//VerboseLog("DACOMMS: adding to callback queue...\n");
			h2d_CallbackQueueAdd( psBuffer, i );
			// Remove the buffer from the buffer queue
			h2d_QueueRemove( g_sH2DAsyncContext.eBuffType, g_sH2DAsyncContext.ui32DataBufferNum, i );
			// Do a callback if we're ready
			//VerboseLog("DACOMMS: perform callback...\n");
			h2d_PerformCallback( &eCBResult );
			// Reset the buffer pointer to we can retrieve a new one in the next pass
			psBuffer = IMG_NULL;
			//inc counter
			g_sH2DAsyncContext.ui32CurrentRecv++;
		}
	}
	g_sH2DAsyncContext.ui32NumBytes = 0;
	g_sH2DAsyncContext.bError = IMG_FALSE;

	//done with the async buffer. allow the host to use it again
	//VerboseLog("DACOMMS: Reading async done!\n");
	DashScriptInitialize();
	WriteByte(dash, ADDR_AsyncBufferStart, DASH_BUFFER_EMPTY);
	DashScriptUninitialize();

	return IMG_TRUE;
}

/*!
******************************************************************************

 @Function              h2d_SendCommandData

******************************************************************************/
IMG_VOID h2d_SendCommandData()
{
	DACOMMS_sDashCmdRspMsg	sDashCmdRspMsg;
	unsigned int i;

	//VerboseLog("DACOMMS: Sending command...\n");

	//check buffer is empty
	DashScriptInitialize();
	sDashCmdRspMsg.ui8Status = (COMMS_eDashStatus) ReadByte(dash, ADDR_CmdBufferStart);
	DashScriptUninitialize();

	if ( sDashCmdRspMsg.ui8Status == DASH_BUFFER_FULL)
	{
		//buffer is still being used by the device, so we cannot send a new command yet
		return;
	}

	//Copy across the data
	DashScriptInitialize();
	WriteLong(dash, ADDR_CmdBufferStart + DACOMMS_CMD_LENGTH_START, g_sH2DCmdRspContext.ui32CmdNumBytes);
	for ( i = 0; i < g_sH2DCmdRspContext.ui32CmdNumBytes; i++ )
	{
		WriteByte(dash, ADDR_CmdBufferStart + DACOMMS_CMD_PAYLOAD_START + i, g_sH2DCmdRspContext.pui8CmdBuffer[i]);
	}
	DashScriptUninitialize();

	g_sH2DCmdRspContext.ui32CmdBytesLeft = 0;
	g_sH2DContext.bSendingCmd = IMG_FALSE;
	//VerboseLog("DACOMMS: Sending command done!\n");
	DashScriptInitialize();
	WriteByte(dash, ADDR_CmdBufferStart, DASH_BUFFER_FULL);
	DashScriptUninitialize();
}

/*!
******************************************************************************

 @Function              h2d_CommThread

******************************************************************************/
DWORD WINAPI h2d_CommThread( LPVOID lParam )
{
	DACOMMS_sDashCmdRspMsg	sDashCmdRspMsg;
	DACOMMS_sDashAsyncMsg	sDashAsyncMsg;

	while ( g_sH2DContext.bInitialised )
	{
		if ( g_sH2DContext.bDisabled )
		{
			SetEvent( g_sH2DContext.hDisabledEvent );
			Sleep( 100 );
			continue;
		}

		//Read the "response ready" memory addr
		//Read the "async ready" memory addr
		DashScriptInitialize();
		sDashCmdRspMsg.ui8Status = (COMMS_eDashStatus) (0xFF & ReadLong(dash, ADDR_RspBufferStart));
		sDashAsyncMsg.ui8Status = (COMMS_eDashStatus) (0xFF & ReadLong(dash, ADDR_AsyncBufferStart));
		DashScriptUninitialize();

		// Check if we have a command to send
		if ( g_sH2DContext.bSendingCmd )
		{
			h2d_SendCommandData();

			// Check if the host is going to be disabled
			if ( g_sH2DCmdRspContext.bDisable )
			{
//				ResetEvent( g_sH2DContext.hDisabledEvent );
				g_sH2DContext.bDisabled = IMG_TRUE;
				g_sH2DCmdRspContext.eResult = H2D_SUCCESS;
				ReleaseSemaphore( g_sH2DCmdRspContext.hSemaphore, 1, NULL );
			}
		}
		// we have a reposnse to get
		else if ( sDashCmdRspMsg.ui8Status == DASH_BUFFER_FULL )
		{
			h2d_ReadRspData();
		}
		// we have aa async to get
		else if ( sDashAsyncMsg.ui8Status == DASH_BUFFER_FULL )
		{
			h2d_ReadAsyncData();
		}
	}

	return 0;
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
	IMG_ASSERT( ui32GroupIndex < DACOMMS_NUM_GROUPED_BUFFERS );

	// Check if the queue is full
	IMG_ASSERT( g_sCallbackQueue.aui32NumElements[ ui32GroupIndex ] < H2D_CALLBACK_QUEUE_SIZE );

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
	IMG_ASSERT( ui32GroupIndex < DACOMMS_NUM_GROUPED_BUFFERS );

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
	IMG_ASSERT( ui32GroupIndex < DACOMMS_NUM_GROUPED_BUFFERS );

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
	IMG_UINT32			i;

	for ( i = 0; i < DACOMMS_NUM_GROUPED_BUFFERS; ++i )
	{
		/* Allocate the queue element */
		H2D_sBuffer *psBuffer = (H2D_sBuffer *)IMG_MALLOC( sizeof( H2D_sBuffer ) );
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
				IMG_ASSERT( ( psBufferDesc->eBuffType == H2D_BT_DATA ) ||
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

	IMG_ASSERT( ui32GroupIndex < DACOMMS_NUM_GROUPED_BUFFERS );
	switch ( eBuffType )
	{
		case H2D_BT_DATA:
		{
			IMG_ASSERT( ui32DataBufferNum < DACOMMS_NUM_HOST_ASYNC_QUEUES );
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
	IMG_FREE( psBuffer );
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
	IMG_ASSERT( ui32GroupIndex < DACOMMS_NUM_GROUPED_BUFFERS );
	switch ( eBuffType )
	{
		case H2D_BT_DATA:
		{
			IMG_ASSERT( ui32DataBufferNum < DACOMMS_NUM_HOST_ASYNC_QUEUES );
			return g_aapsH2DDataBufferFront[ ui32DataBufferNum ][ ui32GroupIndex ];
		}
		case H2D_BT_MSG:
		{
			return g_apsH2DInfoBufferFront[ ui32GroupIndex ];
		}
	}

	IMG_ASSERT( 0 );
	return IMG_NULL;
}

