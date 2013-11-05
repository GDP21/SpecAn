/*!
*******************************************************************************
  file   sys_util.h

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

#if !defined (__SYS_UTIL_H__)
#define __SYS_UTIL_H__

#include <img_defs.h>

/*
	GTI serial data stream format is
	b31:20		GTI register address
	b19			GTI operation. 1 = write to GTI register, 0 = read from GTI register
	b18:16		must be 0
	b15:8		data to be written or don't care if a read (use 0's)
	b7:0		must be 0
*/
#define GTI_CTRL_REG_ADDR_SHIFT			20
#define GTI_CTRL_REG_ADDR_MASK			0x00000FFF		/* 12 bits */
#define GTI_CTRL_OP_SHIFT				19
#define GTI_CTRL_OP_MASK				0x00000001		/* 1 bit */
#define GTI_CTRL_DATA_SHIFT				8
#define GTI_CTRL_DATA_MASK				0x000000FF		/* 8 bits */

#define GTI_CTRL_OP_WRITE				0x00000001
#define GTI_CTRL_OP_READ				0x00000000

#define GTI_CLK_HI						1
#define GTI_CLK_LO						0
#define GTI_NOT_IN_RESET				1
#define GTI_IN_RESET					0
#define GTI_NOT_TEST_MODE				0
#define GTI_TEST_MODE					1

#define GTI_MAKE_CRTL_REG(a,b,c,d) (((a&1)<<3) | ((b&1)<<2) | ((c&1)<<1) | ((d&1)<<0) )

/******************************************************************************
****************************** Macro Definitions ******************************
*******************************************************************************/
#define READ_REG(baseAddr, name)                                    \
(                                                                   \
    (*(volatile img_uint32 *)(baseAddr + name##_OFFSET))			\
)                                                   

#define WRITE_REG(baseAddr, name, val)                              \
(                                                                   \
    *(volatile img_uint32 *)(baseAddr + name##_OFFSET) = (val)		\
)

#define READ_REG_FIELD(regval,field)								\
(																	\
	((field##_MASK) == 0xFFFFFFFF) ?								\
	(																\
		(regval)													\
	)																\
	:																\
	(																\
		((regval) & (field##_MASK)) >> (field##_SHIFT)				\
	)																\
)

#define WRITE_REG_FIELD(regval,field,val)							\
(																	\
	((field##_MASK) == 0xFFFFFFFF) ?								\
	(																\
		(val)														\
	)																\
	:																\
	(																\
		(															\
			(regval) & ~(field##_MASK)								\
		) |															\
		(															\
			((val) << (field##_SHIFT)) & (field##_MASK)				\
		)															\
	)																\
)

/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

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
IMG_VOID SYS_FlushCache(	IMG_UINT32		ui32LinearAddress,
							IMG_UINT32		ui32Size );


/*!
*******************************************************************************

 @Function              @SYS_BindDMACChannel

 <b>Description:</b>\n
 This function connects up a peripheral to a DMAC channel by setting up the
 channel select offset registers.\n
 Peripherals are identified by their Id as declared in the system architecture
 TRM.

 \param     ui32PeripheralID      Peripheral to be connected to DMAC channel.\n
 \param     ui32DMAChannelID      Channel number to be used.\n


*******************************************************************************/
IMG_RESULT SYS_BindDMACChannel(	IMG_UINT32				ui32PeripheralID,
								IMG_UINT32				ui32DMAChannelID	);

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
						IMG_UINT32	ui32Value		);


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
						IMG_UINT32	ui32GTIReg );


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
						IMG_BOOL	bReset );

#endif /* __SYS_UTIL_H__ */
