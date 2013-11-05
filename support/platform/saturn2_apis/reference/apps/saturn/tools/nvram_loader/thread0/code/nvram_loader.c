/*!
*******************************************************************************
  file   nvram_loader.c

  brief  Saturn NVRAM Loader Application

  author Imagination Technologies

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

*******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log: nvram_loader.c,v $

  --- Revision Logs Removed --- 

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
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

#include <MeOS.h>
#include <img_defs.h>
#include <system.h>
#include <sys_util.h>
#include <sys_config.h>

#include "nvram_api.h"

/*============================================================================
====	D E F I N E S
=============================================================================*/

/* Timer defines...*/
#define STACK_SIZE			(3096)

#define STACK_FILL			(0xdeadbeef)

// Default settings for 36 Mhz FPGA clock
#define CS_SETUP_FLASH		(0x1)
#define CS_HOLD_FLASH		(0x1)
#define CS_DELAY_FLASH		(0x8)

#define CS_SETUP_EEPROM		(0x9)
#define CS_HOLD_EEPROM		(0x9)
#define CS_DELAY_EEPROM		(0x9)

#define CS_IDLE_LEVEL		(1)
#define DATA_IDLE_LEVEL		(0)

#define API_SCRATCH_SIZE	(2048)

#define MAX_ONE_OFF_SIZE	(32 * 1024) // This must be <= the amount (and, <= half the amount if verification is needed ) of free memory after the end of the $data section...

#define NUM_NVRAM_DEVICES	(5)

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

/*============================================================================
====	T Y P E D E F S
=============================================================================*/

typedef struct TEST_tag_sDescription
{
	IMG_UINT32		ui32BitRate;
	IMG_UINT32		ui32Device;
	IMG_UINT32		ui32Size;
	IMG_UINT32		ui32Address;
	IMG_INT			fBin;
	IMG_BOOL		bLoadBin;
	IMG_BOOL		bSaveBin;
	IMG_BOOL		bVerify;
	IMG_BOOL		bErase;
	IMG_UINT32		ui32BlockIndex;
} TEST_sDescription;

typedef struct TEST_tag_sContext
{
	KRN_TASK_T		*	psBackgroundTask;								// Background Task Control Block
	KRN_TASK_T		*	psTimerTask;									// Timer Task Control Block
	KRN_SCHEDULE_T		sMeOSScheduler;									// MEOS Scheduler
	KRN_TASKQ_T			asSchedQueues[ MEOS_MAX_PRIORITY_LEVEL + 1 ];	// Scheduler queues
	IMG_UINT32			aui32TimerStack[TIM_STACK_SIZE];				// Stack for the Timer Task
	KRN_TASK_T			sMainTask;										// Main Task Control Block
	IMG_UINT32			aui32TaskStack[STACK_SIZE];						// Stack for the Main Task
	QIO_DEVENTRY_T		aVectoredDevTable[QIO_NO_VECTORS];				// QIO structures
	QIO_SYS_T			sQio;

	SPIM_sBlock			sSPIMaster;
	SPIM_sInitParam		sSPIInitParam;

	NVRAM_sDevice		sDevice[ NUM_NVRAM_DEVICES ];

	SPIM_eDevice		eSPIChipSelect;
} TEST_sContext;

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


/* The following line is a hack used to locate the end of the normal data section. This assumes the linked will place the .endofdata section after the data section. If it does,
   we can use the memory between the end of $data and the end of the physical memory to read in large files. Yes, this is dodgy... */
__attribute__ ((__section__(".endofdata"))) IMG_UINT8 g_ui8EndOfDataDummy;

IMG_UINT8	*	g_pui8Data;
#define DATA_SECTION	__attribute__ ((__section__(".bulkbuffers")))
//#define DATA_SECTION
DATA_SECTION IMG_UINT8	g_aui8Scratch[ API_SCRATCH_SIZE ];

static TEST_sContext		g_sTESTContext = { 0 };
static TEST_sDescription	g_sTESTDescription = { 0 };

/*============================================================================
====	F U N C T I O N S
=============================================================================*/

/*!
******************************************************************************

 @Function				SetupSPIM

******************************************************************************/

