#if !defined (__GDMA_API_H__)
#define __GDMA_API_H__

#include "img_defs.h"
#include "ioblock_defs.h"

/*
******************************************************************************
*	Controls values for RPC code generation
*****************************************************************************/
#ifdef __RPCCODEGEN__
#define	rpc_prefix		GDMA
#define	rpc_filename	gdma_api
#endif

#include <MeOS.h>

#if !defined GDMA_HW_IMPLEMENTATION
	#error "No DMAC hardware implementation has been specified for the GDMA API"
#endif

#define GDMA_SYSBUS 0
#define GDMA_MDC	1

#if GDMA_HW_IMPLEMENTATION == GDMA_SYSBUS
	#include "gdma_sysbus.h"
#elif GDMA_HW_IMPLEMENTATION == GDMA_MDC
	#include "gdma_mdc.h"
#endif

/******************************************************************************
******************************** Enumerations *********************************
*******************************************************************************/


/*!
*******************************************************************************

 This defines the status of the DMAC control block's state machine.

*******************************************************************************/
typedef enum
{
    /*! Module un-initialised */
    GDMA_DEINITIALISED = 0,

    /*! Module initialised */
    GDMA_INITIALISED,

    /*! Channel opened to allow transfers */
    GDMA_IDLE,

    /*! Transfers disabled */
    GDMA_DISABLED,

    /*! Transfer running */
    GDMA_RUNNING,

	/*! DMAC finishing */
    GDMA_FINISHING,

    /*! Number of module states */
    GDMA_STATE_MAX

} GDMA_eState;


/******************************************************************************
****************************** End enumerations *******************************
*******************************************************************************/


/******************************************************************************
****************************** Macro Definitions ******************************
*******************************************************************************/

/*! Wait until resources become available */
#define GDMA_INFWAIT   -1

/*! Return immediately if there are no free resources */
#define GDMA_NOWAIT     0


/******************************************************************************
**************************** End Macro Definitions ****************************
*******************************************************************************/

/*!
*******************************************************************************

 This structure defines the poolable transfer object used by the DMAC module.

*******************************************************************************/
typedef struct tag_gdma_txfr_object
{
    /*! Pool information */
    KRN_POOLLINK_T			pool;

    /*! Read pointer */
    IMG_UINT8 *				pui8ReadPointer;

    /*! Auto increment read pointer */
    IMG_BOOL				bIncReadPointer;

    /*! Read width in bytes */
    IMG_UINT32				ui32ReadWidth;

    /*! Write pointer */
    IMG_UINT8 *				pui8WritePointer;

    /*! Auto increment write pointer */
    IMG_BOOL				bIncWritePointer;

    /*! Write width in bytes */
    IMG_UINT32				ui32WriteWidth;

    /*! Size of buffer */
    IMG_UINT32				ui32SizeInBytes;

    /*! Transfer priority */
    IMG_UINT8				ui8Priority;

    /*! Byte swapping */
    IMG_BOOL				bReverseEndianism;

    /*! Wait Unpack */
    IMG_BOOL                bWaitUnpack;

    /*! Hold off */
    IMG_UINT32				ui32HoldOff;

    /*! Burst size in bytes */
    IMG_UINT32				ui32BurstSize;

	/*! User data */
	IMG_VOID *				pvUserData;

	/*! User data size */
	IMG_UINT32				ui32UserDataSize;

} GDMA_sTransferObject;

/*!
*******************************************************************************

 This defines the device specific function type that can be used to extend
 the standard QIO device driver functions within the DMAC device driver.\n

 It is common to the extension functions for init, start and cancel. There is
 no extension function for isr as the completion function performs this task.

 \param
 	IMG_VOID *
 	Pointer provided by user when DMAC was initialised.

*******************************************************************************/
typedef IMG_RESULT (*GDMA_pfnDevice)(IMG_VOID *);


/*!
*******************************************************************************

 This type defines the DMAC device driver completion function type.\n

 This function (if registered) will be called from the standard DMAC
 completion function on completion of a transfer and can be used to perform
 any device specific operation required. Note: When in linked list mode and
 using a circular linked list, this function may be called repeatedly for each
 element, as the list cycles. In circular linked list mode, this callback
 function should not be used to indicate that a transfer block is no longer
 required, only that it has been processed by the hardware.

 \param
 	GDMA_sTransferObject *
 	Pointer to the completed DMA transfer object.

 \param
 	QIO_STATUS_T
 	The manner in which this operation completed.

 \param
 	IMG_VOID *
 	Pointer provided by user when DMAC was initialised.

*******************************************************************************/
typedef IMG_RESULT (*GDMA_pfnCompletion)(GDMA_sTransferObject *, QIO_STATUS_T, IMG_VOID *);

