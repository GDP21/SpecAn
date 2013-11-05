/*!
******************************************************************************
 @file   : thread0.c

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
         Comet MTX load test.

 <b>Platform:</b>\n
	     Platform Independent

 @Version
	     1.0

******************************************************************************/
/*
******************************************************************************
 Modifications :-



*****************************************************************************/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>


/*============================================================================
====	I N C L U D E S
=============================================================================*/


#include "img_defs.h"

//#include <startup_api.h>

#include <MeOS.h>
#include <system.h>

/*============================================================================
====	D E F I N E S
=============================================================================*/

/* Timer defines...*/
#define STACK_SIZE			(1024)

#define STACK_FILL			0xdeadbeef

#define POLL_WAIT			MILLISECOND_TO_TICK(10)

#define	MAX_SIZE			(5120)

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

/* Main Task Control Block */
KRN_TASK_T					g_sMainTask;

/* Stack for the Main Task */
IMG_UINT32					g_aui32TaskStack[STACK_SIZE];

/* QIO structures */
QIO_SYS_T					g_sQio;
QIO_DEVENTRY_T				g_aVectoredDevTable[QIO_NO_VECTORS];

DQ_T						g_sTaskQueue;

KRN_SEMAPHORE_T				sSemaphore;

#define MTX_0				0
#define MTX_1				1

#define START_OF_NVRAM		0

/*
	Include dbn images of demod standards
*/
#include "demod_standard_dvbt.h"
#include "demod_standard_atsc.h"
#include "demod_standard_13seg.h"
#include "demod_standard_tdmb.h"
#include "demod_standard_dvbh.h"
#include "demod_standard_fm.h"
#include "demod_standard_1seg.h"



typedef struct
{
	IMG_UINT16				ui16Func;
	IMG_UINT16				ui16Length;
	IMG_UINT32				ui32ParameterA;
	IMG_UINT32				ui32ParameterB;
	IMG_UINT16				ui16ParameterC;
	IMG_UINT16				ui16Checksum;
}
IMG_sDBNHDR;

#define	FALSE				0
#define	TRUE				1

#define	MTX_CORE_CODE_ADDRESS					0x80880000
#define MTX_CORE_DATA_ADDRESS					0x82880000
#define MTX_CORE_MEM_SIZE						0x00050000

/*--------------------------------------------------------------------------------*/

#define SYSTEM_CONTROL_BASE_ADDRESS				(0x02000000)

#define CR_MC_RSVD0								(*(volatile unsigned long *)(SYSTEM_CONTROL_BASE_ADDRESS + 0x17000 + 0x0004))
#define CR_MC_RSVD1								(*(volatile unsigned long *)(SYSTEM_CONTROL_BASE_ADDRESS + 0x17000 + 0x000C))

#define UCC1_HOST_SYS_BUS						(SYSTEM_CONTROL_BASE_ADDRESS + 0x14000)

#define	MTX_TXENABLE_ADDR						0
#define	MTX_START_EXECUTION						1
#define	MTX_DONT_START_EXECUTION				0

typedef struct
{
	unsigned long								Bracket1;
	unsigned long								MTXStartExecution;
	unsigned long								Bracket2;
}
DBG_Demo;

volatile DBG_Demo								DBGDemo = { 0xC1A0BABE, MTX_START_EXECUTION, 0xB1FFBAFF };

volatile unsigned long							MTXStartExecution = MTX_START_EXECUTION;

#define MTX1_TXENABLE_REG						(*(volatile unsigned long *)(UCC1_HOST_SYS_BUS + MTX_TXENABLE_ADDR))

#define NUM_STANDARDS							7
unsigned long *									paStandard[NUM_STANDARDS] = {	DemodStandard_DVBT,		/* 0 */
																				DemodStandard_ATSC,		/* 1 */
																				DemodStandard_13SEG,		/* 2 */
																				DemodStandard_TDMB,		/* 3 */
																				DemodStandard_DVBH,		/* 4 */
																				DemodStandard_FM,			/* 5 */
																				DemodStandard_1SEG			/* 6 */
																			};

