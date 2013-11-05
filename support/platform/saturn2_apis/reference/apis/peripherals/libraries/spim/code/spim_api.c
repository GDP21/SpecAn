/*!
*******************************************************************************
  file   spim_api.c

  brief  Serial Peripheral Interface Master API

         This file defines the functions that make up the Serial Peripheral
         Interface Master (SPIM) API.

  author Imagination Technologies

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

*******************************************************************************/

/*============================================================================*/
/*                                                                            */
/*                          INCLUDE FILES		                              */
/*                                                                            */
/*============================================================================*/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

/* MeOS Library */
#include <MeOS.h>

#include <ioblock_defs.h>

#include <gdma_api.h>
/* SPI Master Driver */
#include "spim_api.h"
#include "spim_drv.h"

/*============================================================================*/
/*                                                                            */
/*                          MACRO DEFINITIONS	                              */
/*                                                                            */
/*============================================================================*/

/* infinite wait on QIO IO */
#define INFWAIT (-1)

/* definitions to improve readability */
#ifndef TRUE
#define TRUE  (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

extern const QIO_DRIVER_T g_sDriver;

img_void spimDefine(	ioblock_sBlockDescriptor	*	psBlockDescriptor	)
{
	// Check for a valid descriptor
	IMG_ASSERT( psBlockDescriptor );
	// Check our internal descriptor pointer array is big enough
	IMG_ASSERT( psBlockDescriptor->ui32Index < MAX_NUM_SPIM_BLOCKS );
	// Assign the descriptor to our internal list
	g_apsSPIMBlock[ psBlockDescriptor->ui32Index ] = psBlockDescriptor;
}


/*============================================================================*/
/*                                                                            */
/*                          EXPORTED FUNCTIONS	                              */
/*                                                                            */
/*============================================================================*/

/*!
*******************************************************************************

 @Function				@SPIMDefine

 @Description
 This function defines an instance of a SPI Master block.

*******************************************************************************/
img_void SPIMDefine(	ioblock_sBlockDescriptor	*	psBlockDescriptor	)
{
	// This function is now deprecated
	return;
}

