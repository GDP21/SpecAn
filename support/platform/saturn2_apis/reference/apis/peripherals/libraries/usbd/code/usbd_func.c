/*!
*******************************************************************************
 @file   usbd_func.c

 @brief  USBD driver control functions.


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

#if defined __META_MEOS__ || defined __MTX_MEOS__ || defined __META_NUCLEUS_PLUS__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <MeOS.h>

#include "img_defs.h"
#include "ioblock_defs.h"

#include "usbd_api.h"
#include "usb_spec.h"
#include "usb_hal.h"
#include "dwc_otg_pcd.h"
#include "usb_defs.h"

#if !defined (USBD_NO_CBMAN_SUPPORT)
	#include "cbman.h"
#endif

// minimum and maximum macros
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#define min(a,b)  (((a) < (b)) ? (a) : (b))

/*-------------------------------------------------------------------------*/

static IMG_VOID usbd_ResetConfig ( const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	USB_DC_T	*	psContext	 = (USB_DC_T *)psBlockDesc->pvAPIContext;
	USBD_DC_T	*	psDevContext = &(psContext->sDevContext);
	img_bool		bActive = IMG_FALSE;
	img_uint32		i;
	/* We are about to close the I/O, so notify the caller if required */
	for ( i = 0; i < USBD_MAX_EPS; ++i )
	{
		if ( psDevContext->psEp[i] )
		{
			bActive = IMG_TRUE;
			break;
		}
	}
	if ( bActive )
	{
	  #if defined (USBD_NO_CBMAN_SUPPORT)
		usbd_EventNotify( psContext, USBD_EVENT_IO_CLOSED );

	  #else	
		/* Call application callback */
		CBMAN_ExecuteCallbackWithEventType (psDevContext->ui32CallbackSlot,
											USBD_EVENT_IO_CLOSED,
											0,
											IMG_NULL);		
	  #endif	
	}

	/* just disable endpoints, forcing completion of pending i/o.
	 * all our completion handlers free their requests in this case.
	 */
	for ( i = 0; i < USBD_MAX_EPS; ++i )
	{
		if (psDevContext->psEp[i]) 
		{
			dwc_otg_pcd_ep_disable( psBlockDesc, psDevContext->psEp[i] );
			psDevContext->psEp[i] = IMG_NULL;
		}
	}

	psDevContext->ui32EpsEnabled = 0;

	return;
}

