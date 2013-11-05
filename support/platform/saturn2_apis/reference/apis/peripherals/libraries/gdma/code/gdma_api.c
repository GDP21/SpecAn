/*!
*******************************************************************************
  file   gdma_api.c

  author Imagination Technologies

         <b>Copyright 2008 by Imagination Technologies Limited.</b>\n
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

#include <stdio.h>	/* Required for 'sprintf' */
#include <assert.h>
#include <sys_util.h>

/* Device files */
#include "ioblock_defs.h"
#include "gdma_api.h"
#include "gdma_drv.h"
#include "gdma_hal.h"

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
******************************* Local Variables *******************************
*******************************************************************************/

// GDMA_MAX_NO_OF_HARDWARE_INSTANCES defined in gdma_drv.h
ioblock_sBlockDescriptor * GDMA_apsHardwareInstances [ GDMA_MAX_NO_OF_HARDWARE_INSTANCES ] = 
{
	IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL
};

const char g_szDevName[11] = "GDMAC_CHAN";

/******************************************************************************
***************************** End Local Variables *****************************
*******************************************************************************/

/******************************************************************************
****************************** Global Functions *******************************
*******************************************************************************/

IMG_RESULT GDMA_DriverStart
(
    GDMA_sContext *				psContext,
    GDMA_sTransferObject *		psTransferObject,
	QIO_DEVICE_T *				psDev
);


IMG_VOID GDMA_DriverCancel
(
	QIO_DEVICE_T * psDev
);

IMG_RESULT gdma_SubmitOperationToDriver
(
    GDMA_sContext *					psContext,
    GDMA_sTransferObject *			psObject,
    IMG_INT32						i32Timeout
);

IMG_RESULT gdma_DeviceCompletion
(
	GDMA_sContext *				psContext,
	GDMA_sTransferObject *		psTransferObject,
	QIO_STATUS_T				eStatus
);

/*!
*******************************************************************************

 @Function              @gdma_Define

*******************************************************************************/
img_void	gdma_Define(	ioblock_sBlockDescriptor	*	psDescriptor	)
{
	IMG_ASSERT( psDescriptor );

	/* Check that the provided index is within permissable range */
	IMG_ASSERT( psDescriptor->ui32Index < GDMA_MAX_NO_OF_HARDWARE_INSTANCES );

	/* Check that an instance hasn't already been registered with this hw index */
	IMG_ASSERT( !GDMA_apsHardwareInstances[ psDescriptor->ui32Index ] );

	/* Register this hardware in local array */
	GDMA_apsHardwareInstances[ psDescriptor->ui32Index ] = psDescriptor;
}

/*!
*******************************************************************************

 @Function              @GDMA_Define

*******************************************************************************/
IMG_RESULT	GDMA_Define	
(	
	ioblock_sBlockDescriptor *	psThisHWInstance	
)
{
	return IMG_SUCCESS;
}


