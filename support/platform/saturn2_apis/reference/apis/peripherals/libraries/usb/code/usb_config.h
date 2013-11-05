/*!
*******************************************************************************
 @file   usb_config.h

 @brief  USB Driver Configuration


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

 <b>Platform:</b>\n
         MobileTV

*******************************************************************************/

#if !defined (__USB_CONFIG_H__)
#define __USB_CONFIG_H__


/////////////////////////////////////////////////////////////////////////////////
// USBD API Configuration
/////////////////////////////////////////////////////////////////////////////////

/* Number of endpoints (excluding EP0) supported by API. 
   This is usually set to match the number of endpoints the hardware supports
   but can be reduced to reduce driver size									  */
#define USBD_MAX_EPS							(8) /* Excluding endpoint 0 */

/* Maximum number of alternate or default interfaces supported in per
   device configuration */
#define USBD_MAX_INTERFACES						(5)

/* Maximum number of in or out endpoints supported per interface */
#define USBD_MAX_INTERFACE_EP_PAIRS				(5)

/////////////////////////////////////////////////////////////////////////////////
// Driver Feature Configuration
/////////////////////////////////////////////////////////////////////////////////

/* Device only code */
#define DWC_DEVICE_ONLY
/* Ignore suspend interrupts */
//#define IGNORE_SUSPEND
/* Special IN endpoint disabling code - Should be used */
#define DISABLE_IN_EP_FEATURE
/* Special OUT endpoint disabling code - Should be used */
#define DISABLE_OUT_EP_FEATURE
/* Use only DMA mode code (Buffer and Descriptor DMA) */
#define USE_DMA_INTERNAL
/* Use only Descriptor DMA code */
#define USE_DDMA_ONLY
/* Use only dedicated TxFIFO mode */
#define USE_MULTIPLE_TX_FIFO

/* Full/boot code build specifics */
#if defined (USBD_BOOT_CODE)
	/* Timer function is not available in the boot ROM case */
	#define TIMER_NOT_AVAILABLE
#else 
	/* Define this flag if ISOC transactions are to be performed in slave mode. */
	//#define USE_PERIODIC_EP
	/* Enable optimised Descriptor DMA based Isochronous transfers */
	#define _PER_IO_
#endif /* BOOT_CODE */

/* Enable partial power down feature (only used when ghwcfg4:EnablePwrOpt is set) */
//#define PARTIAL_POWER_DOWN
/* Enable clock gating (if partial power down feature is not used) */
//#define CLOCK_GATING

/* If _PER_IO_ is defined, define USE_DDMA_ONLY */
#if defined (_PER_IO_)
	#if !defined (USE_DDMA_ONLY)
		#warning Isochronous DDMA Enabled - Disabling Buffered DMA code path
		#define USE_DDMA_ONLY
	#endif
#endif

/////////////////////////////////////////////////////////////////////////////////
// Internal Driver Configuration
/////////////////////////////////////////////////////////////////////////////////

/* Number of statically allocated DMA Descriptors per endpoint. */
#define DESCRIPTORS_PER_EP						(16)
/* Maximum number of isochronous packets per EP transfer request */
#define MAX_ISO_PACKETS							(16)
/* Isoc packet size in bytes */
/* This must correspond to the Windows driver's packet size */
#define ISOC_PACKET_SIZE						(1024)
/* Maximum request objects */
#define MAX_REQUEST_OBJECTS		(10)

#endif /* __USB_CONFIG_H__ */
