/*!
*******************************************************************************
 @file   sdios_api.h

 @brief  SDIO Slave API

         This file contains the header file information for the SDIO Slave API.

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

#if !defined (__SDIOS_API_H__)
#define __SDIOS_API_H__

/******************************************************************************
**************************** Included header files ****************************
*******************************************************************************/
#include <MeOS.h>
#include "ioblock_defs.h"
#include "gdma_api.h"


/******************************************************************************
********************************** Defines ************************************
*******************************************************************************/
// Maximum number of SDIO blocks that can be supported.
#define MAX_SDIOS_NUM_BLOCKS			4

#define SDIO_TESTING

#define SDIOS_DMA_MAX_CHANNELS  4 // 0 to 3

// Events used in the Register Callback function
#define SDIOS_F0_UPDATE_EVENT                   (1 << 0)
#define SDIOS_F1_UPDATE_EVENT                   (1 << 1)
#define SDIOS_FIFO_OVERFLOW_EVENT               (1 << 2)
#define SDIOS_FIFO_UNDERFLOW_EVENT              (1 << 3)
#define SDIOS_CRC_CHECK_FAILED_EVENT            (1 << 4)
#define SDIOS_TRANSFER_ABORT_EVENT              (1 << 5)
#define SDIOS_TRANSFER_ELEMENT_COMPLETE_EVENT   (1 << 6)
#define SDIOS_BLOCK_COMPLETE_EVENT              (1 << 7)
// - NOTE: This is a bitfield, so additional definitions must always only set a unique bit.

/******************************************************************************
******************************** Enumerations *********************************
*******************************************************************************/

/*!
*******************************************************************************

 This type defines the sdio initialisation function return value.

*******************************************************************************/
typedef enum
{
    /*! Operation completed successfully. */
    SDIOS_STATUS_SUCCESS = 0,

    /*! DMA channel parameter is invalid. */
    SDIOS_INVALID_DMA_CHANNEL,

	/*! Block index is invalid */
	SDIOS_INVALID_BLOCK_INDEX,

    /*! The device is not initialised. */
    SDIOS_ERROR_NOT_INITIALISED,

    /*! The configuration parameters are not valid. */
    SDIOS_ERROR_INVALID_CONFIGURATION,

    /*! Operation was cancelled. */
    SDIOS_STATUS_CANCEL,

    /*! Operation was timed out. */
    SDIOS_STATUS_TIMEOUT,

    /*! Asynchronous I/O has been requested, but the routine would need to block. */
    SDIOS_STATUS_WOULD_BLOCK

} SDIOS_RETURN_T;

typedef enum
{
    SDIOS_TRANSFER_DIR_UNSET = 0,
    SDIOS_TRANSFER_DIR_HOST_TO_CARD,
    SDIOS_TRANSFER_DIR_CARD_TO_HOST

} SDIOS_eTfrDir;

/******************************************************************************
****************************** End enumerations *******************************
*******************************************************************************/


/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/

/*! Infinite wait (do not time out). */
#define SDIOS_INF_TIMEOUT -1

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
typedef struct SDIOS_EVENT_CALLBACK_DATA_T_
{
    unsigned int    uiEvent;
    void*           pData;
} SDIOS_EVENT_CALLBACK_DATA_T;

typedef struct SDIOS_Stats_
{
    unsigned int     TRN_UpdateCount;
    unsigned int     TRN_CompleteCount;
    unsigned int     TRN_AbortCount;
    unsigned int     BLK_CompleteCount;
    unsigned int     DMA_CompleteCount;
    unsigned int     F0_UpdateCount;
    unsigned int     F1_UpdateCount;
    unsigned int     CRC_ErrorCount;
    unsigned int     FIFO_UF_ErrorCount;
    unsigned int     FIFO_OF_ErrorCount;
    unsigned int     DMA_Count_TX;
    unsigned int     DMA_Count_RX;
#if defined SDIOS_ISR_STATS
    SDIOS_Isr_Stats  sIsrStats;
#endif // defined SDIOS_ISR_STATS
}SDIOS_Stats;

