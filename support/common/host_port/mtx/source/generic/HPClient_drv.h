/*!
******************************************************************************
 @file   HPClient_drv.h

 @brief  Host Port Interface, Client side

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

#ifndef __HPCLIENT_DRV_H__
#define __HPCLIENT_DRV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "img_defs.h"
#include "system.h"

/*============================================================================
====	D E F I N E S
=============================================================================*/
#define HP_BASE_ADDRESS (SYSTEM_CONTROL_BASE_ADDRESS + 0x0400)
#define HOST_INT_INTERRUPT_BIT (10)

/*============================================================================
====	E N U M S
=============================================================================*/

typedef enum HPCLIENT_STATE_T_tag
{
	HPCLIENT_WAIT_FOR_START,
	HPCLIENT_WAIT_FOR_PAYLOAD
} HPCLIENT_STATE_T;

/*============================================================================
====	T Y P E D E F S
=============================================================================*/

/* device context */
typedef struct HPCLIENT_DEVICE_CONTEXT_T_tag
{
	IMG_BOOL			bInitialised;				// IMG_TRUE if the sub-system is initialised
    HPCLIENT_STATE_T	eState;						// state of reciever state machine

    IMG_UINT32			ui32countToRx;    			// count of transfers yet to recieve
    IMG_UINT8			*pui8rxBuffer;
    IMG_UINT8			*pui8rxBufferCurrent;
    IMG_UINT32			ui32rxBufferSizeInBytes;
    IMG_UINT32			*pui32numberOfBytesRead;
	IMG_BOOL			bRxFinished;
	IMG_BOOL			bWriteTimeoutFlag;

} HPCLIENT_DEVICE_CONTEXT_T;


/*============================================================================
	E N D
=============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* __HPCLIENT_DRV_H__   */