/*!
*******************************************************************************

 This describes the peripheral specific device driver extension functions.

*******************************************************************************/
typedef struct tag_gdma_device_functions
{
    /*! Pointer to the peripheral specific init function */
    GDMA_pfnDevice            	pfnInitDevice;

    /*! Pointer to the peripheral specific start function */
    GDMA_pfnDevice            	pfnStartDevice;

    /*! Pointer to the peripheral specific cancel function */
    GDMA_pfnDevice            	pfnCancelDevice;

    /*! Pointer to the peripheral specific completion function. This function 	*/
    /*	pointer may not be set to IMG_NULL when the GDMA API is operating in  	*/
    /*  'bypassQIO' mode. This is because normally (1) QIO posts details of 	*/
    /*	completed operations to the mailbox contained in the master context		*/
    /*	structure and (2) the 'GDMA_WaitObject' function is used to block until	*/
    /*  a completed operation is available for collection from the mailbox.		*/
    /* 	When operations are not being queued by QIO, this mechanism is not		*/
    /* 	available (and the 'GDMA_WaitObject' function will return an error if	*/
    /*	called. As such, the callback functions are the only means by which the	*/
    /* 	user is informed of operations completing - because, in this mode, 		*/
    /* 	it is not possible to queue operations, the user must use the 			*/
    /* 	completion callback to ensure that new operations are not submitted		*/
    /*	before the previous one is complete.									*/
    GDMA_pfnCompletion     	 	pfnCompletion;

} GDMA_sCallbackFunctions;


/*!
*******************************************************************************

 This wraps the hardware specific linked list descriptor object in a standard
 structure.

*******************************************************************************/
typedef struct GDMA_sWrappedLinkedListDescriptor_tag
{
	/*! Hardware specific linked list descriptor. This structure is declared in the header file for the DMA hardware being used */
	GDMA_sLinkedListDescriptor	sLinkedListDescriptor;

	/*! A pointer to the transfer object which was used to set up this linked list descriptor. This pointer is only used when	*/
	/*  the GDMA API is being used in 'bypassQIO' mode, in which case there are no IO control blocks being returned by QIO from	*/
	/*  which to obtain the transfer object originally submitted. As such, it is necessary to store this pointer along with	the	*/
	/*  linked list descriptors themselves.																						*/
	GDMA_sTransferObject *		psTransferObject;

} GDMA_sWrappedLinkedListDescriptor;

/*! Macro used to allocate a linked list structure. This should be used to allocate storage space for linked list descriptors.	*/
/* 	The macro will create an array of name <listName> and <size> linked list descriptors. The same values used as parameters to	*/
/* 	this array should then be used as parameters 'pasLinkedListElements' and 'ui32NoOfLinkedListElements' in the subsequent 	*/
/*	to the function 'GDMA_Initialise' (see below).																				*/
#define GDMA_ALLOCATE_LINKED_LIST(listName, size) GDMA_sWrappedLinkedListDescriptor listName[(size*(sizeof(GDMA_sWrappedLinkedListDescriptor)))] __attribute__ ((aligned(8)))

