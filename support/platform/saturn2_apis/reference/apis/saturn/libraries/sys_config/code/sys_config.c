/*!
*******************************************************************************
  file   sys_config.c

  brief  Saturn system configuration

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

 $Log: sys_config.c,v $

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

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 


******************************************************************************/

#include <stdlib.h>
#include <img_defs.h>
#include <sys_util.h>
#include <system.h>

#include "sys_config.h"

#define FP_SCALE				(20)

// XTAL1 frequencies
IMG_UINT32		g_aui32XTALFrequencies_fp[10] =
{
	0x010624DD,		// 16.384
	0x01333333,		// 19.2
	0x01800000,		// 24
	0x0189374B,		// 24.576
	0x01A00000,		// 26
	0x02400000,		// 36
	0x024DD2F1,		// 36.864
	0x02666666,		// 38.4
	0x02800000,		// 40
	0x03000000		// 48
};

static IMG_BOOL					g_bSysConfigureCalled = IMG_FALSE;
static IMG_UINT8				g_ui8XTALIndex;
/* The frequency of XTAL2, in MHz, Q12.20 format */
static IMG_UINT32				g_ui32XTAL2Freq_fp;
static IMG_UINT32				g_ui32SysUndeletedFreq_fp;
static IMG_BOOL					g_bWIFIEnabled;
static IMG_BOOL					g_bADCPLLClockConfigured;
static SYS_sADCPLLClockConfig	g_sADCPLLClockConfig;

/******************************************************************************
**************************** AFE GTI data/regs ********************************
*******************************************************************************/
IMG_UINT8	aaui8RXADCReg[3][6] =
{
	{ 0x3F, 0x3E, 0x00, 0x01, 0x07, 0x08 },	/* Reg Addr */
	{ 0x0F, 0x3F, 0x07, 0x38, 0xC0, 0xC0 }, /* Reg Mask */
	{ 0,    0,    0,    3,    6,    6    }  /* Reg Shift */
};

IMG_UINT8	aaui8RXADCRegValues[8][6] =
{
	{ 0xE, 0x3F, 0x5, 0x5, 0x3, 0x3 }, /* 2 */
	{ 0xE, 0x1D, 0x5, 0x5, 0x3, 0x3 }, /* 4 */
	{ 0xD, 0x0C, 0x5, 0x5, 0x3, 0x3 }, /* 8 */
	{ 0xC, 0x04, 0x0, 0x0, 0x0, 0x0 }, /* 16 */
	{ 0xC, 0x05, 0x0, 0x0, 0x0, 0x0 }, /* 20 */
	{ 0xA, 0x07, 0x0, 0x0, 0x0, 0x0 }, /* 30 */
	{ 0x9, 0x00, 0x0, 0x0, 0x0, 0x0 }, /* 40 */
	{ 0x0, 0x00, 0x0, 0x0, 0x0, 0x0 }  /* 80 */
};

IMG_UINT8	aaui8IQADCReg[3][6] =
{
	{ 0x4B, 0x4A, 0x1F, 0x20, 0x45, 0x46 }, /* Reg Addr */
	{ 0x0F, 0x3F, 0x07, 0x38, 0xC0, 0xC0 }, /* Reg Mask */
	{ 0,    0,    0,    3,    6,    6    }  /* Reg Shift */
};

IMG_UINT8	aaui8IQADCRegValues[8][6] =
{
	{ 0x3, 0x33, 0x0, 0x0, 0x0, 0x0 }, /* 2 */
	{ 0x3, 0x11, 0x0, 0x0, 0x0, 0x0 }, /* 4 */
	{ 0x0, 0x00, 0x0, 0x0, 0x0, 0x0 }, /* 8 */
	{ 0x1, 0x08, 0x5, 0x5, 0x3, 0x3 }, /* 16 */
	{ 0x1, 0x09, 0x5, 0x5, 0x3, 0x3 }, /* 20 */
	{ 0x7, 0x0B, 0x5, 0x5, 0x3, 0x3 }, /* 30 */
	{ 0x4, 0x0C, 0x5, 0x5, 0x3, 0x3 }, /* 40 */
	{ 0xD, 0x0C, 0x5, 0x5, 0x3, 0x3 }  /* 80 */
};

static IMG_VOID SYS_setupSystemClock( SYS_sConfig	*	psConfig	);
#if !defined (__IMG_HW_FPGA__)
static IMG_VOID SYS_setupUCCClocks( SYS_sConfig	*		psConfig );
IMG_VOID	SYS_getDividerValues(	const IMG_UINT32		ui32SourceFreq_fp,
									const IMG_UINT32		ui32TargetFreq_fp,
									const IMG_UINT32		ui32DividerRange,
									 	  IMG_UINT32	 *	pui32DivideBy,
									 	  IMG_UINT32	 *	pui32ActualFreq_fp	);

static IMG_VOID SYS_getPLLSetupValues(	IMG_UINT32		ui32SourceFreq_fp,
										IMG_UINT32		ui32TargetFreq_fp,
										IMG_UINT16	*	pui16CLKF,
										IMG_UINT8	*	pui8CLKR,
										IMG_UINT8	*	pui8CLKOD,
										IMG_UINT16	*	pui16BWAdj,
										IMG_UINT32	*	pui32ActualFreq_fp );
#endif

/*!
*******************************************************************************

 @Function              @SYS_Configure

 <b>Description:</b>\n
 This function sets up the PLL for the target platform

 \param						None

 \return                    None

*******************************************************************************/
IMG_VOID SYS_Configure( SYS_sConfig	*	psConfig )
{
	IMG_UINT32				ui32Reg;

#if	defined	(__IMG_HW_FPGA__)
	g_ui32SysUndeletedFreq_fp = 0x01800000;
	g_bSysConfigureCalled = IMG_TRUE;
	return;
#endif

	g_bADCPLLClockConfigured = IMG_FALSE;
	IMG_MEMSET ( &g_sADCPLLClockConfig, 0, sizeof(g_sADCPLLClockConfig) );

	// Get the XTAL frequency
	ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_RESET_CFG );
	g_ui8XTALIndex = (ui32Reg & 0x00000F00 ) >> 8;

	// Check that index is valid
	IMG_ASSERT ( g_ui8XTALIndex < (sizeof(g_aui32XTALFrequencies_fp)/sizeof(g_aui32XTALFrequencies_fp[0])) );

	g_ui32XTAL2Freq_fp = psConfig->ui32XTAL2Freq_fp;

	// Configure X2 for oscillator
	if ( psConfig->bUseXTAL2AsOsc )
	{
		// Read from SOC_GPIO_CONTROL2
		ui32Reg = *(IMG_UINT32 *)(PDC_BASE + 0x0508);
		ui32Reg |= (0x3 << 7);
		*(IMG_UINT32 *)(PDC_BASE + 0x0508) = (IMG_UINT32)(ui32Reg);
	}

	// Enable E-fuse for Wifi and others
	ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_READ_EFUSE );
	ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_READ_BANK, 0x1 );
	WRITE_REG( CR_TOP_BASE, CR_TOP_READ_EFUSE, ui32Reg );
	// Poll on bank 0
	do
	{
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_READ_EFUSE );
	} while ( READ_REG_FIELD( ui32Reg, CR_TOP_EFUSE_BANK0 ) == 0 );
	ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_READ_BANK, 0x3 );
	WRITE_REG( CR_TOP_BASE, CR_TOP_READ_EFUSE, ui32Reg );
	// Poll on bank 1
	do
	{
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_READ_EFUSE );
	} while ( READ_REG_FIELD( ui32Reg, CR_TOP_EFUSE_BANK1 ) == 0 );
	// Efuse read now - Wifi clocks enabled

	if ( psConfig->bSetupSystemClock )
	{
		// Setup system clock
		SYS_setupSystemClock( psConfig );
	}
	else
	{
		// Find out which clock is feeding meta
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );
		if ( READ_REG_FIELD( ui32Reg, CR_TOP_SYSCLK1_SW ) == 0 )
		{
			// XTAL1 is Meta's source
			psConfig->ui32ActualFreq_fp = g_aui32XTALFrequencies_fp[ g_ui8XTALIndex ];
		}
		else if ( READ_REG_FIELD( ui32Reg, CR_TOP_SYSCLK1_SW ) == 1 )
		{
			IMG_UINT32	ui32SourceFreq_fp;
			// PLL source
			// Get the PLL source - either XTAL1 or XTAL2
			if ( READ_REG_FIELD( ui32Reg, CR_TOP_SYSCLK0_SW ) )
			{
				ui32SourceFreq_fp = psConfig->ui32XTAL2Freq_fp;
			}
			else
			{
				ui32SourceFreq_fp = g_aui32XTALFrequencies_fp[ g_ui8XTALIndex ];
			}

			// Check the PLL is out of reset/out of bypass
			ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_SYSPLL_CTL1 );
			if ( READ_REG_FIELD( ui32Reg, CR_TOP_SYSPLL_RESET ) || READ_REG_FIELD( ui32Reg, CR_TOP_SYSPLL_BYPASS ) )
			{
				// PLL is bypassed - so XTAL1/XTAL2 is going through PLL unmodified to Meta
				psConfig->ui32ActualFreq_fp = ui32SourceFreq_fp;
			}
			else
			{
				// PLL is enabled - calculate resulting frequency
				IMG_UINT64	ui32Freq_fp;
				IMG_UINT32  ui32Ctl0;
				IMG_UINT32	ui32Temp_fp;

				ui32Ctl0 = READ_REG( CR_TOP_BASE, CR_TOP_SYSPLL_CTL0 );

				ui32Freq_fp = READ_REG_FIELD( ui32Ctl0, CR_TOP_SYSPLL_CLKF );
				ui32Freq_fp = (ui32Freq_fp + 1) << FP_SCALE; // f = clkF + 1
				ui32Freq_fp >>= 1;	// f = f / 2 [ f = (clkF + 1) / 2 ]
				ui32Freq_fp = ((IMG_UINT64)ui32Freq_fp * (IMG_UINT64)ui32SourceFreq_fp) >> FP_SCALE; // f = f * source [ f = source * (clkF + 1) / 2 ]
				ui32Temp_fp = READ_REG_FIELD( ui32Reg, CR_TOP_SYSPLL_CLKR );
				ui32Temp_fp = (ui32Temp_fp + 1) << FP_SCALE;
				ui32Freq_fp = ((IMG_UINT64)ui32Freq_fp << FP_SCALE) / ((IMG_UINT64)ui32Temp_fp); // f = f / (clkR + 1) [ f = source / (clkR + 1) * ((clkF + 1) / 2) ]
				ui32Temp_fp = READ_REG_FIELD( ui32Ctl0, CR_TOP_SYSPLL_CLKOD );
				ui32Temp_fp = (ui32Temp_fp + 1) << FP_SCALE;
				ui32Freq_fp = ((IMG_UINT64)ui32Freq_fp << FP_SCALE) / ((IMG_UINT64)ui32Temp_fp); // f = f / (clkOD + 1) [ f = source / (clkR + 1) * ((clkF + 1) / 2) / (clkOD + 1) ]

				psConfig->ui32ActualFreq_fp = ui32Freq_fp;
			}
		}

		// Find out whether the system is running at half the Meta frequency
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_META_CLKDIV );
		if ( ui32Reg & 0x01 )
		{
			g_ui32SysUndeletedFreq_fp = psConfig->ui32ActualFreq_fp >> 1;
		}
		else
		{
			g_ui32SysUndeletedFreq_fp = psConfig->ui32ActualFreq_fp;
		}
	}



	// Setup UCC clocks
#if !defined (__IMG_HW_FPGA__)
	SYS_setupUCCClocks( psConfig );
#endif

#if !defined (__IMG_HW_FPGA__)
	if ( psConfig->bEnableWIFI )
	{
		// Disable GPIO for WIFI control
		ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT1 );
		ui32Reg &= ~(0x3FF << 16);
		WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT1, ui32Reg );

		ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT2 );
		ui32Reg &= ~(0x1F << 0);
		WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT2, ui32Reg );
	}
#endif
	g_bWIFIEnabled = psConfig->bEnableWIFI;

	g_bSysConfigureCalled = IMG_TRUE;
}

