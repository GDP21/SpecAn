
/*******************************************************************************
 @file   usbd_std_descriptors.h

 @brief  USBD API

         This file contains a set of descriptors used for ethernet and video classes

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

#if !defined (__USBD_ETH_VID_DESCRIPTORS_H__)
#define __USBD_ETH_VID_DESCRIPTORS_H__


/******************************************************************************
**************************** SHARED ETH/VID CONFIG ****************************
*******************************************************************************/

USB_QUALIFIER_DESCRIPTOR IMG_sEthVidQualifierDescriptor __USBD_ATTR__ IMG_ALIGN(4) = 
{
	USB_QUALIFIER_DESCRIPTOR_LENGTH,
	USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE,		// constant = 0x06
	USB_INI16(0x0200),							// USB spec version = 2.00
	0x00,										// device class = 0xFF (vendor specific)
	0,											// device subclass
	0,											// device protocol
	64,											// maximum packet size for other config
	0x01,										// number of other speed configurations
};

#define CS_INTERFACE						(0x24)
#define CS_ENDPOINT							(0x25)
#define USB_CDC_CLASS_DATA					(0x0A)

/* Header functional descriptor */
typedef struct usb_header_functional_descriptor_t
{
	IMG_UINT8	bLength;
	IMG_UINT8	bDescriptorType;
	IMG_UINT8	bDescriptorSubtype;
	IMG_UINT8	bcdCDC[2];
}__ATTR_PACKED__ HEADER_FUNCTIONAL_DESCRIPTOR;

#define CDC_HEADER_TYPE						(0x00)
#define HEADER_FUNCTIONAL_DESCRIPTOR_LENGTH	(5)

/* Union functional descriptor */
typedef struct usb_union_functional_descriptor_t
{
	IMG_UINT8	bLength;
	IMG_UINT8	bDescriptorType;
	IMG_UINT8	bDescriptorSubtype;
	IMG_UINT8	bMasterInterface;
	IMG_UINT8	bSlaveInterface0;
}__ATTR_PACKED__ UNION_FUNCTIONAL_DESCRIPTOR;

#define CDC_UNION_TYPE						(0x06)
#define UNION_FUNCTIONAL_DESCRIPTOR_LENGTH	(5)

/******************************************************************************
******************************** ETH/VID CONFIG *******************************
*******************************************************************************/

USB_DEVICE_DESCRIPTOR	IMG_sEthVidDeviceDescriptor __USBD_ATTR__ IMG_ALIGN(4) =
{
	USB_DEVICE_DESCRIPTOR_LENGTH,
	USB_DEVICE_DESCRIPTOR_TYPE,
	USB_INI16(0x0200),
	0x00, // Interface will determine device class
	0x00,
	0x01,
	64,
	USB_INI16(0x0000),
	USB_INI16(0x0000),
	USB_INI16(0x0000),
	0x00,
	0x00,
	0x00,
	0x01
};

/* ETH functional descriptor */
typedef struct usb_eth_functional_descriptor_t
{
    IMG_UINT8	bLength;
    IMG_UINT8	bDescriptorType;
    IMG_UINT8	bDescriptorSubtype;
	IMG_UINT8	iMACAddress;
	IMG_UINT8	bmEthernetStatistics[4];
	IMG_UINT8	wMaxSegmentSize[2];
	IMG_UINT8	wNumberMCFilters[2];
	IMG_UINT8	bNumberPowerFilters;
}__ATTR_PACKED__ ETH_FUNCTIONAL_DESCRIPTOR;

#define CDC_ETHERNET_TYPE					(0x0F)
#define ETH_FUNCTIONAL_DESCRIPTOR_LENGTH	(13)

