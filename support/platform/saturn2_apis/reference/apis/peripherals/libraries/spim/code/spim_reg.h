/*!
*******************************************************************************
 @file   spim_reg.h

 @brief  Serial Peripheral Interface Master Register Access definitions

         This file contains the header file information for the Serial
         Peripheral Interface Master (SPIM) register interface.

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

#if !defined (__SPIM_REG_H__)
#define __SPIM_REG_H__

/*****************************************************************************
**																			**
**						SPIM REGISTER BLOCK									**
**																			**
*****************************************************************************/

typedef struct spim_reg_t
{
	unsigned long device_0_parm;
	unsigned long device_1_parm;
	unsigned long device_2_parm;
	unsigned long transaction;
	unsigned long control_status;
} SPIM_REG_T;

#define SPIM_DEVICE_0_PARAM_REG_OFFSET		(0x00)
#define SPIM_DEVICE_1_PARAM_REG_OFFSET		(0x04)
#define SPIM_DEVICE_2_PARAM_REG_OFFSET		(0x08)
#define SPIM_TRANSACTION_REG_OFFSET			(0x0C)
#define SPIM_CONTROL_STATUS_REG_OFFSET		(0x10)

#define SPIM_SEND_DATA_REG_OFFSET			(0x14)
#define SPIM_GET_DATA_REG_OFFSET			(0x18)

#define SPIM_DMA_READ_INT_STATUS_OFFSET		(0x1C)
#define SPIM_DMA_READ_INT_ENABLE_OFFSET		(0x20)
#define SPIM_DMA_READ_INT_CLEAR_OFFSET		(0x24)

#define SPIM_DMA_WRITE_INT_STATUS_OFFSET	(0x28)
#define SPIM_DMA_WRITE_INT_ENABLE_OFFSET	(0x2C)
#define SPIM_DMA_WRITE_INT_CLEAR_OFFSET		(0x30)

#define SPIM_COMPARE_DATA_REG_OFFSET		(0x38)



/*============================================================================*/
/*                                                                            */
/*			MASKS AND SHIFTS TO ACCESS RELEVANT ELEMENTS OF REGISTERS	      */
/*                                                                            */
/*============================================================================*/

/*****************************************************************************
**																			**
**					REGISTERS 0, 1 AND 2 - DEVICE PARAMETERS				**
**																			**
*****************************************************************************/

/* ---- Reg - 0x0000 - Device 0 Parameters (CS0) */

/*
		Fields		Name		Description
		======		====		===========

		b31:24		CLK_RATE	Bit Clock rate = (24.576 * value / 512) MHz
		b23:16		CS_SETUP	Chip Select setup = (40 * value) ns
		b15:8		CS_HOLD		Chip Select hold = (40 * value) ns
		b7:0		CS_DELAY	Chip Select delay = (40 * value) ns
*/

#define SPIM_CLK_DIVIDE_MASK     0xFF000000
#define SPIM_CS_SETUP_MASK       0x00FF0000
#define SPIM_CS_HOLD_MASK        0x0000FF00
#define SPIM_CS_DELAY_MASK       0x000000FF
#define SPIM_CS_PARAM_MASK		(SPIM_CS_SETUP_MASK | SPIM_CS_HOLD_MASK | SPIM_CS_DELAY_MASK)

#define SPIM_CLK_DIVIDE_SHIFT    24
#define SPIM_CS_SETUP_SHIFT      16
#define SPIM_CS_HOLD_SHIFT       8
#define SPIM_CS_DELAY_SHIFT      0
#define SPIM_CS_PARAM_SHIFT		 0


/*****************************************************************************
**																			**
**					REGISTER 3 - TRANSACTION PARAMETERS						**
**																			**
*****************************************************************************/

#define SPIM_BYTE_DELAY				0x20000000
#define SPIM_CS_DEASSERT_MASK		0x10000000
#define SPIM_CONTINUE_MASK			0x08000000
#define SPIM_SOFT_RESET_MASK		0x04000000
#define	SPIM_GET_DMA_MASK			0x02000000
#define SPIM_SEND_DMA_MASK			0x01000000
#define SPIM_DEVICE_SELECT_MASK		0x00030000
#define SPIM_RX_SIZE_MASK			0x0000FFFF

