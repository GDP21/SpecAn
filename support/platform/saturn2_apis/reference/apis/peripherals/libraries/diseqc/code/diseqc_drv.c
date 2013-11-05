/*!*******************************************************************************
  file   diseqc_drv.c

  brief  DiSEqC Device Driver 

         This file defines the functions that make up the DiSEqC device driver.

  author Imagination Technologies

         <b>Copyright 2011 by Imagination Technologies Limited.</b>\n
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


/*===========================================================================*/
/*                              INCLUDE FILES                                */
/*===========================================================================*/


/* Include these before any other include to ensure TBI used correctly */
/* All METAG and METAC values from machine.inc and then tbi. */

#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include <assert.h>

/* MeOS Library */
#include <MeOS.h>

/* System */
#include <system.h>
#include <ioblock_defs.h>
#include <ioblock_utils.h>
#include <sys_util.h>
#include <sys_config.h>


/* DISEQC Slave Driver */
#include <diseqc_api.h>
#include "diseqc_drv.h"
#include "diseqc_reg_defs.h"

/*===========================================================================*/
/*                            MACRO DEFINITIONS                              */
/*===========================================================================*/

#define DISEQC_WRITE(a,d)    (*(volatile img_uint32 *)(a) = (d))
#define DISEQC_READ(a)       (*(volatile img_uint32 *)(a))

#define	DISEQC_WRITE_REG_FIELD(Reg,Field,Val)									\
	(Reg) = WRITE_REG_FIELD((Reg),Field,(Val));

#define	DISEQC_MODIFY_REG_FIELD(Base,RegName,FieldName,Val)						\
	ui32Reg = READ_REG((Base),RegName);											\
	DISEQC_WRITE_REG_FIELD(ui32Reg,FieldName,(Val));							\
	WRITE_REG((Base),RegName,ui32Reg)		

#define	DISEQC_GET_REG(Base,RegName,FieldName)									\
	READ_REG_FIELD(READ_REG((Base),RegName),FieldName)

