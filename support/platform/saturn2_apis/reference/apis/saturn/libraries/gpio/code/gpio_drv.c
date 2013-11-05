/*!
*******************************************************************************
  file   gpio_drv.c

  brief  General Purpose Input/Output Device Driver 

         This file defines the functions that make up the General
		 Purpose Input/Output (GPIO) device driver.

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


/*===========================================================================*/
/*                              INCLUDE FILES                                */
/*===========================================================================*/


/* Include these before any other include to ensure TBI used correctly */
/* All METAG and METAC values from machine.inc and then tbi. */

#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include <assert.h>

/* MeOS Library */
#include <MeOS.h>

/* System */
#include <ioblock_defs.h>
#include <ioblock_utils.h>
#include <system.h>
#include <sys_util.h>

/* GPIO Driver */
#include <gpio_api.h>
#include "gpio_drv.h"
#include "gpio_reg_defs.h"



/*===========================================================================*/
/*                            MACRO DEFINITIONS                              */
/*===========================================================================*/

#define GPIO_WRITE(a,d)    (*(volatile img_uint32 *)(a) = (d))
#define GPIO_READ(a)       (*(volatile img_uint32 *)(a))

#define	GPIO_ONES(NumBits)														\
	((1 << (NumBits)) - 1)

/* Use for core registers with separate bit for every GPIO line with correct 
   block offsets and Value is compile time constant */
#define	GPIO_WRITE_BIT_CONST(Reg,Value,Mask,MaskClear)							\
	(Reg) = (((Value) == 0) ? ((Reg) & (MaskClear)) : ((Reg) | (Mask)))

/* Use for core registers with separate bit for every GPIO line with correct 
   block offsets and Value is compile time constant */
#define	GPIO_MODIFY_REG_BIT_CONST(Reg,Base,RegName,Value,Mask,MaskClear)		\
	(Reg) = READ_REG((Base),RegName);											\
	GPIO_WRITE_BIT_CONST((Reg),(Value),(Mask),(MaskClear));						\
	WRITE_REG((Base),RegName,(Reg))

/* Use for other registers with shared bit(s) for GPIO lines / incorrect 
   block offsets */
