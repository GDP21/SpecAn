/*!
******************************************************************************
 @file   testData.h

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

#ifndef __TEST_DATA_H__
#define __TEST_DATA_H__

#define DATA_PASS 1
#define DATA_FAIL 2

int checkTestBuffer(unsigned char  *pui8buffer, unsigned int ui32bufferSize, unsigned int ui32startIndex);
void fillTestBuffer(unsigned char  *pui8buffer, unsigned int ui32bufferSize, unsigned int ui32startIndex);

#endif /* __TEST_DATA_H__ */