typedef struct configuration_descriptor_eth_vid_t
{
	USB_CONFIG_DESCRIPTOR			sConfigDesc;
	// CDC ECM control descriptors
	USB_INTERFACE_DESCRIPTOR		sECMControl;	// Eth control class interface
	HEADER_FUNCTIONAL_DESCRIPTOR	sHdrDesc;
	UNION_FUNCTIONAL_DESCRIPTOR		sUnionDesc;
	ETH_FUNCTIONAL_DESCRIPTOR		sEthDesc;
	// Status endpoint
	USB_ENDPOINT_DESCRIPTOR			sNotifyEP;
	// Data interface, alt settings 0 and 1
	USB_INTERFACE_DESCRIPTOR		sNoEPInterface;	// Default interface has 0 endpoints
	USB_INTERFACE_DESCRIPTOR		sDataInterface;	// Data class interface
	USB_ENDPOINT_DESCRIPTOR			sEpIn;
	USB_ENDPOINT_DESCRIPTOR			sEpOut;
	
} __ATTR_PACKED__ CONFIGURATION_DESCRIPTOR_ETH_VID;

#define CONF_DESCRIPTOR_ETH_VID_LENGTH	sizeof(CONFIGURATION_DESCRIPTOR_ETH_VID)

/* High speed version */
static CONFIGURATION_DESCRIPTOR_ETH_VID IMG_sEthVidConfigurationDescriptor_HS __USBD_ATTR__ IMG_ALIGN(4) =
{
	/* Configuration descriptor */
	{
		USB_CONFIGURATION_DESCRIPTOR_LENGTH,
		USB_CONFIGURATION_DESCRIPTOR_TYPE,
		USB_INI16(CONF_DESCRIPTOR_ETH_VID_LENGTH),
		//0x02, // 2 interfaces for now
		//0x03, // 3 interfaces for now
		0x04, // 4 interfaces for now
		0x01, // configuration value
		0x00, // no string index
		0xC0,
		0x00
	},
	/* Communication Class Interface descriptor */
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,
		USB_INTERFACE_DESCRIPTOR_TYPE,
		0x00, // interface number
		0x00, // alt setting
		0x01, // num endpoints
		0x02, // Communications Interface Class code
		0x06, // ECM code
		0x00, // Protocol ?
		0x00
	},
	/* Header functional descriptor */
	{
		HEADER_FUNCTIONAL_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		CDC_HEADER_TYPE,
		USB_INI16(0x0120)
	},
	/* Union functional descriptor */
	{
		UNION_FUNCTIONAL_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		CDC_UNION_TYPE,
		0x0,
		0x1
	},
	/* Eth functional descriptor */
	{
		ETH_FUNCTIONAL_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		CDC_ETHERNET_TYPE, // Ethernet Networking Functional Descriptor subtype
		0x4, // iMacAddress - hardcoded for now
		{ 0 }, // no stats
		USB_INI16(1514), // wMaxSegmentSize
		{ 0 }, // wNumberMCFilters
		0 // bNumberPowerFilters
	},
	/* Notify endpoint */
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,
		USB_ENDPOINT_DESCRIPTOR_TYPE,
		0x81, // ep1 in
		0x03, // interrupt
		USB_INI16(16), // 8 byte header + data for status
		5 + 4 // 2^8 msec
	},
	/* Data Class Interface with 0 endpoints */
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,
		USB_INTERFACE_DESCRIPTOR_TYPE,
		0x01,	// interface number 1
		0x00,	// alt 0
		0x00,	// no endpoints
		USB_CDC_CLASS_DATA,
		0,
		0,
		0
	},
	/* Real Data Class interface with 2 endpoints (IF1) */
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,
		USB_INTERFACE_DESCRIPTOR_TYPE,
		0x01,	// interface 1
		0x01,	// alt 1
		0x02,	// 2 eps
		USB_CDC_CLASS_DATA,
		0,
		0,
		0
	},
	/* EP0 */
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,
		USB_ENDPOINT_DESCRIPTOR_TYPE,
		0x82,
		0x02, // bulk
		USB_INI16(512),
		0x0B	// NAK rate
	},
	/* EP1 */
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,
		USB_ENDPOINT_DESCRIPTOR_TYPE,
		0x03,
		0x02, // bulk
		USB_INI16(512),
		0x0B,	// NAK rate
	},
