/*!
*******************************************************************************
  file   usbd_drv.c

  brief  Universal Serial Bus Device Driver

         USBD device driver code.

  author Imagination Technologies

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




/*============================================================================*/
/*                                                                            */
/*                          INCLUDE FILES		                              */
/*                                                                            */
/*============================================================================*/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES

#if defined __META_MEOS__ || defined __MTX_MEOS__ || defined __META_NUCLEUS_PLUS__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <MeOS.h>

#include <img_defs.h>
#include <ioblock_defs.h>
#include <ioblock_utils.h>

#include <system.h>

#if !defined (USBD_NO_CBMAN_SUPPORT)
#include <cbman.h>
#endif

/* USB Driver/API header files */
#include "usbd_api.h"
#include "usb_config.h"
#include "usbd_drv.h"
#include "usb_hal.h"
#include "dwc_otg_cil.h"
#include "dwc_otg_pcd.h"
#include "usb_defs.h"
#include "usb_drv.h"

#define WRITE(a,d)    (*(volatile unsigned long *)(a) = (d))
#define READ(a)       (*(volatile unsigned long *)(a))

/*============================================================================*/
/*                                                                            */
/*                          MACRO DEFINITIONS	                              */
/*                                                                            */
/*============================================================================*/

/*============================================================================*/
/*                                                                            */
/*                          FUNCTION PROTOTYPES	                              */
/*                                                                            */
/*============================================================================*/

static IMG_INT32	usbd_Init(QIO_DEVICE_T *dev, QIO_DEVPOWER_T *pwrClass, IMG_INT32 *devRank, unsigned intMasks[QIO_MAXVECGROUPS]);
static IMG_VOID		usbd_Start(QIO_DEVICE_T *dev, QIO_IOPARS_T *ioPars);
static IMG_VOID		usbd_Isr(QIO_DEVICE_T *dev);
static IMG_VOID		usbd_Cancel(QIO_DEVICE_T *dev);
extern img_void		usbdat_OverrideDFUTransferSize( img_uint16	ui16TransferSizeFS, img_uint16 ui16TransferSizeHS );

/* the driver object */
const QIO_DRIVER_T USBD_driver =
{
    usbd_Isr,		/* ISR                       */
    usbd_Init,		/* init function             */
    usbd_Start,     /* start function            */
    usbd_Cancel,	/* cancel function           */
    IMG_NULL,  		/* no power control function */
    IMG_NULL,	    /* no sim start function     */
    IMG_NULL
};

// MAX_NUM_USBD_BLOCKS defined in usbd_drv.h
ioblock_sBlockDescriptor	*	g_apsUSBDBlock[ MAX_NUM_USBD_BLOCKS ] =
{	IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL,
	IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL,
	IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL,
	IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL
};

/* Global driver structures */

typedef struct tag_sUSBTransferMemory
{
	IMG_UINT8 aui8SetupPkt  [5*8] IMG_ALIGN(4); /* There is a malloc of 5*8bytes in the code, but only the first 8 bytes seem to be used */
	IMG_UINT8 aui8StatusBuf [2] IMG_ALIGN(4);
	IMG_UINT8 aui8DMASetupDesc[ sizeof(dwc_otg_dev_dma_desc_t) * 2 ] IMG_ALIGN(4);
	IMG_UINT8 aui8DMAInDesc[ sizeof(dwc_otg_dev_dma_desc_t) ] IMG_ALIGN(4);
	IMG_UINT8 aui8DMAOutDesc[ sizeof(dwc_otg_dev_dma_desc_t) ] IMG_ALIGN(4);
	IMG_UINT8 aui8DMAEpDesc[ sizeof(dwc_otg_dev_dma_desc_t) * DESCRIPTORS_PER_EP * ( 2 * (MAX_EPS_CHANNELS - 1) + 1 )] IMG_ALIGN(4);

#if defined (_EN_ISOC_)
	IMG_UINT8 aui8IsocPktInfo[ sizeof( iso_pkt_info_t ) * (2 * (MAX_EPS_CHANNELS - 1) + 1) * MAX_ISO_PACKETS ] IMG_ALIGN(4); // 16 pkt infos per EP
#endif
} USB_sTransferMemory;

