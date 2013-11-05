/*!
*******************************************************************************
  file   ioblock_utils.c

  brief  IOBLOCK Utility Functions

  author Imagination Technologies

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

/******************************************************************************
**************************** Included header files ****************************
*******************************************************************************/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES

#include <metag/machine.inc>
#include <metag/metagtbi.h>
#include <MeOS.h>

#include <img_defs.h>
#include <assert.h>
#include "ioblock_defs.h"

/* ------------------------------------------------------------------------ */
/*                                                                          */
/*                        FUNCTION DEFINITIONS                              */
/*                                                                          */
/* ------------------------------------------------------------------------ */

#if 0
img_void IOBLOCK_CalculateInterruptInformation(	const ioblock_sBlockDescriptor	*	psBlockDescriptor,
												const ioblock_sInterruptBlock	*	psInterruptDescriptor,
												ioblock_sISRInfo				*	psISRInfo,
												img_uint32							aui32IntMasks[ QIO_MAXVECGROUPS ]	)
#else
img_void IOBLOCK_CalculateInterruptInformation(	ioblock_sBlockDescriptor	*	psBlockDescriptor )
#endif						
{
	img_uint32					ui32IntraOffset;
	img_uint32					ui32InterOffset;
	img_uint32					i;
	img_bool					bMatchingHWSTATEXTAddrFound = IMG_FALSE;
	ioblock_sInterruptBlock	*	psInterruptDescriptor;
	ioblock_sISRInfo *			psISRInfo;

	/* psBlockDescriptor must be valid */
	IMG_ASSERT ( psBlockDescriptor );

	/* psInterruptDescriptor must be valid */
////	IMG_ASSERT( psInterruptDescriptor );
	psInterruptDescriptor	= psBlockDescriptor->psInterruptDescriptor;
	psISRInfo				= &(psBlockDescriptor->sDeviceISRInfo);

	/*
		Calculate and save the interrupt register info.
		ui32IntraOffset is the index of the trigger bit for the specified trigger within the associated block of 32 triggers.
		ui32InterOffset is the index of the bank of 32 triggers for the specified trigger (eg triggers 0 - 31 are bank 0,
																								triggers 32 - 63 are bank 1,
																								triggers 64 - 85 are bank 2,
																								etc)																							
	*/

	for (i = 0; i < QIO_MAXVECGROUPS; ++i)
	{
		psBlockDescriptor->ui32IntMasks[i] = 0;
	}

	ui32IntraOffset = psBlockDescriptor->ui32InterruptTrigger & 31;
	ui32InterOffset = psBlockDescriptor->ui32InterruptTrigger / 32;

	psISRInfo->ui32STATEXTAddress	= psInterruptDescriptor->sSTATEXT.ui32Base + 
									  (psInterruptDescriptor->sSTATEXT.ui32InterBlockStrideInBytes * ui32InterOffset) + 
									  (((psInterruptDescriptor->sSTATEXT.ui32IntraBlockStrideInBits * ui32IntraOffset) / 32 ) * 4 );
	
	psISRInfo->ui32STATEXTMask		=	1 << ( (ui32IntraOffset * psInterruptDescriptor->sSTATEXT.ui32IntraBlockStrideInBits) & 31 );

	psISRInfo->ui32LEVELEXTAddress	=	psInterruptDescriptor->sLEVELEXT.ui32Base + 
										(psInterruptDescriptor->sLEVELEXT.ui32InterBlockStrideInBytes * ui32InterOffset) + 
										(((psInterruptDescriptor->sLEVELEXT.ui32IntraBlockStrideInBits * ui32IntraOffset) / 32) * 4);

	psISRInfo->ui32LEVELEXTMask		=	1 << ( (ui32IntraOffset * psInterruptDescriptor->sLEVELEXT.ui32IntraBlockStrideInBits) & 31 );

	/* Set interrupt masks */
	for (i = 0; i < QIO_MAXVECGROUPS; i++)
	{
		if ( psBlockDescriptor->psIVDesc->hwStatAdds[i] == psISRInfo->ui32STATEXTAddress )
		{
			if ( bMatchingHWSTATEXTAddrFound )
			{
				/* IVDesc contains duplicate hwStat addresses */
				IMG_ASSERT ( IMG_FALSE );
			}
////			aui32IntMasks[i] |= psISRInfo->ui32STATEXTMask;
			psBlockDescriptor->ui32IntMasks[i] |= psISRInfo->ui32STATEXTMask;
			bMatchingHWSTATEXTAddrFound = IMG_TRUE;
		}
	}

	IMG_ASSERT ( bMatchingHWSTATEXTAddrFound );
}

