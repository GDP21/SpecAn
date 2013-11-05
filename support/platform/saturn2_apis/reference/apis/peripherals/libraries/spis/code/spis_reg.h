/*!
*******************************************************************************
 @file   spis_reg.h

 @brief  Serial Peripheral Interface Slave Register Access definitions

         This file contains the header file information for the Serial
         Peripheral Interface Slave (SPIS) register interface.

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

#if !defined (__SPIS_REG_H__)
#define __SPIS_REG_H__


/*============================================================================*/
/*                                                                            */
/*          MASKS AND SHIFTS TO ACCESS RELEVANT ELEMENTS OF REGISTERS         */
/*                                                                            */
/*============================================================================*/

/* ---- Reg - 0x0010 - Control */

/*
        Fields      Name          Description
        ======      ====          ===========

		b19			RX_SLOWSYNC	  1 = Slow sync mode enabled on RX interface
								  0 = Slow sync mode disabled on RX interface
		b18			TX_SLOWSYNC	  1 = Slow sync mode enabled on TX interface
								  0 = Slow sync mode disabled on TX interface
		b17			RX_RESYNC	  1 = Synchroniser enabled on RX interface
								  0 = Synchroniser disabled on RX interface
		b16			TX_RESYNC	  1 = Synchroniser enabled on TX interface
								  0 = Synchroniser disabled on TX interface
        b15:05      -             Reserved
        b04         CS_LEVEL      Value of chip select (when active) as used by master
        b03         CK_IDLE       Clock polarity
                                  0 = Clock is low when idle
                                  1 = Clock is high when idle
        b02         CK_PHASE      Clock phase
                                  0 = Data is valid on second clock transition
                                  1 = Data is valid on first clock transition
        b01         RX_EN         1 = Enable data receive
        b00         TX_EN         1 = Enable data transmit
*/

#define SPI_S_CNTRL_OFFSET			(0x0010)

#define SPI_S_CNTRL_RES2_OFFSET		(SPI_S_CNTRL_OFFSET)
#define SPI_S_CNTRL_RES2_SHIFT		(20)
#define SPI_S_CNTRL_RES2_MASK		(0xFFF00000)
#define SPI_S_CNTRL_RES2_LENGTH		(12)

#define SPI_S_RX_SLOWSYNC_OFFSET	(SPI_S_CNTRL_OFFSET)
#define SPI_S_RX_SLOWSYNC_SHIFT		(19)
#define SPI_S_RX_SLOWSYNC_MASK		(0x00080000)
#define SPI_S_RX_SLOWSYNC_LENGTH	(1)

#define SPI_S_TX_SLOWSYNC_OFFSET	(SPI_S_CNTRL_OFFSET)
#define SPI_S_TX_SLOWSYNC_SHIFT		(18)
#define SPI_S_TX_SLOWSYNC_MASK		(0x00040000)
#define SPI_S_TX_SLOWSYNC_LENGTH	(1)

#define SPI_S_RX_RESYNC_OFFSET		(SPI_S_CNTRL_OFFSET)
#define SPI_S_RX_RESYNC_SHIFT		(17)
#define SPI_S_RX_RESYNC_MASK		(0x00020000)
#define SPI_S_RX_RESYNC_LENGTH		(1)

#define SPI_S_TX_RESYNC_OFFSET		(SPI_S_CNTRL_OFFSET)
#define SPI_S_TX_RESYNC_SHIFT		(16)
#define SPI_S_TX_RESYNC_MASK		(0x00010000)
#define SPI_S_TX_RESYNC_LENGTH		(1)

#define SPI_S_CNTRL_RES_OFFSET    SPI_S_CNTRL_OFFSET
#define SPI_S_CNTRL_RES_SHIFT     5
#define SPI_S_CNTRL_RES_MASK      0x0000FFE0
#define SPI_S_CNTRL_RES_LENGTH    11

#define SPI_S_CS_LEVEL_OFFSET     SPI_S_CNTRL_OFFSET
#define SPI_S_CS_LEVEL_SHIFT      4
#define SPI_S_CS_LEVEL_MASK       0x00000010
#define SPI_S_CS_LEVEL_LENGTH     1