/*!
******************************************************************************

 @Function              usbd_EnumerateDescriptors

 @Description	Checks and converts the device descriptors to internal structures

******************************************************************************/
static IMG_VOID usbd_EnumerateDescriptors( USBD_DC_T	*	psContext,	USBD_INIT_PARAM_T			*	psInitParam	)
{
	img_uint32		ui32Config;
	img_uint32		ui32TotalLength;
	img_uint8	*	pui8Ptr;
	img_uint8		ui8NumIfs;
	img_uint8		ui8NumInEps = 0, ui8NumOutEps = 0;

	// Assign the device and qualifier descriptors
	psContext->psDeviceDescriptor		= psInitParam->psDeviceDescriptor;
	psContext->psQualifierDescriptor	= psInitParam->psQualifierDescriptor;

	// Check the number of configurations in the descriptor matches that in the initparam structure
	IMG_ASSERT( psContext->psDeviceDescriptor->bNumConfigurations == psInitParam->psConfigDescriptors->ui8NumConfigurations );
	IMG_ASSERT( psContext->psQualifierDescriptor->bNumConfigurations == psInitParam->psConfigDescriptors->ui8NumConfigurations );
	psContext->sLogicalDeviceInfo.ui8NumberOfConfigurations = psInitParam->psConfigDescriptors->ui8NumConfigurations;

	// Reset maximum number of endpoints for any configuration - used to allocate FIFO sizes
	psContext->ui32MaxConfigInEPs = 0;

	for ( ui32Config = 0; ui32Config < USBD_MAX_CONFIGURATIONS; ++ui32Config )
	{
		// Check FS
		if ( psInitParam->psConfigDescriptors->apvConfigDescriptorFullSpeed[ ui32Config ] )
		{
			USBD_LOGICAL_CONFIGURATION_INFO	*	psConfInfo = &psContext->sLogicalDeviceInfo.asConfigsFullSpeed[ ui32Config ];
			USBD_LOGICAL_INTERFACE_INFO		*	psIfInfo = IMG_NULL;
			USB_DESCRIPTOR_HEADER			*	psHdr;
			USB_INTERFACE_DESCRIPTOR		*	psIfDesc = IMG_NULL;
			psConfInfo->psDescriptor = psInitParam->psConfigDescriptors->apvConfigDescriptorFullSpeed[ ui32Config ];

			// Make sure we have enough space for interfaces
			IMG_ASSERT( psConfInfo->psDescriptor->bNumInterfaces <= USBD_MAX_INTERFACES );

			// Get total length of config descriptor
			ui32TotalLength = USB_LE16_TO_CPU(psConfInfo->psDescriptor->wTotalLength);
			// Advance to first
			pui8Ptr = (img_uint8 *)psConfInfo->psDescriptor + sizeof( USB_CONFIG_DESCRIPTOR );

			ui8NumIfs = 0;
			while ( pui8Ptr < (img_uint8 *)psConfInfo->psDescriptor + ui32TotalLength )
			{
				// Check what kind of descriptor is next
				psHdr = (USB_DESCRIPTOR_HEADER *)pui8Ptr;
				if ( psHdr->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE )
				{
					psIfDesc = (USB_INTERFACE_DESCRIPTOR *)pui8Ptr;
					// Check whether we have enough space for another interface
					IMG_ASSERT( ui8NumIfs < USBD_MAX_INTERFACES );
					// Found an IF descriptor, see whether it is alternate
					if ( psIfDesc->bAlternateSetting != 0 )
					{
						img_uint32	i;
						img_bool	bFoundDefault = IMG_FALSE;
						// Find the index of the default interface
						for ( i = 0; i < USBD_MAX_INTERFACES; ++i )
						{
							if ( psConfInfo->asIfDefault[i].psDescriptor->bInterfaceNumber == psIfDesc->bInterfaceNumber )
							{
								bFoundDefault = IMG_TRUE;
								break;
							}
						}

						IMG_ASSERT( bFoundDefault );
						// We can only support one alternate interface, check that it hasn't already been defined
						IMG_ASSERT( psConfInfo->asIfAlternate[ i ].psDescriptor == IMG_NULL );
						psIfInfo = &psConfInfo->asIfAlternate[ i ];
					}
					else
					{
						psIfInfo = &psConfInfo->asIfDefault[ ui8NumIfs++ ];
					}

					// Reset number of endpoints for this interface
					ui8NumInEps = 0;
					ui8NumOutEps = 0;

					// Get the location of the descriptor
					psIfInfo->psDescriptor = psIfDesc;
				}
				else if ( psHdr->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE )
				{
					USB_ENDPOINT_DESCRIPTOR	*	psEPDesc = (USB_ENDPOINT_DESCRIPTOR *)pui8Ptr;
					// Check that we have an interface for this
					IMG_ASSERT( psIfDesc );

					// Check whether the EP is IN or OUT
					if ( psEPDesc->bEndpointAddress & USB_DIR_IN )
					{
						// Make sure we have enough space for another endpoint on this interface
						IMG_ASSERT( ui8NumInEps < USBD_MAX_INTERFACE_EP_PAIRS );
						psIfInfo->psDataInEp[ ui8NumInEps++ ] = psEPDesc;
					}
					else
					{
						// Make sure we have enough space for another endpoint on this interface
						IMG_ASSERT( ui8NumOutEps < USBD_MAX_INTERFACE_EP_PAIRS );
						psIfInfo->psDataOutEp[ ui8NumOutEps++ ] = psEPDesc;
					}
				}

				// Check if this IF has the most endpoints
				if ( ui8NumInEps > psContext->ui32MaxConfigInEPs )
				{
					psContext->ui32MaxConfigInEPs = ui8NumInEps;
				}

				pui8Ptr += psHdr->bLength;
			}
		}
		// Check HS
		if ( psInitParam->psConfigDescriptors->apvConfigDescriptorHighSpeed[ ui32Config ] )
		{
			USBD_LOGICAL_CONFIGURATION_INFO	*	psConfInfo = &psContext->sLogicalDeviceInfo.asConfigsHighSpeed[ ui32Config ];
			USBD_LOGICAL_INTERFACE_INFO		*	psIfInfo = IMG_NULL;
			USB_DESCRIPTOR_HEADER			*	psHdr;
			USB_INTERFACE_DESCRIPTOR		*	psIfDesc = IMG_NULL;
			psConfInfo->psDescriptor = psInitParam->psConfigDescriptors->apvConfigDescriptorHighSpeed[ ui32Config ];

			// Get total length of config descriptor
			ui32TotalLength = USB_LE16_TO_CPU(psConfInfo->psDescriptor->wTotalLength);
			// Advance to first
			pui8Ptr = (img_uint8 *)psConfInfo->psDescriptor + sizeof( USB_CONFIG_DESCRIPTOR );

			ui8NumIfs = 0;
			while ( pui8Ptr < (img_uint8 *)psConfInfo->psDescriptor + ui32TotalLength )
			{
				// Check what kind of descriptor is next
				psHdr = (USB_DESCRIPTOR_HEADER *)pui8Ptr;
				if ( psHdr->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE )
				{
					psIfDesc = (USB_INTERFACE_DESCRIPTOR *)pui8Ptr;
					// Check whether we have enough space for another interface
					IMG_ASSERT( ui8NumIfs < USBD_MAX_INTERFACES );
					// Found an IF descriptor, see whether it is alternate
					if ( psIfDesc->bAlternateSetting != 0 )
					{
						img_uint32	i;
						img_bool	bFoundDefault = IMG_FALSE;
						// Find the index of teh default interface
						for ( i = 0; i < USBD_MAX_INTERFACES; ++i )
						{
							if ( psConfInfo->asIfDefault[i].psDescriptor->bInterfaceNumber == psIfDesc->bInterfaceNumber )
							{
								bFoundDefault = IMG_TRUE;
								break;
							}
						}
						IMG_ASSERT( bFoundDefault );
						// We can only support one alternate interface, check that it hasn't already been defined
						IMG_ASSERT( psConfInfo->asIfAlternate[ i ].psDescriptor == IMG_NULL );
						psIfInfo = &psConfInfo->asIfAlternate[ i ];
					}
					else
					{
						psIfInfo = &psConfInfo->asIfDefault[ ui8NumIfs++ ];
					}

					// Reset number of endpoints for this interface
					ui8NumInEps = 0;
					ui8NumOutEps = 0;

					// Get the location of the descriptor
					psIfInfo->psDescriptor = psIfDesc;
				}
				else if ( psHdr->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE )
				{
					USB_ENDPOINT_DESCRIPTOR	*	psEPDesc = (USB_ENDPOINT_DESCRIPTOR *)pui8Ptr;
					// Check that we have an interface for this
					IMG_ASSERT( psIfDesc );

					// Check whether the EP is IN or OUT
					if ( psEPDesc->bEndpointAddress & USB_DIR_IN )
					{
						// Make sure we have enough space for another endpoint on this interface
						IMG_ASSERT( ui8NumInEps < USBD_MAX_INTERFACE_EP_PAIRS );
						psIfInfo->psDataInEp[ ui8NumInEps++ ] = psEPDesc;
					}
					else
					{
						// Make sure we have enough space for another endpoint on this interface
						IMG_ASSERT( ui8NumOutEps < USBD_MAX_INTERFACE_EP_PAIRS );
						psIfInfo->psDataOutEp[ ui8NumOutEps++ ] = psEPDesc;
					}
				}

				// Check if this IF has the most endpoints
				if ( ui8NumInEps > psContext->ui32MaxConfigInEPs )
				{
					psContext->ui32MaxConfigInEPs = ui8NumInEps;
				}

				pui8Ptr += psHdr->bLength;
			}
		}
	}

	// Check max IN eps is less than number of TXFIFOs we have
	IMG_ASSERT( ui8NumInEps < MAX_TX_FIFOS );
}