/*!
*******************************************************************************

 This describes the current status of the linked list transfer. This structure
 is not used when operating in single shot mode.

        Example linked list using 5 of 7 available transfer descriptors.
        Head, Tail and Last indices shown. Numbers show order in which
        operations were submitted. 'x' indicates an unused descriptor.

      -------------------------------------------------------------------
      |                                                                 |
      |  |-----|  |-----|  |-----|  |-----|  |-----|  |-----|  |-----|  |
      |  |     |  |     |  |     |  |     |  |     |  |     |  |     |  |
      |->| 4th |->| 5th |  |  x  |  |  x  |  | 1st |->| 2nd |->| 3rd |--|
    	 |     |  |     |  |     |  |     |  |     |  |     |  |     |
    	 |-----|  |-----|  |-----|  |-----|  |-----|  |-----|  |-----|
                     ^        ^                 ^
                     |        |                 |
    		       Last     Tail              Head
    		       (=1)     (=2)              (=4)

       In the example diagram above, NoOfListElements = 7 and
       ListElementsInUse = 5

*******************************************************************************/
typedef struct tag_gdma_linkedlist_status
{
    /*! Pointer to list elements. These should be declared using the 		*/
    /*  'GDMA_ALLOCATE_LINKED_LIST' macro.									*/
    GDMA_sWrappedLinkedListDescriptor *   pasListElements;

    /*! The number of wrapped linked list descriptor elements pointed to by */
    /*	'pasListElements'. This number should be identical to the 'size'	*/
    /*	parameter passed to the 'GDMA_ALLOCATE_LINKED_LIST' macro.			*/
    IMG_UINT32		ui32NoOfListElements;

    /*! The transfer control block containing the oldest operation provided	*/
    /*	by the user. 														*/
    IMG_UINT32		ui32Head;

    /*! The transfer control block containing the most recently provided 	*/
    /*	operation.													 		*/
    IMG_UINT32		ui32Last;

    /*! The next unused transfer control block. */
    IMG_UINT32		ui32Tail;

    /*! The index of the block the hardware is currently working on. */
    IMG_UINT32		ui32ActiveBlockIndex;

    /*! The number of list elements that are currently in use. This 		*/
    /* 	includes:															*/
    /*	- Elements which have been submitted but not yet processed by the	*/
    /*	  DMAC hardware.													*/
    /*	- The element which the hardware is currently working on.			*/
    IMG_UINT32		ui32ListElementsInUse;

    /*! Start list threshold - there must be at least this many elements in */
    /*	the linked list before it may be started. If autostart is enabled	*/
    /*	then the hardware will begin processing the list when this 			*/
    /* 	threshold is reached. If autostart is not enabled, then attempts	*/
    /* 	to manually start operation (using 'GDMA_StartTransfer') will fail	*/
    /*  if there are less than the specified number of list elements 		*/
    /*  present.															*/
    IMG_UINT32		ui32StartListThreshold;

    /*! Circular list - a flag indicating whether the current list has an	*/
    /* 	end point, or will continue ad infinitum. If the latter, then 		*/
    /*  no transfer control blocks will be freed until the list is flushed	*/
    /*  or the API unconfigured.											*/
    IMG_BOOL		bCircularList;

} GDMA_sLinkedListStatus;

/*!
*******************************************************************************

 This describes the GDMA API context.

*******************************************************************************/
typedef struct tag_gdma_context
{
	/* Device name */
	IMG_UINT8					aui8DeviceName [20];

	/*! The index of the instance of DMA hardware being controlled by this 	*/
	/*	API context. 														*/
	IMG_UINT32					ui32Channel;

	/*! 'Channel group' of which this hardware channel is a member.			*/
	/* 	Ignored if DMA hardware does not support channel grouping.			*/
	IMG_UINT8					ui8ChannelGroup;

    /*! The current state of this DMA API context.							*/
    GDMA_eState    				eState;

    /*! Pre-disable state. Because the 'Disabled' state may be entered and 	*/
    /*	exited from several states, it is necessary to remember the state	*/
    /* 	the API was in before the disable, to enable the correct action	to	*/
    /* 	be taken if 'GDMA_Enable' is subsequently called.					*/
    GDMA_eState					ePreDisableState;

	/*! A structure containing the user registered callback functions.		*/
	GDMA_sCallbackFunctions		sCallbackFunctions;

	/*! User specified callback parameter. The user can specify a single	*/
	/* 	pointer when 'GDMA_Initialise' is called. The value provided will	*/
	/* 	then be returned as a parameter to all of the GDMA user specified	*/
	/*	callback functions.													*/
	IMG_VOID *					pvCallbackParameter;

    /*! A flag indicating whether the GDMA API is being operated in linked	*/
    /*	list or single shot mode.											*/
    IMG_BOOL					bLinkedListMode;

	/*! A structure containing all information specific to linked list 	 	*/
	/*	operation.															*/
	GDMA_sLinkedListStatus		sLinkedListStatus;

    /*! A flag indicating whether 'autostart mode' is enabled.				*/
    IMG_BOOL					bAutostartEnabled;

    /*! The QIO device object - unused when operating in 'bypassQIO' mode.	*/
    QIO_DEVICE_T				sDevice;

    /*! The QIO driver object - unused when operating in 'bypassQIO' mode.	*/
    QIO_DRIVER_T *				sDriver;

    /*! A mailbox to which QIO will post completed operations - unused when */
    /*  operating in 'bypassQIO' mode.										*/
    KRN_MAILBOX_T				sMBox;

    /*! A pool of IO control blocks which are used to describe operations 	*/
    /* 	to QIO - unused when operating in 'bypassQIO' mode.					*/
    KRN_POOL_T					sIOCBPool;

    /*! A flag indicating whether QIO is being used to queue operations.	*/
    IMG_BOOL					bBypassQIO;

    /*! A pointer to a transfer control block describing the currently 		*/
    /*	active single shot operation. This pointer is unused when operating	*/
    /* 	in linked list mode.												*/
    GDMA_sTransferObject *		psCurrentSingleShotOperation;

} GDMA_sContext;

/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

