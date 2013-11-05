/*!
******************************************************************************
 @file   : scbm_test.c

 @brief

 @Author Imagination Technologies

 @date   10/11/2010

         <b>Copyright 2010 by Imagination Technologies Limited.</b>\n
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
         SCB Master Test Application.

 <b>Platform:</b>\n
	     Platform Independent

 @Version
	     1.0

******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log:


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
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <MeOS.h>
#include <img_defs.h>
#include <ioblock_defs.h>
#include <system.h>
#include <sys_config.h>
#include <unistd.h>

#include "scbm_api.h"

/*============================================================================
====	D E F I N E S
=============================================================================*/

/* Timer defines...*/
#define STACK_SIZE			(2048)

#define MAX_MESSAGE_SIZE	(100)
#define SCBM_TEST_STATUS_ASYNC_DEFAULT (0xFF)

#define NUM_QUEUED_READS	(5)
#define NUM_QUEUED_WRITES	(5)
#define NUM_STRESS_READS	(100000)
#define NUM_STRESS_WRITES	(100000)

#define NUM_IO_BLOCKS		(8)

#define STACK_FILL			(0xdeadbeef)

#define POLL_WAIT			MILLISECOND_TO_TICK(10)

/*
** VERBOSE_LOGGING definition used to restrict logging output
*/
//#define	VERBOSE_LOGGING
#ifdef VERBOSE_LOGGING
#define verboseLog __TBILogF
#else
void dummyLog() {}
#define verboseLog dummyLog
#endif

/*============================================================================
====	E N U M S
=============================================================================*/


typedef enum
{
	TEST_ASYNC_READ_CTX = 1,		   /* 1 */
	TEST_ASYNC_READ_GET_RESULT,        /* 2 */
	TEST_ASYNC_READ_IGNORE,			   /* 3 */
	TEST_ASYNC_WRITE_CTX,			   /* 4 */
	TEST_ASYNC_WRITE_GET_RESULT,	   /* 5 */
	TEST_ASYNC_WRITE_IGNORE,		   /* 6 */
	TEST_ASYNC_QUEUE,				   /* 7 */
	TEST_ASYNC_TOO_MANY_QUEUED,		   /* 8 */
	TEST_TIMEOUT_READ_SYNC,			   /* 9 */
	TEST_TIMEOUT_WRITE_SYNC,		   /* 10 */
	TEST_TIMEOUT_GET_RESULT,		   /* 11 */
	TEST_CANCEL_ASYNC_THEN_QUEUE_NEW,  /* 12 */
	TEST_CANCEL_ASYNC_QUEUE,		   /* 13 */
	TEST_NACK_WRITE,				   /* 14 */
	TEST_NACK_READ,					   /* 15 */
	TEST_ASYNC_READ_QUEUE,			   /* 16 */
	TEST_ASYNC_WRITE_QUEUE,			   /* 17 */
	TEST_SYNC_READ_STRESS,			   /* 18 */
	TEST_SYNC_WRITE_STRESS,			   /* 19 */
	TEST_SYNC_READ_WRITE_LENGTH,	   /* 20 */
	TEST_SYNC_READ_WRITE_STRESS,	   /* 21 */
	TEST_TEN_BIT_ADDRESSING,		   /* 22 */
	TEST_INACTIVE_BUS,				   /* 23 */
	TEST_MIRICS_TUNER,				   /* 24 */
	TEST_DEBUG,						   /* 25 */
	TEST_DEBUG3,					   /* 26 */
	TEST_DEBUG2,					   /* 27 */
	NUM_TESTS_PLUS_ONE				   /* 28 */
} TEST_eTestMode;

#define NUM_TESTS (NUM_TESTS_PLUS_ONE - 1)

/*============================================================================
====	T Y P E D E F S
=============================================================================*/

typedef struct scbm_test_async_ctx_tag
{
	IMG_VOID	*	pContext;
	IMG_INT32		i32Read;
	IMG_UINT32		ui32Address;
	IMG_UINT8	*	pui8Buffer;
	IMG_UINT32		ui32NumBytesTransferred;
	IMG_UINT32		ui32Status;
} TEST_sAsyncCtx;

/*
** The TEST_DESC_T provides a location for all test configuration data
** and should be modified for the particular peripheral under test.
*/
typedef struct test_desc_port_tag
{
	IMG_UINT32		ui32SlaveAddress;
	SCBM_ASYNC_T	sAsync;
} TEST_sTestDescPort;

typedef struct test_desc_tag
{
	IMG_UINT32			ui32Size;
	IMG_UINT32			ui32TestMode;
	IMG_UINT32			ui32BitRate;
	IMG_INT				fOut;
	IMG_INT				fRef;
	TEST_sTestDescPort	sPortA;
	IMG_UINT32			ui32BlockNum;
	IMG_UINT32			ui32CoreClockVal;
	IMG_UINT32			ui32ClockSrc;	/* 0 for XTAL1, sys_clk_undeleted otherwise */
	IMG_UINT32			ui32PLLFreq;
} TEST_sTestDesc;

/*============================================================================
====	D A T A
=============================================================================*/

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
char *metag_argv[] = { "dummy", argv1, argv2, argv3, argv4, argv5, argv6, argv7, argv8, argv9, argv10,
						argv11, argv12, argv13, argv14, argv15, argv16,argv17,argv18,argv19,argv20,argv21};
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

static TEST_sTestDesc	g_sTestDescription;
static SCBM_PORT_T		g_sPort;
unsigned long			g_ui32PortANumWrite;
unsigned long			g_ui32PortANumRead;
long					g_i32PortARead;
unsigned long			g_ui32PortAAddress;

img_uint32				g_ui32DelayMS = 0;
img_uint32				g_ui32NumReads = 0;

// Buffers
IMG_UINT8				g_aui8PortWriteBuffer[MAX_MESSAGE_SIZE];
IMG_UINT8				g_aui8PortReadBuffer[MAX_MESSAGE_SIZE];
IMG_UINT8				g_aui8PortRefBuffer[MAX_MESSAGE_SIZE];

static SCBM_IO_BLOCK_T	g_sIOBlocks[NUM_IO_BLOCKS];

int						jTemp = 0;

/******************************************************************************
	Internal function prototypes
 ******************************************************************************/
static IMG_VOID ParseCommandLine(IMG_INT iArgc, IMG_CHAR	*	pszArgv[]);
static IMG_VOID Usage(IMG_CHAR	*	pszCmd);
static IMG_VOID CompleteFunc(	void			*	pContext,
								long				i32Read,
								unsigned long		ui32Address,
								unsigned char	*	pui8Buffer,
								unsigned long		ui32NumBytesTransferred,
								unsigned long		ui32Status );

/*!
******************************************************************************

 @Function				MainTask

******************************************************************************/

