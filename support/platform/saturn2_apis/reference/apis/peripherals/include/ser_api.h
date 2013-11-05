/*!
*******************************************************************************
 @file   ser_api.h

 @brief  Serial Port (SER/UART) API header

         This file contains the header file information for the Serial
         Peripheral (UART/SER) API.

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

#if !defined (__SER_API_H__)
#define __SER_API_H__

/******************************************************************************
**************************** Included header files ****************************
*******************************************************************************/

#include <MeOS.h>

/******************************************************************************
******************************** Enumerations *********************************
*******************************************************************************/

/*!
*******************************************************************************

 These define the status codes that are returned by SER API functions.

*******************************************************************************/
enum SER_STATUS_CODES
{
    /*! Operation completed successfully. */
    SER_STATUS_SUCCESS = 0,

    /*! Invalid port number. */
    SER_STATUS_INVALID_PORT,

    /*! Invalid word format. */
    SER_STATUS_INVALID_WORDFORMAT,

    /*! Invalid CTS/RTS setting. */
    SER_STATUS_INVALID_CTSRTS,

    /*! Invalid receiver FIFO trigger threshold. */
    SER_STATUS_INVALID_THRESHOLD,

    /*! Port configuration failed. */
    SER_STATUS_CONFIG_FAILED,

    /*! An error occurs during the operation. */
    SER_STATUS_ERROR,

    /*! An overrun error occurs during the operation. */
    SER_STATUS_OVERRUN_ERROR,

    /*! Operation was cancelled. */
    SER_STATUS_CANCEL,

    /*! Operation was timed out. */
    SER_STATUS_TIMEOUT,

    /*! Asynchronous I/O has been requested, but the routine would need to block. */
    SER_STATUS_WOULD_BLOCK
};

/*!
*******************************************************************************

 These enumerations define serial port word formats.
 They are used in the 'format' field in the ::SER_SETTINGS_T structure.
 Note that 'sticky' parity is not supported in this API as it is not a
 particularly useful data format.

*******************************************************************************/
enum SER_FORMATS
{
    /*! Set format to 5 bits, 1 stop, no parity. */
    SER_FORMAT_B5S1P0 = 0,

    /*! Set format to 6 bits, 1 stop, no parity. */
    SER_FORMAT_B6S1P0,

    /*! Set format to 7 bits, 1 stop, no parity. */
    SER_FORMAT_B7S1P0,

    /*! Set format to 8 bits, 1 stop, no parity. */
    SER_FORMAT_B8S1P0,

    /*! Set format to 5 bits, 2 stop, no parity. */
    SER_FORMAT_B5S2P0,

    /*! Set format to 6 bits, 2 stop, no parity. */
    SER_FORMAT_B6S2P0,

    /*! Set format to 7 bits, 2 stop, no parity. */
    SER_FORMAT_B7S2P0,

    /*! Set format to 8 bits, 2 stop, no parity. */
    SER_FORMAT_B8S2P0,

    /*! Set format to 5 bits, 1 stop, odd parity. */
    SER_FORMAT_B5S1PODD,

    /*! Set format to 6 bits, 1 stop, odd parity. */
    SER_FORMAT_B6S1PODD,

    /*! Set format to 7 bits, 1 stop, odd parity. */
    SER_FORMAT_B7S1PODD,

    /*! Set format to 8 bits, 1 stop, odd parity. */
    SER_FORMAT_B8S1PODD,

    /*! Set format to 5 bits, 2 stop, odd parity. */
    SER_FORMAT_B5S2PODD,

    /*! Set format to 6 bits, 2 stop, odd parity. */
    SER_FORMAT_B6S2PODD,

    /*! Set format to 7 bits, 2 stop, odd parity. */
    SER_FORMAT_B7S2PODD,

    /*! Set format to 8 bits, 2 stop, odd parity. */
    SER_FORMAT_B8S2PODD,

    /*! Reserved. */
    SER_FORMAT_RESERVED0,

    /*! Reserved. */
    SER_FORMAT_RESERVED1,

    /*! Reserved. */
    SER_FORMAT_RESERVED2,

    /*! Reserved. */
    SER_FORMAT_RESERVED3,

    /*! Reserved. */
    SER_FORMAT_RESERVED4,

    /*! Reserved. */
    SER_FORMAT_RESERVED5,

    /*! Reserved. */
    SER_FORMAT_RESERVED6,

    /*! Reserved. */
    SER_FORMAT_RESERVED7,

    /*! Set format to 5 bits, 1 stop, even parity. */
    SER_FORMAT_B5S1PEVEN,

