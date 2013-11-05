/*!
*******************************************************************************
 @file   gpio_api.h

 @brief  General Purpose Input/Output API

         This file describes the software interface for the General Purpose
         Input/Output (GPIO) module.
		 
		 This module contains the core GPIO pins. For a list of which pins this includes,
		 consult the #defines in this header file. Other GPIO pins exist that are not 
		 in this module, they will be available through the software interface provided by
		 the module they belong to.

		 Some GPIO pins share settings for slew rate, drive strength and schmitt triggering.
		 Consult the comments at the end of this header file for the groupings.

         This API provides the facility to register pins which may be configured 
		 as input or output. The GPIO interrupt transition/level detection system can be 
		 configured and maintained.

		 All GPIO API callback functions are called in the interrupt context. The code for these 
		 callbacks should be short and adhere to interrupt context code execution rules defined 
		 in the MeOS documentation. 
		 
		 Some GPIO API functions may be called under interrupt context. Consult the individual
		 function documentation.


 @author Imagination Technologies

         <b>Copyright 2011 by Imagination Technologies Limited.</b>\n
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

#if !defined (__GPIO_API_H__)
#define __GPIO_API_H__

#include "img_defs.h"
#include <MeOS.h>
#include <ioblock_defs.h>



/******************************************************************************
******************************** Enumerations *********************************
*******************************************************************************/

/*!
*******************************************************************************

 These define the GPIO API function return values.

*******************************************************************************/
typedef enum
{
    /*! Function completed normally */
    GPIO_STATUS_OK = 0,

	/*! Error: GPIO has already been initialised */
	GPIO_STATUS_ERR_INITIALISED,

	/*! Error: GPIO has not been initialised */
	GPIO_STATUS_ERR_UNINITIALISED,

	/*! Error: Pin is already active */
	GPIO_STATUS_ERR_ACTIVE,

	/*! Error: Active pins remain, cannot deinitialise */
	GPIO_STATUS_ERR_ACTIVEPINS,

	/*! Error: Pin is not active */
	GPIO_STATUS_ERR_INACTIVE,

    /*! Error: This pin has already been registered with the API */
    GPIO_STATUS_ERR_REPEAT,

	/*! Error: The block index parameter was not valid */
    GPIO_STATUS_INVALID_BLOCK,

    /*! Error: The pin number parameter was not valid */
    GPIO_STATUS_INVALID_PIN,

    /*! Error: The direction parameter was not valid */
    GPIO_STATUS_INVALID_DIRECTION,

    /*! Error: The level parameter was not valid */
    GPIO_STATUS_INVALID_LEVEL,

    /*! Error: The pullup/pulldown parameter was not valid */
    GPIO_STATUS_INVALID_PULLUP_PULLDOWN,

	/*! Error: The drive strength parameter was not valid */
	GPIO_STATUS_INVALID_DRIVE_STRENGTH,

	/*! Error: The slew rate parameter was not valid */
	GPIO_STATUS_INVALID_SLEW_RATE,

	/*! Error: The schmitt parameter was not valid */
	GPIO_STATUS_INVALID_SCHMITT,

	/*! Error: The interrupt type parameter was not valid */
	GPIO_STATUS_INVALID_INTERRUPT_TYPE,

} GPIO_STATUS_T;

/*!
*******************************************************************************

 These define the GPIO pin logic levels.

*******************************************************************************/
typedef enum
{
    /*! Low */
    GPIO_LEVEL_LOW = 0,

    /*! High */
    GPIO_LEVEL_HIGH

} GPIO_LEVEL_T;

#define	GPIO_LEVEL_T_BITS	(1)

/*!
*******************************************************************************

 These define the GPIO pin directions.

*******************************************************************************/
typedef enum
{
    /*! Pin is suitable for output (either High or Low). */
    GPIO_DIR_OUTPUT = 0,

	/*! Pin is high impedance, suitable for input. */
    GPIO_DIR_INPUT

} GPIO_DIRECTION_T;

