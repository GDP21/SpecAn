/*!
******************************************************************************
  file   isoc_test.c

  brief  Test harness for USB Device module and Isochronous endpoints

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

#include <img_defs.h>
#include <ioblock_defs.h>

#include <system.h>
#include <sys_config.h>
#include <sys_util.h>

#include <usbd_api.h>
#include <usbd_std_descriptors.h>
#include <usbd_isoc_descriptors.h>



/*============================================================================
====	D E F I N E S
=============================================================================*/

/* Set attribute to allocate the data inside bulk memory */
#define DATA_ATTR

#define TEST_BUFFER_SIZE					(32 * 1024)
#define PASS								(0)
#define FAIL								(-1)
#define USBD_TEST_STATUS_ASYNC_DEFAULT		(0xFF)
#define POLL_WAIT							(5000 * 5) /1000

#define TIMER_NAME							"Timer task"
#define STACK_SIZE							(4096)
#define TIMER_TICK							(5000)		//timer tick every 1 second
#define NUM_IO_BLOCKS						(8)

/* USB strings */
#define MANUFACTURER_STRING "Imagination Technologies"
#define PRODUCT_STRING      "MobileTV"
#define SERIAL_STRING       "0001"

#define STACK_FILL				0xdeadbeef

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

typedef struct
{
    volatile IMG_UINT8 *buffer;
	volatile IMG_UINT32 num_bytes_transferred;
    volatile IMG_UINT32 status;
    volatile IMG_UINT32 operationId;

} USBD_TEST_ASYNC_CTX_T;


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
static USBD_IO_BLOCK_T			g_asEPInIOBlock[NUM_IO_BLOCKS];
static USBD_IO_BLOCK_T			g_asEPOutIOBlock[NUM_IO_BLOCKS];

USBD_sEP					*	g_psEPIn;
USBD_sEP					*	g_psEPOut;

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

DATA_ATTR static IMG_ALIGN(4) IMG_UINT8				g_aui8Buffer[TEST_BUFFER_SIZE];

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


	sAsync.iUsingCallback = IMG_TRUE;
	sAsync.callback_context = &sTestAsyncCtxt;
	sAsync.callback_routine = &completeFunc;

	sTestAsyncCtxt.status                 = USBD_TEST_STATUS_ASYNC_DEFAULT;
	sTestAsyncCtxt.num_bytes_transferred  = 0;