    /*! Set format to 6 bits, 1 stop, even parity. */
    SER_FORMAT_B6S1PEVEN,

    /*! Set format to 7 bits, 1 stop, even parity. */
    SER_FORMAT_B7S1PEVEN,

    /*! Set format to 8 bits, 1 stop, even parity. */
    SER_FORMAT_B8S1PEVEN,

    /*! Set format to 5 bits, 2 stop, even parity. */
    SER_FORMAT_B5S2PEVEN,

    /*! Set format to 6 bits, 2 stop, even parity. */
    SER_FORMAT_B6S2PEVEN,

    /*! Set format to 7 bits, 2 stop, even parity. */
    SER_FORMAT_B7S2PEVEN,

    /*! Set format to 8 bits, 2 stop, even parity. */
    SER_FORMAT_B8S2PEVEN

};

/*!
*******************************************************************************

 These enumerations define RX FIFO trigger thresholds.
 They are used in the 'rxthreshold' field in the ::SER_SETTINGS_T structure.

*******************************************************************************/
enum SER_RXTHRESHOLDS
{
    /*! Trigger with 1 byte in the FIFO. */
    SER_RXTHRESHOLD_1 = 0,

    /*! Trigger with 4 bytes in the FIFO. */
    SER_RXTHRESHOLD_4,

    /*! Trigger with 8 bytes in the FIFO. */
    SER_RXTHRESHOLD_8,

    /*! Trigger with 14 bytes in the FIFO. */
    SER_RXTHRESHOLD_14
};

/*!
*******************************************************************************

 These enumerations define the flow control type.

*******************************************************************************/
enum SER_FLOW_CONTROL
{
    /*! No flow control */
    SER_FLOW_CONTROL_NONE = 0,

    /*! Hardware CTS/RTS */
    SER_FLOW_CONTROL_CTSRTS,
};

/******************************************************************************
****************************** End enumerations *******************************
*******************************************************************************/


/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/

/*! Infinite wait (do not time out). */
#define SER_INF_TIMEOUT -1

/******************************************************************************
************************** End constant definitions ***************************
*******************************************************************************/


/******************************************************************************
****************************** Type definitions *******************************
*******************************************************************************/

/*!
*******************************************************************************

 @brief This type defines the Callback Routine used by the SER device driver in the
 asynchronous transfer mode.

 The callback function provides a way for the device driver/API to notify the
 calling application of I/O completion. The driver does not require the callback
 function to provide any specific function, but it must not use any MeOS
 functions that would require a scheduling operation.

 \param     *context            Pointer to private context for the callback.
 \param     *buffer             Pointer to buffer where data was transferred to/from.
 \param     num_bytes_to_transfer  Number of bytes that serial port was programmed
                                   to transfer.
 \param     num_bytes_transferred  Number of bytes actually transferred.
 \param     status              Status code describing the completion status of
                                the transaction:\n
                                ::SER_STATUS_SUCCESS - Operation completed successfully.\n
                                ::SER_STATUS_CANCEL  - Operation was cancelled.\n
                                ::SER_STATUS_TIMEOUT - Operation was timed out.\n

*******************************************************************************/
typedef void SER_CALLBACKROUTINE_T(void          *context,
                                   unsigned char *buffer,
                                   unsigned long num_bytes_to_transfer,
                                   unsigned long num_bytes_transferred,
                                   unsigned long status);

/*!
*******************************************************************************

 @brief This structure defines the SER Callback type.

*******************************************************************************/
typedef struct ser_callback_t
{
    /*! Callback routine. */
    SER_CALLBACKROUTINE_T *routine;

    /*! Pointer to private context that the callback routine will be entered with. */
    void                  *context;
} SER_CALLBACK_T;

/*!
*******************************************************************************

 @brief This structure defines the Serial Port I/O block type required for asynchronous
 operation.

*******************************************************************************/
typedef struct ser_io_block_t
{
    /*! MeOS pool linkage structure, allowing this structure to be pooled */
    KRN_POOLLINK;

    /*! MeOS QIO Control Block */
    QIO_IOCB_T     iocb;

    /*! Callback parameters */
    SER_CALLBACK_T callback;

} SER_IO_BLOCK_T;

