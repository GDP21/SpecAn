/*!
*******************************************************************************
  file   spis_drv.c

  brief  Serial Peripheral Interface Slave Device Driver API

         This file defines the functions that make up the Serial Peripheral
         Interface Slave (SPIS) device driver.

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

#include <assert.h>

/* MeOS Library */
#include <MeOS.h>

/* System */
#include <ioblock_defs.h>
#include <ioblock_utils.h>
#include <sys_util.h>
#include <gdma_api.h>

/* SPI Slave Driver */
#include "spis_api.h"
#include "spis_drv.h"
#include "spis_reg.h"

/* -------------------------- MACRO DEFINITIONS -------------------------- */

/* definitions to improve readability */
#ifndef TRUE
#define TRUE  (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#define WRITE(a,d)    (*(volatile unsigned long *)(a) = (d))
#define READ(a)       (*(volatile unsigned long *)(a))


/* --------------------------- DATA STRUCTURES --------------------------- */

/* ------------------------- FUNCTION PROTOTYPES ------------------------- */

static int  init(QIO_DEVICE_T *dev, QIO_DEVPOWER_T *pwrClass, int *devRank, unsigned intMasks[QIO_MAXVECGROUPS]);
static void start(QIO_DEVICE_T *dev, QIO_IOPARS_T *ioPars);
static void isr(QIO_DEVICE_T *dev);


/* ----------------------------- GLOBAL DATA ----------------------------- */

/* the driver object */
const QIO_DRIVER_T SPIS_driver =
{
    isr,	/* ISR                       */
    init,	/* init function             */
    start,	/* start function            */
    NULL,	/* no cancel function        */
    NULL,	/* no power control function */
    NULL,	/* no sim start function     */
    NULL   	/* no shut function          */
};

// MAX_NUM_SPIS_BLOCKS defined in spis_drv.h
ioblock_sBlockDescriptor	*	g_apsSPISBlock[ MAX_NUM_SPIS_BLOCKS ] =
{
	IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL
};

/* --------------------------- STATIC FUNCTIONS -------------------------- */

/*
** FUNCTION:    init
**
** DESCRIPTION: Driver initialisation. Configures the SPI hardware. Updates
**              information about the device's power class and rank.
**
** INPUTS:      NAME            DESCRIPTION
**              dev             Pointer to the QIO device object.
**              intNumber       The device's interrupt number.
**
** OUTPUTS:     NAME            DESCRIPTION
**              pwrClass        Updated with the device's power class.
**              devRank         Updated with the device's rank.
**
** RETURNS:     void
*/
static int init(QIO_DEVICE_T *dev, QIO_DEVPOWER_T *pwrClass, int *devRank, unsigned intMasks[QIO_MAXVECGROUPS])
{
	unsigned int					controlReg = 0;
	unsigned long					ui32Reg;
    int								lockState;
    SPIS_PARAM_T				*	paramPtr;
	ioblock_sBlockDescriptor	*	psBlockDesc;
	SPIS_PORT_T					*	psContext;

	// get pointer to the device setup parameters, then build the control register.
    paramPtr = (SPIS_PARAM_T *) dev->id;

	// Get context structure from block descriptor
	psBlockDesc		= g_apsSPISBlock[ paramPtr->ui32BlockIndex ];
	psContext		= (SPIS_PORT_T	*)psBlockDesc->pvAPIContext;
	
	psContext->ui32DMAChannel = paramPtr->dmaChannel;

    switch (paramPtr->spiMode)
    {
		case SPI_MODE_0:
			// do nothing
			break;
		case SPI_MODE_1:
			controlReg = WRITE_REG_FIELD( controlReg, SPI_S_CK_PHASE, 1 );
			break;
		case SPI_MODE_2:
			controlReg = WRITE_REG_FIELD( controlReg, SPI_S_CK_IDLE, 1 );
			break;
		case SPI_MODE_3:
        default: //default added to keep compiler happy
			controlReg = WRITE_REG_FIELD( controlReg, SPI_S_CK_PHASE, 1 );
			controlReg = WRITE_REG_FIELD( controlReg, SPI_S_CK_IDLE, 1 );
			break;
	}

	switch ( paramPtr->spiSyncMode )
	{
		case SPI_SYNC_MODE_RESYNC:
			controlReg = WRITE_REG_FIELD( controlReg, SPI_S_TX_RESYNC, 1 );
			controlReg = WRITE_REG_FIELD( controlReg, SPI_S_RX_RESYNC, 1 );
			break;
		case SPI_SYNC_MODE_SLOW:
			controlReg = WRITE_REG_FIELD( controlReg, SPI_S_TX_SLOWSYNC, 1 );
			controlReg = WRITE_REG_FIELD( controlReg, SPI_S_RX_SLOWSYNC, 1 );
			break;
		case SPI_SYNC_MODE_LEGACY:
		default:
			// nothing to do here
			break;
	}

	if (paramPtr->csLevel)
	{
        controlReg = WRITE_REG_FIELD( controlReg, SPI_S_CS_LEVEL, 1 );
	}

    /* provide information about the device */
    *devRank = 1;
    *pwrClass = QIO_POWERNONE;

	psContext->psIOPars = IMG_NULL;

	/* initialise the SPI hardware */
    WRITE_REG( psBlockDesc->ui32Base, SPI_S_CNTRL, controlReg);

	/* Set the input and output FIFO threshold lengths to 16 */
	WRITE_REG( psBlockDesc->ui32Base, SPI_S_FIFO_FLAG, 0x00001010 );
	
	if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind )
	{
		if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind( psBlockDesc->ui32DMACChannelSelectWriteValue, psContext->ui32DMAChannel ) != IMG_SUCCESS )
		{
			/* Bind failed */
			IMG_ASSERT (0);	
		}
	}

	IOBLOCK_CalculateInterruptInformation( psBlockDesc );

	IMG_MEMCPY( intMasks, psBlockDesc->ui32IntMasks, sizeof( img_uint32 ) * QIO_MAXVECGROUPS );

	//read-modify-write to configure level sensitive interrupts
	TBI_LOCK(lockState);

	ui32Reg = READ( psBlockDesc->sDeviceISRInfo.ui32LEVELEXTAddress );
	if ( psBlockDesc->eInterruptLevelType == HWLEVELEXT_LATCHED )
	{
		ui32Reg &= ~( psBlockDesc->sDeviceISRInfo.ui32LEVELEXTMask );
	}
	else if ( psBlockDesc->eInterruptLevelType == HWLEVELEXT_NON_LATCHED )
	{
		ui32Reg |= ( psBlockDesc->sDeviceISRInfo.ui32LEVELEXTMask );
	}
	WRITE( psBlockDesc->sDeviceISRInfo.ui32LEVELEXTAddress, ui32Reg );

	TBI_UNLOCK(lockState);

	// Update the id member to indicate the block index for the ISR and start functions.
	dev->id = paramPtr->ui32BlockIndex;

    return 0;
}

