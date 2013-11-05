/*!
*******************************************************************************
 @file   ioblock_defs.h

 @brief  Typedefs etc for the I/O driver "define" API functions


 @author Imagination Technologies

         <b>Copyright 2009 by Imagination Technologies Limited.</b>\n
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

#if !defined (__IOBLOCK_DEFS_API_H__)
#define __IOBLOCK_DEFS_API_H__

#include <img_defs.h>
#include <MeOS.h>

/******************************************************************************
******************************** Enumerations *********************************
*******************************************************************************/

/*!
	The ioblock_eInterruptLevelType enum lists the options for the HWLEVELEXT (and/or CR_PERIP_HWLEVELEXT) register
	fields.

	META/MTX interrupts can be "latched" or "non-latched" by the interrupt h/w.  The interrupt status bit for a
	"latched" interrupt will be persistent even if the associated interrupt line is deasserted and must be
	explicitly cleared by s/w.  The interrupt status bit for a "non-latched" interrupt will always reflect the
	state of the associated interrupt line and, therefore, does not need to be explicitly cleared by s/w - it will
	be cleared automatically when the interrupt processing acknowledges (and clears) the cause of the interrupt.

	The terms "edge-triggered" and "level-sensitive" are misleading and do not accurately reflect the behaviour
	of the interrupt h/w.  The h/w will ONLY detect an interrupt in all cases when the incoming interrupt signal
	changes state from deasserted to asserted regardless of how the interrupt is configured and whether the incoming
	signal state is persistent or not.

	QIO support for interrupt dispatching relies upon persistent status bits - the QIO dispatcher needs to be able
	to determine the source(s) of interrupts by interrogation of the status bits, and if the status bits are
	transitory then the dispatcher will not "see" them.

	For interrupting sources which continue to assert their interrupt signal until the interrupt processing s/w
	clears the cause of the interrupt the "non-latched" set-up is used so that explicit clearing of the status bits
	is unnecessary.

	For any interrupting sources which only pulse their interrupt signal (eg the 2D block) the
	"latched" set-up must be used so that QIO (or other dispatching s/w) can determine the interrupting source.
	The interrupt processing s/w for such a set-up must include the explicit clearing of the latched interrupt.
*/

typedef enum
{
	/*! Latched = "edge-triggered" type. */
	HWLEVELEXT_LATCHED = 0,

	/*! Non-latched = "level-sensitive" type. */
	HWLEVELEXT_NON_LATCHED = 1

} ioblock_eInterruptLevelType;

/******************************************************************************
****************************** End enumerations *******************************
*******************************************************************************/


/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/

/******************************************************************************
************************** End constant definitions ***************************
*******************************************************************************/


/******************************************************************************
****************************** Type definitions *******************************
*******************************************************************************/

/*!
	The caller-supplied function typedefs.
*/

typedef unsigned long ioblock_fn_ConvertVirtualToPhysical	( unsigned long VirtualAddress );
typedef unsigned long ioblock_fn_ConvertPhysicalToOffset	( unsigned long PhysicalAddress );
typedef unsigned long ioblock_fn_FlushCache					( unsigned long TransferBaseAddress, unsigned long TransferSize );
typedef IMG_RESULT ioblock_fn_DMACChannelBind				( IMG_UINT32 DMACChannelSelectValue, IMG_UINT32 DMACChannel );


/*!
	The ioblock_sInterruptRegister structure holds information about one of the various register
	sets that comprise the interrupt hardware of a META or MTX processor.
*/

typedef struct ioblock_sInterruptRegisterTag
{
    /*! Interrupt register block base address. */
    unsigned long		ui32Base;

    /*! Interrupt intra-block stride (in bits) */
    unsigned long		ui32IntraBlockStrideInBits;

    /*! Interrupt inter-block stride (in bytes) */
    unsigned long		ui32InterBlockStrideInBytes;

} ioblock_sInterruptRegister;


/*!
	The ioblock_sInterruptBlock structure holds information about the various register
	sets that comprise the interrupt hardware of a META or MTX processor.
*/

typedef struct ioblock_sInterruptBlockTag
{

    /*! Status register information. */
    ioblock_sInterruptRegister		sSTATEXT;

    /*! Level register information. */
    ioblock_sInterruptRegister		sLEVELEXT;

    /*! Vector register information. */
    ioblock_sInterruptRegister		sVECnEXT;

} ioblock_sInterruptBlock;


/*!
	The ioblock_sSystemDescriptor structure holds system-specific information.
*/

