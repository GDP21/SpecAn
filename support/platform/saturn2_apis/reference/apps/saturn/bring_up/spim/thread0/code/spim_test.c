/*!
******************************************************************************
 @file   : spim_test.c

 @brief

 @Author Imagination Technologies

 @date   07/06/2011

         <b>Copyright 2011 by Imagination Technologies Limited.</b>\n
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
         SPI Master Test Application.

 <b>Platform:</b>\n
	     Platform Independent

 @Version
	     1.0

******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log: spim_test.c,v $

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>


#include <MeOS.h>
#include <img_defs.h>
#include <ioblock_defs.h>
#include <system.h>
#include <sys_util.h>
#include <sys_config.h>
#include <ioblock_defs.h>
#include <gdma_api.h>

#include "spim_api.h"

/*============================================================================
====	D E F I N E S
=============================================================================*/

/* Timer defines...*/
#define STACK_SIZE			(4096)

#define	STACK_FILL			(0xdeadbeef)

#define BITRATE_CONVERSION	(0.048)
#define NUMBER_TRANSACTION	(2)

// Default settings: CS setup and hold = 400ns, delay = 800ns
#define CS_SETUP_VALUE		(0xA)
#define CS_HOLD_VALUE		(0xA)
#define CS_DELAY_VALUE		(0xA)

#define CS_CMP_VALUE		(5)
#define CS_CMP_MASK			(0xFF)
#define CS_CMP_EQ			(1)

#define SPIM_DEVICE_0_PARAM_REG_OFFSET		(0x00)
#define SPIM_DEVICE_1_PARAM_REG_OFFSET		(0x04)
#define SPIM_DEVICE_2_PARAM_REG_OFFSET		(0x08)

#define SPIM_0_DEVICE_0_PARAM_REG (SYSTEM_CONTROL_BASE_ADDRESS + SPIM_0_REGS_OFFSET + SPIM_DEVICE_0_PARAM_REG_OFFSET)
#define SPIM_0_DEVICE_1_PARAM_REG (SYSTEM_CONTROL_BASE_ADDRESS + SPIM_0_REGS_OFFSET + SPIM_DEVICE_1_PARAM_REG_OFFSET)
#define SPIM_0_DEVICE_2_PARAM_REG (SYSTEM_CONTROL_BASE_ADDRESS + SPIM_0_REGS_OFFSET + SPIM_DEVICE_2_PARAM_REG_OFFSET)

#define SPIM_1_DEVICE_0_PARAM_REG (SYSTEM_CONTROL_BASE_ADDRESS + SPIM_1_REGS_OFFSET + SPIM_DEVICE_0_PARAM_REG_OFFSET)
#define SPIM_1_DEVICE_1_PARAM_REG (SYSTEM_CONTROL_BASE_ADDRESS + SPIM_1_REGS_OFFSET + SPIM_DEVICE_1_PARAM_REG_OFFSET)
#define SPIM_1_DEVICE_2_PARAM_REG (SYSTEM_CONTROL_BASE_ADDRESS + SPIM_1_REGS_OFFSET + SPIM_DEVICE_2_PARAM_REG_OFFSET)

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

enum SPIM_TEST_NUM
{
    TEST_ILLEGAL_DMA_CHANNELS = 1,				/* 1 */
    TEST_ILLEGAL_SPI_MODE,
    TEST_ILLEGAL_CS_IDLE_LEVEL,
    TEST_ILLEGAL_DATA_IDLE_LEVEL,
    TEST_ILLEGAL_CS_LINE,						/* 5 */
    TEST_ILLEGAL_SIZE,
    TEST_ILLEGAL_READ_WRITE,
    TEST_ILLEGAL_CONTINUE,
    TEST_ILLEGAL_COMPARE_EQ,
    TEST_SUPPORT_RANGE_SIZE,					/* 10 */
    TEST_READ_FOLLOWING_READ,
    TEST_WRITE_FOLLOWING_WRITE,
    TEST_WRITE_FOLLOWING_READ,
    TEST_READ_FOLLOWING_WRITE,
    TEST_2READS_CONTINUE,						/* 15 */
    TEST_READ_FOLLOWING_DELAYED_READ,
    TEST_DELAYED_READ_FOLLOWING_READ,			/* 17 */
	TEST_CHIP_SELECT_SETUP,
	TEST_DEINIT
};

/*============================================================================
====	T Y P E D E F S
=============================================================================*/
/*
** The TEST_DESC_T provides a location for all test configuration data
** and should be modified for the particular peripheral under test.
*/
typedef struct
{
	IMG_UINT32		ui32TestNum;
	IMG_UINT8	  	ui8CmpValue;
	IMG_UINT8	  	ui8CmpMask;
	IMG_INT			iCmpEq;
	IMG_UINT32  	ui32Size;
	IMG_INT			iDev;
	IMG_UINT32  	ui32BitRate;
	IMG_UINT32  	ui32DmaChannel;
	SPIM_eMode		eSPIMode;
    IMG_UINT32		ui32DataIdleLevel;
	IMG_UINT32		ui32CSIdleLevel;
	IMG_UINT32		ui32InterByteDelay;
    IMG_INT			iCSSetup;
    IMG_INT  		iCSHold;
    IMG_INT  		iCSDelay;
	IMG_INT  		f_refWrite1;
	IMG_INT  		f_refWrite2;
	IMG_INT  		f_refRead;
	img_uint32		ui32Block;
} TEST_sTestDesc;

/*============================================================================
====	D A T A
=============================================================================*/

/* Background Task Control Block */
KRN_TASK_T		*	g_psBackgroundTask;

/* Timer Task Control Block */
KRN_TASK_T		*	g_psTimerTask;

/* MEOS Scheduler */
KRN_SCHEDULE_T		g_sMeOSScheduler;

/* Scheduler queues */
KRN_TASKQ_T			g_asSchedQueues[ MEOS_MAX_PRIORITY_LEVEL + 1 ];

/* Stack for the Timer Task */
IMG_UINT32			g_aui32TimerStack[TIM_STACK_SIZE];

/* Main Task Control Block */
KRN_TASK_T			g_sMainTask;

/* Stack for the Main Task */
IMG_UINT32			g_aui32TaskStack[STACK_SIZE];

/* QIO structures */
QIO_DEVENTRY_T		g_aVectoredDevTable[QIO_NO_VECTORS];
QIO_SYS_T			g_sQio;

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
char argv22[32] = { '@', '\0' };
char argv23[32] = { '@', '\0' };
char argv24[32] = { '@', '\0' };
char argv25[32] = { '@', '\0' };
char argv26[32] = { '@', '\0' };
char argv27[32] = { '@', '\0' };
char argv28[32] = { '@', '\0' };
char argv29[32] = { '@', '\0' };
char argv30[32] = { '@', '\0' };
char argv31[32] = { '@', '\0' };
char argv32[32] = { '@', '\0' };
char argv33[32] = { '@', '\0' };
char argv34[32] = { '@', '\0' };
char argv35[32] = { '@', '\0' };
char argv36[32] = { '@', '\0' };

