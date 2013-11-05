/*!
*******************************************************************************
  file   gdma_hal_sysbus.c

  author Imagination Technologies

         <b>Copyright 2008 by Imagination Technologies Limited.</b>\n
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

/******************************************************************************
**************************** Included header files ****************************
*******************************************************************************/

/* Keep these first ... */
#if defined(METAG) & !defined(METAG_ALL_VALUES)
#define METAG_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#include <metag/tbiccb00.h>
#else
#include <string.h>
#endif  /* METAG and NOT METAG_ALL_VALUES */

#include <MeOS.h>

#include <assert.h>

/* Device files */
#include "ioblock_defs.h"
#include "gdma_api.h"
#include "gdma_drv.h"
#include "gdma_hal.h"
#include "gdma_mdc.h"

/* -------------------- Register MDC_GENERAL_CONFIG -------------------- */

#define MDC_GENERAL_CONFIG_OFFSET				(0x000)

#define MDC_LIST_IEN_OFFSET						MDC_GENERAL_CONFIG_OFFSET
#define MDC_LIST_IEN_SHIFT						(31)
#define MDC_LIST_IEN_MASK						(0x80000000)
#define MDC_LIST_IEN_FLAGS						(REG_RW)
#define MDC_LIST_IEN_LENGTH						(1)

#define MDC_BSWAP_OFFSET						MDC_GENERAL_CONFIG_OFFSET
#define MDC_BSWAP_SHIFT							(30)
#define MDC_BSWAP_MASK							(0x40000000)
#define MDC_BSWAP_FLAGS							(REG_RW)
#define MDC_BSWAP_LENGTH						(1)

#define MDC_IEN_OFFSET							MDC_GENERAL_CONFIG_OFFSET
#define MDC_IEN_SHIFT							(29)
#define MDC_IEN_MASK							(0x20000000)
#define MDC_IEN_FLAGS							(REG_RW)
#define MDC_IEN_LENGTH							(1)

#define MDC_LEVEL_INT_OFFSET					MDC_GENERAL_CONFIG_OFFSET
#define MDC_LEVEL_INT_SHIFT						(28)
#define MDC_LEVEL_INT_MASK						(0x10000000)
#define MDC_LEVEL_INT_FLAGS						(REG_RW)
#define MDC_LEVEL_INT_LENGTH					(1)

#define MDC_CHANNEL_OFFSET						MDC_GENERAL_CONFIG_OFFSET
#define MDC_CHANNEL_SHIFT						(20)
#define MDC_CHANNEL_MASK						(0x03F00000)
#define MDC_CHANNEL_FLAGS						(REG_RW)
#define MDC_CHANNEL_LENGTH						(6)

#define MDC_ACC_DEL_OFFSET						MDC_GENERAL_CONFIG_OFFSET
#define MDC_ACC_DEL_SHIFT						(16)
#define MDC_ACC_DEL_MASK						(0x00070000)
#define MDC_ACC_DEL_FLAGS						(REG_RW)
#define MDC_ACC_DEL_LENGTH						(1)

#define MDC_WAIT_UNPACK_OFFSET					MDC_GENERAL_CONFIG_OFFSET
#define MDC_WAIT_UNPACK_SHIFT					(13)
#define MDC_WAIT_UNPACK_MASK					(0x00002000)
#define MDC_WAIT_UNPACK_FLAGS					(REG_RW)
#define MDC_WAIT_UNPACK_LENGTH					(1)

#define MDC_INC_W_OFFSET						MDC_GENERAL_CONFIG_OFFSET
#define MDC_INC_W_SHIFT							(12)
#define MDC_INC_W_MASK							(0x00001000)
#define MDC_INC_W_FLAGS							(REG_RW)
#define MDC_INC_W_LENGTH						(1)

#define MDC_WAIT_PACK_OFFSET					MDC_GENERAL_CONFIG_OFFSET
#define MDC_WAIT_PACK_SHIFT						(9)
#define MDC_WAIT_PACK_MASK						(0x00000200)
#define MDC_WAIT_PACK_FLAGS						(REG_RW)
#define MDC_WAIT_PACK_LENGTH					(1)

#define MDC_INC_R_OFFSET						MDC_GENERAL_CONFIG_OFFSET
#define MDC_INC_R_SHIFT							(8)
#define MDC_INC_R_MASK							(0x00000100)
#define MDC_INC_R_FLAGS							(REG_RW)
#define MDC_INC_R_LENGTH						(1)

#define MDC_PHYSICAL_W_OFFSET					MDC_GENERAL_CONFIG_OFFSET
#define MDC_PHYSICAL_W_SHIFT					(7)
#define MDC_PHYSICAL_W_MASK						(0x00000080)
#define MDC_PHYSICAL_W_FLAGS					(REG_RW)
#define MDC_PHYSICAL_W_LENGTH					(1)

