/*!
******************************************************************************
 @file   : spis_test.c

 @brief

 @Author Imagination Technologies

 @date   10/07/2007

         <b>Copyright 2007 by Imagination Technologies Limited.</b>\n
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
         SPI Slave Test Application.

 <b>Platform:</b>\n
	     Platform Independent

 @Version
	     1.0

******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log: spis_test.c,v $

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 



*****************************************************************************/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES

/*============================================================================
====	I N C L U D E S
=============================================================================*/

#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <img_defs.h>

#include <MeOS.h>
#include <ioblock_defs.h>

#include <system.h>
#include <sys_config.h>
#include <sys_util.h>

#include <gdma_api.h>

#include "spis_api.h"

#include "spis_autotest.h"

/*============================================================================
====	D E F I N E S
=============================================================================*/\

/* Timer defines...*/
#define STACK_SIZE			(2048)

#define STACK_FILL			0xdeadbeef

#define POLL_WAIT			MILLISECOND_TO_TICK(10)

#define SPIS_TEST_STATUS_ASYNC_DEFAULT 	(SPIS_STATUS_WOULD_BLOCK + 1)
#define SPIS_NUM_IO_BLOCKS 				(4)
#define SPIS_TEST_NUM_Q_TRANSACTION 	(SPIS_NUM_IO_BLOCKS + 2)

#define	MAX_SIZE			(5120)


/*
** VERBOSE_LOGGING definition used to restrict logging output
*/
#ifdef VERBOSE_LOGGING
#define verboseLog __TBILogF
#else
void dummyLog() {}
#define verboseLog dummyLog
#endif

/*============================================================================
====	E N U M S
=============================================================================*/

enum SPIS_TEST_NUM
{
    TEST_ILLEGAL_DMA_CHANNELS = 1,	/* 1 */
    TEST_ILLEGAL_SPI_MODE,
    TEST_ILLEGAL_CS_ACTIVE_LEVEL,
    TEST_SUPPORT_READS,				/* 4 */
    TEST_SUPPORT_WRITES,
	TEST_ASYNC_TX_CALLBACK,
	TEST_ASYNC_TX_GET_RESULT,
	TEST_ASYNC_TX_IGNORE,
	TEST_ASYNC_RX_CALLBACK,
	TEST_ASYNC_RX_GET_RESULT,
	TEST_ASYNC_RX_IGNORE,
	TEST_QUEUE_ASYNC_TX,
	TEST_QUEUE_ASYNC_RX,
	TEST_QUEUE_ASYNC_TOO_MANY,
	TEST_QUEUE_MIX_RX_TX,
	TEST_TIMEOUT,
	TEST_TIMEOUT_GET_RESULT,
	TEST_CANCEL_QUEUE_ASYNC,
	TEST_CANCEL_ASYNC_THEN_QUEUE_NEW,
    TEST_NUM_MAX,
};

/*============================================================================
====	T Y P E D E F S
=============================================================================*/

typedef struct
{
	IMG_UINT32			ui32TestNum;
	IMG_UINT32  		ui32Size;
	IMG_UINT32			ui32NumJobs;
	IMG_UINT32			ui32DmaChannel;
	SPIS_MODE_T			eSPIMode;
	SPIS_SYNC_MODE_T	eSyncMode;
	IMG_UINT32			ui32CSLevel;
	IMG_INT				fRefWrite;
	IMG_INT				fRefRead;
	IMG_INT				ui32BlockNum;
} TEST_sTestDesc;

// asynchronous operations
typedef struct
{
   	IMG_UINT8	*	pui8Buffer;
   	unsigned long	ui32NumBytesToTransfer;
	unsigned long 	ui32Status;
	unsigned long 	ui32OperationId;
} TEST_sAsyncCtx;


/*============================================================================
====	D A T A
=============================================================================*/

/*
** In the hardware environments, argument passing is handled by declaration of a metag_argv
** array to which the arguments are written (via a script in either Codescape or DashScript).
** The main() argc and *argv[] variables are assigned by reference to this array.
*/

/* Parameter data strings (initialised to "@" to enable simple calculation of argc) */
char argv1[32] = { '@', '\0' };
char argv2[32] = { '@', '\0' };
char argv3[32] = { '@', '\0' };
char argv4[32] = { '@', '\0' };
char argv5[32] = { '@', '\0' };
char argv6[32] = { '@', '\0' };
char argv7[32] = { '@', '\0' };
char argv8[32] = { '@', '\0' };
char argv9[32] = { '@', '\0' };
char argv10[32] = { '@', '\0' };
char argv11[32] = { '@', '\0' };
char argv12[32] = { '@', '\0' };
char argv13[32] = { '@', '\0' };
char argv14[32] = { '@', '\0' };
char argv15[32] = { '@', '\0' };
char argv16[32] = { '@', '\0' };
char argv17[32] = { '@', '\0' };
char argv18[32] = { '@', '\0' };
char argv19[32] = { '@', '\0' };
char argv20[32] = { '@', '\0' };
char argv21[32] = { '@', '\0' };


/* Array of arguments - the first element is unused, the other elements point to the argvXX[]
** strings that are initialised by script */
char *metag_argv[] = { "dummy", argv1, argv2, argv3, argv4, argv5, argv6, argv7, argv8, argv9, argv10, argv11, argv12, argv13, argv14, argv15, argv16, argv17, argv18, argv19, argv20, argv21};

/* Argument count, updated by initialisation script. Useful for debug. */
int metag_argc = 0;

/* Background Task Control Block */
KRN_TASK_T		*		g_psBackgroundTask;

/* Timer Task Control Block */
KRN_TASK_T		*		g_psTimerTask;

/* MEOS Scheduler */
KRN_SCHEDULE_T			g_sMeOSScheduler;

/* Scheduler queues */
KRN_TASKQ_T				g_asSchedQueues[ MEOS_MAX_PRIORITY_LEVEL + 1 ];

/* Stack for the Timer Task */
IMG_UINT32				g_aui32TimerStack[TIM_STACK_SIZE];

/* Main Task Control Block */
KRN_TASK_T				g_sMainTask;

/* Stack for the Main Task */
IMG_UINT32				g_aui32TaskStack[STACK_SIZE];

/* QIO structures */
QIO_DEVENTRY_T			g_aVectoredDevTable[QIO_NO_VECTORS];
QIO_SYS_T				g_sQio;

DQ_T					g_sTaskQueue;

/*
** The TEST_sDesc provides a location for all test configuration data
** and should be modified for the particular peripheral under test.
*/

static TEST_sTestDesc 		g_sTestDescription;
static SPIS_PARAM_T			g_sInitParam;
static SPIS_PORT_T 			g_sSPISlave;

static SPIS_IO_BLOCK_T 		g_sIOBlocks[SPIS_NUM_IO_BLOCKS];

