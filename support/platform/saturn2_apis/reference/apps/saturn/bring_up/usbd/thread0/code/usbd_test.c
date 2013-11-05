/*!
******************************************************************************
  file   usbd_test.c

  brief  Test harness for USB Device module

  author Imagination Technologies

         <b>Copyright 2006 by Imagination Technologies Limited.</b>\n
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
#include <sys_config.h>
#include <sys_util.h>
#include <ioblock_defs.h>
#include <usbd_api.h>
#include <usbd_std_descriptors.h>

#define USBD_LIBRARY_RAND_AVAILABLE
#if defined USBD_LIBRARY_RAND_AVAILABLE
#else
  #define USBD_USE_TIMER_FOR_RAND		((IMG_UINT8)(*(unsigned long *)0x04800010))
#endif


/*============================================================================
====	D E F I N E S
=============================================================================*/

/* Set attribute to allocate the data inside bulk memory */
#define DATA_ATTR

#define TEST_BUFFER_SIZE					(1024 * 1024)
#define PASS								(0)
#define FAIL								(-1)
#define USBD_TEST_STATUS_ASYNC_DEFAULT		(0xFF)
#define POLL_WAIT							(5000 * 5) /1000


#define PARAM_RANDOM						(0x01)
#define PARAM_VERSION_1						(0x02)
#define PARAM_VERSION_2						(0x04)
#define PARAM_CRC							(0x08)
#define PARAM_CHANNEL_MASK					(0x0000FF00)
#define PARAM_CHANNEL_SHIFT					(8)

//#define BOOT_TEST

#define TIMER_NAME							"Timer task"
#define STACK_SIZE							(4096)
#define TIMER_TICK							(5000)		//timer tick every 1 second
#define NUM_IO_BLOCKS						(8)

#define NUM_EPS								(5)
#define INT_EP_INDEX						(2) // EP 3/index 2 in this configuration

/* USB strings */
#define MANUFACTURER_STRING "Imagination Technologies"
#define PRODUCT_STRING      "MobileTV"
#define SERIAL_STRING       "0001"

#define X25_INIT				0xFFFFU						// X25 CRC bits'n'pieces
#define X25_RES					0x1D0FU

#define STACK_FILL				0xdeadbeef

/*============================================================================
====	E N U M S
=============================================================================*/

typedef enum
{
	OPER_BULK_IN,
	OPER_BULK_OUT,
	OPER_BULK_LOOPBACK,
	OPER_INT,
	OPER_STREAM
} TEST_eOperation;

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

typedef struct
{
    volatile IMG_UINT8 *buffer;
	volatile IMG_UINT32 num_bytes_transferred;
    volatile IMG_UINT32 status;
    volatile IMG_UINT32 operationId;

} USBD_TEST_ASYNC_CTX_T;


typedef struct usbd_testmsg_t
{
    IMG_UINT32	uiType;
    IMG_UINT32	uiLength;
    IMG_UINT32	uiCount;
    IMG_UINT32	uiParam;
} __attribute__ ((packed)) TEST_sTestDescPacket;

typedef struct usbd_intrmsg_t
{
	IMG_UINT32	uiLength;
	IMG_UINT32	ui32BulkChannel;
} __attribute__ ((packed)) TEST_sInterruptMsg;

typedef struct usbd_ackmsg_t
{
	IMG_UINT8	ui8Data[2];
	IMG_UINT16	ui16CRC;
} __attribute__ ((packed)) TEST_sAckPacket;

typedef struct usbd_testdesc_t
{
	TEST_eOperation		eOperation;
	IMG_UINT32			ui32Length;
	IMG_UINT32			ui32Count;
	IMG_BOOL			bUseRandomData;
	IMG_UINT32			ui32HostVersion;
	IMG_UINT32			ui32Channel;
	IMG_BOOL			bUseCRC;
} TEST_sDescription;


/*============================================================================
====	D A T A
=============================================================================*/


/******************************************************************************
**
**                       MeOS RESOURCES
**
******************************************************************************/

/* Background Task Control Block */
KRN_TASK_T					*	g_psBackgroundTask;

/* Timer Task Control Block */
KRN_TASK_T					*	g_psTimerTask;

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

/* QIO structures */
QIO_DEVENTRY_T					g_aVectoredDevTable[ QIO_NO_VECTORS ];
QIO_SYS_T						g_sQIO;