/*!
*******************************************************************************

 @brief This structure defines the asynchronous I/O descriptor type.

*******************************************************************************/
typedef struct ser_async_t
{
    /*! Set to TRUE if the application does not want to retrieve
        the result of the operation, FALSE otherwise.*/
    signed long           forget;

    /*! Pointer to SER Callback routine. (Set to NULL if no callback is required). */
    SER_CALLBACKROUTINE_T *callback_routine;

    /*! Pointer to private context that the callback routine will be entered with. */
    void                  *callback_context;
} SER_ASYNC_T;


/*! Async descriptor helper macro used to get result of async operation. */
#define SER_ASYNC_GET_RESULT(async, context) \
    { (async)->forget = 0;                     \
      (async)->callback_context = (context); }

/*! Async descriptor helper macro used to ignore result of async operation. */
#define SER_ASYNC_FORGET(async) \
    { (async)->forget = 1; \
      (async)->callback_routine = NULL; }

/*! Async descriptor helper macro used to register a callback routine to
   collect the result of an async operation. */
#define SER_ASYNC_CALLBACK(async, routine, context) \
    { (async)->forget = 1; \
      (async)->callback_routine = routine; \
      (async)->callback_context = context; }

/*!
*******************************************************************************

 @brief This structure holds the context required by one-direction of the serial
 port link.

*******************************************************************************/
typedef struct ser_link_t
{
    /*! QIO device. */
    QIO_DEVICE_T  device;

    /*! KRN mailbox */
    KRN_MAILBOX_T mailbox;

} SER_LINK_T;

/*!
*******************************************************************************

 @brief This structure defines the Serial Port configurationsettings.

*******************************************************************************/
typedef struct ser_settings_t
{
    /*! Baud rate divisor. */
    unsigned short baudrate;

    /*! Word format. This must be set to one of the ::SER_FORMATS values.
        The format defines the number of data bits, number of stop bits and
        parity type for the transaction. */
    unsigned char format;

    /*! Receive FIFO trigger threshold. This must be set to one of the
        ::SER_RXTHRESHOLDS values */
    unsigned char rxthreshold;

} SER_SETTINGS_T;

/*!
*******************************************************************************

 @brief This structure describes a serial port logical object.
 It contains all context required by a single serial port.

*******************************************************************************/
typedef struct ser_port_t
{
	/*! Is API initialised? */
	IMG_BOOL		bInitialised;
	
    /*! Port number */
    unsigned char	port_number;

    /*! CTS/RTS select (TRUE to use CTS/RTS) */
    signed long		use_cts_rts;

    /*! Port settings */
    SER_SETTINGS_T	port_settings;

    /*! I/O block's KRN pool */
    KRN_POOL_T		io_block_pool;

    /*! Serial port transmit operation link structure */
    SER_LINK_T		tx;

    /*! Serial port receive operation link structure */
    SER_LINK_T		rx;

#define INTERNAL_SER_MEM_SIZE		(70)
	/* Memory for internal driver structures */
	img_uint8		aui8InternalMem[ INTERNAL_SER_MEM_SIZE ];

} SER_PORT_T;

/******************************************************************************
**************************** End type definitions *****************************
*******************************************************************************/


/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

/*!
*******************************************************************************
 @Function				@SERDefine

 <b>Description:</b>\n
 This function defines an instance of a UART block.

*******************************************************************************/
img_void SERDefine(		ioblock_sBlockDescriptor	*	psBlockDescriptor	);


/*!
*******************************************************************************

 @Function              @SERInit

 <b>Description:</b>\n
 This function is used to initialise the Serial Port. It should be called
 before carrying out a serial port transaction by calling SERRead() or
 SERWrite(). It is not necessary to re-initialise the port before further
 transactions unless any of the parameters set at initialisation are to be
 altered.

 Most of the parameters set up at the initialisation stage are provided in the
 port_settings input argument. This contains the baud rate divisor, data format,
 RX FIFO trigger threshold and serial port reference clock select.

 The 'use_cts_rts' flag determines whether or not CTS/RTS flow control is used
 and is passed in as a seperate argument. The number of the serial port to be
 used is also passed to this initialisation function.

 Following initialisation, the port descriptor (::SER_PORT_T) can be used with
 other API functions to identify the initialised serial port.

 \param     *port               Pointer to port descriptor.
 \param      port_number        Port number.
 \param      use_cts_rts        Flag controlling CTS/RTS flow control.\n
                                    0 disables CTS/RTS.\n
                                    1 enables CTS/RTS.
 \param     *port_settings      Pointer to port configuration options.
 \param     *io_blocks          I/O blocks (required for asynchronous I/O).
 \param      num_io_blocks      Number of I/O blocks.

 \return                        This function returns as follows:\n
                                ::SER_STATUS_SUCCESS        Operation completed successfully.\n
                                ::SER_STATUS_INVALID_PORT   Invalid port number.\n
                                ::SER_STATUS_INVALID_CTSRTS Invalid CTS/RTS setting.\n
                                ::SER_STATUS_CONFIG_FAILED  Port configuration failed.

*******************************************************************************/
unsigned long SERInit(
    SER_PORT_T     *port,
    unsigned char   port_number,
    signed long     use_cts_rts,
    SER_SETTINGS_T *port_settings,
    SER_IO_BLOCK_T *io_blocks,
    unsigned long   num_io_blocks
);