/*!
******************************************************************************

 @Function              usbd_SetupHwConfigParameters

 @Description	Checks and sets up the h/w configuration structure.

******************************************************************************/
static IMG_VOID usbd_SetupHwConfigParameters( const ioblock_sBlockDescriptor	*	psBlockDesc, USB_DEVICE_SPEED EnumerateSpeed)
{
	USB_DC_T				*	psContext		= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_core_if_t		*	core_if			= &(psContext->sOtgCoreIf);
	dwc_otg_core_params_t	*	psCoreParams	= &(core_if->core_params);
	IMG_INT32					i;
	IMG_UINT32					ui32Temp;
	IMG_UINT32					ui32Reserve;

	/* Enforce no HNP SRP capable device mode */

	IMG_ASSERT(core_if->hwcfg2.b.op_mode == DWC_HWCFG2_OP_MODE_HNP_SRP_CAPABLE_OTG);

	psCoreParams->otg_cap = DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE;

	/* Enforce the requirement for internal DMA */
	IMG_ASSERT(core_if->hwcfg2.b.architecture == 2);		// internal DMA
	psCoreParams->dma_enable = 1;
	psCoreParams->dma_burst_size = 32;
	psCoreParams->dma_desc_enable = 1;

	/* Support only dynamic fifo */
	IMG_ASSERT(core_if->hwcfg2.b.dynamic_fifo);
	psCoreParams->enable_dynamic_fifo = 1;


	/* Obtain max_transfer_size and max_packet_count from the hardware parameters */
	psCoreParams->max_transfer_size = ((1 << (core_if->hwcfg3.b.xfer_size_cntr_width + 11)) - 1);
	psCoreParams->max_packet_count = ((1 << (core_if->hwcfg3.b.packet_size_cntr_width + 4)) - 1);


	/* Enforce HS UTMI interface */
	IMG_ASSERT((core_if->hwcfg2.b.hs_phy_type == 3) || (core_if->hwcfg2.b.hs_phy_type == 1));
	psCoreParams->phy_type = DWC_PHY_TYPE_PARAM_UTMI;

	/* Try to enumerate at high speed */
	//psCoreParams->speed = DWC_SPEED_PARAM_HIGH;
	switch(EnumerateSpeed)
	{
	case USB_SPEED_FULL:
		psCoreParams->speed = DWC_SPEED_PARAM_FULL;
		break;
	case USB_SPEED_HIGH:
		psCoreParams->speed = DWC_SPEED_PARAM_HIGH;
		break;

	case USB_SPEED_UNKNOWN: /* Intentional dropthrough */
	case USB_SPEED_LOW:		/* Intentional dropthrough */
	default:
		break;
	}

	/* Enforce no I2C */
	IMG_ASSERT(core_if->hwcfg3.b.i2c == 0);
	psCoreParams->i2c_enable = 0;


	/* Enforce the requirement for multiple TxFIFO */
	IMG_ASSERT(core_if->hwcfg4.b.ded_fifo_en);
	psCoreParams->en_multiple_tx_fifo = 1;


	/* Total FIFO size */
	psCoreParams->data_fifo_size = core_if->hwcfg3.b.dfifo_depth;

	/* We have to have at least 4096 bytes of space */
	IMG_ASSERT (core_if->hwcfg3.b.dfifo_depth >= 0x400);

	/* Set RxFIFO size to 0x300 (3072 bytes) */
	IMG_ASSERT(usb_ReadReg32(psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GRXFSIZ_OFFSET) >= 0x300);
	psCoreParams->dev_rx_fifo_size = 0x300;

	/* Set TxFIFO 0 size to 0x100 (1024 bytes) */
	IMG_ASSERT((usb_ReadReg32(psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GNPTXFSIZ_OFFSET) >> 16) >= 0x100);
	psCoreParams->dev_nperio_tx_fifo_size = 0x100;

#define MIN_FIFO_SIZE	(32)
	if ( psContext->sDevContext.ui32MaxConfigInEPs < MAX_TX_FIFOS )
	{
		ui32Reserve = (MAX_TX_FIFOS - psContext->sDevContext.ui32MaxConfigInEPs) * MIN_FIFO_SIZE; // Each endpoint needs a minimum of 32 words.
	}
	else
	{
		ui32Reserve = 0;
	}

	/* Divide the rest of the space evenly between the maximum possible endpoints */

	ui32Temp = (psCoreParams->data_fifo_size - psCoreParams->dev_rx_fifo_size - psCoreParams->dev_nperio_tx_fifo_size - ui32Reserve) / psContext->sDevContext.ui32MaxConfigInEPs;

	for ( i = 0; i < psContext->sDevContext.ui32MaxConfigInEPs; ++i )
	{
		IMG_ASSERT(usb_ReadReg32(psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_DPTXFSIZ_DIEPTXF_OFFSET + (4 * i)) >= ui32Temp);
		psCoreParams->dev_tx_fifo_size[i] = ui32Temp;
	}
	// Allocate minimum fifo storage for unused endpoints
	for ( ; i < MAX_TX_FIFOS; ++i )
	{
		IMG_ASSERT( usb_ReadReg32(psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_DPTXFSIZ_DIEPTXF_OFFSET + (4 * i)) >= MIN_FIFO_SIZE );
		psCoreParams->dev_tx_fifo_size[i] = MIN_FIFO_SIZE;
	}

	/* Other parameters */
	#if defined (__IMG_HW_FPGA__)
		psCoreParams->phy_utmi_width = 8;
	#else
		psCoreParams->phy_utmi_width = 16;
	#endif

	psCoreParams->phy_ulpi_ddr = 0;
	psCoreParams->phy_ulpi_ext_vbus = DWC_PHY_ULPI_INTERNAL_VBUS;
	psCoreParams->ulpi_fs_ls = 0;
	psCoreParams->ts_dline = 0;
	psCoreParams->thr_ctl = 0;
	psCoreParams->tx_thr_length = 64;
	psCoreParams->rx_thr_length = 64;
}


/*!
******************************************************************************

 @Function              usbd_InitialiseHardware

 @Description	Perform initialisation of the Synopsys USB hardware.

******************************************************************************/
static IMG_INT32 usbd_InitialiseHardware( const ioblock_sBlockDescriptor	*	psBlockDesc, USB_DEVICE_SPEED EnumerateSpeed )
{
	USB_DC_T			*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_core_if_t	*	core_if		= &(psContext->sOtgCoreIf);

	/*
		Ensure this device is really a DWC_otg Controller.
		Read and verify the SNPSID register contents. The value should be
		0x45F42XXX, which corresponds to "OT2", as in "OTG version 2.XX".
		This check may be eliminated, but it is a good basic sanity check
	 */
	IMG_ASSERT((usb_ReadReg32(psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GSNPSID_OFFSET) & 0xFFFFF000) == 0x4F542000);

	/* Store the contents of the hardware configuration registers */
	core_if->hwcfg1.d32 = usb_ReadReg32(psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GHWCFG1_OFFSET);
	core_if->hwcfg2.d32 = usb_ReadReg32(psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GHWCFG2_OFFSET);
	core_if->hwcfg3.d32 = usb_ReadReg32(psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GHWCFG3_OFFSET);
	core_if->hwcfg4.d32 = usb_ReadReg32(psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GHWCFG4_OFFSET);

	/*	Check core parameters against the hwcfg register values */
	usbd_SetupHwConfigParameters( psBlockDesc, EnumerateSpeed );

	/* Disable the global interrupt */
	dwc_otg_disable_global_interrupts( psBlockDesc->ui32Base );

	/* Initialize the DWC_otg core */
	dwc_otg_core_init( psBlockDesc->ui32Base, core_if );

	/* Initialize the PCD */
	dwc_otg_pcd_init( psBlockDesc );

	/* Do not enable interrupts now... enable them after usbd_Init has completed */
	/*dwc_otg_enable_global_interrupts( );*/

	return 0;
}


/*!
******************************************************************************

 @Function              usbd_XferComplete

 @Description	Transfer completion function. Used through a callback from
				the PCD layer.

******************************************************************************/
static IMG_VOID usbd_XferComplete( const ioblock_sBlockDescriptor	*	psBlockDesc, IMG_VOID * pvPointer, struct dwc_otg_pcd_request	*req )
{
	USB_DC_T			*	psContext		= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_pcd_ep_t	*	ep				= (dwc_otg_pcd_ep_t *) pvPointer;
	USBD_sEP			*	psUSBDEP		= &psContext->asUSBDEP[ ep->dwc_ep.num - 1 ];
	QIO_DEVICE_T		*	dev				= &psUSBDEP->sDevice;

	/*	We only expect this function to be called from the ISR context or on a
		call to cancel command (when the endpoints are reset */
	IMG_ASSERT(psContext->sDevContext.bInsideISR || psUSBDEP->bCancel);

	/* Need to add some checks to see if the transfer is completed normally */
	if (pvPointer && req)
	{
		/* Update the number of bytes transferred */
		psUSBDEP->psCurrentIo->counter += req->actual;

		if ( !ep->dwc_ep.is_in && (req->actual > 0 ) )
		{
			if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
			{
				psBlockDesc->psSystemDescriptor->pfn_FlushCache( (IMG_UINT32)req->buf, req->actual );
			}
		}

		/* free up the request object */
		dwc_otg_pcd_free_request(req);

		/* If we have finished all the ep requests then it is time to finish the R/W operation */
		if (!dwc_otg_pcd_ep_is_queued(ep))
		{
			/* Transfer finished... return to xfer idle state */
			psUSBDEP->ui32XferStatus = USBD_XFER_STATUS_IDLE;

			if (!psUSBDEP->bCancel)
			{
				/* Complete the transfer in the QIO and start another if queued */
				QIO_complete(dev, QIO_IOCOMPLETE);
				QIO_start(dev);
			}
		}
	}
	else
	{
		IMG_ASSERT(0);
	}
}

