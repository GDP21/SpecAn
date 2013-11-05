/*!
*******************************************************************************
 @file   usbd_api.h

 @brief  USB Device API

         This file contains the header file information for the USB Device API.

 @author Imagination Technologies

         <b>Copyright 2007 by Imagination Technologies Limited.</b>\n
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

#if !defined (__USBD_API_H__)
#define __USBD_API_H__

/******************************************************************************
**************************** Included header files ****************************
*******************************************************************************/

#define METAG_ALL_VALUES
#define METAC_ALL_VALUES

#include <metag/machine.inc>
#include <metag/metagtbi.h>
#include <MeOS.h>

#include <img_defs.h>
#if !defined (USBD_NO_CBMAN_SUPPORT)
#include <img_common.h>
#endif

#include <usb_spec.h>

/******************************************************************************
******************************** Enumerations *********************************
*******************************************************************************/
#define USBD_MAX_CONFIGURATIONS			2 /* Maximum number of configurations supported by API */

/*!
*******************************************************************************

 This type defines the USBD events.

*******************************************************************************/
typedef enum
{
    /*! The event indicates that connection to the host has been established and that the device is ready for reads and/or writes */
    USBD_EVENT_IO_OPENED           					= 0x00000001,
    /*! The event indicates that connection to the host has been lost and that the device read and write functionality is no longer available */
    USBD_EVENT_IO_CLOSED            				= 0x00000002,
    /*! The event indicates that a control message has been received that the default driver does not know how to handle */
    USBD_EVENT_UNHANDLED_CONTROL_MESSAGE_RECEIVED	= 0x00000003,
    /*! The event indicates that handling of a control message has been completed */
    USBD_EVENT_UNHANDLED_CONTROL_MESSAGE_COMPLETE	= 0x00000004,
	/*! A base event for class handlers */
	USBD_EVENT_CLASS_HANDLER						= 0x00000005
} USBD_EVENT_T;


/*!
*******************************************************************************

 This type defines the USBD initialisation function return value.

*******************************************************************************/
typedef enum
{
    /*! Operation completed successfully. */
    USBD_STATUS_SUCCESS = 0,

    /*! The device is not initialised. */
    USBD_ERROR_NOT_INITIALISED,

    /*! The configuration parameters are not valid. */
    USBD_ERROR_INVALID_CONFIGURATION,

    /*! Operation was cancelled. */
    USBD_STATUS_CANCEL,

    /*! Operation was timed out. */
    USBD_STATUS_TIMEOUT,

    /*! Asynchronous I/O has been requested, but the routine would need to block. */
    USBD_STATUS_WOULD_BLOCK,

    /*! The requested I/O operation is unavailable in the current configuration */
    USBD_STATUS_IO_OPERATION_UNAVAILABLE,

	/*! The channel specified is invalid */
	USBD_ERROR_INVALID_CHANNEL

} USBD_RETURN_T;

/******************************************************************************
****************************** End enumerations *******************************
*******************************************************************************/


/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/

/*! Infinite wait (do not time out). */
#define USBD_INF_TIMEOUT -1

/******************************************************************************
************************** End constant definitions ***************************
*******************************************************************************/



/******************************************************************************
****************************** Type definitions *******************************
*******************************************************************************/

/*!
*******************************************************************************

 @brief This structure contains the callback data parameters.

*******************************************************************************/
typedef struct USBD_EVENT_CALLBACK_DATA_T_
{
    /*! Event(s) that triggered the callback */
    IMG_UINT32      uiEvent;

    /*! Other data */
    IMG_VOID *      pData;

} USBD_EVENT_CALLBACK_DATA_T;


/*!
*******************************************************************************

 @brief This type defines the Callback function used by the USBD device driver.

 The callback function provides a way for the device driver to notify the
 calling application of occurence of registered events.

 \param     sCallbackData            Callback data parameters structure.

*******************************************************************************/
typedef IMG_VOID USBD_EVENT_CALLBACK_FUNCTION_T ( USBD_EVENT_CALLBACK_DATA_T sCallbackData );

/*! USB STRING Descriptor type (Table 9-5 of USB 2.0 Spec) */
#define USBD_STRING_DESCRIPTOR_IDENTIFIER   (0x03)