DQ_T							g_sTaskQueue;				//queue for hibernating tasks

/* I/O available flag */
static volatile IMG_BOOL		bIOAvailable = 0;

USBD_sBlock						g_sUSBDBlock;
static USBD_IO_BLOCK_T			g_aasIOBlocks[NUM_EPS][NUM_IO_BLOCKS];

USBD_sEP					*	g_apsEP[NUM_EPS];

// CRC Table
static const unsigned short X25Tab[16] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF
};

/******************************************************************************
**
**                       TEST HARNESS RESOURCES
**
******************************************************************************/

/* All buffers need to be 32 bit aligned to work with this device */
DATA_ATTR static IMG_ALIGN(4) TEST_sTestDescPacket		g_sTestMsg;
DATA_ATTR static IMG_ALIGN(4) TEST_sInterruptMsg		g_sIntrMsg;
DATA_ATTR static IMG_ALIGN(4) TEST_sAckPacket			g_sAckMsg;

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

//DATA_ATTR static IMG_ALIGN(4) IMG_UINT8					g_aui8ReceiveBuffer[TEST_BUFFER_SIZE];
//DATA_ATTR static IMG_ALIGN(4) IMG_UINT8					g_aui8SendBuffer[TEST_BUFFER_SIZE];
// Stick buffers in DDR so that we can get large transfers.
static IMG_UINT8	*	g_aui8ReceiveBuffer = (IMG_UINT8 *)0xB0000000;
static IMG_UINT8	*	g_aui8SendBuffer	= (IMG_UINT8 *)(0xB0000000 + TEST_BUFFER_SIZE);

USB_STRING_DESCRIPTORS_T	sDeviceStrings =
{
	(USBD_STRING_DESCRIPTOR_T*) &LanguageStringDescriptor,
	(USBD_STRING_DESCRIPTOR_T*) &ManufacturerStringDescriptor,
    (USBD_STRING_DESCRIPTOR_T*) &ProductStringDescriptor,
    (USBD_STRING_DESCRIPTOR_T*) &SerialStringDescriptor
};

IMG_HANDLE	ahMyCallback[2] = {IMG_NULL, IMG_NULL};

/******************************************************************************
**
**                       LOCAL FUNCTION PROTOTYPES
**
******************************************************************************/

static IMG_INT32	UsbAsyncTransfer( IMG_UINT32	ui32Channel, IMG_UINT8	*	pcBuffer, IMG_UINT32	uiLength, IMG_UINT8 bWrite );

/*!
******************************************************************************

 @Function				MainTask

******************************************************************************/

/*!
******************************************************************************

 @Function				CRCX25

******************************************************************************/
unsigned short CRCX25(	unsigned char	*	pui8Data,
						unsigned long		ui32Size	)
{
	unsigned short ui16CRCReg = X25_INIT;
	unsigned short ui16Temp;
	unsigned short ui8Data = (unsigned char)0;

	/* Use 4 bits out of the data/polynomial at a time */
	do
	{
		/* Get data byte */
		ui8Data = (unsigned short)*pui8Data++;

		/* Use 4 bits out of the data/polynomial at a time */
		ui16Temp = (unsigned short)(ui16CRCReg >> 12u);
		ui16Temp ^= (unsigned short)(ui8Data >> 4u);				// xor data (MS 4 bits) with the MS 4 bits
		ui16Temp &= 0xf;										// of the CRC reg to be used as index in array
		ui16CRCReg = (unsigned short)( ( ui16CRCReg << 4u ) ^ X25Tab[ ui16Temp ] );

		/* Now do second half of byte */
		ui16Temp = (unsigned short)(ui16CRCReg >> 12u);
		ui16Temp ^= ui8Data;									// xor data with the 4 MS bits of the CRC reg
		ui16Temp &= 0xf;										// to be used as index in array
		ui16CRCReg = (unsigned short)( ( ui16CRCReg << 4u ) ^ X25Tab[ ui16Temp ] );

	} while ( (--ui32Size) != 0 );

	return ( (unsigned short)( ui16CRCReg ^ X25_RES ) );
}

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
 @Function              TEST_GenerateRandomData

 @Description			Fills a buffer with random data