/*!
*******************************************************************************

 @Function              @GDMA_Define

 Declares an instance of DMA hardware. This must be called before all other
 GDMA functions for a given hardware instance.

  \param
  	psThisHWInstance
  	A pointer to a hardware block descriptor.

  \return
  	IMG_RESULT
  	Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  	did not complete successfully.

*******************************************************************************/
IMG_RESULT	GDMA_Define
(
	ioblock_sBlockDescriptor *	psThisHWInstance
);


/*!
*******************************************************************************

 @Function              @GDMA_Initialise

 Initialises driver & resets hardware. After a given DMA hardware instance has
 been instantiated using 'GDMA_Define', the instance must be initialised using
 this function before any other GDMA functions will succeed for that context.

 The context which is passed to this function must be in the
 'GDMA_DEINITIALISED' state prior to the call. Following the call, the context
 will be left in the 'GDMA_INITIALISED' state.

 \param
  	psContext
  	A pointer to a user declared 'GDMA_sContext' structure. It is not necessary
  	for the user to concern themselves with the contents of this structure, only
  	to declare its existence. This structure is set up by the 'GDMA_Initialise'
  	function and a pointer to the structure must be passed in to every other
  	GDMA function call in order to differentiate between DMA hardware channels
  	(one 'GDMA_sContext' structure must be allocated for each DMA hardware
  	channel being used).

 \param
	ui32HWChannel
	The index of the DMA hardware channel to which the context structure
	referenced in 'psContext' refers. This parameter is the only place where
	the user needs to specify the hardware index - after this function call,
	this hardware is identified through its context structure.

 \param
	ui8ChannelGroup
	Some DMA hardware implementations allow the grouping of different hardware
	channels, such that relative priorities can be assigned to each group. This
	parameter allows the user to associate the specified DMA hardware channel
	with a group ID. If the DMA hardware implementation does not support channel
	grouping then this parameter will be ignored.

 \param
	bLinkedListMode
	Indicates whether the DMA is to be used to transfer linked lists or single
	block elements. It is not possible to change this mode without first
	deinitialising the API.

 \param
	psCallbackFunctions
	A pointer to a 'GDMA_sCallbackFunctions' structure containing the pointers
	of user registered functions which the GDMA API will call on certain events.
	See definition of callback functions structure for more details.

 \param
	pvCallbackParameter
	A void pointer which the user can optionally specify. The GDMA API will pass
	this pointer back as a parameter to all user registered callback functions
	specified through the 'psCallbackFunctions' parameter.

 \param
	pasLinkedListElements
	A pointer to storage space for the linked list descriptor elements. These
	should be allocated using the 'GDMA_ALLOCATE_LINKED_LIST' macro - the same
	name specified in the 'listName' parameter to the macro should be used
	for this function parameter. If 'bLinkedListMode' is set to 'IMG_FALSE'
	then this parameter MUST be set to 'IMG_NULL'.

 \param
	ui32NoOfLinkedListElements
	The number of linked list descriptor elements contained in the storage
	space referenced by the 'pasLinkedListElements' parameter. The same
	value specified in the 'size' parameter to the 'GDMA_ALLOCATE_LINKED_LIST'
	macro should be used for this function parameter. If 'bLinkedListMode' is
	set to 'IMG_FALSE' then this parameter MUST be set to 0.

 \param
	bAutostartEnabled
	A flag indicating whether 'autostart mode' is enabled - autostart mode is
	used to cause submitted transfers to begin automatically (without a call
	to 'GMDA_StartTransfer'). For linked list operation, the DMA hardware will
	not be automatically started until the 'Start List Threshold' has been reached.
	When operating in single shot mode, 'autostart mode' can still be used, but
	it is not possible specify an 'autostart threshold' (the 'threshold' is
	effectively locked to a value of one) - when autostart is enabled in single
	shot mode, a submitted transfer will be started as soon as the hardware
	is idle. Queued	single shot transfers (non QIO bypass mode only - see
	'bBypassQIO', below) will automatically start as soon as the previous transfer
	is finished. When 'autostart mode' is NOT enabled for single shot transfers,
	then EVERY submitted transfer must be started manually using 'GDMA_StartTransfer'.
	As this function can only be called when a new transfer is queued and ready to
	start, the user must wait until the hardware has returned to idle, after
	finishing the previous transfer, before 'start transfer' is called again (it is
	NOT possible to queue n transfers and then call 'GDMA_StartTransfer' n times
	in quick succession - the starts must be called when one transfer has finished
	and the next is lined up and ready to go). In non QIO bypass mode, this can
	be achieved by using 'GDMA_WaitObject' to indicate when the previous transfer
	has finished. In QIO bypass mode, the 'operation complete' callback should be
	used to indicate when an operation has finished (see 'psCallbackFunctions',
	above).

 \param
	ui32StartListThreshold
	The minimum number of linked list elements which must be present before
	the DMA hardware can start processing the list. If 'autostart' is not enabled
	then this threshold is still used, and will cause calls to 'GDMA_StartTransfer'
	to fail if the threshold has not yet been reached. If 'autostart' is enabled
	then the list processing will automatically begin when this threshold is
	reached. If 'bLinkedListMode' is set to 'IMG_FALSE' then this parameter MUST
	be set to 0.
	Note: A linked list which is started using 'autostart' will not STOP when the
		  list length drops below the autostart threshold again (as list elements
		  are processed by the DMA hardware). The threshold will, however, be
		  consulted if an active list transfer is paused (using 'GDMA_Disable')
		  and then re-enabled (using 'GDMA_Enable') - in this scenario, the
		  threshold is not used to auto restart after the pause, but will cause
		  'GDMA_Enable' to return an error if the restart is being applied when
		  the list length is below the autostart threshold.

 \param
	pasIOCBs
	A pointer to an array of 'QIO_IOCB_T' structures. One IOCB structure is
	required to describe each operation to QIO, so the array should be sized based
	on the maximum number of simulateously queued operations required. If
	'bLinkedListMode' is set to 'IMG_FALSE' then this parameter MUST be set to
	'IMG_NULL'.

 \param
	ui32NoOfIOCBs
	The number of 'QIO_IOCB_T' structures in the array pointed to by the
	'pasIOCBs' parameter. If 'bLinkedListMode' is set to 'IMG_FALSE' then this
	parameter MUST be set to 0.

 \param
	bBypassQIO
	A flag indicating whether the QIO framework should be used to queue operations.
	If set to 'IMG_TRUE' then QIO is used to queue operations, and the
	'GDMA_WaitObject' function must be used to retrieve completed operations. This
	mode should not be used if the calling API itself uses a QIO based driver, due
	to restrictions inherent in the QIO system on calls made from QIO invoked
	functions into other QIO functions.
	If the calling API *is* does itself use a QIO based driver, then this flag should
	be set to 'IMG_FALSE'. When operating in this mode, the QIO framework is used
	only to register the GDMA's interrupts and invoke the GDMA API's ISR. In this
	mode it is still possible to add numerous elements to a linked list, including
	a list that is already running, but it is NOT possible to queue numerous
	individual lists. It is also not possible to queue single shot transfers - a
	single shot transfer must complete before another can be submitted. There is
	also no 'operation complete' mailbox, so the function 'GDMA_WaitObject' should not
	be called (and will always return an error if it is). Instead, the caller must
	use the 'completion' callback function registered with the GDMA API to keep track
	of when an operation has completed.

 \return
  IMG_RESULT
  Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  did not complete successfully.

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
);


/*!
*******************************************************************************

 @Function              @GDMA_Deinitialise

 Switches the driver from the 'GDMA_INITIALISED' state back into the
 'GDMA_DEINITIALISED' state.

  \param
  psContext
  A pointer to the 'GDMA_sContext' structure which was specified in the call
  to 'GDMA_Initialise' for the required DMA hardware channel.

  \return
  IMG_RESULT
  Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  did not complete successfully.

*******************************************************************************/
IMG_RESULT GDMA_Deinitialise
(
    GDMA_sContext *				psContext
);


