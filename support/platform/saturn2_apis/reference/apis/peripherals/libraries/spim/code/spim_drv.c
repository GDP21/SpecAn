/*!
*******************************************************************************
  file   spim_drv.c

  brief  Serial Peripheral Interface Master Device Driver

         Serial Peripheral Interface Master (SPIM) device driver.

  author Imagination Technologies

         <b>Copyright 2006-2007 by Imagination Technologies Limited.</b>\n
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

/*============================================================================*/
/*                                                                            */
/*                          INCLUDE FILES		                              */
/*                                                                            */
/*============================================================================*/

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

/* System */
#include <ioblock_defs.h>
#include <ioblock_utils.h>
#include <sys_util.h>
#include <gdma_api.h>
#include <gdma_hal.h>

/* SPI driver */
#include "spim_api.h"
#include "spim_drv.h"
#include "spim_reg.h"


/*============================================================================*/
/*                                                                            */
/*                          MACRO DEFINITIONS	                              */
/*                                                                            */
/*============================================================================*/

/* driver context state machine */
enum eDeviceStates 
{ 
	IDLE = 0, 
	FIRST, 
	SECOND 
};

/* definitions to improve readability */
#ifndef TRUE
#define TRUE  (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#define WRITE(a,d)    (*(volatile unsigned long *)(a) = (d))
#define READ(a)       (*(volatile unsigned long *)(a))
//#define WRITE(a,d)		SYS_LogRegWrite(a, d)
//#define READ(a)			SYS_LogRegRead(a)

//#define DEBUG

#ifdef DEBUG
#define DEBUGLOG(x...)		__TBILogF(x)
#else
#define DEBUGLOG(x...)		
#endif

/*============================================================================*/
/*                                                                            */
/*                          DATA STRUCTURES		                              */
/*                                                                            */
/*============================================================================*/

/* SPI transaction register. LSB first. */
typedef struct spim_trans_t
{
    IMG_UINT32			bfCount		: 16;
    IMG_UINT32			bfDevice	: 2;
    IMG_UINT32						: 6;
    IMG_UINT32			bfSendDma	: 1;
    IMG_UINT32			bfGetDma	: 1;
    IMG_UINT32			bfSoftReset	: 1;
    IMG_UINT32			bfCont		: 1;
    IMG_UINT32			bfCSDeass	: 1;
    IMG_UINT32			bfByteDelay	: 1;
    IMG_UINT32						: 2;
    	
} SPIM_sTransReg;

/*============================================================================*/
/*                                                                            */
/*                          FUNCTION PROTOTYPES	                              */
/*                                                                            */
/*============================================================================*/

static int  DriverInit(	QIO_DEVICE_T	*,
						QIO_DEVPOWER_T	*,
						int				*,
						unsigned		intMasks[ QIO_MAXVECGROUPS ] );

static void DriverStart(QIO_DEVICE_T	*,
						QIO_IOPARS_T	* );

static void DriverISR(	QIO_DEVICE_T	* );


/*============================================================================*/
/*                                                                            */
/*                          GLOBAL DATA			                              */
/*                                                                            */
/*============================================================================*/

/* the driver object */
const QIO_DRIVER_T g_sDriver = {
    DriverISR,		/* ISR                       */
    DriverInit,		/* init function             */
    DriverStart,	/* start function            */
    NULL,			/* no cancel function        */
    NULL,			/* no power control function */
    NULL,			/* no sim start function     */
    NULL			/* no shut function          */
};


/*============================================================================*/
/*                                                                            */
/*                          STATIC DATA			                              */
/*                                                                            */
/*============================================================================*/

// MAX_NUM_SPIM_BLOCKS defined in spim_drv.h
ioblock_sBlockDescriptor	*	g_apsSPIMBlock[ MAX_NUM_SPIM_BLOCKS ] =
{
	IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL
};

/*============================================================================*/
/*                                                                            */
/*                          STATIC FUNCTIONS			                      */
/*                                                                            */
/*============================================================================*/