#define	GPIO_MODIFY_REG_BITS(Reg,Base,RegName,Offset,Value,Shift,NumBits)		\
	(Reg) = GPIO_READ((Base) + RegName##_OFFSET + (Offset));					\
	(Reg) = ((Reg) & (~(GPIO_ONES(NumBits) << (Shift)))) | ((Value) << (Shift));		\
	WRITE_REG((Base),RegName,(Reg))


/*===========================================================================*/
/*                             DATA STRUCTURES                               */
/*===========================================================================*/

/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*===========================================================================*/

static img_int32  GPIODriverInit(QIO_DEVICE_T *dev, QIO_DEVPOWER_T *pwrClass, img_int32 *devRank, unsigned intMasks[QIO_MAXVECGROUPS]);
static void GPIODriverStart(QIO_DEVICE_T *dev, QIO_IOPARS_T *ioPars);
static void GPIODriverISR(QIO_DEVICE_T *dev);


/*===========================================================================*/
/*                                GLOBAL DATA                                */
/*===========================================================================*/

/* The driver object */
const QIO_DRIVER_T GPIO_Driver =
{
    GPIODriverISR,		/* ISR                       */
    GPIODriverInit,		/* init function             */
    GPIODriverStart,	/* start function            */
    IMG_NULL,			/* no cancel function        */
    IMG_NULL,			/* no power control function */
    IMG_NULL,			/* no sim start function     */
    IMG_NULL   			/* no shut function          */
};

/* Global block descripter pointers */
ioblock_sBlockDescriptor *g_apsGPIOBlock[MAX_NUM_GPIO_BLOCKS] =
{
	IMG_NULL, IMG_NULL, IMG_NULL
};

/* Global block data structures */
GPIO_BLOCK_T g_GPIOBlocks[MAX_NUM_GPIO_BLOCKS];

/*===========================================================================*/
/*                             STATIC FUNCTIONS                              */
/*===========================================================================*/


static IMG_INT32 GPIODriverInit(QIO_DEVICE_T *psDevice, QIO_DEVPOWER_T *pePowerClass, img_int32 *pui32DeviceRank, img_uint32 aui32InterruptMasks[QIO_MAXVECGROUPS])
{
	img_uint32					ui32BlockIndex;
	img_uint32					ui32Reg;
    img_int32					lockState;
	ioblock_sBlockDescriptor	*psBlockDesc;
	
	ui32BlockIndex = psDevice->id;
	psBlockDesc = g_apsGPIOBlock[ui32BlockIndex];

    /* Provide information about the device */
    *pui32DeviceRank = 1;
    *pePowerClass = QIO_POWERNONE;

	/* Calculate interrupt masks */
	IOBLOCK_CalculateInterruptInformation(psBlockDesc);

	IMG_MEMCPY(aui32InterruptMasks, psBlockDesc->ui32IntMasks, sizeof(img_uint32)*QIO_MAXVECGROUPS);

	/* Read-modify-write to configure level sensitive interrupts */
	TBI_LOCK(lockState);

	ui32Reg = GPIO_READ(psBlockDesc->sDeviceISRInfo.ui32LEVELEXTAddress);
	if(psBlockDesc->eInterruptLevelType == HWLEVELEXT_LATCHED)
	{
		ui32Reg &= ~(psBlockDesc->sDeviceISRInfo.ui32LEVELEXTMask);
	}
	else if(psBlockDesc->eInterruptLevelType == HWLEVELEXT_NON_LATCHED)
	{
		ui32Reg |= (psBlockDesc->sDeviceISRInfo.ui32LEVELEXTMask);
	}
	GPIO_WRITE(psBlockDesc->sDeviceISRInfo.ui32LEVELEXTAddress, ui32Reg);

	TBI_UNLOCK(lockState);

    return 0;
}


static void GPIODriverStart(QIO_DEVICE_T *psDevice, QIO_IOPARS_T *psIOParams)
{
	/* No QIO operations */
	IMG_ASSERT(0);
}



static void GPIODriverISR(QIO_DEVICE_T *psDevice)
{
	ioblock_sBlockDescriptor *psBlockDesc;
	GPIO_BLOCK_T *psBlock;
	GPIO_PIN_T *psPin;
	IMG_UINT32 ui32Reg;
	IMG_UINT32 ui32RegB;
	IMG_UINT32 ui32PinMask;
	IMG_UINT32 ui32PinMaskClear;
	IMG_UINT32 ui32Base;
	IMG_UINT32 ui32ISR;
	IMG_UINT32 ui32IMR;
	GPIO_LEVEL_T IntLevel;


	psBlockDesc = g_apsGPIOBlock[psDevice->id];
	if(psBlockDesc == IMG_NULL)
	{
		IMG_ASSERT(0);
		return;
	}
	ui32Base = psBlockDesc->ui32Base;
	psBlock = (GPIO_BLOCK_T *)psBlockDesc->pvAPIContext;

	ui32Reg = GPIO_READ(psBlockDesc->sDeviceISRInfo.ui32STATEXTAddress);
	if (!(ui32Reg & psBlockDesc->sDeviceISRInfo.ui32STATEXTMask))
	{
		/* Spurious interrupt? */
		IMG_ASSERT(0);
		return;
	}

	
	/* Keep checking the ISR and IMR until there are no active interrupts. This ensures the IRQ will not remain 
	   asserted, which would block further interrupts. Following the execution of a callback function everything 
	   is rechecked as most GPIO API functions can be called from the callback, ie anything could have changed. */
	while(IMG_TRUE)
	{
		ui32ISR = READ_REG(ui32Base,GPIO_REG_INTERRUPT_STATUS);
		ui32IMR = READ_REG(ui32Base,GPIO_REG_INTERRUPT_ENABLE);

		ui32ISR &= ui32IMR;
		if(!ui32ISR)
		{
			return;
		}

		/* Traverse the interrupt linked list for the block looking for one of the pin(s) causing the interrupt */
		psPin = (GPIO_PIN_T *)DQ_first(&psBlock->InterruptPins);
		if(psPin == IMG_NULL)
		{
			/* Any pins that can cause interrupts should be in the interrupt linked list for the block */
			IMG_ASSERT(0);
			return;
		}
	
		while(IMG_TRUE)
		{
			ui32PinMask = 1 << psPin->ui8PinNumber;

			/* Interrupt for this pin? */
			if(ui32ISR & ui32PinMask)
			{
				ui32PinMaskClear = ~ui32PinMask;
				IntLevel = psPin->InterruptLevel;

				/* Dual edge interrupt type? */
				if(psPin->bDualEdgeTransitionInterrupt)
				{
					/* Update the level detection. If the pin has already changed to that level it will be detected in a future iteration of the ISR loop */
					if(psPin->InterruptLevel == GPIO_LEVEL_LOW)
					{
						GPIO_MODIFY_REG_BIT_CONST(ui32RegB,ui32Base,GPIO_REG_INTERRUPT_POLARITY,GPIO_I_INT_POL_HIGH_RISING,ui32PinMask,ui32PinMaskClear);
						psPin->InterruptLevel = GPIO_LEVEL_HIGH;
					}
					else
					{
						GPIO_MODIFY_REG_BIT_CONST(ui32RegB,ui32Base,GPIO_REG_INTERRUPT_POLARITY,GPIO_I_INT_POL_LOW_FALLING,ui32PinMask,ui32PinMaskClear);
						psPin->InterruptLevel = GPIO_LEVEL_LOW;
					}
				}

				/* Read-modify-write to clear the interrupt. Ideally there should be an interrupt clear register, but there is not, so this
				   is the best available solution. There is a chance an interrupt could occur during the read-modify-write and be
				   erroneously cleared. */
				GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_INTERRUPT_STATUS,GPIO_I_INT_CLEAR,ui32PinMask,ui32PinMaskClear);

				/* Callback function? */
				if(psPin->pfnInterruptCallback != IMG_NULL)
				{
					psPin->pfnInterruptCallback(psPin->pInterruptCallbackContext,IntLevel);
				}

				/* Break to ISR loop */
				break;
			}
			else
			{
				psPin = (GPIO_PIN_T *)DQ_next(psPin);
			}

			/* No more interrupt pins? */
			if((void *)psPin == (void *)&psBlock->InterruptPins)
			{
				/* Any pins that can cause interrupts should be in the interrupt linked list for the block */
				IMG_ASSERT(0);
				return;
			}
		}
	}
}

