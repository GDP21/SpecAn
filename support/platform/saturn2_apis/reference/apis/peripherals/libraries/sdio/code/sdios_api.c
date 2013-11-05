/*!
*******************************************************************************
  file   sdios_api.c

  brief  SDIOS API

         This file defines the functions that make up the SDIO Slave API.

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

 \n<b>Platform:</b>\n


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

/* SDIO Slave Driver */
#include "sdios_api.h"
#include "sdios_drv.h"
#include "assert.h"

#if defined USE_EVENT_LOGGING
#include "sdios_eventlogging.h"
#endif /* USE_EVENT_LOGGING */

/* -------------------------- MACRO DEFINITIONS -------------------------- */

/* infinite wait on QIO IO */
#define INFWAIT (-1)


/* -------------------------- EXPORTED FUNCTIONS ------------------------- */


/* ----------------------- LOCAL FUNCTION PROTOTYPES---------------------- */
static unsigned long prv_SDIOS_TranslateReturnCode(QIO_STATUS_T  qio_status);
static int           SdiosComplete                (QIO_DEVICE_T  *dev,
                                                   QIO_IOCB_T    *iocb,
                                                   QIO_IOPARS_T  *iopars,
                                                   QIO_STATUS_T  qio_status);

/* -------------------------- LOCAL DATA --------------------------------- */

static int						gDriverInitialised = 0;
ioblock_sBlockDescriptor *		apsSDIOS_Descriptor[ MAX_SDIOS_NUM_BLOCKS ];


/* -------------------------- STATIC FUNCTIONS --------------------------- */

/*!
******************************************************************************

 @Function              SdiosComplete

 @Description			Completion routine.

 						Standard QIO completion routine arguments and return.

******************************************************************************/
static int SdiosComplete(
    QIO_DEVICE_T    *dev,
    QIO_IOCB_T      *iocb,
    QIO_IOPARS_T    *iopars,
    QIO_STATUS_T    qio_status
    )
{
    SDIOS_IO_BLOCK_T    *io_block;
    unsigned long       sdios_status;

	(void)dev;
	(void)iocb;

    io_block = (SDIOS_IO_BLOCK_T *) iopars->spare;

    /* Check status. */
    sdios_status = prv_SDIOS_TranslateReturnCode(qio_status);

    /* Call the callback routine. */
    if (io_block->callback.routine != NULL)
    {
        (*io_block->callback.routine)(io_block->callback.context, iopars->pointer,
                                      iopars->counter, sdios_status);
    }

    KRN_returnPool(io_block);

    /* Never deliver to the mailbox. */
    return SDIOS_TRUE;
}

/*!
******************************************************************************

 @Function              SdiosComplete

 @Description			Internal define function.

******************************************************************************/
static IMG_VOID sdiosDefine( ioblock_sBlockDescriptor	*	psBlockDescriptor	)
{
	/* Do once-off driver initialisation if not yet done */
	if ( gDriverInitialised == 0 )
	{
		int		i;

		for (i = 0; i < MAX_SDIOS_NUM_BLOCKS; i++)
		{
			apsSDIOS_Descriptor[i] = 0;
		}

		gDriverInitialised = 1;
	}

	/* Sanity checks */
	IMG_ASSERT ( psBlockDescriptor->ui32Index < MAX_SDIOS_NUM_BLOCKS );
	IMG_ASSERT ( apsSDIOS_Descriptor[ psBlockDescriptor->ui32Index ] == 0 );

	/* Save i/o block descriptor for the specified device instance */
	apsSDIOS_Descriptor[ psBlockDescriptor->ui32Index ] = psBlockDescriptor;

	/* Ensure the context pointer in the i/o block descriptor is null */
	psBlockDescriptor->pvAPIContext = 0;
}


