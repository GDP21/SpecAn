/*!
*******************************************************************************
 @file   scbm_api.h

 @brief  Serial Control Bus Master API

         This file contains the header file information for the Serial Control
         Bus Master (SCBM) API.

 @author Imagination Technologies

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

#if !defined (__SCBM_API_H__)
#define __SCBM_API_H__

/******************************************************************************
**************************** Included header files ****************************
*******************************************************************************/

#include <MeOS.h>

#include "img_defs.h"
#include "ioblock_defs.h"


/******************************************************************************
******************************** Enumerations *********************************
*******************************************************************************/

/*!
*******************************************************************************

 These define the status codes that are returned by SCBM API functions.

*******************************************************************************/
enum SCBM_STATUS_CODES
{
    /*! Operation completed successfully. */
    SCBM_STATUS_SUCCESS = 0,

    /*! Invalid port number. */
    SCBM_STATUS_INVALID_PORT,

    /*! Operation was cancelled. */
    SCBM_STATUS_CANCEL,

    /*! Operation timed out. */
    SCBM_STATUS_TIMEOUT,

    /*! Asynchronous I/O has been requested, but the routine would need to block. */
    SCBM_STATUS_WOULD_BLOCK,
};


typedef enum scbm_err_status
{
	SCBM_ERR_NONE,									/* No error					*/
	SCBM_ERR_ADDRESS_ERROR,							/* Invalid address			*/
	SCBM_ERR_TRANSFER_ERROR,						/* Error during transfer	*/
	SCBM_ERR_BUS_ERROR,								/* Error on the bus			*/
	SCBM_ERR_BUS_INACTIVE							/* Bus inactive				*/

}	SCBM_eErrorStatus;


/******************************************************************************
****************************** End enumerations *******************************
*******************************************************************************/


/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/

/*! Infinite wait (do not time out). */
#define SCBM_INF_TIMEOUT			(-1)

/*! Upper slave address limit */
#define SCBM_ADDR_UPPER_LIMIT		(0x3FF)


/******************************************************************************
************************** End constant definitions ***************************
*******************************************************************************/


/******************************************************************************
****************************** Type definitions *******************************
*******************************************************************************/

/*!
*******************************************************************************

 @brief This structure defines the SCBM port settings.

*******************************************************************************/
typedef struct scbm_settings_t
{
    /*! Data transfer bit rate (in kHz). */
    unsigned long bitrate;

	/*! Core clock (in kHz). */
	unsigned long coreclock;

	/*! Bus delay (in 10th of ns). */
	unsigned long busdelay;

	/*! Block index */
	unsigned long ui32BlockIndex;

} SCBM_SETTINGS_T;

/*!
*******************************************************************************

 @brief This type defines the Callback Routine used by the SCBM device driver in the
 asynchronous transfer mode.

 The callback function provides a way for the device driver/API to notify the
 calling application of I/O completion. The driver does not require the callback
 function to provide any specific function, but it must not use any MeOS
 functions that would require a scheduling operation.

 \param     *context            Pointer to private context for the callback.
 \param     read                States whether this is read or write.
 \param     address             Address of slave that data was transferred to/from.
 \param     *buffer             Pointer to buffer where data was transferred to/from.
 \param     num_bytes_transferred Number of bytes transferred in the transaction.
 \param     status              Status code describing the completion status of
                                the transaction (one of SCBM_STATUS_CODES).

*******************************************************************************/
typedef void SCBM_CALLBACKROUTINE_T(void          *context,
                                    signed long    read,
                                    unsigned long address,
                                    unsigned char *buffer,
                                    unsigned long  num_bytes_transferred,
                                    unsigned long  status);


/*!
*******************************************************************************

 @brief This structure defines the SCBM Callback type.

*******************************************************************************/
typedef struct scbm_callback_t
{
    /*! Callback routine. */
    SCBM_CALLBACKROUTINE_T *routine;

    /*! Pointer to private context for callback routine. */
    void                   *context;

} SCBM_CALLBACK_T;

/*!
*******************************************************************************

 @brief This structure defines the SCBM I/O Block type required for asynchronous
 operation.

*******************************************************************************/
typedef struct scbm_io_block_t
{
    /*! MeOS pool linkage structure, allowing this structure to be pooled */
    KRN_POOLLINK;

    /*! MeOS QIO Control Block */
    QIO_IOCB_T      iocb;

	/*! Callback parameters */
    SCBM_CALLBACK_T callback;

} SCBM_IO_BLOCK_T;

/*!
*******************************************************************************

 @brief This structure defines the asynchronous I/O descriptor type.

*******************************************************************************/
typedef struct scbm_async_t
{
	/*! Set to TRUE if the application does not want to retrieve
		the result of the operation, FALSE otherwise.*/
    signed long            forget;

    /*! Pointer to SCBM callback routine. */
    SCBM_CALLBACKROUTINE_T *callback_routine;

    /*! Pointer to private context for callback routine. */
    void                   *callback_context;

} SCBM_ASYNC_T;