IMG_VOID SYS_ConfigureSPIM(	SYS_sSPIMConfig		asConfig[2]	)
{
	IMG_UINT32	ui32Reg;

	IMG_ASSERT ( g_bSysConfigureCalled == IMG_TRUE );

	if ( asConfig[0].bConfigure )
	{
		if ( asConfig[0].bEnable )
		{
			// Enable register clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPIM_SYS_CLK_EN, 1 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );
			// Switch combined block to SPIM
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SPI_CTRL );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPI_SLAVE_EN, 0 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPI_CLK_OUTPUT, 1 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_SPI_CTRL, ui32Reg );
		#if !defined (__IMG_HW_FPGA__)
			// Configure interface clock divider
			if ( asConfig[0].bTargetFrequency )
			{
				IMG_UINT32		ui32Divider;
				SYS_getDividerValues(	g_ui32SysUndeletedFreq_fp,
										asConfig[0].ui32TargetFreq_fp,
										256,
										&ui32Divider,
										&asConfig[0].ui32ActualFreq_fp );

				WRITE_REG( CR_TOP_BASE, CR_TOP_SPICLK_DIV, (ui32Divider - 1) );
			}
			// Enable SPIM0 pads
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg &= ~(0x3F << 0);
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
		else
		{
			// Disable register clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPIM_SYS_CLK_EN, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );
			// Reset master/slave switch
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SPI_CTRL );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPI_SLAVE_EN, 0 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPI_CLK_OUTPUT, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_SPI_CTRL, ui32Reg );
		#if !defined (__IMG_HW_FPGA__)
			// Reset clock divider
			WRITE_REG( CR_TOP_BASE, CR_TOP_SPICLK_DIV, 0 );
			// Disable SPIM0 pads
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg |= (0x3F << 0);
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
	}
	if ( asConfig[1].bConfigure )
	{
		if ( asConfig[1].bEnable )
		{
			// Enable register clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPIM1_SYS_CLK_EN, 1 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );
		#if !defined (__IMG_HW_FPGA__)
			// Configure interface clock divider
			if ( asConfig[1].bTargetFrequency )
			{
				IMG_UINT32		ui32Divider;
				SYS_getDividerValues(	g_ui32SysUndeletedFreq_fp,
										asConfig[1].ui32TargetFreq_fp,
										256,
										&ui32Divider,
										&asConfig[1].ui32ActualFreq_fp );

				WRITE_REG( CR_TOP_BASE, CR_TOP_SPI1CLK_DIV, (ui32Divider - 1) );
			}
			// Enable SPIM1 pads
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg &= ~(0x3F << 6);
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
		else
		{
			// Disable register clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPIM1_SYS_CLK_EN, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );
		#if !defined (__IMG_HW_FPGA__)
			// Reset clock divider
			WRITE_REG( CR_TOP_BASE, CR_TOP_SPI1CLK_DIV, 0 );
			// Disable SPIM1 pads
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg |= (0x3F << 6);
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
	}
}

IMG_VOID SYS_ConfigureSPIS(	SYS_sSPISConfig *	psConfig	)
{
	IMG_UINT32	ui32Reg;

	IMG_ASSERT ( g_bSysConfigureCalled == IMG_TRUE );

	if ( psConfig->bConfigure )
	{
		if ( psConfig->bEnable )
		{
		// Enable register/core clock
		ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPIS_SYS_CLK_EN, 1 );
		WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );

		// Reset block
		ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SRST );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPI_SOFT_RESET, 1 );
		WRITE_REG( CR_PERIP_BASE, CR_PERIP_SRST, ui32Reg );

		ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SRST );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPI_SOFT_RESET, 0 );
		WRITE_REG( CR_PERIP_BASE, CR_PERIP_SRST, ui32Reg );

		// Set slave mode for the combo block
		ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SPI_CTRL );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPI_SLAVE_EN, 1 );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPI_CLK_OUTPUT, 0 );
		WRITE_REG( CR_PERIP_BASE, CR_PERIP_SPI_CTRL, ui32Reg );
	#if !defined (__IMG_HW_FPGA__)
		// Disable GPIO
		ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
		ui32Reg &= ~(0x3F << 8);
		WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
	#endif
		}
		else
		{
			// Disable register/core clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPIS_SYS_CLK_EN, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );

			// Reset master/slave switch
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SPI_CTRL );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPI_SLAVE_EN, 0 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SPI_CLK_OUTPUT, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_SPI_CTRL, ui32Reg );
		#if !defined (__IMG_HW_FPGA__)
			// Enable GPIO
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg |= (0x3F << 8);
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
	}
}

IMG_VOID SYS_ConfigureSCB(	SYS_sSCBConfig *	psConfig	)
{
	IMG_UINT32	ui32Reg;

	IMG_ASSERT ( g_bSysConfigureCalled == IMG_TRUE );

	if ( psConfig->asBlockConfig[0].bConfigure )
	{
		if ( psConfig->asBlockConfig[0].bEnable )
		{
			// Enable register/core clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SCB0_SYS_CLK_EN, 1 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );

			// Reset core
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SRST );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SCB0_SOFT_RESET, 1 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_SRST, ui32Reg );

			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SRST );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SCB0_SOFT_RESET, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_SRST, ui32Reg );
		#if !defined (__IMG_HW_FPGA__)
			// Disable GPIO
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg &= ~( 0x3 << 18 );
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
		else
		{
			// Disable register/core clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SCB0_SYS_CLK_EN, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );
		#if !defined (__IMG_HW_FPGA__)
			// Enable GPIO
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg |= ( 0x3 << 18 );
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
	}
	if ( psConfig->asBlockConfig[1].bConfigure )
	{
		if ( psConfig->asBlockConfig[1].bEnable )
		{
			// Enable register/core clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SCB1_SYS_CLK_EN, 1 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );

			// Reset core
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SRST );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SCB1_SOFT_RESET, 1 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_SRST, ui32Reg );

			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SRST );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SCB1_SOFT_RESET, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_SRST, ui32Reg );
		#if !defined (__IMG_HW_FPGA__)
			// Disable GPIO
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg &= ~( 0x3 << 20 );
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
		else
		{
			// Disable register/core clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SCB1_SYS_CLK_EN, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );
		#if !defined (__IMG_HW_FPGA__)
			// Enable GPIO
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg |= ( 0x3 << 20 );
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
	}
	if ( psConfig->asBlockConfig[2].bConfigure )
	{
		if ( psConfig->asBlockConfig[2].bEnable )
		{
			// Enable register/core clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SCB2_SYS_CLK_EN, 1 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );

			// Reset core
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SRST );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SCB2_SOFT_RESET, 1 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_SRST, ui32Reg );

			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SRST );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SCB2_SOFT_RESET, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_SRST, ui32Reg );
		#if !defined (__IMG_HW_FPGA__)
			// Disable GPIO
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg &= ~( 0x3 << 22 );
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
		else
		{
			// Disable register/core clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_SCB2_SYS_CLK_EN, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );
		#if !defined (__IMG_HW_FPGA__)
			// Enable GPIO
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg |= ( 0x3 << 22 );
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
	}

	// COMMON TO ALL SCB BLOCKS BELOW
#if !defined (__IMG_HW_FPGA__)
	ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
	if ( !READ_REG_FIELD( ui32Reg, CR_PERIP_SCB0_SYS_CLK_EN ) &&
			!READ_REG_FIELD( ui32Reg, CR_PERIP_SCB1_SYS_CLK_EN ) &&
			!READ_REG_FIELD( ui32Reg, CR_PERIP_SCB2_SYS_CLK_EN ) )
	{
		// If all the register clocks are disabled, we can turn off the bus clock
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKENAB );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SCB_EN, 0 );
		WRITE_REG( CR_TOP_BASE, CR_TOP_CLKENAB, ui32Reg );
	}
	else
	{
		// Turn on the bus clock
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKENAB );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SCB_EN, 1 );
		WRITE_REG( CR_TOP_BASE, CR_TOP_CLKENAB, ui32Reg );
	}

	// Configure interface clock source
	if ( psConfig->bOverrideClockSource )
	{
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH  );
		if ( psConfig->eClockSource == CLOCK_SOURCE_XTAL1 )
		{
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SCB_SW, 0 );
		}
		else if ( psConfig->eClockSource == CLOCK_SOURCE_SYS_UNDELETED )
		{
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SCB_SW, 1 );
		}
		WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );
	}
	else
	{
		// Reset interface clock source
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH  );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SCB_SW, 0 );
		WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );
	}
#endif
}

IMG_VOID SYS_ConfigureUART(	SYS_sUARTConfig	*	psConfig	)
{
	IMG_UINT32	ui32Reg;
#if !defined (__IMG_HW_FPGA__)
	IMG_UINT32	ui32Source_fp;
#endif

	IMG_ASSERT ( g_bSysConfigureCalled == IMG_TRUE );

	if ( psConfig->asBlockConfig[0].bConfigure )
	{
		if ( psConfig->asBlockConfig[0].bEnable )
		{
			// Enable register/core clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_UART0_SYS_CLK_EN, 1 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );

			// Reset core
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SRST );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_UART0_SOFT_RESET, 1 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_SRST, ui32Reg );

			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SRST );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_UART0_SOFT_RESET, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_SRST, ui32Reg );
		#if !defined (__IMG_HW_FPGA__)
			// Disable GPIO
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg &= ~( 0xF << 12 );
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
		else
		{
			// Disable register/core clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_UART0_SYS_CLK_EN, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );
		#if !defined (__IMG_HW_FPGA__)
			// Enable GPIO
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg |= ( 0xF << 12 );
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
	}
	if ( psConfig->asBlockConfig[1].bConfigure )
	{
		if ( psConfig->asBlockConfig[1].bEnable )
		{
			// Enable register/core clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_UART1_SYS_CLK_EN, 1 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );

			// Reset core
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SRST );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_UART1_SOFT_RESET, 1 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_SRST, ui32Reg );

			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SRST );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_UART1_SOFT_RESET, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_SRST, ui32Reg );
		#if !defined (__IMG_HW_FPGA__)
			// Disable GPIO
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg &= ~( 0x3 << 16 );
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
		else
		{
			// Disable register/core clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_UART1_SYS_CLK_EN, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );
		#if !defined (__IMG_HW_FPGA__)
			// Enable GPIO
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg |= ( 0x3 << 16 );
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
	}

	// COMMON TO ALL UART BLOCKS BELOW
#if !defined (__IMG_HW_FPGA__)
	// Configure interface clock source
	if ( psConfig->bOverrideClockSource )
	{
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH  );
		if ( psConfig->eClockSource == CLOCK_SOURCE_XTAL1 )
		{
			ui32Source_fp = g_aui32XTALFrequencies_fp[ g_ui8XTALIndex ];
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_UART_SW, 0 );
		}
		else if ( psConfig->eClockSource == CLOCK_SOURCE_SYS_UNDELETED )
		{
			ui32Source_fp = g_ui32SysUndeletedFreq_fp;
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_UART_SW, 1 );
		}
		else
		{
			/* Unsupported Clock Source */
			IMG_ASSERT ( IMG_FALSE );

			/* Set to default XTAL1 */
			ui32Source_fp = g_aui32XTALFrequencies_fp[ g_ui8XTALIndex ];
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_UART_SW, 0 );
		}

		WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );
	}
	else
	{
		// Default source is XTAL1
		ui32Source_fp = g_aui32XTALFrequencies_fp[ g_ui8XTALIndex ];

		// Reset interface clock source
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH  );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_UART_SW, 0 );
		WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );
	}

	if ( psConfig->bTargetFrequency )
	{
		// Configure interface clock divider
		IMG_UINT32		ui32Divider;
		SYS_getDividerValues(	ui32Source_fp,
								psConfig->ui32TargetFreq_fp,
								256,
								&ui32Divider,
								&psConfig->ui32ActualFreq_fp );

		WRITE_REG( CR_TOP_BASE, CR_TOP_UARTCLK_DIV, (ui32Divider - 1) );
	}
	else
	{
		// Reset clock divider
		WRITE_REG( CR_TOP_BASE, CR_TOP_UARTCLK_DIV, 0 );
	}