#define SPI_S_CK_IDLE_OFFSET      SPI_S_CNTRL_OFFSET
#define SPI_S_CK_IDLE_SHIFT       3
#define SPI_S_CK_IDLE_MASK        0x00000008
#define SPI_S_CK_IDLE_LENGTH      1

#define SPI_S_CK_PHASE_OFFSET     SPI_S_CNTRL_OFFSET
#define SPI_S_CK_PHASE_SHIFT      2
#define SPI_S_CK_PHASE_MASK       0x00000004
#define SPI_S_CK_PHASE_LENGTH     1

#define SPI_S_RX_EN_OFFSET        SPI_S_CNTRL_OFFSET
#define SPI_S_RX_EN_SHIFT         1
#define SPI_S_RX_EN_MASK          0x00000002
#define SPI_S_RX_EN_LENGTH        1

#define SPI_S_TX_EN_OFFSET        SPI_S_CNTRL_OFFSET
#define SPI_S_TX_EN_SHIFT         0
#define SPI_S_TX_EN_MASK          0x00000001
#define SPI_S_TX_EN_LENGTH        1


/* ---- Reg - 0x0014 - TX_Data */

/*
        Fields      Name          Description
        ======      ====          ===========

        b31:08      -             Reserved
        b07:0       TXDAT         Data to be sent from SPI port
*/

#define SPI_S_TX_DATA_OFFSET      (0x0014)

#define SPI_S_TX_DATA_RES_OFFSET  SPI_S_TX_DATA_OFFSET
#define SPI_S_TX_DATA_RES_SHIFT   8
#define SPI_S_TX_DATA_RES_MASK    0xFFFFFF00
#define SPI_S_TX_DATA_RES_LENGTH  24

#define SPI_S_TXDAT_OFFSET        SPI_S_TX_DATA_OFFSET
#define SPI_S_TXDAT_SHIFT         7
#define SPI_S_TXDAT_MASK          0x000000FF
#define SPI_S_TXDAT_LENGTH        8


/* ---- Reg - 0x0018 - RX_Data */

/*
        Fields      Name          Description
        ======      ====          ===========

        b31:08      -             Reserved
        b07:0       RXDAT         Data received from SPI port
*/

#define SPI_S_RX_DATA_OFFSET      (0x0018)

#define SPI_S_RX_DATA_RES_OFFSET  SPI_S_RX_DATA_OFFSET
#define SPI_S_RX_DATA_RES_SHIFT   8
#define SPI_S_RX_DATA_RES_MASK    0xFFFFFF00
#define SPI_S_RX_DATA_RES_LENGTH  24

#define SPI_S_RXDAT_OFFSET        SPI_S_RX_DATA_OFFSET
#define SPI_S_RXDAT_SHIFT         7
#define SPI_S_RXDAT_MASK          0x000000FF
#define SPI_S_RXDAT_LENGTH        8


/* ---- Reg - 0x001C - DMA Read Interrupt Status */

/*
        Fields      Name          Description
        ======      ====          ===========

        b31:05      -             Reserved
        b04         SDEMP         1 = Send data FIFO is empty
        b03         SDFUL         1 = Send data FIFO is full
        b02         SDHF          1 = Send data FIFO is half full or greater
        b01         SDEX          1 = Data exists in send data FIFO
        b00         SDTRIG        1 = New data byte received
*/

#define SPI_S_RIS_OFFSET          (0x001C)

#define SPI_S_RIS_RES_OFFSET      SPI_S_RIS_OFFSET
#define SPI_S_RIS_RES_SHIFT       5
#define SPI_S_RIS_RES_MASK        0xFFFFFFE0
#define SPI_S_RIS_RES_LENGTH      27

#define SPI_S_SDEMP_OFFSET        SPI_S_RIS_OFFSET
#define SPI_S_SDEMP_SHIFT         4
#define SPI_S_SDEMP_MASK          0x00000010
#define SPI_S_SDEMP_LENGTH        1

#define SPI_S_SDFUL_OFFSET        SPI_S_RIS_OFFSET
#define SPI_S_SDFUL_SHIFT         3
#define SPI_S_SDFUL_MASK          0x00000008
#define SPI_S_SDFUL_LENGTH        1

