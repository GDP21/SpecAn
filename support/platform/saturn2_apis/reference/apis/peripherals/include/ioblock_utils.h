/*!
*******************************************************************************
 @file   ioblock_utils.h

 @brief  IOBLOCK Utility Functions

 @author Imagination Technologies

         <b>Copyright 2009 by Imagination Technologies Limited.</b>\n
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

#if !defined (__IOBLOCK_UTILS_H__)
#define __IOBLOCK_UTILS_H__

/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

/*!
*******************************************************************************

 @Function              @IOBLOCK_CalculateInterruptInformation

 <b>Description:</b>\n
 This function sets up the following elements of the block context structure
 pointed to by psBlockContext: sDeviceISRInfo, sDMACChannelISRInfo, and
 ui32IntMasks, based on the values supplied by psBlockDescriptor.

 psBlockContext->psBlockDescriptor must be a valid pointer to a completed
 block descriptor.

 \param     psBlockContext  A pointer to a block context structure.

 \return                    None.

*******************************************************************************/
#if 0
img_void IOBLOCK_CalculateInterruptInformation(	const ioblock_sBlockDescriptor	*	psBlockDescriptor,
												const ioblock_sInterruptBlock	*	psInterruptDescriptor,
												ioblock_sISRInfo				*	psISRInfo,
												img_uint32							aui32IntMasks[ QIO_MAXVECGROUPS ]	);
#else
img_void IOBLOCK_CalculateInterruptInformation(	ioblock_sBlockDescriptor	*	psBlockDescriptor );
#endif
/******************************************************************************
******************************** End Functions ********************************
*******************************************************************************/

#endif /* __IOBLOCK_UTILS_H__ */