#define MDC_WIDTH_W_OFFSET						MDC_GENERAL_CONFIG_OFFSET
#define MDC_WIDTH_W_SHIFT						(4)
#define MDC_WIDTH_W_MASK						(0x00000070)
#define MDC_WIDTH_W_FLAGS						(REG_RW)
#define MDC_WIDTH_W_LENGTH						(3)

#define MDC_PHYSICAL_R_OFFSET					MDC_GENERAL_CONFIG_OFFSET
#define MDC_PHYSICAL_R_SHIFT					(3)
#define MDC_PHYSICAL_R_MASK						(0x00000008)
#define MDC_PHYSICAL_R_FLAGS					(REG_RW)
#define MDC_PHYSICAL_R_LENGTH					(1)

#define MDC_WIDTH_R_OFFSET						MDC_GENERAL_CONFIG_OFFSET
#define MDC_WIDTH_R_SHIFT						(0)
#define MDC_WIDTH_R_MASK						(0x00000007)
#define MDC_WIDTH_R_FLAGS						(REG_RW)
#define MDC_WIDTH_R_LENGTH						(3)

/* -------------------- Register MDC_READ_PORT_CONFIG -------------------- */

#define MDC_READ_PORT_CONFIG_OFFSET				(0x004)

#define MDC_STHREAD_OFFSET						MDC_READ_PORT_CONFIG_OFFSET
#define MDC_STHREAD_SHIFT						(28)
#define MDC_STHREAD_MASK						(0xF0000000)
#define MDC_STHREAD_FLAGS						(REG_RW)
#define MDC_STHREAD_LENGTH						(4)

#define MDC_RTHREAD_OFFSET						MDC_READ_PORT_CONFIG_OFFSET
#define MDC_RTHREAD_SHIFT						(24)
#define MDC_RTHREAD_MASK						(0x0F000000)
#define MDC_RTHREAD_FLAGS						(REG_RW)
#define MDC_RTHREAD_LENGTH						(4)

#define MDC_PRIORITY_OFFSET						MDC_READ_PORT_CONFIG_OFFSET
#define MDC_PRIORITY_SHIFT						(20)
#define MDC_PRIORITY_MASK						(0x00F00000)
#define MDC_PRIORITY_FLAGS						(REG_RW)
#define MDC_PRIORITY_LENGTH						(4)

#define MDC_WTHREAD_OFFSET						MDC_READ_PORT_CONFIG_OFFSET
#define MDC_WTHREAD_SHIFT						(16)
#define MDC_WTHREAD_MASK						(0x000F0000)
#define MDC_WTHREAD_FLAGS						(REG_RW)
#define MDC_WTHREAD_LENGTH						(4)

#define MDC_HOLD_OFF_OFFSET						MDC_READ_PORT_CONFIG_OFFSET
#define MDC_HOLD_OFF_SHIFT						(12)
#define MDC_HOLD_OFF_MASK						(0x0000F000)
#define MDC_HOLD_OFF_FLAGS						(REG_RW)
#define MDC_HOLD_OFF_LENGTH						(4)

#define MDC_BURST_SIZE_OFFSET					MDC_READ_PORT_CONFIG_OFFSET
#define MDC_BURST_SIZE_SHIFT					(4)
#define MDC_BURST_SIZE_MASK						(0x00000FF0)
#define MDC_BURST_SIZE_FLAGS					(REG_RW)
#define MDC_BURST_SIZE_LENGTH					(8)

#define MDC_DREQ_ENABLE_OFFSET					MDC_READ_PORT_CONFIG_OFFSET
#define MDC_DREQ_ENABLE_SHIFT					(1)
#define MDC_DREQ_ENABLE_MASK					(0x00000002)
#define MDC_DREQ_ENABLE_FLAGS					(REG_RW)
#define MDC_DREQ_ENABLE_LENGTH					(1)

#define MDC_READBACK_OFFSET						MDC_READ_PORT_CONFIG_OFFSET
#define MDC_READBACK_SHIFT						(0)
#define MDC_READBACK_MASK						(0x00000001)
#define MDC_READBACK_FLAGS						(REG_RW)
#define MDC_READBACK_LENGTH						(1)

/* -------------------- Register MDC_READ_ADDRESS -------------------- */

#define MDC_READ_ADDRESS_OFFSET					(0x008)

#define MDC_ADDR_R_OFFSET						MDC_READ_ADDRESS_OFFSET
#define MDC_ADDR_R_SHIFT						(0)
#define MDC_ADDR_MASK							(0xFFFFFFFF)
#define MDC_ADDR_FLAGS							(REG_RW)
#define MDC_ADDR_LENGTH							(32)

/* -------------------- Register MDC_WRITE_ADDRESS -------------------- */

#define MDC_WRITE_ADDRESS_OFFSET				(0x00C)