/* Array of arguments - the first element is unused, the other elements point to the argvXX[]
** strings that are initialised by script */
char *metag_argv[] = { "dummy", argv1, argv2, argv3, argv4, argv5, argv6, argv7, argv8, argv9, argv10, argv11, argv12, argv13, argv14,
						argv15, argv16, argv17, argv18, argv19, argv20, argv21, argv22, argv23, argv24, argv25, argv26, argv27, argv28,
						argv29, argv30, argv31, argv32, argv33, argv34, argv35, argv36};

/* Argument count, updated by initialisation script. Useful for debug. */
int metag_argc = 0;


static TEST_sTestDesc		g_sTestDescription;
static SPIM_sInitParam		g_sInitParam;

SPIM_sBlock					g_sSPIMaster;
SPIM_sBuffer				g_sTransactionWrite1, g_sTransactionRead1, g_sTransactionWaitRead;
SPIM_sBuffer				g_sTransactionWrite2, g_sTransactionRead2;

__attribute__ ((__section__ (".bulkbuffers"))) IMG_UINT8 refWriteBuffer1[SPIM_MAX_TRANSFER_BYTES];
__attribute__ ((__section__ (".bulkbuffers"))) IMG_UINT8 refWriteBuffer2[SPIM_MAX_TRANSFER_BYTES];
__attribute__ ((__section__ (".bulkbuffers"))) IMG_UINT8 refReadBuffer[SPIM_MAX_TRANSFER_BYTES];
__attribute__ ((__section__ (".bulkbuffers"))) IMG_UINT8 readBuffer1[SPIM_MAX_TRANSFER_BYTES];
__attribute__ ((__section__ (".bulkbuffers"))) IMG_UINT8 readBuffer2[SPIM_MAX_TRANSFER_BYTES];

IMG_UINT32 writeFile1 = 0;
IMG_UINT32 writeFile2 = 0;
IMG_UINT32 readFile   = 0;

// Local function prototypes

SPIM_eDevice SetCSTiming(void);
static void ParseCommandLine(int argc, char *argv[]);
static void Usage(char *cmd);