/*!
*******************************************************************************
 @brief This types defines the string descriptor structure
*******************************************************************************/
typedef struct USB_STRING_DESCRIPTOR_T_
{
    IMG_UINT8  bLength;
    IMG_UINT8  bDescriptorType;
    IMG_UINT16 wString[1];

} USBD_STRING_DESCRIPTOR_T;

/* Exclude from doxygen documentation */
#ifndef DOXYGEN_WILL_SEE_THIS

/*! String decriptor macro */
#define USBD_STRING_DESCRIPTOR_DEF(Length) \
    IMG_UINT8  bLength;               \
    IMG_UINT8  bDescriptorType;       \
    IMG_UINT16 bString[Length];


/*! Macro used to work out the length of a string descriptor */
#define USBD_STRING_DESCRIPTOR_LENGTH(Length) (2 + 2*(Length))


/* Macro that works out the length of an ANSI String */
#define USBD_STRING_LEN(AnsiString) (sizeof(AnsiString) - 1)

#endif  /* DOXYGEN_WILL_SEE_THIS */

/*!
*******************************************************************************
    
    @brief Structure used to specify the power attributes of the device. 
    
    These are
	used by the driver in building up the enumeration data that will be sent
	to the host.

*******************************************************************************/
typedef struct USBD_POWER_ATTR_T_
{
	/*! Set to 1 if self-powered, 0 if bus-powered */
	IMG_BOOL	bSelfPowered;

	/*! Maximum current drawn by the device from the usb bus (mA) */ 
	IMG_UINT32	ui32MaxPower;

} USBD_POWER_ATTR_T;

typedef struct USBD_CONFIGURATION_T_
{
	IMG_VOID	*	apvConfigDescriptorFullSpeed[ USBD_MAX_CONFIGURATIONS ];
	IMG_VOID	*	apvConfigDescriptorHighSpeed[ USBD_MAX_CONFIGURATIONS ];
	IMG_UINT8		ui8NumConfigurations;
} USBD_CONFIGURATION_T;

/*!
*******************************************************************************

 @brief This structure contains the initialisation parameters.

*******************************************************************************/
typedef struct USBD_INIT_PARAM_T_
{
    /*! Device configuration selection */
	USB_DEVICE_DESCRIPTOR			*	psDeviceDescriptor;
	USB_QUALIFIER_DESCRIPTOR		*	psQualifierDescriptor;
	USBD_CONFIGURATION_T			*	psConfigDescriptors;

    /*! Strings used during enumeration - if a string is used, string descriptor 0 must contain 
	    one or more Language Identifier Codes */
    USBD_STRING_DESCRIPTOR_T **			ppsStrings;

	/*! Power parameters to be used during enumeration */
	USBD_POWER_ATTR_T					sUsbdPower;

    /*! When set to IMG_TRUE, the driver performs a soft disconnect + connect at start up */
    IMG_INT32							iSoftReconnect; 

#if defined (USBD_NO_CBMAN_SUPPORT)
	 /*! Callback function pointer. Set to IMG_NULL if no callback function is to be used */
    USBD_EVENT_CALLBACK_FUNCTION_T *	pfCallbackFunc;

    /*! Bitfield of the events to be registered with the API for callback through pfCallbackFunc */
    IMG_UINT32							uiCallbackEvents;
#endif
	/*! Block index */
	img_uint32							ui32BlockIndex;

	/*! When set to IMG_TRUE, the driver forces the controller into device mode */
	IMG_INT32							iForceDeviceMode;

	/*! When set to IMG_TRUE, the driver will follow transfers that are a multiple of the endpoint's
		maximum packet size with a zero-length packet */
	IMG_INT32							iEnableZLP;

	/*! Speed to attempt to enumerate at */
	USB_DEVICE_SPEED					EnumerateSpeed;

	/*! Internal API variable */
	IMG_BOOL							bMainInit;

} USBD_INIT_PARAM_T;



typedef struct usbd_ep_tag
{
	/*! QIO device */
	QIO_DEVICE_T				sDevice;

    /*! I/O block's KRN pool */
    KRN_POOL_T					sIOBlockPool;

	/*! KRN mailbox */
    KRN_MAILBOX_T				sMailbox;

	/*! Initialisation flag */
	IMG_BOOL					bInitialised;

	/*! Cancel flag */
	IMG_BOOL					bCancel;

	/* Current IO */
	QIO_IOPARS_T			*	psCurrentIo;

	/* EP Direction */
	IMG_BOOL					bIsIn;

	/* EP Descriptor */
	IMG_VOID				*	pvEP;

	/* EP Transfer Status */
	IMG_UINT32					ui32XferStatus;

} USBD_sEP;


