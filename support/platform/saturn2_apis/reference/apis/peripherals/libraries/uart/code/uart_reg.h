/*!
*******************************************************************************
 @file   uart_reg.h

 @brief  UART Device Driver Register Access definitions

         This file contains header file information for the UART registers.

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

#if !defined (__UART_REG_H__)
#define __UART_REG_H__


/*****************************************************************************
**                                                                          **
**                      UART REGISTER BLOCK                                 **
**                                                                          **
*****************************************************************************/

typedef struct ser_reg_t
{
    unsigned long txrx_holding;         //TX (W)/ RX (R) holding register
    unsigned long interrupt_en;         //Interrupt enable
    unsigned long fcr_iid;              //FIFO control (W) Interrupt ID (R)
    unsigned long line_control;         //Line control
    unsigned long modem_control;        //Modem control
    unsigned long line_status;          //Line status
    unsigned long modem_status;         //Modem status
    unsigned long scratch;
    //Hidden registers used for read back - not accessed directly
    unsigned long rx_holding;           //RX (R) holding register
    unsigned long divisor_lsb;          //Divisor latch LSB
    unsigned long divisor_msb;          //Divisor latch MSB
    unsigned long interrupt_id;         //ID of interrupt source
    unsigned long imask;                //Current interrupt mask
} SER_REG_T;

#define UART_DUMMY_REG_OFFSET	0xD300


/*============================================================================*/
/*                                                                            */
/*          MASKS AND SHIFTS TO ACCESS RELEVANT ELEMENTS OF REGISTERS         */
/*                                                                            */
/*============================================================================*/

//Reg 1: Interrupt Enable
#define SER_IER_EDSSI_MASK      0x00000008  //Modem status
#define SER_IER_ELSI_MASK       0x00000004  //Line status
#define SER_IER_ETBFI_MASK      0x00000002  //TX ready for data
#define SER_IER_ERBFI_MASK      0x00000001  //RX data available

#define SER_IER_EDSSI_SHIFT     3
#define SER_IER_ELSI_SHIFT      2
#define SER_IER_ETBFI_SHIFT     1
#define SER_IER_ERBFI_SHIFT     0


//Reg 2 (R): Interrupt ID register
#define SER_IIR_FEN_MASK        0x000000C0  //Fifos enabled
#define SER_IIR_ID_MASK         0x0000000E  //Interrupt ID
#define SER_IIR_IPEND_MASK      0x00000001  //Interrupt pending

#define SER_IIR_FEN_SHIFT       6
#define SER_IIR_ID_SHIFT        1
#define SER_IIR_IPEND_SHIFT     0

#define SER_IIR_TRGID           6           //Character timeout id
#define SER_IIR_RXERR           3           //Receive error id
#define SER_IIR_RXDATA          2           //Receive data id
#define SER_IIR_TXRDY           1           //Transmit ready id
#define SER_IIR_MOSTS           0           //Modem status id

//Interrupt IDs
#define TX_FIFO_EMPTY           (SER_IIR_TXRDY  << SER_IIR_ID_SHIFT)
#define RX_DATA                 (SER_IIR_RXDATA << SER_IIR_ID_SHIFT)
#define CHAR_TIMEOUT            (SER_IIR_TRGID  << SER_IIR_ID_SHIFT)
#define RECEIVER_LINE_STATUS    (SER_IIR_RXERR  << SER_IIR_ID_SHIFT)
#define MODEM_STATUS_CHANGE     (SER_IIR_MOSTS  << SER_IIR_ID_SHIFT)
#define NONE                    (SER_IIR_IPEND_MASK)


//Reg 2 (W): FIFO Control
#define SER_FCR_RXTR_MASK       0x000000C0  //Rx trigger bits
#define SER_FCR_RSTXF_MASK      0x00000004  //Reset tx fifo
#define SER_FCR_RSRXF_MASK      0x00000002  //Reset rx fifo
#define SER_FCR_ENFIFO_MASK     0x00000001  //Enable fifos

#define SER_FCR_RXTR_SHIFT      6
#define SER_FCR_RSTXF_SHIFT     2
#define SER_FCR_RSRXF_SHIFT     1
#define SER_FCR_ENFIFO_SHIFT    0

#define FCR_RXTHRES_1           0
#define FCR_RXTHRES_4           1
#define FCR_RXTHRES_8           2
#define FCR_RXTHRES_14          3


//Reg 3: Line control register
#define SER_LCR_DLAB_MASK       0x00000080  //Divisor latch accessed if = 1
#define SER_LCR_TXBK_MASK       0x00000040  //Tx break while = 1
#define SER_LCR_STPA_MASK       0x00000020  //Sticky parity force = EVPA
#define SER_LCR_EPS_MASK        0x00000010  //Even parity if = 1
#define SER_LCR_PEN_MASK        0x00000008  //Enable parity
#define SER_LCR_STB_MASK        0x00000004  //Stop bits (2 if =1 (1.5 if 5 bits))
#define SER_LCR_WLS_MASK        0x00000003  //Divisor latch accessed if = 1