#define	DISEQC_SET_REG(Base,RegName,FieldName,Val)								\
	WRITE_REG((Base),RegName,((Val)<<(FieldName##_SHIFT))&(FieldName##_MASK))	

#define	DISEQC_INTERRUPT_ACTIVE(Reg,FieldName)									\
	((Reg)&(FieldName##_MASK))


/* Default settings */
#define	DISEQC_DEFAULT_RX_TONE_THR					500
#define	DISEQC_DEFAULT_RX_TONE_EDGE_THR				9
#define	DISEQC_DEFAULT_RX_CHUNK_WIDTH				12
#define	DISEQC_DEFAULT_SLAVE_POST_MESSAGE_SILENCE	0x84

#define	DISEQC_DEFAULT_SHORT_CHUNK_WIDTH			11
#define	DISEQC_DEFAULT_LONG_CHUNK_WIDTH				22

#define	DISEQC_FIFO_LENGTH							16

#define	DISEQC_DEFAULT_MASTER_EMPTY_TIMEOUT			0x84
#define	DISEQC_DEFAULT_SLAVE_RESPONSE_TIMEOUT		0xCE4
#define	DISEQC_DEFAULT_POST_TRANSACTION_SILENCE		0x14A



/*===========================================================================*/
/*                             DATA STRUCTURES                               */
/*===========================================================================*/

/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*===========================================================================*/

static IMG_INT32	DISEQCDriverInit(QIO_DEVICE_T *dev, QIO_DEVPOWER_T *pwrClass, img_int32 *devRank, unsigned intMasks[QIO_MAXVECGROUPS]);
static IMG_VOID		DISEQCDriverStart(QIO_DEVICE_T *dev, QIO_IOPARS_T *ioPars);
static IMG_VOID		DISEQCDriverISR(QIO_DEVICE_T *dev);

static IMG_VOID		DISEQCDriverClearNonIdleInterrupts(IMG_UINT32 ui32Base);
static IMG_VOID		DISEQCDriverSetIdle(IMG_UINT32 ui32Base, DISEQC_PORT_T *psPort);
static IMG_VOID		DISEQCDriverCompleteOperation(IMG_UINT32 ui32Base, QIO_DEVICE_T *psDevice, DISEQC_PORT_T *psPort);
static IMG_VOID		DISEQCDriverDelayOperation(IMG_UINT32 ui32Base, DISEQC_PORT_T *psPort);
static IMG_VOID		DISEQCDriverSetMasterData(IMG_UINT32 ui32Base, IMG_BYTE *pui8Ptr, IMG_UINT8 ui8NumBytes);
static IMG_VOID		DISEQCDriverSendMessage(DISEQC_PORT_T *psPort, QIO_IOPARS_T *psIOParams, IMG_UINT32 ui32Base);
static IMG_VOID		DISEQCDriverDoTimer(IMG_UINT32 ui32Base, IMG_UINT32 ui32Time);
static IMG_VOID		DISEQCDriverClearSlaveFIFO(IMG_UINT32 ui32Base);
static IMG_VOID		DISEQCDriverGetSlaveData(IMG_UINT32 ui32Base, IMG_BYTE *pui8Ptr, IMG_UINT8 ui8NumBytes);
static IMG_VOID		DISEQCDriverCompleteUnexpectedReceive(IMG_UINT32 ui32Base, QIO_DEVICE_T *psDevice, DISEQC_PORT_T *psPort);
static IMG_VOID		DISEQCDriverCompleteSilence(IMG_UINT32 ui32Base, QIO_DEVICE_T *psDevice, DISEQC_PORT_T *psPort, IMG_UINT32 SilenceDone, IMG_VOID (*CompleteFunction)(IMG_UINT32,QIO_DEVICE_T *,DISEQC_PORT_T *));



/*===========================================================================*/
/*                                GLOBAL DATA                                */
/*===========================================================================*/

/* the driver object */
const QIO_DRIVER_T DISEQC_Driver =
{
    DISEQCDriverISR,		/* ISR                       */
    DISEQCDriverInit,		/* init function             */
    DISEQCDriverStart,		/* start function            */
    IMG_NULL,				/* no cancel function        */
    IMG_NULL,				/* no power control function */
    IMG_NULL,				/* no sim start function     */
    IMG_NULL   				/* no shut function          */
};

/* Global pointers to DiSEqC block descriptors */
ioblock_sBlockDescriptor *g_apsDISEQCBlock[DISEQC_NUM_BLOCKS] =
{
	IMG_NULL,IMG_NULL
};


/*===========================================================================*/
/*                             STATIC FUNCTIONS                              */
/*===========================================================================*/



static IMG_INT32 DISEQCDriverInit(QIO_DEVICE_T *psDevice, QIO_DEVPOWER_T *pePowerClass, img_int32 *pui32DeviceRank, img_uint32 aui32InterruptMasks[QIO_MAXVECGROUPS])
{
	img_uint32					ui32BlockIndex;
	img_uint32					ui32Base;
	img_uint32					ui32Reg;
    img_int32					lockState;
	ioblock_sBlockDescriptor	*psBlockDesc;
	DISEQC_PORT_SETTINGS_T		*psSettings;
	IMG_UINT32					ui32RXToneThr;
	IMG_UINT32					ui32RXToneEdgeThr;
	IMG_UINT32					ui32RXChunkWidth;
	IMG_UINT32					ui32RXSilence;
	IMG_UINT32					ui32SlaveResponseTimeout;
	IMG_UINT32					ui32TXShortChunkWidth;
	IMG_UINT32					ui32TXLongChunkWidth;
	IMG_UINT32					ui32MasterEmptyTimeout;
	IMG_UINT32					ui32PostTransactionSilence;
	IMG_UINT32					ui32FreqMhz;
	DISEQC_PORT_T				*psPort;
	
	
	ui32BlockIndex = psDevice->id;
	psBlockDesc = g_apsDISEQCBlock[ui32BlockIndex];
	ui32Base = psBlockDesc->ui32Base;
	psPort = (DISEQC_PORT_T *)psBlockDesc->pvAPIContext;
	psSettings = psPort->psSettings;


	/* Disable interrupts */
	WRITE_REG(ui32Base,DISEQC_CR_DISEQC_IMR,0xFFFFFFFF);



	/* Provide information about the device */
    *pui32DeviceRank = 1;
    *pePowerClass = QIO_POWERNONE;

	/* Calculate interrupt masks */
	IOBLOCK_CalculateInterruptInformation(psBlockDesc);

	IMG_MEMCPY(aui32InterruptMasks, psBlockDesc->ui32IntMasks, sizeof(img_uint32)*QIO_MAXVECGROUPS);

	/* Read-modify-write to configure level sensitive interrupts */
	TBI_LOCK(lockState);

	ui32Reg = DISEQC_READ(psBlockDesc->sDeviceISRInfo.ui32LEVELEXTAddress);
	if(psBlockDesc->eInterruptLevelType==HWLEVELEXT_LATCHED)
	{
		ui32Reg &= ~(psBlockDesc->sDeviceISRInfo.ui32LEVELEXTMask);
	}
	else if(psBlockDesc->eInterruptLevelType==HWLEVELEXT_NON_LATCHED)
	{
		ui32Reg |= (psBlockDesc->sDeviceISRInfo.ui32LEVELEXTMask);
	}
	DISEQC_WRITE(psBlockDesc->sDeviceISRInfo.ui32LEVELEXTAddress,ui32Reg);

	TBI_UNLOCK(lockState);





	/* Get settings from the psSettings structure or use the defaults where settings have not been given */

	if(psSettings->psReceiveTimings==IMG_NULL)
	{
		ui32RXToneThr = DISEQC_DEFAULT_RX_TONE_THR;
		ui32RXToneEdgeThr = DISEQC_DEFAULT_RX_TONE_EDGE_THR;
		ui32RXChunkWidth = DISEQC_DEFAULT_RX_CHUNK_WIDTH;
		ui32SlaveResponseTimeout = DISEQC_DEFAULT_SLAVE_RESPONSE_TIMEOUT;
		ui32RXSilence = DISEQC_DEFAULT_SLAVE_POST_MESSAGE_SILENCE;
	}
	else
	{
		ui32RXToneThr = psSettings->psReceiveTimings->ui16ToneTolerance;
		ui32RXToneEdgeThr = psSettings->psReceiveTimings->ui8MinDetections;
		ui32RXChunkWidth = psSettings->psReceiveTimings->ui8ChunkWidth;
		ui32SlaveResponseTimeout = psSettings->psReceiveTimings->ui16SlaveResponseTimeout;
		ui32RXSilence = psSettings->psReceiveTimings->ui8PostMessageSilence;
	}

	/* Calculation to get tone threshold from tolerance value as per the TRM (changed to 0-1000 to keep as integers) */
	ui32FreqMhz = SYS_getSysUndeletedFreq_fp()>>20;

	ui32RXToneThr = ((1000-ui32RXToneThr)*ui32FreqMhz)/44;

	if(psSettings->psSendTimings==IMG_NULL)
	{
		ui32TXShortChunkWidth = DISEQC_DEFAULT_SHORT_CHUNK_WIDTH;
		ui32TXLongChunkWidth = DISEQC_DEFAULT_LONG_CHUNK_WIDTH;
		ui32MasterEmptyTimeout = DISEQC_DEFAULT_MASTER_EMPTY_TIMEOUT;
		ui32PostTransactionSilence = DISEQC_DEFAULT_POST_TRANSACTION_SILENCE;
	}
	else
	{
		ui32TXShortChunkWidth = psSettings->psSendTimings->ui8ShortChunkWidth;
		ui32TXLongChunkWidth = psSettings->psSendTimings->ui8LongChunkWidth;
		ui32MasterEmptyTimeout = psSettings->psSendTimings->ui16MasterEmptyTimeout;
		ui32PostTransactionSilence = psSettings->psSendTimings->ui16PostTransactionSilence;
	}

	/* Update the port context with the timeout/silence times */

	psPort->ui32MasterEmptyTimeout = ui32MasterEmptyTimeout;
	psPort->ui32SlaveResponseTimeout = ui32SlaveResponseTimeout;
	psPort->ui32PostTransactionSilence = ui32PostTransactionSilence;
	psPort->ui32PostSlaveMessageSilence = ui32RXSilence;

	/* Write the settings to the registers */

	ui32Reg = 0;
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_RX_ACC_THRESHOLDS_CR_DISEQC_RX_TONE_THR,ui32RXToneThr);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_RX_ACC_THRESHOLDS_CR_DISEQC_RX_TONE_EDGE_THR,ui32RXToneEdgeThr);
	WRITE_REG(ui32Base,DISEQC_CR_DISEQC_RX_ACC_THRESHOLDS,ui32Reg);

	ui32Reg = 0;
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_EDGE_THRESHOLDS_CR_DISEQC_SHORT_MOD,ui32TXShortChunkWidth);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_EDGE_THRESHOLDS_CR_DISEQC_LONG_MOD,ui32TXLongChunkWidth);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_EDGE_THRESHOLDS_CR_DISEQC_RX_CHUNK_WIDTH,ui32RXChunkWidth);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_EDGE_THRESHOLDS_CR_DISEQC_WAIT_MOD,ui32RXSilence);
	WRITE_REG(ui32Base,DISEQC_CR_DISEQC_EDGE_THRESHOLDS,ui32Reg);

	ui32Reg = 0;
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_TIMEOUT_CR_DISEQC_MST_TIMEOUT,ui32MasterEmptyTimeout);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_TIMEOUT_CR_DISEQC_SLV_TIMEOUT,ui32SlaveResponseTimeout);
	WRITE_REG(ui32Base,DISEQC_CR_DISEQC_TIMEOUT,ui32Reg);

	DISEQC_SET_REG(ui32Base,DISEQC_CR_DISEQC_SILENCE_TIMER,DISEQC_CR_DISEQC_SILENCE_TIMER_CR_DISEQC_POST_TIMER,ui32PostTransactionSilence);

	ui32Reg = READ_REG(ui32Base,DISEQC_CR_DISEQC_CFG);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_CFG_CR_DISEQC_HALF_CLK_BEATS,psSettings->ui32TargetFreq);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_CFG_CR_DISEQC_LOOPBACK,psSettings->Loopback);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_CFG_CR_DISEQC_TONE_DETECT_OFF,psSettings->Receive);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_CFG_CR_DISEQC_RX_MOD_POL,psSettings->InputPolarity);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_CFG_CR_DISEQC_TX_MOD_POL,psSettings->OutputPolarity);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_CFG_CR_DISEQC_TONE_OUT,psSettings->Tone);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_CFG_CR_DISEQC_EN,1);
	WRITE_REG(ui32Base,DISEQC_CR_DISEQC_CFG,ui32Reg);

	/* Clear the read FIFO */
	DISEQCDriverClearSlaveFIFO(ui32Base);

	/* Clear all interrupts */
	WRITE_REG(ui32Base,DISEQC_CR_DISEQC_ISR_CLR,0xFFFFFFFF);

	/* Set the driver to the idle state */
	DISEQCDriverSetIdle(ui32Base,psPort);

	/* Continuous tone not present */
	psPort->bContinuousTone = IMG_FALSE;

    return 0;
}

