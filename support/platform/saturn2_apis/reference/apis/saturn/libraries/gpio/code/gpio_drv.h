/*!
*******************************************************************************
 @file   gpio_drv.h

 @brief  General Purpose Input/Output Device Driver

         This file contains the header file information for the
         General Purpose Input/Output (GPIO) device driver.

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

#if !defined (__GPIO_DRV_H__)
#define __GPIO_DRV_H__


typedef struct gpio_block_t
{
	QIO_DEVICE_T sDevice;
	DQ_T InterruptPins;
	DQ_T NonInterruptPins;
	img_uint32 ui32ActiveMask;
}GPIO_BLOCK_T;

typedef enum
{
    GPIO_I_BIT_EN_DISABLE = 0,
    GPIO_I_BIT_EN_ENABLE
} GPIO_I_BIT_ENABLE_T;

typedef enum
{
	GPIO_I_INT_TYPE_LEVEL = 0,
	GPIO_I_INT_TYPE_EDGE
}GPIO_I_INTERRUPT_TYPE_T;


typedef enum
{
	GPIO_I_INT_POL_LOW_FALLING = 0,
	GPIO_I_INT_POL_HIGH_RISING
}GPIO_I_INTERRUPT_POLARITY_T;

typedef enum
{
    GPIO_I_FUNC_SET_TO_PRIMARY = 0,
    GPIO_I_FUNC_SET_TO_GPIO
} GPIO_I_FUNCTION_T;

typedef enum
{
    GPIO_I_INT_DISABLE = 0,
    GPIO_I_INT_ENABLE
} GPIO_I_INTERRUPT_ENABLE_T;

typedef enum
{
    GPIO_I_INT_CLEAR = 0,
    GPIO_I_INT_NOCLEAR
} GPIO_I_INTERRUPT_CLEAR_T;


IMG_VOID		GPIODriverConfigure(GPIO_PIN_T *psPin, GPIO_PIN_SETTINGS_T *psSettings, img_bool bNewPin);
IMG_VOID		GPIODriverSet(IMG_UINT32 ui32Base, IMG_UINT8 ui8PinNumber, GPIO_LEVEL_T Level);
GPIO_LEVEL_T	GPIODriverRead(IMG_UINT32 ui32Base, IMG_UINT8 ui8PinNumber);
IMG_VOID		GPIODriverEnableInterrupt(GPIO_PIN_T *psPin, GPIO_INTERRUPT_TYPE_T InterruptType, GPIO_CALLBACK_T pfnInterruptCallback, IMG_PVOID pInterruptContext);
IMG_VOID		GPIODriverDisableInterrupt(GPIO_PIN_T *psPin);
IMG_VOID		GPIODriverRemovePin(GPIO_PIN_T *psPin);

/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/

#define MAX_NUM_GPIO_BLOCKS			(3)
/*! Block Descriptor pointer array */
extern ioblock_sBlockDescriptor	*g_apsGPIOBlock[MAX_NUM_GPIO_BLOCKS];
extern ioblock_sBlockDescriptor	IMG_asGPIOBlock[];

extern GPIO_BLOCK_T g_GPIOBlocks[MAX_NUM_GPIO_BLOCKS];


/******************************************************************************
**************************** Export Device Driver *****************************
*******************************************************************************/

/*! Device driver object. */
extern const QIO_DRIVER_T GPIO_Driver;

/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/



#endif /* __GPIO_DRV_H__ */
