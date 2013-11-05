/*!
*******************************************************************************
 @file   usbd_hal.h

 @brief  USBD Hardware Abstraction Layer header


 @author Imagination Technologies

         <b>Copyright 2007 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to the third parties without the
         express written permission of Imagination Technologies
         Limited, Home Park Estate, Kings Langley, Hertfordshire,
         WD4 8LZ, U.K.

 <b>Platform:</b>\n
         MobileTV

*******************************************************************************/
#if !defined(__USBD_HAL_H__)
#define __USBD_HAL_H__

#include <img_types.h>
#include <sys_util.h>

/*
******************************************************************************
								MTX/META BUILD 
******************************************************************************/

//#define __USBD_INLINE__	
#define __USBD_INLINE__		__inline__

/* This attribute is applied to long functions called multiple times */
#define __USBD_INLINE_OPTIONAL__
//#define __USBD_INLINE_OPTIONAL__	__USBD_INLINE__

/* Set attribute to allocate the data inside bulk memory */
//#define __USBD_ATTR__
#define __USBD_ATTR__ __attribute__ ((__section__ (".usbbuffers")))

/* 
USB Address Map in MobileTV:
										Synopsys Address	MobileTV Address
USB Normal Register Access Start		0x0000				
USB Normal Register Access End			0x7FFF				0x0480FFFF
USB Direct access to data fifo start 	0x20000				0x04804000
USB Direct access to data fifo end		0x23FFF				0x04807FFF
*/

#define usbd_InitialiseHAL()


#if !defined (LOG_REGS)
#define usb_ReadReg32(_base, _reg) (*(volatile IMG_UINT32 *)(_base + _reg))
#define usb_WriteReg32(_base, _reg, _value) (*(volatile IMG_UINT32 *)(_base + _reg) = (_value))
#else
#define usb_ReadReg32(_base, _reg)	SYS_LogRegRead( _base + _reg )
#define usb_WriteReg32(_base, _reg, _value)	SYS_LogRegWrite( _base + _reg, _value )
#endif


#define usb_ModifyReg32(_base, _reg, _clear_mask, _set_mask) usb_WriteReg32(_base, _reg,((usb_ReadReg32(_base, _reg) & ~_clear_mask) | _set_mask))

/* write the memory reference directly on an unified memory system */ 
#define usb_WriteMemrefReg32(_base, _reg, memref) (usb_WriteReg32(_base, _reg, memref))

#define usb_ReadCtrlReg32(_reg) (*(volatile IMG_UINT32 *)( g_sSysBlock.ui32BaseAddress + _reg))
#define usb_WriteCtrlReg32(_reg, _value) (*(volatile IMG_UINT32 *)( g_sSysBlock.ui32BaseAddress + _reg) = (_value))

/* Macros used to read/write register fields to integer variables  */
#define usbd_ReadRegField(regval,field)								\
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

#define usbd_WriteRegField(regval,field,val)						\
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



#define UDELAY(_usecs)
#define MDELAY(_msecs)

#define SPIN_LOCK(lock)
#define SPIN_UNLOCK(lock)
#define SPIN_LOCK_IRQSAVE( _l, _f )
#define SPIN_UNLOCK_IRQRESTORE( _l,_f )
#define SET_DEBUG_LEVEL(_new )

#define DWC_DEBUGPL(x, y...)
#define DWC_DEBUGP(x...)
#define DWC_WARN(x...)
#define DWC_PRINT(x...)

#define CHK_DEBUG_LEVEL(level) (0)
#define DWC_ERROR(...) IMG_ASSERT(0)
#define DWC_NOTICE(x)
#define WARN_ON(x)


#endif  //__USBD_HAL_H__

