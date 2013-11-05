 /* ==========================================================================
 * Synopsys HS OTG Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 * 
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 * 
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================== */
#ifndef DWC_HOST_ONLY

/** @file 
 * This file implements the Peripheral Controller Driver.
 *
 * The Peripheral Controller Driver (PCD) is responsible for
 * translating requests from the Function Driver into the appropriate
 * actions on the DWC_otg controller. It isolates the Function Driver
 * from the specifics of the controller by providing an API to the
 * Function Driver. 
 *
 * The Peripheral Controller Driver for Linux will implement the
 * Gadget API, so that the existing Gadget drivers can be used.
 * (Gadget Driver is the Linux terminology for a Function Driver.)
 * 
 * The Linux Gadget API is defined in the header file
 * <code><linux/usb_gadget.h></code>.  The USB EP operations API is
 * defined in the structure <code>usb_ep_ops</code> and the USB
 * Controller API is defined in the structure
 * <code>usb_gadget_ops</code>.
 *
 * An important function of the PCD is managing interrupts generated
 * by the DWC_otg controller. The implementation of the DWC_otg device
 * mode interrupt service routines is in dwc_otg_pcd_intr.c.
 *
 * IMG_SYNOP_CR
 * @Potential enhancements: 
 *	- Add Device Mode test modes (Test J mode, Test K mode, etc).
 *	- Add support for request size > DEPTSIZ transfer size
 *
 */
 
#if defined __META_MEOS__ || defined __MTX_MEOS__ || defined __META_NUCLEUS_PLUS__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <MeOS.h>

#include "img_defs.h"
#include "ioblock_defs.h"

#include "usbd_api.h"
#include "usb_hal.h"
#include <usb_spec.h>
#include "dwc_otg_pcd.h"
#include "usb_defs.h"
#include "usb_drv.h"


/**
 * This function completes a request.  It call's the request call back.
 */
IMG_VOID request_done( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep, dwc_otg_pcd_request_t *req, IMG_INT32 status )
{
	unsigned stopped = ep->stopped;

	DWC_DEBUGPL(DBG_PCDV, "%s(%p)\n",  ep);
	
	LST_remove(&ep->sRequestList, req);

	if (req->status == -USBD_ERR_INPROGRESS) 
	{
		req->status = status;
	} 
	else 
	{	
		status = req->status;
	}
		
	/* don't modify queue heads during completion callback */
	ep->stopped = 1;
	
	if (ep->request_pending > 0)
	{
		--ep->request_pending;
	}

	SPIN_UNLOCK(&ep->pcd->lock);
	//invoke the callback function....
	req->complete( psBlockDesc, ep, req);
	SPIN_LOCK(&ep->pcd->lock);
		
	ep->stopped = stopped;
}

/**
 * This function terminates all the requsts in the EP request queue.
 */
IMG_VOID dwc_otg_pcd_ep_request_nuke( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep )
{
	dwc_otg_pcd_request_t *req;

	ep->stopped = 1;

	while (!LST_empty(&ep->sRequestList))
	{
		req = (dwc_otg_pcd_request_t *) LST_first(&ep->sRequestList);
		request_done( psBlockDesc, ep, req, -USBD_ERR_SHUTDOWN );
	}
}


#ifdef DWC_SUPPORT_OTG
/**
 * This function updates the otg values in the gadget structure. 
 */
IMG_VOID dwc_otg_pcd_update_otg( const unsigned _reset )
{
	dwc_otg_pcd_t *_pcd	 = sUSBDDeviceContext.psOtgPcd;
	if (!_pcd->is_otg)
		return;

	if (_reset) 
	{
		_pcd->b_hnp_enable = 0;
		_pcd->a_hnp_support = 0;
		_pcd->a_alt_hnp_support = 0;
	}

	//_pcd->gadget.b_hnp_enable = _pcd->b_hnp_enable;
	//_pcd->gadget.a_hnp_support =  _pcd->a_hnp_support;
	//_pcd->gadget.a_alt_hnp_support = _pcd->a_alt_hnp_support;
}

/**
 * PCD Callback function for initializing the PCD when switching to
 * device mode.
 *
 * @param _p IMG_VOID pointer to the <code>dwc_otg_pcd_t</code>
 */
static IMG_INT32 dwc_otg_pcd_start_cb( IMG_VOID *_p )
{
	dwc_otg_core_dev_init();
	return 1;
}

/**
 * PCD Callback function for stopping the PCD when switching to Host
 * mode.
 *
 * @param _p IMG_VOID pointer to the <code>dwc_otg_pcd_t</code>
 */
static IMG_INT32 dwc_otg_pcd_stop_cb( IMG_VOID *_p )
{
	/*dwc_otg_pcd_t *pcd = (dwc_otg_pcd_t *)_p;*/
	extern IMG_VOID dwc_otg_pcd_stop( );
	
	dwc_otg_pcd_stop( );
	return 1;
}
#endif  //DWC_SUPPORT_OTG

/**
 * PCD Callback function for notifying the PCD when going into
 * suspend.
 *
 * @param _p IMG_VOID pointer to the <code>dwc_otg_pcd_t</code>
 */
static IMG_INT32 dwc_otg_pcd_suspend_cb( IMG_VOID *p  )
{
	SPIN_UNLOCK(&pcd->lock);
	usbd_Suspend( p );
	SPIN_LOCK(&pcd->lock);
	
	return 1;
}


/**
 * PCD Callback function for notifying the PCD when resuming from
 * suspend.
 *
 * @param _p IMG_VOID pointer to the <code>dwc_otg_pcd_t</code>
 */
static IMG_INT32 dwc_otg_pcd_resume_cb( IMG_VOID *p )
{
	SPIN_UNLOCK(&pcd->lock);
	usbd_Resume( p );
	SPIN_LOCK(&pcd->lock);
	
#ifdef DWC_SUPPORT_OTG
	/* Stop the SRP timeout timer. */
  #ifdef USE_I2C
	if ((core_if->core_params->phy_type != DWC_PHY_TYPE_PARAM_FS) ||
		(!core_if->core_params->i2c_enable))
  #endif
	{
		if (core_if->srp_timer_started) 
		{
			core_if->srp_timer_started = 0;
			del_timer( &pcd->srp_timer );
		}
	}
#endif

	return 1;
}


/**
 * PCD Callback structure for handling mode switching.
 */
static dwc_otg_cil_callbacks_t pcd_callbacks = 
{
  #ifdef DWC_SUPPORT_OTG
	dwc_otg_pcd_start_cb,
	dwc_otg_pcd_stop_cb,
  #else
	0,
	0,
  #endif
	0,
	dwc_otg_pcd_resume_cb,
	dwc_otg_pcd_suspend_cb,
	0,
	0, /* Set at registration */
};


/* USB Endpoint Operations */

#if defined (_EN_ISOC_)

/**
 * This function initializes a descriptor chain for Isochronous transfer
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param dwc_ep The EP to start the transfer on.
 *
 */
