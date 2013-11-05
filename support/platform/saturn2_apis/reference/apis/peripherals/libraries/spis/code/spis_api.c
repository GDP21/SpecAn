/*!
*******************************************************************************
  file   spis_api.c

  brief  Serial Peripheral Interface Slave API

         This file defines the functions that make up the Serial Peripheral
         Interface Slave (SPIS) API.

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

#include <ioblock_defs.h>

#include <gdma_api.h>

/* SPI Slave Driver */
#include "spis_api.h"
#include "spis_drv.h"


/* -------------------------- MACRO DEFINITIONS -------------------------- */

/* definitions to improve readability */
#ifndef TRUE
#define TRUE  (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

/* infinite wait on QIO IO */
#define INFWAIT (-1)

/* -------------------------- FUNCTIONS PROTOTYPES ----------------------- */

static unsigned long SpisFinishInfo(QIO_STATUS_T  qio_status);
static int SpisComplete(QIO_DEVICE_T *dev, QIO_IOCB_T *iocb,
                        QIO_IOPARS_T *iopars, QIO_STATUS_T qio_status);

/* -------------------------- STATIC FUNCTIONS --------------------------- */


/*!
******************************************************************************

 @Function              ScbsComplete

 @Description			Completion routine.

 						Standard QIO completion routine arguments and return.

******************************************************************************/
static int SpisComplete(
    QIO_DEVICE_T *dev,
    QIO_IOCB_T   *iocb,
    QIO_IOPARS_T *iopars,
    QIO_STATUS_T qio_status
    )
{
    SPIS_IO_BLOCK_T *io_block;
    unsigned long   spis_status;

	(void)dev;
	(void)iocb;

    io_block = (SPIS_IO_BLOCK_T *) iopars->spare;

    //
    // Check status.
    //
    spis_status = SpisFinishInfo(qio_status);

    //
    // Call the callback routine.
    //
    if (io_block->callback.routine != NULL)
    {
        (*io_block->callback.routine)(io_block->callback.context, iopars->pointer,
                                      iopars->counter, spis_status);
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

/*!
******************************************************************************

 @Function              SpisFinishInfo

 @Description

 Helper function to check IO completion status and build API
 status information from it.

 @Input		qio_status	: QIO completion status.

 @Output	None

 @Return	unsigned long

			Status code as follows:

			SPIS_STATUS_SUCCESS  Operation completed successfully.
			SPIS_STATUS_CANCEL   Operation was cancelled.
			SPIS_STATUS_TIMEOUT  Operation timed out.

******************************************************************************/
static unsigned long SpisFinishInfo(QIO_STATUS_T  qio_status)
{
    unsigned long spis_status;

    if (qio_status == QIO_NORMAL)
    {
        spis_status = SPIS_STATUS_SUCCESS;
    }
    else if (qio_status == QIO_CANCEL)
    {
        spis_status = SPIS_STATUS_CANCEL;
    }
    else if (qio_status == QIO_TIMEOUT)
    {
        spis_status = SPIS_STATUS_TIMEOUT;
    }
    else
    {
        spis_status = SPIS_STATUS_SUCCESS;
    }

    return spis_status;
}

static img_void spisDefine( ioblock_sBlockDescriptor	*	psBlockDescriptor	)
{	
	// Check for a valid descriptor
	IMG_ASSERT( psBlockDescriptor );
	// Check our internal descriptor pointer array is big enough
	IMG_ASSERT( psBlockDescriptor->ui32Index < MAX_NUM_SPIS_BLOCKS );
	// Check that this instance has not already been defined
	IMG_ASSERT( !g_apsSPISBlock[ psBlockDescriptor->ui32Index ] );

	// Assign the descriptor to our internal list
	g_apsSPISBlock[ psBlockDescriptor->ui32Index ] = psBlockDescriptor;
}

/* -------------------------- EXPORTED FUNCTIONS ------------------------- */


/*!
*******************************************************************************

 @Function				@SPIMDefine

 @Description
 This function defines an instance of a SPI Master block.

*******************************************************************************/
img_void SPISDefine(	ioblock_sBlockDescriptor	*	psBlockDescriptor	)
{
	// This function is now deprecated, but is left here for backward compatibility
	return;
}

/*!
*******************************************************************************

 @Function              @SPISInit

 <b>Description:</b>\n
 This function is used to initialise the SPI Slave. This must be done before a
 transaction is carried out using SPISRead() or SPISWrite(). It is not
 necessary to re-initialise the SPIS before further transactions unless any of
 the parameters set up at initialisation are to be changed.

 The function initialises the port by allocating a QIO device object to the
 SPIS port object defined by the structure SPIS_PORT_T.

 The parameters set up at initialisation are the SPI mode, CS active level
 and DMA input and output channel numbers. DMA transfers are used to transfer
 data from memory to the SPIS (for writes) and from the SPIS to memory (for
 reads). The DMA channel numbers determine which DMA channels are used.

 The input and output DMA channels must be different.

 \param     *port               Pointer to port descriptor.
 \param     *initParam          Pointer to initialisation parameters.
 \param     *io_blocks          I/O blocks (required for asynchronous I/O).
 \param      num_io_blocks      Number of I/O blocks.

 \return                        SPIS_OK if initialisation parameters are valid
                                SPIS_INVALID_<type> if <type> parameter is not valid

*******************************************************************************/
unsigned long SPISInit
(
	SPIS_PORT_T 	*port, 
	SPIS_PARAM_T 	*initParam,
    SPIS_IO_BLOCK_T *io_blocks, 
    unsigned long 	num_io_blocks
)
{
	GDMA_sCallbackFunctions		sDMACallbacks;

    /* Check initialisation parameters */
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_FALSE );
	IMG_ASSERT ( initParam != IMG_NULL );

	/* Clear context structure */
	IMG_MEMSET ( port, 0, sizeof(SPIS_PORT_T) );

    if (initParam->spiMode > SPI_MODE_3)
    {
        return SPIS_INVALID_SPI_MODE;
	}
    if (initParam->csLevel > 1)
    {
        return SPIS_INVALID_CS_ACTIVE_LEVEL;
	}
	if ( initParam->spiSyncMode > SPI_SYNC_MODE_LEGACY )
	{
		return SPIS_INVALID_SYNC_MODE;
	}

	// Check if this block has been defined, and if not, then define it ourselves
	if ( !g_apsSPISBlock[ initParam->ui32BlockIndex ] )
	{
		spisDefine( &IMG_asSPISBlock[ initParam->ui32BlockIndex ] );
	}

	// Set the context space
	g_apsSPISBlock[ initParam->ui32BlockIndex ]->pvAPIContext = (img_void *)port;

	sDMACallbacks.pfnInitDevice		= IMG_NULL;
	sDMACallbacks.pfnStartDevice	= IMG_NULL;
	sDMACallbacks.pfnCancelDevice	= IMG_NULL;
	sDMACallbacks.pfnCompletion		= &SpisDmaComplete;
	// Initialise GDMA
	GDMA_Initialise(	&port->sDMAContext,
						initParam->dmaChannel,
						0,
						IMG_FALSE,
						&sDMACallbacks,
						(img_void *)g_apsSPISBlock[ initParam->ui32BlockIndex ],
						IMG_NULL,
						0,
						IMG_TRUE,
						0,
						IMG_NULL,
						0,
						IMG_TRUE );
	GDMA_Configure(	&port->sDMAContext );

    //
    // Initialise mailboxe.
    //
    KRN_initMbox(&port->sMailbox);

    //
    // Initialise the IOCB pool.
    //
    if (io_blocks != NULL)
    {
        KRN_initPool(&port->io_block_pool,
                     io_blocks,
                     num_io_blocks,
                     sizeof(SPIS_IO_BLOCK_T));
    }

    /* Initialise the device */
    /* pass pointer to parameters via the id field */
    QIO_init(&port->sDevice, "SPI Slave", (unsigned int)initParam, &SPIS_driver);

    /* Enable the device */
    QIO_enable(&port->sDevice);
    
    /* Mark API as initialised */
    port->bInitialised = IMG_TRUE;

    return SPIS_OK;
}

/*!
*******************************************************************************

 @Function              @SPISRead

 <b>Description:</b>\n
 This function is used to perform a 'read from master' transaction. When the
 function is called, the SPIS is made ready to receive data. The transaction
 from the peripheral to the outside world is controlled by the master,
 which sets the CS line active to start the transaction, then transmits
 a series of bytes on the MOSI (master out slave in) data line.

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
                                ::SPIS_STATUS_SUCCESS      Operation completed successfully.
                                ::SPIS_STATUS_CANCEL       Operation was cancelled.
                                ::SPIS_STATUS_TIMEOUT      Operation timed out.

*******************************************************************************/
unsigned long SPISRead(SPIS_PORT_T *port, unsigned char *buf, unsigned long dmaLength,
              SPIS_ASYNC_T *async, long timeout)
{
    QIO_DEVICE_T *device;
	QIO_IOPARS_T iopars;
	unsigned long spis_status;

	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );

    device = &port->sDevice;

	iopars.opcode  = SPIS_OPCODE_READ;
	iopars.pointer = (void *)buf;
	iopars.counter = dmaLength;

    //
    // Fire off the I/O.
    //
    if (async == NULL)
    {
        //
        // Synchronous IO.
        //
        QIO_STATUS_T  qio_status;

        qio_status = QIO_qioWait(device, &iopars, timeout);

        //
        // Check completion status.
        //
        spis_status = SpisFinishInfo(qio_status);
    }
    else
    {
        //
        // Asynchronous I/O.
        //
        SPIS_IO_BLOCK_T *io_block;

        io_block = KRN_takePool(&port->io_block_pool, 0);
        if (io_block == NULL)
        {
            return SPIS_STATUS_WOULD_BLOCK;
        }
        else
        {
            spis_status = SPIS_STATUS_SUCCESS;
        }

        io_block->callback.routine = async->callback_routine;
        io_block->callback.context = async->callback_context;

        iopars.spare = io_block;

        QIO_qio(device,
                &io_block->iocb,
                &iopars, &port->sMailbox,
                async->forget ? SpisComplete:NULL,
                timeout);

        spis_status = SPIS_STATUS_SUCCESS;
    }

    return spis_status;
}

