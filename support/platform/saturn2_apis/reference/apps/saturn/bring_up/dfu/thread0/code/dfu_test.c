/*!
******************************************************************************
  file   dfu_test.c

  brief  Test harness for USB Device Firmware Upgrade

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

 <b>Platform:</b>\n

******************************************************************************/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/

/*============================================================================
====	I N C L U D E S
=============================================================================*/

#define METAG_ALL_VALUES
#define METAC_ALL_VALUES

#include <metag/machine.inc>
#include <metag/metagtbi.h>
#include <MeOS.h>

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <system.h>
#include <img_defs.h>
#include <sys_util.h>
#include <ioblock_defs.h>
#include <sys_config.h>
#include <spim_api.h>
#include <nvram_api.h>

#include <usbd_api.h>
#include <usbd_std_descriptors.h>
#include <usbd_dfu_descriptors.h>
#include <usb_dfu.h>

/*============================================================================
====	D E F I N E S
=============================================================================*/

/* Set attribute to allocate the data inside bulk memory */
#define DATA_ATTR

#define TIMER_NAME							"Timer task"
#define STACK_SIZE							(4096)
#define TIMER_TICK							(5000)		//timer tick every 1 second
#define NUM_IO_BLOCKS						(8)


/* USB strings */
#define MANUFACTURER_STRING		"Imagination Technologies"
#define PRODUCT_STRING			"MobileTV"
#define SERIAL_STRING       	"DFU"

#define STACK_FILL				0xdeadbeef

#define SPIM_BITRATE		(0xFF)

#define CS_SETUP_FLASH		(0x1)
#define CS_HOLD_FLASH		(0x1)
#define CS_DELAY_FLASH		(0x8)

#define CS_SETUP_EEPROM		(0x9)
#define CS_HOLD_EEPROM		(0x9)
#define CS_DELAY_EEPROM		(0x9)

#define CS_IDLE_LEVEL		(1)
#define DATA_IDLE_LEVEL		(0)

#define FLASH_START			(0)
#define FLASH_FW_SIZE		(256 * 1024) // Make it 256k for now
#define FLASH_PAGE			(1056)
#define FLASH_END			(FLASH_START + FLASH_FW_SIZE - 1)


/*============================================================================
====	E N U M S
=============================================================================*/

/*============================================================================
====	T Y P E D E F S
=============================================================================*/

typedef struct manufactuter_string_descriptor_t
{
    USBD_STRING_DESCRIPTOR_DEF(USBD_STRING_LEN(MANUFACTURER_STRING))
} MANUFACTURER_STRING_DESCRIPTOR_T;

typedef struct product_string_descriptor_t
{
    USBD_STRING_DESCRIPTOR_DEF(USBD_STRING_LEN(PRODUCT_STRING))
} PRODUCT_STRING_DESCRIPTOR_T;

typedef struct serial_string_descriptor_t
{
    USBD_STRING_DESCRIPTOR_DEF(USBD_STRING_LEN(SERIAL_STRING))
} SERIAL_STRING_DESCRIPTOR_T;

typedef struct usb_string_descriptors_t
{
	USBD_STRING_DESCRIPTOR_T	*	psLangIDs;
	USBD_STRING_DESCRIPTOR_T	*	psManufacturerString;
	USBD_STRING_DESCRIPTOR_T	*	psProductString;
	USBD_STRING_DESCRIPTOR_T	*	psSerialString;
} USB_STRING_DESCRIPTORS_T;


typedef struct download_info_tag
{
	LST_LINK;
	IMG_UINT32		ui32StartAddr;
	IMG_UINT8	*	pui8Buffer;
	IMG_UINT32		ui32Length;
	IMG_BOOL		bManifestation;
} DOWNLOAD_INFO;


/*============================================================================
====	D A T A
=============================================================================*/


/******************************************************************************
**
**                       MeOS RESOURCES
**
******************************************************************************/

/* MEOS Scheduler */
KRN_SCHEDULE_T					g_sMeOSScheduler;

/* Scheduler queues */
KRN_TASKQ_T						g_asSchedQueues[ MEOS_MAX_PRIORITY_LEVEL + 1 ];

/* Stack for the Timer Task */
IMG_UINT32						g_aui32TimerStack[TIM_STACK_SIZE];

/* Main Task Control Block */
KRN_TASK_T						g_sMainTask;