IMG_VOID GPIODriverConfigure(GPIO_PIN_T *psPin, GPIO_PIN_SETTINGS_T *psSettings, img_bool bNewPin)
{
	img_uint32 ui32Reg;
	ioblock_sBlockDescriptor *psBlockDesc;
	img_uint8 ui8Shift;
	img_uint32 ui32Offset;
	img_uint8 ui8BlockIndex;
	img_uint8 ui8PinNumber;
	img_uint32 ui32PinMask;
	img_uint32 ui32PinMaskClear;
	img_uint32 ui32Base;
	
	ui8BlockIndex = psPin->ui8BlockIndex;
	ui8PinNumber = psPin->ui8PinNumber;
	ui32PinMask = (1 << ui8PinNumber);
	ui32PinMaskClear = ~ui32PinMask;
	psBlockDesc = g_apsGPIOBlock[ui8BlockIndex];
	ui32Base = psBlockDesc->ui32Base;
	
	if(psSettings->Direction == GPIO_DIR_INPUT)
	{
		/* Update schmitt and pullup/pulldown registers */
		ui8Shift = g_GPIOSchmittMapping[ui8BlockIndex][ui8PinNumber];
		GPIO_MODIFY_REG_BITS(ui32Reg,GPIO_REGS_BASE,GPIO_REG_SCHMITT,0,psSettings->Input.Schmitt,ui8Shift,GPIO_SCHMITT_T_BITS);

		ui32Offset = g_GPIOPullupPulldownMapping[ui8BlockIndex][ui8PinNumber][0] * 4;
		ui8Shift = g_GPIOPullupPulldownMapping[ui8BlockIndex][ui8PinNumber][1];
		GPIO_MODIFY_REG_BITS(ui32Reg,GPIO_REGS_BASE,GPIO_REG_PULLUP_PULLDOWN,ui32Offset,psSettings->Input.PullupPulldown,ui8Shift,GPIO_PULLUP_PULLDOWN_T_BITS);
		
		/* If the pin is not already an input pin, ensure interrupts for the pin are disabled and update the pin context variables and direction register */
		if((psPin->Direction == GPIO_DIR_OUTPUT) || (bNewPin))
		{
			GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_INTERRUPT_ENABLE,GPIO_I_INT_DISABLE,ui32PinMask,ui32PinMaskClear);
			psPin->Direction = GPIO_DIR_INPUT;
			GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_DIR,GPIO_DIR_INPUT,ui32PinMask,ui32PinMaskClear);
		}
	}
	else
	{
		/* Update pin output, slew rate and drive strength registers */
		GPIO_MODIFY_REG_BITS(ui32Reg,psBlockDesc->ui32Base,GPIO_REG_DOUT,0,psSettings->Output.Level,ui8PinNumber,GPIO_LEVEL_T_BITS);

		ui8Shift = g_GPIOSlewRateMapping[ui8BlockIndex][ui8PinNumber];
		GPIO_MODIFY_REG_BITS(ui32Reg,GPIO_REGS_BASE,GPIO_REG_SLEW_RATE,0,psSettings->Output.Slew,ui8Shift,GPIO_SLEW_T_BITS);
		
		ui8Shift = g_GPIODriveStrengthMapping[ui8BlockIndex][ui8PinNumber];
		GPIO_MODIFY_REG_BITS(ui32Reg,GPIO_REGS_BASE,GPIO_REG_DRIVE_STRENGTH,0,psSettings->Output.DriveStrength,ui8Shift,GPIO_DRIVE_STRENGTH_T_BITS);

		/* If the pin is not already an output pin, disable interrupts for the pin and update the pin context variables and direction register */
		if((psPin->Direction == GPIO_DIR_INPUT) || (bNewPin))
		{
			GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_INTERRUPT_ENABLE,GPIO_I_INT_DISABLE,ui32PinMask,ui32PinMaskClear);

			/* If the pin is currently an input interrupting pin, move it to the non-interrupt linked list for the block and update the pin context variables */
			if(psPin->bInterrupt)
			{
				DQ_remove(psPin);
				DQ_addHead(&g_GPIOBlocks[ui8BlockIndex].NonInterruptPins,psPin);
				psPin->bInterrupt = IMG_FALSE;
				psPin->bDualEdgeTransitionInterrupt = IMG_FALSE;
				psPin->pfnInterruptCallback = IMG_NULL;
				psPin->pInterruptCallbackContext = IMG_NULL;
			}
			psPin->Direction = GPIO_DIR_OUTPUT;
			GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_DIR,GPIO_DIR_OUTPUT,ui32PinMask,ui32PinMaskClear);
		}
	}

	/* If the pin is a new pin, update the bit enable and select registers */
	if(bNewPin)
	{
		GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_BIT_ENABLE,GPIO_I_BIT_EN_ENABLE,ui32PinMask,ui32PinMaskClear);

		GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_SELECT,GPIO_I_FUNC_SET_TO_GPIO,ui32PinMask,ui32PinMaskClear);
	}

}

