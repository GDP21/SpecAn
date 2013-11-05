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
#include "gdma_sysbus.h"

#include "dmac_reg_defs.h"

/*!
*******************************************************************************

 This macro writes a value to a register in the DMAC register block.

 \param     C               Context pointer.
 \param     O               Register offset within the block.
 \param     V               32-bit value to be written.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_WRITE(C, O, V)  ( *(volatile unsigned int *)( (C) + (O) ) = ( (unsigned int)V ) )

/*!
*******************************************************************************

 This macro reads from a register in the DMAC register block.

 \param     C               Context pointer.
 \param     O               Register offset within the block.

 \return                    32-bit value read from register.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_READ(C, O)      (*(volatile unsigned int *)( (C) + (O) ))

/*!
*******************************************************************************

 This macro resets the registers for a DMA channel.

 \param     channel         DMA channel to reset.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_RESET_CHANNEL(context)                                          \
{                                                                                      \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_COUNTN_REG_OFFSET,   DMAC_PERIP_SRST_MASK); \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_ISTATN_REG_OFFSET,   0x00000000);           \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_SETUPN_REG_OFFSET,   0x00000000);           \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_PERADDRN_REG_OFFSET, 0x00000000);           \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_PERADDRN2_REG_OFFSET,0x00000000);           \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_COUNTN_REG_OFFSET,   0x00000000);           \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_2D_MODEN_REG_OFFSET, 0x00000000);           \
}

/*!
*******************************************************************************

 This macro configures the DMAC for the given channel.

 \param     channel         DMA channel to configure.
 \param     countReg        Value to set count register.
 \param     periphAddrReg   Value to set peripheral register.
 \param     setupReg        Value to set set-up register.
 \param     mode2DReg       Value for 2D mode register.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_SET_CONFIG(context, countReg, periphAddrReg, periphAddrReg2, setupReg, mode2DReg)  \
{                                                                                                         \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_COUNTN_REG_OFFSET,   (countReg));                              \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_PERADDRN_REG_OFFSET, (periphAddrReg));                         \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_PERADDRN2_REG_OFFSET,(periphAddrReg2));                        \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_SETUPN_REG_OFFSET,   (setupReg));                              \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_2D_MODEN_REG_OFFSET, (mode2DReg));                             \
}

/*!
*******************************************************************************

 This macro reads the current configuration of the DMAC for the given channel.

 \param     channel         DMA channel to configure.
 \param     countReg        Updated with the value from the count register.
 \param     periphAddrReg   Updated with the value from the peripheral register.
 \param     setupReg        Updated with the value from the set-up register.
 \param     mode2DReg       Updated with the value from the 2D mode register.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_GET_CONFIG(context, countReg, periphAddrReg, periphAddrReg2, setupNReg, mode2DReg) \
{                                                                                                         \
    /* Read the configuration */                                                                          \
    (countReg)      = GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_COUNTN_REG_OFFSET);                           \
    (periphAddrReg) = GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_PERADDRN_REG_OFFSET);                         \
    (periphAddrReg2)= GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_PERADDRN2_REG_OFFSET);                        \
    (setupNReg)     = GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_SETUPN_REG_OFFSET);                           \
    (mode2DReg)     = GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_2D_MODEN_REG_OFFSET);                         \
}