typedef struct SDIOS_Internals_
{
    unsigned int    bEnableF1UpdateInterrupts;
    unsigned int    uiRegisteredEventsBitField;
    unsigned int    uiDMAChannel;
    void            (*pfCallback)(SDIOS_EVENT_CALLBACK_DATA_T);
    KRN_SEMAPHORE_T tDataSem;
    unsigned char*  pucNextOperationBufferAddress;
    unsigned int    uiNextOperationDataSizeBytes;
    unsigned int    uiNextOperationDMASizeBytes;
    SDIOS_eTfrDir   eCurrentTransferDirection;
    unsigned int    bDmaTfrCompleted;
    SDIOS_Stats     sStats;

////	unsigned long	ui32BaseAddress;
////	unsigned long	ui32DMAWriteBit;
////	unsigned long	ui32DMAReadBit;
////	ioblock_sBlockContext sBlockContext;

	unsigned int    bBlockDefined;	
	unsigned int    SDIOS_bCallBackFunctionRegistered;
	
	QIO_DEVICE_T *	dev;

    /*! DMAC context structure, IOCB and transfer object for DMA */
    GDMA_sContext			sDMAContext;
    QIO_IOCB_T				sDMAIOCB;
    GDMA_sTransferObject	sDMATransferObject;

}SDIOS_Internals;

/*!
*******************************************************************************

 @brief This structure defines the SDIOS port object.

*******************************************************************************/
typedef struct SDIOS_PORT_T_
{
	/*! Is API initialised? */
	IMG_BOOL				bInitialised;
	
    /*! QIO device. */
    QIO_DEVICE_T			device;

    /*! I/O block's KRN pool */
    KRN_POOL_T				io_block_pool;

    /*! KRN mailbox */
    KRN_MAILBOX_T			mailbox;
    
    /*! Driver internal context structure */
    SDIOS_Internals			sDriverInternals;
    
} SDIOS_PORT_T;


/*!
*******************************************************************************

 @brief This structure contains the initialisation parameters.

*******************************************************************************/
typedef struct SDIOS_INIT_PARAM_T_
{
    /*! DMA channel */
    unsigned int uiDmaChannel;

    /*! Disables SDIO DIR signals */
    unsigned int bDisable_SDIO_DIR_Signals;

	/*! Block index */
	unsigned long	ui32BlockIndex;
	
	/*! Clock edge on which to drive data */
	unsigned char bClockDataOnTrailingEdge;

} SDIOS_INIT_PARAM_T;


/*!
*******************************************************************************

 @brief This structure contains the configuration parameters.

*******************************************************************************/
typedef struct SDIOS_CONFIG_PARAM_T_
{
    unsigned int bEnable_F1_Update_Interrupt;
} SDIOS_CONFIG_PARAM_T;


#if 0 /* Moved up in this file */
/*!
*******************************************************************************

 @brief This structure contains the callback data parameters.

*******************************************************************************/
typedef struct SDIOS_EVENT_CALLBACK_DATA_T_
{
    unsigned int    uiEvent;
    void*           pData;
} SDIOS_EVENT_CALLBACK_DATA_T;

#endif

/*!
*******************************************************************************

 @brief This type defines the Callback Routine used by the SDIOS device driver in the
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
typedef void SDIOS_IO_CALLBACKROUTINE_T(void            *context,
                                        unsigned char   *buffer,
                                        unsigned long   num_bytes_to_transfer,
                                        unsigned long   status);



/*!
*******************************************************************************

 @brief This structure defines the SDIOS Callback type.

*******************************************************************************/
typedef struct SDIOS_IO_CALLBACK_T_
{
    /*! Callback routine. */
    SDIOS_IO_CALLBACKROUTINE_T  *routine;

    /*! Pointer to private context for callback routine. */
    void                        *context;

} SDIOS_IO_CALLBACK_T;