static IMG_INT32 usbd_ActivateInterface(	const ioblock_sBlockDescriptor	*	psBlockDesc, 
											USB_DC_T						*	psContext,
											USBD_LOGICAL_INTERFACE_INFO		*	psInterface, 
											IMG_BOOL						*	pbEndpointsOpened )
{
	USBD_DC_T				*	psDevContext = &psContext->sDevContext;
	img_uint32				i;
	img_int32				iResult = 0;
	dwc_otg_pcd_ep_t	*	psEP;
	USBD_sEP			*	psUSBDEP;

	for ( i = 0; i < USBD_MAX_INTERFACE_EP_PAIRS; ++i )
	{
		if ( psInterface->psDataInEp[i] )
		{
			// Make sure we have enough space for another ep
			IMG_ASSERT( psDevContext->ui32EpsEnabled + 1 < USBD_MAX_EPS );
			psDevContext->psEp[ psDevContext->ui32EpsEnabled ] = &psDevContext->sOtgPcd.in_ep[ (psInterface->psDataInEp[i]->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK) - 1 ];
			psEP = psDevContext->psEp[ psDevContext->ui32EpsEnabled ];
			iResult += dwc_otg_pcd_ep_enable( psBlockDesc, psEP, psInterface->psDataInEp[i] );
			// Configure USBD_sEP endpoint
			psUSBDEP = &psContext->asUSBDEP[ psEP->dwc_ep.num - 1 ];
			psUSBDEP->bCancel		= IMG_FALSE;
			psUSBDEP->bIsIn			= psEP->dwc_ep.is_in;
			psUSBDEP->pvEP			= (img_void *)psEP;
			// For endpoints, we store the endpoint number in the upper 16 bits of the device id.
			// This is used in USBD_Cancel
			psUSBDEP->sDevice.id = (psEP->dwc_ep.num << 16) | (psBlockDesc->ui32Index & 0x0000FFFF);
			psUSBDEP->bInitialised	= IMG_TRUE;
			
			*pbEndpointsOpened = IMG_TRUE;
			++psDevContext->ui32EpsEnabled;
		}

		if ( psInterface->psDataOutEp[i] )
		{
			// Make sure we have enough space for another ep
			IMG_ASSERT( psDevContext->ui32EpsEnabled + 1 < USBD_MAX_EPS );
			psDevContext->psEp[ psDevContext->ui32EpsEnabled ] = &psDevContext->sOtgPcd.out_ep[ (psInterface->psDataOutEp[i]->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK) - 1 ];
			psEP = psDevContext->psEp[ psDevContext->ui32EpsEnabled ];
			iResult += dwc_otg_pcd_ep_enable( psBlockDesc, psDevContext->psEp[ psDevContext->ui32EpsEnabled ], psInterface->psDataOutEp[i] );
			// Configure USBD_sEP endpoint
			psUSBDEP = &psContext->asUSBDEP[ psEP->dwc_ep.num - 1 ];
			psUSBDEP->bCancel		= IMG_FALSE;
			psUSBDEP->bIsIn			= psEP->dwc_ep.is_in;
			psUSBDEP->pvEP			= (img_void *)psEP;
			// For endpoints, we store the endpoint number in the upper 16 bits of the device id.
			// This is used in USBD_Cancel
			psUSBDEP->sDevice.id = (psEP->dwc_ep.num << 16) | (psBlockDesc->ui32Index & 0x0000FFFF);
			psUSBDEP->bInitialised	= IMG_TRUE;
			
			*pbEndpointsOpened = IMG_TRUE;
			++psDevContext->ui32EpsEnabled;
		}
	}

	return iResult;
}

/* 
	Set the current config
*/
static IMG_INT32 usbd_SetConfig ( const ioblock_sBlockDescriptor	*	psBlockDesc, IMG_UINT8  ui8Number )
{
	USB_DC_T						*	psContext			= (USB_DC_T *)psBlockDesc->pvAPIContext;
	IMG_BOOL							bEndpointsOpened	= IMG_FALSE;
	IMG_INT32							iResult				= 0;
	USBD_DC_T						*	psDevContext		= &(psContext->sDevContext);
	USBD_LOGICAL_DEVICE_INFO		*	psDeviceInfo		= &psDevContext->sLogicalDeviceInfo;
	USBD_LOGICAL_CONFIGURATION_INFO *	psCurrentConfigurationInfo; 
	IMG_UINT32							i;

	/* Reset config to start with */
	usbd_ResetConfig ( psBlockDesc );

	if (ui8Number == 0)
	{
		psDevContext->ui8CurrentConfigurationNumber = 0;
	}
	else
	{
		if (ui8Number <= psDeviceInfo->ui8NumberOfConfigurations)
		{
			/* Is this a brand new configuration?, else the call has been made to change the interface */
			if (psDevContext->ui8CurrentConfigurationNumber != ui8Number)
			{
				/* Set the interfaces to 0 (default) interface */
				for ( i = 0; i < USBD_MAX_INTERFACES; ++i )
				{
					psDevContext->abAlternateIfSetting[i] = 0;
				}

				psDevContext->ui8CurrentConfigurationNumber = ui8Number;
			}
		}
		else
		{
			IMG_ASSERT(0);
			iResult = -1;
		}
		
		/* Choose the configuration for the currently enumerated device speed */
		if (psDevContext->sOtgPcd.speed == USB_SPEED_HIGH)
		{
			psCurrentConfigurationInfo = &psDeviceInfo->asConfigsHighSpeed[ui8Number-1];
		}
		else
		{
			psCurrentConfigurationInfo = &psDeviceInfo->asConfigsFullSpeed[ui8Number-1];
		}

		/* Go through every default interface and active it */
		for ( i = 0; i < USBD_MAX_INTERFACES; ++i )
		{
			USBD_LOGICAL_INTERFACE_INFO		* psCurrentInterface;
		
			psCurrentInterface = &psCurrentConfigurationInfo->asIfDefault[i];

			if ( psCurrentInterface->psDescriptor )
			{
				iResult = usbd_ActivateInterface( psBlockDesc, psContext, psCurrentInterface, &bEndpointsOpened );
			}
		}

		if (iResult != 0)
		{
			usbd_ResetConfig( psBlockDesc );
			psDevContext->ui8CurrentConfigurationNumber = 0;
			IMG_ASSERT(0); 
		}
	}

	/* Notify the caller of the opening of I/O */
	if (bEndpointsOpened)
	{
	  #if defined (USBD_NO_CBMAN_SUPPORT)
		usbd_EventNotify ( psContext, USBD_EVENT_IO_OPENED );
	  #else
		/* Call application callback */
		CBMAN_ExecuteCallbackWithEventType (psDevContext->ui32CallbackSlot,
											USBD_EVENT_IO_OPENED,
											0,
											IMG_NULL);
	  #endif
	}


	return iResult;
}