#define MDC_ADDR_W_OFFSET						MDC_WRITE_ADDRESS_OFFSET
#define MDC_ADDR_W_SHIFT						(0)
#define MDC_ADDR_W_MASK							(0xFFFFFFFF)
#define MDC_ADDR_W_FLAGS						(REG_RW)
#define MDC_ADDR_W_LENGTH						(32)

/* -------------------- Register MDC_TRANSFER_SIZE -------------------- */

#define MDC_TRANSFER_SIZE_OFFSET				(0x010)

#define MDC_CNT_OFFSET							MDC_TRANSFER_SIZE_OFFSET
#define MDC_CNT_SHIFT							(0)
#define MDC_CNT_MASK							(0x00FFFFFF)
#define MDC_CNT_FLAGS							(REG_RW)
#define MDC_CNT_LENGTH							(24)

/* -------------------- Register MDC_LIST_NODE_ADDRESS -------------------- */

#define MDC_LIST_NODE_ADDRESS_OFFSET			(0x014)

#define MDC_ADDR_S_OFFSET						MDC_LIST_NODE_ADDRESS_OFFSET
#define MDC_ADDR_S_SHIFT						(0)
#define MDC_ADDR_S_MASK							(0xFFFFFFFF)
#define MDC_ADDR_S_FLAGS						(REG_RW)
#define MDC_ADDR_S_LENGTH						(32)

/* -------------------- Register MDC_CMDS_PROCESSED -------------------- */

#define MDC_CMDS_PROCESSED_OFFSET				(0x018)

#define MDC_CMD_PROCESSED_OFFSET				MDC_CMDS_PROCESSED_OFFSET
#define MDC_CMD_PROCESSED_SHIFT					(16)
#define MDC_CMD_PROCESSED_MASK					(0x003F0000)
#define MDC_CMD_PROCESSED_FLAGS					(REG_RW)
#define MDC_CMD_PROCESSED_LENGTH				(6)

#define MDC_INT_ACTIVE_OFFSET					MDC_CMDS_PROCESSED_OFFSET
#define MDC_INT_ACTIVE_SHIFT					(8)
#define MDC_INT_ACTIVE_MASK						(0x00000100)
#define MDC_INT_ACTIVE_FLAGS					(REG_RW)
#define MDC_INT_ACTIVE_LENGTH					(1)

#define MDC_CMDS_DONE_OFFSET					MDC_CMDS_PROCESSED_OFFSET
#define MDC_CMDS_DONE_SHIFT						(0)
#define MDC_CMDS_DONE_MASK						(0x0000003F)
#define MDC_CMDS_DONE_FLAGS						(REG_RW)
#define MDC_CMDS_DONE_LENGTH					(6)


/* -------------------- Register MDC_CONTROL_AND_STATUS -------------------- */

#define MDC_CONTROL_AND_STATUS_OFFSET			(0x01C)

#define MDC_TAG_OFFSET							MDC_CONTROL_AND_STATUS_OFFSET
#define MDC_TAG_SHIFT							(24)
#define MDC_TAG_MASK							(0x0F000000)
#define MDC_TAG_FLAGS							(REG_RW)
#define MDC_TAG_LENGTH							(4)

#define MDC_CANCEL_OFFSET						MDC_CONTROL_AND_STATUS_OFFSET
#define MDC_CANCEL_SHIFT						(20)
#define MDC_CANCEL_MASK							(0x00100000)
#define MDC_CANCEL_FLAGS						(REG_RW)
#define MDC_CANCEL_LENGTH						(1)

#define MDC_DREQ_OFFSET							MDC_CONTROL_AND_STATUS_OFFSET
#define MDC_DREQ_SHIFT							(16)
#define MDC_DREQ_MASK							(0x00010000)
#define MDC_DREQ_FLAGS							(REG_RO)
#define MDC_DREQ_LENGTH							(1)

#define MDC_FIFO_DEPTH_OFFSET 					MDC_CONTROL_AND_STATUS_OFFSET
#define MDC_FIFO_DEPTH_SHIFT					(8)
#define MDC_FIFO_DEPTH_MASK						(0x00000100)
#define MDC_FIFO_DEPTH_FLAGS					(REG_RO)
#define MDC_FIFO_DEPTH_LENGTH 					(1)

#define MDC_LIST_EN_OFFSET						MDC_CONTROL_AND_STATUS_OFFSET
#define MDC_LIST_EN_SHIFT						(4)
#define MDC_LIST_EN_MASK						(0x00000010)
#define MDC_LIST_EN_FLAGS						(REG_RW)
#define MDC_LIST_EN_LENGTH						(1)

#define MDC_EN_OFFSET							MDC_CONTROL_AND_STATUS_OFFSET
#define MDC_EN_SHIFT							(0)
#define MDC_EN_MASK								(0x00000001)
#define MDC_EN_FLAGS							(REG_RW)
#define MDC_EN_LENGTH							(1)

