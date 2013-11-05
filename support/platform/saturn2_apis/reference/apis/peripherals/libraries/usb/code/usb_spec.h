
#ifndef __USB_SPEC_H
#define __USB_SPEC_H

#include "img_defs.h"

#define USB_INI16(w) \
    { w & 0xff, (w >> 8) & 0xff }

#define USB_INI32(dw) \
    { dw & 0xff, (dw >> 8) & 0xff, (dw >> 16) & 0xff, (dw >> 24) & 0xff }

#define USB_LE16_TO_CPU(pc) (   (((IMG_UINT16)(*(pc+1)) & 0xFF) << 8) | (((IMG_UINT16)(*pc) & 0xFF)) )

#define USB_CPU_TO_LE16(pc, val)  { *pc = (val & 0xff); *(pc + 1) = ((val >> 8) & 0xFF); }

#define __ATTR_PACKED__		__attribute__ ((packed))

/*-------------------------------------------------------------------------*/

/* CONTROL REQUEST QUANTITIES */

/* USB directions used in endpoint descriptor bEndpointAddress field */
#define USB_DIR_OUT				0			/* to device */
#define USB_DIR_IN				0x80		/* to host */

/* USB types, the second of three bRequestType fields */
#define USB_TYPE_MASK			(0x03 << 5)
#define USB_TYPE_STANDARD		(0x00 << 5)
#define USB_TYPE_CLASS			(0x01 << 5)
#define USB_TYPE_VENDOR			(0x02 << 5)
#define USB_TYPE_RESERVED		(0x03 << 5)

/* USB recipients, the third of three bRequestType fields */
#define USB_RECIP_MASK			0x1f
#define USB_RECIP_DEVICE		0x00
#define USB_RECIP_INTERFACE		0x01
#define USB_RECIP_ENDPOINT		0x02
#define USB_RECIP_OTHER			0x03

/* Standard requests, for the bRequest field of a SETUP packet */
#define USB_REQ_GET_STATUS			0x00
#define USB_REQ_CLEAR_FEATURE		0x01
#define USB_REQ_SET_FEATURE			0x03
#define USB_REQ_SET_ADDRESS			0x05
#define USB_REQ_GET_DESCRIPTOR		0x06
#define USB_REQ_SET_DESCRIPTOR		0x07
#define USB_REQ_GET_CONFIGURATION	0x08
#define USB_REQ_SET_CONFIGURATION	0x09
#define USB_REQ_GET_INTERFACE		0x0A
#define USB_REQ_SET_INTERFACE		0x0B
#define USB_REQ_SYNCH_FRAME			0x0C

/* USB feature flags */
#define USB_DEVICE_SELF_POWERED			0	/* (read only) */
#define USB_DEVICE_REMOTE_WAKEUP		1	/* dev may initiate wakeup */
#define USB_DEVICE_TEST_MODE			2	/* (high speed only) */
#define USB_DEVICE_B_HNP_ENABLE			3	/* dev may initiate HNP */
#define USB_DEVICE_A_HNP_SUPPORT		4	/* RH port supports HNP */
#define USB_DEVICE_A_ALT_HNP_SUPPORT	5	/* other RH port does */
#define USB_DEVICE_DEBUG_MODE			6	/* (special devices only) */

#define USB_ENDPOINT_HALT				0	/* IN/OUT will STALL */


/* USB_CTRLREQUEST - SETUP data for a USB device control request */
typedef struct usb_ctrlrequest_t 
{
	IMG_UINT8 bRequestType;
	IMG_UINT8 bRequest;
	IMG_UINT16 wValue;
	IMG_UINT16 wIndex;
	IMG_UINT16 wLength;

} USB_CTRLREQUEST;

/*-------------------------------------------------------------------------*/

/* STANDARD DESCRIPTORS */

/*  Descriptor types (USB 2.0 spec, table 9.5) */

/* Descriptor codes */
#define USB_DEVICE_DESCRIPTOR_TYPE        0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE 0x02
#define USB_STRING_DESCRIPTOR_TYPE        0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE     0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE      0x05


#define USB_DEVICE_DESCRIPTOR_TYPE				0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE		0x02
#define USB_STRING_DESCRIPTOR_TYPE				0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE			0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE			0x05
#define USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE	0x06
#define USB_OTHER_SPEED_CONFIG_DESCRIPTOR_TYPE	0x07
#define USB_INTERFACE_POWER_DESCRIPTOR_TYPE		0x08


/* All standard descriptors have these 2 fields at the beginning */
typedef struct usb_descriptor_header_t 
{
	IMG_UINT8  bLength;
	IMG_UINT8  bDescriptorType;

} USB_DESCRIPTOR_HEADER;


/*-------------------------------------------------------------------------*/