#define	GPIO_DIRECTION_T_BITS	(1)

/*!
*******************************************************************************

 These define the GPIO pin output pullup/pulldown configurations.

*******************************************************************************/
typedef enum
{
    /*! Pullup/pulldown not connected (Z)*/
    GPIO_PU_PD_OFF	=	0,

    /*! Pullup */
    GPIO_PU_PD_PULLUP,

	/*! Pulldown */
	GPIO_PU_PD_PULLDOWN,

	/*! Bus keeper enabled */
	GPIO_PU_PD_BUS_KEEPER

} GPIO_PULLUP_PULLDOWN_T;

#define	GPIO_PULLUP_PULLDOWN_T_BITS	(2)


/*!
*******************************************************************************

 These define the GPIO pin options for drive strength

*******************************************************************************/
typedef enum
{
    /*! 2mA drive strength */
    GPIO_DRIVE_2MA = 0,

    /*! 4mA drive strength */
    GPIO_DRIVE_4MA,

	/*! 8mA drive strength */
    GPIO_DRIVE_8MA,

	/*! 16mA drive strength */
    GPIO_DRIVE_16MA

} GPIO_DRIVE_STRENGTH_T;

#define	GPIO_DRIVE_STRENGTH_T_BITS	(2)

/*!
*******************************************************************************

 These define the GPIO pin options for schmitt triggering

*******************************************************************************/
typedef enum
{
    /*! No schmitt trigger */
    GPIO_SCHMITT_OFF = 0,

    /*! Schmitt trigger */
    GPIO_SCHMITT_ON

} GPIO_SCHMITT_T;

#define	GPIO_SCHMITT_T_BITS	(1)

/*!
*******************************************************************************

 These define the GPIO pin options for slew rate

*******************************************************************************/
typedef enum
{
    /*! Slow slew rate */
    GPIO_SLEW_SLOW = 0,

    /*! Fast slew rate */
    GPIO_SLEW_FAST

} GPIO_SLEW_T;

#define	GPIO_SLEW_T_BITS	(1)

/*!
*******************************************************************************

 These define the GPIO pin interrupt configuration options.

*******************************************************************************/
typedef enum
{
    /*! Level detection - low polarity. */
    GPIO_INT_TYPE_LEVEL_LOW = 0,

    /*! Level detection - high polarity. */
    GPIO_INT_TYPE_LEVEL_HIGH,

    /*! Edge/transition detection - falling edge. */
    GPIO_INT_TYPE_EDGE_FALL,

    /*! Edge/transition detection - rising edge. */
    GPIO_INT_TYPE_EDGE_RISE,

	/*! Edge/transition detection - rising and falling edge. */
	GPIO_INT_TYPE_EDGE_TRANSITION,

} GPIO_INTERRUPT_TYPE_T;

#define	GPIO_INTERRUPT_TYPE_T_BITS	(3)




/*! Callback function definition. Context contains the value given at GPIOInterruptEnable, Level indicates the level on the pin that caused the interrupt. */
typedef void (*GPIO_CALLBACK_T)(IMG_PVOID pContext, GPIO_LEVEL_T Level);

/*!
*******************************************************************************

 @brief This structure describes a GPIO pin logical object. It contains all 
 context required by a single GPIO pin.

*******************************************************************************/

typedef struct gpio_pin_t
{
	/*! Used internally */
	DQ_LINK;
	/*! Pin is active */
	IMG_BOOL			bActive;
	/*! Used internally */
	IMG_UINT8			ui8BlockIndex;
	/*! Used internally */
	IMG_UINT8			ui8PinNumber;
	/*! Used internally */
	GPIO_DIRECTION_T	Direction;
	/*! Used internally */
	IMG_BOOL			bInterrupt;
	/*! Used internally */
	IMG_BOOL			bDualEdgeTransitionInterrupt;
	/*! Used internally */
	GPIO_LEVEL_T		InterruptLevel;
	/*! Used internally */
	GPIO_CALLBACK_T		pfnInterruptCallback;
	/*! Used internally */
	IMG_PVOID			pInterruptCallbackContext;
}GPIO_PIN_T;