/*!
*******************************************************************************

 @Function              @GDMA_Initialise

*******************************************************************************/
IMG_RESULT GDMA_Initialise
(
	GDMA_sContext *							psContext,
	IMG_UINT32								ui32HWChannel,
	IMG_UINT8								ui8ChannelGroup,
	IMG_BOOL								bLinkedListMode,
	GDMA_sCallbackFunctions	*				psCallbackFunctions,
	IMG_VOID *								pvCallbackParameter,
	GDMA_sWrappedLinkedListDescriptor *		pasLinkedListElements,
	IMG_UINT32								ui32NoOfLinkedListElements,
	IMG_BOOL								bAutostartEnabled,
	IMG_UINT32								ui32StartListThreshold,
    QIO_IOCB_T *							pasIOCBs,
    IMG_UINT32								ui32NoOfIOCBs,
    IMG_BOOL								bBypassQIO
)
{	
#if !defined (BOOT_CODE)
	img_uint32	ui32StrChannel;
#endif

	IMG_ASSERT ( psContext != IMG_NULL );
	IMG_ASSERT ( psContext->eState == GDMA_DEINITIALISED );
	
	/* Check that the specified hardware channel has been instantiated */
	IMG_ASSERT ( ui32HWChannel < GDMA_MAX_NO_OF_HARDWARE_INSTANCES );
	if ( !GDMA_apsHardwareInstances[ui32HWChannel] )
	{
		gdma_Define( &IMG_asGDMABlock[ ui32HWChannel ] );
	}
	
	/* Clear, then populate provided context structure */
	IMG_MEMSET ( psContext, 0, sizeof(GDMA_sContext) );
	
	psContext->ui32Channel 			= ui32HWChannel;
	psContext->ui8ChannelGroup		= ui8ChannelGroup;
	psContext->bLinkedListMode		= bLinkedListMode;
	psContext->bBypassQIO			= bBypassQIO;
	
	/* Point hardware descriptor at context structure */
	GDMA_apsHardwareInstances[ui32HWChannel]->pvAPIContext = (IMG_VOID *) psContext;
	
	if (( psContext->bBypassQIO == IMG_TRUE ) &&
		(( psCallbackFunctions == IMG_NULL ) ||
		 ( psCallbackFunctions->pfnCompletion == IMG_NULL )))
	{
		/* There is no support for waiting for operations to complete (using 'GDMA_WaitObject') when QIO is bypassed. 	*/
		/* As such, a completion callback MUST be registered, as this is the only way the user can be informed of an  	*/
		/* operation finishing.																							*/
		return IMG_ERROR_INVALID_PARAMETERS;	
	}
	
	if ( psCallbackFunctions != IMG_NULL )
	{		
		psContext->sCallbackFunctions 	= *psCallbackFunctions;
		psContext->pvCallbackParameter 	= pvCallbackParameter;
	}
		
	GDMA_HAL_ResetHardware(GDMA_apsHardwareInstances[ui32HWChannel]);
	
	/* If we are bypassing QIO, then IOCBs are not required */
	if ( psContext->bBypassQIO == IMG_TRUE )
	{
		if (( pasIOCBs != IMG_NULL ) ||
			( ui32NoOfIOCBs != 0 ))
		{
			return IMG_ERROR_INVALID_PARAMETERS;
		}
	}
	else
	{
	    /* Initialise driver return mailbox */
	    KRN_initMbox(&psContext->sMBox);
	
	    /* Initialise pointer to the IOCB objects pool */
	    IMG_ASSERT ( pasIOCBs != IMG_NULL );
	    IMG_ASSERT ( ui32NoOfIOCBs > 0 );    
	    KRN_initPool( &(psContext->sIOCBPool),
	    			  pasIOCBs, 
	    			  ui32NoOfIOCBs, 
	    			  sizeof(QIO_IOCB_T) );
	}
		    			  	    			  
	/* Initialise the QIO device driver */
	/* We do this even when 'Bypass QIO' is enabled, as we still use the QIO 	*/
	/* framework to handle DMAC interrupts, just not to perform queueing.		*/	
#if !defined (BOOT_CODE)
	IMG_MEMCPY( psContext->aui8DeviceName, g_szDevName, 10 );
	ui32StrChannel = ui32HWChannel;
	psContext->aui8DeviceName[10] = '0' + (ui32StrChannel / 100);
	
	ui32StrChannel = ui32StrChannel - (100 * (ui32StrChannel / 100));
	psContext->aui8DeviceName[11] = '0' + ui32StrChannel / 10;

	ui32StrChannel = ui32StrChannel - (10 * (ui32StrChannel / 10));
	psContext->aui8DeviceName[12] = '0' + ui32StrChannel;
	psContext->aui8DeviceName[13] = 0;
	    
	QIO_init( &(psContext->sDevice),
	  		  (char *)(psContext->aui8DeviceName),
	   		  ui32HWChannel,
	   		  &GDMA_sQIODriverFunctions	);
#else
	QIO_init( &(psContext->sDevice),
			  "GDMA",
			  ui32HWChannel,
			  &GDMA_sQIODriverFunctions );
#endif
	
	if (( ui32NoOfLinkedListElements > 0 ) ||
		( pasLinkedListElements != IMG_NULL ))
	{
		/* No point having a linked list if there's only ever one element */
		IMG_ASSERT ( ui32NoOfLinkedListElements > 1 );
		IMG_ASSERT ( pasLinkedListElements != IMG_NULL );
		psContext->sLinkedListStatus.pasListElements = pasLinkedListElements;
		psContext->sLinkedListStatus.ui32NoOfListElements = ui32NoOfLinkedListElements;
		psContext->sLinkedListStatus.ui32ListElementsInUse = 0;
	}
			
	psContext->bAutostartEnabled = bAutostartEnabled;
	psContext->sLinkedListStatus.ui32StartListThreshold = ui32StartListThreshold;
	
    /* Call the device specific init function (if provided) */
    if ((psContext->sCallbackFunctions.pfnInitDevice) != NULL)
    {
        (psContext->sCallbackFunctions.pfnInitDevice)(psContext->pvCallbackParameter);
    }
	
	psContext->eState = GDMA_INITIALISED;

	return IMG_SUCCESS;	
}


/*!
*******************************************************************************

 @Function              @GDMA_Configure

*******************************************************************************/
IMG_RESULT GDMA_Configure
(
    GDMA_sContext *					psContext
)
{
    /* Test have a valid context */
	IMG_ASSERT ( psContext != IMG_NULL );
	IMG_ASSERT ( psContext->ui32Channel < GDMA_MAX_NO_OF_HARDWARE_INSTANCES );
	IMG_ASSERT ( GDMA_apsHardwareInstances[psContext->ui32Channel] != IMG_NULL );

    /* This function may only be called from the initialised state */
    if (psContext->eState != GDMA_INITIALISED)
    {
        return IMG_ERROR_NOT_INITIALISED;
    }

	if ( psContext->bBypassQIO == IMG_FALSE )
	{
    	/* Enable queuing of transfer objects to the device */
    	QIO_enable(&(psContext->sDevice));
    }

    /* Device enters the idle state */
    psContext->eState = GDMA_IDLE;

    return IMG_SUCCESS;
}

