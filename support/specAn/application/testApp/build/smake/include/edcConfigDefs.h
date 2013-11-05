/*!
*****************************************************************************

 @file      edcConfigDefs.h
 @brief     EDC configuration definitions

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#ifndef _EDC_CONFIG_DEFS_H_
#define _EDC_CONFIG_DEFS_H_

/*-------------------------------------------------------------------------*/

/*
 * Platform-specific features of the EDC configuration
 */

/* The EDC_PERIP_BASIC0 register has a HOLDOFF field instead of COARSE_RATE and FINE_RATE fields */ 
#if (__UCC__ == 320)
    #define EDC_CONFIG_HAS_HOLDOFF
#endif

/* The EDC_PERIP_BASIC0 register has a PA_EXT field */ 
#if (__UCC__ >= 330)
    #define EDC_CONFIG_HAS_EXTENDED_PA
#endif

/*-------------------------------------------------------------------------*/

/*
 * Basic configuration.
 */

/** Latched channel software reset.  If PER_BUS=0 then the TBus packer/unpacker selected using PA is also reset.
**  Note: If this channel is feeding offsets to an address generator via the TBus then resetting flushes the pipelined offsets.
*/
typedef enum
{
    /** Do not reset the EDC channel     */
    EDC_LATCHED_RESET_OFF,

    /** Do reset the EDC channel     */
    EDC_LATCHED_RESET_ON
} EDC_LATCHED_RESET_T;


/** Self-clearing channel software reset.  If PER_BUS=0 then the TBus packer/unpacker selected using PA is also reset.
*   Note: If this channel is feeding offsets to an address generator via the TBus then resetting flushes the pipelined offsets
*/
typedef enum
{
    /** Do not reset the EDC channel     */
    EDC_PULSED_RESET_OFF,

    /** Do reset the EDC channel     */
    EDC_PULSED_RESET_ON
} EDC_PULSED_RESET_T;


/** Extended PA address bit for accessing BT exchange RAM */
typedef enum
{
    /** Do not use extended PA addressing */
    EDC_PA_EXT_NOT_USED,

    /** Use extended PA addressing     */
    EDC_PA_EXT_USED
} EDC_PA_EXT_T;


/** Note: Must be set to ::EDC_TBUS when ::EDC_TFR_MODE_T = ::EDC_MMR_TBUS */
typedef enum
{
    /** Peripheral transfer is carried over the transfer bus (TBus) using
     *  hardware flow control of peripheral accesses
     */
    EDC_TBUS = 0,
    
    /** Peripheral transfer is carried over the non-stalling peripheral bus
     *  (PBus) using (PA + incrOffset) as the peripheral address
     */
    EDC_PBUS
} EDC_PER_BUS_T;

/** 
 *  Notes: 1. ITEMS must match for peripheral to peripheral transfers.
 *         2. Partial address masks only supported for TBus transfers.
 */
typedef enum
{
    /** Read from memory (MA+agenOffset), write to peripheral (PA+incrOffset)
     */
    EDC_MEM_PERIP = 0,
    
    /** Read from memory (MA+agenOffset), write to EDC channel (ECN) */
    EDC_MEM_ECN,
    
    /** Read from peripheral (PA+incrOffset), write to memory (MA+agenOffset)
     */
    EDC_PERIP_MEM,
    
    /** Read from peripheral (PA+incrOffset), write to EDC channel (ECN) */
    EDC_PERIP_ECN,
    
    /** Read from EDC channel (ECN), write to memory (MA+agenOffset) */
    EDC_ECN_MEM,
    
    /** Read from EDC channel (ECN), write to peripheral (PA+incrOffset) */
    EDC_ECN_PERIP,
    
    /** Read FDSG/LFSR/PRBS offsets from MMR (MA+[0:3]), write to transfer
     *  bus (TBus)
     */
    EDC_MMR_TBUS
} EDC_TFR_MODE_T;

typedef enum
{
    /** DMA job completes after TRANSFERS*ITEMS transfers */
    EDC_EOP_TRANSFERS = 0,
    
    /** DMA job completes on receipt of EOP signal from peripheral */
    EDC_EOP_SIGNAL,
    
    /** DMA job completes after TRANSFERS*ITEMS transfers, or on receipt of
     *  EOP signal from peripheral, whichever occurs first
     */
    EDC_EOP_TRANSFERS_OR_EOP
} EDC_EOP_MODE_T;

