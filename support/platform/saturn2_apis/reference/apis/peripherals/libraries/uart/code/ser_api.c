/*!
*******************************************************************************
  file   ser_api.c

  brief  Serial Port (SER/UART) API

         This file contains the functions that make up the Serial Port (SER) API.

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

/* ---------------------------- INCLUDE FILES ---------------------------- */

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

#include <img_defs.h>

#include <ioblock_defs.h>

/* Serial Port Driver */
#include "ser_api.h"
#include "uart_drv.h"


/* -------------------------- MACRO DEFINITIONS -------------------------- */

/* definitions to improve readability */
#ifndef TRUE
#define TRUE  (1)
#endif


/* ------------------------- FUNCTION PROTOTYPES ------------------------- */

static int SerComplete(
    QIO_DEVICE_T *dev,
    QIO_IOCB_T   *iocb,
    QIO_IOPARS_T *iopars,
    QIO_STATUS_T qio_status
    );

static unsigned long SerReadWrite(
    signed long   read,
    SER_PORT_T    *port,
    unsigned char *buffer,
    unsigned long num_bytes_to_transfer,
    unsigned long *num_bytes_transferred,
    SER_ASYNC_T   *async,
    long          timeout
    );

static unsigned long SerGetReadWriteResult(
    signed long   read,
    SER_PORT_T    *port,
    unsigned long *num_bytes_transferred,
    void          **context,
    signed long   block,
    long          timeout
    );

static unsigned long SerConfigPort(SER_PORT_T *port);

static void SerStartPort(SER_PORT_T *port);
static void SerStopPort(SER_PORT_T *port);


/* --------------------------- STATIC FUNCTIONS -------------------------- */

/*
** FUNCTION:    SerComplete
**
** DESCRIPTION: Completion routine.
**
** Standard QIO completion routine arguments and return.
*/
static int SerComplete(
    QIO_DEVICE_T *dev,
    QIO_IOCB_T   *iocb,
    QIO_IOPARS_T *iopars,
    QIO_STATUS_T qio_status
    )
{
    SER_IO_BLOCK_T *io_block;
    unsigned long  num_bytes_transferred;
    unsigned long  ser_status = SER_STATUS_TIMEOUT;
    (void)dev;		// make compiler happy as this parameter is not used
    (void)iocb;		// make compiler happy as this parameter is not used

    io_block = (SER_IO_BLOCK_T *) iopars->spare;

    num_bytes_transferred = 0;

    //
    // Check status.
    //
    if (qio_status == QIO_NORMAL)
    {
		//
		// Set success/error flag
		//
		ser_status = iopars->opcode;

        num_bytes_transferred = iopars->counter;
    }
    else if (qio_status == QIO_CANCEL)
    {
        ser_status = SER_STATUS_CANCEL;
    }
    else if (qio_status == QIO_TIMEOUT)
    {
        ser_status = SER_STATUS_TIMEOUT;
    }

    //
    // Call the callback routine.
    //
    if (io_block->callback.routine != NULL)
    {
        (*io_block->callback.routine)(io_block->callback.context,
                                      iopars->pointer,
                                      iopars->counter,
                                      num_bytes_transferred,
                                      ser_status);
    }

    //
    // Return the I/O block to the pool.
    //
    KRN_returnPool(io_block);

    //
    // Never deliver to the mailbox.
    //
    return TRUE;
}