/*!
*******************************************************************************

 @Function              @SPISWrite

 <b>Description:</b>\n
 This function is used to perform a 'write to master' transaction. When the
 function is called, the SPIS is made ready to transmit data. The transaction
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

 \return                        This function returns as follows:
                                ::SPIS_STATUS_SUCCESS      Operation completed successfully.
                                ::SPIS_STATUS_CANCEL       Operation was cancelled.
                                ::SPIS_STATUS_TIMEOUT      Operation timed out.

*******************************************************************************/
unsigned long SPISWrite(SPIS_PORT_T *port, unsigned char *buf, unsigned long dmaLength,
               SPIS_ASYNC_T *async, long timeout)
{
    QIO_DEVICE_T *device;
	QIO_IOPARS_T iopars;
	unsigned long spis_status;

	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );

    device = &port->sDevice;

	iopars.opcode  = SPIS_OPCODE_WRITE;
	iopars.pointer = (void *)buf;
	iopars.counter = dmaLength;

    //
    // Fire off the I/O.
    //
    if (async == NULL)
    {
        //
        // Synchronous IO.
        //
        QIO_STATUS_T  qio_status;

        qio_status = QIO_qioWait(device, &iopars, timeout);

        //
        // Check completion status.
        //
        spis_status = SpisFinishInfo(qio_status);
    }
    else
    {
        //
        // Asynchronous I/O.
        //
        SPIS_IO_BLOCK_T *io_block;

        io_block = KRN_takePool(&port->io_block_pool, 0);
        if (io_block == NULL)
        {
            return SPIS_STATUS_WOULD_BLOCK;
        }
        else
        {
            spis_status = SPIS_STATUS_SUCCESS;
        }

        io_block->callback.routine = async->callback_routine;
        io_block->callback.context = async->callback_context;

        iopars.spare = io_block;

        QIO_qio(device,
                &io_block->iocb,
                &iopars, &port->sMailbox,
                async->forget ? SpisComplete:NULL,
                timeout);

        spis_status = SPIS_STATUS_SUCCESS;
    }

    return spis_status;
}

