/*!
*******************************************************************************
 @file   dmac_reg_defs.h

 @brief  DMAC register definition header

         This file provides the register definitions for the DMAC hardware.

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

*******************************************************************************/

#if !defined (__DMAC_REG_DEFS_H__)
#define __DMAC_REG_DEFS_H__


/******************************************************************************
******************************** Enumerations *********************************
*******************************************************************************/

/*!
*******************************************************************************

 Register numbering (offsets) within the register block.

 Registers Setup_n, Count_n, Peripheral_addr_n, IRQ_Stat_n and 2D_Mode_n are
 replicated for each channel.

 Soft_Reset is a single register shared between channels and does not appear
 at offset 5 within each block.

*******************************************************************************/
typedef enum
{
	/*! Setup_n */
	DMAC_SETUPN_REGISTER = 0,

	/*! Count_n */
    DMAC_COUNTN_REGISTER,

	/*! Peripheral_addr_n */
    DMAC_PERADDRN_REGISTER,

    /*! IRQ_Stat_n */
    DMAC_ISTATN_REGISTER,

    /*! 2D_Mode_n */
    DMAC_2D_MODEN_REGISTER,

    /*! Soft_Reset - this is not replicated per channel */
    DMAC_SRST_REGISTER,

    /*! Number of registers */
    DMAC_NUM_REGISTERS

} eDmacRegisters;

/******************************************************************************
****************************** End enumerations *******************************
*******************************************************************************/

/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/

/**** DMAC registers ****/

/* ---- Reg - 0x0000 - Setup n */
/*
        Fields      Name    Description
        ======      ====    ===========
        b31:00      SA      If LISTEN = '1' then
                                Word-aligned (bits 1:0 must be zero) start
                                address of the system memory from where to
                                read the linked list element describing the
                                next transfer. Once a transfer has completed,
                                this address is updated to reflect the new start
                                address contained within the linked list structure.
                            else if LISTEN = '0' then
                                Byte start address of the system memory from
                                where to read or write the data.
*/

#define DMAC_PERIP_SETUPN_OFFSET          (0x0000)

#define DMAC_PERIP_SA_OFFSET              DMAC_PERIP_SETUPN_OFFSET
#define DMAC_PERIP_SA_SHIFT               0
#define DMAC_PERIP_SA_MASK                0xFFFFFFFF
#define DMAC_PERIP_SA_FLAGS               REG_RW
#define DMAC_PERIP_SA_LENGTH              32

/* Entire Register */
#define DMAC_PERIP_SETUPN_REG_OFFSET      DMAC_PERIP_SETUPN_OFFSET
#define DMAC_PERIP_SETUPN_REG_SHIFT       0
#define DMAC_PERIP_SETUPN_REG_MASK        (unsigned int) (0xFFFFFFFF << 0)
#define DMAC_PERIP_SETUPN_REG_FLAGS       0
#define DMAC_PERIP_SETUPN_REG_LENGTH      32
#define DMAC_PERIP_SETUPN_REG_VALID_MASK  (DMAC_PERIP_SA_MASK)