#define SPI_S_SDHF_OFFSET         SPI_S_RIS_OFFSET
#define SPI_S_SDHF_SHIFT          2
#define SPI_S_SDHF_MASK           0x00000004
#define SPI_S_SDHF_LENGTH         1

#define SPI_S_SDEX_OFFSET         SPI_S_RIS_OFFSET
#define SPI_S_SDEX_SHIFT          1
#define SPI_S_SDEX_MASK           0x00000002
#define SPI_S_SDEX_LENGTH         1

#define SPI_S_SDTRIG_OFFSET       SPI_S_RIS_OFFSET
#define SPI_S_SDTRIG_SHIFT        0
#define SPI_S_SDTRIG_MASK         0x00000001
#define SPI_S_SDTRIG_LENGTH       1


/* ---- Reg - 0x0020 - DMA Read Interrupt Enable */

/*
        Fields      Name          Description
        ======      ====          ===========

        b31:05      -             Reserved
        b04         SDEMP         1 = Enable interrupt on SDEMP
        b03         SDFUL_EN      1 = Enable interrupt on SDFUL
        b02         SDHF_EN       1 = Enable interrupt on SDHF
        b01         SDEX_EN       1 = Enable interrupt on SDEX
        b00         SDTRIG_EN     1 = Enable interrupt on SDTRIG
*/

#define SPI_S_RIE_OFFSET          (0x0020)

#define SPI_S_RIE_RES_OFFSET      SPI_S_RIE_OFFSET
#define SPI_S_RIE_RES_SHIFT       5
#define SPI_S_RIE_RES_MASK        0xFFFFFFE0
#define SPI_S_RIE_RES_LENGTH      27

#define SPI_S_SDEMP_EN_OFFSET     SPI_S_RIE_OFFSET
#define SPI_S_SDEMP_EN_SHIFT      4
#define SPI_S_SDEMP_EN_MASK       0x00000010
#define SPI_S_SDEMP_EN_LENGTH     1

#define SPI_S_SDFUL_EN_OFFSET     SPI_S_RIE_OFFSET
#define SPI_S_SDFUL_EN_SHIFT      3
#define SPI_S_SDFUL_EN_MASK       0x00000008
#define SPI_S_SDFUL_EN_LENGTH     1

#define SPI_S_SDHF_EN_OFFSET      SPI_S_RIE_OFFSET
#define SPI_S_SDHF_EN_SHIFT       2
#define SPI_S_SDHF_EN_MASK        0x00000004
#define SPI_S_SDHF_EN_LENGTH      1

#define SPI_S_SDEX_EN_OFFSET      SPI_S_RIE_OFFSET
#define SPI_S_SDEX_EN_SHIFT       1
#define SPI_S_SDEX_EN_MASK        0x00000002
#define SPI_S_SDEX_EN_LENGTH      1

#define SPI_S_SDTRIG_EN_OFFSET    SPI_S_RIE_OFFSET
#define SPI_S_SDTRIG_EN_SHIFT     0
#define SPI_S_SDTRIG_EN_MASK      0x00000001
#define SPI_S_SDTRIG_EN_LENGTH    1


/* ---- Reg - 0x0024 - DMA Read Interrupt Status Clear */

/*
        Fields      Name          Description
        ======      ====          ===========

        b31:05      -             Reserved
        b04         SDEMP         1 = Clear interrupt status on SDEMP. 0 = No effect
        b03         SDFUL_CL      1 = Clear interrupt status on SDFUL. 0 = No effect
        b02         SDHF_CL       1 = Clear interrupt status on SDHF. 0 = No effect
        b01         SDEX_CL       1 = Clear interrupt status on SDEX. 0 = No effect
        b00         SDTRIG_CL     1 = Clear interrupt status on SDTRIG. 0 = No effect
*/

#define SPI_S_RICL_OFFSET         (0x0024)

#define SPI_S_RICL_RES_OFFSET     SPI_S_RICL_OFFSET
#define SPI_S_RICL_RES_SHIFT      5
#define SPI_S_RICL_RES_MASK       0xFFFFFFE0
#define SPI_S_RICL_RES_LENGTH     27

#define SPI_S_SDEMP_CL_OFFSET     SPI_S_RICL_OFFSET
#define SPI_S_SDEMP_CL_SHIFT      4
#define SPI_S_SDEMP_CL_MASK       0x00000010
#define SPI_S_SDEMP_CL_LENGTH     1