//unsigned long 									MTXFirstStandard = 0; // DVBT
//unsigned long 									MTXFirstStandard = 1; // ATSC
unsigned long 									MTXFirstStandard = 2; // 13SEG
//unsigned long 									MTXFirstStandard = 3; // DAB

#define CR_MC_NO_SWAP_NEEDED					0xFFFFFFFF




	TBISTR_DECL(UCC0CoreLogicalBaseStr,		UCC0_CORE_LOGICAL_BASE);
	TBISTR_DECL(UCC1CoreLogicalBaseStr,		UCC1_CORE_LOGICAL_BASE);
	TBISTR_DECL(UCCBulkLogicalBaseStr,		UCC_BULK_LOGICAL_BASE);

	unsigned long							ui32UCC0CoreLogicalBase = 0xFFFFFFFF;
	unsigned long							ui32UCC1CoreLogicalBase = 0xFFFFFFFF;
	unsigned long							ui32UCCBulkLogicalBase = 0xFFFFFFFF;



/*****************************************************************************************************************/

#define MTX_PC_REG_IND_ADDR			(0x00000005)
#define MTX_A0STP_REG_IND_ADDR		(0x00000003)

void WriteMTXRegIndirect(	unsigned long 	ui32MTXID, unsigned long 	ui32MTXReg, unsigned long 	ui32Value	)
{
	switch ( ui32MTXID )
	{
		case 0:
		{
			*(volatile unsigned long *)(UCC0_HOST_SYS_BUS + 0xF8) = ui32Value;
			*(volatile unsigned long *)(UCC0_HOST_SYS_BUS + 0xFC) = ui32MTXReg;
			return;
		}
		case 1:
		{
			*(volatile unsigned long *)(UCC1_HOST_SYS_BUS + 0xF8) = ui32Value;
			*(volatile unsigned long *)(UCC1_HOST_SYS_BUS + 0xFC) = ui32MTXReg;
			return;
		}
		default:
		{
			IMG_ASSERT(0);
		}
	}
}


void WriteMTXRegDirect(	unsigned long 	ui32MTXID, unsigned long 	ui32MTXReg, unsigned long 	ui32Value	)
{
	switch ( ui32MTXID )
	{
		case 0:
		{
			*(volatile unsigned long *)(UCC0_HOST_SYS_BUS + ui32MTXReg) = ui32Value;
			return;
		}
		case 1:
		{
			*(volatile unsigned long *)(UCC1_HOST_SYS_BUS + ui32MTXReg) = ui32Value;
			return;
		}
		default:
		{
			IMG_ASSERT(0);
		}
	}
}


#define MTX_BULK_BASE_LOGICAL_ADDR		0xB0000000
#define MTX_CORE_BASE_LOGICAL_ADDR		0x82880000

void	LoadCommandReceived	(	unsigned long **	ppLoadData,
								unsigned long		ui32MTXLogicalAddress,
								unsigned long		ui32SizeInBytes		)
{

	unsigned long *				pSrc = *ppLoadData;
	unsigned long *				pDst;
	int							MTXOffset;
	unsigned int				i;

	MTXOffset = (int)ui32MTXLogicalAddress - 0xB0000000;
	if ( MTXOffset >= 0 )
	{
		/* It's a bulk address */
		pDst = (unsigned long *)(ui32UCCBulkLogicalBase + MTXOffset);
	}
	else
	{
		MTXOffset = (int)ui32MTXLogicalAddress - 0x82880000;
		if ( MTXOffset >= 0 )
		{
			/* It's a core DATA address */
			pDst = (unsigned long *)(ui32UCC1CoreLogicalBase + MTXOffset);
		}
		else
		{
			/* It's a core CODE address */
			MTXOffset = (int)ui32MTXLogicalAddress - 0x80880000;
			pDst = (unsigned long *)(ui32UCC1CoreLogicalBase + MTXOffset);
		}
	}

////__TBILogF("Load %lu bytes at address 0x%08lX\n", ui32SizeInBytes, (unsigned long)pDst);

	for (i = 0; i < (ui32SizeInBytes / 4); i++)
	{
		*pDst++ = *pSrc++;
	}

	*ppLoadData = pSrc;
}