/* ---- Reg - 0x0004 - Count n */
/*
        Fields      Name        Description
        ======      ====        ===========

        b31         LIST_IEN    Enable an interrupt when linked list
                                processing has completed.
        b30         BSWAP       Big/little endian byte swap. If set, byte
                                order will be reversed in all read/write
                                transfers between peripheral and memory.
        b29         IEN         Enable an interrupt when all scheduled
                                transfers have completed.
        b28:27      PW          Specifies the width of the peripherals
                                DMA register:
                                00 : 32 bit
                                01 : 16 bit
                                10 : 8 bit
        b26         DIR         Specifies the direction of the transfer:
                                0 : Data is transferred from memory to the
                                    peripheral.
                                1 : Data is transferred from the peripheral
                                    to memory
        b25:23      -           Reserved
        b22         LIST_FINCTL Linked-list finished status control.
                                0 : The linked-list finished status is set '1'
                                    after the final transfer has been initiated,
                                    and before this last transfer completes.
                                1 : The linked-list finished status is set '1'
                                    when the final transfer has completed,
                                    despite the linked-list expiring prior to
                                    this event.
        b21         -           Reserved
        b20         DREQ (RO)   Indicates state of DMA request line from channel
                                n (read-only).
        b19         SRST        Software channel reset - setting to '1' resets
                                channel n. Must subsequently be set to '0' before
                                channel can be used. Note that configuration
                                registers are not reset so count and enable must
                                be set to zero during software reset.
        b18         LIST_EN     Linked-list enable. This bit should only be set
                                once all registers and linked list structure
                                are valid for the transfers.
                                0 : List disabled, CNT and SA are used directly
                                    to control DMA transfer.
                                1 : List enabled, count and start address read
                                    from linked-list.
                                This will be cleared to 0 once the last linked-list
                                element has been processed.
        b17         2D_MODE     2D DMA transfer mode enable. This bit should only
                                be set once the registers and 2D mode control
                                register has been set (only set when LIST_EN = '0')
                                0 : 2D mode disabled
                                1 : 2D mode enabled
        b16         EN          Enables the DMA transfer. This bit should only
                                be set once all other register settings are valid
                                for the transfer.
                                0 : The channel is disabled
                                1 : The channel is enabled for communication (only
                                    set when LIST_EN = '0')
                                This will be cleared to 0 at the end of the transfer
        b15:00      CNT         If LIST_EN = '1' then
                                    Once the transfer has started, this register
                                    will reflect the number of DMA transfers
                                    remaining (read only status).
                                else if 2D_EN = '1' then
                                    This register will reflect the number of DMA
                                    transfers remaining (read only status). This
                                    reflects the REP_COUNT element of the transfer.
                                else if LIST_EN = '0' then
                                    Specifies the transfer size in PW units.
                                Once the transfer has started, this register will
                                reflect the number of DMA transfers remaining.
*/

#define DMAC_PERIP_COUNTN_OFFSET          (0x0004)

#define DMAC_PERIP_LIST_IEN_OFFSET        DMAC_PERIP_COUNTN_OFFSET
#define DMAC_PERIP_LIST_IEN_SHIFT         31
#define DMAC_PERIP_LIST_IEN_MASK          0x80000000
#define DMAC_PERIP_LIST_IEN_FLAGS         REG_RW
#define DMAC_PERIP_LIST_IEN_LENGTH        1

#define DMAC_PERIP_BSWAP_OFFSET           DMAC_PERIP_COUNTN_OFFSET
#define DMAC_PERIP_BSWAP_SHIFT            30
#define DMAC_PERIP_BSWAP_MASK             0x40000000
#define DMAC_PERIP_BSWAP_FLAGS            REG_RW
#define DMAC_PERIP_BSWAP_LENGTH           1

#define DMAC_PERIP_IEN_OFFSET             DMAC_PERIP_COUNTN_OFFSET
#define DMAC_PERIP_IEN_SHIFT              29
#define DMAC_PERIP_IEN_MASK               0x20000000
#define DMAC_PERIP_IEN_FLAGS              REG_RW
#define DMAC_PERIP_IEN_LENGTH             1

#define DMAC_PERIP_PW_OFFSET              DMAC_PERIP_COUNTN_OFFSET
#define DMAC_PERIP_PW_SHIFT               27
#define DMAC_PERIP_PW_MASK                0x18000000
#define DMAC_PERIP_PW_FLAGS               REG_RW
#define DMAC_PERIP_PW_LENGTH              2

#define DMAC_PERIP_DIR_OFFSET             DMAC_PERIP_COUNTN_OFFSET
#define DMAC_PERIP_DIR_SHIFT              26
#define DMAC_PERIP_DIR_MASK               0x04000000
#define DMAC_PERIP_DIR_FLAGS              REG_RW
#define DMAC_PERIP_DIR_LENGTH             1

#define DMAC_PERIP_COUNTN_RES1_OFFSET     DMAC_PERIP_COUNTN_OFFSET
#define DMAC_PERIP_COUNTN_RES1_SHIFT      23
#define DMAC_PERIP_COUNTN_RES1_MASK       0x03800000
#define DMAC_PERIP_COUNTN_RES1_FLAGS      REG_RESERVED
#define DMAC_PERIP_COUNTN_RES1_LENGTH     3

#define DMAC_PERIP_LIST_FINCTL_OFFSET     DMAC_PERIP_COUNTN_OFFSET
#define DMAC_PERIP_LIST_FINCTL_SHIFT      22
#define DMAC_PERIP_LIST_FINCTL_MASK       0x00400000
#define DMAC_PERIP_LIST_FINCTL_FLAGS      REG_RW
#define DMAC_PERIP_LIST_FINCTL_LENGTH     1

