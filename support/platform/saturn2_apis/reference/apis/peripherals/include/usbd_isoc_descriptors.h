
/*******************************************************************************
 @file   usbd_isoc_descriptors.h

 @brief  USBD API

         This file contains a set of descriptors used for basic IO testing on 
		 isochronous endpoints.

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

/******************************************************************************
********************************* ISOC CONFIG *********************************
*******************************************************************************/
typedef struct configuration_descriptor_isoc_t
{
    USB_CONFIG_DESCRIPTOR		sConfigDesc;
    USB_INTERFACE_DESCRIPTOR	sInterfaceDesc0;
	USB_INTERFACE_DESCRIPTOR	sInterfaceDesc1;
    USB_ENDPOINT_DESCRIPTOR		sDataInEp;
	USB_ENDPOINT_DESCRIPTOR		sDataOutEp;

}__ATTR_PACKED__ CONFIGURATION_DESCRIPTOR_ISOC;

#define CONF_DESCRIPTOR_ISOC_LENGTH		sizeof(CONFIGURATION_DESCRIPTOR_ISOC)

static CONFIGURATION_DESCRIPTOR_ISOC		IMG_sISOCConfigurationDescriptor_FS __USBD_ATTR__ IMG_ALIGN(4) = 
{
	/* Configuration descriptor */
	{
		USB_CONFIGURATION_DESCRIPTOR_LENGTH,    // UCHAR   bLength;
		USB_CONFIGURATION_DESCRIPTOR_TYPE,      // UCHAR   bDescriptorType;
		USB_INI16(CONF_DESCRIPTOR_ISOC_LENGTH), // USHORT  wTotalLength;
		0x01,                                   // UCHAR   bNumInterfaces;
		0x01,                                   // UCHAR   bConfigurationValue;
		0x00,                                   // UCHAR   iConfiguration;
		0xC0,                                   // UCHAR   bmAttributes D7=reserved(set to 1);D6=self powered; D5=remote wakeup; D4-0 reserved (0);
		0x00                                    // UCHAR   MaxPower;
	},
	/* Interface descriptor */
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,        // UCHAR   bLength;
		USB_INTERFACE_DESCRIPTOR_TYPE,          // UCHAR   bDescriptorType;
		0x00,                                   // UCHAR   bInterfaceNumber;
		0x00,                                   // UCHAR   bAlternateSetting;
		0x00,                                   // UCHAR   bNumEndpoints;
		USB_CLASS_VENDOR_SPEC,	                // UCHAR   bInterfaceClass;
		0x00,                                   // UCHAR   bInterfaceSubClass;
		0x00,                                   // UCHAR   bInterfaceProtocol;
		0x00                                    // UCHAR   iInterface;
	},
	/* Interface descriptor */
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,        // UCHAR   bLength;
		USB_INTERFACE_DESCRIPTOR_TYPE,          // UCHAR   bDescriptorType;
		0x00,                                   // UCHAR   bInterfaceNumber;
		0x01,                                   // UCHAR   bAlternateSetting;
		0x02,                                   // UCHAR   bNumEndpoints;
		USB_CLASS_VENDOR_SPEC,	                // UCHAR   bInterfaceClass;
		0x00,                                   // UCHAR   bInterfaceSubClass;
		0x00,                                   // UCHAR   bInterfaceProtocol;
		0x00                                    // UCHAR   iInterface;
	},
	/* Endpoint descriptors */
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,         // UCHAR   bLength;
		USB_ENDPOINT_DESCRIPTOR_TYPE,           // UCHAR   ENDPOINT descriptor = 0x05
		0x81,                                   // UCHAR   endpoint number (b3-b0: address, b7: direction (0=out, 1=in)
		0x01,									// UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=IMG_INT32). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(0x0040),                      // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x01									// UCHAR   interval
	},
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,         // UCHAR   bLength;
		USB_ENDPOINT_DESCRIPTOR_TYPE,           // UCHAR   ENDPOINT descriptor = 0x05
		0x01,		                            // UCHAR   endpoint number (b3-b0: address, b7: direction (0=out, 1=in)
		0x01,									// UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=IMG_INT32). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(0x0040),                      // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x01							        // UCHAR   interval
	}
};

/* High speed */
static CONFIGURATION_DESCRIPTOR_ISOC		IMG_sISOCConfigurationDescriptor_HS __USBD_ATTR__ IMG_ALIGN(4) = 
{
	/* Configuration descriptor */
	{
		USB_CONFIGURATION_DESCRIPTOR_LENGTH,    // UCHAR   bLength;
		USB_CONFIGURATION_DESCRIPTOR_TYPE,      // UCHAR   bDescriptorType;
		USB_INI16(CONF_DESCRIPTOR_ISOC_LENGTH), // USHORT  wTotalLength;
		0x01,                                   // UCHAR   bNumInterfaces;
		0x01,                                   // UCHAR   bConfigurationValue;
		0x00,                                   // UCHAR   iConfiguration;
		0xC0,                                   // UCHAR   bmAttributes D7=reserved(set to 1);D6=self powered; D5=remote wakeup; D4-0 reserved (0);
		0x00                                    // UCHAR   MaxPower;
	},
	/* Interface descriptor */
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,        // UCHAR   bLength;
		USB_INTERFACE_DESCRIPTOR_TYPE,          // UCHAR   bDescriptorType;
		0x00,                                   // UCHAR   bInterfaceNumber;
		0x00,                                   // UCHAR   bAlternateSetting;
		0x00,                                   // UCHAR   bNumEndpoints;
		USB_CLASS_VENDOR_SPEC,	                // UCHAR   bInterfaceClass;
		0x00,                                   // UCHAR   bInterfaceSubClass;
		0x00,                                   // UCHAR   bInterfaceProtocol;
		0x00                                    // UCHAR   iInterface;
	},
	/* Interface descriptor */
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,        // UCHAR   bLength;
		USB_INTERFACE_DESCRIPTOR_TYPE,          // UCHAR   bDescriptorType;
		0x00,                                   // UCHAR   bInterfaceNumber;
		0x01,                                   // UCHAR   bAlternateSetting;
		0x02,                                   // UCHAR   bNumEndpoints;
		USB_CLASS_VENDOR_SPEC,	                // UCHAR   bInterfaceClass;
		0x00,                                   // UCHAR   bInterfaceSubClass;
		0x00,                                   // UCHAR   bInterfaceProtocol;
		0x00                                    // UCHAR   iInterface;
	},
	/* Endpoint descriptors */
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,         // UCHAR   bLength;
		USB_ENDPOINT_DESCRIPTOR_TYPE,           // UCHAR   ENDPOINT descriptor = 0x05
		0x81,                                   // UCHAR   endpoint number (b3-b0: address, b7: direction (0=out, 1=in)
		0x01,									// UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=IMG_INT32). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(1024),                        // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x01									// UCHAR   interval
	},
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,         // UCHAR   bLength;
		USB_ENDPOINT_DESCRIPTOR_TYPE,           // UCHAR   ENDPOINT descriptor = 0x05
		0x02,		                            // UCHAR   endpoint number (b3-b0: address, b7: direction (0=out, 1=in)
		0x01,									// UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=IMG_INT32). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(1024),                        // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x01							        // UCHAR   interval
	}
};

USBD_CONFIGURATION_T	IMG_sIsocConfigurations =
{
	{
		&IMG_sISOCConfigurationDescriptor_FS,
		0
	},											/* apsConfigsFullSpeed */
	{
		&IMG_sISOCConfigurationDescriptor_HS,
		0
	},											/* apsConfigsHighSpeed */
	1											/* the number of configurations available */
};