/*!
*******************************************************************************

 @Function              @SDIOSDefine

 <b>Description:</b>\n
 This function is used to define the h/w parameters of an instance of the SDIO Slave. This must be done before the
 device is initialised using SDIOSInit.

 \param     *psBlockDescriptor  Pointer to i/o block descriptor.

 \return                        ::SDIOS_STATUS_SUCCESS if parameters are valid
                                ::SDIOS_INVALID_<type> if <type> parameter is not valid

*******************************************************************************/
SDIOS_RETURN_T SDIOSDefine(ioblock_sBlockDescriptor *	psBlockDescriptor )
{
	// This function is now deprecated, but is left here for backward compatibility.
	return SDIOS_STATUS_SUCCESS;
}


/*!
*******************************************************************************

 @Function              @SDIOSInit

 <b>Description:</b>\n
 This function is used to initialise the SDIO Slave. This must be done before a
 transaction is carried out using SDIOSRead() or SDIOSWrite(). It is not
 necessary to re-initialise the SDIOS before further transactions unless any of
 the parameters set up at initialisation are to be changed.

 The function initialises the port by allocating a QIO device object to the
 SDIOS port object defined by the structure SDIOS_PORT_T.

 \param     *port               Pointer to port descriptor.
 \param     *initParam          Pointer to initialisation parameters.
 \param     *io_blocks          I/O blocks (required for asynchronous I/O).
 \param      num_io_blocks      Number of I/O blocks.

 \return                        ::SDIOS_STATUS_SUCCESS if initialisation parameters are valid
                                ::SDIOS_INVALID_<type> if <type> parameter is not valid

*******************************************************************************/
SDIOS_RETURN_T SDIOSInit( SDIOS_PORT_T              *port,
                          SDIOS_INIT_PARAM_T        *initParam,
                          SDIOS_IO_BLOCK_T          *io_blocks,
                          unsigned long              num_io_blocks)
{
	SDIOS_Internals *			psInternalContext;
	IMG_RESULT					rResult = IMG_SUCCESS;
	ioblock_sBlockDescriptor *	psBlockDescriptor;
	GDMA_sCallbackFunctions		sSDIOSCallbackFunctions;

	/*
		It is incorrect to initialise the same piece of hardware more than once.  If this instance
		has not yet been initialised then the pvAPIContext member of the i/o block descriptor for
		this instance (passed in via the SDIOSDefine function call for this instance of the h/w -
		which MUST have been done prior to this call to SDIOSInit) will be NULL.
	*/
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_FALSE );

	/* Clear port structure */
	IMG_MEMSET ( port, 0, sizeof(SDIOS_PORT_T) );

	/* Sanity checks */
	IMG_ASSERT ( initParam->ui32BlockIndex < MAX_SDIOS_NUM_BLOCKS );

	if ( !apsSDIOS_Descriptor[ initParam->ui32BlockIndex ] )
	{
		sdiosDefine( &IMG_asSDIOSBlock[ initParam->ui32BlockIndex ] );
	}

	/* Get local pointer to I/O block structure */
	psBlockDescriptor = apsSDIOS_Descriptor[ initParam->ui32BlockIndex ];

	/* Save context structure pointer in i/o block descriptor */
	psBlockDescriptor->pvAPIContext = (IMG_VOID *)port;

	/* Get access to the "internals" part of the context structure */
	psInternalContext = &(port->sDriverInternals);

    /* Reset the callback registered flag */
    psInternalContext->SDIOS_bCallBackFunctionRegistered = SDIOS_FALSE;

    /* Initialise mailbox */
    KRN_initMbox(&port->mailbox);

    /* Initialise the IOCB pool. */
    if (io_blocks != NULL)
    {
        KRN_initPool(&port->io_block_pool,
                     io_blocks,
                     num_io_blocks,
                     sizeof(SDIOS_IO_BLOCK_T));
    }

	// Set up GDMA

	/* Set up callback functions structure */
	sSDIOSCallbackFunctions.pfnInitDevice		= IMG_NULL;
	sSDIOSCallbackFunctions.pfnStartDevice		= IMG_NULL;
	sSDIOSCallbackFunctions.pfnCancelDevice		= IMG_NULL;
	sSDIOSCallbackFunctions.pfnCompletion		= &SDIOS_DMAComplete;

	rResult = GDMA_Initialise(	&(psInternalContext->sDMAContext),
								psInternalContext->uiDMAChannel,
								SDIOS_DMAC_CHANNEL_GROUP,
								IMG_FALSE,						/* Not linked list mode								*/
								&sSDIOSCallbackFunctions,		/* DMA callback functions structure					*/
								psBlockDescriptor,				/* Use i/o block descriptor as callback parameter	*/
								IMG_NULL,						/* No linked list elements							*/
								0,								/* No linked list elements							*/
								IMG_TRUE,						/* Disable autostart (not applicable to non-QIO)	*/
								0,								/* No start list threshold							*/
								IMG_NULL,						/* No IOCB needed as not using QIO					*/
								0,								/* No IOCBs											*/
								IMG_TRUE						/* Bypass QIO										*/
							 );

	rResult = GDMA_Configure(	&(psInternalContext->sDMAContext) );

    /* Initialise the device */
    /* pass pointer to parameters via the id field */
    QIO_init(&port->device, "SDIO Slave", (unsigned int)initParam, &SDIOS_driver);

	/* Use device block number as teh device "id" */
	port->device.id = initParam->ui32BlockIndex;

    /* Enable the device */
    QIO_enable(&port->device);

	/* Flag device as now initialised */
    port->bInitialised = IMG_TRUE;

    return SDIOS_STATUS_SUCCESS;
}