#endif
}

IMG_VOID SYS_ConfigureUSB(	SYS_sUSBConfig *	psConfig	)
{
	IMG_UINT32	ui32Reg;

	IMG_ASSERT ( g_bSysConfigureCalled == IMG_TRUE );

	if ( psConfig->bConfigure )
	{
		if ( psConfig->bEnable )
		{
				#if !defined (__IMG_HW_FPGA__)
					IMG_UINT32	ui32Source_fp = g_aui32XTALFrequencies_fp[ g_ui8XTALIndex ]; // Default power-on clock source
				#endif

					// Enable register/core clock
					ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_USB_SYS_CLK_EN, 1 );
					WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );

					// PHY clock sources:			USB XTAL (5), core_clk_12_24_48 sources
					//
					// core_clk_12_24_48 sources:	XTAL1,
					//								XTAL2,
					//								sys_clk_undeleted,
					//								adc_pll,
					//								afe_progdiv3clk_to_soc

				#if !defined (__IMG_HW_FPGA__)
					if ( psConfig->bOverrideClockSource )
					{
						// Set up clock routing
						switch ( psConfig->eClockSource )
						{
							case CLOCK_SOURCE_XTAL1:
							{
								// CLKSWITCH
								ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );
								ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_USB_CLK_1_SW, 0 );
								ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_USB_CLK_3_SW, 0 );
								ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_USB_CLK_2_SW, 0 );
								WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );

								break;
							}
							case CLOCK_SOURCE_XTAL2:
							{
								// CLKSWITCH
								ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );
								ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_USB_CLK_1_SW, 0 );
								ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_USB_CLK_3_SW, 0 );
								ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_USB_CLK_2_SW, 1 );
								WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );

								ui32Source_fp = g_ui32XTAL2Freq_fp;
								break;
							}
							case CLOCK_SOURCE_SYS_UNDELETED:
							{
								// CLKSWITCH
								ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );
								ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_USB_CLK_1_SW, 0 );
								ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_USB_CLK_3_SW, 1 );
								WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );

								ui32Source_fp = g_ui32SysUndeletedFreq_fp;

								break;
							}
							case CLOCK_SOURCE_ADC_PLL:
							{
								// Make sure the ADC_PLL is configured
								IMG_ASSERT( g_bADCPLLClockConfigured );
								IMG_ASSERT( g_sADCPLLClockConfig.bOverrideClockSource );
								IMG_ASSERT( g_sADCPLLClockConfig.eClockSource == CLOCK_SOURCE_PLL );

								// CLKSWITCH
								ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );
								ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_USB_CLK_1_SW, 1 );
								ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_USB_CLK_0_SW, 0 );
								WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );

								ui32Source_fp = g_sADCPLLClockConfig.ui32ActualFreq_fp;

								break;
							}
							case CLOCK_SOURCE_AFE_PROGDIV3:
							{
								// CLKSWITCH
								ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );
								ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_USB_CLK_1_SW, 1 );
								ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_USB_CLK_0_SW, 1 );
								WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );

								break;
							}
							case CLOCK_SOURCE_XTALUSB:
							{
								// Do nothing here
								break;
							}
							default:
							{
								// Shouldn't get here mkay
								IMG_ASSERT(0);
								break;
							}
						}

						// CLKENAB
						ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKENAB );
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_USB_CLK_1_EN, 1 );
						WRITE_REG( CR_TOP_BASE, CR_TOP_CLKENAB, ui32Reg );
					}

					if ( psConfig->bTargetPHYClock )
					{
						IMG_UINT32	ui32PHYClock_fp = ( psConfig->ePHYClock << FP_SCALE );
						IMG_UINT32	ui32Actual_fp;
						// Check if our source and divider settings give us a valid clock
						switch ( psConfig->eClockSource )
						{
							case CLOCK_SOURCE_XTAL1:
							case CLOCK_SOURCE_XTAL2:
							case CLOCK_SOURCE_SYS_UNDELETED:
							case CLOCK_SOURCE_ADC_PLL:
							{
								// Check if the clock is invalid and see if we can configure the divider appropriately
								if ( ui32Source_fp != ui32PHYClock_fp )
								{
									IMG_UINT32	ui32Divider;
									// We can try divide down
									SYS_getDividerValues(	ui32Source_fp,
															ui32PHYClock_fp,
															256,
															&ui32Divider,
															&ui32Actual_fp );
									// Check if we have the right frequency now!
									if ( ui32Actual_fp == ui32PHYClock_fp )
									{
										// Its ok, so set up the divider
										WRITE_REG( CR_TOP_BASE, CR_TOP_USB_PLLDIV, (ui32Divider - 1) );
									}
									else
									{
										// No sirree
										IMG_ASSERT(0);
									}
								}
								break;
							}
							case CLOCK_SOURCE_AFE_PROGDIV3:
							{
								// Can't work out the frequency of progdiv3 yet...
								IMG_ASSERT(0);
								break;
							}
							default:
							{
								IMG_ASSERT(0);
							}
						}
					}

					// Set up REFCLKSEL
					// Need to assert power on reset while we change this
					ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SRST );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_USB_PHY_PON_RESET, 1 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_USB_PHY_PORTRESET, 1 );
					WRITE_REG( CR_PERIP_BASE, CR_PERIP_SRST, ui32Reg );

					ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_USB_PHY_STRAP_CONTROL );

					switch ( psConfig->ePHYClock )
					{
						case PHY_eNotSet:
						{
							// This must be set to 12/24/48!
							IMG_ASSERT(0);
							return;
						}
						case PHY_e12:
						{
							ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_USB_REFCLKDIV, 0x0 );
							break;
						}
						case PHY_e24:
						{
							ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_USB_REFCLKDIV, 0x1 );
							break;
						}
						case PHY_e48:
						{
							ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_USB_REFCLKDIV, 0x2 );
							break;
						}
						default:
						{
							IMG_ASSERT(0);
						}
					}

					if ( psConfig->eClockSource == CLOCK_SOURCE_XTALUSB )
					{
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_USB_REFCLKSEL, 0x0 );
					}
					else
					{
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_USB_REFCLKSEL, 0x2 );
					}
					WRITE_REG( CR_PERIP_BASE, CR_PERIP_USB_PHY_STRAP_CONTROL, ui32Reg );

					// De-assert PON reset
					ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_SRST );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_USB_PHY_PON_RESET, 0 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_USB_PHY_PORTRESET, 0 );
					WRITE_REG( CR_PERIP_BASE, CR_PERIP_SRST, ui32Reg );

					// Now configure the PHY

					// Turn isolation off
					ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_USB_PHY_STRAP_CONTROL );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_USB_ISO_PHY, 1 );
					WRITE_REG( CR_PERIP_BASE, CR_PERIP_USB_PHY_STRAP_CONTROL, ui32Reg );

					// Wait until PHY and UTMI are ready
					while ( 1 )
					{
						ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_USB_PHY_STATUS );
						if ( ( READ_REG_FIELD( ui32Reg, CR_PERIP_USB_RX_PHY_CLK ) ) &&
							 ( READ_REG_FIELD( ui32Reg, CR_PERIP_USB_RX_UTMI_CLK ) ) )
						{
							break;
						}
					}
				#endif
		}
		else
		{
			// Disable register/core clock
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_CLKEN );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_USB_SYS_CLK_EN, 0 );
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_CLKEN, ui32Reg );
		}
	}
}

/*!
*******************************************************************************

 @Function              @SYS_setupADCPLLClock

 <b>Description:</b>\n
 This function sets up the ADC interface clock for the target platform.

 \param			psConfig	Pointer to configured SYS_sConfig structure.

 \return                    None

*******************************************************************************/
#if !defined (__IMG_HW_FPGA__)
IMG_VOID	SYS_ConfigureADCPLLClock( SYS_sADCPLLClockConfig *		psConfig )
{
	IMG_UINT32				ui32Reg;
	CLOCK_eSource			eClockSource = CLOCK_SOURCE_XTAL1; // Default clock source for PS1

	IMG_ASSERT ( g_bSysConfigureCalled == IMG_TRUE );

	if ( psConfig->bOverrideClockSource )
	{
		eClockSource = psConfig->eClockSource;
	}

	if ( eClockSource == CLOCK_SOURCE_PLL )
	{
		IMG_UINT8		ui8ClkR, ui8ClkOD;
		IMG_UINT16		ui16ClkF, ui16BWAdj;

		// Make sure target frequency is set
		IMG_ASSERT( psConfig->bTargetFrequency );

		// Calculate the PLL setup + divider values
		if ( psConfig->ePLLSource == CLOCK_SOURCE_XTAL1 )
		{
			SYS_getPLLSetupValues(	g_aui32XTALFrequencies_fp[ g_ui8XTALIndex ],
									psConfig->ui32TargetFreq_fp,
									&ui16ClkF, &ui8ClkR, &ui8ClkOD, &ui16BWAdj,
									&(psConfig->ui32ActualFreq_fp) );
		}
		else if ( psConfig->ePLLSource == CLOCK_SOURCE_XTAL2 )
		{
			SYS_getPLLSetupValues(	g_ui32XTAL2Freq_fp,
									psConfig->ui32TargetFreq_fp,
									&ui16ClkF, &ui8ClkR, &ui8ClkOD, &ui16BWAdj,
									&(psConfig->ui32ActualFreq_fp) );
		}
		else
		{
			IMG_ASSERT(0);
		}

		// Switch to xtal1 output
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );
		// Switch PLL source
		if ( psConfig->ePLLSource == CLOCK_SOURCE_XTAL1 )
		{
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_0_SW, 0 );
		}
		else
		{
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_0_SW, 1 );
		}
		WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );

		// Put PLL into reset & enable PLL bypass
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_ADCPLL_CTL1 );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_RESET, 1 );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_BYPASS, 1 );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_ENSAT, 1 );
		WRITE_REG( CR_TOP_BASE, CR_TOP_ADCPLL_CTL1, ui32Reg );

		// Write the CLKR value to CTL1
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLKR, ui8ClkR );
		WRITE_REG( CR_TOP_BASE, CR_TOP_ADCPLL_CTL1, ui32Reg );

		// Write the CLKF, CLKOD, feedback values to CTL0
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_ADCPLL_CTL0 );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLKF, ui16ClkF );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLKOD, ui8ClkOD );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_BWADJ, ui16BWAdj );
		WRITE_REG( CR_TOP_BASE, CR_TOP_ADCPLL_CTL0, ui32Reg );

		// Take PLL out of reset
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_ADCPLL_CTL1 );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_RESET, 0 );
		WRITE_REG( CR_TOP_BASE, CR_TOP_ADCPLL_CTL1, ui32Reg );

		// Remove PLL bypass
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_ADCPLL_CTL1 );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_BYPASS, 0 );
		WRITE_REG( CR_TOP_BASE, CR_TOP_ADCPLL_CTL1, ui32Reg );

		// Wait for 500us ?
	}
	else
	{
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );
		// Switch PLL source
		if ( psConfig->ePLLSource == CLOCK_SOURCE_XTAL1 )
		{
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_0_SW, 0 );
			psConfig->ui32ActualFreq_fp = g_aui32XTALFrequencies_fp[ g_ui8XTALIndex ];
		}
		else
		{
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_0_SW, 1 );
			psConfig->ui32ActualFreq_fp = g_ui32XTAL2Freq_fp;
		}

		WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );

		// Put PLL into reset & enable PLL bypass
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_ADCPLL_CTL1 );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_RESET, 1 );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_BYPASS, 1 );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_ENSAT, 1 );
		WRITE_REG( CR_TOP_BASE, CR_TOP_ADCPLL_CTL1, ui32Reg );
	}

	// Take a copy of the config structure
	g_sADCPLLClockConfig = *psConfig;
	g_bADCPLLClockConfigured = IMG_TRUE;
}
#endif

