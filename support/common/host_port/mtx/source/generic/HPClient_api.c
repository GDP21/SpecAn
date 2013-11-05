/*!
******************************************************************************
 @file   HPClient_api.c

 @brief  Host Port Interface, Client side

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

\n\n\n

******************************************************************************/

/*============================================================================
====	I N C L U D E S
=============================================================================*/

#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

/* MeOS Library */
#include <MeOS.h>

#include "img_defs.h"
#include "sys_util.h"
#include "hp_reg_defs.h"

#include "HPClient_api.h"
#include "HPClient_drv.h"
#include "HP_common.h"

/*============================================================================
====	D E F I N E S
=============================================================================*/

/*============================================================================
====	E N U M S
=============================================================================*/

/*============================================================================
====	T Y P E D E F S
=============================================================================*/

/*============================================================================
====	D A T A
=============================================================================*/

KRN_TASKQ_T HP_mtx_to_host_poll_queue;
IMG_UINT32 g_ui32pollingPeriod;
KRN_TIMER_T writeTimer;

extern const QIO_DRIVER_T HP_host_to_mtx_driver;
extern QIO_DEVICE_T	HP_host_to_mtx_device;
extern HPCLIENT_DEVICE_CONTEXT_T g_sHPClientContext;

/*============================================================================
====	F U N C T I O N S
=============================================================================*/
void writeTimeoutFunction(KRN_TIMER_T *timer, void *timerPar);
static void HPClient_WriteReadyToServer (void);

/*!
******************************************************************************

 @Function              init

******************************************************************************/
HPClient_eResult HPClient_init (
    IMG_UINT32			ui32pollingPeriod
    )
{
	//check not already initialsed
	if ( g_sHPClientContext.bInitialised == IMG_TRUE )
	{
		assert(0);
		return HPCLIENT_ERROR;
	}

	//check valid inputs
	if (ui32pollingPeriod == 0)
	{
		assert(0);
		return HPCLIENT_ILLEGAL_PARAM;
	}
	g_ui32pollingPeriod = ui32pollingPeriod;

	//init the polling /sleeping queue
	DQ_init(&HP_mtx_to_host_poll_queue);

    //
    // Initialise the read device.
    //
    QIO_init( &HP_host_to_mtx_device,
	         "Host to Client",
	         (unsigned int) NULL,
             &HP_host_to_mtx_driver);

	//
	// Enable the devices.
	//
	QIO_enable(&HP_host_to_mtx_device);

	//write the ready to the server
	HPClient_WriteReadyToServer();

	// Mark API as initialised
	g_sHPClientContext.bInitialised = IMG_TRUE;

	return HPCLIENT_SUCCESS;
}


/*!
******************************************************************************

 @Function              HPClient_ReadServer

******************************************************************************/
HPClient_eResult HPClient_ReadServer (
    IMG_UINT8			*pui8buffer,						//buffer address
    IMG_UINT32 			ui32bufferSizeInBytes,				//buffer size
    IMG_UINT32			*pui32numberOfBytesRead,			//number of bytes read
    IMG_INT32			timeout
    )
{
    QIO_IOPARS_T  sIoPars;
    QIO_STATUS_T qio_status;
    HPClient_eResult status = HPCLIENT_SUCCESS;

	//check initialsed
	if ( g_sHPClientContext.bInitialised == IMG_FALSE )
	{
		assert(0);
		return HPCLIENT_ERROR;
	}

	//check valid pointers
	if ( (pui8buffer == IMG_NULL) | (pui32numberOfBytesRead == IMG_NULL) )
	{
		assert(0);
		return HPCLIENT_ILLEGAL_PARAM;
	}

	if (timeout == 0)
	{
		assert(0);
		return HPCLIENT_ILLEGAL_PARAM;
	}

    //
    // Build the IO parameters.
    //
    sIoPars.opcode  = (unsigned int) NULL;					//not needed
    sIoPars.pointer = pui8buffer;
    sIoPars.counter = ui32bufferSizeInBytes;
    sIoPars.spare   = pui32numberOfBytesRead;

	//
	// Synchronous I/O.
	//
	qio_status = QIO_qioWait(&HP_host_to_mtx_device, &sIoPars, timeout);

    if (qio_status == QIO_NORMAL)
    {
        status = HPCLIENT_SUCCESS;
    }
    else if (qio_status == QIO_CANCEL)
    {
        status = HPCLIENT_READ_CANCEL;
        *pui32numberOfBytesRead = 0;
    }
    else if (qio_status == QIO_TIMEOUT)
    {
        status = HPCLIENT_READ_TIMEOUT;
        *pui32numberOfBytesRead = 0;
    }

	return status;
}