IMG_VOID GPIODriverSet(IMG_UINT32 ui32Base, IMG_UINT8 ui8PinNumber, GPIO_LEVEL_T Level)
{
	IMG_UINT32	ui32Reg;

	GPIO_MODIFY_REG_BITS(ui32Reg,ui32Base,GPIO_REG_DOUT,0,Level,ui8PinNumber,GPIO_LEVEL_T_BITS);
}

GPIO_LEVEL_T GPIODriverRead(IMG_UINT32 ui32Base, IMG_UINT8 ui8PinNumber)
{
	IMG_UINT32 ui32Reg;

	ui32Reg = READ_REG(ui32Base, GPIO_REG_DIN);
	return (GPIO_LEVEL_T)((ui32Reg >> ui8PinNumber) & GPIO_ONES(GPIO_LEVEL_T_BITS));
}

IMG_VOID GPIODriverEnableInterrupt(GPIO_PIN_T *psPin, GPIO_INTERRUPT_TYPE_T InterruptType, GPIO_CALLBACK_T pfnInterruptCallback, IMG_PVOID pInterruptContext)
{
	img_uint32 ui32Reg;
	img_uint32 ui32RegB;
	img_uint32 ui32RegC;
	ioblock_sBlockDescriptor *psBlockDesc;
	img_uint8 ui8BlockIndex;
	img_uint8 ui8PinNumber;
	img_uint32 ui32PinMask;
	img_uint32 ui32PinMaskClear;
	img_uint32 ui32Base;
	GPIO_LEVEL_T Value;

	ui8BlockIndex = psPin->ui8BlockIndex;
	ui8PinNumber = psPin->ui8PinNumber;
	ui32PinMask = (1 << ui8PinNumber);
	ui32PinMaskClear = ~ui32PinMask;
	psBlockDesc = g_apsGPIOBlock[ui8BlockIndex];
	ui32Base = psBlockDesc->ui32Base;

	/* Update the callback and context for all interrupt types */
	psPin->pfnInterruptCallback = pfnInterruptCallback;
	psPin->pInterruptCallbackContext = pInterruptContext;

	ui32Reg = READ_REG(ui32Base, GPIO_REG_INTERRUPT_TYPE);
	ui32RegB = READ_REG(ui32Base, GPIO_REG_INTERRUPT_POLARITY);

	switch(InterruptType)
	{
	/* Update the level and polarity bits and set the pin context variables to indicate the level
	   that will cause the interrupt, and if the interrupt is dual edge */
	case GPIO_INT_TYPE_LEVEL_LOW:
		GPIO_WRITE_BIT_CONST(ui32Reg,GPIO_I_INT_TYPE_LEVEL,ui32PinMask,ui32PinMaskClear);
		GPIO_WRITE_BIT_CONST(ui32RegB,GPIO_I_INT_POL_LOW_FALLING,ui32PinMask,ui32PinMaskClear);
		psPin->InterruptLevel = GPIO_LEVEL_LOW;
		psPin->bDualEdgeTransitionInterrupt = IMG_FALSE;
		break;

	case GPIO_INT_TYPE_LEVEL_HIGH:
		GPIO_WRITE_BIT_CONST(ui32Reg,GPIO_I_INT_TYPE_LEVEL,ui32PinMask,ui32PinMaskClear);
		GPIO_WRITE_BIT_CONST(ui32RegB,GPIO_I_INT_POL_HIGH_RISING,ui32PinMask,ui32PinMaskClear);
		psPin->InterruptLevel = GPIO_LEVEL_HIGH;
		psPin->bDualEdgeTransitionInterrupt = IMG_FALSE;
		break;

	case GPIO_INT_TYPE_EDGE_FALL:
		GPIO_WRITE_BIT_CONST(ui32Reg,GPIO_I_INT_TYPE_EDGE,ui32PinMask,ui32PinMaskClear);
		GPIO_WRITE_BIT_CONST(ui32RegB,GPIO_I_INT_POL_LOW_FALLING,ui32PinMask,ui32PinMaskClear);
		psPin->InterruptLevel = GPIO_LEVEL_LOW;
		psPin->bDualEdgeTransitionInterrupt = IMG_FALSE;
		break;

	case GPIO_INT_TYPE_EDGE_RISE:
		GPIO_WRITE_BIT_CONST(ui32Reg,GPIO_I_INT_TYPE_EDGE,ui32PinMask,ui32PinMaskClear);
		GPIO_WRITE_BIT_CONST(ui32RegB,GPIO_I_INT_POL_HIGH_RISING,ui32PinMask,ui32PinMaskClear);
		psPin->InterruptLevel = GPIO_LEVEL_HIGH;
		psPin->bDualEdgeTransitionInterrupt = IMG_FALSE;
		break;

	/* For a dual edge interrupt, read the current level first then use the opposite polarity for
	   the trigger */
	case GPIO_INT_TYPE_EDGE_TRANSITION:
		psPin->bDualEdgeTransitionInterrupt = IMG_TRUE;
		GPIO_WRITE_BIT_CONST(ui32Reg,GPIO_I_INT_TYPE_LEVEL,ui32PinMask,ui32PinMaskClear);
		ui32RegC = READ_REG(ui32Base, GPIO_REG_DIN);
		Value = (GPIO_LEVEL_T)((ui32RegC >> ui8PinNumber) & GPIO_ONES(GPIO_LEVEL_T_BITS));
		if(Value == GPIO_LEVEL_LOW)
		{
			GPIO_WRITE_BIT_CONST(ui32RegB,GPIO_I_INT_POL_HIGH_RISING,ui32PinMask,ui32PinMaskClear);
			psPin->InterruptLevel = GPIO_LEVEL_HIGH;
		}
		else
		{
			GPIO_WRITE_BIT_CONST(ui32RegB,GPIO_I_INT_POL_LOW_FALLING,ui32PinMask,ui32PinMaskClear);
			psPin->InterruptLevel = GPIO_LEVEL_LOW;
		}
		break;
	}
	WRITE_REG(ui32Base, GPIO_REG_INTERRUPT_TYPE, ui32Reg);
	WRITE_REG(ui32Base, GPIO_REG_INTERRUPT_POLARITY, ui32RegB);

	/* Disable and clear the interrupt to remove an erroneous interrupt */
	GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_INTERRUPT_ENABLE,GPIO_I_INT_DISABLE,ui32PinMask,ui32PinMaskClear);
	GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_INTERRUPT_STATUS,GPIO_I_INT_CLEAR,ui32PinMask,ui32PinMaskClear);

	/* Move the pin to the interrupt pins linked list for the block */
	DQ_remove(psPin);
	DQ_addHead(&g_GPIOBlocks[ui8BlockIndex].InterruptPins,psPin);

	psPin->bInterrupt = IMG_TRUE;

	/* Enable the interrupt */
	GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_INTERRUPT_ENABLE,GPIO_I_INT_ENABLE,ui32PinMask,ui32PinMaskClear);
}