/*!
*******************************************************************************

 This macro sets the DMAC setup register for the given channel.

 \param     channel         DMA channel to configure.
 \param     startAddress    Address to write to the register.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_SET_SETUPN(context, startAddress)   \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_SA_OFFSET, (((startAddress) << DMAC_PERIP_SA_SHIFT) & DMAC_PERIP_SA_MASK))


/*!
*******************************************************************************

 This macro reads the current value from the setup register for the given
 channel.

 \param     channel         DMA channel to configure.

 \return                    Value currently in the setup register.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_GET_SETUPN(context) \
    ((GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_SA_OFFSET) & DMAC_PERIP_SA_MASK) >> DMAC_PERIP_SA_SHIFT)


/*!
*******************************************************************************

 This macro sets the DMAC count register for the given channel.

 \param     channel         DMA channel to configure.
 \param     count           Count configuration to write to the register.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_SET_COUNTN(context, count)   \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_COUNTN_REG_OFFSET, (count))


/*!
*******************************************************************************

 This macro reads the current value from the count register for the given
 channel.

 \param     channel         DMA channel to configure.

 \return                    Value currently in the count register.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_GET_COUNTN(context) \
    GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_COUNTN_REG_OFFSET)


/*!
*******************************************************************************

 This macro reads the current values from the peripheral address register for
 the given channel.

 \param     channel         DMA channel to configure.
 \param     periphAddr      Updated with the address stored in the register.
 \param     burstSize       Updated with the burst size stored in the register.
 \param     inc             Updated with the peripheral address increment mode.
 \param     delay           Updated with the access delay.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_GET_PER_ADDRSN(context, periphAddr, burstSize, inc, delay)                                                   \
{                                                                                                                                   \
    /* Read the peripheral register fields */                                                                                       \
    (periphAddr) = (GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_ADDR_OFFSET)    & DMAC_PERIP_ADDR_MASK)    >> DMAC_PERIP_ADDR_SHIFT;      \
    (burstSize)  = (GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_BURST_OFFSET)   & DMAC_PERIP_BURST_MASK)   >> DMAC_PERIP_BURST_SHIFT;     \
    (inc)        = (GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_INCR_OFFSET)    & DMAC_PERIP_INCR_MASK)    >> DMAC_PERIP_INCR_SHIFT;      \
    (delay)      = (GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_ACC_DEL_OFFSET) & DMAC_PERIP_ACC_DEL_MASK) >> DMAC_PERIP_ACC_DEL_SHIFT;   \
}