static IMG_VOID DISEQCDriverStart(QIO_DEVICE_T *psDevice, QIO_IOPARS_T *psIOParams)
{
	IMG_UINT32					ui32Reg;
	ioblock_sBlockDescriptor	*psBlockDesc;
	IMG_UINT32					ui32BlockIndex;
	IMG_UINT32					ui32Base;
	DISEQC_PORT_T				*psPort;
	DISEQC_IO_BLOCK_T			*psIOB;
	
	ui32BlockIndex = psDevice->id;
	psBlockDesc = g_apsDISEQCBlock[ui32BlockIndex];
	ui32Base = psBlockDesc->ui32Base;
	psPort = (DISEQC_PORT_T *)psBlockDesc->pvAPIContext;
	psIOB = (DISEQC_IO_BLOCK_T *)psIOParams->spare;

	/* New operation? */
	if(psPort->psDelayOperation==IMG_NULL)
	{
		/* The master FIFO will not contain any data */
		psPort->bMasterData = IMG_FALSE;
	}

	psPort->psCurOperation = psIOParams;
	psPort->psDelayOperation = IMG_NULL;
	
	/* Unexpected reply in progress? */
	if((psPort->ui8Received)||(DISEQC_GET_REG(ui32Base,DISEQC_CR_DISEQC_STATUS,DISEQC_CR_DISEQC_STATUS_CR_DISEQC_SLAVE_REPLY_IN_PROG)))
	{
		/* Delay the operation until it has completed */
		DISEQCDriverDelayOperation(ui32Base,psPort);
		return;
	}

	/* Set the port context for a new operation */
	psPort->ui8Received = 0;
	psPort->bSilence = IMG_FALSE;
	psPort->Status = DISEQC_STATUS_OK;
	psPort->ui8TotalReceived = 0;

	switch(psIOParams->opcode)
	{
	case DISEQC_OPCODE_CONTINUOUS_TONE:

		/* Assert the continuous tone */
		DISEQC_MODIFY_REG_FIELD(ui32Base,DISEQC_CR_DISEQC_CFG,DISEQC_CR_DISEQC_CFG_CR_DISEQC_TONE_ON,1);

		/* No data will be recevied */
		psIOB->pui8RecvBuf = IMG_NULL;

		/* Continous tone is now present on the port */
		psPort->bContinuousTone = IMG_TRUE;

		/* Can't receive unexpected bytes while a continous tone is present, ignore all interrupts */
		WRITE_REG(ui32Base,DISEQC_CR_DISEQC_IMR,0xFFFFFFFF);

		/* Operation completes immediately */
		DISEQCDriverCompleteOperation(ui32Base,psDevice,psPort);

		return;




	case DISEQC_OPCODE_END_CONTINUOUS_TONE:

		/* Deassert the continuous tone */
		DISEQC_MODIFY_REG_FIELD(ui32Base,DISEQC_CR_DISEQC_CFG,DISEQC_CR_DISEQC_CFG_CR_DISEQC_TONE_ON,0);

		/* No data will be recevied */
		psIOB->pui8RecvBuf = IMG_NULL;

		/* Continuous tone no longer present on port */
		psPort->bContinuousTone = IMG_FALSE;

		/* Silence on the bus for PostTransactionSilence time */
		DISEQCDriverDoTimer(ui32Base,psPort->ui32PostTransactionSilence);

		break;




	case DISEQC_OPCODE_TONE_BURST_A:

		/* Continuous tone present? */
		if(psPort->bContinuousTone==IMG_TRUE)
		{
			/* Can't do a tone burst A if a continuous tone is present. Return error */
			psPort->Status = DISEQC_STATUS_ERR_CONTINUOUS_TONE;
			DISEQCDriverCompleteOperation(ui32Base,psDevice,psPort);
		}
		else
		{
			/* Update the automatic post transaction silence */
			DISEQC_MODIFY_REG_FIELD(ui32Base,DISEQC_CR_DISEQC_SILENCE_TIMER,DISEQC_CR_DISEQC_SILENCE_TIMER_CR_DISEQC_POST_TIMER,psPort->ui32PostTransactionSilence);

			/* Start the tone burst A*/
			DISEQC_SET_REG(ui32Base,DISEQC_CR_DISEQC_ACTION,DISEQC_CR_DISEQC_ACTION_CR_DISEQC_TBA,1);

			/* No data will be recevied */
			psIOB->pui8RecvBuf = IMG_NULL;

			/* Enable the correct interrupts for tone burst A */
			ui32Reg = 0xFFFFFFFF;
			DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_XACTION_DONE_MASK,0);
			DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_XACTION_NOT_DONE_MASK,0);
			WRITE_REG(ui32Base,DISEQC_CR_DISEQC_IMR,ui32Reg);
		}

		break;




	case DISEQC_OPCODE_TONE_BURST_B:

		/* Continuous tone present? */
		if(psPort->bContinuousTone==IMG_TRUE)
		{
			/* Can't do a tone burst B if a continuous tone is present. Return error */
			psPort->Status = DISEQC_STATUS_ERR_CONTINUOUS_TONE;
			DISEQCDriverCompleteOperation(ui32Base,psDevice,psPort);
		}
		else
		{
			/* Update the automatic post transaction silence */
			DISEQC_MODIFY_REG_FIELD(ui32Base,DISEQC_CR_DISEQC_SILENCE_TIMER,DISEQC_CR_DISEQC_SILENCE_TIMER_CR_DISEQC_POST_TIMER,psPort->ui32PostTransactionSilence);

			/* Start the tone burst B*/
			DISEQC_SET_REG(ui32Base,DISEQC_CR_DISEQC_ACTION,DISEQC_CR_DISEQC_ACTION_CR_DISEQC_TBB,1);

			/* No data will be recevied */
			psIOB->pui8RecvBuf = IMG_NULL;

			/* Enable the correct interrupts for tone burst B */
			ui32Reg = 0xFFFFFFFF;
			DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_XACTION_DONE_MASK,0);
			DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_XACTION_NOT_DONE_MASK,0);
			WRITE_REG(ui32Base,DISEQC_CR_DISEQC_IMR,ui32Reg);
		}

		break;




	case DISEQC_OPCODE_SEND_MESSAGE:

		/* Continuous tone present? */
		if(psPort->bContinuousTone==IMG_TRUE)
		{
			/* Can't send a message if a continuous tone is present. Return error */
			psPort->Status = DISEQC_STATUS_ERR_CONTINUOUS_TONE;
			DISEQCDriverCompleteOperation(ui32Base,psDevice,psPort);
		}
		else
		{
			/* Send the message */
			DISEQCDriverSendMessage(psPort,psIOParams,ui32Base);
		}

		break;




	case DISEQC_OPCODE_DEINIT:

		/* Deassert a continuous tone and disable the port */
		ui32Reg = READ_REG(ui32Base,DISEQC_CR_DISEQC_CFG);
		DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_CFG_CR_DISEQC_TONE_ON,0);
		DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_CFG_CR_DISEQC_EN,0);
		WRITE_REG(ui32Base,DISEQC_CR_DISEQC_CFG,ui32Reg);

		/* Disable interrupts */
		WRITE_REG(ui32Base,DISEQC_CR_DISEQC_IMR,0xFFFFFFFF);

		QIO_complete(psDevice,QIO_IOCOMPLETE);
		QIO_start(psDevice);

		return;




	default:
		IMG_ASSERT(0);
		return;
	}

	
}