/*!
*******************************************************************************

 @brief This structure describes the settings for a GPIO pin.

*******************************************************************************/

typedef struct gpio_pin_settings_t
{
	/*! I/O Direction of the pin */
    GPIO_DIRECTION_T				Direction;
	union
	{
		struct
		{
			/*! (Input) Schmitt triggering */
			GPIO_SCHMITT_T			Schmitt;
			/*! (Input) Pullup/Pulldown */
			GPIO_PULLUP_PULLDOWN_T	PullupPulldown;
		}Input;
		struct
		{
			/*! (Output) Logic level to be output */
			GPIO_LEVEL_T			Level;
			/*! (Output) Drive strength */
			GPIO_DRIVE_STRENGTH_T	DriveStrength;
			/*! (Output) Slew rate */
			GPIO_SLEW_T				Slew;
		}Output;
	};
}GPIO_PIN_SETTINGS_T;


/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

/*!
*******************************************************************************

 @Function              @GPIOInit

<b>Description:</b>\n
 This function is used to initialise the GPIO API. It should be called prior to 
 any other GPIO API functions being called. This function should not be called
 in the interrupt context.

 \return				This function returns as follows:\n
                        ::GPIO_STATUS_OK Operation completed successfully.\n
						::GPIO_STATUS_ERR_INITIALISED Already initialised.\n
							
*******************************************************************************/

GPIO_STATUS_T GPIOInit();


/*!
*******************************************************************************

 @Function              @GPIOAddPin

<b>Description:</b>\n
 This function is used to initialise a GPIO pin. Interrupts will be initially disabled
 for the pin following a call to this function. The pin descriptor must be initialised
 prior to being used with other GPIO API functions. This function can be called
 in the interrupt context.

 \param     *psPin			Pointer to pin descriptor. The bActive member must be 
							set to IMG_FALSE. The pin descriptor must be preserved 
							until a call is made to GPIORemovePin with the pin descriptor.

 \param		ui8BlockIndex	Index of the block the pin belongs to. Consult the #defines
							in this header file to obtain block indexes and pin numbers

 \param		ui8PinNumber	Pin number within the block. Consult the #defines
							in this header file to obtain block indexes and pin numbers

 \param     *psSettings		Pointer to initial settings for the pin.

 \return                    This function returns as follows:\n
                            ::GPIO_STATUS_OK Operation completed successfully.\n
							::GPIO_STATUS_ERR_UNINITIALISED GPIO has not been intialised.\n
							::GPIO_STATUS_ERR_ACTIVE Pin descriptor is already active.\n
							::GPIO_STATUS_INVALID_BLOCK Invalid block index.\n
							::GPIO_STATUS_INVALID_PIN Invalid pin number.\n
							::GPIO_STATUS_ERR_REPEAT Pin already registered with another descriptor.\n
							::GPIO_STATUS_INVALID_SCHMITT invalid schmitt parameter.\n
							::GPIO_STATUS_INVALID_PULLUP_PULLDOWN Invalid pullup/pulldown parameter.\n
							::GPIO_STATUS_INVALID_LEVEL Invalid level parameter.\n
							::GPIO_STATUS_INVALID_DRIVE_STRENGTH Invalid drive strength parameter.\n
							::GPIO_STATUS_INVALID_SLEW_RATE Invalid slew rate parameter.\n
							::GPIO_STATUS_INVALID_DIRECTION Invalid direction parameter.\n

*******************************************************************************/
GPIO_STATUS_T GPIOAddPin
(
	GPIO_PIN_T			*psPin,
	img_uint8			ui8BlockIndex,
	img_uint8			ui8PinNumber,
	GPIO_PIN_SETTINGS_T	*psSettings
);