/*!
******************************************************************************

 @Function              DriverInit

 @Description	Driver initialisation. Configures the SPI hardware. Updates
				information about the device's power class and rank..

 @Input		psDev			: Pointer to the QIO device object.
 			uIntMasks		: The system's interrupt masks, to be updated with the
 							  device's interrupt number.

 @Output	psPwrClass		: Updated with the device's power class.
 			piDevRank		: Updated with the device's rank.

 @Return	voids

******************************************************************************/
static IMG_INT DriverInit(	QIO_DEVICE_T	*	psDev, 
							QIO_DEVPOWER_T	*	psPwrClass, 
							IMG_INT			*	piDevRank, 
							unsigned			uIntMasks[ QIO_MAXVECGROUPS ] )
{
    IMG_INT32						iLockState;
    IMG_INT32						n;
    IMG_UINT32						ui32SPIControl;
    SPIM_sInitParam				*	psInitParams;
	IMG_UINT32						ui32Reg;
	ioblock_sBlockDescriptor	*	psBlockDesc;
	SPIM_sBlock					*	psDeviceContext;
	
    IMG_UINT32						aui32DeviceParamConfig	[ SPIM_NUM_PORTS_PER_BLOCK ];
    IMG_UINT8						aui8DeviceControl		[ SPIM_NUM_PORTS_PER_BLOCK ];
    SPIM_sDeviceParam			*	apsDeviceParam			[ SPIM_NUM_PORTS_PER_BLOCK ];	

    psInitParams = (SPIM_sInitParam *)psDev->id;

	// Get context structure from block descriptor
	psBlockDesc		= g_apsSPIMBlock[ psInitParams->ui32BlockIndex ];
	psDeviceContext = (SPIM_sBlock *)psBlockDesc->pvAPIContext;
	psDeviceContext->ui32DMAChannel = psInitParams->ui32DMAChannel;
	
    // provide information about the device
    *piDevRank = 1;
    *psPwrClass = QIO_POWERNONE;

	apsDeviceParam[0] = &(psInitParams->sDev0Param);
	apsDeviceParam[1] = &(psInitParams->sDev1Param);
	apsDeviceParam[2] = &(psInitParams->sDev2Param);

	for ( n = 0; n < SPIM_NUM_PORTS_PER_BLOCK; n++ )
	{
		//set up values to be written to each device parameter register
		aui32DeviceParamConfig[n] =  ( ( ( apsDeviceParam[n]->ui8BitRate << SPIM_CLK_DIVIDE_SHIFT ) & SPIM_CLK_DIVIDE_MASK ) |
									   ( ( apsDeviceParam[n]->ui8CSSetup << SPIM_CS_SETUP_SHIFT ) & SPIM_CS_SETUP_MASK ) |
									   ( ( apsDeviceParam[n]->ui8CSHold << SPIM_CS_HOLD_SHIFT ) & SPIM_CS_HOLD_MASK ) |
									   ( ( apsDeviceParam[n]->ui8CSDelay << SPIM_CS_DELAY_SHIFT ) & SPIM_CS_DELAY_MASK ) );

		//data for control and status register
		aui8DeviceControl[n] = ( ( apsDeviceParam[n]->ui32CSIdleLevel << SPIM_CS0_IDLE_SHIFT ) & SPIM_CS0_IDLE_MASK );

        aui8DeviceControl[n] |= ( ( apsDeviceParam[n]->ui32DataIdleLevel << SPIM_DATA0_IDLE_SHIFT ) & SPIM_DATA0_IDLE_MASK );

		switch ( apsDeviceParam[n]->eSPIMode )
		{
			case SPIM_MODE_0:
			{
				break;
			}
			case SPIM_MODE_1:
			{
				aui8DeviceControl[n] |= ( ( 1 << SPIM_CLOCK0_PHASE_SHIFT ) & SPIM_CLOCK0_PHASE_MASK );
				break;
			}
			case SPIM_MODE_2:
			{
				aui8DeviceControl[n] |= ( ( 1 << SPIM_CLOCK0_IDLE_SHIFT ) & SPIM_CLOCK0_IDLE_MASK );
				break;
			}
			case SPIM_MODE_3:
			{
				aui8DeviceControl[n] |= ( ( ( 1 << SPIM_CLOCK0_IDLE_SHIFT ) & SPIM_CLOCK0_IDLE_MASK ) |
									      ( ( 1 << SPIM_CLOCK0_PHASE_SHIFT ) & SPIM_CLOCK0_PHASE_MASK ) );
				break;
			}
		}
	}

	// set spiControl variable that will be written to the Control and Status register
	ui32SPIControl = ( aui8DeviceControl[0] << SPIM_CS0_IDLE_SHIFT ) | 
					 ( aui8DeviceControl[1] << SPIM_CS1_IDLE_SHIFT ) | 
					 ( aui8DeviceControl[2] << SPIM_CS2_IDLE_SHIFT );
	
	// Enable Edge of TX and RX - Data received and transmitted on the same edge of SPI clock
	// (provides improved operation frequency) according to TRM.
	ui32SPIControl |= ( 1 << SPIM_EDGE_TX_RX_SHIFT ); 

    // initialise the SPI hardware
    WRITE_REG( psBlockDesc->ui32Base, SPIM_DEVICE_0_PARAM_REG, aui32DeviceParamConfig[0] );
    WRITE_REG( psBlockDesc->ui32Base, SPIM_DEVICE_1_PARAM_REG, aui32DeviceParamConfig[1] );
    WRITE_REG( psBlockDesc->ui32Base, SPIM_DEVICE_2_PARAM_REG, aui32DeviceParamConfig[2] );
    WRITE_REG( psBlockDesc->ui32Base, SPIM_CONTROL_STATUS_REG, ui32SPIControl );

	if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind )
	{
		if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind( psBlockDesc->ui32DMACChannelSelectWriteValue, psInitParams->ui32DMAChannel ) != IMG_SUCCESS )
		{
			/* Bind failed */
			IMG_ASSERT (0);	
		}
	}
    
	IOBLOCK_CalculateInterruptInformation( psBlockDesc );

	IMG_MEMCPY( uIntMasks, psBlockDesc->ui32IntMasks, sizeof( unsigned long ) * QIO_MAXVECGROUPS );

    //read-modify-write to configure level sensitive DMA interrupts
	TBI_LOCK( iLockState );

	// HWLEVELEXT for SPIM
	ui32Reg = READ( psBlockDesc->sDeviceISRInfo.ui32LEVELEXTAddress );
	if ( psBlockDesc->eInterruptLevelType == HWLEVELEXT_LATCHED )
	{
		ui32Reg &= ~( psBlockDesc->sDeviceISRInfo.ui32LEVELEXTMask );
	}
	else if ( psBlockDesc->eInterruptLevelType == HWLEVELEXT_NON_LATCHED )
	{
		ui32Reg |= psBlockDesc->sDeviceISRInfo.ui32LEVELEXTMask;
	}
	WRITE( psBlockDesc->sDeviceISRInfo.ui32LEVELEXTAddress, ui32Reg );
	
	TBI_UNLOCK( iLockState );

	// Reassign ID for future calls
	psDev->id = psInitParams->ui32BlockIndex;

    return 0;
}