#if 0
	/* Video Interface Association Descriptor */
	{
		USB_INTERFACE_ASSOCIATION_DESCRIPTOR_LENGTH,
		USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE,
		0x02,
		0x02,	// one interface for control and one for streaming
		0x0E,	// Video Interface class code
		0x03,	// SC_VIDEO_INTERFACE_COLLECTION
		0x00,	// no protocol
		0x02	// no string
	},
	/* Video Control interface (IF2)*/
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,
		USB_INTERFACE_DESCRIPTOR_TYPE,
		0x02,	// IF 2
		0x00,   // alt 0
		0x01,	// 1 video status interrupt for now
		0x0E,	// CC_VIDEO
		0x01,	// SC_VIDEOCONTROL
		0x00,	// no protocol
		0x02	// no string
	},
	/* Class-specific VC interface descriptor */
	{
		USB_VC_INTERFACE_HEADER_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		USB_VC_INTERFACE_HEADER_DESCRIPTOR_TYPE,
		USB_INI16(0x0100), // Video Class 1.0
		USB_INI16((USB_VC_INTERFACE_HEADER_DESCRIPTOR_LENGTH + USB_VC_INPUT_TERMINAL_DESCRIPTOR_LENGTH + USB_VC_OUTPUT_TERMINAL_DESCRIPTOR_LENGTH)),
		USB_INI32(0x005B8D80), // Deprecated
		0x01, // 1 streaming IF
		0x01 // vid stream IF 1 - MAY NEED TO CHANGE
	},
	/* Input terminal descriptor */
	{
		USB_VC_INPUT_TERMINAL_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		USB_VC_INPUT_TERMINAL_DESCRIPTOR_TYPE,
		0x01, // ID of input terminal
		USB_INI16(0x0200), // ITT_VENDOR_SPECIFIC
		0x00, // No assoc
		0x00 // no string
	},
	/* Output terminal descriptor */
	{
		USB_VC_OUTPUT_TERMINAL_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		USB_VC_OUTPUT_TERMINAL_DESCRIPTOR_TYPE,
		0x02, // ID of output terminal
		USB_INI16(0x0101), // TT_STREAMING
		0x00, // No association
		0x01, // Input terminal connected directly to this output terminal
		0x00 // no string
	},
	/* Video status interrupt endpoint */
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,
		USB_ENDPOINT_DESCRIPTOR_TYPE,
		0x84, // ep4 in
		0x03, // interrupt
		USB_INI16(16),
		0x06,
	},
	/* Class specific interrupt endpoint descriptor */
	{
		USB_VC_INTERRUPT_ENDPOINT_DESCRIPTOR_LENGTH,
		CS_ENDPOINT,
		0x03, // EP_INTERRUPT
		USB_INI16(16) // wMaxTransferSize
	},

	/* Video Stream interface alt-0 */
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,
		USB_INTERFACE_DESCRIPTOR_TYPE,
		0x03, // IF 3
		0x00, // alt 0
		0x00, // 0 endpoints
		0x0E, // CC_VIDEO
		0x02, // SC_VIDEOSTREAMING
		0x00, // PC_PROTOCOL_UNDEFINED
		0x00 // no string
	},
	/* Video stream interface input header descriptor */
	{
		USB_VS_INTERFACE_INPUT_HEADER_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		USB_VS_INTERFACE_INPUT_HEADER_DESCRIPTOR_TYPE,
		0x01, // 1 format
		USB_INI16((USB_VS_INTERFACE_INPUT_HEADER_DESCRIPTOR_LENGTH + USB_VS_MPEG2_TS_FORMAT_DESCRIPTOR_LENGTH)),
		0x85, // Bulk in endpoint address
		0x00, // no dynamic format change supported
		0x02, // this vs interface supplies terminal ID 2 (output terminal)
		0x00, // no still image capture support
		0x00, // no trigger support
		0x00, // ignored as no trigger support
		0x01, // 1 byte control fields
		0x00 // no specific controls supported
	},
	/* MPEG2 TS format descriptor */
	{
		USB_VS_MPEG2_TS_FORMAT_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		USB_VS_MPEG2_TS_FORMAT_DESCRIPTOR_TYPE,
		0x01, // first and only format descriptor
		0x00, // no stride
		188, // bPacketLength
		188, // bStrideLength
		//{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } // guidStrideFormat (no stride for now)
	},

	/* Video streaming alternate interface */
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,
		USB_INTERFACE_DESCRIPTOR_TYPE,
		0x03, // IF 3
		0x01, // alt 1
		0x01, // 1 endpoint
		0x0E, // CC_VIDEO
		0x02, // SC_VIDEOSTREAMING
		0x00, // PC_PROTOCOL_UNDEFINED
		0x00 // noString
	},
	/* Bulk in video ep */
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,
		USB_ENDPOINT_DESCRIPTOR_TYPE,
		0x85, // ep 5 in
		0x02, // bulk
		USB_INI16(512), // maxPacketSize
		0x0B // Nake rate
	}