/** Controls whether output of transfers should be discarded */
typedef enum
{
    /** Output not discarded */
    EDC_DISCARD_OFF = 0,
    
    /** Output discarded, transfers otherwise operate as normal */
    EDC_DISCARD_ON
} EDC_DISCARD_OP_T;

/** Specifies incrementing peripheral address burst mode.  By default all
 *  operations to the peripheral are to a single register location.
 */
typedef enum
{
    /** Static peripheral address defined by PERIP_ADDR.  Typically used when
     *  transferring to/from a FIFO mapped to a single data port peripheral
     *  address.
     */
    EDC_INCR_STATIC = 0,
    
    /** Incrementing peripheral address, start address defined by ADDR.
     *  Typically used when transferring to a block of configuration
     *  registers.
     */
    EDC_INCR_INC
} EDC_INCR_T;

/** Specifies bit reversal within transferred bytes
 *  Note : Just the parameter for the destination channel is used when linked using ECN.
 */
typedef enum
{
    /** Bytes are not bit reversed */
    EDC_BITREV_OFF= 0,

    /** Bytes are bit reversed */
    EDC_BITREV_ON
} EDC_BITREV_T;

/** Defines the delay between bursts on this channel.
 *  Note: HOLDOFF is unused for TFR_MODE=100,101, where the HOLDOFF for the
 *        ECN linked channel with TFR_MODE=001,011 is used once for both
 *        transfers. 
 */
typedef enum
{
    /** 0 cycles holdoff between each burst */
    EDC_HOLDOFF_0 = 0,
    
    /** 1 cycles holdoff between each burst */
    EDC_HOLDOFF_1,
    
    /** 2 cycle holdoff between each burst */
    EDC_HOLDOFF_2,
    
    /** 4 cycles holdoff between each burst */
    EDC_HOLDOFF_4,
    
    /** 8 cycles holdoff between each burst */
    EDC_HOLDOFF_8,
    
    /** 16 cycles holdoff between each burst */
    EDC_HOLDOFF_16,
    
    /** 32 cycles holdoff between each burst */
    EDC_HOLDOFF_32,
    
    /** 64 cycles holdoff between each burst */
    EDC_HOLDOFF_64
} EDC_HOLDOFF_T;

/**
 * Together, the coarse rate (CR) and fine rate (FR) define the rate at which
 * 12-byte bursts may be transferred over a channel, according to the following
 * expression:
 *   (sysclk * 12 * FR * CR) bytes/second
 */

typedef enum
{
    /** A transfer rate factor of 1/8 */
    EDC_COARSE_RATE_1_OVER_8 = 0,

    /** A transfer rate factor of 1/16 */
    EDC_COARSE_RATE_1_OVER_16,

    /** A transfer rate factor of 1/32 */
    EDC_COARSE_RATE_1_OVER_32,

    /** A transfer rate factor of 1/64 */
    EDC_COARSE_RATE_1_OVER_64,

    /** A transfer rate factor of 1/128 */
    EDC_COARSE_RATE_1_OVER_128,

    /** A transfer rate factor of 1/256 */
    EDC_COARSE_RATE_1_OVER_256,

    /** A transfer rate factor of 1/512 */
    EDC_COARSE_RATE_1_OVER_512,

    /** A transfer rate factor of 1/1024 */
    EDC_COARSE_RATE_1_OVER_1024
} EDC_COARSE_RATE_T;

typedef enum
{
    /** A transfer rate factor of 7 */
    EDC_FINE_RATE_7 = 0,

    /** A transfer rate factor of 6 */
    EDC_FINE_RATE_6,

    /** A transfer rate factor of 5 */
    EDC_FINE_RATE_5,

    /** A transfer rate factor of 4 */
    EDC_FINE_RATE_4
} EDC_FINE_RATE_T;

/** Specifies the alignment of peripheral data */
typedef enum
{
    /** Data is left aligned (conventional UCC data alignment) */
    EDC_PALIGN_LEFT = 0,
    
    /** Data is right aligned (MDC and SysBus DMAC requirement) */
    EDC_PALIGN_RIGHT
} EDC_PALIGN_T;

/** Specifies the width of the peripheral transfer.  If transfers are being
 *  performed on the wide Tbus then parallel transfers of this width take
 *  place.
 */
typedef enum
{
    /** 8 bit */
    EDC_PW_8BIT = 0,
    
    /** 16 bit */
    EDC_PW_16BIT,
    
    /** 24 bit */
    EDC_PW_24BIT
} EDC_PW_T;


