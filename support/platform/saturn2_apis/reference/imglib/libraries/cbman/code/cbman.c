/*! 
******************************************************************************
 @file   : cbman_api.c

 @brief  

 @Author Alex Pim

 @date   05/06/2003
 
         <b>Copyright 2003 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to third parties without the
         express written permission of Imagination Technologies
         Limited, Unit 8, HomePark Industrial Estate,
         King's Langley, Hertfordshire, WD4 8LZ, U.K.

 <b>Description:</b>\n
         Callback Manager Implementation File

 <b>Platform:</b>\n
	     Meson 

 @Version	
	    -	1.0	1st Release 

******************************************************************************/
/* 
******************************************************************************
 Modifications :-

 $Log: cbman.c,v $

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

 This is major commit #1 of 3. There will be another major commit based
 upon the IMG Framework application and libraries, as well as another
 for scripts and tagging.

 -----> THIS IS NOT YET A USEABLE STRUCTURE <-----

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

 

*****************************************************************************/

#if defined (__META_MEOS__) || defined (__MTX_MEOS__)
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "img_defs.h"
#include "img_common.h"
#include "krn.h"
#include "list_manager.h"
#include "cbman.h"
#include "system.h"
#include "krn.h"


/*!
******************************************************************************

 These defines represent the errors that could have been returned from this 
 API.  The API now asserts.

******************************************************************************/
#define CB_MGR_ERR_NO_ERR							IMG_SUCCESS

#define CB_MGR_ERR_BASE								((img_result)0x20)

#define CB_MGR_ERR_COULD_NOT_ADD_CALLBACK			(LIST_MGR_ERR_BASE + 0)
#define CB_MGR_ERR_COULD_NOT_REMOVE_CALLBACK		(LIST_MGR_ERR_BASE + 1)
#define CB_MGR_ERR_NO_MORE_MODULES_ALLOWED			(LIST_MGR_ERR_BASE + 2)
#define CB_MGR_ERR_INVALID_SLOT_NUMBER				(LIST_MGR_ERR_BASE + 3)


static img_bool bInitialised = IMG_FALSE;

/*!
******************************************************************************

 This is an array that contains the registered callback systems for each module

******************************************************************************/
LIST_MGR_sControlBlock			CBMAN_sCallbackListControl	[CBMAN_MAX_MODULES];

/*!
******************************************************************************

 This is an array that contains the registered remote callback functions.

******************************************************************************/
IMG_pfnRemoteEventCallback	apfnRemoteEventCallback[CBMAN_MAX_MODULES];

/*!
******************************************************************************

 This is an array that contains the system in-use flags for each module.

******************************************************************************/
IMG_BOOL	gabSystemInUse[CBMAN_MAX_MODULES];


/*!
******************************************************************************

 Create a callback store for each of the possible callback systems

******************************************************************************/
CBMAN_sCallbackStore	CBMan_sCallbackList	[ CBMAN_MAX_MODULES ] [	CBMAN_MAX_NO_OF_CALLBACKS ];


