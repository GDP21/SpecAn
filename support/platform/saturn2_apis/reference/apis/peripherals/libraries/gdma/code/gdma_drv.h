/*!
*******************************************************************************
 @file   dmac_perip_drv.h

 @brief  Tarantino peripheral drivers DMAC device driver header

         This file provides the private interface to the DMAC device driver.

 @author Imagination Technologies

         <b>Copyright 2006 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to the third parties without the
         express written permission of Imagination Technologies
         Limited, Home Park Estate, Kings Langley, Hertfordshire,
         WD4 8LZ, U.K.

 \n<b>Platform:</b>\n
         Tarantino

*******************************************************************************/

#if !defined (__GDMA_DRV_H__)
#define __GDMA_DRV_H__


/******************************************************************************
**************************** Included header files ****************************
*******************************************************************************/

/*
* Early TBI variants don't contain support for extended contexts
* or CCB command passing. We define some dummies to keep the
* keader file happy and then use conditional compiles to
* omit the unsupported code.
*/
#ifndef TBI_1_1
typedef int TBIEXTCTX;
typedef int TBICMD;
#endif

#include <MeOS.h>

/******************************************************************************
***************************** Exported variables ******************************
*******************************************************************************/

/*! The private DMAC device driver object is exported to the API module */
extern const QIO_DRIVER_T			GDMA_sQIODriverFunctions;


/* Maximum number of individual GDMA hardware instances which this driver will support */
#define GDMA_MAX_NO_OF_HARDWARE_INSTANCES	(10)
/*! An array of pointers to hardware block descriptors - this allows the 	*/
/* 	GDMA API to obtain information about accessing the DMA hardware based	*/
/*	only on a hardware index. The information referenced by these pointers	*/
/*	is provide through calls to the function 'GDMA_Define'.					*/
extern ioblock_sBlockDescriptor * GDMA_apsHardwareInstances[GDMA_MAX_NO_OF_HARDWARE_INSTANCES];

/******************************************************************************
*************************** End Exported variables ****************************
*******************************************************************************/

#endif /* __DMAC_PERIP_DRV_H__ */
