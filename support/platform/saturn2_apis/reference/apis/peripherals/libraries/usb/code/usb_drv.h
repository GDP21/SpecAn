

// Common header file


#if !defined __USB_DRV_H__
#define __USB_DRV_H__

#include <img_defs.h>
#include <usb_config.h>
#include <usb_spec.h>

/* These are externally defined types */
struct dwc_otg_dev_if;
struct dwc_otg_pcd;
struct dwc_otg_core_if;
struct dwc_otg_pcd_ep;
struct dwc_otg_pcd_request;

/* Logical description of the interfaces and configurations used by the driver */ 

/* Interfaces currently supported can have a maximum of USBD_MAX_INTERFACE_EP_PAIRS in and out endpoints each */
typedef struct usbd_logical_interface_info
{
	USB_INTERFACE_DESCRIPTOR		*psDescriptor;								/* Pointer to the inteface descriptor */
	USB_ENDPOINT_DESCRIPTOR			*psDataInEp[USBD_MAX_INTERFACE_EP_PAIRS];	/* Array of pointers to IN endpoint descriptors */
	USB_ENDPOINT_DESCRIPTOR			*psDataOutEp[USBD_MAX_INTERFACE_EP_PAIRS];	/* Array of pointer to OUT endpoint descriptor */
} USBD_LOGICAL_INTERFACE_INFO;


typedef struct usbd_logical_configuration_info
{
	USB_CONFIG_DESCRIPTOR			*psDescriptor;								/* Pointer to the configuration descriptor */
	USBD_LOGICAL_INTERFACE_INFO		asIfDefault[USBD_MAX_INTERFACES];			/* Array of default interfaces */
	USBD_LOGICAL_INTERFACE_INFO		asIfAlternate[USBD_MAX_INTERFACES];			/* Array of alternate interfaces */
} USBD_LOGICAL_CONFIGURATION_INFO;


typedef struct usbd_logical_device_info
{
	USBD_LOGICAL_CONFIGURATION_INFO 	asConfigsFullSpeed[USBD_MAX_CONFIGURATIONS];	/* FS settings for all the available configurations */
	USBD_LOGICAL_CONFIGURATION_INFO 	asConfigsHighSpeed[USBD_MAX_CONFIGURATIONS];	/* HS settings for the configurations */
	IMG_UINT8							ui8NumberOfConfigurations;						/* the number of configurations available */
} USBD_LOGICAL_DEVICE_INFO;

/* USBD Device context */
typedef struct usbd_dc_t
{
	struct dwc_otg_pcd 				sOtgPcd;				/* Pcd context */
	struct dwc_otg_dev_if 		*	psOtgDevIf;				/* Pointer to device IF context */
		
	IMG_UINT8						ui8SelfPowered;			/* Indicates if the device is self powered */
	USBD_STATE						eState;					/* Inidcates current device state */

	IMG_UINT8						bInsideISR;				/* Set when the driver is inside the ISR */

	IMG_UINT32						uiRegisteredEvents;		/* Bitmap of registered callback events */
	IMG_VOID						(*pfCallback)(USBD_EVENT_CALLBACK_DATA_T);	/* Pointer to the API callback function */
	
	img_uint32						ui32CallbackSlot;		/*! Callback slot for Callback Manager */

	struct dwc_otg_pcd_ep *			psEp[ USBD_MAX_EPS ];
	img_uint32						ui32EpsEnabled;

	struct dwc_otg_pcd_request 		sCtrlRequest;					/* Pointer to a request object to use in control transfers */
	IMG_BOOL						bLastControlRequestUnhandledByDriver;

	/* Enumeration information */
	IMG_UINT8 **					ppcStrings;						/* Pointer to an enumeration strings struct. Set up during device initialisation */
	
	USB_DEVICE_DESCRIPTOR		*	psDeviceDescriptor;							/* Pointer to the device descriptor given by the application */
	USB_QUALIFIER_DESCRIPTOR	*	psQualifierDescriptor;						/* Pointer to the device qualifier descriptor given by the application */
	USBD_LOGICAL_DEVICE_INFO		sLogicalDeviceInfo;							/* Logical device information */
	IMG_UINT8						ui8CurrentConfigurationNumber;				/* Current configuration */
	IMG_UINT32						abAlternateIfSetting[USBD_MAX_INTERFACES];	/* Current setting on each of the interfaces */
	img_uint32						ui32MaxConfigInEPs;							/* Maximum number of IN endpoints for any configuration given to the API */

	IMG_BOOL						bReqZero;
	
	KRN_TASKQ_T						hibernateQ;				/* Hibernate queue */

} USBD_DC_T;

/* Global Device Context */
typedef struct usb_dc_t
{
	img_bool						bInitialised;

	struct dwc_otg_core_if			sOtgCoreIf;
	
	USBD_DC_T						sDevContext;

	struct tag_sUSBTransferMemory * psTransferMem;

	USBD_sEP						asUSBDEP[ USBD_MAX_EPS ];		/* USBD API EPs */

#if !defined DWC_DEVICE_ONLY
	USBHC_DC_T						sHostContext;
#endif

} USB_DC_T;

#endif /* __USB_DRV_H__ */
