/*!
*******************************************************************************
 @file   sys_hwaddr.h

 @brief  DMAC device driver address mangling functionality

 @author Imagination Technologies

         <b>Copyright 2006 by Imagination Technologies Limited.</b>\n
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

#if !defined (__SYS_HWADDR__)
#define __SYS_HWADDR__


#include <img_defs.h>

/******************************************************************************
****************************** Macro Definitions ******************************
*******************************************************************************/
#define READ_REG(base, name)                                        \
(                                                                   \
    (*(volatile img_uint32 *)(base##_BASE + name##_OFFSET))         \
)                                                   

#define WRITE_REG(base, name, val)                                  \
(                                                                   \
    *(volatile img_uint32 *)(base##_BASE + name##_OFFSET) = (val)   \
)

#define READ_REG_FIELD(regval,field)								\
(																	\
	((field##_MASK) == 0xFFFFFFFF) ?								\
	(																\
		(regval)													\
	)																\
	:																\
	(																\
		((regval) & (field##_MASK)) >> (field##_SHIFT)				\
	)																\
)

#define WRITE_REG_FIELD(regval,field,val)							\
(																	\
	((field##_MASK) == 0xFFFFFFFF) ?								\
	(																\
		(##val)														\
	)																\
	:																\
	(																\
		(															\
			(##regval) & ~(field##_MASK)							\
		) |															\
		(															\
			((##val) << (field##_SHIFT)) & (field##_MASK)			\
		)															\
	)																\
)

/*!
*******************************************************************************

 This enumeration defines the Device Ids for the registered audio ports.

*******************************************************************************/
typedef enum
{
    /*! Serial Audio (I2S) Output */
    DEVICE_I2S_OUT_1 = 0,
    /*! Number of supported devices */
    DEVICE_NUM_IDS

} DEVICE_ID_E;

/******************************************************************************
****************************** End enumerations *******************************
*******************************************************************************/

/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/

/*! Number of elements in the Serial Audio Output DMAC linked-list */
#define I2S_OUT_DMA_LLIST_SIZE      (64)

/*! Number of I/O control blocks for the Serial Audio Output port
    (must be >= minimum elements to start DMAC) */
#define I2S_OUT_DMA_QUEUE_SIZE      (32)

/*! I2S output register block base address */
#define I2S_OUT_1_REG_BASE_ADDRESS			(SYSTEM_CONTROL_BASE_ADDRESS + I2S_BASE_OFFSET)

/******************************************************************************
************************** End constant definitions ***************************
*******************************************************************************/

/******************************************************************************
************************ External variable declaration ************************
*******************************************************************************/



/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

/*!
*******************************************************************************

 @Function              @SYS_getHardwareAddress

 <b>Description:</b>\n
 This function  is used to convert an address to its physical equivalent

 \param     hwAddress       Address in RAM.

 \return                    Physical address suitable for DMA register/linked-list\n

*******************************************************************************/
IMG_UINT32 SYS_getHardwareAddress(IMG_UINT32 ui32HWAddress);

/*!
*******************************************************************************

 @Function              @SYS_configureAudioDMAC

 <b>Description:</b>\n
 Sets up a DMAC device table, connect the peripheral to a channel, and then
 initialises the DMAC device

 \param     peripheral_id	    Peripheral to be connected to a DMAC channel
 \param     dma_channel		    Channel number to connect the peripheral to
 \param     table_element	    Element in the DMAC device table to configure

*******************************************************************************/
IMG_VOID SYS_configureAudioDMAC
(
	IMG_UINT32		ui32PeripheralID,
	IMG_UINT32		ui32DmaChannel,
	IMG_UINT32		ui32TableElement
);




/******************************************************************************
******************************** End Functions ********************************
*******************************************************************************/

#endif /* __SYS_HWADDR__ */