static IMG_VOID DISEQCDriverISR(QIO_DEVICE_T *psDevice)
{
	ioblock_sBlockDescriptor	*psBlockDesc;
	DISEQC_PORT_T				*psPort;
	IMG_UINT32					ui32Reg;
	IMG_UINT32					ui32ISR;
	IMG_UINT32					ui32IMR;
	IMG_UINT32					ui32Base;
	DISEQC_IO_BLOCK_T			*psIOB;
	IMG_UINT8					ui8BytesReceived;
	QIO_IOPARS_T				*psCurOperation;
	IMG_UINT8					ui8Remaining;
	IMG_UINT8					ui8RemainingBuffer;

	psBlockDesc = g_apsDISEQCBlock[psDevice->id];
	if(psBlockDesc==IMG_NULL)
	{
		IMG_ASSERT(0);
		return;
	}
	ui32Base = psBlockDesc->ui32Base;
	psPort = (DISEQC_PORT_T *)psBlockDesc->pvAPIContext;

	ui32Reg = DISEQC_READ(psBlockDesc->sDeviceISRInfo.ui32STATEXTAddress);
	if(!(ui32Reg&psBlockDesc->sDeviceISRInfo.ui32STATEXTMask))
	{
		/* Spurious interrupt? */
		IMG_ASSERT(0);
		return;
	}


	/* Keep checking the ISR and IMR until there are no active interrupts. This ensures the IRQ will not remain 
	   asserted, which would block further interrupts. The current operation may change in response to the
	   interrupts detected, so it is re-read */
	while(IMG_TRUE)
	{
		psCurOperation = psPort->psCurOperation;
		if(psCurOperation==IMG_NULL){psIOB=IMG_NULL;}
		else{psIOB = (DISEQC_IO_BLOCK_T *)psCurOperation->spare;}

		ui32ISR = READ_REG(ui32Base,DISEQC_CR_DISEQC_ISR);
		ui32IMR = READ_REG(ui32Base,DISEQC_CR_DISEQC_IMR);

		ui32ISR &= ~ui32IMR;

		if(!ui32ISR){return;}

		/* Unexpected message being received? */
		if(psCurOperation==IMG_NULL)
		{
			/* Post receive silence complete? */
			if(DISEQC_INTERRUPT_ACTIVE(ui32ISR,DISEQC_CR_DISEQC_ISR_CR_DISEQC_XACTION_DONE))
			{
				DISEQCDriverClearNonIdleInterrupts(ui32Base);
				DISEQCDriverCompleteUnexpectedReceive(ui32Base,psDevice,psPort);
			}

			/* Slave FIFO full and another write attempted (data lost)? */
			else if(DISEQC_INTERRUPT_ACTIVE(ui32ISR,DISEQC_CR_DISEQC_ISR_CR_DISEQC_SLV_FIFO_FULL))
			{
				DISEQCDriverClearSlaveFIFO(ui32Base);
				psPort->Status = DISEQC_STATUS_ERR_RECEIVE_OVERFLOW;

				/* Some data has been lost, no point in receiving further data. Enable completed and failed interrupts only */ 
				ui32Reg = 0xFFFFFFFF;
				DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_RX_XACTION_MASK,0);
				DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_XACTION_FAIL_MASK,0);
			 	WRITE_REG(ui32Base,DISEQC_CR_DISEQC_IMR,ui32Reg);
			}

			/* Receive completed successfully? */
			else if(DISEQC_INTERRUPT_ACTIVE(ui32ISR,DISEQC_CR_DISEQC_ISR_CR_DISEQC_RX_XACTION))
			{
				/* Receive overflow occurred at some point during the receive? */
				if(psPort->Status==DISEQC_STATUS_ERR_RECEIVE_OVERFLOW)
				{
					DISEQCDriverClearSlaveFIFO(ui32Base);
				}
				else
				{
					/* Get total number of bytes received */
					ui8BytesReceived = DISEQC_GET_REG(ui32Base,DISEQC_CR_DISEQC_MSG_INFO,DISEQC_CR_DISEQC_MSG_INFO_CR_DISEQC_SLV_LEN);
					psPort->ui8TotalReceived = ui8BytesReceived;

					/* No buffer to hold the unexpected data? */
					if(psPort->pui8UnexpectedData==IMG_NULL)
					{
						DISEQCDriverClearSlaveFIFO(ui32Base);
					}
					else
					{
						ui8Remaining = ui8BytesReceived - psPort->ui8Received;
						ui8RemainingBuffer = psPort->ui8UnexpectedDataMaxSize - psPort->ui8Received;

						/* Less space in buffer than bytes in FIFO? */
						if(ui8Remaining>ui8RemainingBuffer)
						{
							DISEQCDriverGetSlaveData(ui32Base,psPort->pui8UnexpectedData+psPort->ui8Received,ui8RemainingBuffer);
							DISEQCDriverClearSlaveFIFO(ui32Base);
						}
						else
						{
							DISEQCDriverGetSlaveData(ui32Base,psPort->pui8UnexpectedData+psPort->ui8Received,ui8Remaining);
						}
					}
				}

				WRITE_REG(ui32Base,DISEQC_CR_DISEQC_ISR_CLR,0xFFFFFFFF);

				/* Complete the silence requirement */
				DISEQCDriverCompleteSilence(ui32Base,psDevice,psPort,psPort->ui32PostSlaveMessageSilence,DISEQCDriverCompleteUnexpectedReceive);
			}

			/* Receive failed? */
			else if(DISEQC_INTERRUPT_ACTIVE(ui32ISR,DISEQC_CR_DISEQC_ISR_CR_DISEQC_XACTION_FAIL))
			{
				DISEQCDriverClearSlaveFIFO(ui32Base);

				DISEQC_MODIFY_REG_FIELD(ui32Base,DISEQC_CR_DISEQC_CFG,DISEQC_CR_DISEQC_CFG_CR_DISEQC_EN,0);
				DISEQC_SET_REG(ui32Base,DISEQC_CR_DISEQC_ACTION,DISEQC_CR_DISEQC_ACTION_CR_DISEQC_RESET,1);
				DISEQC_SET_REG(ui32Base,DISEQC_CR_DISEQC_ACTION,DISEQC_CR_DISEQC_ACTION_CR_DISEQC_RESET,0);
				DISEQC_MODIFY_REG_FIELD(ui32Base,DISEQC_CR_DISEQC_CFG,DISEQC_CR_DISEQC_CFG_CR_DISEQC_EN,1);

				WRITE_REG(ui32Base,DISEQC_CR_DISEQC_ISR_CLR,0xFFFFFFFF);

				psPort->Status = DISEQC_STATUS_ERR_RECEIVE;

				/* Complete the silence requirement */
				DISEQCDriverCompleteSilence(ui32Base,psDevice,psPort,0,DISEQCDriverCompleteUnexpectedReceive);
			}

			/* Receive FIFO full? */
			else if(DISEQC_INTERRUPT_ACTIVE(ui32ISR,DISEQC_CR_DISEQC_ISR_CR_DISEQC_FIFO_WM_REACHED))
			{
				ui8RemainingBuffer = psPort->ui8UnexpectedDataMaxSize-psPort->ui8Received;

				/* Less space in buffer than bytes in FIFO? */
				if(ui8RemainingBuffer<DISEQC_FIFO_LENGTH)
				{
					DISEQCDriverGetSlaveData(ui32Base,psPort->pui8UnexpectedData+psPort->ui8Received,ui8RemainingBuffer);
					DISEQCDriverClearSlaveFIFO(ui32Base);
					psPort->ui8Received += ui8RemainingBuffer;
				}
				else
				{
					DISEQCDriverGetSlaveData(ui32Base,psPort->pui8UnexpectedData+psPort->ui8Received,DISEQC_FIFO_LENGTH);
					psPort->ui8Received += DISEQC_FIFO_LENGTH;
				}
			
				ui32Reg = 0xFFFFFFFF;
				DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_ISR_CLR_CR_DISEQC_SLV_FIFO_FULL_CLR,0);
				DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_ISR_CLR_CR_DISEQC_RX_XACTION_CLR,0);
				DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_ISR_CLR_CR_DISEQC_XACTION_FAIL_CLR,0);
				WRITE_REG(ui32Base,DISEQC_CR_DISEQC_ISR_CLR,ui32Reg);
			}

			else
			{
				/* Shouldn't get here, IMR should be masking out other interrupts */
				IMG_ASSERT(0);
				return;
			}
		}




		/* Expected transaction */
		else
		{
			switch(psCurOperation->opcode)
			{
			case DISEQC_OPCODE_END_CONTINUOUS_TONE:

				/* Post end continuous tone silence complete? */
				if(DISEQC_INTERRUPT_ACTIVE(ui32ISR,DISEQC_CR_DISEQC_ISR_CR_DISEQC_XACTION_DONE))
				{
					DISEQCDriverClearNonIdleInterrupts(ui32Base);
					DISEQCDriverCompleteOperation(ui32Base,psDevice,psPort);
				}

				else
				{
					/* Shouldn't get here, IMR should be masking out other interrupts */
					IMG_ASSERT(0);
					return;
				}

				break;




			case DISEQC_OPCODE_TONE_BURST_A:
			case DISEQC_OPCODE_TONE_BURST_B:

				/* Tone burst not done as unexpected message is being received? */
				if(DISEQC_INTERRUPT_ACTIVE(ui32ISR,DISEQC_CR_DISEQC_ISR_CR_DISEQC_XACTION_NOT_DONE))
				{
					DISEQCDriverClearNonIdleInterrupts(ui32Base);
					DISEQCDriverDelayOperation(ui32Base,psPort);
				}

				/* Tone bust complete (automatic post tone burst silence also complete)? */
				else if(DISEQC_INTERRUPT_ACTIVE(ui32ISR,DISEQC_CR_DISEQC_ISR_CR_DISEQC_XACTION_DONE))
				{	
					DISEQCDriverClearNonIdleInterrupts(ui32Base);
					DISEQCDriverCompleteOperation(ui32Base,psDevice,psPort);
				}

				else
				{
					/* Shouldn't get here, IMR should be masking out other interrupts */
					IMG_ASSERT(0);
					return;
				}

				break;




			case DISEQC_OPCODE_SEND_MESSAGE:

				/* Send message not done as unexpected message is being received? */
				if(DISEQC_INTERRUPT_ACTIVE(ui32ISR,DISEQC_CR_DISEQC_ISR_CR_DISEQC_XACTION_NOT_DONE))
				{
					DISEQCDriverClearNonIdleInterrupts(ui32Base);
					DISEQCDriverDelayOperation(ui32Base,psPort);
				}

				/* Timeout reached while waiting for slave reply? */
				else if(DISEQC_INTERRUPT_ACTIVE(ui32ISR,DISEQC_CR_DISEQC_ISR_CR_DISEQC_SLV_REPLY_TIMEOUT))
				{
					/* Slave reply timeouts require a reset action */
					DISEQC_MODIFY_REG_FIELD(ui32Base,DISEQC_CR_DISEQC_CFG,DISEQC_CR_DISEQC_CFG_CR_DISEQC_EN,0);
					DISEQC_SET_REG(ui32Base,DISEQC_CR_DISEQC_ACTION,DISEQC_CR_DISEQC_ACTION_CR_DISEQC_RESET,1);
					DISEQC_SET_REG(ui32Base,DISEQC_CR_DISEQC_ACTION,DISEQC_CR_DISEQC_ACTION_CR_DISEQC_RESET,0);
					DISEQC_MODIFY_REG_FIELD(ui32Base,DISEQC_CR_DISEQC_CFG,DISEQC_CR_DISEQC_CFG_CR_DISEQC_EN,1);

					WRITE_REG(ui32Base,DISEQC_CR_DISEQC_ISR_CLR,0xFFFFFFFF);

					psPort->Status = DISEQC_STATUS_ERR_RECEIVE_TIMEOUT;

					/* Complete the silence requirement */
					DISEQCDriverCompleteSilence(ui32Base,psDevice,psPort,psPort->ui32SlaveResponseTimeout,DISEQCDriverCompleteOperation);
				}

				/* Master FIFO empty for too long? */
				else if(DISEQC_INTERRUPT_ACTIVE(ui32ISR,DISEQC_CR_DISEQC_ISR_CR_DISEQC_MST_FIFO_EMPTY_TIMEOUT))
				{
					WRITE_REG(ui32Base,DISEQC_CR_DISEQC_ISR_CLR,0xFFFFFFFF);

					psPort->Status = DISEQC_STATUS_ERR_SEND_TIMEOUT;

					/* Complete the silence requirement */
					DISEQCDriverCompleteSilence(ui32Base,psDevice,psPort,psPort->ui32MasterEmptyTimeout,DISEQCDriverCompleteOperation);
				}
			
				/* Slave FIFO full and another write attempted (data lost)? */
				else if(DISEQC_INTERRUPT_ACTIVE(ui32ISR,DISEQC_CR_DISEQC_ISR_CR_DISEQC_SLV_FIFO_FULL))
				{
					DISEQCDriverClearSlaveFIFO(ui32Base);
					psPort->Status = DISEQC_STATUS_ERR_RECEIVE_OVERFLOW;

					/* Some data has been lost, no point in receiving further data. Enable completed and failed interrupts only */
					ui32Reg = 0xFFFFFFFF;
					DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_XACTION_DONE_MASK,0);
					DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_XACTION_FAIL_MASK,0);
					WRITE_REG(ui32Base,DISEQC_CR_DISEQC_IMR,ui32Reg);
				}

				/* Message operation completed successfully? */
				else if(DISEQC_INTERRUPT_ACTIVE(ui32ISR,DISEQC_CR_DISEQC_ISR_CR_DISEQC_XACTION_DONE))
				{
					/* Post message silence complete */
					if(psPort->bSilence)
					{
						DISEQCDriverClearNonIdleInterrupts(ui32Base);
						DISEQCDriverCompleteOperation(ui32Base,psDevice,psPort);
					}
					else
					{
						/* Message with reply complete? */
						if(psIOB->pui8RecvBuf!=IMG_NULL)
						{
							/* Receive overflow occurred at some point during the operation? */
							if(psPort->Status==DISEQC_STATUS_ERR_RECEIVE_OVERFLOW)
							{
								DISEQCDriverClearSlaveFIFO(ui32Base);
							}
							else
							{
								ui8BytesReceived = DISEQC_GET_REG(ui32Base,DISEQC_CR_DISEQC_MSG_INFO,DISEQC_CR_DISEQC_MSG_INFO_CR_DISEQC_SLV_LEN);
								psPort->ui8TotalReceived = ui8BytesReceived;

								if(psIOB->pui8Received!=IMG_NULL){*psIOB->pui8Received = ui8BytesReceived;}

								ui8Remaining = ui8BytesReceived - psPort->ui8Received;
								ui8RemainingBuffer = psIOB->ui8RecvBufSize - psPort->ui8Received;

								/* Less space in buffer than bytes in FIFO? */
								if(ui8Remaining>ui8RemainingBuffer)
								{
									DISEQCDriverGetSlaveData(ui32Base,psIOB->pui8RecvBuf+psPort->ui8Received,ui8RemainingBuffer);
									DISEQCDriverClearSlaveFIFO(ui32Base);
								}
								else
								{
									DISEQCDriverGetSlaveData(ui32Base,psIOB->pui8RecvBuf+psPort->ui8Received,ui8Remaining);
								}
							}
			
							WRITE_REG(ui32Base,DISEQC_CR_DISEQC_ISR_CLR,0xFFFFFFFF);

							/* Complete the silence requirement */
							DISEQCDriverCompleteSilence(ui32Base,psDevice,psPort,psPort->ui32PostSlaveMessageSilence,DISEQCDriverCompleteOperation);
						}
						/* Message without reply complete */
						else
						{
							WRITE_REG(ui32Base,DISEQC_CR_DISEQC_ISR_CLR,0xFFFFFFFF);

							DISEQCDriverCompleteOperation(ui32Base,psDevice,psPort);
						}
					}
				}

				/* Receive failed? */
				else if(DISEQC_INTERRUPT_ACTIVE(ui32ISR,DISEQC_CR_DISEQC_ISR_CR_DISEQC_XACTION_FAIL))
				{
					DISEQCDriverClearSlaveFIFO(ui32Base);

					DISEQC_MODIFY_REG_FIELD(ui32Base,DISEQC_CR_DISEQC_CFG,DISEQC_CR_DISEQC_CFG_CR_DISEQC_EN,0);
					DISEQC_SET_REG(ui32Base,DISEQC_CR_DISEQC_ACTION,DISEQC_CR_DISEQC_ACTION_CR_DISEQC_RESET,1);
					DISEQC_SET_REG(ui32Base,DISEQC_CR_DISEQC_ACTION,DISEQC_CR_DISEQC_ACTION_CR_DISEQC_RESET,0);
					DISEQC_MODIFY_REG_FIELD(ui32Base,DISEQC_CR_DISEQC_CFG,DISEQC_CR_DISEQC_CFG_CR_DISEQC_EN,1);

					psPort->Status = DISEQC_STATUS_ERR_RECEIVE;

					WRITE_REG(ui32Base,DISEQC_CR_DISEQC_ISR_CLR,0xFFFFFFFF);
					
					/* Complete the silence requirement */
					DISEQCDriverCompleteSilence(ui32Base,psDevice,psPort,0,DISEQCDriverCompleteOperation);
				}

				/* FIFO watermark reached? */
				else if(DISEQC_INTERRUPT_ACTIVE(ui32ISR,DISEQC_CR_DISEQC_ISR_CR_DISEQC_FIFO_WM_REACHED))
				{
					/* Send FIFO empty */
					if(psCurOperation->counter>DISEQC_FIFO_LENGTH)
					{
						psCurOperation->counter -= DISEQC_FIFO_LENGTH;
						psCurOperation->pointer = (IMG_BYTE *)psCurOperation->pointer + DISEQC_FIFO_LENGTH;
						
						if(psCurOperation->counter>DISEQC_FIFO_LENGTH){ui8Remaining = DISEQC_FIFO_LENGTH;}
						else{ui8Remaining = psCurOperation->counter;}
						DISEQCDriverSetMasterData(ui32Base,(IMG_BYTE *)psCurOperation->pointer,ui8Remaining);
					}
					/* Receive FIFO full */
					else if(psIOB->pui8RecvBuf != IMG_NULL)
					{
						ui8RemainingBuffer = psIOB->ui8RecvBufSize-psPort->ui8Received;
						if(ui8RemainingBuffer<DISEQC_FIFO_LENGTH)
						{
							/* More data received than there is space in the buffer. Fill the buffer then clear the FIFO */
							DISEQCDriverGetSlaveData(ui32Base,psIOB->pui8RecvBuf+psPort->ui8Received,ui8RemainingBuffer);
							DISEQCDriverClearSlaveFIFO(ui32Base);
							psPort->ui8Received += ui8RemainingBuffer;
						}
						else
						{
							/* Space is available in the buffer for all the data */
							DISEQCDriverGetSlaveData(ui32Base,psIOB->pui8RecvBuf+psPort->ui8Received,DISEQC_FIFO_LENGTH);
							psPort->ui8Received += DISEQC_FIFO_LENGTH;
						}
					}
					DISEQC_SET_REG(ui32Base,DISEQC_CR_DISEQC_ISR_CLR,DISEQC_CR_DISEQC_ISR_CLR_CR_DISEQC_FIFO_WM_REACHED_CLR,1);
				}

				else
				{
					/* Shouldn't get here, IMR should be masking out other interrupts */
					IMG_ASSERT(0);
					return;
				}

				break;




			default:
				/* Shouldn't get here, invalid opcode */
				IMG_ASSERT(0);
				return;
			}
		}
	}
}