/*!
*******************************************************************************

 @brief This structure defines the SDIOS I/O Block type required for asynchronous
 operation.

*******************************************************************************/
typedef struct SDIOS_IO_BLOCK_T_
{
    /*! MeOS pool linkage structure, allowing this structure to be pooled */
    KRN_POOLLINK;

    /*! MeOS QIO Control Block */
    QIO_IOCB_T      iocb;

    /*! Callback parameters */
    SDIOS_IO_CALLBACK_T callback;

} SDIOS_IO_BLOCK_T;




/*!
*******************************************************************************

 @brief This structure defines the asynchronous I/O descriptor type.

*******************************************************************************/
typedef struct SDIOS_ASYNC_T_
{
    /*! Set to SDIOS_TRUE if the application is going to use the callback method
        of confirming the result.  Set to SDIOS_FALSE if the application
        is going to use the GetResult method.
     */
    int                         iUsingCallback;

    /*! Pointer to SCBS Callback routine. */
    SDIOS_IO_CALLBACKROUTINE_T  *callback_routine;

    /*! Pointer to private context for callback routine. */
    void                        *callback_context;

} SDIOS_ASYNC_T;


/******************************************************************************
**************************** End type definitions *****************************
*******************************************************************************/


/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

/*!
*******************************************************************************

 @Function              @SDIOSDefine

 <b>Description:</b>\n
 This function is used to define the SDIO Slave by providing hardware
 information about the instance of the peripheral. This must be done before
 calling SDIOSInit.

 \param     *psBlockDescriptor  Pointer to a block descriptor.

 \return                        ::SDIOS_STATUS_SUCCESS

*******************************************************************************/
SDIOS_RETURN_T SDIOSDefine(	ioblock_sBlockDescriptor *	psBlockDescriptor );

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

 \return                        ::SDIOS_STATUS_SUCCESS       if initialisation parameters are valid
                                ::SDIOS_INVALID_DMA_CHANNELS if the DMA input and output channels are not unique or are not valid.

*******************************************************************************/
SDIOS_RETURN_T SDIOSInit
(
    SDIOS_PORT_T        *port,
    SDIOS_INIT_PARAM_T  *initParam,
    SDIOS_IO_BLOCK_T    *io_blocks,
    unsigned long        num_io_blocks
);

/*!
*******************************************************************************

 @Function              @SDIOSDeinit
 
 <b>Description:</b>\n
 This function is used to deinitialise the SDIO slave API, and should be used
 as part of a safe system shutdown. 
 
 \return     ::SDIOS_STATUS_SUCCESS              if initialisation parameters are valid
             ::SDIOS_ERROR_NOT_INITIALISED       if the device is not initialised
             ::SDIOS_ERROR_INVALID_CONFIGURATION if any parameters are invalid 
 
*******************************************************************************/ 
SDIOS_RETURN_T SDIOSDeinit
( 
	SDIOS_PORT_T *port 
);

/*!
*******************************************************************************

 @Function              @SDIOSConfigure

 <b>Description:</b>\n
 This function is used to configure the SDIO Slave, which can be done at any time.
 It is only necessary to call this function if the default values are not suitable.

  \param     *ConfigurationParam Pointer to configuration parameters.

 \return     ::SDIOS_STATUS_SUCCESS              if initialisation parameters are valid
             ::SDIOS_ERROR_NOT_INITIALISED       if the device is not initialised
             ::SDIOS_ERROR_INVALID_CONFIGURATION if any parameters are invalid

*******************************************************************************/
SDIOS_RETURN_T SDIOSConfigure
(
	SDIOS_PORT_T			*	psPort,
    SDIOS_CONFIG_PARAM_T    *	ConfigurationParam
);

