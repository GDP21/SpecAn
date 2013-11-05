/*!
******************************************************************************
 @file   : thread0.c

 @brief

 @Author Imagination Technologies

 @date   28/09/2010

         <b>Copyright 2010 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         otherwise, or disclosed to third parties without the
         express written permission of Imagination Technologies
         Limited, Home Park Estate, King's Langley, Hertfordshire,
         WD4 8LZ, U.K.

 <b>Description:</b>\n
         Comet System Stress Test.

 <b>Platform:</b>\n
	     Saturn

******************************************************************************/
/*
******************************************************************************
 Modifications :-

*****************************************************************************/
#if defined (METAG_TARGET)
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif // defined (METAG_TARGET)

#include <stdio.h>
#include <string.h>
#include "img_defs.h"
#include "img_common.h"
#include <MeOS.h>
#include <ioblock_defs.h>
#include <sys_util.h>
#include <sys_config.h>
#include <system.h>

#include <gdma_api.h>
#include "spim_api.h"
#include "spis_api.h"



/* General test defines */
#define WriteLong(dest, val) *(unsigned int*)(dest) = (val)

/* Timer defines...*/
#define STACK_SIZE			(2048)

#define STACK_FILL			0xdeadbeef

#define POLL_WAIT			MILLISECOND_TO_TICK(10)

/* Background Task Control Block */
KRN_TASK_T		*			g_psBackgroundTask;
/* Timer Task Control Block */
KRN_TASK_T		*			g_psTimerTask;
/* MEOS Scheduler */
KRN_SCHEDULE_T				g_sMeOSScheduler;
/* Scheduler queues */
KRN_TASKQ_T					g_asSchedQueues[ MEOS_MAX_PRIORITY_LEVEL + 1 ];
/* Stack for the Timer Task */
IMG_UINT32					g_aui32TimerStack[TIM_STACK_SIZE];
/* QIO structures */
QIO_SYS_T					g_sQio;
QIO_DEVENTRY_T				g_aVectoredDevTable[QIO_NO_VECTORS];

#define MAX_SPI_TRANSFER_SIZE  		5120
#define TEST_TRANSFER_SIZE_BYTES 	4096 // MAX 5120?
#define BYTES_PER_MB 				(1024 * 1024)
#define SPI_MODE_TO_USE				SPI_MODE_0
#define SPIM_DMA_CHANNEL			0
#define SPIS_DMA_CHANNEL			1
__attribute__ ((__section__ (".ddr_buffer"))) IMG_UINT8 g_aui8ReadBuffer[MAX_SPI_TRANSFER_SIZE];

extern IMG_UINT8 refWriteBuffer1[MAX_SPI_TRANSFER_SIZE];

IMG_UINT64 			gui64BytesTransferred;
KRN_SEMAPHORE_T		g_sLoopBackCtlSem;

/************************************************/
/*     Main Task Specific Definitions           */
/************************************************/
KRN_TASK_T	g_sMainTask;
DQ_T 		g_sMain_TaskQueue;
IMG_UINT32	g_aui32MainTaskStack[STACK_SIZE];

/************************************************/
/*     Main Task Specific Definitions           */
/************************************************/
KRN_TASK_T	g_sMTestTask;
DQ_T 		g_sMTest_TaskQueue;
IMG_UINT32	g_aui32MTestTaskStack[STACK_SIZE];


/************************************************/
/*     SPIM Task Specific Definitions           */
/************************************************/
KRN_TASK_T	g_sSPIMTask;
DQ_T 		g_sSPIM_TaskQueue;
IMG_UINT32	g_aui32SPIMTaskStack[STACK_SIZE];
static SPIM_sInitParam		g_sSPIMInitParam;
SPIM_sBuffer				g_sTransactionWrite1;

/************************************************/
/*     SPIS Task Specific Definitions           */
/************************************************/
KRN_TASK_T	g_sSPISTask;
DQ_T 		g_sSPIS_TaskQueue;
IMG_UINT32	g_aui32SPISTaskStack[STACK_SIZE];

static SPIS_PARAM_T			g_sInitParam;
static SPIS_PORT_T 			g_sSPISlave;



/*******************************************/
/*         General Test Definitions        */
/*******************************************/

typedef enum
{
    ST_SPIS_NOT_INITIALISED = 0,
    ST_SPIS_FAILED_TO_INIT,
    ST_WAITING_FOR_DATA,
    ST_VERIFIYING_DATA,
    ST_LOOPBACK_STOPPED,
} eSPIS_State;