/*!
*******************************************************************************

 @Function              @SDIOSDeinit

*******************************************************************************/
SDIOS_RETURN_T SDIOSDeinit( SDIOS_PORT_T *port )
{
	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );

    /* Disable the device */
    QIO_disable(&port->device);

    /* Unload the device */
    QIO_unload(&port->device);

    GDMA_Disable ( &(port->sDriverInternals.sDMAContext) );
    GDMA_Unconfigure ( &(port->sDriverInternals.sDMAContext) );
    GDMA_Deinitialise ( &(port->sDriverInternals.sDMAContext) );

	/* API is no longer initialised */
	port->bInitialised = IMG_FALSE;

	return SDIOS_STATUS_SUCCESS;
}

/*!
*******************************************************************************

 @Function              @SDIOSConfigure

 <b>Description:</b>\n
 This function is used to configure the SDIO Slave, which can be done at any time.
 It is only necessary to call this function if the default values are not suitable.

 \param     *ConfigurationParam Pointer to initialisation parameters.

 \return                        ::SDIOS_STATUS_SUCCESS              if initialisation parameters are valid
                                ::SDIOS_ERROR_NOT_INITIALISED       if the device is not initialised
                                ::SDIOS_ERROR_INVALID_CONFIGURATION if any parameters are invalid

*******************************************************************************/

SDIOS_RETURN_T SDIOSConfigure(	SDIOS_PORT_T			*	psPort,
								SDIOS_CONFIG_PARAM_T	*	ConfigurationParam)
{
	ioblock_sBlockDescriptor	*	psBlockDescriptor;

    /* Only allow a configure when the device is initialised. */
	IMG_ASSERT ( psPort != IMG_NULL );
	IMG_ASSERT ( psPort->bInitialised == IMG_TRUE );

	/* Get local pointer to I/O block structure */
	psBlockDescriptor = apsSDIOS_Descriptor[ psPort->device.id ];

	SDIOS_EnableF1UpdateInterrupts( ConfigurationParam->bEnable_F1_Update_Interrupt,
									psBlockDescriptor );

    return SDIOS_STATUS_SUCCESS;
}