IMG_BOOL SetupSPIM()
{
	g_sTESTContext.sSPIInitParam.sDev0Param.ui8BitRate = (unsigned char)g_sTESTDescription.ui32BitRate;
	g_sTESTContext.sSPIInitParam.sDev0Param.ui8CSSetup = CS_SETUP_FLASH;
	g_sTESTContext.sSPIInitParam.sDev0Param.ui8CSHold = CS_HOLD_FLASH;
	g_sTESTContext.sSPIInitParam.sDev0Param.ui8CSDelay = CS_DELAY_FLASH;
	g_sTESTContext.sSPIInitParam.sDev0Param.eSPIMode = SPIM_MODE_0;
	g_sTESTContext.sSPIInitParam.sDev0Param.ui32CSIdleLevel = CS_IDLE_LEVEL;
	g_sTESTContext.sSPIInitParam.sDev0Param.ui32DataIdleLevel = DATA_IDLE_LEVEL;

	g_sTESTContext.sSPIInitParam.sDev1Param.ui8BitRate = (unsigned char)g_sTESTDescription.ui32BitRate;
	g_sTESTContext.sSPIInitParam.sDev1Param.ui8CSSetup = CS_SETUP_EEPROM;
	g_sTESTContext.sSPIInitParam.sDev1Param.ui8CSHold = CS_HOLD_EEPROM;
	g_sTESTContext.sSPIInitParam.sDev1Param.ui8CSDelay = CS_DELAY_EEPROM;
	g_sTESTContext.sSPIInitParam.sDev1Param.eSPIMode = SPIM_MODE_0;
	g_sTESTContext.sSPIInitParam.sDev1Param.ui32CSIdleLevel = CS_IDLE_LEVEL;
	g_sTESTContext.sSPIInitParam.sDev1Param.ui32DataIdleLevel = DATA_IDLE_LEVEL;

	g_sTESTContext.sSPIInitParam.sDev2Param.ui8BitRate = (unsigned char)g_sTESTDescription.ui32BitRate;
	g_sTESTContext.sSPIInitParam.sDev2Param.ui8CSSetup = CS_SETUP_EEPROM;
	g_sTESTContext.sSPIInitParam.sDev2Param.ui8CSHold = CS_HOLD_EEPROM;
	g_sTESTContext.sSPIInitParam.sDev2Param.ui8CSDelay = CS_DELAY_EEPROM;
	g_sTESTContext.sSPIInitParam.sDev2Param.eSPIMode = SPIM_MODE_0;
	g_sTESTContext.sSPIInitParam.sDev2Param.ui32CSIdleLevel = CS_IDLE_LEVEL;
	g_sTESTContext.sSPIInitParam.sDev2Param.ui32DataIdleLevel = DATA_IDLE_LEVEL;

	g_sTESTContext.sSPIInitParam.ui32DMAChannel 	= 0;
	g_sTESTContext.sSPIInitParam.ui32BlockIndex		= g_sTESTDescription.ui32BlockIndex;

	if ( SPIMInit( &g_sTESTContext.sSPIMaster, &g_sTESTContext.sSPIInitParam ) != SPIM_OK )
	{
		return IMG_FALSE;
	}

	return IMG_TRUE;
}

