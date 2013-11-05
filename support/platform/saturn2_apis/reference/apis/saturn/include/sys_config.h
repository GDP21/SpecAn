/*!
*******************************************************************************
  file   sys_config.h

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

#if !defined (__SYS_CONF_H__)
#define __SYS_CONF_H__

/*============================================================================
====	D E F I N E S
=============================================================================*/

// Commonly used frequencies can be added here, in fixed point Q12.20 notation
#define SYS_FREQ_500MHZ         (500 << 20)
#define SYS_FREQ_400MHZ         (400 << 20)     // Ideal Meta HTP clock
#define SYS_FREQ_360MHZ         (360 << 20)     // Typical Meta HTP clock
#define SYS_FREQ_200MHZ         (200 << 20)     // Ideal UCC0, UCC1, memory, sysinf, 2d/pdp/lcd clock
#define SYS_FREQ_180MHZ         (180 << 20)     // Default Comet Bringup Clock Speed
#define SYS_FREQ_160MHZ         (160 << 20)     // Default IMG clock speed 160MHz
#define SYS_FREQ_1_8432MHZ		(0x001D7DBF)		// UART CLK Min
#define SYS_FREQ_25MHZ			(0x01900000)		// UART Clk Ideal
#define RTC_FREQ				(0x00008312)		// 32kHz

#define BASE_MEMBERS																								\
	/*!	Set to true if this block should be configured.
		If true and bEnable is true, the block will be enabled.
		If true and bEnable is false, the block will be disabled. */												\
	IMG_BOOL		bConfigure;																						\
	/*! Set to true if the block should be enabled */																\
	IMG_BOOL		bEnable;																						\

#define TARGET_FRQUENCY_MEMBERS																						\
	/*! Set to true to have the API calculate the required dividers for the target frequency specified below */		\
	IMG_BOOL		bTargetFrequency;																				\
	/*! The desired interface clock frequency for the block, in MHz, Q12.20 format */								\
	IMG_UINT32		ui32TargetFreq_fp;																				\
	/*! The actual interface clock frequency, in MHz, Q12.20 format (set by the SYS_ api) */						\
	IMG_UINT32		ui32ActualFreq_fp;																				\

#define CLOCK_SOURCE_OVERRIDE_MEMBERS																				\
	/*! Override the default clock source for blocks that have multiple possible sources */							\
	IMG_BOOL		bOverrideClockSource;																			\
	/*! The clock source to use */																					\
	CLOCK_eSource	eClockSource;																					\

/*============================================================================
====	E N U M S
=============================================================================*/

// The source clock for a block to use
typedef enum
{
	CLOCK_SOURCE_PLL,
	CLOCK_SOURCE_XTAL1,
	CLOCK_SOURCE_XTAL2,
	CLOCK_SOURCE_XTALUSB,
	CLOCK_SOURCE_SYS_UNDELETED,
	CLOCK_SOURCE_ADC_PLL,
	CLOCK_SOURCE_AFE_PROGDIV1,
	CLOCK_SOURCE_AFE_PROGDIV2,
	CLOCK_SOURCE_AFE_PROGDIV3,
	CLOCK_SOURCE_UCC0_IF,
	CLOCK_SOURCE_UCC1_IF,
	CLOCK_SOURCE_USB_PLL,
	CLOCK_SOURCE_USB_PHY,
	CLOCK_SOURCE_AFE_RXSYNC,
	CLOCK_SOURCE_EXT_ADC_DAC,
} CLOCK_eSource;

/* The following is a list of possible control sources for PDM A and PDM B */
typedef enum
{
	PDM_SOURCE_UCC0_EXT_CONTROL_1,
	PDM_SOURCE_UCC0_EXT_CONTROL_2,
	PDM_SOURCE_UCC1_EXT_CONTROL_1,
	PDM_SOURCE_UCC1_EXT_CONTROL_2
} PDM_eControlSource;
/* The following is a list of possible gain sources for PDM C and PDM D */
typedef enum
{
	PDM_SOURCE_UCC0_EXT_GAIN_1,
	PDM_SOURCE_UCC0_EXT_GAIN_2,
	PDM_SOURCE_UCC1_EXT_GAIN_1,
	PDM_SOURCE_UCC1_EXT_GAIN_2
} PDM_eGainSource;
/* The following is a list of possible PDM Pad sources */
typedef enum
{
	PDM_PAD_SOURCE_PDM,
	PDM_PAD_SOURCE_SYMBOL_UCC0,
	PDM_PAD_SOURCE_SYMBOL_UCC1
} PDM_ePadSource;