/*!
*******************************************************************************

 This macro writes a value to a register in the DMAC register block.

 \param     C               Context pointer.
 \param     O               Register offset within the block.
 \param     V               32-bit value to be written.

*******************************************************************************/
#define GDMA_HAL_MDC_WRITE(C, O, V)  (*(volatile unsigned int *)( (C) + (O) ) = ((unsigned int)V))

/*!
*******************************************************************************

 This macro reads from a register in the DMAC register block.

 \param     C               Context pointer.
 \param     O               Register offset within the block.

 \return                    32-bit value read from register.

*******************************************************************************/
#define GDMA_HAL_MDC_READ(C, O)      (*(volatile unsigned int *)( (C) + (O) ))


IMG_RESULT	GDMA_HAL_SanityCheckTransferObject
(
	GDMA_sTransferObject *			psTransferObject
);

IMG_RESULT	GDMA_HAL_ProgramMDCRegs
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	GDMA_sLinkedListDescriptor *	psRegsPointer,
	GDMA_sTransferObject *			psTransferObject,
	IMG_BOOL						bListElement,
	IMG_BOOL						bEnableInterrupt,
	IMG_BOOL						bEnableTransfer,
	IMG_UINT32						ui32NextNodeAddress
);

IMG_RESULT	GDMA_HAL_SetLinkedListDescriptor
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	IMG_UINT32						ui32DescriptorIndex,
	IMG_BOOL						bFirstElementOfList,
	GDMA_sTransferObject *			psTransferObject,
	IMG_UINT32						ui32NextElementIndex
)
{
	IMG_RESULT								rResult;
	IMG_UINT32								ui32NextNodeAddress;
	IMG_BOOL								bEnableInterrupt;
	GDMA_sContext						*	psContext;

	IMG_ASSERT( psBlockDesc );
	IMG_ASSERT( psBlockDesc->pvAPIContext );

	psContext = (GDMA_sContext *)psBlockDesc->pvAPIContext;

	if ( ui32DescriptorIndex >= psContext->sLinkedListStatus.ui32NoOfListElements )
	{
		return IMG_ERROR_VALUE_OUT_OF_RANGE;
	}

	IMG_ASSERT ( psContext->sLinkedListStatus.pasListElements != IMG_NULL );

	/* If details of a new transfer are provided, then clear entire linked list element. 	*/
	/* If they are not provided, then we are only modifying the 'next list element' pointer */
	/* in which case don't destroy the previously set up descriptor.						*/
	if ( psTransferObject != IMG_NULL )
	{
		psContext->sLinkedListStatus.pasListElements[ui32DescriptorIndex].psTransferObject = psTransferObject;
		IMG_MEMSET ( &(psContext->sLinkedListStatus.pasListElements[ui32DescriptorIndex].sLinkedListDescriptor), 0, sizeof(GDMA_sLinkedListDescriptor) );
	}

	if ( psTransferObject != IMG_NULL )
	{
		rResult = GDMA_HAL_SanityCheckTransferObject ( psTransferObject );

		if ( rResult != IMG_SUCCESS )
		{
			return rResult;
		}
	}

	ui32NextNodeAddress	= (IMG_UINT32) ((ui32NextElementIndex == GDMA_HAL__END_OF_LINKED_LIST) ? 0x00000000 : &(psContext->sLinkedListStatus.pasListElements[ui32NextElementIndex].sLinkedListDescriptor));

	/* Don't enable interrupt for the first element of the list - each 'list element loaded' interrupt 					*/
	/* is used to free the previous element, so we don't want an interrupt when the first element is loaded. There is	*/
	/* an additional interrupt when the whole list has finished, which frees the last element, but we still need the 	*/
	/* interrupt when the last element is loaded, as this frees the previous element.									*/
	if (bFirstElementOfList == IMG_TRUE)
	{
		bEnableInterrupt = IMG_FALSE;
	}
	else
	{
		bEnableInterrupt = IMG_TRUE;
	}

	rResult = GDMA_HAL_ProgramMDCRegs ( psBlockDesc,
					 				    &(psContext->sLinkedListStatus.pasListElements[ui32DescriptorIndex].sLinkedListDescriptor),
							  			psTransferObject,
							  			IMG_TRUE,
							  			bEnableInterrupt,
							  			IMG_TRUE,
							  			ui32NextNodeAddress );

	if ( rResult != IMG_SUCCESS )
	{
		return rResult;
	}

	return IMG_SUCCESS;
}


