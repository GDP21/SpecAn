/*!
******************************************************************************
 @file   mdtv.h

 @brief  MobileTV Remote API Message Definitions

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

******************************************************************************/

#ifndef _MDTV_H_
#define _MDTV_H_

/*****************************************************************************

 These definitions define the byte placement and the size of the fields in the
 fixed length header of the MobileTV Remote API Command/Response Messages

******************************************************************************/

#define MDTV_MESSAGE_ID_INDEX									0
#define MDTV_MESSAGE_ID_SIZE									1

#define MDTV_STANDARD_ID_INDEX									(MDTV_MESSAGE_ID_INDEX + MDTV_MESSAGE_ID_SIZE)
#define MDTV_STANDARD_ID_SIZE									1

#define MDTV_FUNCTION_ID_INDEX									(MDTV_STANDARD_ID_INDEX + MDTV_STANDARD_ID_SIZE)
#define MDTV_FUNCTION_ID_SIZE									1

#define MDTV_DATA_SIZE_INDEX									(MDTV_FUNCTION_ID_INDEX + MDTV_FUNCTION_ID_SIZE)
#define MDTV_DATA_SIZE_SIZE										4

#define MDTV_HEADER_SIZE										(MDTV_DATA_SIZE_INDEX + MDTV_DATA_SIZE_SIZE)

/*****************************************************************************

 This type defines the possible Standard ID's avaiable in the MobileTV Remote API

******************************************************************************/

typedef enum {
	MDTV_STANDARD_DVBH = 0,
	MDTV_STANDARD_DAB = 1,
	MDTV_STANDARD_ISDBT = 2,
	MDTV_STANDARD_DVBT = 3,
	MDTV_STANDARD_FM = 4,
	MDTV_STANDARD_ATSC = 5,
	MDTV_STANDARD_TUNER = 6
} MDTV_STANDARD_ID_E;

#endif /* _MDTV_H_ */