/*!
******************************************************************************

 @Function              SpimDrvReadWrite

 @Description

 Called by the start or isr function to carry out a read or write operation

 @Input     psIo			: Pointer to an SPI data structure that
							  describes the read/write to be performed.

 @Input     data			: Updated with data on a read.

 @Return    void


******************************************************************************/
static IMG_VOID SpimDrvReadWrite(	ioblock_sBlockDescriptor	*	psBlockDesc,
									SPIM_sBuffer				*	psIO,
									GDMA_sTransferObject		*	psTransfer)
{
	SPIM_sBlock				*	psContext = (SPIM_sBlock *)psBlockDesc->pvAPIContext;
	SPIM_sTransReg				sTransReg = { 0 };
    IMG_UINT32				*	pui32D;


    sTransReg.bfCount     = psIO->ui32Size;
    sTransReg.bfDevice    = psIO->eChipSelect;
    sTransReg.bfSendDma   = 0;
    sTransReg.bfGetDma    = 0;
    sTransReg.bfSoftReset = 0;
	sTransReg.bfCont      = psIO->i32Cont;
    sTransReg.bfCSDeass   = 0;
    sTransReg.bfByteDelay = psIO->i32InterByteDelay;

    pui32D = (IMG_UINT32 *)&sTransReg;
	
	// Set up common DMA parameters
	psTransfer->ui32SizeInBytes		= psIO->ui32Size;
	psTransfer->ui8Priority			= 0;
	psTransfer->bReverseEndianism	= IMG_FALSE;
	psTransfer->ui32BurstSize		= 4;
	psTransfer->pvUserData			= (img_void *)psIO;

	// Configure the read/write
	if ( psIO->i32Read )
	// Read MISO message from slave
	{
		sTransReg.bfGetDma = 1;

		// Rebind the SPI master to the desired DMA channel
		if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind )
		{
			if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind( psBlockDesc->ui32DMACChannelSelectWriteValue, psContext->ui32DMAChannel ) != IMG_SUCCESS )
			{
				/* Bind failed */
				IMG_ASSERT (0);	
			}
		}
		
		if ( psIO->i32CmpData )
		{
			IMG_UINT32 ui32Value;
			//This is a 'compare data read'

			//set up compare data control register
			ui32Value = READ_REG( psBlockDesc->ui32Base,  SPIM_COMPARE_DATA_REG );
			ui32Value |= ( ( 1 << CP_DAT_EN_SHIFT ) & CP_DAT_EN_MASK ); 				//Enable compare data logic
			ui32Value |= ( ( psIO->ui8CmpValue << CP_DATA_SHIFT ) & CP_DATA_MASK ); 	//Set up ui32Value to compare with incoming data
			ui32Value |= ( ( psIO->ui8CmpMask << CP_MASK_SHIFT ) & CP_MASK_MASK ); 		//Set up mask for CP_DATA compare
			ui32Value |= ( ( psIO->iCmpEq << CP_DAT_EQ_SHIFT ) & CP_DAT_EQ_MASK );		//Set up CP_DAT_EQ bit
            WRITE_REG( psBlockDesc->ui32Base, SPIM_COMPARE_DATA_REG, ui32Value );
		}
		else
		{
			// This is a standard read

			// Set up DMA
			psTransfer->pui8ReadPointer			= (img_uint8 *)(psBlockDesc->ui32Base + SPIM_GET_DATA_REG_OFFSET);
			psTransfer->bIncReadPointer			= IMG_FALSE;
			psTransfer->ui32ReadWidth			= 1;
			psTransfer->pui8WritePointer		= psIO->pui8Buffer;
			psTransfer->bIncWritePointer		= IMG_TRUE;
			psTransfer->ui32WriteWidth			= 4;
		
			GDMA_SingleShotOperation( &psContext->sDMAContext, psTransfer, 0 );
		}

		// enable SPI GDTRIG Interrupt. This is asserted when a receive has completed.
        WRITE_REG( psBlockDesc->ui32Base, SPIM_DMA_WRITE_INT_ENABLE, SPIM_GDTRIG_MASK );

		// start port activity
        WRITE_REG( psBlockDesc->ui32Base, SPIM_TRANSACTION_REG, *pui32D );
	}
	else
	//Write MOSI message to slave
	{
		sTransReg.bfSendDma = 1;

		if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind )
		{
			if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind( psBlockDesc->ui32DMACChannelSelectReadValue, psContext->ui32DMAChannel ) != IMG_SUCCESS )
			{
				/* Bind failed */
				IMG_ASSERT (0);	
			}
		}

		// Set up DMA
		psTransfer->pui8ReadPointer		= psIO->pui8Buffer;
		psTransfer->bIncReadPointer		= IMG_TRUE;
		psTransfer->ui32ReadWidth		= 4;
		psTransfer->pui8WritePointer	= (img_uint8 *)(psBlockDesc->ui32Base + SPIM_SEND_DATA_REG_OFFSET);
		psTransfer->bIncWritePointer	= IMG_FALSE;
		psTransfer->ui32WriteWidth		= 1;

		GDMA_SingleShotOperation( &psContext->sDMAContext, psTransfer, 0 );

		//enable SPI SDTRIG Interrupt. This is asserted when all data in the DMA transfer has
		//been transmitted to the slave.
        WRITE_REG( psBlockDesc->ui32Base, SPIM_DMA_READ_INT_ENABLE, SPIM_SDTRIG_MASK );

		//start port activity
        WRITE_REG( psBlockDesc->ui32Base, SPIM_TRANSACTION_REG, *pui32D );
	}
}