IMG_RESULT GDMA_HAL_ReadAndClearInterrupts
(
	ioblock_sBlockDescriptor	*	psBlockDesc
)
{
	IMG_UINT32	ui32CommandsProcessedReg;
	IMG_UINT32	ui32CommandsProcessedCount;

	IMG_ASSERT( psBlockDesc );

	ui32CommandsProcessedReg = GDMA_HAL_MDC_READ( psBlockDesc->ui32Base, MDC_CMDS_PROCESSED_OFFSET );

	/* Update commands processed count */
	ui32CommandsProcessedCount = (ui32CommandsProcessedReg & MDC_CMD_PROCESSED_MASK) >> MDC_CMD_PROCESSED_SHIFT;
	ui32CommandsProcessedCount ++;

	if ( ui32CommandsProcessedCount >= (1 << MDC_CMD_PROCESSED_LENGTH) )
	{
		ui32CommandsProcessedCount = 0;
	}

	ui32CommandsProcessedReg = (ui32CommandsProcessedCount << MDC_CMD_PROCESSED_SHIFT);

	GDMA_HAL_MDC_WRITE ( psBlockDesc->ui32Base, MDC_CMDS_PROCESSED_OFFSET, ui32CommandsProcessedReg );

	return IMG_SUCCESS;
}


IMG_RESULT GDMA_HAL_DisableHardware
(
	ioblock_sBlockDescriptor	*	psBlockDesc
)
{
	GDMA_sContext	*	psContext;

	IMG_ASSERT( psBlockDesc );
	IMG_ASSERT( psBlockDesc->pvAPIContext );

	psContext = (GDMA_sContext *)psBlockDesc->pvAPIContext;

	/* The MDC has no support for suspending an operation once it's running */
	if ((psContext->eState == GDMA_RUNNING ) ||
		(psContext->eState == GDMA_FINISHING ))
	{
		return IMG_ERROR_NOT_SUPPORTED;
	}

    return IMG_SUCCESS;
}


IMG_RESULT GDMA_HAL_EnableTransfer
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	IMG_BOOL						bListTransfer
)
{
	IMG_UINT32	ui32RegVal;

	IMG_ASSERT( psBlockDesc );

	ui32RegVal = GDMA_HAL_MDC_READ ( psBlockDesc->ui32Base, MDC_CONTROL_AND_STATUS_OFFSET );
	IMG_ASSERT (( ui32RegVal & (MDC_LIST_EN_MASK | MDC_EN_MASK) ) == 0);	/* Neither list nor register enable bits should currently be set */

	/* Set the appropriate start bit */
	ui32RegVal |= ((bListTransfer == IMG_TRUE) ? MDC_LIST_EN_MASK : MDC_EN_MASK);

	/* Now write the register with the enable bit set */
	GDMA_HAL_MDC_WRITE ( psBlockDesc->ui32Base, MDC_CONTROL_AND_STATUS_OFFSET, ui32RegVal );

	return IMG_SUCCESS;
}


IMG_RESULT GDMA_HAL_GetCurrentTransferCount
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	IMG_UINT32 *					pui32Count
)
{
	IMG_ASSERT( psBlockDesc );
	IMG_ASSERT( pui32Count );

	*pui32Count = GDMA_HAL_MDC_READ( psBlockDesc->ui32Base, MDC_TRANSFER_SIZE_OFFSET ) & MDC_CNT_MASK;

	return IMG_SUCCESS;
}

IMG_RESULT GDMA_HAL_ResetHardware
(
	ioblock_sBlockDescriptor	*	psBlockDesc
)
{
	IMG_ASSERT( psBlockDesc );

	GDMA_HAL_MDC_WRITE(psBlockDesc->ui32Base, MDC_CONTROL_AND_STATUS_OFFSET, MDC_CANCEL_MASK);	/* Cancels any outstanding operation */
	GDMA_HAL_MDC_WRITE(psBlockDesc->ui32Base, MDC_GENERAL_CONFIG_OFFSET, 0x00000000);
	GDMA_HAL_MDC_WRITE(psBlockDesc->ui32Base, MDC_READ_PORT_CONFIG_OFFSET, 0x00000000);
	GDMA_HAL_MDC_WRITE(psBlockDesc->ui32Base, MDC_READ_ADDRESS_OFFSET, 0x00000000);
	GDMA_HAL_MDC_WRITE(psBlockDesc->ui32Base, MDC_WRITE_ADDRESS_OFFSET, 0x00000000);
	GDMA_HAL_MDC_WRITE(psBlockDesc->ui32Base, MDC_TRANSFER_SIZE_OFFSET, 0x00000000);
	GDMA_HAL_MDC_WRITE(psBlockDesc->ui32Base, MDC_CONTROL_AND_STATUS_OFFSET, 0x00000000);

	return IMG_SUCCESS;
}