/*!
*******************************************************************************

 @Function              @SERDeinit

 <b>Description:</b>\n
 This function is used to deinitialise the serial API, and should be used as
 part of a safe system shutdown.
 
  \param     *port               Pointer to port descriptor.
  
  \return                        This function returns as follows:\n
                                ::SER_STATUS_SUCCESS        Operation completed successfully.\n
                                ::SER_STATUS_INVALID_PORT   Invalid port number.\n 

*******************************************************************************/
unsigned long SERDeinit(
    SER_PORT_T     *port
);

/*!
*******************************************************************************

 @Function              @SERRead

 <b>Description:</b>\n
 This function is used to carry out a read transaction. When it is called,
 the Serial Port is set up ready to read in data. When the device it is connected
 to transmits data, it is received by the serial port and stored in incrementing
 memory locations, starting at the address specified in the 'buffer' input argument.\n
 Once the port has received 'num_bytes_to_read' bytes, the function completes.\n
 The 'num_bytes_read' variable is updated with the number of bytes received.

 \param     *port               Pointer to port descriptor.
 \param     *buffer             Pointer to received data buffer.
 \param      num_bytes_to_read  Number of bytes to read from UART.
 \param     *num_bytes_read     Updated each time a byte is successfully
                                received, giving a count of the number of
                                bytes read from the slave.
                                This is ONLY updated for synchronous operation.
 \param     *async              Pointer to asynchronous I/O descriptor if a
                                non-blocking transfer is required (else NULL).
 \param     timeout             Number of MeOS timer ticks before operation
                                times out. Set to ::SER_INF_TIMEOUT for an
                                infinite period of time. The timer starts when
                                QIO start function is called. This parameter has
                                the same implication for blocking and non-blocking
                                transfers.

 \return                        This function returns as follows:\n
                                ::SER_STATUS_SUCCESS      Operation completed successfully.\n
                                ::SER_STATUS_CANCEL       Operation was cancelled.\n
                                ::SER_STATUS_TIMEOUT      Operation timed out.\n
                                ::SER_STATUS_WOULD_BLOCK  Asynchronous I/O has been requested,
                                                          but the routine would need to block.

                                Note:
                                This routine will always return successfully
                                if asynchronous I/O is requested.

*******************************************************************************/
unsigned long SERRead
(
    SER_PORT_T    *port,
    unsigned char *buffer,
    unsigned long  num_bytes_to_read,
    unsigned long *num_bytes_read,
    SER_ASYNC_T   *async,
    long           timeout
);

/*!
*******************************************************************************

 @Function              @SERWrite

 <b>Description:</b>\n
 This function is used to carry out a write transaction. When it is called,
 the Serial Port transmits data from incrementing memory locations, starting
 at the address specified in the 'buffer' input argument.\n
 Once the port has transmitted 'num_bytes_to_write' bytes, the function completes.\n
 The 'num_bytes_written' variable is updated with the number of bytes transmitted.

 \param     *port               Pointer to port descriptor.
 \param     *buffer             Pointer to buffer containing data to be transmitted.
 \param      num_bytes_to_write Number of bytes to write.
 \param     *num_bytes_written  Updated each time a byte is successfully
                                transmitted, giving a count of the number of
                                bytes written to the slave.
                                This is ONLY updated for synchronous operation.
 \param     *async              Pointer to asynchronous I/O descriptor if a
                                non-blocking transfer is required (else NULL).
 \param     timeout             Number of MeOS timer ticks before operation
                                times out. Set to ::SER_INF_TIMEOUT for an
                                infinite period of time. The timer starts when
                                QIO start function is called. This parameter has
                                the same implication for blocking and non-blocking
                                transfers.

 \return                        This function returns as follows:\n
                                ::SER_STATUS_SUCCESS      Operation completed successfully.\n
                                ::SER_STATUS_CANCEL       Operation was cancelled.\n
                                ::SER_STATUS_TIMEOUT      Operation timed out.\n
                                ::SER_STATUS_WOULD_BLOCK  Asynchronous I/O has been requested,
                                                          but the routine would need to block.

                                Note:
                                This routine will always return successfully
                                if asynchronous I/O is requested.

*******************************************************************************/
unsigned long SERWrite
(
    SER_PORT_T    *port,
    unsigned char *buffer,
    unsigned long  num_bytes_to_write,
    unsigned long *num_bytes_written,
    SER_ASYNC_T   *async,
    long           timeout
);