/*!
*******************************************************************************

 This macro sets the 2D mode register for the given channel.

 \param     channel         DMA channel to configure.
 \param     mode            2D mode to set-up.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_SET_2D_MODE(context, mode)                       \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_2D_MODEN_REG_OFFSET, mode)   \

/*!
*******************************************************************************

 This macro gets the 2D mode register for the given channel.

 \param     channel         DMA channel to configure.
 \param     mode            Updated with the 2D mode stored in the register.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_GET_2D_MODE(context, mode)                           \
    (mode) = GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_2D_MODEN_REG_OFFSET);    \

/*!
*******************************************************************************

 This macro reads the DMAC IRQ status bits given by statusBits.
 A '1' in statusBits reads that bit position.

 \param     channel         DMA channel to configure.
 \param     statusBits      Bits to read in the interrupt status.

 \return                    Interrupt status register bits requested.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_GET_INT_STATUS_BITS(context, statusBits)   \
    (GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_ISTATN_REG_OFFSET) & (statusBits))

/*!
*******************************************************************************

 This macro clears the DMAC IRQ status bits given by statusBits.

 \param     channel         DMA channel to configure.
 \param     statusBits      Bits to clear in the interrupt status.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_CLEAR_INT_STATUS_BITS(context, statusBits) \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_ISTATN_REG_OFFSET,     \
          (GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_ISTATN_REG_OFFSET) & ~(statusBits)))

/*!
*******************************************************************************

 This macro enables the DMA for single-shot transfer.

 \param     channel         DMA channel to enable.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_ENABLE(context)                                              \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_EN_OFFSET,                               \
          (GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_EN_OFFSET) | DMAC_PERIP_EN_MASK))

/*!
*******************************************************************************

 This macro disables the DMA for single-shot transfer.

 \param     channel         DMA channel to disable.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_DISABLE(context)                                              \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_EN_OFFSET,                                \
          (GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_EN_OFFSET) & ~DMAC_PERIP_EN_MASK))

/*!
*******************************************************************************

 This macro enables the DMA for linked-list transfer.

 \param     channel         DMA channel to enable.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_LIST_ENABLE(context)                                                  \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_LIST_EN_OFFSET,                                   \
          (GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_LIST_EN_OFFSET) | DMAC_PERIP_LIST_EN_MASK))

/*!
*******************************************************************************

 This macro disables the DMA linked-list transfer.

 \param     channel         DMA channel to disable.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_LIST_DISABLE(context)                                                  \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_LIST_EN_OFFSET,                                    \
          (GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_LIST_EN_OFFSET) & ~DMAC_PERIP_LIST_EN_MASK))

/*!
*******************************************************************************

 This macro enables the DMA complete interrupt.

 \param     channel         DMA channel for which to enable interrupt.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_IRQ_ENABLE(context)                                           \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_IEN_OFFSET,                               \
          (GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_IEN_OFFSET) | DMAC_PERIP_IEN_MASK))

/*!
*******************************************************************************

 This macro disables the DMA complete interrupt.

 \param     channel         DMA channel for which to disable interrupt.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_IRQ_DISABLE(context)                                           \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_IEN_OFFSET,                                \
          (GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_IEN_OFFSET) & ~DMAC_PERIP_IEN_MASK))

/*!
*******************************************************************************

 This macro enables the DMA linked-list complete interrupt.

 \param     channel         DMA channel for which to enable interrupt.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_LIST_IRQ_ENABLE(context)                                                \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_LIST_IEN_OFFSET,                                    \
          (GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_LIST_IEN_OFFSET) | DMAC_PERIP_LIST_IEN_MASK))

/*!
*******************************************************************************

 This macro disables the DMA linked-list complete interrupt.

 \param     channel         DMA channel for which to disable interrupt.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_LIST_IRQ_DISABLE(context)                                                \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_LIST_IEN_OFFSET,                                     \
          (GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_LIST_IEN_OFFSET) & ~DMAC_PERIP_LIST_IEN_MASK))

/*!
*******************************************************************************

 This macro enables the all the DMA complete interrupts.

 \param     channel         DMA channel for which to enable interrupts.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_IRQ_ALL_ENABLE(context)                              \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_COUNTN_REG_OFFSET,               \
               (GDMA_HAL_SYSBUS_READ((context), DMAC_PERIP_COUNTN_REG_OFFSET) &  \
                          (DMAC_PERIP_LIST_IEN_MASK | DMAC_PERIP_IEN_MASK)))

/*!
*******************************************************************************

 This macro disables the all the DMA complete interrupts.

 \param     channel         DMA channel for which to disable interrupts.

*******************************************************************************/
#define GDMA_HAL_SYSBUS_REG_IRQ_ALL_DISABLE(context)                              \
    GDMA_HAL_SYSBUS_WRITE((context), DMAC_PERIP_COUNTN_REG_OFFSET,                \
               (GDMA_HAL_SYSBUS_READ((channel), DMAC_PERIP_COUNTN_REG_OFFSET) &   \
                          ~(DMAC_PERIP_LIST_IEN_MASK | DMAC_PERIP_IEN_MASK)))

IMG_RESULT	GDMA_HAL_SanityCheckTransferObject
(
	GDMA_sTransferObject *			psTransferObject,
	IMG_BOOL *						pbMemoryToPeripheral,
	IMG_UINT8 *						pui8PWWidthHWVal
);