#define SPI_S_SDFUL_CL_OFFSET     SPI_S_RICL_OFFSET
#define SPI_S_SDFUL_CL_SHIFT      3
#define SPI_S_SDFUL_CL_MASK       0x00000008
#define SPI_S_SDFUL_CL_LENGTH     1

#define SPI_S_SDHF_CL_OFFSET      SPI_S_RICL_OFFSET
#define SPI_S_SDHF_CL_SHIFT       2
#define SPI_S_SDHF_CL_MASK        0x00000004
#define SPI_S_SDHF_CL_LENGTH      1

#define SPI_S_SDEX_CL_OFFSET      SPI_S_RICL_OFFSET
#define SPI_S_SDEX_CL_SHIFT       1
#define SPI_S_SDEX_CL_MASK        0x00000002
#define SPI_S_SDEX_CL_LENGTH      1

#define SPI_S_SDTRIG_CL_OFFSET    SPI_S_RICL_OFFSET
#define SPI_S_SDTRIG_CL_SHIFT     0
#define SPI_S_SDTRIG_CL_MASK      0x00000001
#define SPI_S_SDTRIG_CL_LENGTH    1


/* ---- Reg - 0x0028 - DMA Write Interrupt Status */

/*
        Fields      Name          Description
        ======      ====          ===========

        b31:03      -             Reserved
        b02         GDFUL         1 = Receive data FIFO is full
        b01         GDHF          1 = Receive data FIFO is half full or greater
        b00         GDEX          1 = Data exists in receive data FIFO
*/

#define SPI_S_WIS_OFFSET          (0x0028)

#define SPI_S_WIS_RES_OFFSET      SPI_S_WIS_OFFSET
#define SPI_S_WIS_RES_SHIFT       3
#define SPI_S_WIS_RES_MASK        0xFFFFFFF8
#define SPI_S_WIS_RES_LENGTH      29

#define SPI_S_GDFUL_OFFSET        SPI_S_WIS_OFFSET
#define SPI_S_GDFUL_SHIFT         2
#define SPI_S_GDFUL_MASK          0x00000004
#define SPI_S_GDFUL_LENGTH        1

#define SPI_S_GDHF_OFFSET         SPI_S_WIS_OFFSET
#define SPI_S_GDHF_SHIFT          1
#define SPI_S_GDHF_MASK           0x00000002
#define SPI_S_GDHF_LENGTH         1

#define SPI_S_GDEX_OFFSET         SPI_S_WIS_OFFSET
#define SPI_S_GDEX_SHIFT          0
#define SPI_S_GDEX_MASK           0x00000001
#define SPI_S_GDEX_LENGTH         1


/* ---- Reg - 0x002C - DMA Write Interrupt Enable */

/*
        Fields      Name          Description
        ======      ====          ===========

        b31:03      -             Reserved
        b02         GDFUL_EN      1 = Enable interrupt on GDFUL
        b01         GDHF_EN       1 = Enable interrupt on GDHF
        b00         GDEX_EN       1 = Enable interrupt on GDEX
*/

#define SPI_S_WIE_OFFSET          (0x002C)

#define SPI_S_WIE_RES_OFFSET      SPI_S_WIE_OFFSET
#define SPI_S_WIE_RES_SHIFT       3
#define SPI_S_WIE_RES_MASK        0xFFFFFFF8
#define SPI_S_WIE_RES_LENGTH      29

#define SPI_S_GDFUL_EN_OFFSET     SPI_S_WIE_OFFSET
#define SPI_S_GDFUL_EN_SHIFT      2
#define SPI_S_GDFUL_EN_MASK       0x00000004
#define SPI_S_GDFUL_EN_LENGTH     1

#define SPI_S_GDHF_EN_OFFSET      SPI_S_WIE_OFFSET
#define SPI_S_GDHF_EN_SHIFT       1
#define SPI_S_GDHF_EN_MASK        0x00000002
#define SPI_S_GDHF_EN_LENGTH      1