/*!
*******************************************************************************

 @Function              @GDMA_ListAdd

*******************************************************************************/
IMG_RESULT GDMA_ListAdd
(
    GDMA_sContext *					psContext,
	GDMA_sTransferObject *			psTransferObject,
    IMG_BOOL						bEndOfList
)
{
    IMG_RESULT				rResult = IMG_SUCCESS;
    IMG_BOOL				bListPreviouslyEmpty;
    IMG_UINT32				ui32LinkLast = 0;
    IMG_UINT32				ui32LinkTail = 0;
    IMG_UINT32				ui32PenultimateListElement;

    /* Test have a valid context */
	IMG_ASSERT ( psContext != IMG_NULL );
	IMG_ASSERT ( psContext->ui32Channel < GDMA_MAX_NO_OF_HARDWARE_INSTANCES );
	IMG_ASSERT ( GDMA_apsHardwareInstances[psContext->ui32Channel] != IMG_NULL );
    IMG_ASSERT ( psTransferObject != NULL );

    /* This function may only be called from idle, disabled or running states */
    if ((psContext->eState != GDMA_IDLE) && 
    	(psContext->eState != GDMA_DISABLED) &&
    	(psContext->eState != GDMA_RUNNING))
    {
        return IMG_ERROR_UNEXPECTED_STATE;
    }

	/* We must be in linked list mode to add elements to a linked list */
	if ( psContext->bLinkedListMode == IMG_FALSE )
	{
		return IMG_ERROR_UNEXPECTED_STATE;
	}

    /* Test there is a free element in the list */
    if (psContext->sLinkedListStatus.ui32ListElementsInUse >= psContext->sLinkedListStatus.ui32NoOfListElements)
    {
        return IMG_ERROR_STORAGE_TYPE_FULL;
    }
        
    if ( psContext->eState == GDMA_RUNNING )
    {
	    /* If we are running, then only allow a new element to be hooked onto the list if we have at least one whole 	*/
	    /* list element between us and the final element (e.g.: if last element is 'n', then it is forbidden to add	to	*/
	    /* the list if the hardware is currently working on 'n' or 'n-1'). This is because at the point the hardware	*/
	    /* switches to the last element, it (depending on the hardware) decides at that point whether this new element	*/
	    /* is the last in the list. As such, we have the 'finishing' state which describes the processing of this last	*/
	    /* block. We cannot risk switching into this state while we're adding to the list - as such, we must have at	*/
	    /* least one 'safe' (i.e.: not 'loading last block') interrupt in hand when we start adding to the list.		*/
	    if ( psContext->sLinkedListStatus.ui32Last == 0 )
	    {
	    	ui32PenultimateListElement = (psContext->sLinkedListStatus.ui32NoOfListElements - 1);
	    }
	    else
	    {
	    	ui32PenultimateListElement = (psContext->sLinkedListStatus.ui32Last - 1);
	    }    	
    	
    	if ((psContext->sLinkedListStatus.bCircularList == IMG_FALSE)	/* If it's currently a circular list, then we can safely add to it, as the worst that will happen is 		*/
    		&&															/* that the hardware will wrap back round to the start of the list and pick up the new element next time.	*/
    		(( psContext->sLinkedListStatus.ui32ActiveBlockIndex == psContext->sLinkedListStatus.ui32Last ) ||	/* This first condition should be trapped by the 'flushing' state, but check anyway */
			 ( psContext->sLinkedListStatus.ui32ActiveBlockIndex == ui32PenultimateListElement )))
		{
			return IMG_ERROR_OPERATION_PROHIBITED;
		}
	}
	
    /* If list is not empty then hook this new element onto the end of the list */
    if (psContext->sLinkedListStatus.ui32Last != psContext->sLinkedListStatus.ui32Tail)
    {
    	bListPreviouslyEmpty = IMG_FALSE;
    	ui32LinkLast = psContext->sLinkedListStatus.ui32Last;
    	ui32LinkTail = psContext->sLinkedListStatus.ui32Tail;
    }
    else
    {
    	bListPreviouslyEmpty = IMG_TRUE;
    }
    
    psContext->sLinkedListStatus.ui32Last = psContext->sLinkedListStatus.ui32Tail;

    /* Update tail to point to next free item and wrap if necessary */
    psContext->sLinkedListStatus.ui32Tail++;
    if (psContext->sLinkedListStatus.ui32Tail >= psContext->sLinkedListStatus.ui32NoOfListElements)
    {
        psContext->sLinkedListStatus.ui32Tail = 0;
    }

	/* Is this a circular list? */
	psContext->sLinkedListStatus.bCircularList = (bEndOfList == IMG_TRUE) ? IMG_FALSE : IMG_TRUE;

    /* Now set up the new linked list element */
	rResult = GDMA_HAL_SetLinkedListDescriptor( GDMA_apsHardwareInstances[psContext->ui32Channel],
												psContext->sLinkedListStatus.ui32Last,
												bListPreviouslyEmpty,
												psTransferObject,
												((bEndOfList == IMG_TRUE) ? GDMA_HAL__END_OF_LINKED_LIST : psContext->sLinkedListStatus.ui32Head) );	/* If this is not the end of the list, then link back to the start of the list */
												
	if ( rResult != IMG_SUCCESS )
	{
		return rResult;	
	}
	
    /* Increment active counter */
    psContext->sLinkedListStatus.ui32ListElementsInUse++;
    
    if (( bListPreviouslyEmpty == IMG_FALSE ) && (psContext->eState != GDMA_FINISHING))
    {
	    /* Now link the new element onto the end of the list (unless this is the first element in the list) */
	    /* We delay -linking- the element until the last possible minute so that (a.) we can guarantee that	*/
	    /* the new element we're linking TO is correctly specified before we link to it and (b.) in case	*/
	    /* the hardware has started processing the last element in the list while we're in the process of	*/
	    /* adding a new element. In this case, we have entered 'finishing' state and we are not allowed to	*/
	    /* add to the list, as the DMAC hardware won't pick up the new element.								*/
		rResult = GDMA_HAL_SetLinkedListDescriptor( GDMA_apsHardwareInstances[psContext->ui32Channel],
													ui32LinkLast,
													((psContext->sLinkedListStatus.ui32Head == ui32LinkLast) ? IMG_TRUE : IMG_FALSE), /* If the element we're linking FROM is the head of the list, then inform the HAL */
													IMG_NULL,
													ui32LinkTail );
													
		if ( rResult != IMG_SUCCESS )
		{
			return rResult;	
		}
	}
	
	/* Submit all operations to the driver, so that they are correctly managed by QIO */
	rResult = gdma_SubmitOperationToDriver ( psContext,
											 psTransferObject,
											 0 );

	/* Final check that we haven't transitioned into the 'finishing' state while we've been in this function */
	IMG_ASSERT ( psContext->eState != GDMA_FINISHING );

	return rResult;
}


