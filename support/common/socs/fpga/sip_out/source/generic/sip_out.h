/*
** FILE NAME:   $RCSfile: sip_out.h,v $
**
** TITLE:       Output to Streaming IP port
**
** PROJECT:		UCCP Common
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Send data out to the streaming IP port.
**				DMA data out to the SIP port provided on our FPGA platforms.
**				This is usually connected to a USB module that allows the data
**				to be captured on a connected PC.
**
**				Copyright (C) 2009, Imagination Technologies Ltd.
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

/*! \file sip_out.h
*******************************************************************************
	\brief	Output to Streaming IP port

	DMA data out to the SIP port provided on Kuroi FPGA platforms.
	This is usually connected to a USB module that allows the data
	to be captured on a connected PC.
*******************************************************************************/
/*! \mainpage SIP Output
*******************************************************************************
 \section intro Introduction

 This set of linked HTML pages provides the functional interface to the
 streaming IP port output driver code.
 The driver allows a series of blocks of data are DMA'ed out of the streaming IP port.
 If the port is busy when a block of data is sent to the driver, it is queued up and
 sent out when the port is free. The data blocks are sent out in the order they are
 presented to the driver. When a block has been sent out the registered callback
 function is called.

 <i>Note: This module will only direct its data at SIP 0. Hence only one stream
    can be output at a time.</i>

 <b>Copyright (C) 2009, Imagination Technologies Ltd.</b>

*******************************************************************************/

#ifndef SIP_OUT_H
#define SIP_OUT_H

/*!
******************************************************************************
 Callback funtion type.
 This function is used to signal out of the driver that the data block has
 been sent out of the SIP port. The parameters match a pair of data block
 start pointer and size previously queued with SIPOut_QueueData().

 <i>Note: These functions may be called from within interrupt context.</i>

  \param[in] dataBlock		Pointer to the start of a data block that has
  							been sent to the SIP 0
  \param[in] dataBlockSize	Size of data block in bytes

******************************************************************************/
typedef void (*SIPOUT_CALLBACK_FUNCTION_T)(unsigned char *dataBlock, int dataBlockSize);

/*!
******************************************************************************

 @Function @SIPOut_Init

 <b>Description:</b>\n
 This function initialises the SIPOut driver. It sets up the given DMA for use
 by the driver. This should be called prior to any other function in the module.

 \param[in] DMAChannel		The system DMAC channel for the driver to use.
 \param[in] callback		Callback function used to signal data sent out of SIP.

******************************************************************************/
void SIPOut_Init(int DMAChannel, SIPOUT_CALLBACK_FUNCTION_T callback);

/*!
******************************************************************************

 @Function @SIPOut_DeInit

 <b>Description:</b>\n
 This function de-initialises the SIPOut driver. It resets the DMA channel
 that the driver uses.
 Any blocks of data sent to the driver that have not already been returned with
 the registered callback function are returned.

 This function can only be called after SIPOut_Init(). After it has been called
 no other SIPOut functions can be called until it has been re-initialised by
 calling SIPOut_Init().

******************************************************************************/
void SIPOut_DeInit(void);

/*!
******************************************************************************

 @Function @SIPOut_QueueData

 <b>Description:</b>\n
 This function queues a block of data to be sent out of the streaming IP port.
 When the block of data has been sent out of the port, the driver's callback (as
 provided at initialisation) is called.

 \param[in] dataBlock		Pointer to the start of a data block to send to SIP 0
 \param[in] dataBlockSize	Size of data block in bytes

 \return					Non-zero if error.

******************************************************************************/
int SIPOut_QueueData(unsigned char *dataBlock, int dataBlockSize);

/*!
******************************************************************************

 @Function @SIPOut_Cancel

 <b>Description:</b>\n
 This function cancels all outstanding output jobs and returns them via the
 driver's callback function.

******************************************************************************/
void SIPOut_Cancel(void);

/*!
******************************************************************************

 @Function @SIPOut_ISR

 <b>Description:</b>\n
 This function handles the system DMAC interrupt coming from the DMAC channel
 the SIP out driver has been configured to use.

******************************************************************************/
void SIPOut_ISR(void);

#endif /* SIP_OUT_H */