IMG_RESULT	GDMA_HAL_SetLinkedListDescriptor
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	IMG_UINT32						ui32DescriptorIndex,
	IMG_BOOL						bFirstElementOfList,
	GDMA_sTransferObject		*	psTransferObject,
	IMG_UINT32						ui32NextElementIndex
)
{
	GDMA_sWrappedLinkedListDescriptor *	pasLinkedList;
	IMG_UINT32							ui32Address;
	IMG_UINT32							ui32PeripheralWidth;
	IMG_UINT32							ui32MemoryBusWidth;
	IMG_UINT32							ui32PeripheralAddress;
	IMG_UINT32							ui32MemoryAddress;
	IMG_UINT32							ui32FieldVal;
	IMG_RESULT							rResult;
	IMG_BOOL							bMemoryToPeripheral;
	IMG_UINT8							ui8PWWidthHWVal;
	GDMA_sContext					*	psContext;

	IMG_ASSERT( psBlockDesc );
	IMG_ASSERT( psBlockDesc->pvAPIContext );
	IMG_ASSERT( psBlockDesc->psSystemDescriptor );

	psContext = (GDMA_sContext *)psBlockDesc->pvAPIContext;

	if ( ui32DescriptorIndex >= psContext->sLinkedListStatus.ui32NoOfListElements )
	{
		return IMG_ERROR_VALUE_OUT_OF_RANGE;
	}

	pasLinkedList = psContext->sLinkedListStatus.pasListElements;
	IMG_ASSERT ( pasLinkedList != IMG_NULL );

	/* If details of a new transfer are provided, then clear entire linked list element. 	*/
	/* If they are not provided, then we are only modifying the 'next list element' pointer */
	/* in which case don't destroy the previously set up descriptor.						*/
	if ( psTransferObject != IMG_NULL )
	{
		pasLinkedList[ui32DescriptorIndex].psTransferObject = psTransferObject;
		IMG_MEMSET ( &(pasLinkedList[ui32DescriptorIndex].sLinkedListDescriptor), 0, sizeof(GDMA_sLinkedListDescriptor) );
	}

	/* Set up 'next element' pointer */
	if ( ui32NextElementIndex == GDMA_HAL__END_OF_LINKED_LIST )
	{
		/* Set 'end of list' flag */
		pasLinkedList[ui32DescriptorIndex].sLinkedListDescriptor.ui32Word1 |= DMAC_PERIP_LL_LIST_FIN_MASK;

		/* Clear 'next linked list element' pointer */
		pasLinkedList[ui32DescriptorIndex].sLinkedListDescriptor.ui32Word7 &= ~DMAC_PERIP_LL_LISTPTR_MASK;
	}
	else
	{
		if ( ui32NextElementIndex >= psContext->sLinkedListStatus.ui32NoOfListElements )
		{
			return IMG_ERROR_VALUE_OUT_OF_RANGE;
		}

		/* Clear 'end of list' flag */
		pasLinkedList[ui32DescriptorIndex].sLinkedListDescriptor.ui32Word1 &= ~DMAC_PERIP_LL_LIST_FIN_MASK;

		if ( psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical != IMG_NULL )
		{
			ui32Address = psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical ( (IMG_UINT32) &(pasLinkedList[ui32NextElementIndex].sLinkedListDescriptor) );
		}
		else
		{
			ui32Address = (IMG_UINT32) &(pasLinkedList[ui32NextElementIndex].sLinkedListDescriptor);
		}

		pasLinkedList[ui32DescriptorIndex].sLinkedListDescriptor.ui32Word7 &= ~DMAC_PERIP_LL_LISTPTR_MASK;
		pasLinkedList[ui32DescriptorIndex].sLinkedListDescriptor.ui32Word7 |= ui32Address >> DMAC_PERIP_LL_LISTPTR_BIT_SHIFT;
	}


	/* If provided, set up details of transfer */
	if ( psTransferObject != IMG_NULL )
	{
		rResult = GDMA_HAL_SanityCheckTransferObject ( 	psTransferObject,
														&bMemoryToPeripheral,
														&ui8PWWidthHWVal  );
		if ( rResult != IMG_SUCCESS )
		{
			return rResult;
		}
		else
		{
			/* Set up transfer */

			/**********/
			/* Word 0 */
			/**********/

			/* Byte swap */
			if ( psTransferObject->bReverseEndianism == IMG_TRUE )
			{
				pasLinkedList[ui32DescriptorIndex].sLinkedListDescriptor.ui32Word0 |= DMAC_PERIP_LL_BSWAP_MASK;
			}

			if ( bMemoryToPeripheral == IMG_TRUE )
			{
				/* Writing to peripheral */
				ui32PeripheralWidth = psTransferObject->ui32WriteWidth;
				ui32MemoryBusWidth	= psTransferObject->ui32ReadWidth;
				ui32PeripheralAddress = (IMG_UINT32) psTransferObject->pui8WritePointer;
				ui32MemoryAddress = (IMG_UINT32) psTransferObject->pui8ReadPointer;
			}
			else
			{
				/* Reading from peripheral */

				/* Direction of transfer */
				pasLinkedList[ui32DescriptorIndex].sLinkedListDescriptor.ui32Word0 |= DMAC_PERIP_LL_DIR_MASK;

				ui32PeripheralWidth = psTransferObject->ui32ReadWidth;
				ui32MemoryBusWidth	= psTransferObject->ui32WriteWidth;
				ui32PeripheralAddress = (IMG_UINT32) psTransferObject->pui8ReadPointer;
				ui32MemoryAddress = (IMG_UINT32) psTransferObject->pui8WritePointer;
			}

			/* Peripheral width */
			pasLinkedList[ui32DescriptorIndex].sLinkedListDescriptor.ui32Word0 |= (ui8PWWidthHWVal << DMAC_PERIP_LL_PW_SHIFT);

			/**********/
			/* Word 1 */
			/**********/

			/* 'LIST_FIN' set above */
			/* 'LIST_INT' not used */
			/* Not incrementing peripheral address */

			/* Length of transfer in PW units */
			if ( psTransferObject->ui32SizeInBytes < ui32PeripheralWidth )
			{
				return IMG_ERROR_VALUE_OUT_OF_RANGE;
			}

			ui32FieldVal = (psTransferObject->ui32SizeInBytes / ui32PeripheralWidth);
			if ( ui32FieldVal > ((1 << DMAC_PERIP_LL_LEN_LENGTH)-1) )
			{
				return IMG_ERROR_VALUE_OUT_OF_RANGE;
			}
			else
			{
				pasLinkedList[ui32DescriptorIndex].sLinkedListDescriptor.ui32Word1 |= ui32FieldVal;
			}

			/**********/
			/* Word 2 */
			/**********/

			/* Peripheral address - for system bus DMAC, this is programmed 'as is' */
			pasLinkedList[ui32DescriptorIndex].sLinkedListDescriptor.ui32Word2 = ui32PeripheralAddress;

			/**********/
			/* Word 3 */
			/**********/

			/* ACC_DEL set to zero */

			/* Burst size */
			/* Must be a multiple of peripheral width */
			if ( (psTransferObject->ui32BurstSize >> 2) > ((1 << DMAC_PERIP_LL_BURST_LENGTH)-1))
			{
				return IMG_ERROR_VALUE_OUT_OF_RANGE;
			}
			else
			{
				pasLinkedList[ui32DescriptorIndex].sLinkedListDescriptor.ui32Word3 |= ((psTransferObject->ui32BurstSize >> 2) << DMAC_PERIP_LL_BURST_SHIFT);
			}

			/* EXT_SA (Extended address) not used */

			/**********/
			/* Word 4 */
			/**********/

			/* 2D_MODE not used */
			/* REP_COUNT not used */

			/**********/
			/* Word 5 */
			/**********/

			/* LINE_ADD_OFF not used */
			/* ROW_LENGTH not used */

			/**********/
			/* Word 6 */
			/**********/

			/* System memory address */
			if ( psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical != IMG_NULL )
			{
				ui32Address = psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical ( ui32MemoryAddress );
			}
			else
			{
				ui32Address = ui32MemoryAddress;
			}

			pasLinkedList[ui32DescriptorIndex].sLinkedListDescriptor.ui32Word6 = ui32Address;

			/**********/
			/* Word 7 */
			/**********/

			/* Pointer to next list element is dealt with above */
		}
	}

	return IMG_SUCCESS;
}