static IMG_INT32 usbd_SetInterface( const ioblock_sBlockDescriptor	*	psBlockDesc, IMG_UINT16	ui16Interface )
{
	USB_DC_T						*	psContext		= (USB_DC_T *)psBlockDesc->pvAPIContext;
	IMG_INT32							iResult			= 0;
	USBD_DC_T						*	psDevContext	= &psContext->sDevContext;
	USBD_LOGICAL_DEVICE_INFO		*	psDeviceInfo	= &psDevContext->sLogicalDeviceInfo;
	USBD_LOGICAL_CONFIGURATION_INFO	*	psCurrentConfigInfo;
	USBD_LOGICAL_INTERFACE_INFO		*	psCurrentInterface;
	IMG_BOOL							bEndpointsOpened = IMG_FALSE;

	if ( psDevContext->sOtgPcd.speed == USB_SPEED_HIGH )
	{
		psCurrentConfigInfo = &psDeviceInfo->asConfigsHighSpeed[ psDevContext->ui8CurrentConfigurationNumber - 1 ];
	}
	else
	{
		psCurrentConfigInfo = &psDeviceInfo->asConfigsFullSpeed[ psDevContext->ui8CurrentConfigurationNumber - 1 ];
	}

	IMG_ASSERT( ui16Interface < USBD_MAX_INTERFACES );

	if ( psDevContext->abAlternateIfSetting[ ui16Interface ] )
	{
		psCurrentInterface = &psCurrentConfigInfo->asIfAlternate[ ui16Interface ];
	}
	else
	{
		psCurrentInterface = &psCurrentConfigInfo->asIfDefault[ ui16Interface ];
	}

	IMG_ASSERT( psCurrentInterface );

	iResult = usbd_ActivateInterface( psBlockDesc, psContext, psCurrentInterface, &bEndpointsOpened );
			
	if (iResult != 0)
	{
		usbd_ResetConfig( psBlockDesc );
		psDevContext->ui8CurrentConfigurationNumber = 0;
		IMG_ASSERT(0); 
	}

	/* Notify the caller of the opening of I/O */
	if (bEndpointsOpened)
	{
	  #if defined (USBD_NO_CBMAN_SUPPORT)
		usbd_EventNotify ( psContext, USBD_EVENT_IO_OPENED );
	  #else
		/* Call application callback */
		CBMAN_ExecuteCallbackWithEventType (psDevContext->ui32CallbackSlot,
											USBD_EVENT_IO_OPENED,
											0,
											IMG_NULL);
	  #endif
	}


	return iResult;
}


/*-------------------------------------------------------------------------*/

/* A small scratch buffer used for short transfers */
__USBD_ATTR__ static IMG_ALIGN(4) IMG_UINT8 aui8ScratchBuffer[4];

