/*!
*******************************************************************************
  file   diseqc_api.c

  brief  DiSEqC Master API

         This file defines the functions that make up the DiSEqC Master API.

  author Imagination Technologies

         <b>Copyright 2011 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to the third parties without the
         express written permission of Imagination Technologies
         Limited, Home Park Estate, Kings Langley, Hertfordshire,
         WD4 8LZ, U.K.

*******************************************************************************/

/*============================================================================*/
/*                          INCLUDE FILES                                     */
/*============================================================================*/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
** TBI_NO_INLINES is needed for use of TBI_CRITON() and TBI_CRITOFF()
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#define TBI_NO_INLINES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

/* MeOS */
#include <MeOS.h>

/* System */
#include <system.h>
#include <sys_util.h>
#include <ioblock_defs.h>
#include <sys_config.h>

/* DISEQC */
#include <diseqc_api.h>
#include "diseqc_drv.h"
#include "diseqc_reg_defs.h"

/*============================================================================*/
/*                          MACRO DEFINITIONS                                 */
/*============================================================================*/


/* 22khz pulse in Q12.20 mhz*/
#define	DISEQC_DEFAULT_FREQUENCY	0x5A1D


#define	DISEQC_API(API)																		\
	DISEQC_STATUS_T Ret;																	\
	if((Ret = DISEQCEnter(psPort)) == DISEQC_STATUS_OK)										\
	{																						\
		Ret = API;																			\
		DISEQCLeave(psPort);																\
	}																						\
	return Ret;


/* Code for starting a DiSEqC operation - take an IO block from the pool and set the status
   to DISEQC_STATUS_OK. Update the complete function + context and the sync mode of the operation */
#define	DISEQC_OP_START																			\
	QIO_IOPARS_T		IOParams;																\
	DISEQC_IO_BLOCK_T	*psIOB;																	\
	DISEQC_STATUS_T		Ret;																	\
	Ret = DISEQC_STATUS_OK;																		\
	psIOB = (DISEQC_IO_BLOCK_T *)KRN_takePool(&psPort->IOBlockPool,0);							\
	if(psIOB==IMG_NULL){return DISEQC_STATUS_ERR_IO_BLOCK;}										\
	psIOB->pfnCompleteCallback = pfnCompleteFunction;											\
	psIOB->pCompleteCallbackContext = pCompleteContext;											\
	psIOB->Sync = Sync;


/* Code for ending a DiSEqC operation - call QIO_qio and return DISEQC_STATUS_OK for
   asynchronous and use a semaphore to wait for the operation to complete + return the
   IOB to the pool for synchronous. */
#define	DISEQC_OP_END																			\
	IOParams.spare = psIOB;																		\
	if(Sync == DISEQC_SYNC_ASYNC)																\
	{																							\
		QIO_qio(&psPort->sDevice,&psIOB->iocb,&IOParams,IMG_NULL,DISEQCComplete,i32Timeout);	\
	}																							\
	else																						\
	{																							\
		KRN_initSemaphore(&psIOB->Sema,0);														\
																								\
		QIO_qio(&psPort->sDevice,&psIOB->iocb,&IOParams,IMG_NULL,DISEQCComplete,i32Timeout);	\
																								\
		KRN_testSemaphore(&psIOB->Sema,1,KRN_INFWAIT);											\
																								\
		Ret = (DISEQC_STATUS_T)psIOB->iocb.ioParameters.opcode;									\
																								\
		KRN_returnPool(psIOB);																	\
	}																							\
																								\
	return Ret;


/*============================================================================*/
/*                           DATA STRUCTURES                                  */
/*============================================================================*/


/*============================================================================*/
/*                             STATIC DATA                                    */
/*============================================================================*/


/*============================================================================*/
/*                     STATIC FUNCTION DECLARATIONS                           */
/*============================================================================*/


/*============================================================================*/
/*                               FUNCTIONS                                    */
/*============================================================================*/

static inline DISEQC_STATUS_T DISEQCEnter(DISEQC_PORT_T *psPort)
{
	if(psPort->State != DISEQC_PORT_STATE_INITIALISED)
	{
		IMG_ASSERT(0);
		return DISEQC_STATUS_ERR_UNINITIALISED;
	}
	KRN_testSemaphore(&psPort->Sema,1,KRN_INFWAIT);
	if(psPort->State != DISEQC_PORT_STATE_INITIALISED)
	{
		KRN_setSemaphore(&psPort->Sema,1);
		IMG_ASSERT(0);
		return DISEQC_STATUS_ERR_UNINITIALISED;
	}
	return DISEQC_STATUS_OK;
}