void dwc_otg_iso_ep_start_ddma_transfer( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_core_if_t * core_if, dwc_ep_t * dwc_ep)
{
	dsts_data_t		dsts = { 0 };
	depctl_data_t	depctl = { 0 };
	img_uint32		addr;
	int i, j;

	if (dwc_ep->is_in)
	{
		dwc_ep->desc_cnt = dwc_ep->buf_proc_intrvl / dwc_ep->bInterval;
	}
	else
	{
		dwc_ep->desc_cnt = dwc_ep->buf_proc_intrvl * dwc_ep->pkt_per_frm / dwc_ep->bInterval;
	}

	dsts.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DSTS_OFFSET );

	/** ISO OUT EP */
	if (dwc_ep->is_in == 0) 
	{
		dev_dma_desc_sts_t		sts = { 0 };
		dwc_otg_dev_dma_desc_t *dma_desc = dwc_ep->iso_desc_addr;
		img_uint32 				dma_ad;
		img_uint32 				data_per_desc;
		//int						offset;

		addr = DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + dwc_ep->num * DWC_EP_REG_OFFSET;
		dma_ad = (img_uint32)usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0 + dwc_ep->num * DWC_EP_REG_OFFSET ); // This line makes no sense O.o

		/** Buffer 0 descriptors setup */
		dma_ad = dwc_ep->dma_addr0;

		sts.b_iso_out.bs = BS_HOST_READY;
		sts.b_iso_out.rxsts = 0;
		sts.b_iso_out.l = 0;
		sts.b_iso_out.sp = 0;
		sts.b_iso_out.ioc = 0;
		sts.b_iso_out.pid = 0;
		sts.b_iso_out.framenum = 0;

		//offset = 0;
		for (i = 0; i < dwc_ep->desc_cnt - dwc_ep->pkt_per_frm; i += dwc_ep->pkt_per_frm) 
		{
			for (j = 0; j < dwc_ep->pkt_per_frm; ++j) 
			{
				data_per_desc = ((j + 1) * dwc_ep->maxpacket > dwc_ep->data_per_frame) ? dwc_ep->data_per_frame - j * dwc_ep->maxpacket : dwc_ep->maxpacket;

				data_per_desc += (data_per_desc % 4) ? (4 - data_per_desc % 4) : 0;
				
				sts.b_iso_out.rxbytes = data_per_desc;
				
				dma_desc->buf = dma_ad;
				dma_desc->status.d32 = sts.d32;

				//offset += data_per_desc;
				dma_desc++;
				dma_ad += data_per_desc;
			}
		}

		for (j = 0; j < dwc_ep->pkt_per_frm - 1; ++j) 
		{
			data_per_desc = ((j + 1) * dwc_ep->maxpacket > dwc_ep->data_per_frame) ? dwc_ep->data_per_frame - j * dwc_ep->maxpacket : dwc_ep->maxpacket;
			data_per_desc += (data_per_desc % 4) ? (4 - data_per_desc % 4) : 0;
			
			sts.b_iso_out.rxbytes = data_per_desc;
			
			dma_desc->buf = dma_ad;
			dma_desc->status.d32 = sts.d32;

			//offset += data_per_desc;
			dma_desc++;
			dma_ad += data_per_desc;
		}

		sts.b_iso_out.ioc = 1;
		data_per_desc = ((j + 1) * dwc_ep->maxpacket > dwc_ep->data_per_frame) ? dwc_ep->data_per_frame - j * dwc_ep->maxpacket : dwc_ep->maxpacket;
		data_per_desc += (data_per_desc % 4) ? (4 - data_per_desc % 4) : 0;
		sts.b_iso_out.rxbytes = data_per_desc;

		dma_desc->buf = dma_ad;
		dma_desc->status.d32 = sts.d32;
		dma_desc++;

		/** Buffer 1 descriptors setup */
		sts.b_iso_out.ioc = 0;
		dma_ad = dwc_ep->dma_addr1;

		// offset = 0;
		for (i = 0; i < dwc_ep->desc_cnt - dwc_ep->pkt_per_frm; i += dwc_ep->pkt_per_frm) 
		{
			for (j = 0; j < dwc_ep->pkt_per_frm; ++j) 
			{
				data_per_desc = ((j + 1) * dwc_ep->maxpacket > dwc_ep->data_per_frame) ? dwc_ep->data_per_frame - j * dwc_ep->maxpacket : dwc_ep->maxpacket;
				data_per_desc += (data_per_desc % 4) ? (4 - data_per_desc % 4) : 0;
				sts.b_iso_out.rxbytes = data_per_desc;
				dma_desc->buf = dma_ad;
				dma_desc->status.d32 = sts.d32;

				// offset += data_per_desc;
				dma_desc++;
				dma_ad += data_per_desc;
			}
		}
		for (j = 0; j < dwc_ep->pkt_per_frm - 1; ++j) 
		{
			data_per_desc = ((j + 1) * dwc_ep->maxpacket > dwc_ep->data_per_frame) ? dwc_ep->data_per_frame - j * dwc_ep->maxpacket : dwc_ep->maxpacket;
			data_per_desc += (data_per_desc % 4) ? (4 - data_per_desc % 4) : 0;
			sts.b_iso_out.rxbytes = data_per_desc;
			dma_desc->buf = dma_ad;
			dma_desc->status.d32 = sts.d32;

			// offset += data_per_desc;
			dma_desc++;
			dma_ad += data_per_desc;
		}

		sts.b_iso_out.ioc = 1;
		sts.b_iso_out.l = 1;
		data_per_desc = ((j + 1) * dwc_ep->maxpacket > dwc_ep->data_per_frame) ? dwc_ep->data_per_frame - j * dwc_ep->maxpacket : dwc_ep->maxpacket;
		data_per_desc += (data_per_desc % 4) ? (4 - data_per_desc % 4) : 0;
		sts.b_iso_out.rxbytes = data_per_desc;

		dma_desc->buf = dma_ad;
		dma_desc->status.d32 = sts.d32;

		dwc_ep->next_frame = 0;

		/** Write dma_ad into DOEPDMA register */
		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0 + dwc_ep->num * DWC_EP_REG_OFFSET, dwc_ep->iso_dma_desc_addr );
	}
	/** ISO IN EP */
	else 
	{
		dev_dma_desc_sts_t			sts = { 0 };
		dwc_otg_dev_dma_desc_t *	dma_desc = dwc_ep->iso_desc_addr;
		img_uint32					dma_ad;
		unsigned int				frmnumber;
		fifosize_data_t				txfifosize, rxfifosize;

		txfifosize.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DTXFSTS0 * dwc_ep->num * DWC_EP_REG_OFFSET );
		rxfifosize.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GRXFSIZ_OFFSET );

		addr = DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + dwc_ep->num * DWC_EP_REG_OFFSET;

		dma_ad = dwc_ep->dma_addr0;

		dsts.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DSTS_OFFSET );

		sts.b_iso_in.bs = BS_HOST_READY;
		sts.b_iso_in.txsts = 0;
		sts.b_iso_in.sp = (dwc_ep->data_per_frame % dwc_ep->maxpacket) ? 1 : 0;
		sts.b_iso_in.ioc = 0;
		sts.b_iso_in.pid = dwc_ep->pkt_per_frm;

		frmnumber = dwc_ep->next_frame;

		sts.b_iso_in.framenum = frmnumber;
		sts.b_iso_in.txbytes = dwc_ep->data_per_frame;
		sts.b_iso_in.l = 0;

		/** Buffer 0 descriptors setup */
		for (i = 0; i < dwc_ep->desc_cnt - 1; i++) 
		{
			dma_desc->buf = dma_ad;
			dma_desc->status.d32 = sts.d32;
			dma_desc++;

			dma_ad += dwc_ep->data_per_frame;
			sts.b_iso_in.framenum += dwc_ep->bInterval;
		}

		sts.b_iso_in.ioc = 1;
		dma_desc->buf = dma_ad;
		dma_desc->status.d32 = sts.d32;
		++dma_desc;

		/** Buffer 1 descriptors setup */
		sts.b_iso_in.ioc = 0;
		dma_ad = dwc_ep->dma_addr1;

		for (i = 0; i < dwc_ep->desc_cnt - dwc_ep->pkt_per_frm; i += dwc_ep->pkt_per_frm) 
		{
			dma_desc->buf = dma_ad;
			dma_desc->status.d32 = sts.d32;
			dma_desc++;

			dma_ad += dwc_ep->data_per_frame;
			sts.b_iso_in.framenum += dwc_ep->bInterval;

			sts.b_iso_in.ioc = 0;
		}
		sts.b_iso_in.ioc = 1;
		sts.b_iso_in.l = 1;

		dma_desc->buf = dma_ad;
		dma_desc->status.d32 = sts.d32;

		dwc_ep->next_frame = sts.b_iso_in.framenum + dwc_ep->bInterval;

		/** Write dma_ad into diepdma register */
		usb_WriteMemrefReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPDMA0 + dwc_ep->num * DWC_EP_REG_OFFSET, dwc_ep->iso_dma_desc_addr );
	}
	/** Enable endpoint, clear nak  */
	depctl.d32 = 0;
	depctl.b.epena = 1;
	depctl.b.usbactep = 1;
	depctl.b.cnak = 1;

	usb_ModifyReg32( psBlockDesc->ui32Base, addr, depctl.d32, depctl.d32 );
	depctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, addr );
}

/**
 * This function initializes a descriptor chain for Isochronous transfer
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP to start the transfer on.
 *
 */
void dwc_otg_iso_ep_start_buf_transfer( const img_uint32 ui32BaseAddress, dwc_otg_core_if_t * core_if, dwc_ep_t * ep)
{
	depctl_data_t	depctl = { 0 };
	img_uint32		addr;

	if (ep->is_in) 
	{
		addr = DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + ep->num * DWC_EP_REG_OFFSET;
	} 
	else 
	{
		addr = DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->num * DWC_EP_REG_OFFSET;
	}

	if (core_if->dma_enable == 0 || core_if->dma_desc_enable != 0) 
	{
		return;
	} 
	else 
	{
		deptsiz_data_t deptsiz = { 0 };

		ep->xfer_len = ep->data_per_frame * ep->buf_proc_intrvl / ep->bInterval;
		ep->pkt_cnt = (ep->xfer_len - 1 + ep->maxpacket) / ep->maxpacket;
		ep->xfer_count = 0;
		ep->xfer_buff = (ep->proc_buf_num) ? ep->xfer_buff1 : ep->xfer_buff0;
		ep->dma_addr = (ep->proc_buf_num) ? ep->dma_addr1 : ep->dma_addr0;

		if (ep->is_in) 
		{
			/* Program the transfer size and packet count
			 *      as follows: xfersize = N * maxpacket +
			 *      short_packet pktcnt = N + (short_packet
			 *      exist ? 1 : 0)  
			 */
			deptsiz.b.mc = ep->pkt_per_frm;
			deptsiz.b.xfersize = ep->xfer_len;
			deptsiz.b.pktcnt = (ep->xfer_len - 1 + ep->maxpacket) / ep->maxpacket;
			usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + ep->num * DWC_EP_REG_OFFSET, deptsiz.d32);

			/* Write the DMA register */
			usb_WriteMemrefReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPDMA0 + ep->num * DWC_EP_REG_OFFSET, ep->dma_addr );

		} 
		else 
		{
			deptsiz.b.pktcnt = (ep->xfer_len + (ep->maxpacket - 1)) / ep->maxpacket;
			deptsiz.b.xfersize = deptsiz.b.pktcnt * ep->maxpacket;

			usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + ep->num * DWC_EP_REG_OFFSET, deptsiz.d32 );

			/* Write the DMA register */
			usb_WriteMemrefReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0 + ep->num * DWC_EP_REG_OFFSET, ep->dma_addr );
 
		}
		/** Enable endpoint, clear nak  */
		depctl.d32 = 0;
		usb_ModifyReg32( ui32BaseAddress, addr, depctl.d32, depctl.d32 );

		depctl.b.epena = 1;
		depctl.b.cnak = 1;

		usb_ModifyReg32( ui32BaseAddress, addr, depctl.d32, depctl.d32 );
	}
}

/**
 * This function does the setup for a data transfer for an EP and
 * starts the transfer.	 For an IN transfer, the packets will be
 * loaded into the appropriate Tx FIFO in the ISR. For OUT transfers,
 * the packets are unloaded from the Rx FIFO in the ISR.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP to start the transfer on.
 */

static void dwc_otg_iso_ep_start_transfer( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_core_if_t * core_if, dwc_ep_t * ep )
{
  #if !defined USE_DMA_INTERNAL
	if (core_if->dma_enable) 
  #endif /* USE_DMA_INTERNAL */
	{
		if (core_if->dma_desc_enable) 
		{
			dwc_otg_iso_ep_start_ddma_transfer( psBlockDesc, core_if, ep );
		} 
		else 
		{
			if (core_if->pti_enh_enable) 
			{
				dwc_otg_iso_ep_start_buf_transfer( psBlockDesc->ui32Base, core_if, ep );
			} 
			else 
			{
				ep->cur_pkt_addr = (ep->proc_buf_num) ? ep->xfer_buff1 : ep->xfer_buff0;
				ep->cur_pkt_dma_addr = (ep->proc_buf_num) ? ep->dma_addr1 : ep->dma_addr0;
				dwc_otg_iso_ep_start_frm_transfer( psBlockDesc->ui32Base, core_if, ep);
			}
		}
	} 
  #if !defined USE_DMA_INTERNAL
	else 
	{
		ep->cur_pkt_addr = (ep->proc_buf_num) ? ep->xfer_buff1 : ep->xfer_buff0;
		ep->cur_pkt_dma_addr = (ep->proc_buf_num) ? ep->dma_addr1 : ep->dma_addr0;
		dwc_otg_iso_ep_start_frm_transfer( psBlockDesc->ui32Base, core_if, ep);
	}
  #endif /* USE_DMA_INTERNAL */
}