/*!
*******************************************************************************

 @Function              @SDIOSRegisterCallback

 <b>Description:</b>\n
 This function is used to register a callback function with the SDIO Slave.  This can only be done once the driver has
 been initialised.  The Callback will be removed by any subsequent calls to SDIOSInit.  Calls to SDIOSConfigure do
 not affect the  registered callback.

 \param     void (*pfCallback)(SDIOS_EVENT_CALLBACK_DATA_T)   A pointer to the callback function to be registered.

 \return    ::SDIOS_STATUS_SUCCESS              if initialisation parameters are valid
            ::SDIOS_ERROR_NOT_INITIALISED       if the device is not initialised
            ::SDIOS_ERROR_INVALID_CONFIGURATION if any parameters are invalid

*******************************************************************************/
SDIOS_RETURN_T SDIOSRegisterCallback(	SDIOS_PORT_T        *port,
										void (*pfCallback)(SDIOS_EVENT_CALLBACK_DATA_T),
										const unsigned int uiEventsField)
{
	ioblock_sBlockDescriptor *	psBlockDescriptor;
//	SDIOS_Internals			 *	psInternals = &port->sDriverInternals;

	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );

	/* Get local pointer to I/O block structure */
	psBlockDescriptor = apsSDIOS_Descriptor[ port->device.id ];

	/* Sanity checks */
    IMG_ASSERT(pfCallback);
    IMG_ASSERT( port->sDriverInternals.SDIOS_bCallBackFunctionRegistered == 0);
       
	/* Configure the event callbacks */
	SDIOS_ConfigureEventCallbacks(uiEventsField, psBlockDescriptor );

	/* Register the callback in the driver */
	SDIOS_AddCallback(pfCallback, psBlockDescriptor );

	return SDIOS_STATUS_SUCCESS;
}


/*!
*******************************************************************************

 @Function              @SDIOSRead

 <b>Description:</b>\n
 This function is used to perform a 'read from master' transaction. When the
 function is called, the SDIOS is made ready to receive data.

 A DMA transfer is used to transfer data from the peripheral to memory.
 The SDIOS stores data in incrementing memory locations, starting at the
 address specified in the 'buf' parameter.

 \param     *port               Pointer to port descriptor.
 \param     *buf                Buffer for received data
 \param      dmaLength          Number of bytes to be read in.
 \param     *async              Pointer to asynchronous I/O descriptor if a
                                non-blocking transfer is required (else NULL).
 \param      timeout            Number of MeOS timer ticks before operation
                                times out. Set to ::SDIOS_INF_TIMEOUT for an
                                infinite period of time. The timer starts when
                                QIO start function is called. This parameter has
                                the same implication for blocking and non-blocking
                                transfers.

\return                        This function returns as follows:
                                ::SDIOS_STATUS_SUCCESS      Operation completed successfully.
                                ::SDIOS_STATUS_CANCEL       Operation was cancelled.
                                ::SDIOS_STATUS_TIMEOUT      Operation timed out.
*******************************************************************************/
SDIOS_RETURN_T SDIOSRead(SDIOS_PORT_T *port, unsigned char *buf, unsigned long dmaLength, SDIOS_ASYNC_T *async, long timeout)
{
    QIO_DEVICE_T    *device;
	QIO_IOPARS_T    iopars;
    unsigned long   sdios_status;

	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );

    device = &port->device;

    iopars.opcode  = SDIOS_OPCODE_READ;
	iopars.pointer = (void *)buf;
	iopars.counter = dmaLength;

    if(!async)
    {
        /* =====================================*/
        /* ===== This is a synchronous I/O =====*/
        /* =====================================*/
        QIO_STATUS_T  qio_status;

        /* This is longwinded for ease of debugging - the compiler will optimise out the temporary variables. */
        qio_status   = QIO_qioWait(device, &iopars, timeout);
        sdios_status = prv_SDIOS_TranslateReturnCode(qio_status);

    }
    else
    {
        /* =====================================*/
        /* ==== This is an Asynchronous I/O ====*/
        /* =====================================*/
        SDIOS_IO_BLOCK_T  *io_block;

        io_block = KRN_takePool(&port->io_block_pool, 0);

        if (io_block == NULL)
        {
            /* We have run out of control blocks, so cannot queue this I/O */
            sdios_status = SDIOS_STATUS_WOULD_BLOCK;
        }
        else
        {
            io_block->callback.routine = async->callback_routine;
            io_block->callback.context = async->callback_context;

            iopars.spare = io_block;

            QIO_qio(device,
                    &io_block->iocb,
                    &iopars, &port->mailbox,
                    async->iUsingCallback ? SdiosComplete:NULL,
                    timeout);

            sdios_status = SDIOS_STATUS_SUCCESS;
        }
    }

    return sdios_status;
}