// MSH - this is a temporary alias to allow Ensigma code to build with the this
// later version of the file.
#define fn_getHardwareAddress			pfn_ConvertVirtualToPhysical

typedef struct ioblock_sSystemDescriptorTag
{
    /*! Pointer to caller's ConvertVirtualToPhysical function. */
	ioblock_fn_ConvertVirtualToPhysical *	pfn_ConvertVirtualToPhysical;

	/*! Pointer to caller's ConvertPhysicalToOffset function. */
	ioblock_fn_ConvertPhysicalToOffset *	pfn_ConvertPhysicalToOffset;

    /*! Pointer to caller's DMACChannelBind function. */
	ioblock_fn_DMACChannelBind *			pfn_DMACChannelBind;

    /*! Pointer to caller's FlushCache function. */
	ioblock_fn_FlushCache *					pfn_FlushCache;

} ioblock_sSystemDescriptor;


/*!
	The ioblock_sDMACChannelDescriptor structure holds information about a DMAC channel.
*/

typedef struct ioblock_sDMACChannelDescriptorTag
{
    /*! Channel index. */
    unsigned long						ui32Index;

    /*! Channel interrupt trigger. */
    unsigned long						ui32InterruptTrigger;

	/* --- Pointers to other descriptors etc --- */

    /*! Pointer to system descriptor. */
    ioblock_sSystemDescriptor *			psSystemDescriptor;

    /*! Pointer to interrupt descriptor. */
    ioblock_sInterruptBlock *			psInterruptDescriptor;

} ioblock_sDMACChannelDescriptor;


typedef struct ioblock_sISRInfoTag
{
    /*! STATEXT register address. */
    unsigned long						ui32STATEXTAddress;

    /*! STATEXT bitmask. */
    unsigned long						ui32STATEXTMask;

    /*! LEVELEXT register address. */
    unsigned long						ui32LEVELEXTAddress;

    /*! LEVEL bitmask. */
    unsigned long						ui32LEVELEXTMask;

} ioblock_sISRInfo;


/*!
	The ioblock_sBlockDescriptor structure holds information about an instance of an
	I/O peripheral block.
*/

typedef struct ioblock_sBlockDescriptorTag
{
    /*! Block index. */
    unsigned long						ui32Index;

    /*! Block register base address. */
    unsigned long						ui32Base;

    /*! Block interrupt trigger. */
    unsigned long						ui32InterruptTrigger;

    /*! Interrupt type (latched/not latched). */
    ioblock_eInterruptLevelType			eInterruptLevelType;

    /*! DMAC channel READ select value. */
    unsigned long						ui32DMACChannelSelectReadValue;

    /*! DMAC channel WRITE select value. */
    unsigned long						ui32DMACChannelSelectWriteValue;

    /*! DMAC burst size */
    unsigned long						ui32DMACBurstSize;

	/*! Pointer to API context */
	img_void *							pvAPIContext;

	/*! Pointer to block specific hardware descriptor */
	img_void *							pvBlockSpecificHardwareDescriptor;

	/* --- Pointers to other descriptors etc --- */

    /*! Pointer to system descriptor. */
    ioblock_sSystemDescriptor *			psSystemDescriptor;

    /*! Pointer to interrupt descriptor. */
    ioblock_sInterruptBlock *			psInterruptDescriptor;

    /*! Pointer to QIO_IVDESC_T structure. */
    QIO_IVDESC_T *						psIVDesc;

    /*! Device ISR info. */
    ioblock_sISRInfo					sDeviceISRInfo;

    /*! Interrupt mask info for QIO. */
	unsigned long						ui32IntMasks[QIO_MAXVECGROUPS];


} ioblock_sBlockDescriptor;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



#if 0
/*!
	The ioblock_sBlockContext structure holds saved/derived information about an instance of an
	I/O peripheral block.
*/

typedef struct ioblock_sBlockContextTag
{

    /*! Pointer to caller's block descriptor for the device instance. */
	ioblock_sBlockDescriptor *			psBlockDescriptor;

    /*! Device ISR info. */
    ioblock_sISRInfo					sDeviceISRInfo;

    /*! DMAC channel ISR info. */
    ioblock_sISRInfo					sDMACChannelISRInfo;

    /*! Interrupt mask info for QIO. */
	unsigned long						ui32IntMasks[QIO_MAXVECGROUPS];

} ioblock_sBlockContext;
#endif

#endif /* __IOBLOCK_DEFS_API_H__ */