#define SER_LCR_DLAB_SHIFT      7
#define SER_LCR_TXBK_SHIFT      6
#define SER_LCR_STPA_SHIFT      5
#define SER_LCR_EPS_SHIFT       4
#define SER_LCR_PEN_SHIFT       3
#define SER_LCR_STB_SHIFT       2
#define SER_LCR_WLS_SHIFT       0

#define LCR_WLS_5               0
#define LCR_WLS_6               1
#define LCR_WLS_7               2
#define LCR_WLS_8               3


//Reg 4: Modem control register
#define SER_MCR_LOOP_MASK       0x00000010  //Loopback when = 1
#define SER_MCR_RTS_MASK        0x00000002  //Logical state of RTS output
#define SER_MCR_DTR_MASK        0x00000001	//Data terminal ready

#define SER_MCR_LOOP_SHIFT      4
#define SER_MCR_RTS_SHIFT       1
#define SER_MCR_DTR_SHIFT       0


//Reg 5: Line Status Register
#define SER_LSR_RXER_MASK       0x00000080  //RX error (PE,FE,BI)
#define SER_LSR_TEMT_MASK       0x00000040  //TX underflow
#define SER_LSR_THRE_MASK       0x00000020  //TX ready for new word
#define SER_LSR_BI_MASK         0x00000010  //Break received
#define SER_LSR_FE_MASK         0x00000008  //RX framing error
#define SER_LSR_PE_MASK         0x00000004  //RX parity error
#define SER_LSR_OE_MASK         0x00000002  //RX overflow (new wor discarded)
#define SER_LSR_DR_MASK         0x00000001  //RX data available

#define SER_LSR_RXER_SHIFT      7
#define SER_LSR_TEMT_SHIFT      6
#define SER_LSR_THRE_SHIFT      5
#define SER_LSR_BI_SHIFT        4
#define SER_LSR_FE_SHIFT        3
#define SER_LSR_PE_SHIFT        2
#define SER_LSR_OE_SHIFT        1
#define SER_LSR_DR_SHIFT        0


//Reg 6: Modem status register
#define SER_MSR_DSR_MASK        0x00000020	//Data Set Ready
#define SER_MSR_CTS_MASK        0x00000010  //Logical state of CTS input
#define SER_MSR_DCTS_MASK       0x00000001  //=1 if CTS has changed >1 since last read
#define SER_MSR_DDCD_MASK       0x00000008  //=1 if DCD has changed >1 since last read

#define SER_MSR_DSR_SHIFT       5
#define SER_MSR_CTS_SHIFT       4
#define SER_MSR_DCTS_SHIFT      0
#define SER_MSR_DDCD_SHIFT      3


/*
** Hidden registers:
*/

//Imask register
#define SER_IMR_TOUT_MASK       0x00000080  //RX FIFO timeout
#define SER_IMR_CTS_MASK        0x00000040  //TX ready for new word
#define SER_IMR_THRE_MASK       0x00000020  //TX ready for new word
#define SER_IMR_BI_MASK         0x00000010  //Break received
#define SER_IMR_FE_MASK         0x00000008  //RX framing error
#define SER_IMR_PE_MASK         0x00000004  //RX parity error
#define SER_IMR_OE_MASK         0x00000002  //RX overflow (new word discarded)
#define SER_IMR_DR_MASK         0x00000001  //RX data available

#define SER_IMR_TOUT_SHIFT      7
#define SER_IMR_CTS_SHIFT       6
#define SER_IMR_THRE_SHIFT      5
#define SER_IMR_BI_SHIFT        4
#define SER_IMR_FE_SHIFT        3
#define SER_IMR_PE_SHIFT        2
#define SER_IMR_OE_SHIFT        1
#define SER_IMR_DR_SHIFT        0


/*
**	Programmable parameters used by serial port driver code
*/

//Baud Rate
#define SER_BAUD_115200			(1)
#define SER_BAUD_57600			(2)
#define SER_BAUD_38400			(3)
#define SER_BAUD_28800			(4)
#define SER_BAUD_19200			(6)
#define SER_BAUD_9600			(12)

//Word format
#define SER_B5S1P0_FMT          0	//5 bits, 1 stop, no parity
#define SER_B6S1P0_FMT          1   //6 bits, 1 stop, no parity
#define SER_B7S1P0_FMT          2   //7 bits, 1 stop, no parity
#define SER_B8S1P0_FMT          3   //8 bits, 1 stop, no parity

//RX FIFO trigger threshold
#define SER_RXTHRES_1           0	//trigger with 1 byte in FIFO
#define SER_RXTHRES_4           1	//trigger with 4 bytes in FIFO
#define SER_RXTHRES_8           2	//etc...
#define SER_RXTHRES_14          3

#endif /* __UART_REG_H__ */