/**
 * This function stops transfer for an EP and
 * resets the ep's variables.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP to start the transfer on.
 */

void dwc_otg_iso_ep_stop_transfer( const img_uint32	ui32BaseAddress, dwc_otg_core_if_t * core_if, dwc_ep_t * ep)
{
	depctl_data_t	depctl = {.d32 = 0 };
	img_uint32		addr;

	if (ep->is_in == 1) 
	{
		addr = DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + ep->num * DWC_EP_REG_OFFSET;
	} 
	else 
	{
		addr = DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->num * DWC_EP_REG_OFFSET;
	}

	/* disable the ep */
	depctl.d32 = usb_ReadReg32( ui32BaseAddress, addr );

	depctl.b.epdis = 1;
	depctl.b.snak = 1;

	usb_WriteReg32( ui32BaseAddress, addr, depctl.d32);

	/* reset varibales */
	ep->dma_addr0 = 0;
	ep->dma_addr1 = 0;
	ep->xfer_buff0 = 0;
	ep->xfer_buff1 = 0;
	ep->data_per_frame = 0;
	ep->data_pattern_frame = 0;
	ep->sync_frame = 0;
	ep->buf_proc_intrvl = 0;
	ep->bInterval = 0;
	ep->proc_buf_num = 0;
	ep->pkt_per_frm = 0;
	ep->pkt_per_frm = 0;
	ep->desc_cnt = 0;
	ep->iso_desc_addr = 0;
	ep->iso_dma_desc_addr = 0;
}

IMG_INT32 dwc_otg_pcd_iso_ep_start(	const ioblock_sBlockDescriptor	*	psBlockDesc,
									dwc_otg_pcd_ep_t				*	ep, 
									usb_iso_request_t				*	req					)
{
	dwc_ep_t			*	dwc_ep;
	img_int32				frm_data;
	dsts_data_t				dsts;
	USB_DC_T			*	psContext		= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_core_if_t	*	core_if			= &(psContext->sOtgCoreIf);

	if (!ep || !ep->desc || ep->dwc_ep.num == 0) 
	{
		DWC_WARN("bad ep\n");
		return -USBD_ERR_INVAL;
	}

	dwc_ep = &ep->dwc_ep;

	if (ep->iso_req) 
	{
		DWC_WARN("ISO request in progress\n");
	}

	dwc_ep->dma_addr0 = req->dma0;
	dwc_ep->dma_addr1 = req->dma1;

	dwc_ep->xfer_buff0 = req->buf0;
	dwc_ep->xfer_buff1 = req->buf1;

	dwc_ep->data_per_frame = req->data_per_frame;

	/* IMG_SYNOP_CR */
	/** Potential enhancements: Implement pattern data support */
	dwc_ep->data_pattern_frame = req->data_pattern_frame;
	dwc_ep->sync_frame = req->sync_frame;

	dwc_ep->buf_proc_intrvl = req->buf_proc_intrvl;

	dwc_ep->bInterval = 1 << (ep->desc->bInterval - 1);

	dwc_ep->proc_buf_num = 0;

	dwc_ep->pkt_per_frm = 0;
	frm_data = ep->dwc_ep.data_per_frame;
	while (frm_data > 0) 
	{
		dwc_ep->pkt_per_frm++;
		frm_data -= ep->dwc_ep.maxpacket;
	}

	dsts.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DSTS_OFFSET );

	if (req->start_frame == -1) 
	{
		dwc_ep->next_frame = dsts.b.soffn + 1;
		if (dwc_ep->bInterval != 1) 
		{
			dwc_ep->next_frame = dwc_ep->next_frame + (dwc_ep->bInterval - 1 - dwc_ep->next_frame % dwc_ep->bInterval);
		}
	} 
	else 
	{
		dwc_ep->next_frame = req->start_frame;
	}

	if (!core_if->pti_enh_enable) 
	{
		dwc_ep->pkt_cnt = dwc_ep->buf_proc_intrvl * dwc_ep->pkt_per_frm / dwc_ep->bInterval;
	} 
	else 
	{
		dwc_ep->pkt_cnt = (dwc_ep->data_per_frame * (dwc_ep->buf_proc_intrvl / dwc_ep->bInterval) - 1 + dwc_ep->maxpacket) / dwc_ep->maxpacket;
	}

	// Check we have enough memory for the packets
	IMG_ASSERT( dwc_ep->pkt_cnt < MAX_ISO_PACKETS );

	if (core_if->pti_enh_enable) 
	{
		IMG_MEMSET(dwc_ep->pkt_info, 0, sizeof(iso_pkt_info_t) * dwc_ep->pkt_cnt);
	}

	dwc_ep->cur_pkt = 0;
	ep->iso_req = req;

	dwc_otg_iso_ep_start_transfer( psBlockDesc, core_if, dwc_ep );
	return 0;
}

IMG_INT32 dwc_otg_pcd_iso_ep_stop( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t	*ep, usb_iso_request_t	*req )
{
	USB_DC_T			*	psContext		= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_core_if_t	*	core_if			= &(psContext->sOtgCoreIf);
	dwc_ep_t			*	dwc_ep;

	if (!ep || !ep->desc || ep->dwc_ep.num == 0) 
	{
		DWC_WARN("bad ep\n");
		return -USBD_ERR_INVAL;
	}
	dwc_ep = &ep->dwc_ep;

	dwc_otg_iso_ep_stop_transfer( psBlockDesc->ui32Base, core_if, dwc_ep );

	if (ep->iso_req != req) 
	{
		return -USBD_ERR_INVAL;
	}

	ep->iso_req = 0;
	return 0;
}

/**
 * This function is used for perodical data exchnage between PCD and gadget drivers.
 * for Isochronous EPs
 *
 *	- Every time a sync period completes this function is called to 
 *	  perform data exchange between PCD and gadget
 */
IMG_VOID dwc_otg_iso_buffer_done( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t * ep, usb_iso_request_t *req)
{
	int						i;
	dwc_ep_t			*	dwc_ep;

	dwc_ep = &ep->dwc_ep;

	SPIN_UNLOCK(&ep->pcd->lock);
	// Call complete function
	usbd_IsocXferComplete( psBlockDesc, ep, req );
	SPIN_LOCK(&ep->pcd->lock);

	for (i = 0; i < dwc_ep->pkt_cnt; ++i) 
	{
		dwc_ep->pkt_info[i].status = 0;
		dwc_ep->pkt_info[i].offset = 0;
		dwc_ep->pkt_info[i].length = 0;
	}
}

#endif				/* _EN_ISOC_ */

static IMG_VOID dwc_otg_pcd_init_ep( dwc_otg_pcd_t * pcd, dwc_otg_pcd_ep_t * pcd_ep, IMG_UINT32 is_in, IMG_UINT32 ep_num )
{
	/* Init EP structure */
	pcd_ep->desc = 0;
	pcd_ep->pcd = pcd;
	pcd_ep->stopped = 1;
	pcd_ep->queue_sof = 0;

	/* Init DWC ep structure */
	pcd_ep->dwc_ep.is_in = is_in;
	pcd_ep->dwc_ep.num = ep_num;
	pcd_ep->dwc_ep.active = 0;
	pcd_ep->dwc_ep.tx_fifo_num = 0;
	/* Control until ep is actvated */
	pcd_ep->dwc_ep.type = DWC_OTG_EP_TYPE_CONTROL; 
	pcd_ep->dwc_ep.maxpacket = USBD_MPS_DEFAULT;
	pcd_ep->dwc_ep.dma_addr = 0;
	pcd_ep->dwc_ep.start_xfer_buff = 0;
	pcd_ep->dwc_ep.xfer_buff = 0;
	pcd_ep->dwc_ep.xfer_len = 0;
	pcd_ep->dwc_ep.xfer_count = 0;
	pcd_ep->dwc_ep.sent_zlp = 0;
	pcd_ep->dwc_ep.total_len = 0;
	pcd_ep->dwc_ep.dma_desc_addr = 0;
	LST_init(&pcd_ep->sRequestList);
}

/**
 * This function initialized the pcd Dp structures to their default
 * state.
 *
 * @param _pcd the pcd structure.
 */
static IMG_VOID dwc_otg_pcd_reinit( dwc_otg_pcd_t	*	pcd, dwc_otg_core_if_t	*	core_if )
{
	IMG_INT32			i;
	IMG_UINT32			hwcfg1;
	dwc_otg_pcd_ep_t	*ep;
	IMG_INT32			in_ep_cntr, out_ep_cntr;
	IMG_UINT32			num_in_eps	= core_if->dev_if.num_in_eps;
	IMG_UINT32			num_out_eps	= core_if->dev_if.num_out_eps;
	
	pcd->speed = USB_SPEED_UNKNOWN;

	/**
	 * Initialize the EP0 structure.
	 */
	ep = &pcd->ep0;
	dwc_otg_pcd_init_ep(pcd, ep, 0, 0);

	in_ep_cntr = 0;
	hwcfg1 = core_if->hwcfg1.d32 >> 3;
	for (i = 1; in_ep_cntr < num_in_eps; i++) 
	{
		if((hwcfg1 & 0x1) == 0)
		{
			dwc_otg_pcd_ep_t *ep = &pcd->in_ep[in_ep_cntr];
			in_ep_cntr ++;
			/**
			 * Potential enhancements: Add direction to EP, based on contents
			 * of HWCFG1.  Potentially take a copy of HWCFG1 in pcd structure. */
			/* IMG_SYNOP_CR */
			dwc_otg_pcd_init_ep(pcd, ep, 1 /* IN */ , i);
		}
		hwcfg1 >>= 2;
	}

	out_ep_cntr = 0;
	hwcfg1 = (core_if)->hwcfg1.d32 >> 2;

	for (i = 1; out_ep_cntr < num_out_eps; i++) 
	{
		if((hwcfg1 & 0x1) == 0)
		{
			dwc_otg_pcd_ep_t *ep = &pcd->out_ep[out_ep_cntr];
			out_ep_cntr++;
			/**
			 * Potential enhancements: Add direction to EP, based on contents
			 * of HWCFG1.  Potentially take a copy of HWCFG1 in pcd structure. */
			/* IMG_SYNOP_CR */
			dwc_otg_pcd_init_ep(pcd, ep, 0 /* OUT */ , i);
	}
		hwcfg1 >>= 2;
	}
	 
	pcd->ep0state = EP0_DISCONNECT;	  
	pcd->ep0.dwc_ep.maxpacket = USBD_MPS_EP0;
	pcd->ep0.dwc_ep.type = DWC_OTG_EP_TYPE_CONTROL;
}