IMG_RESULT GDMA_HAL_ReadAndClearInterrupts
(
	ioblock_sBlockDescriptor	*	psBlockDesc
)
{
	IMG_UINT32	ui32InterruptStatus;

	IMG_ASSERT( psBlockDesc );

	/* Get interrupt status */
	ui32InterruptStatus = GDMA_HAL_SYSBUS_REG_GET_INT_STATUS_BITS( psBlockDesc->ui32Base, DMAC_PERIP_ISTATN_REG_VALID_MASK );
	IMG_ASSERT ( ui32InterruptStatus != 0 );

	/* We should not ever receive the 'list_int' interrupt */
	IMG_ASSERT ( (ui32InterruptStatus & DMAC_PERIP_LIST_INT_MASK) == 0 );

	/* Clear all interrupts - this is done as a read/modify/write, because the hardware has no 'interrupt clear' 		*/
	/* register, so you just have to clear the bits in the interrupt status reg. This gives rise to potential missed	*/
	/* interrupts, if the h/w sets a bit between our reading the status register and clearing the set bits. In an		*/
	/* attempt to minimise the likelihood of this proble occurring, we perform a tight R-M-W of the status register,  	*/
	/* so there are only a few cycles in which the h/w generating a DMAC interrupt will be a problem (between the read 	*/
	/* and the write).																									*/
	GDMA_HAL_SYSBUS_REG_CLEAR_INT_STATUS_BITS ( psBlockDesc->ui32Base, ui32InterruptStatus );

	if ((ui32InterruptStatus & DMAC_PERIP_FIN_MASK) != 0)
	{
		/* GDMA model is only interested in 'block complete' interrupt */
		return IMG_SUCCESS;
	}
	else
	{
		return IMG_ERROR_UNEXPECTED_STATE;
	}
}