IMG_RESULT GDMA_HAL_PrepareLinkedListTransfer
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	IMG_UINT32						ui32FirstListElementIndex
)
{
	GDMA_sLinkedListDescriptor			sTempRegs;
	GDMA_sWrappedLinkedListDescriptor *	pasListElements;
	IMG_RESULT							rResult;
	GDMA_sContext					*	psContext;

	IMG_ASSERT( psBlockDesc );
	IMG_ASSERT( psBlockDesc->pvAPIContext );

	psContext = (GDMA_sContext *)psBlockDesc->pvAPIContext;

	if ( ui32FirstListElementIndex >= psContext->sLinkedListStatus.ui32NoOfListElements )
	{
		return IMG_ERROR_VALUE_OUT_OF_RANGE;
	}

	pasListElements = psContext->sLinkedListStatus.pasListElements;
	IMG_ASSERT ( pasListElements != IMG_NULL );

	IMG_MEMSET ( &sTempRegs, 0, sizeof(GDMA_sLinkedListDescriptor) );
	rResult = GDMA_HAL_ProgramMDCRegs ( psBlockDesc,
										&sTempRegs,
										IMG_NULL,
										IMG_TRUE,		/* List element */
										IMG_TRUE,		/* We need to enable the interrupt in the register proper, or we'll get no interrupts at all */
										IMG_FALSE,		/* Enable transfer */
										(IMG_UINT32 ) &(pasListElements[ui32FirstListElementIndex].sLinkedListDescriptor) );

	if ( rResult != IMG_SUCCESS )
	{
		return rResult;
	}

	/* Now write the registers proper - only need to write sufficient registers to enable the h/w to load the first list element */
	GDMA_HAL_MDC_WRITE ( psBlockDesc->ui32Base, MDC_GENERAL_CONFIG_OFFSET, sTempRegs.ui32GenConfReg );
	GDMA_HAL_MDC_WRITE ( psBlockDesc->ui32Base, MDC_LIST_NODE_ADDRESS_OFFSET, sTempRegs.ui32ListNodeAddrReg );

	return IMG_SUCCESS;
}

IMG_RESULT GDMA_HAL_PrepareSingleShotTransfer
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	GDMA_sTransferObject		*	psTransferObject
)
{
	IMG_RESULT					rResult;
	GDMA_sLinkedListDescriptor 	sTempRegs;

	IMG_ASSERT( psBlockDesc );
	IMG_ASSERT( psTransferObject );

    /* Start the single-shot transfer using the parameters in ioPars and
       those stored in the singleConfig element of the channel configuration */
	rResult = GDMA_HAL_SanityCheckTransferObject ( psTransferObject );
	if ( rResult != IMG_SUCCESS )
	{
		return rResult;
	}

	// Clear all registers
	IMG_MEMSET( &sTempRegs, 0, sizeof( GDMA_sLinkedListDescriptor ) );

	rResult = GDMA_HAL_ProgramMDCRegs ( psBlockDesc,
										&sTempRegs,
										psTransferObject,
										IMG_FALSE,		/* List element */
										IMG_TRUE,		/* Enable interrupt */
										IMG_FALSE,		/* Enable transfer */
										0 );			/* Next list node address - unused here */

	if ( rResult != IMG_SUCCESS )
	{
		return rResult;
	}

	/* Now write the registers proper - only need to write sufficient registers to enable the h/w to load the first list element */
	GDMA_HAL_MDC_WRITE ( psBlockDesc->ui32Base, MDC_GENERAL_CONFIG_OFFSET, sTempRegs.ui32GenConfReg );
	GDMA_HAL_MDC_WRITE ( psBlockDesc->ui32Base, MDC_READ_PORT_CONFIG_OFFSET, sTempRegs.ui32ReadPortConfReg );
	GDMA_HAL_MDC_WRITE ( psBlockDesc->ui32Base, MDC_READ_ADDRESS_OFFSET, sTempRegs.ui32ReadAddrReg );
	GDMA_HAL_MDC_WRITE ( psBlockDesc->ui32Base, MDC_WRITE_ADDRESS_OFFSET, sTempRegs.ui32WriteAddrReg );
	GDMA_HAL_MDC_WRITE ( psBlockDesc->ui32Base, MDC_TRANSFER_SIZE_OFFSET, sTempRegs.ui32TransferSizeReg );
	GDMA_HAL_MDC_WRITE ( psBlockDesc->ui32Base, MDC_LIST_NODE_ADDRESS_OFFSET, 0 );
	GDMA_HAL_MDC_WRITE ( psBlockDesc->ui32Base, MDC_CONTROL_AND_STATUS_OFFSET, sTempRegs.ui32ControlStatusReg );

    return IMG_SUCCESS;
}