/* Stack for the Main Task */
IMG_UINT32						g_aui32TaskStack[ STACK_SIZE ];

/* Programmer TCB */
KRN_TASK_T						g_sProgrammerTask;

/* Mailbox for control of programmer task */
KRN_MAILBOX_T					g_sProgrammerMbox;

/* Stack for the Programmer Task */
IMG_UINT32						g_aui32ProgrammerTaskStack[ STACK_SIZE ];

/* QIO structures */
QIO_DEVENTRY_T					g_aVectoredDevTable[ QIO_NO_VECTORS ];
QIO_SYS_T						g_sQIO;

/* I/O available flag */
static volatile IMG_BOOL		bIOAvailable = 0;

USBD_sBlock						g_sUSBDBlock;
DFU_T							g_sDFU;

#define	API_SCRATCH_SIZE		(2048)
DATA_ATTR img_uint8				g_aui8Scratch[ API_SCRATCH_SIZE ];

NVRAM_sDevice					g_sAtmel;
SPIM_sBlock						g_sSPIMaster;

DOWNLOAD_INFO					g_sDownloadInfo;


/******************************************************************************
**
**                       TEST HARNESS RESOURCES
**
******************************************************************************/

/* All buffers need to be 32 bit aligned to work with this device */
DATA_ATTR static IMG_ALIGN(4) USBD_STRING_DESCRIPTOR_T LanguageStringDescriptor =
{
    USBD_STRING_DESCRIPTOR_LENGTH(1),
    USBD_STRING_DESCRIPTOR_IDENTIFIER,
    { 0x0409 }
};

DATA_ATTR static IMG_ALIGN(4) MANUFACTURER_STRING_DESCRIPTOR_T ManufacturerStringDescriptor =
{
    USBD_STRING_DESCRIPTOR_LENGTH(USBD_STRING_LEN(MANUFACTURER_STRING)),
    USBD_STRING_DESCRIPTOR_IDENTIFIER
};

DATA_ATTR static IMG_ALIGN(4) PRODUCT_STRING_DESCRIPTOR_T ProductStringDescriptor =
{
    USBD_STRING_DESCRIPTOR_LENGTH(USBD_STRING_LEN(PRODUCT_STRING)),
    USBD_STRING_DESCRIPTOR_IDENTIFIER
};

DATA_ATTR static IMG_ALIGN(4) SERIAL_STRING_DESCRIPTOR_T SerialStringDescriptor =
{
    USBD_STRING_DESCRIPTOR_LENGTH(USBD_STRING_LEN(SERIAL_STRING)),
    USBD_STRING_DESCRIPTOR_IDENTIFIER
};

DATA_ATTR static IMG_ALIGN(4) img_uint8 g_aui8Buffer[ FLASH_PAGE ];
DATA_ATTR static IMG_ALIGN(4) img_uint8 g_aui8FirstPage[ FLASH_PAGE ];
DATA_ATTR static IMG_ALIGN(4) img_uint8	g_aui8Verify[ FLASH_PAGE ];

USB_STRING_DESCRIPTORS_T	sDeviceStrings =
{
	(USBD_STRING_DESCRIPTOR_T*) &LanguageStringDescriptor,
	(USBD_STRING_DESCRIPTOR_T*) &ManufacturerStringDescriptor,
    (USBD_STRING_DESCRIPTOR_T*) &ProductStringDescriptor,
	(USBD_STRING_DESCRIPTOR_T*) &SerialStringDescriptor
};

IMG_HANDLE	ahCallback[5] = {IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL};

/* Memory to store upload data */
IMG_UINT8		g_aui8UploadFirmware[ FLASH_FW_SIZE ];

/******************************************************************************
**
**                       LOCAL FUNCTION PROTOTYPES
**
******************************************************************************/


/*!
******************************************************************************
 @Function              fillBuffer

 @Description			Fills a buffer with data

******************************************************************************/
void fillBuffer(IMG_UINT8* pui8Buffer, IMG_UINT32 ui32Size)
{
	IMG_UINT32*	pui32Buffer = (IMG_UINT32*) pui8Buffer;
	IMG_UINT32	ui32Index;

	for (ui32Index = 0; ui32Index < (ui32Size/sizeof(IMG_UINT32)); ui32Index++)
	{
		pui32Buffer[ui32Index] = ui32Index;
	}
}