#endif
};

USBD_CONFIGURATION_T	IMG_sEthVidConfigurations =
{
	{
		&IMG_sEthVidConfigurationDescriptor_HS,
		0
	},											/* apsConfigsFullSpeed */
	{
		&IMG_sEthVidConfigurationDescriptor_HS,
		0
	},											/* apsConfigsHighSpeed */
	1											/* the number of configurations available */
};

/******************************************************************************
**************************** RNDIS ETH/VID CONFIG *****************************
*******************************************************************************/

USB_DEVICE_DESCRIPTOR	IMG_sRNDISEthVidDeviceDescriptor __USBD_ATTR__ IMG_ALIGN(4) =
{
	USB_DEVICE_DESCRIPTOR_LENGTH,
	USB_DEVICE_DESCRIPTOR_TYPE,
	USB_INI16(0x0200),
	0xEF,	// Multifunction class code for IADs
	0x02,	// ""
	0x01,	// ""
	64,
	USB_INI16(0x0000),
	USB_INI16(0x0000),
	USB_INI16(0x0000),
	0x00,
	0x00,
	0x00,
	0x01
};

typedef struct usb_interface_association_descriptor_t
{
	IMG_UINT8	bLength;
	IMG_UINT8	bDescriptorType;
	IMG_UINT8	bFirstInterface;
	IMG_UINT8	bInterfaceCount;
	IMG_UINT8	bFunctionClass;
	IMG_UINT8	bFunctionSubClass;
	IMG_UINT8	bFunctionProtocol;
	IMG_UINT8	iFunction;
}__ATTR_PACKED__ USB_INTERFACE_ASSOCIATION_DESCRIPTOR;

#define USB_INTERFACE_ASSOCIATION_DESCRIPTOR_LENGTH		(8)
#define USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE		(11)

/* ACM functional descriptor */
typedef struct usb_acm_functional_descriptor_t
{
	IMG_UINT8	bLength;
	IMG_UINT8	bDescriptorType;
	IMG_UINT8	bDescriptorSubtype;
	IMG_UINT8	bmCapabilities;
}__ATTR_PACKED__ ACM_FUNCTIONAL_DESCRIPTOR;

#define CDC_ACM_TYPE						(0x02)
#define ACM_FUNCTIONAL_DESCRIPTOR_LENGTH	(0x04)

/* Call Management functional descriptor */
typedef struct usb_call_management_functional_descriptor_t
{
	IMG_UINT8	bLength;
	IMG_UINT8	bDescriptorType;
	IMG_UINT8	bDescriptorSubtype;
	IMG_UINT8	bmCapabilities;
	IMG_UINT8	bDataInterface;
}__ATTR_PACKED__ CM_FUNCTIONAL_DESCRIPTOR;

#define CDC_CM_TYPE							(0x01)
#define CM_FUNCTIONAL_DESCRIPTOR_LENGTH		(0x05)

//////////////////// VIDEO STRUCTS //////////////////////////////////
typedef struct usb_vc_interface_header_descriptor_t
{
	IMG_UINT8	bLength;
	IMG_UINT8	bDescriptorType;
	IMG_UINT8	bDescriptorSubtype;
	IMG_UINT8	bcdUVC[2];
	IMG_UINT8	wTotalLength[2];
	IMG_UINT8	dwClockFrequency[4];
	IMG_UINT8	bInCollection;
	IMG_UINT8	bnInterfaceNr;	// Just one Video Streaming IF for now
}__ATTR_PACKED__ USB_VC_INTERFACE_HEADER_DESCRIPTOR;

