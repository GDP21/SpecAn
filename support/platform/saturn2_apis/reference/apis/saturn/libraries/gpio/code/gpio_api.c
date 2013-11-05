/*!
*******************************************************************************
  file   gpio_api.c

  brief  General Purpose Input/Output API

         This file defines the functions that make up the General Purpose
         Input/Output (GPIO) API.

  author Imagination Technologies

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

/*============================================================================*/
/*                          INCLUDE FILES                                     */
/*============================================================================*/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
** TBI_NO_INLINES is needed for use of TBI_CRITON() and TBI_CRITOFF()
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#define TBI_NO_INLINES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

/* MeOS */
#include <MeOS.h>

/* System */
#include <system.h>
#include <sys_util.h>

/* GPIO */
#include "gpio_api.h"
#include "gpio_drv.h"
#include "gpio_reg_defs.h"

/*============================================================================*/
/*                          MACRO DEFINITIONS                                 */
/*============================================================================*/

#define	GPIO_API(API)																		\
	KRN_IPL_T oldipl;																		\
	GPIO_STATUS_T Ret;																		\
	if((Ret = GPIOEnter(&oldipl)) == GPIO_STATUS_OK)										\
	{																						\
		Ret = API;																			\
		GPIOLeave(oldipl);																	\
	}																						\
	return Ret;																				

/*============================================================================*/
/*                          DATA STRUCTURES                                   */
/*============================================================================*/


/* Global GPIO state types */
typedef enum
{
	GPIO_STATE_UNINITIALISED = 0,
	GPIO_STATE_INITIALISING,
	GPIO_STATE_INITIALISED,
	GPIO_STATE_DEINITIALISING
}GPIO_STATE_T;


/*============================================================================*/
/*                            STATIC DATA                                     */
/*============================================================================*/


/* Global GPIO state variable */
static GPIO_STATE_T		g_GPIOState = GPIO_STATE_UNINITIALISED;


/*============================================================================*/
/*                     STATIC FUNCTION DECLARATIONS                           */
/*============================================================================*/


static GPIO_STATUS_T _GPIOAddPin(GPIO_PIN_T *psPin, img_uint8 ui8BlockIndex, img_uint8 ui8PinNumber, GPIO_PIN_SETTINGS_T *psSettings);
static GPIO_STATUS_T _GPIOConfigure(GPIO_PIN_T *psPin, GPIO_PIN_SETTINGS_T *psSettings);
static GPIO_STATUS_T _GPIOSet(GPIO_PIN_T *psPin, GPIO_LEVEL_T Level);
static GPIO_STATUS_T _GPIORead(GPIO_PIN_T *pin, GPIO_LEVEL_T *value);
static GPIO_STATUS_T _GPIOEnableInterrupt(GPIO_PIN_T *psPin, GPIO_INTERRUPT_TYPE_T InterruptType, GPIO_CALLBACK_T pfnInterruptCallback, IMG_PVOID pInterruptContext);
static GPIO_STATUS_T _GPIODisableInterrupt(GPIO_PIN_T *psPin);
static GPIO_STATUS_T _GPIORemovePin(GPIO_PIN_T *psPin);
static GPIO_STATUS_T _GPIODeinit();


/*============================================================================*/
/*                              FUNCTIONS                                     */
/*============================================================================*/

/* Synchronisation code for entering a GPIO API function - everything done in the
   interrupt context to allow the APIs to be called from the interrupt context.
   Global GPIO state is checked. */
static inline GPIO_STATUS_T GPIOEnter(KRN_IPL_T *oldipl)
{
	*oldipl = KRN_raiseIPL();
	if(g_GPIOState != GPIO_STATE_INITIALISED)
	{
		KRN_restoreIPL(*oldipl);
		IMG_ASSERT(0);
		return GPIO_STATUS_ERR_UNINITIALISED;
	}
	return GPIO_STATUS_OK;
}

/* Synchronisation code for leaving a GPIO API function */
static inline IMG_VOID GPIOLeave(KRN_IPL_T oldipl)
{
	KRN_restoreIPL(oldipl);
}

/*!
*******************************************************************************

 @Function              @GPIOInit
							
*******************************************************************************/

