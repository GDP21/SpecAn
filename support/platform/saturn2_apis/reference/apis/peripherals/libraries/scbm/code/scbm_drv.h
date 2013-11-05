/*!
*******************************************************************************
 @file   scbm_drv.h

 @brief  Serial Control Bus Master Device Driver

         This file contains the header file information for the
         Serial Control Bus Master (SCBM) device driver.

 @author Imagination Technologies

         <b>Copyright 2005 by Imagination Technologies Limited.</b>\n
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

*******************************************************************************/

#if !defined (__SCBM_DRV_H__)
#define __SCBM_DRV_H__

/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/

/*! SCBM Method: Read */
#define SCBM_DD_READ  0
/*! SCBM Method: Write */
#define SCBM_DD_WRITE 1

/*!
*******************************************************************************

 Driver opcode.

 This is laid out as follows:

 Bits 0 to 10   are the bus address.
 Bits 11 to 31  are the driver method.

*******************************************************************************/

/*! Helper macro for SCBM driver opcode.\n
    Macro is passed the method and address and calculates the resulting opcode.*/
#define SCBM_OPCODE(method, address) ((address) | ((method) << 11))

/*! Helper macro for SCBM driver opcode.
    Macro is passed the opcode and extracts the address part of it. */
#define SCBM_ADDRESS(opcode) ((opcode) & 0x03ff)

/*! Helper macro for SCBM driver opcode.
    Macro is passed the opcode and extracts the method part of it. */
#define SCBM_METHOD(opcode)  (((opcode) >> 11) & 0x0fffff)


/******************************************************************************
************************** End constant definitions ***************************
*******************************************************************************/


/******************************************************************************
**************************** Export Device Driver *****************************
*******************************************************************************/

/*! Device driver object. */
extern const QIO_DRIVER_T SCBM_driver;

/******************************************************************************
************************** End Export Device Driver ***************************
*******************************************************************************/

#define MAX_NUM_SCBM_BLOCKS			(8)
/*! Block Descriptor pointer array */
extern ioblock_sBlockDescriptor	*	g_apsSCBMBlock[ MAX_NUM_SCBM_BLOCKS ];

/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

unsigned long SCBMasterGetErrorStatus(const SCBM_PORT_T *pPort);
/******************************************************************************
******************************** End Functions ********************************
*******************************************************************************/

#endif /* __SCBM_DRV_H__ */