static IMG_VOID usbd_SetupComplete( const ioblock_sBlockDescriptor	*	psBlockDesc, IMG_VOID * data, struct dwc_otg_pcd_request *req )
{
	if (req->status || req->actual != req->length)
	{
		/* simply assert for now */
		IMG_ASSERT(0);
	}
}


IMG_INT32 usbd_Setup ( const ioblock_sBlockDescriptor	*	psBlockDesc, struct usb_ctrlrequest_t *ctrl )
{
	USB_DC_T					*	psGlobalContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	USBD_DC_T					*	psContext		= &(psGlobalContext->sDevContext);
	USBD_LOGICAL_DEVICE_INFO	*	psDeviceInfo	= &psContext->sLogicalDeviceInfo;	
	dwc_otg_pcd_t				*	pcd				= &(psContext->sOtgPcd);
	IMG_INT32						value			= -USBD_ERR_OPNOTSUPP;
	struct dwc_otg_pcd_request	*	req				= &(psContext->sCtrlRequest);

	/* Set up basic parameters in the ep0 request structure */
	req->complete = usbd_SetupComplete;

	/*	usually this stores reply data in the pre-allocated ep0 buffer,
		but config change events will reconfigure hardware.
	*/
	switch (ctrl->bRequest) 
	{
		case USB_REQ_GET_DESCRIPTOR:
		{
			IMG_UINT8	ui8DescType = (IMG_UINT8)(ctrl->wValue >> 8);

			switch (ui8DescType) 
			{
				case USB_DEVICE_DESCRIPTOR_TYPE:
				{ 
					value = min (ctrl->wLength, (IMG_UINT16) USB_DEVICE_DESCRIPTOR_LENGTH );

					req->buf = psContext->psDeviceDescriptor;
					break;
				}
				case USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE:
				{
					/* If we are a full speed only device, then we don't need to send a device qualifier */
					if (!pcd->is_dualspeed)
					{
						break;
					}
					
					/*	We have chosen the device_qualifier in such a way that it is the same if we 
						are currently operating at high or at full speed. If we need to, we can have
						separate descripors for the two speeds
					*/
					value = min (ctrl->wLength, (IMG_UINT16) USB_QUALIFIER_DESCRIPTOR_LENGTH );

					req->buf = psContext->psQualifierDescriptor;
					break;
				}
				case USB_OTHER_SPEED_CONFIG_DESCRIPTOR_TYPE:
				{
					/* If we are a full speed only device then there is no other speed configuration */
					if (!pcd->is_dualspeed)
					{
						break;
					}
					// FALLTHROUGH
				}
				case USB_CONFIGURATION_DESCRIPTOR_TYPE:
				{
					IMG_UINT8	ui8Index = ctrl->wValue & 0xFF;
		
					if ( ui8Index < psDeviceInfo->ui8NumberOfConfigurations)
					{
						USBD_LOGICAL_CONFIGURATION_INFO *psConfigurationInfo;
						IMG_UINT16						ui16Length;
					  
						/* Decide what configuration to return on this command */
						if ( ((ui8DescType == USB_CONFIGURATION_DESCRIPTOR_TYPE) && (pcd->speed == USB_SPEED_FULL))		||
							 ((ui8DescType == USB_OTHER_SPEED_CONFIG_DESCRIPTOR_TYPE) && (pcd->speed == USB_SPEED_HIGH))
						)
						{
							psConfigurationInfo = &psDeviceInfo->asConfigsFullSpeed[ui8Index];
						}
						else
						{
							psConfigurationInfo = &psDeviceInfo->asConfigsHighSpeed[ui8Index];
						}

						/*	Set the descriptor type before we send the data to the host */
						psConfigurationInfo->psDescriptor->bDescriptorType = (ctrl->wValue >> 8);

						ui16Length = USB_LE16_TO_CPU( psConfigurationInfo->psDescriptor->wTotalLength );
						value = min (ctrl->wLength, ui16Length);
						
						req->buf = psConfigurationInfo->psDescriptor;
					}
					else
					{
						/* An error value? */
						value = -USBD_ERR_INVAL;
					}
					break;
				}
				case USB_STRING_DESCRIPTOR_TYPE:
				{
					if ( ( psContext->ppcStrings != IMG_NULL ) &&
						 /*( ( ctrl->wValue & 0xff) < USBD_NUM_STRINGS ) &&*/
						   (psContext->ppcStrings[ctrl->wValue & 0xff] != IMG_NULL ) )
					{
						/* The very first byte represents the length of the string */
						value  = min (ctrl->wLength, (IMG_UINT16)(*(psContext->ppcStrings[ctrl->wValue & 0xff])));
						req->buf = psContext->ppcStrings[ctrl->wValue & 0xff];
					}
					else
					{
						// do nothing for now... the usb1.1 driver would actually issue a stall at this point.
						// E.g. we may see a string index 0xEE (microsoft OS string). There is some info on this at
						// http://www.microsoft.com/whdc/system/bus/USB/USBFAQ_intermed.mspx
						
						// IMG_ASSERT(0);
						value = -USBD_ERR_INVAL;
					}
					break;
				}
			}
			break;
		}	// USB_REQ_GET_DESCRIPTOR
		
		/* currently two configs, two speeds */
		case USB_REQ_SET_CONFIGURATION:
		{
			if (ctrl->bRequestType != 0)
			{
				goto unknown;
			}

			value = usbd_SetConfig( psBlockDesc, ctrl->wValue & 0xFF );

			/* Change the state to configured */
			psContext->eState = USBD_STATE_CONFIGURED;
			break;
		}		
		case USB_REQ_GET_CONFIGURATION:
		{
			if (ctrl->bRequestType != USB_DIR_IN)
			{
				goto unknown;
			}
			req->buf = (IMG_VOID*)&aui8ScratchBuffer[0];

			*(IMG_UINT8 *)req->buf =  psContext->ui8CurrentConfigurationNumber;
			value = min (ctrl->wLength, (IMG_UINT16) 1);
			break;
		}		
		case USB_REQ_SET_INTERFACE:
		{
			if (ctrl->bRequestType != USB_RECIP_INTERFACE)
			{
				goto unknown;
			}

			if (ctrl->wIndex < USBD_MAX_INTERFACES)
			{
				//value = usbd_SetInterface(ctrl->wIndex, ctrl->wValue);

				psContext->abAlternateIfSetting[ctrl->wIndex] = ctrl->wValue;
				
				/* Re-set the configuration */
				value = usbd_SetInterface( psBlockDesc, ctrl->wIndex );
			}
			else
			{
				IMG_ASSERT(0);
				value = -USBD_ERR_DOM;
			}

			break;
		}
		case USB_REQ_GET_INTERFACE:
		{
			if ((ctrl->bRequestType == 0x21) && (ctrl->wIndex == 0x02)) 
			{
				value = ctrl->wLength;
				break;
			}
			else 
			{
				if (ctrl->bRequestType != (USB_DIR_IN|USB_RECIP_INTERFACE))
				{
					goto unknown;
				}

				if (ctrl->wIndex < USBD_MAX_INTERFACES)
				{
					req->buf = (IMG_VOID*)&aui8ScratchBuffer[0];
					*(IMG_UINT8 *)req->buf = (IMG_UINT8) psContext->abAlternateIfSetting[ctrl->wIndex];
					value = min (ctrl->wLength, (IMG_UINT16) 1);
				}
				else
				{
					value = -USBD_ERR_DOM;
					break;
				}
			}
			break;
		}
		default:
unknown:
		{	
#if !defined (USBD_NO_CBMAN_SUPPORT)
			img_uint16	ui16OldWLength = ctrl->wLength; // Save the old wLength
			psContext->bLastControlRequestUnhandledByDriver = IMG_TRUE;

			// If the callback wishes to modify "value", it must modify ctrl->wLength
			CBMAN_ExecuteCallbackWithEventType (	psContext->ui32CallbackSlot,
													USBD_EVENT_UNHANDLED_CONTROL_MESSAGE_RECEIVED,
													(IMG_UINT32) &(req->buf),
													(IMG_VOID *) ctrl	);
			// New value is in ctrl->wLength
			value = ctrl->wLength;
			// Restore ctrl->wLength
			ctrl->wLength = ui16OldWLength;
#endif
		}
	}

	/* respond with data transfer before status phase? */
	if (value >= 0)
	{
		req->length = value;
		req->zero = value < ctrl->wLength && (value % pcd->ep0.dwc_ep.maxpacket) == 0;

		/* simply set the dma pointer to be the same as the buffer */
		if ( psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical )
		{
			req->dma = psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical( (img_uint32)req->buf );
		}
		else
		{
			req->dma = (img_uint32)req->buf;
		}
		IMG_ASSERT((req->dma & 0x3) == 0);
	  
		if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
		{
			psBlockDesc->psSystemDescriptor->pfn_FlushCache( (IMG_UINT32)req->buf, req->length );
		}

		value = dwc_otg_pcd_ep_queue( psBlockDesc, &pcd->ep0, req );

		if (value < 0) 
		{
			req->status = 0;
			usbd_SetupComplete( psBlockDesc, (IMG_VOID*) &pcd->ep0, req);
		}
	}

	/* device either stalls (value < 0) or reports success */
	return value;
}