#define DMAC_PERIP_COUNTN_RES2_OFFSET     DMAC_PERIP_COUNTN_OFFSET
#define DMAC_PERIP_COUNTN_RES2_SHIFT      21
#define DMAC_PERIP_COUNTN_RES2_MASK       0x00200000
#define DMAC_PERIP_COUNTN_RES2_FLAGS      REG_RESERVED
#define DMAC_PERIP_COUNTN_RES2_LENGTH     1

#define DMAC_PERIP_DREQ_OFFSET            DMAC_PERIP_COUNTN_OFFSET
#define DMAC_PERIP_DREQ_SHIFT             20
#define DMAC_PERIP_DREQ_MASK              0x00100000
#define DMAC_PERIP_DREQ_FLAGS             REG_RO
#define DMAC_PERIP_DREQ_LENGTH            1

#define DMAC_PERIP_SRST_OFFSET            DMAC_PERIP_COUNTN_OFFSET
#define DMAC_PERIP_SRST_SHIFT             19
#define DMAC_PERIP_SRST_MASK              0x00080000
#define DMAC_PERIP_SRST_FLAGS             REG_RW
#define DMAC_PERIP_SRST_LENGTH            1

#define DMAC_PERIP_LIST_EN_OFFSET         DMAC_PERIP_COUNTN_OFFSET
#define DMAC_PERIP_LIST_EN_SHIFT          18
#define DMAC_PERIP_LIST_EN_MASK           0x00040000
#define DMAC_PERIP_LIST_EN_FLAGS          REG_RW
#define DMAC_PERIP_LIST_EN_LENGTH         1

#define DMAC_PERIP_2D_MODE_OFFSET         DMAC_PERIP_COUNTN_OFFSET
#define DMAC_PERIP_2D_MODE_SHIFT          17
#define DMAC_PERIP_2D_MODE_MASK           0x00020000
#define DMAC_PERIP_2D_MODE_FLAGS          REG_RW
#define DMAC_PERIP_2D_MODE_LENGTH         1

#define DMAC_PERIP_EN_OFFSET              DMAC_PERIP_COUNTN_OFFSET
#define DMAC_PERIP_EN_SHIFT               16
#define DMAC_PERIP_EN_MASK                0x00010000
#define DMAC_PERIP_EN_FLAGS               REG_RW
#define DMAC_PERIP_EN_LENGTH              1

#define DMAC_PERIP_CNT_OFFSET             DMAC_PERIP_COUNTN_OFFSET
#define DMAC_PERIP_CNT_SHIFT              0
#define DMAC_PERIP_CNT_MASK               0x0000FFFF
#define DMAC_PERIP_CNT_FLAGS              REG_RW
#define DMAC_PERIP_CNT_LCNTGTH            16

/* Entire Register */
#define DMAC_PERIP_COUNTN_REG_OFFSET      DMAC_PERIP_COUNTN_OFFSET
#define DMAC_PERIP_COUNTN_REG_SHIFT       0
#define DMAC_PERIP_COUNTN_REG_MASK        (unsigned int) (0xFFFFFFFF << 0)
#define DMAC_PERIP_COUNTN_REG_FLAGS       0
#define DMAC_PERIP_COUNTN_REG_LENGTH      32
#define DMAC_PERIP_COUNTN_REG_VALID_MASK  (	DMAC_PERIP_LIST_IEN_MASK | DMAC_PERIP_BSWAP_MASK       | \
                                            DMAC_PERIP_IEN_MASK      | DMAC_PERIP_PW_MASK          | \
                                            DMAC_PERIP_DIR_MASK      | DMAC_PERIP_LIST_FINCTL_MASK | \
                                            DMAC_PERIP_DREQ_MASK     | DMAC_PERIP_SRST_MASK        | \
                                            DMAC_PERIP_LIST_EN_MASK  | DMAC_PERIP_2D_MODE_MASK     | \
                                            DMAC_PERIP_EN_MASK       | DMAC_PERIP_CNT_MASK)