#define USB_VC_INTERFACE_HEADER_DESCRIPTOR_LENGTH		(13)
#define USB_VC_INTERFACE_HEADER_DESCRIPTOR_TYPE			(0x01)

typedef struct usb_vc_input_terminal_descriptor_t
{
	IMG_UINT8	bLength;
	IMG_UINT8	bDescriptorType;
	IMG_UINT8	bDescriptorSubtype;
	IMG_UINT8	bTerminalID;
	IMG_UINT8	wTerminalType[2];
	IMG_UINT8	bAssocTerminal;
	IMG_UINT8	iTerminal;
}__ATTR_PACKED__ USB_VC_INPUT_TERMINAL_DESCRIPTOR;

#define USB_VC_INPUT_TERMINAL_DESCRIPTOR_LENGTH			(8)
#define USB_VC_INPUT_TERMINAL_DESCRIPTOR_TYPE			(0x02)

typedef struct usb_vc_output_terminal_descriptor_t
{
	IMG_UINT8	bLength;
	IMG_UINT8	bDescriptorType;
	IMG_UINT8	bDescriptorSubtype;
	IMG_UINT8	bTerminalID;
	IMG_UINT8	wTerminalType[2];
	IMG_UINT8	bAssocTerminal;
	IMG_UINT8	bSourceID;
	IMG_UINT8	iTerminal;
}__ATTR_PACKED__ USB_VC_OUTPUT_TERMINAL_DESCRIPTOR;

#define USB_VC_OUTPUT_TERMINAL_DESCRIPTOR_LENGTH		(9)
#define USB_VC_OUTPUT_TERMINAL_DESCRIPTOR_TYPE			(0x03)

// Class-specific VC Interrupt Endpoint Descriptor
typedef struct usb_vc_interrupt_endpoint_descriptor_t
{
	IMG_UINT8	bLength;
	IMG_UINT8	bDescriptorType;
	IMG_UINT8	bDescriptorSubtype;
	IMG_UINT8	wMaxTransferSize[2];
}__ATTR_PACKED__ USB_VC_INTERRUPT_ENDPOINT_DESCRIPTOR;

#define USB_VC_INTERRUPT_ENDPOINT_DESCRIPTOR_LENGTH		(5)

typedef struct usb_vs_interface_input_header_descriptor_t
{
	IMG_UINT8	bLength;
	IMG_UINT8	bDescriptorType;
	IMG_UINT8	bDescriptorSubtype;
	IMG_UINT8	bNumFormats;
	IMG_UINT8	wTotalLength[2];
	IMG_UINT8	bEndpointAddress;
	IMG_UINT8	bmInfo;
	IMG_UINT8	bTerminalLink;
	IMG_UINT8	bStillCaptureMethod;
	IMG_UINT8	bTriggerSupport;
	IMG_UINT8	bTriggerUSage;
	IMG_UINT8	bControlSize;
	IMG_UINT8	bmaControls;
}__ATTR_PACKED__ USB_VS_INTERFACE_INPUT_HEADER_DESCRIPTOR;

#define USB_VS_INTERFACE_INPUT_HEADER_DESCRIPTOR_LENGTH	(14)
#define USB_VS_INTERFACE_INPUT_HEADER_DESCRIPTOR_TYPE	(0x01)

typedef struct usb_vs_interface_output_header_descriptor_t
{
	IMG_UINT8	bLength;
	IMG_UINT8	bDescriptorType;
	IMG_UINT8	bDescriptorSubtype;
	IMG_UINT8	bNumFormats;
	IMG_UINT8	wTotalLength[2];
	IMG_UINT8	bEndpointAddress;
	IMG_UINT8	bTerminalLink;
	IMG_UINT8	bContorlSize;
	IMG_UINT8	bmaControls;
}__ATTR_PACKED__ USB_VS_INTERFACE_OUTPUT_HEADER_DESCRIPTOR;