static IMG_VOID DISEQCDriverClearNonIdleInterrupts(IMG_UINT32 ui32Base)
{
	IMG_UINT32 ui32Reg;

	ui32Reg = 0xFFFFFFFF;
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_ISR_CLR_CR_DISEQC_SLV_FIFO_FULL_CLR,0);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_ISR_CLR_CR_DISEQC_FIFO_WM_REACHED_CLR,0);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_ISR_CLR_CR_DISEQC_RX_XACTION_CLR,0);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_ISR_CLR_CR_DISEQC_XACTION_FAIL_CLR,0);
	WRITE_REG(ui32Base,DISEQC_CR_DISEQC_ISR_CLR,ui32Reg);
}

static IMG_VOID DISEQCDriverSetIdleIMR(IMG_UINT32 ui32Base, DISEQC_PORT_T *psPort)
{
	IMG_UINT32 ui32Reg;

	ui32Reg = 0xFFFFFFFF;
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_RX_XACTION_MASK,0);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_XACTION_FAIL_MASK,0);

	/* Don't need to receive the data if no buffer to hold it */
	if(psPort->pui8UnexpectedData)
	{
		//DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_FIFO_WM_REACHED_MASK,0);
		DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_SLV_FIFO_FULL_MASK,0);
	}

	WRITE_REG(ui32Base,DISEQC_CR_DISEQC_IMR,ui32Reg);
}