/*!
*******************************************************************************

 @Function              @GDMA_SingleShotOperation

*******************************************************************************/
IMG_RESULT GDMA_SingleShotOperation
(
    GDMA_sContext *					psContext,
    GDMA_sTransferObject *			psObject,
    IMG_INT32						i32Timeout
)
{
    /* Test have a valid context */
    IMG_ASSERT (psContext != IMG_NULL);
    IMG_ASSERT (psObject != IMG_NULL);
    
    if ( psContext->bLinkedListMode == IMG_TRUE )
    {
    	/* This function only applies to single shot operations */
    	return IMG_ERROR_UNEXPECTED_STATE;	
    }
    
    /* Submit the operation */
	return (gdma_SubmitOperationToDriver( psContext,
										  psObject,
										  i32Timeout ));
}


/*!
*******************************************************************************

 @Function              @GDMA_WaitObject

*******************************************************************************/
IMG_RESULT GDMA_WaitObject
(
    GDMA_sContext *					psContext,
    GDMA_sTransferObject **			ppsObject,
    IMG_INT32						i32Timeout
)
{
    QIO_IOCB_T *					psIOCB = IMG_NULL;
    QIO_IOPARS_T					sIOPars;
    QIO_STATUS_T					eStatus;
    QIO_DEVICE_T *					psDevice;

    /* Test have a valid context */
    IMG_ASSERT (psContext != IMG_NULL); 
    IMG_ASSERT (ppsObject != IMG_NULL);
    
    /* This function is redundant if QIO is not being used to queue operations */
    if (psContext->bBypassQIO == IMG_TRUE)
    {
    	return IMG_ERROR_OPERATION_PROHIBITED;	
    }

    /* This function may be called from the enabled, disabled or initialised states */
    if  (psContext->eState == GDMA_DEINITIALISED)
    {    	
        return IMG_ERROR_UNEXPECTED_STATE;
    }

    /* We must not block if in disabled or initialised states as no further transfers
       will complete - we can only pick up the results of completed/cancelled transfers. */
    if  ((psContext->eState == GDMA_INITIALISED) || (psContext->eState == GDMA_DISABLED))
    {
        i32Timeout = 0;
    }

    /* Wait for DMA to finish or time out */
    psIOCB = QIO_result (&psContext->sMBox, &psDevice, &eStatus, &sIOPars, i32Timeout);

    if (psIOCB == IMG_NULL)
    {
        /* we have timed out -> there is no completed transfer available at this time */
        return IMG_TIMEOUT;
    }

    /* Return the IOCB to its pool */
    KRN_returnPool(psIOCB);

    /* Set-up return pointer to object */
    *ppsObject = sIOPars.pointer;

    /* check that the context matches the object */
    IMG_ASSERT (psContext == sIOPars.spare);

    /* check the return status of the QIO transaction */
    if (eStatus == QIO_NORMAL)
    {
        return IMG_SUCCESS;
    }
    else if (eStatus == QIO_CANCEL)
    {
        return IMG_ERROR_CANCELLED;
    }
    else
    {
    	/* Should not receive any other QIO completion types */
        IMG_ASSERT(0);
    }
    
    return IMG_SUCCESS;
}


/*!
*******************************************************************************

 @Function              @GDMA_Disable

*******************************************************************************/
IMG_RESULT GDMA_Disable
(
    GDMA_sContext *					psContext
)
{
	IMG_RESULT	rResult;
	
    /* Test have a valid context */
    IMG_ASSERT (psContext != IMG_NULL);
	IMG_ASSERT( GDMA_apsHardwareInstances[psContext->ui32Channel] );

	if ( psContext->eState == GDMA_DISABLED )
	{
		/* Already disabled - nothing to do */
		return IMG_SUCCESS;	
	}

    /* This function may only be called from the idle or running states */
    if ((psContext->eState != GDMA_IDLE) && 
    	(psContext->eState != GDMA_RUNNING))
    {
        return IMG_ERROR_UNEXPECTED_STATE;
    }        

    /* Disable the DMAC hardware - both linked-list and standard enable
       are cleared as this code then covers both modes of operation. */
    rResult = GDMA_HAL_DisableHardware ( GDMA_apsHardwareInstances[psContext->ui32Channel] );
    if ( rResult != IMG_SUCCESS )
    {
    	return rResult;	
    }
    	
    /*
    *   Device enters the disabled state
    */
    psContext->ePreDisableState = psContext->eState;
    psContext->eState = GDMA_DISABLED;

    return IMG_SUCCESS;
}