/* ---- Reg - 0x0008 - Peripheral param n */
/*
        Fields      Name        Description
        ======      ====        ===========

        b31:29      ACC_DEL     Access Delay - controls the rate of transfer, each
                                DMA burst transfer will be spaced out by on the
                                system bus interface, the number of core clock
                                cycles is defined in 256 cycle units:
                                0 = zerp delay
                                1 = 256 core clock cycle delay
                                ...
                                7 = 1792 core clock cycle delay
                                (only set when LIST_EN = '0')
        b28         -           Reserved
        b27         INCR        Peripheral address increment mode (only set when
                                LIST_EN = '0')
                                0 = static peripheral address defined by ADDR.
                                1 = incrementing peripheral address, start address
                                    defined by ADDR.
        b26:24      BURST       Specifies the burst size of the transfer in
                                multiples of the number of bytes transferred within
                                system memory bus width. This should correspond
                                to the amount of data that the peripheral will
                                either be able to supply or accept from its FIFO.
                                (only set when LIST_EN = '0')
        b23         -           Reserved
        b22:00      ADDR        Specifies the address of the peripheral register
                                to be accessed in this DMA channel. This represents
                                bits 24:2 of the full address bus range (32-bit bus,
                                bits 25:3 for 64-bit bus) as the access is only to
                                system bus peripherals. (only set when LIST_EN = '0')
*/

#define DMAC_PERIP_PERADDRN_OFFSET            (0x0008)

#define DMAC_PERIP_ACC_DEL_OFFSET             DMAC_PERIP_PERADDRN_OFFSET
#define DMAC_PERIP_ACC_DEL_SHIFT              29
#define DMAC_PERIP_ACC_DEL_MASK               0xE0000000
#define DMAC_PERIP_ACC_DEL_FLAGS              REG_RW
#define DMAC_PERIP_ACC_DEL_LENGTH             3

#define DMAC_PERIP_PERADDRN_RES1_OFFSET       DMAC_PERIP_PERADDRN_OFFSET
#define DMAC_PERIP_PERADDRN_RES1_SHIFT        28
#define DMAC_PERIP_PERADDRN_RES1_MASK         0x10000000
#define DMAC_PERIP_PERADDRN_RES1_FLAGS        REG_RESERVED
#define DMAC_PERIP_PERADDRN_RES1_LENGTH       1

#define DMAC_PERIP_INCR_OFFSET                DMAC_PERIP_PERADDRN_OFFSET
#define DMAC_PERIP_INCR_SHIFT                 27
#define DMAC_PERIP_INCR_MASK                  0x08000000
#define DMAC_PERIP_INCR_FLAGS                 REG_RW
#define DMAC_PERIP_INCR_LENGTH                1

#define DMAC_PERIP_BURST_OFFSET               DMAC_PERIP_PERADDRN_OFFSET
#define DMAC_PERIP_BURST_SHIFT                24
#define DMAC_PERIP_BURST_MASK                 0x07000000
#define DMAC_PERIP_BURST_FLAGS                REG_RW
#define DMAC_PERIP_BURST_LENGTH               3

#define DMAC_PERIP_PERADDRN_RES2_OFFSET       DMAC_PERIP_PERADDRN_OFFSET
#define DMAC_PERIP_PERADDRN_RES2_SHIFT        28
#define DMAC_PERIP_PERADDRN_RES2_MASK         0x00800000
#define DMAC_PERIP_PERADDRN_RES2_FLAGS        REG_RESERVED
#define DMAC_PERIP_PERADDRN_RES2_LENGTH       1

#define DMAC_PERIP_ADDR_OFFSET                DMAC_PERIP_PERADDRN_OFFSET
#define DMAC_PERIP_ADDR_SHIFT                 0
#define DMAC_PERIP_ADDR_MASK                  0x007FFFFF
#define DMAC_PERIP_ADDR_FLAGS                 REG_RW
#define DMAC_PERIP_ADDR_LENGTH                23

/* Entire Register */
#define DMAC_PERIP_PERADDRN_REG_OFFSET        DMAC_PERIP_PERADDRN_OFFSET
#define DMAC_PERIP_PERADDRN_REG_SHIFT         0
#define DMAC_PERIP_PERADDRN_REG_MASK          (unsigned int) (0xFFFFFFFF << 0)
#define DMAC_PERIP_PERADDRN_REG_FLAGS         0
#define DMAC_PERIP_PERADDRN_REG_LENGTH        32
#define DMAC_PERIP_PERADDRN_REG_VALID_MASK    (DMAC_PERIP_ACC_DEL_MASK | DMAC_PERIP_INCR_MASK | \
                                               DMAC_PERIP_BURST_MASK | DMAC_PERIP_ADDR_MASK)