/*!
*******************************************************************************

 @Function              @GPIOConfigure

<b>Description:</b>\n
 This function is used to change the configuration of a GPIO pin. This function 
 can be called in the interrupt context.

 \param     *psPin				Pointer to initialised pin descriptor.

 \param		*psSettings			Pointer to new settings for the pin.

 \return                        This function returns as follows:\n
                                ::GPIO_STATUS_OK Operation completed successfully.\n
								::GPIO_STATUS_ERR_UNINITIALISED GPIO has not been intialised.\n
								::GPIO_STATUS_ERR_INACTIVE Pin descriptor is inactive.\n
								::GPIO_STATUS_INVALID_SCHMITT invalid schmitt parameter.\n
								::GPIO_STATUS_INVALID_PULLUP_PULLDOWN Invalid pullup/pulldown parameter.\n
								::GPIO_STATUS_INVALID_LEVEL Invalid level parameter.\n
								::GPIO_STATUS_INVALID_DRIVE_STRENGTH Invalid drive strength parameter.\n
								::GPIO_STATUS_INVALID_SLEW_RATE Invalid slew rate parameter.\n
								::GPIO_STATUS_INVALID_DIRECTION Invalid direction parameter.\n
                                

*******************************************************************************/
GPIO_STATUS_T GPIOConfigure
(
	GPIO_PIN_T			*psPin,
	GPIO_PIN_SETTINGS_T	*psSettings
);

/*!
*******************************************************************************

 @Function              @GPIOSet

 <b>Description:</b>\n
 This function sets the level of a GPIO line to high or low. This function should 
 only be called for GPIO lines that have been set to I/O direction output. This 
 function can be called in the interrupt context.

 \param     psPin    Pointer to initialised pin descriptor.

 \param     Level    The logic level to which the pin is to be set.

 \return             This function returns as follows:\n
                     ::GPIO_OK Function completed normally\n
					 ::GPIO_STATUS_ERR_UNINITIALISED GPIO has not been intialised.\n
					 ::GPIO_STATUS_ERR_INACTIVE Pin descriptor is inactive.\n
					 ::GPIO_STATUS_INVALID_LEVEL Level parameter is invalid.\n

*******************************************************************************/
GPIO_STATUS_T GPIOSet
(
    GPIO_PIN_T		*psPin,
    GPIO_LEVEL_T	Level
);

/*!
*******************************************************************************

 @Function              @GPIORead

 <b>Description:</b>\n
 This function reads the logic level of a GPIO line. This function should only be 
 called for GPIO lines that have been set to I/O direction input. This function can
 be called in the interrupt context.

 \param     psPin    Pointer to initialised pin descriptor.

 \param     Value    Updated with value read from GPIO pin.

 \return             This function returns as follows:\n
                     ::GPIO_OK Function completed normally\n
					 ::GPIO_STATUS_ERR_UNINITIALISED GPIO has not been intialised.\n
					 ::GPIO_STATUS_ERR_INACTIVE Pin descriptor is inactive.\n

*******************************************************************************/
GPIO_STATUS_T GPIORead
(
    GPIO_PIN_T		*psPin,
    GPIO_LEVEL_T	*Value
);

/*!
*******************************************************************************

 @Function              @GPIOEnableInterrupt

 <b>Description:</b>\n
 This function configures and enables the interrupt for a GPIO line. This function
 should only be called for GPIO lines that have been set to I/O direction input. This
 function can be called in the interrupt context.

 \param     psPin					Pointer to initialised pin descriptor.

 \param     InterruptType			Interrupt type.

 \param     pfnInterruptCallback	Interrupt callback.

 \param     pInterruptContext		Interrupt callback context.

 \return							This function returns as follows:\n
									::GPIO_OK Function completed normally\n
									::GPIO_STATUS_ERR_UNINITIALISED GPIO has not been intialised.\n
									::GPIO_STATUS_ERR_INACTIVE Pin descriptor is inactive.\n
									::GPIO_STATUS_INVALID_INTERRUPT_TYPE Invalid interrupt type.\n

*******************************************************************************/