static IMG_VOID DISEQCDriverSetIdle(IMG_UINT32 ui32Base, DISEQC_PORT_T *psPort)
{
	DISEQCDriverSetIdleIMR(ui32Base,psPort);

	psPort->psCurOperation = IMG_NULL;
	psPort->psDelayOperation = IMG_NULL;
	psPort->bSilence = IMG_FALSE;
	psPort->ui8Received = 0;
	psPort->Status = DISEQC_STATUS_OK;
	psPort->ui8TotalReceived = 0;
}

static IMG_VOID DISEQCDriverCompleteOperation(IMG_UINT32 ui32Base, QIO_DEVICE_T *psDevice, DISEQC_PORT_T *psPort)
{
	psPort->psCurOperation->counter = psPort->ui8TotalReceived;
	psPort->psCurOperation->opcode = psPort->Status;
	DISEQCDriverSetIdle(ui32Base,psPort);
	QIO_complete(psDevice,QIO_IOCOMPLETE);
	QIO_start(psDevice);
}

static IMG_VOID DISEQCDriverDelayOperation(IMG_UINT32 ui32Base, DISEQC_PORT_T *psPort)
{
	psPort->psDelayOperation = psPort->psCurOperation;
	psPort->psCurOperation = IMG_NULL;

	DISEQCDriverSetIdleIMR(ui32Base,psPort);
}

