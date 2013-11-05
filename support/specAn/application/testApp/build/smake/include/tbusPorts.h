/*!
*****************************************************************************

 @file      tbusPorts.h
 @brief     Definition of codes to be loaded into the PA field in EDC_BASIC_CONFIG_T,
 to select TBus transfer destinations.

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#ifndef _UCCRT_TBUS_PORTS_H_
#define _UCCRT_TBUS_PORTS_H_

/*-------------------------------------------------------------------------*/

/*
* Read port assignments for TBus channels.  These values need to be loaded
* into the PA field in EDC_BASIC_CONFIG_T to direct TBus accesses from the appropriate
* peripheral.
*/
#define EDC_PA_REG_TBUS_RD_FLAG               (1 << 15) /* Bit within the PA register signifying a read port */

#if (__UCC__ >= 330)
/* Bit mask for selecting an address generator offset */
#define EDC_TBUS_WR_PORT_EDC_AGEN_OFFSET_MASK (1 << 14) 
#endif

#define EDC_TBUS_RD_PORT_SCP                 (0 | EDC_PA_REG_TBUS_RD_FLAG)  /* SCP (ADC) output (legacy DMA1) */
#define EDC_TBUS_RD_PORT_SCP2                (1 | EDC_PA_REG_TBUS_RD_FLAG)  /* SCP 2 (ADC) output (legacy DMA7) */
#define EDC_TBUS_RD_PORT_SCP3                (2 | EDC_PA_REG_TBUS_RD_FLAG)  /* SCP 3 (ADC) output (legacy DMA8) */
#define EDC_TBUS_RD_PORT_SCP4                (3 | EDC_PA_REG_TBUS_RD_FLAG)  /* SCP 4 (ADC) output (legacy DMA9) */
#define EDC_TBUS_RD_PORT_VITERBI             (4 | EDC_PA_REG_TBUS_RD_FLAG)  /* Viterbi output (legacy DMA4, no TS Sync) */
#define EDC_TBUS_RD_PORT_DFE_1               (5 | EDC_PA_REG_TBUS_RD_FLAG)  /* DFE output 1 (legacy DMA4) */
#define EDC_TBUS_RD_PORT_DFE_2               (6 | EDC_PA_REG_TBUS_RD_FLAG)  /* DFE output 2 (legacy DMA5) */
#define EDC_TBUS_RD_PORT_DFE_3               (7 | EDC_PA_REG_TBUS_RD_FLAG)  /* DFE output 3 (legacy to TS Sync) */
#define EDC_TBUS_RD_PORT_TXP                 (8 | EDC_PA_REG_TBUS_RD_FLAG)  /* TXP output (legacy DMA5) */
#define EDC_TBUS_RD_PORT_NRGDISP             (9 | EDC_PA_REG_TBUS_RD_FLAG)  /* Energy Dispersal (Descrambler) input */
#define EDC_TBUS_RD_PORT_LDPC_SOFT          (10 | EDC_PA_REG_TBUS_RD_FLAG)  /* LDPC soft output */
#define EDC_TBUS_RD_PORT_LDPC_HARD          (11 | EDC_PA_REG_TBUS_RD_FLAG)  /* LDPC hard output */
#define EDC_TBUS_RD_PORT_PHYWAY_1           (12 | EDC_PA_REG_TBUS_RD_FLAG)  /* PHYway channel 0 output (legacy DMA1) */
#define EDC_TBUS_RD_PORT_PHYWAY_2           (13 | EDC_PA_REG_TBUS_RD_FLAG)  /* PHYway channel 1 output (legacy DMA4) */
#define EDC_TBUS_RD_PORT_PHYWAY_3           (14 | EDC_PA_REG_TBUS_RD_FLAG)  /* PHYway channel 2 output (legacy DMA7) */
#define EDC_TBUS_RD_PORT_PHYWAY_4           (15 | EDC_PA_REG_TBUS_RD_FLAG)  /* PHYway channel 3 output (legacy DMA9) */
#define EDC_TBUS_RD_PORT_TS_SYNC            (16 | EDC_PA_REG_TBUS_RD_FLAG)  /* TS Sync output */
#define EDC_TBUS_RD_PORT_RESAMP             (17 | EDC_PA_REG_TBUS_RD_FLAG)  /* Resampler output */
#define EDC_TBUS_RD_PORT_HSBDI_LDPC_RD_ADDR (18 | EDC_PA_REG_TBUS_RD_FLAG)  /* HSBDI output (LDPC Read Address Sequence) */
#define EDC_TBUS_RD_PORT_HSBDI_LDPC_WR      (19 | EDC_PA_REG_TBUS_RD_FLAG)  /* HSBDI output (LDPC Write Addr+Wr Mask+Wr Data sequence) */
#define EDC_TBUS_RD_PORT_HSBDI_DMPR_WR      (20 | EDC_PA_REG_TBUS_RD_FLAG)  /* HSBDI output (Demapper Write LLR sequence) */
#define EDC_TBUS_RD_PORT_OUTPROC            (21 | EDC_PA_REG_TBUS_RD_FLAG)  /* Output Processor output */
#define EDC_TBUS_RD_PORT_DEMAP_EXT_LLR      (22 | EDC_PA_REG_TBUS_RD_FLAG)  /* DeMapper, Extrinsic LLRs */
#define EDC_TBUS_RD_PORT_PACKER             (23 | EDC_PA_REG_TBUS_RD_FLAG)  /* Bit Packer output */
#define EDC_TBUS_RD_PORT_DEMAP_EXTEND_DATA  (24 | EDC_PA_REG_TBUS_RD_FLAG)  /* Demapper extended data output */
#if (__UCC__ >= 330)
#define EDC_TBUS_RD_PORT_DEAGG              (25 | EDC_PA_REG_TBUS_RD_FLAG)  /* De-aggregation output */
#define EDC_TBUS_RD_PORT_DFE2_1             (26 | EDC_PA_REG_TBUS_RD_FLAG)  /* DFE 2 output 1 */
#define EDC_TBUS_RD_PORT_DFE2_2             (27 | EDC_PA_REG_TBUS_RD_FLAG)  /* DFE 2 output 2 */
#define EDC_TBUS_RD_PORT_DFE2_3             (28 | EDC_PA_REG_TBUS_RD_FLAG)  /* DFE 2 output 3 */
#define EDC_TBUS_RD_PORT_TS_SYNC2           (29 | EDC_PA_REG_TBUS_RD_FLAG)  /* TS Sync 2 output */
#define EDC_TBUS_RD_PORT_VITERBI2           (30 | EDC_PA_REG_TBUS_RD_FLAG)  /* Viterbi 2 output */
#define EDC_TBUS_RD_PORT_LDPC2_SOFT         (31 | EDC_PA_REG_TBUS_RD_FLAG)  /* LDPC 2 soft output */
#define EDC_TBUS_RD_PORT_LDPC2_HARD         (32 | EDC_PA_REG_TBUS_RD_FLAG)  /* LDPC 2 hard output */
#define EDC_TBUS_RD_PORT_OUTPROC2           (33 | EDC_PA_REG_TBUS_RD_FLAG)  /* Output Processor 2 output */
#define EDC_TBUS_RD_PORT_SCP5               (34 | EDC_PA_REG_TBUS_RD_FLAG)  /* SCP 5 (ADC) output */
#define EDC_TBUS_RD_PORT_SCP6               (35 | EDC_PA_REG_TBUS_RD_FLAG)  /* SCP 6 (ADC) output */
#define EDC_TBUS_RD_PORT_SCP7               (36 | EDC_PA_REG_TBUS_RD_FLAG)  /* SCP 7 (ADC) output */
#define EDC_TBUS_RD_PORT_SCP8               (37 | EDC_PA_REG_TBUS_RD_FLAG)  /* SCP 8 (ADC) output */
#define EDC_TBUS_RD_PORT_SCP9               (38 | EDC_PA_REG_TBUS_RD_FLAG)  /* SCP 9 (ADC) output */
#define EDC_TBUS_RD_PORT_SCP10              (39 | EDC_PA_REG_TBUS_RD_FLAG)  /* SCP 10 (ADC) output */
#define EDC_TBUS_RD_PORT_SCP11              (40 | EDC_PA_REG_TBUS_RD_FLAG)  /* SCP 11 (ADC) output */
#define EDC_TBUS_RD_PORT_SCP12              (41 | EDC_PA_REG_TBUS_RD_FLAG)  /* SCP 12 (ADC) output */
#define EDC_TBUS_RD_PORT_PACKER2            (42 | EDC_PA_REG_TBUS_RD_FLAG)  /* Bit Packer 2 output */
#endif