GPIO_STATUS_T GPIOEnableInterrupt
(
	GPIO_PIN_T				*psPin,
	GPIO_INTERRUPT_TYPE_T	InterruptType,
	GPIO_CALLBACK_T			pfnInterruptCallback,
	IMG_PVOID				pContext
);

/*!
*******************************************************************************

 @Function              @GPIODisableInterrupt

 <b>Description:</b>\n
 This function disables the interrupt for a GPIO line. This function should only 
 be called for GPIO lines that have been set to I/O direction input. This function 
 can be called in the interrupt context.

 \param     psPin		 Pointer to initialised pin descriptor.

 \return             This function returns as follows:\n
                     ::GPIO_OK Function completed normally\n
					 ::GPIO_STATUS_ERR_UNINITIALISED GPIO has not been intialised.\n
					 ::GPIO_STATUS_ERR_INACTIVE Pin descriptor is inactive.\n

*******************************************************************************/

GPIO_STATUS_T GPIODisableInterrupt
(
	GPIO_PIN_T *psPin
);

/*!
*******************************************************************************

 @Function              @GPIORemovePin

<b>Description:</b>\n
 This function is used to deinitialise a GPIO pin. It should be called
 if the primary function of the pin is to be restored or in the event of
 a system shutdown. This function can be called in the interrupt context.

 The pins register settings will be restored to their default values. The
 pin will be deselected as a GPIO pin.

 \param     *psPin		Pointer to initialised pin descriptor.

 \return                This function returns as follows:\n
                        ::GPIO_STATUS_OK Operation completed successfully.\n
						::GPIO_STATUS_ERR_UNINITIALISED GPIO has not been intialised.\n
						::GPIO_STATUS_ERR_INACTIVE Pin descriptor is inactive.\n
                                

*******************************************************************************/
GPIO_STATUS_T GPIORemovePin
(
	GPIO_PIN_T *psPin
);

/*!
*******************************************************************************

 @Function              @GPIODeinit

<b>Description:</b>\n
 This function is used to deinitialise the GPIO API. This function should not be called
 in the interrupt context.

 \return				This function returns as follows:\n
                        ::GPIO_STATUS_OK Operation completed successfully.\n
						::GPIO_STATUS_ERR_UNINITIALISED GPIO has not been intialised.\n
                        ::GPIO_STATUS_ERR_ACTIVEPINS There are active pins 
						that must be removed using GPIORemovePin.\n

*******************************************************************************/

GPIO_STATUS_T GPIODeinit();

/******************************************************************************
********************** Pin Block Indexes + Pin Numbers ************************
*******************************************************************************/

/* Below is a list of all core GPIO pins with the correct block indexes and pin numbers. */