IMG_VOID GPIODriverDisableInterrupt(GPIO_PIN_T *psPin)
{
	img_uint32 ui32Reg;
	ioblock_sBlockDescriptor *psBlockDesc;
	img_uint8 ui8BlockIndex;
	img_uint8 ui8PinNumber;
	img_uint32 ui32PinMask;
	img_uint32 ui32PinMaskClear;
	img_uint32 ui32Base;

	ui8BlockIndex = psPin->ui8BlockIndex;
	ui8PinNumber = psPin->ui8PinNumber;
	ui32PinMask = (1 << ui8PinNumber);
	ui32PinMaskClear = ~ui32PinMask;
	psBlockDesc = g_apsGPIOBlock[ui8BlockIndex];
	ui32Base = psBlockDesc->ui32Base;

	/* Disable the interrupt */
	GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_INTERRUPT_ENABLE,GPIO_I_INT_DISABLE,ui32PinMask,ui32PinMaskClear);

	/* Move the pin to the non-interrupt linked list for the block */
	DQ_remove(psPin);
	DQ_addHead(&g_GPIOBlocks[ui8BlockIndex].NonInterruptPins,psPin);
	
	/* Update the pin context variables */
	psPin->bInterrupt = IMG_FALSE;
	psPin->bDualEdgeTransitionInterrupt = IMG_FALSE;
	psPin->pfnInterruptCallback = IMG_NULL;
	psPin->pInterruptCallbackContext = IMG_NULL;
}