/*!
*******************************************************************************

 @Function              @GDMA_Enable

*******************************************************************************/
IMG_RESULT GDMA_Enable
(
    GDMA_sContext *					psContext
)
{
	IMG_RESULT	rResult;
	
    /* Test have a valid context */
    IMG_ASSERT (psContext != IMG_NULL);
	IMG_ASSERT( GDMA_apsHardwareInstances[psContext->ui32Channel] );

    /* This function may only be called from the disabled state */
    if (psContext->eState != GDMA_DISABLED)
    {
        return IMG_ERROR_UNEXPECTED_STATE;
    }

	/* If we were running when we were disabled, then resume running, otherwise drop back into idle */
	if ( psContext->ePreDisableState == GDMA_RUNNING )
	{
		/* Cannot resuming linked list operation until we have enough list elements */
		if ((psContext->bLinkedListMode == IMG_TRUE) &&
			(psContext->sLinkedListStatus.ui32ListElementsInUse < psContext->sLinkedListStatus.ui32StartListThreshold))
		{		
			return IMG_ERROR_MINIMUM_LIMIT_NOT_MET;
		}
		else
		{
			psContext->eState = GDMA_RUNNING;	/* Set state first, in case we were near the end of a transfer, and the state is reset to 'idle' in ISR before we set it here */
			rResult = GDMA_HAL_EnableTransfer(GDMA_apsHardwareInstances[psContext->ui32Channel], (psContext->bLinkedListMode));
			if ( rResult != IMG_SUCCESS )
        	{
	        	psContext->eState = GDMA_DISABLED;	/* We've had a problem - switch back into 'disabled' and return an error */
    	  		return rResult;	
        	}
        }
	}
	else
	if ( psContext->ePreDisableState == GDMA_IDLE )
	{
		/* We were in idle mode before we were disabled, so switch back into it, unless we now have sufficient buffers to meet our 	*/
		/* autostart criteria.																										*/
		if ((psContext->bLinkedListMode == IMG_TRUE) &&
			(psContext->bAutostartEnabled == IMG_TRUE) &&
			(psContext->sLinkedListStatus.ui32ListElementsInUse >= psContext->sLinkedListStatus.ui32StartListThreshold))
		{
			rResult = GDMA_HAL_EnableTransfer(GDMA_apsHardwareInstances[psContext->ui32Channel], IMG_TRUE);
			if ( rResult != IMG_SUCCESS )
	        {
	        	psContext->eState = GDMA_DISABLED;	/* We've had a problem - switch back into 'disabled' and return an error */
	    	  	return rResult;	
	        }
	        else
	        {
	        	psContext->eState = GDMA_RUNNING;
	        }
		}
		else
		{
			psContext->eState = GDMA_IDLE;
		}
	}
	else
	{
		/* It should not be possible to have entered 'disable' mode from anything other than 'idle' or 'running' */
		IMG_ASSERT (0);		
	}

    return IMG_SUCCESS;
}


/*!
*******************************************************************************

 @Function              @GDMA_Flush

*******************************************************************************/
IMG_RESULT GDMA_Flush
(
    GDMA_sContext *					psContext
)
{
	IMG_RESULT 	rResult;
	IMG_UINT32	ui32ThisElement;
	
    /* Test have a valid context */
    IMG_ASSERT (psContext != IMG_NULL);

    /* This function may only be called from the disabled state */
    if  (psContext->eState != GDMA_DISABLED)
    {
        return IMG_ERROR_UNEXPECTED_STATE;
    }

	if (psContext->bBypassQIO == IMG_TRUE)
	{
		/* Manually call driver 'cancel all' function */
		GDMA_DriverCancel (&(psContext->sDevice));
		
		/* Now manually call the 'completion' function for all outstanding operations */
		if ((psContext->bLinkedListMode == IMG_FALSE) && (psContext->psCurrentSingleShotOperation != IMG_NULL))
		{
			rResult = gdma_DeviceCompletion ( psContext,
											  psContext->psCurrentSingleShotOperation,
											  QIO_CANCEL );
											  
			if (rResult != IMG_SUCCESS)
			{
				return rResult;
			}
		}
		else
		{
			ui32ThisElement = psContext->sLinkedListStatus.ui32ActiveBlockIndex;
			do
			{	
				IMG_ASSERT ( psContext->sLinkedListStatus.pasListElements [ui32ThisElement].psTransferObject != IMG_NULL );
				rResult = gdma_DeviceCompletion ( psContext,
												  psContext->sLinkedListStatus.pasListElements [ui32ThisElement].psTransferObject,
												  QIO_CANCEL );
												  
				if (rResult != IMG_SUCCESS)
				{
					return rResult;
				}
				
				ui32ThisElement ++;
				if ( ui32ThisElement >= psContext->sLinkedListStatus.ui32NoOfListElements )
				{	
					ui32ThisElement = 0;	
				}
			}
			while ( ui32ThisElement != psContext->sLinkedListStatus.ui32Last );	
		}
	}
	else
	{
    	/* cancel all queued transfers on the channel */
    	QIO_cancelAll(&(psContext->sDevice));
    }

	/* Reset list indices */
    psContext->sLinkedListStatus.ui32Head = 0;
    psContext->sLinkedListStatus.ui32Last = 0;
    psContext->sLinkedListStatus.ui32Tail = 0;
    psContext->sLinkedListStatus.ui32ListElementsInUse = 0;	

	/* Switch the 'before disabled' state into 'idle' - we can't re-enable after a flush */
	psContext->eState = GDMA_IDLE;
	psContext->ePreDisableState = GDMA_IDLE;

    /* Call the device specific cancel function (if provided) */
    if ((*psContext->sCallbackFunctions.pfnCancelDevice) != IMG_NULL)
    {
        (*psContext->sCallbackFunctions.pfnCancelDevice)(psContext->pvCallbackParameter);
    }

    return IMG_SUCCESS;
}


/*!
*******************************************************************************

 @Function              @GDMA_Unconfigure

*******************************************************************************/
IMG_RESULT GDMA_Unconfigure
(
    GDMA_sContext *					psContext
)
{
	IMG_RESULT	rResult;
	
    /* Test have a valid context */
    IMG_ASSERT (psContext != IMG_NULL);
	IMG_ASSERT( GDMA_apsHardwareInstances[psContext->ui32Channel] );

    /* This function may only be called from the disabled state */
    if (psContext->eState != GDMA_DISABLED)
    {
        return IMG_ERROR_UNEXPECTED_STATE;
    }

    /* Disable the QIO device, cancelling all queued operations */
    QIO_disable(&psContext->sDevice);

    /* Call the device specific cancel function (if provided) */
    if ((*psContext->sCallbackFunctions.pfnCancelDevice) != IMG_NULL)
    {
        (*psContext->sCallbackFunctions.pfnCancelDevice)(psContext->pvCallbackParameter);
    }
    
    /* Reset list indices */
    psContext->sLinkedListStatus.ui32Head = 0;
    psContext->sLinkedListStatus.ui32Last = 0;
    psContext->sLinkedListStatus.ui32Tail = 0;
    psContext->sLinkedListStatus.ui32ListElementsInUse = 0;
    
    /* Reset the DMAC hardware */
    rResult = GDMA_HAL_ResetHardware ( GDMA_apsHardwareInstances[psContext->ui32Channel] );
    if ( rResult != IMG_SUCCESS )
    {
    	return rResult;
    }
	else
	{
    	/* Device re-enters the initialised state */
    	psContext->eState = GDMA_INITIALISED;
    }

    return IMG_SUCCESS;
}