/*!
*******************************************************************************

 @Function              @SDIOSWrite

 <b>Description:</b>\n
 This function is used to perform a 'write to master' transaction. When the
 function is called, the SDIOS is made ready to transmit data.

 A DMA transfer is used to transfer data from memory to the peripheral.
 The function will only complete when 'dmaLength' bytes has been transferred
 from the system memory to the peripheral. Once it has transferred this number
 of bytes, the function will complete, even if the master has not ended the transaction.
 If the SDIOS attempts to write more data than the master has requested,
 then the transaction will not complete until the master has requested enough
 further data from the slave such that the entire slave transaction has been
 transferred from the system memory to the peripheral.

 \param     *port               Pointer to port descriptor.
 \param     *buf                Buffer of data for transmission
 \param      dmaLength          Number of bytes to be transmitted.
 \param     *async              Pointer to asynchronous I/O descriptor if a
                                non-blocking transfer is required (else NULL).
 \param      timeout            Number of MeOS timer ticks before operation
                                times out. Set to SDIOS_INF_TIMEOUT for an
                                infinite period of time. The timer starts when
                                QIO start function is called. This parameter has
                                the same implication for blocking and non-blocking
                                transfers.

 \return                        This function returns as follows:
                                ::SDIOS_STATUS_SUCCESS      Operation completed successfully.
                                ::SDIOS_STATUS_CANCEL       Operation was cancelled.
                                ::SDIOS_STATUS_TIMEOUT      Operation timed out.
******************************************************************************/
SDIOS_RETURN_T SDIOSWrite      (SDIOS_PORT_T   *port,
                                unsigned char  *buf,
                                unsigned long  dmaLength,
                                SDIOS_ASYNC_T  *async,
                                long           timeout)
{
    QIO_DEVICE_T     *device;
	QIO_IOPARS_T     iopars;
	unsigned long    sdios_status;

	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );

    device = &port->device;

	iopars.opcode  = SDIOS_OPCODE_WRITE;
    iopars.pointer = (void *)buf;
	iopars.counter = dmaLength;

    if(!async)
    {
        /* =====================================*/
        /* ===== This is a synchronous I/O =====*/
        /* =====================================*/
        QIO_STATUS_T  qio_status;

        /* The compiler will optimise out the temporary variables. */
        qio_status   = QIO_qioWait(device, &iopars, timeout);
        sdios_status = prv_SDIOS_TranslateReturnCode(qio_status);
    }
    else
    {
        /* =====================================*/
        /* ==== This is an Asynchronous I/O ====*/
        /* =====================================*/
        SDIOS_IO_BLOCK_T  *io_block;

        io_block = KRN_takePool(&port->io_block_pool, 0);

        if (io_block == NULL)
        {
            /* We have run out of control blocks, so cannot queue this I/O */
            sdios_status = SDIOS_STATUS_WOULD_BLOCK;
        }
        else
        {
            sdios_status = SDIOS_STATUS_SUCCESS;
            io_block->callback.routine = async->callback_routine;
            io_block->callback.context = async->callback_context;

            iopars.spare = io_block;

            QIO_qio(device,
                    &io_block->iocb,
                    &iopars, &port->mailbox,
                    async->iUsingCallback ? SdiosComplete:NULL,
                    timeout);

            sdios_status = SDIOS_STATUS_SUCCESS;
        }
    }

    return sdios_status;
}


