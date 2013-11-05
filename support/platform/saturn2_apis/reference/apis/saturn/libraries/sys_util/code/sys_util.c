/*!
*******************************************************************************
  file   sys_util.c

  brief  Saturn system utilities

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

 $Log: sys_util.c,v $

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

 
******************************************************************************/

#include "sys_util.h"
#include "system.h"

#define CACHE_LINE	(64)

/*!
*******************************************************************************

 @Function              @SYS_FlushCache

 <b>Description:</b>\n
 This function flushes the data lines in the cache covering the data specified by the
 parameters.

 \param     ui32LinearAddress	Address of memory area to flush.

 \param		ui32Size			Size of memory area

 \return	None

*******************************************************************************/
__inline__ IMG_VOID SYS_FlushCache(	IMG_UINT32	ui32LinearAddress,
									IMG_UINT32	ui32Size )
{
	// This function is only applicable for Meta
	unsigned topAddress = (unsigned)(ui32LinearAddress) + ui32Size;
	unsigned p			= (unsigned)(ui32LinearAddress) & ~(CACHE_LINE - 1);

	if ( ui32LinearAddress )
	{
		/* For all lines the block is within */
		while ( p <= topAddress )
		{
			/* Flush cache line */
			TBIDCACHE_FLUSH( p );

			/* Move to next line */
			p += CACHE_LINE;
		}
	}
}

/*!
*******************************************************************************

 @Function              @SYS_BindDMACChannel

 <b>Description:</b>\n
 This function connects up a peripheral to a DMAC channel by setting up the
 channel select offset registers.\n
 Peripherals are identified by their Id as declared in the system architecture
 TRM.

 \param     ePeripheralID    Peripheral to be connected to DMAC channel.\n
 \param     ui32DMAChannelID      Channel number to be used.\n


*******************************************************************************/
IMG_RESULT SYS_BindDMACChannel
(
    IMG_UINT32				ui32PeripheralID,
    IMG_UINT32				ui32DMAChannelID
)
{
    img_uint32		ui32Shift = 0;
	img_uint32		ui32Reg = 0;

	ui32Shift = (ui32DMAChannelID * CR_PERIP_DMA_SEL_0_LENGTH);

	ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_DMA_ROUTE_SEL );
	ui32Reg &= ~( CR_PERIP_DMA_SEL_0_MASK << ui32Shift );
	ui32Reg |= ui32PeripheralID << ui32Shift;
	WRITE_REG( CR_PERIP_BASE, CR_PERIP_DMA_ROUTE_SEL, ui32Reg );

	return IMG_SUCCESS;
}

/*!
*******************************************************************************
 @Function              @GTIWait
*******************************************************************************/
static IMG_VOID GTIWait( IMG_VOID )
{
	/* Rather naff implementation of a delay function */
	volatile int	i, j;

	j = 0;
	for (i = 0; i < 100; i++)
	{
		j = (j * i) + i;
	}
}


/*!
*******************************************************************************
 @Function              @SYS_GTIWrite
 <b>Description:</b>\n
 This function issues a write to the specified (indirect) register address via
 the specified GTI "port" (top level register).

 NOTE:	It is assumed that the "port" specifiec is the "control" register and that
		the data out register immediately follows the control register in the
		register map.

 \param     ui32GTIPort			Address of the GTI port.
 \param     ui32GTIReg			Address of the GTI register.
 \param     ui32Value			Value to write.

 \return	None
*******************************************************************************/
IMG_VOID SYS_GTIWrite(	IMG_UINT32	ui32GTIPort,
						IMG_UINT32	ui32GTIReg,
						IMG_UINT32	ui32Value		)
{
	IMG_UINT32			ui32DataStream;
	IMG_UINT32			ui32GTIControlRegValue;
	IMG_UINT32			ui32DataBit;

	int					i;

	/*
		First construct the serial data stream word
	*/
	ui32DataStream = 0;
	ui32DataStream |= ((ui32GTIReg & GTI_CTRL_REG_ADDR_MASK) << GTI_CTRL_REG_ADDR_SHIFT);
	ui32DataStream |= ((ui32Value & GTI_CTRL_DATA_MASK) << GTI_CTRL_DATA_SHIFT);
	ui32DataStream |= ((GTI_CTRL_OP_WRITE & GTI_CTRL_OP_MASK) << GTI_CTRL_OP_SHIFT);

	/*
		Make sure the GTI control register is in the right state
	*/
	ui32GTIControlRegValue = GTI_MAKE_CRTL_REG( 0, GTI_CLK_LO, GTI_NOT_IN_RESET, GTI_NOT_TEST_MODE );
	*((volatile unsigned long *)ui32GTIPort) = ui32GTIControlRegValue;
	GTIWait();

	/*
		Clock out the bits starting with the MSB
	*/
	for (i = 0; i < 32; i++)
	{
		ui32DataBit = (ui32DataStream & 0x80000000) >> 31;
		ui32DataStream <<= 1;

		ui32GTIControlRegValue = GTI_MAKE_CRTL_REG( ui32DataBit, GTI_CLK_HI, GTI_NOT_IN_RESET, GTI_NOT_TEST_MODE );
		*((volatile unsigned long *)ui32GTIPort) = ui32GTIControlRegValue;
		GTIWait();
		ui32GTIControlRegValue = GTI_MAKE_CRTL_REG( ui32DataBit, GTI_CLK_LO, GTI_NOT_IN_RESET, GTI_NOT_TEST_MODE );
		*((volatile unsigned long *)ui32GTIPort) = ui32GTIControlRegValue;
		GTIWait();
	}
}


