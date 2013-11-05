/*!
*******************************************************************************
  file   scbm_drv.c

  brief  Serial Control Bus Master device Driver using Automatic Mode

         Serial Control Bus Master (SCBM) device driver code.

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

#include <assert.h>

#include <img_defs.h>
#include <sys_util.h>

#include <ioblock_defs.h>
#include <ioblock_utils.h>

/* SCB Driver/API header files */
#include "scbm_api.h"
#include "scbm_drv.h"
#include "scb_reg_defs.h"

/*============================================================================*/
/*                                                                            */
/* I2C Timing Parameters (in ns)											  */
/*										                                      */
/*============================================================================*/
/* Need >= than the value */
#define TIMING_TCKH_STDMODE			(4000)
#define TIMING_TCKL_STDMODE			(4700)
#define TIMING_TSDH_STDMODE			(4700)
#define TIMING_TSDL_STDMODE			(8700)

/* Need <= than the value */
#define TIMING_TP2S_STDMODE			(4700)
#define TIMING_TPL_STDMODE			(4700)
#define TIMING_TPH_STDMODE			(4000)

#define TIMING_TBI_STDMODE			(0xFFFE)
#define TIMING_TSL_STDMODE			(0x120)
#define TIMING_TDL_STDMODE			(0x120)


/* Need >= than the value */
#define TIMING_TCKH_FASTMODE		(600)
#define TIMING_TCKL_FASTMODE		(1300)
#define TIMING_TSDH_FASTMODE		(600)
#define TIMING_TSDL_FASTMODE		(1200)

/* Need <= than the value */
#define TIMING_TP2S_FASTMODE		(1300)
#define TIMING_TPL_FASTMODE			(600)
#define TIMING_TPH_FASTMODE			(600)

#define TIMING_TBI_FASTMODE			(0xFFFE)
#define TIMING_TSL_FASTMODE       	(0x2000)
#define TIMING_TDL_FASTMODE			(0x120)

/*============================================================================*/
/*                                                                            */
/*	System level and MTX control register #defines							  */
/*                                                                            */
/*============================================================================*/


/*============================================================================*/
/*                                                                            */
/*                          MACRO DEFINITIONS	                              */
/*                                                                            */
/*============================================================================*/

#define LEVEL_TRIGGERED_INTERRUPTS

#define WRITE(a,d)    (*(volatile unsigned long *)(a) = (d))
#define READ(a)       (*(volatile unsigned long *)(a))
//#define TARGET_OSCAR

/*
	Changes to the bus inactive timeout (to extend the period) have introduced an
	as yet unexplained problem with Comet when switching between demod standards.
	The short-term fix to this problem is to not enable the interrupt associated
	with this event.
*/
#define ENABLE_BUS_INACTIVE_INTERRUPT		0

/*============================================================================*/
/*                                                                            */
/*                          DATA STRUCTURES		                              */
/*                                                                            */
/*============================================================================*/

/* device context */
typedef struct scbm_dc_t
{
	int					bInitialised;				/* indicates if the device has been initialised											*/
	int					bTransactionStarted;		/* indicates if the transaction has been started										*/
	int					bComplete;					/* indicates if the transaction has completed and that the QIO system has been notified	*/
    QIO_IOPARS_T		*pCurrent;      			/* current IO parameters (NULL for idle)												*/
    unsigned long		ui32NumBytesLeft;			/* number of bytes left to transfer														*/
    unsigned long		ui32NumBytesTransferred;	/* number of bytes transferred															*/
    unsigned long		ui32Method;					/* driver method - read or write														*/
    unsigned long		ui32Bitrate;				/* bitrate																				*/
	unsigned long		ui32CoreClockKHz;			/* clock frequency that drives the scb module											*/
	unsigned long		ui32BusDelayNS;				/* bus delay of the line, in nano seconds												*/
    unsigned long		ui32Address;				/* slave address																		*/
    unsigned char		*pucBuffer;					/* buffer where data is read from/written to											*/
	SCBM_eErrorStatus	eErrorStatus;				/* specifies the current error status of the device										*/
	int					bCancel;					/* indicates if a cancel has been issued												*/

} SCBM_DC_T;


/*============================================================================*/
/*                                                                            */
/*                          FUNCTION PROTOTYPES	                              */
/*                                                                            */
/*============================================================================*/

static int  DriverInit( QIO_DEVICE_T *pDevice, QIO_DEVPOWER_T *pPowerClass, int *pnDeviceRank, unsigned int ui32InterruptMasks[QIO_MAXVECGROUPS] );
static void DriverStart( QIO_DEVICE_T *pDevice, QIO_IOPARS_T *pIOPars );
static void DriverISR( QIO_DEVICE_T *pDevice );
static void DriverCancel( QIO_DEVICE_T *pDevice );

static void ScbmWriteFIFO( ioblock_sBlockDescriptor * psBlockDesc );
static void ScbmReadFIFO( QIO_DEVICE_T *pDevice, ioblock_sBlockDescriptor * psBlockDesc );
static void ScbmCompleteIO( QIO_DEVICE_T *pDevice, SCBM_DC_T *psDeviceContext, unsigned long ui32NumBytesTransferred );
static void ScbmTerminate( QIO_DEVICE_T *pDevice, SCBM_DC_T *psDeviceContext );