/*!
******************************************************************************
 @Function              initString

 @Description			Convert an ANSI encoded string into UNICODE format.

******************************************************************************/
static IMG_VOID initString(IMG_VOID *pvStringDescriptor, IMG_CHAR *pcAnsiString)
{
    IMG_CHAR                    *p;
    USBD_STRING_DESCRIPTOR_T	*s;
    IMG_UINT8					stringLength;
    IMG_INT32                   end;
    IMG_UINT8					i;

    p = pcAnsiString;
    s = (USBD_STRING_DESCRIPTOR_T *) pvStringDescriptor;
    stringLength = (s->bLength - 2)/2;
    end = 0;
    for (i = 0; i < stringLength; i++)
    {
        IMG_CHAR c;

        if (!end)
        {
            c = p[i];
            if (c != 0)
            {
                s->wString[i] = (IMG_UINT16) p[i];
            }
            else
            {
                end = 1;
            }
        }

        if (end)
        {
            s->wString[i] = 0;
        }
    }
}

/*!
******************************************************************************
 @Function              TestHarness_Callback

 @Description		USBD event callback routine. USBD callbacks are made
					from a high priority ISR task and thus this function
					must be very fast and should not use any semaphores.
******************************************************************************/
img_result TestHarness_Callback (
    img_uint32                  eEvent,
    img_void *                  pCallbackParameter,
    img_uint32                  ui32Param,
    img_void *                  pvParam
)
{
	switch (eEvent)
	{
		case USBD_EVENT_IO_OPENED:
		{
			bIOAvailable = 1;
			break;
		}
		case USBD_EVENT_IO_CLOSED:
		{
			bIOAvailable = 0;
			break;
		}
		default:
		{
			IMG_ASSERT(0);
			break;
		}
	}

	return IMG_SUCCESS;
}

img_void CreateUploadImage()
{
	IMG_UINT32		ui32BytesLeft;
	IMG_UINT32		ui32BytesToRead;
	IMG_UINT8	*	pui8Data;

	ui32BytesLeft   = FLASH_FW_SIZE;
	pui8Data		= g_aui8UploadFirmware;
	while ( ui32BytesLeft )
	{
		if ( ui32BytesLeft > (32 * 1024) )
		{
			ui32BytesToRead = (32 * 1024);
		}
		else
		{
			ui32BytesToRead = ui32BytesLeft;
		}

		if ( NVRAM_SUCCESS != NVRAM_ReadN( &g_sAtmel, FLASH_START + (FLASH_FW_SIZE - ui32BytesLeft), pui8Data, ui32BytesToRead ) )
		{
			// Read failed
			IMG_ASSERT( 0 );
			return;
		}

		ui32BytesLeft 	-= ui32BytesToRead;
		pui8Data 		+= ui32BytesToRead;
	}
}

img_bool ProgramPage( DFU_T	*	psDFU, img_uint32	ui32StartAddr, img_uint8	*	pui8Buffer, img_uint32	ui32Length )
{
	img_uint32		i;
	img_uint8	*	pui8Verify = g_aui8Verify;

	// Write the block
	if ( NVRAM_SUCCESS != NVRAM_WriteN( &g_sAtmel, ui32StartAddr, pui8Buffer, ui32Length ) )
	{
		// Write failed
		DFUBuildError( psDFU, IMG_NULL, DFU_ERR_PROG );
		return IMG_FALSE;
	}

	// Read back the data and verify
	if ( NVRAM_SUCCESS != NVRAM_ReadN( &g_sAtmel, ui32StartAddr, pui8Verify, ui32Length ) )
	{
		// Readback failed
		DFUBuildError( psDFU, IMG_NULL, DFU_ERR_VERIFY );
		return IMG_FALSE;
	}

	// Verify the data
	for ( i = 0; i < ui32Length; ++i )
	{
		if ( *pui8Buffer++ != *pui8Verify++ )
		{
			// Failed
			DFUBuildError( psDFU, IMG_NULL, DFU_ERR_VERIFY );
			return IMG_FALSE;
		}
	}

	return IMG_TRUE;
}

