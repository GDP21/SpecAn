/*!****************************************************************************
File          main.c

Title         Memory Test

Author        Bob Lambert

Date          29 June 2005

Copyright     Copyright 2005 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either
                material or conceptual may be copied or distributed,
                transmitted, transcribed, stored in a retrieval system
                or translated into any human or computer language in any
                form by any means, electronic, mechanical, manual or
                other-wise, or disclosed to third parties without the
                express written permission of Imagination Technologies
                Limited, Unit 8, HomePark Industrial Estate,
                King's Langley, Hertfordshire, WD4 8LZ, U.K.

Platform      Any

Description   Memory Test top module

DocVer        1.0 1st Release

******************************************************************************/

/******************************************************************************
*              file : $RCSfile: main.c,v $
*            author : $Author: jah $
* date last revised : $Date: 2011/04/19 14:44:03 $
*   current version : $Revision: 1.2 $
******************************************************************************/

/******************************* Includes ************************************/

#include <metag/metagtbi.h>

/******************************* External functions **************************/

//int memtest(void);
int memtest_harness(void);

/******************************* Code ****************************************/

int main()
{
	/* Do memory test (configured by TBI strings, see test.img) */
	int Result = 0;

	__TBILogF ("Memory test\n");

	Result = memtest_harness();

	if (Result == 0)
	{
		__TBILogF ("Result: 0x%08lX - ALL PASSED\n", Result);
		Result = 0;
	}
	else
	{
		__TBILogF ("Test %s\n", (Result) ? "Failed" : "Passed");
		Result = 0x7E57BAAD;
	}

	return Result;
}

/* End of main.c */