******************************************************************************/
void TEST_GenerateRandomData( IMG_UINT8	*	pui8Buffer,	IMG_UINT32	ui32Size, IMG_BOOL bUseCRC, unsigned short	*	pui16CRC )
{
	IMG_UINT32		i;

	for ( i = 0; i < ui32Size; ++i )
	{
#if defined USBD_LIBRARY_RAND_AVAILABLE
		pui8Buffer[i] = rand();
#else
		pui8Buffer[i] = USBD_USE_TIMER_FOR_RAND;
#endif
	}

	// Calculate a CRC for the data
	IMG_ASSERT( pui16CRC );

	if ( ui32Size && bUseCRC )
	{
		*pui16CRC = CRCX25( pui8Buffer, ui32Size );
	}
}

/*!
******************************************************************************

 @Function              TEST_SendAck

******************************************************************************/
int TEST_SendAck(	IMG_UINT32	ui32Channel, const unsigned short	*	pui16CRC	)
{
#if defined USBD_LIBRARY_RAND_AVAILABLE
	g_sAckMsg.ui8Data[0] = rand();
#else
	g_sAckMsg.ui8Data[0] = USBD_USE_TIMER_FOR_RAND;
#endif
	g_sAckMsg.ui8Data[1] = ~g_sAckMsg.ui8Data[0];

	if ( pui16CRC )
	{
		g_sAckMsg.ui16CRC = *pui16CRC;
	}

	if ( UsbAsyncTransfer( ui32Channel, (IMG_UINT8 *)&g_sAckMsg, sizeof( g_sAckMsg ), IMG_TRUE ) )
	{
		return 0;
	}

	return 1;
}