typedef enum
{
	RANGE_1V_P2P = 0,
	RANGE_2V_P2P
} ADC_eInputRange;

/*============================================================================
====	S T R U C T S
=============================================================================*/

/*!
***********************************************************
 This structure contains the system clock setup parameters
***********************************************************/
typedef struct SYS_tag_sConfig
{
	/*! Set to true to set up the system clock */
	IMG_BOOL			bSetupSystemClock;
	struct
	{
		/*! The source clock used to generate the Meta clock
			-- Possible sources:  CLOCK_SOURCE_XTAL1, CLOCK_SOURCE_PLL */
		CLOCK_eSource		eMetaClockSource;
		/*! System and UCC clock divider, between 0 and 3 inclusive */
		IMG_UINT8			ui8SysClockDivider;
		/*! The source clock used by the SYS PLL, if PLL is chosen as source for Meta Clock (above)
			-- Possible sources: CLOCK_SOURCE_XTAL1, CLOCK_SOURCE_XTAL2 */
		CLOCK_eSource		ePLLSource;
		/*! Number of cycles deleted from each 1024 Meta Clock cycle period */
		IMG_UINT16			ui16MetaClockDeletion;
		/*! Number of cycles deleted from each 1024 System Clock cycle period */
		IMG_UINT16			ui16SysClockDeletion;
		/*! DDR Clock Divider */
		IMG_UINT8			ui8DDRClkDivider;
		/*! The desired META Clock frequency, in MHz, Q12.20 format */
		IMG_UINT32			ui32TargetFreq_fp;
	} sSystemClock;
	/*! If bSetupSystemClock is true, then the API will set the following to the META Clock
		frequency after system clock configuration, in MHz, Q12.20 format */
	/*! If bSetupSystemClock is false, then the API will set the following to the META Clock
		frequency as set up by the IMG file (or current register configuration), in MHZ, Q12.20 format */
	IMG_UINT32			ui32ActualFreq_fp;
	/*! The frequency of XTAL2, in MHz, Q12.20 format */
	IMG_UINT32			ui32XTAL2Freq_fp;
	/*! Set to true to allow XTAL2 to be fed by an oscillator (rather than an xtal) */
	IMG_BOOL			bUseXTAL2AsOsc;

	/*! The UCC configuration */
	struct
	{
		IMG_BOOL		bEnable;

		/* Clock deletion for a UCC */
		IMG_UINT16			ui16ClkDeletion;

		/* Clock source for IF_CLK */
		IMG_BOOL			bEnableIFClock;
		CLOCK_eSource		eIFClockSource;

		/* Clock source for ext_stc_clk */
		IMG_BOOL			bEnableSTCClock;
		CLOCK_eSource		eSTCClockSource;

		/* Set to true to enable UCC0 DAC clock */
		IMG_BOOL			bEnableUCC0DACClk;

		/* Set to true to have the UCC use the external ADC interface (this can be used for (bring up only) digital playout input) */
		/* Set to false to have the UCC use the internal ADC interface (default) */
		IMG_BOOL			bUseExternalADC;

	} sUCCConfig;

	/*! Set to true to enable the WIFI */
	IMG_BOOL			bEnableWIFI;

} SYS_sConfig;

typedef struct SYS_tag_sBlockConfig
{
	BASE_MEMBERS;

} SYS_sBlockConfig;

typedef struct SYS_tag_sSPIMConfig
{
	BASE_MEMBERS;
	TARGET_FRQUENCY_MEMBERS;

} SYS_sSPIMConfig;

typedef struct SYS_tag_sUARTConfig
{
	SYS_sBlockConfig asBlockConfig[2];
	TARGET_FRQUENCY_MEMBERS;
	CLOCK_SOURCE_OVERRIDE_MEMBERS;

} SYS_sUARTConfig;

typedef struct SYS_tag_sSCBConfig
{
	SYS_sBlockConfig	asBlockConfig[3];
	CLOCK_SOURCE_OVERRIDE_MEMBERS;

} SYS_sSCBConfig;

typedef struct SYS_tag_sPDMBlockConfig
{
	BASE_MEMBERS;
	PDM_ePadSource		ePadSource;
	PDM_eControlSource	eControlSource;
	PDM_eGainSource		eGainSource;

} SYS_sPDMBlockConfig;

typedef struct SYS_tag_sPDMConfig
{
	SYS_sPDMBlockConfig	asBlockConfig[4];
	TARGET_FRQUENCY_MEMBERS;

} SYS_sPDMConfig;