IMG_RESULT GDMA_HAL_DisableHardware
(
	ioblock_sBlockDescriptor	*	psBlockDesc
)
{
	IMG_ASSERT( psBlockDesc );

    GDMA_HAL_SYSBUS_REG_LIST_DISABLE(psBlockDesc->ui32Base);
    GDMA_HAL_SYSBUS_REG_DISABLE(psBlockDesc->ui32Base);

    return IMG_SUCCESS;
}


IMG_RESULT GDMA_HAL_EnableTransfer
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	IMG_BOOL						bListTransfer
)
{
	IMG_ASSERT( psBlockDesc );

	if ( bListTransfer == IMG_TRUE )
	{
		GDMA_HAL_SYSBUS_REG_LIST_ENABLE(psBlockDesc->ui32Base);
	}
	else
	{
		GDMA_HAL_SYSBUS_REG_ENABLE(psBlockDesc->ui32Base);
	}

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

	*pui32Count = (GDMA_HAL_SYSBUS_REG_GET_COUNTN(psBlockDesc->ui32Base) & DMAC_PERIP_CNT_MASK);

	return IMG_SUCCESS;
}

IMG_RESULT GDMA_HAL_ResetHardware
(
	ioblock_sBlockDescriptor	*	psBlockDesc
)
{
	IMG_ASSERT( psBlockDesc );

	GDMA_HAL_SYSBUS_REG_RESET_CHANNEL ( psBlockDesc->ui32Base );

	return IMG_SUCCESS;
}

IMG_RESULT GDMA_HAL_PrepareLinkedListTransfer
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	IMG_UINT32						ui32FirstListElementIndex
)
{
	GDMA_sWrappedLinkedListDescriptor *	pasLinkedList;
	IMG_UINT32							ui32Address;
	GDMA_sContext					*	psContext;

	IMG_ASSERT( psBlockDesc );
	IMG_ASSERT( psBlockDesc->pvAPIContext );

	psContext = (GDMA_sContext *)psBlockDesc->pvAPIContext;

	if ( ui32FirstListElementIndex >= psContext->sLinkedListStatus.ui32NoOfListElements )
	{
		return IMG_ERROR_VALUE_OUT_OF_RANGE;
	}

	pasLinkedList = psContext->sLinkedListStatus.pasListElements;
	IMG_ASSERT ( pasLinkedList != IMG_NULL );

	if ( psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical != IMG_NULL )
	{
		ui32Address = psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical ( (IMG_UINT32) &(pasLinkedList[ui32FirstListElementIndex].sLinkedListDescriptor) );
	}
	else
	{
		ui32Address = (IMG_UINT32) &(pasLinkedList[ui32FirstListElementIndex].sLinkedListDescriptor);
	}

	GDMA_HAL_SYSBUS_REG_SET_SETUPN(psBlockDesc->ui32Base, ui32Address);
	GDMA_HAL_SYSBUS_REG_SET_COUNTN(psBlockDesc->ui32Base, (DMAC_PERIP_LIST_IEN_MASK | DMAC_PERIP_IEN_MASK));

	return IMG_SUCCESS;
}