/*!
*******************************************************************************

 @Function              @SPIMInit

 <b>Description:</b>\n
 This function is used to initialise the SPI Master module.

 It must be called before any other SPI Master API function. It is not
 necessary to call it again unless any of the parameters set up in SPIMInit()
 are to be altered.

 The function initialises the block, ready for a read or write transaction
 using any of the available chip select (CS) lines.\n
 Separate timing parameters may be specified for each CS line and the DMA
 channels to be used for input and output data are configured.\n
 The input and output DMA channels must not be the same.

 \param     *psBlock            Pointer to block descriptor.
 \param     *psInitParam        Pointer to initialisation parameters.

 \return                        SPIM_OK if initialisation parameters are valid
                                SPIM_INVALID_<type> if <type> parameter is not valid

*******************************************************************************/
SPIM_eReturn SPIMInit(	SPIM_sBlock		*	psBlock, 
						SPIM_sInitParam	*	psInitParam	)
{
	QIO_DEVICE_T				*	psDevice;

	IMG_ASSERT ( psBlock != IMG_NULL );
	IMG_ASSERT ( psBlock->bInitialised == IMG_FALSE );                                          
	                                            
	IMG_ASSERT ( psInitParam != IMG_NULL );

	/* Clear block structure */
	IMG_MEMSET ( psBlock, 0, sizeof(SPIM_sBlock) );

	psDevice = &psBlock->sDevice;

    /* Check initialisation parameters */
//	if ( psInitParam->ui32DMAChannel > DMAC_PERIP_MAX_CHANNEL )
//	{
//		return SPIM_INVALID_DMA_CHANNELS;
//	}
    if ( ( psInitParam->sDev0Param.eSPIMode > SPIM_MODE_3 ) ||
         ( psInitParam->sDev1Param.eSPIMode > SPIM_MODE_3 ) ||
         ( psInitParam->sDev2Param.eSPIMode > SPIM_MODE_3 ) )
    {
        return SPIM_INVALID_SPI_MODE;
	}
    if ( ( psInitParam->sDev0Param.ui32CSIdleLevel > 1 ) ||
         ( psInitParam->sDev1Param.ui32CSIdleLevel > 1 ) ||
         ( psInitParam->sDev2Param.ui32CSIdleLevel > 1 ) )
    {
        return SPIM_INVALID_CS_IDLE_LEVEL;
	}
    if ( ( psInitParam->sDev0Param.ui32DataIdleLevel > 1 ) ||
         ( psInitParam->sDev1Param.ui32DataIdleLevel > 1 ) ||
         ( psInitParam->sDev2Param.ui32DataIdleLevel > 1 ) )
    {
        return SPIM_INVALID_DATA_IDLE_LEVEL;
	}

	if ( !g_apsSPIMBlock[ psInitParam->ui32BlockIndex ] )
	{
		spimDefine( &IMG_asSPIMBlock[ psInitParam->ui32BlockIndex ] );
	}

	// Assign the context space
	g_apsSPIMBlock[ psInitParam->ui32BlockIndex ]->pvAPIContext = (img_void *)psBlock;

#if !defined (BOOT_CODE)
	if ( psInitParam->bBypassQIO )
	{
		if ( psInitParam->bBypassDMA )
		{
			BasicSPIMInit( psBlock, psInitParam );
		}
		else
		{
			BasicSPIMDMAInit( psBlock, psInitParam );
		}
	}
	else
#endif
	{
		GDMA_sCallbackFunctions			sDMACallbacks;
		
		sDMACallbacks.pfnInitDevice		= IMG_NULL;
		sDMACallbacks.pfnStartDevice	= IMG_NULL;
		sDMACallbacks.pfnCancelDevice	= IMG_NULL;
		sDMACallbacks.pfnCompletion		= &SpimDmaComplete;
		// Initialise GDMA
		GDMA_Initialise(	&psBlock->sDMAContext,
							psInitParam->ui32DMAChannel,
							0,
							IMG_FALSE,
							&sDMACallbacks,
							(img_void *)g_apsSPIMBlock[ psInitParam->ui32BlockIndex ],
							IMG_NULL,
							0,
							IMG_TRUE,
							0,
							IMG_NULL,
							0,
							IMG_TRUE );
		GDMA_Configure(		&psBlock->sDMAContext	);

		/* Initialise the device */
		QIO_init( psDevice, "SPI Master", (IMG_UINT32)psInitParam, &g_sDriver);

		/* Enable the device */
		QIO_enable( psDevice );
	}
                  
    psBlock->bInitialised = IMG_TRUE;
                  
    return SPIM_OK;
}
                           
 
 /*!
*******************************************************************************

 @Function              @SPIMDeinit
                           
*******************************************************************************/
SPIM_eReturn SPIMDeinit( SPIM_sBlock		*	psBlock )
{
	IMG_ASSERT ( psBlock != IMG_NULL );
	IMG_ASSERT ( psBlock->bInitialised == IMG_TRUE );
	
	QIO_disable ( &psBlock->sDevice );
	QIO_unload ( &psBlock->sDevice );
	
	if ( psBlock->bBypassQIO == IMG_FALSE )
	{
		GDMA_Disable( &psBlock->sDMAContext );
		GDMA_Unconfigure( &psBlock->sDMAContext );
		GDMA_Deinitialise( &psBlock->sDMAContext );
	}
	
	psBlock->bInitialised = IMG_FALSE;
	
	return SPIM_OK;
}