/*============================================================================*/
/*                                                                            */
/*                          GLOBAL DATA			                              */
/*                                                                            */
/*============================================================================*/

/* the driver object */
const QIO_DRIVER_T SCBM_driver =
{
    DriverISR,      /* ISR                       */
    DriverInit,     /* init function             */
    DriverStart,    /* start function            */
    DriverCancel,   /* cancel function           */
    IMG_NULL,		/* no power control function */
    IMG_NULL,       /* no sim start function     */
    IMG_NULL
};


/*============================================================================*/
/*                                                                            */
/*                          STATIC DATA			                              */
/*                                                                            */
/*============================================================================*/

// MAX_NUM_SCBM_BLOCKS defined in scbm_drv.h
ioblock_sBlockDescriptor	*	g_apsSCBMBlock[ MAX_NUM_SCBM_BLOCKS ] =
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

 @Function              ScbmWriteFIFO

 @Description	Performs a master write using the hardwares FIFO

 @Input		psDeviceContext				: The device's context.

 @Output	none

 @Return	void

******************************************************************************/
static void ScbmWriteFIFO( ioblock_sBlockDescriptor * psBlockDesc )
{
	unsigned int		bFIFOFull = 0;
	unsigned long		ui32Reg;
	SCBM_DC_T		*	psDeviceContext = (SCBM_DC_T *)psBlockDesc->pvAPIContext;

	/* Check if this is the start of a new write to this address */
	if (!psDeviceContext->bTransactionStarted)
	{
		/* Specify address */
		WRITE_REG(psBlockDesc->ui32Base, SCB_MASTER_WRITE_ADDRESS, psDeviceContext->ui32Address);
		/* Specify write count */
		WRITE_REG(psBlockDesc->ui32Base, SCB_MASTER_WRITE_COUNT, psDeviceContext->ui32NumBytesLeft);

		psDeviceContext->bTransactionStarted = IMG_TRUE;
	}

	/* Fill up FIFO */
	while (1)
	{
		/* Get FIFO status */
		ui32Reg = READ_REG(psBlockDesc->ui32Base, SCB_MASTER_FILL_STATUS);
		bFIFOFull = READ_REG_FIELD(ui32Reg, SCB_MASTER_FILL_STATUS_MWS_REG_FULL);
		if (bFIFOFull || psDeviceContext->ui32NumBytesLeft == 0)
		{
			if (psDeviceContext->ui32NumBytesLeft == 0)
			{
				/* Disable "Master write fifo almost empty" interrupt when we have no more data to be written */
				WRITE_REG(psBlockDesc->ui32Base, SCB_INTERRUPT_MASK, 0x00000E3E | ENABLE_BUS_INACTIVE_INTERRUPT );
			}
			else
			{
				/* Enable "Master write fifo almost empty" interrupt when we have more data to be written */
				WRITE_REG(psBlockDesc->ui32Base, SCB_INTERRUPT_MASK, 0x00001E3E | ENABLE_BUS_INACTIVE_INTERRUPT );
			}

			break;
		}

		/* Send data to FIFO */
		WRITE_REG(psBlockDesc->ui32Base, SCB_MASTER_WRITE_DATA, psDeviceContext->pucBuffer[psDeviceContext->ui32NumBytesTransferred]);
		WRITE_REG(psBlockDesc->ui32Base,CR_SCB_CORE_REV,0);
		WRITE_REG(psBlockDesc->ui32Base,CR_SCB_CORE_REV,0);
		psDeviceContext->ui32NumBytesTransferred++;
		psDeviceContext->ui32NumBytesLeft--;
	}
}

/*!
******************************************************************************

 @Function              ScbmReadFIFO

 @Description	Reads data from the hardware's FIFO

 @Input		psDeviceContext			: The device's context.

 @Output	none

 @Return	void

******************************************************************************/
static void ScbmReadFIFO(QIO_DEVICE_T *pDevice, ioblock_sBlockDescriptor	*	psBlockDesc )
{
	SCBM_DC_T		*	psDeviceContext = (SCBM_DC_T *)psBlockDesc->pvAPIContext;
	unsigned long		ui32Reg;
	unsigned char		ucData;

	if (!psDeviceContext->bTransactionStarted)
	{
		/* Write address */
		WRITE_REG(psBlockDesc->ui32Base, SCB_MASTER_READ_ADDRESS, psDeviceContext->ui32Address);
		/* Write read_count */
		WRITE_REG(psBlockDesc->ui32Base, SCB_MASTER_READ_COUNT, psDeviceContext->ui32NumBytesLeft);

		psDeviceContext->bTransactionStarted = IMG_TRUE;
	}
	else
	{
		while (1)
		{
			/* If the FIFO is empty or we don't have any more bytes to read, then exit the loop */
			ui32Reg = READ_REG(psBlockDesc->ui32Base, SCB_MASTER_FILL_STATUS);
			if ((psDeviceContext->ui32NumBytesLeft == 0) || ( READ_REG_FIELD(ui32Reg, SCB_MASTER_FILL_STATUS_MRS_REG_EMPTY) ))
			{
				break;
			}


			/* Read the data */
			ucData = READ_REG(psBlockDesc->ui32Base, SCB_MASTER_READ_DATA);

			/* Store this data in the application buffer */
			psDeviceContext->pucBuffer[psDeviceContext->ui32NumBytesTransferred] = ucData;

			psDeviceContext->ui32NumBytesTransferred++;
			psDeviceContext->ui32NumBytesLeft--;

			/* Advance the FIFO */
			WRITE_REG(psBlockDesc->ui32Base, SCB_MASTER_DATA_READ, 0xFF);
			WRITE_REG(psBlockDesc->ui32Base,CR_SCB_CORE_REV,0);
			WRITE_REG(psBlockDesc->ui32Base,CR_SCB_CORE_REV,0);
		}
	}
}