static IMG_VOID DISEQCDriverSetMasterData(IMG_UINT32 ui32Base, IMG_BYTE *pui8Ptr, IMG_UINT8 ui8NumBytes)
{
	IMG_UINT32 ui32Reg;

	while(1)
	{
		switch(ui8NumBytes)
		{
		case 0:
			return;
		case 1:
			ui32Reg = pui8Ptr[0];
			WRITE_REG(ui32Base,DISEQC_CR_DISEQC_MSG_MASTER,ui32Reg);
			return;
		case 2:
			ui32Reg = pui8Ptr[0]|(pui8Ptr[1]<<8);
			WRITE_REG(ui32Base,DISEQC_CR_DISEQC_MSG_MASTER,ui32Reg);
			return;
		case 3:
			ui32Reg = pui8Ptr[0]|(pui8Ptr[1]<<8)|(pui8Ptr[2]<<16);
			WRITE_REG(ui32Base,DISEQC_CR_DISEQC_MSG_MASTER,ui32Reg);
			return;
		default:
			ui32Reg = *(IMG_UINT32 *)pui8Ptr;
			WRITE_REG(ui32Base,DISEQC_CR_DISEQC_MSG_MASTER,ui32Reg);
			pui8Ptr += 4;
			ui8NumBytes -= 4;
			break;
		}
	}
}