/*!
*******************************************************************************

 @Function              @GDMA_Reset

 This function resets the DMAC hardware. The function can only be called from
 the 'GDMA_INITIALISED' state, so there can not be any queued or active
 operations when it is called.

  \param
  psContext
  A pointer to the 'GDMA_sContext' structure which was specified in the call
  to 'GDMA_Initialise' for the required DMA hardware channel.

  \return
  IMG_RESULT
  Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  did not complete successfully.

*******************************************************************************/
IMG_RESULT GDMA_Reset
(
    GDMA_sContext *				psContext
);


/*!
*******************************************************************************

 @Function              @GDMA_ListAdd

 This function adds an element to the end of the linked-list for the given
 channel. If there is no list currently active then this function will begin
 a new list. The function will return an error if this function is called when
 the hardware is processing either of the last two elements of a linked list -
 adding to a list on which the hardware has progressed to the penultimate or
 final elements while 'bypass QIO' mode is active is not permitted.
 This function may only be called when the API is being operated in 'linked
 list' mode. Note: This function allows the creation of a list if there is
 not currently one, or the addition of elements to an existing list. This API
 does not provide a means of queuing one distinct linked list behind another
 (although there should be no requirement to do so - the user should be able
 to just add and submit elements to the API, either manually restarting the
 DMA when the list 'runs dry' or allowing the 'autostart' mechanism to start
 the list whenever there are sufficient elements.

 This function can only be called from the GDMA_IDLE, GDMA_DISABLED or
 GDMA_RUNNING states. If 'autostart' is enabled, the API is in the GDMA_IDLE
 state, and adding this element causes the 'start list threshold' to be
 reached, then calling this function will cause the hardware to start
 operating and the API to transition into the 'GDMA_RUNNING' state.

  \param
  psContext
  A pointer to the 'GDMA_sContext' structure which was specified in the call
  to 'GDMA_Initialise' for the required DMA hardware channel.

  \param
  psTransferObject
  A pointer to a 'GDMA_sTransferObject' structure which describes the operation
  to be added to the list. The structure to which this pointer refers must be
  persistent, until such time until the GDMA API indicates that it is no longer
  required (via the completion callback and / or the GDMA_WaitObject function).
  It is recommended that the caller use a MeOS pool to manage transfer objects.

  \param
  bEndOfList
  A flag which indicates whether the list should be terminated after the
  specified transfer is completed. When constructing a linked list of finite
  length, each new element should be marked as the end of the list (by setting
  this flag to 'IMG_TRUE' - marking the end of the list does not preclude
  subsequent addition of new elements to the end of the list. If this flag is
  set to 'IMG_FALSE' then the list is wrapped - the 'next element' pointer
  from the element being added is set to the first element of the list. Again,
  marking a new list element in this way does not preclude the subsequent
  addition of elements to the list.

  \return
  IMG_RESULT
  Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  did not complete successfully.

  The error code 'IMG_ERROR_STORAGE_TYPE_FULL' is returned if there are no
  more linked list descriptor elements available for this context.

*******************************************************************************/
IMG_RESULT GDMA_ListAdd
(
    GDMA_sContext *					psContext,
	GDMA_sTransferObject *			psTransferObject,
    IMG_BOOL						bEndOfList
);