/*!
******************************************************************************

 @Function				MainTask

******************************************************************************/
void MainTask( void )
{
	IMG_UINT32		j;
	SPIM_eDevice	eChipSelect;
	SYS_sConfig		sConfig;
	SYS_sSPIMConfig	sSPIMConfig[2];

	// Set up system
	IMG_MEMSET( &sConfig, 0, sizeof( sConfig ) );
	SYS_Configure( &sConfig );

	IMG_MEMSET( &sSPIMConfig, 0, 2 * sizeof( SYS_sSPIMConfig ) );
	IMG_ASSERT(g_sTestDescription.ui32Block < 2); // Only 2 SPIM blocks, 0 and 1

	sSPIMConfig[g_sTestDescription.ui32Block].bConfigure 	= IMG_TRUE;
	sSPIMConfig[g_sTestDescription.ui32Block].bEnable 		= IMG_TRUE;

	sSPIMConfig[g_sTestDescription.ui32Block].bTargetFrequency = IMG_TRUE;
	sSPIMConfig[g_sTestDescription.ui32Block].ui32TargetFreq_fp = 0xE00000; // 15MHz in 12.20 format;

	//sSPIMConfig.asBlockConfig[ BLOCK_UCC0 ].bEnable = IMG_TRUE;
	//sSPIMConfig.asBlockConfig[ BLOCK_UCC1 ].bEnable = IMG_TRUE;

	SYS_ConfigureSPIM( sSPIMConfig );

	/* Set chip select depending on value entered by the user */
	eChipSelect = SetCSTiming();

	/* Set DMA channel depending on value entered by user */
	g_sInitParam.ui32DMAChannel 	= g_sTestDescription.ui32DmaChannel;
	g_sInitParam.ui32BlockIndex		= g_sTestDescription.ui32Block;

	/* Read messages from files */
	if ( writeFile1 == 1 )
	{
		read( g_sTestDescription.f_refWrite1, refWriteBuffer1, g_sTestDescription.ui32Size );
	}
	if ( writeFile2 == 1 )
	{
		read( g_sTestDescription.f_refWrite2, refWriteBuffer2, g_sTestDescription.ui32Size );
	}
	if ( readFile == 1 )
	{
		read( g_sTestDescription.f_refRead, refReadBuffer, g_sTestDescription.ui32Size );
	}

	g_sTransactionRead1.pui8Buffer			= readBuffer1;
	g_sTransactionRead1.ui32Size			= g_sTestDescription.ui32Size;
	g_sTransactionRead1.i32Read				= IMG_TRUE;
	g_sTransactionRead1.eChipSelect			= eChipSelect;
	g_sTransactionRead1.i32Cont				= IMG_FALSE;
	g_sTransactionRead1.i32InterByteDelay 	= g_sTestDescription.ui32InterByteDelay;

	g_sTransactionWrite1.pui8Buffer			= refWriteBuffer1;
	g_sTransactionWrite1.ui32Size			= g_sTestDescription.ui32Size;
	g_sTransactionWrite1.i32Read			= IMG_FALSE;
	g_sTransactionWrite1.eChipSelect		= eChipSelect;
	g_sTransactionWrite1.i32Cont			= IMG_FALSE;
	g_sTransactionWrite1.i32InterByteDelay 	= g_sTestDescription.ui32InterByteDelay;

	if ( g_sTestDescription.ui8CmpMask != 0 )
	{
		/* values defined by user */
		g_sTransactionWaitRead.i32Cont      = IMG_FALSE;
		g_sTransactionWaitRead.eChipSelect	= eChipSelect;
		g_sTransactionWaitRead.ui8CmpValue	= g_sTestDescription.ui8CmpValue;
		g_sTransactionWaitRead.ui8CmpMask	= g_sTestDescription.ui8CmpMask;
		g_sTransactionWaitRead.iCmpEq		= g_sTestDescription.iCmpEq;
	}
	else
	{
		/* default values */
		g_sTransactionWaitRead.i32Cont      = IMG_FALSE;
		g_sTransactionWaitRead.eChipSelect	= eChipSelect;
		g_sTransactionWaitRead.ui8CmpValue	= CS_CMP_VALUE;
		g_sTransactionWaitRead.ui8CmpMask	= CS_CMP_MASK;
		g_sTransactionWaitRead.iCmpEq		= CS_CMP_EQ;
	}

	switch( g_sTestDescription.ui32TestNum )
	{
		case TEST_ILLEGAL_DMA_CHANNELS:
		{
			/*
			** Check rejection of illegal DMA channels
			*/
			/*
			** API no longer supports this use case
			*/
			
			/*
			g_sInitParam.ui32DMAChannel = 11;
			if (SPIM_INVALID_DMA_CHANNELS != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: rejection of illegal output DMA channel failed!\n");
				return;
			}
			__TBILogF("SUCCESS: rejection of illegal DMA channel!\n");
			*/
			__TBILogF("SKIP: rejection of illegal DMA channel!\n");

			break;
		}

		case TEST_ILLEGAL_SPI_MODE:
		{
			/*
			** Check rejection of illegal SPI mode
			*/
			g_sInitParam.sDev0Param.eSPIMode = SPIM_MODE_3 + 1;
			if (SPIM_INVALID_SPI_MODE != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: rejection of illegal SPI mode for CS0 failed!\n");
				return;
			}
			g_sInitParam.sDev0Param.eSPIMode = g_sTestDescription.eSPIMode;
			g_sInitParam.sDev1Param.eSPIMode = SPIM_MODE_3 + 1;
			/* Since it is expected that SPIMInit() above fails, and this occurs before init complete,
			** SPIMDeinit() not required (and will assert if attempted, due to incomplete init)
			SPIMDeinit(&g_sSPIMaster);*/
			if (SPIM_INVALID_SPI_MODE != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: rejection of illegal SPI mode for CS1 failed!\n");
				return;
			}
			g_sInitParam.sDev1Param.eSPIMode = g_sTestDescription.eSPIMode;
			g_sInitParam.sDev2Param.eSPIMode = SPIM_MODE_3 + 1;
			/* Since it is expected that SPIMInit() above fails, and this occurs before init complete,
			** SPIMDeinit() not required (and will assert if attempted, due to incomplete init)
			SPIMDeinit(&g_sSPIMaster);*/
			if (SPIM_INVALID_SPI_MODE != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: rejection of illegal SPI mode for CS2 failed!\n");
				return;
			}
			__TBILogF("SUCCESS: rejection of illegal SPI mode!\n");
			break;
		}

		case TEST_ILLEGAL_CS_IDLE_LEVEL:
		{
			/*
			** Check rejection of illegal CS idle level
			*/
			g_sInitParam.sDev0Param.ui32CSIdleLevel = 2;
			if (SPIM_INVALID_CS_IDLE_LEVEL != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: rejection of illegal CS0 idle level failed!\n");
				return;
			}
			g_sInitParam.sDev0Param.ui32CSIdleLevel = 1;
			g_sInitParam.sDev1Param.ui32CSIdleLevel = 2;
			/* Since it is expected that SPIMInit() above fails, and this occurs before init complete,
			** SPIMDeinit() not required (and will assert if attempted, due to incomplete init)
			SPIMDeinit(&g_sSPIMaster);*/
			if (SPIM_INVALID_CS_IDLE_LEVEL != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: rejection of illegal CS1 idle level failed!\n");
				return;
			}
			g_sInitParam.sDev1Param.ui32CSIdleLevel = 1;
			g_sInitParam.sDev2Param.ui32CSIdleLevel = 2;
			/* Since it is expected that SPIMInit() above fails, and this occurs before init complete,
			** SPIMDeinit() not required (and will assert if attempted, due to incomplete init)
			SPIMDeinit(&g_sSPIMaster);*/
			if (SPIM_INVALID_CS_IDLE_LEVEL != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: rejection of illegal CS2 idle level failed!\n");
				return;
			}
			__TBILogF("SUCCESS: rejection of illegal CS idle level!\n");
			break;
		}

		case TEST_ILLEGAL_DATA_IDLE_LEVEL:
		{
			/*
			** Check rejection of illegal data idle level
			*/
			g_sInitParam.sDev0Param.ui32DataIdleLevel = 2;
			if (SPIM_INVALID_DATA_IDLE_LEVEL != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: rejection of illegal data idle level for CS0 failed!\n");
				return;
			}
			g_sInitParam.sDev0Param.ui32DataIdleLevel = 1;
			g_sInitParam.sDev1Param.ui32DataIdleLevel = 2;
			/* Since it is expected that SPIMInit() above fails, and this occurs before init complete,
			** SPIMDeinit() not required (and will assert if attempted, due to incomplete init)
			SPIMDeinit(&g_sSPIMaster);*/
			if (SPIM_INVALID_DATA_IDLE_LEVEL != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: rejection of illegal data idle level for CS1 failed!\n");
				return;
			}
			g_sInitParam.sDev1Param.ui32DataIdleLevel = 1;
			g_sInitParam.sDev2Param.ui32DataIdleLevel = 2;
			/* Since it is expected that SPIMInit() above fails, and this occurs before init complete,
			** SPIMDeinit() not required (and will assert if attempted, due to incomplete init)
			SPIMDeinit(&g_sSPIMaster);*/
			if (SPIM_INVALID_DATA_IDLE_LEVEL != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: rejection of illegal data idle level for CS2 failed!\n");
				return;
			}
			__TBILogF("SUCCESS: rejection of illegal data idle level!\n");
			break;
		}

		case TEST_ILLEGAL_CS_LINE:
		{
			/*
			** Check rejection of illegal chip select lines
			*/
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}
			g_sTransactionWrite1.eChipSelect = SPIM_DUMMY_CS + 1;
			if (SPIM_INVALID_CS_LINE != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionWrite1, NULL, NULL))
			{
				__TBILogF("FAIL: rejection of illegal chip select line for 1st operation failed!\n");
				return;
			}
			if (SPIM_INVALID_CS_LINE != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, &g_sTransactionWrite1, NULL))
			{
				__TBILogF("FAIL: rejection of illegal chip select line for 2nd operation failed!\n");
				return;
			}
			
			/* API no longer supports use case, will assert if SPIMReadWrite() called as below
		
			g_sTransactionWaitRead.eChipSelect = SPIM_DUMMY_CS + 1;
			if (SPIM_INVALID_CS_LINE != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, NULL, &g_sTransactionWaitRead))
			{
				__TBILogF("FAIL: rejection of illegal chip select line for compare data detection failed!\n");
				return;
			}
			*/
			__TBILogF("SKIP: rejection of illegal chip select line for compare data detection\n");
			__TBILogF("SUCCESS: rejection of illegal chip select line!\n");
			
			break;
		}

		case TEST_ILLEGAL_SIZE:
		{
			/*
			** Check rejection of illegal size parameter
			*/
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}
			g_sTransactionWrite1.ui32Size = SPIM_MAX_TRANSFER_BYTES + 1;
			if (SPIM_INVALID_SIZE != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionWrite1, NULL, NULL))
			{
				__TBILogF("FAIL: rejection of illegal size parameter for 1st operation failed!\n");
				return;
			}
			if (SPIM_INVALID_SIZE != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, &g_sTransactionWrite1, NULL))
			{
				__TBILogF("FAIL: rejection of illegal size parameter for 2nd operation failed!\n");
				return;
			}
			__TBILogF("SUCCESS: rejection of illegal size parameter!\n");
			break;
		}

		case TEST_ILLEGAL_READ_WRITE:
		{
			/*
			** Check rejection of illegal read/write parameter
			*/
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}
			g_sTransactionRead1.i32Read = 2;
			if (SPIM_INVALID_READ_WRITE != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, NULL, NULL))
			{
				__TBILogF("FAIL: rejection of illegal read/write parameter failed!\n");
				return;
			}
			if (SPIM_INVALID_READ_WRITE != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, &g_sTransactionWrite1, NULL))
			{
				__TBILogF("FAIL: rejection of illegal read/write parameter for 2nd operation failed!\n");
				return;
			}
			__TBILogF("SUCCESS: rejection of illegal read/write parameter!\n");
			break;
		}

		case TEST_ILLEGAL_CONTINUE:
		{
			/*
			** Check rejection of illegal continue parameter
			*/
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}
			g_sTransactionRead1.i32Cont = 2;
			if (SPIM_INVALID_CONTINUE != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, NULL, NULL))
			{
				__TBILogF("FAIL: rejection of illegal continue parameter failed!\n");
				return;
			}
			if (SPIM_INVALID_CONTINUE != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, &g_sTransactionWrite1, NULL))
			{
				__TBILogF("FAIL: rejection of illegal continue parameter for 2nd operation failed!\n");
				return;
			}
			g_sTransactionWaitRead.i32Cont = 2;
			if (SPIM_INVALID_CONTINUE != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, NULL, &g_sTransactionWaitRead))
			{
				__TBILogF("FAIL: rejection of illegal continue parameter for compare data detection failed!\n");
				return;
			}
			__TBILogF("SUCCESS: rejection of illegal continue parameter!\n");
			break;
		}

		case TEST_ILLEGAL_COMPARE_EQ:
		{
			/*
			** Check rejection of illegal compare equal parameter
			*/
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}

			/* API no longer supports use case, will assert if SPIMReadWrite() called as below
			
			g_sTransactionWaitRead.iCmpEq = 2;
			if (SPIM_INVALID_COMPARE_EQ != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, NULL, &g_sTransactionWaitRead))
			{
				__TBILogF("FAIL: rejection of illegal compare equal parameter failed!\n");
				return;
			}
			__TBILogF("SUCCESS: rejection of illegal compare equal parameter!\n");
			*/
			__TBILogF("SKIP: rejection of illegal compare equal parameter\n");
			break;
		}

		case TEST_SUPPORT_RANGE_SIZE:
		{
			/*
			** Check support for range of transaction size
			*/
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}
			if (SPIM_OK != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, &g_sTransactionWrite1, NULL))
			{
				__TBILogF("FAIL: read/write operation failed!\n");
				return;
			}

			/* Compare message read from slave with expected MISO message */
			//__TBIDataCacheFlush(g_sTransactionRead1.pui8Buffer, g_sTestDescription.ui32Size);
			//__TBIDataCacheFlush(refReadBuffer, g_sTestDescription.ui32Size);
			for (j=0; j<g_sTestDescription.ui32Size; j++)
			{
				if(g_sTransactionRead1.pui8Buffer[j] != refReadBuffer[j])
				{
					__TBILogF("Error in byte %d\n", j);
					__TBILogF("Expected: %d\n", refReadBuffer[j]);
					__TBILogF("Found: %d\n", g_sTransactionRead1.pui8Buffer[j]);
					return;
				}
			}
			__TBILogF("SUCCESS: support for read of %d byte!\n", g_sTestDescription.ui32Size);
			__TBILogF("SUCCESS: support for write of %d byte?\n", g_sTestDescription.ui32Size);
			__TBILogF("     - Check written output at the slave's read result\n");
			__TBILogF("     - ignore the first %d bytes read by the slave occuring from the preceding operation\n", g_sTestDescription.ui32Size);
			break;
		}

		case TEST_READ_FOLLOWING_READ:
		{
			/*
			** Check support for read immediately following a different read
			*/
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}
			g_sTransactionRead2.pui8Buffer  = readBuffer2;
			g_sTransactionRead2.ui32Size    = 10;
			g_sTransactionRead2.i32Read     = IMG_TRUE;
			g_sTransactionRead2.eChipSelect	= eChipSelect;
			g_sTransactionRead2.i32Cont     = 0;
			g_sTransactionRead2.i32InterByteDelay = g_sTestDescription.ui32InterByteDelay;
			if (SPIM_OK != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, &g_sTransactionRead2, NULL))
			{
				__TBILogF("FAIL: read/write operation failed!\n");
				return;
			}
			/* Compare message read from slave with expected MISO message */
			//__TBIDataCacheFlush(g_sTransactionRead1.pui8Buffer, g_sTestDescription.ui32Size);
			//__TBIDataCacheFlush(g_sTransactionRead2.pui8Buffer, 10);
			//__TBIDataCacheFlush(refReadBuffer, g_sTestDescription.ui32Size);
			for (j=0; j<g_sTestDescription.ui32Size; j++)
			{
				if(g_sTransactionRead1.pui8Buffer[j] != refReadBuffer[j])
				{
					__TBILogF("Error in operation 1 and byte %d\n", j);
					__TBILogF("Expected: %d\n", refReadBuffer[j]);
					__TBILogF("Found: %d\n", g_sTransactionRead1.pui8Buffer[j]);
					return;
				}
			}
			for (j=0; j<g_sTransactionRead2.ui32Size; j++)
			{
				if(g_sTransactionRead2.pui8Buffer[j] != refReadBuffer[j])
				{
					__TBILogF("Error in operation 2 and byte %d\n", j);
					__TBILogF("Expected: %d\n", refReadBuffer[j]);
					__TBILogF("Found: %d\n", g_sTransactionRead2.pui8Buffer[j]);
					return;
				}
			}
			__TBILogF("SUCCESS: support for read immediately following a different read!\n");
			break;
		}

		case TEST_WRITE_FOLLOWING_WRITE:
		{
			/*
			** Check support for write immediately following a different write
			*/
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}
			g_sTransactionWrite2.pui8Buffer    = refWriteBuffer2;
			g_sTransactionWrite2.ui32Size      = 10;
			g_sTransactionWrite2.i32Read       = IMG_FALSE;
			g_sTransactionWrite2.eChipSelect   = eChipSelect;
			g_sTransactionWrite2.i32Cont       = 0;
			g_sTransactionWrite2.i32InterByteDelay = g_sTestDescription.ui32InterByteDelay;
			if (SPIM_OK != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionWrite1, &g_sTransactionWrite2, NULL))
			{
				__TBILogF("FAIL: read/write operation failed!\n");
				return;
			}
			__TBILogF("SUCCESS: support for write immediately following a different write?\n");
			__TBILogF("     - Check written output at the slave's read result\n");
			__TBILogF("     - written output should be the combination of the 2 writes:\n");
			__TBILogF("     - 1st write: %d bytes from mosi_1.bin\n", g_sTransactionWrite1.ui32Size);
			__TBILogF("     - 2nd write: %d bytes from mosi_2.bin\n", g_sTransactionWrite2.ui32Size);
			break;
		}

		case TEST_WRITE_FOLLOWING_READ:
		{
			/*
			** Check support for write immediately following a read
			*/
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}
			if (SPIM_OK != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, &g_sTransactionWrite1, NULL))
			{
				__TBILogF("FAIL: read/write operation failed!\n");
				return;
			}
			/* Compare message read from slave with expected MISO message */
			//__TBIDataCacheFlush(g_sTransactionRead1.pui8Buffer, g_sTestDescription.ui32Size);
			//__TBIDataCacheFlush(refReadBuffer, g_sTestDescription.ui32Size);
			for (j=0; j<g_sTestDescription.ui32Size; j++)
			{
				if(g_sTransactionRead1.pui8Buffer[j] != refReadBuffer[j])
				{
					__TBILogF("Error in byte %d\n", j);
					__TBILogF("Expected: %d\n", refReadBuffer[j]);
					__TBILogF("Found: %d\n", g_sTransactionRead1.pui8Buffer[j]);
					return;
				}
			}
			__TBILogF("SUCCESS: support for write immediately following a read?\n");
			__TBILogF("     - Check written output at the slave's read result\n");
			__TBILogF("     - ignore the first %d bytes read by the slave occuring from the preceding operation\n", g_sTestDescription.ui32Size);
			break;
		}

		case TEST_READ_FOLLOWING_WRITE:
		{
			/*
			** Check support for read immediately following a write
			*/
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}
			if (SPIM_OK != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionWrite1, &g_sTransactionRead1, NULL))
			{
				__TBILogF("FAIL: read/write operation failed!\n");
				return;
			}
			/* Compare message read from slave with expected MISO message */
			//__TBIDataCacheFlush(g_sTransactionRead1.pui8Buffer, g_sTestDescription.ui32Size);
			//__TBIDataCacheFlush(refReadBuffer, g_sTestDescription.ui32Size);
			for (j=0; j<g_sTestDescription.ui32Size; j++)
			{
				if(g_sTransactionRead1.pui8Buffer[j] != refReadBuffer[j])
				{
					__TBILogF("Error in byte %d\n", j);
					__TBILogF("Expected: %d\n", refReadBuffer[j]);
					__TBILogF("Found: %d\n", g_sTransactionRead1.pui8Buffer[j]);
					return;
				}
			}
			__TBILogF("SUCCESS: support for read immediately following a write?\n");
			__TBILogF("     - Check written output at the slave's read result\n");
			__TBILogF("     - ignore the last %d bytes read by the slave occuring from the following operation\n", g_sTestDescription.ui32Size);
			break;
		}

		case TEST_2READS_CONTINUE:
		{
			/*
			** Check support for 2 following reads with Continue set
			*/
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}
			g_sTransactionRead1.ui32Size		= 10;
			g_sTransactionRead1.i32Cont			= IMG_TRUE;
			g_sTransactionRead2.pui8Buffer      = readBuffer2;
			g_sTransactionRead2.ui32Size		= 10;
			g_sTransactionRead2.i32Read			= IMG_TRUE;
			g_sTransactionRead2.eChipSelect		= eChipSelect;
			g_sTransactionRead2.i32Cont			= 0;
			g_sTransactionRead2.i32InterByteDelay = g_sTestDescription.ui32InterByteDelay;
			if (SPIM_OK != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, &g_sTransactionRead2, NULL))
			{
				__TBILogF("FAIL: read/write operation failed!\n");
				return;
			}
			/* Compare message read from slave with expected MISO message */
			//__TBIDataCacheFlush(g_sTransactionRead1.pui8Buffer, g_sTransactionRead1.ui32Size);
			//__TBIDataCacheFlush(g_sTransactionRead2.pui8Buffer, g_sTransactionRead2.ui32Size);
			//__TBIDataCacheFlush(refReadBuffer, g_sTestDescription.ui32Size);
			for (j=0; j<g_sTransactionRead1.ui32Size; j++)
			{
				if(g_sTransactionRead1.pui8Buffer[j] != refReadBuffer[j])
				{
					__TBILogF("Error in byte %d\n", j);
					__TBILogF("Expected: %d\n", refReadBuffer[j]);
					__TBILogF("Found: %d\n", g_sTransactionRead1.pui8Buffer[j]);
					return;
				}
				if(g_sTransactionRead2.pui8Buffer[j] != refReadBuffer[j+g_sTransactionRead1.ui32Size])
				{
					__TBILogF("Error in byte %d\n", j);
					__TBILogF("Expected: %d\n", refReadBuffer[j+g_sTransactionRead1.ui32Size]);
					__TBILogF("Found: %d\n", g_sTransactionRead2.pui8Buffer[j]);
					return;
				}
			}
			__TBILogF("SUCCESS: support for 2 following reads with Continue set!\n");
			break;
		}

		case TEST_READ_FOLLOWING_DELAYED_READ:
		{
			/*
			** Check support for read following a different delayed read
			*/
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}
			g_sTransactionRead2.pui8Buffer    = readBuffer2;
			g_sTransactionRead2.ui32Size      = 10;
			g_sTransactionRead2.i32Read       = IMG_TRUE;
			g_sTransactionRead2.eChipSelect = eChipSelect;
			g_sTransactionRead2.i32Cont       = 0;
			g_sTransactionRead2.i32InterByteDelay = g_sTestDescription.ui32InterByteDelay;
			
			if (SPIM_OK != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, &g_sTransactionRead2, NULL))
			{
				__TBILogF("FAIL: read/write operation failed!\n");
				return;
			}
			/* Compare message read from slave with expected MISO message */
			//__TBIDataCacheFlush(g_sTransactionRead1.pui8Buffer, g_sTestDescription.ui32Size);
			//__TBIDataCacheFlush(g_sTransactionRead2.pui8Buffer, g_sTransactionRead2.ui32Size);
			//__TBIDataCacheFlush(refReadBuffer, g_sTestDescription.ui32Size);
			for (j=0; j<g_sTestDescription.ui32Size; j++)
			{
				if(g_sTransactionRead1.pui8Buffer[j] != refReadBuffer[j])
				{
					__TBILogF("Error in 1st read and byte %d\n", j);
					__TBILogF("Expected: %d\n", refReadBuffer[j]);
					__TBILogF("Found: %d\n", g_sTransactionRead1.pui8Buffer[j]);
					return;
				}
			}
			for (j=0; j<g_sTransactionRead2.ui32Size; j++)
			{
				if(g_sTransactionRead2.pui8Buffer[j] != refReadBuffer[j])
				{
					__TBILogF("Error in 2nd read and byte %d\n", j);
					__TBILogF("Expected: %d\n", refReadBuffer[j]);
					__TBILogF("Found: %d\n", g_sTransactionRead2.pui8Buffer[j]);
					return;
				}
			}
			__TBILogF("SUCCESS: support for read following a different delayed read!\n");
			break;
		}

		case TEST_DELAYED_READ_FOLLOWING_READ:
		{
			/*
			** Check support for delayed read following a different read
			*/
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}
			if (SPIM_OK != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, NULL, NULL))
			{
				__TBILogF("FAIL: read/write operation failed!\n");
				return;
			}
			/* Compare message read from slave with expected MISO message */
			//__TBIDataCacheFlush(g_sTransactionRead1.pui8Buffer, g_sTestDescription.ui32Size);
			//__TBIDataCacheFlush(refReadBuffer, g_sTestDescription.ui32Size);
			for (j=0; j<g_sTestDescription.ui32Size; j++)
			{
				if(g_sTransactionRead1.pui8Buffer[j] != refReadBuffer[j])
				{
					__TBILogF("Error in 1st read and byte %d\n", j);
					__TBILogF("Expected: %d\n", refReadBuffer[j]);
					__TBILogF("Found: %d\n", g_sTransactionRead1.pui8Buffer[j]);
					return;
				}
			}
			g_sTransactionRead1.pui8Buffer  = readBuffer2;
			g_sTransactionRead1.ui32Size = 10;

			if (SPIM_OK != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionRead1, NULL, NULL))
			{
				__TBILogF("FAIL: read/write operation failed!\n");
				return;
			}
			/* Compare message read from slave with expected MISO message */
			//__TBIDataCacheFlush(g_sTransactionRead1.pui8Buffer, g_sTransactionRead1.ui32Size);
			//__TBIDataCacheFlush(refReadBuffer, g_sTestDescription.ui32Size);
			for (j=0; j<g_sTransactionRead1.ui32Size; j++)
			{
				if(g_sTransactionRead1.pui8Buffer[j] != refReadBuffer[j])
				{
					__TBILogF("Error in 2nd read and byte %d\n", j);
					__TBILogF("Expected: %d\n", refReadBuffer[j]);
					__TBILogF("Found: %d\n", g_sTransactionRead1.pui8Buffer[j]);
					return;
				}
			}
			__TBILogF("SUCCESS: support for delayed read following a different read!\n");
			break;
		}
		case TEST_CHIP_SELECT_SETUP:
		{
			unsigned long	ui32Cmp;
			unsigned long	ui32ParamReg;
			// Fill in arbitrary values for the device we're not targetting.
			if ( g_sTestDescription.iDev != 0 )
			{
				g_sInitParam.sDev0Param.ui8BitRate = 0x1C;
				g_sInitParam.sDev0Param.ui8CSSetup = 0x2B;
				g_sInitParam.sDev0Param.ui8CSHold  = 0x3A;
				g_sInitParam.sDev0Param.ui8CSDelay = 0x49;
			}
			if ( g_sTestDescription.iDev != 1 )
			{
				g_sInitParam.sDev1Param.ui8BitRate = 0x58;
				g_sInitParam.sDev1Param.ui8CSSetup = 0x67;
				g_sInitParam.sDev1Param.ui8CSHold  = 0x76;
				g_sInitParam.sDev1Param.ui8CSDelay = 0x85;
			}
			if ( g_sTestDescription.iDev != 2 )
			{
				g_sInitParam.sDev2Param.ui8BitRate = 0x94;
				g_sInitParam.sDev2Param.ui8CSSetup = 0xA3;
				g_sInitParam.sDev2Param.ui8CSHold  = 0xB2;
				g_sInitParam.sDev2Param.ui8CSDelay = 0xC1;
			}

			// Initialise the SPI master
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}

			// Check the device parameter registers correspond to the settings we chose.
			ui32Cmp = (g_sInitParam.sDev0Param.ui8BitRate << 24) |
					  (g_sInitParam.sDev0Param.ui8CSSetup << 16) |
					  (g_sInitParam.sDev0Param.ui8CSHold << 8)  |
					  (g_sInitParam.sDev0Param.ui8CSDelay);

			ui32ParamReg = (g_sTestDescription.ui32Block == 0)
                           ? SPIM_0_DEVICE_0_PARAM_REG : SPIM_1_DEVICE_0_PARAM_REG ;

			if ( (*(volatile unsigned long *)ui32ParamReg) != ui32Cmp )
			{
				__TBILogF("FAIL: Device 0 parameters are incorrect\n");
				return;
			}
			ui32Cmp = (g_sInitParam.sDev1Param.ui8BitRate << 24) |
					  (g_sInitParam.sDev1Param.ui8CSSetup << 16) |
					  (g_sInitParam.sDev1Param.ui8CSHold << 8)  |
					  (g_sInitParam.sDev1Param.ui8CSDelay);

			ui32ParamReg = (g_sTestDescription.ui32Block == 0)
                           ? SPIM_0_DEVICE_1_PARAM_REG : SPIM_1_DEVICE_1_PARAM_REG ;

			if ( (*(volatile unsigned long *)ui32ParamReg) != ui32Cmp )
			{
				__TBILogF("FAIL: Device 1 parameters are incorrect\n");
				return;
			}
			ui32Cmp = (g_sInitParam.sDev2Param.ui8BitRate << 24) |
					  (g_sInitParam.sDev2Param.ui8CSSetup << 16) |
					  (g_sInitParam.sDev2Param.ui8CSHold << 8)  |
					  (g_sInitParam.sDev2Param.ui8CSDelay);

			ui32ParamReg = (g_sTestDescription.ui32Block == 0)
                           ? SPIM_0_DEVICE_2_PARAM_REG : SPIM_1_DEVICE_2_PARAM_REG ;

			if ( (*(volatile unsigned long *)ui32ParamReg) != ui32Cmp )
			{
				__TBILogF("FAIL: Device 2 parameters are incorrect\n");
				return;
			}

			__TBILogF("SUCCESS: Device parameters are correctly set up\n");
			break;
		}
		case TEST_DEINIT:
		{
			/*
			** Check support for API deinit/reinit
			*/
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}
			if (SPIM_OK != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionWrite1, NULL, NULL))
			{
				__TBILogF("FAIL: read/write operation failed!\n");
				return;
			}
			SPIMDeinit( &g_sSPIMaster );
			if (SPIM_OK != SPIMInit(&g_sSPIMaster, &g_sInitParam))
			{
				__TBILogF("FAIL: initialisation of the SPI master failed!\n");
				return;
			}
			if (SPIM_OK != SPIMReadWrite(&g_sSPIMaster, &g_sTransactionWrite1, NULL, NULL))
			{
				__TBILogF("FAIL: read/write operation failed!\n");
				return;
			}

			__TBILogF("SUCCESS: support for two writes of %d byte?\n", g_sTestDescription.ui32Size);
			__TBILogF("     - Check written output at the slave's read result\n");

			break;
		}
		default:
		{
			__TBILogF("FAIL: Invalid test number specified\n");
			return;
		}
	}

    __TBILogF("Test completed.\n");

	return; /* End of Main Task */
}

