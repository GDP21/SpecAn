/*!
*******************************************************************************
 @file   sdioS_drv.h

 @brief  SDIOS Device Driver

         This file contains the header file information for the SDIO slave device driver.

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

 \n<b>Platform:</b>\n


*******************************************************************************/

#if !defined (__SDIOS_DRV_H__)
#define __SDIOS_DRV_H__


/* ------------------------------------------------------------------------ */
/*                                                                          */
/*                         COMPILE OPTIONS                                  */
/*                                                                          */
/* ------------------------------------------------------------------------ */
//#define USE_EVENT_LOGGING         // Comment IN to enable event logging
//#define ENABLE_SDIOS_DBG_COMMENTS // Comment IN to enable sdios driver debug comments.
//#define SDIOS_ISR_STATS           // Comment IN to enable ISR statistics
//#define SDIO_EDGE_TRIGGERED       // Comment OUT for level triggered
//#define SDIOS_UNDERFLOW_TESTING   // Comment IN to enable sdios underflow testing
//#define SDIOS_TESTING             // Comment OUT to reduce code size when not testing


/******************************************************************************
******************************** Enumerations *********************************
*******************************************************************************/

/*!
*******************************************************************************

 These enumerations define the supported SDIO driver methods.

*******************************************************************************/
enum SDIOS_OPS
{
    /*! SDIO driver method: Write */
    SDIOS_OPCODE_WRITE = 0,

    /*! SDIO driver method: Read */
    SDIOS_OPCODE_READ
};

void    SDIOS_SetHostInterruptFlag      ( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
void    SDIOS_EnableF1UpdateInterrupts  (unsigned int, ioblock_sBlockDescriptor *	psIOBlockDescriptor);
void    SDIOS_ConfigureEventCallbacks   (const unsigned int, ioblock_sBlockDescriptor *	psIOBlockDescriptor);
void    SDIOS_AddCallback               (void(*pfCallback)(SDIOS_EVENT_CALLBACK_DATA_T), ioblock_sBlockDescriptor *	psIOBlockDescriptor);



#if defined SDIOS_TESTING
    unsigned int SDIOS_TESTAPI_TestWriteReg      (void);
#endif /* SDIOS_TESTING */


/* Definitions to improve readability */
#ifndef SDIOS_TRUE
#define SDIOS_TRUE  (1)
#endif

#ifndef SDIOS_FALSE
#define SDIOS_FALSE (0)
#endif

/* Register Access Macros */
    #define  Read(baseAddr, offset, dataPtr)  (*dataPtr = (*(volatile unsigned long *)((baseAddr + offset))))
    #define Write(baseAddr, offset, data)     (*(volatile unsigned long *)(baseAddr + offset) = (data))

/* Debug Output Macros */
#if defined ENABLE_SDIOS_DBG_COMMENTS
    #define SDIO_DBG_COMMENT(memspace, comment)   __TBILogF("%s \n", (comment))
#else

    #define SDIO_DBG_COMMENT(memspace, comment)

#endif /* ENABLE_SDIOS_DBG_COMMENTS */


#define SDIOS_RX_FIFO_READ_WIDTH		4
#define SDIOS_MEMORY_WRITE_WIDTH		4

#define SDIOS_TX_FIFO_WRITE_WIDTH		4
#define SDIOS_MEMORY_READ_WIDTH			4

#define SDIOS_DMA_READ_HOLD_OFF			0xF
#define SDIOS_DMA_READ_WAIT_UNPACK		1

#define SDIOS_DMAC_CHANNEL_GROUP		0

/*!
*******************************************************************************

 This structure contains the parameters that describe an SDIO read/write.

*******************************************************************************/
typedef struct
{
    /*! The number of bytes to read/write. */
    unsigned long dmaLength;

    /*! The buffer to read from or write to. */
    unsigned char *buf;

} SDIOS_RW_PARAM_T;


/******************************************************************************
**************************** Export Device Driver *****************************
*******************************************************************************/

/*! SDIO device driver structure */
extern const QIO_DRIVER_T SDIOS_driver;

/******************************************************************************
************************** End Export Device Driver ***************************
*******************************************************************************/

/******************************************************************************
**************************** Function declarations ****************************
*******************************************************************************/
void SDIOS_CancelAll( ioblock_sBlockDescriptor *	psIOBlockDescriptor );
IMG_RESULT SDIOS_DMAComplete( GDMA_sTransferObject	*	psTransfer, QIO_STATUS_T	eQIOStatus, IMG_VOID * pvParam );
/******************************************************************************
************************** End Function declarations **************************
*******************************************************************************/


#endif /* __SDIOS_DRV_H__ */