typedef struct SYS_tag_sSPISConfig
{
	BASE_MEMBERS;

} SYS_sSPISConfig;

typedef struct SYS_tag_sUSBConfig
{
	BASE_MEMBERS;
	CLOCK_SOURCE_OVERRIDE_MEMBERS;

	/* Set to true to have the API calculate the required dividers for the phy clock speed specified in ePHYClock */
	IMG_BOOL			bTargetPHYClock;
	/* If using the USB XTAL, this specifies the frequency of that XTAL */
	/* If not using the USB XTAL, this specific the frequency that the PHY should run at */
	enum
	{
		PHY_eNotSet =	0,
		PHY_e12		=	12,
		PHY_e24		=	24,
		PHY_e48		=	48
	} ePHYClock;

} SYS_sUSBConfig;

typedef struct SYS_tag_sClockOutConfig
{
	BASE_MEMBERS;
	CLOCK_SOURCE_OVERRIDE_MEMBERS;
	IMG_BOOL			bOverrideDivider;
	IMG_UINT8			ui8Divider;
	/* Set to true to invert clockout */
	IMG_BOOL			bInvertClkOutput;

} SYS_sClockOutConfig;

typedef struct SYS_tag_sADCPLLClockConfig
{
	CLOCK_SOURCE_OVERRIDE_MEMBERS;
	TARGET_FRQUENCY_MEMBERS;
	/*! The source clock used by the SYS PLL, if PLL is chosen as source for Meta Clock (above)
	-- Possible sources: CLOCK_SOURCE_XTAL1, CLOCK_SOURCE_XTAL2 */
	CLOCK_eSource		ePLLSource;

} SYS_sADCPLLClockConfig;

typedef struct SYS_tag_sAFEConfig
{
	/*! Override the default clock divider, must be set to false if the API should calculate
	the divider values for a target frequency */
	IMG_BOOL					bOverrideDivider;
	/*! The clock division to use */
	IMG_UINT8					ui8Divider;

	CLOCK_SOURCE_OVERRIDE_MEMBERS;

	struct
	{
		IMG_BOOL			bConfigure;
		IMG_BOOL			bEnable;

		/* If the AFE PLL is enabled, setting this to the desired sample rate will configure the dividers appropriately */
		IMG_UINT32			ui32SampleRate_fp;
		/* The actual sample rate configured */
		IMG_UINT32			ui32ActualRate_fp;

		/* Configures the RXADC input voltage range */
		ADC_eInputRange		eInputRange;

	} sRxADC;

	struct
	{
		IMG_BOOL			bConfigure;
		IMG_BOOL			bEnable;

		/* If the AFE PLL is enabled, setting this to the desired sample rate will configure the dividers appropriately */
		IMG_UINT32			ui32SampleRate_fp;
		/* The actual sample rate configured */
		IMG_UINT32			ui32ActualRate_fp;

		/* IMG_FALSE indicates DAC should be set to output common mode voltage of 0.6V, IMG_TRUE indicated 1.2V */
		IMG_BOOL			bCommonMode1v2;

	} sTxDAC;

	struct
	{
		IMG_BOOL			bConfigureADC;
		IMG_BOOL			bEnableADC;

		IMG_BOOL			bConfigureDAC;
		IMG_BOOL			bEnableDAC;

		/* If the AFE PLL is enabled, setting this to the desired sample rate will configure the dividers appropriately */
		IMG_UINT32			ui32SampleRate_fp;
		/* The actual sample rate configured */
		IMG_UINT32			ui32ActualRate_fp;

		enum
		{
			DATA_SOURCE_AFE_AUXDACIN_REG = 0,
			DATA_SOURCE_UCC0_EXT_GAIN_1,
			DATA_SOURCE_UCC0_EXT_CONTROL_1
		} eDACDataSource;

	} sAux;

	struct
	{
		IMG_BOOL			bConfigure;
		IMG_BOOL			bEnable;

		/* If the AFE PLL is enabled, setting this to the desired sample rate will configure the dividers appropriately */
		IMG_UINT32			ui32SampleRate_fp;
		/* The actual sample rate configured */
		IMG_UINT32			ui32ActualRate_fp;

		/* Set to true to use the AFE's internal PLL as the IQADC's clock source */
		/* Set to false to use the external clock input. This source can be configured with the structure below. */
		IMG_BOOL			bUseInternalPLL;

		struct
		{
			/* If the above is set to false, use the following to configure the external IQADC clock input
				-- Possible values are: XTAL1, XTAL2, ADC_PLL, SYS_CLK_UNDELETED */
			CLOCK_eSource		eClockSource;

			/* Set to true to override the ADCPLLDIV divider manually. Must be set to false if the API should calculate
				the divider values for a target frequency (bTargetFreq) */
			IMG_BOOL			bOverrideDivider;

			/* The divide value */
			IMG_UINT8			ui8Divider;

			/* Set to true to have the API calculate the required ADCPLLDIV divider for the target frequency specified below */
			IMG_BOOL			bTargetFreq;

			/* The desired external clock frequency, in MHz, Q12.20 format */
			IMG_UINT32			ui32TargetFreq_fp;
		} sExtClock;

		/* Configures the IQADC input voltage range */
		ADC_eInputRange		eInputRange;

	} sIQADC;

	/* Set to true to enable the AFE's internal PLL. The reference clock to the PLL is set up by using the AFE SYS_sBlockConfig structure.
		When set to true, the PLL will feed the RxADC, TxDAC, AuxADC/DAC, and optionally the IQADC */
	/* Set to false to bypass the AFE's internal PLL. The AFE source clock, configured in the AFE SYS_sBlockConfig structure, will directly feed the
		RxADC, TxDAC, and AuxADC/DAC */
	IMG_BOOL			bEnablePLL;

	/* Set to true when bEnablePLL is true to internally bypass the AFE's internal PLL.
		This will result in the Fin being the same as Fvco */
	IMG_BOOL			bBypassPLL;

	/* Set to true to _not_ reset the AFE before configuring */
	IMG_BOOL			bNoReset;

} SYS_sAFEConfig;

