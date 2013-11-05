/*!
*******************************************************************************
  file   gdma_drv.c

  brief  Generic DMA driver

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

/******************************************************************************
**************************** Included header files ****************************
*******************************************************************************/

/* Keep these first ... */
#if defined(METAG) & !defined(METAG_ALL_VALUES)
#define METAG_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#include <metag/tbiccb00.h>
#else
#include <string.h>
#endif  /* METAG and NOT METAG_ALL_VALUES */

#include <MeOS.h>

#include <assert.h>

/* Device files */
#include "ioblock_defs.h"
#include "ioblock_utils.h"
#include "gdma_api.h"
#include "gdma_drv.h"
#include "gdma_hal.h"

//#define LOG_EVENTS

#if defined (LOG_EVENTS)
#include "gdma_api_log.h"
#endif

/******************************************************************************
****************************** Macro Definitions ******************************
*******************************************************************************/

/* Simple register read/write macros */
#define WRITE(R,V)  *((volatile unsigned int *)(R)) = ((unsigned int)V)
#define READ(R)     *((volatile unsigned int *)(R))

/******************************************************************************
**************************** End Macro Definitions ****************************
*******************************************************************************/


/******************************************************************************
************************** Local Function Prototypes **************************
*******************************************************************************/

static IMG_INT32 GDMA_DriverInit
(
    QIO_DEVICE_T    *	psDev,
    QIO_DEVPOWER_T  *	psDevPwr,
    IMG_INT32       *	pi32DevRank,
    IMG_UINT32         	aui32IntMasks [QIO_MAXVECGROUPS]
);

static IMG_VOID	GDMA_DriverISR
(
	QIO_DEVICE_T *	psDev
);

IMG_VOID GDMA_DriverCancel
(
	QIO_DEVICE_T *	psDev
);

static IMG_VOID GDMA_DriverStartWrapper
(
	QIO_DEVICE_T *	psDev,
	QIO_IOPARS_T *	psIOPars
);


IMG_RESULT gdma_DeviceCompletion
(
	GDMA_sContext *				psContext,
	GDMA_sTransferObject *		psTransferObject,
	QIO_STATUS_T				eStatus
);


/******************************************************************************
************************ End Local Function Prototypes ************************
*******************************************************************************/


/******************************************************************************
******************************* Local Variables *******************************
*******************************************************************************/

/*! Driver function table */
const QIO_DRIVER_T GDMA_sQIODriverFunctions =
{
    GDMA_DriverISR,      		/* ISR function    */
    GDMA_DriverInit,     		/* Init function   */
    GDMA_DriverStartWrapper,    /* Start function  */
    GDMA_DriverCancel,   		/* Cancel function */
    NULL,
    NULL,
    NULL
};

/******************************************************************************
***************************** End Local Variables *****************************
*******************************************************************************/


/******************************************************************************
******************************* Local Functions *******************************
*******************************************************************************/

/*!
******************************************************************************

 @Function              @GDMA_DriverInit

******************************************************************************/
static IMG_INT32 GDMA_DriverInit
(
    QIO_DEVICE_T    *	psDev,
    QIO_DEVPOWER_T  *	psDevPwr,
    IMG_INT32       *	pi32DevRank,
    IMG_UINT32         	aui32IntMasks [QIO_MAXVECGROUPS]
)
{
    IMG_UINT32			ui32TrigState;
    GDMA_sContext *		psContext;
    IMG_UINT32			ui32Reg;

    /* Set device power and rank */
    *psDevPwr  		= QIO_POWERNONE;
    *pi32DevRank 	= 1;

    IMG_ASSERT ( psDev != IMG_NULL );
    IMG_ASSERT ( GDMA_apsHardwareInstances[psDev->id] != IMG_NULL );
    psContext = (GDMA_sContext *) GDMA_apsHardwareInstances[psDev->id]->pvAPIContext;
    IMG_ASSERT ( psContext != IMG_NULL );

	IOBLOCK_CalculateInterruptInformation( GDMA_apsHardwareInstances[psContext->ui32Channel] );

	/* Copy the newly calculated interrupt masks back into the structure provided by QIO */
	IMG_MEMCPY( aui32IntMasks, GDMA_apsHardwareInstances[psContext->ui32Channel]->ui32IntMasks, sizeof( unsigned long ) * QIO_MAXVECGROUPS );

	TBI_LOCK(ui32TrigState);
		ui32Reg = READ(GDMA_apsHardwareInstances[psContext->ui32Channel]->sDeviceISRInfo.ui32LEVELEXTAddress);

		// HWLEVELEXT
		if ( GDMA_apsHardwareInstances[psContext->ui32Channel]->eInterruptLevelType == HWLEVELEXT_LATCHED )
		{
			/* Clear the LEV bit in the HWLEVELEXT */
			ui32Reg &= ~(GDMA_apsHardwareInstances[psContext->ui32Channel]->sDeviceISRInfo.ui32LEVELEXTMask);
		}
		else if ( GDMA_apsHardwareInstances[psContext->ui32Channel]->eInterruptLevelType == HWLEVELEXT_NON_LATCHED )
		{
			/* Set the LEV bit in the HWLEVELEXT */
			ui32Reg |= (GDMA_apsHardwareInstances[psContext->ui32Channel]->sDeviceISRInfo.ui32LEVELEXTMask);
		}
		else
		{
			// Unknown InterruptLevelType
			IMG_ASSERT (0);
		}

		WRITE(GDMA_apsHardwareInstances[psContext->ui32Channel]->sDeviceISRInfo.ui32LEVELEXTAddress, ui32Reg);
	TBI_UNLOCK(ui32TrigState);

    return 0;
}


