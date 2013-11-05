/*!
*******************************************************************************
  file   saturn_desc.c

  brief  Saturn IO Block Descriptors

  author Imagination Technologies

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

*******************************************************************************/

/*
******************************************************************************
 Modifications :-

 $Log: saturn_desc.c,v $

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 


******************************************************************************/


#define METAG_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include <MeOS.h>

#include <img_defs.h>
#include <ioblock_defs.h>

#include <sys_util.h>
#include <system.h>

#define DECL_DEFAULT_BLOCK( _index, _baseAddress, _interruptBit, _dmaRead, _dmaWrite )							\
	{																											\
		_index,														/* Index							*/		\
		_baseAddress,												/* Base Address						*/		\
		_interruptBit,												/* Int trigger num					*/		\
		HWLEVELEXT_NON_LATCHED,										/* Int type							*/		\
		_dmaRead,													/* DMA read bind index				*/		\
		_dmaWrite,													/* DMA write bind index				*/		\
		0,															/* Burst size (hardcoded in driver)	*/		\
		IMG_NULL,													/* API context - set by API			*/		\
		IMG_NULL,													/* Block specific HW descriptor		*/		\
		&IMG_asProcDesc,											/* System descriptor				*/		\
		&g_sMTPIntDesc,												/* Interrupt Descriptor				*/		\
		&QIO_MTPIVDesc,												/* QIO_IVDESC_T						*/		\
		{ 0 },														/* Device ISR info - set by API		*/		\
		{ 0 },														/* Int mask array - set by API		*/		\
	}

QIO_IVDESC_T    QIO_MTPIVDesc =
{
    {	HWSTATEXT, HWSTATEXT2, HWSTATEXT4, HWSTATEXT6	},
    {	HWVEC0EXT, HWVEC20EXT, HWVEC40EXT, HWVEC60EXT	},
    HWVECnEXT_STRIDE
};

ioblock_sSystemDescriptor	IMG_asProcDesc =
{
	IMG_NULL,
	IMG_NULL,
	(ioblock_fn_DMACChannelBind *)&SYS_BindDMACChannel,
	(ioblock_fn_FlushCache *)&SYS_FlushCache
};

ioblock_sInterruptBlock	g_sMTPIntDesc =
{
	/* HWSTATEXT */
	{	HWSTATEXT,	1,	8		},
	/* HWLEVELEXT */
	{	HWLEVELEXT,	1,	8		},
	/* HWVECEXT */
	{	HWVEC0EXT,	32,	0x1000	}
};

// SCB0, SCB1, SCB2
ioblock_sBlockDescriptor	IMG_asSCBBlock[3] =
{
	DECL_DEFAULT_BLOCK( 0, SYSTEM_CONTROL_BASE_ADDRESS + SCB_0_REGS_OFFSET, SCB_0_INTERRUPT_BIT, 0, 0 ),
	DECL_DEFAULT_BLOCK( 1, SYSTEM_CONTROL_BASE_ADDRESS + SCB_1_REGS_OFFSET, SCB_1_INTERRUPT_BIT, 0, 0 ),
	DECL_DEFAULT_BLOCK( 2, SYSTEM_CONTROL_BASE_ADDRESS + SCB_2_REGS_OFFSET, SCB_2_INTERRUPT_BIT, 0, 0 )
};

// UART0, UART1
ioblock_sBlockDescriptor	IMG_asUARTBlock[2] =
{
	DECL_DEFAULT_BLOCK( 0, SYSTEM_CONTROL_BASE_ADDRESS + UART_0_REGS_OFFSET, UART_0_INTERRUPT_BIT, 0, 0 ),
	DECL_DEFAULT_BLOCK( 1, SYSTEM_CONTROL_BASE_ADDRESS + UART_1_REGS_OFFSET, UART_1_INTERRUPT_BIT, 0, 0 )
};

// SPIM0, SPIM1
ioblock_sBlockDescriptor	IMG_asSPIMBlock[2] =
{
	DECL_DEFAULT_BLOCK( 0, SYSTEM_CONTROL_BASE_ADDRESS + SPIM_0_REGS_OFFSET, SPIM_0_INTERRUPT_BIT, DMA_REQUEST_SPIM_0_READ, DMA_REQUEST_SPIM_0_WRITE ),
	DECL_DEFAULT_BLOCK( 1, SYSTEM_CONTROL_BASE_ADDRESS + SPIM_1_REGS_OFFSET, SPIM_1_INTERRUPT_BIT, DMA_REQUEST_SPIM_1_READ, DMA_REQUEST_SPIM_1_WRITE )
};

// SPIS0, SPIS1
ioblock_sBlockDescriptor	IMG_asSPISBlock[1] =
{
	DECL_DEFAULT_BLOCK( 0, SYSTEM_CONTROL_BASE_ADDRESS + SPIS_0_REGS_OFFSET, SPIS_0_INTERRUPT_BIT, DMA_REQUEST_SPIS_0_READ, DMA_REQUEST_SPIS_0_WRITE )
};

// USB
ioblock_sBlockDescriptor	IMG_asUSBBlock[1] =
{
	DECL_DEFAULT_BLOCK( 0, SYSTEM_CONTROL_BASE_ADDRESS + USB_REGS_OFFSET, USB_INTERRUPT_BIT, 0, 0 )
};

// GDMA
ioblock_sBlockDescriptor	IMG_asGDMABlock[4] =
{
	DECL_DEFAULT_BLOCK( 0, SYSTEM_CONTROL_BASE_ADDRESS + SBDMAC_REGS_OFFSET_0, SBDMAC_INTERRUPT_BIT_0, 0, 0 ),
	DECL_DEFAULT_BLOCK( 1, SYSTEM_CONTROL_BASE_ADDRESS + SBDMAC_REGS_OFFSET_1, SBDMAC_INTERRUPT_BIT_1, 0, 0 ),
	DECL_DEFAULT_BLOCK( 2, SYSTEM_CONTROL_BASE_ADDRESS + SBDMAC_REGS_OFFSET_2, SBDMAC_INTERRUPT_BIT_2, 0, 0 ),
	DECL_DEFAULT_BLOCK( 3, SYSTEM_CONTROL_BASE_ADDRESS + SBDMAC_REGS_OFFSET_3, SBDMAC_INTERRUPT_BIT_3, 0, 0 )
};

//GPIO
ioblock_sBlockDescriptor	IMG_asGPIOBlock[3] =
{
	DECL_DEFAULT_BLOCK( 0, SYSTEM_CONTROL_BASE_ADDRESS + GPIO_0_REGS_OFFSET, GPIO_0_INTERRUPT_BIT, 0, 0 ),
	DECL_DEFAULT_BLOCK( 1, SYSTEM_CONTROL_BASE_ADDRESS + GPIO_1_REGS_OFFSET, GPIO_1_INTERRUPT_BIT, 0, 0 ),
	DECL_DEFAULT_BLOCK( 2, SYSTEM_CONTROL_BASE_ADDRESS + GPIO_2_REGS_OFFSET, GPIO_2_INTERRUPT_BIT, 0, 0 )
};

//DiSEqC
ioblock_sBlockDescriptor	IMG_asDISEQCBlock[2] =
{
	DECL_DEFAULT_BLOCK( 0, SYSTEM_CONTROL_BASE_ADDRESS + DISEQC_0_REGS_OFFSET, DISEQC_0_INTERRUPT_BIT, 0, 0 ),
	DECL_DEFAULT_BLOCK( 1, SYSTEM_CONTROL_BASE_ADDRESS + DISEQC_1_REGS_OFFSET, DISEQC_1_INTERRUPT_BIT, 0, 0 ),
};
