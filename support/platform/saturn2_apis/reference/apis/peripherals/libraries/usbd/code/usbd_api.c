/*!
*******************************************************************************
  file   usbd_api.c

  brief  USBD API

         This file defines the functions that make up the USB Device API.

  author Imagination Technologies

         <b>Copyright 2007 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to the third parties without the
         express written permission of Imagination Technologies
         Limited, Home Park Estate, Kings Langley, Hertfordshire,
         WD4 8LZ, U.K.

 \n<b>Platform:</b>\n
		MobileTV


*******************************************************************************/

/* ---------------------------- INCLUDE FILES ---------------------------- */

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES

#include <metag/machine.inc>
#include <metag/metagtbi.h>
/* MeOS Library */
#include <MeOS.h>

#include <img_defs.h>
#include <ioblock_defs.h>

#if defined (USBD_BOOT_CODE)
	#warning USBD boot code build!
#endif

/* USBD Driver */
#include <usb_config.h>
#include "usbd_api.h"
#include "usbd_drv.h"
#include "usb_hal.h"

/* -------------------------- MACRO DEFINITIONS -------------------------- */


/* -------------------------- EXPORTED FUNCTIONS ------------------------- */


/* ----------------------- LOCAL FUNCTION PROTOTYPES---------------------- */
static IMG_UINT32		UsbdFinishInfo				(	QIO_STATUS_T		qio_status,
														IMG_UINT32			size,
														IMG_UINT32 *		num_bytes_transferred);

static IMG_INT32		UsbdComplete				(	QIO_DEVICE_T *		dev, 
														QIO_IOCB_T *		iocb,
														QIO_IOPARS_T *		iopars, 
														QIO_STATUS_T		qio_status);

static USBD_RETURN_T	UsbdStartOperation			(	USBD_sEP	 *	psEP,
														IMG_UINT8 *		buf, 
														IMG_UINT32		uiTransferLen, 
														IMG_UINT32 *	uiNumberOfBytesTransferred, 
														USBD_ASYNC_T *	async, 
														IMG_INT32		timeout);

/*!
*******************************************************************************

 @Function              @usbdDefine

*******************************************************************************/
static img_void usbdDefine( ioblock_sBlockDescriptor	*	psBlockDescriptor	)
{
	// Check there is a valid context thats passed in
	IMG_ASSERT( psBlockDescriptor );
	// Check our internal descriptor pointer array is big enough!
	IMG_ASSERT( psBlockDescriptor->ui32Index < MAX_NUM_USBD_BLOCKS );
	// Check that this instance has not already been defined
	IMG_ASSERT( !g_apsUSBDBlock[ psBlockDescriptor->ui32Index ] );

	// Assign the descriptor to our internal list
	g_apsUSBDBlock[ psBlockDescriptor->ui32Index ] = psBlockDescriptor;
}

/*!
*******************************************************************************

 @Function              @USBDInit

*******************************************************************************/
USBD_RETURN_T USBDInit (	  
	USBD_sBlock			*	psBlock, 
	USBD_INIT_PARAM_T	*	initParam	)
{
	img_uint32		i;
	USBD_sEP	*	psEP;

	IMG_ASSERT( psBlock != IMG_NULL );
	IMG_ASSERT( initParam != IMG_NULL );
	IMG_ASSERT( initParam->ui32BlockIndex < MAX_NUM_USBD_BLOCKS );
	IMG_ASSERT((initParam->EnumerateSpeed==USB_SPEED_HIGH)||(initParam->EnumerateSpeed==USB_SPEED_FULL));

	// Check this block has been defined, and if it hasn't, define it ourselves
	if ( !g_apsUSBDBlock[ initParam->ui32BlockIndex ] )
	{
		usbdDefine( &IMG_asUSBBlock[ initParam->ui32BlockIndex ] );
	}

	// Assign the context space
	g_apsUSBDBlock[ initParam->ui32BlockIndex ]->pvAPIContext = (img_void *)&psBlock->aui8InternalMem;

	/* Clear context structure */
	IMG_MEMSET ( psBlock, 0, sizeof(USBD_sBlock) );

	/* Force device mode */
	if ( initParam->iForceDeviceMode )
	{
		USBD_ForceDeviceMode( initParam->ui32BlockIndex );
	}

	/* Perform a soft disconnect if instructed to do so */
	if (initParam->iSoftReconnect)
	{
		USBD_PerformSoftDisconnect( initParam->ui32BlockIndex );
	}

	/* Initialise the endpoint devices */
	/* This happens before the main device as the device.id needs to get overridden by the main device */
	initParam->bMainInit = IMG_FALSE;
	for ( i = 1; i <= USBD_MAX_EPS; ++i )
	{
		USBD_GetEP( initParam->ui32BlockIndex, i, &psEP );
		QIO_init( &(psEP->sDevice), "USB ep", (IMG_UINT32)initParam, &USBD_driver );
	}

    /* Initialise the device */
    /* pass pointer to parameters via the id field */
	initParam->bMainInit = IMG_TRUE;
    QIO_init(&psBlock->sMainDevice, "USB device", (IMG_UINT32)initParam, &USBD_driver);

	/* Enable USB device's interrupts */
	USBD_EnableCoreInterrupts( initParam->ui32BlockIndex, IMG_TRUE );

    /* Enable the device */
    QIO_enable(&psBlock->sMainDevice);

	/* Mark API as initialised */
	USBD_SetInitialised( initParam->ui32BlockIndex, IMG_TRUE );

    return USBD_STATUS_SUCCESS;
}