/*!
*******************************************************************************
 @Function              @SYS_GTIRead
 <b>Description:</b>\n
 This function issues a read to the specified (indirect) register address via
 the specified GTI "port" (top level register).

 NOTE:	It is assumed that the "port" specifiec is the "control" register and that
		the data out register immediately follows the control register in the
		register map.

 \param     ui32GTIPort			Address of the GTI port.
 \param     ui32GTIReg			Address of the GTI register.

 \return	IMG_UINT32			Value read from GTI register
*******************************************************************************/
IMG_UINT32 SYS_GTIRead(	IMG_UINT32	ui32GTIPort,
						IMG_UINT32	ui32GTIReg )
{
	IMG_UINT32			ui32Value = 0;

	IMG_UINT32			ui32DataStream;
	IMG_UINT32			ui32GTIControlRegValue;
	IMG_UINT32			ui32DataBit;

	IMG_UINT32			ui32DataOutBit;
	IMG_UINT32			ui32DataOutStream = 0;

	int					i;

	/*
		First construct the serial data stream word
	*/
	ui32DataStream = 0;
	ui32DataStream |= ((ui32GTIReg & GTI_CTRL_REG_ADDR_MASK) << GTI_CTRL_REG_ADDR_SHIFT);
	ui32DataStream |= ((0 & GTI_CTRL_DATA_MASK) << GTI_CTRL_DATA_SHIFT);
	ui32DataStream |= ((GTI_CTRL_OP_READ & GTI_CTRL_OP_MASK) << GTI_CTRL_OP_SHIFT);

	/*
		Make sure the GTI control register is in the right state
	*/
	ui32GTIControlRegValue = GTI_MAKE_CRTL_REG( 0, GTI_CLK_LO, GTI_NOT_IN_RESET, GTI_NOT_TEST_MODE );
	*((volatile unsigned long *)ui32GTIPort) = ui32GTIControlRegValue;
	GTIWait();

	/*
		Clock out the bits starting with the MSB and clock in the data bits returned
	*/
	for (i = 0; i < 32; i++)
	{
		ui32DataBit = (ui32DataStream & 0x80000000) >> 31;
		ui32DataStream <<= 1;

		ui32GTIControlRegValue = GTI_MAKE_CRTL_REG( ui32DataBit, GTI_CLK_HI, GTI_NOT_IN_RESET, GTI_NOT_TEST_MODE );
		*((volatile unsigned long *)ui32GTIPort) = ui32GTIControlRegValue;
		GTIWait();
		ui32GTIControlRegValue = GTI_MAKE_CRTL_REG( ui32DataBit, GTI_CLK_LO, GTI_NOT_IN_RESET, GTI_NOT_TEST_MODE );
		*((volatile unsigned long *)ui32GTIPort) = ui32GTIControlRegValue;
		GTIWait();

		/* Read the DOUT register and shift into the data read value */
		ui32DataOutBit = *((volatile unsigned long *)(ui32GTIPort+4));
		ui32DataOutStream <<= 1;
		ui32DataOutStream |= ui32DataOutBit;
	}

	/*
		ui32DataOutStream now contains the returned GTI register read data in b15:8.
	*/
	ui32Value = (ui32DataOutStream >> GTI_CTRL_DATA_SHIFT) & GTI_CTRL_DATA_MASK;

	return ( ui32Value );
}


/*!
*******************************************************************************
 @Function              @SYS_GTIReset
 <b>Description:</b>\n
 This function enables or disables the reset on the specified GTI "port"
 (top level register).

 \param     ui32GTIPort			Address of the GTI port.
 \param     bReset				TRUE = put in reset, FALSE = put in not reset.

 \return	None.
*******************************************************************************/
IMG_VOID SYS_GTIReset(	IMG_UINT32	ui32GTIPort,
						IMG_BOOL	bReset )
{
    IMG_UINT32 ui32GTIControlRegValue;

	ui32GTIControlRegValue = GTI_MAKE_CRTL_REG( 0, GTI_CLK_LO, (bReset ? GTI_IN_RESET : GTI_NOT_IN_RESET), GTI_NOT_TEST_MODE );
	*((volatile unsigned long *)ui32GTIPort) = ui32GTIControlRegValue;

	return;
}