#define SPIM_BYTE_DELAY_SHIFT		29
#define SPIM_CS_DEASSERT_SHIFT		28
#define SPIM_CONTINUE_SHIFT			27
#define SPIM_SOFT_RESET_SHIFT		26
#define	SPIM_GET_DMA_SHIFT			25
#define SPIM_SEND_DMA_SHIFT			24
#define SPIM_DEVICE_SELECT_SHIFT	16
#define SPIM_RX_SIZE_SHIFT			0


/*****************************************************************************
**																			**
**					REGISTER 4 - CONTROL AND STATUS							**
**																			**
*****************************************************************************/

#define SPIM_EDGE_TX_RX_MASK	 0x00001000
#define SPIM_CLOCK2_PHASE_MASK   0x00000800
#define SPIM_DATA2_IDLE_MASK     0x00000400
#define SPIM_CLOCK2_IDLE_MASK    0x00000200
#define SPIM_CS2_IDLE_MASK       0x00000100
#define SPIM_CLOCK1_PHASE_MASK   0x00000080
#define SPIM_DATA1_IDLE_MASK     0x00000040
#define SPIM_CLOCK1_IDLE_MASK    0x00000020
#define SPIM_CS1_IDLE_MASK       0x00000010
#define SPIM_CLOCK0_PHASE_MASK   0x00000008
#define SPIM_DATA0_IDLE_MASK     0x00000004
#define SPIM_CLOCK0_IDLE_MASK    0x00000002
#define SPIM_CS0_IDLE_MASK       0x00000001

#define SPIM_EDGE_TX_RX_SHIFT	 12
#define SPIM_CLOCK2_PHASE_SHIFT  11
#define SPIM_DATA2_IDLE_SHIFT    10
#define SPIM_CLOCK2_IDLE_SHIFT   9
#define SPIM_CS2_IDLE_SHIFT      8
#define SPIM_CLOCK1_PHASE_SHIFT  7
#define SPIM_DATA1_IDLE_SHIFT    6
#define SPIM_CLOCK1_IDLE_SHIFT   5
#define SPIM_CS1_IDLE_SHIFT      4
#define SPIM_CLOCK0_PHASE_SHIFT  3
#define SPIM_DATA0_IDLE_SHIFT    2
#define SPIM_CLOCK0_IDLE_SHIFT   1
#define SPIM_CS0_IDLE_SHIFT      0


/*****************************************************************************
**																			**
**					COMPARE DATA CONTROL REGISTER							**
**																			**
*****************************************************************************/

#define CP_DAT_EN_SHIFT		31
#define CP_DAT_DETECT_SHIFT	30
#define CP_DET_CLR_SHIFT	29
#define CP_DET_SET_SHIFT	28
#define CP_DAT_EQ_SHIFT		27
#define CP_MASK_SHIFT		8
#define CP_DATA_SHIFT		0

#define CP_DAT_EN_MASK		0x80000000
#define CP_DAT_DETECT_MASK	0x40000000
#define CP_DET_CLR_MASK		0x20000000
#define CP_DET_SET_MASK		0x10000000
#define CP_DAT_EQ_MASK		0x08000000
#define CP_MASK_MASK		0x0000FF00
#define CP_DATA_MASK		0x000000FF


/*****************************************************************************
**																			**
**	DMA READ INTERRUPT REGISTERS (for 'transmit to slave' transactions)		**
**																			**
*****************************************************************************/

#define SPIM_SDFUL_MASK		0x00000008
#define SPIM_SDHF_MASK		0x00000004
#define SPIM_SDEX_MASK		0x00000002
#define SPIM_SDTRIG_MASK	0x00000001

#define SPIM_SDFUL_SHIFT	3
#define SPIM_SDHF_SHIFT		2
#define SPIM_SDEX_SHIFT		1
#define SPIM_SDTRIG_SHIFT	0


/*****************************************************************************
**																			**
**	DMA WRITE INTERRUPT REGISTERS (for 'receive from slave' transactions)	**
**																			**
*****************************************************************************/

#define SPIM_ALLDONE_MASK	0x00000010
#define SPIM_GDFUL_MASK		0x00000008
#define SPIM_GDHF_MASK		0x00000004
#define SPIM_GDEX_MASK		0x00000002
#define SPIM_GDTRIG_MASK	0x00000001

#define SPIM_ALLDONE_SHIFT	4
#define SPIM_GDFUL_SHIFT	3
#define SPIM_GDHF_SHIFT		2
#define SPIM_GDEX_SHIFT		1
#define SPIM_GDTRIG_SHIFT	0

#endif /* __SPIM_REG_H__ */