static inline IMG_VOID DISEQCLeave(DISEQC_PORT_T *psPort)
{
	KRN_setSemaphore(&psPort->Sema,1);
}

int DISEQCComplete(QIO_DEVICE_T *psDevice, QIO_IOCB_T *psIOCB, QIO_IOPARS_T *psIOParams, QIO_STATUS_T Status)
{
	/* QIO complete function. Status is in psIOParams->opcode. Bytes received in psIOParams->counter */

	DISEQC_IO_BLOCK_T	*psIOB;

	psIOB = (DISEQC_IO_BLOCK_T *)psIOParams->spare;

	switch(Status)
	{
	case QIO_CANCEL:
		psIOParams->opcode = DISEQC_STATUS_ERR_CANCEL;
		break;
	case QIO_TIMEOUT:
		psIOParams->opcode = DISEQC_STATUS_ERR_TIMEOUT;
		break;
	default:
		break;
	}

	if(psIOB->pfnCompleteCallback != IMG_NULL)
	{
		psIOB->pfnCompleteCallback(psIOB->pui8RecvBuf,psIOParams->counter,(DISEQC_STATUS_T)psIOParams->opcode,psIOB->pCompleteCallbackContext);
	}

	if(psIOB->Sync == DISEQC_SYNC_SYNC)
	{
		KRN_setSemaphore(&psIOB->Sema,1);
	}
	else
	{
		KRN_returnPool(psIOB);
	}

	return IMG_TRUE;
}

static DISEQC_STATUS_T _DISEQCInit(DISEQC_PORT_T *psPort, DISEQC_PORT_SETTINGS_T *psSettings, DISEQC_IO_BLOCK_T *pasIOBlocks, IMG_UINT32 ui32NumIOBlocks)
{
	IMG_UINT32	ui32BlockIndex;
	IMG_UINT32	ui32TargetFreq;
	IMG_UINT32	ui32DivideBy;
	IMG_UINT32	ui32ActualFreq;

	ui32BlockIndex = psSettings->ui32BlockIndex;

	/* Check params and update the global pointer for the block if it is not already set */
	if(ui32BlockIndex>=DISEQC_NUM_BLOCKS){IMG_ASSERT(0);return DISEQC_STATUS_ERR_INVALIDBLOCK;}

	if(ui32NumIOBlocks<1){IMG_ASSERT(0);return DISEQC_STATUS_ERR_NUM_IO_BLOCKS;}

	if((psSettings->pui8UnexpectedData!=IMG_NULL)&&(psSettings->ui8UnexpectedDataMaxSize==0))
	{
		IMG_ASSERT(0);
		return DISEQC_STATUS_ERR_UNEXPECTED_SIZE;
	}

	if(g_apsDISEQCBlock[ui32BlockIndex] == IMG_NULL)
	{
		g_apsDISEQCBlock[ui32BlockIndex] = &IMG_asDISEQCBlock[ui32BlockIndex];
	}

	/* Get target frequency from settings structure or use default if no override specified */
	if(psSettings->bOverrideFreq == IMG_TRUE){ui32TargetFreq = psSettings->ui32TargetFreq;}
	else{ui32TargetFreq = DISEQC_DEFAULT_FREQUENCY;}

	/* Timing field in config register is number of clock periods per half 22khz pulse, ie 2x frequency */
	ui32TargetFreq *= 2;

	/* Do this here as the function can take a length of time to execute and should not be executed under the interrupt context */
	SYS_getDividerValues(SYS_getSysUndeletedFreq_fp(),ui32TargetFreq,65535,&ui32DivideBy,&ui32ActualFreq);

	/* Used by QIO init function */
	psSettings->ui32TargetFreq = ui32DivideBy - 1;

	/* Initialise port context */
	psPort->psSettings = psSettings;
	psPort->pfnReadUnexpectedCallback = psSettings->pfnReadUnexpectedCallback;
	psPort->pui8UnexpectedData = psSettings->pui8UnexpectedData;
	psPort->ui8UnexpectedDataMaxSize = psSettings->ui8UnexpectedDataMaxSize;
	KRN_initPool(&psPort->IOBlockPool,pasIOBlocks,ui32NumIOBlocks,(IMG_BYTE *)(pasIOBlocks+1)-(IMG_BYTE *)pasIOBlocks);

	/* Used by QIO functions to locate the port structure */
	g_apsDISEQCBlock[ui32BlockIndex]->pvAPIContext = psPort;

	/* Initialise the port */
	QIO_init(&psPort->sDevice,"DiSEqC",psSettings->ui32BlockIndex,&DISEQC_Driver);

	/* Set to actual frequency */
	psSettings->ui32TargetFreq = ui32ActualFreq/2;

	QIO_enable(&psPort->sDevice);

	/* Only initialise the semaphore if port is not reinitialising to ensure synchronisation correctness */
	if(psPort->State == DISEQC_PORT_STATE_INITIALISING)
	{
		KRN_initSemaphore(&psPort->Sema,1);
	}

	psPort->State = DISEQC_PORT_STATE_INITIALISED;

	return DISEQC_STATUS_OK;
}