/*!
*******************************************************************************

 @Function              @SPIMReadWrite

 <b>Description:</b>\n
 This function is used to perform an SPI master transaction.

 The transaction can involve up to three stages, described by the inputs a, b
 and c, which are pointers to SPIM_BUF_T structures containing parameters
 for each operation.

 'a' describes the first standard read or write operation.

 If required, this can be followed immediately by another read or write
 operation described by 'b'. If this second read/write is not required, b
 should be set to NULL.

 If the SPIM's compare data functionality is to be used, then this part of the
 transaction is described in 'c'.

 The SPIM API and device driver treats the compare data detection as another
 operation, which is why the calling function defines it using a third
 SPIM_BUF_T pointer. The compare date operation will always precede the first
 standard read/write described in 'a'. The compare data operation involves
 reading in a series of data bytes from the slave (which are discarded) until
 it receives a byte that is valid for compare data detection (as defined by
 the parameters in c). At this point it starts the operation defined by 'a'

 If the compare data operation is not required, c should be set to NULL.

 Of the parameters contained in the SPIM_BUF_T structure, cmpValue, cmpMask
 and cmpEq define the conditions for detection of a 'compare data byte'.
 cmpValue is the value that the SPIM compares with each byte received in a
 compare data operation. cmpMask is the bit mask applied to both the received
 byte and cmpValue when the comparison is made. cmpEq determines whether a
 detection occurs when the two bytes are equal or when they are not equal.

 When a standard SPIM operation (as described by a and/or b) is carried out,
 the SPIM sets one of the three CS lines (decided by 'chipSelect') to active
 to begin the operation and transfers 'size' bytes in and out of the SPIM.

 This driver does not support simultaneous input and output of data, so for
 reads no data is written (or any that may inadvertantly be written should
 be ignored by the slave) and for writes, any received data is ignored.

 The 'chipSelect' parameter determines which of the three SPIM_DEVICE_PARAM_T
 parameter settings (set in the SPIMInit function) will be used.

 The 'read' parameter determines whether an operation is a read or write.

 The 'buf' parameter points to the first term in the buffer array where,
 for reads, received data will be stored and, for writes, data will be written
 from. At the end of the operation, the CS line will go inactive unless 'cont'
 is set to TRUE, in which case the CS line will remain active for the next operation.

 \param     *psBlock            Pointer to block descriptor.
 \param     *psA                Pointer to parameters for first read or write
                                operation in the transaction (excluding compare
                                data operation).
 \param     *psB                Pointer to parameters for second read or write
                                operation in the transaction (NULL if not required).
 \param     *psC                Pointer to parameters for compare data operation
                                (NULL if not required).

 \return                        SPIM_OK if initialisation parameters are valid
                                SPIM_INVALID_<type> if <type> parameter is not valid

*******************************************************************************/
SPIM_eReturn SPIMReadWrite(	SPIM_sBlock		*	psBlock, 
							SPIM_sBuffer	*	psA, 
							SPIM_sBuffer	*	psB, 
							SPIM_sBuffer	*	psC )
{
    QIO_DEVICE_T	*	psDevice;
    QIO_IOPARS_T		sIOPars;
    
	IMG_ASSERT ( psBlock != IMG_NULL );
	IMG_ASSERT ( psBlock->bInitialised == IMG_TRUE );    

    /* Check operation parameters for a */
    if ( psA->eChipSelect > SPIM_DUMMY_CS )
    {
        return SPIM_INVALID_CS_LINE;
	}
    if ( psA->ui32Size > SPIM_MAX_TRANSFER_BYTES )
    {
        return SPIM_INVALID_SIZE;
	}
    if ( psA->i32Read > 1 )
    {
        return SPIM_INVALID_READ_WRITE;
	}
    if ( psA->i32Cont > 1 )
    {
        return SPIM_INVALID_CONTINUE;
	}

    /* Get the device object */
    psDevice = &psBlock->sDevice;

	if ( psC != IMG_NULL )
	{
		// Assert as the code will not work with updated compare IP.
		IMG_ASSERT(0);

		/* Check operation parameters for c */
		if ( psC->eChipSelect > SPIM_DUMMY_CS )
		{
			return SPIM_INVALID_CS_LINE;
		}
		if ( psC->i32Cont > 1 )
		{
			return SPIM_INVALID_CONTINUE;
		}
		if ( psC->iCmpEq > 1 )
		{
			return SPIM_INVALID_COMPARE_EQ;
		}

		/* Start 'wait read' transaction. This reads from the slave continuously until a
		** specific byte of data is received.
		*/
		psC->ui32Size = 1;
		psC->i32Read = TRUE;
		//make sure cmpData element of c is TRUE so the device driver performs a 'wait read'
		psC->i32CmpData = TRUE;
		sIOPars.opcode  = SPIM_READWRITE;
		sIOPars.pointer = (IMG_VOID *)psC;
		sIOPars.counter = 1;
		QIO_qioWait( psDevice, &sIOPars, INFWAIT );
	}

	//make sure cmpData elements of a and b are set to FALSE, so that a 'wait read' is not performed
	//in the main Read/Write job
	psA->i32CmpData = FALSE;

    sIOPars.opcode  = SPIM_READWRITE;
    sIOPars.pointer = (void *)psA;
    if ( psB == IMG_NULL)
        sIOPars.counter = 1;
    else
    {
		/* Check operation parameters for b */
		if ( psB->eChipSelect > SPIM_DUMMY_CS )
		{
			return SPIM_INVALID_CS_LINE;
		}
		if ( psB->ui32Size > SPIM_MAX_TRANSFER_BYTES )
		{
			return SPIM_INVALID_SIZE;
		}
		if ( psB->i32Read > 1 )
		{
			return SPIM_INVALID_READ_WRITE;
		}
		if ( psB->i32Cont > 1 )
		{
			return SPIM_INVALID_CONTINUE;
		}
		psB->i32CmpData = FALSE;
        sIOPars.counter = 2;
        sIOPars.spare   = (IMG_VOID *)psB;
    }

#if !defined (BOOT_CODE)
	if ( psBlock->bBypassQIO )
	{
		if ( psBlock->bBypassDMA )
		{
			BasicSPIMIO( psBlock, &sIOPars );
		}
		else
		{
			BasicSPIMDMA( psBlock, &sIOPars );			
		}
	}
	else
#endif
	{
		QIO_qioWait( psDevice, &sIOPars, INFWAIT );
	}

    return SPIM_OK;
}