#if defined (_PER_IO_)

static IMG_VOID usbd_XIsocComplete( const ioblock_sBlockDescriptor	*	psBlockDesc, IMG_VOID	*	pvPointer, struct dwc_otg_pcd_request	*req )
{
	USB_DC_T						*	psContext		= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_pcd_ep_t				*	ep				= (dwc_otg_pcd_ep_t	*)pvPointer;
	USBD_sEP						*	psUSBDEP		= &psContext->asUSBDEP[ ep->dwc_ep.num - 1 ];
	QIO_DEVICE_T					*	dev				= &psUSBDEP->sDevice;
	struct dwc_iso_pkt_desc_port	*	pkt_desc;

	/*	We only expect this function to be called from the ISR context or on a
		call to cancel command (when the endpoitns are reset */
	IMG_ASSERT(psContext->sDevContext.bInsideISR || psUSBDEP->bCancel);

	/* Need to add some checks to see if the transfer is completed normally */
	if (pvPointer && req)
	{
		IMG_UINT32	i;
		pkt_desc = req->ext_req.per_io_frame_descs;
		/* Go through every packet and add to bytes transferred count as well as flush the cache. */
		for ( i = 0; i < req->ext_req.pio_pkt_count; ++i )
		{
			psUSBDEP->psCurrentIo->counter += pkt_desc->actual_length;
			IMG_ASSERT( pkt_desc->status == 0 );
		}

		if ( !ep->dwc_ep.is_in && (psUSBDEP->psCurrentIo->counter > 0) )
		{
			if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
			{
				/* We flush the entire queued buffer as this should be faster than individiual calls for each packet */
				psBlockDesc->psSystemDescriptor->pfn_FlushCache( (IMG_UINT32)req->buf, req->length );
			}
		}

		/* free up the request object */
		dwc_otg_pcd_free_request(req);

		/* IF we have finished all the ep requests then it is time to finish the R/W operation */
		if (!dwc_otg_pcd_ep_is_queued(ep))
		{
			/* Transfer finished... return to xfer idle state */
			psUSBDEP->ui32XferStatus = USBD_XFER_STATUS_IDLE;

			if (!psUSBDEP->bCancel)
			{
				/* Complete the transfer in the QIO and start another if queued */
				QIO_complete(dev, QIO_IOCOMPLETE);
				QIO_start(dev);
			}
		}
	}
	else
	{
		IMG_ASSERT(0);
	}
}

#endif /* _PER_IO_ */

#if defined (_EN_ISOC_)
/*!
******************************************************************************

 @Function              usbd_IsocXferComplete

 @Description	Isochronous Transfer completion function. Used through a callback from
				the PCD layer.

******************************************************************************/
IMG_VOID usbd_IsocXferComplete( const ioblock_sBlockDescriptor	*	psBlockDesc, IMG_VOID * pvPointer, struct usb_iso_request	*req )
{
	USB_DC_T						*	psContext		= (USB_DC_T *)psBlockDesc->pvAPIContext;
	USBD_DC_T						*	psDevContext	= &(psContext->sDevContext);
	dwc_otg_pcd_ep_t				*	ep				= (dwc_otg_pcd_ep_t *) pvPointer;
	dwc_ep_t						*	dwc_ep			= &ep->dwc_ep;
	QIO_DEVICE_T					*	dev				= psDevContext->psDevice;
	img_uint32							proc_buf_num;
	img_int32							i, packet_count;
	usb_iso_packet_descriptor_t		*	iso_packet		= IMG_NULL;
	img_void						*	pBuf;

	/*	We only expect this function to be called from the ISR context or on a
		call to cancel command (when the endpoints are reset) */
	IMG_ASSERT( psDevContext->bInsideISR || psDevContext->bCancel );

	proc_buf_num = dwc_ep->proc_buf_num ^ 0x1;

	if ( proc_buf_num )
	{
		iso_packet = req->iso_packet_desc1;
		pBuf = req->buf1;
	}
	else
	{
		iso_packet = req->iso_packet_desc0;
		pBuf = req->buf0;
	}

	packet_count = dwc_ep->pkt_cnt;

	for ( i = 0; i < packet_count; ++i )
	{
		iso_packet[i].status = dwc_ep->pkt_info[i].status;
		iso_packet[i].offset = dwc_ep->pkt_info[i].offset;
		iso_packet[i].actual = dwc_ep->pkt_info[i].length;
	}

	req->process_buffer( proc_buf_num, pBuf, iso_packet );

		/* free up the request object */
//		dwc_otg_pcd_free_iso_request( req ); TODO: Do this when stopping isoc xfer

		/* If we have finished all the ep requests then it is time to finish the R/W operation */
//		if ( !dwc_otg_pcd_ep_is_queued( ep ) ) TODO: Do this when stopping isoc xfer
//		{
			/* Transfer finished... return to xfer idle state */
//			psDevContext->eXferStatus = USBD_XFER_STATUS_IDLE;
//
//			if ( !psDevContext->bCancel )
//			{
//				/* Complete the transfer in the QIO and start another if queued */
//				QIO_complete(dev, QIO_IOCOMPLETE);
//				QIO_start(dev);
//			}
//		}
//	}
//	else
//	{
//		IMG_ASSERT(0);
//	}
}

#endif