/*!
******************************************************************************

 @Function				TEST_GetTestMessage

******************************************************************************/
int TEST_GetTestMessage(	TEST_sDescription	*psDesc,	unsigned short	*	pui16CRC	)
{
	// Always get test message on channel 0!
	if ( UsbAsyncTransfer( 0, (IMG_UINT8 *)&g_sTestMsg, sizeof( TEST_sTestDescPacket ), IMG_FALSE ) )
	{
		IMG_ASSERT(0);
		return 0;
	}

	psDesc->eOperation		= (TEST_eOperation)g_sTestMsg.uiType;
	psDesc->ui32Length		= g_sTestMsg.uiLength;
	psDesc->ui32Count		= g_sTestMsg.uiCount;
	psDesc->bUseRandomData	= g_sTestMsg.uiParam & PARAM_RANDOM;
	psDesc->ui32Channel		= (g_sTestMsg.uiParam & PARAM_CHANNEL_MASK) >> PARAM_CHANNEL_SHIFT;
	psDesc->bUseCRC			= g_sTestMsg.uiParam & PARAM_CRC;

	if ( (g_sTestMsg.uiParam & 0xFFFF) & PARAM_VERSION_1 )
	{
		psDesc->ui32HostVersion = 1;
	}
	else if ( (g_sTestMsg.uiParam & 0xFFFF) & PARAM_VERSION_2 )
	{
		psDesc->ui32HostVersion = 2;
	}

	IMG_ASSERT( pui16CRC );
	if ( psDesc->bUseCRC )
	{
		*pui16CRC				= (g_sTestMsg.uiParam & 0xFFFF0000) >> 16;
	}

	return 1;
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
 @Function              completeFunc

 @Description		Transfer completion callback function registered with
					the USBD driver.
******************************************************************************/
static IMG_VOID completeFunc
(
    IMG_VOID          	*context,
   	IMG_UINT8 	*buffer,
    IMG_UINT32	num_bytes_transferred,
	IMG_UINT32 	status
)
{
	USBD_TEST_ASYNC_CTX_T *asyncCtx = (USBD_TEST_ASYNC_CTX_T*)context;
	asyncCtx->buffer                = buffer;
	asyncCtx->num_bytes_transferred = num_bytes_transferred;
	asyncCtx->status                = status;
    return;
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



/*!
******************************************************************************
 @Function              UsbAsyncTransfer

 @Description			Performs an asynchronous USBD transfer

******************************************************************************/
static IMG_INT32 UsbAsyncTransfer(IMG_UINT32 ui32Channel, IMG_UINT8 * pcBuffer, IMG_UINT32 uiLength, IMG_UINT8 bWrite)
{
	IMG_UINT32						uiNumBytesTransferred = 0;
	IMG_INT32						iTimeout = USBD_INF_TIMEOUT;
	USBD_ASYNC_T					sAsync;
	USBD_TEST_ASYNC_CTX_T			sTestAsyncCtxt;
	IMG_UINT32						ui32EPIndex;


	sAsync.iUsingCallback = IMG_TRUE;
	sAsync.callback_context = &sTestAsyncCtxt;
	sAsync.callback_routine = &completeFunc;

	sTestAsyncCtxt.status                 = USBD_TEST_STATUS_ASYNC_DEFAULT;
	sTestAsyncCtxt.num_bytes_transferred  = 0;

	// Convert from channel to index into the endpoint array
	// 0 - EP 1 BULK IN
	// 1 - EP 2 BULK OUT
	// 2 - EP 3 INT IN
	// 3 - EP 4 BULK IN
	// 4 - EP 5 BULK OUT
	if ( ui32Channel == 0 )
	{
		ui32EPIndex = 0 + ((bWrite == 1) ? 0 : 1);
	}
	else if ( ui32Channel == 1 )
	{
		ui32EPIndex = 3 + ((bWrite == 1) ? 0 : 1);
	}
	else
	{
		IMG_ASSERT( 0 );
	}

//	IMG_ASSERT(uiLength <= TEST_BUFFER_SIZE);

	if (bWrite)
	{
		if (USBD_STATUS_SUCCESS != USBDWrite(&g_sUSBDBlock, g_apsEP[ ui32EPIndex ], pcBuffer, uiLength, &uiNumBytesTransferred, &sAsync, iTimeout))
		{
			__TBILogF("Write failed\n");
			return -1;
		}
	}
	else
	{
		if (USBD_STATUS_SUCCESS != USBDRead(&g_sUSBDBlock, g_apsEP[ ui32EPIndex ], pcBuffer, uiLength, &uiNumBytesTransferred, &sAsync, iTimeout))
		{
			__TBILogF("Read failed\n");
			return -1;
		}
	}

	/* Wait until the transfer has completed */
	while (sTestAsyncCtxt.status == USBD_TEST_STATUS_ASYNC_DEFAULT)
	{
//		KRN_hibernate(&taskQueue, 1);  //removing this under windows significanly improves performance
	}

	if (sTestAsyncCtxt.status != USBD_STATUS_SUCCESS)
	{
		__TBILogF("FAIL: callback had incorrect status! (status = %d)\n", sTestAsyncCtxt.status);
		return -1;
	}

	if (sTestAsyncCtxt.num_bytes_transferred != uiLength)
	{
		__TBILogF("Incomplete transfer - Transferred %d bytes. \n", sTestAsyncCtxt.num_bytes_transferred);
		return -1;
	}

	return 0;
}


/*!
******************************************************************************
 @Function              USBTestLoopback

 @Description			USB loopback test

******************************************************************************/
static IMG_INT32 USBTestLoopback(IMG_UINT32 ui32Channel, IMG_INT32 uiLength, IMG_INT32 uiNumber)
{
	IMG_INT32 uiCount = 0;

	while (uiCount < uiNumber)
	{
		/* Read then write the same data */
		if( UsbAsyncTransfer( ui32Channel, g_aui8ReceiveBuffer, uiLength, IMG_FALSE ) != 0 )
		{
			return -1;
		}

		if( UsbAsyncTransfer( ui32Channel, g_aui8ReceiveBuffer, uiLength, IMG_TRUE ) != 0 )
		{
			return -1;
		}

		uiCount++;
	}

	return 0;
}

/*!
******************************************************************************
 @Function              USBTestInterrupt

 @Description			USB Interrupt endpoint test

******************************************************************************/
static IMG_INT32 USBTestInterrupt( IMG_UINT32 ui32Length )
{
	IMG_UINT32		ui32BytesWritten;

	IMG_ASSERT( ui32Length > 0 );

	if ( USBDWrite( &g_sUSBDBlock, g_apsEP[INT_EP_INDEX], (IMG_UINT8 *)g_aui8SendBuffer, ui32Length, &ui32BytesWritten, IMG_NULL, USBD_INF_TIMEOUT ) )
	{
		return -1;
	}

	if ( ui32Length != ui32BytesWritten )
	{
		return -1;
	}

	return 0;
}

/*!
******************************************************************************
 @Function              USBTestStream

 @Description			USB Stream test

******************************************************************************/
static IMG_INT32 USBTestStream( IMG_UINT32	ui32Channel, IMG_INT32 uiLength )
{
	IMG_UINT32		ui32BytesSent = 0, ui32BytesToWrite = 4;
	IMG_UINT8 *		pui8DataBuffer = &g_aui8SendBuffer[0];

	if ( uiLength == 0 )
	{
		IMG_ASSERT(0);
		return -1;
	}

	while ( ui32BytesSent < uiLength )
	{
		IMG_UINT32	uiBytesWritten;

		/* Signal the length of data we are about to send */
		g_sIntrMsg.uiLength = ui32BytesToWrite;
		g_sIntrMsg.ui32BulkChannel = ui32Channel;
		if (USBDWrite(&g_sUSBDBlock, g_apsEP[ INT_EP_INDEX ], (IMG_UINT8*)&g_sIntrMsg, sizeof( TEST_sInterruptMsg ), &uiBytesWritten, IMG_NULL, USBD_INF_TIMEOUT) != USBD_STATUS_SUCCESS)
		{
			return -1;
		}
		IMG_ASSERT(uiBytesWritten == sizeof( TEST_sInterruptMsg ));

		/* Send the data */
		if(UsbAsyncTransfer( ui32Channel, pui8DataBuffer, ui32BytesToWrite, IMG_TRUE) != 0)
		{
			return -1;
		}

		/* Update the bytes sent variable */
		ui32BytesSent += ui32BytesToWrite;

		/* Update the data buffer address */
		pui8DataBuffer += ui32BytesToWrite;

		/* Write 4 bytes more every time */
		ui32BytesToWrite+= 4;

		/* See if we need to cap the (last) transfer */
		if (ui32BytesToWrite > (uiLength - ui32BytesSent))
		{
			ui32BytesToWrite = uiLength - ui32BytesSent;
		}
	}

	return 0;
}

#define TEST_TASK_STACK_SIZE	(4096)
KRN_TASK_T	g_sTestTask;
img_uint32	g_aui32TestTaskStack[ TEST_TASK_STACK_SIZE ];

static img_void TestTask()
{
	img_uint32				ui32NumBytesTransferred;

	while ( 1 )
	{
		while ( !bIOAvailable )
		{
		}
		if (USBD_STATUS_SUCCESS != USBDRead( &g_sUSBDBlock, g_apsEP[1], g_aui8ReceiveBuffer, 1024, &ui32NumBytesTransferred, IMG_NULL, USBD_INF_TIMEOUT))
		{
			__TBILogF("Read failed\n");
			break;
		}
	}

	KRN_removeTask(IMG_NULL);
}
/*!
******************************************************************************
 @Function              USBTest

 @Description			Test harness

******************************************************************************/
static void MainTask()
{
	USBD_INIT_PARAM_T			initParam;
	TEST_sDescription			sDesc;
	IMG_UINT16					ui16TxCRC, ui16RxCRC;
	IMG_UINT32					ui32Reg;
	SYS_sConfig					sConfig;
	SYS_sUSBConfig				sUSBConfig;
	IMG_UINT32					ui32EP;

	// Initialise random number generator
#if defined USBD_LIBRARY_RAND_AVAILABLE
	srand( time( NULL ) );
#endif

	// Setup the system
	IMG_MEMSET( &sConfig, 0, sizeof( SYS_sConfig ) );

	sConfig.bSetupSystemClock				= IMG_FALSE;
	SYS_Configure( &sConfig );

	IMG_MEMSET( &sUSBConfig, 0, sizeof( SYS_sUSBConfig ) );
	// Enable USB

	sUSBConfig.bConfigure 	= IMG_TRUE;
	sUSBConfig.bEnable 		= IMG_TRUE;
	sUSBConfig.bOverrideClockSource = IMG_TRUE;
	sUSBConfig.eClockSource = CLOCK_SOURCE_XTALUSB;		// We're using USB XTAL3
	sUSBConfig.ePHYClock = PHY_e12;						// We're running a 12MHz xtal on XTAL5

	SYS_ConfigureUSB( &sUSBConfig );

	/* Initialise the send buffer */
	fillBuffer( g_aui8SendBuffer, sizeof( g_aui8SendBuffer ));

	/* Initialise strings */
	initString(&ManufacturerStringDescriptor, MANUFACTURER_STRING);
    initString(&ProductStringDescriptor, PRODUCT_STRING);
    initString(&SerialStringDescriptor, SERIAL_STRING);

	/* Set the initialisation parameters */
	initParam.ui32BlockIndex	= 0;

#if defined (BOOT_TEST)
	initParam.psStrings					= IMG_NULL;
	initParam.sUsbdPower.bSelfPowered	= IMG_FALSE;
	initParam.iSoftReconnect			= IMG_FALSE;
#else
	initParam.ppsStrings				= (USBD_STRING_DESCRIPTOR_T **)&sDeviceStrings;
	initParam.sUsbdPower.bSelfPowered	= IMG_TRUE;
	initParam.iSoftReconnect			= IMG_FALSE;
#endif

	initParam.sUsbdPower.ui32MaxPower	= 80;
	initParam.iForceDeviceMode			= 0;
	initParam.iEnableZLP				= IMG_TRUE;

#if defined (USBD_NO_CBMAN_SUPPORT)
		 /*! Callback function pointer. Set to IMG_NULL if no callback function is to be used */
    initParam.pfCallbackFunc = &TestHarness_Callback;

    /*! Bitfield of the events to be registered with the API for callback through pfCallbackFunc */
    initParam.uiCallbackEvents = USBD_EVENT_IO_OPENED | USBD_EVENT_IO_CLOSED;
#endif

	initParam.psDeviceDescriptor	= &IMG_sStdDeviceDescriptor;
	initParam.psQualifierDescriptor = &IMG_sStdQualifierDescriptor;
	initParam.psConfigDescriptors	= &IMG_sStdConfigurations;
	initParam.EnumerateSpeed = USB_SPEED_FULL;

	/* Read the vid, pid and bcddevice parameters from Saturn top-level registers */
	ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_USB_IDS );

	USB_CPU_TO_LE16( IMG_sStdDeviceDescriptor.idVendor, READ_REG_FIELD( ui32Reg, CR_PERIP_USB_VENDOR_ID ) );
	//IMG_sStdDeviceDescriptor.idVendor[0] = 0x9A;
	//IMG_sStdDeviceDescriptor.idVendor[1] = 0x14;
	USB_CPU_TO_LE16( IMG_sStdDeviceDescriptor.idProduct, 0x2 ); // Override the product ID as this is not for boot

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

	// Initialise the endpoints
	for ( ui32EP = 1; ui32EP <= NUM_EPS; ++ui32EP )
	{
		if ( USBDInitEP( &g_sUSBDBlock, ui32EP, g_aasIOBlocks[ ui32EP - 1 ], NUM_IO_BLOCKS, &g_apsEP[ ui32EP - 1 ] ) != USBD_STATUS_SUCCESS )
		{
			IMG_ASSERT(0);
			return;
		}
	}

#if !defined (USBD_NO_CBMAN_SUPPORT)
	USBD_AddEventCallback (	&g_sUSBDBlock,
							USBD_EVENT_IO_OPENED,
	 						TestHarness_Callback,
	 						IMG_NULL,
	 						&(ahMyCallback[0]) );

	USBD_AddEventCallback (	&g_sUSBDBlock,
							USBD_EVENT_IO_CLOSED,
	 						TestHarness_Callback,
	 						IMG_NULL,
	 						&(ahMyCallback[1]) );
#endif

#if 0 // Test with writes and reads in different threads
	{
		img_uint32		ui32NumBytesTransferred;
		DQ_T			sTaskQueue;

		DQ_init( &sTaskQueue );

		// CUSTOM TEST
		KRN_startTask( (KRN_TASKFUNC_T *)TestTask,
						&g_sTestTask,
						g_aui32TestTaskStack,
						TEST_TASK_STACK_SIZE,
						4,
						IMG_NULL,
						"Test Task" );

		// Switch to the new task?
		KRN_release();

		// Perform a write
		while ( 1 )
		{
			while ( !bIOAvailable )
			{
			}

			if (USBD_STATUS_SUCCESS != USBDWrite( &g_sUSBDBlock, g_apsEP[0], g_aui8ReceiveBuffer, 1024, &ui32NumBytesTransferred, IMG_NULL, USBD_INF_TIMEOUT))
			{
				__TBILogF("Write failed\n");
				KRN_removeTask(IMG_NULL);
			}
		}
	}
#endif
	while (1)
	{
		/* Wait until the I/O becomes available */
		while (!bIOAvailable)
		{
		}

		/* Read the test information message */
		if ( !TEST_GetTestMessage( &sDesc, &ui16RxCRC ) )
		{
			IMG_ASSERT(0);
			break;
		}

		// Check that we have enough allocated memory for this test
		IMG_ASSERT( sDesc.ui32Length <= TEST_BUFFER_SIZE );

		if ( sDesc.ui32HostVersion == 2 )
		{
			if ( ( sDesc.eOperation == OPER_BULK_IN ) ||
				 ( sDesc.eOperation == OPER_INT ) ||
			     ( sDesc.eOperation == OPER_STREAM ) )
			{
				// Generate data for device->host data
				TEST_GenerateRandomData( g_aui8SendBuffer, sDesc.ui32Length, sDesc.bUseCRC, &ui16TxCRC );

				// Send ACK with CRC
				if ( !TEST_SendAck( sDesc.ui32Channel, &ui16TxCRC ) )
				{
					break;
				}
			}
			else
			{
				// Send ACK without CRC
				if ( !TEST_SendAck( sDesc.ui32Channel, NULL ) )
				{
					break;
				}
			}
		}

		if ( sDesc.ui32HostVersion == 2 )
		{
			sDesc.ui32Count = 1;
		}

		switch ( sDesc.eOperation )
		{
			case OPER_BULK_IN:
			{
				IMG_INT32 uiCount = 0;
				while ( uiCount < sDesc.ui32Count )
				{
					/* Write to host */
					if( UsbAsyncTransfer( sDesc.ui32Channel, g_aui8SendBuffer, sDesc.ui32Length, IMG_TRUE ) != 0 )
					{
						break;
					}
					uiCount++;
				}
				break;
			}
			case OPER_BULK_OUT:
			{
				IMG_INT32 uiCount = 0;
				while (uiCount < sDesc.ui32Count)
				{
					/* Read from host*/
					if( UsbAsyncTransfer( sDesc.ui32Channel, g_aui8ReceiveBuffer, sDesc.ui32Length, IMG_FALSE ) != 0)
					{
						break;
					}
					uiCount++;
				}
				break;
			}
			case OPER_BULK_LOOPBACK:
			{
				USBTestLoopback( sDesc.ui32Channel, sDesc.ui32Length, sDesc.ui32Count );
				break;
			}
			case OPER_INT:
			{
				IMG_INT32 iCount = 0;
				while ( iCount < sDesc.ui32Count )
				{
					if ( USBTestInterrupt( sDesc.ui32Length ) )
					{
						break;
					}
					iCount++;
				}
				break;
			}
			case OPER_STREAM:
			{
				IMG_INT32 uiCount = 0;

				while ( uiCount < sDesc.ui32Count)
				{
					if ( USBTestStream( sDesc.ui32Channel, sDesc.ui32Length ) )
					{
						break;
					}
					uiCount++;
				}
				break;
			}
			default:
			{
				/* No support for other test types */
				IMG_ASSERT(0);
			}
		}


		// Check the data if necessary
		if ( ( sDesc.ui32HostVersion == 2 ) &&
			 ( ( sDesc.eOperation == OPER_BULK_OUT ) ||
			   ( sDesc.eOperation == OPER_BULK_LOOPBACK ) ) &&
			 ( sDesc.ui32Length ) &&
			 ( sDesc.bUseCRC ) )
		{
			if ( CRCX25( g_aui8ReceiveBuffer, sDesc.ui32Length ) != ui16RxCRC )
			{
				IMG_ASSERT(0);
				break;
			}
		}
	}

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

	/* Reset QIO */
    QIO_reset(&g_sQIO,
              g_aVectoredDevTable,
              QIO_NO_VECTORS,
              &QIO_MTPIVDesc,
			  TBID_SIGNUM_TR2(__TBIThreadId() >> TBID_THREAD_S),
              IMG_NULL,
			  0);

	/* Start Meos Main Task */
	g_psBackgroundTask = KRN_startOS("Background Task");

	
	                                           
	/* Start Meos Main Timer Task... */
	g_psTimerTask = KRN_startTimerTask("Timer Task", g_aui32TimerStack, TIM_STACK_SIZE, TIM_TICK_PERIOD );

	DQ_init( &g_sTaskQueue );

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