/*!
******************************************************************************

 @Function              DriverStart

 @Description	Device operation entry point. This function is called when a
				new operation is de-queued.

 @Input		psDev			: Pointer to the QIO device object.
 			psIOPars		: Pointer to the IO parameters.

 @Output	None

 @Return	void

******************************************************************************/
static IMG_VOID DriverStart(	QIO_DEVICE_T	*	psDev, 
								QIO_IOPARS_T	*	psIOPars )
{
    IMG_UINT32						ui32Opcode	= psIOPars->opcode;
	ioblock_sBlockDescriptor	*	psBlockDesc = g_apsSPIMBlock[ psDev->id ];
	SPIM_sBlock					*	psContext	= (SPIM_sBlock *)psBlockDesc->pvAPIContext;

	psContext->bSPIMInterruptHandled = IMG_FALSE;

    /* switch on the opcode */
    switch ( ui32Opcode )
    {
        /* read/write */
        case SPIM_READWRITE:
		{
            if ( psIOPars->counter == 1)
            {
                psContext->ui32State = SECOND;
            }
            else
            {
                psContext->ui32State = FIRST;
            }
            
            psContext->psIOPars = psIOPars;

            SpimDrvReadWrite( psBlockDesc, psIOPars->pointer, &psContext->sDMATransfer[0] );
            break;
		}
        /* unsupported opcode */
        default:
		{
            IMG_ASSERT( 0 );
            break;
		}
    }
}

/*!
******************************************************************************

 @Function              DriverISR

 @Description	Interrupt service routine

 @Input		dev		: Pointer to the QIO device object.

 @Output	None

 @Return	void

******************************************************************************/
static IMG_VOID DriverISR( QIO_DEVICE_T * psDev )
{
    IMG_UINT32						ui32SPITrigger;
    IMG_UINT32						ui32Value;
    ioblock_sBlockDescriptor	*	psBlockDesc		= g_apsSPIMBlock[ psDev->id ];
	SPIM_sBlock					*	psContext		= (SPIM_sBlock *)psBlockDesc->pvAPIContext;

	// Read statext register
	ui32SPITrigger = READ( psBlockDesc->sDeviceISRInfo.ui32STATEXTAddress );
	if ( !( ui32SPITrigger & psBlockDesc->sDeviceISRInfo.ui32STATEXTMask ) )
	{
		// Spurious interrupt ?
		IMG_ASSERT(0);
		return;
	}

	/* This could be a compare data operation interrupt */
	/* Check CP_DAT_DETECT bit in Compare Data Control register to see if this is a normal Receive or a Compare Data Receive. */
	ui32Value = READ_REG( psBlockDesc->ui32Base,  SPIM_COMPARE_DATA_REG );
	if ( ( ui32Value & CP_DAT_DETECT_MASK ) == CP_DAT_DETECT_MASK )
	{
		/* This is a compare data Receive */
		/* Clear CP_DAT_EN bit in compare data control register */
		ui32Value = READ_REG( psBlockDesc->ui32Base,  SPIM_COMPARE_DATA_REG );
		ui32Value |= ( ( 0 << CP_DAT_EN_SHIFT ) & CP_DAT_EN_MASK ); //Disable compare data logic
        WRITE_REG( psBlockDesc->ui32Base, SPIM_COMPARE_DATA_REG, ui32Value );
		//clear cp_dat_detect bit and disable compare data
        WRITE_REG( psBlockDesc->ui32Base, SPIM_COMPARE_DATA_REG, CP_DET_CLR_MASK );

		// Nothing more to do
		return;
	}
	
	// Check if this is a Perip->Mem interrupt
	ui32Value = READ_REG( psBlockDesc->ui32Base, SPIM_DMA_WRITE_INT_STATUS );
	if ( ( ui32Value & SPIM_GDTRIG_MASK ) == SPIM_GDTRIG_MASK )
	{
		// Perip->Mem interrupt: The registered DMA complete function calls QIO_complete.
		// We just clear the SPI interrupt
		WRITE_REG( psBlockDesc->ui32Base, SPIM_DMA_WRITE_INT_CLEAR, ui32Value );

		return;
	}
	
	// Check if this is a Mem->Perip interrupt
	ui32Value = READ_REG( psBlockDesc->ui32Base, SPIM_DMA_READ_INT_STATUS );
	if ( ( ui32Value & SPIM_SDTRIG_MASK ) == 1 )
	
    {
		// Mem->Perip interrupt: Clear SPI interrupts and start new second phase or complete

		// Clear interrupt status register
		WRITE_REG( psBlockDesc->ui32Base, SPIM_DMA_READ_INT_CLEAR, ui32Value );
		psContext->bSPIMInterruptHandled = IMG_TRUE;
		switch ( psContext->ui32State )
		{
			case FIRST:
			{
				// Check if the DMA has completed - otherwise the DMA ISR func still needs to be called, and it can 
				// Kick off the new xfer
				GDMA_eState eState;
				GDMA_GetStatus( &psContext->sDMAContext, &eState );
				if ( eState == GDMA_IDLE )
				{
					// We will let the SpimDmaComplete function kick off the new xfer
					SpimDrvReadWrite( psBlockDesc, (SPIM_sBuffer *)psContext->psIOPars->spare, &psContext->sDMATransfer[1] );
					psContext->ui32State = SECOND;
				}
				break;
			}
			case SECOND:
			{
				psContext->ui32State = IDLE;
				QIO_complete( psDev, QIO_IOCOMPLETE );
				QIO_start( psDev );
				break;
			}
		}

		return;
    }

	// Should never get here
	IMG_ASSERT(0);

	return;
}