/*!
******************************************************************************

 @Function              usbd_EpStartXfer

 @Description	Set up an i/o transfer

 @Input		pcBuffer	: Pointer to the host buffer
 			uiLength	: Transfer length
			ui8Opcode	: Opcode of the transfer required

 @Return	Returns 0 when the transfer is queued successfully, an error code otherwise.

******************************************************************************/
IMG_UINT32 usbd_EpStartXfer( const ioblock_sBlockDescriptor	*	psBlockDesc, USBD_sEP	*	psUSBDEP,	IMG_UINT8 * pcBuffer, IMG_UINT32 uiLength )
{
	dwc_otg_pcd_ep_t			*	ep;
	IMG_INT32						status;
	IMG_UINT32						uiMaxXferSize, uiBytesQueued, uiNumQueuedReqs;
  #if !defined (USE_DDMA_ONLY)
	dwc_otg_core_if_t			*	core_if			= &(((USB_DC_T *)psBlockDesc->pvAPIContext)->sOtgCoreIf);
  #endif
	USBD_DC_T					*	psContext		= &(((USB_DC_T *)psBlockDesc->pvAPIContext)->sDevContext);

	if (psUSBDEP->ui32XferStatus == USBD_XFER_STATUS_IDLE)
	{
		ep = (dwc_otg_pcd_ep_t *)psUSBDEP->pvEP;
		psUSBDEP->ui32XferStatus = USBD_XFER_STATUS_ACTIVE;

		/* Ensure that the endpoint is available and that the MPS for the endpoint is aligned to the DWORD boundary */
		if ((!ep) || (ep->dwc_ep.maxpacket & 0x00000003))
		{
			IMG_ASSERT(0);
			psUSBDEP->ui32XferStatus = USBD_XFER_STATUS_IDLE;
			return 1;
		}

	  #if defined (_EN_ISOC_)
		if (ep->dwc_ep.type == DWC_OTG_EP_TYPE_ISOC)
		{
			/*	We can only transfer in MPS multiples.
				TODO: consider how we can expoit the multiple packets/microframe
				when HS mode is used
			*/
			uiMaxXferSize = ep->dwc_ep.maxpacket;
		}
		else
	  #elif defined (_PER_IO_)
		if (ep->dwc_ep.type == DWC_OTG_EP_TYPE_ISOC)
		{
			// We can only handle a limited number of packets in each request
			uiMaxXferSize = ISOC_PACKET_SIZE * MAX_ISO_PACKETS;
		}
		else
	  #endif
		{
		  #if !defined (USE_DDMA_ONLY)
			/* section 6.5.2.1.6 (2) states that the transfer size needs to be a whole number of MPS for non-isoc OUT transfers */
			if (!ep->dwc_ep.is_in && !core_if->dma_desc_enable)
			{
				IMG_UINT32 uiMod  = (uiLength % ep->dwc_ep.maxpacket);
				if (uiMod)
				{
					uiLength = uiLength + ep->dwc_ep.maxpacket - uiMod;
				}
			}
		  #endif

			/* Can have any size transfer for non-isochronous IN endpoints */
			uiMaxXferSize = uiLength;
		}

		uiBytesQueued = 0;
		uiNumQueuedReqs = 0;

	  #if defined (_EN_ISOC_)
		if ( ep->dwc_ep.type != DWC_OTG_EP_TYPE_ISOC )
	  #endif /* _EN_ISOC_ */
		{
			struct dwc_otg_pcd_request	*	req;
			/* Allow 0 length transfers */
			while ((uiNumQueuedReqs == 0) || (uiBytesQueued < uiLength))
			{
				IMG_UINT32 uiThisXferLength = ((uiLength - uiBytesQueued) <= uiMaxXferSize)? (uiLength - uiBytesQueued) : uiMaxXferSize;

				// Make for, for isoc transfers, this is a multiple of the packet size
				if ( ep->dwc_ep.type == DWC_OTG_EP_TYPE_ISOC )
				{
					IMG_ASSERT( (uiThisXferLength & 0x3F) == 0 );
				}

				req = dwc_otg_pcd_alloc_request( );

				IMG_ASSERT(req);

				req->buf = pcBuffer + uiBytesQueued;

				if ( psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical )
				{
					req->dma = psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical( (img_uint32)req->buf );
				}
				else
				{
					req->dma = (img_uint32)req->buf;
				}

				/* Check data alignment of the buffer */
				IMG_ASSERT((((IMG_UINT32)req->buf) & 0x3) == 0);

				uiBytesQueued += uiThisXferLength;
				req->length = uiThisXferLength;
			#if defined (_PER_IO_)
				if ( ep->dwc_ep.type == DWC_OTG_EP_TYPE_ISOC )
				{
					req->complete = &usbd_XIsocComplete;
				}
				else
			#endif
				{
					req->complete = &usbd_XferComplete;
				}

				req->status = 0;
				req->zero = psContext->bReqZero;

			#if defined (_PER_IO_)
				if ( ep->dwc_ep.type == DWC_OTG_EP_TYPE_ISOC )
				{
					IMG_UINT32	i;
					// Split the transfer into ISOC_PACKET_SIZE byte packets (as the host driver is expecting this)
					req->ext_req.pio_pkt_count = uiThisXferLength / ISOC_PACKET_SIZE;

					// Fill in the packet info
					for ( i = 0; i < req->ext_req.pio_pkt_count; ++i )
					{
						req->ext_req.per_io_frame_descs[i].offset = i * ISOC_PACKET_SIZE;
						req->ext_req.per_io_frame_descs[i].length = ISOC_PACKET_SIZE;
					}

					req->ext_req.tr_sub_flags = DWC_EREQ_TF_ASAP;
					status = dwc_otg_pcd_xiso_ep_queue( psBlockDesc, ep, req );
				}
				else
				{
			#endif
					/* queue up the transfer on the endpoint */
					status = dwc_otg_pcd_ep_queue( psBlockDesc, ep, req );
			#if defined (_PER_IO_)
				}
			#endif

				/* Check if there were problems setting up this transfer */
				if (status)
				{
					/*	Assert for now... in reality should do something more useful.
						e.g. check if this is the first call to the function for this
						transfer and if so then complete transfer.. otherwise exit
					*/
					IMG_ASSERT(0);
					return 1;
				}

				uiNumQueuedReqs ++;
			}
		}
	  #if defined (_EN_ISOC_)
		else
		{
			// Should never get here
			IMG_ASSERT(0);
		}
	  #endif
	}
	/* There is a transfer already in progress */
	else
	{
		/* Simply assert for now */
		IMG_ASSERT(0);
		return 1;
	}

	return 0;
}