/* USB_DT_DEVICE: Device descriptor */
typedef struct usb_device_descriptor_t 
{
    IMG_UINT8 bLength;
    IMG_UINT8 bDescriptorType;
    IMG_UINT8 bcdUSB[2];
    IMG_UINT8 bDeviceClass;
    IMG_UINT8 bDeviceSubClass;
    IMG_UINT8 bDeviceProtocol;
    IMG_UINT8 bMaxPacketSize0;
    IMG_UINT8 idVendor[2];
    IMG_UINT8 idProduct[2];
    IMG_UINT8 bcdDevice[2];
    IMG_UINT8 iManufacturer;
    IMG_UINT8 iProduct;
    IMG_UINT8 iSerialNumber;
    IMG_UINT8 bNumConfigurations;

}__ATTR_PACKED__ USB_DEVICE_DESCRIPTOR;



#define USB_DEVICE_DESCRIPTOR_LENGTH 18


/* Vendor specific class USB code */
#define USB_CLASS_VENDOR_SPEC		0xff

/*-------------------------------------------------------------------------*/

/* USB_DT_CONFIG: Configuration descriptor information. */

typedef struct usb_config_descriptor_t 
{
	IMG_UINT8  bLength;
	IMG_UINT8  bDescriptorType;

	IMG_UINT8  wTotalLength[2]; 
	IMG_UINT8  bNumInterfaces;
	IMG_UINT8  bConfigurationValue;
	IMG_UINT8  iConfiguration;
	IMG_UINT8  bmAttributes;
	IMG_UINT8  bMaxPower;

}__ATTR_PACKED__ USB_CONFIG_DESCRIPTOR;

#define USB_CONFIGURATION_DESCRIPTOR_LENGTH 9

/* from config descriptor bmAttributes */
#define USB_CONFIG_ATT_ONE			(1 << 7)	/* must be set */
#define USB_CONFIG_ATT_SELFPOWER	(1 << 6)	/* self powered */
#define USB_CONFIG_ATT_WAKEUP		(1 << 5)	/* can wakeup */

/*-------------------------------------------------------------------------*/

/* USB_DT_INTERFACE: Interface descriptor */

typedef struct usb_interface_descriptor_t 
{
	IMG_UINT8  bLength;
	IMG_UINT8  bDescriptorType;

	IMG_UINT8  bInterfaceNumber;
	IMG_UINT8  bAlternateSetting;
	IMG_UINT8  bNumEndpoints;
	IMG_UINT8  bInterfaceClass;
	IMG_UINT8  bInterfaceSubClass;
	IMG_UINT8  bInterfaceProtocol;
	IMG_UINT8  iInterface;

}__ATTR_PACKED__ USB_INTERFACE_DESCRIPTOR;

#define USB_INTERFACE_DESCRIPTOR_LENGTH 9

/*-------------------------------------------------------------------------*/

/* USB_DT_ENDPOINT: Endpoint descriptor */

typedef struct usb_endpoint_descriptor_t 
{
	IMG_UINT8  bLength;
	IMG_UINT8  bDescriptorType;

	IMG_UINT8  bEndpointAddress;
	IMG_UINT8  bmAttributes;
	IMG_UINT8  wMaxPacketSize[2];
	IMG_UINT8  bInterval;

}__ATTR_PACKED__ USB_ENDPOINT_DESCRIPTOR;

#define USB_ENDPOINT_DESCRIPTOR_LENGTH 7


/*
 * Endpoints
 */
#define USB_ENDPOINT_NUMBER_MASK	0x0f	/* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK		0x80

#define USB_ENDPOINT_XFERTYPE_MASK	0x03	/* in bmAttributes */
#define USB_ENDPOINT_XFER_CONTROL	0
#define USB_ENDPOINT_XFER_ISOC		1
#define USB_ENDPOINT_XFER_BULK		2
#define USB_ENDPOINT_XFER_INT		3


/*-------------------------------------------------------------------------*/

/* USB_DT_DEVICE_QUALIFIER: Device Qualifier descriptor */

typedef struct usb_qualifier_descriptor_t 
{
	IMG_UINT8  bLength;
	IMG_UINT8  bDescriptorType;

	IMG_UINT8  bcdUSB[2];
	IMG_UINT8  bDeviceClass;
	IMG_UINT8  bDeviceSubClass;
	IMG_UINT8  bDeviceProtocol;
	IMG_UINT8  bMaxPacketSize0;
	IMG_UINT8  bNumConfigurations;
	IMG_UINT8  bRESERVED;

}__ATTR_PACKED__ USB_QUALIFIER_DESCRIPTOR;

#define USB_QUALIFIER_DESCRIPTOR_LENGTH		10

/*-------------------------------------------------------------------------*/

typedef enum usb_device_speed 
{
	USB_SPEED_UNKNOWN = 0,				/* enumerating */
	USB_SPEED_LOW, 
	USB_SPEED_FULL,						/* usb 1.1 */
	USB_SPEED_HIGH						/* usb 2.0 */

} USB_DEVICE_SPEED;


#endif	/* __USB_SPEC_H */