/* ---- Reg - 0x000C - IRQ status n */
/*
        Fields      Name        Description
        ======      ====        ===========

        b31:22      -           Reserved
        b21         LIST_FIN    Interrupt status for current linked-list state.
                                0 : The linked-list has not finished initiating
                                    transfers.
                                1 : The linked-list has initiated the final
                                    transfer. If LIST_IEN is set for the channel,
                                    an interrupt will be generated on the IRQ
                                    line until this bit is cleared.
                                Must be cleared to 0 by software before starting
                                a new transfer.
        b20         LIST_INT    List element status for current linked-list state.
                                0 : The last lisnked list element processed did
                                    not initiate an interrupt.
                                1 : The last linked-list element processed initiated
                                    an interrupt. If LIST_EN is set for the channel,
                                    an interrupt will be generated on the IRQ line
                                    until this bit is cleared.
                                Must be cleared to 0 by software before starting
                                a new transfer.
        b19:18      -           Reserved
        b17         FIN         Interrupt status for current transfer state.
                                0 : The transfer has not finished.
                                1 : The transfer has finished. If IEN is set for
                                    the channel, an interrupt will be generated
                                    on the IRQ line until this bit is cleared.
                                Must be cleared to 0 by software before starting
                                a new transfer.
        b16:00      -           Reserved
*/

#define DMAC_PERIP_ISTATN_OFFSET          (0x000C)

#define DMAC_PERIP_ISTATN_RES1_OFFSET     DMAC_PERIP_ISTATN_OFFSET
#define DMAC_PERIP_ISTATN_RES1_SHIFT      22
#define DMAC_PERIP_ISTATN_RES1_MASK       0xFFC00000
#define DMAC_PERIP_ISTATN_RES1_FLAGS      REG_RESERVED
#define DMAC_PERIP_ISTATN_RES1_LENGTH     10

#define DMAC_PERIP_LIST_FIN_OFFSET        DMAC_PERIP_ISTATN_OFFSET
#define DMAC_PERIP_LIST_FIN_SHIFT         21
#define DMAC_PERIP_LIST_FIN_MASK          0x00200000
#define DMAC_PERIP_LIST_FIN_FLAGS         REG_RW
#define DMAC_PERIP_LIST_FIN_LENGTH        1

#define DMAC_PERIP_LIST_INT_OFFSET        DMAC_PERIP_ISTATN_OFFSET
#define DMAC_PERIP_LIST_INT_SHIFT         20
#define DMAC_PERIP_LIST_INT_MASK          0x00100000
#define DMAC_PERIP_LIST_INT_FLAGS         REG_RW
#define DMAC_PERIP_LIST_INT_LENGTH        1

#define DMAC_PERIP_ISTATN_RES2_OFFSET     DMAC_PERIP_ISTATN_OFFSET
#define DMAC_PERIP_ISTATN_RES2_SHIFT      18
#define DMAC_PERIP_ISTATN_RES2_MASK       0x000C0000
#define DMAC_PERIP_ISTATN_RES2_FLAGS      REG_RESERVED
#define DMAC_PERIP_ISTATN_RES2_LENGTH     2

#define DMAC_PERIP_FIN_OFFSET             DMAC_PERIP_ISTATN_OFFSET
#define DMAC_PERIP_FIN_SHIFT              17
#define DMAC_PERIP_FIN_MASK               0x00020000
#define DMAC_PERIP_FIN_FLAGS              REG_RW
#define DMAC_PERIP_FIN_LENGTH             1

#define DMAC_PERIP_ISTATN_RES3_OFFSET     DMAC_PERIP_ISTATN_OFFSET
#define DMAC_PERIP_ISTATN_RES3_SHIFT      0
#define DMAC_PERIP_ISTATN_RES3_MASK       0x0001FFF
#define DMAC_PERIP_ISTATN_RES3_FLAGS      REG_RESERVED
#define DMAC_PERIP_ISTATN_RES3_LENGTH     17

/* Entire Register */
#define DMAC_PERIP_ISTATN_REG_OFFSET      DMAC_PERIP_ISTATN_OFFSET
#define DMAC_PERIP_ISTATN_REG_SHIFT       0
#define DMAC_PERIP_ISTATN_REG_MASK        (unsigned int) (0xFFFFFFFF << 0)
#define DMAC_PERIP_ISTATN_REG_FLAGS       0
#define DMAC_PERIP_ISTATN_REG_LENGTH      32
#define DMAC_PERIP_ISTATN_REG_VALID_MASK  (DMAC_PERIP_LIST_FIN_MASK | DMAC_PERIP_LIST_INT_MASK | \
                                           DMAC_PERIP_FIN_MASK)