/*!
******************************************************************************

 @Function				CBMAN_Initialise

******************************************************************************/
img_result CBMAN_Initialise ( img_void )
{
	bInitialised = IMG_TRUE;

	/* Return success */
	return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function				CBMAN_Deinitialise

******************************************************************************/
img_result CBMAN_Deinitialise ( img_void )
{
	IMG_UINT32 ui32Slot, ui32SlotsInUse;

	/* Check no slots are still in use */
	ui32SlotsInUse = 0;
	for (ui32Slot = 0; ui32Slot < CBMAN_MAX_MODULES; ui32Slot++)
	{
		ui32SlotsInUse += gabSystemInUse[ui32Slot] ? 1 : 0;
	}
	IMG_ASSERT(ui32SlotsInUse == 0);

	bInitialised = IMG_FALSE;

	/* Return success */
	return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function              CBMAN_IsInitialised

******************************************************************************/
IMG_BOOL CBMAN_IsInitialised(IMG_VOID)
{
	return bInitialised;
}

/*!
******************************************************************************

 @Function				CBMAN_RegisterMyCBSystem

******************************************************************************/
img_result CBMAN_RegisterMyCBSystem (	img_puint32		pui32AtSlot		)
{
	IMG_UINT32	ui32Slot;
	KRN_IPL_T	prevState;

	/* Check if there is a free slot */
	for (ui32Slot = 0; ui32Slot < CBMAN_MAX_MODULES; ui32Slot++)
	{
		if (!gabSystemInUse[ui32Slot])
		{
			gabSystemInUse[ui32Slot] = IMG_TRUE;
			break;
		}
	}
	if (ui32Slot == CBMAN_MAX_MODULES)
	{
		IMG_ASSERT (IMG_FALSE);
		*pui32AtSlot = IMG_NULL;

		return CB_MGR_ERR_NO_MORE_MODULES_ALLOWED;
	}

	prevState = KRN_raiseIPL();

	/* Return the systems next free slot to the caller */
	*pui32AtSlot = ui32Slot;

	/* Initialise the Callback System for this module */
	LIST_MGR_Initialise	(	&CBMAN_sCallbackListControl	[	ui32Slot	],
							CBMAN_MAX_NO_OF_CALLBACKS,
							&CBMan_sCallbackList	[	ui32Slot	],
							sizeof ( CBMAN_sCallbackStore ) );

	KRN_restoreIPL(prevState);
	
	/* Return success */
	return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function				CBMAN_UnregisterMyCBSystem

******************************************************************************/
img_result CBMAN_UnregisterMyCBSystem (	img_uint32		ui32Slot		)
{
	IMG_ASSERT(gabSystemInUse[ui32Slot]);

	gabSystemInUse[ui32Slot] = IMG_FALSE;
	apfnRemoteEventCallback[ui32Slot] = IMG_NULL;
	IMG_MEMSET(&CBMAN_sCallbackListControl[ui32Slot], 0, sizeof(CBMAN_sCallbackListControl[0]));
	IMG_MEMSET(&CBMan_sCallbackList[ui32Slot], 0, sizeof(CBMan_sCallbackList[0]));
	
	/* Return success */
	return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function				CBMAN_RegisterMyRemoteCBFunc
******************************************************************************/
img_result 
CBMAN_RegisterMyRemoteCBFunc(
	img_uint32						ui32AtSlot,
	IMG_pfnRemoteEventCallback		pfnRemoteEventCallback
)
{
	/* Check the slot number is valid */
	IMG_ASSERT (ui32AtSlot < CBMAN_MAX_MODULES);

	/* Save address of indirect callback function */
	apfnRemoteEventCallback[ui32AtSlot] = pfnRemoteEventCallback;

	/* Return success */
	return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function				CBMAN_AddCallbackToMyCBSystem

******************************************************************************/
img_result CBMAN_AddCallbackToMyCBSystem		(	img_uint32				ui32AtSlot,
													img_uint32				eCallbackType,
													IMG_pfnEventCallback	pfnCallbackToAdd,
													img_puint32				pCallbackParameter,
													CBMAN_hCallback *		phCallback
												)
{
	CBMAN_psCallbackStore			psNewStorageElement;
	CBMAN_psCallbackStore			psNowViewing;

	/* Check the slot number is valid */
	if (ui32AtSlot >= CBMAN_MAX_MODULES)
	{
		IMG_ASSERT (IMG_FALSE);
		return CB_MGR_ERR_INVALID_SLOT_NUMBER;
	}

	/* Check the pointer is valid */
	if ( pfnCallbackToAdd == IMG_NULL )
	{
		IMG_ASSERT (IMG_FALSE);
		return CB_MGR_ERR_COULD_NOT_ADD_CALLBACK;
	}
	else 
	{
		/* Loop over all event callbacks to see it this callback is already registered...*/
		psNowViewing = (CBMAN_psCallbackStore) CBMAN_sCallbackListControl[ ui32AtSlot ].psListHead;
		while (psNowViewing != IMG_NULL) 
		{
			/* Is this callback already registered for this event...*/
			if (
					(psNowViewing->pfCallback    == pfnCallbackToAdd) &&
					(psNowViewing->ui32EventType == eCallbackType)
				)
			{
				/* If the callback parameter is different there is something wrong */
				IMG_ASSERT(psNowViewing->pCallbackParameter == pCallbackParameter);

				/* Handle to the callback is just the address of the element we're using to store it */
				*phCallback = (CBMAN_hCallback) psNowViewing;

				/* Return success */
				return IMG_SUCCESS;
			}

			/* Move to the next in the list */
			psNowViewing = (CBMAN_psCallbackStore) (psNowViewing->sNavigation).psNext;
		} // while


		/* We need to add this callback...*/
		if ( LIST_MGR_GetElement (  &CBMAN_sCallbackListControl[ui32AtSlot],
									(img_pvoid *) &psNewStorageElement ) != CB_MGR_ERR_NO_ERR )
		{
			IMG_ASSERT (IMG_FALSE);
			return CB_MGR_ERR_COULD_NOT_ADD_CALLBACK;
		}
		else
		{

			/* Handle to the callback is just the address of the element we're using to store it */
			*phCallback = (CBMAN_hCallback) psNewStorageElement;

			/* Store the callback type */
			psNewStorageElement->ui32EventType	= eCallbackType;
			
			/* No callback type qualifier via this interface */
			psNewStorageElement->ui32EventTypeQualifier		= 0;
			psNewStorageElement->bEventTypeQualifierValid	= IMG_FALSE;
			
			/* Store the callback parameter */
			psNewStorageElement->pCallbackParameter	= pCallbackParameter;

			/* Store the callback function to execute */
			psNewStorageElement->pfCallback		= pfnCallbackToAdd;
		}
	}

	/* Return success */
	return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function				CBMAN_AddQualifiedCallbackToMyCBSystem

******************************************************************************/
img_result CBMAN_AddQualifiedCallbackToMyCBSystem(	img_uint32				ui32AtSlot,
													img_uint32				eCallbackType,
													img_uint32				ui32CallbackQualifier,
													IMG_pfnEventCallback	pfnCallbackToAdd,
													img_puint32				pCallbackParameter,
													CBMAN_hCallback *		phCallback
												)
{
	CBMAN_psCallbackStore			psNewStorageElement;
	CBMAN_psCallbackStore			psNowViewing;

	/* Check the slot number is valid */
	if (ui32AtSlot >= CBMAN_MAX_MODULES)
	{
		IMG_ASSERT (IMG_FALSE);
		return CB_MGR_ERR_INVALID_SLOT_NUMBER;
	}

	/* Check the pointer is valid */
	if ( pfnCallbackToAdd == IMG_NULL )
	{
		IMG_ASSERT (IMG_FALSE);
		return CB_MGR_ERR_COULD_NOT_ADD_CALLBACK;
	}
	else 
	{
		/* Loop over all event callbacks to see it this callback is already registered...*/
		psNowViewing = (CBMAN_psCallbackStore) CBMAN_sCallbackListControl[ ui32AtSlot ].psListHead;
		while (psNowViewing != IMG_NULL) 
		{
			/* Is this callback already registered for this event...*/
			if (
					(psNowViewing->pfCallback    == pfnCallbackToAdd) &&
					(psNowViewing->ui32EventType == eCallbackType) &&
					(psNowViewing->bEventTypeQualifierValid == IMG_TRUE) &&
					(psNowViewing->ui32EventTypeQualifier == ui32CallbackQualifier) 
				)
			{
				/* If the callback parameter is different there is something wrong */
				IMG_ASSERT(psNowViewing->pCallbackParameter == pCallbackParameter);

				/* Handle to the callback is just the address of the element we're using to store it */
				*phCallback = (CBMAN_hCallback) psNowViewing;

				/* Return success */
				return IMG_SUCCESS;
			}

			/* Move to the next in the list */
			psNowViewing = (CBMAN_psCallbackStore) (psNowViewing->sNavigation).psNext;
		} // while


		/* We need to add this callback...*/
		if ( LIST_MGR_GetElement (  &CBMAN_sCallbackListControl[	ui32AtSlot	],
									(img_pvoid *) &psNewStorageElement ) != CB_MGR_ERR_NO_ERR )
		{
			IMG_ASSERT (IMG_FALSE);
			return CB_MGR_ERR_COULD_NOT_ADD_CALLBACK;
		}
		else
		{

			/* Handle to the callback is just the address of the element we're using to store it */
			*phCallback = (CBMAN_hCallback) psNewStorageElement;

			/* Store the callback type */
			psNewStorageElement->ui32EventType	= eCallbackType;
			
			/* Store callback type qualifier and indicate valid */
			psNewStorageElement->ui32EventTypeQualifier		= ui32CallbackQualifier;
			psNewStorageElement->bEventTypeQualifierValid	= IMG_TRUE;
			
			/* Store the callback parameter */
			psNewStorageElement->pCallbackParameter	= pCallbackParameter;

			/* Store the callback function to execute */
			psNewStorageElement->pfCallback		= pfnCallbackToAdd;
		}
	}

	/* Return success */
	return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function				CBMAN_RemoveCallbackToMyCBSystem

******************************************************************************/
img_result CBMAN_RemoveCallbackToMyCBSystem (	img_uint32				ui32AtSlot,
												CBMAN_hCallback *		phCallback
											)
{

	/* Check the slot number is valid */
	if (ui32AtSlot >= CBMAN_MAX_MODULES)
	{
		IMG_ASSERT (IMG_FALSE);
		return CB_MGR_ERR_INVALID_SLOT_NUMBER;
	}

	/* Check the pointer is valid */
	if ( *phCallback == IMG_NULL )
	{
		IMG_ASSERT (IMG_FALSE);
		return CB_MGR_ERR_COULD_NOT_REMOVE_CALLBACK;
	}
	else 
	if ( LIST_MGR_FreeElement ( &CBMAN_sCallbackListControl[ ui32AtSlot ],
								 (img_pvoid *) *phCallback ) != LIST_MGR_ERR_NO_ERR )
	{
		IMG_ASSERT (IMG_FALSE);
		return CB_MGR_ERR_COULD_NOT_REMOVE_CALLBACK;
	}
	else
	{
		/* Clear callback handle */
		*phCallback = IMG_NULL;
	}	


	/* Return success */
	return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function				CBMAN_ExecuteCallbackWithEventType
 
******************************************************************************/
img_result CBMAN_ExecuteCallbackWithEventType (	img_uint32	ui32AtSlot,
												img_uint32	ui32EventType,
												img_uint32	ui32Param,
												img_void *	pvParam
												)		
{
	img_uint32					pass;
	CBMAN_psCallbackStore		psDefaultCB				= IMG_NULL;
	CBMAN_psCallbackStore		psNowViewing;
	CBMAN_psCallbackStore		psValidHandler;
	img_bool					bCBCalled				= IMG_FALSE;

	IMG_ASSERT(ui32EventType != IMG_EVENT_ALL);
	IMG_ASSERT(ui32EventType != IMG_EVENT_DEFAULT);
	/* This fucntion cannot be used with an associated remote CB - use CBMAN_ExecuteCB...*/
	IMG_ASSERT (apfnRemoteEventCallback[ui32AtSlot] == IMG_NULL) ;

	/* Check the slot number is valid */
	if (ui32AtSlot >= CBMAN_MAX_MODULES)
	{
		IMG_ASSERT (IMG_FALSE);
		return CB_MGR_ERR_INVALID_SLOT_NUMBER;
	}

	for (pass = 0; (pass < 2) && (bCBCalled == IMG_FALSE); pass++) 
	{
		/* Initiate our module callback list pointer */
		psNowViewing = (CBMAN_psCallbackStore) CBMAN_sCallbackListControl[ ui32AtSlot ].psListHead;
		
		/* loop over all event callbacks...*/
		while (psNowViewing != IMG_NULL) 
		{
			psValidHandler = IMG_NULL;
			if (pass == 0) { /* Check if there is a callback registered for this event */
				if ((psNowViewing->ui32EventType == ui32EventType) || (psNowViewing->ui32EventType == IMG_EVENT_ALL))
				{
					psValidHandler = psNowViewing;
				}
				/* also check for default handlers */
				if (psNowViewing->ui32EventType == IMG_EVENT_DEFAULT) 
				{
					/* Should only be one default callback registered */
					IMG_ASSERT(psDefaultCB == IMG_NULL);
					
					/* Save the CB so we can call this if needed...*/
					psDefaultCB = psNowViewing;
				}
			}

			/* Move to the next in the list */
			psNowViewing = (CBMAN_psCallbackStore) (psNowViewing->sNavigation).psNext;

			if (pass == 1) 
			{ // No handlers found so try the default handler
				psValidHandler	= psDefaultCB;
				psNowViewing	= IMG_NULL;
			}

			if (psValidHandler) 
			{
				bCBCalled = IMG_TRUE;

				/* Execute the callback */
				(psValidHandler->pfCallback) (ui32EventType, 
											psValidHandler->pCallbackParameter,
											ui32Param,
											pvParam
											);
			}
		} // while
	} // for
	
	return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function				CBMAN_ExecuteCallbackWithQualifiedEventType
 
******************************************************************************/
img_result CBMAN_ExecuteCallbackWithQualifiedEventType	(	img_uint32				ui32AtSlot,
															img_uint32				ui32EventType,
															img_uint32				ui32EventQualifier,
															img_uint32				ui32Param,
															img_void *				pvParam
														)
{
	img_uint32					pass;
	CBMAN_psCallbackStore		psDefaultCB				= IMG_NULL;
	CBMAN_psCallbackStore		psNowViewing;
	CBMAN_psCallbackStore		psValidHandler;
	img_bool					bCBCalled				= IMG_FALSE;

	IMG_ASSERT(ui32EventType != IMG_EVENT_ALL);
	IMG_ASSERT(ui32EventType != IMG_EVENT_DEFAULT);
	/* This function cannot be used with an associated remote CB - use CBMAN_ExecuteCB...*/
	IMG_ASSERT (apfnRemoteEventCallback[ui32AtSlot] == IMG_NULL) ;

	/* Check the slot number is valid */
	if (ui32AtSlot >= CBMAN_MAX_MODULES)
	{
		IMG_ASSERT (IMG_FALSE);
		return CB_MGR_ERR_INVALID_SLOT_NUMBER;
	}

	for (pass = 0; (pass < 3) && (bCBCalled == IMG_FALSE); pass++) 
	{
		/* Initiate our module callback list pointer */
		psNowViewing = (CBMAN_psCallbackStore) CBMAN_sCallbackListControl[ ui32AtSlot ].psListHead;
		
		/* loop over all event callbacks...*/
		while (psNowViewing != IMG_NULL) 
		{
			psValidHandler = IMG_NULL;
			if (pass == 0) 
			{ /* Check if there is a qualified callback registered for this event */
				if (
						((psNowViewing->ui32EventType == ui32EventType) || (psNowViewing->ui32EventType == IMG_EVENT_ALL)) &&
						(psNowViewing->ui32EventTypeQualifier	== ui32EventQualifier)	&&
						(psNowViewing->bEventTypeQualifierValid == IMG_TRUE)
					)
				{
					psValidHandler = psNowViewing;
				}
				/* also check for default handlers */
				if (psNowViewing->ui32EventType == IMG_EVENT_DEFAULT) 
				{
					/* Should only be one default callback registered */
					IMG_ASSERT(psDefaultCB == IMG_NULL);
					
					/* Save the CB so we can call this if needed...*/
					psDefaultCB = psNowViewing;
				}
			}
			else if (pass == 1) 
			{ /* Check if there is an unqualified callback registered for this event */
				if (
						(psNowViewing->bEventTypeQualifierValid == IMG_FALSE) &&
						((psNowViewing->ui32EventType == ui32EventType) || (psNowViewing->ui32EventType == IMG_EVENT_ALL))
					)
				{
					psValidHandler = psNowViewing;
				}
			}
			
			/* Move to the next in the list */
			psNowViewing = (CBMAN_psCallbackStore) (psNowViewing->sNavigation).psNext;

			if (pass == 2) 
			{ // No qualified or unqualified handlers found so try the default handler
				psValidHandler	= psDefaultCB;
				psNowViewing	= IMG_NULL;
			}

			if (psValidHandler) 
			{
				bCBCalled = IMG_TRUE;

				/* Execute the callback */
				(psValidHandler->pfCallback) (ui32EventType, 
											psValidHandler->pCallbackParameter,
											ui32Param,
											pvParam
											);
			}
		} // while
	} // for
	
	return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function				CBMAN_ExecuteCB
 
******************************************************************************/
img_result CBMAN_ExecuteCB	(
	img_uint32				ui32AtSlot,
	img_uint32				ui32APIId,
	img_uint32				ui32EventType,
	img_bool				bEventQualified,
	img_uint32				ui32EventQualifier,
	img_uint32				ui32Param,
	img_uint32				ui32Sizeui32ParamData,
	img_void *				pvParam,
	img_uint32				ui32SizepvParamData
)
{
	img_uint32					pass;
	CBMAN_psCallbackStore		psDefaultCB				= IMG_NULL;
	CBMAN_psCallbackStore		psNowViewing;
	CBMAN_psCallbackStore		psValidHandler;
	img_bool					bCBCalled				= IMG_FALSE;

	IMG_ASSERT(ui32EventType != IMG_EVENT_ALL);
	IMG_ASSERT(ui32EventType != IMG_EVENT_DEFAULT);

	/* Check the slot number is valid */
	if (ui32AtSlot >= CBMAN_MAX_MODULES)
	{
		IMG_ASSERT (IMG_FALSE);
		return CB_MGR_ERR_INVALID_SLOT_NUMBER;
	}

	/* Loop over the registers callbacks...*/
	for (pass=0; (pass < 3) && (bCBCalled == IMG_FALSE); pass++) 
	{
		/* Initiate our module callback list pointer */
		psNowViewing = (CBMAN_psCallbackStore) CBMAN_sCallbackListControl[ ui32AtSlot ].psListHead;
		
		/* loop over all event callbacks...*/
		while (psNowViewing != IMG_NULL) 
		{
			psValidHandler = IMG_NULL;
			if (pass == 0) 
			{ /* Check if there is a qualified callback registered for this event */
				if (
						((psNowViewing->ui32EventType == ui32EventType) || (psNowViewing->ui32EventType == IMG_EVENT_ALL))	&&
						(bEventQualified) &&
						(psNowViewing->ui32EventTypeQualifier	== ui32EventQualifier)	&&
						(psNowViewing->bEventTypeQualifierValid == IMG_TRUE)
					)
				{
					psValidHandler = psNowViewing;
				}
				/* also check for default handlers */
				if (psNowViewing->ui32EventType == IMG_EVENT_DEFAULT) 
				{
					/* Should only be one default callback registered */
					IMG_ASSERT(psDefaultCB == IMG_NULL);
					
					/* Save the CB so we can call this if needed...*/
					psDefaultCB = psNowViewing;
				}
			}
			else if (pass == 1) 
			{ /* Check if there is an unqualified callback registered for this event */
				if (
						(psNowViewing->bEventTypeQualifierValid == IMG_FALSE) &&
						((psNowViewing->ui32EventType == ui32EventType) || (psNowViewing->ui32EventType == IMG_EVENT_ALL))
					)
				{
					psValidHandler = psNowViewing;
				}
			}
			
			/* Move to the next in the list */
			psNowViewing = (CBMAN_psCallbackStore) (psNowViewing->sNavigation).psNext;

			if (pass == 2) 
			{ // No qualified or unqualified handlers found so try the default handler
				psValidHandler	= psDefaultCB;
				psNowViewing	= IMG_NULL;
			}

			if (psValidHandler != IMG_NULL) 
			{
				bCBCalled = IMG_TRUE;
				/* If there is no indirect callback registered...*/
				if (apfnRemoteEventCallback[ui32AtSlot] == IMG_NULL) 
				{
					/* Execute the callback */
					(psValidHandler->pfCallback) (ui32EventType, 
												psValidHandler->pCallbackParameter,
												ui32Param,
												pvParam
												);
				}
				else 
				{
					/* Execute the indirect callback */
					apfnRemoteEventCallback[ui32AtSlot] (ui32APIId,
														 psValidHandler->pfCallback,
														 ui32EventType, 
														 psValidHandler->pCallbackParameter,
														 ui32Param,
														 ui32Sizeui32ParamData,
														 pvParam,
														 ui32SizepvParamData
														 );
				}
			}
		} // while
	} // for
	
	return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function				CBMAN_ExecuteCallback
 
******************************************************************************/
extern	img_result CBMAN_ExecuteCallback (	CBMAN_hCallback			hCallback,
											img_uint32				ui32EventType,
											img_uint32				ui32Param,
											img_void *				pvParam
										 )
{
	CBMAN_psCallbackStore		psCallbackStore = (CBMAN_psCallbackStore) hCallback;

	IMG_ASSERT(psCallbackStore != IMG_NULL);

	/* Execute the callback */
	(psCallbackStore->pfCallback) (	ui32EventType, 
									psCallbackStore->pCallbackParameter,
									ui32Param,
									pvParam
								  );

	/* Return success */
	return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function				CBMAN_RemoveAllCallbacksOnSlot

******************************************************************************/
img_result CBMAN_RemoveAllCallbacksOnSlot		(	img_uint32				ui32AtSlot	)
{
	/* Check the slot number is valid */
	if (ui32AtSlot >= CBMAN_MAX_MODULES)
	{
		IMG_ASSERT (IMG_FALSE);
		return CB_MGR_ERR_INVALID_SLOT_NUMBER;
	}

	/* Remove all callbacks from this callback system - Just re-initialise the block! */
	LIST_MGR_Initialise	(	&CBMAN_sCallbackListControl	[	ui32AtSlot	],
							CBMAN_MAX_NO_OF_CALLBACKS,
							&CBMan_sCallbackList	[	ui32AtSlot	],
							sizeof ( CBMAN_sCallbackStore ) );


	/* Return success */
	return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function				CBMAN_RemoveQualifiedAllCallbacksOnSlot

******************************************************************************/
img_result CBMAN_RemoveQualifiedAllCallbacksOnSlot(
	img_uint32				ui32AtSlot,
	img_uint32				ui32CallbackQualifier
)
{
	CBMAN_psCallbackStore			psNowViewing;

	/* Check the slot number is valid */
	if (ui32AtSlot >= CBMAN_MAX_MODULES)
	{
		IMG_ASSERT (IMG_FALSE);
		return CB_MGR_ERR_INVALID_SLOT_NUMBER;
	}

	/* Loop over all event callbacks to see it this callback is already registered...*/
	psNowViewing = (CBMAN_psCallbackStore) CBMAN_sCallbackListControl[ ui32AtSlot ].psListHead;
	while (psNowViewing != IMG_NULL) 
	{
		/* If the callback is qualified with this qualifier...*/
		if (
				(psNowViewing->bEventTypeQualifierValid == IMG_TRUE) &&
				(psNowViewing->ui32EventTypeQualifier == ui32CallbackQualifier) 
			)
		{
			/* Free this element...*/
			if ( LIST_MGR_FreeElement ( &CBMAN_sCallbackListControl[ ui32AtSlot ],
										 (img_pvoid *) psNowViewing ) != LIST_MGR_ERR_NO_ERR )
			{
				IMG_ASSERT (IMG_FALSE);
				return CB_MGR_ERR_COULD_NOT_REMOVE_CALLBACK;
			}

			/* Restart at the head of the list...*/
			psNowViewing = (CBMAN_psCallbackStore) CBMAN_sCallbackListControl[ ui32AtSlot ].psListHead;
			continue;
		}

		/* Move to the next in the list */
		psNowViewing = (CBMAN_psCallbackStore) (psNowViewing->sNavigation).psNext;
	} // while

	/* Return success */
	return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function				CBMAN_GetNumberOfCallbacksOnSlot
 
******************************************************************************/
img_result CBMAN_GetNumberOfCallbacksOnSlot	(	img_uint32					ui32AtSlot,
												img_puint32					pui32NumberOfCallbacks,
												CBMAN_pfnTraversalFunction	pfnListTraversalFunction
											)
{
	CBMAN_psCallbackStore		psNowViewing;
	img_uint32					ui32CallbackCount = 0;

	/* Check the slot number is valid */
	if (ui32AtSlot >= CBMAN_MAX_MODULES)
	{
		IMG_ASSERT (IMG_FALSE);
		return CB_MGR_ERR_INVALID_SLOT_NUMBER;
	}

	/* Initiate our module callback list pointer */
	psNowViewing = (CBMAN_psCallbackStore) CBMAN_sCallbackListControl[ ui32AtSlot ].psListHead;

	/* Traverse the callback list */
	while ( psNowViewing != IMG_NULL )
	{
		/* Has a user list traversal function been provided ? */
		if ( pfnListTraversalFunction != IMG_NULL )
		{
			pfnListTraversalFunction ( 	psNowViewing,
										ui32CallbackCount,
										ui32AtSlot	);
		}
		
		ui32CallbackCount++;

		psNowViewing = (CBMAN_psCallbackStore) (psNowViewing->sNavigation).psNext;				

	} /* end of while */

	/* Return the amount of callbacks registered on this system */
	if ( pui32NumberOfCallbacks != IMG_NULL )
	{
		*pui32NumberOfCallbacks = ui32CallbackCount;
	}

	/* Return success */
	return IMG_SUCCESS;

}

/*****************************************************************************/
/*	END OF FILE */
/*****************************************************************************/


