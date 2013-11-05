/*!
******************************************************************************
 @file   testData.c

 @brief  Host Port Interface test harness, common helpers.

 @Author Imagination Technologies

 @date   28/09/2010

         <b>Copyright 2010 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to the third parties without the
         express written permission of Imagination Technologies
         Limited, Home Park Estate, Kings Langley, Hertfordshire,
         WD4 8LZ, U.K.

\n\n\n

******************************************************************************/

#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include <assert.h>
#include "testData.h"

//Some test data (random wikipedia article!)
static char i8TestData01[] = "Tregole is a national park in Queensland, Australia, 603 km west of Brisbane. Until the gazetting of the park in 1975, the area was a grazing property. The park is located where the brigalow and mulga biospheres meet and has a representative sample of semi-arid ecosystems. The park contains almost pure stands of the vulnerable Ooline tree. The Ooline stand in Tregole is unusual as the climate is hot and dry. The park has no camping facilities. The day use area is 10 kilometres south of Morven on the Morven-Bollon Road. There is a short (2.1 km) walk in the day use area.";

void fillTestBuffer(unsigned char  *pui8buffer, unsigned int ui32bufferSize, unsigned int ui32startIndex)
{
	unsigned int i;

	//check ui32startIndex is not bigger than the test data
	if (ui32startIndex > (sizeof(i8TestData01) - 2))
	{
		//if it is then make it zero
		ui32startIndex = 0;
		assert(0);
	}

	for (i=0; i<ui32bufferSize; i++)
	{
		*pui8buffer = i8TestData01[ui32startIndex];
		pui8buffer++;
		ui32startIndex++;
		if (ui32startIndex > (sizeof(i8TestData01) - 2))
		{
			//if it is then make it zero
			ui32startIndex = 0;
		}
	}
}

int checkTestBuffer(unsigned char  *pui8buffer, unsigned int ui32bufferSize, unsigned int ui32startIndex)
{
	unsigned int i;

	//check ui32startIndex is not bigger than the test data
	if (ui32startIndex > (sizeof(i8TestData01) - 2))
	{
		//if it is then make it zero
		ui32startIndex = 0;
		assert(0);
	}

	for (i=0; i<ui32bufferSize; i++)
	{
		if (*pui8buffer != i8TestData01[ui32startIndex])
			return DATA_FAIL;

		*pui8buffer = i8TestData01[ui32startIndex];
		pui8buffer++;
		ui32startIndex++;
		if (ui32startIndex > (sizeof(i8TestData01) - 2))
		{
			//if it is then make it zero
			ui32startIndex = 0;
		}
	}
	return DATA_PASS;
}