/*! Async descriptor helper macro used to get result of async operation. */
#define SCBM_ASYNC_GET_RESULT(async) \
    { (async)->forget = 0; }

/*! Async descriptor helper macro used to ignore result of async operation. */
#define SCBM_ASYNC_FORGET(async) \
    { (async)->forget = 1; \
      (async)->callback_routine = NULL; }

/*! Async descriptor helper macro used to register a callback routine to
   collect the result of an async operation. */
#define SCBM_ASYNC_CALLBACK(async, routine, context) \
    { (async)->forget = 1; \
      (async)->callback_routine = routine; \
      (async)->callback_context = context; }

/*!
*******************************************************************************

 @brief This structure describes a SCB master logical object.

 It contains all context required by a single SCB master port.

*******************************************************************************/
typedef struct scbm_port_t
{
	/*! Flag indicating whether the API has been initialised */
	IMG_BOOL		bInitialised;

    /*! I/O block's KRN pool */
    KRN_POOL_T		io_block_pool;

   	/*! QIO device. */
    QIO_DEVICE_T	device;

    /*! KRN mailbox */
	KRN_MAILBOX_T	mailbox;

#define INTERNAL_SCBM_MEM_SIZE		(70)
	/* Memory for internal driver structures */
	img_uint8		aui8InternalMem[ INTERNAL_SCBM_MEM_SIZE ];

} SCBM_PORT_T;

/******************************************************************************
**************************** End type definitions *****************************
*******************************************************************************/


/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

/*
*******************************************************************************

 @Function				@SCBMDefine

 <b>Description:</b>\n
 This function defines an instance of an SCBM block.

*******************************************************************************/
img_void SCBMDefine(	ioblock_sBlockDescriptor	*	psBlockDescriptor );

/*!
*******************************************************************************

 @Function              @SCBMInit

 <b>Description:</b>\n
 This function is used to initialise the SCB Master. It should be called before
 calling SCBMRead() or SCBMWrite(). It is not necessary to  re-initialise the
 driver before subsequent calls to these functions unless any of the parameters
 set in this function are to be altered.

 The only parameter set up at the initialisation stage is the bitrate, which
 is stored in the ::SCBM_SETTINGS_T structure, pointed to by the port_settings
 input argument. The number of the SCB port to be used is also required.

 If asynchronous operation is not required, io_blocks may be set as NULL and
 num_io_blocks set to zero.

 \param     *port               Pointer to port descriptor.
 \param     *port_settings      Pointer to port settings.
 \param     *io_blocks          I/O blocks (required for asynchronous I/O).
 \param      num_io_blocks      Number of I/O blocks.

 \return                        This function returns ::SCBM_STATUS_INVALID_PORT
                                if an invalid port number is specified and
                                ::SCBM_STATUS_SUCCESS otherwise.

*******************************************************************************/
unsigned long SCBMInit
(
    SCBM_PORT_T     *port,
    SCBM_SETTINGS_T *port_settings,
    SCBM_IO_BLOCK_T *io_blocks,
    unsigned long    num_io_blocks
);


/*!
*******************************************************************************

 @Function              @SCBMDeinit

  <b>Description:</b>\n
  This function is used to deinitialise the API, and should be used as part of
  a safe system shutdown.

  \param     *port               Pointer to port descriptor.

  \return                        This function returns ::SCBM_STATUS_INVALID_PORT
                                 if an invalid port number is specified and
                                 ::SCBM_STATUS_SUCCESS otherwise.

*******************************************************************************/
unsigned long SCBMDeinit
(
    SCBM_PORT_T     *port
);


/*!
*******************************************************************************

 @Function              @SCBMRead

 <b>Description:</b>\n
 This function is used to perform a 'read from slave' transaction. When it is
 called, the SCBM transmits a start condition, slave address (set using the
 'address' input variable) and read R/W select bit to a slave device. The slave
 should respond by transmitting a series of data bytes until the SCBM ends the
 transaction by not acknowledging the last byte and then transmitting a stop
 condition.

 The SCB master stores the received data in incrementing memory locations,
 starting at the location specified in the 'buffer' input variable. (It is
 likely that the calling application will use an array to store the incoming
 data, so 'buffer' would be a pointer to the first term in this array.)
 The number of bytes that the SCBM receives before it ends the transaction is
 determined by the 'num_bytes_to_read' variable.

 If a non-blocking transfer is desired, an asynchronous descriptor must be
 provided giving the driver access to QIO resources and a user defined
 callback routine.

 \param     *port               Pointer to port descriptor.
 \param      address            Address of slave to be used (7 bit, in bits[7:1]).
 \param     *buffer             Pointer to received data buffer.
 \param      num_bytes_to_read  Number of bytes to read from slave.
 \param     *num_bytes_read     Updated each time a byte is successfully
                                received, giving a count of the number of
                                bytes read from the slave.
 \param     *async              Pointer to asynchronous I/O descriptor if a
                                non-blocking transfer is required (else NULL).
 \param     timeout             Number of MeOS timer ticks before operation
                                times out. Set to SCBM_INF_TIMEOUT for an
                                infinite period of time.

 \return                        This function returns as follows:
                                SCBM_STATUS_SUCCESS      Operation completed successfully.
                                SCBM_STATUS_CANCEL       Operation was cancelled.
                                SCBM_STATUS_TIMEOUT      Operation timed out.
                                SCBM_STATUS_WOULD_BLOCK  Asynchronous I/O has been requested,
                                                         but the routine would need to block.

                                Note:
                                This routine will always return successfully
                                if asynchronous I/O is requested.

*******************************************************************************/
unsigned long SCBMRead
(
    SCBM_PORT_T   *port,
    unsigned long address,
    unsigned char *buffer,
    unsigned long  num_bytes_to_read,
    unsigned long *num_bytes_read,
    SCBM_ASYNC_T  *async,
    long           timeout
);