/*!
*******************************************************************************

 @Function              @SERGetReadResult

 <b>Description:</b>\n
 This function gets the result of an asynchronous read operation. It can be
 called after an SER read transaction has been carried out. The function is
 passed the ::SER_PORT_T descriptor for the port it is retrieving the
 result from. It determines the number of bytes read in and updates the
 contents of the num_bytes_read pointer passed to it. It also returns a status
 code describing the way in which the transaction completed.

 \param     *port               Pointer to port descriptor.
 \param     *num_bytes_read     Updated with the number of bytes read.
 \param    **context            Updated with the pointer to the context of
                                the transaction it is retrieving the result from.
 \param      block              Blocking flag. 1 to block, 0 not to block.
 \param      timeout            Timeout to use when blocking.
                                Number of MeOS timer ticks before operation
                                times out. Set to ::SER_INF_TIMEOUT for an
                                infinite period of time.

 \return                        This function returns as follows:\n
                                ::SER_STATUS_SUCCESS      Operation completed successfully.\n
                                ::SER_STATUS_CANCEL       Operation was cancelled.\n
                                ::SER_STATUS_TIMEOUT      Operation timed out.\n
                                ::SER_STATUS_WOULD_BLOCK  No transaction has completed.

*******************************************************************************/
unsigned long SERGetReadResult
(
    SER_PORT_T     *port,
    unsigned long  *num_bytes_read,
    void          **context,
    signed long     block,
    long            timeout
);


/*!
*******************************************************************************

 @Function              @SERGetWriteResult

 <b>Description:</b>\n
 This function gets the result of an asynchronous write operation. It can be
 called after an SER write transaction has been carried out. The function is
 passed the ::SER_PORT_T descriptor for the port it is retrieving the
 result from. It determines the number of bytes written and updates the
 contents of the num_bytes_written pointer passed to it. It also returns a
 status code describing the way in which the transaction completed.

 \param     *port               Pointer to port descriptor.
 \param     *num_bytes_written  Updated with the number of bytes written.
 \param    **context            Updated with the pointer to the context of
                                the transaction it is retrieving the result from.
 \param      block              Blocking flag. 1 to block, 0 not to block.
 \param      timeout            Timeout to use when blocking.
                                Number of MeOS timer ticks before operation
                                times out. Set to ::SER_INF_TIMEOUT for an
                                infinite period of time.

 \return                        This function returns as follows:\n
                                ::SER_STATUS_SUCCESS      Operation completed successfully.\n
                                ::SER_STATUS_CANCEL       Operation was cancelled.\n
                                ::SER_STATUS_TIMEOUT      Operation timed out.\n
                                ::SER_STATUS_WOULD_BLOCK  No transaction has completed.

*******************************************************************************/
unsigned long SERGetWriteResult
(
    SER_PORT_T     *port,
    unsigned long  *num_bytes_written,
    void          **context,
    signed long     block,
    long            timeout
);


/*!
*******************************************************************************

 @Function              @SERCancelRead

 <b>Description:</b>\n
 This function cancels all read operations queued on the connected device,
 resets the driver from any error flag previously detected and
 flushes the hardware receiver FIFO.
 The function is passed the ::SER_PORT_T descriptor for the transaction it
 is cancelling.

 \param     *port               Pointer to port descriptor.

 \return                        none.

*******************************************************************************/
void SERCancelRead
(
    SER_PORT_T    *port
);


/*!
*******************************************************************************

 @Function              @SERCancelWrite

 <b>Description:</b>\n
 This function cancels all write operations queued on the connected device.
 The function is passed the ::SER_PORT_T descriptor for the transaction it
 is cancelling.

 \param     *port               Pointer to port descriptor.

 \return                        none.

*******************************************************************************/
void SERCancelWrite
(
    SER_PORT_T    *port
);

extern ioblock_sBlockDescriptor	IMG_asUARTBlock[];

/******************************************************************************
******************************** End Functions ********************************
*******************************************************************************/

#endif /* __SER_API_H__ */