typedef struct _Initials
{
	IMG_BOOL	bAutoRunLoopbackTest;
	IMG_UINT32	ui32InitialMemTestDelay_ms;
	IMG_UINT32	ui32TransferSizeBytes;
}Initials;

typedef struct _Dynamics
{
	IMG_BOOL	bRunLoopbackTest;
	IMG_UINT32	ui32MemTestDelay_ms;
}Dynamics;

typedef struct _Status
{
	IMG_UINT32 		uiL_MemTestLoops;
	IMG_UINT32 		uiMainTaskTicker;
	IMG_UINT32 		uiSPISIdleTaskTicker;
	eSPIS_State		eSPIS_State;
	IMG_UINT32		ui32Successful_Loopbacks;
	IMG_UINT32		ui32SPIM_SentDataCount;
	IMG_UINT32		ui32SPIS_ReadCount;
	IMG_UINT32		ui32TotalDmaCount_MB;
	IMG_UINT32		ui__EXPECT_ONLY_ZEROES_BELOW__;
	IMG_UINT32		ui32SPIM_WaitCount;
	IMG_UINT32		ui32SPIM_FailedToSendCount;
	IMG_UINT32		ui32Failed_Loopbacks;
}Status;

typedef struct _StressTest
{
    Initials    InitialSettings;
    Dynamics    DynamicSettings;
    Status		CurrentStatus;
}StressTest;

StressTest sStressTest = {{ 1, // bAutoRunLoopbackTest
							1,  // ui32InitialMemTestDelay_ms
							TEST_TRANSFER_SIZE_BYTES //ui32TransferSizeBytes
							},{},{}};



/***************************/
/*   MEMORY TEST ARRAYS    */
/***************************/

volatile unsigned char ucDataArray1024[1024];
volatile unsigned char ucDataArray2048[2048];
void L_MemTest(void);


/*
******************************************************************************

 @Function				MainTask

******************************************************************************/
void MTestTask( void )
{
	DQ_init(&g_sMTest_TaskQueue);

    L_MemTest();
}

/*
******************************************************************************

 @Function				SPIMTask

******************************************************************************/
void SPIMTask( void )
{
	SPIM_sBlock  g_sSPIMaster;
	IMG_UINT32	 i;

	IMG_MEMSET( &g_sSPIMaster, 0, sizeof( SPIM_sBlock ) );

	DQ_init(&g_sSPIM_TaskQueue);

	// Give the slave thread chance to block (needs to be ready for the SPIM Write).
	KRN_hibernate(&g_sSPIM_TaskQueue, MILLISECOND_TO_TICK(10));

	g_sSPIMInitParam.sDev0Param.ui8BitRate = 255;

	g_sSPIMInitParam.sDev0Param.ui8CSSetup = 1;
	g_sSPIMInitParam.sDev0Param.ui8CSHold  = 1;
	g_sSPIMInitParam.sDev0Param.ui8CSDelay = 8;

	/* Spi Mode */
	g_sSPIMInitParam.sDev0Param.eSPIMode = SPI_MODE_TO_USE;

	/* CS Idle Level */
	g_sSPIMInitParam.sDev0Param.ui32CSIdleLevel = 1;

	/* Data idle Level */
	g_sSPIMInitParam.sDev0Param.ui32DataIdleLevel = 0;

	/* Set DMA channel */
	g_sSPIMInitParam.ui32DMAChannel 	= SPIM_DMA_CHANNEL;
	g_sSPIMInitParam.ui32BlockIndex		= 0;

	g_sTransactionWrite1.pui8Buffer			= refWriteBuffer1;
	g_sTransactionWrite1.ui32Size			= TEST_TRANSFER_SIZE_BYTES;
	g_sTransactionWrite1.i32Read			= IMG_FALSE;
	g_sTransactionWrite1.eChipSelect		= SPIM_DEVICE0;
	g_sTransactionWrite1.i32Cont			= IMG_FALSE;
	g_sTransactionWrite1.i32InterByteDelay 	= 0;

	/* Fill buffer with data */
	for(i=0; i<TEST_TRANSFER_SIZE_BYTES; i++)
	{
		refWriteBuffer1[i] = i%256;
	}

	if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sSPIMInitParam))
	{
		__TBILogF("FAIL: initialisation of the SPI master failed!\n");
		return;
	}

	while(1)
	{
		/* Wait for SPIS to be almost ready */
		KRN_testSemaphore( &g_sLoopBackCtlSem, 1, KRN_INFWAIT );

		while(sStressTest.CurrentStatus.eSPIS_State != ST_WAITING_FOR_DATA)
		{
			// Context switch out of this task so that SPIS has a chance to enter the blocking read before we send the data.
			sStressTest.CurrentStatus.ui32SPIM_WaitCount++;
			KRN_release();
		}

		if (SPIM_OK != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionWrite1, NULL, NULL))
		{
			sStressTest.CurrentStatus.ui32SPIM_FailedToSendCount++;
			IMG_ASSERT(IMG_FALSE);
		}
		else
		{
			sStressTest.CurrentStatus.ui32SPIM_SentDataCount++;
		}
	}
}