void	InitCommandReceived	(	unsigned long		ui32MTXLogicalAddress,
								unsigned long		ui32InitValue,
								unsigned long		ui32SizeInBytes		)
{

	unsigned long *				pDst;
	int							MTXOffset;
	unsigned int				i;

	MTXOffset = (int)ui32MTXLogicalAddress - 0xB0000000;
	if ( MTXOffset >= 0 )
	{
		/* It's a bulk address */
		pDst = (unsigned long *)(ui32UCCBulkLogicalBase + MTXOffset);
	}
	else
	{
		MTXOffset = (int)ui32MTXLogicalAddress - 0x82880000;
		if ( MTXOffset >= 0 )
		{
			/* It's a core DATA address */
			pDst = (unsigned long *)(ui32UCC1CoreLogicalBase + MTXOffset);
		}
		else
		{
			/* It's a core CODE address */
			MTXOffset = (int)ui32MTXLogicalAddress - 0x80880000;
			pDst = (unsigned long *)(ui32UCC1CoreLogicalBase + MTXOffset);
		}
	}

////__TBILogF("Init %lu bytes at address 0x%08lX\n", ui32SizeInBytes, (unsigned long)pDst);

	for (i = 0; i < (ui32SizeInBytes / 4); i++)
	{
		*pDst++ = ui32InitValue;
	}
}


void LoadDBN( unsigned long * pStandard )
{
	IMG_BOOL		bFinished;
	unsigned long * p32 = (unsigned long *)pStandard;
	IMG_sDBNHDR	*	psDBNHeader;
	unsigned long	ui32MTXStartAddress = 0xFFFFFFFF;
	unsigned long	ui32MTXLogicalAddress;
	unsigned long	ui32Length;
	unsigned long	ui32Value;

	bFinished = FALSE;

	while ( !bFinished )
	{
		/* Deal with any padding */
		while ( p32[0] == 0 )
		{
			p32++;
		}

		/* Last read wasn't padding, so parse the remainder of the DBN header */
		psDBNHeader = (IMG_sDBNHDR *)p32;

		switch ( psDBNHeader->ui16Func )
		{
			case 0x23C1:
			{
				/* DBN load */
				/* Parameter A: Address 	*/
				/* Parameter B: Checksum 	*/
				/* Parameter C:	Length		*/

				/* Save candidate start PC if none already saved */
				if ( ui32MTXStartAddress == 0xFFFFFFFF)
				{
					/*
						If the load chunk is a code address then save the address.  If the DBN_End
						record does not have a start PC then we will use this instead (and fingers
						crossed!).
					*/
					if ( (psDBNHeader->ui32ParameterA >= MTX_CORE_CODE_ADDRESS)
							&&
							(psDBNHeader->ui32ParameterA < (MTX_CORE_CODE_ADDRESS + MTX_CORE_MEM_SIZE)) )
					{
						ui32MTXStartAddress = (psDBNHeader->ui32ParameterA & 0xFFF00000) + ((psDBNHeader->ui32ParameterA & 0x000FFFFF)<< 1);
					}
				}

				ui32MTXLogicalAddress = psDBNHeader->ui32ParameterA;
				ui32Length = (unsigned long)(psDBNHeader->ui16ParameterC);
				psDBNHeader++;
				p32 = (unsigned long *)psDBNHeader;

				LoadCommandReceived	( 	&p32,
										ui32MTXLogicalAddress,
										ui32Length	);

				break;
			}

			case 0x23C2:
			{
				/* DBN init */
				/* Parameter A: Address 	*/
				/* Parameter B: Value		*/
				/* Parameter C:	Run length 	*/

				ui32MTXLogicalAddress = psDBNHeader->ui32ParameterA;
				ui32Value = (unsigned long)(psDBNHeader->ui32ParameterB);
				ui32Length = (unsigned long)(psDBNHeader->ui16ParameterC);

				InitCommandReceived	(	ui32MTXLogicalAddress,
										ui32Value,
										ui32Length	);

				psDBNHeader++;
				p32 = (unsigned long *)psDBNHeader;

				break;
			}

			case 0x23E0:
			{
				/* MAP command */
				/* Parameter A: Bulk address[31:8], Physical address[31:24] */
				/* Parameter B: Physical address[23:8], Bytes to map[31:16] */
				/* Parameter C: Bytes to map[15:0] */

				/* !! IGNORE !! - all mapping is done by this application */
				psDBNHeader++;
				p32 = (unsigned long *)psDBNHeader;

				break;
			}

			case 0x23F0:
			{
				/* CONFIG command */
				/* Parameter A: Not used */
				/* Parameter B: Checksum */
				/* Paramater C: Data length */

				/* !! IGNORE !! - all config is done by this application */
				psDBNHeader++;
				p32 = (unsigned long *)psDBNHeader;

				break;
			}

			case 0x23C7:
			{
				/* DBN end */
				/* Parameter A: Program counter */
				/* Parameter B: A0StP			*/
				/* Parameter C: Unused			*/

				if ( psDBNHeader->ui32ParameterA != 0 )
				{
					ui32MTXStartAddress = psDBNHeader->ui32ParameterA;
				}

////__TBILogF("MTX start address = 0x%08lX\n", ui32MTXStartAddress);

				WriteMTXRegIndirect( MTX_1, MTX_PC_REG_IND_ADDR, ui32MTXStartAddress );
				if ( psDBNHeader->ui32ParameterB != 0 )
				{
					WriteMTXRegIndirect( MTX_1, MTX_A0STP_REG_IND_ADDR, psDBNHeader->ui32ParameterB );
				}

				/*
					And start the ball rolling .......
				*/
#if 1
			  {
				volatile unsigned long a;
				a = CR_MC_RSVD1;
				/*
					bits 0-3 in CR_MC_RSVD1 are used to control whether the MTX is started or not.
					If they are zero then MTX is started; if non-zero then not.  This provides a means
					of loading but not starting a demod standard for debugging if necessary.
				*/
				if ((a & 0xF) == 0)
				{
					DBGDemo.MTXStartExecution = MTX_START_EXECUTION;
////					__TBILogF("Starting MTX execution\n");
				}
				else
				{
////					__TBILogF("NOT Starting MTX execution\n");
					DBGDemo.MTXStartExecution = MTX_DONT_START_EXECUTION;
				}
////				__TBILogF("Writing %lu to TXENABLE\n", DBGDemo.MTXStartExecution );
				WriteMTXRegDirect( MTX_1, MTX_TXENABLE_ADDR, DBGDemo.MTXStartExecution );
			  }
#else
				WriteMTXRegDirect( MTX_1, MTX_TXENABLE_ADDR, MTX_START_EXECUTION );
#endif

				bFinished = TRUE;

				break;
			}

			default:
			{
				/* Unknown DBN header type */
				IMG_ASSERT (0);
				break;
			}
		}
	}
}

