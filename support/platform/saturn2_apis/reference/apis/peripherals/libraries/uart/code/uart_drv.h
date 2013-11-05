/*!
*******************************************************************************
 @file   uart_drv.h

 @brief  UART Device Driver header

         This file contains the header file information for the UART device driver.

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

#if !defined (__UART_DRV_H__)
#define __UART_DRV_H__

/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/

/*! UARTConfig() return value - port configured successfully. */
#define SER_OK              (0)
/*! UARTConfig() return value - unable to configure port. */
#define SER_ERR_PORT	    (1)

/*! Tx FIFO Depth */
#define TX_FIFO_DEPTH       (16)
/*! Rx FIFO Depth */
#define RX_FIFO_DEPTH       (1)


/******************************************************************************
************************** End constant definitions ***************************
*******************************************************************************/


/******************************************************************************
****************************** Type definitions *******************************
*******************************************************************************/

/*!
*******************************************************************************

 This structure contains the parameter block used to initialise device parameters.

******************************************************************************/
typedef struct
{
	/*! Baud rate divisor (options in uart_reg.h) */
    unsigned short divisor;

    /*! Word format (options in uart_reg.h) */
    unsigned char wordFormat;

    /*! RX FIFO trigger level (see uart_reg.h) */
    unsigned char rxTrigThreshold;

    /*! Enable CTS/RTS flow control (1 = CTS/RTS enabled) */
    unsigned char flowControl;

} UART_PARA_TYPE;

/******************************************************************************
**************************** End type definitions *****************************
*******************************************************************************/


/******************************************************************************
**************************** Export Device Driver *****************************
*******************************************************************************/

/*! UART TX device driver structure. */
extern const QIO_DRIVER_T STX_driver;
/*! UART RX device driver structure. */
extern const QIO_DRIVER_T SRX_driver;

/******************************************************************************
************************** End Export Device Driver ***************************
*******************************************************************************/

#define MAX_NUM_UART_BLOCKS			(8)
extern ioblock_sBlockDescriptor	*	g_apsUARTBlock[ MAX_NUM_UART_BLOCKS ];


/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

/*!
*******************************************************************************

 @Function              @UARTConfig

 <b>Description:</b>\n
 This function configures the UART port. It is passed a set of parameters, as
 contained within a ::UART_PARA_TYPE structure. It uses these to set the
 equivalent parameters in the static 'state' variable.

 \param     *port               Pointer to port descriptor.
 \param     *parameters         Parameters to configure.

 \return                        This function returns as follows:\n
                                ::SER_OK        Port configured successfully.\n
                                ::SER_ERR_PORT  Unable to configure port.\n

*******************************************************************************/
unsigned long UARTConfig
(
    unsigned char port,
    UART_PARA_TYPE *parameters
);

/*!
*******************************************************************************

 @Function              @UARTResetError

 <b>Description:</b>\n
 This function resets the driver error flag and flushes the hardware receiver FIFO.

 \param     *port               Pointer to port descriptor.

 \return                        This function returns as follows:\n
                                ::SER_OK        Error resets successfully.\n

*******************************************************************************/
unsigned long UARTResetError
(
	unsigned char port
);


/******************************************************************************
******************************** End Functions ********************************
*******************************************************************************/

#endif /* __UART_DRV_H__ */