USBD_RETURN_T USBDInitEP(	USBD_sBlock			*	psBlock,
							IMG_UINT32				ui32EPNum,
							USBD_IO_BLOCK_T		*	asIOBlocks,
							IMG_UINT32				ui32NumIOBlocks,
							USBD_sEP			**	ppsEP	)
{
	// Check the block has been initialised
	IMG_ASSERT( USBD_IsInitialised( psBlock->sMainDevice.id ) );

	// Get the endpoint
	USBD_GetEP(	psBlock->sMainDevice.id, ui32EPNum, ppsEP );

	// Check we got an endpoint back
	IMG_ASSERT( ppsEP );
	
	// Initialise mailbox 
	KRN_initMbox( &(*ppsEP)->sMailbox );

	// Initialise the IOCB pool
	if (asIOBlocks != IMG_NULL)
	{
		KRN_initPool(&(*ppsEP)->sIOBlockPool,
			         asIOBlocks,
				     ui32NumIOBlocks,
					 sizeof(USBD_IO_BLOCK_T));
	}
	
	// Enable the ep driver
    QIO_enable(&((*ppsEP)->sDevice));

	return USBD_STATUS_SUCCESS;
}

/*!
*******************************************************************************

 @Function              @USBDDeinit

*******************************************************************************/
USBD_RETURN_T USBDDeinit (	  
	USBD_sBlock			*	psBlock
)
{
	IMG_ASSERT ( psBlock != IMG_NULL );
	IMG_ASSERT ( USBD_IsInitialised( psBlock->sMainDevice.id ) == IMG_TRUE );
	
	/* Disable device */
	QIO_disable ( &psBlock->sMainDevice );
	
	/* Unload device */
	QIO_unload ( &psBlock->sMainDevice );
	
	/* Disable USB device's interrupts */
	USBD_EnableCoreInterrupts( psBlock->sMainDevice.id, IMG_FALSE );
	
	/* Mark API as uninitialised */
	USBD_SetInitialised( psBlock->sMainDevice.id, IMG_FALSE );
	
	return USBD_STATUS_SUCCESS;	
}

/*!
*******************************************************************************

 @Function              @USBDRead

*******************************************************************************/
USBD_RETURN_T USBDRead	(
	USBD_sBlock		*	psBlock, 
	USBD_sEP		*	psEP,
	IMG_UINT8		*	buf, 
	IMG_UINT32			uiTransferLen, 
	IMG_UINT32		*	uiNumberOfBytesRead, 
	USBD_ASYNC_T	*	async, 
	IMG_INT32			timeout
)
{
	IMG_ASSERT( psBlock != IMG_NULL );
	IMG_ASSERT( USBD_IsInitialised( psBlock->sMainDevice.id ) == IMG_TRUE );	
	IMG_ASSERT( psEP->bInitialised );

	// Check direction of endpoint matches API call
	IMG_ASSERT( !psEP->bIsIn );
	
	return UsbdStartOperation (	psEP, 
								buf, 
								uiTransferLen, 
								uiNumberOfBytesRead, 
								async, 
								timeout );
}