#ifdef DWC_SUPPORT_OTG
/**
 * This function is called when the SRP timer expires.	The SRP should
 * complete within 6 seconds. 
 */
static IMG_VOID srp_timeout( IMG_UINT32 _ptr )
{
	gotgctl_data_t gotgctl;
	dwc_otg_core_if_t *core_if = (dwc_otg_core_if_t *)_ptr;
	volatile IMG_UINT32 *addr = (volatile IMG_UINT32 *) DWC_OTG_CORE_GLOBAL_REGS_GOTGCTL_OFFSET;

	gotgctl.d32 = usb_ReadReg32(addr);

	core_if->srp_timer_started = 0;
#ifdef USE_I2C
	if ((core_if->core_params->phy_type == DWC_PHY_TYPE_PARAM_FS) && 
		(core_if->core_params->i2c_enable))
	{
		DWC_PRINT( "SRP Timeout\n");

		if ((core_if->srp_success) && (gotgctl.b.bsesvld))
		{
			if (core_if->pcd_cb && core_if->pcd_cb->resume_wakeup ) 
			{
				core_if->pcd_cb->resume_wakeup(core_if->pcd_cb->p);
			}
			
			/* Clear Session Request */
			gotgctl.d32 = 0;
			gotgctl.b.sesreq = 1;
			usb_ModifyReg32( DWC_OTG_CORE_GLOBAL_REGS_GOTGCTL_OFFSET, gotgctl.d32, 0);
	
			core_if->srp_success = 0;
		}
		else 
		{
			DWC_ERROR( "Device not connected/responding\n");
			gotgctl.b.sesreq = 0;
			usb_WriteReg32(addr, gotgctl.d32);
		}
	}
	else if (gotgctl.b.sesreq)
#else
	if (gotgctl.b.sesreq)
#endif
	{
		DWC_PRINT( "SRP Timeout\n");

		DWC_ERROR( "Device not connected/responding\n");
		gotgctl.b.sesreq = 0;
		usb_WriteReg32(addr, gotgctl.d32);
	} 
	else 
	{
		DWC_PRINT( " SRP GOTGCTL=%0x\n", gotgctl.d32);
	} 
}
#endif /* DWC_SUPPORT_OTG */

extern IMG_VOID start_next_request( dwc_otg_pcd_ep_t *ep );



#if defined(TIMER_NOT_AVAILABLE)

IMG_BOOL	bStartXferTimer = IMG_FALSE;

IMG_VOID start_xfer_func( IMG_VOID	*	pParam )

#else

KRN_TIMER_T		tStartXferTimer;
/*	Bojan: this function gets called when the start_xfer_tasklet
	is kicked off.
*/
IMG_VOID start_xfer_func (KRN_TIMER_T *timer, IMG_VOID * pParam)
#endif
{
	ioblock_sBlockDescriptor	*	psBlockDesc = (ioblock_sBlockDescriptor *)pParam;
	USB_DC_T					*	psContext = (USB_DC_T *)psBlockDesc->pvAPIContext;
	USBD_DC_T					*	psDevContext = &(psContext->sDevContext);
	dwc_otg_pcd_t				*	pcd = &(psDevContext->sOtgPcd);
	dwc_otg_core_if_t			*	core_if = &(psContext->sOtgCoreIf);

	IMG_INT32 i;
	depctl_data_t diepctl;

	DWC_DEBUGPL(DBG_PCDV, "Start xfer tasklet\n");

	diepctl.d32 = usb_ReadReg32(psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0);

	if (pcd->ep0.queue_sof) 
	{
		pcd->ep0.queue_sof = 0;
		start_next_request (&pcd->ep0);
		// break;
	}

	for (i = 0; i < core_if->dev_if.num_in_eps; i++) 
	{
		depctl_data_t diepctl;
		diepctl.d32 = usb_ReadReg32(psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + i * DWC_EP_REG_OFFSET);

		if (pcd->in_ep[i].queue_sof) 
		{
			pcd->in_ep[i].queue_sof = 0;
			start_next_request (&pcd->in_ep[i]);
			// break;
		}
	}

	return;
}

/** 
 * This function initialized the PCD portion of the driver.
 *
 */
IMG_VOID dwc_otg_pcd_init( const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	USB_DC_T			*psContext		= (USB_DC_T *)psBlockDesc->pvAPIContext;
	USBD_DC_T			*psDevContext	= &(psContext->sDevContext);
	dwc_otg_pcd_t		*pcd			= &(psDevContext->sOtgPcd);
	dwc_otg_core_if_t	*core_if		= &(psContext->sOtgCoreIf);
	
	if (core_if->hwcfg4.b.ded_fifo_en)
	{
		DWC_PRINT("Dedicated Tx FIFOs mode\n");
	}
	else
	{
		DWC_PRINT("Shared Tx FIFO mode\n");
	}
	
	/* If the module is set to FS or if the PHY_TYPE is FS then the device
	 * should not report as dual-speed capable.	 replace the following line
	 * with the block of code below it once the software is debugged for
	 * this.  If is_dualspeed = 0 then the  driver should not report
	 * a device qualifier descriptor when queried. */
	if ((core_if->core_params.speed == DWC_SPEED_PARAM_FULL) ||
		((core_if->hwcfg2.b.hs_phy_type == 2) &&
		 (core_if->hwcfg2.b.fs_phy_type == 1) &&
		 (core_if->core_params.ulpi_fs_ls)))
	{
		pcd->is_dualspeed = 0; 
	}
	else 
	{
		pcd->is_dualspeed = 1;
	}

#ifdef DWC_SUPPORT_OTG
	if ((core_if->hwcfg2.b.op_mode == DWC_HWCFG2_OP_MODE_NO_SRP_CAPABLE_DEVICE) ||
		(core_if->hwcfg2.b.op_mode == DWC_HWCFG2_OP_MODE_NO_SRP_CABABLE_HOST) ||
		(core_if->hwcfg2.b.op_mode == DWC_HWCFG2_OP_MODE_SRP_CAPABLE_DEVICE) ||
		(core_if->hwcfg2.b.op_mode == DWC_HWCFG2_OP_MODE_SRP_CAPABLE_HOST))
	{
		pcd->is_otg = 0;
	}
	else
	{
		pcd->is_otg = 1;
	}	
#endif



	/*
	 * Register the PCD Callbacks.
	 */
	dwc_otg_cil_register_pcd_callbacks( core_if, &pcd_callbacks, (img_void *)psBlockDesc );

	/* 
	 * Initialize the DMA buffer for SETUP packets
	 */
	if ( psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical )
	{
		ioblock_fn_ConvertVirtualToPhysical	*	pfnConvert = psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical;
		pcd->setup_pkt_dma_handle = pfnConvert( (img_uint32)pcd->setup_pkt );
		pcd->status_buf_dma_handle = pfnConvert( (img_uint32)pcd->status_buf );

		if (core_if->dma_desc_enable)
		{
			core_if->dev_if.dma_setup_desc_addr[0] = pfnConvert( (img_uint32)core_if->dev_if.setup_desc_addr[0] );
			core_if->dev_if.dma_setup_desc_addr[1] = pfnConvert( (img_uint32)core_if->dev_if.setup_desc_addr[1] );
			core_if->dev_if.dma_in_desc_addr = pfnConvert( (img_uint32)core_if->dev_if.in_desc_addr );
			core_if->dev_if.dma_out_desc_addr = pfnConvert( (img_uint32)core_if->dev_if.out_desc_addr );
		}
	}
	else
	{
		pcd->setup_pkt_dma_handle = (img_uint32)pcd->setup_pkt;
		pcd->status_buf_dma_handle = (img_uint32)pcd->status_buf;

		if (core_if->dma_desc_enable)
		{
			core_if->dev_if.dma_setup_desc_addr[0] = (img_uint32)core_if->dev_if.setup_desc_addr[0];
			core_if->dev_if.dma_setup_desc_addr[1] = (img_uint32)core_if->dev_if.setup_desc_addr[1];
			core_if->dev_if.dma_in_desc_addr = (img_uint32)core_if->dev_if.in_desc_addr;
			core_if->dev_if.dma_out_desc_addr = (img_uint32)core_if->dev_if.out_desc_addr;
		}
	}

	/*
	 * Initialize EP structures
	 */
	dwc_otg_pcd_reinit( pcd, core_if );
}



/**
 * This function assigns periodic Tx FIFO to an periodic EP
 * in shared Tx FIFO mode
 */
static IMG_UINT32 assign_tx_fifo( dwc_otg_core_if_t	*	core_if )
{
	IMG_UINT32 TxMsk = 1;
	IMG_INT32 i;
	
	for (i = 0; i < core_if->hwcfg4.b.num_in_eps; ++i)
	{
		if((TxMsk & core_if->tx_msk) == 0)
		{
			core_if->tx_msk |= TxMsk;
			return i + 1;
		}
		TxMsk <<= 1;
	}
	return 0;
}