/*!
******************************************************************************

 @Function				MainTask

******************************************************************************/
void MainTask( void )
{
	img_uint32			i;
	NVRAM_eResult		eResult;
	NVRAM_sDevice	*	psDevice = IMG_NULL;
	SYS_sConfig			sConfig;
	SYS_sSPIMConfig		sSPIMConfig[2];
	
	IMG_MEMSET( &sConfig, 0, sizeof( SYS_sConfig ) );
	SYS_Configure( &sConfig );

	IMG_MEMSET( &sSPIMConfig, 0, 2 * sizeof( SYS_sSPIMConfig ) );
	sSPIMConfig[g_sTESTDescription.ui32BlockIndex].bConfigure = IMG_TRUE;
	sSPIMConfig[g_sTESTDescription.ui32BlockIndex].bEnable = IMG_TRUE;

	sSPIMConfig[g_sTESTDescription.ui32BlockIndex].bTargetFrequency = IMG_TRUE;
	sSPIMConfig[g_sTESTDescription.ui32BlockIndex].ui32TargetFreq_fp = 0xE00000; // 15MHz in 12.20 format;
	
	SYS_ConfigureSPIM( sSPIMConfig );

	// Setup SPI Master
	if ( !SetupSPIM() )
	{
		__TBILogF("Error initialising SPI Master!\n");
		return;
	}

	if ( g_sTESTDescription.ui32Size == 0 )
	{
		// Get size of file
		struct stat		info;
		fstat( g_sTESTDescription.fBin, &info );
		g_sTESTDescription.ui32Size = info.st_size;
	}

	if ( g_sTESTDescription.fBin != -1 && g_sTESTDescription.ui32Size < MAX_ONE_OFF_SIZE )
	{
		g_pui8Data = &g_ui8EndOfDataDummy;
		read( g_sTESTDescription.fBin, g_pui8Data, g_sTESTDescription.ui32Size );
	}

	// Initialise the NVRAM API
	NVRAM_Initialise( &g_sTESTContext.sSPIMaster, g_aui8Scratch, API_SCRATCH_SIZE );

	// Setup the device structures
	// EEPROM
	// 256kBit M95256
	IMG_MEMSET( &g_sTESTContext.sDevice[0], 0, sizeof( NVRAM_sDevice ) );
	g_sTESTContext.sDevice[0].eChipSelect = SPIM_DEVICE0;
	g_sTESTContext.sDevice[0].ui32BlockSize = 64;
	g_sTESTContext.sDevice[0].ui32NumBlocks = 512;
	g_sTESTContext.sDevice[0].ui8AddressWidth = 2;
	g_sTESTContext.sDevice[0].bUseAPIErase = IMG_TRUE;
	g_sTESTContext.sDevice[0].bUseAPIWriteN = IMG_FALSE;
	g_sTESTContext.sDevice[0].bUseWriteEnable = IMG_TRUE;
	NVRAM_InitDevice( &g_sTESTContext.sDevice[0] );

	// FLASH
	// M25PE80
	IMG_MEMSET( &g_sTESTContext.sDevice[1], 0, sizeof( NVRAM_sDevice ) );
	g_sTESTContext.sDevice[1].eChipSelect			= SPIM_DEVICE0;
	g_sTESTContext.sDevice[1].ui32BlockSize			= 256;
	g_sTESTContext.sDevice[1].ui32NumBlocks			= 4096;
	g_sTESTContext.sDevice[1].ui8AddressWidth		= 3;
	g_sTESTContext.sDevice[1].bOverrideWriteInstr	= IMG_TRUE;
	g_sTESTContext.sDevice[1].ui8WriteInstr			= 0x0A;
	g_sTESTContext.sDevice[1].bUseAPIWriteN			= IMG_TRUE;
	g_sTESTContext.sDevice[1].ui8EraseInstr			= 0xDB;
	g_sTESTContext.sDevice[1].bUseWriteEnable		= IMG_TRUE;
	NVRAM_InitDevice( &g_sTESTContext.sDevice[1] );

	// ATMEL AT45DB041D-SU 4Mbit 66MHz Flash
	// This device will read data at the low frequency mode (33Mhz)
	IMG_MEMSET( &g_sTESTContext.sDevice[2], 0, sizeof( NVRAM_sDevice ) );
	g_sTESTContext.sDevice[2].eChipSelect						= SPIM_DEVICE0;
	g_sTESTContext.sDevice[2].ui32BlockSize						= 264;
	g_sTESTContext.sDevice[2].ui32NumBlocks						= 1986; // This isn't exactly correct for this device but shouldn't affect operation
	g_sTESTContext.sDevice[2].ui8AddressWidth					= 3;
	g_sTESTContext.sDevice[2].bOverrideWriteInstr				= IMG_TRUE;
	g_sTESTContext.sDevice[2].ui8WriteInstr						= 0x82;
	g_sTESTContext.sDevice[2].bUseAPIWriteN						= IMG_TRUE;
	g_sTESTContext.sDevice[2].ui8EraseInstr						= 0x81;
	g_sTESTContext.sDevice[2].bOverrideStatusRegBehaviour		= IMG_TRUE;
	g_sTESTContext.sDevice[2].ui8StatusRegInstr					= 0xD7;
	g_sTESTContext.sDevice[2].ui8StatusRegReadyBit				= 7;
	g_sTESTContext.sDevice[2].ui8StatusRegReadyBitReadyValue	= 0x1;
	g_sTESTContext.sDevice[2].bOverrideReadInstr				= IMG_TRUE;
	g_sTESTContext.sDevice[2].ui8ReadInstr						= 0xe8;
	g_sTESTContext.sDevice[2].ui8PaddingBytes					= 4;
	g_sTESTContext.sDevice[2].ui8BlockAddressBitBase			= 9;
	NVRAM_InitDevice( &g_sTESTContext.sDevice[2] );

	// ATMEL AT45DB321D 32Mbit Flash
	IMG_MEMSET( &g_sTESTContext.sDevice[3], 0, sizeof( NVRAM_sDevice ) );
	g_sTESTContext.sDevice[3].eChipSelect						= SPIM_DEVICE0;
	g_sTESTContext.sDevice[3].ui32BlockSize						= 528;
	g_sTESTContext.sDevice[3].ui32NumBlocks						= 8192;
	g_sTESTContext.sDevice[3].ui8AddressWidth					= 3;
	g_sTESTContext.sDevice[3].bOverrideWriteInstr				= IMG_TRUE;
	g_sTESTContext.sDevice[3].ui8WriteInstr						= 0x82;
	g_sTESTContext.sDevice[3].bUseAPIWriteN						= IMG_TRUE;
	g_sTESTContext.sDevice[3].ui8EraseInstr						= 0x81;
	g_sTESTContext.sDevice[3].bOverrideStatusRegBehaviour		= IMG_TRUE;
	g_sTESTContext.sDevice[3].ui8StatusRegInstr					= 0xD7;
	g_sTESTContext.sDevice[3].ui8StatusRegReadyBit				= 7;
	g_sTESTContext.sDevice[3].ui8StatusRegReadyBitReadyValue	= 0x1;
	g_sTESTContext.sDevice[3].bOverrideReadInstr				= IMG_TRUE;
	g_sTESTContext.sDevice[3].ui8ReadInstr						= 0xE8;
	g_sTESTContext.sDevice[3].ui8PaddingBytes					= 4;
	g_sTESTContext.sDevice[3].ui8BlockAddressBitBase			= 10;
	NVRAM_InitDevice( &g_sTESTContext.sDevice[3] );

	// ATMEL AT45DB642D 64Mbit Flash
	IMG_MEMSET( &g_sTESTContext.sDevice[4], 0, sizeof( NVRAM_sDevice ) );
	g_sTESTContext.sDevice[4].eChipSelect						= SPIM_DEVICE0;
	g_sTESTContext.sDevice[4].ui32BlockSize						= 1056;
	g_sTESTContext.sDevice[4].ui32NumBlocks						= 8192;
	g_sTESTContext.sDevice[4].ui8AddressWidth					= 3;
	g_sTESTContext.sDevice[4].bOverrideWriteInstr				= IMG_TRUE;
	g_sTESTContext.sDevice[4].ui8WriteInstr						= 0x82;
	g_sTESTContext.sDevice[4].bUseAPIWriteN						= IMG_TRUE;
	g_sTESTContext.sDevice[4].ui8EraseInstr						= 0x81;
	g_sTESTContext.sDevice[4].bOverrideStatusRegBehaviour		= IMG_TRUE;
	g_sTESTContext.sDevice[4].ui8StatusRegInstr					= 0xD7;
	g_sTESTContext.sDevice[4].ui8StatusRegReadyBit				= 7;
	g_sTESTContext.sDevice[4].ui8StatusRegReadyBitReadyValue	= 0x1;
	g_sTESTContext.sDevice[4].bOverrideReadInstr				= IMG_TRUE;
	g_sTESTContext.sDevice[4].ui8ReadInstr						= 0xE8;
	g_sTESTContext.sDevice[4].ui8PaddingBytes					= 4;
	g_sTESTContext.sDevice[4].ui8BlockAddressBitBase			= 11;
	NVRAM_InitDevice( &g_sTESTContext.sDevice[4] );

	IMG_ASSERT( g_sTESTDescription.ui32Device < NUM_NVRAM_DEVICES );

	psDevice = &g_sTESTContext.sDevice[ g_sTESTDescription.ui32Device ];

	// Does the user want to erase memory ?
	if ( g_sTESTDescription.bErase )
	{
		i = 0;
		while ( i < g_sTESTDescription.ui32Size )
		{
			eResult = NVRAM_BlockErase( psDevice, g_sTESTDescription.ui32Address + i * psDevice->ui32BlockSize );
			i += psDevice->ui32BlockSize;
		}
	}

	// Does the user want to load a binary ?
	if ( g_sTESTDescription.bLoadBin )
	{
		// As the MTX has limited memory we can only write a certain amount of data at a time. See if we can do this quickly in one pass.
		if ( g_sTESTDescription.ui32Size < MAX_ONE_OFF_SIZE )
		{
			// Write the binary file to device
			eResult = NVRAM_WriteN( psDevice, g_sTESTDescription.ui32Address, g_pui8Data, g_sTESTDescription.ui32Size );
			if ( eResult != NVRAM_SUCCESS )
			{
				__TBILogF("Error writing binary to memory device!\n");
				return;
			}
			// If we must check the data...
			if ( g_sTESTDescription.bVerify )
			{
				IMG_UINT8	*	pui8ReadBack = g_pui8Data + g_sTESTDescription.ui32Size;
				memset( pui8ReadBack, 0, g_sTESTDescription.ui32Size );
				// Read the data back from the device
				eResult = NVRAM_ReadN( psDevice, g_sTESTDescription.ui32Address, pui8ReadBack, g_sTESTDescription.ui32Size );
				if ( eResult != NVRAM_SUCCESS )
				{
					__TBILogF("Error reading back data from flash!\n");
					return;
				}
				// And compare
				for ( i = 0; i < g_sTESTDescription.ui32Size; ++i )
				{
					if ( pui8ReadBack[i] != g_pui8Data[i] )
					{
						__TBILogF("Error verifying data at position %d: %02X should be %02X!\n", i, pui8ReadBack[i], g_pui8Data[i] );
						return;
					}
				}
				__TBILogF("Data in flash is correct!\n");
				return;
			}
		}
		else
		{
			// We need to read in a chunk from file and write it out
			IMG_UINT32	ui32BytesLeft = g_sTESTDescription.ui32Size;
			IMG_UINT32	ui32BytesToWrite, ui32BytesToRead;
			IMG_UINT32	ui32Offset = 0;
			g_pui8Data = g_aui8Scratch + API_SCRATCH_SIZE;
			while ( ui32BytesLeft > 0 )
			{
				if ( ui32BytesLeft > MAX_ONE_OFF_SIZE )
				{
					ui32BytesToWrite = MAX_ONE_OFF_SIZE;
				}
				else
				{
					ui32BytesToWrite = ui32BytesLeft;
				}
				// Seek to the place
				lseek( g_sTESTDescription.fBin, ui32Offset, SEEK_SET );

				// Read in chunk from file
				read( g_sTESTDescription.fBin, g_pui8Data, ui32BytesToWrite );

				// Write to memory device
				eResult = NVRAM_WriteN( psDevice, g_sTESTDescription.ui32Address + ui32Offset, g_pui8Data, ui32BytesToWrite );
				if ( eResult != NVRAM_SUCCESS )
				{
					__TBILogF("Error writing binary to memory device!\n");
					return;
				}

				ui32BytesLeft -= ui32BytesToWrite;
				ui32Offset += ui32BytesToWrite;
			}

			// Do we verify the data ?
			if ( g_sTESTDescription.bVerify )
			{
				IMG_UINT8	*	pui8ReadBack = g_pui8Data + MAX_ONE_OFF_SIZE;
				ui32BytesLeft = g_sTESTDescription.ui32Size;
				ui32Offset = 0;

				while ( ui32BytesLeft > 0 )
				{
					if ( ui32BytesLeft > MAX_ONE_OFF_SIZE )
					{
						ui32BytesToRead = MAX_ONE_OFF_SIZE;
					}
					else
					{
						ui32BytesToRead = ui32BytesLeft;
					}
					// Clear the read-from-memory-device buffer
					IMG_MEMSET( pui8ReadBack, 0, MAX_ONE_OFF_SIZE );
					// Seek to the file position
					lseek( g_sTESTDescription.fBin, ui32Offset, SEEK_SET );
					// Read in chunk from file
					read( g_sTESTDescription.fBin, g_pui8Data, ui32BytesToRead );
					// Read in chunk from memory device
					eResult = NVRAM_ReadN( psDevice, g_sTESTDescription.ui32Address + ui32Offset, pui8ReadBack, ui32BytesToRead );
					if ( eResult != NVRAM_SUCCESS )
					{
						__TBILogF("Error reading data back from memory device!\n");
						return;
					}
					for ( i = 0; i < ui32BytesToRead; ++i )
					{
						if ( pui8ReadBack[i] != g_pui8Data[i] )
						{
							__TBILogF("Error verifying data at position %d: %02X should be %02X!\n", i, pui8ReadBack[i], g_pui8Data[i] );
							return;
						}
					}

					ui32BytesLeft -= ui32BytesToRead;
					ui32Offset += ui32BytesToRead;

				}

				__TBILogF("Data in flash is correct!\n");
				return;
			}
		}
	}
	else if ( g_sTESTDescription.bSaveBin )
	{
		img_uint32	ui32BytesLeft = g_sTESTDescription.ui32Size;
		img_uint32	ui32BytesToRead;
		img_uint32	ui32Offset = 0;
		g_pui8Data = g_aui8Scratch + API_SCRATCH_SIZE;
		while ( ui32BytesLeft > 0 )
		{
			if ( ui32BytesLeft > MAX_ONE_OFF_SIZE )
			{
				ui32BytesToRead = MAX_ONE_OFF_SIZE;
			}
			else
			{
				ui32BytesToRead = ui32BytesLeft;
			}
			// Read from flash
			eResult = NVRAM_ReadN( psDevice, g_sTESTDescription.ui32Address + ui32Offset, g_pui8Data, ui32BytesToRead );
			if ( eResult != NVRAM_SUCCESS )
			{
				__TBILogF("Error reading binary from memory device!\n");
				return;
			}
			// Write to file
			write( g_sTESTDescription.fBin, g_pui8Data, ui32BytesToRead );

			ui32BytesLeft -= ui32BytesToRead;
			ui32Offset += ui32BytesToRead;
		}

		close( g_sTESTDescription.fBin );
	}

	if ( eResult != NVRAM_SUCCESS )
	{
		__TBILogF("Error transferring data\n");
		return;
	}

	return; /* End of Main Task */
}