IMG_VOID SYS_ConfigureClockOut(	SYS_sClockOutConfig		asConfig[2]	)
{
	IMG_UINT32	ui32Reg;

	IMG_ASSERT ( g_bSysConfigureCalled == IMG_TRUE );

	if ( asConfig[0].bConfigure )
	{
		if ( asConfig[0].bEnable )
		{
			// Possible sources supported by this software are: XTAL1, XTAL2, sys_clk_undeleted, UCC0_IF_CLK, afe_progdiv1clk_to_soc
		#if !defined (__IMG_HW_FPGA__)
			ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );

			if ( asConfig[0].bOverrideClockSource )
			{
				// Configure clock out source
				switch ( asConfig[0].eClockSource )
				{
					case CLOCK_SOURCE_XTAL1:
					{
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_3_SW, 0 );
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_2_SW, 0 );
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_0_SW, 0 );
						break;
					}
					case CLOCK_SOURCE_XTAL2:
					{
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_3_SW, 0 );
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_2_SW, 1 );
						break;
					}
					case CLOCK_SOURCE_SYS_UNDELETED:
					{
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_3_SW, 1 );
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_1_SW, 0 );
						break;
					}
					case CLOCK_SOURCE_AFE_PROGDIV1:
					{
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_3_SW, 0 );
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_2_SW, 0 );
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_0_SW, 1 );
						break;
					}
					case CLOCK_SOURCE_UCC0_IF:
					{
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_3_SW, 1 );
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_1_SW, 1 );
						break;
					}
					default:
					{
						// Unsupported
						IMG_ASSERT(0);
					}
				}
			}
			else
			{
				/* Reset to default XTAL1 */
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_3_SW, 0 );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_2_SW, 0 );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_0_SW, 0 );
			}
			WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );

			// Configure divider
			ui32Reg = 0;
			if ( asConfig[0].bOverrideDivider )
			{
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_DIV, asConfig[0].ui8Divider );
			}
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_PAD_EN, 0 ); // Active low
			WRITE_REG( CR_TOP_BASE, CR_TOP_CLKOUT0_DIV, ui32Reg );

			// Enable clock
			ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKENAB );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_3_EN, 1 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_INV, asConfig[0].bInvertClkOutput );
			WRITE_REG( CR_TOP_BASE, CR_TOP_CLKENAB, ui32Reg );
		#endif
		}
		else
		{
			// Disable clock
			ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKENAB );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT0_3_EN, 0 );
			WRITE_REG( CR_TOP_BASE, CR_TOP_CLKENAB, ui32Reg );
		}
	}
	if ( asConfig[1].bConfigure )
	{
		if ( asConfig[1].bEnable )
		{
			// Possible sources supported by this software are: XTAL1, XTAL2, ADC_PLL_CLK, SYS_CLK_UNDELETED
		#if !defined (__IMG_HW_FPGA__)
			ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );

			// Configure clock out source
			switch ( asConfig[1].eClockSource )
			{
				case CLOCK_SOURCE_XTAL1:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_3_SW, 0 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_0_SW, 0 );
					break;
				}
				case CLOCK_SOURCE_XTAL2:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_3_SW, 0 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_0_SW, 1 );
					break;
				}
				case CLOCK_SOURCE_SYS_UNDELETED:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_3_SW, 1 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_2_SW, 1 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_1_SW, 0 );
					break;
				}
				case CLOCK_SOURCE_ADC_PLL:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_3_SW, 1 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_2_SW, 0 );
					break;
				}
				case CLOCK_SOURCE_UCC1_IF:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_3_SW, 1 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_2_SW, 1 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_1_SW, 1 );
					break;
				}
				default:
				{
					// Not supported
					IMG_ASSERT(0);
				}
			}

			WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );

			// Configure divider and pad enable
			ui32Reg = 0;
			if ( asConfig[1].bOverrideDivider )
			{
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_DIV, asConfig[1].ui8Divider );
			}
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_PAD_EN, 0 ); // Active low retarded pad-enable
			WRITE_REG( CR_TOP_BASE, CR_TOP_CLKOUT1_DIV, ui32Reg );

			// Enable clock
			ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKENAB );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_3_EN, 1 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_INV, asConfig[1].bInvertClkOutput );
			WRITE_REG( CR_TOP_BASE, CR_TOP_CLKENAB, ui32Reg );
		#endif
		}
		else
		{
			// Disable clock
			ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKENAB );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_CLKOUT1_3_EN, 0 );
			WRITE_REG( CR_TOP_BASE, CR_TOP_CLKENAB, ui32Reg );
		}
	}
}

