
/*******************************************************************************
 @file   usbd_std_descriptors.h

 @brief  USBD API

         This file contains a set of standard descriptors used for basic IO testing

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

#if !defined (__USBD_STD_DESCRIPTORS_H__)
#define __USBD_STD_DESCRIPTORS_H__

/******************************************************************************
**************************** Included header files ****************************
*******************************************************************************/

#include <usb_spec.h>

#define __USBD_ATTR__ __attribute__ ((__section__ (".usbdescriptors")))

/******************************************************************************
******************************* BULK/INT CONFIG *******************************
*******************************************************************************/
// 1 bulk in, 1 bulk out, 1 int in
/* Device descriptor */
USB_DEVICE_DESCRIPTOR	IMG_sStdDeviceDescriptor __USBD_ATTR__ IMG_ALIGN(4) = 
{
    USB_DEVICE_DESCRIPTOR_LENGTH,           // UCHAR   bLength;
    USB_DEVICE_DESCRIPTOR_TYPE,             // UCHAR   bDescriptorType;
    USB_INI16(0x0200),                      // USHORT  bcdUSB;
    USB_CLASS_VENDOR_SPEC,	                // UCHAR   bDeviceClass;
    0x00,                                   // UCHAR   bDeviceSubClass;
    0x00,                                   // UCHAR   bDeviceProtocol;
    64,					                    // UCHAR   bMaxPacketSize0;
	USB_INI16(0x0000),                      // USHORT  idVendor - filled out by application
    USB_INI16(0x0000),                      // USHORT  idProduct - filled out by application
    USB_INI16(0x0000),                      // USHORT  bcdDevice - filled out by application
	0x00,                                   // UCHAR   iManufacturer - filled out by application
	0x00,                                   // UCHAR   iProduct - filled out by application
	0x00,                                   // UCHAR   iSerialNumber - filled out by application
    0x01                                    // UCHAR   bNumConfigurations - may be changed based on API calls
};

USB_QUALIFIER_DESCRIPTOR IMG_sStdQualifierDescriptor __USBD_ATTR__ IMG_ALIGN(4) = 
{
	USB_QUALIFIER_DESCRIPTOR_LENGTH,
	USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE,		// constant = 0x06
	USB_INI16(0x0200),							// USB spec version = 2.00
	USB_CLASS_VENDOR_SPEC,						// device class = 0xFF (vendor specific)
	0,											// device subclass
	0,											// device protocol
	64,											// maximum packet size for other config
	0x01,										// number of other speed configurations
};

typedef struct configuration_descriptor_std_t
{
    USB_CONFIG_DESCRIPTOR		sConfigDesc;
    USB_INTERFACE_DESCRIPTOR	sInterfaceDesc0;
    USB_ENDPOINT_DESCRIPTOR		sDataInEp;
	USB_ENDPOINT_DESCRIPTOR		sDataOutEp;
	USB_ENDPOINT_DESCRIPTOR		sInterruptInEp;
	USB_ENDPOINT_DESCRIPTOR		sDataInEp2;
	USB_ENDPOINT_DESCRIPTOR		sDataOutEp2;
}__ATTR_PACKED__ CONFIGURATION_DESCRIPTOR_STD;

#define CONF_DESCRIPTOR_STD_LENGTH       \
		(USB_CONFIGURATION_DESCRIPTOR_LENGTH +  \
		USB_INTERFACE_DESCRIPTOR_LENGTH     +  \
		USB_ENDPOINT_DESCRIPTOR_LENGTH      +  \
		USB_ENDPOINT_DESCRIPTOR_LENGTH      +  \
		USB_ENDPOINT_DESCRIPTOR_LENGTH      +  \
		USB_ENDPOINT_DESCRIPTOR_LENGTH      +  \
		USB_ENDPOINT_DESCRIPTOR_LENGTH)