/*!
******************************************************************************

 @Function              ScbmCompleteIo

 @Description	Completes an IO.

 @Input		pDevice						: The QIO device to complete.
 			psDeviceContext				: The device's context.
 			ui32NumBytesTransferred		: Number of bytes transferred

 @Output	psDeviceContext				: The IO is completed.

 @Return	void

******************************************************************************/
static void ScbmCompleteIO(
    QIO_DEVICE_T  *pDevice,
    SCBM_DC_T     *psDeviceContext,
    unsigned long ui32NumBytesTransferred
    )
{
    /* Set the number of bytes transferred. */
    psDeviceContext->pCurrent->counter = ui32NumBytesTransferred;
	psDeviceContext->bComplete = IMG_TRUE;

	if ( !psDeviceContext->bCancel )
	{
		/* Complete the IO. */
		QIO_complete(pDevice, QIO_IOCOMPLETE);
		QIO_start(pDevice);
	}
}

/*!
******************************************************************************

 @Function              ScbmTerminate

 @Description	Terminates a transfer early.

 @Input		pDevice				: The QIO device.
 			psDeviceContext		: The device's context.

 @Output	psDeviceContext		: The IO is completed.

 @Return	void

******************************************************************************/
static void ScbmTerminate(QIO_DEVICE_T *pDevice, SCBM_DC_T *psDeviceContext)
{
	if (!psDeviceContext->bComplete)
	{
		psDeviceContext->pCurrent->counter = 0;
		psDeviceContext->bComplete = IMG_TRUE;

		/* Complete this IO. */
		QIO_complete(pDevice, QIO_IOCOMPLETE);
	}
}

