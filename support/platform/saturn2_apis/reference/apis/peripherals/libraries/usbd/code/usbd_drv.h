/*!
*******************************************************************************
 @file   usbd_drv.h

 @brief  USBD Device Driver


 @author Imagination Technologies

         <b>Copyright 2007 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to the third parties without the
         express written permission of Imagination Technologies
         Limited, Home Park Estate, Kings Langley, Hertfordshire,
         WD4 8LZ, U.K.

 <b>Platform:</b>\n
         MobileTV

*******************************************************************************/

#if !defined (__USBD_DRV_H__)
#define __USBD_DRV_H__

#include "img_defs.h"


/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/

/******************************************************************************
************************** End constant definitions ***************************
*******************************************************************************/

/******************************************************************************
**************************** Export Device Driver *****************************
*******************************************************************************/

/*! Device driver object. */
extern const QIO_DRIVER_T USBD_driver;

#define MAX_NUM_USBD_BLOCKS				(16)
/*! Block Descriptor pointer array */
extern ioblock_sBlockDescriptor	*	g_apsUSBDBlock[ MAX_NUM_USBD_BLOCKS ];

/*! Macro to get USB_DC_T from block index */
#define USB_GET_CONTEXT( blockIndex )	((USB_DC_T *)(g_apsUSBDBlock[ blockIndex ]->pvAPIContext))

typedef struct usb_iso_transfer
{
	IMG_UINT8						ui8Opcode;
	IMG_UINT8					*	pui8Buf0;
	IMG_UINT8					*	pui8Buf1;
	IMG_UINT32						ui32DataPerFrame;
	IMG_UINT32						ui32BufferProcessingInterval;
	IMG_VOID						(*pfnProcessBuffer)(IMG_UINT8	ui8BufferNum,	IMG_UINT8	*	pui8Buffer	);
} usb_iso_transfer_t;

/******************************************************************************
************************** End Export Device Driver ***************************
*******************************************************************************/


/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

img_void			USBD_PerformSoftDisconnect		(	img_uint32					ui32BlockIndex );
img_void			USBD_ForceDeviceMode			(	img_uint32					ui32BlockIndex );
img_void			USBD_EnableCoreInterrupts		(	img_uint32					ui32BlockIndex, 
														img_bool					bEnable );
img_bool			USBD_IsInitialised				(	img_uint32					ui32BlockIndex );
img_void			USBD_SetInitialised				(	img_uint32					ui32BlockIndex,
														img_bool					bInitialised );
img_void			USBD_GetEP						(	img_uint32					ui32BlockIndex, 
														img_uint32					ui32EPNum,	
														USBD_sEP				**	ppsEP	);


#if !defined (USBD_NO_CBMAN_SUPPORT)
img_void			USBD_AddCallback				(	img_uint32					ui32BlockIndex,
														USBD_EVENT_T				eEvent,
														IMG_pfnEventCallback		pfnEventCallback,
														img_void				*	pvCallbackParameter,
														IMG_hCallback			*	phEventCallback );
USBD_RETURN_T		USBD_ExecuteCallback			(	img_uint32					ui32BlockIndex,
														USBD_EVENT_T				eEvent,
														img_uint32					ui32Param,
														img_void				*	pvParam	);
#endif

/******************************************************************************
******************************** End Functions ********************************
*******************************************************************************/

#endif /* __USBD_DRV_H__ */
