/*!
*******************************************************************************
 @file   sscs_api.h

 @brief  Serial Control Bus Slave API

         This file contains the header file information for the Serial
         Control Bus Slave (SCBIS) API.

 @author Imagination Technologies

         <b>Copyright 2010 by Imagination Technologies Limited.</b>\n
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

#if !defined (__SCBS_API_H__)
#define __SCBS_API_H__

/******************************************************************************
**************************** Included header files ****************************
*******************************************************************************/

#include <MeOS.h>

/*
******************************************************************************
*   Controls values for RPC code generation
*****************************************************************************/
#ifdef __RPCCODEGEN__
#define rpc_prefix      SCBS
#define rpc_filename    scbs_api
#endif

/******************************************************************************
******************************** Enumerations *********************************
*******************************************************************************/

/*!
*******************************************************************************

 These define the status codes that are returned by SCBS API functions.

*******************************************************************************/
typedef enum
{
    /*! Operation completed successfully. */
    SCBS_STATUS_SUCCESS = 0,

    /*! Invalid port number. */
    SCBS_STATUS_INVALID_PORT,
    
    /*! Invalid slave address base/mask combination. */
    SCBS_INVALID_SLAVE_ADDRESS,

    /*! Operation was cancelled. */
    SCBS_STATUS_CANCEL,

    /*! Operation timed out. */
    SCBS_STATUS_TIMEOUT,

    /*! Asynchronous I/O has been requested, but the routine would need to block. */
    SCBS_STATUS_WOULD_BLOCK,

} SCBS_RETURN_T;

typedef enum scbs_err_status
{
	SCBS_ERR_NONE,									/* No error					*/
	SCBS_ERR_ADDRESS_ERROR,							/* Invalid address			*/
	SCBS_ERR_TRANSFER_ERROR,						/* Error during transfer	*/
	SCBS_ERR_BUS_ERROR,								/* Error on the bus			*/
	SCBS_ERR_BUS_INACTIVE							/* Bus inactive				*/

}	SCBS_eErrorStatus;


/******************************************************************************
****************************** End enumerations *******************************
*******************************************************************************/

/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/

/*! Infinite wait (do not time out). */
#define SCBS_INF_TIMEOUT -1

/*! Upper slave address limit */
#define SCBS_ADDR_UPPER_LIMIT		(0x3FF)



/******************************************************************************
************************** End constant definitions ***************************
*******************************************************************************/

/******************************************************************************
****************************** Type definitions *******************************
*******************************************************************************/

/*!
*******************************************************************************

 @brief This structure defines the SCBS port object.

*******************************************************************************/
typedef struct scbs_port_t
{
	/*! Is API initialised? */
	IMG_BOOL				bInitialised;
	
    /*! QIO device. */
    QIO_DEVICE_T			sDevice;

    /*! I/O block's KRN pool */
    KRN_POOL_T				io_block_pool;

    /*! KRN mailbox */
    KRN_MAILBOX_T			sMailbox;

	/* Pointer to current transfer */
	QIO_IOPARS_T		*	psIOPars;
	
#define INTERNAL_SCBS_MEM_SIZE		(70)
	/* Memory for internal driver structures */
	img_uint8		aui8InternalMem[ INTERNAL_SCBS_MEM_SIZE ];

} SCBS_PORT_T;


/*!
*******************************************************************************

 @brief This structure contains initialisation parameters.

*******************************************************************************/
typedef struct
{
	/* SCBS Block Index */
	unsigned long		ui32BlockIndex;

	/* SCBS address base (7 or 10 bit) */
	unsigned long		ui32SlaveAddressBase;

	/* SCBS address mask (7 or 10 bit) */
	unsigned long		ui32SlaveAddressMask;

	/* SCBS bit rates (in KHz) */
	unsigned long		ui32BitRate;

	/*! Core clock (in MHz). */
	unsigned long		ui32CoreClock;

	/*! Bus delay (in 10th of ns). */
	unsigned long		ui32BusDelayNS;

} SCBS_PARAM_T;

#define SCBS_MAX_ADDR_MASK			0x3FF		/* 10 bits maximum address */

/*!
*******************************************************************************

 @brief This type defines the Callback Routine used by the SCBS device driver in the
 asynchronous transfer mode.

 The callback function provides a way for the device driver/API to notify the
 calling application of I/O completion. The driver does not require the callback
 function to provide any specific function, but it must not use any MeOS
 functions that would require a scheduling operation.

 \param     *context            Pointer to private context for the callback.
 \param     *buffer             Pointer to buffer where data was transferred to/from.
 \param     num_bytes_to_transfer The number of bytes the slave was 'expecting'
                                  to transfer (tis is the size of the buffer
                                  that data was transferred to/from)
 \param     status              Status code describing the completion status of
                                the transaction.

*******************************************************************************/
typedef void SCBS_CALLBACKROUTINE_T(void         *context,
                                   unsigned char *buffer,
                                   unsigned long num_bytes_to_transfer,
                                   unsigned long status);