/* ---- Reg - 0x0010 - 2D mode n */
/*
        Fields      Name            Description
        ======      ====            ===========

        b31         -               Reserved
        b30:20      REP_COUNT       Repeat count of the 2D DMA mode, as specified in
                                    figure 2 of the System Bus DMAC TRM.
                                    0 = no repeat (single line)
                                    (only set when LIST_EN = '0')
        b19:10      LINE_ADD_OFF    Line address offset of the 2D DMA mode,
                                    expressed in system memory data size units
                                    i.e. 128bits.
                                    0 = no offset
                                    (only set when LIST_EN = '0')
        b9:0        ROW_LENGTH      Row length of the 2D DMA mode, expressed in
                                    system memory data size units, i.e. 128bits.
                                    0 = 1024 system memory units (8kbytes)
                                    1 = 1 system memory unit (128bits)
                                    (only set when LIST_EN = '0')
*/

#define DMAC_PERIP_2D_MODEN_OFFSET         (0x0010)

#define DMAC_PERIP_2D_MODEN_RES_OFFSET     DMAC_PERIP_2D_MODEN_OFFSET
#define DMAC_PERIP_2D_MODEN_RES_SHIFT      31
#define DMAC_PERIP_2D_MODEN_RES_MASK       0x80000000
#define DMAC_PERIP_2D_MODEN_RES_FLAGS      REG_RESERVED
#define DMAC_PERIP_2D_MODEN_RES_LENGTH     1

#define DMAC_PERIP_REP_COUNT_OFFSET		   DMAC_PERIP_2D_MODEN_OFFSET
#define DMAC_PERIP_REP_COUNT_SHIFT         20
#define DMAC_PERIP_REP_COUNT_MASK          0x7FF00000
#define DMAC_PERIP_REP_COUNT_FLAGS         REG_RW
#define DMAC_PERIP_REP_COUNT_LENGTH        11

#define DMAC_PERIP_LINE_ADD_OFF_OFFSET     DMAC_PERIP_2D_MODEN_OFFSET
#define DMAC_PERIP_LINE_ADD_OFF_SHIFT      10
#define DMAC_PERIP_LINE_ADD_OFF_MASK       0x000FFC00
#define DMAC_PERIP_LINE_ADD_OFF_FLAGS      REG_RW
#define DMAC_PERIP_LINE_ADD_OFF_LENGTH     10

#define DMAC_PERIP_ROW_LENGTH_OFFSET       DMAC_PERIP_2D_MODEN_OFFSET
#define DMAC_PERIP_ROW_LENGTH_SHIFT        0
#define DMAC_PERIP_ROW_LENGTH_MASK         0x000003FF
#define DMAC_PERIP_ROW_LENGTH_FLAGS        REG_RESERVED
#define DMAC_PERIP_ROW_LENGTH_LENGTH       10

/* Entire Register */
#define DMAC_PERIP_2D_MODEN_REG_OFFSET     DMAC_PERIP_2D_MODEN_OFFSET
#define DMAC_PERIP_2D_MODEN_REG_SHIFT      0
#define DMAC_PERIP_2D_MODEN_REG_MASK       (unsigned int) (0xFFFFFFFF << 0)
#define DMAC_PERIP_2D_MODEN_REG_FLAGS      0
#define DMAC_PERIP_2D_MODEN_REG_LENGTH     32
#define DMAC_PERIP_2D_MODEN_REG_VALID_MASK (DMAC_PERIP_REP_COUNT_MASK | DMAC_PERIP_LINE_ADD_OFF_MASK | \
                                            DMAC_PERIP_ROW_LENGTH_MASK)



/* ---- Reg - 0x0014 - Perip addr n */
/*
        Fields      Name            Description
        ======      ====            ===========

        b31:0       ADDR            Specifies the address of the peripheral register to be
                                    accessed in this DMA channel. This represents is the byte
                                    address of the peripheral. For a 32 bit peripheral bus
                                    appropriate byte masks are applied to the bus to mask the
                                    transfer.
*/

#define DMAC_PERIP_PERADDRN2_OFFSET           (0x0014)

#define DMAC_PERIP_ADDR2_OFFSET               DMAC_PERIP_PERADDRN2_OFFSET
#define DMAC_PERIP_ADDR2_SHIFT                0
#define DMAC_PERIP_ADDR2_MASK                 0xFFFFFFFF
#define DMAC_PERIP_ADDR2_FLAGS                REG_RW
#define DMAC_PERIP_ADDR2_LENGTH               32