void MainTask( void )
{
	IMG_INT					i,	j;
	IMG_INT					iTimeout = 1;
	IMG_INT					iCancelled = 0;

	SCBM_SETTINGS_T			portSettings;
	TEST_sAsyncCtx			asyncCallbackCtxA, asyncCallbackCtxQA[ NUM_IO_BLOCKS * 2 ];
	SCBM_ASYNC_T			asyncA;
	unsigned long			ui32Return;

	SYS_sConfig				sConfig;
	IMG_MEMSET( &sConfig, 0, sizeof( SYS_sConfig ) );

	sConfig.bSetupSystemClock				= IMG_TRUE;
	sConfig.sSystemClock.eMetaClockSource	= CLOCK_SOURCE_PLL;
	sConfig.sSystemClock.ePLLSource			= CLOCK_SOURCE_XTAL1;
	sConfig.sSystemClock.ui32TargetFreq_fp	= (g_sTestDescription.ui32PLLFreq << 20);

	SYS_Configure(&sConfig);

	SYS_sSCBConfig			sSCBConfig;
	IMG_MEMSET( &sSCBConfig, 0, sizeof( SYS_sSCBConfig ) );
	if(g_sTestDescription.ui32ClockSrc==0)
	{
		sSCBConfig.bOverrideClockSource = IMG_FALSE;
	}
	else
	{
		sSCBConfig.bOverrideClockSource = IMG_TRUE;
		sSCBConfig.eClockSource = CLOCK_SOURCE_SYS_UNDELETED;
	}
	sSCBConfig.asBlockConfig[0].bEnable = IMG_TRUE;
	sSCBConfig.asBlockConfig[0].bConfigure = IMG_TRUE;
	sSCBConfig.asBlockConfig[1].bEnable = IMG_TRUE;
	sSCBConfig.asBlockConfig[1].bConfigure = IMG_TRUE;
	sSCBConfig.asBlockConfig[2].bEnable = IMG_TRUE;
	sSCBConfig.asBlockConfig[2].bConfigure = IMG_TRUE;
	SYS_ConfigureSCB(&sSCBConfig);

	if ( g_sTestDescription.fOut != (-1) )
	{
		read( g_sTestDescription.fOut, g_aui8PortWriteBuffer, g_sTestDescription.ui32Size );
	}

	if ( g_sTestDescription.fRef != (-1) )
	{
		read( g_sTestDescription.fRef, g_aui8PortRefBuffer, g_sTestDescription.ui32Size );
	}

	//prepare to initialise the SCBM bitrate
	portSettings.bitrate = g_sTestDescription.ui32BitRate;

	// set core clock (for FPGA - change for silicon) and busdelay
	portSettings.coreclock = g_sTestDescription.ui32CoreClockVal;
	portSettings.busdelay = 0;
	portSettings.ui32BlockIndex = g_sTestDescription.ui32BlockNum;

	// Initialise the port
	if ( SCBM_STATUS_SUCCESS != SCBMInit(&g_sPort, &portSettings, g_sIOBlocks, NUM_IO_BLOCKS ) )
	{
		__TBILogF("FAIL: initialisation of port failed!\n");
		return;
	}

	/* Test deinit and reinit */
	SCBMDeinit ( &g_sPort );

	if ( SCBM_STATUS_SUCCESS != SCBMInit(&g_sPort, &portSettings, g_sIOBlocks, NUM_IO_BLOCKS ) )
	{
		__TBILogF("FAIL: reinitialisation of port failed!\n");
		return;
	}

	//perform test
	switch( g_sTestDescription.ui32TestMode )
	{
		/************************************************/
		case TEST_ASYNC_READ_CTX:
		{
			asyncCallbackCtxA.ui32Status = SCBM_TEST_STATUS_ASYNC_DEFAULT;
			SCBM_ASYNC_CALLBACK(&asyncA, (SCBM_CALLBACKROUTINE_T*)&CompleteFunc, &asyncCallbackCtxA);

			// read from slave
			if (SCBM_STATUS_SUCCESS != SCBMRead(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortReadBuffer,
										g_sTestDescription.ui32Size, &g_ui32PortANumRead, &asyncA, SCBM_INF_TIMEOUT))
			{
				__TBILogF("FAIL: read failed!\n");
				return;
			}

			// wait for read to complete
			while (asyncCallbackCtxA.ui32Status == SCBM_TEST_STATUS_ASYNC_DEFAULT)
			{
				KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			}
			if (asyncCallbackCtxA.ui32Status != SCBM_STATUS_SUCCESS)
			{
				__TBILogF("FAIL: callback had incorrect status! (status = %d)\n", asyncCallbackCtxA.ui32Status);
				return;
			}
			if (asyncCallbackCtxA.ui32NumBytesTransferred != g_sTestDescription.ui32Size)
			{
				__TBILogF("FAIL: incorrect number of bytes transferred! (number = %d)\n", asyncCallbackCtxA.ui32NumBytesTransferred);
				return;
			}

			// read completed, compare message read from slave with expected MISO message
			for (j=0; j<(int)g_sTestDescription.ui32Size; j++)
			{
				if(g_aui8PortReadBuffer[j] != g_aui8PortRefBuffer[j])
				{
					__TBILogF("FAIL: Expected: %d, found %d\n", g_aui8PortRefBuffer[j], g_aui8PortReadBuffer[j]);
					return;
				}
			}

			__TBILogF("SUCCESS: asynchronous read operation using callback function!\n");
			break;
		}
		/************************************************/
		case TEST_ASYNC_READ_GET_RESULT:
		{
			SCBM_ASYNC_GET_RESULT(&asyncA);

			// read from slave
			if (SCBM_STATUS_SUCCESS != SCBMRead(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortReadBuffer,
										g_sTestDescription.ui32Size, &g_ui32PortANumRead, &asyncA, SCBM_INF_TIMEOUT))
			{
				__TBILogF("FAIL: read failed!\n");
				return;
			}

			//wait for read to complete
			if (SCBM_STATUS_SUCCESS != SCBMGetResult(&g_sPort, &g_i32PortARead, &g_ui32PortAAddress,
													&g_ui32PortANumRead, 1, SCBM_INF_TIMEOUT))
			{
				__TBILogF("FAIL: asynchronous read operation using API 'get result' function failed!\n");
				return;
			}
			if (g_ui32PortANumRead != g_sTestDescription.ui32Size)
			{
				__TBILogF("FAIL: incorrect number of bytes transferred! (number written = %d)\n", g_ui32PortANumRead);
				return;
			}

			// read completed, compare message read from slave with expected MISO message
			for (j=0; j<(int)g_sTestDescription.ui32Size; j++)
			{
				if(g_aui8PortReadBuffer[j] != g_aui8PortRefBuffer[j])
				{
					__TBILogF("FAIL: Expected: %d, found %d\n", g_aui8PortRefBuffer[j], g_aui8PortReadBuffer[j]);
					return;
				}
			}

			__TBILogF("SUCCESS: asynchronous read operation using API 'get result' function!\n");
			break;
		}
		/************************************************/
		case TEST_ASYNC_READ_IGNORE:
		{
			SCBM_ASYNC_FORGET(&asyncA);

			// read from slave
			if (SCBM_STATUS_SUCCESS != SCBMRead(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortReadBuffer,
										g_sTestDescription.ui32Size, &g_ui32PortANumRead, &asyncA, SCBM_INF_TIMEOUT))
			{
				__TBILogF("FAIL: read failed!\n");
				return;
			}

			KRN_hibernate(&g_sTaskQueue, POLL_WAIT);

			__TBILogF("SUCCESS: asynchronous read operation ignoring result!\n");
			break;
		}
		/************************************************/
		case TEST_ASYNC_WRITE_CTX:
		{
			asyncCallbackCtxA.ui32Status = SCBM_TEST_STATUS_ASYNC_DEFAULT;
			SCBM_ASYNC_CALLBACK(&asyncA, (SCBM_CALLBACKROUTINE_T*)&CompleteFunc, &asyncCallbackCtxA);

			// write to slave
			if (SCBM_STATUS_SUCCESS != SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer,
										g_sTestDescription.ui32Size, &g_ui32PortANumWrite, &asyncA, SCBM_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			// wait for write to complete
			while (asyncCallbackCtxA.ui32Status == SCBM_TEST_STATUS_ASYNC_DEFAULT)
			{
				KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			}
			if (asyncCallbackCtxA.ui32Status != SCBM_STATUS_SUCCESS)
			{
				__TBILogF("FAIL: callback had incorrect status! (status = %d)\n", asyncCallbackCtxA.ui32Status);
				return;
			}
			if (asyncCallbackCtxA.ui32NumBytesTransferred != g_sTestDescription.ui32Size)
			{
				__TBILogF("FAIL: incorrect number of bytes transferred! (number = %d)\n", asyncCallbackCtxA.ui32NumBytesTransferred);
				return;
			}

			__TBILogF("SUCCESS: asynchronous write operation using callback function?\n");
			__TBILogF("     - Check written output\n");
			break;
		}
		/************************************************/
		case TEST_ASYNC_WRITE_GET_RESULT:
		{
			SCBM_ASYNC_GET_RESULT(&asyncA);

			// write to slave
			if (SCBM_STATUS_SUCCESS != SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer,
										g_sTestDescription.ui32Size, &g_ui32PortANumWrite, &asyncA, SCBM_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			//wait for write to complete
			if (SCBM_STATUS_SUCCESS != SCBMGetResult(&g_sPort, &g_i32PortARead, &g_ui32PortAAddress,
													&g_ui32PortANumWrite, 1, SCBM_INF_TIMEOUT))
			{
				__TBILogF("FAIL: asynchronous write operation using API 'get result' function failed!\n");
				return;
			}
			if (g_ui32PortANumWrite != g_sTestDescription.ui32Size)
			{
				__TBILogF("FAIL: incorrect number of bytes transferred! (number written = %d)\n", g_ui32PortANumWrite);
				return;
			}

			__TBILogF("SUCCESS: asynchronous write operation using API 'get result' function?\n");
			__TBILogF("     - Check written output\n");
			break;
		}
		/************************************************/
		case TEST_ASYNC_WRITE_IGNORE:
		{
			SCBM_ASYNC_FORGET(&asyncA);

			// write to slave
			if (SCBM_STATUS_SUCCESS != SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer,
										g_sTestDescription.ui32Size, &g_ui32PortANumWrite, &asyncA, SCBM_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			__TBILogF("SUCCESS: asynchronous write operation ignoring result?\n");
			__TBILogF("     - Check written output\n");
			break;
		}
		/************************************************/
		case TEST_ASYNC_QUEUE:
		{
#define NUM_TRANSACTIONS 6
			for (i=0; i<NUM_TRANSACTIONS; i++)
				asyncCallbackCtxQA[i].ui32Status = SCBM_TEST_STATUS_ASYNC_DEFAULT;

			// write to slave multiple times, 2 write, 2 reads, 1 write, 1 read
			for (i=0; i<2; i++)
			{
				SCBM_ASYNC_CALLBACK(&asyncA, (SCBM_CALLBACKROUTINE_T*)&CompleteFunc, &asyncCallbackCtxQA[i]);
				if (SCBM_STATUS_SUCCESS != SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer,
										g_sTestDescription.ui32Size, &g_ui32PortANumWrite, &asyncA, SCBM_INF_TIMEOUT))
				{
					__TBILogF("FAIL: write failed!\n");
					return;
				}
			}
			for (i=2; i<4; i++)
			{
				SCBM_ASYNC_CALLBACK(&asyncA, (SCBM_CALLBACKROUTINE_T*)&CompleteFunc, &asyncCallbackCtxQA[i]);
				if (SCBM_STATUS_SUCCESS != SCBMRead(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortReadBuffer,
										g_sTestDescription.ui32Size, &g_ui32PortANumRead, &asyncA, SCBM_INF_TIMEOUT))
				{
					__TBILogF("FAIL: read failed!\n");
					return;
				}
			}
			SCBM_ASYNC_CALLBACK(&asyncA, (SCBM_CALLBACKROUTINE_T*)&CompleteFunc, &asyncCallbackCtxQA[i]);
			if (SCBM_STATUS_SUCCESS != SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer,
									g_sTestDescription.ui32Size, &g_ui32PortANumWrite, &asyncA, SCBM_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}
			i++;
			SCBM_ASYNC_CALLBACK(&asyncA, (SCBM_CALLBACKROUTINE_T*)&CompleteFunc, &asyncCallbackCtxQA[i]);
			if (SCBM_STATUS_SUCCESS != SCBMRead(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer,
									g_sTestDescription.ui32Size, &g_ui32PortANumRead, &asyncA, SCBM_INF_TIMEOUT))
			{
				__TBILogF("FAIL: read failed\n");
				return;
			}

			// wait for all the writes to complete
			for (i=0; i<NUM_TRANSACTIONS; i++)
			{
				while (asyncCallbackCtxQA[i].ui32Status == SCBM_TEST_STATUS_ASYNC_DEFAULT)
				{
					KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
				}
				if (asyncCallbackCtxQA[i].ui32Status != SCBM_STATUS_SUCCESS)
				{
					__TBILogF("FAIL: callback had incorrect status! (status = %d)\n", asyncCallbackCtxQA[i].ui32Status);
					return;
				}
				if (asyncCallbackCtxQA[i].ui32NumBytesTransferred != g_sTestDescription.ui32Size)
				{
					__TBILogF("FAIL: incorrect number of bytes transferred! (number written = %d) (transaction %d)\n", asyncCallbackCtxQA[i].ui32NumBytesTransferred, i);
					return;
				}
			}

			__TBILogF("SUCCESS: queuing multiple operations succeeded!\n", i, NUM_IO_BLOCKS);

			break;
		}
		/************************************************/
		case TEST_ASYNC_TOO_MANY_QUEUED:
		{
			for (i=0; i<(NUM_IO_BLOCKS*2); i++)
				asyncCallbackCtxQA[i].ui32Status = SCBM_TEST_STATUS_ASYNC_DEFAULT;

			// write to slave multiple times
			i = 0;
			do {
				SCBM_ASYNC_CALLBACK(&asyncA, (SCBM_CALLBACKROUTINE_T*)&CompleteFunc, &asyncCallbackCtxQA[i]);
				ui32Return = SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer,
										g_sTestDescription.ui32Size, &g_ui32PortANumWrite, &asyncA, SCBM_INF_TIMEOUT);
				i ++;
				if (i == (NUM_IO_BLOCKS*2))
				{
					__TBILogF("FAIL: not enough IO blocks to finish test!\n");
					return;
				}
			} while (ui32Return == SCBM_STATUS_SUCCESS);

			if (ui32Return != SCBM_STATUS_WOULD_BLOCK)
			{
				__TBILogF("FAIL: queue ending for unknown reason!\n");
				return;
			}

			// wait for all the non blocked writes to complete
			for (j=0; j<(i-1); j++)
			{
				while (asyncCallbackCtxQA[j].ui32Status == SCBM_TEST_STATUS_ASYNC_DEFAULT)
				{
					KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
				}
				if (asyncCallbackCtxQA[j].ui32Status != SCBM_STATUS_SUCCESS)
				{
					__TBILogF("FAIL: callback had incorrect status! (status = %d)\n", asyncCallbackCtxQA[j].ui32Status);
					return;
				}
				if (asyncCallbackCtxQA[j].ui32NumBytesTransferred != g_sTestDescription.ui32Size)
				{
					__TBILogF("FAIL: incorrect number of bytes transferred! (number written = %d)\n", asyncCallbackCtxQA[j].ui32NumBytesTransferred);
					return;
				}
			}

			__TBILogF("SUCCESS: queuing too many operations succeed after %d operations requested whilst only %d operations can be queued!\n", i, NUM_IO_BLOCKS);

			break;
		}
		/************************************************/
		case TEST_TIMEOUT_READ_SYNC:
		{
			// synchronous read iTimeout
			if (SCBM_STATUS_TIMEOUT != SCBMRead(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortReadBuffer,
													g_sTestDescription.ui32Size, &g_ui32PortANumRead, NULL, iTimeout))
			//if (SCBM_STATUS_TIMEOUT != SCBMRead(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortReadBuffer,
			//										g_sTestDescription.ui32Size, &g_ui32PortANumRead, NULL, 100000))
			{
				__TBILogF("FAIL: timeout on synchronous read failed! Ensure bitrate for script is lower than slave bitrate\n");

				return;
			}

			__TBILogF("SUCCESS: timeout on synchronous read!\n");
			break;
		}
		/************************************************/
		case TEST_TIMEOUT_WRITE_SYNC:
		{
			// synchronous write iTimeout
			if (SCBM_STATUS_TIMEOUT != SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer,
													g_sTestDescription.ui32Size, &g_ui32PortANumWrite, NULL, iTimeout))
			{
				__TBILogF("FAIL: timeout on synchronous write failed! Ensure bitrate for script is lower than slave bitrate\n");
				return;
			}

			__TBILogF("SUCCESS: timeout on synchronous write!\n");
			break;
		}
		/************************************************/
		case TEST_TIMEOUT_GET_RESULT:
		{
			SCBM_ASYNC_GET_RESULT(&asyncA);

			// write to slave
			if (SCBM_STATUS_SUCCESS != SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer,
										g_sTestDescription.ui32Size, &g_ui32PortANumWrite, &asyncA, SCBM_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			//don't wait for write to complete
			if (SCBM_STATUS_TIMEOUT != SCBMGetResult(&g_sPort, &g_i32PortARead, &g_ui32PortAAddress,
													&g_ui32PortANumWrite, 1, iTimeout))
			{
				__TBILogF("FAIL: timeout on API 'get result' function failed!\n");
				return;
			}

			__TBILogF("SUCCESS: timeout on API 'get result' function!\n");
			break;
		}
		/************************************************/
		case TEST_CANCEL_ASYNC_THEN_QUEUE_NEW:
		{
			asyncCallbackCtxA.ui32Status = SCBM_TEST_STATUS_ASYNC_DEFAULT;
			SCBM_ASYNC_CALLBACK(&asyncA, (SCBM_CALLBACKROUTINE_T*)&CompleteFunc, &asyncCallbackCtxA);

			// write to slave
			if (SCBM_STATUS_SUCCESS != SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer,
										g_sTestDescription.ui32Size, &g_ui32PortANumWrite, &asyncA, SCBM_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			// cancel
			SCBMCancel(&g_sPort);

			// wait for write to complete
			while (asyncCallbackCtxA.ui32Status == SCBM_TEST_STATUS_ASYNC_DEFAULT)
			{
				KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			}
			if (asyncCallbackCtxA.ui32Status != SCBM_STATUS_CANCEL)
			{
				__TBILogF("FAIL: callback had incorrect status! (status = %d)\n", asyncCallbackCtxA.ui32Status);
				return;
			}
			else
			{
				__TBILogF("SUCCESS: asynchronous write function cancelled!\n");
			}

			// write to slave
			asyncCallbackCtxA.ui32Status = SCBM_TEST_STATUS_ASYNC_DEFAULT;
			if (SCBM_STATUS_SUCCESS != SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer,
										g_sTestDescription.ui32Size, &g_ui32PortANumWrite, &asyncA, SCBM_INF_TIMEOUT))
			{
				__TBILogF("FAIL: write failed!\n");
				return;
			}

			// wait for write to complete
			while (asyncCallbackCtxA.ui32Status == SCBM_TEST_STATUS_ASYNC_DEFAULT)
			{
				KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			}
			if (asyncCallbackCtxA.ui32Status != SCBM_STATUS_SUCCESS)
			{
				__TBILogF("FAIL: callback had incorrect status! (status = %d)\n", asyncCallbackCtxA.ui32Status);
				return;
			}
			if (asyncCallbackCtxA.ui32NumBytesTransferred != g_sTestDescription.ui32Size)
			{
				__TBILogF("FAIL: incorrect number of bytes transferred! (number written = %d)\n", asyncCallbackCtxA.ui32NumBytesTransferred);
				return;
			}

			__TBILogF("SUCCESS: asynchronous write operation after cancelled operation?\n");
			__TBILogF("     - Check written output\n");

			break;
		}
		/************************************************/
		case TEST_CANCEL_ASYNC_QUEUE:
		{
			for (i=0; i<NUM_IO_BLOCKS; i++)
				asyncCallbackCtxQA[i].ui32Status = SCBM_TEST_STATUS_ASYNC_DEFAULT;

			// write to slave multiple times
			for (i=0; i<NUM_IO_BLOCKS; i++)
			{
				SCBM_ASYNC_CALLBACK(&asyncA, (SCBM_CALLBACKROUTINE_T*)&CompleteFunc, &asyncCallbackCtxQA[i]);
				if (SCBM_STATUS_SUCCESS != SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer,
										g_sTestDescription.ui32Size, &g_ui32PortANumWrite, &asyncA, SCBM_INF_TIMEOUT))
				{
					__TBILogF("FAIL: write failed!\n");
					return;
				}
			}

			// cancel
			SCBMCancel(&g_sPort);

			// wait for all the writes to complete
			for (i=0; i<NUM_IO_BLOCKS; i++)
			{
				while (asyncCallbackCtxQA[i].ui32Status == SCBM_TEST_STATUS_ASYNC_DEFAULT)
				{
					KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
				}
				if ((asyncCallbackCtxQA[i].ui32Status != SCBM_STATUS_SUCCESS) &&
					(asyncCallbackCtxQA[i].ui32Status != SCBM_STATUS_CANCEL))
				{
					__TBILogF("FAIL: callback had incorrect status! (status = %d)\n", asyncCallbackCtxQA[i].ui32Status);
					return;
				}
				if (asyncCallbackCtxQA[i].ui32Status == SCBM_STATUS_CANCEL)
				{
					// we have cancelled at least 1 job
					iCancelled++;
				}
				else // we shouldn't have cancelled any yet
				{
					if (iCancelled)
					{
						__TBILogF("FAIL: job that should have been cancelled is not!\n");
						return;
					}
				}
				if (asyncCallbackCtxQA[i].ui32NumBytesTransferred > g_sTestDescription.ui32Size)
				{
					__TBILogF("FAIL: incorrect number of bytes transferred! (number written = %d)\n",
								asyncCallbackCtxQA[i].ui32NumBytesTransferred);
					return;
				}
			}

			if (!iCancelled)
			{
				__TBILogF("FAIL: none of the queue is cancelled!\n");
				return;
			}

			__TBILogF("SUCCESS: last %d of %d jobs have been cancelled!\n", iCancelled, NUM_IO_BLOCKS);
			break;
		}
		/************************************************/
		case TEST_NACK_READ:
		case TEST_NACK_WRITE:
		{
			__TBILogF("Disconnect the slave to force NACKs\n");
			asyncCallbackCtxA.ui32Status = SCBM_TEST_STATUS_ASYNC_DEFAULT;
			SCBM_ASYNC_CALLBACK(&asyncA, (SCBM_CALLBACKROUTINE_T*)&CompleteFunc, &asyncCallbackCtxA);

			if ( g_sTestDescription.ui32TestMode == TEST_NACK_WRITE )
			{
				// write to slave
				if (SCBM_STATUS_SUCCESS != SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer,
											g_sTestDescription.ui32Size, &g_ui32PortANumWrite, &asyncA, SCBM_INF_TIMEOUT))
				{
					__TBILogF("FAIL: write failed!\n");
					return;
				}
			}
			else
			{
				// read from slave
				if (SCBM_STATUS_SUCCESS != SCBMRead(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer,
											g_sTestDescription.ui32Size, &g_ui32PortANumWrite, &asyncA, SCBM_INF_TIMEOUT))
				{
					__TBILogF("FAIL: read failed!\n");
					return;
				}
			}

			// wait for write to complete
			while (asyncCallbackCtxA.ui32Status == SCBM_TEST_STATUS_ASYNC_DEFAULT)
			{
				KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
			}
			if (asyncCallbackCtxA.ui32Status != SCBM_STATUS_SUCCESS)
			{
				__TBILogF("FAIL: callback had incorrect status! (status = %d)\n", asyncCallbackCtxA.ui32Status);
				return;
			}
			if ( SCBMGetErrorStatus( &g_sPort ) != SCBM_ERR_ADDRESS_ERROR )
			{
				__TBILogF("FAIL: SCBMGetErrorStatus returned incorrect status! (status = %d)\n ", SCBMGetErrorStatus( &g_sPort ) );
				return;
			}
			if (asyncCallbackCtxA.ui32NumBytesTransferred != 0)
			{
				__TBILogF("FAIL: incorrect number of bytes transferred! (number = %d)\n", asyncCallbackCtxA.ui32NumBytesTransferred);
				return;
			}

			__TBILogF("SUCCESS: NACK handled correctly\n");
			break;
		}
		/************************************************/
		case TEST_ASYNC_READ_QUEUE:
		{
			for (i = 0; i < NUM_QUEUED_READS; ++i)
				asyncCallbackCtxQA[i].ui32Status = SCBM_TEST_STATUS_ASYNC_DEFAULT;

			for (i = 0; i < NUM_QUEUED_READS; ++i)
			{
				SCBM_ASYNC_CALLBACK(&asyncA, (SCBM_CALLBACKROUTINE_T *)&CompleteFunc, &asyncCallbackCtxQA[i]);
				if (SCBMRead(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortReadBuffer, g_sTestDescription.ui32Size, &g_ui32PortANumRead,
							 &asyncA, SCBM_INF_TIMEOUT) != SCBM_STATUS_SUCCESS)
				{
					__TBILogF("FAIL: read %d failed!\n", i);
					return;
				}
			}

			for (i = 0; i < NUM_QUEUED_READS; ++i)
			{
				while (asyncCallbackCtxQA[i].ui32Status == SCBM_TEST_STATUS_ASYNC_DEFAULT)
				{
					KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
				}
				if (asyncCallbackCtxQA[i].ui32Status != SCBM_STATUS_SUCCESS)
				{
					__TBILogF("FAIL: callback had incorrect status! (status = %d)\n", asyncCallbackCtxQA[i].ui32Status);
					return;
				}
				if (asyncCallbackCtxQA[i].ui32NumBytesTransferred != g_sTestDescription.ui32Size)
				{
					__TBILogF("FAIL: incorrect number of bytes transferred! (number written = %d) (transaction %d)\n", asyncCallbackCtxQA[i].ui32NumBytesTransferred, i);
					return;
				}
			}

			__TBILogF("SUCCESS: queuing multiple read operations succeeded!\n");

			break;
		}
		/************************************************/
		case TEST_ASYNC_WRITE_QUEUE:
		{
			for (i = 0; i < NUM_QUEUED_WRITES; ++i)
				asyncCallbackCtxQA[i].ui32Status = SCBM_TEST_STATUS_ASYNC_DEFAULT;

			// write to slave multiple times
			for (i = 0; i < NUM_QUEUED_WRITES; ++i)
			{
				SCBM_ASYNC_CALLBACK(&asyncA, (SCBM_CALLBACKROUTINE_T *)&CompleteFunc, &asyncCallbackCtxQA[i]);
				if (SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer, g_sTestDescription.ui32Size, &g_ui32PortANumWrite,
							  &asyncA, SCBM_INF_TIMEOUT) != SCBM_STATUS_SUCCESS)
				{
					__TBILogF("FAIL: write %d failed!\n", i);
					return;
				}
			}

			// wait for all the writes to complete
			for (i = 0; i < NUM_QUEUED_WRITES; i++)
			{
				while (asyncCallbackCtxQA[i].ui32Status == SCBM_TEST_STATUS_ASYNC_DEFAULT)
				{
					KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
				}
				if (asyncCallbackCtxQA[i].ui32Status != SCBM_STATUS_SUCCESS)
				{
					__TBILogF("FAIL: callback had incorrect status! (status = %d)\n", asyncCallbackCtxQA[i].ui32Status);
					return;
				}
				if (asyncCallbackCtxQA[i].ui32NumBytesTransferred != g_sTestDescription.ui32Size)
				{
					__TBILogF("FAIL: incorrect number of bytes transferred! (number written = %d) (transaction %d)\n", asyncCallbackCtxQA[i].ui32NumBytesTransferred, i);
					return;
				}
			}

			__TBILogF("SUCCESS: queuing multiple write operations succeeded!\n");

			break;
		}
		/************************************************/
		case TEST_SYNC_READ_STRESS:
		{
			while (jTemp < NUM_STRESS_READS)
			{
				for (i = 0; i < NUM_QUEUED_READS; ++i)
				{
					if (SCBMRead(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortReadBuffer, g_sTestDescription.ui32Size, &g_ui32PortANumRead,
							 NULL, SCBM_INF_TIMEOUT) != SCBM_STATUS_SUCCESS)
					{
						__TBILogF("FAIL: read %d failed!\n", jTemp);
						return;
					}
					if (g_ui32PortANumRead != g_sTestDescription.ui32Size)
					{
						__TBILogF("FAIL: incorrect number of bytes transferred! (number read = %d) (transaction %d)\n", g_ui32PortANumRead, jTemp);
						return;
					}
					++jTemp;
				}
			}
			__TBILogF("SUCCESS: %d read stressing operations succeeded!\n", NUM_STRESS_READS);
			break;
		}
		/************************************************/
		case TEST_SYNC_WRITE_STRESS:
		{
			j = 0;
			while (j < NUM_STRESS_WRITES)
			{
				for (i = 0; i < NUM_QUEUED_WRITES; ++i)
				{
					if (SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer, g_sTestDescription.ui32Size, &g_ui32PortANumWrite,
							NULL, SCBM_INF_TIMEOUT) != SCBM_STATUS_SUCCESS)
					{
						__TBILogF("FAIL: write %d failed!\n", j);
						return;
					}
					if (g_ui32PortANumWrite != g_sTestDescription.ui32Size)
					{
						__TBILogF("FAIL: incorrect number of bytes transferred! (number written = %d) (transaction %d)\n", g_ui32PortANumWrite, j);
						return;
					}
					++j;
				}
			}
			__TBILogF("SUCCESS: %d write stressing operations succeeded!\n", NUM_STRESS_WRITES);
			break;
		}
		/************************************************/
		case TEST_SYNC_READ_WRITE_LENGTH:
		{
			/* Test varying length reads */
			for (j = 1; j <= (int)g_sTestDescription.ui32Size; ++j)
			{
				/* Fire off the reads */
				for (i = 0; i < NUM_QUEUED_READS; ++i)
				{
					if (SCBMRead(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortReadBuffer, j, &g_ui32PortANumRead, NULL,
							 SCBM_INF_TIMEOUT) != SCBM_STATUS_SUCCESS)
					{
						__TBILogF("FAIL: read of %d bytes failed!\n", j);
						return;
					}
					if ((int)g_ui32PortANumRead != j)
					{
						__TBILogF("FAIL: incorrect number of bytes transferred! (%d instead of %d)\n", g_ui32PortANumRead, j);
						return;
					}
				}
			}

			/* Test varying length writes */
			for (j = 1; j <= (int)g_sTestDescription.ui32Size; ++j)
			{
				/* Fire off the writes */
				for (i = 0; i < NUM_QUEUED_WRITES; ++i)
				{
					asyncCallbackCtxQA[i].ui32Status = SCBM_TEST_STATUS_ASYNC_DEFAULT;
					SCBM_ASYNC_CALLBACK(&asyncA, (SCBM_CALLBACKROUTINE_T *)&CompleteFunc, &asyncCallbackCtxQA[i]);
					if (SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer, j, &g_ui32PortANumWrite, NULL,
								SCBM_INF_TIMEOUT) != SCBM_STATUS_SUCCESS)
					{
						__TBILogF("FAIL: write of %d bytes failed!\n", j);
						return;
					}
					if ((int)g_ui32PortANumWrite != j)
					{
						__TBILogF("FAIL: incorrect number of bytes transferred! (%d instead of %d)\n", g_ui32PortANumWrite, j);
						return;
					}
				}
			}

			__TBILogF("SUCCESS: Varying length transactions succeeded !\n");
			break;
		}
		/************************************************/
		case TEST_SYNC_READ_WRITE_STRESS:
		{
			j = 0;
			while (j < NUM_STRESS_READS + NUM_STRESS_WRITES)
			{
				for (i = 0; i < NUM_QUEUED_READS; ++i)
				{
					if (SCBMRead(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortReadBuffer, g_sTestDescription.ui32Size, &g_ui32PortANumRead,
								 NULL, SCBM_INF_TIMEOUT) != SCBM_STATUS_SUCCESS)
					{
						__TBILogF("FAIL: read %d failed!\n", j);
						return;
					}
					if (g_ui32PortANumRead != g_sTestDescription.ui32Size)
					{
						__TBILogF("FAIL: incorrect number of bytes read! (number written = %d) (transaction  %d)\n", g_ui32PortANumRead, j);
						return;
					}
					++j;
				}

				for (i = 0; i < NUM_QUEUED_WRITES; ++i)
				{
					if (SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer, g_sTestDescription.ui32Size, &g_ui32PortANumWrite,
							NULL, SCBM_INF_TIMEOUT) != SCBM_STATUS_SUCCESS)
					{
						__TBILogF("FAIL: write %d failed!\n", j);
						return;
					}
					if (g_ui32PortANumWrite != g_sTestDescription.ui32Size)
					{
						__TBILogF("FAIL: incorrect number of bytes written! (number written = %d) (transaction %d)\n", g_ui32PortANumWrite, j);
						return;
					}
					++j;
				}
			}
			__TBILogF("SUCCESS: %d read and write operations succeeded!\n", NUM_STRESS_READS + NUM_STRESS_WRITES);
			break;
		}
		/************************************************/
		case TEST_TEN_BIT_ADDRESSING:
		{
			__TBILogF("Writing to 7bit slave at address 0x%X...\n", (0x7 << 4 ) & (0x2 << 2) & ((g_sTestDescription.sPortA.ui32SlaveAddress >> 8) & 0x3) );
			if ( SCBMWrite( &g_sPort,
							g_sTestDescription.sPortA.ui32SlaveAddress,
							g_aui8PortWriteBuffer,
							g_sTestDescription.ui32Size,
							&g_ui32PortANumWrite,
							NULL,
							SCBM_INF_TIMEOUT ) != SCBM_STATUS_SUCCESS )
			{
				__TBILogF("FAIL: write failed!\n" );
				return;
			}
			if ( g_ui32PortANumWrite != g_sTestDescription.ui32Size )
			{
				__TBILogF("FAIL: incorrect number of bytes written! (number written = %d)\n", g_ui32PortANumWrite );
				return;
			}

			__TBILogF("SUCCESS: Check the number of bytes received by the slave is %d and that the first byte of data is 0x%X\n",
					  g_sTestDescription.ui32Size + 1,
					  (g_sTestDescription.sPortA.ui32SlaveAddress & 0xFF ) );

			break;
		}
		/************************************************/
		case TEST_INACTIVE_BUS:
		{
			__TBILogF("Disconnect the slave to force an inactive bus interrupt\n");
			while ( 1 )
			{
				asyncCallbackCtxA.ui32Status = SCBM_TEST_STATUS_ASYNC_DEFAULT;
				SCBM_ASYNC_CALLBACK(&asyncA, (SCBM_CALLBACKROUTINE_T*)&CompleteFunc, &asyncCallbackCtxA);

				// write to slave
				if (SCBM_STATUS_SUCCESS != SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_aui8PortWriteBuffer,
											g_sTestDescription.ui32Size, &g_ui32PortANumWrite, &asyncA, SCBM_INF_TIMEOUT))
				{
					__TBILogF("FAIL: write failed!\n");
					return;
				}

				// wait for write to complete
				while (asyncCallbackCtxA.ui32Status == SCBM_TEST_STATUS_ASYNC_DEFAULT)
				{
					KRN_hibernate(&g_sTaskQueue, POLL_WAIT);
				}
				if (asyncCallbackCtxA.ui32Status != SCBM_STATUS_SUCCESS)
				{
					__TBILogF("FAIL: callback had incorrect status! (status = %d)\n", asyncCallbackCtxA.ui32Status);
					return;
				}
				else // A transaction completed, check the data was sent.
				{
					unsigned long errorStatus = SCBMGetErrorStatus( &g_sPort );
					 // The transaction failed, get the more detailed error and check it is a bus error.
					if ( errorStatus == SCBM_ERR_BUS_INACTIVE )
					{
						__TBILogF("SUCCESS: 'Bus Inactive' interrupt handled correctly\n");
						return;
					}
					else if ( errorStatus == SCBM_ERR_TRANSFER_ERROR )
					{
						__TBILogF("SUCCESS: 'Write Acknowledge Error' interrupt handled correctly\n");
						return;
					}
					else if ( errorStatus == SCBM_ERR_ADDRESS_ERROR )
					{
						__TBILogF("SUCCESS: 'Address Acknowledge Error' interrupt handled correctly\n");
						return;
					}
					else if ( g_sTestDescription.ui32Size != asyncCallbackCtxA.ui32NumBytesTransferred )
					{
						__TBILogF("FAIL: incorrect number of bytes written! (number written = %d)\n", g_ui32PortANumWrite );
						return;
					}
				}
			}
			break;
		}
		/************************************************/
		case TEST_MIRICS_TUNER:
		{
			img_uint8	tunerData[4] = { 0, 0x1, 0x2 << 5, 0 };
			if ( SCBM_STATUS_SUCCESS != SCBMWrite( &g_sPort, 0x4C, tunerData,
													4, &g_ui32PortANumWrite, NULL, SCBM_INF_TIMEOUT ) )
			{
				__TBILogF("FAIL: write to tuner failed!\n" );
				return;
			}
			else
			{
				__TBILogF("SUCCESS: Dummy write to tuner ack'd!\n" );
				return;
			}
			break;
		}
		case TEST_DEBUG:
		{
			img_uint32	ui32NextPct = 1;
			while ( g_ui32NumReads < 1000000 )
			{
				if ( SCBM_STATUS_SUCCESS != SCBMRead(	&g_sPort,
														g_sTestDescription.sPortA.ui32SlaveAddress,
														g_aui8PortReadBuffer,
														g_sTestDescription.ui32Size,
														&g_ui32PortANumRead,
														IMG_NULL,
														SCBM_INF_TIMEOUT ) )
				{
					__TBILogF("FAIL: read %d failed!\n", g_ui32NumReads );
					return;
				}
				if ( g_ui32PortANumRead != g_sTestDescription.ui32Size )
				{
					__TBILogF("FAIL: incorrect number of bytes transferred! (number read = %d) (transaction %d)\n", g_ui32PortANumRead, g_ui32NumReads);
					return;
				}
				++g_ui32NumReads;
				if ( (g_ui32NumReads / 10000) == ui32NextPct )
				{
					__TBILogF("INFO: %d percent complete.\n", ui32NextPct );
					++ui32NextPct;
				}
			}
			break;
		}
		case TEST_DEBUG3:
		{
			img_uint32 ui32NextPct = 1;
			while ( g_ui32NumReads < 1000000 )
			{
				if ( SCBM_STATUS_SUCCESS != SCBMWrite(	&g_sPort,
														g_sTestDescription.sPortA.ui32SlaveAddress,
														g_aui8PortWriteBuffer,
														g_sTestDescription.ui32Size,
														&g_ui32PortANumWrite,
														IMG_NULL,
														SCBM_INF_TIMEOUT ) )
				{
					__TBILogF("FAIL: read %d failed!\n", g_ui32NumReads );
					return;
				}
				if ( g_ui32PortANumWrite != g_sTestDescription.ui32Size )
				{
					__TBILogF("FAIL: incorrect number of bytes transferred! (number written = %d) (transaction %d)\n", g_ui32PortANumWrite, g_ui32NumReads );
					return;
				}
				++g_ui32NumReads;
				if ( (g_ui32NumReads / 10000) == ui32NextPct )
				{
					__TBILogF("INFO: %d percent complete.\n", ui32NextPct );
					++ui32NextPct;
				}
			}
			break;
		}
		case TEST_DEBUG2:
		{
			img_uint8	aui8ReadBuffer[10];
			img_uint32	ui32NextPct = 1;
			volatile unsigned long i;
			DQ_T		sTaskQueue;

			DQ_init( &sTaskQueue );
			while ( g_ui32NumReads < 1000000 )
			{
//				KRN_hibernate( &sTaskQueue, g_ui32DelayMS );
				for ( i = 0; i < g_ui32DelayMS * 1000; ++i )
				{
				}
				*(volatile unsigned long *)0x02004898 = 0x3F;
				if ( SCBM_STATUS_SUCCESS != SCBMRead(	&g_sPort,
														g_sTestDescription.sPortA.ui32SlaveAddress,
														aui8ReadBuffer,
														1,
														&g_ui32PortANumWrite,
														IMG_NULL,
														SCBM_INF_TIMEOUT ) )
				{
					__TBILogF("FAIL: read %d failed!\n", g_ui32NumReads );
				}
				if ( SCBMGetErrorStatus( &g_sPort ) != SCBM_ERR_ADDRESS_ERROR )
				{
					__TBILogF("FAIL: Error status is not SCBM_ERR_ADDRESS_ERROR!\n" );
				}
				if ( g_ui32PortANumWrite != 0 )
				{
					__TBILogF("FAIL: read %d failed!\n", g_ui32NumReads );
				}
				++g_ui32NumReads;
				if ( (g_ui32NumReads / 10000) == ui32NextPct )
				{
					__TBILogF("INFO: %d percent complete.\n", ui32NextPct );
					++ui32NextPct;
				}
			}
			break;
		}
		/************************************************/
	}

	return; /* End of Main Task */
}