#define LED_0_OFF_1_OFF			0x3F
#define LED_0_ON_1_ON			0x27

#define LED_0_OFF_1_ON			0x37
#define LED_0_ON_1_OFF			0x2F

unsigned long			LEDValueA = LED_0_ON_1_ON;
unsigned long			LEDValueB = LED_0_ON_1_ON;
unsigned long			LEDValue = LED_0_ON_1_ON;

unsigned long			MTXPollCount = 0;
#define MTX_POLL_LIMIT	10000

#define DEMOD_SWAP_POLL_INTERVAL			250										// Poll every this milliseconds
#define DEMOD_SWAP_RESET_WAIT				(DEMOD_SWAP_POLL_INTERVAL * 4)			// Wait after resetting MTX for a second
#define	CHECK_STATUP_DELAY_COUNT			(32500 / DEMOD_SWAP_POLL_INTERVAL)		// Delay MTX startup by 32.5 seconds
volatile unsigned long		CheckCounter = 0;

void ClearMTXCoreRAM( void )
{
	#define MTX_CORE_RAM_SIZE_IN_BYTES		(320 * 1024)
	#define MTX_CORE_RAM_SIZE_IN_DWORDS		(MTX_CORE_RAM_SIZE_IN_BYTES >> 2)
	unsigned long				i;
	volatile unsigned long *	p;

	p = (volatile unsigned long *)ui32UCC1CoreLogicalBase;
	for (i = 0; i < MTX_CORE_RAM_SIZE_IN_DWORDS; i++)
	{
		p[i] = 0;
	}
}