/* Full speed version */
static CONFIGURATION_DESCRIPTOR_STD		IMG_sStdConfigurationDescriptor_FS __USBD_ATTR__ IMG_ALIGN(4) = 
{
	/* Configuration descriptor */
	{
		USB_CONFIGURATION_DESCRIPTOR_LENGTH,    // UCHAR   bLength;
		USB_CONFIGURATION_DESCRIPTOR_TYPE,      // UCHAR   bDescriptorType;
		USB_INI16(CONF_DESCRIPTOR_STD_LENGTH),	// USHORT  wTotalLength;
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
		0x05,									// UCHAR   bNumEndpoints;
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
		0x02,                                   // UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=int). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(0x0040),                      // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x0B									// UCHAR   interval
	},
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,         // UCHAR   bLength;
		USB_ENDPOINT_DESCRIPTOR_TYPE,           // UCHAR   ENDPOINT descriptor = 0x05
		0x02,                                   // UCHAR   endpoint number (b3-b0: address, b7: direction (0=out, 1=in)	
		0x02,                                   // UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=int). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(0x0040),                      // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x0B									// UCHAR   interval
	},
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,         // UCHAR   bLength;
		USB_ENDPOINT_DESCRIPTOR_TYPE,           // UCHAR   ENDPOINT descriptor = 0x05
		0x83,                                   // UCHAR   endpoint number (b3-b0: address, b7: direction (0=out, 1=in)	
		0x03,                                   // UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=IMG_INT32). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(0x0040),                      // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x01									// UCHAR   interval (1-255)
	},
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,         // UCHAR   bLength;
		USB_ENDPOINT_DESCRIPTOR_TYPE,           // UCHAR   ENDPOINT descriptor = 0x05
		0x84,                                   // UCHAR   endpoint number (b3-b0: address, b7: direction (0=out, 1=in)
		0x02,                                   // UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=int). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(0x0040),                      // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x0B									// UCHAR   interval
	},
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,         // UCHAR   bLength;
		USB_ENDPOINT_DESCRIPTOR_TYPE,           // UCHAR   ENDPOINT descriptor = 0x05
		0x05,                                   // UCHAR   endpoint number (b3-b0: address, b7: direction (0=out, 1=in)	
		0x02,                                   // UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=int). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(0x0040),                      // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x0B									// UCHAR   interval
	}
};

/* High speed version */
static CONFIGURATION_DESCRIPTOR_STD		IMG_sStdConfigurationDescriptor_HS __USBD_ATTR__ IMG_ALIGN(4) = 
{
	/* Configuration descriptor */
	{
		USB_CONFIGURATION_DESCRIPTOR_LENGTH,    // UCHAR   bLength;
		USB_CONFIGURATION_DESCRIPTOR_TYPE,      // UCHAR   bDescriptorType;
		USB_INI16(CONF_DESCRIPTOR_STD_LENGTH),	// USHORT  wTotalLength;
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
		0x05,									// UCHAR   bNumEndpoints;
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
		0x02,                                   // UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=IMG_INT32). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(512),                         // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x0B									// UCHAR   interval
	},
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,         // UCHAR   bLength;
		USB_ENDPOINT_DESCRIPTOR_TYPE,           // UCHAR   ENDPOINT descriptor = 0x05
		0x02,                                   // UCHAR   endpoint number (b3-b0: address, b7: direction (0=out, 1=in)	
		0x02,                                   // UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=IMG_INT32). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(512),                         // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x0B									// UCHAR   interval
	},
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,         // UCHAR   bLength;
		USB_ENDPOINT_DESCRIPTOR_TYPE,           // UCHAR   ENDPOINT descriptor = 0x05
		0x83,                                   // UCHAR   endpoint number (b3-b0: address, b7: direction (0=out, 1=in)	
		0x03,                                   // UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=IMG_INT32). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(0x0040),                      // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x04									// UCHAR   interval (1-16). Used as T=2^(interval-1) in HS
	},
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,         // UCHAR   bLength;
		USB_ENDPOINT_DESCRIPTOR_TYPE,           // UCHAR   ENDPOINT descriptor = 0x05
		0x84,                                   // UCHAR   endpoint number (b3-b0: address, b7: direction (0=out, 1=in)
		0x02,                                   // UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=IMG_INT32). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(512),                         // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x0B									// UCHAR   interval
	},
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,         // UCHAR   bLength;
		USB_ENDPOINT_DESCRIPTOR_TYPE,           // UCHAR   ENDPOINT descriptor = 0x05
		0x05,                                   // UCHAR   endpoint number (b3-b0: address, b7: direction (0=out, 1=in)	
		0x02,                                   // UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=IMG_INT32). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(512),                         // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x0B									// UCHAR   interval
	}
};

