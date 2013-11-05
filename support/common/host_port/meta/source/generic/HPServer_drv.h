/*!
******************************************************************************
 @file   HPServer_drv.h

 @brief  Host Port Interface, Server side

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

#ifndef __HPSERVER_DRV_H__
#define __HPSERVER_DRV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "img_defs.h"
#include "sys_proc.h"

/*============================================================================
====	D E F I N E S
=============================================================================*/
#define HP_BASE_ADDRESS (SYSTEM_CONTROL_BASE_ADDRESS + 0x14400)

//trigger 33. Ie bit1 in HWSTATEXT2
#define MTX_INT_TRIGGER                     33

#define MTX_INT_HWVECEXT                    HWVEC20EXT
#define MTX_INT_HWLEVELEXT                  HWLEVELEXT2
#define MTX_INT_HWSTATEXT                   HWSTATEXT2
#define MTX_INT_HWVECEXT_BASE_TRIGGER       32         // trigger number of first trigger in chosen HWVECEXT register */

#define MTX_INT_TRIGGER_OFFSET              (MTX_INT_TRIGGER-MTX_INT_HWVECEXT_BASE_TRIGGER)
#define MTX_INT_HWSTAT_MASK                 (1<<MTX_INT_TRIGGER_OFFSET)


/*============================================================================
====	E N U M S
=============================================================================*/

typedef enum HPSERVER_STATE_T_tag
{
	HPSERVER_WAIT_FOR_START,
	HPSERVER_WAIT_FOR_PAYLOAD
} HPSERVER_STATE_T;

/*============================================================================
====	T Y P E D E F S
=============================================================================*/

/* device context */
typedef struct HPSERVER_DEVICE_CONTEXT_T_tag
{
	IMG_BOOL			bInitialised;				// IMG_TRUE if the sub-system is initialised
    HPSERVER_STATE_T	eState;						// state of reciever state machine

    IMG_UINT32			ui32countToRx;    			// count of transfers yet to recieve
    IMG_UINT8			*pui8rxBuffer;
    IMG_UINT8			*pui8rxBufferCurrent;
    IMG_UINT32			ui32rxBufferSizeInBytes;
    IMG_UINT32			*pui32numberOfBytesRead;
	IMG_BOOL			bRxFinished;
	IMG_BOOL			bWriteTimeoutFlag;

} HPSERVER_DEVICE_CONTEXT_T;


/*============================================================================
	E N D
=============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* __HPSERVER_DRV_H__   */