/*
** FUNCTION:    SerReadWrite
**
** DESCRIPTION: Work function for SERRead/SERWrite.
**
** INPUTS:      NAME            DESCRIPTION
**              read            Read/write flag (1 for read).
**
** All other arguments and return as per SERRead/SERWrite.
*/
static unsigned long SerReadWrite(
    signed long   read,
    SER_PORT_T    *port,
    unsigned char *buffer,
    unsigned long num_bytes_to_transfer,
    unsigned long *num_bytes_transferred,
    SER_ASYNC_T   *async,
    long          timeout
    )
{
    QIO_DEVICE_T  *device;
    KRN_MAILBOX_T *mailbox;
    QIO_IOPARS_T  iopars;
    unsigned long ser_status;

    *num_bytes_transferred = 0;

    ser_status = SER_STATUS_SUCCESS;

    //
    // Are we reading or writing?
    //
    if (read)
    {
        device  = &port->rx.device;
        mailbox = &port->rx.mailbox;
    }
    else
    {
        device  = &port->tx.device;
        mailbox = &port->tx.mailbox;
    }

    //
    // Fill in I/O parameters.
    //
    iopars.opcode  = SER_STATUS_SUCCESS;
    iopars.pointer = buffer;
    iopars.counter = num_bytes_to_transfer;

    //
    // Fire off the I/O.
    //
    if (async == NULL)
    {
        //
        // Synchronous I/O.
        //
        QIO_STATUS_T  qio_status;

        qio_status = QIO_qioWait(device, &iopars, timeout);

        if (qio_status == QIO_NORMAL)
        {
			//
			// Set success/error flag
			//
			ser_status = iopars.opcode;

            *num_bytes_transferred = iopars.counter;
        }
        else if (qio_status == QIO_CANCEL)
        {
            ser_status = SER_STATUS_CANCEL;
        }
        else if (qio_status == QIO_TIMEOUT)
        {
            ser_status = SER_STATUS_TIMEOUT;
        }
    }
    else
    {
        //
        // Asynchronous I/O.
        //
        SER_IO_BLOCK_T *io_block;

        io_block = KRN_takePool(&port->io_block_pool, 0);
        if (io_block == NULL)
        {
            return SER_STATUS_WOULD_BLOCK;
        }

        io_block->callback.routine = async->callback_routine;
        io_block->callback.context = async->callback_context;

        iopars.spare = io_block;

        QIO_qio(device,
                &io_block->iocb,
                &iopars,
                mailbox,
                async->forget ? SerComplete:NULL,
                timeout);
    }

    return ser_status;
}

/*
** FUNCTION:    SerGetReadWriteResult
**
** DESCRIPTION: Work function for SERGetReadResult/SERGetWriteResult.
**
** INPUTS:      NAME            DESCRIPTION
**              read            Read/write flag (1 for read).
**
** All other arguments and return as per SERGetReadResult/SERGetWriteResult.
*/
static unsigned long SerGetReadWriteResult(
    signed long   read,
    SER_PORT_T    *port,
    unsigned long *num_bytes_transferred,
    void          **context,
    signed long   block,
    long          timeout
    )
{
    KRN_MAILBOX_T *mailbox;
    QIO_IOCB_T    *iocb;
    QIO_DEVICE_T  *dev;
    QIO_STATUS_T  qio_status;
    QIO_IOPARS_T  iopars;
    unsigned long ser_status = SER_STATUS_TIMEOUT;
    SER_IO_BLOCK_T *ioBlock;

    *num_bytes_transferred = 0;

    //
    // Are we getting the result for a read or a write?
    //
    if (read)
    {
        mailbox = &port->rx.mailbox;
    }
    else
    {
        mailbox = &port->tx.mailbox;
    }

    //
    // Get the result from the mailbox.
    //
    iocb = QIO_result(mailbox,
                      &dev,
                      &qio_status,
                      &iopars,
                      block ? timeout:0);
    if (iocb == NULL)
    {
        return SER_STATUS_WOULD_BLOCK;
    }

    ioBlock = (SER_IO_BLOCK_T *) iopars.spare;
    *context = ioBlock->callback.context;

    //
    // Return the IOCB to the pool.
    //
    KRN_returnPool(ioBlock);

    //
    // Check status.
    //
    if (qio_status == QIO_NORMAL)
    {
		//
		// Set success/error flag
		//
		ser_status = iopars.opcode;

        *num_bytes_transferred = iopars.counter;

    }
    else if (qio_status == QIO_CANCEL)
    {
        ser_status = SER_STATUS_CANCEL;
    }
    else if (qio_status == QIO_TIMEOUT)
    {
        ser_status = SER_STATUS_TIMEOUT;
    }

    return ser_status;
}