IMG_RESULT	SpimDmaComplete(	GDMA_sTransferObject	*	psTransferObject, QIO_STATUS_T	eQIOStatus, img_void	*	pvUserContext )
{
	ioblock_sBlockDescriptor	*	psBlockDesc = (ioblock_sBlockDescriptor	*)pvUserContext;
	SPIM_sBlock					*	psContext = (SPIM_sBlock *)psBlockDesc->pvAPIContext;
	SPIM_sBuffer				*	psSPIMIO = (SPIM_sBuffer *)psTransferObject->pvUserData;

	// Check if we are idle
	if ( psContext->ui32State == IDLE )
	{
		// Just return as we have handled the completion in DriverISR()
		return IMG_SUCCESS;
	}
	
	// Check that we have something
	IMG_ASSERT( psSPIMIO );

	// Check which direction this transaction is in
	if ( psSPIMIO->i32Read )
	{
		// Perip->Mem

		// Flush the cache if needed
		if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
		{
			psBlockDesc->psSystemDescriptor->pfn_FlushCache( (img_uint32)psSPIMIO->pui8Buffer, psSPIMIO->ui32Size );
		}

		// Kick off another SPIM transaction if needed
		switch ( psContext->ui32State )
		{
			case FIRST:
			{	
				SpimDrvReadWrite( psBlockDesc, (SPIM_sBuffer *)psContext->psIOPars->spare, &psContext->sDMATransfer[1] );
				psContext->ui32State = SECOND;
				break;
			}
			case SECOND:
			{
				psContext->ui32State = IDLE;
				QIO_complete( &psContext->sDevice, QIO_IOCOMPLETE );
				QIO_start( &psContext->sDevice );
				break;
			}
		}
	}
	else
	{
		// Mem->Perip
		// Kick off another SPIM transaction iff the SPI ISR has already been called, and has delayed kickoff to now.
		if ( ( psContext->ui32State == FIRST ) &&
			 ( psContext->bSPIMInterruptHandled ) )
		{
			SpimDrvReadWrite( psBlockDesc, (SPIM_sBuffer *)psContext->psIOPars->spare, &psContext->sDMATransfer[1] );
			psContext->ui32State = SECOND;
		}		
	}
	
	return IMG_SUCCESS;
}

/******************************************************************************************************************************
**  QIO Free driver																											 **
******************************************************************************************************************************/
#if !defined (BOOT_CODE)
img_void BasicSPIMInit( SPIM_sBlock	*	psBlock, SPIM_sInitParam	*	psInitParams )
{
	ioblock_sBlockDescriptor	*	psBlockDesc = g_apsSPIMBlock[ psInitParams->ui32BlockIndex ];
	IMG_UINT32						aui32DeviceParamConfig	[ SPIM_NUM_PORTS_PER_BLOCK ];
    IMG_UINT8						aui8DeviceControl		[ SPIM_NUM_PORTS_PER_BLOCK ];
    SPIM_sDeviceParam			*	apsDeviceParam			[ SPIM_NUM_PORTS_PER_BLOCK ];	
	img_uint32						n;
	img_uint32						ui32SPIControl;
	img_uint32						ui32Reg;
	img_int32						iLockState;

	psBlock->bBypassQIO = IMG_TRUE;

	apsDeviceParam[0] = &(psInitParams->sDev0Param);
	apsDeviceParam[1] = &(psInitParams->sDev1Param);
	apsDeviceParam[2] = &(psInitParams->sDev2Param);

	for ( n = 0; n < SPIM_NUM_PORTS_PER_BLOCK; n++ )
	{
		//set up values to be written to each device parameter register
		aui32DeviceParamConfig[n] =  ( ( ( apsDeviceParam[n]->ui8BitRate << SPIM_CLK_DIVIDE_SHIFT ) & SPIM_CLK_DIVIDE_MASK ) |
									   ( ( apsDeviceParam[n]->ui8CSSetup << SPIM_CS_SETUP_SHIFT ) & SPIM_CS_SETUP_MASK ) |
									   ( ( apsDeviceParam[n]->ui8CSHold << SPIM_CS_HOLD_SHIFT ) & SPIM_CS_HOLD_MASK ) |
									   ( ( apsDeviceParam[n]->ui8CSDelay << SPIM_CS_DELAY_SHIFT ) & SPIM_CS_DELAY_MASK ) );

		//data for control and status register
		aui8DeviceControl[n] = ( ( apsDeviceParam[n]->ui32CSIdleLevel << SPIM_CS0_IDLE_SHIFT ) & SPIM_CS0_IDLE_MASK );

        aui8DeviceControl[n] |= ( ( apsDeviceParam[n]->ui32DataIdleLevel << SPIM_DATA0_IDLE_SHIFT ) & SPIM_DATA0_IDLE_MASK );

		switch ( apsDeviceParam[n]->eSPIMode )
		{
			case SPIM_MODE_0:
			{
				break;
			}
			case SPIM_MODE_1:
			{
				aui8DeviceControl[n] |= ( ( 1 << SPIM_CLOCK0_PHASE_SHIFT ) & SPIM_CLOCK0_PHASE_MASK );
				break;
			}
			case SPIM_MODE_2:
			{
				aui8DeviceControl[n] |= ( ( 1 << SPIM_CLOCK0_IDLE_SHIFT ) & SPIM_CLOCK0_IDLE_MASK );
				break;
			}
			case SPIM_MODE_3:
			{
				aui8DeviceControl[n] |= ( ( ( 1 << SPIM_CLOCK0_IDLE_SHIFT ) & SPIM_CLOCK0_IDLE_MASK ) |
									      ( ( 1 << SPIM_CLOCK0_PHASE_SHIFT ) & SPIM_CLOCK0_PHASE_MASK ) );
				break;
			}
		}
	}

	// set spiControl variable that will be written to the Control and Status register
	ui32SPIControl = ( aui8DeviceControl[0] << SPIM_CS0_IDLE_SHIFT ) | 
					 ( aui8DeviceControl[1] << SPIM_CS1_IDLE_SHIFT ) | 
					 ( aui8DeviceControl[2] << SPIM_CS2_IDLE_SHIFT );

    // initialise the SPI hardware
    WRITE_REG( psBlockDesc->ui32Base, SPIM_DEVICE_0_PARAM_REG, aui32DeviceParamConfig[0] );
    WRITE_REG( psBlockDesc->ui32Base, SPIM_DEVICE_1_PARAM_REG, aui32DeviceParamConfig[1] );
    WRITE_REG( psBlockDesc->ui32Base, SPIM_DEVICE_2_PARAM_REG, aui32DeviceParamConfig[2] );
    WRITE_REG( psBlockDesc->ui32Base, SPIM_CONTROL_STATUS_REG, ui32SPIControl );

	IOBLOCK_CalculateInterruptInformation( psBlockDesc );

	//read-modify-write to configure level sensitive DMA interrupts
	TBI_LOCK( iLockState );

	// HWLEVELEXT for SPIM
	ui32Reg = READ( psBlockDesc->sDeviceISRInfo.ui32LEVELEXTAddress );
	if ( psBlockDesc->eInterruptLevelType == HWLEVELEXT_LATCHED )
	{
		ui32Reg &= ~( psBlockDesc->sDeviceISRInfo.ui32LEVELEXTMask );
	}
	else if ( psBlockDesc->eInterruptLevelType == HWLEVELEXT_NON_LATCHED )
	{
		ui32Reg |= psBlockDesc->sDeviceISRInfo.ui32LEVELEXTMask;
	}
	WRITE( psBlockDesc->sDeviceISRInfo.ui32LEVELEXTAddress, ui32Reg );
	
	TBI_UNLOCK( iLockState );

	// Assign block index
	psBlock->sDevice.id = psInitParams->ui32BlockIndex;
}