static SPIS_ASYNC_T			g_sAsync[SPIS_TEST_NUM_Q_TRANSACTION];
static TEST_sAsyncCtx		g_sAsyncCallbackCtx[SPIS_TEST_NUM_Q_TRANSACTION];
static TEST_sAsyncCtx	*	g_psAsyncGetCtx[SPIS_TEST_NUM_Q_TRANSACTION];


__attribute__ ((__section__ (".bulkbuffers"))) IMG_UINT8 g_aui8RefReadBuffer[MAX_SIZE];
__attribute__ ((__section__ (".bulkbuffers"))) IMG_UINT8 g_aui8RefWriteBuffer[MAX_SIZE];
__attribute__ ((__section__ (".bulkbuffers"))) IMG_UINT8 g_aui8ReadBuffer[MAX_SIZE];

IMG_BOOL	g_bWriteFile = IMG_FALSE;
IMG_BOOL	g_bReadFile  = IMG_FALSE;

IMG_BOOL	g_bAutomaticMode = IMG_FALSE;
img_uint32	g_ui32BlockNum = 0;

/******************************************************************************
**
**                       LOCAL FUNCTION PROTOTYPES
**
******************************************************************************/

static void CompleteFunc(void *pContext,
						 unsigned char *pui8Buffer,
                         unsigned long ui32NumBytesToTransfer,
						 unsigned long ui32Status);
static void ParseCommandLine(int argc, char *argv[]);
static void Usage(char *cmd);