/*!
******************************************************************************

 @Function				SetCSTiming

 @Description

 Set the different timings for selected chip select.

 @Return				Selected chip select

******************************************************************************/
SPIM_eDevice SetCSTiming(void)
{
	SPIM_eDevice	eChipSelect;

	/* Set bitRate depending on value entered by user */
	g_sInitParam.sDev0Param.ui8BitRate = (unsigned char) g_sTestDescription.ui32BitRate;
	g_sInitParam.sDev1Param.ui8BitRate = (unsigned char) g_sTestDescription.ui32BitRate;
	g_sInitParam.sDev2Param.ui8BitRate = (unsigned char) g_sTestDescription.ui32BitRate;

	if (g_sTestDescription.iCSSetup == -1)
	{
		/* Set default Chip Select setup, hold and delay times */
		g_sInitParam.sDev0Param.ui8CSSetup = CS_SETUP_VALUE;
		g_sInitParam.sDev0Param.ui8CSHold  = CS_HOLD_VALUE;
		g_sInitParam.sDev0Param.ui8CSDelay = CS_DELAY_VALUE;
		g_sInitParam.sDev1Param.ui8CSSetup = CS_SETUP_VALUE;
		g_sInitParam.sDev1Param.ui8CSHold  = CS_HOLD_VALUE;
		g_sInitParam.sDev1Param.ui8CSDelay = CS_DELAY_VALUE;
		g_sInitParam.sDev2Param.ui8CSSetup = CS_SETUP_VALUE;
		g_sInitParam.sDev2Param.ui8CSHold  = CS_HOLD_VALUE;
		g_sInitParam.sDev2Param.ui8CSDelay = CS_DELAY_VALUE;

		verboseLog("Using default chip select timings\n");
	}
	else
	{
		g_sInitParam.sDev0Param.ui8CSSetup = g_sTestDescription.iCSSetup;
		g_sInitParam.sDev0Param.ui8CSHold  = g_sTestDescription.iCSHold;
		g_sInitParam.sDev0Param.ui8CSDelay = g_sTestDescription.iCSDelay;
		g_sInitParam.sDev1Param.ui8CSSetup = g_sTestDescription.iCSSetup;
		g_sInitParam.sDev1Param.ui8CSHold  = g_sTestDescription.iCSHold;
		g_sInitParam.sDev1Param.ui8CSDelay = g_sTestDescription.iCSDelay;
		g_sInitParam.sDev2Param.ui8CSSetup = g_sTestDescription.iCSSetup;
		g_sInitParam.sDev2Param.ui8CSHold = g_sTestDescription.iCSHold;
		g_sInitParam.sDev2Param.ui8CSDelay = g_sTestDescription.iCSDelay;
	}

	/* Spi Mode */
	g_sInitParam.sDev0Param.eSPIMode = g_sTestDescription.eSPIMode;
	g_sInitParam.sDev1Param.eSPIMode = g_sTestDescription.eSPIMode;
	g_sInitParam.sDev2Param.eSPIMode = g_sTestDescription.eSPIMode;

	/* CS Idle Level */
	g_sInitParam.sDev0Param.ui32CSIdleLevel = g_sTestDescription.ui32CSIdleLevel;
	g_sInitParam.sDev1Param.ui32CSIdleLevel = g_sTestDescription.ui32CSIdleLevel;
	g_sInitParam.sDev2Param.ui32CSIdleLevel = g_sTestDescription.ui32CSIdleLevel;

	/* Data idle Level */
	g_sInitParam.sDev0Param.ui32DataIdleLevel = g_sTestDescription.ui32DataIdleLevel;
	g_sInitParam.sDev1Param.ui32DataIdleLevel = g_sTestDescription.ui32DataIdleLevel;
	g_sInitParam.sDev2Param.ui32DataIdleLevel = g_sTestDescription.ui32DataIdleLevel;

	switch ( g_sTestDescription.iDev )
	{
		case 0:
		{
			eChipSelect = SPIM_DEVICE0;
			break;
		}
		case 1:
		{
			eChipSelect = SPIM_DEVICE1;
			break;
		}
		case 2:
		{
			eChipSelect = SPIM_DEVICE2;
			break;
		}
		case 3:
		{
			eChipSelect = SPIM_DUMMY_CS;
			break;
		}
        default:
		{
                /* keep the compiler happy */
            eChipSelect = SPIM_DUMMY_CS;
		}
	}

	return ( eChipSelect );
}