/*!
******************************************************************************

 @Function				RunTest

******************************************************************************/
static img_void RunTest( img_void )
{

	/* Reset the Kernel */
    KRN_reset(&(g_sTESTContext.sMeOSScheduler),
              g_sTESTContext.asSchedQueues,
              MEOS_MAX_PRIORITY_LEVEL,
              STACK_FILL,
              IMG_NULL,
              0);

	/* Start Meos Main Task - no background task if MeOS Abstraction Layer */
	g_sTESTContext.psBackgroundTask = KRN_startOS("Background Task");

	/* Reset QIO */
    QIO_reset(&g_sTESTContext.sQio,
              g_sTESTContext.aVectoredDevTable,
              QIO_NO_VECTORS,
              &QIO_MTPIVDesc,
              TBID_SIGNUM_TR2(__TBIThreadId() >> TBID_THREAD_S),
              IMG_NULL,
              0);

	/* Start Meos Main Timer Task...*/
	g_sTESTContext.psTimerTask = KRN_startTimerTask("Timer Task", g_sTESTContext.aui32TimerStack, TIM_STACK_SIZE, TIM_TICK_PERIOD);

    /* Start the main task...*/
    KRN_startTask(MainTask, &g_sTESTContext.sMainTask, g_sTESTContext.aui32TaskStack, STACK_SIZE, KRN_LOWEST_PRIORITY+1, IMG_NULL, "MainTask");

	return;
}