IMG_VOID GPIODriverRemovePin(GPIO_PIN_T *psPin)
{
	ioblock_sBlockDescriptor *psBlockDesc;
	img_uint8 ui8BlockIndex;
	img_uint8 ui8PinNumber;
	img_uint32 ui32Reg;
	img_uint32 ui32PinMask;
	img_uint32 ui32PinMaskClear;
	img_uint32 ui32Base;
	img_uint8 ui8Shift;
	img_uint32 ui32Offset;
	GPIO_BLOCK_T *psBlock;

	ui8BlockIndex = psPin->ui8BlockIndex;
	ui8PinNumber = psPin->ui8PinNumber;
	ui32PinMask = (1 << ui8PinNumber);
	ui32PinMaskClear = ~ui32PinMask;
	psBlockDesc = g_apsGPIOBlock[ui8BlockIndex];
	ui32Base = psBlockDesc->ui32Base;
	psBlock = &g_GPIOBlocks[ui8BlockIndex];

	/* Remove the pin for the linked list it is in */
	DQ_remove(psPin);
	
	/* Update the block active mask and pin state */
	psBlock->ui32ActiveMask &= ui32PinMaskClear;
	psPin->bActive = IMG_FALSE;
	
	/* Restore the default settings for the pin for all GPIO registers */
	GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_INTERRUPT_ENABLE,GPIO_I_INT_DISABLE,ui32PinMask,ui32PinMaskClear);

	GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_INTERRUPT_TYPE,GPIO_I_INT_TYPE_LEVEL,ui32PinMask,ui32PinMaskClear);
	GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_INTERRUPT_POLARITY,GPIO_I_INT_POL_LOW_FALLING,ui32PinMask,ui32PinMaskClear);
	
	GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_SELECT,GPIO_I_FUNC_SET_TO_PRIMARY,ui32PinMask,ui32PinMaskClear);
	GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_BIT_ENABLE,GPIO_I_BIT_EN_DISABLE,ui32PinMask,ui32PinMaskClear);

	GPIO_MODIFY_REG_BIT_CONST(ui32Reg,ui32Base,GPIO_REG_DIR,GPIO_DIR_INPUT,ui32PinMask,ui32PinMaskClear);

	ui8Shift = g_GPIOSchmittMapping[ui8BlockIndex][ui8PinNumber];
	GPIO_MODIFY_REG_BITS(ui32Reg,GPIO_REGS_BASE,GPIO_REG_SCHMITT,0,GPIO_SCHMITT_OFF,ui8Shift,GPIO_SCHMITT_T_BITS);

	ui32Offset = g_GPIOPullupPulldownMapping[ui8BlockIndex][ui8PinNumber][0] * 4;
	ui8Shift = g_GPIOPullupPulldownMapping[ui8BlockIndex][ui8PinNumber][1];
	GPIO_MODIFY_REG_BITS(ui32Reg,GPIO_REGS_BASE,GPIO_REG_PULLUP_PULLDOWN,ui32Offset,GPIO_PU_PD_OFF,ui8Shift,GPIO_PULLUP_PULLDOWN_T_BITS);

	ui8Shift = g_GPIOSlewRateMapping[ui8BlockIndex][ui8PinNumber];
	GPIO_MODIFY_REG_BITS(ui32Reg,GPIO_REGS_BASE,GPIO_REG_SLEW_RATE,0,GPIO_SLEW_SLOW,ui8Shift,GPIO_SLEW_T_BITS);

	ui8Shift = g_GPIODriveStrengthMapping[ui8BlockIndex][ui8PinNumber];
	GPIO_MODIFY_REG_BITS(ui32Reg,GPIO_REGS_BASE,GPIO_REG_DRIVE_STRENGTH,0,GPIO_DRIVE_2MA,ui8Shift,GPIO_DRIVE_STRENGTH_T_BITS);
}

