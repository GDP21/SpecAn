/*!
*******************************************************************************
 @file   spis_drv.h

 @brief  Serial Peripheral Interface Slave Device Driver

         This file contains the header file information for the Serial
         Peripheral Interface Slave (SPIS) device driver.

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

#if !defined (__SPIS_DRV_H__)
#define __SPIS_DRV_H__

/******************************************************************************
******************************** Enumerations *********************************
*******************************************************************************/

/*!
*******************************************************************************

 These enumerations define the supported SPIS driver methods.

*******************************************************************************/
enum SPIS_OPS
{
	/*! SPIS driver method: Write */
	SPIS_OPCODE_WRITE = 0,

	/*! SPIS driver method: Read */
	SPIS_OPCODE_READ
};


/******************************************************************************
****************************** End enumerations *******************************
*******************************************************************************/


/******************************************************************************
**************************** Export Device Driver *****************************
*******************************************************************************/

/*! SPIS device driver structure */
extern const QIO_DRIVER_T SPIS_driver;

#define MAX_NUM_SPIS_BLOCKS			(8)
/*! Block Descriptor pointer array */
extern ioblock_sBlockDescriptor	*	g_apsSPISBlock[ MAX_NUM_SPIS_BLOCKS ];

IMG_RESULT	SpisDmaComplete(	GDMA_sTransferObject	*	psTransferObject,	QIO_STATUS_T	eQIOStatus, img_void	*	pvUserContext );

/******************************************************************************
************************** End Export Device Driver ***************************
*******************************************************************************/

#endif /* __SPIS_DRV_H__ */
