/*!
*******************************************************************************
  file   scbm_api.c

  brief  Serial Control Bus Master API

         This file defines the main functions that make up the Serial
         Control Bus Master (SCBM) API.

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

 <b>Platform:</b>\n
         Tarantino

*******************************************************************************/

/*============================================================================*/
/*                                                                            */
/*                          INCLUDE FILES		                              */
/*                                                                            */
/*============================================================================*/

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


/* SCB Driver/API header files */
#include "scbm_api.h"
#include "scbm_drv.h"


/*============================================================================*/
/*                                                                            */
/*                          MACRO DEFINITIONS	                              */
/*                                                                            */
/*============================================================================*/

/* infinite wait on QIO IO */
#define INFWAIT (-1)

/* definitions to improve readability */
#ifndef TRUE
#define TRUE  (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

/*============================================================================*/
/*                                                                            */
/*                          FUNCTION PROTOTYPES	                              */
/*                                                                            */
/*============================================================================*/

/*
**	These are prototypes for API functions defined in this file, which are used
**	within the API and not accessable to the calling application.
*/

static int ScbmComplete(
    QIO_DEVICE_T *dev,
    QIO_IOCB_T   *iocb,
    QIO_IOPARS_T *iopars,
    QIO_STATUS_T qio_status
    );

static unsigned long ScbmFinishInfo(
    QIO_STATUS_T  qio_status,
    unsigned long size,
    unsigned long *num_bytes_transferred
    );

static void ScbmOperationInfo(
    unsigned long opcode,
    signed long   *read,
    unsigned long *address
    );

static unsigned long ScbmReadWrite(
    signed long   read,
    SCBM_PORT_T   *port,
    unsigned long address,
    unsigned char *buffer,
    unsigned long num_bytes_to_transfer,
    unsigned long *num_bytes_transferred,
    SCBM_ASYNC_T  *async,
    long          timeout
    );


/*============================================================================*/
/*                                                                            */
/*                          STATIC FUNCTIONS	                              */
/*                                                                            */
/*============================================================================*/


/*!
******************************************************************************

  Function              ScbmComplete

  Description			Completion routine.

 						Standard QIO completion routine arguments and return.

******************************************************************************/
static int ScbmComplete(
    QIO_DEVICE_T *dev,
    QIO_IOCB_T   *iocb,
    QIO_IOPARS_T *iopars,
    QIO_STATUS_T qio_status
    )
{
    SCBM_IO_BLOCK_T *io_block;
    signed long     read;
    unsigned long   address;
    unsigned long   num_bytes_transferred;
    unsigned long   scbm_status;

    io_block = (SCBM_IO_BLOCK_T *) iopars->spare;

    //
    // Retrieve operation information.
    //
    ScbmOperationInfo(iopars->opcode, &read, &address);

    //
    // Check completion status.
    //
    scbm_status = ScbmFinishInfo(qio_status,
                                 iopars->counter,
                                 &num_bytes_transferred);

    //
    // Call the callback routine.
    //
    if (io_block->callback.routine != NULL)
    {
        (*io_block->callback.routine)(io_block->callback.context,
                                      read,
                                      address,
                                      iopars->pointer,
                                      num_bytes_transferred,
                                      scbm_status);
    }

    //
    // Return the IO block to the pool.
    //
    KRN_returnPool(io_block);

    //
    // Never deliver to the mailbox.
    //
    return TRUE;
}


/*!
******************************************************************************

  Function              ScbmFinishInfo

  Description

 Helper function to check IO completion status and build API
 status information from it.

  Input		qio_status	: QIO completion status.
 			size		: Number of bytes to indicate on
                          success.

  Output	*num_bytes_transferred
 						: Number of bytes transferred (status information).

  Return	unsigned long

			Status code as follows:

			SCBM_STATUS_SUCCESS      Operation completed successfully.
			SCBM_STATUS_CANCEL       Operation was cancelled.
			SCBM_STATUS_TIMEOUT      Operation timed out.

******************************************************************************/
static unsigned long ScbmFinishInfo(
    QIO_STATUS_T  qio_status,
    unsigned long size,
    unsigned long *num_bytes_transferred
    )
{
    unsigned long scbm_status = SCBM_STATUS_TIMEOUT;

    if (qio_status == QIO_NORMAL)
    {
        scbm_status = SCBM_STATUS_SUCCESS;
        *num_bytes_transferred = size;
    }
    else if (qio_status == QIO_CANCEL)
    {
        scbm_status = SCBM_STATUS_CANCEL;
        *num_bytes_transferred = 0;
    }
    else if (qio_status == QIO_TIMEOUT)
    {
        scbm_status = SCBM_STATUS_TIMEOUT;
        *num_bytes_transferred = 0;
    }

    return scbm_status;
}


