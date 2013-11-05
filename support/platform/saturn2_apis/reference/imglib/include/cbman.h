/*!
******************************************************************************
 @file   : cbman.h

 @brief

 @Author Imagination Technologies

 @date   02/06/2003

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
         Callback Manager Header File

 <b>Platform:</b>\n
	     Platform Independent

 @Version
    -    1.0 First Release
    -    1.1 Move error codes which are no longer returned.

******************************************************************************/
/*
******************************************************************************
 Modifications:

 $Log: cbman.h,v $

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

  --- Revision Logs Removed --- 


*****************************************************************************/


#if !defined (__CBMAN_H__)
#define __CBMAN_H__

#include "list_manager.h"
#include "img_common.h"

#if defined (__cplusplus)
extern "C" {
#endif

/*!
******************************************************************************

 This typedef represents what a CB Manager callback is

******************************************************************************/
typedef void * CBMAN_hCallback;


/*!
******************************************************************************

 This structure holds information about a registered callback

******************************************************************************/
typedef struct	CBMAN_tag_sCallbackStore
{
	/*! Link to next element in the list			*/
	LIST_MGR_sNavigationElement		sNavigation;
	/*! Pointer to the the callback function		*/
	IMG_pfnEventCallback			pfCallback;
	/*! The associated event						*/
	img_uint32						ui32EventType;
	/*! The associated event qualifier (valid if bEventTypeQualifierValid = IMG_TRUE) */
	img_uint32						ui32EventTypeQualifier;
	/*! The associated event qualifier valid flag	*/
	img_bool						bEventTypeQualifierValid;
	/*! The associated "user" callback parameter	*/
	img_void *						pCallbackParameter;

} CBMAN_sCallbackStore, * CBMAN_psCallbackStore;

/*!
******************************************************************************

 @Function				CBMAN_Initialise

 @Description

 This function initialises the Callback Manager Utility

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
extern	img_result CBMAN_Initialise						(	img_void	 );


/*!
******************************************************************************

 @Function				CBMAN_Deinitialise

 @Description

 This function deinitialises the Callback Manager Utility

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
extern	img_result CBMAN_Deinitialise						(	img_void	 );

/*!
******************************************************************************

 @Function              CBMAN_IsInitialised

 @Description

 This function reports whether the API is initialised. Used to ensure a safe
 shutdown of the entire system by making sure that all initialised APIs have
 been deinitalised.

 @Input     None.

 @Return    IMG_BOOL		The state of the API's initialised flag.

******************************************************************************/
extern IMG_BOOL CBMAN_IsInitialised(IMG_VOID);

/*!
******************************************************************************

 @Function				CBMAN_RegisterMyCBSystem

 @Description

 This function allocates a slot for a modules callback system and initialises
 the appropriate memory

 @Input    pui32AtSlot	: Pointer to an img_uint32 that will recieve the allocated slot number.

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
extern	img_result CBMAN_RegisterMyCBSystem				(	img_puint32				pui32AtSlot			);

/*!
******************************************************************************

 @Function				CBMAN_UnregisterMyCBSystem

 @Description

 This function deallocates a slot for a modules callback system.

 @Input    ui32Slot	    : The slot number.

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
extern	img_result CBMAN_UnregisterMyCBSystem				(	img_uint32				ui32Slot			);

/*!
******************************************************************************

 @Function				CBMAN_RegisterMyRemoteCBFunc

 @Description

 This function is used to register an remote callback function.  The remote
 callback function will be called in preference to the applications callback.
 This provides a level of indirection which can be used to perform inter-thread
 comms and have the callback remote from the API.

 @Input		ui32AtSlot	: The slot that your callback system is at.

 @Input		pfnRemoteEventCallback : A pointer to the remote callback
						  function.  IMG_NULL if no remote callback is to
						  be register.

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
extern	img_result
CBMAN_RegisterMyRemoteCBFunc(
	img_uint32						ui32AtSlot,
	IMG_pfnRemoteEventCallback		pfnRemoteEventCallback
);

/*!
******************************************************************************

 @Function				CBMAN_AddCallbackToMyCBSystem

 @Description

 This function will add a callback to your callback system.

 @Input		ui32AtSlot			: The slot that your callback system is at.

 @Input		eCallbackType		: The event type of your callback system.

 @Input		pfnCallbackToAdd	: The Function that is to executed.

 @Input		pCallbackParameter  : Pointer to an application specified ID.

 @Input		phCallback			: A pointer to a handle that will be initialised.
								  This can be used to remove a callback.

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
extern	img_result CBMAN_AddCallbackToMyCBSystem		(	img_uint32				ui32AtSlot,
															img_uint32				eCallbackType,
															IMG_pfnEventCallback	pfnCallbackToAdd,
															img_puint32				pCallbackParameter,
															CBMAN_hCallback *		phCallback
														);

/*!
******************************************************************************

 @Function				CBMAN_AddQualifiedCallbackToMyCBSystem

 @Description

 This function will add a callback (with a qualifier value) to your callback
 system.

 @Input		ui32AtSlot			: The slot that your callback system is at.

 @Input		eCallbackType		: The event type of your callback system.

 @Input		ui32CallbackQualifier : The qualifier for the event type.

 @Input		pfnCallbackToAdd	: The function that is to executed.

 @Input		pCallbackParameter  : Pointer to an application specified ID.

 @Input		phCallback			: A pointer to a handle that will be initialised.
								  This can be used to remove a callback.

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
extern	img_result CBMAN_AddQualifiedCallbackToMyCBSystem(	img_uint32				ui32AtSlot,
															img_uint32				eCallbackType,
															img_uint32				ui32CallbackQualifier,
															IMG_pfnEventCallback	pfnCallbackToAdd,
															img_puint32				pCallbackParameter,
															CBMAN_hCallback *		phCallback
														);

/*!
******************************************************************************

 @Function				CBMAN_RemoveCallbackToMyCBSystem

 @Description

 This function will remove a callback from your callback system.

 @Input		ui32AtSlot			: The slot that your callback system is at.

 @Input		phCallback			: A pointer to the handle of the callback to be removed.


 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
extern	img_result CBMAN_RemoveCallbackToMyCBSystem		(	img_uint32				ui32AtSlot,
															CBMAN_hCallback	*		phCallback
														);

/*!
******************************************************************************

 @Function				CBMAN_ExecuteCallbackWithEventType

 @Description

 This function will execute any registered callbacks with a specific event type,
 that exists in your callback system.

 NOTE: CBMAN_ExecuteCB() must be used if the API supports has a
 remote calback registered via CBMAN_RegisterMyRemoteCBFunc();

 @Input		ui32AtSlot			: The slot that your callback system is at.

 @Input		ui32EventType		: The event type of the callbacks you wish to execute.

 @Input		ui32Param			: Parameter defined by event type.

 @Input		pvParam				: Parameter defined by event type.

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
extern	img_result CBMAN_ExecuteCallbackWithEventType	(	img_uint32				ui32AtSlot,
															img_uint32				ui32EventType,
															img_uint32				ui32Param,
															img_void *				pvParam
														);

/*!
******************************************************************************

 @Function				CBMAN_ExecuteCallbackWithQualifiedEventType

 @Description

 This function will execute any registered callbacks with a specific event type,
 that exists in your callback system.

 NOTE: CBMAN_ExecuteCB() must be used if the API supports has a
 remote calback registered via CBMAN_RegisterMyRemoteCBFunc();

 @Input		ui32AtSlot			: The slot that your callback system is at.

 @Input		ui32EventType		: The event type of the callbacks you wish to execute.

 @Input		ui32EventQualifier	: The event qualifier.

 @Input		ui32Param			: Parameter defined by event type.

 @Input		pvParam				: Parameter defined by event type.

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
extern	img_result CBMAN_ExecuteCallbackWithQualifiedEventType	(	img_uint32				ui32AtSlot,
																	img_uint32				ui32EventType,
																	img_uint32				ui32EventQualifier,
																	img_uint32				ui32Param,
																	img_void *				pvParam
																);

/*!
******************************************************************************

 @Function				CBMAN_ExecuteCB

  @Description

 This function will execute any registered callbacks with a specific event type,
 that exists in your callback system.

 NOTE: This function MUST be used in place of CBMAN_ExecuteCallbackWithQualifiedEventType()
 or CBMAN_ExecuteCallbackWithEventType() if the API supports has a
 remote calback registered via CBMAN_RegisterMyRemoteCBFunc();

 @Input		ui32AtSlot			: The slot that your callback system is at.

 @Input		ui32APIId	: The id of the API (on the remote thread).  This
						  should be uniquie on the thread as it is used to
						  serialise callbacks from the API.

 @Input		ui32EventType	: The event type of the callbacks you wish to execute.

 @Input		bEventQualified	: IMG_TRUE if #ui32EventQualifier is valid
						and should be used in locating the correct
						callback.

 @Input		ui32EventQualifier	: The event qualifier.

 @Input		ui32Param	: A uint32 value to be passed to the callback.  The
						  value/meaning is dependent upon the event being signalled.

						  NOTE: If #ui32Sizeui32ParamData is not 0 but #ui32Param
						  is NOT being used to pass back an address then
						  #ui32Param MUST be set to IMG_NULL.

 @Input		ui32Sizeui32ParamData : If ui32Param is being used to pass a pointer
						  to some data then this contains the size (in bytes) of the
						  data.  This allows the code calling the callback to flush
						  the cashe to ensure that the data is "current" and
						  not stale.  0 if ui32Param does not point to data being
						  passed.

 @Input		pvParam		: A pointer value to be passed to the callback.  The
						  value/meaning is dependent upon the event being signalled.

						  NOTE: If ui32SizepvParamData is not 0 but #pvParam
						  is NOT being used to pass back an address then
						  #pvParam MUST be set to IMG_NULL.

 @Input		ui32SizepvParamData : If pvParam is being used to pass a pointer
						  to some data then this contains the size (in bytes) of the
						  data.  This allows the code calling the callback to flush
						  the cashe to ensure that the data is "current" and
						  not stale.  0 if pvParam does not point to data being
						  passed.

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
extern
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
);

/*!
******************************************************************************

 @Function				CBMAN_ExecuteCallback

 @Description

 This function is used to execute a specific callback.

 @Input		hCallback			: The handle to the callback.

 @Input		ui32EventType		: The event type.

 @Input		ui32Param			: Parameter defined by event type.

 @Input		pvParam				: Parameter defined by event type.

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
extern	img_result CBMAN_ExecuteCallback (	CBMAN_hCallback			hCallback,
											img_uint32				ui32EventType,
											img_uint32				ui32Param,
											img_void *				pvParam
										 );

/*!
******************************************************************************

 @Function				CBMAN_RemoveQualifiedAllCallbacksOnSlot

 @Description

 This function will remove all registered callbacks on a slot for a specified
 qualifier.

 @Input		ui32AtSlot		: The slot that your callback system is at.

 @Input		ui32CallbackQualifier : The qualifier for which callbacks are to be
							removed.

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
extern	img_result CBMAN_RemoveQualifiedAllCallbacksOnSlot(
	img_uint32				ui32AtSlot,
	img_uint32				ui32CallbackQualifier
);

/*!
******************************************************************************

 @Function				CBMAN_RemoveAllCallbacksOnSlot

 @Description

 This function will remove all registered callbacks on a slot.

 @Input		ui32AtSlot		: The slot that your callback system is at.

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
extern	img_result CBMAN_RemoveAllCallbacksOnSlot		(	img_uint32				ui32AtSlot	);

/*!
******************************************************************************

 @Function				CBMAN_pfnListTraversalCallback

 @Description

 This is the prototype for a callback list traversal user function.
 Such a function can be passed to the 'CBMAN_GetNumberOfCallbacksOnSlot'
 utility function - the registered function will then be called for each of the
 callbacks traversed as the utility function counts callbacks registered on a
 given slot.

 @Input    CBMAN_psCallbackStore	: The current callback being viewed by the
 									  traversal function.

 @Input    ui32Count	: The number of callbacks previously viewed by the
						  traversal function.

 @Input	   ui32Slot		: The callback slot to which this callback belongs.

******************************************************************************/
typedef img_result ( * CBMAN_pfnTraversalFunction) (
	CBMAN_psCallbackStore		psThisCallback,
	img_uint32					ui32Count,
	img_uint32					ui32Slot
);

/*!
******************************************************************************

 @Function				CBMAN_GetNumberOfCallbacksOnSlot

 @Description

 This function will execute any registered callbacks with a specific event type,
 that exists in your callback system.

 @Input		ui32AtSlot			: The slot that your callback system is at.

 @Input		ui32EventType		: The number of callbacks registered on this slot (in this system).

 @Input		pfnListTraversalFunction : Pointer to a function matching the 'CBMAN_pfnTraversalFunction'
 									   prototype. The provided function will be called for each
 									   callback located on the given slot. Should be set to
 									   'IMG_NULL' if no traversal function is provided.

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
extern	img_result CBMAN_GetNumberOfCallbacksOnSlot	(	img_uint32					ui32AtSlot,
														img_puint32					pui32NumberOfCallbacks,
														CBMAN_pfnTraversalFunction	pfnListTraversalFunction
													);


#if defined (__cplusplus)
}
#endif

#endif /* __CBMAN_H__	*/