#ifndef USE_MULTIPLE_TX_FIFO
/**
 * This function assigns periodic Tx FIFO to an periodic EP
 * in shared Tx FIFO mode
 */
static IMG_UINT32 assign_perio_tx_fifo( dwc_otg_core_if_t	*	core_if )
{
	IMG_UINT32 PerTxMsk = 1;
	IMG_INT32 i;
	for (i = 0; i < core_if->hwcfg4.b.num_dev_perio_in_ep; ++i)
	{
		if((PerTxMsk & core_if->p_tx_msk) == 0)
		{
			core_if->p_tx_msk |= PerTxMsk;
			return i + 1;
		}
		PerTxMsk <<= 1;
	}
	return 0;
}

/**
 * This function releases periodic Tx FIFO 
 * in shared Tx FIFO mode
 */
IMG_VOID release_perio_tx_fifo( dwc_otg_core_if_t	*	core_if, IMG_UINT32 fifo_num )
{
	core_if->p_tx_msk = (core_if->p_tx_msk & (1 << (fifo_num - 1))) ^ core_if->p_tx_msk;
}

#endif

/**
 * This function releases a Tx FIFO 
 * in dedicated Tx FIFO mode
 */
IMG_VOID release_tx_fifo( dwc_otg_core_if_t	*	core_if , IMG_UINT32 fifo_num )
{
	core_if->tx_msk = (core_if->tx_msk & (1 << (fifo_num - 1))) ^ core_if->tx_msk;
}

/**
 * This function is called by the Driver for each EP to be
 * configured for the current configuration (SET_CONFIGURATION).  
 * 
 * This function initializes the dwc_otg_ep_t data structure, and then
 * calls dwc_otg_ep_activate.
 */
IMG_INT32 dwc_otg_pcd_ep_enable( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep, const struct usb_endpoint_descriptor_t *ep_desc )
{
	USB_DC_T			*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_pcd_t		*	pcd			= &(psContext->sDevContext.sOtgPcd);
	dwc_otg_core_if_t	*	core_if		= &(psContext->sOtgCoreIf);

	if (!ep_desc || ep_desc->bDescriptorType != USB_ENDPOINT_DESCRIPTOR_TYPE) 
	{
		DWC_WARN( "%s, bad ep or descriptor\n");
		return -USBD_ERR_INVAL;
	}
	if (ep == &pcd->ep0)
	{
		DWC_WARN("%s, bad ep(0)\n");
		return -USBD_ERR_INVAL;
	}
		
	/* Check FIFO size? */
	if (!ep_desc->wMaxPacketSize) 
	{
		DWC_WARN("bad maxpacket\n");
		return -USBD_ERR_RANGE;
	}

	if (pcd->speed == USB_SPEED_UNKNOWN) 
	{
		DWC_WARN("%s, bogus device state\n");
		return -USBD_ERR_SHUTDOWN;
	}

	SPIN_LOCK_IRQSAVE(&pcd->lock, flags);
		
	ep->desc = ep_desc;
	ep->dwc_ep.maxpacket =  USB_LE16_TO_CPU(ep_desc->wMaxPacketSize);   /*le16_to_cpu (_ep_desc->wMaxPacketSize);*/
		
	/*
	 * Activate the EP
	 */
	ep->stopped = 0;
		
	ep->dwc_ep.is_in = (USB_DIR_IN & ep_desc->bEndpointAddress) != 0;
	
	ep->dwc_ep.type = ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;

	if(ep->dwc_ep.is_in)
	{
	  #ifndef USE_MULTIPLE_TX_FIFO
		if(!core_if->en_multiple_tx_fifo)
		{
			ep->dwc_ep.tx_fifo_num = 0;
		
			if (ep->dwc_ep.type == USB_ENDPOINT_XFER_ISOC )
			{
				/* 
				 * if ISOC EP then assign a Periodic Tx FIFO.
				 */
				ep->dwc_ep.tx_fifo_num = assign_perio_tx_fifo( core_if );
			 }
		}
		else
	  #endif
		{
			/* 
			 * if Dedicated FIFOs mode is on then assign a Tx FIFO.
			 */
			ep->dwc_ep.tx_fifo_num = assign_tx_fifo( core_if );
		}
	}		 
	/* Set initial data PID. */
	if (ep->dwc_ep.type == USB_ENDPOINT_XFER_BULK)
	{
		ep->dwc_ep.data_pid_start = 0;	
	}
		
	/* Alloc DMA Descriptors */
	if ( core_if->dma_desc_enable )
	{
	  #if defined (_EN_ISOC_)
		if ( ep->dwc_ep.type != USB_ENDPOINT_XFER_ISOC )
	  #endif /* _EN_ISOC_ */
		{
			if ( psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical )
			{
				ep->dwc_ep.dma_desc_addr = psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical( (img_uint32)ep->dwc_ep.desc_addr );
			}
			else
			{
				ep->dwc_ep.dma_desc_addr = (img_uint32)ep->dwc_ep.desc_addr;
			}
			if ( !ep->dwc_ep.dma_desc_addr )
			{
				DWC_WARN("%s, can't allocate DMA desecriptor\n" );
				return -USBD_ERR_NOMEM;
			}
		}
	  #if defined (_EN_ISOC_)
		else
		{
			if ( psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical )
			{
				ep->dwc_ep.iso_dma_desc_addr = psBlockDesc->psSystemDescriptor->pfn_ConvertVirtualToPhysical( (img_uint32)ep->dwc_ep.iso_desc_addr );
			}
			else
			{
				ep->dwc_ep.iso_dma_desc_addr = (img_uint32)ep->dwc_ep.iso_desc_addr;
			}
			if ( !ep->dwc_ep.iso_dma_desc_addr )
			{
				DWC_WARN("%s, can't allocate ISO DMA descriptor\n" );
				return -USBD_ERR_NOMEM;
			}
		}
	  #endif /* _EN_ISOC_ */
	}


	DWC_DEBUGPL(DBG_PCD, "Activate %s-%s: type=%d, mps=%d desc=%p\n", ep->ep.name, (ep->dwc_ep.is_in ?"IN":"OUT"), ep->dwc_ep.type, ep->dwc_ep.maxpacket, ep->desc );
#if defined (_PER_IO_)
	ep->dwc_ep.xiso_bInterval = 1 << (ep->desc->bInterval - 1);
#endif /* _PER_IO_ */
		
	dwc_otg_ep_activate( psBlockDesc->ui32Base, &ep->dwc_ep );
	SPIN_UNLOCK_IRQRESTORE(&pcd->lock, flags);
	return 0;
}

/** 
 * This function is called when an EP is disabled due to disconnect or
 * change in configuration. Any pending requests will terminate with a
 * status of -USBD_ERR_SHUTDOWN.
 *
 * This function modifies the dwc_otg_ep_t data structure for this EP,
 * and then calls dwc_otg_ep_deactivate.
 */
IMG_INT32 dwc_otg_pcd_ep_disable( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep )
{
	USB_DC_T			*	psContext	= (USB_DC_T	*)psBlockDesc->pvAPIContext;
	dwc_otg_core_if_t	*	core_if		= &(psContext->sOtgCoreIf);

	if (!ep || !ep->desc) 
	{
		return -USBD_ERR_INVAL;
	}
		
	SPIN_LOCK_IRQSAVE(&ep->pcd->lock, flags);

	dwc_otg_pcd_ep_request_nuke( psBlockDesc, ep );
	
	if(ep->dwc_ep.is_in)
	{
	  #ifdef DISABLE_IN_EP_FEATURE //bojan
		depctl_data_t		diepctl	= {0};
		/* This only works in dedicated fifo mode! */	
		diepctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + ep->dwc_ep.num * DWC_EP_REG_OFFSET);
		
		if (diepctl.b.epena)
		{
			/* Start the disabling process for the endpoint */
			ep->disabling = 1;
			
			/* This may be called on the reset interrupt... ensure that the interrupts for this ep are enabled */
			dwc_otg_ep_activate( psBlockDesc->ui32Base, &ep->dwc_ep );

			/* Set the NAK for the endpoint */
			diepctl.b.snak = 1;
			usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + ep->dwc_ep.num * DWC_EP_REG_OFFSET, diepctl.d32);
		}
		else
	  #endif
		{
			dwc_otg_ep_deactivate( psBlockDesc->ui32Base, core_if, &ep->dwc_ep );
			ep->desc = 0;
			ep->stopped = 1;
			dwc_otg_flush_tx_fifo( psBlockDesc->ui32Base, ep->dwc_ep.tx_fifo_num );
		  #if !defined (USE_MULTIPLE_TX_FIFO)
			release_perio_tx_fifo( core_if, ep->dwc_ep.tx_fifo_num );
		  #endif
			release_tx_fifo( core_if, ep->dwc_ep.tx_fifo_num );
		}
	}
	else
	{
		depctl_data_t		doepctl_rd	= {0};
		dctl_data_t			dctl		= {0};
		gintmsk_data_t		intr_mask	= {0};
		

		dwc_otg_ep_deactivate( psBlockDesc->ui32Base, core_if, &ep->dwc_ep );
		ep->desc = 0;
		ep->stopped = 1;

	  #ifdef DISABLE_OUT_EP_FEATURE //bojan
		doepctl_rd.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->dwc_ep.num * DWC_EP_REG_OFFSET);
		
		/* If the endpoint is enabled then we need to disable it */
		if (doepctl_rd.b.epena)
		{
			/* enable goutnakeffective interrupt */
			intr_mask.b.goutnakeff = 1;
			usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, 0, intr_mask.d32);

			/* Start the disabling process for the endpoint */
			ep->disabling = 1;

			/* Set global out nak */
			dctl.b.sgoutnak = 1;
			usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, 0, dctl.d32 );
		}
	  #endif
	}

	SPIN_UNLOCK_IRQRESTORE(&ep->pcd->lock, flags);

	//DWC_DEBUGPL(DBG_PCD, "%s disabled\n", ep->name);
	return 0;
}

/**********************************************************************************/
#if defined (_PER_IO_)