IMG_RESULT GDMA_HAL_PrepareSingleShotTransfer
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	GDMA_sTransferObject		*	psTransferObject
)
{
	IMG_RESULT	rResult;

	IMG_UINT32	ui32CountReg;
	IMG_UINT32	ui32PeripReg;
	IMG_UINT32	ui32PeripAddrReg;
	IMG_UINT32	ui32SetupReg;

	IMG_UINT32	ui32PeripheralAddress;
	IMG_UINT32	ui32MemoryAddress;
	IMG_BOOL	bMemoryToPeripheral;
	IMG_UINT8	ui8PWHWVal;
	IMG_UINT32	ui32TransferSizeInPWUnits;

	IMG_ASSERT( psBlockDesc );
	IMG_ASSERT( psTransferObject );

    /* Start the single-shot transfer using the parameters in ioPars and
       those stored in the singleConfig element of the channel configuration */
	rResult = GDMA_HAL_SanityCheckTransferObject ( psTransferObject,
												   &bMemoryToPeripheral,
												   &ui8PWHWVal );
	if ( rResult != IMG_SUCCESS )
	{
		return rResult;
	}

	/*************/
	/* COUNT reg */
	/*************/
	ui32CountReg = DMAC_PERIP_IEN_MASK | (ui8PWHWVal << DMAC_PERIP_PW_SHIFT);

	/* Byte swap */
	if ( psTransferObject->bReverseEndianism == IMG_TRUE )
	{
		ui32CountReg |= DMAC_PERIP_BSWAP_MASK;
	}

	/* Direction of travel */
	if ( bMemoryToPeripheral == IMG_FALSE )
	{
		ui32CountReg |= DMAC_PERIP_DIR_MASK;
	}

	/* Transfer size */
	if ( bMemoryToPeripheral == IMG_TRUE )
	{
		ui32TransferSizeInPWUnits = psTransferObject->ui32SizeInBytes / psTransferObject->ui32WriteWidth;
		ui32PeripheralAddress = (IMG_UINT32) psTransferObject->pui8WritePointer;
		ui32MemoryAddress = (IMG_UINT32) psTransferObject->pui8ReadPointer;
	}
	else
	{
		ui32TransferSizeInPWUnits = psTransferObject->ui32SizeInBytes / psTransferObject->ui32ReadWidth;
		ui32PeripheralAddress = (IMG_UINT32) psTransferObject->pui8ReadPointer;
		ui32MemoryAddress = (IMG_UINT32) psTransferObject->pui8WritePointer;
	}

	if ( ui32TransferSizeInPWUnits >= ((1<<DMAC_PERIP_CNT_LCNTGTH)-1) )
	{
		/* Transfer is too long for hardware */
		return IMG_ERROR_VALUE_OUT_OF_RANGE;
	}
	else
	{
		ui32CountReg |= ui32TransferSizeInPWUnits << DMAC_PERIP_CNT_SHIFT;
	}

	/*************/
	/* PERIP reg */
	/*************/
	ui32PeripReg = 0;

	/* Burst size */
	if ( (psTransferObject->ui32BurstSize >> 2) > ((1 << DMAC_PERIP_BURST_LENGTH)-1) )
	{
		/* Burst size exceeds capabilities of hardware */
		return IMG_ERROR_VALUE_OUT_OF_RANGE;
	}
	else
	{
		ui32PeripReg |= (psTransferObject->ui32BurstSize >> 2) << DMAC_PERIP_BURST_SHIFT;
	}

	/******************/
	/* PERIP ADDR reg */
	/******************/

	/* Peripheral address is written 'as is' */
	ui32PeripAddrReg = ui32PeripheralAddress;

	/*************/
	/* SETUP reg */
	/*************/
	if ( psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical != IMG_NULL )
	{
		ui32SetupReg = psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical ( ui32MemoryAddress );
	}
	else
	{
		ui32SetupReg = ui32MemoryAddress;
	}

    GDMA_HAL_SYSBUS_REG_SET_CONFIG( psBlockDesc->ui32Base, ui32CountReg, ui32PeripReg, ui32PeripAddrReg, ui32SetupReg, 0);

    return IMG_SUCCESS;
}

