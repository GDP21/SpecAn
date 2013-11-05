/*!
*******************************************************************************
  file   system.h

  brief  Saturn SOC parameters

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

 $Log: system.h,v $

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 


******************************************************************************/

#if !defined (__SYSTEM_H__)
#define __SYSTEM_H__

#if defined (__cplusplus)
extern "C" {
#endif

#define METAG_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include <MeOS.h>

#include <img_defs.h>


#include "cr_perip_reg_defs.h"
#include "cr_top_reg_defs.h"


#define SYSTEM_CONTROL_BASE_ADDRESS		(0x02000000)

/**********************************
** Interrupt list from reg.def 05/07/2010

[29] usb EP8
[28] usb EP7
[27] usb EP6
[26] usb EP5
[25] usb EP4
[24] usb EP3
[23] usb EP2
[22] usb EP1
[21] usb EP0
[20] power down interrupt
[19] watch dog interrupt status
[18] RTC interrupt status
[17] Infra-red interrupt status
[16] GPIO2 interrupt status
[15] GPIO1 interrupt status
[14] GPIO0 interrupt status
[13] USB Interrupt Status
[12] DMAC Chan 3 Interrupt status
[11] DMAC Chan 2 Interrupt status
[10] DMAC Chan 1  interrupt status
[9] DMAC Chan 0 Interrupt Status
[8] SOCIF Interrupt status
[7]  SPI Master 1 Interrupt status
[6]  SPI slave interrupt status
[5]  SPI Master 0 Interrupt Status
[4]  Uart 1 interrupt status
[3]  Uart 0 interrupt status
[2]  SCB2 interrupt status
[1]  SCB1 interrupt status
[0]  SCB0 interrupt status

***********************************/


/******************************************************************************
************************ Top-level (CR_PERIP, etc)    *************************
*******************************************************************************/

#define SYSTEM_REGS_OFFSET				(0x14000)
#define CR_TOP_REGS_OFFSET				(0x15800)

#define CR_PERIP_BASE					(SYSTEM_CONTROL_BASE_ADDRESS + SYSTEM_REGS_OFFSET)
#define CR_TOP_BASE						(SYSTEM_CONTROL_BASE_ADDRESS + CR_TOP_REGS_OFFSET)

/******************************************************************************
***********************************  SCB  *************************************
*******************************************************************************/

#define SCB_0_REGS_OFFSET				(0x14400)
#define SCB_1_REGS_OFFSET				(0x14600)
#define SCB_2_REGS_OFFSET				(0x14800)

#define SCB_0_INTERRUPT_BIT				(64 + 0)
#define SCB_1_INTERRUPT_BIT				(64 + 1)
#define SCB_2_INTERRUPT_BIT				(64 + 2)

/******************************************************************************
***************************** Serial Port (UART)  *****************************
*******************************************************************************/

#define UART_0_REGS_OFFSET				(0x14B00)
#define UART_1_REGS_OFFSET				(0x14C00)

#define UART_0_INTERRUPT_BIT			(64 + 3)
#define UART_1_INTERRUPT_BIT			(64 + 4)

/******************************************************************************
***********************  Serial Peripheral Interface  *************************
*******************************************************************************/

// SPIS 0 is SPI Master/Slave block
// SPIM 0 is SPI Master/Slave block
// SPIM 1 is SPI Master standalone block

#define SPIS_0_REGS_OFFSET				(0x14D80)
#define SPIS_0_INTERRUPT_BIT			(64 + 6)

#define SPIM_0_REGS_OFFSET				(0x14D00)
#define SPIM_1_REGS_OFFSET				(0x14E00)

#define SPIM_0_INTERRUPT_BIT			(64 + 5)
#define SPIM_1_INTERRUPT_BIT			(64 + 7)

/******************************************************************************
********************************  USB  ****************************************
*******************************************************************************/

#define USB_REGS_OFFSET					(0x20000)
#define USB_INTERRUPT_BIT				(64 + 13)

/******************************************************************************
********************************  GPIO  ***************************************
*******************************************************************************/

#define	GPIO_REGS_OFFSET				(0x15800)
#define GPIO_0_REGS_OFFSET				(0x15800)
#define GPIO_0_INTERRUPT_BIT			(64 + 14)
#define GPIO_1_REGS_OFFSET				(0x15804)
#define GPIO_1_INTERRUPT_BIT			(64 + 15)
#define GPIO_2_REGS_OFFSET				(0x15808)
#define GPIO_2_INTERRUPT_BIT			(64 + 16)

/******************************************************************************
*******************************  DISEQC  **************************************
*******************************************************************************/

#define	DISEQC_NUM_BLOCKS				(2)
#define DISEQC_0_REGS_OFFSET			(0x15000)
#define DISEQC_0_INTERRUPT_BIT			(64 + 30)
//#define DISEQC_0_INTERRUPT_BIT			(64 + 23)
#define DISEQC_1_REGS_OFFSET			(0x15100)
#define DISEQC_1_INTERRUPT_BIT			(64 + 31)

/******************************************************************************
**************************  System Bus DMAC  **********************************
*******************************************************************************/

#define SBDMAC_REGS_OFFSET				(0x15400)
#define SBDMAC_REGS_STRIDE				(0x20)

#define SBDMAC_REGS_OFFSET_0			(SBDMAC_REGS_OFFSET + (0 * SBDMAC_REGS_STRIDE))
#define SBDMAC_REGS_OFFSET_1			(SBDMAC_REGS_OFFSET + (1 * SBDMAC_REGS_STRIDE))
#define SBDMAC_REGS_OFFSET_2			(SBDMAC_REGS_OFFSET + (2 * SBDMAC_REGS_STRIDE))
#define SBDMAC_REGS_OFFSET_3			(SBDMAC_REGS_OFFSET + (3 * SBDMAC_REGS_STRIDE))

#define SBDMAC_INTERRUPT_BIT_0			(64 + 9)
#define SBDMAC_INTERRUPT_BIT_1			(64 + 10)
#define SBDMAC_INTERRUPT_BIT_2			(64 + 11)
#define SBDMAC_INTERRUPT_BIT_3			(64 + 12)

// DMA Request lines (READ/WRITE indicates from/to memory)
typedef enum
{
	DMA_REQUEST_NONE = 0,
	DMA_REQUEST_SPIM_0_WRITE,
	DMA_REQUEST_SPIM_0_READ,
	DMA_REQUEST_SPIS_0_WRITE,
	DMA_REQUEST_SPIS_0_READ,
	DMA_REQUEST_SPIM_1_WRITE,
	DMA_REQUEST_SPIM_1_READ,

} SYS_eDMARequestLine;

/******************************************************************************
*************************  Powerdown Controller  ******************************
*******************************************************************************/

#define PDC_REGS_OFFSET					(0x16000)
#define PDC_BASE						(SYSTEM_CONTROL_BASE_ADDRESS + PDC_REGS_OFFSET)

/*!
******************************************************************************
 MeOS (or MeOS Abstraction Layer) configuration values:
******************************************************************************/
/*! Timer tick period for MeOS (or MeOS abstraction layer). */
#define META_TIMER_FREQ			(1000000) /* 1MHz Meta timer frequency (ie 1us period) */

#define TIM_TICK_PERIOD			(META_TIMER_FREQ / 1000)	/* ie (1 / META_TIMER_FREQ) * 1000 for a 1000us (1 ms) tick period */

#define MEOS_MAX_PRIORITY_LEVEL (5)

#define QIO_NO_VECTORS          (128)

/*! To convert milliseconds to timer ticks. */
#define MILLISECOND_TO_TICK(x) ((img_uint32)(x * 1000 / TIM_TICK_PERIOD))	/* Convert ms value to us, then divide by tick period */

#define TIM_STACK_SIZE			(2048)

/******************************************************************************
*************************  Interrupt defines  *********************************
*******************************************************************************/
extern QIO_IVDESC_T		QIO_MTPIVDesc;

/*!
******************************************************************************

 CB Manager configuration values:

******************************************************************************/
/*!  his define specifies how many modules/API's can use this CB Manager    */
#define CBMAN_MAX_MODULES                   4
/*! This define specifies how callbacks each module/API can use             */
#define CBMAN_MAX_NO_OF_CALLBACKS           8


#if defined (__cplusplus)
}
#endif

#endif /* __SYSTEM_H__ */