/* Mappings for the registers that don't follow the standard offsets for the bits */

/* 1 Schmitt register at a fixed offset from the GPIO register base, many pins share schmitt settings. 
   The number is the bit offset in the register for the given [BlockIndex][PinNumber]. 99 for pins that dont exist */
const IMG_UINT8 g_GPIOSchmittMapping[3][32] = 
{
/*  {0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31} */
	{7 ,7 ,7 ,7 ,7 ,7 ,3 ,3 ,3 ,3 ,3 ,3 ,5 ,5 ,5 ,5 ,5 ,5 ,2 ,2 ,4 ,4 ,9 ,9 ,1 ,1 ,99,99,99,99,99,99},
	{2 ,2 ,12,12,12,12,12,12,12,12,12,12,12,12,12,12,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,99,99,99,99,99,99},
	{0 ,0 ,0 ,0 ,0 ,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,99,99,99,99,99,99,99},
};

/* 6 Pullup/Pulldown registers at a fixed offset from the GPIO register base, each pin has a seperate pullup/pulldown. 
   The numbers are the block and bit offsets from the pullup/pulldown base for the given [BlockIndex][PinNumber] 
   ([0] is block offset, [1] bit offset). 99,99 for pins that dont exist */
const IMG_UINT8 g_GPIOPullupPulldownMapping[3][32][2] =
{
/*  {0      ,1      ,2      ,3      ,4      ,5      ,6      ,7      ,8      ,9      ,10     ,11     ,12     ,13     ,14     ,15     ,16     ,17     ,18     ,19     ,20     ,21     ,22     ,23     ,24     ,25     ,26     ,27     ,28     ,29     ,30     ,31     } */
	{{0 ,16},{0 ,18},{0 ,20},{0 ,22},{0 ,24},{0 ,26},{0 ,28},{0 ,30},{1 ,00},{1 ,2 },{1 ,4 },{1 ,6 },{1 ,8 },{1 ,10},{1 ,12},{1 ,14},{1 ,16},{1 ,18},{1 ,20},{1 ,22},{1 ,24},{1 ,26},{1 ,28},{1 ,30},{4 ,12},{4 ,14},{99,99},{99,99},{99,99},{99,99},{99,99},{99,99}},

	{{4 ,18},{4 ,20},{2 ,0 },{2 ,2 },{2 ,4 },{2 ,6 },{2 ,8 },{2 ,10},{2 ,12},{2 ,14},{2 ,16},{2 ,18},{2 ,20},{2 ,22},{2 ,24},{2 ,26},{4 ,24},{4 ,26},{4 ,28},{4 ,30},{5 ,0 },{5 ,6 },{5 ,8 },{5 ,10},{5 ,12},{5 ,14},{99,99},{99,99},{99,99},{99,99},{99,99},{99,99}},

	{{5 ,16},{5 ,18},{5 ,20},{5 ,2 },{5 ,4 },{2 ,28},{2 ,30},{3 ,0 },{3 ,2 },{3 ,4 },{3 ,6 },{3 ,8 },{3 ,10},{3 ,12},{3 ,14},{3 ,16},{3 ,18},{3 ,20},{3 ,22},{3 ,24},{3 ,26},{3 ,28},{3 ,30},{4 ,0 },{ 4,2 },{99,99},{99,99},{99,99},{99,99},{99,99},{99,99},{99,99}},

};