//#define DEBUG_DMALESS // Enable to capture some debug info

#if defined (DEBUG_DMALESS)
#define DEBUG_SIZE	(2048)
img_uint32	g_aui32Debug[DEBUG_SIZE];
img_uint32	g_ui32DebugCnt = 0;
#endif

img_void BasicSPIMReadWrite( SPIM_sBlock	*	psBlock, SPIM_sBuffer	*	psIO )
{
	ioblock_sBlockDescriptor	*	psBlockDesc = g_apsSPIMBlock[ psBlock->sDevice.id ];
	SPIM_sTransReg					sTransReg = { 0 };
	img_uint32					*	pui32D;
	img_uint32						ui32NumBytes;
	volatile img_uint8			*	pui8DataReg;
	//volatile img_uint32			*	pui32DataReg;
	img_uint8					*	pui8MemReg = psIO->pui8Buffer;
	img_uint32						ui32FifoStatus;
	img_uint32						ui32Reg;

	sTransReg.bfCount     = psIO->ui32Size;
    sTransReg.bfDevice    = psIO->eChipSelect;
    sTransReg.bfSendDma   = 0;
    sTransReg.bfGetDma    = 0;
    sTransReg.bfSoftReset = 0;
	sTransReg.bfCont      = psIO->i32Cont;
    sTransReg.bfCSDeass   = 0;
    sTransReg.bfByteDelay = psIO->i32InterByteDelay;

    pui32D = (IMG_UINT32 *)&sTransReg;

	ui32NumBytes = psIO->ui32Size;

	if ( psIO->i32Read )
	{
		// Read message from slave
		sTransReg.bfGetDma = 1;

		IMG_ASSERT( !psIO->i32CmpData );
		
		pui8DataReg = (volatile img_uint8 *)(psBlockDesc->ui32Base + SPIM_GET_DATA_REG_OFFSET);
		//pui32DataReg = (volatile img_uint32 *)(psBlockDesc->ui32Base + SPIM_GET_DATA_REG_OFFSET);

		// Enable SPI GDTRIG interrupt, which is asserted when the receive has completed.
		WRITE_REG( psBlockDesc->ui32Base, SPIM_DMA_WRITE_INT_ENABLE, SPIM_GDTRIG_MASK );

		// Start the transaction
		WRITE_REG( psBlockDesc->ui32Base, SPIM_TRANSACTION_REG, *pui32D );

		// Read data from the FIFO, pausing if its empty, until we've read everything
		while ( ui32NumBytes )
		{
			// Wait untl the FIFO is not empty
			do
			{
				ui32FifoStatus = READ_REG( psBlockDesc->ui32Base, SPIM_DMA_WRITE_INT_STATUS );
			} while ( READ_REG_FIELD( ui32FifoStatus, SPIM_GDEX ) == 0 );

			// Read a byte
			*pui8MemReg++ = *pui8DataReg;
			//*pui8MemReg++ = (img_uint8)(*pui32DataReg & 0xFF);
			ui32NumBytes--;
			
			// Clear the FIFO not empty bit
			ui32Reg = WRITE_REG_FIELD( 0, SPIM_GDEX, 1 );
			WRITE_REG( psBlockDesc->ui32Base, SPIM_DMA_WRITE_INT_CLEAR, ui32Reg );
		}
		
		// We should have all the data, check & clear interrupts
		ui32Reg = READ_REG( psBlockDesc->ui32Base, SPIM_DMA_WRITE_INT_STATUS );
		if ( READ_REG_FIELD( ui32Reg, SPIM_GDTRIG ) )
		{
			WRITE_REG( psBlockDesc->ui32Base, SPIM_DMA_WRITE_INT_CLEAR, ui32Reg );
		}
		else
		{
			IMG_ASSERT( 0 );
		}

	}
	else
	{
		// Write message to slave
		sTransReg.bfSendDma = 1;

		pui8DataReg = (volatile img_uint8 *)(psBlockDesc->ui32Base + SPIM_SEND_DATA_REG_OFFSET);
		//pui32DataReg = (volatile img_uint32 *)(psBlockDesc->ui32Base + SPIM_SEND_DATA_REG_OFFSET);

		// Enable SPI SDTRIG Interrupt, which is asserted when all data has been transmitted by the slave
		WRITE_REG( psBlockDesc->ui32Base, SPIM_DMA_READ_INT_ENABLE, SPIM_SDTRIG_MASK );

		// Write data to the FIFO until it's full or we've finished writing everything
		do
		{
			// Write a byte
			*pui8DataReg = *pui8MemReg++;
			//*pui32DataReg = (img_uint32)*pui8MemReg++;
			ui32NumBytes--;
			ui32FifoStatus = READ_REG( psBlockDesc->ui32Base, SPIM_DMA_READ_INT_STATUS );
		} while ( ( !READ_REG_FIELD( ui32FifoStatus, SPIM_SDFUL ) ) && ui32NumBytes );

#if defined (DEBUG_DMALESS)
		g_ui32DebugCnt = 0;
		IMG_MEMSET( g_aui32Debug, 0, sizeof(img_uint32) * DEBUG_SIZE );
		g_aui32Debug[g_ui32DebugCnt++] = ui32NumBytes;
#endif /* DEBUG_DMALESS */
		// Enable the transaction
		WRITE_REG( psBlockDesc->ui32Base, SPIM_TRANSACTION_REG, *pui32D );
		
#if defined (DEBUG_DMALESS)
		g_aui32Debug[g_ui32DebugCnt++] = (1 << 29) | READ_REG( psBlockDesc->ui32Base, SPIM_DMA_READ_INT_STATUS );
#endif /* DEBUG_DMALESS */

		// If we have bytes left to transfer, write them to the FIFO, whilst not overflowing the FIFO
		while ( ui32NumBytes )
		{
			ui32FifoStatus = READ_REG( psBlockDesc->ui32Base, SPIM_DMA_READ_INT_STATUS );
			if ( READ_REG_FIELD( ui32FifoStatus, SPIM_SDFUL ) )
			{
#if defined (DEBUG_DMALESS)
				g_aui32Debug[g_ui32DebugCnt++] = ui32NumBytes;
#endif /* DEBUG_DMALESS */
				// FIFO is full, clear the status
				ui32Reg = WRITE_REG_FIELD( 0, SPIM_SDFUL, 1 );
				WRITE_REG( psBlockDesc->ui32Base, SPIM_DMA_READ_INT_CLEAR, ui32Reg );
			}
			else
			{
				// There is space in the FIFO, so write a byte
				*pui8DataReg = *pui8MemReg++;
				//*pui32DataReg = (img_uint32)*pui8MemReg++;
				ui32NumBytes--;
			}
#if defined (DEBUG_DMALESS)
			g_aui32Debug[g_ui32DebugCnt++] = (1 << 31) | READ_REG( psBlockDesc->ui32Base, SPIM_DMA_READ_INT_STATUS );
#endif /* DEBUG_DMALESS */
		}

		// Wait for completion interrupt
		do
		{
			ui32Reg = READ_REG( psBlockDesc->ui32Base, SPIM_DMA_READ_INT_STATUS );
#if defined (DEBUG_DMALESS)
			g_aui32Debug[g_ui32DebugCnt++] = (1 << 30) | ui32Reg;
#endif
		} while ( !READ_REG_FIELD( ui32Reg, SPIM_SDTRIG ) );

		// Clear the interrupts
		WRITE_REG( psBlockDesc->ui32Base, SPIM_DMA_READ_INT_CLEAR, ui32Reg );
	}
}

