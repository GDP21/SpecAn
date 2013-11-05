/*!
*******************************************************************************
  file   sdios_drv.c

  brief  SDIOS Device Driver

         This file defines the functions that make up the SDIOS device driver.

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

 \n<b>Platform:</b>\n


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
#include <MeOS.h>

/* System */
#include <assert.h>

#include "ioblock_defs.h"
#include "ioblock_utils.h"

#include "gdma_api.h"

/* SDIOS Slave Driver */
#include "sdios_api.h"
#include "sdios_drv.h"
#include "sdios_reg.h"

#if defined USE_EVENT_LOGGING
#include "sdios_eventlogging.h"
#endif   /* USE_EVENT_LOGGING */


#if defined ENABLE_SDIOS_DBG_COMMENTS
#include <stdio.h> /* for sprintf */
#endif   /* ENABLE_SDIOS_DBG_COMMENTS */


/* ------------------------------------------------------------------------ */
/*                                                                          */
/*                         MACRO DEFINITIONS                                */
/*                                                                          */
/* ------------------------------------------------------------------------ */
#if defined USE_EVENT_LOGGING
                                        #define SDIOS_EVENTLOG(e, d0, d1) Add_SDIOS_EventLog(e, d0, d1 )
#else
                                        #define SDIOS_EVENTLOG(e, d0, d1) (void)e // do nothing
#endif /* USE_EVENT_LOGGING */

#if defined ENABLE_SDIOS_DBG_COMMENTS
                                        #define SDIO_DBG_PRINT_DMA_PARAMS(a) prv_DBG_Print_DMA_Params((a))
                                        #define SDIO_DBG_PRINT_TFR_PARAMS(a) prv_DBG_Print_TFR_Params((a))
#else
                                        #define SDIO_DBG_PRINT_DMA_PARAMS(a)
                                        #define SDIO_DBG_PRINT_TFR_PARAMS(a)
#endif /* ENABLE_SDIOS_DBG_COMMENTS */

#define SDIOS_FIFO_WIDTH_BYTES          4
#define SDIOS_SND_THRESHOLD_MAX         7
#define SDIOS_BASE_ADDR                 (psInternals->ui32BaseAddress)

#define SDIO_MAX_CMD53_BLOCK_SIZE                512
#define SDIOS_F1_SPACE_EXT_REG_START_OFFSET     (0x18)
#define SDIOS_F1_SPACE_EXT_REG_SIZE             (0x20000 - SDIOS_F1_SPACE_EXT_REG_START_OFFSET)

#define SDIOS_TFR_BASE_ADDR_SELECT   IMG_SDIO_F1_32_BIT_REG_0_OFFSET
#define SDIOS_TFR_SEM_REG            IMG_SDIO_F1_32_BIT_REG_1_OFFSET
#define SDIOS_TFR_SEM_REG_CMD53_BIT  (0x1 << 0)

#define SDIO_DMA_BURST_SIZE 4  /* 4 Words - SDIO Hardware requirement */