/*!
*******************************************************************************

 @Function              @GDMA_Configure

 This function configures the DMAC channel following initialisation, enabling
 the submission of transfer objects to the API.

 This function may only be called from the GDMA_INITIALISED state and will
 leave the API in the GDMA_IDLE state.

  \param
  psContext
  A pointer to the 'GDMA_sContext' structure which was specified in the call
  to 'GDMA_Initialise' for the required DMA hardware channel.

  \return
  IMG_RESULT
  Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  did not complete successfully.

*******************************************************************************/
IMG_RESULT GDMA_Configure
(
    GDMA_sContext *					psContext
);


/*!
*******************************************************************************

 @Function              @GDMA_Unconfigure

 This function unconfigures the DMAC channel, stopping the hardware and
 preventing the submission of further transfer objects to the API.

 All queued or active transfer objects are cancelled and must be collected
 using calls to GDMA_WaitObject (if 'bypassQIO' mode is not being used), or
 via the completion callback function if 'bypassQIO' mode is being used.

 This function calls the user registered 'cancel' callback, if one was provided.

 This function may only be called from the GDMA_DISABLED state and leaves the
 API in the 'GDMA_INITIALISED' state.

  \param
  psContext
  A pointer to the 'GDMA_sContext' structure which was specified in the call
  to 'GDMA_Initialise' for the required DMA hardware channel.

  \return
  IMG_RESULT
  Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  did not complete successfully.

*******************************************************************************/
IMG_RESULT GDMA_Unconfigure
(
    GDMA_sContext *					psContext
);


/*!
*******************************************************************************

 @Function              @GDMA_Disable

 This function suspends operation of the DMA hardware. The function may be
 called from the 'GDMA_RUNNING' state in order to 'pause' the current operation
 (note: not all DMAC hardware permits 'pausing' of active transfers - an error
 will be returned by the function in this case) or from the 'GDMA_IDLE' state
 in order to prevent the starting of new transfers. This function must be
 called prior to a call to 'GDMA_Unconfigure' in order to ensure that the
 hardware is not started (either manually or automatically) during a shutdown.
 Following a successful call to this function the API will be left in the
 'GDMA_DISABLED' state.

  \param
  psContext
  A pointer to the 'GDMA_sContext' structure which was specified in the call
  to 'GDMA_Initialise' for the required DMA hardware channel.

  \return
  IMG_RESULT
  Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  did not complete successfully.

*******************************************************************************/
IMG_RESULT GDMA_Disable
(
    GDMA_sContext *					psContext
);