/*!
*******************************************************************************

 @brief This structure defines the USBD block object.

*******************************************************************************/
typedef struct usbd_block_tag
{
	QIO_DEVICE_T				sMainDevice;

#define INTERNAL_MEM_SIZE	(7600)
	/* Memory for internal driver structures */
	img_uint8					aui8InternalMem[INTERNAL_MEM_SIZE];

} USBD_sBlock;

/*!
*******************************************************************************

 @brief This type defines the Callback Routine used by the USBD device driver in the
 asynchronous transfer mode.

 The callback function provides a way for the device driver/API to notify the
 calling application of data I/O completion. The driver does not require the callback
 function to provide any specific function, but it must not use any MeOS
 functions that would require a scheduling operation.

 \param     *context            Pointer to private context for the callback.
 \param     *buffer             Pointer to buffer where data was transferred to/from.
 \param     num_bytes_to_transfer The number of bytes the slave was 'expecting'
                                  to transfer (this is the size of the buffer
                                  that data was transferred to/from)
 \param     status              Status code describing the completion status of
                                the transaction.

*******************************************************************************/
typedef IMG_VOID USBD_IO_CALLBACKROUTINE_T( IMG_VOID *  context,
                                            IMG_UINT8 * buffer,
                                            IMG_UINT32  num_bytes_to_transfer,
                                            IMG_UINT32  status);



/*!
*******************************************************************************

 @brief This structure defines the USBD Callback type.

*******************************************************************************/
typedef struct USBD_IO_CALLBACK_T_
{
    /*! Callback routine. */
    USBD_IO_CALLBACKROUTINE_T *     routine;

    /*! Pointer to private context for callback routine. */
    IMG_VOID *                      context;

} USBD_IO_CALLBACK_T;



/*!
*******************************************************************************

 @brief This structure defines the USBD I/O Block type required for asynchronous
 operation.

*******************************************************************************/
typedef struct USBD_IO_BLOCK_T_
{
    /*! MeOS pool linkage structure, allowing this structure to be pooled */
    KRN_POOLLINK;

    /*! MeOS QIO Control Block */
    QIO_IOCB_T          iocb;

    /*! Callback parameters */
    USBD_IO_CALLBACK_T  callback;

} USBD_IO_BLOCK_T;




/*!
*******************************************************************************

 @brief This structure defines the asynchronous I/O descriptor type.

*******************************************************************************/
typedef struct USBD_ASYNC_T_
{
    /*! Set to IMG_TRUE if the application is going to use the callback method
        of confirming the result.  Set to IMG_FALSE if the application
        is going to use the GetResult method.
     */
    IMG_INT32                   iUsingCallback;

    /*! Pointer to USBD Callback routine. */
    USBD_IO_CALLBACKROUTINE_T * callback_routine;

    /*! Pointer to private context for callback routine. */
    IMG_VOID *                  callback_context;

} USBD_ASYNC_T;

/******************************************************************************
**************************** End type definitions *****************************
*******************************************************************************/


/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

/*!
*******************************************************************************

 @Function				@USBDInit

 <b>Description:</b>\n
 This function is used to initialise the USB Device.
 It initialises the block by allocating a QIO device object to the USBD block object 
 defined by the structure USBD_sBlock. At the completion of this function, 
 the device is ready to be connected to the host. The application may initiate an I/O 
 transfer after the device is successfuly connected to the host. The establishment of 
 the connection shall be flagged via an optional event callback that the application
 may choose to set up through the initParam parameter to this function.

 This function must be called before any other USBD API function.

 It is not necessary to re-initialise the USBD before further transactions unless any of
 the parameters set up at initialisation are to be changed.

 \param     *psBlock            Pointer to block descriptor.
 \param     *initParam          Pointer to initialisation parameters.
 
 \return                        ::USBD_STATUS_SUCCESS       if initialisation parameters are valid

*******************************************************************************/
USBD_RETURN_T USBDInit (
    USBD_sBlock			*	psBlock,
    USBD_INIT_PARAM_T	*	initParam
);