IMG_VOID ProgrammerTask()
{
	DFU_T			*	psDFU;
	DOWNLOAD_INFO	*	psDownloadInfo;

	psDFU = KRN_taskParameter( IMG_NULL );

	while ( 1 )
	{
		// Block on a download DFU request
		psDownloadInfo = KRN_getMbox( &g_sProgrammerMbox, KRN_INFWAIT );

		// To handle downloads being interrupted, erase the first page at the beginning
		// and program it at the very end (at manifestation)
		if ( !psDownloadInfo->bManifestation )
		{
			if ( psDownloadInfo->ui32StartAddr == FLASH_START)
			{
				// Erase the first page
				NVRAM_BlockErase( &g_sAtmel, FLASH_START );
				// Erase first page in upload image to nullify it
				IMG_MEMSET( g_aui8UploadFirmware, 0, FLASH_PAGE );

				// Save the first page for later
				IMG_MEMCPY( g_aui8FirstPage, psDFU->Buffer, psDownloadInfo->ui32Length );
			}
			else
			{
				// Do the page program
				ProgramPage( psDFU, psDownloadInfo->ui32StartAddr, psDownloadInfo->pui8Buffer, psDownloadInfo->ui32Length );
			}
		}
		else
		{
			// Mannifestation

			// Program the last page
			ProgramPage( psDFU, psDownloadInfo->ui32StartAddr, psDownloadInfo->pui8Buffer, psDownloadInfo->ui32Length );

			// Now read everything out for future verifies and uploads
			CreateUploadImage();
		}

		// Tell the DFU class we're finished
		psDFU->BytesLeft = 0;
	}
}

img_void UsbDownload( DFU_T	*	psDFU )
{
	img_uint32	ui32Length = psDFU->Length;
	img_uint32	ui32StartAddr = FLASH_START + psDFU->NumBytesTransferred;
	img_uint32	ui32EndAddr = ui32StartAddr + ui32Length - 1;

	// Check the start and end addresses
	if ( ( ui32StartAddr < FLASH_START ) || ( ui32EndAddr > FLASH_END ) )
	{
		// This block occupies and invalid address range
		DFUBuildError( psDFU, IMG_NULL, DFU_ERR_ADDRESS );
		return;
	}

	// Tell the programmer task to program this page
	// Program the page
	g_sDownloadInfo.bManifestation	= IMG_FALSE;
	g_sDownloadInfo.ui32StartAddr	= ui32StartAddr;
	g_sDownloadInfo.pui8Buffer		= psDFU->Buffer;
	g_sDownloadInfo.ui32Length		= ui32Length;
	KRN_putMbox( &g_sProgrammerMbox, (IMG_VOID *)&g_sDownloadInfo );

	return;
}

img_void UsbManifest( DFU_T	*	psDFU )
{
	// Program the first page we saved

//	ProgramPage( psDFU, FLASH_START, g_aui8FirstPage, FLASH_PAGE );
	// Tell the programmer task to program this page
	// Program the page
	g_sDownloadInfo.bManifestation	= IMG_TRUE;
	g_sDownloadInfo.ui32StartAddr	= FLASH_START;
	g_sDownloadInfo.pui8Buffer		= g_aui8FirstPage;
	g_sDownloadInfo.ui32Length		= FLASH_PAGE;
	KRN_putMbox( &g_sProgrammerMbox, (IMG_VOID *)&g_sDownloadInfo );
}

img_void UsbUpload( DFU_T	*	psDFU )
{
	img_uint32		ui32Length;

	// Calculate the upload block length
	ui32Length = FLASH_FW_SIZE - psDFU->NumBytesTransferred;

	if ( ui32Length > psDFU->Length )
	{
		ui32Length = psDFU->Length;
	}
	else
	{
		psDFU->Length = ui32Length;
	}

	if ( !ui32Length )
	{
		return;
	}

	// Copy pre-read data to buffer
	IMG_MEMCPY( psDFU->Buffer, g_aui8UploadFirmware + psDFU->NumBytesTransferred, ui32Length );
}

