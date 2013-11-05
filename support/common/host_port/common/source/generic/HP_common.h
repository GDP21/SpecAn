/*!
******************************************************************************
 @file   HP_common.h

 @brief  Host Port Interface


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

#if !defined (__HP_COMMON_H__)
#define __HP_COMMON_H__

#if (__cplusplus)
extern "C" {
#endif

typedef enum HP_CONTROL_BITS_T_tag
{
	HP_START_MESSAGE = 1,
	HP_PAYLOAD_MESSAGE,
	HP_CLIENT_READY_MESSAGE
} HP_CONTROL_BITS_T;

#define HP_CONTROL_BITS_SHIFT				  (24)
#define HP_CONTROL_BITS_MASK				  (0x7F000000)

#define HP_PAYLOAD_BITS_SHIFT				  (0)
#define HP_PAYLOAD_BITS_MASK				  (0x00FFFFFF)

#if (__cplusplus)
}
#endif

#endif  /* __HP_COMMON_H__ */