/** Specifies the priority of DCP thread execution: */
typedef enum
{
    /** Low priority DCP thread.*/
    EDC_DCP_PRIORITY_NORMAL = 0,

    /** High priority DCP thread (use for realtime peripherals such as the SCP). */
    EDC_DCP_PRIORITY_HIGH
} EDC_DCP_PRIORITY_T;

/** Big/little endian byte swap.  If set, byte order will be reversed in all
 *  memory read/write transfers.  MEM_BSWAP is applied on 24-bit GRAM or
 *  32-bit External RAM memory accesses.  MEM_BSWAP is applied on the memory
 *  word after the peripheral data to transfer has been packed, or on the
 *  memory word read from RAM before it is unpacked for transfer to the
 *  peripheral.
 */
typedef enum
{
    /** Byte swap off */
    EDC_BSWAP_OFF = 0,
    
    /** Byte swap on */
    EDC_BSWAP_ON
} EDC_BSWAP_T;

/*-------------------------------------------------------------------------*/

/*
 * Address generator configuration.
 */

/** Address generator mode */
typedef enum
{
    /** Modulo */
    EDC_AGEN_MODULO = 0,
    
    /** Repeat-skip */
    EDC_AGEN_REPEAT_SKIP,
    
    /** Row-column */
    EDC_AGEN_ROW_COLUMN,
    
    /** Twisted row-column */
    EDC_AGEN_TWISTED_ROW_COLUMN,
    
    /** Burst row-column */
    EDC_AGEN_BURST_ROW_COLUMN,
    
    /** FIFO */
    EDC_AGEN_FIFO,
    
    /** Run-length */
    EDC_AGEN_RUN_LENGTH,

    /** Row-column convolutional de-interleaver */
    EDC_AGEN_ROW_COLUMN_CD
} EDC_AGEN_MODE_T;

/** EDC memory types */
typedef enum
{
    /** Global RAM */
    EDC_GRAM = 0,
    
    /** External RAM */
    EDC_EXTRAM
} EDC_MEM_TYPE_T;

/** Controls linking of even numbered write channel 'N' and odd numbered read
 *  channel 'N+1'.
 */
typedef enum
{
    /** Linking disabled */
    EDC_LINK_DISABLED = 0,
    
    /** Link using FIFO mode */
    EDC_LINK_FIFO,
    
    /** Link using BLOCK mode */
    EDC_LINK_BLOCK
} EDC_LINK_MODE_T;

/*-------------------------------------------------------------------------*/

/** Structure to describe basic channel configuration, except for
 *  length (transfers and items) and MA, which are handled separately.
 */
typedef struct edc_basic_config_t
{
    EDC_LATCHED_RESET_T latchedReset;
    EDC_PULSED_RESET_T  pulsedReset;
#ifdef EDC_CONFIG_HAS_EXTENDED_PA
    EDC_PA_EXT_T        paExt;
#endif
    EDC_PER_BUS_T       perBus;
    EDC_TFR_MODE_T      tfrMode;
    EDC_EOP_MODE_T      eopMode;
    EDC_DISCARD_OP_T    discardOp;
    EDC_INCR_T          incr;
    EDC_BITREV_T        bitrev;
#ifdef EDC_CONFIG_HAS_HOLDOFF
    EDC_HOLDOFF_T       holdoff;
#else
    EDC_COARSE_RATE_T   coarseRate;
    EDC_FINE_RATE_T     fineRate;
#endif
    EDC_PALIGN_T        palign;
    EDC_PW_T            pw;
    EDC_DCP_PRIORITY_T  dcpPriority;
    EDC_BSWAP_T         bswap;
    uint8_t             ecn;
    uint16_t            pa;
} EDC_BASIC_CONFIG_T;

/** Structure to describe address generator configuration */
typedef struct edc_agen_config_t
{
    EDC_AGEN_MODE_T agenMode;
    uint8_t itemBytes;
    EDC_MEM_TYPE_T memType;
    uint16_t n0;
    uint16_t n1;
    uint32_t n2;
    EDC_LINK_MODE_T linkMode;
    uint8_t lp1;
    uint16_t lp0;
} EDC_AGEN_CONFIG_T;

/** Stucture to describe job length */
typedef struct edc_length_t
{
    uint16_t items;
    uint8_t  transfers;
} EDC_LENGTH_T;

/*-------------------------------------------------------------------------*/

#endif /* _EDC_CONFIG_DEFS_H_ */

/*-------------------------------------------------------------------------*/