img_result DFUCallback(	img_uint32			eEvent,
						img_void		*	pvCallbackParam,
						img_uint32			ui32Param,
						img_void		*	pvParam	)
{
	DFU_T	*	psDFU = (DFU_T *)pvParam;

	if ( eEvent != USBD_EVENT_DFU )
	{
		IMG_ASSERT(0);
		return IMG_SUCCESS;
	}

	switch ( psDFU->Event )
	{
		case DFU_EVENT_DOWNLOAD_BLOCK:
		{
			UsbDownload( psDFU );
			break;
		}
		case DFU_EVENT_UPLOAD_BLOCK:
		{
			UsbUpload( psDFU );
			break;
		}
		case DFU_EVENT_MANIFEST:
		{
			__TBILogF("Manifesting\n");
			UsbManifest( psDFU );
			break;
		}
		case DFU_EVENT_IDLE:
		{
			__TBILogF("Idle\n");
			break;
		}
		case DFU_EVENT_START_DOWNLOAD:
		{
			__TBILogF("Download commencing...\n");
			break;
		}
		case DFU_EVENT_START_UPLOAD:
		{
			__TBILogF("Upload commencing...\n");
			break;
		}
		case DFU_EVENT_ERROR:
		{
			__TBILogF("ERROR\n");
			break;
		}
		default:
		{
			IMG_ASSERT(0);
			break;
		}
	}

	return IMG_SUCCESS;
}

IMG_BOOL SetupSPIM()
{
	SPIM_sInitParam	sInitParam;

	sInitParam.sDev0Param.ui8BitRate = SPIM_BITRATE;
	sInitParam.sDev0Param.ui8CSSetup = CS_SETUP_FLASH;
	sInitParam.sDev0Param.ui8CSHold = CS_HOLD_FLASH;
	sInitParam.sDev0Param.ui8CSDelay = CS_DELAY_FLASH;
	sInitParam.sDev0Param.eSPIMode = 0;
	sInitParam.sDev0Param.ui32CSIdleLevel = CS_IDLE_LEVEL;
	sInitParam.sDev0Param.ui32DataIdleLevel = DATA_IDLE_LEVEL;

	sInitParam.sDev1Param.ui8BitRate = SPIM_BITRATE;
	sInitParam.sDev1Param.ui8CSSetup = CS_SETUP_FLASH;
	sInitParam.sDev1Param.ui8CSHold = CS_HOLD_FLASH;
	sInitParam.sDev1Param.ui8CSDelay = CS_DELAY_FLASH;
	sInitParam.sDev1Param.eSPIMode = 0;
	sInitParam.sDev1Param.ui32CSIdleLevel = CS_IDLE_LEVEL;
	sInitParam.sDev1Param.ui32DataIdleLevel = DATA_IDLE_LEVEL;

	sInitParam.sDev2Param.ui8BitRate = SPIM_BITRATE;
	sInitParam.sDev2Param.ui8CSSetup = CS_SETUP_FLASH;
	sInitParam.sDev2Param.ui8CSHold = CS_HOLD_FLASH;
	sInitParam.sDev2Param.ui8CSDelay = CS_DELAY_FLASH;
	sInitParam.sDev2Param.eSPIMode = 0;
	sInitParam.sDev2Param.ui32CSIdleLevel = CS_IDLE_LEVEL;
	sInitParam.sDev2Param.ui32DataIdleLevel = DATA_IDLE_LEVEL;

	sInitParam.ui32DMAChannel = 0;
	sInitParam.ui32BlockIndex = 0;
	//sInitParam.bBypassQIO = IMG_TRUE;
	sInitParam.bBypassQIO = IMG_FALSE;
	sInitParam.bBypassDMA = IMG_FALSE;

	if ( SPIMInit( &g_sSPIMaster, &sInitParam ) != SPIM_OK )
	{
		return IMG_FALSE;
	}

	return IMG_TRUE;
}