#define	GPIO_PIN_SPI0_MCLK_BLOCK				(0)
#define	GPIO_PIN_SPI0_MCLK_PIN					(0)
#define	GPIO_PIN_SPI0_CS0_BLOCK					(0)
#define	GPIO_PIN_SPI0_CS0_PIN					(1)
#define	GPIO_PIN_SPI0_CS1_BLOCK					(0)
#define	GPIO_PIN_SPI0_CS1_PIN					(2)
#define	GPIO_PIN_SPI0_CS2_BLOCK					(0)
#define	GPIO_PIN_SPI0_CS2_PIN					(3)
#define	GPIO_PIN_SPI0_DOUT_BLOCK				(0)
#define	GPIO_PIN_SPI0_DOUT_PIN					(4)
#define	GPIO_PIN_SPI0_DIN_BLOCK					(0)
#define	GPIO_PIN_SPI0_DIN_PIN					(5)
#define	GPIO_PIN_SPI1_MCLK_BLOCK				(0)
#define	GPIO_PIN_SPI1_MCLK_PIN					(6)
#define	GPIO_PIN_SPI1_CS0_BLOCK					(0)
#define	GPIO_PIN_SPI1_CS0_PIN					(7)
#define	GPIO_PIN_SPI1_CS1_BLOCK					(0)
#define	GPIO_PIN_SPI1_CS1_PIN					(8)
#define	GPIO_PIN_SPI1_CS2_BLOCK					(0)
#define	GPIO_PIN_SPI1_CS2_PIN					(9)
#define	GPIO_PIN_SPI1_DOUT_BLOCK				(0)
#define	GPIO_PIN_SPI1_DOUT_PIN					(10)
#define	GPIO_PIN_SPI1_DIN_BLOCK					(0)
#define	GPIO_PIN_SPI1_DIN_PIN					(11)
#define	GPIO_PIN_UART0_RXD_BLOCK				(0)
#define	GPIO_PIN_UART0_RXD_PIN					(12)
#define	GPIO_PIN_UART0_TXD_BLOCK				(0)
#define	GPIO_PIN_UART0_TXD_PIN					(13)
#define	GPIO_PIN_UART0_CTS_BLOCK				(0)
#define	GPIO_PIN_UART0_CTS_PIN					(14)
#define	GPIO_PIN_UART0_RTS_BLOCK				(0)
#define	GPIO_PIN_UART0_RTS_PIN					(15)
#define	GPIO_PIN_UART1_RXD_BLOCK				(0)
#define	GPIO_PIN_UART1_RXD_PIN					(16)
#define	GPIO_PIN_UART1_TXD_BLOCK				(0)
#define	GPIO_PIN_UART1_TXD_PIN					(17)
#define	GPIO_PIN_SCB0_SDAT_BLOCK				(0)
#define	GPIO_PIN_SCB0_SDAT_PIN					(18)
#define	GPIO_PIN_SCB0_SCLK_BLOCK				(0)
#define	GPIO_PIN_SCB0_SCLK_PIN					(19)
#define	GPIO_PIN_SCB1_SDAT_BLOCK				(0)
#define	GPIO_PIN_SCB1_SDAT_PIN					(20)
#define	GPIO_PIN_SCB1_SCLK_BLOCK				(0)
#define	GPIO_PIN_SCB1_SCLK_PIN					(21)
#define	GPIO_PIN_SCB2_SDAT_BLOCK				(0)
#define	GPIO_PIN_SCB2_SDAT_PIN					(22)
#define	GPIO_PIN_SCB2_SCLK_BLOCK				(0)
#define	GPIO_PIN_SCB2_SCLK_PIN					(23)
#define	GPIO_PIN_PDM_A_BLOCK					(0)
#define	GPIO_PIN_PDM_A_PIN						(24)
#define	GPIO_PIN_PDM_B_BLOCK					(0)
#define	GPIO_PIN_PDM_B_PIN						(25)