IMG_VOID SYS_ConfigureAFE(	SYS_sAFEConfig *	psConfig	)
{
#if !defined (__IMG_HW_FPGA__)
	IMG_UINT32	ui32Reg;
#endif

	IMG_ASSERT ( g_bSysConfigureCalled == IMG_TRUE );

	#if !defined (__IMG_HW_FPGA__)
		IMG_UINT32						ui32Source_fp	= g_aui32XTALFrequencies_fp[ g_ui8XTALIndex ]; // Default source is XTAL1
		IMG_UINT32						ui32AFEPLL_fp	= 0;
		IMG_UINT32						ui32SampleRate_fp;
		IMG_BOOL						bUsedClkSwitch3 = IMG_FALSE;
		CLOCK_eSource					eClkSwitch3Source;

		if ( !psConfig->bNoReset )
		{
			// Reset GTI port
			SYS_GTIReset( CR_TOP_BASE + CR_TOP_AFEGTI_CTRL_OFFSET, IMG_TRUE );
			SYS_GTIReset( CR_TOP_BASE + CR_TOP_AFEGTI_CTRL_OFFSET, IMG_FALSE );
		}

		// Set up AFE external clock source
		if ( psConfig->bOverrideClockSource )
		{
			ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );
			// Set up clock routing
			switch ( psConfig->eClockSource )
			{
				case CLOCK_SOURCE_XTAL1:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_AFE_CLK_0_SW, 0 );
					break;
				}
				case CLOCK_SOURCE_XTAL2:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_AFE_CLK_0_SW, 1 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_AFE_CLK_1_SW, 1 );

					ui32Source_fp = g_ui32XTAL2Freq_fp;
					break;
				}
				case CLOCK_SOURCE_SYS_UNDELETED:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_AFE_CLK_0_SW, 1 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_AFE_CLK_1_SW, 0 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_3_SW, 0 );

					ui32Source_fp = g_ui32SysUndeletedFreq_fp;
					break;
				}
				case CLOCK_SOURCE_ADC_PLL:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_AFE_CLK_0_SW, 1 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_AFE_CLK_1_SW, 0 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_3_SW, 1 );

					// Make sure BLOCK_ADCPLLCLOCK has been configured
					IMG_ASSERT( g_bADCPLLClockConfigured == IMG_TRUE );

					ui32Source_fp = g_sADCPLLClockConfig.ui32ActualFreq_fp;
					break;
				}
				default:
				{
					// Invalid clock source
					IMG_ASSERT(0);
				}
			}
			WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );

			// Enable clksw3 if needed
			if ( ( psConfig->eClockSource == CLOCK_SOURCE_SYS_UNDELETED ) ||
					( psConfig->eClockSource == CLOCK_SOURCE_ADC_PLL ) )
			{
				ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKENAB );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_3_EN, 1 );
				WRITE_REG( CR_TOP_BASE, CR_TOP_CLKENAB, ui32Reg );

				// Save the clkswitch3 settings to check the IQADC ext input doesn't clash
				bUsedClkSwitch3 = IMG_TRUE;
			}
			else
			{
				bUsedClkSwitch3 = IMG_FALSE;
			}
			eClkSwitch3Source = psConfig->eClockSource;
		}
		else
		{
			// Reset interface clock source - default is XTAL1
			ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_AFE_CLK_0_SW, 0 );
			WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );

			bUsedClkSwitch3 = IMG_FALSE;
			eClkSwitch3Source = CLOCK_SOURCE_XTAL1;
		}

		if ( psConfig->bOverrideDivider )
		{
			WRITE_REG( CR_TOP_BASE, CR_TOP_AFE_DIV, psConfig->ui8Divider );
			// Update the source_fp
			ui32Source_fp /= (psConfig->ui8Divider + 1);
		}

		ui32Reg = READ_REG( CR_TOP_BASE, CR_AFE_CTRL );
		// Configure the PLL
		if ( psConfig->bEnablePLL )
		{
			IMG_UINT32	ui32PLLMode = 0;
			IMG_UINT32	ui32Reg2;

			if ( psConfig->bBypassPLL )
			{
				ui32AFEPLL_fp = ui32Source_fp;
			}
			else
			{
				// Set the PLL Mode, VCO output frequency, and check the input is valid if not bypassing
				switch ( ui32Source_fp )
				{
					case (13 << FP_SCALE):
					{
						ui32PLLMode = 0;
						ui32AFEPLL_fp = (2080 << FP_SCALE);
						break;
					}
					case (0x1333333): // 19.2
					{
						ui32PLLMode = 1;
						ui32AFEPLL_fp = (1920 << FP_SCALE);
						break;
					}
					case (20 << FP_SCALE):
					{
						ui32PLLMode = 2;
						ui32AFEPLL_fp = (1920 << FP_SCALE);
						break;
					}
					case (24 << FP_SCALE):
					{
						ui32PLLMode = 3;
						ui32AFEPLL_fp = (1920 << FP_SCALE);
						break;
					}
					case (0x0189374B): // 24.576
					{
						ui32PLLMode = 3;
						ui32AFEPLL_fp = (0x7AE147AE);   // 24.576 * 80 = 1966.08
						break;
					}
					case (26 << FP_SCALE):
					{
						ui32PLLMode = 3;
						ui32AFEPLL_fp = (2080 << FP_SCALE);
						break;
					}
					case (0x2666666): // 38.4
					{
						ui32PLLMode = 4;
						ui32AFEPLL_fp = (1920 << FP_SCALE);
						break;
					}
					case (40 << FP_SCALE):
					{
						ui32PLLMode = 5;
						ui32AFEPLL_fp = (1920 << FP_SCALE);
						break;
					}
					default:
					{
						// Invalid input frequency for the AFE PLL
						IMG_ASSERT(0);
					}
				}
			}

			// Powerdown then powerup PLL
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_PLLPD, 1 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_PLLSTBY, 1 );
			WRITE_REG( CR_TOP_BASE, CR_AFE_CTRL, ui32Reg );
			ui32Reg = READ_REG( CR_TOP_BASE, CR_AFE_CTRL );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_PLLBYPASS, psConfig->bBypassPLL );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_PLLPD, 0 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_PLLSTBY, 0 );

			if ( !psConfig->bBypassPLL )
			{
				// Set the PLL Mode
				ui32Reg2 = READ_REG( CR_TOP_BASE, CR_AFE_CLK_CTRL2 );
				ui32Reg2 = WRITE_REG_FIELD( ui32Reg2, CR_AFE_PLLMODE, ui32PLLMode );
				WRITE_REG( CR_TOP_BASE, CR_AFE_CLK_CTRL2, ui32Reg2 );
			}
		}
		else
		{
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_PLLBYPASS, 1 );
		}
		WRITE_REG( CR_TOP_BASE, CR_AFE_CTRL, ui32Reg );

		if ( psConfig->sRxADC.bEnable )
		{
			IMG_UINT32	ui32Index = 0, i;
			IMG_UINT8	ui8Reg;
			if ( psConfig->bEnablePLL )
			{
				IMG_UINT32	ui32Divider;
				// If the internal PLL is enabled, we need to calculate the divider to get to the target sample rate */
				SYS_getDividerValues(	ui32AFEPLL_fp,
										psConfig->sRxADC.ui32SampleRate_fp,
										1024,
										&ui32Divider,
										&psConfig->sRxADC.ui32ActualRate_fp );

				// Minimum divider value is 2
				if ( ui32Divider == 1 )
				{
					ui32Divider = 2;
					psConfig->sRxADC.ui32ActualRate_fp = psConfig->sRxADC.ui32SampleRate_fp / 2;

				}

				ui32Reg = READ_REG( CR_TOP_BASE, CR_AFE_CLK_CTRL2 );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_N_RXADCCLK, ui32Divider );
				WRITE_REG( CR_TOP_BASE, CR_AFE_CLK_CTRL2, ui32Reg );

				ui32SampleRate_fp = psConfig->sRxADC.ui32ActualRate_fp;
			}
			else
			{
				// Otherwise make sure that the frequency coming externally is alright.
				if ( ui32Source_fp < (2 << FP_SCALE) ||
						ui32Source_fp > (80 << FP_SCALE) )
				{
					IMG_ASSERT(0);
				}

				ui32SampleRate_fp = ui32Source_fp;
			}

			// Program GTI registers for the sample rate
			if ( ui32SampleRate_fp > 0 && ui32SampleRate_fp < (3 << FP_SCALE) ) // 0 - 2.9999 ( 2MHz in table )
			{
				ui32Index = 0;
			}
			else if ( ui32SampleRate_fp < (6 << FP_SCALE) )					// 3 - 5.9999 ( 4MHz in table )
			{
				ui32Index = 1;
			}
			else if ( ui32SampleRate_fp < (12 << FP_SCALE) )				// 6 - 11.9999 ( 8MHz in table )
			{
				ui32Index = 2;
			}
			else if ( ui32SampleRate_fp < (18 << FP_SCALE) )				// 12 - 17.9999 ( 16MHz in table )
			{
				ui32Index = 3;
			}
			else if ( ui32SampleRate_fp < (25 << FP_SCALE) )				// 18 - 24.9999 ( 20MHz in table )
			{
				ui32Index = 4;
			}
			else if ( ui32SampleRate_fp < (35 << FP_SCALE) )				// 25 - 34.9999 ( 30 MHz in table )
			{
				ui32Index = 5;
			}
			else if ( ui32SampleRate_fp <= (40 << FP_SCALE) )				// 35 - 40 ( 40 MHz in table )
			{
				ui32Index = 6;
			}
			else if ( ui32SampleRate_fp <= (80 << FP_SCALE) )				// 40.0001 - 80 ( 80 MHz in table )
			{
				ui32Index = 7;
			}
			else
			{
				// Invalid sample rate
				IMG_ASSERT(0);
			}

			// Program the GTI regs
			for ( i = 0; i < 6; ++i )
			{
				ui8Reg = SYS_GTIRead( CR_TOP_BASE + CR_TOP_AFEGTI_CTRL_OFFSET, aaui8RXADCReg[0][i] );
				ui8Reg &= ~aaui8RXADCReg[1][i];
				ui8Reg |= aaui8RXADCRegValues[ ui32Index ][ i ] << aaui8RXADCReg[2][i];
				SYS_GTIWrite( CR_TOP_BASE + CR_TOP_AFEGTI_CTRL_OFFSET, aaui8RXADCReg[0][i], ui8Reg );
			}

			// Enable Rx ADC
			ui32Reg = READ_REG( CR_TOP_BASE, CR_AFE_CTRL );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_RXADCCLK_EN, 1 );
			if ( ui32SampleRate_fp <= (40 << FP_SCALE) )
			{
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_RXEN_40M, 1 );
			}
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_RXEN_2V, psConfig->sRxADC.eInputRange ); // Configure input range
			WRITE_REG( CR_TOP_BASE, CR_AFE_CTRL, ui32Reg );

			ui32Reg = READ_REG( CR_TOP_BASE, CR_AFE_CTRL2 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_RXPWDN, 0 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_RXISTNDBY, 0 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_RXQSTNDBY, 0 );
			WRITE_REG( CR_TOP_BASE, CR_AFE_CTRL2, ui32Reg );
		}

		if ( psConfig->sTxDAC.bEnable )
		{
			// If WIFI is enabled, then reduce IQDAC common mode to within spec
			if ( g_bWIFIEnabled )
			{
				IMG_UINT32 ui32GTIReg = SYS_GTIRead( CR_TOP_BASE + CR_TOP_AFEGTI_CTRL_OFFSET, 0x1C );
				ui32GTIReg |= (1 << 6);
				SYS_GTIWrite( CR_TOP_BASE + CR_TOP_AFEGTI_CTRL_OFFSET, 0x1C, ui32GTIReg );
			}

			if ( psConfig->bEnablePLL )
			{
				IMG_UINT32	ui32Divider;
				// If the internal PLL is enabled, we need to calculate the divider to get to the target sample rate */
				SYS_getDividerValues(	ui32AFEPLL_fp,
										psConfig->sTxDAC.ui32SampleRate_fp,
										1024,
										&ui32Divider,
										&psConfig->sTxDAC.ui32ActualRate_fp	);

				// Minimum divider value is 2
				if ( ui32Divider == 1 )
				{
					ui32Divider = 2;
					psConfig->sTxDAC.ui32ActualRate_fp = psConfig->sTxDAC.ui32SampleRate_fp / 2;

				}

				ui32Reg = READ_REG( CR_TOP_BASE, CR_AFE_CLK_CTRL2 );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_N_TXDACCLK, ui32Divider );
				WRITE_REG( CR_TOP_BASE, CR_AFE_CLK_CTRL2, ui32Reg );
			}
			else
			{
				// Otherwise make sure that the frequency coming externally is alright.
				if ( ui32Source_fp > (160 << FP_SCALE) )
				{
					IMG_ASSERT(0);
				}
			}

			// Enable Tx DAC and set 1.2v common mode if needed
			ui32Reg = READ_REG( CR_TOP_BASE, CR_AFE_CTRL );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_TXDACCLK_EN, 1 );
			if ( psConfig->sTxDAC.bCommonMode1v2 )
			{
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_TXCMPROG, 1 );
			}
			WRITE_REG( CR_TOP_BASE, CR_AFE_CTRL, ui32Reg );

			ui32Reg = READ_REG( CR_TOP_BASE, CR_AFE_CTRL2 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_TXISTBY, 0 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_TXQSTBY, 0 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_TXPD, 0 );
			WRITE_REG( CR_TOP_BASE, CR_AFE_CTRL2, ui32Reg );
		}

		if ( psConfig->sAux.bEnableADC || psConfig->sAux.bEnableDAC )
		{
			// Note: Source->AUXADCFCLK becomes CLK_240M, which is then divided by 12 internally to feed the ADC
			if ( psConfig->bEnablePLL )
			{
				IMG_UINT32	ui32Divider;
				// If the internal PLL is enabled, we need to calculate the divider to get to the target sample rate */
				SYS_getDividerValues(	ui32AFEPLL_fp,
										psConfig->sAux.ui32SampleRate_fp * 12,	// We want samplerate * 12 as it will get divided later by 12.
										1024,
										&ui32Divider,
										&psConfig->sAux.ui32ActualRate_fp );

				// Minimum divider value is 2
				if ( ui32Divider == 1 )
				{
					ui32Divider = 2;
					psConfig->sAux.ui32ActualRate_fp = psConfig->sAux.ui32SampleRate_fp / 2;

				}

				psConfig->sAux.ui32ActualRate_fp /= 12; // Readjust to real sample rate

				ui32Reg = READ_REG( CR_TOP_BASE, CR_AFE_CLK_CTRL0 );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_N_AUXADCCLK, ui32Divider );
				WRITE_REG( CR_TOP_BASE, CR_AFE_CLK_CTRL0, ui32Reg );
			}
			else
			{
				// Otherwise make sure that the frequency coming externally is alright.
				if ( ui32Source_fp > ((12 * 20) << FP_SCALE) )
				{
					IMG_ASSERT(0);
				}
			}

			if ( psConfig->sAux.bEnableADC )
			{
				// Enable Aux ADC
				ui32Reg = READ_REG( CR_TOP_BASE, CR_AFE_CTRL );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_AUXADCCLK_EN, 1 );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_AUXADCSTBY, 0 );
				WRITE_REG( CR_TOP_BASE, CR_AFE_CTRL, ui32Reg );
			}

			if ( psConfig->sAux.bEnableDAC )
			{
				// Enable AUX DAC
				ui32Reg = READ_REG( CR_TOP_BASE, CR_AFE_CTRL );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_AUXDACPD, 0 );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_AUXDACSTBY, 0 );
				WRITE_REG( CR_TOP_BASE, CR_AFE_CTRL, ui32Reg );

				// Configure DAC data source
				ui32Reg = READ_REG( CR_TOP_BASE, CR_AFE_AUXDAC );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_AUXDACSEL, psConfig->sAux.eDACDataSource );
				WRITE_REG( CR_TOP_BASE, CR_AFE_AUXDAC, ui32Reg );
			}
		}

		if ( psConfig->sIQADC.bEnable )
		{
			IMG_UINT32	ui32Index = 0, i;
			IMG_UINT8	ui8Reg;

			if ( psConfig->sIQADC.bUseInternalPLL )
			{
				IMG_UINT32	ui32Divider;
				// Check the internal PLL has been enabled
				if ( !psConfig->bEnablePLL )
				{
					// AFE internal PLL not enabled
					IMG_ASSERT(0);
				}

				// Calculate the correct divider for prog2div
				SYS_getDividerValues(	ui32AFEPLL_fp,
										psConfig->sIQADC.ui32SampleRate_fp,
										1023,
										&ui32Divider,
										&psConfig->sIQADC.ui32ActualRate_fp );

				// Minimum divider value is 2
				if ( ui32Divider == 1 )
				{
					ui32Divider = 2;
					psConfig->sIQADC.ui32ActualRate_fp = psConfig->sIQADC.ui32SampleRate_fp / 2;

				}

				ui32Reg = READ_REG( CR_TOP_BASE, CR_AFE_CLK_CTRL1 );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_N_PROGDIV2CLK, ui32Divider );
				WRITE_REG( CR_TOP_BASE, CR_AFE_CLK_CTRL1, ui32Reg );

				// Enable progdiv2clk
				ui32Reg = READ_REG( CR_TOP_BASE, CR_AFE_CTRL );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_PROGDIV2CLK_EN, 1 );
				WRITE_REG( CR_TOP_BASE, CR_AFE_CTRL, ui32Reg );

				// Switch the IQADC to use AFE PLL
				ui32Reg = READ_REG( CR_TOP_BASE, CR_IQADC_CTRL );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_ADC_CLKSRCSLCT, 1 );
				WRITE_REG( CR_TOP_BASE, CR_IQADC_CTRL, ui32Reg );

				ui32SampleRate_fp = psConfig->sIQADC.ui32ActualRate_fp;
			}
			else
			{
				// IQADC will use the external source clock

				ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );
				switch ( psConfig->sIQADC.sExtClock.eClockSource )
				{
					case CLOCK_SOURCE_XTAL1:
					{
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_2_SW, 1 );
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_1_SW, 0 );

						ui32Source_fp = g_aui32XTALFrequencies_fp[ g_ui8XTALIndex ];
						break;
					}
					case CLOCK_SOURCE_XTAL2:
					{
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_2_SW, 1 );
						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_1_SW, 1 );

						ui32Source_fp = g_ui32XTAL2Freq_fp;
						break;
					}
					case CLOCK_SOURCE_SYS_UNDELETED:
					{
						// Make sure clksw3 isn't in use or, if it is in use, then it must have been set to the correct
						// path already
						if ( bUsedClkSwitch3 &&
								eClkSwitch3Source != CLOCK_SOURCE_SYS_UNDELETED )
						{
							// We have a clash
							IMG_ASSERT( 0 );
						}

						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_2_SW, 0 );

						if ( !bUsedClkSwitch3 )
						{
							ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_3_SW, 0 );
						}

						ui32Source_fp = g_ui32SysUndeletedFreq_fp;
						break;
					}
					case CLOCK_SOURCE_ADC_PLL:
					{
						// Make sure clksw3 isn't in use or, if it is in use, then it must have been set to the correct
						// path already
						if ( bUsedClkSwitch3 &&
								eClkSwitch3Source != CLOCK_SOURCE_ADC_PLL )
						{
							// We have a clash
							IMG_ASSERT( 0 );
						}

						// Make sure the ADCDIVCLOCK is set up
						IMG_ASSERT( g_bADCPLLClockConfigured );

						ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_2_SW, 0 );

						if ( !bUsedClkSwitch3 )
						{
							ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_3_SW, 1 );
						}

						ui32Source_fp = g_sADCPLLClockConfig.ui32ActualFreq_fp;
						break;
					}
					default:
					{
						IMG_ASSERT(0);
					}
				}

				WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );

				// Enable clksw3 if needed
				if ( !bUsedClkSwitch3 &&
					( ( psConfig->sIQADC.sExtClock.eClockSource == CLOCK_SOURCE_SYS_UNDELETED ) ||
						( psConfig->sIQADC.sExtClock.eClockSource == CLOCK_SOURCE_ADC_PLL ) )
					)
				{
					ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKENAB );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_ADCPLL_CLK_3_EN, 1 );
					WRITE_REG( CR_TOP_BASE, CR_TOP_CLKENAB, ui32Reg );
				}

				if ( psConfig->sIQADC.sExtClock.bOverrideDivider )
				{
					// Set the divider
					WRITE_REG( CR_TOP_BASE, CR_TOP_ADC_PLLDIV, psConfig->sIQADC.sExtClock.ui8Divider );

					ui32Source_fp /= (psConfig->sIQADC.sExtClock.ui8Divider + 1);
				}
				else if ( psConfig->sIQADC.sExtClock.bTargetFreq )
				{
					IMG_UINT32	ui32Divider;
					IMG_UINT32	ui32ActualFreq_fp;

					// Work out the right divider
					SYS_getDividerValues(	ui32Source_fp,
											psConfig->sIQADC.sExtClock.ui32TargetFreq_fp,
											256,
											&ui32Divider,
											&ui32ActualFreq_fp );

					// Set the divider
					WRITE_REG( CR_TOP_BASE, CR_TOP_ADC_PLLDIV, (ui32Divider - 1) );

					ui32Source_fp = ui32ActualFreq_fp;
				}

				// Check the frequency coming in is within spec
				if ( ui32Source_fp < (2 << FP_SCALE) ||
						ui32Source_fp > (80 << FP_SCALE) )
				{
					IMG_ASSERT(0);
				}

				// Switch the IQADC to use external clock
				ui32Reg = READ_REG( CR_TOP_BASE, CR_IQADC_CTRL );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_ADC_CLKSRCSLCT, 0 );
				WRITE_REG( CR_TOP_BASE, CR_IQADC_CTRL, ui32Reg );

				// Enable progdiv2clk
				ui32Reg = READ_REG( CR_TOP_BASE, CR_AFE_CTRL );
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_AFE_PROGDIV2CLK_EN, 1 );
				WRITE_REG( CR_TOP_BASE, CR_AFE_CTRL, ui32Reg );

				ui32SampleRate_fp = ui32Source_fp;
			}

			// Program GTI registers for the sample rate
			if ( ui32SampleRate_fp > 0 && ui32SampleRate_fp < (3 << FP_SCALE) ) // 0 - 2.9999 ( 2MHz in table )
			{
				ui32Index = 0;
			}
			else if ( ui32SampleRate_fp < (6 << FP_SCALE) )					// 3 - 5.9999 ( 4MHz in table )
			{
				ui32Index = 1;
			}
			else if ( ui32SampleRate_fp < (12 << FP_SCALE) )				// 6 - 11.9999 ( 8MHz in table )
			{
				ui32Index = 2;
			}
			else if ( ui32SampleRate_fp < (18 << FP_SCALE) )				// 12 - 17.9999 ( 16MHz in table )
			{
				ui32Index = 3;
			}
			else if ( ui32SampleRate_fp < (25 << FP_SCALE) )				// 18 - 24.9999 ( 20MHz in table )
			{
				ui32Index = 4;
			}
			else if ( ui32SampleRate_fp < (35 << FP_SCALE) )				// 25 - 34.9999 ( 30 MHz in table )
			{
				ui32Index = 5;
			}
			else if ( ui32SampleRate_fp <= (40 << FP_SCALE) )				// 35 - 40 ( 40 MHz in table )
			{
				ui32Index = 6;
			}
			else if ( ui32SampleRate_fp <= (80 << FP_SCALE) )				// 40.0001 - 80 ( 80 MHz in table )
			{
				ui32Index = 7;
			}
			else
			{
				// Invalid sample rate
				IMG_ASSERT(0);
			}

			for ( i = 0; i < 6; ++i )
			{
				ui8Reg = SYS_GTIRead( CR_TOP_BASE + CR_TOP_AFEGTI_CTRL_OFFSET, aaui8IQADCReg[0][i] );
				ui8Reg &= ~aaui8IQADCReg[1][i];
				ui8Reg |= aaui8IQADCRegValues[ ui32Index ][ i ] << aaui8IQADCReg[2][i];
				SYS_GTIWrite( CR_TOP_BASE + CR_TOP_AFEGTI_CTRL_OFFSET, aaui8IQADCReg[0][i], ui8Reg );
			}

			// Enable the IQADC
			ui32Reg = READ_REG( CR_TOP_BASE, CR_IQADC_CTRL );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_ADC_RXIPWDN, 0 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_ADC_RXQPWDN, 0 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_ADC_RXISTNDBY, 0 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_ADC_RXQSTNDBY, 0 );
			ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_ADC_EN_2V, psConfig->sIQADC.eInputRange ); // Configure input range
			if ( ui32SampleRate_fp <= (40 << FP_SCALE) )
			{
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_ADC_EN_40M, 1 );
			}
			else
			{
				ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_ADC_EN_40M, 0 );
			}

			WRITE_REG( CR_TOP_BASE, CR_IQADC_CTRL, ui32Reg );
		}
	#endif

}