DISEQC_STATUS_T DISEQCInit(DISEQC_PORT_T *psPort, DISEQC_PORT_SETTINGS_T *psSettings, DISEQC_IO_BLOCK_T *pasIOBlocks, IMG_UINT32 ui32NumIOBlocks)
{
	KRN_IPL_T		IPL;
	DISEQC_STATUS_T Ret = DISEQC_STATUS_OK;

	/* Check and update the port state */
	IPL = KRN_raiseIPL();
	if(psPort->State == DISEQC_PORT_STATE_UNINITIALISED){psPort->State = DISEQC_PORT_STATE_INITIALISING;}
	else if(psPort->State == DISEQC_PORT_STATE_DEINITIALISED){psPort->State = DISEQC_PORT_STATE_REINITIALISING;}
	else{KRN_restoreIPL(IPL); return DISEQC_STATUS_ERR_INITIALISED;}
	KRN_restoreIPL(IPL);

	/* Initialise */
	Ret = _DISEQCInit(psPort,psSettings,pasIOBlocks,ui32NumIOBlocks);

	return Ret;
}


static DISEQC_STATUS_T _DISEQOperationNoParams(DISEQC_PORT_T *psPort, DISEQC_SYNC_T Sync, IMG_INT32 i32Timeout, DISEQC_OPERATION_COMPLETE_CALLBACK_T pfnCompleteFunction, IMG_PVOID pCompleteContext, IMG_UINT32 ui32Opcode)
{
	/* For operations that have no additional parameters beyond the opcode */

	DISEQC_OP_START;

	IOParams.opcode = ui32Opcode;

	DISEQC_OP_END;
}

DISEQC_STATUS_T DISEQCStartContinuousTone(DISEQC_PORT_T *psPort, DISEQC_SYNC_T Sync, IMG_INT32 i32Timeout, DISEQC_OPERATION_COMPLETE_CALLBACK_T pfnCompleteFunction, IMG_PVOID pCompleteContext)
{
	DISEQC_API(_DISEQOperationNoParams(psPort,Sync,i32Timeout,pfnCompleteFunction,pCompleteContext,DISEQC_OPCODE_CONTINUOUS_TONE));
}

DISEQC_STATUS_T DISEQCEndContinuousTone(DISEQC_PORT_T *psPort, DISEQC_SYNC_T Sync, IMG_INT32 i32Timeout, DISEQC_OPERATION_COMPLETE_CALLBACK_T pfnCompleteFunction, IMG_PVOID pCompleteContext)
{
	DISEQC_API(_DISEQOperationNoParams(psPort,Sync,i32Timeout,pfnCompleteFunction,pCompleteContext,DISEQC_OPCODE_END_CONTINUOUS_TONE));
}

DISEQC_STATUS_T DISEQCToneBurstA(DISEQC_PORT_T *psPort, DISEQC_SYNC_T Sync, IMG_INT32 i32Timeout, DISEQC_OPERATION_COMPLETE_CALLBACK_T pfnCompleteFunction, IMG_PVOID pCompleteContext)
{
	DISEQC_API(_DISEQOperationNoParams(psPort,Sync,i32Timeout,pfnCompleteFunction,pCompleteContext,DISEQC_OPCODE_TONE_BURST_A));
}