/*
** FUNCTION:    SerConfigPort
**
** DESCRIPTION: Configures a serial port.
**
** INPUTS:      NAME            DESCRIPTION
**              port            The port to configure.
**
** OUTPUTS:     NAME            DESCRIPTION
**              port            The port has been configured.
**
** RETURNS:     void
*/
static unsigned long SerConfigPort(SER_PORT_T *port)
{
    UART_PARA_TYPE para;

    para.divisor         = port->port_settings.baudrate;
    para.wordFormat      = port->port_settings.format;
    para.rxTrigThreshold = port->port_settings.rxthreshold;
    para.flowControl     = port->use_cts_rts;

    if (UARTConfig(port->port_number, &para) != SER_OK)
    {
        return SER_STATUS_CONFIG_FAILED;
    }

    return SER_STATUS_SUCCESS;
}

/*
** FUNCTION:    SerStartPort.
**
** DESCRIPTION: Starts a serial port.
**
** INPUTS:      NAME            DESCRIPTION
**              port            The port to start.
**
** OUTPUTS:     NAME            DESCRIPTION
**              port            The port devices are initialised and enabled.
**
** RETURNS:     void
*/
static void SerStartPort(SER_PORT_T *port)
{
    //
    // Initialise the devices.
    //
    // TX first: Must be this order...
    QIO_init(&port->tx.device, "STX device", port->port_number, &STX_driver);
    QIO_init(&port->rx.device, "SRX device", port->port_number, &SRX_driver);

    //
    // Enable the devices.
    //
    QIO_enable(&port->tx.device);
    QIO_enable(&port->rx.device);
}

/*
** FUNCTION:    SerStartPort.
*/
static void SerStopPort(SER_PORT_T *port)
{
    //
    // Disable the devices.
    //
    QIO_disable(&port->tx.device);
    QIO_disable(&port->rx.device);
}


static img_void serDefine( ioblock_sBlockDescriptor	*	psBlockDescriptor	)
{
	// Check for a valid descriptor
	IMG_ASSERT( psBlockDescriptor );
	// Check our internal descriptor pointer array is big enough
	IMG_ASSERT( psBlockDescriptor->ui32Index < MAX_NUM_UART_BLOCKS );
	// Check that this instance has not already been defined
	IMG_ASSERT( !g_apsUARTBlock[ psBlockDescriptor->ui32Index ] );

	// Assign the descriptor to our internal list
	g_apsUARTBlock[ psBlockDescriptor->ui32Index ] = psBlockDescriptor;
}


/* -------------------------- EXPORTED FUNCTIONS ------------------------- */

/*!
*******************************************************************************
 @Function				@SERDefine

 <b>Description:</b>\n
 This function defines an instance of a UART block.

*******************************************************************************/
img_void SERDefine(		ioblock_sBlockDescriptor	*	psBlockDescriptor	)
{
	// This function is deprecated, but left here for backward compatibility
	return;
}

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
    unsigned char  port_number,
    signed long    use_cts_rts,
    SER_SETTINGS_T *port_settings,
    SER_IO_BLOCK_T *io_blocks,
    unsigned long  num_io_blocks)
{
    unsigned long ser_status;

	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_FALSE );
	IMG_ASSERT ( port_settings != IMG_NULL );
	IMG_ASSERT ( io_blocks != IMG_NULL );
	IMG_ASSERT ( num_io_blocks > 0 );
	
	/* Clear context structure */
	IMG_MEMSET ( port, 0, sizeof(SER_PORT_T) );

    //
    // Check and setup parameters.
    //
    if (port_number >= MAX_NUM_UART_BLOCKS)
    {
        return SER_STATUS_INVALID_PORT;
    }

    if (((port_settings->format > SER_FORMAT_B8S2PODD) && (port_settings->format < SER_FORMAT_B5S1PEVEN)) ||
        (port_settings->format > SER_FORMAT_B8S2PEVEN))
    {
        return SER_STATUS_INVALID_WORDFORMAT;
	}

    if (port_settings->rxthreshold > SER_RXTHRESHOLD_14)
    {
        return SER_STATUS_INVALID_THRESHOLD;
	}

    if ((use_cts_rts != SER_FLOW_CONTROL_NONE) && (use_cts_rts != SER_FLOW_CONTROL_CTSRTS))
    {
        return SER_STATUS_INVALID_CTSRTS;
    }

    port->port_number   = port_number;
    port->use_cts_rts   = use_cts_rts;
    port->port_settings = *port_settings;

	// Check if this block has been defined, and if not, then define it
	if ( !g_apsUARTBlock[ port_number ] )
	{
		serDefine( &IMG_asUARTBlock[ port_number ] );
	}

	// Set up context pointer
	g_apsUARTBlock[ port_number ]->pvAPIContext = (img_void *)&port->aui8InternalMem;

    //
    // Configure the serial port.
    //
    ser_status = SerConfigPort(port);
    if (ser_status != SER_STATUS_SUCCESS)
    {
        return ser_status;
    }

    //
    // Initialise mailboxes.
    //
    KRN_initMbox(&port->tx.mailbox);
    KRN_initMbox(&port->rx.mailbox);

    //
    // Initialise the IOCB pool.
    //
    KRN_initPool(&port->io_block_pool,
                 io_blocks,
                 num_io_blocks,
                 sizeof(SER_IO_BLOCK_T));

    //
    // Start the serial port.
    //
    SerStartPort(port);

	// Mark API as initialised
	port->bInitialised = IMG_TRUE;
	
    //
    // All OK.
    //
    return SER_STATUS_SUCCESS;
}