typedef struct SYS_tag_sDISEQCBlockConfig
{
	BASE_MEMBERS;

} SYS_sDISEQCBlockConfig;

typedef struct SYS_tag_sDISEQCConfig
{
	SYS_sDISEQCBlockConfig	asBlockConfig[2];

} SYS_sDISEQCConfig;

/*!
*******************************************************************************

 @Function              @SYS_configure

 <b>Description:</b>\n
 This function sets up the system according to the config structure passed in.

 \param						psConfig	Pointer to a SYS_sConfig structure

 \return                    None

*******************************************************************************/
extern IMG_VOID SYS_Configure( SYS_sConfig * psConfig );

/*!
*******************************************************************************

 @Function              @SYS_ConfigureSPIM

 <b>Description:</b>\n
 This function sets up the two SPI Master blocks according to the array of
 config structures passed in.  asConfig[0] specifies the configuration of SPIM0
 and asConfig[1] specifies the configuration of SPIM1.

 \param						asConfig	Array of two SYS_sSPIMConfig structures

 \return                    None

*******************************************************************************/
extern IMG_VOID SYS_ConfigureSPIM( SYS_sSPIMConfig asConfig[2] );

/*!
*******************************************************************************

 @Function              @SYS_ConfigureSPIS

 <b>Description:</b>\n
 This function sets up the SPI Slave block according to the config structure
 passed in.

 \param						psConfig	Pointer to a SYS_sSPISConfig structure

 \return                    None

*******************************************************************************/
extern IMG_VOID SYS_ConfigureSPIS( SYS_sSPISConfig * psConfig );

/*!
*******************************************************************************

 @Function              @SYS_ConfigureSCB

 <b>Description:</b>\n
 This function sets up the three SCB blocks according to the config structure
 passed in.  psConfig->asBlockConfig[0] specifies the configuration of SCB0,
 psConfig->asBlockConfig[1] specifies the configuration of SCB1, and
 psConfig->asBlockConfig[2] specifies the configuration of SCB2.

 \param						psConfig	Pointer to a SYS_sSCBConfig structure

 \return                    None

*******************************************************************************/
extern IMG_VOID SYS_ConfigureSCB( SYS_sSCBConfig * psConfig );

/*!
*******************************************************************************

 @Function              @SYS_ConfigureUART

 <b>Description:</b>\n
 This function sets up the two UART blocks according to the config structure
 passed in.  psConfig->asBlockConfig[0] specifies the configuration of UART0
 and psConfig->asBlockConfig[1] specifies the configuration of UART1.

 \param						psConfig	Pointer to a SYS_sUARTConfig structure

 \return                    None

*******************************************************************************/
extern IMG_VOID SYS_ConfigureUART( SYS_sUARTConfig * psConfig );