IMG_RESULT	GDMA_HAL_SanityCheckTransferObject
(
	GDMA_sTransferObject *			psTransferObject,
	IMG_BOOL *						pbMemoryToPeripheral,
	IMG_UINT8 *						pui8PWWidthHWVal
)
{
	IMG_UINT32	ui32PeripheralWidth;
	IMG_UINT32	ui32MemoryBusWidth;
	IMG_UINT32	ui32PeripheralAddress;

	IMG_ASSERT ( psTransferObject != IMG_NULL );

	/* System bus DMAC can only transfer between memory and a peripheral. The peripheral is accessed via a single FIFO port	*/
	/* and hence must have 'increment address' set to zero, and the memory should have 'increment address' set to non zero	*/
	/* (even if only a single transfer is being performed). For this reason, this HAL uses the 'increment address' values 	*/
	/* to establish which of the two addresses specified (read and write) is the peripheral and which is memory.			*/
	if ((( psTransferObject->bIncReadPointer == IMG_FALSE ) && ( psTransferObject->bIncWritePointer == IMG_FALSE ))
		||
		(( psTransferObject->bIncReadPointer == IMG_TRUE ) && ( psTransferObject->bIncWritePointer == IMG_TRUE )))
	{
		return IMG_ERROR_INVALID_PARAMETERS;
	}

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

	/* Direction of travel */
	if ( pbMemoryToPeripheral != IMG_NULL )
	{
		if ( psTransferObject->bIncReadPointer == IMG_FALSE )
		{
			/* Reading from peripheral */
			*pbMemoryToPeripheral = IMG_FALSE;
		}
		else
		{
			/* Writing to peripheral */
			*pbMemoryToPeripheral = IMG_TRUE;
		}
	}

	/* Peripheral width hardware value */
	if ( psTransferObject->bIncReadPointer == IMG_FALSE )
	{
		/* Reading from peripheral */
		ui32PeripheralWidth = psTransferObject->ui32ReadWidth;
		ui32MemoryBusWidth = psTransferObject->ui32WriteWidth;
		ui32PeripheralAddress = (IMG_UINT32) psTransferObject->pui8ReadPointer;
	}
	else
	{
		/* Writing to peripheral */
		ui32PeripheralWidth = psTransferObject->ui32WriteWidth;
		ui32MemoryBusWidth = psTransferObject->ui32ReadWidth;
		ui32PeripheralAddress = (IMG_UINT32) psTransferObject->pui8WritePointer;
	}

	if ( pui8PWWidthHWVal != IMG_NULL )
	{
		switch ( ui32PeripheralWidth )	/* Peripheral width in bytes */
		{
			case 1:
			{
				*pui8PWWidthHWVal = (IMG_UINT8) 2;
				break;
			}

			case 2:
			{
				*pui8PWWidthHWVal = (IMG_UINT8) 1;
				break;
			}

			case 4:
			{
				*pui8PWWidthHWVal = (IMG_UINT8) 0;
				break;
			}

			default:
			{
				return IMG_ERROR_INVALID_PARAMETERS;
				break;
			}
		}
	}

	/* Transfer length */
	if (( psTransferObject->ui32SizeInBytes % ui32PeripheralWidth ) != 0)
	{
		/* Transfer must be a multiple of peripheral width */
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

	*pbInterrupt = (GDMA_HAL_SYSBUS_REG_GET_INT_STATUS_BITS( psBlockDesc->ui32Base, DMAC_PERIP_ISTATN_REG_VALID_MASK ) & DMAC_PERIP_FIN_MASK) >> DMAC_PERIP_FIN_SHIFT;

	return IMG_SUCCESS;
}