/*!
*******************************************************************************

 @Function              @GDMA_Reset

*******************************************************************************/
IMG_RESULT GDMA_Reset
(
    GDMA_sContext *				psContext
)
{
	IMG_RESULT				rResult;

    /* Test have a valid context */
    IMG_ASSERT (psContext != IMG_NULL);
	IMG_ASSERT( GDMA_apsHardwareInstances[psContext->ui32Channel] );

    /* This function may only be called from the initialised state */
    if (psContext->eState != GDMA_INITIALISED)
    {
        return IMG_ERROR_UNEXPECTED_STATE;
    }

    /* Reset the DMAC hardware */
    rResult = GDMA_HAL_ResetHardware ( GDMA_apsHardwareInstances[psContext->ui32Channel] );
    if ( rResult != IMG_SUCCESS )
    {
    	return rResult;	
    }

    /* Reset linked-list if required */
    if (psContext->bLinkedListMode == IMG_TRUE)
    {
        psContext->sLinkedListStatus.ui32Head = 0;
        psContext->sLinkedListStatus.ui32Last = 0;
        psContext->sLinkedListStatus.ui32Tail = 0;
        psContext->sLinkedListStatus.ui32ListElementsInUse = 0;
    }

    return IMG_SUCCESS;
}


/*!
*******************************************************************************

 @Function              @GDMA_Deinitialise

*******************************************************************************/
IMG_RESULT GDMA_Deinitialise
(
    GDMA_sContext *				psContext
)
{	
    /* Test have a valid context */
    IMG_ASSERT (psContext != IMG_NULL);

 	if ( psContext->eState != GDMA_INITIALISED )
 	{
 		/* Can only deinitialise from the 'intialised' state */
 		return IMG_ERROR_UNEXPECTED_STATE;	
 	}

	/* Unload the driver */
    QIO_unload(&psContext->sDevice);

    /* Device re-enters the deinitialised state */
    psContext->eState = GDMA_DEINITIALISED;
    
    return IMG_SUCCESS;
}


/*!
*******************************************************************************

 @Function              @GDMA_ReadyToStart

*******************************************************************************/
IMG_RESULT GDMA_ReadyToStart
(
    GDMA_sContext *					psContext,
    IMG_BOOL *						pbReadyToStart
)
{
    /* Test have a valid context and state */
    IMG_ASSERT (psContext != IMG_NULL);
    IMG_ASSERT (pbReadyToStart != IMG_NULL);

    /* This function may only be called from the idle, running or disabled state */
    if  (((psContext->eState != GDMA_IDLE) && 
    	 (psContext->eState != GDMA_DISABLED))
    	 ||
    	 (psContext->bAutostartEnabled == IMG_TRUE))
	{
        return IMG_ERROR_UNEXPECTED_STATE;
    }

	/* Assume we're not ready to start, unless it subsequently proves otherwise */
	*pbReadyToStart = IMG_FALSE;

    if (psContext->bLinkedListMode == IMG_TRUE)
    	
    {
    	if (psContext->sLinkedListStatus.ui32ListElementsInUse >= psContext->sLinkedListStatus.ui32StartListThreshold)
    	{
        	*pbReadyToStart = IMG_TRUE;
        }
    }
    else if (psContext->psCurrentSingleShotOperation != IMG_NULL)
    {
        *pbReadyToStart = IMG_TRUE;
    }
    
    return IMG_SUCCESS;
}