#if defined (_EN_ISOC_)
IMG_UINT32 usbd_EpStartIsocXfer( const ioblock_sBlockDescriptor	*	psBlockDesc, usb_iso_transfer_t	*	psTransfer, IMG_UINT8 ui8Opcode )
{
	USB_DC_T			*	psContext		= (USB_DC_T *)psBlockDesc->pvAPIContext;
	USBD_DC_T			*	psDevContext	= &(psContext->sDevContext);
	dwc_otg_pcd_ep_t	*	ep				= IMG_NULL;
	IMG_INT32				status;
	usb_iso_request_t	*	req;

	if ( psDevContext->eXferStatus == USBD_XFER_STATUS_IDLE )
	{
		/* Write (IN) on the isochronous channel */
		if ( ui8Opcode == USBD_OPCODE_ISOC_WRITE_START )
		{
			ep = psDevContext->psEpIn[ 0 ];
			psDevContext->eXferStatus = USBD_XFER_STATUS_WRITE;
			psDevContext->ui32Channel = 0;
		}
		else if ( ui8Opcode == USBD_OPCODE_ISOC_READ_START )
		{
			ep = psDevContext->psEpOut[ 0 ];
			psDevContext->eXferStatus = USBD_XFER_STATUS_READ;
			psDevContext->ui32Channel = 0;
		}

		/* Ensure that the endpoint is available and that the MPS for the endpoint is aligned to the DWORD boundary */
		if ( (!ep) || (ep->dwc_ep.maxpacket & 0x00000003) )
		{
			IMG_ASSERT(0);
			psDevContext->eXferStatus = USBD_XFER_STATUS_IDLE;
			return 1;
		}

		req = dwc_otg_pcd_alloc_iso_request();

		IMG_ASSERT( req );

		/*******************************************
		** BUFFER 0
		********************************************/
		req->buf0 = psTransfer->pui8Buf0;

		if ( psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical )
		{
			req->dma0 = psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical( (img_uint32)req->buf0 );
		}
		else
		{
			req->dma0 = (img_uint32)req->buf0;
		}

		/* Check data alignment of the buffer */
		IMG_ASSERT((((IMG_UINT32)req->buf0) & 0x3) == 0);

		/*******************************************
		** BUFFER 1
		********************************************/
		req->buf1 = psTransfer->pui8Buf1;

		if ( psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical )
		{
			req->dma1 = psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical( (img_uint32)req->buf1 );
		}
		else
		{
			req->dma1 = (img_uint32)req->buf1;
		}

		/* Check data alignment of the buffer */
		IMG_ASSERT((((IMG_UINT32)req->buf1) & 0x3) == 0);

		req->process_buffer = psTransfer->pfnProcessBuffer;

		req->zero = psDevContext->bReqZero;

		/* Start transaction ASAP */
		req->start_frame = -1;

		req->data_per_frame = psTransfer->ui32DataPerFrame;
		req->buf_proc_intrvl = psTransfer->ui32BufferProcessingInterval;

		/* queue up the transfer on the endpoint */
		status = dwc_otg_pcd_iso_ep_start( psBlockDesc, ep, req );

		/* Check if there were problems setting up this transfer */
		if ( status )
		{
			/*	Assert for now... in reality should do something more useful.
				e.g. check if this is the first call to the function for this
				transfer and if so then complete transfer.. otherwise exit
			*/
			IMG_ASSERT(0);
			return 1;
		}

	}
	/* There is a transfer already in progress */
	else
	{
		/* Simply assert for now */
		IMG_ASSERT(0);
		return 1;
	}

	return 0;
}
#endif
/*!
******************************************************************************

 @Function              usbd_Init

 @Description	Driver initialisation. Configures the USB hardware.
				Updates information about the device's power class and rank.

 @Input		dev			: Pointer to the QIO device object.
 			intMasks	: The system's interrupt masks, to be updated with the
 						  device's interrupt number.

 @Output	pwrClass	: Updated with the device's power class.
 			devRank		: Updated with the device's rank.

 @Return	Returns 0.

******************************************************************************/
static IMG_INT32 usbd_Init(QIO_DEVICE_T *dev, QIO_DEVPOWER_T *pwrClass, IMG_INT32 *devRank, unsigned intMasks[QIO_MAXVECGROUPS])
{
	ioblock_sBlockDescriptor	*	psBlockDesc;
	USBD_INIT_PARAM_T			*	psInitParam;
	img_int32						n;
	img_uint32						uiReg;
	USB_DC_T					*	psGlobalContext;
	USBD_DC_T					*	psContext;
	USB_sTransferMemory			*	psXferMem;
	img_uint32						ui32Addr;
	img_void					*	pvContextSpace;
	img_int32						iLockState;

	/* No power saving */
	*pwrClass = QIO_POWERNONE;

	/* Only a single rank */
	*devRank = 1;

	/* Pointer to init params is passed via the id parameter */
	psInitParam = (USBD_INIT_PARAM_T *)dev->id;
	IMG_ASSERT( psInitParam );
	psBlockDesc		= g_apsUSBDBlock[ psInitParam->ui32BlockIndex ];

	if ( !psInitParam->bMainInit )
	{
		// Calculate interrupt info for USB Core */
		IOBLOCK_CalculateInterruptInformation( psBlockDesc );

		IMG_MEMCPY( intMasks, psBlockDesc->ui32IntMasks, sizeof( unsigned int ) * QIO_MAXVECGROUPS );
		return 0;
	}

	// Check we have enough memory to "allocate" our structures
	IMG_ASSERT( sizeof( USB_DC_T ) + sizeof( USB_sTransferMemory ) + 4 <= INTERNAL_MEM_SIZE );

	// Setup pointers to structs
	pvContextSpace	= psBlockDesc->pvAPIContext;
	psGlobalContext = (USB_DC_T *)( pvContextSpace );
	ui32Addr		= (img_uint32)( pvContextSpace + sizeof( USB_DC_T ));
	ui32Addr		= ((ui32Addr - 1) & ~(3)) + 4;		// Align transfer memory to 4 bytes
	psXferMem		= (USB_sTransferMemory *)( ui32Addr );

	psContext = &(psGlobalContext->sDevContext);
	IMG_MEMSET( psContext, 0, sizeof( USBD_DC_T ) );

	//psContext->sOtgPcd.speed = USB_SPEED_HIGH;
	psContext->sOtgPcd.speed = psInitParam->EnumerateSpeed;
	psContext->sOtgPcd.setup_pkt = (img_void *)psXferMem->aui8SetupPkt;
	psContext->sOtgPcd.status_buf = (img_uint16 *)psXferMem->aui8StatusBuf;
	psContext->sOtgPcd.ep0.dwc_ep.desc_addr = &((dwc_otg_dev_dma_desc_t *)psXferMem->aui8DMAEpDesc)[ 0 ];
	for ( n = 0; n < MAX_EPS_CHANNELS - 1; ++n )
	{
		psContext->sOtgPcd.in_ep[ n ].dwc_ep.desc_addr = &((dwc_otg_dev_dma_desc_t *)psXferMem->aui8DMAEpDesc)[ (n + 1) * DESCRIPTORS_PER_EP ];
		psContext->sOtgPcd.out_ep[ n ].dwc_ep.desc_addr = &((dwc_otg_dev_dma_desc_t *)psXferMem->aui8DMAEpDesc)[ ((n + 1) + (MAX_EPS_CHANNELS - 1)) * DESCRIPTORS_PER_EP ];
	}

  #if defined (_EN_ISOC_)
	psContext->sOtgPcd.ep0.dwc_ep.pkt_info = &((iso_pkt_info_t *)psXferMem->aui8IsocPktInfo)[0];
	for ( n = 0; n < MAX_EPS_CHANNELS - 1; ++n )
	{
		// We use the same descriptor memory for iso transfers as for non-iso transfers.
		psContext->sOtgPcd.in_ep[ n ].dwc_ep.iso_desc_addr = &((dwc_otg_dev_dma_desc_t *)psXferMem->aui8DMAEpDesc)[ (n + 1) * DESCRIPTORS_PER_EP ];
		psContext->sOtgPcd.out_ep[ n ].dwc_ep.iso_desc_addr = &((dwc_otg_dev_dma_desc_t *)psXferMem->aui8DMAEpDesc)[ ((n + 1) + (MAX_EPS_CHANNELS - 1)) * DESCRIPTORS_PER_EP ];
		// ISO Packet info
		psContext->sOtgPcd.in_ep[ n ].dwc_ep.pkt_info = &((iso_pkt_info_t *)psXferMem->aui8IsocPktInfo)[ n + 1 ];
		psContext->sOtgPcd.out_ep[ n ].dwc_ep.pkt_info = &((iso_pkt_info_t *)psXferMem->aui8IsocPktInfo)[ n + 1 + (MAX_EPS_CHANNELS - 1) ];
	}
  #endif /* _EN_ISOC_ */


	IMG_MEMSET( &(psGlobalContext->sOtgCoreIf), 0, sizeof( dwc_otg_core_if_t ) );

	psContext->psOtgDevIf = &(psGlobalContext->sOtgCoreIf.dev_if);
	psContext->psOtgDevIf->setup_desc_addr[0] = &((dwc_otg_dev_dma_desc_t *)psXferMem->aui8DMASetupDesc)[0];
	psContext->psOtgDevIf->setup_desc_addr[1] = &((dwc_otg_dev_dma_desc_t *)psXferMem->aui8DMASetupDesc)[1];
	psContext->psOtgDevIf->in_desc_addr = (dwc_otg_dev_dma_desc_t *)psXferMem->aui8DMAInDesc;
	psContext->psOtgDevIf->out_desc_addr = (dwc_otg_dev_dma_desc_t *)psXferMem->aui8DMAOutDesc;

	/* Initialise the hibernate queue */
	DQ_init( &psContext->hibernateQ );

	psContext->bReqZero = psInitParam->iEnableZLP;

	#if !defined (USBD_NO_CBMAN_SUPPORT)
		/* Initialise the Callback Manager */
		CBMAN_Initialise();

		/* Register a callback system on a slot */
		CBMAN_RegisterMyCBSystem(&psContext->ui32CallbackSlot);
	#endif


	/* hardware abstraction layer initialisation */
	usbd_InitialiseHAL();

	usbd_EnumerateDescriptors( psContext, psInitParam );

	/* Program the power attributes into the configuration descriptors */
	{
		IMG_UINT8	ui8bmAttributes = USB_CONFIG_ATT_ONE;											/* Bit7 needs to be set at all times */
		IMG_UINT8	ui8MaxPower = (IMG_UINT8) (((psInitParam->sUsbdPower.ui32MaxPower)>>1) & 0xFF);	/* obtain the value in terms of 2mA units */

		if ( psInitParam->sUsbdPower.bSelfPowered)
		{
			ui8bmAttributes |= USB_CONFIG_ATT_SELFPOWER;											/* Set the self-powered bit if appropriate */
			psContext->ui8SelfPowered = 1;
		}
		else
		{
			psContext->ui8SelfPowered = 0;
		}

		/* Full speed configuration */
		psContext->sLogicalDeviceInfo.asConfigsFullSpeed[0].psDescriptor->bmAttributes = ui8bmAttributes;
		psContext->sLogicalDeviceInfo.asConfigsFullSpeed[0].psDescriptor->bMaxPower = ui8MaxPower;

		/* High speed configuration */
		psContext->sLogicalDeviceInfo.asConfigsHighSpeed[0].psDescriptor->bmAttributes = ui8bmAttributes;
		psContext->sLogicalDeviceInfo.asConfigsHighSpeed[0].psDescriptor->bMaxPower = ui8MaxPower;
	}

	/* Store the pointer to the strings */
	psContext->ppcStrings = (IMG_UINT8 **)psInitParam->ppsStrings;

  #if defined (USBD_NO_CBMAN_SUPPORT)
	/* Callback function registration */
	if (psInitParam->pfCallbackFunc)
	{
		psContext->pfCallback = psInitParam->pfCallbackFunc;
		psContext->uiRegisteredEvents = psInitParam->uiCallbackEvents;
	}
  #endif

	/* Soft reset core */
	dwc_otg_core_reset( psBlockDesc->ui32Base );

	/* Perform initialisation of the core */
	usbd_InitialiseHardware( psBlockDesc, psInitParam->EnumerateSpeed );

	/* Initialise the request queue */
	dwc_otg_pcd_initialise_request_list();

  #if defined (_EN_ISOC_)
	dwc_otg_pcd_initialise_iso_request_list();
  #endif

	/*********************************************************************
							Set up interrupts

	Interrupts are enabled through a separate call to USBD_EnableCoreInterrupts.
	**********************************************************************/

	// Calculate interrupt info for USB Core */
	IOBLOCK_CalculateInterruptInformation( psBlockDesc );

	IMG_MEMCPY( intMasks, psBlockDesc->ui32IntMasks, sizeof( unsigned int ) * QIO_MAXVECGROUPS );

	TBI_LOCK( iLockState );

	// HWLEVELEXT for USB
	uiReg = READ( psBlockDesc->sDeviceISRInfo.ui32LEVELEXTAddress );
	if ( psBlockDesc->eInterruptLevelType == HWLEVELEXT_LATCHED )
	{
		uiReg &= ~( psBlockDesc->sDeviceISRInfo.ui32LEVELEXTMask );
	}
	else if ( psBlockDesc->eInterruptLevelType == HWLEVELEXT_NON_LATCHED )
	{
		uiReg |= psBlockDesc->sDeviceISRInfo.ui32LEVELEXTMask;
	}
	WRITE( psBlockDesc->sDeviceISRInfo.ui32LEVELEXTAddress, uiReg );

	TBI_UNLOCK( iLockState );

	/* Store the block index in the id member */
	dev->id = (int)psInitParam->ui32BlockIndex;

    return 0;
}