/**
 * Start the next request in the endpoint's queue.
 *
 */
int dwc_otg_pcd_xiso_start_next_request( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep )
{
	IMG_INT32							i;
	dwc_otg_pcd_request_t			*	req		= 0;
	dwc_ep_t						*	dwcep	= NULL;
	struct dwc_iso_xreq_port		*	ereq = NULL;
	struct dwc_iso_pkt_desc_port	*	ddesc_iso;
	IMG_UINT32							nat; /*  */
	depctl_data_t						diepctl;

	dwcep = &ep->dwc_ep;

	if (dwcep->xiso_active_xfers > 0) 
	{
#if 0 /* Disabling this warning to decrease the s/w overhead, this is crucial for Isoc transfers */
		DWC_WARN("There are currently active transfers for EP%d (active=%d; queued=%d)",
					dwcep->num, dwcep->xiso_active_xfers, dwcep->xiso_queued_xfers);
#endif
		return 0;
	}

//DWC_DEBUG();

	nat = (IMG_UINT32)*(IMG_UINT16*)ep->desc->wMaxPacketSize;
	nat = (nat >> 11) & 0x03;
	
	if (!LST_empty(&ep->sRequestList))
	{
		req = (dwc_otg_pcd_request_t *)LST_first(&ep->sRequestList);
		ereq = &req->ext_req;
		ep->stopped = 0;
		
		/* Get the frame number */
		if (dwcep->xiso_frame_num == 0xFFFFFFFF) 
		{
			dwcep->xiso_frame_num = dwc_otg_get_frame_number( psBlockDesc->ui32Base );
			DWC_DEBUGP("FRM_NUM=%d", dwcep->xiso_frame_num);
		}

		ddesc_iso = ereq->per_io_frame_descs;
		
		if (dwcep->is_in) 
		{
			/* Setup DMA Descriptor chain for IN Isoc request */
			for(i = 0; i < ereq->pio_pkt_count - 1; i++) 
			{
				//if ((i % (nat + 1)) == 0)
 					dwcep->xiso_frame_num = (dwcep->xiso_bInterval + dwcep->xiso_frame_num) & 0x3FFF;
				dwcep->desc_addr[i].buf = req->dma + ddesc_iso[i].offset;
				dwcep->desc_addr[i].status.b_iso_in.txbytes = ddesc_iso[i].length;
				dwcep->desc_addr[i].status.b_iso_in.framenum = dwcep->xiso_frame_num;
				dwcep->desc_addr[i].status.b_iso_in.bs = BS_HOST_READY;
				dwcep->desc_addr[i].status.b_iso_in.txsts = 0;
				dwcep->desc_addr[i].status.b_iso_in.sp = (ddesc_iso[i].length % dwcep->maxpacket) ? 1 : 0;
				dwcep->desc_addr[i].status.b_iso_in.ioc = 0;
				dwcep->desc_addr[i].status.b_iso_in.pid = nat + 1;
				dwcep->desc_addr[i].status.b_iso_in.l = 0;
			}
			
			//if ((i % (nat + 1)) == 0)
 				dwcep->xiso_frame_num = (dwcep->xiso_bInterval + dwcep->xiso_frame_num) & 0x3FFF;

			dwcep->desc_addr[i].buf = req->dma + ddesc_iso[i].offset;
			dwcep->desc_addr[i].status.b_iso_in.txbytes = ddesc_iso[i].length;
			dwcep->desc_addr[i].status.b_iso_in.framenum = dwcep->xiso_frame_num;
			dwcep->desc_addr[i].status.b_iso_in.bs = BS_HOST_READY;
			dwcep->desc_addr[i].status.b_iso_in.txsts = 0;
			dwcep->desc_addr[i].status.b_iso_in.sp = (ddesc_iso[i].length % dwcep->maxpacket) ? 1 : 0;
			dwcep->desc_addr[i].status.b_iso_in.ioc = 1;
			dwcep->desc_addr[i].status.b_iso_in.pid = nat + 1;
			dwcep->desc_addr[i].status.b_iso_in.l = 1;
			
			/* Setup and start the transfer for this endpoint */
			dwcep->xiso_active_xfers++;
			usb_WriteMemrefReg32(psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPDMA0 + dwcep->num * DWC_EP_REG_OFFSET, dwcep->dma_desc_addr);
			diepctl.d32 = 0;
			diepctl.b.epena = 1;
			diepctl.b.cnak = 1;
			usb_ModifyReg32(psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + dwcep->num * DWC_EP_REG_OFFSET, 0, diepctl.d32);
		} 
		else 
		{
			/* Setup DMA Descriptor chain for OUT Isoc request */
			for(i = 0; i < ereq->pio_pkt_count - 1; i++) 
			{
				//if ((i % (nat + 1)) == 0)
				dwcep->xiso_frame_num = (dwcep->xiso_bInterval + dwcep->xiso_frame_num) & 0x3FFF;
				dwcep->desc_addr[i].buf = req->dma + ddesc_iso[i].offset;
				dwcep->desc_addr[i].status.b_iso_out.rxbytes = ddesc_iso[i].length;
				dwcep->desc_addr[i].status.b_iso_out.framenum = dwcep->xiso_frame_num;
				dwcep->desc_addr[i].status.b_iso_out.bs = BS_HOST_READY;
				dwcep->desc_addr[i].status.b_iso_out.rxsts = 0;
				dwcep->desc_addr[i].status.b_iso_out.sp = (ddesc_iso[i].length % dwcep->maxpacket) ? 1 : 0;
				dwcep->desc_addr[i].status.b_iso_out.ioc = 0;
				dwcep->desc_addr[i].status.b_iso_out.pid = nat + 1;
				dwcep->desc_addr[i].status.b_iso_out.l = 0;
			}
			
			//if ((i % (nat + 1)) == 0)
			/* If the frame number logic is to be implemented, then additional 
			 * logic is needed for initializing the last descriptor
			 */
			dwcep->xiso_frame_num = (dwcep->xiso_bInterval + dwcep->xiso_frame_num) & 0x3FFF;
			
			dwcep->desc_addr[i].buf = req->dma + ddesc_iso[i].offset;
			dwcep->desc_addr[i].status.b_iso_out.rxbytes = ddesc_iso[i].length;
			dwcep->desc_addr[i].status.b_iso_out.framenum = dwcep->xiso_frame_num;
			dwcep->desc_addr[i].status.b_iso_out.bs = BS_HOST_READY;
			dwcep->desc_addr[i].status.b_iso_out.rxsts = 0;
			dwcep->desc_addr[i].status.b_iso_out.sp = (ddesc_iso[i].length % dwcep->maxpacket) ? 1 : 0;
			dwcep->desc_addr[i].status.b_iso_out.ioc = 1;
			dwcep->desc_addr[i].status.b_iso_out.pid = nat + 1;
			dwcep->desc_addr[i].status.b_iso_out.l = 1;
			
			/* Setup and start the transfer for this endpoint */
			dwcep->xiso_active_xfers++;
			usb_WriteMemrefReg32(psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0 + dwcep->num * DWC_EP_REG_OFFSET, dwcep->dma_desc_addr);
			diepctl.d32 = 0;
			diepctl.b.epena = 1;
			diepctl.b.cnak = 1;
			usb_ModifyReg32(psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + dwcep->num * DWC_EP_REG_OFFSET, 0, diepctl.d32);		
		}

	} 
	else 
	{
		ep->stopped = 1;
	}

	return 0;
}

/**
 *	- Remove the request from the queue
 */
void complete_xiso_ep( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep)
{
	dwc_otg_pcd_request_t *req = 0;
	struct dwc_iso_xreq_port *ereq = NULL;
	struct dwc_iso_pkt_desc_port *ddesc_iso = NULL;
	dwc_ep_t* dwcep = NULL;
	int i;

	dwcep = &ep->dwc_ep;
	
	/* Get the first pending request from the queue */
	if (!LST_empty(&ep->sRequestList)) 
	{
		req = LST_first(&ep->sRequestList);
		if (!req) 
		{
			DWC_PRINT("complete_ep 0x%p, req = NULL!\n", ep);
			return;
		}
		dwcep->xiso_active_xfers--;
		dwcep->xiso_queued_xfers--;
		/* Remove this request from the queue */
	} 
	else 
	{
		DWC_PRINT("complete_ep 0x%p, ep->queue empty!\n", ep);
		return;
	}
	
	ep->stopped = 1;
	ereq = &req->ext_req;
	ddesc_iso = ereq->per_io_frame_descs;
	
	if (dwcep->xiso_active_xfers < 0) 
	{
		DWC_WARN("EP#%d (xiso_active_xfers=%d)", dwcep->num,
				dwcep->xiso_active_xfers);
	}

	/* Flush the cache of the descriptors */
	if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
	{
		psBlockDesc->psSystemDescriptor->pfn_FlushCache( (IMG_UINT32)dwcep->desc_addr, sizeof( dwc_otg_dev_dma_desc_t ) * ereq->pio_pkt_count );
	}
	
	/* Fill the Isoc descs of portable extended req from dma descriptors */
	for(i = 0; i < ereq->pio_pkt_count; i++) 
	{
		if (dwcep->is_in) 
		{ /* IN endpoints */
			ddesc_iso[i].actual_length = ddesc_iso[i].length - dwcep->desc_addr[i].status.b_iso_in.txbytes;
			ddesc_iso[i].status = dwcep->desc_addr[i].status.b_iso_in.txsts;
		}
		else 
		{ /* OUT endpoints */
			ddesc_iso[i].actual_length = ddesc_iso[i].length - dwcep->desc_addr[i].status.b_iso_out.rxbytes;
			ddesc_iso[i].status = dwcep->desc_addr[i].status.b_iso_out.rxsts;
		}
	}

	/* Call the completion function in the non-portable logic */
	request_done( psBlockDesc, ep, req, 0 );

	/* Start the next request */
	dwc_otg_pcd_xiso_start_next_request( psBlockDesc, ep);
	
	return;
}

/**
 *
 */