#define USB_VS_INTERFACE_OUTPUT_HEADER_DESCRIPTOR_LENGTH	(10)
#define USB_VS_INTERFACE_OUTPUT_HEADER_DESCRIPTOR_TYPE		(0x02)

typedef struct usb_mpeg2_ts_format_descriptor_t
{
	IMG_UINT8	bLength;
	IMG_UINT8	bDescriptorType;
	IMG_UINT8	bDescriptorSubtype;
	IMG_UINT8	bFormatIndex;
	IMG_UINT8	bDataOffset;
	IMG_UINT8	bPacketLength;
	IMG_UINT8	bStrideLength;
//	IMG_UINT8	guidStrideFormat[16];
}__ATTR_PACKED__ USB_VS_MPEG2_TS_FORMAT_DESCRIPTOR;

#define USB_VS_MPEG2_TS_FORMAT_DESCRIPTOR_LENGTH			(sizeof(USB_VS_MPEG2_TS_FORMAT_DESCRIPTOR))
#define USB_VS_MPEG2_TS_FORMAT_DESCRIPTOR_TYPE				(0x0A)

// RNDIS version with Video woop
typedef struct configuration_descriptor_rndis_eth_vid_t
{
	USB_CONFIG_DESCRIPTOR						sConfigDesc;
	
	// Ethernet
	USB_INTERFACE_ASSOCIATION_DESCRIPTOR		sEthIAD;
	// CDC ACM control descriptors
	USB_INTERFACE_DESCRIPTOR					sECMControl;		// IF 0
	HEADER_FUNCTIONAL_DESCRIPTOR				sHdrDesc;
	CM_FUNCTIONAL_DESCRIPTOR					sCMDesc;
	ACM_FUNCTIONAL_DESCRIPTOR					sACMDesc;
	UNION_FUNCTIONAL_DESCRIPTOR					sUnionDesc;
	// Status endpoint
	USB_ENDPOINT_DESCRIPTOR						sNotifyEP;
	USB_INTERFACE_DESCRIPTOR					sDataInterface;		// IF 1
	USB_ENDPOINT_DESCRIPTOR						sEpIn;
	USB_ENDPOINT_DESCRIPTOR						sEpOut;

	// Video
	USB_INTERFACE_ASSOCIATION_DESCRIPTOR		sVidIAD;
	
	USB_INTERFACE_DESCRIPTOR					sVidControl;
	USB_VC_INTERFACE_HEADER_DESCRIPTOR			sVCIHeader;
	USB_VC_INPUT_TERMINAL_DESCRIPTOR			sVCInputTerminal;
	USB_VC_OUTPUT_TERMINAL_DESCRIPTOR			sVCIOutputTerminal;
	USB_ENDPOINT_DESCRIPTOR						sVidStatusEP;
	USB_VC_INTERRUPT_ENDPOINT_DESCRIPTOR		sClassVidStatusEP;

	USB_INTERFACE_DESCRIPTOR					sVidStream;
	USB_VS_INTERFACE_INPUT_HEADER_DESCRIPTOR	sVSIInputHeader;
	USB_VS_MPEG2_TS_FORMAT_DESCRIPTOR			sMpeg2Format;

	USB_INTERFACE_DESCRIPTOR					sVidStreamAlt;
	USB_ENDPOINT_DESCRIPTOR						sVidInEP;

} __ATTR_PACKED__ CONFIGURATION_DESCRIPTOR_RNDIS_ETH_VID;

#define CONF_DESCRIPTOR_ETH_VID_LENGTH	sizeof(CONFIGURATION_DESCRIPTOR_ETH_VID)