USBD_CONFIGURATION_T	IMG_sStdConfigurations =
{
	{
		&IMG_sStdConfigurationDescriptor_FS,
		0
	},											/* apsConfigDescriptorFullSpeed */
	{
		&IMG_sStdConfigurationDescriptor_HS,
		0
	},											/* apsConfigDescriptorHighSpeed */
	1											/* ui8NumConfigurations */
};

/******************************************************************************
********************************* BOOT CONFIG *********************************
*******************************************************************************/

typedef struct configuration_descriptor_boot_t
{
    USB_CONFIG_DESCRIPTOR		sConfigDesc;
    USB_INTERFACE_DESCRIPTOR	sInterfaceDesc0;
    USB_ENDPOINT_DESCRIPTOR		sDataInEp;
	USB_ENDPOINT_DESCRIPTOR		sDataOutEp;
}__ATTR_PACKED__ CONFIGURATION_DESCRIPTOR_BOOT;

#define CONF_DESCRIPTOR_BOOT_LENGTH       \
		(USB_CONFIGURATION_DESCRIPTOR_LENGTH +  \
		USB_INTERFACE_DESCRIPTOR_LENGTH     +  \
		USB_ENDPOINT_DESCRIPTOR_LENGTH      +  \
		USB_ENDPOINT_DESCRIPTOR_LENGTH)

/* Full speed version */
static CONFIGURATION_DESCRIPTOR_BOOT	IMG_sBootConfigurationDescriptor_FS __USBD_ATTR__ IMG_ALIGN(4) = 
{
	/* Configuration descriptor */
	{
		USB_CONFIGURATION_DESCRIPTOR_LENGTH,    // UCHAR   bLength;
		USB_CONFIGURATION_DESCRIPTOR_TYPE,      // UCHAR   bDescriptorType;
		USB_INI16(CONF_DESCRIPTOR_BOOT_LENGTH), // USHORT  wTotalLength;
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
		0x02,                                   // UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=int). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(0x0040),                      // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x0B									// UCHAR   interval
	},
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,         // UCHAR   bLength;
		USB_ENDPOINT_DESCRIPTOR_TYPE,           // UCHAR   ENDPOINT descriptor = 0x05
		0x02,                                   // UCHAR   endpoint number (b3-b0: address, b7: direction (0=out, 1=in)	
		0x02,                                   // UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=int). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(0x0040),                      // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x0B									// UCHAR   interval
	}
};

/* High speed version */
static CONFIGURATION_DESCRIPTOR_BOOT	IMG_sBootConfigurationDescriptor_HS __USBD_ATTR__ IMG_ALIGN(4) = 
{
	/* Configuration descriptor */
	{
		USB_CONFIGURATION_DESCRIPTOR_LENGTH,    // UCHAR   bLength;
		USB_CONFIGURATION_DESCRIPTOR_TYPE,      // UCHAR   bDescriptorType;
		USB_INI16(CONF_DESCRIPTOR_BOOT_LENGTH), // USHORT  wTotalLength;
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
		0x02,                                   // UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=IMG_INT32). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(512),                         // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x0B									// UCHAR   interval
	},
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,         // UCHAR   bLength;
		USB_ENDPOINT_DESCRIPTOR_TYPE,           // UCHAR   ENDPOINT descriptor = 0x05
		0x02,                                   // UCHAR   endpoint number (b3-b0: address, b7: direction (0=out, 1=in)	
		0x02,                                   // UCHAR   attributes: b1-b0: Tx type (00=control, 01=iso, 10=bulk, 11=IMG_INT32). for iso {b3-b2: sync type, b5-b4: usage type}
		USB_INI16(512),                         // USHORT  max packet size b10-b0=max packet size (56). For high speed iso: b12-b11 = number of additional transactions opportunities per microframe (0)
		0x0B									// UCHAR   interval
	}
};

USBD_CONFIGURATION_T	IMG_sBootConfigurations =
{
	{
		&IMG_sBootConfigurationDescriptor_FS,
		0
	},											/* apsConfigsFullSpeed */
	{
		&IMG_sBootConfigurationDescriptor_HS,
		0,
	},											/* apsConfigsHighSpeed */
	1											/* the number of configurations available */
};

#endif /* __USBD_STD_DESCRIPTORS_H__ */