/*!
******************************************************************************

 @Function				ParseCommandLine

 @Description

 Parses the command line setting test parameters and populates the static test
 description (where applicable). This function should be modified to fit the
 peripheral under test.

 @Return				void - will terminate operation on error

******************************************************************************/
static void ParseCommandLine(int argc, char *argv[])
{
    char *cmd;
    char *option;
    char  tempString[20], *fileName;
    float mhzBitRate;
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

	g_sTestDescription.f_refWrite1 = -1;
	g_sTestDescription.f_refWrite2 = -1;
	g_sTestDescription.f_refRead = -1;
	g_sTestDescription.ui8CmpMask = 0;
    g_sTestDescription.iCSSetup = -1;

    /* Parse command line */
    while (argc>1)
    {
        if (*argv[0] == '-')
        {
            option = (*argv);

            if (strncmp(option, "-test", 5) == 0)
            {
            	//Configure test number
                argv++;
                argc--;
                sscanf(*argv, "%d", &(g_sTestDescription.ui32TestNum));

                verboseLog("Test number %d\n", g_sTestDescription.ui32TestNum);
            }

			else if (strncmp(option, "-cval", 5) == 0)
            {
            	//Configure Compare Data value
                argv++;
                argc--;
                sscanf(*argv, "%x", &(temp));
                g_sTestDescription.ui8CmpValue = (unsigned char)temp;
                verboseLog("Compare data value is: %x\n", g_sTestDescription.ui8CmpValue);
            }

			else if (strncmp(option, "-cmsk", 5) == 0)
            {
            	//Configure Compare Data mask
                argv++;
                argc--;
                sscanf(*argv, "%x", &(temp));
                g_sTestDescription.ui8CmpMask = (unsigned char)temp;
	            verboseLog("Compare data mask is %x\n", g_sTestDescription.ui8CmpMask);
	        }
			else if (strncmp(option, "-ceq", 4) == 0)
            {
            	//Configure Compare Data equal select
                argv++;
                argc--;
                sscanf(*argv, "%d", &(g_sTestDescription.iCmpEq));
				if((g_sTestDescription.iCmpEq < 0)||(g_sTestDescription.iCmpEq > 1))
                {
					verboseLog("Compare Data Equal Select must be 1 or 0\n");
					Usage(cmd);
				}

				if (g_sTestDescription.iCmpEq == 0)
	                verboseLog("Flag will be set when incoming MISO data is not equal to compare data value\n");
	            else if (g_sTestDescription.iCmpEq == 1)
	            	verboseLog("Flag will be set when incoming MISO data is equal to compare data value\n");
            }
			else if (strncmp(option, "-size", 5) == 0)
            {
            	//Configure transfer size
                argv++;
                argc--;
                sscanf(*argv, "%d", &(g_sTestDescription.ui32Size));
                if(g_sTestDescription.ui32Size > 4096)
                {
					verboseLog("Transfer size cannot exceed 4096 bytes\n");
					Usage(cmd);
				}
                verboseLog("Each transfer contains %d bytes\n", g_sTestDescription.ui32Size);
            }
			else if (strncmp(option, "-dev", 5) == 0)
            {
            	//Configure device select
                argv++;
                argc--;
                sscanf(*argv, "%d", &(g_sTestDescription.iDev));
                if(g_sTestDescription.iDev > 3)
                {
					verboseLog("SPI device number cannot be greater than 3\n");
					Usage(cmd);
				}
                verboseLog("SPI device %d selected\n", g_sTestDescription.iDev);
            }
			else if (strncmp(option, "-br", 3) == 0)
            {
            	//Configure bit rate
                argv++;
                argc--;
                sscanf(*argv, "%d", &(g_sTestDescription.ui32BitRate));
                if(g_sTestDescription.ui32BitRate > 0xff)
                {
                    verboseLog("Bitrate valid range 0 to 255\n");
                    Usage(cmd);
                }
                mhzBitRate = g_sTestDescription.ui32BitRate * BITRATE_CONVERSION;
                verboseLog("Bitrate set to %fMHz\n", mhzBitRate);
            }
			else if ( strncmp( option, "-dma", 4 ) == 0 )
			{
				// Configure DMA channel
				argv++;
				argc--;
				sscanf( *argv, "%d", &(g_sTestDescription.ui32DmaChannel) );
				if ( g_sTestDescription.ui32DmaChannel > 10 )
				{
					verboseLog("SPI DMA channel cannot be greater than 10\n");
					Usage(cmd);
				}
				verboseLog("SPI DMA channel: %d\n", g_sTestDescription.ui32DmaChannel );
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
					case 0:	g_sTestDescription.eSPIMode = SPIM_MODE_0;
							break;

					case 1:	g_sTestDescription.eSPIMode = SPIM_MODE_1;
							break;

					case 2:	g_sTestDescription.eSPIMode = SPIM_MODE_2;
							break;

					case 3:	g_sTestDescription.eSPIMode = SPIM_MODE_3;
							break;
				}

                verboseLog("SPI set to Mode %d\n", temp);
            }

            else if (strncmp(option, "-data", 5) == 0)
            {
                //Configure data idle level
                argv++;
                argc--;
                sscanf(*argv, "%d", &(g_sTestDescription.ui32DataIdleLevel));
                if(g_sTestDescription.ui32DataIdleLevel > 1)
                {
                    verboseLog("Data idle level must be set to 0 or 1\n");
                    Usage(cmd);
                }

                verboseLog("Data idle level set to %d\n", g_sTestDescription.ui32DataIdleLevel);
            }

            else if (strncmp(option, "-cs", 3) == 0)
            {
                //Configure CS idle level
                argv++;
                argc--;
                sscanf(*argv, "%d", &(g_sTestDescription.ui32CSIdleLevel));
                if(g_sTestDescription.ui32CSIdleLevel > 1)
                {
                    verboseLog("Chip select idle level must be set to 0 or 1\n");
                    Usage(cmd);
                }

                verboseLog("Chip select idle level set to %d\n", g_sTestDescription.ui32CSIdleLevel);
            }

            else if (strncmp(option, "-hold", 5) == 0)
            {
                //Configure CS hold period
                argv++;
                argc--;
                sscanf(*argv, "%d", &(g_sTestDescription.iCSHold));
                if(g_sTestDescription.iCSHold > 0xff)
                {
                    verboseLog("Chip select hold valid range 0 to 255\n");
                    Usage(cmd);
                }

                verboseLog("Chip select hold period set to %d\n", g_sTestDescription.iCSHold);
            }

            else if (strncmp(option, "-setup", 6) == 0)
            {
                //Configure CS setup period
                argv++;
                argc--;
                sscanf(*argv, "%d", &(g_sTestDescription.iCSSetup));
                if(g_sTestDescription.iCSSetup > 0xff)
                {
                    verboseLog("Chip select setup valid range 0 to 255\n");
                    Usage(cmd);
                }

                verboseLog("Chip select setup period set to %d\n", g_sTestDescription.iCSSetup);
            }

            else if (strncmp(option, "-delay", 6) == 0)
            {
                //Configure CS delay period
                argv++;
                argc--;
                sscanf(*argv, "%d", &(g_sTestDescription.iCSDelay));
                if(g_sTestDescription.iCSDelay > 0xff)
                {
                    verboseLog("Chip select delay valid range 0 to 255\n");
                    Usage(cmd);
                }

                verboseLog("Chip select delay set to %d\n", g_sTestDescription.iCSDelay);
            }

			else if ( strncmp( option, "-bytedelay", 10) == 0 )
			{
				// Configure inter byte delay
				argv++;
				argc--;
				sscanf(*argv, "%d", &(g_sTestDescription.ui32InterByteDelay) );
				if ( g_sTestDescription.ui32InterByteDelay > 1 )
				{
					verboseLog("Interbyte delay select must be either 0 or 1\n");
					Usage(cmd);
				}

				verboseLog("Interbyte delay select set to %d\n", g_sTestDescription.ui32InterByteDelay );
			}

			else if (strncmp(option, "-write1", 7) == 0)
            {
            	//Open write file
            	fileName = tempString;
                argv++;
                argc--;
                sscanf(*argv, "%s", fileName);

                g_sTestDescription.f_refWrite1 = open(fileName, O_RDONLY, 0744);

                if (g_sTestDescription.f_refWrite1 == -1)
                {
                	__TBILogF("Error opening write file %s\n", fileName);
                	Usage(cmd);
                }

                verboseLog("Write1 file name %s\n", fileName);
                writeFile1 = 1;
            }

			else if (strncmp(option, "-write2", 7) == 0)
            {
            	//Open write file
            	fileName = tempString;
                argv++;
                argc--;
                sscanf(*argv, "%s", fileName);

                g_sTestDescription.f_refWrite2 = open(fileName, O_RDONLY, 0744);

                if (g_sTestDescription.f_refWrite2 == -1)
                {
                	__TBILogF("Error opening write file %s\n", fileName);
                	Usage(cmd);
                }

                verboseLog("Write2 file name %s\n", fileName);
                writeFile2 = 1;
            }

            else if (strncmp(option, "-read", 5) == 0)
            {
            	//Open read file
            	fileName = tempString;
                argv++;
                argc--;
                sscanf(*argv, "%s", fileName);

                g_sTestDescription.f_refRead = open(fileName, O_RDONLY, 0744);

                if (g_sTestDescription.f_refRead == -1)
                {
                	__TBILogF("Error opening read file %s\n", fileName);
                	Usage(cmd);
                }

                verboseLog("Read file name %s\n", fileName);
                readFile = 1;
            }

			else if ( !strncmp( option, "-block", 6 ) )
			{
				// Configure block number
				argv++;
				argc--;
				sscanf(*argv, "%d", &(g_sTestDescription.ui32Block) );
				verboseLog("Block number set to %d\n", g_sTestDescription.ui32Block );
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

/*!
******************************************************************************

 @Function				Usage

 @Description

 Outputs correct usage instructions

 @Return				Terminates on completion - never returns

******************************************************************************/
static void Usage(char *cmd)
{
    __TBILogF("\nUsage:  %s <..options..>\n", cmd);
    __TBILogF("          -test N          Test\n");
    __TBILogF("          -cval N	  	  Compare Data Value. Enter as a hex value\n");
    __TBILogF("          -cmsk N	  	  Compare Data Mask. Enter as a hex value\n");
    __TBILogF("          -ceq N		  	  Compare Data Equal Select. 1=Flag when equal. 0=Flag when not equal.\n");
    __TBILogF("          -size N		  Number of bytes in each job (maximum: 4096)\n");
    __TBILogF("          -dev N		  	  SPI device (slave) select\n");
    __TBILogF("          -br N		  	  Bit clock rate value\n");
	__TBILogF("			 -dma N			  SPI DMA channel\n");
    __TBILogF("          -mode N		  SPI Mode. Must be between 0 and 3\n");
    __TBILogF("          -data N          Data idle level\n");
    __TBILogF("          -cs N            Chip Select idle level\n");
    __TBILogF("          -hold N          Chip Select hold period (8-bit)\n");
    __TBILogF("          -setup N         Chip Select setup period (8-bit)\n");
    __TBILogF("          -delay N         Chip Select delay period (8-bit)\n");
	__TBILogF("			 -bytedelay N	  Inter byte delay select\n");
    __TBILogF("          -write1 <filename>Reference write file containing MOSI_1 message\n");
    __TBILogF("          -write2 <filename>Reference write file containing MOSI_2 message\n");
    __TBILogF("          -read <filename> Reference read file containing MISO message\n");
	__TBILogF("			 -block N		  SPI Master block to use\n");

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
	KRN_TASKQ_T hibernateQ;

	ParseCommandLine( argc, argv );

	RunTest();

	DQ_init(&hibernateQ);
	KRN_hibernate( &hibernateQ, KRN_INFWAIT );
    return 1;
}

#else

#error CPU and OS not recognised

#endif