/*
* Write port assignments for TBus channels.  These values need to be loaded
* into the pa field in EDC_BASIC_CONFIG_T to direct TBus accesses to the appropriate
* peripheral.
*/
#define EDC_TBUS_WR_PORT_VITERBI             0  /* Viterbi input (legacy DMA2, no TDI/BDI) */
#define EDC_TBUS_WR_PORT_OP_RESAMP           1  /* Output Resampler (DAC) input (legacy DMA3) */
#define EDC_TBUS_WR_PORT_TXP                 2  /* TXP input (legacy DMA6) */
#define EDC_TBUS_WR_PORT_DFE                 3  /* DFE input (legacy DMA6) */
#define EDC_TBUS_WR_PORT_TSO                 4  /* TSO input */
#define EDC_TBUS_WR_PORT_NRGDISP             5  /* Energy Dispersal (Descrambler) input */
#define EDC_TBUS_WR_PORT_LDPC                6  /* LDPC input */
#define EDC_TBUS_WR_PORT_PHYWAY_1            7  /* PHYway channel 0 input (legacy DMA2) */
#define EDC_TBUS_WR_PORT_PHYWAY_2            8  /* PHYway channel 1 input (legacy DMA3) */
#define EDC_TBUS_WR_PORT_PHYWAY_3            9  /* PHYway channel 2 input (legacy DMA6) */
#define EDC_TBUS_WR_PORT_PHYWAY_4           10  /* PHYway channel 3 input (legacy DMA8) */
#define EDC_TBUS_WR_PORT_TS_SYNC            11  /* TS Sync input */
#define EDC_TBUS_WR_PORT_RESAMP             12  /* Resampler input  */
#define EDC_TBUS_WR_PORT_HSBDI_LDPC_RD_DATA 13  /* HSBDI input (LDPC Read Returned LLR Data) */
#define EDC_TBUS_WR_PORT_HSBDI_DMPR_RD      14  /* HSBDI input (Demapper Read data) */
#define EDC_TBUS_WR_PORT_OUT_PROC_DATA      15  /* DVB-T2 Output Processor data stream input */
#define EDC_TBUS_WR_PORT_DEMAP_CELL_DATA    16  /* Demapper - Input Cell Data */
#define EDC_TBUS_WR_PORT_DEMAP_CSI_DATA     17  /* Demapper - Input CSI Data */
#define EDC_TBUS_WR_PORT_DEMAP_PRIOR_LLR    18  /* Demapper - Input Priori LLR Data */
#define EDC_TBUS_WR_PORT_PACKER             19  /* Bit Packer input */
#define EDC_TBUS_WR_PORT_OD_SYND            20  /* Outer Decoder Syndrome Codeword Symbol input */
#define EDC_TBUS_WR_PORT_OD_LAMBDA          21  /* Outer Decoder Lambda Erasure Flags input */
#define EDC_TBUS_WR_PORT_OUT_PROC_COMMON    22  /* DVB-T2 Output Processor common stream input */
#if (__UCC__ == 320)
#define EDC_TBUS_WR_PORT_EDC_AGEN_BASE      23
#else
#define EDC_TBUS_WR_PORT_DFE2               23  /* DFE 2 input */
#define EDC_TBUS_WR_PORT_TS_SYNC2           24  /* TS Sync 2 input */
#define EDC_TBUS_WR_PORT_TSO2               25  /* TSO 2 input */
#define EDC_TBUS_WR_PORT_VITERBI2           26  /* Viterbi 2 input */
#define EDC_TBUS_WR_PORT_LDPC2              27  /* LDPC 2 input */
#define EDC_TBUS_WR_PORT_OUTPROC2_DATA      28  /* Output Processor 2 data stream input */
#define EDC_TBUS_WR_PORT_OUTPROC2_COMN      29  /* Output Processor 2 common stream input */
#define EDC_TBUS_WR_PORT_RESAMP2            30  /* Resampler 2 input */
#define EDC_TBUS_WR_PORT_RESAMP3            31  /* Resampler 3 input */
#define EDC_TBUS_WR_PORT_RESAMP4            32  /* Resampler 4 input */
#define EDC_TBUS_WR_PORT_RESAMP5            33  /* Resampler 5 input */
#define EDC_TBUS_WR_PORT_PACKER2            34  /* Bit Packer 2 input */
#define EDC_TBUS_WR_PORT_OD2_SYND           35  /* Outer Decoder 2 Syndrome Codeword Symbol input */
#define EDC_TBUS_WR_PORT_OD2_LAMBDA         36  /* Outer Decoder 2 Lambda Erasure Flags input */
#endif

/* Base port of EDC address generator channel offsets */

#endif /* _UCCRT_TBUS_PORTS_H_ */
