/*!
*******************************************************************************
 @file   uart_hw.h

 @brief  UART Device Driver Hardware Interface

         This file contains header file information for the UART device driver.
         It defines the low level interface to the hardware.

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

#if !defined (__UART_HW_H__)
#define __UART_HW_H__

/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/

/*! TRUE defined to improve readability */
#ifndef TRUE
#define TRUE  (1)
#endif

/*! FALSE defined to improve readability */
#ifndef FALSE
#define FALSE (0)
#endif

/*! Writes value 'd' into address 'a' */
#define WRITE(a,d)    (*(volatile unsigned long *)(a) = (d))

/*! Reads value held in address 'a' */
#define READ(a)       (*(volatile unsigned long *)(a))

/******************************************************************************
************************** End constant definitions ***************************
*******************************************************************************/


/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

/*!
*******************************************************************************

 @Function              @SERReadCTS

 <b>Description:</b>\n
 This function reads the current status of the CTS modem status signal.

 \param     base_address        Serial port base address.

 \return                        CTS status (TRUE/FALSE)

*******************************************************************************/
inline static unsigned long SERReadCTS(unsigned long base_address)
{
    unsigned long temp = READ(&((SER_REG_T *)base_address)->modem_status);
    return((temp & SER_MSR_CTS_MASK) ? TRUE : FALSE);
}

/*!
*******************************************************************************

 @Function              @SERReadTrigSource

 <b>Description:</b>\n
 This function reads the IIR (interrupt identification register).

 \param     base_address        Serial port base address.

 \return                        Reason for trigger (interrupt from serial port).

*******************************************************************************/
inline static int SERReadTrigSource(unsigned long base_address)
{
    unsigned long temp = READ(&((SER_REG_T *)base_address)->fcr_iid);
    return((int)(temp & (SER_IIR_ID_MASK | SER_IIR_IPEND_MASK)));
}

/*!
*******************************************************************************

 @Function              @SERReadLSR

 <b>Description:</b>\n
 This function reads the Line Status Register.

 \param     base_address        Serial port base address.

 \return                        Contents of line status register.

*******************************************************************************/
inline static int SERReadLSR(unsigned long base_address)
{
    unsigned long temp = READ(&((SER_REG_T *)base_address)->line_status);
    return((int)(temp));
}

/*!
*******************************************************************************

 @Function              @SERTx

 <b>Description:</b>\n
 This function transmits a byte of data.

 \param     base_address        Serial port base address.
 \param     data                Data byte to be transmitted.

*******************************************************************************/
inline static void SERTx(unsigned long base_address, unsigned char data)
{
    WRITE(&((SER_REG_T *)base_address)->txrx_holding, data);
    return;
}

/*!
*******************************************************************************

 @Function              @SERRxData

 <b>Description:</b>\n
 This function reads a byte of received data.

 \param     base_address        Serial port base address.

 \return                        Byte received.

*******************************************************************************/
inline static unsigned char SERRxData(unsigned long base_address)
{
    unsigned long temp = READ(&((SER_REG_T *)base_address)->txrx_holding);
    return((unsigned char)(temp & 0xFF));
}

/*!
*******************************************************************************

 @Function              @SERTxFifoEmpty

 <b>Description:</b>\n
 This function reads the status of the transmit fifo.

 \param     base_address        Serial port base address.

 \return                        Tx Fifo status (Empty == TRUE).

*******************************************************************************/
inline static signed long SERTxFifoEmpty(unsigned long base_address)
{
    unsigned long temp = READ(&((SER_REG_T *)base_address)->line_status);
    return((temp & SER_LSR_THRE_MASK) ? TRUE : FALSE);
}

/*!
*******************************************************************************

 @Function              @SERRxFifoEmpty

 <b>Description:</b>\n
 This function reads the status of the receive fifo.

 \param     base_address        Serial port base address.

 \return                        Rx Fifo status (Empty == TRUE).

*******************************************************************************/
inline static signed long SERRxFifoEmpty(unsigned long base_address)
{
    unsigned long temp = READ(&((SER_REG_T *)base_address)->line_status);
    return((temp & SER_LSR_DR_MASK) ? FALSE : TRUE);
}

/*!
*******************************************************************************

 @Function              @SERSetRTS

 <b>Description:</b>\n
 This function sets the RTS modem control signal.

 \param     base_address        Serial port base address.

*******************************************************************************/
inline static void SERSetRTS(unsigned long base_address)
{
    WRITE(&((SER_REG_T *)base_address)->modem_control, SER_MCR_RTS_MASK); /* RTS low */
    return;
}

/*!
*******************************************************************************

 @Function              @SERClearRTS

 <b>Description:</b>\n
 This function clears the RTS modem control signal.

 \param     base_address        Serial port base address.

*******************************************************************************/
inline static void SERClearRTS(unsigned long base_address)
{
    WRITE(&((SER_REG_T *)base_address)->modem_control, 0); /* RTS high */
    return;
}

/*!
*******************************************************************************

 @Function              @SEREnableRxTrig

 <b>Description:</b>\n
 This function enables generation of a trigger when received data is available.

 \param     base_address        Serial port base address.

*******************************************************************************/
inline static void SEREnableRxTrig(unsigned long base_address)
{
    unsigned long temp = READ(&((SER_REG_T *)base_address)->interrupt_en);
    temp |= SER_IER_ERBFI_MASK | SER_IER_EDSSI_MASK | SER_IER_ELSI_MASK;
    WRITE(&((SER_REG_T *)base_address)->interrupt_en, temp);
    return;
}

/*!
*******************************************************************************

 @Function              @SERDisableRxTrig

 <b>Description:</b>\n
 This function disables generation of a trigger when received data is available.

 \param     base_address        Serial port base address.

*******************************************************************************/
inline static void SERDisableRxTrig(unsigned long base_address)
{
    unsigned long temp = READ(&((SER_REG_T *)base_address)->interrupt_en);
    temp &= ~SER_IER_ERBFI_MASK;
    WRITE(&((SER_REG_T *)base_address)->interrupt_en, temp);
    return;
}

/*!
*******************************************************************************

 @Function              @DummyWrite

 <b>Description:</b>\n
 This function writes to a dummy register. Required after each register access.

 \param     base_address        Serial port base address.

*******************************************************************************/
inline static void DummyWrite(unsigned long base_address)
{
//    WRITE((base_address + UART_DUMMY_REG_OFFSET), 0); /* dummy register */
    return;
}


/******************************************************************************
******************************** End Functions ********************************
*******************************************************************************/

#endif /* __UART_HW_H__ */