/*!
******************************************************************************

  Function              ScbmOperationInfo

  Description

  Helper function to retrieve IO method and address from an opcode.

  Input		opcode		: Opcode to decode.

  Output	*read		: Updated with read/write flag.
 			*address	: Updated with address.

  Return	unsigned long

			Status code as follows:

			SCBM_STATUS_SUCCESS      Operation completed successfully.
			SCBM_STATUS_CANCEL       Operation was cancelled.
			SCBM_STATUS_TIMEOUT      Operation timed out.

******************************************************************************/
static void ScbmOperationInfo(
    unsigned long opcode,
    signed long   *read,
    unsigned long *address
    )
{
    *read    = (SCBM_METHOD(opcode) == SCBM_DD_READ) ? 1:0;
    *address = SCBM_ADDRESS(opcode);
}


/*!
******************************************************************************

  Function              ScbmReadWrite

  Description

  Reads/writes data from/to a port.

  Input		read					: Read/write flag.
 			port					: Port object.
 			address					: Address of slave that data will be transferred to/from.
 			num_bytes_to_transfer	: The number of bytes to transfer.
 			async              		: Asynchronous I/O descriptor.
 			timeout           		: Operation timeout value.

  Output	buffer            		: Data is read into/written from this buffer.
 			num_bytes_transferred	: For a synchronous operation this returns
                                  	  the number of bytes actually transferred.

  Return	unsigned long

			Status code as follows:

			SCBM_STATUS_SUCCESS      Operation completed successfully.
			SCBM_STATUS_CANCEL       Operation was cancelled.
			SCBM_STATUS_TIMEOUT      Operation timed out.
			SCBM_STATUS_WOULD_BLOCK  Asynchronous I/O has been requested,
									 but the routine would need to block.

			Note:
			This routine will always return successfully if asynchronous
			I/O is requested.

******************************************************************************/
static unsigned long ScbmReadWrite(
    signed long   read,
    SCBM_PORT_T   *port,
    unsigned long address,
    unsigned char *buffer,
    unsigned long num_bytes_to_transfer,
    unsigned long *num_bytes_transferred,
    SCBM_ASYNC_T  *async,
    long          timeout
    )
{
    QIO_IOPARS_T  iopars;
    unsigned long scbm_status;

    //
    // Build the IO parameters.
    //
    iopars.opcode  = SCBM_OPCODE(read ? SCBM_DD_READ:SCBM_DD_WRITE, address);
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
        QIO_STATUS_T qio_status;

        iopars.spare = NULL;

        qio_status = QIO_qioWait(&port->device, &iopars, timeout);

        scbm_status = ScbmFinishInfo(qio_status,
                                     iopars.counter,
                                     num_bytes_transferred);
    }
    else
    {
        //
        // Asynchronous I/O.
        //
        SCBM_IO_BLOCK_T *io_block;

        io_block = KRN_takePool(&port->io_block_pool, 0);
        if (io_block == NULL)
        {
            return SCBM_STATUS_WOULD_BLOCK;
        }
        else
        {
            scbm_status = SCBM_STATUS_SUCCESS;
        }

        io_block->callback.routine = async->callback_routine;
        io_block->callback.context = async->callback_context;

        iopars.spare = io_block;

        QIO_qio(&port->device,
                &io_block->iocb,
                &iopars,
                &port->mailbox,
                async->forget ? ScbmComplete:NULL,
                timeout);
    }

    return scbm_status;
}

img_void scbmDefine(	ioblock_sBlockDescriptor	*	psBlockDescriptor	)
{
	// Check for a valid descriptor
	IMG_ASSERT( psBlockDescriptor );
	// Check our internal descriptor pointer array is big enough
	IMG_ASSERT( psBlockDescriptor->ui32Index < MAX_NUM_SCBM_BLOCKS );
	// Check that this instance has not already been defined
	IMG_ASSERT( !g_apsSCBMBlock[ psBlockDescriptor->ui32Index ] );

	// Assign the descriptor to our internal list
	g_apsSCBMBlock[ psBlockDescriptor->ui32Index ] = psBlockDescriptor;
}


/*============================================================================*/
/*                                                                            */
/*                          EXPORTED FUNCTIONS	                              */
/*                                                                            */
/*============================================================================*/