img_void BasicSPIMIO( SPIM_sBlock	*	psBlock, QIO_IOPARS_T	*	psIOPars )
{
	// Check the opcode is fine - should always be
	IMG_ASSERT( psIOPars->opcode == SPIM_READWRITE );

	// Perform first transaction
	BasicSPIMReadWrite( psBlock, (SPIM_sBuffer *)psIOPars->pointer );

	// Perform second transaction if there is one
	if ( psIOPars->counter != 1 )
	{
		BasicSPIMReadWrite( psBlock, (SPIM_sBuffer *)psIOPars->spare );
	}
}

img_void BasicSPIMDMAInit( SPIM_sBlock *	psBlock, SPIM_sInitParam	*	psInitParam )
{
	// Initialise basic SPIM
	BasicSPIMInit( psBlock, psInitParam );

	psBlock->bBypassDMA = psInitParam->bBypassDMA;

	IMG_MEMSET( &psBlock->sDMAContext, 0, sizeof( GDMA_sContext ) );
	psBlock->sDMAContext.ui32Channel = psInitParam->ui32DMAChannel;
	IMG_asGDMABlock[ psInitParam->ui32DMAChannel ].pvAPIContext = &psBlock->sDMAContext;
	GDMA_HAL_ResetHardware( &IMG_asGDMABlock[ psInitParam->ui32DMAChannel ] );
}