/*!
*******************************************************************************

 @Function				@USBDInitEP

 <b>Description:</b>\n
 This function is used to initialise an endpoint (other than endpoint 0) and 
 must be called before an endpoint is used. Endpoint numbering is linked to 
 the order of endpoints in an interface.

 This function must be called after USBDInit().

 \param		*psBlock			Pointer to block structure passed to USBDInit
 \param		ui32EPNum			Endpoint number (>=1)
 \param		asIOBlocks			An array of IO blocks for this endpoint
 \param		ui32NumIOBlock		Number of IO blocks for this endpoint
 \param		ppsEP				Address of a pointer that, when returned, will point
								to an internal USBD_sEP structure. This will be used
								for all read/write transfers.
*******************************************************************************/

USBD_RETURN_T USBDInitEP(
	USBD_sBlock			*	psBlock,
	IMG_UINT32				ui32EPNum,
	USBD_IO_BLOCK_T		*	asIOBlocks,
	IMG_UINT32				ui32NumIOBlocks,
	USBD_sEP			**	ppsEP
);

/*!
*******************************************************************************

 @Function              @USBDDeinit
 
  <b>Description:</b>\n
  This function is used to deinitialise the USB device API, and should be used
  as part of a safe system shutdown.
  
  \param     *block               Pointer to block descriptor.  
 
  \return                        ::USBD_STATUS_SUCCESS       if initialisation parameters are valid 
 
*******************************************************************************/ 
USBD_RETURN_T USBDDeinit (	  
	USBD_sBlock			*	psBlock
);


/*!
*******************************************************************************

 @Function              @USBDRead

 <b>Description:</b>\n
 This function is used to perform a data 'read from host' transaction on the 
 given endpoint. When the function is called, the USBD is made ready to receive 
 data.

 A DMA transfer is used to transfer data from USBD RxFIFO to memory.
 The USBD stores data in incrementing memory locations, starting at the
 address specified in the 'buf' parameter. It must be ensured that the address
 is 32 bit aligned and that the entire buffer is accessible through the USBD
 DMA controller.

 \param     *psBlock            Pointer to block descriptor.
 \param		*psEP				Pointer to endpoint structure.
 \param     *buf                Buffer for received data. Must be 32 bit aligned.
 \param     uiTransferLen       Number of bytes to be read in.
 \param     *uiNumberOfBytesRead In synchronous operation indicates the number of 
                                bytes that have been read.
 \param     *async              Pointer to asynchronous I/O descriptor if a
                                non-blocking transfer is required (else IMG_NULL).
 \param      timeout            Number of MeOS timer ticks before operation
                                times out. Set to ::USBD_INF_TIMEOUT for an
                                infinite period of time. The timer starts when
                                QIO start function is called. This parameter has
                                the same implication for blocking and non-blocking
                                transfers.

\return                        This function returns as follows:
                                ::USBD_STATUS_SUCCESS      Operation completed successfully.
                                ::USBD_ERROR_INVALID_CONFIGURATION  If any of the parameters are invalid.
                                ::USBD_STATUS_IO_OPERATION_UNAVAILABLE If the block is not available for reading.
                                ::USBD_STATUS_CANCEL       Operation was cancelled.
                                ::USBD_STATUS_TIMEOUT      Operation timed out.
*******************************************************************************/
USBD_RETURN_T USBDRead  (
    USBD_sBlock		*   psBlock,
	USBD_sEP		*	psEP,
    IMG_UINT8		*	buf, 
    IMG_UINT32			uiTransferLen, 
    IMG_UINT32		*	uiNumberOfBytesRead, 
    USBD_ASYNC_T	*	async, 
    IMG_INT32			timeout
);