/*!
******************************************************************************

 @Function              @GDMA_DriverStart

******************************************************************************/
IMG_RESULT GDMA_DriverStart
(
    GDMA_sContext *				psContext,
    GDMA_sTransferObject *		psTransferObject,
	QIO_DEVICE_T *				psDev
)
{
    IMG_RESULT						rResult;
	ioblock_sBlockDescriptor	*	psBlockDesc;

	IMG_ASSERT ( psContext != IMG_NULL );
    IMG_ASSERT ( psTransferObject != IMG_NULL );

	psBlockDesc = GDMA_apsHardwareInstances[psContext->ui32Channel];
	IMG_ASSERT( psBlockDesc );

#if !defined (BOOT_CODE)
    if (psContext->bLinkedListMode == IMG_FALSE)
    {
#endif
    	IMG_ASSERT ( psContext->psCurrentSingleShotOperation == IMG_NULL );
    	psContext->psCurrentSingleShotOperation = psTransferObject;

		rResult = GDMA_HAL_PrepareSingleShotTransfer ( psBlockDesc, psTransferObject );
		if ( rResult != IMG_SUCCESS )
		{
			IMG_ASSERT (0);
		}

        if (psContext->bAutostartEnabled == IMG_TRUE)
        {
            /* Enable the single shot DMA transfer */
            rResult = GDMA_HAL_EnableTransfer ( psBlockDesc, IMG_FALSE );
			if ( rResult != IMG_SUCCESS )
			{
				if ( psContext->bBypassQIO == IMG_FALSE )
				{
					/* Cancel the operation */
					QIO_complete(psDev, QIO_CANCEL);
				}
				else
				{
					return rResult;
				}
			}
			else
			{
				psContext->eState = GDMA_RUNNING;

				/* Call the device specific start function (if provided) */
		    	if ((*psContext->sCallbackFunctions.pfnStartDevice) != IMG_NULL)
			    {
			        (*psContext->sCallbackFunctions.pfnStartDevice)(psContext->pvCallbackParameter);
		    	}
		    }
		}
#if !defined (BOOT_CODE)
    }
    else
    {
        /* In linked list mode we ignore pIOPars, getting all the information
           for the transfer from the linked list module */
		if (psContext->eState != GDMA_RUNNING)
		{
			/* To use the DMAC linked list functionality, we must link a number of elements
			  BEFORE starting the DMAC. The DMAC reads the link structure before reading the
			  buffer so the link must be set before the buffer is picked up. */
			if ((psContext->bAutostartEnabled == IMG_TRUE) &&
				(psContext->sLinkedListStatus.ui32ListElementsInUse >= psContext->sLinkedListStatus.ui32StartListThreshold))
			{
				/* Reset DMAC */
				rResult = GDMA_HAL_ResetHardware ( psBlockDesc );
				if ( rResult != IMG_SUCCESS )
				{
					if ( psContext->bBypassQIO == IMG_FALSE )
					{
						/* Cancel the operation */
						QIO_complete(psDev, QIO_CANCEL);
					}
					else
					{
						return rResult;
					}
				}

				/* Setup the DMAC */
				psContext->sLinkedListStatus.ui32ActiveBlockIndex = psContext->sLinkedListStatus.ui32Head;
				rResult = GDMA_HAL_PrepareLinkedListTransfer ( psBlockDesc, psContext->sLinkedListStatus.ui32Head );
				if ( rResult != IMG_SUCCESS )
				{
					if ( psContext->bBypassQIO == IMG_FALSE )
					{
						/* Cancel the operation */
						QIO_complete(psDev, QIO_CANCEL);
					}
					else
					{
						return rResult;
					}
				}
				else
				{
					/* Setup the DMAC */
					rResult = GDMA_HAL_EnableTransfer ( psBlockDesc, IMG_TRUE );
					if ( rResult != IMG_SUCCESS )
					{
						if ( psContext->bBypassQIO == IMG_FALSE )
						{
							/* Cancel the operation */
							QIO_complete(psDev, QIO_CANCEL);
						}
						else
						{
							return rResult;
						}
					}
					else
					{
						psContext->eState = GDMA_RUNNING;

						/* Call the device specific start function (if provided) */
					    if ((*psContext->sCallbackFunctions.pfnStartDevice) != IMG_NULL)
					    {
			        		(*psContext->sCallbackFunctions.pfnStartDevice)(psContext->pvCallbackParameter);
			    		}
					}
				}
			}
		}
    }
#endif /* !defined BOOT_CODE */

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              @GDMA_DriverStartWrapper

******************************************************************************/
static IMG_VOID GDMA_DriverStartWrapper
(
	QIO_DEVICE_T *	psDev,
	QIO_IOPARS_T *	psIOPars
)
{
	GDMA_sTransferObject *		psTransferObject;
	GDMA_sContext *				psContext;

	IMG_ASSERT ( psDev != IMG_NULL );
	IMG_ASSERT ( psIOPars != IMG_NULL );

	psTransferObject = (GDMA_sTransferObject *)psIOPars->pointer;

    IMG_ASSERT ( GDMA_apsHardwareInstances[psDev->id] != IMG_NULL );
    psContext = (GDMA_sContext *) GDMA_apsHardwareInstances[psDev->id]->pvAPIContext;
    IMG_ASSERT ( psContext != IMG_NULL );

	GDMA_DriverStart ( psContext, psTransferObject, psDev );
}

/*!
******************************************************************************

 @Function              @GDMA_DriverISR

******************************************************************************/
static IMG_VOID GDMA_DriverISR(QIO_DEVICE_T * psDev)
{
	GDMA_sContext *			psContext;
	IMG_RESULT				rResult;
	GDMA_sTransferObject *	psTransferObject;

    /* Get DMAC context */
    IMG_ASSERT ( psDev != IMG_NULL );
    IMG_ASSERT ( GDMA_apsHardwareInstances[psDev->id] != IMG_NULL );
    psContext = (GDMA_sContext *) GDMA_apsHardwareInstances[psDev->id]->pvAPIContext;
    IMG_ASSERT ( psContext != IMG_NULL );

	rResult = GDMA_HAL_ReadAndClearInterrupts ( GDMA_apsHardwareInstances[psDev->id] );

  #if defined (LOG_EVENTS)
	LOG_EVENT(GDMA, ISR, (LOG_FLAG_START|LOG_FLAG_QUAL_ARG1), psDev->id, 0);
  #endif

	if ( rResult == IMG_SUCCESS )
	{
		if (( psContext->bBypassQIO == IMG_TRUE ) || ( psContext->sLinkedListStatus.bCircularList == IMG_TRUE ))
		{
			if ( psContext->bLinkedListMode == IMG_TRUE )
			{
				psTransferObject = (psContext->sLinkedListStatus.pasListElements [psContext->sLinkedListStatus.ui32ActiveBlockIndex]).psTransferObject;
			}
			else
			{
				psTransferObject = psContext->psCurrentSingleShotOperation;
			}

			IMG_ASSERT ( psTransferObject != IMG_NULL );

			/* Call completion function directly */
			gdma_DeviceCompletion ( psContext, psTransferObject, QIO_NORMAL );
		}
		else
		{
    		/* Complete operation and start next queued operation */
    		QIO_complete(psDev, QIO_IOCOMPLETE);
    		QIO_start(psDev);
    	}
    }

  #if defined (LOG_EVENTS)
	LOG_EVENT(GDMA, ISR, (LOG_FLAG_END|LOG_FLAG_QUAL_ARG1), psDev->id, 0);
  #endif

}


/*!
******************************************************************************

 @Function              @GDMA_DriverCancel

******************************************************************************/
IMG_VOID GDMA_DriverCancel(QIO_DEVICE_T * psDev)
{
    GDMA_sContext *		psContext;
    IMG_RESULT			rResult;

    IMG_ASSERT ( psDev != IMG_NULL );
    IMG_ASSERT ( GDMA_apsHardwareInstances[psDev->id] != IMG_NULL );
    psContext = (GDMA_sContext *) GDMA_apsHardwareInstances[psDev->id]->pvAPIContext;
    IMG_ASSERT ( psContext != IMG_NULL );

    /* Ensure DMAC disabled */
    rResult = GDMA_HAL_ResetHardware( GDMA_apsHardwareInstances[psDev->id] );
    if ( rResult != IMG_SUCCESS )
    {
    	IMG_ASSERT (0);
    }

    return;
}

/******************************************************************************
***************************** End Local Functions *****************************
*******************************************************************************/