/* Macros used to read/write register fields to integer variables  */
#define ReadRegField(regval,field)                                  \
(                                                                   \
    ((field##_MASK) == 0xFFFFFFFF) ?                                \
    (                                                               \
        (regval)                                                    \
    )                                                               \
    :                                                               \
    (                                                               \
        ((regval) & (field##_MASK)) >> (field##_SHIFT)              \
    )                                                               \
)

#define WriteRegField(regval,field,val)                             \
(                                                                   \
    ((field##_MASK) == 0xFFFFFFFF) ?                                \
    (                                                               \
        (val)                                                     \
    )                                                               \
    :                                                               \
    (                                                               \
        (                                                           \
            (regval) & ~(field##_MASK)                            \
        ) |                                                         \
        (                                                           \
            ((val) << (field##_SHIFT)) & (field##_MASK)           \
        )                                                           \
    )                                                               \
)



/* ------------------------------------------------------------------------ */
/*                                                                          */
/*                              TYPE DEFINITIONS                            */
/*                                                                          */
/* ------------------------------------------------------------------------ */
#if defined SDIOS_ISR_STATS

typedef struct SDIOS_Isr_Stats_
{
    unsigned int ISR_EntryCount;                     // Number of times the ISR has been called
    unsigned int DMA_InterruptCount;                 // Number of times the ISR has been called due to a DMA interrupt
    unsigned int SDIOS_CoreInterruptCount;           // Number of times the ISR has been called due to a Core interrupt
    unsigned int SpuriousDmaInterruptCount;          // Number of times the ISR has been called due to a DMA interrupt with no apparent reason
    unsigned int SpuriousCoreInterruptCount;         // Number of times the ISR has been called due to a Core interrupt with no apparent reason
    unsigned int QIO_StartCount;                     // Number of times the function QIO_Start has been called
    unsigned int QIO_CompleteCount;                  // Number of times the function QIO_Complete has been called
    unsigned int SDIOS_LargestNumOfCoreIntLoopsDone; // Records the maximum number of times the core interrupt has looped during one ISR call.
    unsigned int SDIOS_CoreInterruptLoopProfile[10]; // [1] contains number of instances where the loop was execued once,
                                                     // [2] contains number of instances where the loop was execued twice,
                                                     // etc..
                                                     // [9] contains the number of instances where the loop executed 9 times OR MORE
}SDIOS_Isr_Stats;

#endif /* defined SDIOS_ISR_STATS */

/* ------------------------------------------------------------------------ */
/*                                                                          */
/*                         FUNCTION PROTOTYPES                              */
/*                                                                          */
/* ------------------------------------------------------------------------ */

/* External prototypes */
int  SDIOS_drv_init         (QIO_DEVICE_T *dev, QIO_DEVPOWER_T *pwrClass, int *devRank, unsigned intMasks[]);
void SDIOS_drv_start        (QIO_DEVICE_T *dev, QIO_IOPARS_T *ioPars);
void SDIOS_drv_isr          (QIO_DEVICE_T *dev);

/* Internal prototypes */

/*
	Changed all internal functions such that they are all passed the I/O block descriptor pointer
	from which all other data can be accessed.
*/

static void  prv_handle_trn_update_interrupt                     ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_handle_blk_complete_interrupt                   ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_handle_trn_complete_interrupt                   ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_handle_trn_abort_interrupt                      ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_handle_trn_suspend_interrupt                    ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_handle_CRC_interrupt                            ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_handle_FIFO_overflow_interrupt                  ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_handle_FIFO_underflow_interrupt                 ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_handle_F0_update_interrupt                      ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_handle_F1_update_interrupt                      ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_ConfigureDmaChannel                             ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_ResetTransferControlData                        ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_ResetStatistics                                 ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_SetupDmaForTransfer                             ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_handle_sdio_core_interrupts                     ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_handle_dmac_interrupt                           ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );

#if defined  ENABLE_SDIOS_DBG_COMMENTS
static void  prv_DBG_Print_DMA_Params                            ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
static void  prv_DBG_Print_TFR_Params                            (unsigned int uiTfrCmd);
#endif /* ENABLE_SDIOS_DBG_COMMENTS */

#if defined SDIOS_ISR_STATS
static void  prv_updateLoopProfile                               (ioblock_sBlockDescriptor *	psIOBlockDescriptor , unsigned int uiNumLoops);
#endif /* SDIOS_ISR_STATS */


/* ------------------------------------------------------------------------ */
/*                                                                          */
/*                              GLOBAL DATA                                 */
/*                                                                          */
/* ------------------------------------------------------------------------ */

extern ioblock_sBlockDescriptor *	apsSDIOS_Descriptor[ MAX_SDIOS_NUM_BLOCKS ];

const  QIO_DRIVER_T SDIOS_driver =
{
    SDIOS_drv_isr,              /* ISR                       */
    SDIOS_drv_init,             /* init function             */
    SDIOS_drv_start,            /* start function            */
    NULL,                       /* no cancel function        */
    NULL,                       /* no power control function */
    NULL,                       /* no sim start function     */
    NULL                        /* no shut function          */
};


/* ------------------------------------------------------------------------ */
/*                                                                          */
/*                              Statistics                                  */
/*                                                                          */
/* ------------------------------------------------------------------------ */
#if defined SDIOS_ISR_STATS
    #define SDIOS_STATS_INCREMENT_ISR_ENTRY_COUNT               psInternals->sStats.sIsrStats.ISR_EntryCount++
    #define SDIOS_STATS_INCREMENT_DMA_INTERRUPT_COUNT           psInternals->sStats.sIsrStats.DMA_InterruptCount++
    #define SDIOS_STATS_INCREMENT_CORE_INTERRUPT_COUNT          psInternals->sStats.sIsrStats.SDIOS_CoreInterruptCount++
    #define SDIOS_STATS_INCREMENT_SPURIOUS_DMA_INTERRUPT_COUNT  psInternals->sStats.sIsrStats.SpuriousDmaInterruptCount++
    #define SDIOS_STATS_INCREMENT_SPURIOUS_CORE_INTERRUPT_COUNT psInternals->sStats.sIsrStats.SpuriousCoreInterruptCount++
    #define SDIOS_STATS_INCREMENT_QIO_START_COUNT               psInternals->sStats.sIsrStats.QIO_StartCount++
    #define SDIOS_STATS_INCREMENT_QIO_COMPLETE_COUNT            psInternals->sStats.sIsrStats.QIO_CompleteCount++
    #define SDIOS_STATS_UPDATE_LOOP_PROFILE(intern, numLoops)   prv_updateLoopProfile(intern, numLoops)
#else
    #define SDIOS_STATS_INCREMENT_ISR_ENTRY_COUNT
    #define SDIOS_STATS_INCREMENT_DMA_INTERRUPT_COUNT
    #define SDIOS_STATS_INCREMENT_CORE_INTERRUPT_COUNT
    #define SDIOS_STATS_INCREMENT_SPURIOUS_DMA_INTERRUPT_COUNT
    #define SDIOS_STATS_INCREMENT_SPURIOUS_CORE_INTERRUPT_COUNT
    #define SDIOS_STATS_INCREMENT_QIO_START_COUNT
    #define SDIOS_STATS_INCREMENT_QIO_COMPLETE_COUNT
    #define SDIOS_STATS_UPDATE_LOOP_PROFILE(intern, numLoops)
#endif /* defined SDIOS_ISR_STATS */



/* ------------------------------------------------------------------------ */
/*                                                                          */
/*                        FUNCTION DEFINITIONS                              */
/*                                                                          */
/* ------------------------------------------------------------------------ */

/*================================================================================*/
/*  SDIOS_DMAComplete                                                             */
/*																				  */
/*  Callback invoked by the GDMA driver when a DMA operation completes.  The	  */
/*	pvParam passed in is the address of the I/O block descriptor for this		  */
/*	instance of the SDIOS block.												  */
/*																				  */
/*================================================================================*/
IMG_RESULT SDIOS_DMAComplete( GDMA_sTransferObject	*	psTransfer, QIO_STATUS_T	eQIOStatus, IMG_VOID * pvParam )
{
	IMG_RESULT					rResult = IMG_SUCCESS;
    ioblock_sBlockDescriptor *	psIOBlockDescriptor;
	SDIOS_Internals *			psInternals;
	SDIOS_PORT_T *				psPort;
	GDMA_sContext *				psDMAContext;
	QIO_DEVICE_T *				dev;

	/* Set up useful local pointers */
    psIOBlockDescriptor	= (ioblock_sBlockDescriptor *)pvParam;
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);
	psDMAContext	= &(psInternals->sDMAContext);
    dev				= psInternals->dev;

	prv_handle_dmac_interrupt( psIOBlockDescriptor );
    prv_ResetTransferControlData( psIOBlockDescriptor );

	/*
		The DMA completion event is our cue that the transfer has finished and so we
		can do the QIO completion things
	*/
    QIO_complete(dev, QIO_IOCOMPLETE);
    SDIOS_STATS_INCREMENT_QIO_COMPLETE_COUNT;
    QIO_start(dev);

	return (rResult);
}


/*================================================================================*/
/*  SDIOS_drv_init                                                                */
/*================================================================================*/
int  SDIOS_drv_init (QIO_DEVICE_T *dev, QIO_DEVPOWER_T *pwrClass, int *devRank, unsigned intMasks[])
{
    SDIOS_INIT_PARAM_T *		psInitParam;
    unsigned int				uiReg;
    unsigned int				i;
    int							lockState;
    ioblock_sBlockDescriptor *	psIOBlockDescriptor;
	SDIOS_Internals *			psInternals;
	SDIOS_PORT_T *				psPort;

    SDIOS_EVENTLOG(1, 2, 3);

	/* Set up useful local pointers */
    psInitParam			= (SDIOS_INIT_PARAM_T *)dev->id;
    psIOBlockDescriptor	= apsSDIOS_Descriptor[ psInitParam->ui32BlockIndex ];
    psPort				= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals			= &(psPort->sDriverInternals);

	/* Save dev for later use */
	psInternals->dev	= dev;

	/* Initialise the data lock */
    KRN_initSemaphore(&psInternals->tDataSem, 1);

    /* Reset internal data */
    prv_ResetTransferControlData( psIOBlockDescriptor );
    prv_ResetStatistics( psIOBlockDescriptor );

    /* No power saving */
    *pwrClass = QIO_POWERNONE;

    /* Only a single rank */
    *devRank = 1;

    /* Enable CRC corruption on FIFO Over & underflows & set send threshold. */
    Read( psIOBlockDescriptor->ui32Base, IMG_SDIO_SDIO_CONFIGURATION_OFFSET, &uiReg);
   	uiReg = WriteRegField(uiReg, IMG_SDIO_SDIO_CONFIGURATION_SDIO_EDGE_SEL, psInitParam->bClockDataOnTrailingEdge);
    uiReg = WriteRegField(uiReg, IMG_SDIO_SDIO_CONFIGURATION_CORRUPT_CRC_OVFL, 1);
    uiReg = WriteRegField(uiReg, IMG_SDIO_SDIO_CONFIGURATION_CORRUPT_CRC_UNFL, 1);
    uiReg = WriteRegField(uiReg, IMG_SDIO_SDIO_CONFIGURATION_SDIO_SND_THRESHOLD, SDIOS_SND_THRESHOLD_MAX);
    Write(psIOBlockDescriptor->ui32Base, IMG_SDIO_SDIO_CONFIGURATION_OFFSET, uiReg);

    /*===================*/
    /* Set up interrupts */
    /*===================*/

	for ( i = 0; i < QIO_MAXVECGROUPS; ++i )
	{
		intMasks[i] = 0;
	}

	/* Calculate interrupt info for SDIO */
	IOBLOCK_CalculateInterruptInformation(	psIOBlockDescriptor );
	for ( i = 0; i < QIO_MAXVECGROUPS; ++i )
	{
		intMasks[i] = psIOBlockDescriptor->ui32IntMasks[i];
	}

	TBI_LOCK(lockState);		/*-------------------------------------------------------------------------------*/

	Read(psIOBlockDescriptor->sDeviceISRInfo.ui32LEVELEXTAddress, 0, &uiReg);

	/* HWLEVELEXT for SDIOS */
	if ( psIOBlockDescriptor->eInterruptLevelType == HWLEVELEXT_LATCHED )
	{
		/* Clear the SDIOS_LEV bit in the HWLEVELEXT */
		uiReg &= ~(psIOBlockDescriptor->sDeviceISRInfo.ui32LEVELEXTMask);
	}
	else
	if ( psIOBlockDescriptor->eInterruptLevelType == HWLEVELEXT_NON_LATCHED )
	{
		/* Set the SDIOS_LEV bit in the HWLEVELEXT */
		uiReg |= (psIOBlockDescriptor->sDeviceISRInfo.ui32LEVELEXTMask);
	}
	else
	{
		/* Unknown InterruptLevelType */
		IMG_ASSERT ( IMG_FALSE );
	}

	Write(psIOBlockDescriptor->sDeviceISRInfo.ui32LEVELEXTAddress, 0, uiReg);

	TBI_UNLOCK(lockState);		/*-------------------------------------------------------------------------------*/

    /* Disable and clear all interrupts */
    Write(psIOBlockDescriptor->ui32Base, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_OFFSET, 0);
    Write(psIOBlockDescriptor->ui32Base, IMG_SDIO_INTERRUPT_CLEAR_REGISTER_OFFSET, 0x3FF);  /* Bits 0 - 9 */

    /* Define the default configuration */
    psInternals->bEnableF1UpdateInterrupts = SDIOS_TRUE;

    /* Flush the FIFO's */
    Write(psIOBlockDescriptor->ui32Base, IMG_SDIO_CLEAR_BUSY_OFFSET, IMG_SDIO_CLEAR_BUSY_FLUSH_INPUT_FIFO_MASK);
    Write(psIOBlockDescriptor->ui32Base, IMG_SDIO_CLEAR_BUSY_OFFSET, IMG_SDIO_CLEAR_BUSY_FLUSH_OUTPUT_FIFO_MASK);

    /* Enable the required interrupts */
    uiReg = 0;
    uiReg = WriteRegField(uiReg, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_EN_INT_TRN_UPDATE,   1);
    uiReg = WriteRegField(uiReg, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_EN_INT_BLK_COMPLETE, 1);
    uiReg = WriteRegField(uiReg, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_EN_INT_TRN_COMPLETE, 1);
    uiReg = WriteRegField(uiReg, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_EN_INT_TRN_ABORT,    1);
    uiReg = WriteRegField(uiReg, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_EN_INT_TRN_CRC_ERR,  1);
    uiReg = WriteRegField(uiReg, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_EN_INT_TRN_SUSPEND,  0);    /* Suspend is not supported */
    uiReg = WriteRegField(uiReg, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_EN_INT_FIFO_OVFL,    1);
    uiReg = WriteRegField(uiReg, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_EN_INT_FIFO_UNFL,    1);
    uiReg = WriteRegField(uiReg, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_EN_INT_F0_UPDATE,    1);
    uiReg = WriteRegField(uiReg, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_EN_INT_F1_UPDATE, psInternals->bEnableF1UpdateInterrupts);
    Write(psIOBlockDescriptor->ui32Base, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_OFFSET, uiReg);

	/* Set up DMA channel h/w */
	psInternals->uiDMAChannel = psInitParam->uiDmaChannel;

    return 0;
}


/*================================================================================*/
/*  SDIOS_drv_start                                                               */
/*================================================================================*/
void SDIOS_drv_start (QIO_DEVICE_T *dev, QIO_IOPARS_T *ioPars)
{
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;
    ioblock_sBlockDescriptor *	psIOBlockDescriptor;

	/* Set up useful local pointers */
    psIOBlockDescriptor	= apsSDIOS_Descriptor[ dev->id ];
    psPort				= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals			= &(psPort->sDriverInternals);

    SDIOS_STATS_INCREMENT_QIO_START_COUNT;

    /*===========================================================================================================*/
    /* We need to store the address and transfer length, ready for the trn_update interrupt to kick off the tfr. */
    /*===========================================================================================================*/

    // Verify that there is not an outstanding operation that would be overwritten, and other sanity checks
    IMG_ASSERT(!psInternals->pucNextOperationBufferAddress);	/* WARNING - a previous transfer has not completed.	*/
    IMG_ASSERT(!psInternals->uiNextOperationDataSizeBytes);		/* WARNING - a previous transfer has not completed.	*/
    IMG_ASSERT(!psInternals->uiNextOperationDMASizeBytes);		/* WARNING - a previous transfer has not completed.	*/
	IMG_ASSERT ( ioPars );										/* Valid pointer?									*/
    IMG_ASSERT(ioPars->counter);								/* Transfer cannot have a zero size					*/
    IMG_ASSERT(ioPars->pointer);								/* Transfer cannot have a null buffer address		*/

    /* Access the transfer parameters and store them. */
    psInternals->pucNextOperationBufferAddress = ioPars->pointer;
    psInternals->uiNextOperationDataSizeBytes  = ioPars->counter;

    /*===========================*/
    /* Prepare for a CMD53 Write */
    /*===========================*/
    if (ioPars->opcode == SDIOS_OPCODE_READ)
    {
        SDIO_DBG_COMMENT(psInternals->ui32BaseAddress, "SDIO Driver: drv_start: preparing for CMD53 write");

        psInternals->eCurrentTransferDirection = SDIOS_TRANSFER_DIR_HOST_TO_CARD;

        if (ioPars->counter % SDIOS_FIFO_WIDTH_BYTES)
        {
            /*
				The number of bytes is NOT an exact multiple of the fifo width, so round up - it is the responsibility of the
				card side app to ensure that the appropriate number of bytes are allocated in the receiving buffer!
			*/
            psInternals->uiNextOperationDMASizeBytes =
                ((ioPars->counter / SDIOS_FIFO_WIDTH_BYTES) * SDIOS_FIFO_WIDTH_BYTES) + SDIOS_FIFO_WIDTH_BYTES;
        }
        else
        {
            /* The number of bytes is an exact multiple of the fifo width, so all of the data can be DMA'd. */
            psInternals->uiNextOperationDMASizeBytes = ioPars->counter;
        }

        prv_SetupDmaForTransfer( psIOBlockDescriptor );
    }
    else
    if (ioPars->opcode  == SDIOS_OPCODE_WRITE)
    {
        unsigned int uiExcessBytes;

        psInternals->eCurrentTransferDirection = SDIOS_TRANSFER_DIR_CARD_TO_HOST;

        /*==============*/
        /* CMD 53 Read  */
        /*==============*/

        /* Prepare for a SDIOS Write (CMD53Read) */

        SDIO_DBG_COMMENT(psInternals->ui32BaseAddress, "SDIO Driver: CMD53 Read, configuring transfer parameters in trn_update");

        /*
			For a write (card -> host), the entire transfer is completed using a DMA from memory to the SDIO TX FIFO.
			If the number of bytes is not a whole number of words (4 bytes), then we must round up.   This is necessary
			because  the DMA transfer to the FIFO is 4 bytes wide.
		*/
        uiExcessBytes = psInternals->uiNextOperationDataSizeBytes % 4;

        if(uiExcessBytes)
        {
            psInternals->uiNextOperationDMASizeBytes = psInternals->uiNextOperationDataSizeBytes + (4 - uiExcessBytes);
        }
        else
        {   /* The number of bytes is a whole number of words. */
            psInternals->uiNextOperationDMASizeBytes = psInternals->uiNextOperationDataSizeBytes;
        }
        prv_SetupDmaForTransfer( psIOBlockDescriptor );
        SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Finished configuring transfer parameters");
    }
    else
    {
        IMG_ASSERT(SDIOS_FALSE);  /* Illegal SDIO opcode. */
    }
}


/*================================================================================*/
/*  SDIOS_drv_isr                                                                 */
/*================================================================================*/
void SDIOS_drv_isr (QIO_DEVICE_T *dev)
{
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;
    ioblock_sBlockDescriptor *	psIOBlockDescriptor;

	/* Set up useful local pointers */
    psIOBlockDescriptor	= apsSDIOS_Descriptor[ dev->id ];
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    SDIOS_STATS_INCREMENT_ISR_ENTRY_COUNT;

	/*
		This ISR only handles SDIOS device interrupts, none of which are directly related to the
		completion of read or write transfer operations.  That is signalled by a DMA channel completion
		event which is handled by the GDMA driver which will call our callback function "SDIOS_DMAComplete".
	*/

	/* This function will loop until all sdio core interrupts have been serviced. */
	prv_handle_sdio_core_interrupts( psIOBlockDescriptor );
}


/*================================================================================*/
/*  prv_handle_dmac_interrupt                                                     */
/*================================================================================*/
static void prv_handle_dmac_interrupt( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    SDIOS_STATS_INCREMENT_DMA_INTERRUPT_COUNT;

	/* Signal that the DMA operation has finished */
    psInternals->bDmaTfrCompleted = SDIOS_TRUE;
    psInternals->sStats.DMA_CompleteCount++;
    SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Received DMA complete interrupt");

    // Flush the cache if needed

    if ( ( psInternals->eCurrentTransferDirection == SDIOS_TRANSFER_DIR_HOST_TO_CARD ) &&
         ( psIOBlockDescriptor->psSystemDescriptor->pfn_FlushCache )
       )
    {
        psIOBlockDescriptor->psSystemDescriptor->pfn_FlushCache( (img_uint32)psInternals->sDMATransferObject.pui8WritePointer,
                                                                             psInternals->sDMATransferObject.ui32SizeInBytes );
    }

}

/*================================================================================*/
/*  prv_handle_sdio_core_interrupts                                               */
/*================================================================================*/
static void prv_handle_sdio_core_interrupts( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
    unsigned int				uiSDIO_InterruptStatus;
    unsigned int				uiSDIO_InterruptEnable;
    unsigned int				uiNumLoops = 0;
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    SDIOS_STATS_INCREMENT_CORE_INTERRUPT_COUNT;

    do
    {
		if ( psIOBlockDescriptor->eInterruptLevelType == HWLEVELEXT_LATCHED )
		{
			unsigned int uiTrigger;
			Read( psIOBlockDescriptor->sDeviceISRInfo.ui32STATEXTAddress, 0, &uiTrigger );

			if ( uiTrigger & psIOBlockDescriptor->sDeviceISRInfo.ui32STATEXTMask )
			{
				/* Write to just the bit that corresponds to SDIO to clear the interrupt */
				Write( psIOBlockDescriptor->sDeviceISRInfo.ui32STATEXTAddress, 0, psIOBlockDescriptor->sDeviceISRInfo.ui32STATEXTMask );
			}
		}

        /* Read the SDIOS Interrupt Status Register */
        Read(psIOBlockDescriptor->ui32Base, IMG_SDIO_INTERRUPT_STATUS_REGISTER_OFFSET, &uiSDIO_InterruptStatus);
        Read(psIOBlockDescriptor->ui32Base, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_OFFSET, &uiSDIO_InterruptEnable);

        /* Only react to enabled interrupts */
        uiSDIO_InterruptStatus &= uiSDIO_InterruptEnable;

        if (uiSDIO_InterruptStatus)
        {
            /* Clear interrupts that have been serviced with one write */
            Write(  psIOBlockDescriptor->ui32Base, IMG_SDIO_INTERRUPT_CLEAR_REGISTER_OFFSET, uiSDIO_InterruptStatus);

            if (uiSDIO_InterruptStatus & IMG_SDIO_INTERRUPT_STATUS_REGISTER_STAT_INT_TRN_ABORT_MASK)   {prv_handle_trn_abort_interrupt(psIOBlockDescriptor);}
            if (uiSDIO_InterruptStatus & IMG_SDIO_INTERRUPT_STATUS_REGISTER_STAT_INT_FIFO_OVFL_MASK)   {prv_handle_FIFO_overflow_interrupt(psIOBlockDescriptor);}
            if (uiSDIO_InterruptStatus & IMG_SDIO_INTERRUPT_STATUS_REGISTER_STAT_INT_FIFO_UNFL_MASK)   {prv_handle_FIFO_underflow_interrupt(psIOBlockDescriptor);}
            if (uiSDIO_InterruptStatus & IMG_SDIO_INTERRUPT_STATUS_REGISTER_STAT_INT_TRN_UPDATE_MASK)  {prv_handle_trn_update_interrupt(psIOBlockDescriptor);}
            if (uiSDIO_InterruptStatus & IMG_SDIO_INTERRUPT_STATUS_REGISTER_STAT_INT_BLK_COMPLETE_MASK){prv_handle_blk_complete_interrupt(psIOBlockDescriptor);}
            if (uiSDIO_InterruptStatus & IMG_SDIO_INTERRUPT_STATUS_REGISTER_STAT_INT_TRN_COMPLETE_MASK){prv_handle_trn_complete_interrupt(psIOBlockDescriptor);}
            if (uiSDIO_InterruptStatus & IMG_SDIO_INTERRUPT_STATUS_REGISTER_STAT_INT_TRN_SUSPEND_MASK) {prv_handle_trn_suspend_interrupt(psIOBlockDescriptor);}
            if (uiSDIO_InterruptStatus & IMG_SDIO_INTERRUPT_STATUS_REGISTER_STAT_INT_F0_UPDATE_MASK)   {prv_handle_F0_update_interrupt(psIOBlockDescriptor);}
            if (uiSDIO_InterruptStatus & IMG_SDIO_INTERRUPT_STATUS_REGISTER_STAT_INT_F1_UPDATE_MASK)   {prv_handle_F1_update_interrupt(psIOBlockDescriptor);}
            if (uiSDIO_InterruptStatus & IMG_SDIO_INTERRUPT_STATUS_REGISTER_STAT_INT_TRN_CRC_ERR_MASK) {prv_handle_CRC_interrupt(psIOBlockDescriptor);}
        }
        else
        {
            /* Spurious interrupt */
            SDIOS_STATS_INCREMENT_SPURIOUS_CORE_INTERRUPT_COUNT;
            return;
        }

        uiNumLoops++;

        Read(psIOBlockDescriptor->ui32Base, IMG_SDIO_INTERRUPT_STATUS_REGISTER_OFFSET, &uiSDIO_InterruptStatus);
    }
    while(uiSDIO_InterruptStatus & uiSDIO_InterruptEnable);

    SDIOS_STATS_UPDATE_LOOP_PROFILE(psInternals, uiNumLoops);

	if ( psIOBlockDescriptor->eInterruptLevelType == HWLEVELEXT_LATCHED )
	{
		/*
			Force deassertion of interrupt line just in case another interrupt has sneaked in.
			Only need in LATCHED mode (which is NOT the recommended mode of operation).
		*/
		Write(  psIOBlockDescriptor->ui32Base, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_OFFSET, 0);
		Write(  psIOBlockDescriptor->ui32Base, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_OFFSET, uiSDIO_InterruptEnable);
	}
}


/*================================================================================*/
/*  SDIOS_SetHostInterruptFlag                                                    */
/*================================================================================*/
void SDIOS_SetHostInterruptFlag( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
    /* Set the interrupt flag */
    Write( psIOBlockDescriptor->ui32Base, IMG_SDIO_HOST_INTERRUPT_CONTROL_OFFSET, 1);
}


/*================================================================================*/
/*  prv_handle_trn_update_interrupt                                               */
/*================================================================================*/
static void prv_handle_trn_update_interrupt( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
    unsigned int				uiTfrCmdRegValue;
    unsigned int				uiFunctionNo;
    unsigned int				bBlockMode          = 0;
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Received Transfer Update Interrupt");

    psInternals->sStats.TRN_UpdateCount++;

    /*================================*/
    /* Get the CMD53 transfer details */
    /*================================*/
    Read(psIOBlockDescriptor->ui32Base, IMG_SDIO_SDIO_TRANSFER_COMMAND_OFFSET, &uiTfrCmdRegValue);

    SDIO_DBG_PRINT_TFR_PARAMS(uiTfrCmdRegValue);

    uiFunctionNo     = ReadRegField(uiTfrCmdRegValue, IMG_SDIO_SDIO_TRANSFER_COMMAND_TRANSFER_FUNCTION_NUMBER);
    bBlockMode       = ReadRegField(uiTfrCmdRegValue, IMG_SDIO_SDIO_TRANSFER_COMMAND_TRANSFER_BLOCK_MODE);

    /*============================================*/
    /* Check that this transfer is for function 1 */
    /*============================================*/
    if(uiFunctionNo == 0)
    {
        return; /* EARLY EXIT!  This is handled internally by the core */
    }
    else
    if (uiFunctionNo > 1)
    {
        IMG_ASSERT(SDIOS_FALSE); /* Only one function is supported */
        return;
    }

    if(bBlockMode)
    {
		/* Block mode */
        SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Transfer is block mode");
    }
    else
    {
		/* Byte mode */
        SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Transfer is byte mode");
    }
}


/*================================================================================*/
/*  prv_handle_blk_complete_interrupt                                             */
/*================================================================================*/
static void prv_handle_blk_complete_interrupt( ioblock_sBlockDescriptor *	psIOBlockDescriptor	)
{
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Received block complete interrupt");

    psInternals->sStats.BLK_CompleteCount++;

    if(psInternals->uiRegisteredEventsBitField & SDIOS_BLOCK_COMPLETE_EVENT)
    {
        SDIOS_EVENT_CALLBACK_DATA_T sCallbackData;

        sCallbackData.uiEvent   = SDIOS_BLOCK_COMPLETE_EVENT;
        sCallbackData.pData     = 0;
        psInternals->pfCallback(sCallbackData);
    }
}


/*================================================================================*/
/*  prv_handle_trn_complete_interrupt                                             */
/*================================================================================*/
static void prv_handle_trn_complete_interrupt( ioblock_sBlockDescriptor *	psIOBlockDescriptor	)
{
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Received transfer complete interrupt");

    psInternals->sStats.TRN_CompleteCount++;

    /* If a callback is registered for this event, call it now. */
    if(psInternals->uiRegisteredEventsBitField & SDIOS_TRANSFER_ELEMENT_COMPLETE_EVENT)
    {
        SDIOS_EVENT_CALLBACK_DATA_T sCallbackData;

        sCallbackData.uiEvent   = SDIOS_TRANSFER_ELEMENT_COMPLETE_EVENT;
        sCallbackData.pData     = 0;
        psInternals->pfCallback(sCallbackData);
    }
}


/*================================================================================*/
/*  prv_handle_trn_abort_interrupt                                                */
/*================================================================================*/
static void prv_handle_trn_abort_interrupt( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
	IMG_RESULT					rResult = IMG_SUCCESS;
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Received transfer abort interrupt");

    psInternals->sStats.TRN_AbortCount++;

    /* Clear any outstanding DMA transfers */
	rResult = GDMA_Reset( &(psInternals->sDMAContext) );
    prv_ResetTransferControlData( psIOBlockDescriptor );

    /* Flush the Input Buffer FIFO */
    Write(psIOBlockDescriptor->ui32Base, IMG_SDIO_CLEAR_BUSY_OFFSET, IMG_SDIO_CLEAR_BUSY_FLUSH_INPUT_FIFO_MASK);

    /* Flush the Output Buffer FIFO */
    Write(psIOBlockDescriptor->ui32Base, IMG_SDIO_CLEAR_BUSY_OFFSET, IMG_SDIO_CLEAR_BUSY_FLUSH_OUTPUT_FIFO_MASK);

    /* If a callback is registered for this event, call it now. */
    if(psInternals->uiRegisteredEventsBitField & SDIOS_TRANSFER_ABORT_EVENT)
    {
        SDIOS_EVENT_CALLBACK_DATA_T sCallbackData;

        sCallbackData.uiEvent   = SDIOS_TRANSFER_ABORT_EVENT;
        sCallbackData.pData     = 0;
        psInternals->pfCallback(sCallbackData);
    }
}


/*================================================================================*/
/*  prv_handle_trn_suspend_interrupt                                              */
/*================================================================================*/
static void prv_handle_trn_suspend_interrupt( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
    SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Received transfer suspend interrupt");

    /* Process the interrupt */
    IMG_ASSERT(SDIOS_FALSE); /* This mode is not yet supported */
}


/*================================================================================*/
/*  prv_handle_CRC_interrupt                                                      */
/*================================================================================*/
static void prv_handle_CRC_interrupt( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Received crc error interrupt");

    IMG_ASSERT(SDIOS_FALSE); /* CRC interrupt occurred! */

    /* Flush the Input Buffer FIFO */
    Write(psIOBlockDescriptor->ui32Base, IMG_SDIO_CLEAR_BUSY_OFFSET, IMG_SDIO_CLEAR_BUSY_FLUSH_INPUT_FIFO_MASK);

    /* Flush the Output Buffer FIFO */
    Write(psIOBlockDescriptor->ui32Base, IMG_SDIO_CLEAR_BUSY_OFFSET, IMG_SDIO_CLEAR_BUSY_FLUSH_OUTPUT_FIFO_MASK);

    /* Unlike the FIFO overflow, The clear busy mask does not need to be reset for a CRC interrupt */

    /* If a callback is registered for this event, call it now. */
    if(psInternals->uiRegisteredEventsBitField & SDIOS_CRC_CHECK_FAILED_EVENT)
    {
        SDIOS_EVENT_CALLBACK_DATA_T sCallbackData;

        sCallbackData.uiEvent   = SDIOS_CRC_CHECK_FAILED_EVENT;
        sCallbackData.pData     = 0;
        psInternals->pfCallback(sCallbackData);
    }
}


/*================================================================================*/
/*  prv_handle_FIFO_overflow_interrupt                                            */
/*================================================================================*/
static void prv_handle_FIFO_overflow_interrupt( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
	IMG_RESULT					rResult = IMG_SUCCESS;
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Received fifo overflow interrupt");

    psInternals->sStats.FIFO_OF_ErrorCount++;

    IMG_ASSERT(SDIOS_FALSE); /* FIFO overflow has occurred. */

    /* Clear any outstanding DMA transfers */
	rResult = GDMA_Reset( &(psInternals->sDMAContext) );
    prv_ResetTransferControlData( psIOBlockDescriptor );

    /* Flush the Input Buffer FIFO */
    Write(psIOBlockDescriptor->ui32Base, IMG_SDIO_CLEAR_BUSY_OFFSET, IMG_SDIO_CLEAR_BUSY_FLUSH_INPUT_FIFO_MASK);

    /* Flush the Output Buffer FIFO */
    Write(psIOBlockDescriptor->ui32Base, IMG_SDIO_CLEAR_BUSY_OFFSET, IMG_SDIO_CLEAR_BUSY_FLUSH_OUTPUT_FIFO_MASK);

    /* Finally, Write to the Clear Busy bit */
    Write(psIOBlockDescriptor->ui32Base, IMG_SDIO_CLEAR_BUSY_OFFSET, IMG_SDIO_CLEAR_BUSY_CLR_BUSY_MASK);

    /* If there is a callback registered for this event, send it. */
    if(psInternals->uiRegisteredEventsBitField & SDIOS_FIFO_OVERFLOW_EVENT)
    {
        SDIOS_EVENT_CALLBACK_DATA_T sCallbackData;

        sCallbackData.uiEvent   = SDIOS_FIFO_OVERFLOW_EVENT;
        sCallbackData.pData     = 0;
        psInternals->pfCallback(sCallbackData);
    }
}


/*================================================================================*/
/*  prv_handle_FIFO_underflow_interrupt                                           */
/*================================================================================*/
static void prv_handle_FIFO_underflow_interrupt( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
	IMG_RESULT					rResult = IMG_SUCCESS;
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;
	QIO_DEVICE_T *				dev;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);
	dev				= &(psPort->device);

    SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Received fifo underflow interrupt");

    psInternals->sStats.FIFO_UF_ErrorCount++;

    IMG_ASSERT(SDIOS_FALSE); /* FIFO underflow has occurred! */

    /* Cancel any outstanding DMA transfers. */
	rResult = GDMA_Reset( &(psInternals->sDMAContext) );

    /* Flush the Output Buffer FIFO */
    Write(psIOBlockDescriptor->ui32Base, IMG_SDIO_CLEAR_BUSY_OFFSET, IMG_SDIO_CLEAR_BUSY_FLUSH_OUTPUT_FIFO_MASK);

    /* This transfer is over, so clean up. */
    prv_ResetTransferControlData( psIOBlockDescriptor );
    QIO_complete(dev, QIO_IOCOMPLETE);
    QIO_start(dev);

    /* The busy bit must be cleared to allow transactions to continue */
    Write(psIOBlockDescriptor->ui32Base, IMG_SDIO_CLEAR_BUSY_OFFSET, IMG_SDIO_CLEAR_BUSY_CLR_BUSY_MASK);

    /* If this event requires a callback, do it now. */
    if(psInternals->uiRegisteredEventsBitField & SDIOS_FIFO_UNDERFLOW_EVENT)
    {
        SDIOS_EVENT_CALLBACK_DATA_T sCallbackData;

        sCallbackData.uiEvent = SDIOS_FIFO_UNDERFLOW_EVENT;
        sCallbackData.pData     = 0;
        psInternals->pfCallback(sCallbackData);
    }
}


/*================================================================================*/
/*  prv_handle_F0_update_interrupt                                                */
/*================================================================================*/
static void prv_handle_F0_update_interrupt( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Received F0 update interrupt");

    psInternals->sStats.F0_UpdateCount++;

    /* If this event requires a callback, do it now. */
    if(psInternals->uiRegisteredEventsBitField & SDIOS_F0_UPDATE_EVENT)
    {
        SDIOS_EVENT_CALLBACK_DATA_T sCallbackData;

        sCallbackData.uiEvent = SDIOS_F0_UPDATE_EVENT;
        sCallbackData.pData     = 0;
        psInternals->pfCallback(sCallbackData);
    }
}


/*================================================================================*/
/*  prv_handle_F1_update_interrupt                                                */
/*================================================================================*/
static void prv_handle_F1_update_interrupt( ioblock_sBlockDescriptor *	psIOBlockDescriptor	)
{
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Received F1 update interrupt");

    psInternals->sStats.F1_UpdateCount++;

    /* If this event requires a callback, do it now. */
    if(psInternals->uiRegisteredEventsBitField & SDIOS_F1_UPDATE_EVENT)
    {
        SDIOS_EVENT_CALLBACK_DATA_T     sCallbackData;
        unsigned int                    ui32F1Reg;

        Read (psIOBlockDescriptor->ui32Base, IMG_SDIO_F1_UPDATE_OFFSET, &ui32F1Reg);

        sCallbackData.uiEvent   = SDIOS_F1_UPDATE_EVENT;
        sCallbackData.pData     = (unsigned int*)ui32F1Reg;
        psInternals->pfCallback(sCallbackData);
    }
}


/*================================================================================*/
/*  prv_ConfigureDmaChannel                                                       */
/*================================================================================*/
static void prv_ConfigureDmaChannel( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
    unsigned int				bWriteToMem = 0;
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    if(psInternals->eCurrentTransferDirection == SDIOS_TRANSFER_DIR_HOST_TO_CARD){bWriteToMem = 1;}

    /* Configure the DMA channel for a data read / data write */
	if ( psIOBlockDescriptor->psSystemDescriptor->pfn_DMACChannelBind != IMG_NULL )
	{
		psIOBlockDescriptor->psSystemDescriptor->pfn_DMACChannelBind( bWriteToMem ? psIOBlockDescriptor->ui32DMACChannelSelectWriteValue : psIOBlockDescriptor->ui32DMACChannelSelectReadValue, psInternals->uiDMAChannel );
	}
	else
	{
		IMG_ASSERT(0);		/* Maybe okay to have no bind function - but trap for now */
	}
}

/*================================================================================*/
/*  SDIOS_EnableF1UpdateInterrupts                                                */
/*================================================================================*/
void SDIOS_EnableF1UpdateInterrupts(unsigned int bEnable, ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
    unsigned int				uiReg;
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    KRN_testSemaphore(&(psInternals->tDataSem), 1, KRN_INFWAIT);

    /* Enable or Disable the required interrupt */
    psInternals->bEnableF1UpdateInterrupts = bEnable;
    Read ( psIOBlockDescriptor->ui32Base, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_OFFSET, &uiReg);
    uiReg = WriteRegField(uiReg, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_EN_INT_F1_UPDATE, bEnable);
    Write( psIOBlockDescriptor->ui32Base, IMG_SDIO_INTERRUPT_ENABLE_REGISTER_OFFSET, uiReg);

	KRN_setSemaphore(&(psInternals->tDataSem), 1);
}

/*================================================================================*/
/*  SDIOS_ConfigureEventCallbacks                                                 */
/*================================================================================*/
void SDIOS_ConfigureEventCallbacks(const unsigned int uiEventsField, ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    KRN_testSemaphore(&(psInternals->tDataSem), 1, KRN_INFWAIT);
    psInternals->uiRegisteredEventsBitField = uiEventsField;
    KRN_setSemaphore(&(psInternals->tDataSem), 1);
}


/*================================================================================*/
/*  SDIOS_AddCallback                                                             */
/*================================================================================*/
void SDIOS_AddCallback(void(*pfCallback)(SDIOS_EVENT_CALLBACK_DATA_T), ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    KRN_testSemaphore(&(psInternals->tDataSem), 1, KRN_INFWAIT);
    psInternals->pfCallback = pfCallback;
    KRN_setSemaphore(&(psInternals->tDataSem), 1);
}


/*================================================================================*/
/*  SDIOS_CancelAll                                                               */
/*================================================================================*/
void SDIOS_CancelAll( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
    /* Clear the transfer parameters */
    prv_ResetTransferControlData( psIOBlockDescriptor );
}


/*================================================================================*/
/*  prv_ResetTransferControlData                                                  */
/*================================================================================*/
static void prv_ResetTransferControlData( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    psInternals->pucNextOperationBufferAddress   = 0;
    psInternals->uiNextOperationDataSizeBytes    = 0;
    psInternals->uiNextOperationDMASizeBytes     = 0;
    psInternals->eCurrentTransferDirection       = SDIOS_TRANSFER_DIR_UNSET;
    psInternals->bDmaTfrCompleted                = SDIOS_FALSE;
}


/*================================================================================*/
/*  prv_ResetStatistics                                                           */
/*================================================================================*/
static void prv_ResetStatistics( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);

    psInternals->sStats.TRN_UpdateCount                                 = 0;
    psInternals->sStats.TRN_CompleteCount                               = 0;
    psInternals->sStats.TRN_AbortCount                                  = 0;
    psInternals->sStats.BLK_CompleteCount                               = 0;
    psInternals->sStats.DMA_CompleteCount                               = 0;
    psInternals->sStats.F0_UpdateCount                                  = 0;
    psInternals->sStats.F1_UpdateCount                                  = 0;
    psInternals->sStats.CRC_ErrorCount                                  = 0;
    psInternals->sStats.FIFO_UF_ErrorCount                              = 0;
    psInternals->sStats.FIFO_OF_ErrorCount                              = 0;
    psInternals->sStats.DMA_Count_TX                                    = 0;
    psInternals->sStats.DMA_Count_RX                                    = 0;

#if defined SDIOS_ISR_STATS
    psInternals->sStats.sIsrStats.ISR_EntryCount                        = 0;
    psInternals->sStats.sIsrStats.DMA_InterruptCount                    = 0;
    psInternals->sStats.sIsrStats.SDIOS_CoreInterruptCount              = 0;
    psInternals->sStats.sIsrStats.QIO_StartCount                        = 0;
    psInternals->sStats.sIsrStats.QIO_CompleteCount                     = 0;
    psInternals->sStats.sIsrStats.SpuriousDmaInterruptCount             = 0;
    psInternals->sStats.sIsrStats.SpuriousCoreInterruptCount            = 0;
    psInternals->sStats.sIsrStats.SDIOS_LargestNumOfCoreIntLoopsDone    = 0;

    memset(psInternals->sStats.sIsrStats.SDIOS_CoreInterruptLoopProfile,
           0,
           sizeof (psInternals->sStats.sIsrStats.SDIOS_CoreInterruptLoopProfile));

#endif /* defined SDIOS_ISR_STATS */

}

/*================================================================================*/
/*  prv_SetupDmaForTransfer                                                       */
/*================================================================================*/
static void prv_SetupDmaForTransfer( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
	IMG_RESULT					rResult = IMG_SUCCESS;
	SDIOS_Internals	*			psInternals;
	SDIOS_PORT_T *				psPort;

	/* Set up useful local pointers */
    psPort			= (SDIOS_PORT_T *)(psIOBlockDescriptor->pvAPIContext);
	psInternals		= &(psPort->sDriverInternals);


    if(psInternals->eCurrentTransferDirection == SDIOS_TRANSFER_DIR_HOST_TO_CARD)
    {
        /* This is a write command (host -> card) */
        psInternals->sStats.DMA_Count_RX++;
        SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Configuring DMA channel for write (host -> card)");
        prv_ConfigureDmaChannel( psIOBlockDescriptor );

        SDIO_DBG_PRINT_DMA_PARAMS( psIOBlockDescriptor );

		/* Set up the transfer object */
		psInternals->sDMATransferObject.pui8ReadPointer		= (IMG_UINT8 *)(psIOBlockDescriptor->ui32Base + IMG_SDIO_RX_DATA_IN_OFFSET);
		psInternals->sDMATransferObject.bIncReadPointer		= IMG_FALSE;
		psInternals->sDMATransferObject.ui32ReadWidth		= SDIOS_RX_FIFO_READ_WIDTH;

		psInternals->sDMATransferObject.pui8WritePointer	= (IMG_UINT8 *)psInternals->pucNextOperationBufferAddress;
		psInternals->sDMATransferObject.bIncWritePointer	= IMG_TRUE;
		psInternals->sDMATransferObject.ui32WriteWidth		= SDIOS_MEMORY_WRITE_WIDTH;

		psInternals->sDMATransferObject.ui32SizeInBytes		= psInternals->uiNextOperationDMASizeBytes;
		psInternals->sDMATransferObject.ui8Priority			= 0;
		psInternals->sDMATransferObject.ui32BurstSize       = (SDIO_DMA_BURST_SIZE * 4); // Burst size is specified in bytes
		psInternals->sDMATransferObject.pvUserData			= IMG_NULL;

		rResult = GDMA_SingleShotOperation( &psInternals->sDMAContext,
											&psInternals->sDMATransferObject,
											0 );
    }
    else
    if (psInternals->eCurrentTransferDirection == SDIOS_TRANSFER_DIR_CARD_TO_HOST)
    {
        /* This is a read command (card -> host) */
        psInternals->sStats.DMA_Count_TX++;
        SDIO_DBG_COMMENT(psIOBlockDescriptor->ui32Base, "SDIO Driver: Configuring DMA channel for read(card -> host)");
        prv_ConfigureDmaChannel( psIOBlockDescriptor );

		rResult = GDMA_Reset( &(psInternals->sDMAContext) );

        SDIO_DBG_PRINT_DMA_PARAMS(psIOBlockDescriptor);

		/* Set up the transfer object */
		psInternals->sDMATransferObject.pui8ReadPointer		= (IMG_UINT8 *)(psInternals->pucNextOperationBufferAddress);
		psInternals->sDMATransferObject.bIncReadPointer		= IMG_TRUE;
		psInternals->sDMATransferObject.ui32ReadWidth		= SDIOS_MEMORY_READ_WIDTH;

		psInternals->sDMATransferObject.pui8WritePointer	= (IMG_UINT8 *)(psIOBlockDescriptor->ui32Base + IMG_SDIO_TX_DATA_OUT_OFFSET);
		psInternals->sDMATransferObject.bIncWritePointer	= IMG_FALSE;
		psInternals->sDMATransferObject.ui32WriteWidth		= SDIOS_TX_FIFO_WRITE_WIDTH;

		psInternals->sDMATransferObject.ui32SizeInBytes		= psInternals->uiNextOperationDMASizeBytes;
		psInternals->sDMATransferObject.ui8Priority			= 0;
        psInternals->sDMATransferObject.bWaitUnpack			= SDIOS_DMA_READ_WAIT_UNPACK;
        psInternals->sDMATransferObject.ui32HoldOff			= SDIOS_DMA_READ_HOLD_OFF;
        psInternals->sDMATransferObject.ui32BurstSize       = (SDIO_DMA_BURST_SIZE * 4); // Burst size is specified in bytes
		psInternals->sDMATransferObject.pvUserData			= IMG_NULL;



		rResult = GDMA_SingleShotOperation( &psInternals->sDMAContext,
											&psInternals->sDMATransferObject,
											0 );
    }
    else
    {
		/* Transfer direction parameter is not set - probably due to an unexpected trn_update interrupt */
        IMG_ASSERT(SDIOS_FALSE);
    }

}


/* ------------------------------------------------------------------------ */
/*                                                                          */
/*                       Functions FOR DEBUG                                */
/*                                                                          */
/* ------------------------------------------------------------------------ */

#if defined ENABLE_SDIOS_DBG_COMMENTS
/*================================================================================*/
/*  prv_DBG_Print_DMA_Params                                                      */
/*================================================================================*/
static void prv_DBG_Print_DMA_Params( ioblock_sBlockDescriptor *	psIOBlockDescriptor )
{
    char caOutMsg[256];

    sprintf(caOutMsg, "SDIO Driver: Setting up DMA transfer size of %d bytes. Transfer is %d bytes total",
        psInternals->uiNextOperationDMASizeBytes,
        psInternals->uiNextOperationDataSizeBytes);

    SDIO_DBG_COMMENT(psInternals->ui32BaseAddress, caOutMsg);
}

/*================================================================================*/
/*  prv_DBG_Print_TFR_Params                                                      */
/*================================================================================*/
static void prv_DBG_Print_TFR_Params(unsigned int uiTfrCmd)
{
    char caOutMsg[256];

    sprintf(caOutMsg, "SDIO Driver: Trn_update: TfrCmdReg:0x%x",  uiTfrCmd);
    SDIO_DBG_COMMENT(psInternals->ui32BaseAddress, caOutMsg);
}

#endif /* defined ENABLE_SDIOS_DBG_COMMENTS */


#if defined SDIOS_ISR_STATS
/*================================================================================*/
/*  prv_updateLoopProfile                                                         */
/*================================================================================*/
static void prv_updateLoopProfile( ioblock_sBlockDescriptor *	psIOBlockDescriptor , unsigned int uiNumLoops)
{
    IMG_ASSERT(uiNumLoops < 1000);  /* Stuck in an ISR loop! */

    /* Update the largest number of loops done. */
    if(psInternals->sStats.sIsrStats.SDIOS_LargestNumOfCoreIntLoopsDone < uiNumLoops)
    {
        psInternals->sStats.sIsrStats.SDIOS_LargestNumOfCoreIntLoopsDone = uiNumLoops;
    }

    /* Limit number of loops to 9 for use in the loop profile array. */
    if(uiNumLoops > 9)
    {
        uiNumLoops = 9;
    }

    /* Update the loop profile array. */
    psInternals->sStats.sIsrStats.SDIOS_CoreInterruptLoopProfile[uiNumLoops]++;
}
#endif /* defined SDIOS_ISR_STATS */