img_void BasicSPIMDMAReadWrite( SPIM_sBlock	*	psBlock, SPIM_sBuffer	*	psIO )
{
	ioblock_sBlockDescriptor	*	psBlockDesc = g_apsSPIMBlock[ psBlock->sDevice.id ];
	SPIM_sBlock					*	psContext = (SPIM_sBlock *)psBlockDesc->pvAPIContext;
	SPIM_sTransReg					sTransReg = { 0 };
	img_uint32					*	pui32D;
	GDMA_sTransferObject			sDMATransfer;
	img_bool						bDMAInterrupt;
	img_uint32						ui32Reg;
	
	sTransReg.bfCount		= psIO->ui32Size;
	sTransReg.bfDevice		= psIO->eChipSelect;
	sTransReg.bfCont		= psIO->i32Cont;
	sTransReg.bfByteDelay	= psIO->i32InterByteDelay;

	pui32D = (img_uint32 *)&sTransReg;

	// Set up common DMA parameters
	IMG_MEMSET( &sDMATransfer, 0, sizeof( GDMA_sTransferObject ) );
	sDMATransfer.ui32SizeInBytes	= psIO->ui32Size;
	sDMATransfer.ui32BurstSize		= 4;

	if ( psIO->i32Read )
	{
		// Read MISO message from slave
		sTransReg.bfGetDma = 1;
		
		// Rebind the SPI master to the desired DMA channel
		if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind )
		{
			if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind( psBlockDesc->ui32DMACChannelSelectWriteValue, psContext->ui32DMAChannel ) != IMG_SUCCESS )
			{
				/* Bind failed */
				IMG_ASSERT (0);	
			}
		}

		sDMATransfer.pui8ReadPointer	= (img_uint8 *)(psBlockDesc->ui32Base + SPIM_GET_DATA_REG_OFFSET);
		sDMATransfer.bIncReadPointer	= IMG_FALSE;
		sDMATransfer.ui32ReadWidth		= 1;
		sDMATransfer.pui8WritePointer	= psIO->pui8Buffer;
		sDMATransfer.bIncWritePointer	= IMG_TRUE;
		sDMATransfer.ui32WriteWidth		= 4;

		GDMA_HAL_PrepareSingleShotTransfer( &IMG_asGDMABlock[ psContext->ui32DMAChannel ], &sDMATransfer );
		GDMA_HAL_EnableTransfer( &IMG_asGDMABlock[ psContext->ui32DMAChannel ], IMG_FALSE );

		// enable SPI GDTRIG Interrupt. This is asserted when a receive has completed.
        WRITE_REG( psBlockDesc->ui32Base, SPIM_DMA_WRITE_INT_ENABLE, SPIM_GDTRIG_MASK );

		// start port activity
        WRITE_REG( psBlockDesc->ui32Base, SPIM_TRANSACTION_REG, *pui32D );

		// Wait for DMA to finish
		do
		{
			GDMA_HAL_GetInterruptStatus( &IMG_asGDMABlock[ psContext->ui32DMAChannel ], &bDMAInterrupt );
		} while ( !bDMAInterrupt );

		GDMA_HAL_ReadAndClearInterrupts( &IMG_asGDMABlock[ psContext->ui32DMAChannel ] );

		// We should have all the data, check & clear interrupts
		ui32Reg = READ_REG( psBlockDesc->ui32Base, SPIM_DMA_WRITE_INT_STATUS );
		if ( READ_REG_FIELD( ui32Reg, SPIM_GDTRIG ) )
		{
			WRITE_REG( psBlockDesc->ui32Base, SPIM_DMA_WRITE_INT_CLEAR, ui32Reg );
		}
		else
		{
			IMG_ASSERT( 0 );
		}

		// FLush the cash
		if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
		{
			psBlockDesc->psSystemDescriptor->pfn_FlushCache( (img_uint32)psIO->pui8Buffer, psIO->ui32Size );
		}
	}
	else
	{
		// Write MOSI message to slave
		sTransReg.bfSendDma = 1;

		if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind )
		{
			if ( psBlockDesc->psSystemDescriptor->pfn_DMACChannelBind( psBlockDesc->ui32DMACChannelSelectReadValue, psContext->ui32DMAChannel ) != IMG_SUCCESS )
			{
				/* Bind failed */
				IMG_ASSERT (0);	
			}
		}

		// Set up DMA
		sDMATransfer.pui8ReadPointer	= psIO->pui8Buffer;
		sDMATransfer.bIncReadPointer	= IMG_TRUE;
		sDMATransfer.ui32ReadWidth		= 4;
		sDMATransfer.pui8WritePointer	= (img_uint8 *)(psBlockDesc->ui32Base + SPIM_SEND_DATA_REG_OFFSET);
		sDMATransfer.bIncWritePointer	= IMG_FALSE;
		sDMATransfer.ui32WriteWidth		= 1;

		GDMA_HAL_PrepareSingleShotTransfer( &IMG_asGDMABlock[ psContext->ui32DMAChannel ], &sDMATransfer );
		GDMA_HAL_EnableTransfer( &IMG_asGDMABlock[ psContext->ui32DMAChannel ], IMG_FALSE );

		//enable SPI SDTRIG Interrupt. This is asserted when all data in the DMA transfer has
		//been transmitted to the slave.
        WRITE_REG( psBlockDesc->ui32Base, SPIM_DMA_READ_INT_ENABLE, SPIM_SDTRIG_MASK );

		//start port activity
        WRITE_REG( psBlockDesc->ui32Base, SPIM_TRANSACTION_REG, *pui32D );

		// Wait for DMA to finish
		do
		{
			GDMA_HAL_GetInterruptStatus( &IMG_asGDMABlock[ psContext->ui32DMAChannel ], &bDMAInterrupt );
		} while ( !bDMAInterrupt );

		GDMA_HAL_ReadAndClearInterrupts( &IMG_asGDMABlock[ psContext->ui32DMAChannel ] );

		// Wait for completion interrupt
		do
		{
			ui32Reg = READ_REG( psBlockDesc->ui32Base, SPIM_DMA_READ_INT_STATUS );
			//g_aui32Debug[g_ui32DebugCnt++] = (1 << 30) | ui32Reg;
		} while ( !READ_REG_FIELD( ui32Reg, SPIM_SDTRIG ) );

		// Clear the interrupts
		WRITE_REG( psBlockDesc->ui32Base, SPIM_DMA_READ_INT_CLEAR, ui32Reg );
	}
}

img_void BasicSPIMDMA( SPIM_sBlock	*	psBlock, QIO_IOPARS_T	*	psIOPars )
{
	// Check the opcode is fine - should always be
	IMG_ASSERT( psIOPars->opcode == SPIM_READWRITE );

	// Perform first transaction
	BasicSPIMDMAReadWrite( psBlock, (SPIM_sBuffer *)psIOPars->pointer );

	// Perform second transaction if there is one
	if ( psIOPars->counter != 1 )
	{
		BasicSPIMDMAReadWrite( psBlock, (SPIM_sBuffer *)psIOPars->spare );
	}
}

#endif