GPIO_STATUS_T GPIOInit()
{
	img_uint32 i;
	KRN_IPL_T oldipl;

	/* Check and update the global GPIO state */
	oldipl = KRN_raiseIPL();
	if(g_GPIOState != GPIO_STATE_UNINITIALISED)
	{
		KRN_restoreIPL(oldipl);
		IMG_ASSERT(0);
		return GPIO_STATUS_ERR_INITIALISED;
	}
	g_GPIOState = GPIO_STATE_INITIALISING;
	KRN_restoreIPL(oldipl);

	/* Initialise each block */
	for(i=0; i<MAX_NUM_GPIO_BLOCKS; i++)
	{
		if(g_apsGPIOBlock[i] == IMG_NULL)
		{
			g_apsGPIOBlock[i] = &IMG_asGPIOBlock[i];
		}
		g_apsGPIOBlock[i]->pvAPIContext = &g_GPIOBlocks[i];

		DQ_init(&g_GPIOBlocks[i].InterruptPins);
		DQ_init(&g_GPIOBlocks[i].NonInterruptPins);
		g_GPIOBlocks[i].ui32ActiveMask = 0;
		QIO_init(&g_GPIOBlocks[i].sDevice,"GPIO Device",i,&GPIO_Driver);
		QIO_enable(&g_GPIOBlocks[i].sDevice);
	}

	/* Update global GPIO state */
	g_GPIOState = GPIO_STATE_INITIALISED;
	
	return GPIO_STATUS_OK;
}

/*!
*******************************************************************************

 @Function              @_GPIOCheckSettings

*******************************************************************************/

static GPIO_STATUS_T _GPIOCheckSettings(GPIO_PIN_SETTINGS_T *psSettings)
{
	/* Check settings structure */
	if(psSettings->Direction == GPIO_DIR_INPUT)
	{
		if(psSettings->Input.Schmitt >= (1 << GPIO_SCHMITT_T_BITS))
		{
			return GPIO_STATUS_INVALID_SCHMITT;
		}
		if(psSettings->Input.PullupPulldown >= (1 << GPIO_PULLUP_PULLDOWN_T_BITS))
		{
			return GPIO_STATUS_INVALID_PULLUP_PULLDOWN;
		}
	}
	else if(psSettings->Direction == GPIO_DIR_OUTPUT)
	{
		if(psSettings->Output.Level >= (1 << GPIO_LEVEL_T_BITS))
		{
			return GPIO_STATUS_INVALID_LEVEL;
		}
		if(psSettings->Output.DriveStrength >= (1 << GPIO_DRIVE_STRENGTH_T_BITS))
		{
			return GPIO_STATUS_INVALID_DRIVE_STRENGTH;
		}
		if(psSettings->Output.Slew >= (1 << GPIO_SLEW_T_BITS))
		{
			return GPIO_STATUS_INVALID_SLEW_RATE;
		}
	}
	else
	{
		return GPIO_STATUS_INVALID_DIRECTION;
	}
	
	return GPIO_STATUS_OK;
}

/*!
*******************************************************************************

 @Function              @_GPIOAddPin

*******************************************************************************/

static GPIO_STATUS_T _GPIOAddPin(GPIO_PIN_T *psPin, img_uint8 ui8BlockIndex, img_uint8 ui8PinNumber, GPIO_PIN_SETTINGS_T *psSettings)
{
	GPIO_BLOCK_T	*psBlock;
	IMG_UINT32		ui32PinMask;
	GPIO_STATUS_T	Ret;
	
	/* Check pin state and parameters */
	if(psPin->bActive == IMG_TRUE)
	{
		IMG_ASSERT(0);
		return GPIO_STATUS_ERR_ACTIVE;
	}
	if(ui8BlockIndex >= MAX_NUM_GPIO_BLOCKS)
	{
		IMG_ASSERT(0);
		return GPIO_STATUS_INVALID_BLOCK;
	}
	if(ui8PinNumber >= 32)
	{
		IMG_ASSERT(0);
		return GPIO_STATUS_INVALID_PIN;
	}

	Ret = _GPIOCheckSettings(psSettings);
	if(Ret != GPIO_STATUS_OK)
	{
		IMG_ASSERT(0);
		return Ret;
	}

	ui32PinMask = 1<<ui8PinNumber;
	psBlock = &g_GPIOBlocks[ui8BlockIndex];
	
	/* Check the pin does not already exist in the active pin mask for the block */
	if(psBlock->ui32ActiveMask & ui32PinMask)
	{
		IMG_ASSERT(0);
		return GPIO_STATUS_ERR_REPEAT;
	}
	
	/* Add the pin to the active mask for the block */
	psBlock->ui32ActiveMask |= ui32PinMask;
	
	/* Set the pin structure variables to initial values */
	psPin->ui8BlockIndex = ui8BlockIndex;
	psPin->ui8PinNumber = ui8PinNumber;
	psPin->Direction = psSettings->Direction;
	psPin->bInterrupt = IMG_FALSE;
	psPin->bDualEdgeTransitionInterrupt = IMG_FALSE;
	psPin->pfnInterruptCallback = IMG_NULL;
	psPin->pInterruptCallbackContext = IMG_NULL;
	psPin->bActive = IMG_TRUE;

	/* Add the pin to the non-interrupt linked list for the block */
	DQ_addHead(&g_GPIOBlocks[ui8BlockIndex].NonInterruptPins,psPin);

	/* Update the GPIO registers */
	GPIODriverConfigure(psPin,psSettings,IMG_TRUE);

	return GPIO_STATUS_OK;
}

