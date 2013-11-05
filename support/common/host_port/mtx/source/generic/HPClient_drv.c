/*!
******************************************************************************
 @file   HPClient_drv.c

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

#include "HPClient_drv.h"
#include "HP_common.h"

/*============================================================================
====	D E F I N E S
=============================================================================*/
#define WRITE(a,d)    (*(volatile unsigned long *)(a) = (d))
#define READ(a)       (*(volatile unsigned long *)(a))

/*============================================================================
====	F U N C T I O N S
=============================================================================*/

static int  HostToMtxDriverInit( QIO_DEVICE_T *pDevice, QIO_DEVPOWER_T *pPowerClass, int *pnDeviceRank, unsigned int ui32InterruptMasks[QIO_MAXVECGROUPS] );
static void HostToMtxDriverStart( QIO_DEVICE_T *pDevice, QIO_IOPARS_T *pIOPars );
static void HostToMtxDriverISR( QIO_DEVICE_T *pDevice );
static void HostToMtxDriverCancel( QIO_DEVICE_T *pDevice );
static IMG_VOID HPClient_rxStart ( IMG_UINT32 ui32register );
static IMG_VOID HPClient_rxPayload ( IMG_UINT32 ui32register );

/*============================================================================
====	D A T A
=============================================================================*/

/* the driver object */
const QIO_DRIVER_T HP_host_to_mtx_driver =
{
    HostToMtxDriverISR,      /* ISR                       */
    HostToMtxDriverInit,     /* init function             */
    HostToMtxDriverStart,    /* start function            */
    HostToMtxDriverCancel,	 /* cancel function           */
    IMG_NULL,				 /* no power control function */
    IMG_NULL,				 /* no sim start function     */
    IMG_NULL
};

QIO_DEVICE_T	HP_host_to_mtx_device;
HPCLIENT_DEVICE_CONTEXT_T g_sHPClientContext;


/*!
******************************************************************************

 @Function              DriverInit

 @Description	Driver initialisation. Configures the SCB master hardware.
				Updates information about the device's power class and rank.

 @Input		pDevice					: Pointer to the QIO device object.
 			ui32InterruptMasks		: The system's interrupt masks, to be updated with the device's interrupt number.

 @Output	pPowerClass				: Updated with the device's power class.
 			pDeviceRank				: Updated with the device's rank.

 @Return	int

******************************************************************************/
static int HostToMtxDriverInit(QIO_DEVICE_T *pDevice, QIO_DEVPOWER_T *pPowerClass, int *pDeviceRank, unsigned int ui32InterruptMasks[QIO_MAXVECGROUPS])
{
	int lockState;
	unsigned int i;

	(void)pDevice;     /* Remove warnings about unused parameters */

    /* No power saving */
    *pPowerClass = QIO_POWERNONE;

    /* Only a single rank */
    *pDeviceRank = 1;

    /*===================*/
    /* Set up interrupts */
    /*===================*/

	for ( i = 0; i < QIO_MAXVECGROUPS; ++i )
	{
		ui32InterruptMasks[i] = 0;
	}

    //set correct bit in HWSTATEXT interrupt mask
    ui32InterruptMasks[0] = (1 << HOST_INT_INTERRUPT_BIT);

    //read-modify-write to configure level sensitive interrupt
    TBI_LOCK(lockState);

    WRITE(HWLEVELEXT, (READ(HWLEVELEXT) | (1 << HOST_INT_INTERRUPT_BIT)));

    TBI_UNLOCK(lockState);

	return 0;
}

/*!
******************************************************************************

 @Function              DriverStart

 @Description	device operation entry point. This function is called when a
				new operation is de-queued.

 @Input		pDevice			: Pointer to the QIO device object.
 			pIOPars			: Pointer to the IO parameters.

 @Output	None

 @Return	void

******************************************************************************/
static void HostToMtxDriverStart(QIO_DEVICE_T *pDevice, QIO_IOPARS_T *pIOPars)
{
	(void)pDevice;     /* Remove warnings about unused parameters */

    g_sHPClientContext.pui8rxBuffer = (IMG_UINT8 *) pIOPars->pointer;
    g_sHPClientContext.ui32rxBufferSizeInBytes = pIOPars->counter;
    g_sHPClientContext.pui32numberOfBytesRead = (IMG_UINT32 *) pIOPars->spare;
    g_sHPClientContext.bRxFinished = IMG_FALSE;

	//enable the HOST_INT
	WRITE_REG(HP_BASE_ADDRESS, CR_PERIP_HOST_INT_ENABLE, WRITE_REG_FIELD(0, CR_PERIP_HOST_INT_EN, 1));
}