/*!
******************************************************************************

 @Function              DriverCancel

 @Description	cancel a transfer early.

 @Input		pDevice				: The QIO device.
 			psDeviceContext		: The device's context.

 @Output	psDeviceContext		: The IO is completed.

 @Return	void

******************************************************************************/
static void DriverCancel(QIO_DEVICE_T *pDevice)
{
	SCBM_DC_T *psDeviceContext;

	/* Get the device context. */
	psDeviceContext = (SCBM_DC_T *)(g_apsSCBMBlock[pDevice->id]->pvAPIContext);

	/* Set the number of bytes transferred. */
	psDeviceContext->pCurrent->counter = psDeviceContext->ui32NumBytesTransferred;

	psDeviceContext->bCancel = IMG_TRUE;

	psDeviceContext->bComplete = IMG_TRUE;
}

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
static int DriverInit(QIO_DEVICE_T *pDevice, QIO_DEVPOWER_T *pPowerClass, int *pDeviceRank, unsigned int ui32InterruptMasks[QIO_MAXVECGROUPS])
{
	SCBM_DC_T					*	psDeviceContext;
	SCBM_SETTINGS_T				*	psSettings;
	unsigned long					ui32Reg;
	unsigned long					ui32Inc;
	unsigned long					ui32IntClkPeriod;
	unsigned long					ui32BitRateInIntClkPulses;
	unsigned long					ui32Tckh, ui32Tckl, ui32Tsdh;
	unsigned long					ui32ClkEnabFreq;

	int								iLockState;
	ioblock_sBlockDescriptor	*	psBlockDesc;

	/* Get API init settings */
	psSettings = (SCBM_SETTINGS_T *)pDevice->id;
	IMG_ASSERT( psSettings );

	// Check we have enough memory to "allocate" our structures
	IMG_ASSERT( sizeof( SCBM_DC_T ) <= INTERNAL_SCBM_MEM_SIZE );

	// Get context structure from defined block descriptor
	psBlockDesc		= g_apsSCBMBlock[ psSettings->ui32BlockIndex ];
	psDeviceContext = (SCBM_DC_T *)psBlockDesc->pvAPIContext;
	// Setup context structure
    psDeviceContext->ui32Bitrate		= psSettings->bitrate;
	psDeviceContext->ui32CoreClockKHz	= psSettings->coreclock;
	psDeviceContext->ui32BusDelayNS		= psSettings->busdelay;

	/* No power saving at the moment */
	*pPowerClass = QIO_POWERNONE;

	/* Single rank */
	*pDeviceRank = 1;

	/* Set up current IO parameters to NULL (idle) */
	psDeviceContext->pCurrent = IMG_NULL;

	/* Set the bCancel flag to FALSE */
	psDeviceContext->bCancel = IMG_FALSE;

	/* Set the complete flag to TRUE */
	psDeviceContext->bComplete = IMG_TRUE;

	if ( !psDeviceContext->bInitialised )
	{
		/*******************************************************************
		Setup bitrate
		******************************************************************/

		/* Set up pre-scale value of the clock to 7, ie divide by 8 */
		ui32Reg = WRITE_REG_FIELD( 0, SCB_CLK_SET_SCB_PRE_SCALE, 7 );

		/* Calculate clock enable (Fce) */
		ui32ClkEnabFreq = psDeviceContext->ui32CoreClockKHz / 8;

		/* Set up the increment value according to section 3.1.1 of the TRM */
		ui32Inc =	( 256 * 16 * psDeviceContext->ui32Bitrate )
									/
					(
						ui32ClkEnabFreq - ( 16 * psDeviceContext->ui32Bitrate * ( ui32ClkEnabFreq / 1000 ) * psDeviceContext->ui32BusDelayNS )
														/
													  10000
				    );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, SCB_CLK_SET_SCB_INCREMENT, ui32Inc );

		/* Obtain the clock perioud of the fx16 clock in ns */
		ui32IntClkPeriod = ( 256 * 1000000 ) / ( ui32ClkEnabFreq * ui32Inc ) + psDeviceContext->ui32BusDelayNS;

		if ( ui32ClkEnabFreq < 20000 )
		{
			// Disable the filter clock
			ui32Reg = WRITE_REG_FIELD( ui32Reg, SCB_CLK_SET_SCB_FILT_DISABLE, 1 );
		}
		else if ( ui32ClkEnabFreq < 40000 )
		{
			// Bypass the filter clock
			ui32Reg = WRITE_REG_FIELD( ui32Reg, SCB_CLK_SET_SCB_FILT_BYPASS, 1 );
		}
		else
		{
			// We must set up the filter clock

			/*	Set up the LP filter clock increment. We need to filter out any glitches with T < 50ns, but without
				going over 50ns (need to round the INC value up!)
			*/
			ui32Inc = ((640000) / ((ui32ClkEnabFreq / 1000) * (250 - psDeviceContext->ui32BusDelayNS)));
			if ( (640000) % (( ui32ClkEnabFreq / 1000) * (250 - psDeviceContext->ui32BusDelayNS)))
			{
				ui32Inc++;	// Scale up
			}
			if ( ui32Inc > 0x7f )
			{
				ui32Inc = 0x7f;
			}
			ui32Reg = WRITE_REG_FIELD( ui32Reg, SCB_CLK_SET_SCB_FILT_INCREMENT, ui32Inc );
		}

		/* And write the clock settings register */
		WRITE_REG( psBlockDesc->ui32Base, SCB_CLK_SET, ui32Reg );

		/*********************************************************************
							Other SCBM register initialisation
		**********************************************************************/

		/* Scb_slave_address_base = 0 (from VHDL test) */
		WRITE_REG( psBlockDesc->ui32Base, SCB_SLAVE_ADDRESS_BASE, 0 );

		/* Scb_slave_address_mask = 0 (from VHDL test) */
		WRITE_REG( psBlockDesc->ui32Base, SCB_SLAVE_ADDRESS_MASK, 0 );

		/* Calculate the bitrate in terms of internal clock pulses */
		ui32BitRateInIntClkPulses = 1000000 / ( psDeviceContext->ui32Bitrate * ui32IntClkPeriod );
		if ( ( 1000000 % ( psDeviceContext->ui32Bitrate * ui32IntClkPeriod ) )
				>=
			 ( ( psDeviceContext->ui32Bitrate * ui32IntClkPeriod) / 2 ) )
		{
			ui32BitRateInIntClkPulses++;
		}
		// scb_time_tckh
		ui32Tckh = TIMING_TCKH_FASTMODE / ui32IntClkPeriod;
		if ( TIMING_TCKH_FASTMODE % ui32IntClkPeriod)
		{
			ui32Tckh++;				// round up
		}

		if ( ui32Tckh > 0 )
		{
			ui32Reg = ui32Tckh - 1;
		}
		else
		{
			ui32Reg = 0;
		}
		WRITE_REG( psBlockDesc->ui32Base, SCB_TIME_TCKH, ui32Reg );
		// scb_time_tckl
		ui32Tckl = ui32BitRateInIntClkPulses - ui32Tckh;
		IMG_ASSERT( ( ui32Tckl * ui32IntClkPeriod ) >= TIMING_TCKL_FASTMODE );

		if ( ui32Tckl > 0 )
		{
			ui32Reg = ui32Tckl - 1;
		}
		else
		{
			ui32Reg = 0;
		}
		WRITE_REG( psBlockDesc->ui32Base, SCB_TIME_TCKL, ui32Reg );
		// scb_time_tsdh
		ui32Tsdh = TIMING_TSDH_FASTMODE / ui32IntClkPeriod;
		if ( TIMING_TSDH_FASTMODE % ui32IntClkPeriod )
		{
			ui32Tsdh++;
		}

		if ( ui32Tsdh > 1 )
		{
			ui32Reg = ui32Tsdh - 1;
		}
		else
		{
			ui32Reg = 0x01;
		}
		WRITE_REG( psBlockDesc->ui32Base, SCB_TIME_TSDH, ui32Reg );
		ui32Tsdh = ui32Reg;	// This value is used later
		// scb_time_tpl
		ui32Reg = TIMING_TPL_FASTMODE / ui32IntClkPeriod;
		if ( ui32Reg > 0 )
		{
			--ui32Reg;
		}
		WRITE_REG( psBlockDesc->ui32Base, SCB_TIME_TPL, ui32Reg );
		// scb_time_tph
		ui32Reg = TIMING_TPH_FASTMODE / ui32IntClkPeriod;
		if ( ui32Reg > 0 )
		{
			--ui32Reg;
		}
		WRITE_REG( psBlockDesc->ui32Base, SCB_TIME_TPH, ui32Reg );
		// scb_time_tsdl == scb_time_tpl + scb_time_tsdh + 2
		WRITE_REG( psBlockDesc->ui32Base, SCB_TIME_TSDL, ui32Reg + ui32Tsdh + 2 );
		// scb_time_tp2s
		ui32Reg = TIMING_TP2S_FASTMODE / ui32IntClkPeriod;
		if ( ui32Reg > 0 )
		{
			--ui32Reg;
		}
		WRITE_REG( psBlockDesc->ui32Base, SCB_TIME_TP2S, ui32Reg );

		WRITE_REG( psBlockDesc->ui32Base, SCB_TIME_TBI, TIMING_TBI_FASTMODE );
		WRITE_REG( psBlockDesc->ui32Base, SCB_TIME_TSL, TIMING_TSL_FASTMODE );
		WRITE_REG( psBlockDesc->ui32Base, SCB_TIME_TDL, TIMING_TDL_FASTMODE );

		/* scb_general_control setup */
		ui32Reg = 0;
		/* Set soft reset fields to 1 to release the module from soft reset */
		ui32Reg = WRITE_REG_FIELD(ui32Reg, SCB_GENERAL_CONTROL_SCB_GC_SFR_GEN_DATA, 1);	/* bit 0 */
		ui32Reg = WRITE_REG_FIELD(ui32Reg, SCB_GENERAL_CONTROL_SCB_GC_SFR_DET_DATA, 1);	/* bit 1 */
		ui32Reg = WRITE_REG_FIELD(ui32Reg, SCB_GENERAL_CONTROL_SCB_GC_SFR_MASTER, 1);		/* bit 2 */
		ui32Reg = WRITE_REG_FIELD(ui32Reg, SCB_GENERAL_CONTROL_SCB_GC_SFR_SLAVE, 1);		/* bit 3 */
		ui32Reg = WRITE_REG_FIELD(ui32Reg, SCB_GENERAL_CONTROL_SCB_GC_SFR_REG, 1);		/* bit 4 */
		/* enable clocks */
		ui32Reg = WRITE_REG_FIELD(ui32Reg, SCB_GENERAL_CONTROL_SCB_GC_CKD_XDATA, 1);		/* bit 5 */
		ui32Reg = WRITE_REG_FIELD(ui32Reg, SCB_GENERAL_CONTROL_SCB_GC_CKD_MASTER, 1);		/* bit 6 */
		ui32Reg = WRITE_REG_FIELD(ui32Reg, SCB_GENERAL_CONTROL_SCB_GC_CKD_SLAVE, 0);		/* bit 7 */
		ui32Reg = WRITE_REG_FIELD(ui32Reg, SCB_GENERAL_CONTROL_SCB_GC_CKD_REGS, 1);		/* bit 8 */
		WRITE_REG(psBlockDesc->ui32Base, SCB_GENERAL_CONTROL, ui32Reg);

		/*******************************************************************
		Set up interrupts
		*******************************************************************/

		// Calculate interrupt info for SCBM
		IOBLOCK_CalculateInterruptInformation(	psBlockDesc );

		IMG_MEMCPY( ui32InterruptMasks, psBlockDesc->ui32IntMasks, sizeof(unsigned int) * QIO_MAXVECGROUPS );

		TBI_LOCK( iLockState );

		// HWLEVELEXT for SCBM
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

		/* Disable all interrupts */
		WRITE_REG(psBlockDesc->ui32Base, SCB_INTERRUPT_MASK, 0);

		/* Clear the scb_line_status events */
		WRITE_REG(psBlockDesc->ui32Base, SCB_CLEAR_STATUS, 0xFFFFFFFF);

		// Wait until interrupts are cleared, but don't bother about any slave events
		{
			volatile unsigned long Stat = 0;
			do
			{
				/* Clear all interrupts */
				WRITE_REG(psBlockDesc->ui32Base, SCB_INTERRUPT_CLEAR, 0xFFFFFFFF);

				/* Get interrupt status register value */
				Stat = READ_REG( psBlockDesc->ui32Base, SCB_INTERRUPT_STATUS );
			}
			while ( Stat & 0x2FFFF );
		}

		if ( psBlockDesc->eInterruptLevelType == HWLEVELEXT_LATCHED )
		{
			TBI_LOCK( iLockState );
			ui32Reg = READ( psBlockDesc->sDeviceISRInfo.ui32STATEXTAddress );

			if ( ui32Reg & psBlockDesc->sDeviceISRInfo.ui32STATEXTMask )
			{
				WRITE( psBlockDesc->sDeviceISRInfo.ui32STATEXTAddress, psBlockDesc->sDeviceISRInfo.ui32STATEXTMask );
			}
			TBI_UNLOCK( iLockState );
		}

		/* Enable interrupts */

		/*
			Only enable "Master write fifo almost empty" interrupt when we have data to be written
			otherwise it's a waste of time processing pointless interrupts.
		*/
		WRITE_REG(psBlockDesc->ui32Base, SCB_INTERRUPT_MASK, 0x00000E3E | ENABLE_BUS_INACTIVE_INTERRUPT );


		psDeviceContext->bInitialised = IMG_TRUE;
	}

	pDevice->id = psSettings->ui32BlockIndex;

	return 0;
}