/*!
*******************************************************************************

 @Function              @SPISGetResult

 <b>Description:</b>\n
 This function gets the result of an asynchronous operation. It can be
 called after an SPIS transaction has been carried out. The function is
 passed the ::SPIS_PORT_T descriptor for the port it is retrieving the
 result from. It returns a status code describing the way in which the
 transaction completed.

 \param     *port               Pointer to port descriptor.
 \param    **context            Updated with the pointer to the context of
                                the transaction it is retrieving the result from.
 \param      block              Blocking flag. 1 to block, 0 not to block.
 \param      timeout            Timeout to use when blocking.
                                Number of MeOS timer ticks before operation
                                times out. Set to ::SPIS_INF_TIMEOUT for an
                                infinite period of time.

 \return                        This function returns as follows:
                                ::SPIS_STATUS_SUCCESS      Operation completed successfully.
                                ::SPIS_STATUS_CANCEL       Operation was cancelled.
                                ::SPIS_STATUS_TIMEOUT      Operation timed out.

*******************************************************************************/
unsigned long SPISGetResult(SPIS_PORT_T *port, void **context,
                            int block, long timeout)
{
    KRN_MAILBOX_T   *mailbox;
    QIO_IOCB_T      *iocb;
    QIO_DEVICE_T    *dev;
    QIO_STATUS_T    qio_status;
    QIO_IOPARS_T    iopars;
    unsigned long   spis_status;
    SPIS_IO_BLOCK_T *io_block;

	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );

    mailbox = &port->sMailbox;

    //
    // Get the result from the mailbox.
    //
    iocb = QIO_result(mailbox,
                      &dev,
                      &qio_status,
                      &iopars,
                      block ? timeout:0);

    if (iocb == NULL)
        return SPIS_STATUS_WOULD_BLOCK;

	io_block = (SPIS_IO_BLOCK_T *) iopars.spare;
    *context = io_block->callback.context;

    //
    // Return the IOCB to the pool.
    //
    KRN_returnPool(io_block);

    //
    // Check completion status.
    //
    spis_status = SpisFinishInfo(qio_status);

    return spis_status;
}

/*!
*******************************************************************************

 @Function              @SPISCancel

 <b>Description:</b>\n
 This function cancels ALL (active and pending) operations queued on
 the device. The function is passed the ::SPIS_PORT_T descriptor for the
 transaction it is cancelling.

 \param     *port               Pointer to port descriptor.

 \return                        none.

*******************************************************************************/
unsigned long SPISCancel(SPIS_PORT_T *port)
{
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );	
	
    QIO_cancelAll(&port->sDevice);
    return 0;
}

/*!
*******************************************************************************

 @Function              @SPISDeinit

 <b>Description:</b>\n
 Deinitialises SPI Slave
 
*******************************************************************************/
img_void SPISDeinit( SPIS_PORT_T	*	port )
{
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );	
	
	/* Disable QIO device */
	QIO_disable(&port->sDevice);
	
	/* Unload QIO device */
	QIO_unload(&port->sDevice);
	
	// Deinitialise GDMA
	GDMA_Disable( &port->sDMAContext );
	GDMA_Unconfigure( &port->sDMAContext );
	GDMA_Deinitialise( &port->sDMAContext );
	
	/* Mark API as no longer initialised */
	port->bInitialised = IMG_FALSE;
}