/*
** FUNCTION:    start
**
** DESCRIPTION: Device operation entry point. This function is called when a
**              new operation is de-queued.
**
** INPUTS:      NAME            DESCRIPTION
**              dev             Pointer to the QIO device object.
**              ioPars          Pointer to the IO parameters.
**
** OUTPUTS:     NAME            DESCRIPTION
**              None.
**
** RETURNS:     void
*/
static void start(QIO_DEVICE_T *dev, QIO_IOPARS_T *ioPars)
{
	unsigned int					controlReg;
	ioblock_sBlockDescriptor	*	psBlockDesc;
	SPIS_PORT_T					*	psContext;
	GDMA_sTransferObject		*	psTransfer;

	// get pointer to the device setup parameters
	psBlockDesc = g_apsSPISBlock[ dev->id ];
	psContext	= (SPIS_PORT_T	*)psBlockDesc->pvAPIContext;
	psTransfer	= &psContext->sDMATransfer;

	// Save current transaction
	psContext->psIOPars = ioPars;

	// Set up common DMA parameters
	psTransfer->ui32SizeInBytes		= ioPars->counter;
	psTransfer->ui8Priority			= 0;
	psTransfer->bReverseEndianism	= IMG_FALSE;
	psTransfer->ui32BurstSize		= 16;
	psTransfer->pvUserData			= (img_void *)psBlockDesc;

	// configure the read/write
	if ((ioPars->opcode) == SPIS_OPCODE_READ)	//Read message
	{
		// Rebind the SPI slave to the desired DMA channel */
		if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind )
		{
			if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind( psBlockDesc->ui32DMACChannelSelectWriteValue, psContext->ui32DMAChannel ) != IMG_SUCCESS )
			{
				/* Bind failed */
				IMG_ASSERT (0);	
			}
		}

		//set rx_enable bit and disable tx_enable bit in control register
		controlReg = READ_REG( psBlockDesc->ui32Base, SPI_S_CNTRL );
		controlReg = (controlReg | SPI_S_RX_EN_MASK) & ~SPI_S_TX_EN_MASK;
		WRITE_REG( psBlockDesc->ui32Base, SPI_S_CNTRL , controlReg);

		// Set up DMA
		psTransfer->pui8ReadPointer		= (img_uint8 *)(psBlockDesc->ui32Base + SPI_S_RX_DATA_OFFSET);
		psTransfer->bIncReadPointer		= IMG_FALSE;
		psTransfer->ui32ReadWidth		= 1;
		psTransfer->pui8WritePointer	= ioPars->pointer;
		psTransfer->bIncWritePointer	= IMG_TRUE;
		psTransfer->ui32WriteWidth		= 4;

		GDMA_SingleShotOperation( &psContext->sDMAContext, psTransfer, 0 );
	}
	else	//Write message
	{
		// Rebind the SPI slave to the desired DMA channel
		if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind )
		{
			if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind( psBlockDesc->ui32DMACChannelSelectReadValue, psContext->ui32DMAChannel ) != IMG_SUCCESS )
			{
				/* Bind failed */
				IMG_ASSERT (0);	
			}
		}

		//reset the interrupt status, then enable
		WRITE_REG( psBlockDesc->ui32Base, SPI_S_RICL, SPI_S_SDEMP_CL_MASK);
		WRITE_REG( psBlockDesc->ui32Base, SPI_S_RIE, SPI_S_SDEMP_EN_MASK);

		//set tx_enable bit and disable rx_enable bit in control register
		controlReg = READ_REG( psBlockDesc->ui32Base, SPI_S_CNTRL );
		controlReg = ((controlReg | SPI_S_TX_EN_MASK) & ~SPI_S_RX_EN_MASK);
		WRITE_REG( psBlockDesc->ui32Base, SPI_S_CNTRL, controlReg);

		// Set up DMA
		psTransfer->pui8ReadPointer		= ioPars->pointer;
		psTransfer->bIncReadPointer		= IMG_TRUE;
		psTransfer->ui32ReadWidth		= 4;
		psTransfer->pui8WritePointer	= (img_uint8 *)(psBlockDesc->ui32Base + SPI_S_TX_DATA_OFFSET);
		psTransfer->bIncWritePointer	= IMG_FALSE;
		psTransfer->ui32WriteWidth		= 1;

		GDMA_SingleShotOperation( &psContext->sDMAContext, psTransfer, 0 );
	}
}