IMG_INT32 dwc_otg_pcd_xiso_ep_queue(	const ioblock_sBlockDescriptor	*	psBlockDesc,
										dwc_otg_pcd_ep_t				*	ep,
										struct dwc_otg_pcd_request		*	req	)
{	
	IMG_INT32					res;

	if (!ep) 
	{
		DWC_WARN("bad ep\n");
		return -USBD_ERR_INVAL;
	}

	req->status = -USBD_ERR_INPROGRESS;
	req->actual = 0;
	
	req->ext_req.error_count = 0;

	ep->dwc_ep.dma_addr = req->dma;
	ep->dwc_ep.start_xfer_buff = req->buf;
	ep->dwc_ep.xfer_buff = req->buf;
	ep->dwc_ep.xfer_len = 0;
	ep->dwc_ep.xfer_count = 0;
	ep->dwc_ep.sent_zlp = 0;
	ep->dwc_ep.total_len = req->length;

	/* Add this request to the tail */
	ep->dwc_ep.xiso_queued_xfers++;

	++ep->request_pending;

	LST_add(&ep->sRequestList, req);

	/* If the req->status == ASAP  then check if there is any active transfer
	 * for this endpoint. If no active transfers, then get the first entry
	 * from the queue and start that transfer
	 */
	if (req->ext_req.tr_sub_flags == DWC_EREQ_TF_ASAP) 
	{
		res = dwc_otg_pcd_xiso_start_next_request(psBlockDesc, ep);
		if (res) 
		{
			DWC_WARN("Failed to start the next Isoc transfer");
			return res;
		}
	}

	return 0;
}

#endif /* _PER_IO_ */

/**
 * This function is used to submit an I/O Request to an EP.
 *
 *	- When the request completes the request's completion callback
 *	  is called to return the request to the driver.
 *	- An EP, except control EPs, may have multiple requests
 *	  pending.
 *	- Once submitted the request cannot be examined or modified.
 *	- Each request is turned into one or more packets.
 *	- A BULK EP can queue any amount of data; the transfer is
 *	  packetized.
 *	- Zero length Packets are specified with the request 'zero'
 *	  flag.
 */
IMG_INT32 dwc_otg_pcd_ep_queue( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep, dwc_otg_pcd_request_t *req )
{
	USB_DC_T			*psContext		= (USB_DC_T *)psBlockDesc->pvAPIContext;
	USBD_DC_T			*psDevContext	= &(psContext->sDevContext);
	dwc_otg_pcd_t		*pcd			= &(psDevContext->sOtgPcd);
	dwc_otg_core_if_t	*core_if		= &(psContext->sOtgCoreIf);
	IMG_UINT32			 max_transfer;

	if (pcd->speed == USB_SPEED_UNKNOWN) 
	{
		DWC_DEBUGPL(DBG_PCDV, "pcd->speed=%d\n", pcd->speed);
		DWC_WARN("%s, bogus device state\n");
		return -USBD_ERR_SHUTDOWN;
	}

	req->status = -USBD_ERR_INPROGRESS;
	req->actual = 0;

	/* 
	 * For EP0 IN without premature status, zlp is required?
	 */
	/* This check appears to be inconsequential
	if (ep->dwc_ep.num == 0 && ep->dwc_ep.is_in) 
	{
		//DWC_DEBUGPL(DBG_PCDV, "%s-OUT ZLP\n", _ep->name);
		//_req->zero = 1;
	}
	*/ 

	/* Start the transfer */
	if (LST_empty(&ep->sRequestList) && !ep->stopped)
	{
		/* EP0 Transfer? */
		if (ep->dwc_ep.num == 0) 
		{
			switch (pcd->ep0state) 
			{
			case EP0_IN_DATA_PHASE:
				DWC_DEBUGPL(DBG_PCD, "%s ep0: EP0_IN_DATA_PHASE\n", __func__);
				break;

			case EP0_OUT_DATA_PHASE:
				DWC_DEBUGPL(DBG_PCD, "%s ep0: EP0_OUT_DATA_PHASE\n", __func__);
				if (pcd->request_config) 
				{ 
					/* Complete STATUS PHASE */
					ep->dwc_ep.is_in = 1;
					pcd->ep0state = EP0_IN_STATUS_PHASE;
				}
				break;
						
			case EP0_IN_STATUS_PHASE:
				DWC_DEBUGPL(DBG_PCD, "%s ep0: EP0_IN_STATUS_PHASE\n", __func__);
				break;

			default:
				DWC_DEBUGPL(DBG_ANY, "ep0: odd state %d\n", pcd->ep0state);
				return -USBD_ERR_L2HLT;
			}
			// Setup and start the Transfer
			ep->dwc_ep.dma_addr = req->dma;
			ep->dwc_ep.start_xfer_buff = (IMG_UINT8 *)req->buf;
			ep->dwc_ep.xfer_buff = req->buf;
			ep->dwc_ep.xfer_len = req->length;
			ep->dwc_ep.xfer_count = 0;
			ep->dwc_ep.sent_zlp = 0;
			ep->dwc_ep.total_len = ep->dwc_ep.xfer_len;
	
			if (req->zero)
			{
				if ((ep->dwc_ep.xfer_len % ep->dwc_ep.maxpacket == 0 ) &&
				 	(ep->dwc_ep.xfer_len != 0))
				{
					ep->dwc_ep.sent_zlp = 1;
				}
	
			}

			dwc_otg_ep0_start_transfer( psBlockDesc, core_if, &ep->dwc_ep );
		}
		else // non-ep0 endpoints
		{
			max_transfer = core_if->core_params.max_transfer_size;

			/* Setup and start the Transfer */
			ep->dwc_ep.dma_addr = req->dma;
			ep->dwc_ep.start_xfer_buff = (IMG_UINT8 *)req->dma;
			ep->dwc_ep.xfer_buff = (IMG_UINT8 *)req->dma;
			ep->dwc_ep.xfer_len = 0;
			ep->dwc_ep.xfer_count = 0;
			ep->dwc_ep.sent_zlp = 0;
			ep->dwc_ep.total_len = req->length;

			ep->dwc_ep.maxxfer = max_transfer;
			if (core_if->dma_desc_enable)
			{
				IMG_UINT32 out_max_xfer = DDMA_MAX_TRANSFER_SIZE - (DDMA_MAX_TRANSFER_SIZE % 4);
				if (ep->dwc_ep.is_in)
				{
					if (ep->dwc_ep.maxxfer > DDMA_MAX_TRANSFER_SIZE)
					{
						ep->dwc_ep.maxxfer = DDMA_MAX_TRANSFER_SIZE;
					}
				}
				else
				{
					if (ep->dwc_ep.maxxfer > out_max_xfer)
					{
						ep->dwc_ep.maxxfer = out_max_xfer;
					}
				}
			}
			if (ep->dwc_ep.maxxfer < ep->dwc_ep.total_len)
			{
				ep->dwc_ep.maxxfer -= (ep->dwc_ep.maxxfer % ep->dwc_ep.maxpacket);
			}
	
			if (req->zero)
			{
				if ((ep->dwc_ep.total_len % ep->dwc_ep.maxpacket == 0) && (ep->dwc_ep.total_len != 0))
				{
					ep->dwc_ep.sent_zlp = 1;
				}
			}
				
			dwc_otg_ep_start_transfer( psBlockDesc, core_if, &ep->dwc_ep );
		}
	}


	++ep->request_pending;

	/* Add the transfer to the transfer queue */
	LST_add(&ep->sRequestList, req);

  #ifndef USE_DMA_INTERNAL
	if (ep->dwc_ep.is_in && ep->stopped && !(core_if->dma_enable)) 
	{
		/** Potential enhancements: Create a dedicated function for this operation. */
		/* IMG_SYNOP_CR */
		diepmsk_data_t diepmsk = {0};
		diepmsk.b.intktxfemp = 1;
		usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DIEPMSK_OFFSET, 0, diepmsk.d32 );
	}
  #endif /* USE_DMA_INTERNAL */

	return 0;
}

/*	Static link list of requests. It is populated from the request array at
	start up. The allocate and free functions remove and add the request objects
	to the list
*/

static struct dwc_otg_pcd_request	asRequestObjects[MAX_REQUEST_OBJECTS];
static LST_T						gsDriverRequestList;

/**
 * This function allocates a request object to use with the specified
 * endpoint.
 *
 * @param ep The endpoint to be used with with the request
 * @param gfp_flags the GFP_* flags to use.
 */
struct dwc_otg_pcd_request * dwc_otg_pcd_alloc_request( )
{	
	dwc_otg_pcd_request_t *	req = (dwc_otg_pcd_request_t *) LST_removeHead(&gsDriverRequestList);
	IMG_ASSERT( req );
	return req;
}

/**
 * This function frees a request object.
 *
 * @param ep The endpoint associated with the request
 * @param req The request being freed
 */
IMG_VOID dwc_otg_pcd_free_request( struct dwc_otg_pcd_request *req )
{	
	if (0 == req) 
	{
		IMG_ASSERT(0);
		return;
	}

	LST_add(&gsDriverRequestList, req);
}

/** 
 * Bojan: The function is used to reset the endpoint. Any queued transfers
 * are discarded in the process. The function is designed to be called when
 * the QIO driver cancels the transfers 
 */

IMG_VOID dwc_otg_pcd_ep_reset( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep )
{
	USB_DC_T	*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	/* Clean up all the current transfers */
	dwc_otg_pcd_ep_request_nuke( psBlockDesc, ep );
	
	/* Deactivate the endpoint */
	dwc_otg_ep_deactivate( psBlockDesc->ui32Base, &(psContext->sOtgCoreIf), &ep->dwc_ep );
	
	/* IMG_SYNOP_CR */
	
	/* Then immediately activate it */
	ep->stopped = 0;
	dwc_otg_ep_activate( psBlockDesc->ui32Base, &ep->dwc_ep );
}



IMG_VOID dwc_otg_pcd_initialise_request_list ( )
{
	IMG_UINT32	i;

	LST_init(&gsDriverRequestList);

	/* Populate the requests queue */
	for (i = 0; i < MAX_REQUEST_OBJECTS; i++)
	{
		LST_add(&gsDriverRequestList, &asRequestObjects[i]);
	}
}