IMG_VOID SYS_ConfigurePDM(	SYS_sPDMConfig *	psConfig	)
{
#if !defined (__IMG_HW_FPGA__)
	IMG_UINT32	ui32Reg;

	IMG_ASSERT ( g_bSysConfigureCalled == IMG_TRUE );

	// Set up PDM clock
	if ( psConfig->bTargetFrequency )
	{
		IMG_UINT32	ui32Divider;
		SYS_getDividerValues(	g_aui32XTALFrequencies_fp[ g_ui8XTALIndex ],
								psConfig->ui32TargetFreq_fp,
								8,
								&ui32Divider,
								&(psConfig->ui32ActualFreq_fp) );
		WRITE_REG( CR_TOP_BASE, CR_TOP_PDMCK_CTL, (ui32Divider - 1) );
	}
	else
	{
		// Reset clock divider
		WRITE_REG( CR_TOP_BASE, CR_TOP_PDMCK_CTL, 0 );
	}

	// PDM A
	if ( psConfig->asBlockConfig[0].bConfigure )
	{
		if ( psConfig->asBlockConfig[0].bEnable )
		{
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_PDM_CONTROL );
			switch ( psConfig->asBlockConfig[0].ePadSource )
			{
				case PDM_PAD_SOURCE_PDM:
				{
					IMG_UINT32	ui32Reg2 = READ_REG( CR_PERIP_BASE, CR_PERIP_PDM_SELECT );
					ui32Reg2 = WRITE_REG_FIELD( ui32Reg2, CR_PERIP_PDM_A_SEL, psConfig->asBlockConfig[0].eControlSource );
					WRITE_REG( CR_PERIP_BASE, CR_PERIP_PDM_SELECT, ui32Reg2 );

					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP1_A, 0 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP0_A, 0 );
					ui32Reg |= (1 << 0);
					break;
				}
				case PDM_PAD_SOURCE_SYMBOL_UCC0:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP1_A, 0 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP0_A, 1 );
					ui32Reg &= ~(1 << 0);
					break;
				}
				case PDM_PAD_SOURCE_SYMBOL_UCC1:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP1_A, 1 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP0_A, 0 );
					ui32Reg &= ~(1 << 0);
					break;
				}
				default:
				{
					// Invalid pad source
					IMG_ASSERT(0);
				}
			}
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_PDM_CONTROL, ui32Reg );

			// Disable GPIO for PDM A
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg &= ~(1 << 24);
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		}
		else
		{
		#if !defined (__IMG_HW_FPGA__)
			// Enable GPIO for PDM A
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg |= (1 << 24);
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
	}

	// PDM B
	if ( psConfig->asBlockConfig[1].bConfigure )
	{
		if ( psConfig->asBlockConfig[1].bEnable )
		{
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_PDM_CONTROL );
			switch ( psConfig->asBlockConfig[1].ePadSource )
			{
				case PDM_PAD_SOURCE_PDM:
				{
					IMG_UINT32	ui32Reg2 = READ_REG( CR_PERIP_BASE, CR_PERIP_PDM_SELECT );
					ui32Reg2 = WRITE_REG_FIELD( ui32Reg2, CR_PERIP_PDM_B_SEL, psConfig->asBlockConfig[1].eControlSource );
					WRITE_REG( CR_PERIP_BASE, CR_PERIP_PDM_SELECT, ui32Reg2 );

					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP1_B, 0 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP0_B, 0 );
					ui32Reg |= (1 << 1);
					break;
				}
				case PDM_PAD_SOURCE_SYMBOL_UCC0:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP1_B, 0 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP0_B, 1 );
					ui32Reg &= ~(1 << 1);
					break;
				}
				case PDM_PAD_SOURCE_SYMBOL_UCC1:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP1_B, 1 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP0_B, 0 );
					ui32Reg &= ~(1 << 1);
					break;
				}
				default:
				{
					// Invalid pad source
					IMG_ASSERT(0);
				}
			}
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_PDM_CONTROL, ui32Reg );

			// Disable GPIO for PDM B
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg &= ~(1 << 25);
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		}
		else
		{
		#if !defined (__IMG_HW_FPGA__)
			// Enable GPIO for PDM A
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0 );
			ui32Reg |= (1 << 25);
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT0, ui32Reg );
		#endif
		}
	}

	// PDM C
	if ( psConfig->asBlockConfig[2].bConfigure )
	{
		if ( psConfig->asBlockConfig[2].bEnable )
		{
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_PDM_CONTROL );
			switch ( psConfig->asBlockConfig[2].ePadSource )
			{
				case PDM_PAD_SOURCE_PDM:
				{
					IMG_UINT32	ui32Reg2 = READ_REG( CR_PERIP_BASE, CR_PERIP_PDM_SELECT );
					ui32Reg2 = WRITE_REG_FIELD( ui32Reg2, CR_PERIP_PDM_C_SEL, psConfig->asBlockConfig[2].eGainSource );
					WRITE_REG( CR_PERIP_BASE, CR_PERIP_PDM_SELECT, ui32Reg2 );

					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP1_C, 0 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP0_C, 0 );
					ui32Reg |= (1 << 2);
					break;
				}
				case PDM_PAD_SOURCE_SYMBOL_UCC0:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP1_C, 0 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP0_C, 1 );
					ui32Reg &= ~(1 << 2);
					break;
				}
				case PDM_PAD_SOURCE_SYMBOL_UCC1:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP1_C, 1 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP0_C, 0 );
					ui32Reg &= ~(1 << 2);
					break;
				}
				default:
				{
					// Invalid pad source
					IMG_ASSERT(0);
				}
			}
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_PDM_CONTROL, ui32Reg );

			// Disable GPIO for PDM C
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT1 );
			ui32Reg &= ~(1 << 0);
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT1, ui32Reg );
		}
		else
		{
		#if !defined (__IMG_HW_FPGA__)
			// Enable GPIO for PDM A
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT1 );
			ui32Reg |= (1 << 0);
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT1, ui32Reg );
		#endif
		}
	}

	// PDM D
	if ( psConfig->asBlockConfig[3].bConfigure )
	{
		if ( psConfig->asBlockConfig[3].bEnable )
		{
			ui32Reg = READ_REG( CR_PERIP_BASE, CR_PERIP_PDM_CONTROL );
			switch ( psConfig->asBlockConfig[3].ePadSource )
			{
				case PDM_PAD_SOURCE_PDM:
				{
					IMG_UINT32	ui32Reg2 = READ_REG( CR_PERIP_BASE, CR_PERIP_PDM_SELECT );
					ui32Reg2 = WRITE_REG_FIELD( ui32Reg2, CR_PERIP_PDM_D_SEL, psConfig->asBlockConfig[3].eGainSource );
					WRITE_REG( CR_PERIP_BASE, CR_PERIP_PDM_SELECT, ui32Reg2 );

					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP1_D, 0 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP0_D, 0 );
					ui32Reg |= (1 << 3);
					break;
				}
				case PDM_PAD_SOURCE_SYMBOL_UCC0:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP1_D, 0 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP0_D, 1 );
					ui32Reg &= ~(1 << 3);
					break;
				}
				case PDM_PAD_SOURCE_SYMBOL_UCC1:
				{
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP1_D, 1 );
					ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_PERIP_PDM_SYM_UP0_D, 0 );
					ui32Reg &= ~(1 << 3);
					break;
				}
				default:
				{
					// Invalid pad source
					IMG_ASSERT(0);
				}
			}
			WRITE_REG( CR_PERIP_BASE, CR_PERIP_PDM_CONTROL, ui32Reg );

			// Disable GPIO for PDM D
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT1 );
			ui32Reg &= ~(1 << 1);
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT1, ui32Reg );
		}
		else
		{
		#if !defined (__IMG_HW_FPGA__)
			// Enable GPIO for PDM A
			ui32Reg = READ_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT1 );
			ui32Reg |= (1 << 1);
			WRITE_REG( CR_TOP_BASE, CR_PADS_GPIO_SELECT1, ui32Reg );
		#endif
		}
	}