/*!
*******************************************************************************

 @Function              @SERDeinit

*******************************************************************************/
unsigned long SERDeinit( SER_PORT_T *	port )
{
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );
	
    //
    // Stop the serial port.
    //
    SerStopPort(port);

    QIO_unload(&port->tx.device);
    QIO_unload(&port->rx.device);

	// Mark API as initialised
	port->bInitialised = IMG_FALSE;		
	
	return SER_STATUS_SUCCESS;
}


/*!
*******************************************************************************

 @Function              @SERRead

 <b>Description:</b>\n
 This function is used to carry out a read transaction. When it is called,
 the Serial Port is set up ready to read in data. When the device it is connected
 to transmits data, it is received by the serial port and stored in incrementing
 memory locations, starting at the address specified in the 'buffer' input argument.\n
 Once the port has received 'num_bytes_to_read' bytes, the function completes.\n
 The 'num_bytes_read' variable is updated with the number of bytes received,

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
unsigned long SERRead(
    SER_PORT_T    *port,
    unsigned char *buffer,
    unsigned long num_bytes_to_read,
    unsigned long *num_bytes_read,
    SER_ASYNC_T   *async,
    long          timeout)
{
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );
	
    return SerReadWrite(1,
                        port,
                        buffer,
                        num_bytes_to_read,
                        num_bytes_read,
                        async,
                        timeout);
}

/*!
*******************************************************************************

 @Function              @SERWrite

 <b>Description:</b>\n
 This function is used to carry out a write transaction. When it is called,
 the Serial Port transmits data from incrementing memory locations, starting
 at the address specified in the 'buffer' input argument.\n
 Once the port has transmitted 'num_bytes_to_write' bytes, the function completes.\n
 The 'num_bytes_written' variable is updated with the number of bytes transmitted,

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
unsigned long SERWrite(
    SER_PORT_T    *port,
    unsigned char *buffer,
    unsigned long num_bytes_to_write,
    unsigned long *num_bytes_written,
    SER_ASYNC_T   *async,
    long          timeout)
{
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );	
	
    return SerReadWrite(0,
                        port,
                        buffer,
                        num_bytes_to_write,
                        num_bytes_written,
                        async,
                        timeout);
}

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
unsigned long SERGetReadResult(
    SER_PORT_T    *port,
    unsigned long *num_bytes_read,
    void          **context,
    signed long   block,
    long          timeout)
{
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );	
	
    return SerGetReadWriteResult(1,
                                 port,
                                 num_bytes_read,
                                 context,
                                 block,
                                 timeout);
}

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
unsigned long SERGetWriteResult(
    SER_PORT_T    *port,
    unsigned long *num_bytes_written,
    void          **context,
    signed long   block,
    long          timeout)
{
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );	
	
    return SerGetReadWriteResult(0,
                                 port,
                                 num_bytes_written,
                                 context,
                                 block,
                                 timeout);
}

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
void SERCancelRead(
    SER_PORT_T    *port)
{
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );	
	
    QIO_cancelAll(&port->rx.device);

    UARTResetError(port->port_number);
}

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
void SERCancelWrite(
    SER_PORT_T    *port)
{
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );	
	
    QIO_cancelAll(&port->tx.device);
}