/*!
*******************************************************************************

 @Function              @GDMA_StartTransfer

*******************************************************************************/
IMG_RESULT GDMA_StartTransfer
(
	GDMA_sContext *					psContext
)
{
	IMG_RESULT						rResult = IMG_SUCCESS;
	IMG_UINT32						ui32CurrentTransferCount;
	ioblock_sBlockDescriptor	*	psBlockDesc;
	
    /* Test have a valid context */
    IMG_ASSERT (psContext != IMG_NULL);

	psBlockDesc = GDMA_apsHardwareInstances[psContext->ui32Channel];
	IMG_ASSERT( psBlockDesc );

    /* This function can only be called form 'idle', 'running' or 'finishing' states. If called from 	*/
    /* 'running' or 'finishing' states, the function will return 'success', but will not do anything.	*/
    if  ((psContext->eState != GDMA_IDLE) 		&&
    	 (psContext->eState != GDMA_RUNNING)	&& 
    	 (psContext->eState != GDMA_FINISHING))
	{
        return IMG_ERROR_UNEXPECTED_STATE;
    }

    /* This function must not be called for autostarting channels */
    if (psContext->bAutostartEnabled == IMG_TRUE)
    {
        return IMG_ERROR_UNEXPECTED_STATE;
    }

    /* Enable the DMAC hardware according to the mode selected */
    if (psContext->bLinkedListMode == IMG_TRUE)
    {
        if ((psContext->eState != GDMA_RUNNING) && 
        	(psContext->eState != GDMA_FINISHING))
		{
			/* For linked-list transfers, only enable if there are sufficient
			   objects queued. */
			if (psContext->sLinkedListStatus.ui32ListElementsInUse >= psContext->sLinkedListStatus.ui32StartListThreshold)
			{
				/* Reset DMAC */
				rResult = GDMA_HAL_ResetHardware ( psBlockDesc );
				if ( rResult != IMG_SUCCESS )
				{
					return rResult;	
				}

				/* Setup the DMAC */
				psContext->sLinkedListStatus.ui32ActiveBlockIndex = psContext->sLinkedListStatus.ui32Head;
				rResult = GDMA_HAL_PrepareLinkedListTransfer ( psBlockDesc, psContext->sLinkedListStatus.ui32Head );
				if ( rResult != IMG_SUCCESS )
				{
					return rResult;	
				}
				
				/* Setup the DMAC */				
				rResult = GDMA_HAL_EnableTransfer ( psBlockDesc, IMG_TRUE );
				if ( rResult != IMG_SUCCESS )
				{
					return rResult;	
				}								
			}
			else
			{
				return IMG_ERROR_MINIMUM_LIMIT_NOT_MET;
			}
		}
		else
		{
			return IMG_ERROR_OPERATION_PROHIBITED;
		}
    }
    else
    {
    	if (psContext->eState != GDMA_RUNNING)
    	{
	        /* For single-shot operation, only enable if a transfer is
	           programmed (i.e. the count_n register is non-zero). */
	        GDMA_HAL_GetCurrentTransferCount ( psBlockDesc, &ui32CurrentTransferCount );
			if ( rResult != IMG_SUCCESS )
			{
				return rResult;	
			}        
	           
	        if (ui32CurrentTransferCount != 0)
	        {
				rResult = GDMA_HAL_EnableTransfer ( psBlockDesc, IMG_FALSE );
				if ( rResult != IMG_SUCCESS )
				{
					return rResult;	
				}
			}
	        else
	        {
	            return IMG_ERROR_NOT_INITIALISED;
	        }
	    }
	    else
	    {
	    	return IMG_ERROR_OPERATION_PROHIBITED;
	    }
    }
    
    psContext->eState = GDMA_RUNNING;
    
	/* Call the device specific start function (if provided) */
    if ((*psContext->sCallbackFunctions.pfnStartDevice) != IMG_NULL)
    {
        (*psContext->sCallbackFunctions.pfnStartDevice)(psContext->pvCallbackParameter);
    }
    
    return IMG_SUCCESS;
}

/*!
*******************************************************************************

 @Function              @GDMA_GetListState

*******************************************************************************/
IMG_RESULT GDMA_GetListState
(
	GDMA_sContext *					psContext
)
{
	/* Test have a valid context */
    IMG_ASSERT (psContext != IMG_NULL);
	
    /* This function may not be called from the deinitialised state */
    if (psContext->eState == GDMA_DEINITIALISED)
    {
        return IMG_ERROR_NOT_INITIALISED;
    }

    /* Test if the list is empty */
    if (psContext->sLinkedListStatus.ui32ListElementsInUse == 0)
    {
        return IMG_ERROR_STORAGE_TYPE_EMPTY;
    }

    /* Test if the list is full */
    if (psContext->sLinkedListStatus.ui32ListElementsInUse >= psContext->sLinkedListStatus.ui32NoOfListElements)
    {
        return IMG_ERROR_STORAGE_TYPE_FULL;
    }

    /* List is running */
    return IMG_SUCCESS;
}

/*!
*******************************************************************************

 @Function              @GDMA_GetStatus

*******************************************************************************/
IMG_RESULT GDMA_GetStatus
(
	GDMA_sContext *					psContext,
    GDMA_eState *					peState
)
{	
	/* Test have a valid context */
    IMG_ASSERT (psContext != IMG_NULL);	
	
    /* Report the state machine current state */
    if ( peState != IMG_NULL )
    {
    	*peState = psContext->eState;
    }
    
    return IMG_SUCCESS;
}

/**************************************************************************************/
/********************** INTERNAL FUNCTIONS - NOT EXPOSED TO USER **********************/
/**************************************************************************************/

/*!
*******************************************************************************

 @Function              @gdma_DeviceCompletion

*******************************************************************************/
IMG_RESULT gdma_DeviceCompletion
(
	GDMA_sContext *				psContext,
	GDMA_sTransferObject *		psTransferObject,
	QIO_STATUS_T				eStatus
)
{
    IMG_ASSERT ( psContext != IMG_NULL );
    IMG_ASSERT ( psTransferObject != IMG_NULL );
    
    /* If running rather than flushing */
    if ((psContext->eState == GDMA_RUNNING) || 
    	(psContext->eState == GDMA_FINISHING))
    {
#if !defined (BOOT_CODE)
        if (psContext->bLinkedListMode == IMG_FALSE)
        {
#endif
        	IMG_ASSERT ( psContext->psCurrentSingleShotOperation != IMG_NULL );
        	psContext->psCurrentSingleShotOperation = IMG_NULL;     	
            psContext->eState = GDMA_IDLE;
#if !defined (BOOT_CODE)
        }
        else
        {
            if (psContext->eState == GDMA_FINISHING)
            {
	            psContext->eState = GDMA_IDLE;
            }
            else
            {
            	/* Update the 'hardware working on' index */
            	psContext->sLinkedListStatus.pasListElements [psContext->sLinkedListStatus.ui32ActiveBlockIndex].psTransferObject = IMG_NULL;
            	psContext->sLinkedListStatus.ui32ActiveBlockIndex ++;
            	if ( psContext->sLinkedListStatus.ui32ActiveBlockIndex >= psContext->sLinkedListStatus.ui32NoOfListElements )
            	{
            		psContext->sLinkedListStatus.ui32ActiveBlockIndex = 0;	
            	}
            	
	            if ((psContext->sLinkedListStatus.ui32ActiveBlockIndex == psContext->sLinkedListStatus.ui32Last) &&
	            	(psContext->sLinkedListStatus.bCircularList == IMG_FALSE))
	            {
	                /* Do not allow any new list elements to be submitted, as we are now in the process of finishing the list. */
	                psContext->eState = GDMA_FINISHING;
	            }            	
            }
            
			/* If this is not a circular list, remove the completed element from the list */
		    /* Decrement active counter */
		    if ( psContext->sLinkedListStatus.bCircularList == IMG_FALSE )
		    {
			    psContext->sLinkedListStatus.ui32ListElementsInUse --;
			    if (psContext->sLinkedListStatus.ui32ListElementsInUse == 0)
			    {
			    	/* List is now empty */
			        psContext->sLinkedListStatus.ui32Head = 0;
			        psContext->sLinkedListStatus.ui32Tail = 0;
			        psContext->sLinkedListStatus.ui32Last = 0;
			    }
			    else
			    {
			        /* Update head */
			        psContext->sLinkedListStatus.ui32Head ++;
			        if (psContext->sLinkedListStatus.ui32Head >= psContext->sLinkedListStatus.ui32NoOfListElements)
			        {
			            psContext->sLinkedListStatus.ui32Head = 0;
			        }
			    }
			}
        }
#endif /* !defined BOOT_CODE */

        /* SOC specific operations */
        if (psContext->sCallbackFunctions.pfnCompletion != NULL)
        {
            (*psContext->sCallbackFunctions.pfnCompletion)(psTransferObject, eStatus, psContext->pvCallbackParameter);
        }
    }
    else
    {
    	if (( psContext->eState != GDMA_DISABLED ) || ( eStatus != QIO_CANCEL ))
    	{
    		/* The only other reason we should be getting into the completion function is if a 'flush' has occurred, in which case 	*/
    		/* we will receive all queued operations with a 'CANCEL' flag.															*/
    		IMG_ASSERT (0);
    	}
    }

    /* Return */
    return IMG_SUCCESS;
}