/*!
******************************************************************************

 @Function				Usage

******************************************************************************/
static void Usage(char *cmd)
{
    __TBILogF("\nUsage:  %s <..options..>\n", cmd);
	__TBILogF("          -addr N             Address to read from/write to\n");
    __TBILogF("          -size N             Number of bytes to program (if not specified, entire file will be written)\n");
    __TBILogF("          -dev N              Application defined device to use\n");
    __TBILogF("          -br N               Bit clock rate value\n");
	__TBILogF("          -loadbin <filename> Loads a binary file into Bulk SRAM and writes it to flash\n");
	__TBILogF("          -savebin <filename> Saves data from flash to a binary file\n");
	__TBILogF("          -verify N           If -loadbin is specified, set to 1 to verify data written to device.\n");
	__TBILogF("          -erase N            Set to 1 to initially erase all the blocks covering <size> bytes starting at address <addr>\n");
	__TBILogF("          -spiblock N         SPI Master block index\n");

    __TBILogF("Remember to set file server root directory so Codescape looks for files in the correct folder!\n");
    exit(-1);
}

/*!
******************************************************************************

 @Function				ParseCommandLine

******************************************************************************/
static void ParseCommandLine(int argc, char *argv[])
{
    char	*	cmd;
    char	*	option;
    char		tempString[40], *fileName;
	int			temp;

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
	g_sTESTDescription.fBin = (-1);
	g_sTESTDescription.ui32Size = 0;

    verboseLog("Parsing command line - %d arguments\n", argc);

    /* Parse command line */
    while ( argc > 1 )
    {
        if ( *argv[0] == '-' )
        {
            option = (*argv);
			if ( strncmp( option, "-addr", 5 ) == 0 )
			{
				// Configure address
				argv++;
				argc--;
				sscanf( *argv, "%d", &g_sTESTDescription.ui32Address );
				verboseLog("Address is 0x%x\n", g_sTESTDescription.ui32Address );
			}
			else if ( strncmp( option, "-dev", 4 ) == 0 )
            {
            	/* Configure test */
                argv++;
                argc--;
                sscanf( *argv, "%d", &( g_sTESTDescription.ui32Device ) );
				if ( g_sTESTDescription.ui32Device >= NUM_NVRAM_DEVICES )
				{
					verboseLog("Error: incorrect device number. Should be 0 or 1\n");
					Usage(cmd);
				}

                verboseLog("Device number %d\n", g_sTESTDescription.ui32Device );
            }
			else if ( strncmp( option, "-size", 5 ) == 0 )
            {
            	/* Configure job size */
                argv++;
                argc--;
                sscanf( *argv, "%d", &( g_sTESTDescription.ui32Size ) );
                verboseLog("Each job contains %d bytes\n", g_sTESTDescription.ui32Size);
            }
			else if ( strncmp( option, "-br", 3 ) == 0 )
			{
				// Configure bit rate
				argv++;
				argc--;
				sscanf( *argv, "%d", &(g_sTESTDescription.ui32BitRate ) );
				if ( g_sTESTDescription.ui32BitRate > 0xff )
				{
					verboseLog("Bitrate valid range 0 to 255\n");
					Usage(cmd);
				}
			}
			else if ( strncmp( option, "-loadbin", 8 ) == 0 )
			{
				// Open the binary file
				fileName = tempString;
				argv++;
				argc--;
				sscanf( *argv, "%s", fileName );
				g_sTESTDescription.bLoadBin = IMG_TRUE;
				g_sTESTDescription.fBin = open( fileName, O_RDONLY, 0744 );
				if ( g_sTESTDescription.fBin == -1 )
				{
					__TBILogF("Error opening binary file %s\n", fileName );
					Usage(cmd);
				}
				verboseLog("Binary file name %s\n", fileName );
			}
			else if ( strncmp( option, "-savebin", 8 ) == 0 )
			{
				// Open the binary file
				fileName = tempString;
				argv++;
				argc--;
				sscanf( *argv, "%s", fileName );
				g_sTESTDescription.bSaveBin = IMG_TRUE;
				g_sTESTDescription.fBin = open( fileName, O_WRONLY | O_CREAT, 0744 );
				if ( g_sTESTDescription.fBin == -1 )
				{
					__TBILogF("Error opening binary file %s\n", fileName );
					Usage(cmd);
				}
				verboseLog("Binary file name %s\n", fileName );
			}
			else if ( strncmp( option, "-verify", 7 ) == 0 )
			{
				argv++;
				argc--;
				sscanf( *argv, "%d", &temp );
				if ( temp > 1 )
				{
					verboseLog("Error: -verify must be 1 or 0.\n");
					Usage(cmd);
				}
				g_sTESTDescription.bVerify = temp;
				verboseLog("Verify is %d\n", g_sTESTDescription.bVerify );
			}
			else if ( strncmp( option, "-erase", 6 ) == 0 )
			{
				argv++;
				argc--;
				sscanf( *argv, "%d", &temp );
				if ( temp > 1 )
				{
					verboseLog("Error: -erase must be 1 or 0.\n");
					Usage(cmd);
				}
				g_sTESTDescription.bErase = temp;
				verboseLog("Erase is %d\n", g_sTESTDescription.bErase );
			}
			else if ( strncmp( option, "-spiblock", 9 ) == 0 )
			{
				argv++;
				argc--;
				sscanf( *argv, "%d", &g_sTESTDescription.ui32BlockIndex );
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

 @Function				main

******************************************************************************/
#if defined (__MTX_MEOS__) || defined (__META_MEOS__)

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