/*
******************************************************************************

 @Function				SPISTask

******************************************************************************/
void SPISTask( void )
{
	IMG_BOOL	bDataMismatch;

	DQ_init(&g_sSPIS_TaskQueue);

	// setup the fixed parameters for the spi slave
	g_sInitParam.spiMode          = SPI_MODE_TO_USE;
	g_sInitParam.csLevel          = 0;
	g_sInitParam.dmaChannel		  = SPIS_DMA_CHANNEL;
	g_sInitParam.spiSyncMode	  = SPI_SYNC_MODE_SLOW;
	g_sInitParam.ui32BlockIndex   = 0;

	if (SPIS_OK != SPISInit(&g_sSPISlave, &g_sInitParam, NULL, 0))
	{
		sStressTest.CurrentStatus.eSPIS_State = ST_SPIS_FAILED_TO_INIT;
		__TBILogF("FAIL: initialisation of the SPI slave failed!\n");
		return;
	}

	while(1)
	{
		if(sStressTest.DynamicSettings.bRunLoopbackTest)
		{
			// Clear the receive buffer
			memset(g_aui8ReadBuffer, 0, sStressTest.InitialSettings.ui32TransferSizeBytes);

			// Signal to the SPIM task that we are about to enter a blocking read
			KRN_setSemaphore( &g_sLoopBackCtlSem, 1 );

			// Setting this status shows SPIM that we either didn't context switch at the semaphore above,
			// or that we have returned to this task - and so the blocking SPIS read below will have been set
			// since there is nothing to preempt it.
			sStressTest.CurrentStatus.eSPIS_State = ST_WAITING_FOR_DATA;

			// Blocking SPIS read
			SPISRead(&g_sSPISlave, g_aui8ReadBuffer, sStressTest.InitialSettings.ui32TransferSizeBytes, NULL, SPIS_INF_TIMEOUT);

			sStressTest.CurrentStatus.ui32SPIS_ReadCount++;
			sStressTest.CurrentStatus.eSPIS_State = ST_VERIFIYING_DATA;

			bDataMismatch = IMG_FALSE;
			if(memcmp(g_aui8ReadBuffer, refWriteBuffer1, sizeof(refWriteBuffer1)))
			{
				bDataMismatch = IMG_TRUE;
				IMG_ASSERT(0);	// Comment out if uninterrupted soak test is required.

			}
			else
			{
				// Transfer was successful, tot up total amount of data sent & received
				gui64BytesTransferred += 2 * sStressTest.InitialSettings.ui32TransferSizeBytes; // DMA has occured in both directions
				sStressTest.CurrentStatus.ui32TotalDmaCount_MB = gui64BytesTransferred / BYTES_PER_MB;
			}

			// Log success or failure
			bDataMismatch ? sStressTest.CurrentStatus.ui32Failed_Loopbacks++ : sStressTest.CurrentStatus.ui32Successful_Loopbacks++;
		}
		else
		{
			// Not running the loopback test at the moment, so just wait, checking every so often if it's been re-enabled
			sStressTest.CurrentStatus.eSPIS_State = ST_LOOPBACK_STOPPED;
			sStressTest.CurrentStatus.uiSPISIdleTaskTicker++;
			KRN_hibernate(&g_sSPIS_TaskQueue, MILLISECOND_TO_TICK(50));
		}
	}
}



/***************************************************/
/*                   L_MemTest                       */
/***************************************************/
void L_MemTest(void)
{
    unsigned int i;

    for (i = 0; i < 1024; i++)
    {
        ucDataArray1024[i] = i % 64;
    }

    for (i = 0; i < 2048; i++)
    {
        ucDataArray2048[i] = i;
    }

    while (1)
    {
        for (i = 0; i <1024; i++)
        {
            ucDataArray2048[i] += ucDataArray1024[i];
        }
        sStressTest.CurrentStatus.uiL_MemTestLoops++;
        KRN_hibernate(&g_sMTest_TaskQueue, MILLISECOND_TO_TICK(sStressTest.DynamicSettings.ui32MemTestDelay_ms));
    }
}