/*!
******************************************************************************

 @Function				CompleteFunc

******************************************************************************/
static void	CompleteFunc(	void			*	pContext,
							long				i32Read,
							unsigned long		ui32Address,
							unsigned char	*	pui8Buffer,
							unsigned long		ui32NumBytesTransferred,
							unsigned long		ui32Status )
{
	TEST_sAsyncCtx *psAsyncCtx = (TEST_sAsyncCtx *)pContext;

	psAsyncCtx->pContext				= pContext;
	psAsyncCtx->i32Read					= i32Read;
	psAsyncCtx->ui32Address				= ui32Address;
	psAsyncCtx->pui8Buffer				= pui8Buffer;
	psAsyncCtx->ui32NumBytesTransferred	= ui32NumBytesTransferred;
	psAsyncCtx->ui32Status				= ui32Status;

    return;
}

/*!
******************************************************************************

 @Function				ParseCommandLine

******************************************************************************/
static void ParseCommandLine(int argc, char *argv[])
{
    char *cmd;
    char *option;
    char  tempString[20], *fileName;
    unsigned int tempAddr;

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

    /* Initialise file pointers to 'null' (-1) */
    g_sTestDescription.fOut = (-1);
    g_sTestDescription.fRef = (-1);
	g_sTestDescription.ui32CoreClockVal = 0;

    verboseLog("Parsing command line - %d arguments\n", argc);

    /* Parse command line */
    while ( argc > 1 )
    {
        if ( *argv[0] == '-' )
        {
            option = (*argv);

			if ( strncmp( option, "-test", 5 ) == 0 )
            {
            	/* Configure test */
                argv++;
                argc--;
                sscanf( *argv, "%d", &( g_sTestDescription.ui32TestMode ) );
				if ( g_sTestDescription.ui32TestMode > NUM_TESTS )
				{
					verboseLog("Error: incorrect test number. Should be from 1 to %d\n", NUM_TESTS );
					Usage(cmd);
				}

                verboseLog("Test number %d\n", g_sTestDescription.ui32TestMode);
            }

			else if ( strncmp( option, "-size", 5 ) == 0 )
            {
            	/* Configure job size */
                argv++;
                argc--;
                sscanf( *argv, "%d", &( g_sTestDescription.ui32Size ) );
                if ( g_sTestDescription.ui32Size > MAX_MESSAGE_SIZE )
                {
					verboseLog("Job size cannot exceed %d bytes\n", MAX_MESSAGE_SIZE);
					g_sTestDescription.ui32Size = MAX_MESSAGE_SIZE;
				}
                verboseLog("Each job contains %d bytes\n", g_sTestDescription.ui32Size);
            }

			else if ( strncmp( option, "-addrA", 6 ) == 0 )
            {
            	/* Configure slave address */
                argv++;
                argc--;
                sscanf( *argv, "%x", &( tempAddr ) );

				/* Check that the user-specified value is not greater than 10 bits. */
                if ( tempAddr != ( tempAddr & 0x000003ff ) )
                {
					verboseLog("Error:	Slave address should be no larger than 10 bits\n");
					verboseLog("		Maximum hex value: 3ff\n");
					Usage( cmd );
				}

				//assign botton 8 bits of the 32 bit value 'tempAddr' to the unsigned char 'slaveAddress'.
                g_sTestDescription.sPortA.ui32SlaveAddress = (IMG_UINT32)( tempAddr & 0x000003ff );
                verboseLog("Slave address is 0x%x\n", (g_sTestDescription.sPortA.ui32SlaveAddress));
            }

			else if ( strncmp( option, "-br", 3 ) == 0 )
            {
            	//Configure bit rate
                argv++;
                argc--;
                sscanf( *argv, "%ud", &( g_sTestDescription.ui32BitRate ) );

				verboseLog( "BCR value set to %d\n", g_sTestDescription.ui32BitRate );
            }
            else if ( strncmp( option, "-out", 4 ) == 0 )
            {
            	//Open output file
            	fileName = tempString;
                argv++;
                argc--;
                sscanf( *argv, "%s", fileName );

                g_sTestDescription.fOut = open( fileName, O_RDONLY, 0744 );

                if ( g_sTestDescription.fOut == -1 )
                {
                	__TBILogF("Error opening output file %s\n", fileName);
                	Usage( cmd );
                }

                verboseLog("Output file name %s\n", fileName);
            }
			else if ( strncmp( option, "-ref", 4 ) == 0 )
            {
            	//Open output file
            	fileName = tempString;
                argv++;
                argc--;
                sscanf( *argv, "%s", fileName );

                g_sTestDescription.fRef = open( fileName, O_RDONLY, 0744 );

                if ( g_sTestDescription.fRef == -1 )
                {
                	__TBILogF("Error opening reference file %s\n", fileName);
                	Usage( cmd );
                }

                verboseLog("Reference file name %s\n", fileName);
            }
			else if ( strncmp( option, "-block", 6 ) == 0 )
			{
				argv++;
				argc--;
				sscanf( *argv, "%ud", &g_sTestDescription.ui32BlockNum );

				verboseLog("Block number is %u\n", g_sTestDescription.ui32BlockNum );
			}
			else if ( strncmp( option, "-clksrc", 7 ) == 0 )
			{
				argv++;
				argc--;
				sscanf( *argv, "%u", &g_sTestDescription.ui32ClockSrc );
				verboseLog("Clock source is %s\n", g_sTestDescription.ui32ClockSrc == 0 ? "XTAL1" : "sys_clk_undeleted" );
			}
			else if ( strncmp( option, "-coreclockfreq", 14 ) == 0 )
			{
				argv++;
				argc--;
				sscanf( *argv, "%u", &g_sTestDescription.ui32CoreClockVal );
				verboseLog("Clock source frequency is %u\n", g_sTestDescription.ui32CoreClockVal);
			}
			else if ( strncmp( option, "-pllfreq", 8 ) == 0 )
			{
				argv++;
				argc--;
				sscanf( *argv, "%u", &g_sTestDescription.ui32PLLFreq );
				verboseLog("PLL Frequency is %ud\n", g_sTestDescription.ui32PLLFreq );
			}
            else
            {
                Usage( cmd );
            }
        }
		else
		{
			Usage( cmd );
		}

        argv++;
        argc--;
    }
}