/*!
*******************************************************************************

 @Function              @SCBMWrite

 <b>Description:</b>\n
 This function is used to perform a 'write to slave' transaction. When it is
 called, the SCB master transmits a start condition, slave address (set using
 the 'address' input variable) and write R/W select bit to a slave device to
 begin the transaction. It then transmits 'num_bytes_to_write' data bytes from
 incrementing memory locations, starting at the address specified in the
 'buffer' input variable. It ends the transaction by transmitting a stop condition.

 If a non-blocking transfer is desired, an asynchronous descriptor must be
 provided giving the driver access to QIO resources and a user defined
 callback routine.

 \param     *port               Pointer to port descriptor.
 \param      address            Address of slave to be used (10/7 bit addressing. For 7 bit addressing, bits [9..7] must be 0).
 \param     *buffer             Pointer to buffer containing data to be transmitted.
 \param      num_bytes_to_write Number of bytes to transmit to slave.
 \param     *num_bytes_written  Updated each time a byte is successfully
                                transmitted, giving a count of the number of
                                bytes written tp the slave.
 \param     *async              Pointer to asynchronous I/O descriptor if a
                                non-blocking transfer is required (else NULL).
 \param     timeout             Number of MeOS timer ticks before operation
                                times out. Set to SCBM_INF_TIMEOUT for an
                                infinite period of time.

 \return                        This function returns as follows:
                                SCBM_STATUS_SUCCESS      Operation completed successfully.
                                SCBM_STATUS_CANCEL       Operation was cancelled.
                                SCBM_STATUS_TIMEOUT      Operation timed out.
                                SCBM_STATUS_WOULD_BLOCK  Asynchronous I/O has been requested,
                                                         but the routine would need to block.

                                Note:
                                This routine will always return successfully
                                if asynchronous I/O is requested.

*******************************************************************************/
unsigned long SCBMWrite
(
    SCBM_PORT_T   *port,
    unsigned long  address,
    unsigned char *buffer,
    unsigned long  num_bytes_to_write,
    unsigned long *num_bytes_written,
    SCBM_ASYNC_T  *async,
    long           timeout
);

/*!
*******************************************************************************

 @Function              @SCBMGetResult

 <b>Description:</b>\n
 This function gets the result of an asynchronous operation. It can be called
 after an SCBM transaction has been carried out. The function is passed the
 SCBM_PORT_T descriptor for the port it is retrieving the result
 from. It determines the slave address, transaction type (read/write) and number
 of bytes transferred and updates the contents of the pointers passed to it.
 It also returns a status code describing the way in which the transaction
 completed.

 \param     *port               Pointer to port descriptor.
 \param     *read               Updated to show whether the transaction was a
                                read (non-zero) or write (zero).
 \param     *address            Updated with address of slave used in transaction
                                (7 bit, in bits[7:1]).
 \param     *num_bytes_transferred Updated with the number of bytes transferred.
 \param      block              Blocking flag. 1 to block, 0 not to block.
 \param      timeout            Timeout to use when blocking.
                                Number of MeOS timer ticks before operation
                                times out. Set to SCBM_INF_TIMEOUT for an
                                infinite period of time.

 \return                        This function returns as follows:
                                SCBM_STATUS_SUCCESS      Operation completed successfully.
                                SCBM_STATUS_CANCEL       Operation was cancelled.
                                SCBM_STATUS_TIMEOUT      Operation timed out.


******************************************************************************/
unsigned long SCBMGetResult
(
    SCBM_PORT_T   *port,
    signed long   *read,
    unsigned long *address,
    unsigned long *num_bytes_transferred,
    signed long    block,
    long           timeout
);

/*!
*******************************************************************************

 @Function              @SCBMCancel

 <b>Description:</b>\n
 This function cancels the current, and all queued asynchronous operations. The
 function is passed the ::SCBM_PORT_T descriptor for the transaction it is cancelling.

 \param     *port               Pointer to port descriptor.

 \return                        none.

*******************************************************************************/
void SCBMCancel
(
    SCBM_PORT_T    *port
);



unsigned long SCBMGetErrorStatus
(
	const SCBM_PORT_T	*port
);

extern ioblock_sBlockDescriptor	IMG_asSCBBlock[];


/******************************************************************************
******************************** End Functions ********************************
*******************************************************************************/

#endif /* __SCBM_API_H__ */