#define	GPIO_PIN_PDM_C_BLOCK					(1)
#define	GPIO_PIN_PDM_C_PIN						(0)
#define	GPIO_PIN_PDM_D_BLOCK					(1)
#define	GPIO_PIN_PDM_D_PIN						(1)
#define	GPIO_PIN_TS_CLK_BLOCK					(1)
#define	GPIO_PIN_TS_CLK_PIN						(2)
#define	GPIO_PIN_TS_SYNC_BLOCK					(1)
#define	GPIO_PIN_TS_SYNC_PIN					(3)
#define	GPIO_PIN_TS_ERR_BLOCK					(1)
#define	GPIO_PIN_TS_ERR_PIN						(4)
#define	GPIO_PIN_TS_VALID0_BLOCK				(1)
#define	GPIO_PIN_TS_VALID0_PIN					(5)
#define	GPIO_PIN_TS_VALID1_BLOCK				(1)
#define	GPIO_PIN_TS_VALID1_PIN					(6)
#define	GPIO_PIN_TS_VALID2_BLOCK				(1)
#define	GPIO_PIN_TS_VALID2_PIN					(7)
#define	GPIO_PIN_TS_DATA0_BLOCK					(1)
#define	GPIO_PIN_TS_DATA0_PIN					(8)
#define	GPIO_PIN_TS_DATA1_BLOCK					(1)
#define	GPIO_PIN_TS_DATA1_PIN					(9)
#define	GPIO_PIN_TS_DATA2_BLOCK					(1)
#define	GPIO_PIN_TS_DATA2_PIN					(10)
#define	GPIO_PIN_TS_DATA3_BLOCK					(1)
#define	GPIO_PIN_TS_DATA3_PIN					(11)
#define	GPIO_PIN_TS_DATA4_BLOCK					(1)
#define	GPIO_PIN_TS_DATA4_PIN					(12)
#define	GPIO_PIN_TS_DATA5_BLOCK					(1)
#define	GPIO_PIN_TS_DATA5_PIN					(13)
#define	GPIO_PIN_TS_DATA6_BLOCK					(1)
#define	GPIO_PIN_TS_DATA6_PIN					(14)
#define	GPIO_PIN_TS_DATA7_BLOCK					(1)
#define	GPIO_PIN_TS_DATA7_PIN					(15)
#define	GPIO_PIN_TX_ON_BLOCK					(1)
#define	GPIO_PIN_TX_ON_PIN						(16)
#define	GPIO_PIN_RX_ON_BLOCK					(1)
#define	GPIO_PIN_RX_ON_PIN						(17)
#define	GPIO_PIN_PLL_ON_BLOCK					(1)
#define	GPIO_PIN_PLL_ON_PIN						(18)
#define	GPIO_PIN_PA_ON_BLOCK					(1)
#define	GPIO_PIN_PA_ON_PIN						(19)
#define	GPIO_PIN_RX_HP_BLOCK					(1)
#define	GPIO_PIN_RX_HP_PIN						(20)
#define	GPIO_PIN_GAIN0_BLOCK					(1)
#define	GPIO_PIN_GAIN0_PIN						(21)
#define	GPIO_PIN_GAIN1_BLOCK					(1)
#define	GPIO_PIN_GAIN1_PIN						(22)
#define	GPIO_PIN_GAIN2_BLOCK					(1)
#define	GPIO_PIN_GAIN2_PIN						(23)
#define	GPIO_PIN_GAIN3_BLOCK					(1)
#define	GPIO_PIN_GAIN3_PIN						(24)
#define	GPIO_PIN_GAIN4_BLOCK					(1)
#define	GPIO_PIN_GAIN4_PIN						(25)

#define	GPIO_PIN_GAIN5_BLOCK					(2)
#define	GPIO_PIN_GAIN5_PIN						(0)
#define	GPIO_PIN_GAIN6_BLOCK					(2)
#define	GPIO_PIN_GAIN6_PIN						(1)
#define	GPIO_PIN_GAIN7_BLOCK					(2)
#define	GPIO_PIN_GAIN7_PIN						(2)
#define	GPIO_PIN_ANT_SEL0_BLOCK					(2)
#define	GPIO_PIN_ANT_SEL0_PIN					(3)
#define	GPIO_PIN_ANT_SEL1_BLOCK					(2)
#define	GPIO_PIN_ANT_SEL1_PIN					(4)
#define	GPIO_PIN_PHYWAY_CLKOUT_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_CLKOUT_PIN				(5)
#define	GPIO_PIN_PHYWAY_VALIDOUT_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_VALIDOUT_PIN			(6)
#define	GPIO_PIN_PHYWAY_DATAOUT0_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAOUT0_PIN			(7)
#define	GPIO_PIN_PHYWAY_DATAOUT1_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAOUT1_PIN			(8)
#define	GPIO_PIN_PHYWAY_DATAOUT2_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAOUT2_PIN			(9)
#define	GPIO_PIN_PHYWAY_DATAOUT3_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAOUT3_PIN			(10)
#define	GPIO_PIN_PHYWAY_DATAOUT4_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAOUT4_PIN			(11)
#define	GPIO_PIN_PHYWAY_DATAOUT5_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAOUT5_PIN			(12)
#define	GPIO_PIN_PHYWAY_DATAOUT6_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAOUT6_PIN			(13)
#define	GPIO_PIN_PHYWAY_DATAOUT7_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAOUT7_PIN			(14)
#define	GPIO_PIN_PHYWAY_CLKIN_BLOCK				(2)
#define	GPIO_PIN_PHYWAY_CLKIN_PIN				(15)
#define	GPIO_PIN_PHYWAY_VALIDIN_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_VALIDIN_PIN				(16)
#define	GPIO_PIN_PHYWAY_DATAIN0_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAIN0_PIN				(17)
#define	GPIO_PIN_PHYWAY_DATAIN1_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAIN1_PIN				(18)
#define	GPIO_PIN_PHYWAY_DATAIN2_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAIN2_PIN				(19)
#define	GPIO_PIN_PHYWAY_DATAIN3_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAIN3_PIN				(20)
#define	GPIO_PIN_PHYWAY_DATAIN4_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAIN4_PIN				(21)
#define	GPIO_PIN_PHYWAY_DATAIN5_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAIN5_PIN				(22)
#define	GPIO_PIN_PHYWAY_DATAIN6_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAIN6_PIN				(23)
#define	GPIO_PIN_PHYWAY_DATAIN7_BLOCK			(2)
#define	GPIO_PIN_PHYWAY_DATAIN7_PIN				(24)