DISEQC_STATUS_T DISEQCToneBurstB(DISEQC_PORT_T *psPort, DISEQC_SYNC_T Sync, IMG_INT32 i32Timeout, DISEQC_OPERATION_COMPLETE_CALLBACK_T pfnCompleteFunction, IMG_PVOID pCompleteContext)
{
	DISEQC_API(_DISEQOperationNoParams(psPort,Sync,i32Timeout,pfnCompleteFunction,pCompleteContext,DISEQC_OPCODE_TONE_BURST_B));
}

static DISEQC_STATUS_T _DISEQCSendMessage(DISEQC_PORT_T *psPort, IMG_BYTE *pui8SendBuf, IMG_UINT8 ui8SendSize, IMG_BYTE *pui8RecvBuf, IMG_UINT8 ui8MaxRecvSize, IMG_UINT8 *pui8Received, DISEQC_SYNC_T Sync, IMG_INT32 i32Timeout, DISEQC_OPERATION_COMPLETE_CALLBACK_T pfnCompleteFunction, IMG_PVOID pCompleteContext)
{
	DISEQC_OP_START;

	psIOB->pui8RecvBuf = pui8RecvBuf;
	psIOB->ui8RecvBufSize = ui8MaxRecvSize;
	psIOB->pui8Received = pui8Received;

	IOParams.opcode = DISEQC_OPCODE_SEND_MESSAGE;
	IOParams.pointer = pui8SendBuf;
	IOParams.counter = ui8SendSize;

	DISEQC_OP_END;
}

DISEQC_STATUS_T DISEQCSendMessage(DISEQC_PORT_T *psPort, IMG_BYTE *pui8SendBuf, IMG_UINT8 ui8SendSize, IMG_BYTE *pui8RecvBuf, IMG_UINT8 ui8MaxRecvSize, IMG_UINT8 *pui8Received, DISEQC_SYNC_T Sync, IMG_INT32 i32Timeout, DISEQC_OPERATION_COMPLETE_CALLBACK_T pfnCompleteFunction, IMG_PVOID pCompleteContext)
{
	DISEQC_API(_DISEQCSendMessage(psPort,pui8SendBuf,ui8SendSize,pui8RecvBuf,ui8MaxRecvSize,pui8Received,Sync,i32Timeout,pfnCompleteFunction,pCompleteContext));
}

static DISEQC_STATUS_T _DISEQCCancelAll(DISEQC_PORT_T *psPort)
{
	QIO_cancelAll(&psPort->sDevice);

	return DISEQC_STATUS_OK;
}

DISEQC_STATUS_T DISEQCCancelAll(DISEQC_PORT_T *psPort)
{
	DISEQC_API(_DISEQCCancelAll(psPort));
}

static DISEQC_STATUS_T _DISEQCDeinit(DISEQC_PORT_T *psPort)
{
	QIO_IOPARS_T		IOParams;

	_DISEQCCancelAll(psPort);

	/* Synchronous DEINIT operation */
	IOParams.opcode = DISEQC_OPCODE_DEINIT;
	QIO_qioWait(&psPort->sDevice,&IOParams,KRN_INFWAIT);

	/* Deinitialise the QIO device */
	QIO_disable(&psPort->sDevice);
	QIO_unload(&psPort->sDevice);

	return DISEQC_STATUS_OK;
}

DISEQC_STATUS_T DISEQCDeinit(DISEQC_PORT_T *psPort)
{
	KRN_IPL_T		IPL;
	DISEQC_STATUS_T Ret;

	/* Check and update the port state */
	IPL = KRN_raiseIPL();
	if(psPort->State != DISEQC_PORT_STATE_INITIALISED){KRN_restoreIPL(IPL);return DISEQC_STATUS_ERR_UNINITIALISED;}
	psPort->State = DISEQC_PORT_STATE_DEINITIALISING;
	KRN_restoreIPL(IPL);

	/* Wait on any other API calls that may be executing */
	KRN_testSemaphore(&psPort->Sema,0,KRN_INFWAIT);
	Ret = _DISEQCDeinit(psPort);
	KRN_setSemaphore(&psPort->Sema,1);

	/* Update port state */
	if(Ret==DISEQC_STATUS_OK)
	{
		psPort->State = DISEQC_PORT_STATE_DEINITIALISED;
	}
	else
	{
		psPort->State = DISEQC_PORT_STATE_INITIALISED;
	}

	return Ret;
}