/*!
*******************************************************************************

 @Function              @GPIOAddPin

*******************************************************************************/

GPIO_STATUS_T GPIOAddPin(GPIO_PIN_T *psPin, img_uint8 ui8BlockIndex, img_uint8 ui8PinNumber, GPIO_PIN_SETTINGS_T *psSettings)
{
	GPIO_API(_GPIOAddPin(psPin,ui8BlockIndex,ui8PinNumber,psSettings));
}

/*!
*******************************************************************************

 @Function              @_GPIOConfigure

*******************************************************************************/

static GPIO_STATUS_T _GPIOConfigure(GPIO_PIN_T *psPin, GPIO_PIN_SETTINGS_T *psSettings)
{
	GPIO_STATUS_T Ret;
	
	/* Check pin state and parameters */
	if(psPin->bActive == IMG_FALSE)
	{
		IMG_ASSERT(0);
		return GPIO_STATUS_ERR_INACTIVE;
	}

	Ret = _GPIOCheckSettings(psSettings);

	if(Ret == GPIO_STATUS_OK)
	{
		/* Update the GPIO registers */
		GPIODriverConfigure(psPin,psSettings,IMG_FALSE);
	}
	else
	{
		IMG_ASSERT(0);
	}
	
	return Ret;
}

/*!
*******************************************************************************

 @Function              @GPIOConfigure
                                
*******************************************************************************/

GPIO_STATUS_T GPIOConfigure(GPIO_PIN_T *psPin, GPIO_PIN_SETTINGS_T *psSettings)
{
	GPIO_API(_GPIOConfigure(psPin,psSettings));
}

/*!
*******************************************************************************

 @Function              @_GPIOSet

*******************************************************************************/

static GPIO_STATUS_T _GPIOSet(GPIO_PIN_T *psPin, GPIO_LEVEL_T Level)
{
	ioblock_sBlockDescriptor *psBlockDesc;
	img_uint8 ui8PinNumber;

	/* Check pin state and parameters */
	if(psPin->bActive == IMG_FALSE)
	{
		IMG_ASSERT(0);
		return GPIO_STATUS_ERR_INACTIVE;
	}

	if(Level >= (1 << GPIO_LEVEL_T_BITS))
	{
		IMG_ASSERT(0);
		return GPIO_STATUS_INVALID_LEVEL;
	}

	ui8PinNumber = psPin->ui8PinNumber;
	psBlockDesc = g_apsGPIOBlock[psPin->ui8BlockIndex];

	/* Update the GPIO registers */
	GPIODriverSet(psBlockDesc->ui32Base,ui8PinNumber,Level);

	return GPIO_STATUS_OK;
}

/*!
*******************************************************************************

 @Function              @GPIOSet

*******************************************************************************/

GPIO_STATUS_T GPIOSet(GPIO_PIN_T *psPin, GPIO_LEVEL_T Level)
{
	GPIO_API(_GPIOSet(psPin,Level));
}

/*!
*******************************************************************************

 @Function              @_GPIORead

*******************************************************************************/

static GPIO_STATUS_T _GPIORead(GPIO_PIN_T *psPin, GPIO_LEVEL_T *Value)
{
	ioblock_sBlockDescriptor *psBlockDesc;
	img_uint8 ui8PinNumber;

	/* Check pin state and parameters */
	if(psPin->bActive == IMG_FALSE)
	{
		IMG_ASSERT(0);
		return GPIO_STATUS_ERR_INACTIVE;
	}

	ui8PinNumber = psPin->ui8PinNumber;
	psBlockDesc = g_apsGPIOBlock[psPin->ui8BlockIndex];

	/* Read the value from the GPIO register */
	*Value = GPIODriverRead(psBlockDesc->ui32Base,ui8PinNumber);

	return GPIO_STATUS_OK;
}

/*!
*******************************************************************************

 @Function              @GPIORead

*******************************************************************************/

GPIO_STATUS_T GPIORead(GPIO_PIN_T *psPin, GPIO_LEVEL_T *Value)
{
	GPIO_API(_GPIORead(psPin,Value));
}

/*!
*******************************************************************************

 @Function              @_GPIOEnableInterrupt

*******************************************************************************/

static GPIO_STATUS_T _GPIOEnableInterrupt(GPIO_PIN_T *psPin, GPIO_INTERRUPT_TYPE_T InterruptType, GPIO_CALLBACK_T pfnInterruptCallback, IMG_PVOID pInterruptContext)
{
	/* Check pin state and parameters */
	if(psPin->bActive == IMG_FALSE)
	{
		IMG_ASSERT(0);
		return GPIO_STATUS_ERR_INACTIVE;
	}

	if(InterruptType >= (1 << GPIO_INTERRUPT_TYPE_T_BITS))
	{
		IMG_ASSERT(0);
		return GPIO_STATUS_INVALID_INTERRUPT_TYPE;
	}

	/* Update the GPIO registers */
	GPIODriverEnableInterrupt(psPin,InterruptType,pfnInterruptCallback,pInterruptContext);

	return GPIO_STATUS_OK;
}