/*!
******************************************************************************

 @Function              start

 @Description	device operation entry point. This function is called when a
				new operation is de-queued.

 @Input		pDevice			: Pointer to the QIO device object.
 			pIOPars			: Pointer to the IO parameters.

 @Output	None

 @Return	void

******************************************************************************/
static void DriverStart(QIO_DEVICE_T *pDevice, QIO_IOPARS_T *pIOPars)
{
	ioblock_sBlockDescriptor	*	psBlockDesc		= g_apsSCBMBlock[ pDevice->id ];
	SCBM_DC_T					*	psDeviceContext = (SCBM_DC_T *)( psBlockDesc->pvAPIContext );

	/* Check we're not still busy with a transaction */
	IMG_ASSERT( (psDeviceContext->bComplete == IMG_TRUE ) || ( psDeviceContext->bCancel == IMG_TRUE ) );

	/* Setup the device context */
	psDeviceContext->pCurrent					= pIOPars;
	psDeviceContext->ui32NumBytesLeft			= pIOPars->counter;
	psDeviceContext->ui32NumBytesTransferred	= 0;
	psDeviceContext->ui32Method					= SCBM_METHOD(pIOPars->opcode);
	psDeviceContext->ui32Address				= SCBM_ADDRESS(pIOPars->opcode);
	psDeviceContext->pucBuffer					= pIOPars->pointer;
	psDeviceContext->bCancel					= IMG_FALSE;
	psDeviceContext->bTransactionStarted		= IMG_FALSE;
	psDeviceContext->bComplete					= IMG_FALSE;

	/* Set the error status variable to no error */
	psDeviceContext->eErrorStatus = SCBM_ERR_NONE;

	/* start the first/only read/write transaction */
	if (psDeviceContext->ui32Method == SCBM_DD_READ)
	{
		ScbmReadFIFO( pDevice, psBlockDesc );
	}
	else
	{
		ScbmWriteFIFO( psBlockDesc );
	}
}