/*!
*******************************************************************************

 @Function              @SYS_ConfigureUSB

 <b>Description:</b>\n
 This function sets up the USB block according to the config structure passed
 in.

 \param						psConfig	Pointer to a SYS_sUARTConfig structure

 \return                    None

*******************************************************************************/
extern IMG_VOID SYS_ConfigureUSB( SYS_sUSBConfig * psConfig );

/*!
*******************************************************************************

 @Function              @SYS_ConfigureADCPLLClock

 <b>Description:</b>\n
 This function sets up the ADC according to the config structure passed in.

 \param						psConfig	Pointer to a SYS_sADCPLLClockConfig
										structure

 \return                    None

*******************************************************************************/
extern IMG_VOID SYS_ConfigureADCPLLClock( SYS_sADCPLLClockConfig * pConfig );

/*!
*******************************************************************************

 @Function              @SYS_ConfigureClockOut

 <b>Description:</b>\n
 This function sets up the two CLKOUT blocks according to the config structure
 passed in. asConfig[0] specifies the configuration of CLKOUT0 and asConfig[1]
 specifies the configuration of CLKOUT1.

 \param						asConfig	Array of two SYS_sClockOutConfig
										structures

 \return                    None

*******************************************************************************/
extern IMG_VOID SYS_ConfigureClockOut( SYS_sClockOutConfig asConfig[2] );

/*!
*******************************************************************************

 @Function              @SYS_ConfigureAFE

 <b>Description:</b>\n
 This function sets up the AFE according to the config structure passed in.

 \param						psConfig	Pointer to a SYS_sAFEConfig structure

 \return                    None

*******************************************************************************/
extern IMG_VOID SYS_ConfigureAFE( SYS_sAFEConfig * psConfig );

/*!
*******************************************************************************

 @Function              @SYS_ConfigurePDM

 <b>Description:</b>\n
 This function sets up the four PDM blocks according to the config structure
 passed in.  psConfig->asBlockConfig[0] specifies the configuration of PDM0,
 psConfig->asBlockConfig[1] specifies the configuration of PDM1,
 psConfig->asBlockConfig[2] specifies the configuration of PDM2 and
 psConfig->asBlockConfig[3] specifies the configuration of PDM3.

 \param						psConfig	Pointer to a SYS_sPDMConfig structure

 \return                    None

*******************************************************************************/
extern IMG_VOID SYS_ConfigurePDM( SYS_sPDMConfig * psConfig );

/*!
*******************************************************************************

 @Function              @SYS_ConfigureDISEQC

 <b>Description:</b>\n
 This function sets up the two DiSEqC blocks according to the config structure
 passed in.  psConfig->asBlockConfig[0] specifies the configuration of DISEQC0,
 psConfig->asBlockConfig[1] specifies the configuration of DISEQC1,

 \param						psConfig	Pointer to a SYS_sPDMConfig structure

 \return                    None

*******************************************************************************/
extern IMG_VOID SYS_ConfigureDISEQC(SYS_sDISEQCConfig *psConfig);






/*!
*******************************************************************************

 @Function              @SYS_getDividerValues

 <b>Description:</b>\n
 This function calculates a divider value which produces the closest frequency to
 the required output frequency, given the divider "size" and source clock.

 \param			ui32SourceFreq_fp		The source frequency, in MHz, Q12.20 format

 \param			ui32TargetFreq_fp		The desired frequncy, in MHz, Q12.20 format

 \param			ui32DividerRange		The range of possible clock divisions (eg for a DivBy256 divider, specify 256)

 \param			*pui32Divider			Address where calculated divider value will be stored

 \param			*pui32ActualFreq_fp		Address where actual frequency will be stored, in MHz, Q12.20 format

 \return								None

*******************************************************************************/
//#if !defined (__IMG_HW_FPGA__)
IMG_VOID SYS_getDividerValues
(
	const IMG_UINT32		ui32SourceFreq_fp,
	const IMG_UINT32		ui32TargetFreq_fp,
	const IMG_UINT32		ui32DividerRange,
		  IMG_UINT32	*	pui32Divider,
	 	  IMG_UINT32	*	pui32ActualFreq_fp
);
//#endif

/*!
*******************************************************************************

 @Function              @SYS_getSysUndeletedFreq_fp

 <b>Description:</b>\n
 This function returns the sys_clk_undeleted frequency, in MHz, Q12.20 format

 \param						None

 \return                    sys_clk_undeleted frequency.

*******************************************************************************/
extern IMG_UINT32 SYS_getSysUndeletedFreq_fp();

#endif /* __SYS_CONF_H__ */