/*!
*******************************************************************************

 @Function              @GDMA_Enable

 This function enables the DMA hardware for the channel following a call to
 GDMA_Disable(). It may only be called from the GDMA_DISABLED state.\n

 If the hardware was operating on a transfer when 'GDMA_Disable' was called,
 then the transfer will be resumed following a call to 'GDMA_Enable'. Note:
 In the case of a linked list transfer, resumption of a 'paused' transfer will
 only occur if the 'start list threshold' is met when 'GDMA_Enable' is called:
 if the list was paused when the DMAC hardware had less than this threshold
 number of elements left to process, then the call the 'GDMA_Enable' will
 return an error to indicate that it is not possible to re-enable.

 If the hardware was in the 'GDMA_IDLE' state when 'GDMA_Disable' was called,
 then a subsequent call to 'GDMA_Enable' will return the API to the 'GDMA_IDLE'
 state.

  \param
  psContext
  A pointer to the 'GDMA_sContext' structure which was specified in the call
  to 'GDMA_Initialise' for the required DMA hardware channel.

  \return
  IMG_RESULT
  Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  did not complete successfully.

  If a linked list was disabled when the hardware had left than the 'minimum
  start threshold' number of elements left to process, then the function
  will return 'IMG_ERROR_MINIMUM_LIMIT_NOT_MET'.

*******************************************************************************/
IMG_RESULT GDMA_Enable
(
    GDMA_sContext *					psContext
);


/*!
*******************************************************************************

 @Function              @GDMA_SingleShotOperation

 This function submits a new 'single shot' transfer control block to the API
 for processing by the DMAC hardware. This function may only be called when
 linked list mode is not enabled.

 This function can only be called from the GDMA_IDLE, GDMA_DISABLED and
 GDMA_RUNNING states. If 'autostart' is enabled then calling this function
 from the 'GDMA_IDLE' state will cause the hardware to start operating and
 the API to transition into the 'GDMA_RUNNING' state.

  \param
  psContext
  A pointer to the 'GDMA_sContext' structure which was specified in the call
  to 'GDMA_Initialise' for the required DMA hardware channel.

  \param
  psObject
  A pointer to a 'GDMA_sTransferObject', detailing the operation to be
  submitted.

  \param
  i32Timeout
  A maximum time for which the call should block, waiting for an IO control
  block to become available. This parameter is only meaningful when QIO is
  not being bypassed - when 'bypassQIO' mode is being used, this parameter
  must be set to zero. This parameter should be set to 'KRN_INFWAIT' if the
  function should block forever, or to zero if it should return immediately
  if there is not a control block already available.

  \return
  IMG_RESULT
  Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  did not complete successfully.

  If 'bypassQIO' mode is not being used, and the timeout specified in
  'i32Timeout' is exceeded before an IO control block becomes available, then
  the function will return 'IMG_TIMEOUT'.

*******************************************************************************/
IMG_RESULT GDMA_SingleShotOperation
(
    GDMA_sContext *					psContext,
    GDMA_sTransferObject *			psObject,
    IMG_INT32						i32Timeout
);


/*!
*******************************************************************************

 @Function              @GDMA_StartTransfer

 This function manually starts a previously submitted transfer. This function
 may only be called when 'autostart' is not being used. In linked list mode,
 this function will only succeed if the 'start list threshold' has been
 reached.

  \param
  psContext
  A pointer to the 'GDMA_sContext' structure which was specified in the call
  to 'GDMA_Initialise' for the required DMA hardware channel.

  \return
  IMG_RESULT
  Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  did not complete successfully.

  If linked list mode is enabled, and there are insufficient list elements in
  the list to meet the 'start list threshold', then the function returns
  'IMG_ERROR_MINIMUM_LIMIT_NOT_MET'.

*******************************************************************************/
IMG_RESULT GDMA_StartTransfer
(
	GDMA_sContext *					psContext
);


/*!
*******************************************************************************

 @Function              @GDMA_WaitObject

 This function retrieves a completed IO control block from the 'operation
 completed' mailbox registered with QIO, and returns a pointer to the
 associated 'GDMA_sTransferObject' structure to the user. When not using
 'bypassQIO' mode, it is vital that this function be called as a means of
 reclaiming submitted operation control blocks, as is it ensures that
 the GDMA API is kept supplied with IO control blocks. It is recommended that
 this function be used to obtain the addresses of the transfer objects
 submitted via either 'GDMA_ListAdd' or 'GDMA_SingleShotOperation' and to
 return them to a user managed pool.

 Note: 	This function may not be used when 'bypassQIO' mode is enabled. When
 		NOT using 'bypassQIO' mode, ALL submitted operation must be reclaimed
 		through this function, even when 'GDMA_Flush' or 'GDMA_Unconfigure'
 		have been called. No assumptions about the manner in which objects
 		returned by this function have completed should be made - if the user
 		is concerned with trapping cancelled and timed out operations, then
 		a 'completion' callback should be registered.

 This function may be called from any API state other than 'GDMA_DEINITIALISED'.

  \param
  psContext
  A pointer to the 'GDMA_sContext' structure which was specified in the call
  to 'GDMA_Initialise' for the required DMA hardware channel.

  \param
  ppsObject
  A pointer to a pointer to a 'GDMA_sTransferObject' structure. If successful,
  the function will fill the supplied pointer with the address of the
  'GDMA_sTransferObject' structure which described the operation which has
  now completed.

  \i32Timeout
  The maximum period of time (in ticks) for which the function should block
  waiting for an IO control block to be freed. This parameter should be set
  to 'KRN_INFWAIT' if the function should block forever, or to zero if it
  should return immediately if there is not a control block already waiting.

  \return
  IMG_RESULT
  Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  did not complete successfully.

  If the timeout period specified in the 'i32Timeout' parameter was exceeded
  before an IO control block could be retrieved, then the function will return
  'IMG_TIMEOUT'.

*******************************************************************************/
IMG_RESULT GDMA_WaitObject
(
    GDMA_sContext *					psContext,
    GDMA_sTransferObject **			ppsObject,
    IMG_INT32						i32Timeout
);