#endif
}



IMG_VOID SYS_ConfigureDISEQC(SYS_sDISEQCConfig *psConfig)
{
	IMG_UINT32 ui32Reg;

	IMG_ASSERT(g_bSysConfigureCalled == IMG_TRUE);

	WRITE_REG(CR_PERIP_BASE,CR_TOP_DISEQCCLK_DIV,0);

	if(psConfig->asBlockConfig[0].bConfigure)
	{
		if(psConfig->asBlockConfig[0].bEnable)
		{
			ui32Reg = READ_REG(CR_PERIP_BASE,CR_PERIP_CLKEN);
			ui32Reg |= (1<<10);
			WRITE_REG(CR_PERIP_BASE,CR_PERIP_CLKEN,ui32Reg);

			//ui32Reg = READ_REG(CR_PERIP_BASE,CR_PERIP_SRST);
			//ui32Reg |= (1<<19);
			//WRITE_REG(CR_PERIP_BASE,CR_PERIP_SRST,ui32Reg);

			//ui32Reg = READ_REG(CR_PERIP_BASE,CR_PERIP_SRST);
			//ui32Reg &= ~(1<<19);
			//WRITE_REG(CR_PERIP_BASE,CR_PERIP_SRST,ui32Reg);
		}
		else
		{
			ui32Reg = READ_REG(CR_PERIP_BASE,CR_PERIP_CLKEN);
			ui32Reg &= ~(1<<10);
			WRITE_REG(CR_PERIP_BASE,CR_PERIP_CLKEN,ui32Reg);
		}
	}

	if(psConfig->asBlockConfig[1].bConfigure)
	{
		if(psConfig->asBlockConfig[1].bEnable)
		{
			ui32Reg = READ_REG(CR_PERIP_BASE,CR_PERIP_CLKEN);
			ui32Reg |= (1<<11);
			WRITE_REG(CR_PERIP_BASE,CR_PERIP_CLKEN,ui32Reg);

			//ui32Reg = READ_REG(CR_PERIP_BASE,CR_PERIP_SRST);
			//ui32Reg |= (1<<20);
			//WRITE_REG(CR_PERIP_BASE,CR_PERIP_SRST,ui32Reg);

			//ui32Reg = READ_REG(CR_PERIP_BASE,CR_PERIP_SRST);
			//ui32Reg &= ~(1<<20);
			//WRITE_REG(CR_PERIP_BASE,CR_PERIP_SRST,ui32Reg);
		}
		else
		{
			ui32Reg = READ_REG(CR_PERIP_BASE,CR_PERIP_CLKEN);
			ui32Reg &= ~(1<<11);
			WRITE_REG(CR_PERIP_BASE,CR_PERIP_CLKEN,ui32Reg);
		}
	}
}

/*!
******************************************************************************

 @Function	 calculate_actual_frequency_12_20

******************************************************************************/
IMG_UINT32 calculate_actual_frequency_12_20(IMG_UINT32 ui32CLKF, IMG_UINT32 ui32OD, IMG_UINT32 ui32CLKR, IMG_UINT32 ui32Xtal_fp)
{
	IMG_UINT32 ui32ResultFreq;
	IMG_UINT64 ui64InterimValue;

	// The equation for the PLL output frequency is: Fout = ( Fref / (clkR + 1) ) * ( ( clkf / 2 + 0.5 ) / (clkOD + 1) ) / PostDiv

	// Calculate the actual frequency - get the multiplications done first to keep accuracy.
	ui64InterimValue  = (IMG_UINT64)ui32Xtal_fp * (ui32CLKF + 1); // Instead of CLKF/2 + 0.5 we are using CLKF+1, so need to divide by 2 later
	ui64InterimValue /= (ui32CLKR+1);
	ui64InterimValue /= (ui32OD + 1);
	ui32ResultFreq    = (IMG_UINT32)(ui64InterimValue >> 1); // Now dividing by 2 as per the above comment.

	return ui32ResultFreq;
}

/*!
*******************************************************************************

 @Function              @SYS_getPLLSetupValues

 <b>Description:</b>\n
 This function iterativly calculates the values for CLKR, CLKF and CLKOD which produce the frequency
 closest to the desired input frequency. The actual frequency specified by the calculated values is returned.

 \param			ui32TargetFreq_fp		The desired frequency, in MHz, Q16.16 format

 \param			*pui8DivF				Address where calculated PLL setup value, DivF, will be stored

 \param			*pui8DivQ				Address where calculated PLL setup value, DivQ, will be stored

 \param			*pui32ActualFreq_fp		Address where actual frequency will be stored, in MHz, Q16.16 format

 \return								None

*******************************************************************************/
#if !defined (__IMG_HW_FPGA__)
IMG_VOID SYS_getPLLSetupValues( IMG_UINT32		ui32SourceFreq_fp,
								IMG_UINT32		ui32TargetFreq_fp,
								IMG_UINT16	*	pui16CLKF,
								IMG_UINT8	*	pui8CLKR,
								IMG_UINT8	*	pui8CLKOD,
								IMG_UINT16	*	pui16BWAdj,
								IMG_UINT32	*	pui32ActualFreq_fp )
{
#define EPSILON	(0xCCCC)
#define CLKF_SHIFT 4
#define CLKF_MASK  0x0001FFF0
#define CLKR_MASK  0x0000003F
#define CLKOD_MASK 0x00000007

	IMG_UINT64	ui64CLKF_63_1;
	IMG_UINT32	ui32CLKF;
	IMG_UINT32	ui32frequencyProduced 	= 0;
	IMG_BOOL	bOutOfBounds 			= IMG_FALSE;
	IMG_UINT8	ui8ODMax 				= 8;
	IMG_UINT8	r, od;

	// Check if we're going to be setting up the PLL out of bounds.
	if( ui32TargetFreq_fp > (600 << FP_SCALE ) )
	{
		bOutOfBounds = IMG_TRUE;
		ui8ODMax = 0;
	}
	else
	{
		// Not going out of bounds, but limit searches to values of OD that don't obviously put the intermediate freq above the 600MHz Max.
		ui8ODMax = ((600 << FP_SCALE )/ui32TargetFreq_fp) - 1;
		if ( ui8ODMax > 8 )
		{
			ui8ODMax = 8;
		}
	}

	//Loop through possible values of OD
	for ( od = ui8ODMax; od < 0xFF ; od--)  // To minimise jitter we need to have OD as large as possible
	{
		// Loop through possible values of clkr
		for ( r = 0; r < 64; r++ )
		{
			// Calculate clkF in 63/1 so we can round up if necessary
			//                  clkF = (( 2 * (OD+1) * PLL 				 * (CR+1) ) / XT) - 1
			ui64CLKF_63_1 = (IMG_UINT64)  2 * (od+1) * ui32TargetFreq_fp * (r+1) * 2; // Doing all multiplications first (*2 to shift into 63/1)
			ui64CLKF_63_1 /= ui32SourceFreq_fp;										  // we can divide by this directly now rather than using XT/2 as we are in 63_1 notation
			ui64CLKF_63_1 -= 2;														  // -2 rather than -1 as we are working in 63/1 format

			// round up if the fractional bit is set - we want the closest value of CLKF possible.
			ui32CLKF = (ui64CLKF_63_1 & 1) ? ((IMG_UINT32)(ui64CLKF_63_1 >> 1)) + 1 :
			                                  (IMG_UINT32)(ui64CLKF_63_1 >> 1);

			// Verify produced frequency is within precision required
			ui32frequencyProduced = calculate_actual_frequency_12_20(ui32CLKF, od, r, ui32SourceFreq_fp);

			if( abs( ui32frequencyProduced - ui32TargetFreq_fp) < EPSILON)
			{
				// Potentially found a match - check no constraints violated.
				if((ui32CLKF << 4) & ~CLKF_MASK)
				{
					continue; // CLKF value too large
				}

				// Verify intermediate frequencies are acceptable (But only if we are drving the PLL within bounds)
				if ( !bOutOfBounds )
				{
					if((ui32frequencyProduced*(od+1)) > (600 << FP_SCALE) ||
					   (ui32frequencyProduced*(od+1)) < (120 << FP_SCALE))
					{
						continue; // Internal frequencies are out of range
					}
				}

				// Good Settings found
				*pui16CLKF			= (IMG_UINT16)ui32CLKF;
				*pui8CLKR			= (IMG_UINT8)r;
				*pui8CLKOD			= (IMG_UINT8)od;
				*pui32ActualFreq_fp	= ui32frequencyProduced;

				if(ui32CLKF)
				{
					*pui16BWAdj = (IMG_UINT16)(((2 * ui32CLKF) - 1) >> 2);
				}
				else
				{
					*pui16BWAdj = 0;
				}

				return; // Early exit!
			}
		}
	}
}
#endif

IMG_VOID SYS_delay( IMG_UINT32	ui32MetaClock_fp, IMG_UINT32	ui32MicroSeconds )
{
	// Calculate frequency of timer
	IMG_UINT32	ui32TimerDiv_fp = (((*(volatile unsigned long *)0x04830140) & 0xFFF) + 1) << 16;
	IMG_UINT32	ui32TimerFreq_fp = ((IMG_UINT64)(ui32MetaClock_fp >> 4) << 16 ) / ((IMG_UINT64)ui32TimerDiv_fp); // In MHZ

	IMG_UINT32	ui32TimerStart = TBI_GETREG( TXTIMER );
	IMG_UINT32	ui32Delta_fp;
	// Do the delay
	do
	{
		ui32Delta_fp = (TBI_GETREG( TXTIMER ) - ui32TimerStart) << 16;
		ui32Delta_fp = ((IMG_UINT64)ui32Delta_fp << 16) / ((IMG_UINT64)ui32TimerFreq_fp); // in microseconds
	} while ( (IMG_UINT64)(ui32Delta_fp) < (IMG_UINT64)ui32MicroSeconds << 16 );
}