/*!
*******************************************************************************

 @Function              @SDIOSRegisterCallback

 <b>Description:</b>\n
 This function is used to register a callback function with the SDIO Slave.  This can
 only be done once the driver has  been initialised.  The Callback will be removed by
 any subsequent calls to SDIOSInit.  Calls to SDIOSConfigure do not affect the
 registered callback.

 \param     IMG_VOID (*pfCallback)(SDIOS_EVENT_CALLBACK_DATA_T) A pointer to the callback function to be registered.
 \param     ui32EventsField     Events bitfield that represents the events that will trigger the callback.
 \return    ::SDIOS_STATUS_SUCCESS              if initialisation parameters are valid
            ::SDIOS_ERROR_NOT_INITIALISED       if the device is not initialised
            ::SDIOS_ERROR_INVALID_CONFIGURATION if any parameters are invalid

*******************************************************************************/
SDIOS_RETURN_T SDIOSRegisterCallback
(
	SDIOS_PORT_T        *port,
    void (*pfCallback)(SDIOS_EVENT_CALLBACK_DATA_T),
    const unsigned int uiEventsField
);


/*!
*******************************************************************************

 @Function              @SDIOSRead

 <b>Description:</b>\n
 This function is used to perform a 'read from master' transaction. When the
 function is called, the SDIOS is made ready to receive data.

 A DMA transfer is used to transfer data from the peripheral to memory.

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
SDIOS_RETURN_T SDIOSRead
(
    SDIOS_PORT_T   *port,
    unsigned char  *buf,
    unsigned long  dmaLength,
    SDIOS_ASYNC_T  *async,
    long           timeout
);


/*!
*******************************************************************************

 @Function              @SDIOSWrite

 <b>Description:</b>\n
 This function is used to perform a 'write to master' transaction. When the
 function is called, the SDIOS is made ready to transmit data. The transaction
 from the peripheral to the outside world is controlled by the master.

 A DMA transfer is used to transfer data from the peripheral to memory.

 \param     *port               Pointer to port descriptor.
 \param     *buf                Buffer of data for transmission
 \param      dmaLength          Number of bytes to be transmitted.
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

******************************************************************************/
SDIOS_RETURN_T SDIOSWrite
(
    SDIOS_PORT_T   *port,
    unsigned char  *buf,
    unsigned long   dmaLength,
    SDIOS_ASYNC_T  *async,
    long timeout
);


/*!
*******************************************************************************

 @Function              @SDIOSGetResult

 <b>Description:</b>\n
 This function gets the result of an asynchronous operation. It can be
 called after an SDIOS transaction has been carried out. The function is
 passed the ::SDIOS_PORT_T descriptor for the port it is retrieving the
 result from. It returns a status code describing the way in which the
 transaction completed.

 \param     *port               Pointer to port descriptor.
 \param    **context            Updated with the pointer to the context of
                                the transaction it is retrieving the result from.
 \param      block              Blocking flag. 1 to block, 0 not to block.
 \param      timeout            Timeout to use when blocking.
                                Number of MeOS timer ticks before operation
                                times out. Set to ::SDIOS_INF_TIMEOUT for an
                                infinite period of time.

 \return                        This function returns as follows:
                                ::SDIOS_STATUS_SUCCESS      Operation completed successfully.
                                ::SDIOS_STATUS_CANCEL       Operation was cancelled.
                                ::SDIOS_STATUS_TIMEOUT      Operation timed out.

*******************************************************************************/
unsigned long SDIOSGetResult
(
    SDIOS_PORT_T  *port,
    void          **context,
    int           block,
    long          timeout
);



/*!
*******************************************************************************

 @Function              @SDIOSCancel

 <b>Description:</b>\n
 This function cancels ALL (active and pending) operations queued on
 the device. The function is passed the ::SDIOS_PORT_T descriptor for the
 transaction it is cancelling.

 \param     *port               Pointer to port descriptor.

 \return                        none.

*******************************************************************************/
void SDIOSCancel
(
    SDIOS_PORT_T    *port
);

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
void SDIOSInterruptHost
(
    SDIOS_PORT_T    *port
);

extern ioblock_sBlockDescriptor	IMG_asSDIOSBlock[];


/******************************************************************************
******************************** End Functions ********************************
*******************************************************************************/

#endif /* __SDIOS_API_H__ */