/*!
******************************************************************************

 @Function				Usage

******************************************************************************/
static void Usage(char *cmd)
{
    __TBILogF("\nUsage:  %s <..options..>\n", cmd);
    __TBILogF("          -test N			Test\n");
    __TBILogF("          -size N			Number of bytes in each job for PortA\n");
    __TBILogF("          -addrA N (hex)		Address of slave for PortA\n");
    __TBILogF("          -br N				bitrate value (in kHz)\n");
    __TBILogF("          -out <filename>	Output (MOSI) file\n");
    __TBILogF("          -ref <filename>	Reference file\n");
	__TBILogF("          -block N			Block to use\n");
	__TBILogF("          -clksrc N          1 to use SYS_CLK_UNDELETED. 0 for defaults\n" );
	__TBILogF("          -coreclockfreq N   Frequency passed to driver, in KHz\n" );
	__TBILogF("          -pllfreq N         Frequency to set the PLL up at, in MHz\n" );
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
              0xBEAFBEAF,
              IMG_NULL,
              0);

	/* Reset QIO */
    QIO_reset(&g_sQio,
              g_aVectoredDevTable,
              QIO_NO_VECTORS,
              &QIO_MTPIVDesc,
			  TBID_SIGNUM_TR2(__TBIThreadId() >> TBID_THREAD_S),
              IMG_NULL,
              0);


	/* Start Meos Main Task - no background task if MeOS Abstraction Layer */
	g_psBackgroundTask = KRN_startOS("Background Task");

	/* Start Meos Main Timer Task...*/
	g_psTimerTask = KRN_startTimerTask("Timer Task", g_aui32TimerStack, TIM_STACK_SIZE, TIM_TICK_PERIOD);

	DQ_init( &g_sTaskQueue );

    /* Start the main task...*/
    KRN_startTask(MainTask, &g_sMainTask, g_aui32TaskStack, STACK_SIZE, KRN_LOWEST_PRIORITY+1, IMG_NULL, "MainTask");

	return;
}

/*!
******************************************************************************

 @Function				main

******************************************************************************/
#if defined (__META_MEOS__)

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