/*!
******************************************************************************

 @Function				MainTask

******************************************************************************/
static void MainTask()
{
	USBD_INIT_PARAM_T			initParam;
	img_uint32					ui32Reg;
	SYS_sConfig					sConfig;
	SYS_sUSBConfig				sUSBConfig;
	SYS_sSPIMConfig				sSPIMConfig[2];
	DQ_T						sTaskQueue;			// queue for hibernating tasks

	// Setup the system
	IMG_MEMSET( &sConfig, 0, sizeof( SYS_sConfig ) );
	sConfig.bSetupSystemClock	= IMG_FALSE;
	SYS_Configure( &sConfig );

	// Enable USB
	IMG_MEMSET( &sUSBConfig, 0, sizeof( SYS_sUSBConfig ) );
	sUSBConfig.bConfigure 			= IMG_TRUE;
	sUSBConfig.bEnable				= IMG_TRUE;
	sUSBConfig.bOverrideClockSource	= IMG_TRUE;
	sUSBConfig.eClockSource			= CLOCK_SOURCE_XTALUSB;		// We're using USB XTAL3
	sUSBConfig.ePHYClock			= PHY_e12;					// We're running a 12MHz xtal on XTAL3
	SYS_ConfigureUSB( &sUSBConfig );

	// Enable SPIM1
	IMG_MEMSET( &sSPIMConfig[0], 0, 2 * sizeof( SYS_sSPIMConfig ) );
	sSPIMConfig[ 0 ].bConfigure			= IMG_TRUE;
	sSPIMConfig[ 0 ].bEnable			= IMG_TRUE;
	sSPIMConfig[ 0 ].bTargetFrequency	= IMG_TRUE;
	sSPIMConfig[ 0 ].ui32TargetFreq_fp	= 0xE00000; // 12.5MHz in 12.20 format
	SYS_ConfigureSPIM( sSPIMConfig );

	// Initialise spim
	if ( !SetupSPIM() )
	{
		__TBILogF("Error initialising SPI Master 1!\n");
		return;
	}

	// Initialise NVRAM
	NVRAM_Initialise( &g_sSPIMaster, g_aui8Scratch, API_SCRATCH_SIZE );

	// ATMEL AT45DB642D 64Mbit Flash
	IMG_MEMSET( &g_sAtmel, 0, sizeof( NVRAM_sDevice ) );
	g_sAtmel.eChipSelect 					= SPIM_DEVICE0;
	g_sAtmel.ui32BlockSize 					= FLASH_PAGE;
	g_sAtmel.ui32NumBlocks 					= 8192; // This isn't exactly correct for this device but shouldn't affect operation
	g_sAtmel.ui8AddressWidth 				= 3;
	g_sAtmel.bOverrideWriteInstr 			= IMG_TRUE;
	g_sAtmel.ui8WriteInstr 					= 0x82;
	g_sAtmel.bUseAPIWriteN 					= IMG_TRUE;
	g_sAtmel.ui8EraseInstr 					= 0x81;
	g_sAtmel.bOverrideStatusRegBehaviour 	= IMG_TRUE;
	g_sAtmel.ui8StatusRegInstr 				= 0xD7;
	g_sAtmel.ui8StatusRegReadyBit 			= 7;
	g_sAtmel.ui8StatusRegReadyBitReadyValue = 0x1;
	g_sAtmel.bOverrideReadInstr 			= IMG_TRUE;
	g_sAtmel.ui8ReadInstr 					= 0xe8;
	g_sAtmel.ui8PaddingBytes 				= 4;
	g_sAtmel.ui8BlockAddressBitBase 		= 11;
	NVRAM_InitDevice( &g_sAtmel );

	// Read image out of flash for upload
	CreateUploadImage();

	/* Initialise strings */
	initString(&ManufacturerStringDescriptor, MANUFACTURER_STRING);
    initString(&ProductStringDescriptor, PRODUCT_STRING);
    initString(&SerialStringDescriptor, SERIAL_STRING);

	/* Set the initialisation parameters */
	initParam.ui32BlockIndex	= 0;

	initParam.ppsStrings				= (USBD_STRING_DESCRIPTOR_T **)&sDeviceStrings;
	initParam.sUsbdPower.bSelfPowered	= IMG_TRUE;
	initParam.iSoftReconnect			= IMG_TRUE;
	initParam.sUsbdPower.ui32MaxPower	= 80;
	initParam.iForceDeviceMode			= 0;

	initParam.psDeviceDescriptor		= &IMG_sStdDeviceDescriptor;
	initParam.psQualifierDescriptor		= &IMG_sStdQualifierDescriptor;
	initParam.psConfigDescriptors		= &IMG_sDFUConfigurations;

	// Override dfu descriptor page size
	USB_CPU_TO_LE16( IMG_sDFUConfigurationDescriptor_FS.sDFUDesc.wTransferSize, FLASH_PAGE );
	USB_CPU_TO_LE16( IMG_sDFUConfigurationDescriptor_HS.sDFUDesc.wTransferSize, FLASH_PAGE );

	/* Read the vid, pid and bcddevice parameters from Saturn top-level registers */
	ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_USB_IDS );
	USB_CPU_TO_LE16( IMG_sStdDeviceDescriptor.idVendor, READ_REG_FIELD( ui32Reg, CR_PERIP_USB_VENDOR_ID ) );
	USB_CPU_TO_LE16( IMG_sStdDeviceDescriptor.idProduct, 0x3 ); // Override the product ID for DFU

	ui32Reg = READ_REG( CR_PERIP_BASE, CR_SATURN_CORE_REV );
	USB_CPU_TO_LE16( IMG_sStdDeviceDescriptor.bcdDevice, ((READ_REG_FIELD( ui32Reg, CR_SATURN_MAJOR_REV ) << 8) |
															READ_REG_FIELD( ui32Reg, CR_SATURN_MINOR_REV )) );

	// Fill in the string indices
	IMG_sStdDeviceDescriptor.iManufacturer	= 0x1;
	IMG_sStdDeviceDescriptor.iProduct		= 0x2;
	IMG_sStdDeviceDescriptor.iSerialNumber	= 0x3;

	if ( USBDInit( &g_sUSBDBlock, &initParam ) != USBD_STATUS_SUCCESS )
	{
		IMG_ASSERT( 0 );
		return;
	}

	// No endpoints to initialise

	USBD_AddEventCallback (	&g_sUSBDBlock,
							USBD_EVENT_IO_OPENED,
	 						TestHarness_Callback,
	 						IMG_NULL,
	 						&(ahCallback[0]) );

	USBD_AddEventCallback (	&g_sUSBDBlock,
							USBD_EVENT_IO_CLOSED,
	 						TestHarness_Callback,
	 						IMG_NULL,
	 						&(ahCallback[1]) );

	DFUInit( &g_sDFU,
			 DFU_DOWNLOAD_CAPABLE | DFU_UPLOAD_CAPABLE | DFU_MANIFESTATION_TOLERANT,	/* Attributes */
			 1000,																		/* Detach Timeout */
			 20,																		/* Prog Timeout */
			 DFU_IDLE,																	/* State */
			 DFU_OK,																	/* Status */
			 g_aui8Buffer,																/* Buffer */
			 FLASH_FW_SIZE																/* Flash Size */
			 );

	// Start the programmer task
	KRN_initMbox( &g_sProgrammerMbox );
	KRN_startTask( ProgrammerTask, &g_sProgrammerTask, g_aui32ProgrammerTaskStack, STACK_SIZE, MEOS_MAX_PRIORITY_LEVEL - 1, (IMG_VOID *)&g_sDFU, "ProgrammerTask" );

	/* We need to know about DFU events */
    USBD_AddEventCallback(	&g_sUSBDBlock,
							USBD_EVENT_DFU,
							DFUCallback,
							&g_sDFU,
							&(ahCallback[2]) );

	DFUAdd( &g_sUSBDBlock, &g_sDFU );

	// Sleep forever as everything is handled in irq context
	DQ_init( &sTaskQueue );
	KRN_hibernate( &sTaskQueue, KRN_INFWAIT );

	// We should never get here
	IMG_ASSERT(0);
	return;
}