/*
*******************************************************************************

 @Function				@SCBMDefine

 <b>Description:</b>\n
 This function defines an instance of an SCBM block.

*******************************************************************************/
img_void SCBMDefine(	ioblock_sBlockDescriptor	*	psBlockDescriptor )
{
	// This function is now deprecated, but is left here for backward compatibility.
}

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
unsigned long SCBMInit(
    SCBM_PORT_T     *port,
    SCBM_SETTINGS_T *port_settings,
    SCBM_IO_BLOCK_T *io_blocks,
    unsigned long   num_io_blocks
    )
{
	IMG_ASSERT ( port != IMG_NULL );	
	IMG_ASSERT ( port->bInitialised == IMG_FALSE );
	IMG_ASSERT ( port_settings != IMG_NULL );
	
	/* Clear port structure */
	IMG_MEMSET ( port, 0, sizeof(SCBM_PORT_T) );
	
	// Check the block index is in range
	if ( port_settings->ui32BlockIndex >= MAX_NUM_SCBM_BLOCKS )
	{
		return SCBM_STATUS_INVALID_PORT;
	}

	// Define this block if it hasn't already been defined
	if ( !g_apsSCBMBlock[ port_settings->ui32BlockIndex ] )
	{
		scbmDefine( &IMG_asSCBBlock[ port_settings->ui32BlockIndex ] );
	}

	// Set the API Context space
	g_apsSCBMBlock[ port_settings->ui32BlockIndex ]->pvAPIContext = (img_void *)port->aui8InternalMem;

    //
    // Initialise mailbox.
    //
    KRN_initMbox(&port->mailbox);

    //
    // Initialise the IOCB pool.
    //
    if (io_blocks != NULL)
    {
        KRN_initPool(&port->io_block_pool,
                     io_blocks,
                     num_io_blocks,
                     sizeof(SCBM_IO_BLOCK_T));
    }

    //
    // Initialise the device.
    //
    QIO_init( &port->device,
	         "SCBM device",
	         (img_int32)port_settings,
             &SCBM_driver);

	//
	// Enable the device.
	//
	QIO_enable(&port->device);
	
	// Mark API as initialised
	port->bInitialised = IMG_TRUE;

    //
    // All OK.
    //
    return SCBM_STATUS_SUCCESS;
}


/*!
*******************************************************************************

 @Function              @SCBMDeinit

*******************************************************************************/
unsigned long SCBMDeinit( SCBM_PORT_T     *port )
{
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );
	
	// Disable QIO
	QIO_disable(&port->device);
	
	// Unload QIO
	QIO_unload(&port->device);
	
	// Mark API as no longer initialised
	port->bInitialised = IMG_FALSE;
	
	return SCBM_STATUS_SUCCESS;
}


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
 \param      address            Address of slave to be used (7/10 bits. For 7 bit addressing set address[9..7] to 0).
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
unsigned long SCBMRead(
    SCBM_PORT_T   *port,
    unsigned long  address,
    unsigned char *buffer,
    unsigned long  num_bytes_to_read,
    unsigned long *num_bytes_read,
    SCBM_ASYNC_T  *async,
    long           timeout
    )
{
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );
	
    return ScbmReadWrite(1,
                         port,
                         address,
                         buffer,
                         num_bytes_to_read,
                         num_bytes_read,
                         async,
                         timeout);
}


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
 \param      address            Address of slave to be used (7/10 bits. For 7 bit addressing, set address[9..7] to 0).
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
unsigned long SCBMWrite(
    SCBM_PORT_T   *port,
    unsigned long  address,
    unsigned char *buffer,
    unsigned long  num_bytes_to_write,
    unsigned long *num_bytes_written,
    SCBM_ASYNC_T  *async,
    long           timeout
    )
{
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );	
	
    return ScbmReadWrite(0,
                         port,
                         address,
                         buffer,
                         num_bytes_to_write,
                         num_bytes_written,
                         async,
                         timeout);
}


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
 \param     *address            Updated with address of slave used in transaction.
                                (7/10 bits. For 7 bit addressing, set address[9..7] to 0)
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
unsigned long SCBMGetResult(
    SCBM_PORT_T   *port,
    signed long   *read,
    unsigned long *address,
    unsigned long *num_bytes_transferred,
    signed long   block,
    long          timeout
    )
{
    QIO_IOCB_T    *iocb;
    QIO_DEVICE_T  *device;
    QIO_STATUS_T  qio_status;
    QIO_IOPARS_T  iopars;
    unsigned long scbm_status;

	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );

    //
    // Get the result from the mailbox.
    //
    iocb = QIO_result(&port->mailbox,
                      &device,
                      &qio_status,
                      &iopars,
                      block ? timeout:0);
    if (iocb == NULL)
    {
        *num_bytes_transferred = 0;
        return SCBM_STATUS_WOULD_BLOCK;
    }

    //
    // Return the IOCB to the pool.
    //
    KRN_returnPool(iopars.spare);

    //
    // Retrieve operation information.
    //
    ScbmOperationInfo(iopars.opcode, read, address);

    //
    // Process completion status.
    //
    scbm_status = ScbmFinishInfo(qio_status,
                                 iopars.counter,
                                 num_bytes_transferred);

    return scbm_status;
}

/*!
*******************************************************************************

 @Function              @SCBMCancel

 <b>Description:</b>\n
 This function cancels the current, and all queued asynchronous operations. The
 function is passed the ::SCBM_PORT_T descriptor for the transaction it is cancelling.

 \param     *port               Pointer to port descriptor.

 \return                        none.

*******************************************************************************/
void SCBMCancel(
    SCBM_PORT_T    *port
    )
{
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );	
	
    QIO_cancelAll(&port->device);
}


unsigned long SCBMGetErrorStatus(const SCBM_PORT_T *port)
{
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );	
	
	return SCBMasterGetErrorStatus(port);
}