/*!
******************************************************************************

 @Function				MainTask

******************************************************************************/
void MainTask( void )
{
	unsigned int i, j;
	unsigned int transactionCount = 0;
	long timeout = 10;

	/* Read messages from files */
	if ( g_bWriteFile == IMG_TRUE )
	{
		read(g_sTestDescription.fRefWrite, g_aui8RefWriteBuffer, g_sTestDescription.ui32Size);
	}
	if ( g_bReadFile == IMG_TRUE )
	{
		read(g_sTestDescription.fRefRead, g_aui8RefReadBuffer, g_sTestDescription.ui32Size);
	}

    // setup the fixed parameters for the spi slave
    g_sInitParam.spiMode          = g_sTestDescription.eSPIMode;
    g_sInitParam.csLevel          = g_sTestDescription.ui32CSLevel;
    g_sInitParam.dmaChannel		  = g_sTestDescription.ui32DmaChannel;
	g_sInitParam.spiSyncMode	  = g_sTestDescription.eSyncMode;
	g_sInitParam.ui32BlockIndex   = g_sTestDescription.ui32BlockNum;

	// Initialise g_sAsync
	for ( i = 0; i < SPIS_TEST_NUM_Q_TRANSACTION; i++ )
	{
		g_sAsyncCallbackCtx[i].ui32Status = SPIS_TEST_STATUS_ASYNC_DEFAULT;
	}

	switch ( g_sTestDescription.ui32TestNum )
	{
		case TEST_ILLEGAL_DMA_CHANNELS:
		{
			/*
			** Check rejection of illegal DMA channels
			*/
			g_sInitParam.dmaChannel = g_sTestDescription.ui32DmaChannel;
			if ( SPIS_INVALID_DMA_CHANNELS != SPISInit( &g_sSPISlave, &g_sInitParam, NULL, 0 ) )
			{
				__TBILogF("FAIL: rejection of illegal input DMA channel failed!\n");
				return;
			}
			__TBILogF("SUCCESS: rejection of illegal DMA channels!\n");
			break;
		}
		case TEST_ILLEGAL_SPI_MODE:
			/*
			** Check rejection of illegal SPI mode
			*/
			g_sInitParam.spiMode = SPI_MODE_3 + 1;
			if (SPIS_INVALID_SPI_MODE != SPISInit(&g_sSPISlave, &g_sInitParam, NULL, 0))
			{
				__TBILogF("FAIL: rejection of illegal SPI mode failed!\n");
				return;
			}
			__TBILogF("SUCCESS: rejection of illegal SPI mode!\n");
			break;

		case TEST_ILLEGAL_CS_ACTIVE_LEVEL:
			/*
			** Check rejection of illegal CS active level
			*/
			g_sInitParam.csLevel = 2;
			if (SPIS_INVALID_CS_ACTIVE_LEVEL != SPISInit(&g_sSPISlave, &g_sInitParam, NULL, 0))
			{
				__TBILogF("FAIL: rejection of illegal CS active level failed!\n");
				return;
			}
			__TBILogF("SUCCESS: rejection of illegal CS active level!\n");
			break;

		case TEST_SUPPORT_READS:
			/*
			** Check support for consecutive reads
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, NULL, 0))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}
			for (i=0; i < g_sTestDescription.ui32NumJobs; i++)
			{
				__TBILogF("Wait for master to send EXACTLY %d bytes!\n", g_sTestDescription.ui32Size);
				SPISRead(&g_sSPISlave, g_aui8ReadBuffer, g_sTestDescription.ui32Size, NULL, SPIS_INF_TIMEOUT);
				for (j=0; j<g_sTestDescription.ui32Size; j++)
				{
					if(g_aui8ReadBuffer[j] != g_aui8RefReadBuffer[j])
					{
						__TBILogF("Error in byte %d in job %d\n", j, i);
						__TBILogF("Expected: %d\n", g_aui8RefReadBuffer[j]);
						__TBILogF("Found: %d\n", g_aui8ReadBuffer[j]);
						return;
					}
				}
				KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			}
			__TBILogF("SUCCESS: support for %d consecutive reads!\n", g_sTestDescription.ui32NumJobs);
			break;

		case TEST_SUPPORT_WRITES:
			/*
			** Check support for consecutive writes
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, NULL, 0))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}
			for (i=0; i < g_sTestDescription.ui32NumJobs; i++)
			{
				__TBILogF("Wait for master to request %d bytes!\n", g_sTestDescription.ui32Size);
				SPISWrite(&g_sSPISlave, g_aui8RefWriteBuffer, g_sTestDescription.ui32Size, NULL, SPIS_INF_TIMEOUT);
				__TBILogF("Check written output at the slave's read result\n");
				KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			}
			__TBILogF("SUCCESS: support for %d consecutive writes!\n", g_sTestDescription.ui32NumJobs);
			break;

		case TEST_ASYNC_TX_CALLBACK:
			/*
			** Check asynchronous TX operation using callback function
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, &g_sIOBlocks[0], SPIS_NUM_IO_BLOCKS))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}
			SPIS_ASYNC_CALLBACK(&g_sAsync[0], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc, &g_sAsyncCallbackCtx[0]);

			// write to master
			if (SPIS_STATUS_SUCCESS != SPISWrite(&g_sSPISlave, g_aui8RefWriteBuffer, g_sTestDescription.ui32Size,
			                                     &g_sAsync[0], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			// wait for write to complete
			__TBILogF("Waiting for master to read data from slave!\n");
			while (g_sAsyncCallbackCtx[0].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
			{
				KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			}

			// write completed
			if (g_sAsyncCallbackCtx[0].ui32Status != SPIS_STATUS_SUCCESS)
			{
				__TBILogF("FAIL: asynchronous write operation using callback function failed!\n");
				return;
			}
			__TBILogF("SUCCESS: asynchronous write operation using callback function?\n");
			__TBILogF("     - Check written output at the master's read result\n");
			break;

		case TEST_ASYNC_TX_GET_RESULT:
			/*
			** Check asynchronous TX operation using get result function
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, &g_sIOBlocks[0], SPIS_NUM_IO_BLOCKS))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}
			SPIS_ASYNC_GET_RESULT(&g_sAsync[0], &g_sAsyncCallbackCtx[0]);

			// write to master
			if (SPIS_STATUS_SUCCESS != SPISWrite(&g_sSPISlave, g_aui8RefWriteBuffer, g_sTestDescription.ui32Size,
			                                     &g_sAsync[0], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			// wait for write to complete
			__TBILogF("Waiting for master to read data from slave!\n");
			if (SPIS_STATUS_SUCCESS != SPISGetResult(&g_sSPISlave, (void**)&g_psAsyncGetCtx[0], 1, SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: asynchronous write operation using 'get result' function failed!\n");
				return;
			}
			__TBILogF("SUCCESS: asynchronous write operation using API 'get result' function?\n");
			__TBILogF("     - Check written output at the master's read result\n");
			break;

		case TEST_ASYNC_TX_IGNORE:
			/*
			** Check asynchronous TX operation ignoring result
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, &g_sIOBlocks[0], SPIS_NUM_IO_BLOCKS))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}
			SPIS_ASYNC_FORGET(&g_sAsync[0]);

			// write to master
			if (SPIS_STATUS_SUCCESS != SPISWrite(&g_sSPISlave, g_aui8RefWriteBuffer, g_sTestDescription.ui32Size,
			                                     &g_sAsync[0], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}
			KRN_hibernate(&g_sTaskQueue, (POLL_WAIT * 50));
			__TBILogF("SUCCESS: asynchronous write operation ignoring result?\n");
			__TBILogF("     - Check written output\n");
			break;

		case TEST_ASYNC_RX_CALLBACK:
			/*
			** Check asynchronous RX operation using callback function
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, &g_sIOBlocks[0], SPIS_NUM_IO_BLOCKS))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}
			SPIS_ASYNC_CALLBACK(&g_sAsync[0], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc, &g_sAsyncCallbackCtx[0]);

			// read from master
			if (SPIS_STATUS_SUCCESS != SPISRead(&g_sSPISlave, g_aui8ReadBuffer, g_sTestDescription.ui32Size,
			                                     &g_sAsync[0], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			// wait for read to complete
			__TBILogF("Wait for master to send EXACTLY %d bytes!\n", g_sTestDescription.ui32Size);
			while (g_sAsyncCallbackCtx[0].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
			{
				KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			}

			// read completed
			if (g_sAsyncCallbackCtx[0].ui32Status != SPIS_STATUS_SUCCESS)
			{
				__TBILogF("FAIL: asynchronous read operation using callback function failed!\n");
				return;
			}

			// compare message read from slave with reference read buffer
			for (j = 0; j < g_sTestDescription.ui32Size; j++)
			{
				if((g_sAsyncCallbackCtx[0].pui8Buffer)[j] != g_aui8RefReadBuffer[j])
				{
					__TBILogF("FAIL: Expected: %d, found %d\n", g_aui8RefReadBuffer[j], (g_sAsyncCallbackCtx[0].pui8Buffer)[j]);
					return;
				}
			}
			__TBILogF("SUCCESS: asynchronous read operation using callback function!\n");
			break;

		case TEST_ASYNC_RX_GET_RESULT:
			/*
			** Check asynchronous RX operation using get result function
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, &g_sIOBlocks[0], SPIS_NUM_IO_BLOCKS))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}
			SPIS_ASYNC_GET_RESULT(&g_sAsync[0], &g_sAsyncCallbackCtx[0]);

			// read from master
			if (SPIS_STATUS_SUCCESS != SPISRead(&g_sSPISlave, g_aui8ReadBuffer, g_sTestDescription.ui32Size,
			                                     &g_sAsync[0], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			// wait for read to complete
			__TBILogF("Wait for master to send EXACTLY %d bytes!\n", g_sTestDescription.ui32Size);
			if (SPIS_STATUS_SUCCESS != SPISGetResult(&g_sSPISlave, (void**)&g_psAsyncGetCtx[0], 1, SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: asynchronous write operation using 'get result' function failed!\n");
				return;
			}

			// compare message read from slave with reference read buffer
			for (j = 0; j < g_sTestDescription.ui32Size; j++)
			{
				if(g_aui8ReadBuffer[j] != g_aui8RefReadBuffer[j])
				{
					__TBILogF("FAIL: Expected: %d, found %d\n", g_aui8RefReadBuffer[j], g_aui8ReadBuffer[j]);
					return;
				}
			}
			__TBILogF("SUCCESS: asynchronous read operation using 'get result' function!\n");
			break;

		case TEST_ASYNC_RX_IGNORE:
			/*
			** Check asynchronous RX operation ignoring result
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, &g_sIOBlocks[0], SPIS_NUM_IO_BLOCKS))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}
			SPIS_ASYNC_FORGET(&g_sAsync[0]);

			// read from master
			if (SPIS_STATUS_SUCCESS != SPISRead(&g_sSPISlave, g_aui8ReadBuffer, g_sTestDescription.ui32Size,
			                                     &g_sAsync[0], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}
			KRN_hibernate(&g_sTaskQueue, (POLL_WAIT * 50));
			__TBILogF("SUCCESS: asynchronous read operation ignoring result!\n");
			break;

		case TEST_QUEUE_ASYNC_TX:
			/*
			** Check queue of asynchronous TX operations
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, &g_sIOBlocks[0], SPIS_NUM_IO_BLOCKS))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}

			// queue 2 writes
			for (i = 0; i < 2; i++)
			{
				SPIS_ASYNC_CALLBACK(&g_sAsync[i], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc, &g_sAsyncCallbackCtx[i]);
				// write to master
				if (SPIS_STATUS_SUCCESS != SPISWrite(&g_sSPISlave, g_aui8RefWriteBuffer, g_sTestDescription.ui32Size,
			                                     &g_sAsync[i], SPIS_INF_TIMEOUT))
				{
					__TBILogF("FAIL: write failed!\n");
					return;
				}
			}

			// wait for all writes to complete
			for (i = 0; i < 2; i++)
			{
				__TBILogF("Waiting for master to read data from slave!\n");
				while (g_sAsyncCallbackCtx[i].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
				{
					KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
				}
				// write completed
				if (g_sAsyncCallbackCtx[i].ui32Status != SPIS_STATUS_SUCCESS)
				{
					__TBILogF("FAIL: queue write operation number %d failed!\n", i);
					return;
				}
			}
			__TBILogF("SUCCESS: queuing write operations?\n");
			__TBILogF("     - Check written output at the master's read result\n");
			break;

		case TEST_QUEUE_ASYNC_RX:
			/*
			** Check queue of asynchronous RX operations
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, &g_sIOBlocks[0], SPIS_NUM_IO_BLOCKS))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}

			// queue 2 reads
			for (i = 0; i < 2; i++)
			{
				SPIS_ASYNC_CALLBACK(&g_sAsync[i], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc, &g_sAsyncCallbackCtx[i]);
				// write to master
				if (SPIS_STATUS_SUCCESS != SPISRead(&g_sSPISlave, g_aui8ReadBuffer, g_sTestDescription.ui32Size,
			                                     &g_sAsync[i], SPIS_INF_TIMEOUT))
				{
					__TBILogF("FAIL: write failed!\n");
					return;
				}
			}

			// wait for all reads to complete
			for (i = 0; i < 2; i++)
			{
				__TBILogF("Waiting for master to write data to slave!\n");
				while (g_sAsyncCallbackCtx[i].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
				{
					KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
				}
				// read completed
				if (g_sAsyncCallbackCtx[i].ui32Status != SPIS_STATUS_SUCCESS)
				{
					__TBILogF("FAIL: queue read operation number %d failed!\n", i);
					return;
				}
				// compare message read from slave with reference read buffer
				for (j = 0; j < g_sTestDescription.ui32Size; j++)
				{
					if((g_sAsyncCallbackCtx[i].pui8Buffer)[j] != g_aui8RefReadBuffer[j])
					{
						__TBILogF("FAIL: Expected: %d, found %d from read number %d\n", g_aui8RefReadBuffer[j], (g_sAsyncCallbackCtx[i].pui8Buffer)[j], i);
						return;
					}
				}
			}
			__TBILogF("SUCCESS: queuing read operations!\n");

			__TBILogF("Resetting API...\n" );
			SPISDeinit(&g_sSPISlave);
			KRN_hibernate(&g_sTaskQueue, MILLISECOND_TO_TICK(2000) );
			__TBILogF("Reconfiguring API and rerunning test...\n" );

			// Check queue of asynchronous RX operations
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, &g_sIOBlocks[0], SPIS_NUM_IO_BLOCKS))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}

			// queue 2 reads
			for (i = 0; i < 2; i++)
			{
				SPIS_ASYNC_CALLBACK(&g_sAsync[i], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc, &g_sAsyncCallbackCtx[i]);
				// write to master
				if (SPIS_STATUS_SUCCESS != SPISRead(&g_sSPISlave, g_aui8ReadBuffer, g_sTestDescription.ui32Size,
			                                     &g_sAsync[i], SPIS_INF_TIMEOUT))
				{
					__TBILogF("FAIL: write failed!\n");
					return;
				}
			}

			// wait for all reads to complete
			for (i = 0; i < 2; i++)
			{
				__TBILogF("Waiting for master to write data to slave!\n");
				while (g_sAsyncCallbackCtx[i].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
				{
					KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
				}
				// read completed
				if (g_sAsyncCallbackCtx[i].ui32Status != SPIS_STATUS_SUCCESS)
				{
					__TBILogF("FAIL: queue read operation number %d failed!\n", i);
					return;
				}
				// compare message read from slave with reference read buffer
				for (j = 0; j < g_sTestDescription.ui32Size; j++)
				{
					if((g_sAsyncCallbackCtx[i].pui8Buffer)[j] != g_aui8RefReadBuffer[j])
					{
						__TBILogF("FAIL: Expected: %d, found %d from read number %d\n", g_aui8RefReadBuffer[j], (g_sAsyncCallbackCtx[i].pui8Buffer)[j], i);
						return;
					}
				}
			}
			__TBILogF("SUCCESS: queuing read operations!\n");
			break;

		case TEST_QUEUE_ASYNC_TOO_MANY:
			/*
			** Check queuing too many operations
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, &g_sIOBlocks[0], SPIS_NUM_IO_BLOCKS))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}

			transactionCount = 0;
			do
			{	transactionCount ++;
				SPIS_ASYNC_CALLBACK(&g_sAsync[transactionCount], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc,
				                    &g_sAsyncCallbackCtx[transactionCount]);
			}
			while (SPIS_STATUS_SUCCESS == SPISRead(&g_sSPISlave, g_aui8ReadBuffer, g_sTestDescription.ui32Size,
			                                       &g_sAsync[transactionCount], SPIS_INF_TIMEOUT));

			if (transactionCount <= SPIS_NUM_IO_BLOCKS)
			{
				__TBILogF("FAIL: queuing too many operations failed after queuing %d reads!\n", transactionCount);
				return;
			}

			// wait for all the non blocked reads to complete
			for (i = 1; i < transactionCount; i++)
			{
				__TBILogF("Waiting for master to write data to slave!\n");
				while (g_sAsyncCallbackCtx[i].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
				{
					KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
				}
				// read completed
				if (g_sAsyncCallbackCtx[i].ui32Status != SPIS_STATUS_SUCCESS)
				{
					__TBILogF("FAIL: queue read operation number %d failed!\n", i);
					return;
				}
				// compare message read from slave with reference read buffer
				for (j = 0; j < g_sTestDescription.ui32Size; j++)
				{
					if((g_sAsyncCallbackCtx[i].pui8Buffer)[j] != g_aui8RefReadBuffer[j])
					{
						__TBILogF("FAIL: Expected: %d, found %d from read number %d\n", g_aui8RefReadBuffer[j], (g_sAsyncCallbackCtx[i].pui8Buffer)[j], i);
						return;
					}
				}
			}
			__TBILogF("SUCCESS: queuing too many operations succeed after %d operations requested whilst only %d operations can be queued!\n", transactionCount, SPIS_NUM_IO_BLOCKS);
			break;

		case TEST_QUEUE_MIX_RX_TX:
			/*
			** queuing a mix of TX/RX operations
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, &g_sIOBlocks[0], SPIS_NUM_IO_BLOCKS))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}

			// queue a read
			SPIS_ASYNC_CALLBACK(&g_sAsync[0], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc, &g_sAsyncCallbackCtx[0]);
			// write to master
			if (SPIS_STATUS_SUCCESS != SPISRead(&g_sSPISlave, g_aui8ReadBuffer, g_sTestDescription.ui32Size,
											 &g_sAsync[0], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			// queue a write
			SPIS_ASYNC_CALLBACK(&g_sAsync[1], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc, &g_sAsyncCallbackCtx[1]);
			// write to master
			if (SPIS_STATUS_SUCCESS != SPISWrite(&g_sSPISlave, g_aui8RefWriteBuffer, g_sTestDescription.ui32Size,
											 &g_sAsync[1], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			// queue a read
			SPIS_ASYNC_CALLBACK(&g_sAsync[2], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc, &g_sAsyncCallbackCtx[2]);
			// write to master
			if (SPIS_STATUS_SUCCESS != SPISRead(&g_sSPISlave, g_aui8ReadBuffer, g_sTestDescription.ui32Size,
											 &g_sAsync[2], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			// wait for read to complete
			__TBILogF("Wait for master to send EXACTLY %d bytes!\n", g_sTestDescription.ui32Size);
			while (g_sAsyncCallbackCtx[0].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
			{
				KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			}

			// read completed
			if (g_sAsyncCallbackCtx[0].ui32Status != SPIS_STATUS_SUCCESS)
			{
				__TBILogF("FAIL: asynchronous read operation using callback function failed!\n");
				return;
			}

			// compare message read from slave with reference read buffer
			for (j = 0; j < g_sTestDescription.ui32Size; j++)
			{
				if((g_sAsyncCallbackCtx[0].pui8Buffer)[j] != g_aui8RefReadBuffer[j])
				{
					__TBILogF("FAIL: Expected: %d, found %d\n", g_aui8RefReadBuffer[j], (g_sAsyncCallbackCtx[0].pui8Buffer)[j]);
					return;
				}
			}
			__TBILogF("Asynchronous read operation succeeded!\n");

			// wait for write to complete
			__TBILogF("Waiting for master to read data from slave!\n");
			while (g_sAsyncCallbackCtx[1].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
			{
				KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			}

			// write completed
			if (g_sAsyncCallbackCtx[1].ui32Status != SPIS_STATUS_SUCCESS)
			{
				__TBILogF("FAIL: asynchronous write operation using callback function failed!\n");
				return;
			}
			__TBILogF("Asynchronous write operation succeeded?\n");
			__TBILogF("     - Check written output at the master's read result\n");

			// wait for read to complete
			__TBILogF("Wait for master to send EXACTLY %d bytes!\n", g_sTestDescription.ui32Size);
			while (g_sAsyncCallbackCtx[2].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
			{
				KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			}

			// read completed
			if (g_sAsyncCallbackCtx[2].ui32Status != SPIS_STATUS_SUCCESS)
			{
				__TBILogF("FAIL: asynchronous read operation using callback function failed!\n");
				return;
			}

			// compare message read from slave with reference read buffer
			for (j = 0; j < g_sTestDescription.ui32Size; j++)
			{
				if((g_sAsyncCallbackCtx[2].pui8Buffer)[j] != g_aui8RefReadBuffer[j])
				{
					__TBILogF("FAIL: Expected: %d, found %d\n", g_aui8RefReadBuffer[j], (g_sAsyncCallbackCtx[2].pui8Buffer)[j]);
					return;
				}
			}
			__TBILogF("Asynchronous read operation succeeded!\n");

			__TBILogF("SUCCESS: queuing a mix of read and write operations\n");
			break;

		case TEST_TIMEOUT:
			/*
			** Check timeout on synchronous write/read
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, &g_sIOBlocks[0], SPIS_NUM_IO_BLOCKS))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}

			/*
			** Check timeout on synchronous write/read using callback function
			*/

			// Queue the transactions
			for (i = 0; i < 2; i++)
			{
				SPIS_ASYNC_CALLBACK(&g_sAsync[i], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc, &g_sAsyncCallbackCtx[i]);

				// write to master
				if (SPIS_STATUS_SUCCESS != SPISWrite(&g_sSPISlave, g_aui8RefWriteBuffer, g_sTestDescription.ui32Size,
													&g_sAsync[i], timeout))
				{
					__TBILogF("FAIL: write failed!\n");
					return;
				}
			}

			__TBILogF("Waiting for master to read data from slave!\n");

			// Wait for first transaction to complete, 2nd to be cancelled because of timeout
			for (i = 0; i < 2; i++)
			{
				// wait for writes to complete
				while (g_sAsyncCallbackCtx[i].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
				{
					KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
				}
			}

			// write completed
			if (g_sAsyncCallbackCtx[0].ui32Status != SPIS_STATUS_SUCCESS)
			{
				__TBILogF("FAIL: asynchronous write operation using callback function failed!\n");
				return;
			}

			if (g_sAsyncCallbackCtx[1].ui32Status != SPIS_STATUS_TIMEOUT)
			{
				__TBILogF("FAIL: timeout on asynchronous write operation using callback function failed!\n");
				return;
			}

			__TBILogF("Asynchronous write operation succeeded?\n");
			__TBILogF("     - Check written output at the master's read result\n");

			// Queue the read transactions
			for (i = 0; i < 2; i++)
			{
				SPIS_ASYNC_CALLBACK(&g_sAsync[i], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc, &g_sAsyncCallbackCtx[i]);

				// read from master
				g_sAsyncCallbackCtx[i].ui32Status = SPIS_TEST_STATUS_ASYNC_DEFAULT;
				if (SPIS_STATUS_SUCCESS != SPISRead(&g_sSPISlave, g_aui8ReadBuffer, g_sTestDescription.ui32Size,
													&g_sAsync[i], timeout))
				{
					__TBILogF("FAIL: write failed!\n");
					return;
				}
			}

			__TBILogF("Wait for master to send EXACTLY %d bytes!\n", g_sTestDescription.ui32Size);
			// Wait for 1st read transaction to complete, the 2nd one to be cancelled because of timeout.
			for (i = 0; i < 2; i++)
			{
				// wait for read to complete
				while (g_sAsyncCallbackCtx[i].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
				{
					KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
				}
			}

			// read completed
			if (g_sAsyncCallbackCtx[0].ui32Status != SPIS_STATUS_SUCCESS)
			{
				__TBILogF("FAIL: asynchronous read operation using callback function failed!\n");
				return;
			}
			if (g_sAsyncCallbackCtx[1].ui32Status != SPIS_STATUS_TIMEOUT)
			{
				__TBILogF("FAIL: timeout on asynchronous read operation using callback function failed!\n");
				return;
			}
			__TBILogF("SUCCESS: timeout on asynchronous write/read operation using callback function!\n");

			/*
			** Check timeout on asynchronous write/read using get result function
			*/
			SPIS_ASYNC_GET_RESULT(&g_sAsync[0], &g_sAsyncCallbackCtx[0]);
			SPIS_ASYNC_GET_RESULT(&g_sAsync[1], &g_sAsyncCallbackCtx[1]);

			// write to master
			if (SPIS_STATUS_SUCCESS != SPISWrite(&g_sSPISlave, g_aui8RefWriteBuffer, g_sTestDescription.ui32Size,
												&g_sAsync[0], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}
			if (SPIS_STATUS_SUCCESS != SPISWrite(&g_sSPISlave, g_aui8RefWriteBuffer, g_sTestDescription.ui32Size,
												&g_sAsync[1], timeout))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			__TBILogF("Waiting for master to read data from slave!\n");

			// wait for 1st write to complete
			if (SPIS_STATUS_TIMEOUT != SPISGetResult(&g_sSPISlave, (void**)&g_psAsyncGetCtx[0], 1, SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: asynchronous write operation using 'get result' function failed!\n");
				return;
			}
			// wait for 2nd write to complete
			if (SPIS_STATUS_SUCCESS != SPISGetResult(&g_sSPISlave, (void**)&g_psAsyncGetCtx[1], 1, SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: asynchronous write operation using 'get result' function failed!\n");
				return;
			}

			__TBILogF("Asynchronous write operation succeeded?\n");
			__TBILogF("     - Check written output at the master's read result\n");


			SPIS_ASYNC_GET_RESULT(&g_sAsync[0], &g_sAsyncCallbackCtx[0]);
			SPIS_ASYNC_GET_RESULT(&g_sAsync[1], &g_sAsyncCallbackCtx[1]);

			// read from master
			if (SPIS_STATUS_SUCCESS != SPISRead(&g_sSPISlave, g_aui8ReadBuffer, g_sTestDescription.ui32Size,
												&g_sAsync[0], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}
			if (SPIS_STATUS_SUCCESS != SPISRead(&g_sSPISlave, g_aui8ReadBuffer, g_sTestDescription.ui32Size,
												&g_sAsync[1], timeout))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			__TBILogF("Wait for master to send EXACTLY %d bytes!\n", g_sTestDescription.ui32Size);

			// wait for 1st read to complete
			if (SPIS_STATUS_TIMEOUT != SPISGetResult(&g_sSPISlave, (void**)&g_psAsyncGetCtx[0], 1, SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: asynchronous write operation using 'get result' function failed!\n");
				return;
			}
			// wait for 2nd read to complete
			if (SPIS_STATUS_SUCCESS != SPISGetResult(&g_sSPISlave, (void**)&g_psAsyncGetCtx[1], 1, SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: asynchronous write operation using 'get result' function failed!\n");
				return;
			}

			__TBILogF("SUCCESS: timeout on asynchronous write/read operation using 'get result' function!\n");
			break;

		case TEST_TIMEOUT_GET_RESULT:
			/*
			** Check timeout on get write/read result function
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, &g_sIOBlocks[0], SPIS_NUM_IO_BLOCKS))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}

			SPIS_ASYNC_GET_RESULT(&g_sAsync[0], &g_sAsyncCallbackCtx[0]);

			// write to master
			if (SPIS_STATUS_SUCCESS != SPISWrite(&g_sSPISlave, g_aui8RefWriteBuffer, g_sTestDescription.ui32Size,
			                                     &g_sAsync[0], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			// wait for write to complete
			if (SPIS_STATUS_WOULD_BLOCK != SPISGetResult(&g_sSPISlave, (void**)&g_psAsyncGetCtx[0], 1, timeout))
			{
				__TBILogF("FAIL: asynchronous write operation using 'get result' function failed!\n");
				return;
			}

			// read from master
			if (SPIS_STATUS_SUCCESS != SPISRead(&g_sSPISlave, g_aui8ReadBuffer, g_sTestDescription.ui32Size,
			                                     &g_sAsync[0], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			// wait for read to complete
			if (SPIS_STATUS_WOULD_BLOCK != SPISGetResult(&g_sSPISlave, (void**)&g_psAsyncGetCtx[0], 1, timeout))
			{
				__TBILogF("FAIL: asynchronous write operation using 'get result' function failed!\n");
				return;
			}

			__TBILogF("SUCCESS: timeout on get write/read result function!\n");
			break;

		case TEST_CANCEL_QUEUE_ASYNC:
			/*
			** Check cancelling queue of async write/read operations
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, &g_sIOBlocks[0], SPIS_NUM_IO_BLOCKS))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}

			// queue a write
			SPIS_ASYNC_CALLBACK(&g_sAsync[0], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc, &g_sAsyncCallbackCtx[0]);
			// write to master
			if (SPIS_STATUS_SUCCESS != SPISWrite(&g_sSPISlave, g_aui8RefWriteBuffer, g_sTestDescription.ui32Size,
											 &g_sAsync[0], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			// queue a read
			SPIS_ASYNC_CALLBACK(&g_sAsync[1], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc, &g_sAsyncCallbackCtx[1]);
			// write to master
			if (SPIS_STATUS_SUCCESS != SPISRead(&g_sSPISlave, g_aui8ReadBuffer, g_sTestDescription.ui32Size,
											 &g_sAsync[1], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: read failed!\n");
				return;
			}

			// queue a write
			SPIS_ASYNC_CALLBACK(&g_sAsync[2], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc, &g_sAsyncCallbackCtx[2]);
			// write to master
			if (SPIS_STATUS_SUCCESS != SPISWrite(&g_sSPISlave, g_aui8RefWriteBuffer, g_sTestDescription.ui32Size,
											 &g_sAsync[2], SPIS_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			// cancel all
			SPISCancel(&g_sSPISlave);

			__TBILogF("Waiting for master to read data from slave!\n");
			// wait for all operations to complete
			while (g_sAsyncCallbackCtx[0].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
			{
				KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			}
			// operation completed
			if (g_sAsyncCallbackCtx[0].ui32Status != SPIS_STATUS_SUCCESS)
			{
				__TBILogF("FAIL!\n");
				return;
			}
			__TBILogF("Asynchronous write operation succeeded?\n");
			__TBILogF("     - Check written output at the master's read result\n");

			for (i = 1; i < 3; i++)
			{
				while (g_sAsyncCallbackCtx[i].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
				{
					KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
				}
				// operation completed
				if (g_sAsyncCallbackCtx[i].ui32Status != SPIS_STATUS_CANCEL)
				{
					__TBILogF("FAIL: cancelling asynchronous operation in a queue failed!\n");
					return;
				}
			}
			__TBILogF("SUCCESS: cancelling queue of asynchronous write/read operation!\n");
			break;

		case TEST_CANCEL_ASYNC_THEN_QUEUE_NEW:
			/*
			** Check cancelling async write operation and
			** then queuing new async write operation
			*/
			if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, &g_sIOBlocks[0], SPIS_NUM_IO_BLOCKS))
			{
				__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
				return;
			}

			// queue 2 writes
			for (i = 0; i < 2; i++)
			{
				SPIS_ASYNC_CALLBACK(&g_sAsync[i], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc, &g_sAsyncCallbackCtx[i]);
				// write to master
				if (SPIS_STATUS_SUCCESS != SPISWrite(&g_sSPISlave, g_aui8RefWriteBuffer, g_sTestDescription.ui32Size,
			                                     &g_sAsync[i], SPIS_INF_TIMEOUT))
				{
					__TBILogF("FAIL: write failed!\n");
					return;
				}
			}

			// cancel write
			SPISCancel(&g_sSPISlave);

			// queue 2 more writes
			for (i = 2; i < 4; i++)
			{
				SPIS_ASYNC_CALLBACK(&g_sAsync[i], (SPIS_CALLBACKROUTINE_T*)&CompleteFunc, &g_sAsyncCallbackCtx[i]);
				// write to master
				if (SPIS_STATUS_SUCCESS != SPISWrite(&g_sSPISlave, g_aui8RefWriteBuffer, g_sTestDescription.ui32Size,
			                                     &g_sAsync[i], SPIS_INF_TIMEOUT))
				{
					__TBILogF("FAIL: write failed!\n");
					return;
				}
			}

			// wait for write to complete
			while (g_sAsyncCallbackCtx[1].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
			{
				KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			}

			// write completed
			if (g_sAsyncCallbackCtx[1].ui32Status != SPIS_STATUS_CANCEL)
			{
				__TBILogF("FAIL: cancelling asynchronous write operation failed!\n");
				return;
			}

			// wait for write to complete
			__TBILogF("Waiting for master to read data from slave!\n");
			while (g_sAsyncCallbackCtx[0].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
			{
				KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			}

			// write completed
			if (g_sAsyncCallbackCtx[0].ui32Status != SPIS_STATUS_SUCCESS)
			{
				__TBILogF("FAIL: asynchronous write operation using callback function failed!\n");
				return;
			}

			// wait for remaining writes to complete
			for (i = 2; i < 4; i++)
			{
				__TBILogF("Waiting for master to read data from slave!\n");
				while (g_sAsyncCallbackCtx[i].ui32Status == SPIS_TEST_STATUS_ASYNC_DEFAULT)
				{
					KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
				}
				// write completed
				if (g_sAsyncCallbackCtx[i].ui32Status != SPIS_STATUS_SUCCESS)
				{
					__TBILogF("FAIL: asynchronous write operation using callback function failed!\n", i);
					return;
				}
			}
			__TBILogF("SUCCESS: asynchronous write operation using callback function?\n");
			__TBILogF("     - Check written output at the master's read result\n");
			break;
	}

    __TBILogF("Test completed.\n");

    return;
}

/******************************************************************************
**
**                          SUPPORT FUNCTIONS
**
******************************************************************************/

/*
** FUNCTION:        CompleteFunc
**
** DESCRIPTION: 	This function notify the calling application of I/O completion.
**                  It uses the context to report the result of the operation.
**
** PARAMETERS:		*context               	Pointer to private context for the callback.
**					*buffer                	Pointer to buffer where data was transferred to/from.
**					num_bytes_to_transfer   The number of bytes the slave was 'expecting'
**                                          to transfer (tis is the size of the buffer
**                                          that data was transferred to/from)
**					status              	Status code describing the completion status of
**                                			the transaction:
**                                				SCBS_STATUS_SUCCESS - Operation completed successfully.
**                                				SCBS_STATUS_CANCEL  - Operation was cancelled.
**                                				SCBS_STATUS_TIMEOUT - Operation was timed out.
**
** RETURNS:         None
*/
static void CompleteFunc (
    void          	*pContext,
   	unsigned char 	*pui8Buffer,
    unsigned long	ui32NumBytesToTransfer,
	unsigned long 	ui32Status)
{
	TEST_sAsyncCtx	*	psAsyncCtx = (TEST_sAsyncCtx*)pContext;

	psAsyncCtx->pui8Buffer             = pui8Buffer;
	psAsyncCtx->ui32NumBytesToTransfer = ui32NumBytesToTransfer;
	psAsyncCtx->ui32Status             = ui32Status;

    return;
}

/*
** FUNCTION:    ParseCommandLine
**
** DESCRIPTION: Parses the command line setting test parameters and populating
**              the static test description (where applicable). This function
**              should be modified to fit the peripheral under test.
**
** RETURNS:     void - will terminate operation on error
*/
static void ParseCommandLine(int argc, char *argv[])
{
    char *cmd;
    char *option;
    char  tempString[20], *fileName;
    int temp;

	/*
	** Codescape or Dashscript writes the test parameters into the metag_argv array.
	** We copy them to the C standard argc and *argv[] variables.
	*/
	argv = metag_argv;

	/* Assume that all arguments are used - argc can be determined from the size of metag_argv */
	argc = (sizeof(metag_argv)/sizeof(char *));
	while ((argv[argc-1][0] == '\0') || (argv[argc-1][0] == '@'))
	{
		/*
		 * If metag_argv is "@" or "\0", then this argument is unused, so
		 * we must decrement argc as one less argument has been provided.
		 */
		argc--;
	}

	cmd = *argv++;

    verboseLog("Parsing command line - %d arguments\n", argc);

	g_sTestDescription.fRefWrite = -1;
	g_sTestDescription.fRefRead  = -1;
	g_sTestDescription.ui32NumJobs = 1;

    /* Parse command line */
    while (argc>1)
    {
        if (*argv[0] == '-')
        {
            option = (*argv);

			if ( strncmp(option, "-auto", 5) == 0 )
			{
				g_bAutomaticMode = IMG_TRUE;
			}
            else if (strncmp(option, "-test", 5) == 0)
            {
            	//Configure test number
                argv++;
                argc--;
                sscanf(*argv, "%d", &(g_sTestDescription.ui32TestNum));
                if(g_sTestDescription.ui32TestNum == 0)
                {
					verboseLog("Test number minimum is 1.\n");
					Usage(cmd);
				}
                if(g_sTestDescription.ui32TestNum >= TEST_NUM_MAX)
                {
					verboseLog("Test number maximum is %d.\n", TEST_NUM_MAX);
					Usage(cmd);
				}
                verboseLog("Test number %d\n", g_sTestDescription.ui32TestNum);
            }

			else if (strncmp(option, "-size", 5) == 0)
            {
            	//Configure job size
                argv++;
                argc--;
                sscanf(*argv, "%d", &(g_sTestDescription.ui32Size));
                if(g_sTestDescription.ui32Size > 5120)
                {
					verboseLog("Job size cannot exceed 1024 bytes\n");
					Usage(cmd);
				}
                verboseLog("Each job contains %d bytes\n", g_sTestDescription.ui32Size);
            }

			else if (strncmp(option, "-jobs", 5) == 0)
            {
            	//Configure number of jobs
                argv++;
                argc--;
                sscanf(*argv, "%d", &(g_sTestDescription.ui32NumJobs));
				if(g_sTestDescription.ui32NumJobs > 5)
                {
					verboseLog("Number of jobs cannot exceed 5\n");
					Usage(cmd);
				}

                verboseLog("Transaction involves %d jobs\n", g_sTestDescription.ui32NumJobs);
            }

			else if (strncmp(option, "-dma", 4) == 0)
            {
            	//Configure input DMA channel
                argv++;
                argc--;
                sscanf(*argv, "%d", &(g_sTestDescription.ui32DmaChannel));
                if(g_sTestDescription.ui32DmaChannel > 4)
                {
					verboseLog("SPI DMA channel cannot be greater than 4\n");
					Usage(cmd);
				}
                verboseLog("SPI DMA channel: %d\n", g_sTestDescription.ui32DmaChannel);
            }

			else if (strncmp(option, "-mode", 5) == 0)
            {
            	//Configure spi mode
                argv++;
                argc--;
                sscanf(*argv, "%d", &(temp));
				if(temp > 3)
                {
					verboseLog("SPI Mode number must be between 0 and 3\n");
					Usage(cmd);
				}

				switch (temp)
				{
					case 0:	g_sTestDescription.eSPIMode = SPI_MODE_0;
							break;

					case 1:	g_sTestDescription.eSPIMode = SPI_MODE_1;
							break;

					case 2:	g_sTestDescription.eSPIMode = SPI_MODE_2;
							break;

					case 3:	g_sTestDescription.eSPIMode = SPI_MODE_3;
							break;
				}

                verboseLog("SPI set to Mode %d\n", temp);
            }

			else if (strncmp(option, "-cs", 3) == 0)
			{
				//Configure CS active level
				argv++;
				argc--;
				sscanf(*argv, "%d", &(g_sTestDescription.ui32CSLevel));
				if(g_sTestDescription.ui32CSLevel > 1)
				{
					verboseLog("Chip select active level must be set to 0 or 1\n");
					Usage(cmd);
				}
				verboseLog("Chip select active level set to %d\n", g_sTestDescription.ui32CSLevel);
			}
			else if (strncmp(option, "-write", 6) == 0)
            {
            	//Open write file
            	fileName = tempString;
                argv++;
                argc--;
                sscanf(*argv, "%s", fileName);

                g_sTestDescription.fRefWrite = open(fileName, O_RDONLY, 0744);

                if (g_sTestDescription.fRefWrite == -1)
                {
                	__TBILogF("Error opening write file %s\n", fileName);
                	Usage(cmd);
                }

                verboseLog("Write file name %s\n", fileName);
                g_bWriteFile = IMG_TRUE;
            }

            else if (strncmp(option, "-read", 5) == 0)
            {
                //Open read file
                fileName = tempString;
                argv++;
                argc--;
                sscanf(*argv, "%s", fileName);

                g_sTestDescription.fRefRead = open(fileName, O_RDONLY, 0744);

                if (g_sTestDescription.fRefRead == -1)
                {
                    __TBILogF("Error opening read file %s\n", fileName);
                    Usage(cmd);
                }

                verboseLog("Read file name %s\n", fileName);
                g_bReadFile = 1;
            }
			else if ( strncmp( option, "-block", 6 ) == 0 )
			{
				argv++;
				argc--;
				sscanf( *argv, "%d", &(g_sTestDescription.ui32BlockNum) );

				g_ui32BlockNum = g_sTestDescription.ui32BlockNum;
				verboseLog("Use SDIO Block set to %d\n", g_sTestDescription.ui32BlockNum );
			}
			else if ( strncmp( option, "-sync", 5 ) == 0 )
			{
				argv++;
				argc--;
				sscanf( *argv, "%d", &(temp) );

				if ( temp > 2 )
				{
					verboseLog("Sync mode must be between 0 and 2\n");
					Usage(cmd);
				}


				switch ( temp )
				{
					case 0:
						g_sTestDescription.eSyncMode = SPI_SYNC_MODE_RESYNC;
						break;
					case 1:
						g_sTestDescription.eSyncMode = SPI_SYNC_MODE_SLOW;
						break;
					case 2:
						g_sTestDescription.eSyncMode = SPI_SYNC_MODE_LEGACY;
						break;
				}

				verboseLog("Sync mode is %d\n", g_sTestDescription.eSyncMode );
			}

            else
            {
                Usage(cmd);
            }
        }
		else
		{
			Usage(cmd);
		}

        argv++;
        argc--;
    }
}


/*
** FUNCTION:    Usage
**
** DESCRIPTION: Outputs correct usage instructions.
**
** RETURNS:     Terminates on completion - never returns.
*/
static void Usage(char *cmd)
{
    __TBILogF("\nUsage:  %s <..options..>\n", cmd);
    __TBILogF("			-test N          Test\n");
    __TBILogF("			-size N		  Number of bytes in each job (maximum: 20)\n");
	__TBILogF("			-jobs N		  Number of jobs in transaction \n");
    __TBILogF("			-dma N			  SPI DMA channel\n");
    __TBILogF("			-mode N		  SPI Mode. Must be between 0 and 3\n");
    __TBILogF("			-cs N		  	  Chip Select active level\n");
    __TBILogF("			-write <filename>Reference write file containing MISO message\n");
    __TBILogF("			-read <filename> Reference read file containing MOSI message\n");
	__TBILogF("			-block N		  Use the SPI block N\n");
	__TBILogF("			-sync N		  SPI Sync Mode. Must be between 0 and 2\n");
	__TBILogF("			-auto			  Automatic testing mode\n");
    __TBILogF("Remember to set file server root directory so Codescape looks for files in the correct folder\n");
    exit(-1);
}

/*!
******************************************************************************

 @Function				RunTest

******************************************************************************/
static img_void RunTest( img_void )
{
	/* Reset the Kernel */
    KRN_reset(&(g_sMeOSScheduler),
              g_asSchedQueues,
              MEOS_MAX_PRIORITY_LEVEL,
              0,
              IMG_NULL,
              0);

	/* Start Meos Main Task - no background task if MeOS Abstraction Layer */
	g_psBackgroundTask = KRN_startOS("Background Task");

	/* Reset QIO */
    QIO_reset(&g_sQio,
              g_aVectoredDevTable,
              QIO_NO_VECTORS,
              &QIO_MTPIVDesc,
			  TBID_SIGNUM_TR2(__TBIThreadId() >> TBID_THREAD_S),
              IMG_NULL,
			  0);

	/* Start Meos Main Timer Task...*/
	g_psTimerTask = KRN_startTimerTask("Timer Task", g_aui32TimerStack, TIM_STACK_SIZE, TIM_TICK_PERIOD);

	DQ_init( &g_sTaskQueue );

	if ( g_bAutomaticMode )
	{
		KRN_startTask(AutoTask, &g_sMainTask, g_aui32TaskStack, STACK_SIZE, KRN_LOWEST_PRIORITY + 1, IMG_NULL, "AutoTask" );
	}
	else
	{
		/* Start the main task...*/
		KRN_startTask(MainTask, &g_sMainTask, g_aui32TaskStack, STACK_SIZE, KRN_LOWEST_PRIORITY + 1, IMG_NULL, "MainTask");
	}

	return;
}

/*!
******************************************************************************

 @Function				main

******************************************************************************/
#if defined __MTX_MEOS__ || defined __META_MEOS__

int main(int argc, char **argv)
{
	KRN_TASKQ_T	sHibernateQ;

	ParseCommandLine( argc, argv );

	RunTest();

	DQ_init( &sHibernateQ );
	KRN_hibernate( &sHibernateQ, KRN_INFWAIT );
    return 1;
}

#else

#error CPU and OS not recognised

#endif