/*!
******************************************************************************

 @Function              usbd_Start

 @Description	Device operation entry point. This function is called when a
				new operation is de-queued.

 @Input		dev			: Pointer to the QIO device object.
 			ioPars		: Pointer to the IO parameters.

 @Output	None

 @Return	IMG_VOID

******************************************************************************/
static IMG_VOID usbd_Start(QIO_DEVICE_T *dev, QIO_IOPARS_T *ioPars)
{
	ioblock_sBlockDescriptor	*	psBlockDesc	= g_apsUSBDBlock[ dev->id & 0x0000FFFF ];
	USBD_sEP					*	psEP = (USBD_sEP *)ioPars->opcode;

	psEP->psCurrentIo = ioPars;

	if(ioPars == 0)
	{
		IMG_ASSERT(0); /* Data missing */
	}
	else
	{
		IMG_UINT8 *pcBuf = ioPars->pointer;
		IMG_UINT32 uiLength = ioPars->counter;

		/*	The driver will use the counter to signal back to the caller
			the number of bytes that have been transferred. Set it to 0
			to start with
		*/
		ioPars->counter = 0;

		usbd_EpStartXfer( psBlockDesc, psEP, pcBuf, uiLength );
	}

	return;
}

/*!
******************************************************************************

 @Function              usbd_Cancel

 @Description	Transfer cancellation routine

 @Input		dev		: Pointer to the QIO device object.

 @Output	None

 @Return	IMG_VOID

******************************************************************************/
static IMG_VOID usbd_Cancel(QIO_DEVICE_T *dev)
{
	ioblock_sBlockDescriptor	*	psBlockDesc		= g_apsUSBDBlock[ dev->id & 0x0000FFFF ];
	USB_DC_T					*	psContext		= (USB_DC_T *)(psBlockDesc->pvAPIContext);
	img_uint32						ui32EPNum		= (dev->id & 0xFFFF0000) >> 16;
	USBD_sEP					*	psUSBDEP		= &psContext->asUSBDEP[ ui32EPNum - 1 ];

	/* Assert the bCancel flag */
	psUSBDEP->bCancel = IMG_TRUE;

	/* Force the current transfer to complete, by resetting the appropriate endpoint */
	if (psUSBDEP->ui32XferStatus == USBD_XFER_STATUS_ACTIVE)
	{
		dwc_otg_pcd_ep_reset( psBlockDesc, (dwc_otg_pcd_ep_t *)psUSBDEP->pvEP );
	}

	/* Deassert the bCancel flag */
	psUSBDEP->bCancel = IMG_FALSE;
}



/* Use LOG_USB_ISR_EVENTS to switch ISR event logging on/off */
//#define LOG_USB_ISR_EVENTS
#if defined (LOG_USB_ISR_EVENTS) /* ISR Logging */
#define USB_ISR_LOG_SIZE			(100)
IMG_UINT32	ui32USBIsrLogCounter = 0;
IMG_UINT32	aui32USBIsrLog[USB_ISR_LOG_SIZE];
#endif

/*!
******************************************************************************

 @Function              usbd_Isr

 @Description	Interrupt service routine

 @Input		dev		: Pointer to the QIO device object.

 @Output	None

 @Return	IMG_VOID

******************************************************************************/
static IMG_VOID usbd_Isr(QIO_DEVICE_T *dev)
{
	IMG_INT32						retval;
	IMG_UINT32						ui32Trigger;
	img_uint32						ui32BlockIndex = dev->id & 0x0000FFFF;
	ioblock_sBlockDescriptor	*	psBlockDesc = g_apsUSBDBlock[ ui32BlockIndex ];
	USB_DC_T					*	psContext = (USB_DC_T *)(psBlockDesc->pvAPIContext);
	USBD_DC_T					*	psDevContext = &(psContext->sDevContext);

	ui32Trigger = READ( psBlockDesc->sDeviceISRInfo.ui32STATEXTAddress );

	/* Check trigger */
	if ( !(ui32Trigger & psBlockDesc->sDeviceISRInfo.ui32STATEXTMask) )
	{
		/* spurious interrupt? */
		IMG_ASSERT(0);
		return;
	}

	/* We are inside the ISR */
	psDevContext->bInsideISR = IMG_TRUE;

  #ifdef LOG_USB_ISR_EVENTS /* ISR Logging */
	if (ui32USBIsrLogCounter < USB_ISR_LOG_SIZE)
	{
		aui32USBIsrLog[ui32USBIsrLogCounter] = (usb_ReadReg32(psBlockDesc->ui32Base, 0x14) & usb_ReadReg32(psBlockDesc->ui32Base, 0x18));
		ui32USBIsrLogCounter++;
	}
  #endif

	/* Mask all interrupts using the global interrupt mask (bit0 of GAHBCFG register)  */
	usb_ModifyReg32(psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GAHBCFG_OFFSET, 0x00000001, 0);

	/* Handle common interrupts */
	retval = dwc_otg_handle_common_intr( psBlockDesc->ui32Base, &(psContext->sOtgCoreIf) );

	/* Handle device specific interrupts */
	retval = dwc_otg_pcd_handle_intr( psBlockDesc );

	if ( psBlockDesc->eInterruptLevelType == HWLEVELEXT_LATCHED )
	{
		img_uint32 iLockState;
		TBI_LOCK( iLockState );
		WRITE( psBlockDesc->sDeviceISRInfo.ui32STATEXTAddress, psBlockDesc->sDeviceISRInfo.ui32STATEXTMask );
		TBI_UNLOCK( iLockState );
	}

	/* Unmask all interrupts before exiting the ISR */
	usb_ModifyReg32(psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GAHBCFG_OFFSET, 0, 0x00000001);

	/* Update bInsideISR variable */
	psDevContext->bInsideISR = IMG_FALSE;

  #if defined(TIMER_NOT_AVAILABLE)
	if (bStartXferTimer)
	{
		 bStartXferTimer = IMG_FALSE;
		 start_xfer_func( (IMG_VOID *)psBlockDesc );
	}
	if (bDoTestMode)
	{
		 bDoTestMode = IMG_FALSE;
		 do_test_mode_func( (IMG_VOID *)psBlockDesc );
	}
  #endif
	return;
}