#define SPI_S_GDEX_EN_OFFSET      SPI_S_WIE_OFFSET
#define SPI_S_GDEX_EN_SHIFT       0
#define SPI_S_GDEX_EN_MASK        0x00000001
#define SPI_S_GDEX_EN_LENGTH      1


/* ---- Reg - 0x0030 - DMA Write Interrupt Status Clear */

/*
        Fields      Name          Description
        ======      ====          ===========

        b31:03      -             Reserved
        b02         GDFUL_CL      1 = Clear interrupt status on GDFUL. 0 = No effect
        b01         GDHF_CL       1 = Clear interrupt status on GDHF. 0 = No effect
        b00         GDEX_CL       1 = Clear interrupt status on GDEX. 0 = No effect
*/

#define SPI_S_WICL_OFFSET         (0x0030)

#define SPI_S_WICL_RES_OFFSET     SPI_S_WICL_OFFSET
#define SPI_S_WICL_RES_SHIFT      3
#define SPI_S_WICL_RES_MASK       0xFFFFFFF8
#define SPI_S_WICL_RES_LENGTH     29

#define SPI_S_GDFUL_CL_OFFSET     SPI_S_WICL_OFFSET
#define SPI_S_GDFUL_CL_SHIFT      2
#define SPI_S_GDFUL_CL_MASK       0x00000004
#define SPI_S_GDFUL_CL_LENGTH     1

#define SPI_S_GDHF_CL_OFFSET      SPI_S_WICL_OFFSET
#define SPI_S_GDHF_CL_SHIFT       1
#define SPI_S_GDHF_CL_MASK        0x00000002
#define SPI_S_GDHF_CL_LENGTH      1

#define SPI_S_GDEX_CL_OFFSET      SPI_S_WICL_OFFSET
#define SPI_S_GDEX_CL_SHIFT       0
#define SPI_S_GDEX_CL_MASK        0x00000001
#define SPI_S_GDEX_CL_LENGTH      1


/* ---- Reg - 0x0034 - Diagnostic Status */

/*
        Fields      Name          Description
        ======      ====          ===========

        b31:19      -             Reserved
        b18         DI_GDFUL      Reserved
        b17         DI_GDHF       Reserved
        b16         DI_GDEX       Reserved
        b15:04      -             Reserved
        b03         DI_SDFUL      Reserved
        b02         DI_SDHF       Reserved
        b01         DI_SDEX       Reserved
        b00         DI_SDTRIG     Reserved
*/


/* ---- Reg - 0x003C - Fifo Flag */

/*
		Fields		Name					Description
		======		====					===========

		b31:16		-						Reserved
		b15:08		OUTPUT_FIFO_THRESHOLD	Sets the threshold at which the receive fifo will assert the DMA request. This
											should be set to 4 for SDIO SPI oepration and 8 for SPI operation.
		b07:00		INPUT_FIFO_THRESHOLD	Sets the threshold at which the transmit fifo will assert the DMA request. This
											should be set to 4 for SDIO SPI oepration and 8 for SPI operation.
*/

#define SPI_S_FIFO_FLAG_OFFSET		(0x003C)

#define SPI_S_FIFO_FLAG_RES_OFFSET	SPI_S_FIFO_FLAG_OFFSET
#define SPI_S_FIFO_FLAG_RES_SHIFT	16
#define SPI_S_FIFO_FLAG_RES_MASK	0xFFFF0000
#define SPI_S_FIFO_FLAG_RES_LENGTH	16

#define SPI_S_FIFO_FLAG_OUT_THRESH_OFFSET	SPI_S_FIFO_FLAG_OFFSET
#define SPI_S_FIFO_FLAG_OUT_THRESH_SHIFT	8
#define SPI_S_FIFO_FLAG_OUT_THRESH_MASK		0x0000FF00
#define SPI_S_FIFO_FLAG_OUT_THRESH_LENGTH	8

#define SPI_S_FIFO_FLAG_IN_THRESH_OFFSET	SPI_S_FIFO_FLAG_OFFSET
#define SPI_S_FIFO_FLAG_IN_THRESH_SHIFT		0
#define SPI_S_FIFO_FLAG_IN_THRESH_MASK		0x000000FF
#define SPI_S_FIFO_FLAG_IN_THRESH_LENGTH	8

#endif /* __SPIS_REG_H__  */