static IMG_VOID DISEQCDriverSendMessage(DISEQC_PORT_T *psPort, QIO_IOPARS_T *psIOParams, IMG_UINT32 ui32Base)
{
	IMG_UINT32			ui32Reg;
	DISEQC_IO_BLOCK_T	*psIOB;
	IMG_UINT8			ui8Remaining;

	psIOB = (DISEQC_IO_BLOCK_T *)psIOParams->spare;

	if(psIOParams->counter>DISEQC_FIFO_LENGTH){ui8Remaining = DISEQC_FIFO_LENGTH;}
	else{ui8Remaining = psIOParams->counter;}

	if(!psPort->bMasterData)
	{
		/* If data hasn't previously been written to the master FIFO, write it (for an operation that was delayed via 
		   XACTION_NOT_DONE the data will still be in the FIFO) */
		DISEQCDriverSetMasterData(ui32Base,(IMG_BYTE *)psIOParams->pointer,ui8Remaining);
		psPort->bMasterData = IMG_TRUE;
	}

	/* Master message length always needs updating */
	DISEQC_MODIFY_REG_FIELD(ui32Base,DISEQC_CR_DISEQC_MSG_INFO,DISEQC_CR_DISEQC_MSG_INFO_CR_DISEQC_MST_LEN,psIOParams->counter);

	if(psIOB->pui8RecvBuf!=IMG_NULL)
	{
		/* Message with reply. Update bytes received with 0 and set the automatic post transaction timer to 0 so a receive
		   will begin immediately after the message is sent. The silence requirement will be met using the timer if necessary */
		if(psIOB->pui8Received!=IMG_NULL){*psIOB->pui8Received = 0;}

		DISEQC_MODIFY_REG_FIELD(ui32Base,DISEQC_CR_DISEQC_SILENCE_TIMER,DISEQC_CR_DISEQC_SILENCE_TIMER_CR_DISEQC_POST_TIMER,0);

		ui32Reg = 0;
		DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_ACTION_CR_DISEQC_RPLY_EXP,1);
		DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_ACTION_CR_DISEQC_START,1);
		WRITE_REG(ui32Base,DISEQC_CR_DISEQC_ACTION,ui32Reg);
	}
	else
	{
		/* Message without reply. Let the automatic post transaction timer meet the silence requirement for the bus */
		DISEQC_MODIFY_REG_FIELD(ui32Base,DISEQC_CR_DISEQC_SILENCE_TIMER,DISEQC_CR_DISEQC_SILENCE_TIMER_CR_DISEQC_POST_TIMER,psPort->ui32PostTransactionSilence);

		DISEQC_SET_REG(ui32Base,DISEQC_CR_DISEQC_ACTION,DISEQC_CR_DISEQC_ACTION_CR_DISEQC_START,1);
	}

	ui32Reg = 0xFFFFFFFF;
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_SLV_REPLY_TIMEOUT_MASK,0);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_MST_FIFO_EMPTY_TIMEOUT_MASK,0);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_SLV_FIFO_FULL_MASK,0);
	//DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_FIFO_WM_REACHED_MASK,0);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_XACTION_NOT_DONE_MASK,0);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_XACTION_FAIL_MASK,0);
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_XACTION_DONE_MASK,0);
	WRITE_REG(ui32Base,DISEQC_CR_DISEQC_IMR,ui32Reg);
}

static IMG_VOID DISEQCDriverDoTimer(IMG_UINT32 ui32Base, IMG_UINT32 ui32Time)
{
	IMG_UINT32	ui32Reg;

	DISEQC_MODIFY_REG_FIELD(ui32Base,DISEQC_CR_DISEQC_SILENCE_TIMER,DISEQC_CR_DISEQC_SILENCE_TIMER_CR_DISEQC_STANDALONE_TIMER,ui32Time);

	DISEQC_SET_REG(ui32Base,DISEQC_CR_DISEQC_ACTION,DISEQC_CR_DISEQC_ACTION_CR_DISEQC_TIMER,1);

	ui32Reg = 0xFFFFFFFF;
	DISEQC_WRITE_REG_FIELD(ui32Reg,DISEQC_CR_DISEQC_IMR_CR_DISEQC_XACTION_DONE_MASK,0);
	WRITE_REG(ui32Base,DISEQC_CR_DISEQC_IMR,ui32Reg);
}

static IMG_VOID DISEQCDriverClearSlaveFIFO(IMG_UINT32 ui32Base)
{
	IMG_UINT32 ui32Reg;

	while(DISEQC_GET_REG(ui32Base,DISEQC_CR_DISEQC_STATUS,DISEQC_CR_DISEQC_STATUS_CR_DISEQC_SLV_FIFO_READY))
	{
		ui32Reg = READ_REG(ui32Base,DISEQC_CR_DISEQC_MSG_SLAVE);
	}
}

static IMG_VOID DISEQCDriverGetSlaveData(IMG_UINT32 ui32Base, IMG_BYTE *pui8Ptr, IMG_UINT8 ui8NumBytes)
{
	IMG_UINT32 ui32Reg;

	while(1)
	{
		switch(ui8NumBytes)
		{
		case 0:
			return;
		case 1:
			ui32Reg = READ_REG(ui32Base,DISEQC_CR_DISEQC_MSG_SLAVE);
			pui8Ptr[0] = ui32Reg&0xFF;
			return;
		case 2:
			ui32Reg = READ_REG(ui32Base,DISEQC_CR_DISEQC_MSG_SLAVE);
			pui8Ptr[0] = ui32Reg&0xFF;
			pui8Ptr[1] = (ui32Reg>>8)&0xFF;
			return;
		case 3:
			ui32Reg = READ_REG(ui32Base,DISEQC_CR_DISEQC_MSG_SLAVE);
			pui8Ptr[0] = ui32Reg&0xFF;
			pui8Ptr[1] = (ui32Reg>>8)&0xFF;
			pui8Ptr[2] = (ui32Reg>>16)&0xFF;
			return;
		default:
			ui32Reg = READ_REG(ui32Base,DISEQC_CR_DISEQC_MSG_SLAVE);
			*(IMG_UINT32 *)pui8Ptr = ui32Reg;
			pui8Ptr += 4;
			ui8NumBytes -= 4;
			break;
		}
	}
}

static IMG_VOID DISEQCDriverCompleteUnexpectedReceive(IMG_UINT32 ui32Base, QIO_DEVICE_T *psDevice, DISEQC_PORT_T *psPort)
{
	/* Complete unexpected receive. Not a QIO operation so call the callback directly and no QIO_complete/QIO_start */

	if(psPort->pfnReadUnexpectedCallback!=IMG_NULL)
	{
		psPort->pfnReadUnexpectedCallback(psPort->pui8UnexpectedData,psPort->ui8TotalReceived,psPort->Status);
	}

	if(psPort->psDelayOperation!=IMG_NULL)
	{
		/* Start an operation that was delayed for the unexpected receive */
		DISEQCDriverStart(psDevice,psPort->psDelayOperation);
	}
	else
	{
		/* Nothing to do, set to idle state */
		DISEQCDriverSetIdle(ui32Base,psPort);
	}
}

static IMG_VOID DISEQCDriverCompleteSilence(IMG_UINT32 ui32Base, QIO_DEVICE_T *psDevice, DISEQC_PORT_T *psPort, IMG_UINT32 SilenceDone, IMG_VOID (*CompleteFunction)(IMG_UINT32,QIO_DEVICE_T *,DISEQC_PORT_T *))
{
	if((psPort->ui32PostTransactionSilence == 0) || (psPort->ui32PostTransactionSilence <= SilenceDone))
	{
		/* No further waiting required */
		CompleteFunction(ui32Base,psDevice,psPort);
	}
	else
	{
		/* Additional silence is required on the bus */
		psPort->bSilence = IMG_TRUE;
		DISEQCDriverDoTimer(ui32Base,psPort->ui32PostTransactionSilence-SilenceDone);
	}
}
