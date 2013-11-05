/*******************************************************************************
 @file   usbd_dfu_descriptors.h

 @brief  USBD API

         This file contains a set of descriptors used for DFU

 @author Imagination Technologies

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

 \n<b>Platform:</b>\n
         MobileTV

*******************************************************************************/

#if !defined (__USBD_DFU_DESCRIPTORS_H__)
#define __USBD_DFU_DESCRIPTORS_H__

/******************************************************************************
**************************** Included header files ****************************
*******************************************************************************/

#include <usb_dfuspec.h>

#define __USBD_ATTR__ __attribute__ ((__section__ (".usbdescriptors")))

/******************************************************************************
********************************* DFU CONFIGS *********************************
*******************************************************************************/
typedef struct configuration_descriptor_dfu_t
{
	USB_CONFIG_DESCRIPTOR		sConfigDesc;
	USB_INTERFACE_DESCRIPTOR	sInterfaceDesc0;
	DFU_FUNCTIONAL_DESCRIPTOR	sDFUDesc;
}__ATTR_PACKED__ CONFIGURATION_DESCRIPTOR_DFU;

#define CONF_DESCRIPTOR_DFU_LENGTH	\
	(USB_CONFIGURATION_DESCRIPTOR_LENGTH + \
	 USB_INTERFACE_DESCRIPTOR_LENGTH + \
	 DFU_FUNCTIONAL_DESCRIPTOR_LENGTH )

/* Full speed version */
static CONFIGURATION_DESCRIPTOR_DFU IMG_sDFUConfigurationDescriptor_FS __USBD_ATTR__ IMG_ALIGN(4) =
{
	/* Configuration descriptor */
	{
		USB_CONFIGURATION_DESCRIPTOR_LENGTH,	// UCHAR   bLength;
		USB_CONFIGURATION_DESCRIPTOR_TYPE,		// UCHAR   bDescriptorType;
		USB_INI16(CONF_DESCRIPTOR_DFU_LENGTH),	// USHORT  wTotalLength;
		0x01,									// UCHAR   bNumInterfaces;
		0x01,									// UCHAR   bConfigurationValue;
		0x00,									// UCHAR   iConfiguration;
		0xC0,									// UCHAR   bmAttributes D7=reserved(set to 1);D6=self powered; D5=remote wakeup; D4-0 reserved (0);
		0x00									// UCHAR   MaxPower;
	},
	/* Interface descriptor */
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,		// UCHAR   bLength;
		USB_INTERFACE_DESCRIPTOR_TYPE,			// UCHAR   bDescriptorType;
		0x00,									// UCHAR   bInterfaceNumber;
		0x00,									// UCHAR   bAlternateSetting;
		0x00,									// UCHAR   bNumEndpoints;
		DFU_CLASS_CODE,							// UCHAR   bInterfaceClass;
		DFU_SUBCLASS_CODE,						// UCHAR   bInterfaceSubClass;
		0x00,									// UCHAR   bInterfaceProtocol;
		0x00									// UCHAR   iInterface;
	},
	/* DFU functional descriptor */
	{
		DFU_FUNCTIONAL_DESCRIPTOR_LENGTH,		// UCHAR   bLength;
		DFU_DESCRIPTOR_TYPE,					// UCHAR   bDescriptorType;
		DFU_DOWNLOAD_CAPABLE | DFU_UPLOAD_CAPABLE | DFU_MANIFESTATION_TOLERANT, // UCHAR bmAttributes
		USB_INI16(1000),						// USHORT  wDetatchTimeOut;
		USB_INI16(264)							// USHORT  wTransferSize;
	}
};

/* High speed version */
static CONFIGURATION_DESCRIPTOR_DFU IMG_sDFUConfigurationDescriptor_HS __USBD_ATTR__ IMG_ALIGN(4) =
{
	/* Configuration descriptor */
	{
		USB_CONFIGURATION_DESCRIPTOR_LENGTH,	// UCHAR   bLength;
		USB_CONFIGURATION_DESCRIPTOR_TYPE,		// UCHAR   bDescriptorType;
		USB_INI16(CONF_DESCRIPTOR_DFU_LENGTH),	// USHORT  wTotalLength;
		0x01,									// UCHAR   bNumInterfaces;
		0x01,									// UCHAR   bConfigurationValue;
		0x00,									// UCHAR   iConfiguration;
		0xC0,									// UCHAR   bmAttributes D7=reserved(set to 1);D6=self powered; D5=remote wakeup; D4-0 reserved (0);
		0x00									// UCHAR   MaxPower;
	},
	/* Interface descriptor */
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,		// UCHAR   bLength;
		USB_INTERFACE_DESCRIPTOR_TYPE,			// UCHAR   bDescriptorType;
		0x00,									// UCHAR   bInterfaceNumber;
		0x00,									// UCHAR   bAlternateSetting;
		0x00,									// UCHAR   bNumEndpoints;
		DFU_CLASS_CODE,							// UCHAR   bInterfaceClass;
		DFU_SUBCLASS_CODE,						// UCHAR   bInterfaceSubClass;
		0x00,									// UCHAR   bInterfaceProtocol;
		0x00									// UCHAR   iInterface
	},
	/* DFU functional descriptor */
	{
		DFU_FUNCTIONAL_DESCRIPTOR_LENGTH,		// UCHAR   bLength;
		DFU_DESCRIPTOR_TYPE,					// UCHAR   bDescriptorType;
		DFU_DOWNLOAD_CAPABLE | DFU_UPLOAD_CAPABLE | DFU_MANIFESTATION_TOLERANT, // UCHAR bmAttributes
		USB_INI16(1000),						// USHORT  wDetatchTimeOut;
		USB_INI16(264)							// USHORT  wTransferSize;
	}
};

USBD_CONFIGURATION_T	IMG_sDFUConfigurations =
{
	{
		&IMG_sDFUConfigurationDescriptor_FS,
		0
	},											/* apsConfigDescriptorFullSpeed */
	{
		&IMG_sDFUConfigurationDescriptor_HS,
		0										
	},											/* apsConfigDescriptorHighSpeed */
	1											/* ui8NumConfigurations */
};

#endif /* __USBD_DFU_DESCRIPTORS_H */
