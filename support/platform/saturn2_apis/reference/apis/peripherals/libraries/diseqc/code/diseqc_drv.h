/*!
*******************************************************************************
 @file   diseqc_drv.h

 @brief  DiSEqC Master Device Driver

         This file contains the header file information for the
		 DiSEqC master device driver.

 @author Imagination Technologies

         <b>Copyright 2011 by Imagination Technologies Limited.</b>\n
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

#if !defined (__DISEQC_DRV_H__)
#define __DISEQC_DRV_H__

typedef enum
{
	DISEQC_OPCODE_CONTINUOUS_TONE = 0,
	DISEQC_OPCODE_END_CONTINUOUS_TONE,
	DISEQC_OPCODE_TONE_BURST_A,
	DISEQC_OPCODE_TONE_BURST_B,
	DISEQC_OPCODE_SEND_MESSAGE,
	DISEQC_OPCODE_DEINIT,
}DISEQC_OPCODE_T;


/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/

/*! Block Descriptor pointer array */
extern ioblock_sBlockDescriptor	*g_apsDISEQCBlock[DISEQC_NUM_BLOCKS];
extern ioblock_sBlockDescriptor	IMG_asDISEQCBlock[];

/******************************************************************************
**************************** Export Device Driver *****************************
*******************************************************************************/

/*! Device driver object */
extern const QIO_DRIVER_T DISEQC_Driver;

/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

#endif /* __DISEQC_DRV_H__ */