/*!
******************************************************************************

 @Function              isr

 @Description	Interrupt service routine

 @Input		pDevice		: Pointer to the QIO device object.

 @Output	None

 @Return	void

******************************************************************************/
static void DriverISR(QIO_DEVICE_T *pDevice)
{
	ioblock_sBlockDescriptor	*	psBlockDesc = g_apsSCBMBlock[ pDevice->id ];
	SCBM_DC_T	 				*	psDeviceContext = (SCBM_DC_T *)(psBlockDesc->pvAPIContext);
	unsigned long					ui32IrqEnabledState;
	unsigned long					ui32StatusRegValue;
	unsigned long					ui32IrqEnabledStatusRegValue;
	unsigned long					ui32IrqServicedState;
	unsigned long					ui32Trigger;
	unsigned long					ui32Reg;

	ui32Trigger = READ( psBlockDesc->sDeviceISRInfo.ui32STATEXTAddress );
	if ( !( ui32Trigger & psBlockDesc->sDeviceISRInfo.ui32STATEXTMask ) )
	{
		/* spurious interrupt ? */
		IMG_ASSERT(0);
		return;
	}

	/* Read interrupt status register */
	ui32StatusRegValue = READ_REG(psBlockDesc->ui32Base, SCB_INTERRUPT_STATUS);

	/* Read interrupt mask register */
	ui32IrqEnabledState = READ_REG(psBlockDesc->ui32Base, SCB_INTERRUPT_MASK);

	/* Disable all interrupts */
	WRITE_REG(psBlockDesc->ui32Base, SCB_INTERRUPT_MASK, 0x00000000);

	/* Work out the interrupts status of the enabled interrupts */
	ui32IrqEnabledStatusRegValue = (ui32StatusRegValue & ui32IrqEnabledState);

	/* Check the service state of interrupts */
	ui32IrqServicedState = ui32IrqEnabledStatusRegValue;

#if 0
	/*	Clear detected interrupts now rather than at the end of ISR
		as commands are issued in ISR that will cause more interrupts
		to occur
	*/
	WRITE_REG(psBlockDesc->ui32Base, SCB_INTERRUPT_CLEAR, ui32StatusRegValue);
#endif

	if ( g_apsSCBMBlock[ pDevice->id ]->eInterruptLevelType == HWLEVELEXT_LATCHED )
	{
		img_uint32	iLockState;

		// Wait until the enable interrupt are cleared
		while ( READ_REG( psBlockDesc->ui32Base, SCB_INTERRUPT_STATUS ) & ui32IrqEnabledStatusRegValue );
		// Read this to create a bit of a delay
		READ_REG( psBlockDesc->ui32Base, SCB_INTERRUPT_STATUS );

		// Clear the trigger
		TBI_LOCK( iLockState );
		WRITE( psBlockDesc->sDeviceISRInfo.ui32STATEXTAddress, psBlockDesc->sDeviceISRInfo.ui32STATEXTMask );
		TBI_UNLOCK( iLockState );
	}

	/*
	*********************************************************************************
	*********************************************************************************
	Sclk low timeout exceeded
	*********************************************************************************
	*********************************************************************************
	*/
	if (ui32IrqEnabledStatusRegValue & SCB_INTERRUPT_STATUS_IS_SCLK_LOW_INT_MASK)
	{
		/* Update serviced state */
		ui32IrqServicedState ^= SCB_INTERRUPT_STATUS_IS_SCLK_LOW_INT_MASK;

		/* Flag an error */
		psDeviceContext->eErrorStatus = SCBM_ERR_BUS_ERROR;

		ScbmTerminate(pDevice, psDeviceContext);

		IMG_ASSERT(0);
	}

	/*
	*********************************************************************************
	*********************************************************************************
	Sdat low timeout exceeded
	*********************************************************************************
	*********************************************************************************
	*/
	if (ui32IrqEnabledStatusRegValue & SCB_INTERRUPT_STATUS_IS_SDAT_LOW_INT_MASK)
	{
		/* Update serviced state */
		ui32IrqServicedState ^= SCB_INTERRUPT_STATUS_IS_SDAT_LOW_INT_MASK;

		/* Flag an error */
		psDeviceContext->eErrorStatus = SCBM_ERR_BUS_ERROR;

		ScbmTerminate(pDevice, psDeviceContext);

		IMG_ASSERT(0);
	}

	/*
	*********************************************************************************
	*********************************************************************************
	Address acknowledge error interrupt
	*********************************************************************************
	*********************************************************************************
	*/
	if (ui32IrqEnabledStatusRegValue & SCB_INTERRUPT_STATUS_IS_ADDRESS_ACK_ERROR_MASK)
	{
		/* Update serviced state */
		ui32IrqServicedState ^= SCB_INTERRUPT_STATUS_IS_ADDRESS_ACK_ERROR_MASK;

		/* Flag an error */
		psDeviceContext->eErrorStatus = SCBM_ERR_ADDRESS_ERROR;

		ScbmTerminate(pDevice, psDeviceContext);
	}

	/*
	*********************************************************************************
	*********************************************************************************
	Write acknowledge error interrupt
	*********************************************************************************
	*********************************************************************************
	*/
	if (ui32IrqEnabledStatusRegValue & SCB_INTERRUPT_STATUS_IS_WRITE_ACK_ERROR_MASK)
	{
		/* Update serviced state */
		ui32IrqServicedState ^= SCB_INTERRUPT_STATUS_IS_WRITE_ACK_ERROR_MASK;

		psDeviceContext->eErrorStatus = SCBM_ERR_TRANSFER_ERROR;

		ScbmTerminate(pDevice, psDeviceContext);

		IMG_ASSERT(0);
	}

	/*
	*********************************************************************************
	*********************************************************************************
	Unexpected start bit interrupt
	*********************************************************************************
	*********************************************************************************
	*/
	if (ui32IrqEnabledStatusRegValue & SCB_INTERRUPT_STATUS_IS_UNEXP_START_INT_MASK)
	{
		/* Update serviced state */
		ui32IrqServicedState ^= SCB_INTERRUPT_STATUS_IS_UNEXP_START_INT_MASK;

		/* Lets try ignore this */
		psDeviceContext->eErrorStatus = SCBM_ERR_BUS_ERROR;

		ScbmTerminate(pDevice, psDeviceContext);
		IMG_ASSERT(0);
	}

	/*
	*********************************************************************************
	*********************************************************************************
	Master read fifo full interrupt
	*********************************************************************************
	*********************************************************************************
	*/
	if ( (ui32IrqEnabledStatusRegValue & SCB_INTERRUPT_STATUS_IS_MASTER_READ_FULL_MASK) ||
		 (ui32IrqEnabledStatusRegValue & SCB_INTERRUPT_STATUS_IS_MASTER_READ_AFULL_MASK) )
	{
		/* Update serviced state */
		if (ui32IrqEnabledStatusRegValue & SCB_INTERRUPT_STATUS_IS_MASTER_READ_FULL_MASK)
			ui32IrqServicedState ^= SCB_INTERRUPT_STATUS_IS_MASTER_READ_FULL_MASK;
		if (ui32IrqEnabledStatusRegValue & SCB_INTERRUPT_STATUS_IS_MASTER_READ_AFULL_MASK)
			ui32IrqServicedState ^= SCB_INTERRUPT_STATUS_IS_MASTER_READ_AFULL_MASK;

		if ( ( psDeviceContext->eErrorStatus == SCBM_ERR_NONE ) &&
			 ( psDeviceContext->ui32Method == SCBM_DD_READ ) )
		{
			ScbmReadFIFO( pDevice, psBlockDesc );
		}
	}

		/*
	*********************************************************************************
	*********************************************************************************
	Master write fifo empty interrupt
	*********************************************************************************
	*********************************************************************************
	*/
	if (ui32IrqEnabledStatusRegValue & SCB_INTERRUPT_STATUS_IS_MASTER_WRITE_EMPTY_MASK)
	{
		/* Update serviced state */
		ui32IrqServicedState ^= SCB_INTERRUPT_STATUS_IS_MASTER_WRITE_EMPTY_MASK;

		/* Check if we've finished sending everything or if the transaction was cancelled */
		if (	( psDeviceContext->ui32NumBytesLeft > 0 ) &&
				( psDeviceContext->eErrorStatus == SCBM_ERR_NONE ) &&
				( psDeviceContext->ui32Method == SCBM_DD_WRITE )		)
		{
			/* Send some more data to the buffer if there is any left */
			ScbmWriteFIFO( psBlockDesc );
		}
	}

	/*
	*********************************************************************************
	*********************************************************************************
	Master write fifo almost empty interrupt
	*********************************************************************************
	*********************************************************************************
	*/
	if (ui32IrqEnabledStatusRegValue & SCB_INTERRUPT_STATUS_IS_MASTER_WRITE_AEMPTY_MASK)
	{
		/* Update services state */
		ui32IrqServicedState ^= SCB_INTERRUPT_STATUS_IS_MASTER_WRITE_AEMPTY_MASK;

		/* Check if the transaction was cancelled */
		if ( ( psDeviceContext->ui32NumBytesLeft > 0) &&
			 ( psDeviceContext->eErrorStatus == SCBM_ERR_NONE) &&
			 ( psDeviceContext->ui32Method == SCBM_DD_WRITE ) )
		{
			/* Send some more data to the buffer if there is any left */
			ScbmWriteFIFO( psBlockDesc );
		}
	}

	/*
	*********************************************************************************
	*********************************************************************************
	Bus inactive interrupt
	*********************************************************************************
	*********************************************************************************
	*/
	if (ui32IrqEnabledStatusRegValue & SCB_INTERRUPT_STATUS_IS_INACTIVE_INT_MASK)
	{
		/* Update serviced state */
		ui32IrqServicedState ^= SCB_INTERRUPT_STATUS_IS_INACTIVE_INT_MASK;

		/* If we have transferred everything, then this will probably happen,
		   otherwise it is probably an error */
		if ( psDeviceContext->ui32NumBytesLeft != 0 && !psDeviceContext->bComplete )
		{
			psDeviceContext->eErrorStatus = SCBM_ERR_BUS_INACTIVE;

			ScbmTerminate(pDevice, psDeviceContext);
			IMG_ASSERT(0);
		}
	}

	/*
	*********************************************************************************
	*********************************************************************************
	All interrupt events now handled.  Check to see if we've finished a transfer
	(successfully or otherwise)
	*********************************************************************************
	*********************************************************************************
	*/
	if (psDeviceContext->ui32NumBytesLeft == 0 && !psDeviceContext->bComplete)
	{
		if (psDeviceContext->eErrorStatus == SCBM_ERR_NONE)
		{
			if ( psDeviceContext->ui32Method == SCBM_DD_WRITE )
			{
				// If this is a write transaction, check if the buffer is empty in case the next transaction is also a write transaction
				ui32Reg = READ_REG(psBlockDesc->ui32Base, SCB_MASTER_FILL_STATUS);
				if ( READ_REG_FIELD(ui32Reg, SCB_MASTER_FILL_STATUS_MWS_REG_EMPTY) == 1 )
				{
					ScbmCompleteIO(pDevice, psDeviceContext, psDeviceContext->ui32NumBytesTransferred);
				}
			}
			else
			{
				ScbmCompleteIO(pDevice, psDeviceContext, psDeviceContext->ui32NumBytesTransferred);
			}
		}
		else
		{
			ScbmTerminate(pDevice, psDeviceContext);
		}
	}

	/* Check that all interrupts have been serviced */
	IMG_ASSERT(ui32IrqServicedState == 0);

	/*
		Clear detected interrupts now
	*/
	WRITE_REG(psBlockDesc->ui32Base, SCB_INTERRUPT_CLEAR, ui32StatusRegValue);

	/* Enable interrupts */
	WRITE_REG(psBlockDesc->ui32Base, SCB_INTERRUPT_MASK, ui32IrqEnabledState);

	return;
}


/*============================================================================*/
/*                                                                            */
/*                          EXPORTED FUNCTIONS			                      */
/*                                                                            */
/*============================================================================*/

/*!
*******************************************************************************

 @Function              @SCBMasterGetErrorStatus

 <b>Description:</b>\n
 This function gets the error status of the current transaction. It is passed a
 pointer to the port descriptor and returns the current error status of the port.

 (NB. SCBM_PORT_T is defined in scbm_api.h)

 \param     *port               Constant pointer to port descriptor.

*******************************************************************************/
unsigned long SCBMasterGetErrorStatus(const SCBM_PORT_T *port)
{
	return ((SCBM_DC_T *)g_apsSCBMBlock[ port->device.id ]->pvAPIContext)->eErrorStatus;
}