/*!
*******************************************************************************

 @Function              @GPIOEnableInterrupt

*******************************************************************************/

GPIO_STATUS_T GPIOEnableInterrupt(GPIO_PIN_T *psPin, GPIO_INTERRUPT_TYPE_T InterruptType, GPIO_CALLBACK_T pfnInterruptCallback, IMG_PVOID pInterruptContext)
{
	GPIO_API(_GPIOEnableInterrupt(psPin,InterruptType,pfnInterruptCallback,pInterruptContext));
}

/*!
*******************************************************************************

 @Function              @_GPIODisableInterrupt

*******************************************************************************/

static GPIO_STATUS_T _GPIODisableInterrupt(GPIO_PIN_T *psPin)
{
	/* Check pin state and parameters */
	if(psPin->bActive == IMG_FALSE)
	{
		IMG_ASSERT(0);
		return GPIO_STATUS_ERR_INACTIVE;
	}

	if(!psPin->bInterrupt)
	{
		return GPIO_STATUS_OK;
	}

	/* Update the GPIO registers */
	GPIODriverDisableInterrupt(psPin);

	return GPIO_STATUS_OK;
}

/*!
*******************************************************************************

 @Function              @GPIODisableInterrupt

*******************************************************************************/

GPIO_STATUS_T GPIODisableInterrupt(GPIO_PIN_T *psPin)
{
	GPIO_API(_GPIODisableInterrupt(psPin));
}

/*!
*******************************************************************************

 @Function              @_GPIORemovePin

*******************************************************************************/

static GPIO_STATUS_T _GPIORemovePin(GPIO_PIN_T *psPin)
{
	/* Check pin state and parameters */
	if(psPin->bActive == IMG_FALSE)
	{
		IMG_ASSERT(0);
		return GPIO_STATUS_ERR_INACTIVE;
	}

	/* Update the GPIO registers */
	GPIODriverRemovePin(psPin);
	
	return GPIO_STATUS_OK;
}

/*!
*******************************************************************************

 @Function              @GPIORemovePin

*******************************************************************************/

GPIO_STATUS_T GPIORemovePin(GPIO_PIN_T *pin)
{
	GPIO_API(_GPIORemovePin(pin));
}

/*!
*******************************************************************************

 @Function              @_GPIODeinit

*******************************************************************************/

static inline GPIO_STATUS_T _GPIODeinit()
{
	img_uint32 i;
	KRN_IPL_T oldipl;
	GPIO_STATUS_T Ret = GPIO_STATUS_OK;

	/* Check there are no active pins for any of the blocks */
	oldipl = KRN_raiseIPL();
	for(i=0; i<MAX_NUM_GPIO_BLOCKS; i++)
	{
		if(g_GPIOBlocks[i].ui32ActiveMask)
		{
			Ret = GPIO_STATUS_ERR_ACTIVEPINS;
			break;
		}
	}
	KRN_restoreIPL(oldipl);

	if(Ret != GPIO_STATUS_OK)
	{
		IMG_ASSERT(0);
		return Ret;
	}

	/* Deinitialise the QIO devices */
	for(i=0; i<MAX_NUM_GPIO_BLOCKS; i++)
	{
		QIO_disable(&g_GPIOBlocks[i].sDevice);
		QIO_unload(&g_GPIOBlocks[i].sDevice);
	}

	return GPIO_STATUS_OK;
}

/*!
*******************************************************************************

 @Function              @GPIODeinit

*******************************************************************************/

GPIO_STATUS_T GPIODeinit()
{
	KRN_IPL_T oldipl;
	GPIO_STATUS_T Ret;

	/* Check and update the global GPIO state */
	oldipl = KRN_raiseIPL();
	if(g_GPIOState != GPIO_STATE_INITIALISED)
	{
		KRN_restoreIPL(oldipl);
		IMG_ASSERT(0);
		return GPIO_STATUS_ERR_UNINITIALISED;
	}
	g_GPIOState = GPIO_STATE_DEINITIALISING;
	KRN_restoreIPL(oldipl);

	/* Deinitialise */
	Ret = _GPIODeinit();

	/* Update the global GPIO state if deinitialisation successful */
	if(Ret == GPIO_STATUS_OK)
	{
		g_GPIOState = GPIO_STATE_UNINITIALISED;
	}
	else
	{
		g_GPIOState = GPIO_STATE_INITIALISED;
	}

	return Ret;
}