/*!
*******************************************************************************

 @Function              @USBDWrite

 <b>Description:</b>\n
 This function is used to perform a data 'write to host' transaction on the 
 given endpoint. When the function is called, the USBD is made ready to transmit 
 data. 

 A DMA transfer is used to transfer data from memory to the USBD TxFIFO.
 The USBD reads the data from incrementing memory locations, starting at the
 address specified in the 'buf' parameter. It must be ensured that the address
 is 32 bit aligned and that the entire buffer is accessible through the USBD
 DMA controller.

 \param     *psBlock            Pointer to block descriptor.
 \param		*psEP				Pointer to endpoint structure.
 \param     *buf                Buffer of data for transmission. Must be 32 bit aligned.
 \param     uiTransferLen       Number of bytes to be transmitted.
 \param     *uiNumberOfBytesWritten In synchronous operation indicates the number of 
                                bytes that have been written.
 \param     *async              Pointer to asynchronous I/O descriptor if a
                                non-blocking transfer is required (else IMG_NULL).
 \param      timeout            Number of MeOS timer ticks before operation
                                times out. Set to USBD_INF_TIMEOUT for an
                                infinite period of time. The timer starts when
                                QIO start function is called. This parameter has
                                the same implication for blocking and non-blocking
                                transfers.

 \return                        This function returns as follows:
                                ::USBD_STATUS_SUCCESS      Operation completed successfully.
                                ::USBD_ERROR_INVALID_CONFIGURATION  If any of the parameters are invalid.
                                ::USBD_STATUS_IO_OPERATION_UNAVAILABLE If the block is not available for writing.
                                ::USBD_STATUS_CANCEL       Operation was cancelled.
                                ::USBD_STATUS_TIMEOUT      Operation timed out.
******************************************************************************/
USBD_RETURN_T USBDWrite (   
    USBD_sBlock		*   psBlock,
	USBD_sEP		*	psEP,
    IMG_UINT8		*	buf, 
    IMG_UINT32			uiTransferLen,
    IMG_UINT32		*	uiNumberOfBytesWritten,
    USBD_ASYNC_T	*	async, 
    IMG_INT32			timeout
);


/*!
*******************************************************************************

 @Function              @USBDGetResult

 <b>Description:</b>\n
 This function gets the result of an asynchronous I/O operation. It can be
 called after an USBD transaction has been carried out. The function is
 passed the USBD_sBlock descriptor for the block it is retrieving the
 result from. It returns a status code describing the way in which the
 transaction completed.

 \param     *block               Pointer to block descriptor.
 \param    **context            Updated with the pointer to the context of
                                the transaction it is retrieving the result from.
 \param      *num_bytes_transferred Updated with the number of bytes transferred.
 \param      block              Blocking flag. 1 to block, 0 not to block.
 \param      timeout            Timeout to use when blocking.
                                Number of MeOS timer ticks before operation
                                times out. Set to USBD_INF_TIMEOUT for an
                                infinite period of time.

 \return                        This function returns as follows:
                                ::USBD_STATUS_SUCCESS      Operation completed successfully.
                                ::USBD_STATUS_CANCEL       Operation was cancelled.
                                ::USBD_STATUS_TIMEOUT      Operation timed out.

*******************************************************************************/
IMG_UINT32 USBDGetResult (
    USBD_sBlock	*   psBlock, 
	USBD_sEP	*	psEP,
    IMG_VOID	**  context, 
    IMG_UINT32	*   num_bytes_transferred, 
    IMG_INT32       block, 
    IMG_INT32       timeout
);



/*!
*******************************************************************************

 @Function              @USBDCancel

 <b>Description:</b>\n
 This function cancels ALL (active and pending) data transfers queued on
 the endpoint. The function is passed the ::USBD_sBlock descriptor for the
 transaction it is cancelling.

 \param     *block               Pointer to block descriptor.

 \return                        none.

*******************************************************************************/
IMG_VOID USBDCancel (
    USBD_sBlock *   psBlock,
	USBD_sEP	*	psEP
);

#if !defined (USBD_NO_CBMAN_SUPPORT)
/*!
*******************************************************************************

 @Function              @USBD_AddEventCallback

******************************************************************************/

USBD_RETURN_T USBD_AddEventCallback (
	USBD_sBlock				*	psBlock,
	USBD_EVENT_T				eEvent,
	IMG_pfnEventCallback		pfnEventCallback,
	img_void				*	pvCallbackParameter,
	IMG_hCallback			*	phEventCallback
);

/*!
*******************************************************************************

 @Function              @USBD_ExecuteEventCallback

******************************************************************************/
USBD_RETURN_T USBD_ExecuteEventCallback(
	USBD_sBlock				*	psBlock,
	USBD_EVENT_T				eEvent,
	img_uint32					ui32Param,
	img_void				*	pvParam
);

#endif

extern ioblock_sBlockDescriptor	IMG_asUSBBlock[];

/******************************************************************************
******************************** End Functions ********************************
*******************************************************************************/

#endif /* __USBD_API_H__ */
