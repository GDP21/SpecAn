/*!
******************************************************************************
 @file   : uart_bootapp.c

 @brief	

 @Author Imagination Technologies

 @date   10/07/2007

         <b>Copyright 2007 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to third parties without the
         express written permission of Imagination Technologies
         Limited, Unit 8, HomePark Industrial Estate,
         King's Langley, Hertfordshire, WD4 8LZ, U.K.

 <b>Description:</b>\n
         UART Boot Application.

 <b>Platform:</b>\n
		Platform Independent

 @Version
	     1.0

******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log: uart_bootapp.c,v $

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

 
*****************************************************************************/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES

/*============================================================================
====	I N C L U D E S
=============================================================================*/

#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <MeOS.h>

#include <img_defs.h>
#include <ioblock_defs.h>

#include <sys_util.h>
#include <system.h>

/*============================================================================
====	D E F I N E S
=============================================================================*/


/*============================================================================
====	D A T A
=============================================================================*/

typedef struct
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
} UART_T;


extern ioblock_sBlockDescriptor	IMG_asUARTBlock[];

static IMG_UINT32		g_ui32CharsSent = 0;

#define	UART_LC_N_8_1	3
#define UART_USE_FIFO

//static DQ_T							g_sTaskQueue;

volatile UART_T	*		pUARTRegs;

const unsigned long g_aui32SourceClock[10] =
{
	16384000,
	19200000,
	24000000,
	24576000,
	26000000,
	36000000,
	36684000,
	38400000,
	40000000,
	48000000
};

void InitControlComms ( IMG_VOID )
{
	#define UART_BAUD				19200
	unsigned long					ui32SourceClock;
	unsigned long					ui32Reg;
	volatile unsigned long			DLAB_HighValue = 0; 
	volatile unsigned long			DLAB_LowValue  = 0; 
	volatile unsigned long			DLAB_FullValue  = 0;

	// Read straps and determine what frequency xtal1 is at
#if !defined (__IMG_HW_FPGA__)
	ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_RESET_CFG );
	ui32Reg = (ui32Reg & 0x00000F00) >> 8;
#else
	ui32Reg = 2; // 24.0Mhz FPGA
#endif
	ui32SourceClock = g_aui32SourceClock[ui32Reg];

	// Enable UART system clock
	ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
	ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_UART0_SYS_CLK_EN, 1 );
	WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );

	// Set divider to no-divide
	WRITE_REG( CR_TOP_BASE, CR_TOP_UARTCLK_DIV, 0 );

	ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKENAB );
	ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_UART_EN, 1 );
	WRITE_REG( CR_TOP_BASE, CR_TOP_CLKENAB, ui32Reg );

	// Disable GPIO relating to UART
	ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
	ui32Reg &= ~(0xF << 12);
	WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );

	/* Set-up for 57600,N,8,1			*/
	
	DLAB_FullValue = ui32SourceClock / UART_BAUD;
	DLAB_FullValue = DLAB_FullValue / 16;
	
	DLAB_LowValue = DLAB_FullValue & 0xFF;
	DLAB_HighValue = (DLAB_FullValue & ~0xFF) >> 8;

	//Set DLAB = 1 to configure baudrate
	pUARTRegs->line_control = 0x00000080;

	pUARTRegs->interrupt_en = DLAB_HighValue;
	pUARTRegs->txrx_holding = DLAB_LowValue;

	pUARTRegs->line_control = UART_LC_N_8_1;
	pUARTRegs->modem_control = 0;				// ??????
#ifdef UART_USE_FIFO
	pUARTRegs->fcr_iid = 1;
#else
	pUARTRegs->fcr_iid = 0;						// ??????
#endif
}


void Windows_UARTWrite (	IMG_UINT32		ui32BytesToWrite,
							IMG_CHAR *		szData,
							IMG_UINT32 *	pui32BytesWritten )
{
	volatile unsigned long			temp;
	int								i;
	
	for ( i = 0; i < ui32BytesToWrite; i++ )
	{
		// Wait for transmit ready
		temp = 0;
		while ( (temp & (1 << 5)) == 0 )
		{
			temp = pUARTRegs->line_status;
		}

		// Transmit byte
		pUARTRegs->txrx_holding = szData[i];
	}

	*pui32BytesWritten = ui32BytesToWrite;
}

/*!
******************************************************************************

 @Function				main

******************************************************************************/
#if defined (__MTX_MEOS__) || defined (__META_MEOS__)

int main(int argc, char **argv)
{
	IMG_UINT32 charsWritten;

	pUARTRegs = (UART_T	*)(IMG_asUARTBlock[0].ui32Base);

	InitControlComms();

	Windows_UARTWrite( 11, "App booted!", &charsWritten );
	g_ui32CharsSent += charsWritten;

	for ( ; ; )
	{
		Windows_UARTWrite( 11, "Hello boot!", &charsWritten );
		g_ui32CharsSent += charsWritten;
	}

    return 0;
}

#else

#error CPU and OS not recognised

#endif