/*!
******************************************************************************

 @Function				CheckForStandardSwap

******************************************************************************/
void CheckForStandardSwap( void )
{
	unsigned long *	pStandard = 0;
	unsigned long	NextDemodIndex;

	/*
		Delay checking for any standard swap from system reset to allow Linux
		thread to get up and running first.
	*/
	if ( CheckCounter < CHECK_STATUP_DELAY_COUNT )
	{
		CheckCounter++;
		if ( CheckCounter < CHECK_STATUP_DELAY_COUNT )
		{
			return;
		}
	}

	/*
		Check for standard swap (or first standard from system reset)
	*/
	if (CR_MC_RSVD0 != CR_MC_NO_SWAP_NEEDED)
	{
////		__TBILogF("Standard swap to %lu detected\n", CR_MC_RSVD0);

		/*
			MTX (or hard reset default) has indicated a standard swap.  The CR_MC_RSVD0
			register holds the ID of the next standard to load.  Wait for the MTX TXENABLE
			register to show that the MTX processor is not running, load the new standard and let it rip!
		*/
////		__TBILogF("Waiting for MTX idle ...\n");
		MTXPollCount = 0;
		while ( (MTXPollCount < MTX_POLL_LIMIT) && (MTX1_TXENABLE_REG & 1) )
		{
			MTXPollCount++;
		}

		if (MTXPollCount >= MTX_POLL_LIMIT)
		{
////		__TBILogF("Timeout waiting for MTX idle\n");
		}
		else
		{
////		__TBILogF("MTX idle okay after %lu polls\n", MTXPollCount);
		}

		/*
			Change LED pattern to indicate change of standard
		*/
#if 0
		if ( ( LEDValue == LED_0_OFF_1_OFF ) || ( LEDValue == LED_0_ON_1_ON ) )
		{
			/* Change from both on/off to alternating one on, one off */
			LEDValueA = LED_0_OFF_1_ON;
			LEDValueB = LED_0_ON_1_OFF;
		}
		else
		{
			/* Change from alternating one on, one off to both on/off */
			LEDValueA = LED_0_OFF_1_OFF;
			LEDValueB = LED_0_ON_1_ON;
		}
#endif

		/*
			For belts and braces try resetting both MTX and UCC too
		*/
		*((volatile unsigned long *)0x02014200) = 0x00000001;
//		*((volatile unsigned long *)0x02015004) = 0x80000000;  // Harvey didn't need this so I've disabled it too

		KRN_hibernate( &g_sTaskQueue, MILLISECOND_TO_TICK(DEMOD_SWAP_RESET_WAIT) );

#if 0
		IMG_ASSERT( CR_MC_RSVD0 < NUM_STANDARDS );

		pStandard = paStandard[CR_MC_RSVD0];
#else
  #if 0
////		__TBILogF("Forcing next standard to be DAB due to problem in DAB reset function (I presume)\n");
		pStandard = paStandard[3];
  #else
		// DAB reset seems to set a stupid value (= 0x32F6)
		if (CR_MC_RSVD0 >= NUM_STANDARDS)
		{

			// If so use top nibble of CR_MC_RSVD1 as index instead.
			NextDemodIndex = CR_MC_RSVD1;
			NextDemodIndex >>= 28;
			CR_MC_RSVD1 &= 0x00FFFFFF;

////			__TBILogF("Next standard is %lu\n", NextDemodIndex );
			pStandard = paStandard[NextDemodIndex];
		}
		else
		{
			NextDemodIndex = CR_MC_RSVD0;
			pStandard = paStandard[NextDemodIndex];
		}
    #endif

#endif

		if ( MTXFirstStandard != 0xFFFFFFFF )
		{
			// Override first standard loaded (for debugging)
			pStandard = paStandard[MTXFirstStandard];
////			__TBILogF("Forcing standard swap to %lu\n", MTXFirstStandard);
			NextDemodIndex = MTXFirstStandard;
			MTXFirstStandard = 0xFFFFFFFF;
		}

		ClearMTXCoreRAM();

		LoadDBN( pStandard );

		CR_MC_RSVD0 = CR_MC_NO_SWAP_NEEDED;

		// Save new (current) standard in bits 24-27 of CR_MC_RSVD1 for debug purposes
		CR_MC_RSVD1 &= ~(0xF << 24);
		CR_MC_RSVD1 |= (NextDemodIndex << 24);
	}

	return;
}