/*
	Returns true if there is a transfer queued on the endpoint,
	returns false otherwise
*/
IMG_INT32	dwc_otg_pcd_ep_is_queued (dwc_otg_pcd_ep_t *ep)
{
	return (!LST_empty(&ep->sRequestList));
}

#if defined (_EN_ISOC_)

static struct usb_iso_request		asISORequestObjects[ MAX_REQUEST_OBJECTS ];
static LST_T						gsISODriverRequestList;

/**
 * This function allocates an ISOC request object to use with the specified
 * endpoint.
 *
 */
struct usb_iso_request * dwc_otg_pcd_alloc_iso_request()
{
	return ( (usb_iso_request_t *)LST_removeHead(&gsISODriverRequestList));
}

/**
 * This function frees a request object
 * @param req The request being freed
 */
IMG_VOID dwc_otg_pcd_free_iso_request( struct usb_iso_request *req )
{
	if ( !req )
	{
		IMG_ASSERT(0);
		return;
	}

	LST_add(&gsISODriverRequestList, req);
}

IMG_VOID dwc_otg_pcd_initialise_iso_request_list()
{
	img_uint32	i;

	LST_init(&gsISODriverRequestList);

	/* Populate the requests queue */
	for ( i = 0; i < MAX_REQUEST_OBJECTS; ++i )
	{
		LST_add(&gsISODriverRequestList, &asISORequestObjects[i]);
	}
}

#endif /* _EN_ISOC_ */

#if 0 // do not compile, as it is not used at the moment
/**
 * This function cancels an I/O request from an EP.
 */
static IMG_INT32 dwc_otg_pcd_ep_dequeue(dwc_otg_pcd_ep_t *ep, dwc_otg_pcd_request_t *req, USBD_DC_T	*	psContext)
{
	dwc_otg_pcd_request_t *reqTemp;
	dwc_otg_pcd_t	*pcd = psContext->psOtgPcd;

	if (!ep || !req || (!ep->desc && ep->dwc_ep.num != 0)) 
	{
		DWC_WARN("%s, bad argument\n");
		return -USBD_ERR_INVAL;
	}
	
	if (pcd->speed == USB_SPEED_UNKNOWN) 
	{
		DWC_WARN("%s, bogus device state\n");
		return -USBD_ERR_SHUTDOWN;
	}

	/* make sure it's actually queued on this endpoint */
	reqTemp = (dwc_otg_pcd_request_t*) LST_first(&ep->sRequestList);

	while (reqTemp)
	{
		if (reqTemp == req) 
		{
			break;
		}

		reqTemp = (dwc_otg_pcd_request_t*) LST_next(reqTemp);
	}

	if (reqTemp != req) 
	{
		SPIN_UNLOCK_IRQRESTORE(&pcd->lock, flags);
		return -USBD_ERR_INVAL;
	}

	request_done(ep, req, -USBD_ERR_CONNRESET);

	SPIN_UNLOCK_IRQRESTORE(&pcd->lock, flags);

	return 0;
}
#endif

#if 0 // do not compile, as it is not used at the moment 
/**
 * usb_ep_set_halt stalls an endpoint. 
 *
 * usb_ep_clear_halt clears an endpoint halt and resets its data
 * toggle.
 *
 * Both of these functions are implemented with the same underlying
 * function. The behavior depends on the value argument.
 * 
 * @param[in] ep the Endpoint to halt or clear halt.
 * @param[in] value 
 *	- 0 means clear_halt.
 *	- 1 means set_halt, 
 *	- 2 means clear stall lock flag.
 *	- 3 means set  stall lock flag.
 */
static IMG_INT32 dwc_otg_pcd_ep_set_halt(dwc_otg_pcd_ep_t * ep, IMG_INT32 value, USBD_DC_T	*	psContext)
{
	IMG_INT32		retval	= 0;
	dwc_otg_pcd_t	*pcd	= psContext->psOtgPcd;
		
	//DWC_DEBUGPL(DBG_PCD,"HALT %s %d\n", ep->name, value);

	if (!ep || (!ep->desc && ep != &pcd->ep0) || ep->desc->bmAttributes == USB_ENDPOINT_XFER_ISOC) 
	{
		DWC_WARN("%s, bad ep\n");
		return -USBD_ERR_INVAL;
	}
		
	SPIN_LOCK_IRQSAVE(&ep->pcd->lock, flags);
	if (!LST_empty(&ep->sRequestList))
	{
		DWC_WARN("%s() %s XFer In process\n");
		retval = -USBD_ERR_AGAIN;
	}
	else if (value == 0) 
	{
		dwc_otg_ep_clear_stall( &ep->dwc_ep );		 
	}
	else if(value == 1)
	{
		if (ep->dwc_ep.is_in == 1 && ep->pcd->otg_dev->core_if->dma_desc_enable)
		{
			dtxfsts_data_t	txstatus;
			fifosize_data_t	txfifosize;
			
			txfifosize.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_DPTXFSIZ_DIEPTXF_OFFSET + (4 * ep->dwc_ep.tx_fifo_num));
			txstatus.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DTXFSTS0 + (ep->dwc_ep.num * DWC_EP_REG_OFFSET));
			
			if (txstatus.b.txfspcavail < txfifosize.b.depth)
			{
				DWC_WARN("%s() %s Data In Tx Fifo\n", __func__, ep->name);
				retval = -USBD_ERR_AGAIN;
			}
			else
			{
				if (ep->dwc_ep.num == 0)
				{
					ep->pcd->ep0state = EP0_STALL;
				}

				ep->stopped = 1;
				dwc_otg_ep_set_stall( &ep->dwc_ep );
			}
		}
		else
		{
			if (ep->dwc_ep.num == 0) 
			{
				pcd->ep0state = EP0_STALL;
			}
		
			ep->stopped = 1;
			dwc_otg_ep_set_stall( &ep->dwc_ep );
		}
	}
	else if (value == 2) 
	{
		ep->dwc_ep.stall_clear_flag = 0;
	}
	else if (value == 3) 
	{
		ep->dwc_ep.stall_clear_flag = 1;
	}
		
	SPIN_UNLOCK_IRQRESTORE(&ep->pcd->lock, flags);
	return retval;
}
#endif


/*	Device Operations */

/* IMG_SYNOP_CR */

#ifdef DWC_SUPPORT_OTG
IMG_VOID dwc_otg_pcd_initiate_srp( USBD_DC_T	*	psContext)
{
	dwc_otg_pcd_t *pcd = psContext->psOtgPcd;
	IMG_UINT32 *addr = (IMG_UINT32 *) DWC_OTG_CORE_GLOBAL_REGS_GOTGCTL_OFFSET;
	gotgctl_data_t mem;
	gotgctl_data_t val;
		
	val.d32 = usb_ReadReg32( addr );
	if (val.b.sesreq) 
	{
		DWC_ERROR("Session Request Already active!\n");
			return;
	}

	DWC_NOTICE("Session Request Initated\n");
	mem.d32 = usb_ReadReg32(addr);
	mem.b.sesreq = 1;
	usb_WriteReg32(addr, mem.d32);

	/* Start the SRP timer */
	dwc_otg_pcd_start_srp_timer( );
	return;
}
#endif

#if 0
void dwc_otg_pcd_remote_wakeup(dwc_otg_pcd_t *pcd, int set)
{
	dctl_data_t dctl = {.d32=0};
	volatile uint32_t *addr = &(GET_CORE_IF(pcd)->dev_if->dev_global_regs->dctl);

	if (dwc_otg_is_device_mode(GET_CORE_IF(pcd))) {
		if (pcd->remote_wakeup_enable) {
			if (set) {
				dctl.b.rmtwkupsig = 1;
				dwc_modify_reg32(addr, 0, dctl.d32);
				DWC_DEBUGPL(DBG_PCD, "Set Remote Wakeup\n");
				mdelay(1);
				dwc_modify_reg32(addr, dctl.d32, 0);
				DWC_DEBUGPL(DBG_PCD, "Clear Remote Wakeup\n");
			}
			else {
			}
		}
		else {
			DWC_DEBUGPL(DBG_PCD, "Remote Wakeup is disabled\n");
		}
	}
	return;
}
#endif

#if 0
/**
 *
 * If the device is suspended, remote wakeup signaling is started.
 *
 */ MIKE - This function is different to the synopsys one - why so ?
IMG_INT32 dwc_otg_pcd_wakeup()
{
	dsts_data_t		dsts;
	dwc_otg_pcd_t	*_pcd	= sUSBDDeviceContext.psOtgPcd;
	dctl_data_t		dctl	= {0};
		
	/* Check if suspend state */
	dsts.d32 = usb_ReadReg32(DWC_OTG_DEV_GLOBAL_REGS_DSTS_OFFSET);

	if (dsts.b.suspsts && _pcd->remote_wakeup_enable) 
	{
		dctl.b.rmtwkupsig = 1;
		usb_ModifyReg32( DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, 0, dctl.d32 );
		DWC_DEBUGPL(DBG_PCD, "Set Remote Wakeup\n");
			
		/* Need to wait for 1ms<=t<=15ms */
		MDELAY(1);

		usb_ModifyReg32( DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, dctl.d32, 0 );
		DWC_DEBUGPL(DBG_PCD, "Clear Remote Wakeup\n");
	}
	
	return 0;
}
#endif

#ifdef DWC_SUPPORT_OTG
/**
 * Start the SRP timer to detect when the SRP does not complete within 
 * 6 seconds.
 *
 * @param _pcd the pcd structure.
 */
IMG_VOID dwc_otg_pcd_start_srp_timer( )
{
	dwc_otg_pcd_t *_pcd = sUSBDDeviceContext.psOtgPcd;
	struct timer_list *srp_timer = &_pcd->srp_timer;

	core_if->srp_timer_started = 1;
	init_timer( srp_timer );
	srp_timer->function = srp_timeout;
	srp_timer->data = (IMG_UINT32)core_if;
	srp_timer->expires = /*jiffies*/ 0 + (HZ*6);
	add_timer( srp_timer );
}
#endif

#endif /* DWC_HOST_ONLY */