/* 1 Slew rate register at a fixed offset from the GPIO register base, many pins share slew rate settings. 
   The number is the bit offset in the register for the given [BlockIndex][PinNumber]. 99 for pins that dont exist */
const IMG_UINT8 g_GPIOSlewRateMapping[3][32]=
{
/*  {0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31} */
	{7 ,7 ,7 ,7 ,7 ,7 ,3 ,3 ,3 ,3 ,3 ,3 ,5 ,5 ,5 ,5 ,5 ,5 ,2 ,2 ,4 ,4 ,9 ,9 ,1 ,1 ,99,99,99,99,99,99},
	{2 ,2 ,12,12,12,12,12,12,12,12,12,12,12,12,12,12,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,99,99,99,99,99,99},
	{0 ,0 ,0 ,0 ,0 ,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,99,99,99,99,99,99,99},
};

/* 1 Drive strength register at a fixed offset from the GPIO register base, many pins share drive strength settings. 
   The number is the bit offset in the register for the given [BlockIndex][PinNumber]. 99 for pins that dont exist */
const IMG_UINT8 g_GPIODriveStrengthMapping[3][32]=
{
/*  {0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31} */
	{14,14,14,14,14,14,6 ,6 ,6 ,6 ,6 ,6 ,10,10,10,10,10,10,4 ,4 ,8 ,8 ,18,18,2 ,2 ,99,99,99,99,99,99},
	{4 ,4 ,24,24,24,24,24,24,24,24,24,24,24,24,24,24,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,99,99,99,99,99,99},
	{0 ,0 ,0 ,0 ,0 ,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,99,99,99,99,99,99,99},
};