/*!
******************************************************************************

 @Function              DriverISR

 @Description	Interrupt service routine

 @Input		pDevice		: Pointer to the QIO device object.

 @Output	None

 @Return	void

******************************************************************************/
static void HostToMtxDriverISR(QIO_DEVICE_T *pDevice)
{
	unsigned long ui32register;

	(void)pDevice;     /* Remove warnings about unused parameters */

	ui32register = READ(HWSTATEXT);
	if ( !( ui32register & (1 << HOST_INT_INTERRUPT_BIT) ) )
	{
		/* spurious interrupt ? */
		assert(0);
		return;
	}

	ui32register = READ_REG(HP_BASE_ADDRESS, CR_PERIP_HOST_TO_MTX_CMD);
	if (!( READ_REG_FIELD(ui32register, CR_PERIP_HOST_INT) ))
	{
		/* spurious interrupt ? */
		assert( 0 );
		return;
	}

	if ( g_sHPClientContext.bRxFinished == IMG_TRUE )
	{
		/*
		** we shouldn't be here, but it's possible that HOST_INT got set
		** during the QIO_cancel. In that case we should ignore the
		** interrupt until the start function sets up a new transfer.
		*/
		return;
	}

	switch (g_sHPClientContext.eState)
	{
		case HPCLIENT_WAIT_FOR_START:
				HPClient_rxStart(ui32register);
				break;

		case HPCLIENT_WAIT_FOR_PAYLOAD:
				HPClient_rxPayload(ui32register);
				break;
	}

	if ( g_sHPClientContext.bRxFinished == IMG_TRUE )
	{
		//disable the HOST_INT
		WRITE_REG(HP_BASE_ADDRESS, CR_PERIP_HOST_INT_ENABLE, WRITE_REG_FIELD(0, CR_PERIP_HOST_INT_EN, 0));

		//now send the ACK
		WRITE_REG(HP_BASE_ADDRESS, CR_PERIP_MTX_TO_HOST_ACK, WRITE_REG_FIELD(0, CR_PERIP_HOST_INT_CLR, 1));

		/* Complete the IO. */
		QIO_complete(pDevice, QIO_IOCOMPLETE);
		QIO_start(pDevice);
	}
	else
	{
		//now send the ACK
		WRITE_REG(HP_BASE_ADDRESS, CR_PERIP_MTX_TO_HOST_ACK, WRITE_REG_FIELD(0, CR_PERIP_HOST_INT_CLR, 1));
	}

	return;
}

IMG_VOID HPClient_rxStart ( IMG_UINT32 ui32register )
{
	//does ui32register valid contain a start message?
	if (((ui32register & HP_CONTROL_BITS_MASK) >> HP_CONTROL_BITS_SHIFT) == HP_START_MESSAGE)
	{
		//we have a start message, so check payload for count to recieve
		g_sHPClientContext.ui32countToRx = ((ui32register & HP_PAYLOAD_BITS_MASK) >> HP_PAYLOAD_BITS_SHIFT);
		g_sHPClientContext.pui8rxBufferCurrent = g_sHPClientContext.pui8rxBuffer;
		*g_sHPClientContext.pui32numberOfBytesRead = 0;

		//update the count and the state machine
		g_sHPClientContext.eState = HPCLIENT_WAIT_FOR_PAYLOAD;
	}
	else
	{
		//we have a unexpected message
		//silently ignore
	}
}

IMG_VOID HPClient_rxPayload ( IMG_UINT32 ui32register )
{
	unsigned int j;

	//does ui32register contain a valid payload message?
	if (((ui32register & HP_CONTROL_BITS_MASK) >> HP_CONTROL_BITS_SHIFT) == HP_PAYLOAD_MESSAGE)
	{
		/*
		** Unpack 3 (or less) bytes of data
		*/
		j = 3;
		while (j != 0)
		{
			j--;
			if ( ( *g_sHPClientContext.pui32numberOfBytesRead < g_sHPClientContext.ui32countToRx ) &&
			     ( *g_sHPClientContext.pui32numberOfBytesRead < g_sHPClientContext.ui32rxBufferSizeInBytes ) )
			{
				//we have some data
				*g_sHPClientContext.pui8rxBufferCurrent = (ui32register >> (8*j)) & 0xFF;
				g_sHPClientContext.pui8rxBufferCurrent++;
				(*g_sHPClientContext.pui32numberOfBytesRead)++;
			}
		}

		//have we recieved all of this transfer yet?
		if ( *g_sHPClientContext.pui32numberOfBytesRead == g_sHPClientContext.ui32countToRx )
		{
			//get the message back...
			g_sHPClientContext.bRxFinished = IMG_TRUE;

			//update the state machine
			g_sHPClientContext.eState = HPCLIENT_WAIT_FOR_START;
		}
	}
	else if (((ui32register & HP_CONTROL_BITS_MASK) >> HP_CONTROL_BITS_SHIFT) == HP_START_MESSAGE)
	{
		//we have a unexpected start message
		//we have a start message, so check payload for count to recieve
		g_sHPClientContext.ui32countToRx = ((ui32register & HP_PAYLOAD_BITS_MASK) >> HP_PAYLOAD_BITS_SHIFT);
		g_sHPClientContext.pui8rxBufferCurrent = g_sHPClientContext.pui8rxBuffer;
		*g_sHPClientContext.pui32numberOfBytesRead = 0;
	}
	else
	{
		//we have a unexpected message
		//silently ignore
	}
}

/*!
******************************************************************************

 @Function              DriverCancel

 @Description	Cancellation function (needed for timeouts)

 @Input		pDevice		: Pointer to the QIO device object.

 @Output	None

 @Return	void

******************************************************************************/
static void HostToMtxDriverCancel( QIO_DEVICE_T *pDevice )
{
	(void)pDevice;     /* Remove warnings about unused parameters */

	g_sHPClientContext.bRxFinished = IMG_TRUE;

	//update the state machine
	g_sHPClientContext.eState = HPCLIENT_WAIT_FOR_START;
}

/*============================================================================
	E N D
=============================================================================*/