/*!
*******************************************************************************

 @Function              @USBDWrite

******************************************************************************/
USBD_RETURN_T USBDWrite	(	
	USBD_sBlock		*	psBlock, 
	USBD_sEP		*	psEP,
	IMG_UINT8		*	buf, 
    IMG_UINT32			uiTransferLen,
	IMG_UINT32		*	uiNumberOfBytesWritten,
    USBD_ASYNC_T	*	async, 
    IMG_INT32			timeout
)
{
	IMG_ASSERT( psBlock != IMG_NULL );
	IMG_ASSERT( USBD_IsInitialised( psBlock->sMainDevice.id ) == IMG_TRUE );	
	IMG_ASSERT( psEP->bInitialised );

	// Check direction of endpoint matches API call
	IMG_ASSERT( psEP->bIsIn );
	
	return UsbdStartOperation (	psEP, 
								buf, 
								uiTransferLen, 
								uiNumberOfBytesWritten, 
								async, 
								timeout );
}


/*!
*******************************************************************************

 @Function              @USBDGetResult

*******************************************************************************/
IMG_UINT32 USBDGetResult (
	USBD_sBlock *	psBlock, 
	USBD_sEP	*	psEP,
	IMG_VOID	**	context, 
	IMG_UINT32	*	num_bytes_transferred, 
	IMG_INT32		block, 
	IMG_INT32		timeout
)
{
    KRN_MAILBOX_T       *mailbox;
    QIO_IOCB_T          *iocb;
    QIO_DEVICE_T        *dev;
    QIO_STATUS_T        qio_status;
    QIO_IOPARS_T        iopars;
    IMG_UINT32			usbd_status;
    USBD_IO_BLOCK_T    *io_block;

	IMG_ASSERT ( psBlock != IMG_NULL );
	IMG_ASSERT ( USBD_IsInitialised( psBlock->sMainDevice.id ) == IMG_TRUE );

    mailbox = &psEP->sMailbox;

    /* Get the result from the mailbox */
    iocb = QIO_result(mailbox,
                      &dev,
                      &qio_status,
                      &iopars,
                      block ? timeout:0);

    if (iocb == IMG_NULL)
    {
        /* Early Exit */
        return USBD_STATUS_WOULD_BLOCK;
    }

	io_block = (USBD_IO_BLOCK_T *) iopars.spare;
    *context = io_block->callback.context;

    /* Return the IOCB to the pool */
    KRN_returnPool(io_block);

    /* Check completion status */
	usbd_status = UsbdFinishInfo (qio_status, iopars.counter, num_bytes_transferred);

    return usbd_status;
}


/*!
*******************************************************************************

 @Function              @USBDCancel

*******************************************************************************/
IMG_VOID USBDCancel ( 
	USBD_sBlock *	psBlock,
	USBD_sEP	*	psEP
)
{
	IMG_ASSERT ( psBlock != IMG_NULL );
	IMG_ASSERT ( USBD_IsInitialised( psBlock->sMainDevice.id ) == IMG_TRUE );	
	
    QIO_cancelAll(&psEP->sDevice);
}

/****************************************************************************************/
/* Private Functions */
/****************************************************************************************/


/*!
*******************************************************************************

 @Function              UsbdStartOperation

 @Description			Perform a async/sync USBD I/O operation 

*******************************************************************************/
static USBD_RETURN_T UsbdStartOperation	(
	USBD_sEP		*	psEP,
	IMG_UINT8		*	buf, 
	IMG_UINT32			uiTransferLen, 
	IMG_UINT32		*	uiNumberOfBytesTransferred, 
	USBD_ASYNC_T	*	async, 
	IMG_INT32			timeout
)
{
    QIO_DEVICE_T *	device;
	QIO_IOPARS_T    iopars;
    IMG_UINT32		usbd_status;

	iopars.opcode  = (img_uint32)psEP;
	iopars.pointer = (IMG_VOID *)buf;
	iopars.counter = uiTransferLen;

	device = &psEP->sDevice;

    if(!async)
    {
        /*  This is a synchronous I/O */
        QIO_STATUS_T  qio_status;
        
        qio_status   = QIO_qioWait(device, &iopars, timeout);
		usbd_status = UsbdFinishInfo (qio_status, iopars.counter, uiNumberOfBytesTransferred);
    }
    else
    {
        /* This is an Asynchronous I/O */
        USBD_IO_BLOCK_T *	io_block;

        io_block = KRN_takePool(&psEP->sIOBlockPool, 0);

        if (io_block == IMG_NULL)
        {
            /* We have run out of control blocks, so cannot queue this I/O */
            usbd_status = USBD_STATUS_WOULD_BLOCK;
        }
        else
        {
            io_block->callback.routine = async->callback_routine;
            io_block->callback.context = async->callback_context;

            iopars.spare = io_block;

            QIO_qio(device,
                    &io_block->iocb,
                    &iopars, &psEP->sMailbox,
                    async->iUsingCallback ? UsbdComplete:IMG_NULL, 
                    timeout);

            usbd_status = USBD_STATUS_SUCCESS;
        }
    }
    
    return usbd_status;
}