/******************************************************************************
*********** Schmitt Trigger,Slew Rate,Drive Strength Line Sharing *************
*******************************************************************************/

/* Below is a list of lines that are shared for Schmitt Trigger, Slew Rate and
    Drive Strength settings (if a setting is applied for one of the lines it will
	change the setting for the others in the same group) */

/* Group 0 
PLL_ON
PA_ON
RX_HP
ANT_SEL0
ANT_SEL1
GAIN0
GAIN1
GAIN2
GAIN3
GAIN4
GAIN5
GAIN6
GAIN7 */

/* Group 1 
PDM_A
PDM_B
CLK_OUT0 */

/* Group 2 
PDM_C
PDM_D
SCB0_SDAT
SCB0_SCLK */

/* Group 3 
SPI1_MCLK
SPI1_CS0
SPI1_CS1
SPI1_CS2
SPI1_DOUT
SPI1_DIN */

/* Group 4 
SCB1_SDAT
SCB1_SCLK */

/* Group 5 
UART0_RXD
UART0_TXD
UART0_CTS
UART0_RTS
UART1_RXD
UART1_TXD */

/* Group 6 
CLK_OUT1 */

/* Group 7 
SPI0_MCLK
SPI0_CS0
SPI0_CS1
SPI0_CS2
SPI0_DOUT
SPI0_DIN */

/* Group 8 
SCB2_SCLK
SCB2_SDAT */

/* Group 9 
PHYWAY_CLKOUT
PHYWAY_VALOUT
PHYWAY_CLKIN
PHYWAY_VALIN
PHYWAY_DATAOUT0
PHYWAY_DATAOUT1
PHYWAY_DATAOUT2
PHYWAY_DATAOUT3
PHYWAY_DATAOUT4
PHYWAY_DATAOUT5
PHYWAY_DATAOUT6
PHYWAY_DATAOUT7
PHYWAY_DATAIN0
PHYWAY_DATAIN1
PHYWAY_DATAIN2
PHYWAY_DATAIN3
PHYWAY_DATAIN4
PHYWAY_DATAIN5
PHYWAY_DATAIN6
PHYWAY_DATAIN7 */

/* Group 10 
TMS
TDI
TRST
TCK */

/* Group 11 
TS_CLK
TS_SYNC
TS_ERR
TS_VALID0
TS_VALID1
TS_VALID2
TS_DATA0
TS_DATA1
TS_DATA2
TS_DATA3
TS_DATA4
TS_DATA5
TS_DATA6
TS_DATA7 */


#endif /* __GPIO_API_H__ */