/*!
******************************************************************************

 @Function              HPClient_WriteToServer

******************************************************************************/
HPClient_eResult HPClient_WriteToServer (
    IMG_UINT8			*pui8buffer,						//buffer address
    IMG_UINT32 			ui32transferSizeInBytes,			//buffer size
    IMG_UINT32			*pui32numberBytesTransferred,		//number of bytes transferred
    IMG_INT32			timeout
    )
{
	IMG_UINT8 *pui8tempBuffer;
	IMG_UINT32 ui32msg, j;

	//check initialsed
	if ( g_sHPClientContext.bInitialised == IMG_FALSE )
	{
		assert(0);
		return HPCLIENT_ERROR;
	}

	//check valid pointers
	if ( (pui8buffer == IMG_NULL) || (pui32numberBytesTransferred == IMG_NULL) )
	{
		assert(0);
		return HPCLIENT_ILLEGAL_PARAM;
	}
	*pui32numberBytesTransferred = 0;

	//check there is actual data to send (and it's not to big)
	if ( (ui32transferSizeInBytes == 0) | (ui32transferSizeInBytes > HPCLIENT_MAX_DATA_BYTES) )
	{
		assert(0);
		return HPCLIENT_ILLEGAL_PARAM;
	}

	g_sHPClientContext.bWriteTimeoutFlag = IMG_FALSE;
	if (timeout == 0)
	{
		assert(0);
		return HPCLIENT_ILLEGAL_PARAM;
	}
	else if (timeout > 0)
	{
		KRN_setTimer(&writeTimer, writeTimeoutFunction, IMG_NULL, timeout);
	}

	//poll the MTX_INT bit. When clear then the host is ready to recieve
	while ( READ_REG_FIELD(READ_REG(HP_BASE_ADDRESS, CR_PERIP_MTX_TO_HOST_CMD),CR_PERIP_MTX_INT) )
	{
		/* MTX_INT asserted,, so we cannot write yet */
		if ( g_sHPClientContext.bWriteTimeoutFlag == IMG_TRUE )
		{
			//we have timed out
			return HPCLIENT_WRITE_TIMEOUT;
		}
		KRN_hibernate(&HP_mtx_to_host_poll_queue, g_ui32pollingPeriod);
	}

	pui8tempBuffer = pui8buffer;

	//start msg
	ui32msg = ((HP_START_MESSAGE << HP_CONTROL_BITS_SHIFT) & HP_CONTROL_BITS_MASK)
	          | ((ui32transferSizeInBytes << HP_PAYLOAD_BITS_SHIFT) & HP_PAYLOAD_BITS_MASK);

	//write it to the register
	WRITE_REG(HP_BASE_ADDRESS, CR_PERIP_MTX_TO_HOST_CMD, WRITE_REG_FIELD(0, CR_PERIP_MTX_DATA, ui32msg));

	//for each data write required
	while (*pui32numberBytesTransferred < ui32transferSizeInBytes)
	{
		//poll the MTX_INT bit. When clear then the host is ready to recieve
		while ( READ_REG_FIELD(READ_REG(HP_BASE_ADDRESS, CR_PERIP_MTX_TO_HOST_CMD),CR_PERIP_MTX_INT) )
		{
			/* MTX_INT asserted,, so we cannot write yet */
			if ( g_sHPClientContext.bWriteTimeoutFlag == IMG_TRUE )
			{
				//we have timed out
				return HPCLIENT_WRITE_TIMEOUT;
			}
			KRN_hibernate(&HP_mtx_to_host_poll_queue, g_ui32pollingPeriod);
		}

		//serialise 3 bytes of data. If there are less than 3 bytes left, NULL stuff.
		ui32msg = ((HP_PAYLOAD_MESSAGE << HP_CONTROL_BITS_SHIFT) & HP_CONTROL_BITS_MASK);
		j = 3;
		while (j != 0)
		{
			j--;
			if ( (*pui32numberBytesTransferred) < ui32transferSizeInBytes )
			{
				//we have some data
				ui32msg |= ( ((*pui8tempBuffer) & 0xFF) << (8*j) );
				pui8tempBuffer++;
				(*pui32numberBytesTransferred)++;
			}
			else
			{
				//we have reached the end of data
				pui8tempBuffer++;
			}
		}

		//write it to the register
		WRITE_REG(HP_BASE_ADDRESS, CR_PERIP_MTX_TO_HOST_CMD, WRITE_REG_FIELD(0, CR_PERIP_MTX_DATA, ui32msg));
	}

	if (timeout > 0)
	{
		//cancel the timeout timer. Don't care if timer already expired or cancelled
		KRN_cancelTimer(&writeTimer);
	}

	return HPCLIENT_SUCCESS;
}

void writeTimeoutFunction(KRN_TIMER_T *timer, void *timerPar)
{
	(void)timer;     /* Remove warnings about unused parameters */
	(void)timerPar;

	/*
	** If the timeout function executes, then we have taken to long to
	** complete the write. Signal this to the HPServer_WriteToClient loop
	*/
	g_sHPClientContext.bWriteTimeoutFlag = IMG_TRUE;
}

static void HPClient_WriteReadyToServer (void)
{
	unsigned int ui32msg;

	/*
	** This function assumes a power up state in the host port hardware, and
	** therefore doesn't do any polling/waiting
	*/

	//ready msg
	ui32msg = ((HP_CLIENT_READY_MESSAGE << HP_CONTROL_BITS_SHIFT) & HP_CONTROL_BITS_MASK);

	//write it to the register
	WRITE_REG(HP_BASE_ADDRESS, CR_PERIP_MTX_TO_HOST_CMD, WRITE_REG_FIELD(0, CR_PERIP_MTX_DATA, ui32msg));

}


/*============================================================================
	E N D
=============================================================================*/
