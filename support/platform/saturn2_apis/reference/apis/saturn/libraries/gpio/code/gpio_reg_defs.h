/*!
*******************************************************************************
 @file   gpio_reg_defs.h

 @brief  General Purpose Input/Output Device Register Definitions

         This file contains the register offsets for the
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

#if !defined(GPIO_REG_DEFS_H)
#define GPIO_REG_DEFS_H








/* use READ_REG/WRITE_REG(psBlockDesc->ui32Base,Regnamewithout_OFFSET) */

//0x02015800
#define	GPIO_REGS_BASE							(SYSTEM_CONTROL_BASE_ADDRESS+GPIO_REGS_OFFSET)

#define	GPIO_REG_DIR_OFFSET						(0)

#define	GPIO_REG_SELECT_OFFSET					(0x10)

#define	GPIO_REG_INTERRUPT_POLARITY_OFFSET		(0x20)

#define	GPIO_REG_INTERRUPT_TYPE_OFFSET			(0x30)

#define	GPIO_REG_INTERRUPT_ENABLE_OFFSET		(0x40)

#define	GPIO_REG_INTERRUPT_STATUS_OFFSET		(0x50)

#define	GPIO_REG_BIT_ENABLE_OFFSET				(0x60)

#define	GPIO_REG_DIN_OFFSET						(0x70)

#define	GPIO_REG_DOUT_OFFSET					(0x80)


/* use GPIO_READ/WRITE_REG(SYSTEM_CONTROL_BASE_ADDRESS,Regnamewithout_OFFSET) */

#define	GPIO_REG_SCHMITT_OFFSET			(0x90)
extern const img_uint8 g_GPIOSchmittMapping[3][32];

#define	GPIO_REG_PULLUP_PULLDOWN_OFFSET	(0xA0)
extern const img_uint8 g_GPIOPullupPulldownMapping[3][32][2];

#define	GPIO_REG_SLEW_RATE_OFFSET			(0xC0)
extern const img_uint8 g_GPIOSlewRateMapping[3][32];

#define	GPIO_REG_DRIVE_STRENGTH_OFFSET		(0xD0)
extern const img_uint8 g_GPIODriveStrengthMapping[3][32];

#endif

