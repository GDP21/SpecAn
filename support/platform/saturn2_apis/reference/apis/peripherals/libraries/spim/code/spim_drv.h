/*!
*******************************************************************************
 @file   spim_drv.h

 @brief  Serial Peripheral Interface Master Device Driver

         This file contains the header file information for the Serial
         Peripheral Interface Master (SPIM) device driver.

 @author Imagination Technologies

         <b>Copyright 2005 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to the third parties without the
         express written permission of Imagination Technologies
         Limited, Home Park Estate, Kings Langley, Hertfordshire,
         WD4 8LZ, U.K.

*******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log: spim_drv.h,v $

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 


******************************************************************************/

#ifndef __SPIM_DRV_H__
#define __SPIM_DRV_H__

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
====	D E F I N E S
=============================================================================*/


/*============================================================================
====	E N U M S
=============================================================================*/

/*!
*******************************************************************************
 This enumeration defines the supported SPIM driver methods.
*******************************************************************************/
enum SPIM_OPS
{
	/*! SPIM driver method: Read/Write */
	SPIM_READWRITE = 0
};

#define MAX_NUM_SPIM_BLOCKS			(8)
/*! Block Descriptor pointer array */
extern ioblock_sBlockDescriptor	*	g_apsSPIMBlock[ MAX_NUM_SPIM_BLOCKS ];

IMG_RESULT	SpimDmaComplete(	GDMA_sTransferObject	*	psTransferObject, QIO_STATUS_T	eQIOStatus, img_void	*	pvUserContext );

#if !defined (BOOT_CODE)
/* Basic (QIOless) SPIM Driver */
img_void BasicSPIMInit		( SPIM_sBlock	*	psBlock, SPIM_sInitParam	*	psInitParams );
img_void BasicSPIMIO		( SPIM_sBlock	*	psBlock, QIO_IOPARS_T	*	psIOPars );
img_void BasicSPIMDMAInit	( SPIM_sBlock	*	psBlock, SPIM_sInitParam	*	psInitParams );
img_void BasicSPIMDMA		( SPIM_sBlock	*	psBlock, QIO_IOPARS_T	*	psIOPars );
#endif

/*============================================================================
	E N D
=============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* __SPIM_DRV_H__ */