IMG_RESULT	GDMA_HAL_ProgramMDCRegs
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	GDMA_sLinkedListDescriptor	*	psRegsPointer,
	GDMA_sTransferObject		*	psTransferObject,
	IMG_BOOL						bListElement,
	IMG_BOOL						bEnableInterrupt,
	IMG_BOOL						bEnableTransfer,
	IMG_UINT32						ui32NextNodeAddress
)
{
	IMG_UINT32			ui32RegVal;
	IMG_UINT32			ui32HWVal;
	IMG_UINT32			ui32i;
	IMG_UINT32			ui32TempVal;
	GDMA_sContext	*	psContext;

	IMG_ASSERT( psBlockDesc );
	IMG_ASSERT( psBlockDesc->pvAPIContext );
	IMG_ASSERT( psBlockDesc->psSystemDescriptor );
	IMG_ASSERT( psRegsPointer );

	psContext = (GDMA_sContext *)psBlockDesc->pvAPIContext;

	/* Always set up next node address, even if 'psTransferObject' is null */

	/*********************/
	/* List node address */
	/*********************/
	if (( psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical != IMG_NULL ) &&
		( ui32NextNodeAddress != 0 ))	/* For MDC, address zero is a special case, indicating end of list */
	{
		ui32RegVal = psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical ( ui32NextNodeAddress );
	}
	else
	{
		ui32RegVal = ui32NextNodeAddress;
	}

	psRegsPointer->ui32ListNodeAddrReg = ui32RegVal;

	/***************************/
	/* General config register */
	/***************************/

	/* Even if no transfer object is supplied, we can still set up some of this register - this is required to kick off linked list */
	/* transfers.																													*/

	ui32RegVal = psRegsPointer->ui32GenConfReg;

	/* LIST_IEN */
	if (( bListElement == IMG_TRUE) && ( bEnableInterrupt == IMG_TRUE ))
	{
		ui32RegVal |= MDC_LIST_IEN_MASK;
	}

	/* IEN */
	if (( bListElement == IMG_FALSE) && ( bEnableInterrupt == IMG_TRUE ))
	{
		ui32RegVal |= MDC_IEN_MASK;
	}

	/* LEVEL INT - always set */
	ui32RegVal |= MDC_LEVEL_INT_MASK;

	/* CHANNEL */
	if (psContext->ui8ChannelGroup > ((1 << MDC_CHANNEL_LENGTH)-1))
	{
		return IMG_ERROR_VALUE_OUT_OF_RANGE;
	}
	else
	{
		ui32RegVal &= ~MDC_CHANNEL_MASK;
		ui32RegVal |= (psContext->ui8ChannelGroup << MDC_CHANNEL_SHIFT);
	}

	/* ACC_DEL - always zero */

	/* WAIT_PACK - always zero */

	/* PHYSICAL_W - always set */
	ui32RegVal |= MDC_PHYSICAL_W_MASK;

	/* PHYSICAL_R - always set */
	ui32RegVal |= MDC_PHYSICAL_R_MASK;

	psRegsPointer->ui32GenConfReg = ui32RegVal;

	if ( psTransferObject != IMG_NULL )
	{
		/* BSWAP */
		if ( psTransferObject->bReverseEndianism == IMG_TRUE )
		{
			ui32RegVal |= MDC_BSWAP_MASK;
		}

		/* WAIT_UNPACK  */
        if (psTransferObject->bWaitUnpack)
        {
            ui32RegVal |= MDC_WAIT_UNPACK_MASK;
        }
        else
        {
            ui32RegVal &= ~MDC_WAIT_UNPACK_MASK;
        }

		/* INC_W */
		if ( psTransferObject->bIncWritePointer == IMG_TRUE )
		{
			ui32RegVal |= MDC_INC_W_MASK;
		}

		/* INC_R */
		if ( psTransferObject->bIncReadPointer == IMG_TRUE )
		{
			ui32RegVal |= MDC_INC_R_MASK;
		}

		/* WIDTH_W */
		/* Must be a power of two */
		if ( psTransferObject->ui32WriteWidth == 0 )
		{
			return IMG_ERROR_VALUE_OUT_OF_RANGE;
		}
		else
		if ((psTransferObject->ui32WriteWidth & (psTransferObject->ui32WriteWidth - 1)) != 0)
		{
			return IMG_ERROR_VALUE_OUT_OF_RANGE;
		}
		else
		{
			ui32i = 0;
			ui32TempVal = psTransferObject->ui32WriteWidth;
			while ( ui32TempVal > 0 )
			{
				ui32i++;
				ui32TempVal = ui32TempVal>>1;
			}

			IMG_ASSERT ( ui32i > 0 );
			ui32HWVal = (ui32i-1);
			if (ui32HWVal > ((1 << MDC_WIDTH_W_LENGTH)-1))
			{
				return IMG_ERROR_VALUE_OUT_OF_RANGE;
			}
			else
			{
				ui32RegVal |= (ui32HWVal << MDC_WIDTH_W_SHIFT);
			}
		}

		/* WIDTH_R */
		/* Must be a power of two */
		if ( psTransferObject->ui32ReadWidth == 0 )
		{
			return IMG_ERROR_VALUE_OUT_OF_RANGE;
		}
		else
		if ((psTransferObject->ui32ReadWidth & (psTransferObject->ui32ReadWidth - 1)) != 0)
		{
			return IMG_ERROR_VALUE_OUT_OF_RANGE;
		}
		else
		{
			ui32i = 0;
			ui32TempVal = psTransferObject->ui32ReadWidth;
			while ( ui32TempVal > 0 )
			{
				ui32i++;
				ui32TempVal = ui32TempVal>>1;
			}

			IMG_ASSERT ( ui32i > 0 );
			ui32HWVal = (ui32i-1);
			if (ui32HWVal > ((1 << MDC_WIDTH_R_LENGTH)-1))
			{
				return IMG_ERROR_VALUE_OUT_OF_RANGE;
			}
			else
			{
				ui32RegVal |= (ui32HWVal << MDC_WIDTH_R_SHIFT);
			}
		}

		psRegsPointer->ui32GenConfReg = ui32RegVal;

		/********************/
		/* Read port config */
		/********************/
		ui32RegVal = 0;

		/* STHREAD - always zero */

		/* RTHREAD - always zero */

		/* PRIORITY */
		if ( psTransferObject->ui8Priority >= 128 )
		{
			ui32RegVal |= (1 << MDC_PRIORITY_SHIFT);
		}

		/* WTHREAD - always zero */

		/* HOLD_OFF */
		if( psTransferObject->ui32HoldOff >= (1 << MDC_HOLD_OFF_LENGTH))
		{
			return IMG_ERROR_VALUE_OUT_OF_RANGE;
		}
		else
		{
			ui32RegVal |= (psTransferObject->ui32HoldOff << MDC_HOLD_OFF_SHIFT);
		}

		/* BURST_SIZE */
		if ( psTransferObject->ui32BurstSize >= ((1 << MDC_BURST_SIZE_LENGTH)-1) )
		{
			return IMG_ERROR_VALUE_OUT_OF_RANGE;
		}
		else
		{
			ui32RegVal |= (psTransferObject->ui32BurstSize - 1) << MDC_BURST_SIZE_SHIFT;
		}

		/* DREQ_ENABLE - always set */
		ui32RegVal |= MDC_DREQ_ENABLE_MASK;

		/* READBACK - always zero */

		psRegsPointer->ui32ReadPortConfReg = ui32RegVal;

		/****************/
		/* Read address */
		/****************/
		if ( psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical != IMG_NULL )
		{
			ui32RegVal = psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical ( (IMG_UINT32) psTransferObject->pui8ReadPointer );
		}
		else
		{
			ui32RegVal = (IMG_UINT32) psTransferObject->pui8ReadPointer;
		}

		psRegsPointer->ui32ReadAddrReg = ui32RegVal;

		/*****************/
		/* Write address */
		/*****************/
		if ( psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical != IMG_NULL )
		{
			ui32RegVal = psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical ( (IMG_UINT32) psTransferObject->pui8WritePointer );
		}
		else
		{
			ui32RegVal = (IMG_UINT32) psTransferObject->pui8WritePointer;
		}

		psRegsPointer->ui32WriteAddrReg = ui32RegVal;

		/*****************/
		/* Transfer size */
		/*****************/
		psRegsPointer->ui32TransferSizeReg = (psTransferObject->ui32SizeInBytes - 1);

		/**********************/
		/* Commands processed */
		/**********************/
		psRegsPointer->ui32ListNodeAddrReg = 0;

		/**********************/
		/* Control and status */
		/**********************/
		ui32RegVal = 0;
		if ( bEnableTransfer == IMG_TRUE )
		{
			ui32RegVal |= (bListElement == IMG_TRUE) ? (MDC_LIST_EN_MASK | MDC_EN_MASK): MDC_EN_MASK;
		}

		psRegsPointer->ui32ControlStatusReg = ui32RegVal;
	}

	return IMG_SUCCESS;
}

IMG_RESULT	GDMA_HAL_SanityCheckTransferObject
(
	GDMA_sTransferObject *			psTransferObject
)
{
	IMG_ASSERT ( psTransferObject != IMG_NULL );

	/* Check sanity of other transfer object members */
	if (( psTransferObject->pui8ReadPointer == IMG_NULL ) ||
		( psTransferObject->pui8WritePointer == IMG_NULL ) ||
		( psTransferObject->ui32ReadWidth == 0 ) ||
		( psTransferObject->ui32WriteWidth == 0 ) ||
		( psTransferObject->ui32SizeInBytes == 0 ) ||
		( psTransferObject->ui32BurstSize == 0 ))
	{
		return IMG_ERROR_INVALID_PARAMETERS;
	}

	return IMG_SUCCESS;
}

IMG_RESULT GDMA_HAL_GetInterruptStatus
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	IMG_BOOL *						pbInterrupt
)
{
	IMG_ASSERT( psBlockDesc );

	*pbInterrupt = (GDMA_HAL_MDC_READ( psBlockDesc->ui32Base, MDC_CMDS_PROCESSED_OFFSET ) & MDC_INT_ACTIVE_MASK) >> MDC_INT_ACTIVE_SHIFT;

	return IMG_SUCCESS;
}