/*!
*******************************************************************************

 @Function              @GDMA_Flush

 This function will flush all queued transfers from the device driver.
 If not using 'bypassQIO' mode, each transfer be placed in the completion
 mailbox from which they should be retrieved using calls to GDMA_WaitObject().
 This function calls the device specific cancel function if one is registered.
 Regardless of whether 'bypassQIO' mode is being used, each cancelled transfer
 object will be signalled to the user via the 'completion' callback, if one has
 been provided.

 This function may only be called from the GDMA_DISABLED state. This function
 returns the API state to GDMA_IDLE (meaning that when 'GDMA_Flush' has been
 called, a call to 'GDMA_Disable' may NOT be followed by a subsequent call to
 'GDMA_Enable').

  \param
  psContext
  A pointer to the 'GDMA_sContext' structure which was specified in the call
  to 'GDMA_Initialise' for the required DMA hardware channel.

  \return
  IMG_RESULT
  Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  did not complete successfully.

*******************************************************************************/
IMG_RESULT GDMA_Flush
(
    GDMA_sContext *					psContext
);



/*!
*******************************************************************************

 @Function              @GDMA_ReadyToStart

 This function is used to establish whether the 'minimum start threshold' has
 been reached. The function may be called for both linked list and single
 shot mode - in single shot mode, the 'minimum start threshold' is taken to
 be one transfer.

 This function may only be called from the GDMA_IDLE or GDMA_DISABLED states.
 The function may not be called when the 'autostart' mechanism is enabled, as
 responsibility for starting operation when sufficient transfers are provided
 is passed to the API in this mode.

  \param
  psContext
  A pointer to the 'GDMA_sContext' structure which was specified in the call
  to 'GDMA_Initialise' for the required DMA hardware channel.

  \param
  pbReadyToStart
  A pointer to a flag which the function will set to indicate whether the
  'minimum start threshold' has been reached. If

  \return
  IMG_RESULT
  Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  did not complete successfully.

*******************************************************************************/
IMG_RESULT GDMA_ReadyToStart
(
    GDMA_sContext *					psContext,
    IMG_BOOL *						pbReadyToStart
);


/*!
*******************************************************************************

 @Function              @GDMA_GetListState

 This function is used to establish whether the linked list is currently empty
 or full. The function may only be called when operating in linked list mode.

 This function may be called from any API state other than 'GDMA_DEINITIALISED'.

  \param
  psContext
  A pointer to the 'GDMA_sContext' structure which was specified in the call
  to 'GDMA_Initialise' for the required DMA hardware channel.

  \return
  IMG_RESULT
  Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  did not complete successfully.

  If there are currently no transfers in the linked list, the function will
  return 'IMG_ERROR_STORAGE_TYPE_EMPTY'. If all available linked list
  descriptor elements are full, the function will return
  'IMG_ERROR_STORAGE_TYPE_FULL'.

*******************************************************************************/
IMG_RESULT GDMA_GetListState
(
    GDMA_sContext *					psContext
);


/*!
*******************************************************************************

 @Function              @GDMA_GetStatus

 This function is used to query the current API state.

 This function may be called from any state.

  \param
  psContext
  A pointer to the 'GDMA_sContext' structure which was specified in the call
  to 'GDMA_Initialise' for the required DMA hardware channel.

  \param
  peState
  A pointer to a 'GDMA_eState' enumeration, which the function will set to
  the current internal API state.

  \return
  IMG_RESULT
  Returns IMG_SUCCESS or an error code (see 'img_error.h') if the function
  did not complete successfully.

*******************************************************************************/
IMG_RESULT GDMA_GetStatus
(
    GDMA_sContext *					psContext,
    GDMA_eState *					peState
);

extern ioblock_sBlockDescriptor	IMG_asGDMABlock[];


#endif /* __GDMA_API_H__ */