/* Entire Register */
#define DMAC_PERIP_PERADDRN2_REG_OFFSET       DMAC_PERIP_PERADDRN2_OFFSET
#define DMAC_PERIP_PERADDRN2_REG_SHIFT        0
#define DMAC_PERIP_PERADDRN2_REG_MASK         (unsigned int) (0xFFFFFFFF << 0)
#define DMAC_PERIP_PERADDRN2_REG_FLAGS        0
#define DMAC_PERIP_PERADDRN2_REG_LENGTH       32
#define DMAC_PERIP_PERADDRN2_REG_VALID_MASK   (DMAC_PERIP_ADDR2_MASK)


/* ---- Reg - (0x0020*total no channels) - Soft Reset */
/*
        Fields      Name            Description
        ======      ====            ===========

        b31:1       -               Reserved
        b0          SOFT_RESET      Software reset function - all logic (in all
                                    channels) except registers are reset.
                                    0 = normal operation
                                    1 = software reset is held active
*/

#define DMAC_PERIP_SOFTRESET_OFFSET           (0x0020)

#define DMAC_PERIP_SOFT_RESET_OFFSET          DMAC_PERIP_SOFTRESET_OFFSET
#define DMAC_PERIP_SOFT_RESET_SHIFT           0
#define DMAC_PERIP_SOFT_RESET_MASK            0x00000001
#define DMAC_PERIP_SOFT_RESET_FLAGS           REG_RW
#define DMAC_PERIP_SOFT_RESET_LENGTH          1

/* Entire Register */
#define DMAC_PERIP_SOFTRESET_REG_OFFSET       DMAC_PERIP_SOFTRESET_OFFSET
#define DMAC_PERIP_SOFTRESET_REG_SHIFT        0
#define DMAC_PERIP_SOFTRESET_REG_MASK         (unsigned int) (0xFFFFFFFF << 0)
#define DMAC_PERIP_SOFTRESET_REG_FLAGS        0
#define DMAC_PERIP_SOFTRESET_REG_LENGTH       32
#define DMAC_PERIP_SOFTRESET_REG_VALID_MASK   (DMAC_PERIP_SOFT_RESET_MASK)


/*
 These constants may be used to construct the DMAC linked-list

    Word    Symbol      Bits    Description
    ====    ======      ====    ===========

    0       BSWAP       31      Big/little endian byte swap. If set, byte order
                                will be reversed in all read/write transfers
                                between peripheral and memory.
            DIR         30      Specifies the direction of the transfer
                                    0 : Data is transferred from memory to the
                                        peripheral
                                    1 : Data is transferred from the peripheral
                                        to memory
            PW          29:28   Specifies the width of the peripherals DMA
                                register
                                    00 : 32 bit
                                    01 : 16 bit
                                    10 :  8 bit
    --------------------------------------------------------------------------
    1       LIST_FIN    31      Specifies whether this list element is the last
                                to be written by software.
                                    0 : No action
                                    1 : Causes List_FIN interrupt when this list
                                        element is reached, if the interrupt
                                        is enabled for this channel.
                                Note that this bit MUST be set for the last list
                                element, whether an interrupt is desired or not.
                                The exception is where a continuous loop of the
                                list elements is required, when setting of this
                                bit should be omitted. See Word 3 - LISTPTR.
            LIST_INT    30      Specified whether an interrupt should be
                                generated when this list element is reached.
                                    0 : No action
                                    1 : Causes List_INT interrupt when this list
                                        element is reached, if the interrupt
                                        is enabled for this channel.
            INCR        16      Specifies incrementing peripheral address burst
                                mode. By default all operations to the
                                peripheral are to a single register location.
                                The incrementing mode allows access to mass
                                storage devices located in the peripheral
                                address space.
                                    0 : Static peripheral address defined by ADDR
                                    1 : Incrementing peripheral address, start
                                        address defined by ADDR
            LEN         15:0    Length of the next transfer in PW units.
    --------------------------------------------------------------------------
    2       ADDR        22:0    Specifies the address of the peripheral register
                                to be accessed in this DMA channel. This
                                represents bits 24:2 of the full address bus
                                range (32-bit bus, bits 25:3 for 64-bit bus)
                                as the access is only to system bus peripherals.
    --------------------------------------------------------------------------
    3       ACC_DEL     31:29   Access Delay - controls the rate of transfer,
                                each DMA burst transfer will be spaced out by
                                on the system bus interface. The number of core
                                clock cycles is defined in 256 cycle units:
                                    0 : zero delay
                                    1 : 256 core clock cycle delay
                                    ...
                                    7 : 1792 core clock cycle delay
            BURST       28:26   Specifies the burst size of the transfer in
                                multiples of number of bytes transferred within
                                system memory bus width. This should correspond
                                to amount of data that the peripheral will
                                either be able to supply or accept from its FIFO.
                                IMPORTANT NOTE:

                                Unless FIFO depth is changed from 4, this should

                                take the value of 1, 2, 3 or 4 (never 0, 5, 6, 7).

    --------------------------------------------------------------------------
    4       2D_MODE     16      2D Mode Enable
                                    0 : disabled
                                    1 : enabled
            REP_COUNT   10:0    Repeat count of the 2D DMA mode, as specified
                                in figure 2 of the System Bus DMAC TRM.
                                    0 : no repeat (single line)
    --------------------------------------------------------------------------
    5       LINE_ADD_OFF 25:16  Line address offset of the 2D DMA mode,
                                expressed in system memory data size units
                                (i.e. 128bits).
                                    0 : no offset
            ROW_LENGTH  9:0     Row length of the 2D DMA mode, expressed in
                                system memory data size units (i.e. 128 bits).
                                    0 : 1024 system memory units (8 kbytes)
                                    1 : 1 system memory unit (128 bits)
    --------------------------------------------------------------------------
    6       SA          27:0    Byte start address of the system memory from
                                where to read or write the data during the next
                                DMA transfer.

    --------------------------------------------------------------------------
    7       LISTPTR     25:0    Byte start address of the system memory from
                                where to read the linked list element
                                describing the next transfer.

                                For the last list element this may be set to
                                zero, or the first list element address if
                                a loop is desired.

    --------------------------------------------------------------------------
*/