IMG_VOID usbd_Disconnect( const ioblock_sBlockDescriptor	*	psBlockDesc  )
{
	USB_DC_T	*	psContext = (USB_DC_T *)psBlockDesc->pvAPIContext;
	usbd_ResetConfig ( psBlockDesc );
	psContext->sDevContext.ui8CurrentConfigurationNumber = 0;	
}



static USBD_STATE	eStateBeforeSuspend = 0;

IMG_VOID usbd_Suspend( img_void	*	p )
{
	ioblock_sBlockDescriptor	*	psBlockDesc		= (ioblock_sBlockDescriptor *)p;
	USB_DC_T					*	psContext		= (USB_DC_T *)psBlockDesc->pvAPIContext;
	USBD_DC_T					*	psDevContext	= &psContext->sDevContext;
	if (psDevContext->eState != USBD_STATE_SUSPENDED)
	{
		/* Remember the current state */
		eStateBeforeSuspend = psDevContext->eState;

		/* Change to the suspended state */
		psDevContext->eState = USBD_STATE_SUSPENDED;

		/* Disable endpoints and signal application */
		usbd_ResetConfig( psBlockDesc );
	}
}


IMG_VOID usbd_Resume( img_void	*	p )
{
	ioblock_sBlockDescriptor	*	psBlockDesc		= (ioblock_sBlockDescriptor *)p;
	USB_DC_T					*	psContext		= (USB_DC_T *)psBlockDesc->pvAPIContext;
	USBD_DC_T					*	psDevContext	= &psContext->sDevContext;
	if (psDevContext->eState == USBD_STATE_SUSPENDED)
	{
		/* Restore the state before the suspend */
		psDevContext->eState = eStateBeforeSuspend;

		/* Do we need a s/w event? */
	}
}


IMG_VOID usbd_BusReset( const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	USB_DC_T	*	psContext = (USB_DC_T *)psBlockDesc->pvAPIContext;
	/*	Reset the current configuration - this causes termination of transfers and disabling of endpoints 
		If necessary, callbacks are made to the application to notify it of the disabling of the I/O
	*/
	usbd_ResetConfig ( psBlockDesc );

	/* Go to the default state */
	psContext->sDevContext.eState = USBD_STATE_DEFAULT;
}

#if defined (USBD_NO_CBMAN_SUPPORT)
IMG_VOID usbd_EventNotify ( USB_DC_T	*	psContext, USBD_EVENT_T eEvent )
{
	/*	Notify the application if there is a callback function register and callbacks for this
		event have been enabled
	*/
	if (psContext->sDevContext.pfCallback && (psContext->sDevContext.uiRegisteredEvents & eEvent))
	{
		USBD_EVENT_CALLBACK_DATA_T sCallbackData;

		sCallbackData.uiEvent   = eEvent;
		sCallbackData.pData     = 0;

		psContext->sDevContext.pfCallback(sCallbackData);
	}
}
#endif