/* High speed version */
static CONFIGURATION_DESCRIPTOR_RNDIS_ETH_VID IMG_sRNDISEthVidConfigurationDescriptor_HS __USBD_ATTR__ IMG_ALIGN(4) =
{
	/* Configuration descriptor */
	{
		USB_CONFIGURATION_DESCRIPTOR_LENGTH,
		USB_CONFIGURATION_DESCRIPTOR_TYPE,
		USB_INI16(CONF_DESCRIPTOR_ETH_VID_LENGTH),
		//0x02, // 2 interfaces for now
		//0x03, // 3 interfaces for now
		0x04, // 4 interfaces for now
		0x01, // configuration value
		0x00, // no string index
		0xC0,
		0x00
	},
	/* Ethernet Interface Association Descriptor */
	{
		USB_INTERFACE_ASSOCIATION_DESCRIPTOR_LENGTH,
		USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE,
		0x00, // bFirstInterface
		0x02, // 2 contiguous interfaces
		0x02, // class for CDC
		0x02, // subclass for ACM
		0xFF, // vendor specific protocol
		0x00  // no string
	},
	/* Communication Class Interface descriptor */
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,
		USB_INTERFACE_DESCRIPTOR_TYPE,
		0x00, // interface number
		0x00, // alt setting
		0x01, // num endpoints
		0x02, // Communications Interface Class code
		0x02, // ACM code
		0xFF, // Vendor specific protocol
		0x00
	},
	/* Header functional descriptor */
	{
		HEADER_FUNCTIONAL_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		CDC_HEADER_TYPE,
		USB_INI16(0x0120)
	},
	/* Call Management functional descriptor */
	{
		CM_FUNCTIONAL_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		CDC_CM_TYPE,
		0x00, // bmCapabilities - no call management functionality
		0x01, // bInterfaceNUmber of Data Class Interface
	},
	/* ACM functional descriptor */
	{
		ACM_FUNCTIONAL_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		CDC_ACM_TYPE,
		0x00, // bmCapabilities
	},
	/* Union functional descriptor */
	{
		UNION_FUNCTIONAL_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		CDC_UNION_TYPE,
		0x0,
		0x1
	},
	/* Notify endpoint */
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,
		USB_ENDPOINT_DESCRIPTOR_TYPE,
		0x81, // ep1 in
		0x03, // interrupt
		USB_INI16(8), // 8 bytes for RNDIS
		1
	},
	/* Real Data Class interface with 2 endpoints (IF1) */
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,
		USB_INTERFACE_DESCRIPTOR_TYPE,
		0x01,	// interface 1
		0x00,
		0x02,	// 2 eps
		USB_CDC_CLASS_DATA,
		0,
		0,
		0
	},
	/* EP0 */
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,
		USB_ENDPOINT_DESCRIPTOR_TYPE,
		0x82,
		0x02, // bulk
		USB_INI16(64),
		0x0B	// NAK rate
	},
	/* EP1 */
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,
		USB_ENDPOINT_DESCRIPTOR_TYPE,
		0x03,
		0x02, // bulk
		USB_INI16(64),
		0x0B,	// NAK rate
	},
	/* Video Interface Association Descriptor */
	{
		USB_INTERFACE_ASSOCIATION_DESCRIPTOR_LENGTH,
		USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE,
		0x02,
		0x02,	// one interface for control and one for streaming
		0x0E,	// Video Interface class code
		0x03,	// SC_VIDEO_INTERFACE_COLLECTION
		0x00,	// no protocol
		0x02	// no string
	},
	/* Video Control interface (IF2)*/
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,
		USB_INTERFACE_DESCRIPTOR_TYPE,
		0x02,	// IF 2
		0x00,   // alt 0
		0x01,	// 1 video status interrupt for now
		0x0E,	// CC_VIDEO
		0x01,	// SC_VIDEOCONTROL
		0x00,	// no protocol
		0x02	// no string
	},
	/* Class-specific VC interface descriptor */
	{
		USB_VC_INTERFACE_HEADER_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		USB_VC_INTERFACE_HEADER_DESCRIPTOR_TYPE,
		USB_INI16(0x0100), // Video Class 1.0
		USB_INI16((USB_VC_INTERFACE_HEADER_DESCRIPTOR_LENGTH + USB_VC_INPUT_TERMINAL_DESCRIPTOR_LENGTH + USB_VC_OUTPUT_TERMINAL_DESCRIPTOR_LENGTH)),
		USB_INI32(0x005B8D80), // Deprecated
		0x01, // 1 streaming IF
		0x01 // vid stream IF 1 - MAY NEED TO CHANGE
	},
	/* Input terminal descriptor */
	{
		USB_VC_INPUT_TERMINAL_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		USB_VC_INPUT_TERMINAL_DESCRIPTOR_TYPE,
		0x01, // ID of input terminal
		USB_INI16(0x0200), // ITT_VENDOR_SPECIFIC
		0x00, // No assoc
		0x00 // no string
	},
	/* Output terminal descriptor */
	{
		USB_VC_OUTPUT_TERMINAL_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		USB_VC_OUTPUT_TERMINAL_DESCRIPTOR_TYPE,
		0x02, // ID of output terminal
		USB_INI16(0x0101), // TT_STREAMING
		0x00, // No association
		0x01, // Input terminal connected directly to this output terminal
		0x00 // no string
	},
	/* Video status interrupt endpoint */
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,
		USB_ENDPOINT_DESCRIPTOR_TYPE,
		0x84, // ep4 in
		0x03, // interrupt
		USB_INI16(16),
		0x06,
	},
	/* Class specific interrupt endpoint descriptor */
	{
		USB_VC_INTERRUPT_ENDPOINT_DESCRIPTOR_LENGTH,
		CS_ENDPOINT,
		0x03, // EP_INTERRUPT
		USB_INI16(16) // wMaxTransferSize
	},

	/* Video Stream interface alt-0 */
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,
		USB_INTERFACE_DESCRIPTOR_TYPE,
		0x03, // IF 3
		0x00, // alt 0
		0x00, // 0 endpoints
		0x0E, // CC_VIDEO
		0x02, // SC_VIDEOSTREAMING
		0x00, // PC_PROTOCOL_UNDEFINED
		0x00 // no string
	},
	/* Video stream interface input header descriptor */
	{
		USB_VS_INTERFACE_INPUT_HEADER_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		USB_VS_INTERFACE_INPUT_HEADER_DESCRIPTOR_TYPE,
		0x01, // 1 format
		USB_INI16((USB_VS_INTERFACE_INPUT_HEADER_DESCRIPTOR_LENGTH + USB_VS_MPEG2_TS_FORMAT_DESCRIPTOR_LENGTH)),
		0x85, // Bulk in endpoint address
		0x00, // no dynamic format change supported
		0x02, // this vs interface supplies terminal ID 2 (output terminal)
		0x00, // no still image capture support
		0x00, // no trigger support
		0x00, // ignored as no trigger support
		0x01, // 1 byte control fields
		0x00 // no specific controls supported
	},
	/* MPEG2 TS format descriptor */
	{
		USB_VS_MPEG2_TS_FORMAT_DESCRIPTOR_LENGTH,
		CS_INTERFACE,
		USB_VS_MPEG2_TS_FORMAT_DESCRIPTOR_TYPE,
		0x01, // first and only format descriptor
		0x00, // no stride
		188, // bPacketLength
		188, // bStrideLength
		//{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } // guidStrideFormat (no stride for now)
	},

	/* Video streaming alternate interface */
	{
		USB_INTERFACE_DESCRIPTOR_LENGTH,
		USB_INTERFACE_DESCRIPTOR_TYPE,
		0x03, // IF 3
		0x01, // alt 1
		0x01, // 1 endpoint
		0x0E, // CC_VIDEO
		0x02, // SC_VIDEOSTREAMING
		0x00, // PC_PROTOCOL_UNDEFINED
		0x00 // noString
	},
	/* Bulk in video ep */
	{
		USB_ENDPOINT_DESCRIPTOR_LENGTH,
		USB_ENDPOINT_DESCRIPTOR_TYPE,
		0x85, // ep 5 in
		0x02, // bulk
		USB_INI16(512), // maxPacketSize
		0x0B // Nake rate
	}
};

USBD_CONFIGURATION_T	IMG_sRNDISEthVidConfigurations =
{
	{
		&IMG_sRNDISEthVidConfigurationDescriptor_HS,
		0
	},											/* apsConfigsFullSpeed */
	{
		&IMG_sRNDISEthVidConfigurationDescriptor_HS,
		0
	},											/* apsConfigsHighSpeed */
	1											/* the number of configurations available */
};

#endif /* __USBD_ETH_VID_DESCRIPTORS_H__ */
