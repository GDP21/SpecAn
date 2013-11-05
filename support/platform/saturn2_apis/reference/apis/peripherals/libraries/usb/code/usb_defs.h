/*!
*******************************************************************************
 @file   usbd_defs.h

 @brief  USBD Device Driver defs


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

#if !defined (__USBD_DEFS_H__)
#define __USBD_DEFS_H__

#define METAG_ALL_VALUES
#define METAC_ALL_VALUES

#include <metag/machine.inc>
#include <metag/metagtbi.h>
#include <MeOS.h>

#include <img_defs.h>

#include <usb_spec.h>
#include <usbd_api.h>

/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/

/* Default maxpacket size for any EP */
#define USBD_MPS_DEFAULT		(1024)

/* Maxpacket size for EP0 */
#define USBD_MPS_EP0			(64)

/* Maxpacket size for various ep types and speeds */ 
#define USBD_MPS_EP_ISOC_FS		(64)		/* Max 1023 */
#define USBD_MPS_EP_BULK_FS		(64)		/* Max 64 */

#define USBD_MPS_EP_ISOC_HS		(1024)		/* Max 1024 (x3) */
#define USBD_MPS_EP_BULK_HS		(512)		/* 512 mandatory */



/* Error codes used in the driver */
#define USBD_ERR_AGAIN			(1)
#define USBD_ERR_NOMEM			(2)
#define USBD_ERR_NODEV			(3)
#define USBD_ERR_INVAL			(4)
#define USBD_ERR_DOM			(5)
#define USBD_ERR_RANGE			(6)
#define USBD_ERR_INPROGRESS		(7)
#define USBD_ERR_OPNOTSUPP		(8)
#define USBD_ERR_SHUTDOWN		(9)
#define USBD_ERR_L2HLT			(10)
#define USBD_ERR_CONNRESET		(11)
#define USBD_ERR_CONNABORTED	(12)
#define USBD_ERR_OVERFLOW		(13)
#define USBD_ERR_REMOTEIO		(14)
#define USBD_ERR_NODATA			(15)


/******************************************************************************
************************** End constant definitions ***************************
*******************************************************************************/


/******************************************************************************
******************************** Enumerations *********************************
*******************************************************************************/


typedef enum usbd_state_t 
{	
	USBD_STATE_NOTATTACHED = 0,		/* The device has not been attached to the bus */
	USBD_STATE_ATTACHED,			/* The device has been attached to the bus */
	USBD_STATE_POWERED,				/* The device has been powered */
	USBD_STATE_DEFAULT,				/* The device has been reset */
	USBD_STATE_ADDRESS,				/* A device address has been assigned */
	USBD_STATE_CONFIGURED,			/* The device has been configured (functionality fully available) */
	USBD_STATE_SUSPENDED			/* No activity seen for 3ms (device functionality not available) */

} USBD_STATE;

typedef enum usbd_eTransferStatus_t
{
	USBD_XFER_STATUS_IDLE				=	0,
	USBD_XFER_STATUS_ACTIVE				=	1,
} usbd_eTransferStatus;


/******************************************************************************
******************************** Structures ***********************************
*******************************************************************************/

/******************************************************************************
**************************** Export Device Driver *****************************
*******************************************************************************/

/******************************************************************************
************************** End Export Device Driver ***************************
*******************************************************************************/

/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

/******************************************************************************
******************************** End Functions ********************************
*******************************************************************************/

#endif /* __USBD_DEFS_H__ */