/*!
*******************************************************************************

 @brief This structure defines the SCBS Callback type.

*******************************************************************************/
typedef struct scbs_callback_t
{
    /*! Callback routine. */
    SCBS_CALLBACKROUTINE_T *routine;

    /*! Pointer to private context for callback routine. */
    void                   *context;

} SCBS_CALLBACK_T;

/*!
*******************************************************************************

 @brief This structure defines the SCBS I/O Block type required for asynchronous
 operation.

*******************************************************************************/
typedef struct scbs_io_block_t
{
    /*! MeOS pool linkage structure, allowing this structure to be pooled */
    KRN_POOLLINK;

    /*! MeOS QIO Control Block */
    QIO_IOCB_T      iocb;

    /*! Callback parameters */
    SCBS_CALLBACK_T callback;

} SCBS_IO_BLOCK_T;

/*!
*******************************************************************************

 @brief This structure defines the asynchronous I/O descriptor type.

*******************************************************************************/
typedef struct scbs_async_t
{
	/*! Set to TRUE if the application does not want to retrieve
		the result of the operation, FALSE otherwise.*/
    int                    forget;

    /*! Pointer to SCBS Callback routine. */
    SCBS_CALLBACKROUTINE_T *callback_routine;

    /*! Pointer to private context for callback routine. */
    void                   *callback_context;

} SCBS_ASYNC_T;

/*! Async descriptor helper macro used to get result of async operation. */
#define SCBS_ASYNC_GET_RESULT(async, context) \
    { (async)->forget = 0;                     \
      (async)->callback_context = context; }

/*! Async descriptor helper macro used to ignore result of async operation. */
#define SCBS_ASYNC_FORGET(async) \
    { (async)->forget = 1; \
      (async)->callback_routine = NULL; }

/*! Async descriptor helper macro used to register a callback routine to
   collect the result of an async operation. */
#define SCBS_ASYNC_CALLBACK(async, routine, context) \
    { (async)->forget = 1; \
      (async)->callback_routine = routine; \
      (async)->callback_context = context; }

/******************************************************************************
**************************** End type definitions *****************************
*******************************************************************************/


/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

/*!
*******************************************************************************

 @Function				@SCBSDefine

 @Description
 This function defines an instance of a SCB slace block.

*******************************************************************************/
img_void SCBSDefine(	ioblock_sBlockDescriptor	*	psBlockDescriptor	);

/*!
*******************************************************************************

 @Function              @SCBSInit

 <b>Description:</b>\n
 This function is used to initialise the SCB Slave. This must be done before a
 transaction is carried out using SCBSRead() or SCBSWrite(). It is not
 necessary to re-initialise the SPIS before further transactions unless any of
 the parameters set up at initialisation are to be changed.

 The function initialises the port by allocating a QIO device object to the
 SCBS port object defined by the structure SCBS_PORT_T.

											The parameters set up at initialisation are the SPI mode, CS active level
											and DMA input and output channel numbers. DMA transfers are used to transfer
											data from memory to the SPIS (for writes) and from the SPIS to memory (for
											reads). The DMA channel numbers determine which DMA channels are used.

 The input and output DMA channels must be different.

 \param     *port               Pointer to port descriptor.
 \param     *initParam          Pointer to initialisation parameters.
 \param     *io_blocks          I/O blocks (required for asynchronous I/O).
 \param      num_io_blocks      Number of I/O blocks.

 \return                        SCBS_OK if initialisation parameters are valid
                                SCBS_INVALID_<type> if <type> parameter is not valid

*******************************************************************************/
unsigned long SCBSInit
(
    SCBS_PORT_T		*port,
    SCBS_PARAM_T	*initParam,
    SCBS_IO_BLOCK_T *io_blocks,
    unsigned long    num_io_blocks
);

/*!
*******************************************************************************

 @Function              @SCBSRead

 <b>Description:</b>\n
 This function is used to perform a 'read from master' transaction. When the
 function is called, the SCBS is made ready to receive data. The transaction
 from the peripheral to the outside world is controlled by the master.

							A DMA transfer is used to transfer data from the peripheral to memory.
							The SPIS stores data in incrementing memory locations, starting at the
							address specified in the 'buf' parameter. Whilst the master controls when the
							transaction ends by controlling how much data is transmitted to the peripheral
							and when the CS line is disabled, the number of bytes read in by the SPISRead
							function is determined by the DMA transfer.

							If the master does not send as much data as 'dmaLength' specified, and
							if this mismatch is greater than 8 bytes, then the slave read operation will
							not complete and data buffer will be invalid. If the mismatch is less than 8 bytes,
							then the slave read operation will complete with a partially invalid buffer of data.
							If the master sends too much data, whilst the current slave read would complete
							correctly, subsequent reads would not be correct.

 \param     *port               Pointer to port descriptor.
 \param     *buf                Buffer for received data
 \param      dmaLength          Number of bytes to be read in.
 \param     *async              Pointer to asynchronous I/O descriptor if a
                                non-blocking transfer is required (else NULL).
 \param      timeout            Number of MeOS timer ticks before operation
                                times out. Set to ::SPIS_INF_TIMEOUT for an
                                infinite period of time. The timer starts when
                                QIO start function is called. This parameter has
                                the same implication for blocking and non-blocking
                                transfers.

 \return                        This function returns as follows:
                                ::SCBS_STATUS_SUCCESS      Operation completed successfully.
                                ::SCBS_STATUS_CANCEL       Operation was cancelled.
                                ::SCBS_STATUS_TIMEOUT      Operation timed out.

*******************************************************************************/
unsigned long SCBSRead
(
    SCBS_PORT_T   *port,
    unsigned char *buf,
    unsigned long  dmaLength,
    SCBS_ASYNC_T  *async,
    long           timeout
);