/*!
******************************************************************************

 @Function              USBD_PerformSoftDisconnect

 @Description	Perform a soft disconnect on the USB bus

******************************************************************************/
IMG_VOID USBD_PerformSoftDisconnect( IMG_UINT32 ui32BlockIndex )
{
	KRN_TASKQ_T		hibernateQ;
	dctl_data_t	dctl = {0};

	DQ_init(&hibernateQ);

	dctl.d32 = usb_ReadReg32( g_apsUSBDBlock[ ui32BlockIndex ]->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET);
	dctl.b.sftdiscon = 1;
	usb_WriteReg32( g_apsUSBDBlock[ ui32BlockIndex ]->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, dctl.d32);

	KRN_hibernate(&hibernateQ, MILLISECOND_TO_TICK(1)); /* delay to allow the USB host to detect the disconnect */
}

/*!
******************************************************************************

 @Function              USBD_ForceDeviceMode

 @Description	Forces controller into device mode

******************************************************************************/
IMG_VOID USBD_ForceDeviceMode( IMG_UINT32		ui32BlockIndex )
{
	KRN_TASKQ_T		hibernateQ;
	gusbcfg_data_t	gusbcfg = { 0 };

	DQ_init(&hibernateQ);

	gusbcfg.d32 = usb_ReadReg32( g_apsUSBDBlock[ ui32BlockIndex ]->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET );
	gusbcfg.b.forcedevmode = 1;
	usb_WriteReg32( g_apsUSBDBlock[ ui32BlockIndex ]->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET, gusbcfg.d32 );

	// Delay for 25ms
	KRN_hibernate(&hibernateQ, MILLISECOND_TO_TICK(25));
}



/*!
******************************************************************************

 @Function              USBD_EnableCoreInterrupts

 @Description		Enable/Disable USBD interrupts. The function should be called
					after the driver has been initialised through the QIO.

******************************************************************************/
IMG_VOID USBD_EnableCoreInterrupts( IMG_UINT32	ui32BlockIndex, IMG_BOOL	bEnable )
{
	KRN_IPL_T	oldipl;

	oldipl = KRN_raiseIPL();

	if (bEnable)
	{
		/* Unmask USB interrupts */
		usb_ModifyReg32( g_apsUSBDBlock[ ui32BlockIndex ]->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GAHBCFG_OFFSET, 0, 0x00000001);
	}
	else
	{
		/* Mask USB interrupts */
		usb_ModifyReg32( g_apsUSBDBlock[ ui32BlockIndex ]->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GAHBCFG_OFFSET, 0x00000001, 0);
	}

	KRN_restoreIPL(oldipl);
}

/*!
******************************************************************************

 @Function              USBD_IsInitialised

 @Description		Checks if the driver has been initialised

******************************************************************************/
img_bool	USBD_IsInitialised(	img_uint32	ui32BlockIndex )
{
	USB_DC_T	*	psContext = USB_GET_CONTEXT( ui32BlockIndex );

	return psContext->bInitialised;
}

/*!
******************************************************************************

 @Function              USBD_SetInitialised

 @Description		Sets the state of driver initialisation

******************************************************************************/
img_void	USBD_SetInitialised( img_uint32	ui32BlockIndex, img_bool	bInitialised )
{
	USB_DC_T	*	psContext = USB_GET_CONTEXT( ui32BlockIndex );

	psContext->bInitialised = bInitialised;
}

/*!
******************************************************************************

 @Function              USBD_GetEP

 @Description		Sets a pointer to a USBD API Endpoint

******************************************************************************/
img_void	 USBD_GetEP(	img_uint32	ui32BlockIndex, img_uint32	ui32EPNum,	USBD_sEP	**ppsEP	)
{
	USB_DC_T		*	psContext = USB_GET_CONTEXT( ui32BlockIndex );

	IMG_ASSERT( ui32EPNum > 0 && ui32EPNum <= USBD_MAX_EPS );

	*ppsEP = &psContext->asUSBDEP[ ui32EPNum - 1 ];
}


#if !defined (USBD_NO_CBMAN_SUPPORT)
/*!
******************************************************************************

 @Function              USBD_AddCallback

 @Description		Adds a callback

******************************************************************************/
img_void	USBD_AddCallback(	img_uint32					ui32BlockIndex,
								USBD_EVENT_T				eEvent,
								IMG_pfnEventCallback		pfnEventCallback,
								img_void				*	pvCallbackParameter,
								IMG_hCallback			*	phEventCallback )
{
	USB_DC_T	*	psContext = USB_GET_CONTEXT( ui32BlockIndex );

	CBMAN_AddCallbackToMyCBSystem(	psContext->sDevContext.ui32CallbackSlot,
									eEvent,
									pfnEventCallback,
									pvCallbackParameter,
									phEventCallback		);
}

USBD_RETURN_T	USBD_ExecuteCallback(	img_uint32			ui32BlockIndex,
										USBD_EVENT_T		eEvent,
										img_uint32			ui32Param,
										img_void		*	pvParam	)
{
	img_result		iResult;
	USB_DC_T	*	psContext = USB_GET_CONTEXT( ui32BlockIndex );

	iResult = CBMAN_ExecuteCallbackWithEventType(	psContext->sDevContext.ui32CallbackSlot,
													eEvent,
													ui32Param,
													pvParam	);

	IMG_ASSERT( iResult == IMG_SUCCESS );

	return USBD_STATUS_SUCCESS;
}

#endif

/* Remote wakeup functionality - not used normally */
#if defined(REMOTE_WAKEUP_FEATURE) && !defined(TIMER_NOT_AVAILABLE)

KRN_TIMER_T	tStartRmtWkUpTimer;

IMG_VOID usbd_RmtWkUpTimerCallback (KRN_TIMER_T *timer, IMG_VOID * pParam)
{
	dctl_data_t		dctl	= {0};

	/* Clear remote wakeup */
	dctl.b.rmtwkupsig = 1;
	usb_ModifyReg32( DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, dctl.d32, 0 );
}

/*!
******************************************************************************

 @Function              USBDRemoteWakeUp

 @Description		Initialise remote wakeup

******************************************************************************/
IMG_VOID		USBDRemoteWakeUp( )
{
	dsts_data_t		dsts;
	dctl_data_t		dctl	= {0};

	if (psContext->psOtgPcd->remote_wakeup_enable)
	{
		/* Check if suspend state */
		dsts.d32 = usb_ReadReg32(psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DSTS_OFFSET);

		if (dsts.b.suspsts)
		{
			/* Set remote wakeup */
			dctl.b.rmtwkupsig = 1;
			usb_ModifyReg32( DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, 0, dctl.d32 );

			/* Kick off timer that will switch off remote wakeup*/
			KRN_setTimer(&tStartRmtWkUpTimer, &usbd_RmtWkUpTimerCallback, 0, 0);
		}
	}
}
#endif