/*
** FUNCTION:    isr
**
** DESCRIPTION: Interrupt service routine
**
** INPUTS:      NAME            DESCRIPTION
**              dev             Pointer to the QIO device object.
**
** OUTPUTS:     NAME            DESCRIPTION
**              None.
**
** RETURNS:     void
*/
static void isr(QIO_DEVICE_T *dev)
{
	unsigned long					ui32Reg;
	ioblock_sBlockDescriptor	*	psBlockDesc	= g_apsSPISBlock[ dev->id ];
	
	// This ISR should only be called for a mem->perip transfer

	ui32Reg = READ( psBlockDesc->sDeviceISRInfo.ui32STATEXTAddress );
	if ( !( ui32Reg & psBlockDesc->sDeviceISRInfo.ui32STATEXTMask ) )
	{
		// Spurious interrupt ?
		IMG_ASSERT(0);
		return;
	}

	// see if the send data FIFO is empty
	if( READ_REG( psBlockDesc->ui32Base, SPI_S_RIS ) & (1 << SPI_S_SDEMP_SHIFT) )
	{
		//clear interrupt and disable.
		WRITE_REG( psBlockDesc->ui32Base, SPI_S_RICL, SPI_S_SDEMP_CL_MASK);
		WRITE_REG( psBlockDesc->ui32Base, SPI_S_RIE, 0);
	}

	QIO_complete(dev, QIO_IOCOMPLETE);
	QIO_start(dev);
}

IMG_RESULT	SpisDmaComplete(	GDMA_sTransferObject	*	psTransferObject, QIO_STATUS_T	eQIOStatus, img_void	*	pvUserContext	)
{
	ioblock_sBlockDescriptor	*	psBlockDesc	= (ioblock_sBlockDescriptor	*)pvUserContext;
	SPIS_PORT_T					*	psContext	= (SPIS_PORT_T *)psBlockDesc->pvAPIContext;

	// This function will be called for both perip->mem and mem->perip transfers. We don't do anything in the latter case.

	if ( psContext->psIOPars->opcode == SPIS_OPCODE_READ )
	{
		// Perip->Mem
		
		// Flush the cache if needed

		if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
		{
			psBlockDesc->psSystemDescriptor->pfn_FlushCache( (img_uint32)psContext->psIOPars->pointer, psContext->psIOPars->counter );
		}

		// Complete the SPIS transaction
		QIO_complete( &psContext->sDevice, QIO_IOCOMPLETE );
		QIO_start( &psContext->sDevice );
	}
	else
	{
		// Mem->perip: Do nothing
	}

	return IMG_SUCCESS;
}