/*!
*******************************************************************************

 @Function              @SDIOSGetResult

 <b>Description:</b>\n
 This function gets the result of an asynchronous operation. It can be
 called after an SDIOS transaction has been carried out. The function is
 passed the SDIOS_PORT_T descriptor for the port it is retrieving the
 result from. It returns a status code describing the way in which the
 transaction completed.

 \param     *port               Pointer to port descriptor.
 \param    **context            Updated with the pointer to the context of
                                the transaction it is retrieving the result from.
 \param      block              Blocking flag. 1 to block, 0 not to block.
 \param      timeout            Timeout to use when blocking.
                                Number of MeOS timer ticks before operation
                                times out. Set to SDIOS_INF_TIMEOUT for an
                                infinite period of time.

 \return                        This function returns as follows:
                                ::SDIOS_STATUS_SUCCESS      Operation completed successfully.
                                ::SDIOS_STATUS_CANCEL       Operation was cancelled.
                                ::SDIOS_STATUS_TIMEOUT      Operation timed out.

*******************************************************************************/
unsigned long SDIOSGetResult(SDIOS_PORT_T *port, void **context,
                            int block, long timeout)
{
    KRN_MAILBOX_T       *mailbox;
    QIO_IOCB_T          *iocb;
    QIO_DEVICE_T        *dev;
    QIO_STATUS_T        qio_status;
    QIO_IOPARS_T        iopars;
    unsigned long       sdios_status;
    SDIOS_IO_BLOCK_T    *io_block;

	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );

    mailbox = &port->mailbox;

    /* Get the result from the mailbox. */
    iocb = QIO_result(mailbox,
                      &dev,
                      &qio_status,
                      &iopars,
                      block ? timeout:0);

    if (iocb == NULL)
    {
        /* Early Exit */
        return SDIOS_STATUS_WOULD_BLOCK;
    }

	io_block = (SDIOS_IO_BLOCK_T *) iopars.spare;
    *context = io_block->callback.context;

    /* Return the IOCB to the pool. */
    KRN_returnPool(io_block);

    /* Check completion status. */
    sdios_status = prv_SDIOS_TranslateReturnCode(qio_status);

    return sdios_status;
}


/*!
*******************************************************************************

 @Function              @SDIOSInterruptHost

 <b>Description:</b>\n
 This function causes the host to be interrupted.  If in 4 bit mode there may
 be a delay before the interrupt is recognised.  This is because the host will
 only sample the interrupt during the "interrupt period". (See section 8 of the
 SDIO Specification V1.10)  This is due to the physical limitation that pin 8
 is shared between the IRQ and Data Line 1.

 \param     *port               Pointer to port descriptor.

 \return                        none.

*******************************************************************************/
void SDIOSInterruptHost(SDIOS_PORT_T    *port)
{
	ioblock_sBlockDescriptor *	psIOBlockDescriptor;

	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );

	psIOBlockDescriptor = apsSDIOS_Descriptor[ port->device.id ];

        /* This is not currently queued, but it could be a future enhancement. */
	SDIOS_SetHostInterruptFlag( psIOBlockDescriptor );
}


/*!
*******************************************************************************

 @Function              @SDIOSCancel

 <b>Description:</b>\n
 This function cancels ALL (active and pending) operations queued on
 the device. The function is passed the SDIOS_PORT_T descriptor for the
 transaction it is cancelling.

 \param     *port               Pointer to port descriptor.

 \return                        none.

*******************************************************************************/
void SDIOSCancel(SDIOS_PORT_T *port)
{
	ioblock_sBlockDescriptor *	psIOBlockDescriptor;

	IMG_ASSERT ( port != IMG_NULL );
	IMG_ASSERT ( port->bInitialised == IMG_TRUE );

	psIOBlockDescriptor = apsSDIOS_Descriptor[ port->device.id ];

    QIO_cancelAll(&port->device);
    SDIOS_CancelAll( psIOBlockDescriptor );
}


/****************************************************************************************/
/* Private Functions */
/****************************************************************************************/
static unsigned long prv_SDIOS_TranslateReturnCode(QIO_STATUS_T  qio_status)
{
    unsigned long sdios_status;

    if      (qio_status == QIO_NORMAL)  {sdios_status = SDIOS_STATUS_SUCCESS;}
    else if (qio_status == QIO_CANCEL)  {sdios_status = SDIOS_STATUS_CANCEL; }
    else if (qio_status == QIO_TIMEOUT) {sdios_status = SDIOS_STATUS_TIMEOUT;}
    else                                {sdios_status = SDIOS_STATUS_SUCCESS;}

    return sdios_status;
}