#define DMAC_PERIP_LL_BSWAP_SHIFT         31
#define DMAC_PERIP_LL_BSWAP_MASK          0x80000000

#define DMAC_PERIP_LL_DIR_SHIFT           30
#define DMAC_PERIP_LL_DIR_MASK            0x40000000
#define DMAC_PERIP_LL_DIR_MEM2PERIPH      0x00000000
#define DMAC_PERIP_LL_DIR_PERIPH2MEM      0x40000000

#define DMAC_PERIP_LL_PW_SHIFT            28
#define DMAC_PERIP_LL_PW_MASK             0x30000000
#define DMAC_PERIP_LL_PW_32               0x00
#define DMAC_PERIP_LL_PW_16               0x01
#define DMAC_PERIP_LL_PW_8                0x02

#define DMAC_PERIP_LL_LIST_FIN_SHIFT      31
#define DMAC_PERIP_LL_LIST_FIN_MASK       0x80000000

#define DMAC_PERIP_LL_LIST_INT_SHIFT      30
#define DMAC_PERIP_LL_LIST_INT_MASK       0x40000000

#define DMAC_PERIP_LL_INCR_SHIFT          16
#define DMAC_PERIP_LL_INCR_MASK           0x00010000

#define DMAC_PERIP_LL_LEN_SHIFT           0
#define DMAC_PERIP_LL_LEN_LENGTH          16
#define DMAC_PERIP_LL_LEN_MASK            0x0000FFFF

#define DMAC_PERIP_LL_ADDR_SHIFT          0
#define DMAC_PERIP_LL_ADDR_LENGTH		  23
#define DMAC_PERIP_LL_ADDR_MASK           0x007FFFFF

#define DMAC_PERIP_LL_ADDR_BIT_SHIFT      2
#define DMAC_PERIP_LL_ADDR_BIT_MASK       ((DMAC_PERIP_LL_ADDR_MASK >> DMAC_PERIP_LL_ADDR_SHIFT) << DMAC_PERIP_LL_ADDR_BIT_SHIFT)

#define DMAC_PERIP_LL_ACC_DEL_SHIFT       29
#define DMAC_PERIP_LL_ACC_DEL_MASK        0xE0000000

#define DMAC_PERIP_LL_BURST_SHIFT         26
#define DMAC_PERIP_LL_BURST_LENGTH        3
#define DMAC_PERIP_LL_BURST_MASK          0x1C000000

#define DMAC_PERIP_LL_SA_SHIFT            0
#define DMAC_PERIP_LL_SA_MASK             0xFFFFFFFF

#define DMAC_PERIP_LL_LISTPTR_SHIFT       0
#define DMAC_PERIP_LL_LISTPTR_MASK        0xFFFFFFFF

#define DMAC_PERIP_LL_LISTPTR_BIT_SHIFT	  2


/******************************************************************************
************************** End constant definitions ***************************
*******************************************************************************/

#endif /* __DMAC_REG_DEFS_H__ */