//	IMG_ASSERT(uiLength <= TEST_BUFFER_SIZE);

	if (bWrite)
	{
		if (USBD_STATUS_SUCCESS != USBDWrite(&g_sUSBDBlock, g_psEPIn, pcBuffer, uiLength, &uiNumBytesTransferred, &sAsync, iTimeout))
		{
			__TBILogF("Write failed\n");
			return -1;
		}
	}
	else
	{
		if (USBD_STATUS_SUCCESS != USBDRead(&g_sUSBDBlock, g_psEPOut, pcBuffer, uiLength, &uiNumBytesTransferred, &sAsync, iTimeout))
		{
			__TBILogF("Read failed\n");
			return -1;
		}
	}

	/* Wait until the transfer has completed */
	while (sTestAsyncCtxt.status == USBD_TEST_STATUS_ASYNC_DEFAULT)
	{
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
 @Function              USBTestIsocLoopback

 @Description			USB loopback test

******************************************************************************/
static IMG_INT32 USBTestIsocLoopback(IMG_UINT32 ui32Channel, IMG_INT32 uiLength, IMG_INT32 uiNumber)
{
	IMG_INT32 uiCount = 0;

	while (uiCount < uiNumber)
	{
		/* Read then write the same data */
		if( UsbAsyncTransfer( ui32Channel, g_aui8Buffer, uiLength, IMG_FALSE ) != 0 )
		{
			return -1;
		}

		if( UsbAsyncTransfer( ui32Channel, g_aui8Buffer, uiLength, IMG_TRUE ) != 0 )
		{
			return -1;
		}

		uiCount++;
	}

	return 0;
}

/*!
******************************************************************************
 @Function              USBTest

 @Description			Test harness

******************************************************************************/
static void MainTask()
{
	USBD_INIT_PARAM_T			initParam;
	IMG_UINT32					ui32Reg;
//	SYS_sConfig					sConfig;
	DQ_T						sQ;
	IMG_BOOL					bFirstPass = IMG_TRUE;

	DQ_init( &sQ );

#if 0
	// Setup the system
	IMG_MEMSET( &sConfig, 0, sizeof( SYS_sConfig ) );

	sConfig.bSetupSystemClock				= IMG_FALSE;
	sConfig.ui32ActualFreq_fp				= SYS_FREQ_500MHZ;


	// Enable USB
	sConfig.asBlockConfig[ BLOCK_USB ].bEnable = IMG_TRUE;
	sConfig.asBlockConfig[ BLOCK_USB ].bOverrideClockSource = IMG_TRUE;
	sConfig.asBlockConfig[ BLOCK_USB ].eClockSource = CLOCK_SOURCE_XTALUSB;			// We're using USB XTAL/XTAL5
	sConfig.asBlockConfig[ BLOCK_USB ].uSpecificConfig.sUSB.ePHYClock = PHY_e12;	// We're running a 12MHz xtal on XTAL5

	// Enable UCC's
	sConfig.asBlockConfig[ BLOCK_UCC0 ].bEnable = IMG_TRUE;
	sConfig.asBlockConfig[ BLOCK_UCC1 ].bEnable = IMG_TRUE;

	SYS_configure( &sConfig );
#endif

	/* Initialise strings */
	initString(&ManufacturerStringDescriptor, MANUFACTURER_STRING);
    initString(&ProductStringDescriptor, PRODUCT_STRING);
    initString(&SerialStringDescriptor, SERIAL_STRING);

	/* Set the initialisation parameters */
	initParam.ui32BlockIndex	= 0;

	initParam.ppsStrings					= (USBD_STRING_DESCRIPTOR_T **)&sDeviceStrings;
	initParam.sUsbdPower.bSelfPowered	= IMG_TRUE;
	initParam.iSoftReconnect			= IMG_TRUE;

	initParam.sUsbdPower.ui32MaxPower	= 80;
	initParam.iForceDeviceMode			= 0;

	initParam.psDeviceDescriptor	= &IMG_sStdDeviceDescriptor;
	initParam.psQualifierDescriptor	= &IMG_sStdQualifierDescriptor;
	initParam.psConfigDescriptors	= &IMG_sIsocConfigurations;

	/* Read the vid, pid and bcddevice parameters from Saturn top-level registers */
	ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_USB_IDS );

	USB_CPU_TO_LE16( IMG_sStdDeviceDescriptor.idVendor, READ_REG_FIELD( ui32Reg, CR_PERIP_USB_VENDOR_ID ) );
	USB_CPU_TO_LE16( IMG_sStdDeviceDescriptor.idProduct, 0x4 ); // Override the product ID as this is not for boot

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
	if ( USBDInitEP( &g_sUSBDBlock, 1, g_asEPInIOBlock, NUM_IO_BLOCKS, &g_psEPIn ) != USBD_STATUS_SUCCESS )
	{
		IMG_ASSERT( 0 );
		return;
	}
	if ( USBDInitEP( &g_sUSBDBlock, 2, g_asEPOutIOBlock, NUM_IO_BLOCKS, &g_psEPOut ) != USBD_STATUS_SUCCESS )
	{
		IMG_ASSERT( 0 );
		return;
	}

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

	while (1)
	{
		/* Wait until the I/O becomes available */
		while (!bIOAvailable)
		{
		}

		if ( bFirstPass )
		{
			// Read in our data the first time
			if ( UsbAsyncTransfer( 0, g_aui8Buffer, 32768, IMG_FALSE ) )
			{
				break;
			}
			bFirstPass = IMG_FALSE;
		}
		else
		{
			// Write this data back to the host continuously
			if ( UsbAsyncTransfer( 0, g_aui8Buffer, 32768, IMG_TRUE ) )
			{
				break;
			}
		}

//		USBTestIsocLoopback( 0, 32768, 1 );
	}

	// Should never get here
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
	g_psBackgroundTask = KRN_startOS("Background Task");

	/* Reset QIO */
    QIO_reset(&g_sQIO,
				g_aVectoredDevTable,
				QIO_NO_VECTORS,
              &QIO_MTPIVDesc,
				TBID_SIGNUM_TR2(__TBIThreadId() >> TBID_THREAD_S),
				IMG_NULL,
			  0);


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