/*!
*******************************************************************************

 @Function              @gdma_DeviceCompletionWrapper

*******************************************************************************/
IMG_INT32 gdma_DeviceCompletionWrapper
(
    QIO_DEVICE_T *pDev,
    QIO_IOCB_T   *iocb,
    QIO_IOPARS_T *ioPars,
    QIO_STATUS_T  status
)
{
	GDMA_sContext *				psContext;
	GDMA_sTransferObject *		psTransferObject;
	
	IMG_ASSERT ( ioPars != IMG_NULL );
	psContext = (GDMA_sContext *) ioPars->spare;
	psTransferObject = (GDMA_sTransferObject *) ioPars->pointer;
	gdma_DeviceCompletion ( psContext, psTransferObject, status );
	
    /* Return */
    return (IMG_FALSE);	
}


/*!
*******************************************************************************

 @Function              @gdma_SubmitOperationToDriver

*******************************************************************************/
IMG_RESULT gdma_SubmitOperationToDriver
(
    GDMA_sContext *					psContext,
    GDMA_sTransferObject *			psObject,
    IMG_INT32						i32Timeout
)
{
    QIO_IOCB_T *					psIOCB;
    QIO_IOPARS_T					sIOPars;
    IMG_RESULT						rResult;

    /* Test have a valid context */
    IMG_ASSERT (psContext != IMG_NULL);
    IMG_ASSERT (psObject != IMG_NULL);
    
    if ( psContext->bBypassQIO == IMG_TRUE )
    {
    	if ( i32Timeout != 0 )
    	{
    		/* No support for timeouts when QIO is not being used to queue objects */
    		return IMG_ERROR_INVALID_PARAMETERS;
    	}
    	
    	/* QIO is not being used to store operations until they are ready to be submitted to the 	*/
    	/* hardware, so only allow 'DriverStart' to be called if it's safe to do so immediately.	*/
	    if  (psContext->eState != GDMA_IDLE)
	   	{
	        return IMG_ERROR_UNEXPECTED_STATE;
	    }
	    
	    if ( psContext->psCurrentSingleShotOperation != IMG_NULL )
	    {
	    	/* An operation has been submitted, but not started. No support for queueing, so this is not allowed */
	    	return IMG_ERROR_BUSY;	
	    }
    	
    	/* Call start function directly */
    	rResult = GDMA_DriverStart ( psContext,
    								 psObject,
    								 IMG_NULL );
    								 
		if ( rResult != IMG_SUCCESS )
		{
			return rResult;	
		}
    }
    else
    {
	    /* This function may only be called from the idle, running or disabled states */
	    if  ((psContext->eState != GDMA_IDLE) 		&& 
	    	 (psContext->eState != GDMA_RUNNING) 	&& 
	    	 (psContext->eState != GDMA_DISABLED))
	   	{
	        return IMG_ERROR_UNEXPECTED_STATE;
	    }
    	
    	/* Take I/O control block from pool */
    	psIOCB = KRN_takePool(&(psContext->sIOCBPool), i32Timeout);

    	/* Timed out if do not get a valid IOCB from pool */
    	if (psIOCB == IMG_NULL)
    	{
	        return IMG_TIMEOUT;
    	}
    	
	    /*
	    *   The IO parameters are the context and the transfer object.
	    */
	    sIOPars.pointer = (IMG_VOID *)psObject;
	    sIOPars.spare   = (IMG_VOID *)psContext;
	    sIOPars.counter = 0;
	    sIOPars.opcode  = 0;
	
	    QIO_qio (&(psContext->sDevice), psIOCB, &sIOPars, &(psContext->sMBox), (QIO_COMPFUNC_T *)gdma_DeviceCompletionWrapper, KRN_INFWAIT);    	
    }

    return IMG_SUCCESS;
}

/**************************************************************************************/
/****************************** END OF INTERNAL FUNCTIONS *****************************/
/**************************************************************************************/


/******************************************************************************
**************************** End Global Functions *****************************
*******************************************************************************/