/*!
******************************************************************************

 @Function				RunTest

******************************************************************************/
static img_void RunTest( img_void )
{
	/* Initialise the kernel */
    KRN_reset(&(g_sMeOSScheduler),
			  g_asSchedQueues,
			  MEOS_MAX_PRIORITY_LEVEL,
			  STACK_FILL,
			  IMG_NULL,
			  0);

	/* Start Meos Main Task */
	KRN_startOS("Background Task");

	/* Reset QIO */
    QIO_reset(	&g_sQIO,
				g_aVectoredDevTable,
				QIO_NO_VECTORS,
				&QIO_MTPIVDesc,
				TBID_SIGNUM_TR2(__TBIThreadId() >> TBID_THREAD_S),
				IMG_NULL,
				0 );


	/* Start Meos Main Timer Task... */
	KRN_startTimerTask("Timer Task", g_aui32TimerStack, TIM_STACK_SIZE, TIM_TICK_PERIOD );

	KRN_startTask( MainTask, &g_sMainTask, g_aui32TaskStack, STACK_SIZE, KRN_LOWEST_PRIORITY + 1, IMG_NULL, "MainTask" );

	return;
}

/*!
******************************************************************************

 @Function				main

******************************************************************************/
#if defined (__MTX_MEOS__) || defined (__META_MEOS__)

int main(int argc, char *argv[])
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