/*!
*******************************************************************************

 @Function              @SCBSWrite

 <b>Description:</b>\n
 This function is used to perform a 'write to master' transaction. When the
 function is called, the SCBS is made ready to transmit data. 
 
								The transaction
								from the peripheral to the outside world is controlled by the master,
								which sets the CS line active to start the transaction and clocks data out
								of the slave. When the SPIS receives this clock and CS is active,
								it writes data out on the MISO (master in slave out) line.

								A DMA transfer is used to transfer data from the peripheral to memory.
								Data is taken from incrementing memory locations starting at the address
								specified by the 'buf' input parameter. Whilst the master controls the
								transaction by controlling the CS and clock line, the number of bytes
								transmitted by the SPISWrite function is determined by the DMA transfer.

								The function will only complete when 'dmaLength' bytes has been transferred
								from the system memory to the peripheral. Once it has transferred this number
								of bytes, the function will complete, even if the master has not ended the transaction.
								If the SPIS attempts to write more data than the master has requested,
								then the transaction will not complete until the master has requested enough
								further data from the slave such that the entire slave transaction has been
								transferred from the system memory to the peripheral.

 \param     *port               Pointer to port descriptor.
 \param     *buf                Buffer of data for transmission
 \param      dmaLength          Number of bytes to be transmitted.
 \param     *async              Pointer to asynchronous I/O descriptor if a
                                non-blocking transfer is required (else NULL).
 \param      timeout            Number of MeOS timer ticks before operation
                                times out. Set to ::SPIS_INF_TIMEOUT for an
                                infinite period of time. The timer starts when
                                QIO start function is called. This parameter has
                                the same implication for blocking and non-blocking
                                transfers.

 \return                        This function returns as follows:
                                ::SCBS_STATUS_SUCCESS      Operation completed successfully.
                                ::SCBS_STATUS_CANCEL       Operation was cancelled.
                                ::SCBS_STATUS_TIMEOUT      Operation timed out.

*******************************************************************************/
unsigned long SCBSWrite
(
    SCBS_PORT_T   *port,
    unsigned char *buf,
    unsigned long  dmaLength,
    SCBS_ASYNC_T  *async,
    long           timeout
);

/*!
*******************************************************************************

 @Function              @SCBSGetResult

 <b>Description:</b>\n
 This function gets the result of an asynchronous operation. It can be
 called after an SCBS transaction has been carried out. The function is
 passed the ::SCBS_PORT_T descriptor for the port it is retrieving the
 result from. It returns a status code describing the way in which the
 transaction completed.

 \param     *port               Pointer to port descriptor.
 \param    **context            Updated with the pointer to the context of
                                the transaction it is retrieving the result from.
 \param      block              Blocking flag. 1 to block, 0 not to block.
 \param      timeout            Timeout to use when blocking.
                                Number of MeOS timer ticks before operation
                                times out. Set to ::SCBS_INF_TIMEOUT for an
                                infinite period of time.

 \return                        This function returns as follows:
                                ::SCBS_STATUS_SUCCESS      Operation completed successfully.
                                ::SCBS_STATUS_CANCEL       Operation was cancelled.
                                ::SCBS_STATUS_TIMEOUT      Operation timed out.

*******************************************************************************/
unsigned long SCBSGetResult
(
    SCBS_PORT_T   *port,
    void          **context,
    int           block,
    long          timeout
);

/*!
*******************************************************************************

 @Function              @SCBSCancel

 <b>Description:</b>\n
 This function cancels ALL (active and pending) operations queued on
 the device. The function is passed the ::SCBS_PORT_T descriptor for the
 transaction it is cancelling.

 \param     *port               Pointer to port descriptor.

 \return                        none.

*******************************************************************************/
unsigned long SCBSCancel
(
    SCBS_PORT_T    *port
);

/*!
*******************************************************************************

 @Function              @SCBSDeinit

 <b>Description:</b>\n
 This function deinitialises the SCB Slave API, and should be used as part of a
 safe system shutdown.

 \param     *port               Pointer to port descriptor.

 \return                        none.
 
*******************************************************************************/
img_void SCBSDeinit( SCBS_PORT_T	*	port );

extern ioblock_sBlockDescriptor	IMG_asSCBBlock[];

/******************************************************************************
******************************** End Functions ********************************
*******************************************************************************/

#endif /* __SCBS_API_H__ */