/*!
*******************************************************************************

 @Function              @SYS_setupPLL

 <b>Description:</b>\n
 This function sets up the system clock PLL for the target platform

 \param			psConfig	Pointer to configured SYS_sConfig structure.

 \return                    None

*******************************************************************************/
#if !defined (__IMG_HW_FPGA__)
IMG_VOID SYS_setupPLL(	SYS_sConfig		*	psConfig	)
{
	IMG_UINT8	ui8ClkR, ui8ClkOD, ui8SysClk = 0;
	IMG_UINT16	ui16ClkF, ui16BWAdj;
	IMG_UINT32	ui32Reg;

	if ( psConfig->sSystemClock.ePLLSource == CLOCK_SOURCE_XTAL1 )
	{
		SYS_getPLLSetupValues(	g_aui32XTALFrequencies_fp[ g_ui8XTALIndex ],
								psConfig->sSystemClock.ui32TargetFreq_fp,
								&ui16ClkF, &ui8ClkR, &ui8ClkOD, &ui16BWAdj,
								&(psConfig->ui32ActualFreq_fp)  );
	}
	else if ( psConfig->sSystemClock.ePLLSource == CLOCK_SOURCE_XTAL2 )
	{
		SYS_getPLLSetupValues(	psConfig->ui32XTAL2Freq_fp,
								psConfig->sSystemClock.ui32TargetFreq_fp,
								&ui16ClkF, &ui8ClkR, &ui8ClkOD, &ui16BWAdj,
								&(psConfig->ui32ActualFreq_fp)	);
	}
	else
	{
		IMG_ASSERT(0);
	}

	// Switch to XTAL output
	ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );
	// Set PLL source clock
	if ( psConfig->sSystemClock.ePLLSource == CLOCK_SOURCE_XTAL1 )
	{
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SYSCLK0_SW, 0 );
	}
	else
	{
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SYSCLK0_SW, 1 );
	}
	ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SYSCLK1_SW, 0 );
	WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );

	// Possibly wait 30 clock cycles here ?

	// Put PLL into reset & enable PLL bypass
	ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_SYSPLL_CTL1 );
	ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SYSPLL_RESET, 1 );
	ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SYSPLL_BYPASS, 1 );
	ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SYSPLL_ENSAT, 1 );
	WRITE_REG( CR_TOP_BASE, CR_TOP_SYSPLL_CTL1, ui32Reg );

	// Write the CLKR value to CTL1
	ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SYSPLL_CLKR, ui8ClkR );
	WRITE_REG( CR_TOP_BASE, CR_TOP_SYSPLL_CTL1, ui32Reg );

	// Write the CLKF, CLKOD, feedback values to CTL0
	ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_SYSPLL_CTL0 );
	ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SYSPLL_CLKF, ui16ClkF );
	ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SYSPLL_CLKOD, ui8ClkOD );
	ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SYSPLL_BWADJ, ui16BWAdj );
	WRITE_REG( CR_TOP_BASE, CR_TOP_SYSPLL_CTL0, ui32Reg );

	// Take PLL out of reset
	ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_SYSPLL_CTL1 );
	ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SYSPLL_RESET, 0 );
	WRITE_REG( CR_TOP_BASE, CR_TOP_SYSPLL_CTL1, ui32Reg );

	// Set sys undeleted frequency
	IMG_ASSERT( psConfig->sSystemClock.ui8SysClockDivider <= 3 );
	WRITE_REG( CR_TOP_BASE, CR_TOP_META_CLKDIV, psConfig->sSystemClock.ui8SysClockDivider );
	g_ui32SysUndeletedFreq_fp = psConfig->ui32ActualFreq_fp / (psConfig->sSystemClock.ui8SysClockDivider + 1);

	// Set Meta Clock Deletion
	WRITE_REG( CR_TOP_BASE, CR_TOP_META_CLKDELETE, psConfig->sSystemClock.ui16MetaClockDeletion );

	// Set System Clock Deletion
	WRITE_REG( CR_TOP_BASE, CR_TOP_CLKDELETE, psConfig->sSystemClock.ui16SysClockDeletion );

	// Set DDR Clock divider
	WRITE_REG( CR_TOP_BASE, CR_TOP_DDR_CLKDIV, psConfig->sSystemClock.ui8DDRClkDivider );

	// Set UCC0 Deleter
	if ( psConfig->sUCCConfig.bEnable )
	{
		WRITE_REG( CR_TOP_BASE, CR_TOP_UCC0_DELETE, psConfig->sUCCConfig.ui16ClkDeletion );
	}

	// Remove PLL bypass
	ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_SYSPLL_CTL1 );
	ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SYSPLL_BYPASS, 0 );
	WRITE_REG( CR_TOP_BASE, CR_TOP_SYSPLL_CTL1, ui32Reg );

	// Wait for 500 reference divider clock cycles (but actually wait 1ms)
	SYS_delay( (psConfig->sSystemClock.ePLLSource == CLOCK_SOURCE_XTAL1) ? g_aui32XTALFrequencies_fp[ g_ui8XTALIndex ] : psConfig->ui32XTAL2Freq_fp, 1000 );

	// Set the post-PLL divider
	WRITE_REG( CR_TOP_BASE, CR_TOP_SYSCLK_DIV, ui8SysClk );

	// Switch to PLL output
	ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );
	ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SYSCLK1_SW, 1 );
	WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );
}
#endif

/*!
*******************************************************************************

 @Function              @SYS_setupUCCClocks

 <b>Description:</b>\n
 This function sets up the UCC clocks for the target platform

 \param			psConfig	Pointer to configured SYS_sConfig structure.

 \return                    None

*******************************************************************************/
IMG_VOID SYS_setupUCCClocks( SYS_sConfig	*	psConfig	)
{
	IMG_UINT32			ui32ClkSwitch2, ui32ClkEnab2, ui32ClkSwitch, ui32ClkEnab;

	ui32ClkSwitch2 = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH2 );
	ui32ClkEnab2 = READ_REG( CR_TOP_BASE, CR_TOP_CLKENAB2 );
	if ( psConfig->sUCCConfig.bEnable )
	{
		// Set up IF clock source
		if ( psConfig->sUCCConfig.bEnableIFClock )
		{
			switch ( psConfig->sUCCConfig.eIFClockSource )
			{
				case CLOCK_SOURCE_AFE_RXSYNC:
				{
					ui32ClkSwitch2 = WRITE_REG_FIELD( ui32ClkSwitch2, CR_TOP_IF0_CLK_SW, 0 );
					break;
				}
				case CLOCK_SOURCE_EXT_ADC_DAC:
				{
					ui32ClkSwitch2 = WRITE_REG_FIELD( ui32ClkSwitch2, CR_TOP_IF0_CLK_SW, 1 );
					ui32ClkSwitch2 = WRITE_REG_FIELD( ui32ClkSwitch2, CR_TOP_IF0_1_CLK_SW, 1 );
					break;
				}
				default:
				{
					// Invalid clock source
					IMG_ASSERT(0);
				}
			}

			ui32ClkEnab2 = WRITE_REG_FIELD( ui32ClkEnab2, CR_TOP_IF0_CLK_EN, 1 );
		}

		WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH2, ui32ClkSwitch2 );

		// Set up STC clock source
		if ( psConfig->sUCCConfig.bEnableSTCClock )
		{
			ui32ClkSwitch = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );
			switch ( psConfig->sUCCConfig.eSTCClockSource )
			{
				case CLOCK_SOURCE_XTAL1:
				{
					ui32ClkSwitch = WRITE_REG_FIELD( ui32ClkSwitch, CR_TOP_EXT_STC0_CLK_SW, 0 );
					break;
				}
				case CLOCK_SOURCE_XTAL2:
				{
					ui32ClkSwitch = WRITE_REG_FIELD( ui32ClkSwitch, CR_TOP_EXT_STC0_CLK_SW, 1 );
					break;
				}
				default:
				{
					// Invalid clock source
					IMG_ASSERT(0);
				}
			}
			WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32ClkSwitch );

			// Enable clock
			ui32ClkEnab = READ_REG( CR_TOP_BASE, CR_TOP_CLKENAB );
			ui32ClkEnab = WRITE_REG_FIELD( ui32ClkEnab, CR_TOP_EXT_STC0_CLK_EN, 1 );
			WRITE_REG( CR_TOP_BASE, CR_TOP_CLKENAB, ui32ClkEnab );
		}

		if ( psConfig->sUCCConfig.bUseExternalADC )
		{
			ui32ClkEnab2 = WRITE_REG_FIELD( ui32ClkEnab2, CR_TOP_EXT_ADC_DAC_CLK_EN, 1 );
		}

		if ( psConfig->sUCCConfig.bEnableUCC0DACClk )
		{
			ui32ClkEnab2 = WRITE_REG_FIELD( ui32ClkEnab2, CR_TOP_DAC0_CLK_EN, 1 );
		}
	}


	WRITE_REG( CR_TOP_BASE, CR_TOP_CLKENAB2, ui32ClkEnab2 );
}

/*!
*******************************************************************************

 @Function              @SYS_setupSetupSystemClock

 <b>Description:</b>\n
 This function sets up the system clock PLL for the target platform

 \param			psConfig	Pointer to configured SYS_sConfig structure.

 \return                    None

*******************************************************************************/
IMG_VOID SYS_setupSystemClock( SYS_sConfig	*	psConfig	)
{
	#if !defined (__IMG_HW_FPGA__)
		IMG_UINT32		ui32Reg;
	#endif

	// Set up the system clock
	if ( psConfig->sSystemClock.eMetaClockSource == CLOCK_SOURCE_XTAL1 )
	{
		psConfig->ui32ActualFreq_fp = g_aui32XTALFrequencies_fp[ g_ui8XTALIndex ];
	#if !defined (__IMG_HW_FPGA__)
		// Switch source to XTAL1
		ui32Reg = READ_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH );
		ui32Reg = WRITE_REG_FIELD( ui32Reg, CR_TOP_SYSCLK1_SW, 0 );
		WRITE_REG( CR_TOP_BASE, CR_TOP_CLKSWITCH, ui32Reg );

		// Set sys undeleted frequency. For the PLL, this is done inside SYS_setupPLL, before the PLL is unbypassed.
		IMG_ASSERT( psConfig->sSystemClock.ui8SysClockDivider <= 3 );
		WRITE_REG( CR_TOP_BASE, CR_TOP_META_CLKDIV, psConfig->sSystemClock.ui8SysClockDivider );
		g_ui32SysUndeletedFreq_fp = psConfig->ui32ActualFreq_fp / (psConfig->sSystemClock.ui8SysClockDivider + 1);
	#endif
	}
	else if ( psConfig->sSystemClock.eMetaClockSource == CLOCK_SOURCE_PLL )
	{
		// If we're on the FPGA, ignore the PLL setup
	#if !defined (__IMG_HW_FPGA__)
		SYS_setupPLL( psConfig );
	#endif
	}
	else
	{
		// Invalid configuration
		IMG_ASSERT(0);
	}

	// Set META's TIMERCONFIG register if we are Meta!
#if defined (__META_MEOS__)
	*(volatile unsigned long *)0x04830140 = (unsigned long)( ( (psConfig->ui32ActualFreq_fp >> FP_SCALE) & 0xFFF ) - 1);
#endif
}

/*!
*******************************************************************************

 @Function              @SYS_getDividerValues

 <b>Description:</b>\n
 This function calculates a divider value which produces the closest frequency to
 the required output frequency, given the divider "size" and source clock.

 \param			ui32SourceFreq_fp		The source frequency, in MHz, Q12.20 format

 \param			ui32TargetFreq_fp		The desired frequncy, in MHz, Q12.20 format

 \param			ui32DividerRange		The range of possible clock divisions (eg for a DivBy256 divider, specify 256)

 \param			*pui32DivideBy			Address where calculated divider value will be stored

 \param			*pui32ActualFreq_fp		Address where actual frequency will be stored, in MHz, Q12.20 format

 \return								None

*******************************************************************************/
//#if !defined (__IMG_HW_FPGA__)
IMG_VOID SYS_getDividerValues(	const IMG_UINT32		ui32SourceFreq_fp,
								const IMG_UINT32		ui32TargetFreq_fp,
								const IMG_UINT32		ui32DividerRange,
									  IMG_UINT32	 *	pui32DivideBy,
									  IMG_UINT32	 *	pui32ActualFreq_fp	)
{
	IMG_UINT32 ui32DivideBy;

	ui32DivideBy = ui32SourceFreq_fp / ui32TargetFreq_fp;
	if((ui32TargetFreq_fp - (ui32SourceFreq_fp / (ui32DivideBy+1))) < ((ui32SourceFreq_fp / ui32DivideBy) - ui32TargetFreq_fp))
	{
		ui32DivideBy++;
	}

	if(ui32DivideBy>=ui32DividerRange)
	{
		IMG_ASSERT(0);
		ui32DivideBy = ui32DividerRange;
	}

	*pui32DivideBy = ui32DivideBy;
	*pui32ActualFreq_fp = ui32SourceFreq_fp / (ui32DivideBy);
}

//#endif

IMG_UINT32 SYS_getSysUndeletedFreq_fp()
{
	/* g_ui32SysUndeletedFreq_fp is set in SYS_Configure() */
	IMG_ASSERT ( g_bSysConfigureCalled == IMG_TRUE );

	return g_ui32SysUndeletedFreq_fp;
}