/*!
******************************************************************************

 @Function				RunTest

******************************************************************************/
static img_void RunTest( img_void )
{
	SYS_sSPIMConfig		sSPIMConfig[2];
	SYS_sConfig			sConfig;

	/* Reset the Kernel */
    KRN_reset(&(g_sMeOSScheduler),
              g_asSchedQueues,
              MEOS_MAX_PRIORITY_LEVEL,
              STACK_FILL,
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

	DQ_init( &g_sMain_TaskQueue );

	//	STARTUP_Initialise ();

	IMG_MEMSET( &sSPIMConfig, 0, 2 * sizeof( SYS_sSPIMConfig ) );

	IMG_MEMSET( &sConfig, 0, sizeof( sConfig ) );
	SYS_Configure( &sConfig );
	sSPIMConfig[0].bConfigure 			= IMG_TRUE;
	sSPIMConfig[0].bEnable 				= IMG_TRUE;
	sSPIMConfig[0].bTargetFrequency 	= IMG_TRUE;
	sSPIMConfig[0].ui32TargetFreq_fp 	= 0xE00000; // 50MHz in 12.20 format;

	SYS_ConfigureSPIM( sSPIMConfig );

	/* Enable the SPI Loopback feature */
	*(unsigned int*)0x020141D0 |= 0x10;

	/* Initialise looback control semaphore */
	KRN_initSemaphore( &g_sLoopBackCtlSem, 0 );

	/* Implement Autostart requests */
	sStressTest.DynamicSettings.bRunLoopbackTest 	= sStressTest.InitialSettings.bAutoRunLoopbackTest;
	sStressTest.DynamicSettings.ui32MemTestDelay_ms = sStressTest.InitialSettings.ui32InitialMemTestDelay_ms;

	/* Clear any status */
	sStressTest.CurrentStatus.uiL_MemTestLoops 					= 0;
	sStressTest.CurrentStatus.uiMainTaskTicker 					= 0;
	sStressTest.CurrentStatus.uiSPISIdleTaskTicker				= 0;
	sStressTest.CurrentStatus.eSPIS_State 						= ST_SPIS_NOT_INITIALISED;
	sStressTest.CurrentStatus.ui32Successful_Loopbacks 			= 0;
	sStressTest.CurrentStatus.ui32SPIM_SentDataCount 			= 0;
	sStressTest.CurrentStatus.ui32SPIS_ReadCount 				= 0;
	sStressTest.CurrentStatus.ui32TotalDmaCount_MB 				= 0;
	sStressTest.CurrentStatus.ui__EXPECT_ONLY_ZEROES_BELOW__ 	= 0;
	sStressTest.CurrentStatus.ui32SPIM_WaitCount 				= 0;
	sStressTest.CurrentStatus.ui32SPIM_FailedToSendCount 		= 0;
	sStressTest.CurrentStatus.ui32Failed_Loopbacks 				= 0;


	/* Start the tasks...*/
  	KRN_startTask(MTestTask,   &g_sMTestTask, g_aui32MTestTaskStack,  STACK_SIZE, KRN_LOWEST_PRIORITY,   IMG_NULL, "MemTestTask");
  	KRN_startTask(SPISTask,    &g_sSPISTask,  g_aui32SPISTaskStack,   STACK_SIZE, KRN_LOWEST_PRIORITY+2, IMG_NULL, "SPIS_Task"); //+2 to ensure SPIS is ready for SPIM
  	KRN_startTask(SPIMTask,    &g_sSPIMTask,  g_aui32SPIMTaskStack,   STACK_SIZE, KRN_LOWEST_PRIORITY+1, IMG_NULL, "SPIM_Task");


 	for (;;)
	{
		KRN_hibernate( &g_sMain_TaskQueue, KRN_INFWAIT );
	}

	return;
}

/*!
******************************************************************************

 @Function				main

******************************************************************************/
#if defined __META_MEOS__

int main(int argc, char **argv)
{
	KRN_TASKQ_T	sHibernateQ;

	RunTest();

	DQ_init( &sHibernateQ );
	KRN_hibernate( &sHibernateQ, KRN_INFWAIT );
  return 1;
}

#else

#error CPU and OS not recognised

#endif