/*!
******************************************************************************

 @Function				SetUpMTX1BulkRAM

******************************************************************************/
void SetUpMTX1BulkRAM( void )
{
	/* Map all of MTX1's bulk memory to the top of DDR */
	*(unsigned long *)0x02017010 = (0xB0000000 >> 2) ;
	*(unsigned long *)0x02017014 = (0xB03C0000 >> 2);
	*(unsigned long *)0x02017018 = (0xBF400000 >> 2);
	*(unsigned long *)0x0201701C = 2 ;

	return;
}


/*!
******************************************************************************

 @Function				SetUpMTX1BulkRAM

******************************************************************************/
static void GetTBIStrings( void )
{
	unsigned long *		p;

	p = (unsigned long *) __TBITransStr( UCC0CoreLogicalBaseStr, strlen(UCC0CoreLogicalBaseStr));
	ui32UCC0CoreLogicalBase = *p;

	p = (unsigned long *) __TBITransStr( UCC1CoreLogicalBaseStr, strlen(UCC1CoreLogicalBaseStr));
	ui32UCC1CoreLogicalBase = *p;

	p = (unsigned long *) __TBITransStr( UCCBulkLogicalBaseStr, strlen(UCCBulkLogicalBaseStr));
	ui32UCCBulkLogicalBase = *p;

	return;
}


/*!
******************************************************************************

 @Function				MainTask

******************************************************************************/
void MainTask( void )
{
#if 0
	volatile unsigned long * pLEDEnable = (volatile unsigned long *)0x02006500;
	volatile unsigned long * pLED = (volatile unsigned long *)0x02006504;
	unsigned long			LEDCount = 0;
#endif

	GetTBIStrings();
	SetUpMTX1BulkRAM();

#if 0
	*pLEDEnable = 0x02FF0018;
#endif

	while (1)
	{
#if 0
		LEDCount++;
		if (( LEDCount % 4) == 0 )
		{
			*pLED = LEDValue;
			if (LEDValue == LEDValueA)
			{
				LEDValue = LEDValueB;
			}
			else
			{
				LEDValue = LEDValueA;
			}
		}
#endif

		/* Wait for indication that demod standard is to be swapped */
		CheckForStandardSwap();
		KRN_hibernate( &g_sTaskQueue, MILLISECOND_TO_TICK(DEMOD_SWAP_POLL_INTERVAL) );
	}

   return;
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
              &QIO_META12xIVDesc,
              TBID_SIGNUM_TR2(__TBIThreadId() >> TBID_THREAD_S),
              IMG_NULL,
              0);

	/* Start Meos Main Timer Task...*/
	g_psTimerTask = KRN_startTimerTask("Timer Task", g_aui32TimerStack, TIM_STACK_SIZE, TIM_TICK_PERIOD);

	DQ_init( &g_sTaskQueue );

  	/* Start the main task...*/
  	KRN_startTask(MainTask, &g_sMainTask, g_aui32TaskStack, STACK_SIZE, KRN_LOWEST_PRIORITY+1, IMG_NULL, "MainTask");

 	for (;;)
	{
		KRN_hibernate( &g_sTaskQueue, KRN_INFWAIT );
	}

	return;
}

/*!
******************************************************************************

 @Function				main

******************************************************************************/
//#if defined __MTX_MEOS__ || defined __META_MEOS__

int main(int argc, char **argv)
{
	KRN_TASKQ_T	sHibernateQ;

    (void)argc;     /* Remove warnings about unused parameters */
    (void)argv;     /* Remove warnings about unused parameters */

	RunTest();

	DQ_init( &sHibernateQ );
	KRN_hibernate( &sHibernateQ, KRN_INFWAIT );
  return 1;
}

/*#else

#error CPU and OS not recognised

#endif*/