/*!
******************************************************************************

 @Function              UsbdComplete

 @Description			Completion routine.

 						Standard QIO completion routine arguments and return.

******************************************************************************/
static IMG_INT32 UsbdComplete (
    QIO_DEVICE_T *	dev,
    QIO_IOCB_T *	iocb,
    QIO_IOPARS_T *	iopars,
    QIO_STATUS_T    qio_status
)
{
    USBD_IO_BLOCK_T *	io_block;
    IMG_UINT32			usbd_status, num_bytes_transferred;

	(IMG_VOID)dev;
	(IMG_VOID)iocb;

    io_block = (USBD_IO_BLOCK_T *) iopars->spare;

    /* Check status */
    usbd_status = UsbdFinishInfo (qio_status, iopars->counter, &num_bytes_transferred);

    /* Call the callback routine */
    if (io_block->callback.routine != IMG_NULL)
    {
        (*io_block->callback.routine)(io_block->callback.context, iopars->pointer, num_bytes_transferred, usbd_status);
    }

    KRN_returnPool(io_block);

    /* Never deliver to the mailbox */
    return IMG_TRUE;
}


/*!
******************************************************************************

 @Function              UsbdFinishInfo

 @Description			Completion status checking function.

******************************************************************************/
static IMG_UINT32 UsbdFinishInfo (
    QIO_STATUS_T	qio_status,
    IMG_UINT32		size,
    IMG_UINT32 *	num_bytes_transferred
)
{
	IMG_UINT32 usbd_status = USBD_STATUS_TIMEOUT;

	*num_bytes_transferred = 0;

    if      (qio_status == QIO_NORMAL)
	{
		usbd_status = USBD_STATUS_SUCCESS;
		*num_bytes_transferred = size;
	}
    else if (qio_status == QIO_CANCEL) 
	{
		usbd_status = USBD_STATUS_CANCEL;
	}
    else if (qio_status == QIO_TIMEOUT) 
	{
		usbd_status = USBD_STATUS_TIMEOUT;
	}

    return usbd_status;
}

#if !defined (USBD_NO_CBMAN_SUPPORT)
/*!
******************************************************************************

 @Function				USBD_AddEventCallback
 
******************************************************************************/
USBD_RETURN_T USBD_AddEventCallback (
	USBD_sBlock				*	psBlock,
	USBD_EVENT_T				eEvent,
	IMG_pfnEventCallback		pfnEventCallback,
	img_void				*	pCallbackParameter,
	IMG_hCallback			*	phEventCallback
)
{
	/* Check this is the first time we have been initialised */
	IMG_ASSERT( USBD_IsInitialised( psBlock->sMainDevice.id ) );

	/* Check function pointer */
	if (!phEventCallback)
	{
		IMG_ASSERT(IMG_FALSE);
	}

	/* Add callback */
	USBD_AddCallback(	psBlock->sMainDevice.id,
						eEvent,
						pfnEventCallback,
						pCallbackParameter,
						phEventCallback );

	/* Return success */
	return USBD_STATUS_SUCCESS;
}

/*!
*******************************************************************************

 @Function              @USBD_ExecuteEventCallback

******************************************************************************/
USBD_RETURN_T USBD_ExecuteEventCallback(
	USBD_sBlock				*	psBlock,
	USBD_EVENT_T				eEvent,
	img_uint32					ui32Param,
	img_void				*	pvParam
)
{
	IMG_ASSERT( USBD_IsInitialised( psBlock->sMainDevice.id ) );

	return USBD_ExecuteCallback(	psBlock->sMainDevice.id,
									eEvent,
									ui32Param,
									pvParam );
}

#endif
