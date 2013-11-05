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

#if defined __META_MEOS__ || defined __MTX_MEOS__ || defined __META_NUCLEUS_PLUS__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <MeOS.h>

#include <img_defs.h>
#include <ioblock_defs.h>

#include "usbd_api.h"

#if !defined (USBD_NO_CBMAN_SUPPORT)
	#include "cbman.h"
#endif
 
#if !defined DWC_HOST_ONLY
#include "dwc_otg_pcd.h"

//#define DEBUG_EP0

/* request functions defined in "dwc_otg_pcd.c" */
extern IMG_VOID request_done( const ioblock_sBlockDescriptor * psBlockDesc, dwc_otg_pcd_ep_t *ep, dwc_otg_pcd_request_t *req, IMG_INT32 status );
extern IMG_VOID dwc_otg_pcd_ep_request_nuke( const ioblock_sBlockDescriptor * psBlockDesc, dwc_otg_pcd_ep_t *ep );
extern IMG_VOID complete_xiso_ep( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t	*	ep	);

#if defined DWC_SUPPORT_OTG
extern IMG_VOID dwc_otg_pcd_update_otg( const unsigned _reset );
#endif

/** @file 
 * This file contains the implementation of the PCD Interrupt handlers.
 *
 * The PCD handles the device interrupts.  Many conditions can cause a
 * device interrupt. When an interrupt occurs, the device interrupt
 * service routine determines the cause of the interrupt and
 * dispatches handling to the appropriate function. These interrupt
 * handling functions are described below.
 * All interrupt registers are processed from LSB to MSB.
 */


/**
 * This function prints the ep0 state for debug purposes.
 */
static __USBD_INLINE__ IMG_VOID print_ep0_state( USBD_DC_T	*	psContext ) 
{
#if defined USBD_DEBUG
	dwc_otg_pcd_t *pcd = psContext->psOtgPcd;
	IMG_CHAR str[40];
	
	switch (pcd->ep0state)
	{
	case EP0_DISCONNECT:
		strcpy(str, "EP0_DISCONNECT");
		break;
	case EP0_IDLE:
		strcpy(str, "EP0_IDLE");
		break;
	case EP0_IN_DATA_PHASE:
		strcpy(str, "EP0_IN_DATA_PHASE");
		break;
	case EP0_OUT_DATA_PHASE:
		strcpy(str, "EP0_OUT_DATA_PHASE");
		break;
	case EP0_IN_STATUS_PHASE:
		strcpy(str,"EP0_IN_STATUS_PHASE");
		break;
	case EP0_OUT_STATUS_PHASE:
		strcpy(str,"EP0_OUT_STATUS_PHASE");
		break;
	case EP0_STALL:
		strcpy(str,"EP0_STALL");
		break;
	default:
		strcpy(str,"EP0_INVALID");				 
	}
	
	DWC_DEBUGPL(DBG_ANY, "%s(%d)\n", str, pcd->ep0state);		  
#endif
}

/**
 * This function returns pointer to in ep struct with number ep_num
 */
static __USBD_INLINE_OPTIONAL__ dwc_otg_pcd_ep_t* get_in_ep( dwc_otg_core_if_t	*	core_if, dwc_otg_pcd_t	*pcd, IMG_UINT32 ep_num ) 
{
	IMG_INT32		i;
	IMG_INT32		num_in_eps	= core_if->dev_if.num_in_eps;
	
	if (ep_num == 0)
	{
		return &pcd->ep0;
	}
	else
	{
		for(i = 0; i < num_in_eps; ++i)
		{
			if(pcd->in_ep[i].dwc_ep.num == ep_num)
			{
				return &pcd->in_ep[i];
			}
		}
		return 0;
	}
}
/**
 * This function returns pointer to out ep struct with number ep_num
 */
static __USBD_INLINE_OPTIONAL__ dwc_otg_pcd_ep_t* get_out_ep( dwc_otg_core_if_t	*	core_if, dwc_otg_pcd_t	*pcd, IMG_UINT32 ep_num ) 
{
	IMG_INT32		i;
	IMG_INT32		num_out_eps	= core_if->dev_if.num_out_eps;
	
	if (ep_num == 0)
	{
		return &pcd->ep0;
	}
	else
	{
		for(i = 0; i < num_out_eps; ++i)
		{
			if(pcd->out_ep[i].dwc_ep.num == ep_num)
			{
				return &pcd->out_ep[i];
			}
		}
		return 0;
	}
}

/**
 * This functions gets a pointer to an EP from the wIndex address
 * value of the control request.
 */
static dwc_otg_pcd_ep_t * get_ep_by_addr ( dwc_otg_core_if_t	*	core_if, dwc_otg_pcd_t	*	pcd, IMG_UINT16 wIndex )
{
	IMG_INT32			i;
	dwc_otg_pcd_ep_t	*ep;


	if ((wIndex & USB_ENDPOINT_NUMBER_MASK) == 0)
	{
		return &pcd->ep0;
	}

	/* in endpoints */
	for (i = 0; i < core_if->dev_if.num_in_eps; i++)
	{
		IMG_UINT8	bEndpointAddress;

		ep = &pcd->in_ep[i];

		if (!ep->desc)
		{
			continue;
		}
		bEndpointAddress = ep->desc->bEndpointAddress;
		if ((wIndex ^ bEndpointAddress) & USB_DIR_IN)
		{
			continue;
		}
		if ((wIndex & USB_ENDPOINT_NUMBER_MASK) == (bEndpointAddress & USB_ENDPOINT_NUMBER_MASK))
		{
			return ep;
		}
	}


	/* out endpoints */
	for (i = 0; i < core_if->dev_if.num_out_eps; i++)
	{
		IMG_UINT8	bEndpointAddress;

		ep = &pcd->out_ep[i];

		if (!ep->desc)
		{
			continue;
		}
		bEndpointAddress = ep->desc->bEndpointAddress;
		if ((wIndex ^ bEndpointAddress) & USB_DIR_IN)
		{
			continue;
		}
		if ((wIndex & USB_ENDPOINT_NUMBER_MASK) == (bEndpointAddress & USB_ENDPOINT_NUMBER_MASK))
		{
			return ep;
		}
	}

	return 0;
}

/**
 * This function checks the EP request queue, if the queue is not
 * empty the next request is started.
 */
IMG_VOID start_next_request( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep )
{
	USB_DC_T				*	psContext = (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_pcd_request_t	*	req = 0;

	if (!LST_empty(&ep->sRequestList))
	{
		req = (dwc_otg_pcd_request_t *) LST_first(&ep->sRequestList);

		/* Setup and start the Transfer */
		ep->dwc_ep.start_xfer_buff = req->buf;
		ep->dwc_ep.xfer_buff = req->buf;
		ep->dwc_ep.xfer_len = req->length;
		ep->dwc_ep.xfer_count = 0;
		ep->dwc_ep.dma_addr = req->dma;
		ep->dwc_ep.sent_zlp = 0;
		ep->dwc_ep.total_len = ep->dwc_ep.xfer_len;
			
		if (req->zero)
		{
			if ((ep->dwc_ep.xfer_len % ep->dwc_ep.maxpacket == 0) &&
				(ep->dwc_ep.xfer_len != 0 ))
			{
				ep->dwc_ep.sent_zlp = 1;
			}
		}

		//DWC_ERROR(" -> starting transfer (start_next_req) %s %s\n",
		//_ep->ep.name, _ep->dwc_ep.is_in?"IN":"OUT");
			
		dwc_otg_ep_start_transfer( psBlockDesc, &(psContext->sOtgCoreIf), &ep->dwc_ep );
	}
}

/**
 * This function handles the SOF Interrupts. At this time the SOF
 * Interrupt is disabled.
 */
IMG_INT32 dwc_otg_pcd_handle_sof_intr( img_uint32	ui32BaseAddress )
{
	gintsts_data_t		gintsts;
		
	//DWC_DEBUGPL(DBG_PCD, "SOF\n");

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.sofintr = 1;
	usb_WriteReg32 ( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}

#if !defined USE_DMA_INTERNAL
/**
 * This function handles the Rx Status Queue Level Interrupt, which
 * indicates that there is a least one packet in the Rx FIFO.  The
 * packets are moved from the FIFO to memory, where they will be
 * processed when the Endpoint Interrupt Register indicates Transfer
 * Complete or SETUP Phase Done.
 *
 * Repeat the following until the Rx Status Queue is empty:
 *	 -# Read the Receive Status Pop Register (GRXSTSP) to get Packet
 *		info
 *	 -# If Receive FIFO is empty then skip to step Clear the interrupt
 *		and exit
 *	 -# If SETUP Packet call dwc_otg_read_setup_packet to copy the
 *		SETUP data to the buffer
 *	 -# If OUT Data Packet call dwc_otg_read_packet to copy the data
 *		to the destination buffer
 */
IMG_INT32 dwc_otg_pcd_handle_rx_status_q_level_intr( USB_DC_T	*	psContext )
{
	USBD_DC_T				*	psDevContext = psContext->psDevContext;
	dwc_otg_pcd_t				*pcd		 = psDevContext->psOtgPcd;
	dwc_otg_core_if_t		*	core_if		 = &(psContext->sOtgCoreIf);
	gintmsk_data_t				gintmask = { 0 };
	device_grxsts_data_t		status;
	dwc_otg_pcd_ep_t			*ep;
	gintsts_data_t				gintsts;

#if defined USBD_DEBUG
	static IMG_CHAR *dpid_str[] ={ "D0", "D2", "D1", "MDATA" };
#endif
		
	//DWC_DEBUGPL(DBG_PCDV, "%s(%p)\n", __func__, _pcd);
	/* Disable the Rx Status Queue Level interrupt */
	gintmask.b.rxstsqlvl = 1;
	usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, gintmask.d32, 0);


	/* Get the Status from the top of the FIFO */
	status.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GRXSTSP_OFFSET);

	DWC_DEBUGPL(DBG_PCD, "EP:%d BCnt:%d DPID:%s " 
					"pktsts:%x Frame:%d(0x%0x)\n", 
					status.b.epnum, status.b.bcnt,
					dpid_str[status.b.dpid], 
					status.b.pktsts, status.b.fn, status.b.fn);
	
	/* Get pointer to EP structure */
	ep = get_out_ep( core_if, pcd, status.b.epnum );
		
	switch (status.b.pktsts) 
	{
	case DWC_DSTS_GOUT_NAK:
		DWC_DEBUGPL(DBG_PCDV, "Global OUT NAK\n");
		break;
	case DWC_STS_DATA_UPDT:
		DWC_DEBUGPL(DBG_PCDV, "OUT Data Packet\n");
		if (status.b.bcnt && ep->dwc_ep.xfer_buff)
		{
			/* IMG_SYNOP_CR */
			/** Potential enhancement : check for buffer overflow */
			dwc_otg_read_packet( psBlockDesc->ui32Base, ep->dwc_ep.xfer_buff, status.b.bcnt );

			ep->dwc_ep.xfer_count += status.b.bcnt;
			ep->dwc_ep.xfer_buff += status.b.bcnt;
		}
		break;
	case DWC_STS_XFER_COMP:
		DWC_DEBUGPL(DBG_PCDV, "OUT Complete\n");
		break;
	case DWC_DSTS_SETUP_COMP:
#if defined DEBUG_EP0
		DWC_DEBUGPL(DBG_PCDV, "Setup Complete\n");
#endif
		break;
case DWC_DSTS_SETUP_UPDT:
		dwc_otg_read_setup_packet( psBlockDesc->ui32Base, pcd->setup_pkt->d32 );
#if defined DEBUG_EP0
		DWC_DEBUGPL(DBG_PCD, 
					"SETUP PKT: %02x.%02x v%04x i%04x l%04x\n",
					pcd->setup_pkt->req.bRequestType, 
					pcd->setup_pkt->req.bRequest,
					pcd->setup_pkt->req.wValue, 
					pcd->setup_pkt->req.wIndex, 
					pcd->setup_pkt->req.wLength);
#endif
		ep->dwc_ep.xfer_count += status.b.bcnt;
		break;
	default:
		DWC_DEBUGPL(DBG_PCDV, "Invalid Packet Status (0x%0x)\n", status.b.pktsts);
		break;
	}

	/* Enable the Rx Status Queue Level interrupt */
	usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, 0, gintmask.d32);


	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.rxstsqlvl = 1;
	usb_WriteReg32 ( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	//DWC_DEBUGPL(DBG_PCDV, "EXIT: %s\n", __func__);
	return 1;
}
#endif /* !USE_DMA_INTERNAL */

#if !defined USE_MULTIPLE_TX_FIFO
/**
 * This function examines the Device IN Token Learning Queue to
 * determine the EP number of the last IN token received.  This
 * implementation is for the Mass Storage device where there are only
 * 2 IN EPs (Control-IN and BULK-IN).
 *
 * The EP numbers for the first six IN Tokens are in DTKNQR1 and there
 * are 8 EP Numbers in each of the other possible DTKNQ Registers.
 *
 * @param core_if Programming view of DWC_otg controller.
 *
 */
static __USBD_INLINE__ IMG_INT32 get_ep_of_last_in_token( img_uint32	ui32BaseAddress, dwc_otg_core_if_t	*	core_if )
{
	const IMG_UINT32				TOKEN_Q_DEPTH		= core_if->hwcfg2.b.dev_token_q_depth;
	/* Number of Token Queue Registers */
	const IMG_INT32					DTKNQ_REG_CNT		= (TOKEN_Q_DEPTH + 7) / 8;
	dtknq1_data_t					dtknqr1;
	IMG_UINT32						in_tkn_epnums[4];
	IMG_INT32						ndx					= 0;
	IMG_INT32						i					= 0;
	IMG_UINT32						addr				= DWC_OTG_DEV_GLOBAL_REGS_DTKNQR1_OFFSET;
	IMG_INT32						epnum				= 0;

	//DWC_DEBUGPL(DBG_PCD,"dev_token_q_depth=%d\n",TOKEN_Q_DEPTH);
	
	
	/* Read the DTKNQ Registers */
	for (i = 0; i < DTKNQ_REG_CNT; i++) 
	{
		in_tkn_epnums[ i ] = usb_ReadReg32( ui32BaseAddress, addr);
		DWC_DEBUGPL(DBG_PCDV, "DTKNQR%d=0x%08x\n", i+1, in_tkn_epnums[i]);

		if (addr == DWC_OTG_DEV_GLOBAL_REGS_DVBUSDIS_OFFSET) 
		{
			addr = DWC_OTG_DEV_GLOBAL_REGS_DTKNQR3_DTHRCTL_OFFSET;
		} 
		else 
		{
			++addr;
		}
				
		}
		
		/* Copy the DTKNQR1 data to the bit field. */
		dtknqr1.d32 = in_tkn_epnums[0];
		/* Get the EP numbers */
		in_tkn_epnums[0] = dtknqr1.b.epnums0_5;
		ndx = dtknqr1.b.intknwptr - 1;

		//DWC_DEBUGPL(DBG_PCDV,"ndx=%d\n",ndx);
		if (ndx == -1) 
		{
			/* IMG_SYNOP_CR */
			/* Potential enhancement: improve calculation of maximum queue position */
			IMG_INT32 cnt = TOKEN_Q_DEPTH;
			if (TOKEN_Q_DEPTH <= 6) 
			{
				cnt = TOKEN_Q_DEPTH - 1;
			}
			else if (TOKEN_Q_DEPTH <= 14) 
			{
				cnt = TOKEN_Q_DEPTH - 7;
			} 
			else if (TOKEN_Q_DEPTH <= 22) 
			{
				cnt = TOKEN_Q_DEPTH - 15;
			} 
			else 
			{
				cnt = TOKEN_Q_DEPTH - 23;
			}
			epnum = (in_tkn_epnums[ DTKNQ_REG_CNT - 1 ] >> (cnt * 4)) & 0xF;
		} 
		else 
		{
			if (ndx <= 5) 
			{
				epnum = (in_tkn_epnums[0] >> (ndx * 4)) & 0xF;
			} 
			else if (ndx <= 13 ) 
			{
				ndx -= 6;
				epnum = (in_tkn_epnums[1] >> (ndx * 4)) & 0xF;
			} 
			else if (ndx <= 21 ) 
			{
				ndx -= 14;
				epnum = (in_tkn_epnums[2] >> (ndx * 4)) & 0xF;
			} 
			else if (ndx <= 29 ) 
			{
				ndx -= 22;
				epnum = (in_tkn_epnums[3] >> (ndx * 4)) & 0xF;
			}
		}
		//DWC_DEBUGPL(DBG_PCD,"epnum=%d\n",epnum);
		return epnum;
}
#endif


#if !defined USE_MULTIPLE_TX_FIFO
/* This interrupt occurs when the non-periodic Tx FIFO is half-empty. 
   The active request is checked for the next packet to be loaded into
   the non-periodic Tx FIFO. */
IMG_INT32 dwc_otg_pcd_handle_np_tx_fifo_empty_intr( const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	USB_DC_T				*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_core_if_t		*	core_if		= &(psContext->sOtgCoreIf);
	dwc_otg_pcd_t			*	pcd			= &(psContext->sDevContext.sOtgPcd);
	gnptxsts_data_t				txstatus	= { 0 };
	gintsts_data_t				gintsts;
	IMG_INT32					epnum		= 0;
	dwc_otg_pcd_ep_t			*ep			= 0;
	IMG_UINT32					len			= 0;
	IMG_INT32					dwords;

	/* Get the epnum from the IN Token Learning Queue. */
	epnum = get_ep_of_last_in_token( psBlockDesc->ui32Base, core_if );
	ep = get_in_ep( core_if, pcd, epnum );

	DWC_DEBUGPL(DBG_PCD, "NP TxFifo Empty: %s(%d) \n", ep->ep.name, epnum );

	//ep_regs = core_if->dev_if->in_ep_regs[epnum];
	len = ep->dwc_ep.xfer_len - ep->dwc_ep.xfer_count;
	if (len > ep->dwc_ep.maxpacket) 
	{
		len = ep->dwc_ep.maxpacket;
	}
	dwords = (len + 3)/4;
		
		
	/* While there is space in the queue and space in the FIFO and
	* More data to tranfer, Write packets to the Tx FIFO */
	txstatus.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GNPTXSTS_OFFSET);
	DWC_DEBUGPL(DBG_PCDV, "b4 GNPTXSTS=0x%08x\n",txstatus.d32);
	
	while  (txstatus.b.nptxqspcavail > 0 &&
			txstatus.b.nptxfspcavail > dwords &&
			ep->dwc_ep.xfer_count < ep->dwc_ep.xfer_len) 
	{
		/* Write the FIFO */
		dwc_otg_ep_write_packet( psBlockDesc->ui32Base, &ep->dwc_ep, 0 );
		len = ep->dwc_ep.xfer_len - ep->dwc_ep.xfer_count;
		
		if (len > ep->dwc_ep.maxpacket) 
		{
			len = ep->dwc_ep.maxpacket;
		}
		
		dwords = (len + 3)/4;
		txstatus.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GNPTXSTS_OFFSET);
		DWC_DEBUGPL(DBG_PCDV,"GNPTXSTS=0x%08x\n",txstatus.d32);
	}	
				
	DWC_DEBUGPL(DBG_PCDV, "GNPTXSTS=0x%08x\n", usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GNPTXSTS_OFFSET));

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.nptxfempty = 1;
	usb_WriteReg32 ( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}
#endif

#if !defined USE_DMA_INTERNAL
/**
 * This function is called when dedicated Tx FIFO Empty interrupt occurs.
 * The active request is checked for the next packet to be loaded into
 * apropriate Tx FIFO.
 */
static IMG_INT32 write_empty_tx_fifo( img_uint32 ui32BaseAddress, dwc_otg_core_if_t	*	core_if, dwc_otg_pcd_t	*	pcd, IMG_UINT32 epnum )
{
	dtxfsts_data_t				txstatus = { 0 };
	dwc_otg_pcd_ep_t			*ep			= 0;
	IMG_UINT32					len			= 0;
	IMG_INT32					dwords;


	ep = get_in_ep( core_if, pcd, epnum );

	DWC_DEBUGPL(DBG_PCD, "Dedicated TxFifo Empty: %s(%d) \n", ep->ep.name, epnum );

	//ep_regs = core_if->dev_if->in_ep_regs[epnum];
		
	len = ep->dwc_ep.xfer_len - ep->dwc_ep.xfer_count;

	if (len > ep->dwc_ep.maxpacket) 
	{
		len = ep->dwc_ep.maxpacket;
	}
	
	dwords = (len + 3)/4;
		
	/* While there is space in the queue and space in the FIFO and
	 * More data to tranfer, Write packets to the Tx FIFO */
	txstatus.d32 = usb_ReadReg32( ui32BaseAddress,  DWC_OTG_DEV_IN_EP_REGS_DTXFSTS0 + epnum * DWC_EP_REG_OFFSET);
	DWC_DEBUGPL(DBG_PCDV, "b4 dtxfsts[%d]=0x%08x\n",epnum,txstatus.d32);

	while  (txstatus.b.txfspcavail > dwords && 
			ep->dwc_ep.xfer_count < ep->dwc_ep.xfer_len &&
			ep->dwc_ep.xfer_len != 0) 
	{
		/* Write the FIFO */
		dwc_otg_ep_write_packet( ui32BaseAddress, &ep->dwc_ep, 0 );

		len = ep->dwc_ep.xfer_len - ep->dwc_ep.xfer_count;
		if (len > ep->dwc_ep.maxpacket) 
		{
			len = ep->dwc_ep.maxpacket;
		}
				
		dwords = (len + 3)/4;
		txstatus.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DTXFSTS0 + epnum * DWC_EP_REG_OFFSET);
		DWC_DEBUGPL(DBG_PCDV,"dtxfsts[%d]=0x%08x\n", epnum, txstatus.d32);
	}
				
	DWC_DEBUGPL(DBG_PCDV, "b4 dtxfsts[%d]=0x%08x\n",epnum,usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DTXFSTS0 + epnum * DWC_EP_REG_OFFSET));

	return 1;
}
#endif //USE_DMA_INTERNAL
 
#if defined DWC_SUPPORT_OTG //bojan: appears only to be called when otg interrupt happens
/** 
 * This function is called when the Device is disconnected.	 It stops
 * any active requests and informs the Gadget driver of the
 * disconnect.
 */
IMG_VOID dwc_otg_pcd_stop( )
{
	dwc_otg_pcd_t		*pcd = psContext->psOtgPcd;
	dwc_otg_core_if_t	*core_if = psContext->psOtgCoreIf;
	IMG_INT32			i, num_in_eps, num_out_eps;
	dwc_otg_pcd_ep_t	*ep;
	gintmsk_data_t		intr_mask;


	intr_mask.d32 = 0;

	num_in_eps = core_if->dev_if->num_in_eps;
	num_out_eps = core_if->dev_if->num_out_eps;
	
	DWC_DEBUGPL(DBG_PCDV, "%s() \n", __func__ );
	/* don't disconnect drivers more than once */
	if (pcd->ep0state == EP0_DISCONNECT) 
	{
		DWC_DEBUGPL(DBG_ANY, "%s() Already Disconnected\n", __func__ );
		return;		   
	}
	pcd->ep0state = EP0_DISCONNECT;

	/* Reset the OTG state. */
	dwc_otg_pcd_update_otg(1);

	/* Disable the NP Tx Fifo Empty Interrupt. */
	intr_mask.b.nptxfempty = 1;
	usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, 0);


	/* Flush the FIFOs */
	/** Potential enhancement: flush periodic FIFOs */
	/* IMG_SYNOP_CR */
	dwc_otg_flush_tx_fifo(0x10);
	dwc_otg_flush_rx_fifo( );

	/* prevent new request submissions, kill any outstanding requests  */
	ep = &pcd->ep0;
	dwc_otg_pcd_ep_request_nuke(ep, psContext);
	/* prevent new request submissions, kill any outstanding requests  */
	for (i = 0; i < num_in_eps; i++) 
	{
		dwc_otg_pcd_ep_t *ep = &pcd->in_ep[i];
		dwc_otg_pcd_ep_request_nuke(ep);
	}
	/* prevent new request submissions, kill any outstanding requests  */
	for (i = 0; i < num_out_eps; i++) 
	{
		dwc_otg_pcd_ep_t *ep = &pcd->out_ep[i];
		dwc_otg_pcd_ep_request_nuke(ep);
	}

	/* report disconnect; the driver is already quiesced */
	
	SPIN_UNLOCK(&_pcd->lock);
	usbd_Disconnect();
	SPIN_LOCK(&_pcd->lock);
}
#endif

#if defined USE_I2C 
/**
 * This interrupt indicates that ...
 */
IMG_INT32 dwc_otg_pcd_handle_i2c_intr( )
{
	gintmsk_data_t intr_mask = { 0 };
	gintsts_data_t gintsts;

	DWC_PRINT("INTERRUPT Handler not implemented for %s\n", "i2cintr"); 
	intr_mask.b.i2cintr = 1;
	usb_ModifyReg32( psBlockDesc->ui32Base,  DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, 0 );

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.i2cintr = 1;
	usb_WriteReg32 ( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}
#endif

/**
 * This interrupt indicates that ...
 */
IMG_INT32 dwc_otg_pcd_handle_early_suspend_intr( const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	gintsts_data_t		gintsts;
#if defined(VERBOSE)
	DWC_PRINT("Early Suspend Detected\n");
#endif
	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.erlysuspend = 1;
	usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);
	return 1;
}

/* IMG_SYNOP_CR */
/**
 * This function configures EPO to receive SETUP packets.
 *
 *	-# Program the following fields in the endpoint specific registers
 *	for Control OUT EP 0, in order to receive a setup packet
 *	- DOEPTSIZ0.Packet Count = 3 (To receive up to 3 back to back
 *	  setup packets)
 *	- DOEPTSIZE0.Transfer Size = 24 Bytes (To receive up to 3 back
 *	  to back setup packets)
 *		- In DMA mode, DOEPDMA0 Register with a memory address to
 *		  store any setup packets received
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param pcd	  Programming view of the PCD.
 */
static __USBD_INLINE_OPTIONAL__ IMG_VOID ep0_out_start( const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	USB_DC_T			*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
    dwc_otg_core_if_t	*	core_if		= &(psContext->sOtgCoreIf);
	dwc_otg_dev_if_t	*	dev_if		= &(core_if->dev_if);
	dwc_otg_pcd_t		*	pcd			= &(psContext->sDevContext.sOtgPcd);

	dwc_otg_dev_dma_desc_t	*dma_desc;
	depctl_data_t doepctl = { 0 };
  #if !defined (USE_DDMA_ONLY)
	deptsiz0_data_t		doeptsize0 = { 0 };
  #endif

  #if defined VERBOSE
	DWC_DEBUGPL(DBG_PCDV,"%s() doepctl0=%0x\n", __func__, usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0));
  #endif

  #if !defined (USE_DDMA_ONLY)
	doeptsize0.b.supcnt = 3;	
	doeptsize0.b.pktcnt = 1;
	doeptsize0.b.xfersize = 8*3;
  #endif
	
  #if !defined USE_DMA_INTERNAL
	if (core_if->dma_enable)
  #endif /* USE_DMA_INTERNAL */
  #if !defined (USE_DDMA_ONLY)
	{
		if ( !core_if->dma_desc_enable )
		{
			/** put here as for Hermes mode deptisz register should not be written */
			usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0, doeptsize0.d32 );

			/* IMG_SYNOP_CR */
			/** Potential enhancement: permit DMA to handle multiple setup packets (up to 3) */
			usb_WriteMemrefReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0, pcd->setup_pkt_dma_handle);
		}
		else
		{
  #endif /* !USE_DDMA_ONLY */
			dev_if->setup_desc_index = (dev_if->setup_desc_index + 1) & 1;
			dma_desc = dev_if->setup_desc_addr[dev_if->setup_desc_index];

			/** DMA Descriptor Setup */
			dma_desc->status.b.bs = BS_HOST_BUSY;
			dma_desc->status.b.l = 1;
			dma_desc->status.b.ioc = 1;
			dma_desc->status.b.bytes = pcd->ep0.dwc_ep.maxpacket;
			dma_desc->buf = pcd->setup_pkt_dma_handle;
			dma_desc->status.b.bs = BS_HOST_READY;

			/** DOEPDMA0 Register write */
			usb_WriteMemrefReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0, dev_if->dma_setup_desc_addr[dev_if->setup_desc_index]);
  #if !defined (USE_DDMA_ONLY)
		}	
	}
  #endif /* !USE_DDMA_ONLY */
  #if !defined USE_DMA_INTERNAL
	else
	{
		/** put here as for Hermes mode deptisz register should not be written */
		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0, doeptsize0.d32 );
	}
  #endif /* USE_DMA_INTERNAL */
		
	/** DOEPCTL0 Register write */
	doepctl.b.epena = 1;
	doepctl.b.cnak = 1;
	usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0, doepctl.d32);

 #if defined VERBOSE
	DWC_DEBUGPL(DBG_PCDV,"doepctl0=%0x\n", usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0));
	DWC_DEBUGPL(DBG_PCDV,"diepctl0=%0x\n", usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0));
 #endif
}

/**
 * This interrupt occurs when a USB Reset is detected.	When the USB
 * Reset Interrupt occurs the device state is set to DEFAULT and the
 * EP0 state is set to IDLE.
 *	-#	Set the NAK bit for all OUT endpoints (DOEPCTLn.SNAK = 1)
 *	-#	Unmask the following interrupt bits
 *		- DAINTMSK.INEP0 = 1 (Control 0 IN endpoint)
 *	- DAINTMSK.OUTEP0 = 1 (Control 0 OUT endpoint)
 *	- DOEPMSK.SETUP = 1
 *	- DOEPMSK.XferCompl = 1
 *	- DIEPMSK.XferCompl = 1
 *	- DIEPMSK.TimeOut = 1
 *	-# Program the following fields in the endpoint specific registers
 *	for Control OUT EP 0, in order to receive a setup packet
 *	- DOEPTSIZ0.Packet Count = 3 (To receive up to 3 back to back
 *	  setup packets)
 *	- DOEPTSIZE0.Transfer Size = 24 Bytes (To receive up to 3 back
 *	  to back setup packets)
 *		- In DMA mode, DOEPDMA0 Register with a memory address to
 *		  store any setup packets received
 * At this point, all the required initialization, except for enabling
 * the control 0 OUT endpoint is done, for receiving SETUP packets.
 */
IMG_INT32 dwc_otg_pcd_handle_usb_reset_intr( const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	USB_DC_T			*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_core_if_t	*	core_if		= &(psContext->sOtgCoreIf);
	dwc_otg_dev_if_t	*	dev_if		= &(core_if->dev_if);
#if defined (_EN_ISOC_)
	dwc_otg_pcd_t		*	pcd			= &(psContext->sDevContext.sOtgPcd);
#endif
	depctl_data_t			doepctl		= {0};
	daint_data_t			daintmsk	= {0};
	doepmsk_data_t			doepmsk		= {0};
	diepmsk_data_t			diepmsk		= {0};
	dcfg_data_t				dcfg		= {0};
	grstctl_t				resetctl	= {0};
	dctl_data_t				dctl		= {0};
	IMG_INT32				i			= 0;
	gintsts_data_t			gintsts;
						

	DWC_PRINT("USB RESET\n");
#if defined (_EN_ISOC_)
	for ( i = 1; i < 16; ++i )
	{
		dwc_otg_pcd_ep_t	*	ep;
		ep = get_in_ep( core_if, pcd, i );
		if ( ep )
		{
			ep->dwc_ep.next_frame = 0xffffffff;
		}
	}
#endif

  #if defined DWC_SUPPORT_OTG
	/* reset the HNP settings */
	dwc_otg_pcd_update_otg(1);
  #endif

	/* Clear the Remote Wakeup Signalling */
	dctl.b.rmtwkupsig = 1;
	usb_ModifyReg32( psBlockDesc->ui32Base,  DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, dctl.d32, 0 );

	/* Set NAK for all OUT EPs */
	doepctl.b.snak = 1;
	for (i=0; i <= dev_if->num_out_eps; i++) 
	{
		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + i*DWC_EP_REG_OFFSET, doepctl.d32 );
	}

	/* Flush the NP Tx FIFO */
	dwc_otg_flush_tx_fifo( psBlockDesc->ui32Base, 0x10 );
	/* Flush the Learning Queue */
	resetctl.b.intknqflsh = 1;
	usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GRSTCTL_OFFSET, resetctl.d32);

	daintmsk.b.inep0 = 1;
	daintmsk.b.outep0 = 1;
	usb_WriteReg32( psBlockDesc->ui32Base,  DWC_OTG_DEV_GLOBAL_REGS_DAINTMSK_OFFSET, daintmsk.d32 );

	doepmsk.b.setup = 1;
	doepmsk.b.xfercompl = 1;
	doepmsk.b.ahberr = 1;
	doepmsk.b.epdisabled = 1;

	if(core_if->dma_desc_enable)
	{
		doepmsk.b.stsphsercvd = 1;
		//doepmsk.b.bna = 1;
	}

	usb_WriteReg32( psBlockDesc->ui32Base,  DWC_OTG_DEV_GLOBAL_REGS_DOEPMSK_OFFSET, doepmsk.d32 );

	diepmsk.b.xfercompl = 1;
	diepmsk.b.timeout = 1;
	diepmsk.b.epdisabled = 1;
	diepmsk.b.ahberr = 1;
	diepmsk.b.intknepmis = 1;

/*	if(core_if->dma_desc_enable)
	{
		diepmsk.b.bna = 1;
	}*/
	usb_WriteReg32( psBlockDesc->ui32Base,  DWC_OTG_DEV_GLOBAL_REGS_DIEPMSK_OFFSET, diepmsk.d32 );		 
	/* Reset Device Address */
	dcfg.d32 = usb_ReadReg32( psBlockDesc->ui32Base,  DWC_OTG_DEV_GLOBAL_REGS_DCFG_OFFSET );
	dcfg.b.devaddr = 0;
	usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DCFG_OFFSET, dcfg.d32);

	/* setup EP0 to receive SETUP packets */
	ep0_out_start( psBlockDesc );

	/* IMG_SYNOP_CR */
	SPIN_UNLOCK(&pcd->lock);
	usbd_BusReset ( psBlockDesc );
	SPIN_LOCK(&pcd->lock);

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.usbreset = 1;
	usb_WriteReg32 ( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}

/**
 * Get the device speed from the device status register and convert it
 * to USB speed constant.
 *
 * @param _core_if Programming view of DWC_otg controller.
 */
static IMG_INT32 get_device_speed( img_uint32	ui32BaseAddress )
{
	dsts_data_t				dsts;
	enum usb_device_speed	speed		= USB_SPEED_UNKNOWN;
	dsts.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DSTS_OFFSET);

	switch (dsts.b.enumspd) 
	{
		case DWC_DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ:
			speed = USB_SPEED_HIGH;
			break;
		case DWC_DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ:
		case DWC_DSTS_ENUMSPD_FS_PHY_48MHZ:
			speed = USB_SPEED_FULL;
			break;
	
		case DWC_DSTS_ENUMSPD_LS_PHY_6MHZ:
			speed = USB_SPEED_LOW;
			break;
	}

	return speed; 
}

/**
 * Read the device status register and set the device speed in the
 * data structure.	
 * Set up EP0 to receive SETUP packets by calling dwc_ep0_activate.
 */
IMG_INT32 dwc_otg_pcd_handle_enum_done_intr( const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	USB_DC_T			*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_pcd_t		*	pcd			= &(psContext->sDevContext.sOtgPcd);
	dwc_otg_pcd_ep_t	*	ep0			= &pcd->ep0;
	gintsts_data_t			gintsts;
	gusbcfg_data_t			gusbcfg;
	IMG_UINT8				utmi16b, utmi8b;
	
	DWC_DEBUGPL(DBG_PCD, "SPEED ENUM\n");

	utmi16b = 6;
	utmi8b = 9;

	dwc_otg_ep0_activate( psBlockDesc->ui32Base, &ep0->dwc_ep );

#if defined DEBUG_EP0
	print_ep0_state( &(psContext->sDevContext) );
#endif
		
	if (pcd->ep0state == EP0_DISCONNECT) 
	{
		pcd->ep0state = EP0_IDLE;
	} 
	else if (pcd->ep0state == EP0_STALL) 
	{
		pcd->ep0state = EP0_IDLE;
	}
	
	pcd->ep0state = EP0_IDLE;

	ep0->stopped = 0;
		
	pcd->speed = get_device_speed( psBlockDesc->ui32Base );

	/* Set USB turnaround time based on device speed and PHY interface. */
	gusbcfg.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET);

	/* IMG_SYNOP_CR */
	if (pcd->speed == USB_SPEED_HIGH) 
	{
		dwc_otg_core_if_t	*	core_if = &(psContext->sOtgCoreIf);
		if (core_if->hwcfg2.b.hs_phy_type == DWC_HWCFG2_HS_PHY_TYPE_ULPI) 
		{
			/* ULPI interface */
			gusbcfg.b.usbtrdtim = 9;
		}
		if (core_if->hwcfg2.b.hs_phy_type == DWC_HWCFG2_HS_PHY_TYPE_UTMI) 
		{
			/* UTMI+ interface */
			if (core_if->hwcfg4.b.utmi_phy_data_width == 0) 
			{
				gusbcfg.b.usbtrdtim = utmi8b;
			}
			else if (core_if->hwcfg4.b.utmi_phy_data_width == 1) 
			{
				gusbcfg.b.usbtrdtim = utmi16b;
			}
			else if (core_if->core_params.phy_utmi_width == 8) 
			{
				gusbcfg.b.usbtrdtim = utmi8b;
			} 
			else 
			{
				gusbcfg.b.usbtrdtim = utmi16b;
			}
		}
		if (core_if->hwcfg2.b.hs_phy_type == DWC_HWCFG2_HS_PHY_TYPE_UTMI_ULPI) 
		{
			/* UTMI+  OR  ULPI interface */
			if (gusbcfg.b.ulpi_utmi_sel == 1) 
			{
				/* ULPI interface */
				gusbcfg.b.usbtrdtim = 9;
			} 
			else 
			{
				/* UTMI+ interface */
				if (core_if->core_params.phy_utmi_width == 16) 
				{
					gusbcfg.b.usbtrdtim = utmi16b;
				} 
				else 
				{
					gusbcfg.b.usbtrdtim = utmi8b;
				}
			}
		}
	} 
	else 
	{
		/* Full or low speed */
		gusbcfg.b.usbtrdtim = 9;
	}

	/* IMG_SYNOP_CR */

	usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET, gusbcfg.d32);

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.enumdone = 1;
	usb_WriteReg32( psBlockDesc->ui32Base,  DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32 );
	return 1;
}


#if defined (USE_PERIODIC_EP)
/**
 * This interrupt indicates that the ISO OUT Packet was dropped due to
 * Rx FIFO full or Rx Status Queue Full.  If this interrupt occurs
 * read all the data from the Rx FIFO.
 */
IMG_INT32 dwc_otg_pcd_handle_isoc_out_packet_dropped_intr( img_uint32	ui32BaseAddress )
{
	gintmsk_data_t		intr_mask	= {0};
	gintsts_data_t		gintsts;
	
	DWC_PRINT("INTERRUPT Handler not implemented for %s\n", "ISOC Out Dropped");

	intr_mask.b.isooutdrop = 1;
	usb_ModifyReg32( ui32BaseAddress,  DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, 0 );

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.isooutdrop = 1;
	usb_WriteReg32 ( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}

/**
 * This interrupt indicates the end of the portion of the micro-frame
 * for periodic transactions.  If there is a periodic transaction for
 * the next frame, load the packets into the EP periodic Tx FIFO.
 */
IMG_INT32 dwc_otg_pcd_handle_end_periodic_frame_intr( img_uint32	ui32BaseAddress )
{
	gintmsk_data_t		intr_mask	= {0};
	gintsts_data_t		gintsts;
	
	DWC_PRINT("INTERRUPT Handler not implemented for %s\n", "EOP");

	intr_mask.b.eopframe = 1;
	usb_ModifyReg32( ui32BaseAddress,  DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, 0 );

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.eopframe = 1;
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}

#endif /* USE_PERIODIC_EP */

#if !defined USE_MULTIPLE_TX_FIFO
/**
 * This interrupt indicates that EP of the packet on the top of the
 * non-periodic Tx FIFO does not match EP of the IN Token received.
 *
 * The "Device IN Token Queue" Registers are read to determine the
 * order the IN Tokens have been received.	The non-periodic Tx FIFO
 * is flushed, so it can be reloaded in the order seen in the IN Token
 * Queue.
 */
IMG_INT32 dwc_otg_pcd_handle_ep_mismatch_intr( img_uint32	ui32BaseAddress )
{
	gintsts_data_t		gintsts;
	
	DWC_DEBUGPL(DBG_PCDV, "%s(%p)\n", __func__, _core_if);

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.epmismatch = 1;
	usb_WriteReg32 ( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}
#endif

/**
 * This funcion stalls EP0.
 */
static __USBD_INLINE_OPTIONAL__ IMG_VOID ep0_do_stall( const ioblock_sBlockDescriptor	*	psBlockDesc, const IMG_INT32 err_val ) 
{
	USB_DC_T			*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_pcd_t		*	pcd			= &(psContext->sDevContext.sOtgPcd);
	dwc_otg_pcd_ep_t	*	ep0			= &pcd->ep0;
	
	DWC_WARN("req %02x.%02x protocol STALL; err %d\n", ctrl->bRequestType, ctrl->bRequest, err_val);

	ep0->dwc_ep.is_in = 1;
	dwc_otg_ep_set_stall( psBlockDesc->ui32Base, &ep0->dwc_ep );		
	pcd->ep0.stopped = 1;
	pcd->ep0state = EP0_IDLE;
	ep0_out_start( psBlockDesc );
}

/**
 * This functions delegates the setup command to the gadget driver.
 */
static __USBD_INLINE_OPTIONAL__ IMG_VOID do_gadget_setup( const ioblock_sBlockDescriptor	*	psBlockDesc, struct usb_ctrlrequest_t * ctrl )
{
	USB_DC_T		*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_pcd_t	*	pcd			= &(psContext->sDevContext.sOtgPcd);
	IMG_INT32			ret			= 0;
	
	SPIN_UNLOCK(&pcd->lock);
	ret = usbd_Setup( psBlockDesc, ctrl );
	SPIN_LOCK(&pcd->lock);

	if (ret < 0) 
	{
		ep0_do_stall( psBlockDesc, ret );
	}

	/* IMG_SYNOP_CR */
	/** @This is a g_file_storage gadget driver specific
	* workaround: a DELAYED_STATUS result from the fsg_setup
	* routine will result in the gadget queueing a EP0 IN status
	* phase for a two-stage control transfer.	Exactly the same as
	* a SET_CONFIGURATION/SET_INTERFACE except that this is a class
	* specific request.  Need a generic way to know when the gadget
	* driver will queue the status phase.	Can we assume when we
	* call the gadget driver setup() function that it will always
	* queue and require the following flag?
	*/
	if (ret == 256 + 999) 
	{
		pcd->request_config = 1;
	}
}

/**
 * This function starts the Zero-Length Packet for the IN status phase
 * of a 2 stage control transfer. 
 */
static __USBD_INLINE_OPTIONAL__ IMG_VOID do_setup_in_status_phase( const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	USB_DC_T			*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_pcd_t		*	pcd			= &(psContext->sDevContext.sOtgPcd);
	dwc_otg_pcd_ep_t	*	ep0			= &pcd->ep0;
	if (pcd->ep0state == EP0_STALL)
	{
		return;
	}
			
	pcd->ep0state = EP0_IN_STATUS_PHASE;
		
	/* Prepare for more SETUP Packets */
	DWC_DEBUGPL(DBG_PCD, "EP0 IN ZLP\n");
	ep0->dwc_ep.xfer_len = 0;
	ep0->dwc_ep.xfer_count = 0;
	ep0->dwc_ep.is_in = 1;
	ep0->dwc_ep.dma_addr = pcd->setup_pkt_dma_handle;

	dwc_otg_ep0_start_transfer( psBlockDesc, &(psContext->sOtgCoreIf), &ep0->dwc_ep );

	/* Prepare for more SETUP Packets */
	//ep0_out_start( psContext );
}

/**
 * This function starts the Zero-Length Packet for the OUT status phase
 * of a 2 stage control transfer. 
 */
static __USBD_INLINE_OPTIONAL__ IMG_VOID do_setup_out_status_phase( const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	USB_DC_T			*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_pcd_t		*	pcd			= &(psContext->sDevContext.sOtgPcd);
	dwc_otg_pcd_ep_t	*	ep0			= &pcd->ep0;
	
	if (pcd->ep0state == EP0_STALL)
	{
		DWC_DEBUGPL(DBG_PCD, "EP0 STALLED\n");
		return;
	}
	
	pcd->ep0state = EP0_OUT_STATUS_PHASE;
		
	DWC_DEBUGPL(DBG_PCD, "EP0 OUT ZLP\n");
	ep0->dwc_ep.xfer_len = 0;
	ep0->dwc_ep.xfer_count = 0;
	ep0->dwc_ep.is_in = 0;
	ep0->dwc_ep.dma_addr = pcd->setup_pkt_dma_handle;
	dwc_otg_ep0_start_transfer( psBlockDesc, &(psContext->sOtgCoreIf), &ep0->dwc_ep );

	/* Prepare for more SETUP Packets */
  #if !defined USE_DMA_INTERNAL
	if ( !psContext->sOtgCoreIf.dma_enable )
	{
		ep0_out_start( psBlockDesc );
	}
  #endif /* USE_DMA_INTERNAL */
}

/**
 * Clear the EP halt (STALL) and if pending requests start the
 * transfer.
 */
static __USBD_INLINE__ IMG_VOID pcd_clear_halt( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep )
{
  #if !defined USE_DMA_INTERNAL
	USB_DC_T			*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_core_if_t	*	core_if		= &(psContext->sOtgCoreIf);
  #endif

	if (ep->dwc_ep.stall_clear_flag == 0)
	{
		dwc_otg_ep_clear_stall( psBlockDesc->ui32Base, &ep->dwc_ep );
	}

	/* Reactivate the EP */
	dwc_otg_ep_activate( psBlockDesc->ui32Base, &ep->dwc_ep );
	if (ep->stopped) 
	{
		ep->stopped = 0;
		/* If there is a request in the EP queue start it */ 

		/* IMG_SYNOP_CR */
		/** Potential enhancement: this causes an EP mismatch in DMA mode.
		 * epmismatch not yet implemented. */

		/*
		 * Above is solved by implementing a tasklet to call the
		 * start_next_request(), outside of interrupt context at some
		 * time after the current time, after a clear-halt setup packet.
		 * Still need to implement ep mismatch in the future if a gadget
		 * ever uses more than one endpoint at once 
		 */
	  #if !defined USE_DMA_INTERNAL
		if (core_if->dma_enable)
	  #endif /* USE_DMA_INTERNAL */
		{
			ep->queue_sof = 1;
			
			/* Bojan: what happens here is that we want the tasklet to be executed till after the isr has completed.
			   Kick off a timer task to be executed as soon as the ISR completes */
		  #if defined(TIMER_NOT_AVAILABLE)
			bStartXferTimer = IMG_TRUE;
		  #else
			KRN_setTimer(&tStartXferTimer, &start_xfer_func, (img_void *)psBlockDesc, 0);
		  #endif
		}
	  #if !defined USE_DMA_INTERNAL
		else 
		{
		  #if 0
			ep->queue_sof = 1;
			DWC_ERROR("tasklet schedule\n");
		   #if 1
			IMG_ASSERT(0);		//bojan
		   #else
			tasklet_schedule (pcd->start_xfer_tasklet);
		   #endif
			if (core_if->core_params->opt) 
			{
				start_next_request( ep );
			}
		  #endif
		}
	  #endif /* USE_DMA_INTERNAL */
	} 
	/* Start Control Status Phase */
	do_setup_in_status_phase( psBlockDesc );
}

#if defined(TIMER_NOT_AVAILABLE)

IMG_BOOL	bDoTestMode = IMG_FALSE;

IMG_VOID do_test_mode_func( IMG_VOID	*	pParam )

#else
/* Timer object to use to kick off the test mode function */
static KRN_TIMER_T		tDoTestModeTimer;

/**
 * This function is called when the SET_FEATURE TEST_MODE Setup packet
 * is sent from the host.  The Device Control register is written with
 * the Test Mode bits set to the specified Test Mode.  This is done in a
 * a timer callback so that the "Status" phase of the control transfer
 	* completes before transmitting the TEST packets.
 */
static IMG_VOID do_test_mode_func(KRN_TIMER_T *timer, IMG_VOID * pParam)
#endif
{
	ioblock_sBlockDescriptor	*	psBlockDesc = (ioblock_sBlockDescriptor *)pParam;
	USB_DC_T					*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dctl_data_t						dctl;
	dwc_otg_pcd_t				*	pcd			= &(psContext->sDevContext.sOtgPcd);
	IMG_INT32						test_mode	= pcd->test_mode;

	dctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET);
	switch (test_mode) 
	{
	case 1: // TEST_J
		dctl.b.tstctl = 1;
		break;

	case 2: // TEST_K	
		dctl.b.tstctl = 2;
		break;

	case 3: // TEST_SE0_NAK
		dctl.b.tstctl = 3;
		break;

	case 4: // TEST_PACKET
		dctl.b.tstctl = 4;
		break;

	case 5: // TEST_FORCE_ENABLE
		dctl.b.tstctl = 5;
		break;
	}
	usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, dctl.d32);
}

/**
 * This function process the GET_STATUS Setup Commands.
 */
static __USBD_INLINE__ IMG_VOID do_get_status( const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	USB_DC_T			*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_core_if_t	*	core_if		= &(psContext->sOtgCoreIf);
	dwc_otg_pcd_t		*	pcd			= &(psContext->sDevContext.sOtgPcd);
	struct usb_ctrlrequest_t	ctrl	= pcd->setup_pkt->req;
	dwc_otg_pcd_ep_t	*	ep;
	dwc_otg_pcd_ep_t	*	ep0			= &pcd->ep0;
	IMG_UINT16			*	status		= pcd->status_buf;

#if defined DEBUG_EP0
	DWC_DEBUGPL(DBG_PCD, 
			"GET_STATUS %02x.%02x v%04x i%04x l%04x\n",
			ctrl.bRequestType, ctrl.bRequest,
			ctrl.wValue, ctrl.wIndex, ctrl.wLength);
#endif

	switch (ctrl.bRequestType & USB_RECIP_MASK) 
	{
		case USB_RECIP_DEVICE:
			*status = 0x1; /* Self powered */
			*status |= pcd->remote_wakeup_enable << 1; 
			break;
	
		case USB_RECIP_INTERFACE:
			*status = 0;
			break;						  
	
		case USB_RECIP_ENDPOINT:
			ep = get_ep_by_addr( core_if, pcd, ctrl.wIndex );
			if (ep == 0 || ctrl.wLength > 2) 
			{
				ep0_do_stall( psBlockDesc, -USBD_ERR_OPNOTSUPP );
				return;
			}
			
			/* IMG_SYNOP_CR */
			/** Potential enhancement: check for EP stall */
			*status = ep->stopped;
			break;
	}
	pcd->ep0_pending = 1;

	if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
	{
		psBlockDesc->psSystemDescriptor->pfn_FlushCache( (IMG_UINT32)pcd->status_buf, 2 );
	}

	ep0->dwc_ep.start_xfer_buff = (IMG_UINT8 *)status;
	ep0->dwc_ep.xfer_buff = (IMG_UINT8 *)status;
	ep0->dwc_ep.dma_addr = pcd->status_buf_dma_handle;
	ep0->dwc_ep.xfer_len = 2;
	ep0->dwc_ep.xfer_count = 0;
	ep0->dwc_ep.total_len = ep0->dwc_ep.xfer_len;
	dwc_otg_ep0_start_transfer( psBlockDesc, core_if, &ep0->dwc_ep );
}		
/**
 * This function process the SET_FEATURE Setup Commands.
 */
static __USBD_INLINE__ IMG_VOID do_set_feature( const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	USB_DC_T					*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_pcd_t				*	pcd			= &(psContext->sDevContext.sOtgPcd);
	struct usb_ctrlrequest_t		ctrl		= pcd->setup_pkt->req;
	dwc_otg_pcd_ep_t			*	ep			= 0;


	DWC_DEBUGPL(DBG_PCD, "SET_FEATURE:%02x.%02x v%04x i%04x l%04x\n", ctrl.bRequestType, ctrl.bRequest, ctrl.wValue, ctrl.wIndex, ctrl.wLength);
	DWC_DEBUGPL(DBG_PCD,"otg_cap=%d\n", otg_cap_param);
		
	switch (ctrl.bRequestType & USB_RECIP_MASK) 
	{
	case USB_RECIP_DEVICE:
		switch (ctrl.wValue) 
		{
		case USB_DEVICE_REMOTE_WAKEUP:
			pcd->remote_wakeup_enable = 1;
			break;

		case USB_DEVICE_TEST_MODE:
			/* Kick off a timer to do the Test Packet generation after the SETUP Status phase has completed. */			
			pcd->test_mode = ctrl.wIndex >> 8;

		  #if defined(TIMER_NOT_AVAILABLE)
			bDoTestMode = IMG_TRUE;
		  #else
			KRN_setTimer(&tDoTestModeTimer, &do_test_mode_func, (img_void *)psBlockDesc, 0);
		  #endif
			break;

		case USB_DEVICE_B_HNP_ENABLE:
		  #if defined DWC_SUPPORT_OTG			
			DWC_DEBUGPL(DBG_PCDV, "SET_FEATURE: USB_DEVICE_B_HNP_ENABLE\n");
						
			/* dev may initiate HNP */
			if (otg_cap_param == DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE) 
			{
				pcd->b_hnp_enable = 1;
				dwc_otg_pcd_update_otg( 0 );
				DWC_DEBUGPL(DBG_PCD, "Request B HNP\n");
				/* IMG_SYNOP_CR */
				/* Is the gotgctl.devhnpen cleared by a USB Reset? */
				gotgctl.b.devhnpen = 1;
				gotgctl.b.hnpreq = 1;
				usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GOTGCTL_OFFSET, gotgctl.d32 );
			}
			else 
			{
				ep0_do_stall( -USBD_ERR_OPNOTSUPP);
			}
		  #else
			IMG_ASSERT(0);	//Bojan
		  #endif
			break;

		case USB_DEVICE_A_HNP_SUPPORT:
		  #if defined DWC_SUPPORT_OTG
			/* RH port supports HNP */
			DWC_DEBUGPL(DBG_PCDV, "SET_FEATURE: USB_DEVICE_A_HNP_SUPPORT\n");
			if (otg_cap_param == DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE)
			{
				pcd->a_hnp_support = 1;
				dwc_otg_pcd_update_otg( 0 );
			} 
			else 
			{
				ep0_do_stall( -USBD_ERR_OPNOTSUPP);
			}
		  #else
			IMG_ASSERT(0);	//Bojan
		  #endif
			break;

		case USB_DEVICE_A_ALT_HNP_SUPPORT:
		  #if defined DWC_SUPPORT_OTG
			/* other RH port does */
			DWC_DEBUGPL(DBG_PCDV, "SET_FEATURE: USB_DEVICE_A_ALT_HNP_SUPPORT\n");
			if (otg_cap_param == DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE)
			{
				pcd->a_alt_hnp_support = 1;
				dwc_otg_pcd_update_otg( 0 );
			} 
			else 
			{
				ep0_do_stall( -USBD_ERR_OPNOTSUPP);
			}
		  #else
			IMG_ASSERT(0);	//bojan	
		  #endif
			break;
		default:
			ep0_do_stall( psBlockDesc, -USBD_ERR_OPNOTSUPP );
		}
		do_setup_in_status_phase( psBlockDesc );
		break;

	case USB_RECIP_INTERFACE:
		do_gadget_setup( psBlockDesc, &ctrl );
		break;

	case USB_RECIP_ENDPOINT:
		if (ctrl.wValue == USB_ENDPOINT_HALT) 
		{
			ep = get_ep_by_addr( &(psContext->sOtgCoreIf), pcd, ctrl.wIndex );
			if (ep == 0) 
			{
				ep0_do_stall( psBlockDesc, -USBD_ERR_OPNOTSUPP );
				return;
			} 
			ep->stopped = 1;
			dwc_otg_ep_set_stall( psBlockDesc->ui32Base, &ep->dwc_ep );
		}
		do_setup_in_status_phase( psBlockDesc );
		break;
	}
}

/**
 * This function process the CLEAR_FEATURE Setup Commands.
 */
static __USBD_INLINE__ IMG_VOID do_clear_feature(  const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	USB_DC_T					*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_pcd_t				*	pcd			= &(psContext->sDevContext.sOtgPcd);
	struct usb_ctrlrequest_t		ctrl		= pcd->setup_pkt->req;
	dwc_otg_pcd_ep_t			*	ep			= 0;


	DWC_DEBUGPL(DBG_PCD, 
				"CLEAR_FEATURE:%02x.%02x v%04x i%04x l%04x\n",
				ctrl.bRequestType, ctrl.bRequest,
				ctrl.wValue, ctrl.wIndex, ctrl.wLength);

	switch (ctrl.bRequestType & USB_RECIP_MASK) 
	{
	case USB_RECIP_DEVICE:
		switch (ctrl.wValue) 
		{
		case USB_DEVICE_REMOTE_WAKEUP:
			pcd->remote_wakeup_enable = 0;
			break;

		case USB_DEVICE_TEST_MODE:
			/* IMG_SYNOP_CR */
			/* Potential enhancement: add CLEAR_FEATURE for TEST modes. */
			break;
		}
		do_setup_in_status_phase( psBlockDesc );
		break;

	case USB_RECIP_ENDPOINT:
		ep = get_ep_by_addr( &(psContext->sOtgCoreIf), pcd, ctrl.wIndex );
		if (ep == 0) 
		{
			ep0_do_stall( psBlockDesc, -USBD_ERR_OPNOTSUPP );
			return;
		} 

		pcd_clear_halt( psBlockDesc, ep );

		break;
	default:
		ep0_do_stall( psBlockDesc, -USBD_ERR_OPNOTSUPP );
		break;
	}

}

/**
 * This function process the SET_ADDRESS Setup Commands.
 */
static __USBD_INLINE__ IMG_VOID do_set_address( const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	USB_DC_T				*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_pcd_t			*	pcd			= &(psContext->sDevContext.sOtgPcd);
	struct usb_ctrlrequest_t	ctrl		= pcd->setup_pkt->req;

	if (ctrl.bRequestType == USB_RECIP_DEVICE) 
	{
		dcfg_data_t dcfg = {.d32=0};
					
#if defined DEBUG_EP0
//			DWC_DEBUGPL(DBG_PCDV, "SET_ADDRESS:%d\n", ctrl.wValue);
#endif
		dcfg.b.devaddr = ctrl.wValue;
		usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DCFG_OFFSET, 0, dcfg.d32);
		do_setup_in_status_phase( psBlockDesc );
	}
}

/**
 *	This function processes SETUP commands.	 In Linux, the USB Command
 *	processing is done in two places - the first being the PCD and the
 *	second in the Gadget Driver (for example, the File-Backed Storage
 *	Gadget Driver).
 *
 * <table>
 * <tr><td>Command	</td><td>Driver </td><td>Description</td></tr>
 *
 * <tr><td>GET_STATUS </td><td>PCD </td><td>Command is processed as
 * defined in chapter 9 of the USB 2.0 Specification chapter 9
 * </td></tr>
 *
 * <tr><td>CLEAR_FEATURE </td><td>PCD </td><td>The Device and Endpoint
 * requests are the ENDPOINT_HALT feature is procesed, all others the
 * interface requests are ignored.</td></tr>
 *
 * <tr><td>SET_FEATURE </td><td>PCD </td><td>The Device and Endpoint
 * requests are processed by the PCD.  Interface requests are passed
 * to the Gadget Driver.</td></tr>
 *
 * <tr><td>SET_ADDRESS </td><td>PCD </td><td>Program the DCFG reg,
 * with device address received </td></tr>
 *
 * <tr><td>GET_DESCRIPTOR </td><td>Gadget Driver </td><td>Return the
 * requested descriptor</td></tr>
 *
 * <tr><td>SET_DESCRIPTOR </td><td>Gadget Driver </td><td>Optional -
 * not implemented by any of the existing Gadget Drivers.</td></tr>
 *
 * <tr><td>SET_CONFIGURATION </td><td>Gadget Driver </td><td>Disable
 * all EPs and enable EPs for new configuration.</td></tr>
 *
 * <tr><td>GET_CONFIGURATION </td><td>Gadget Driver </td><td>Return
 * the current configuration</td></tr>
 *
 * <tr><td>SET_INTERFACE </td><td>Gadget Driver </td><td>Disable all
 * EPs and enable EPs for new configuration.</td></tr>
 *
 * <tr><td>GET_INTERFACE </td><td>Gadget Driver </td><td>Return the
 * current interface.</td></tr>
 *
 * <tr><td>SYNC_FRAME </td><td>PCD </td><td>Display debug
 * message.</td></tr> 
 * </table>
 *
 * When the SETUP Phase Done interrupt occurs, the PCD SETUP commands are
 * processed by pcd_setup. Calling the Function Driver's setup function from
 * pcd_setup processes the gadget SETUP commands.
 */
static __USBD_INLINE__ IMG_VOID pcd_setup( const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	USB_DC_T					*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_pcd_t				*	pcd			= &(psContext->sDevContext.sOtgPcd);
  #if !defined (USE_DDMA_ONLY)
	dwc_otg_core_if_t			*	core_if		= &(psContext->sOtgCoreIf);
  #endif
	struct usb_ctrlrequest_t		ctrl;
	dwc_otg_pcd_ep_t			*	ep0			= &pcd->ep0;
  #if !defined (USE_DDMA_ONLY)
	deptsiz0_data_t					doeptsize0	= {0};
  #endif

	if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
	{
		psBlockDesc->psSystemDescriptor->pfn_FlushCache( (IMG_UINT32)pcd->setup_pkt, 8 * 3 );
	}

	psContext->sDevContext.bLastControlRequestUnhandledByDriver = IMG_FALSE;
	ctrl = pcd->setup_pkt->req;
	

  #if defined DEBUG_EP0
	DWC_DEBUGPL(DBG_PCD, "SETUP %02x.%02x v%04x i%04x l%04x\n", ctrl.bRequestType, ctrl.bRequest, ctrl.wValue, ctrl.wIndex, ctrl.wLength);
  #endif

  #if !defined (USE_DDMA_ONLY)
	doeptsize0.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0);
	/* IMG_SYNOP_CR */
	/** Potential enhancement: handle > 1 setup packet , assert error for now */
  #if !defined USE_DMA_INTERNAL
	if (core_if->dma_enable && !core_if->dma_desc_enable && (doeptsize0.b.supcnt < 2))
  #else
	if (!core_if->dma_desc_enable && doeptsize0.b.supcnt < 2) 
  #endif /* USE_DMA_INTERNAL */
	{
		DWC_ERROR ("\n\n-----------	 CANNOT handle > 1 setup packet in DMA mode\n\n");
	}
  #endif

	/* Clean up the request queue */
	dwc_otg_pcd_ep_request_nuke( psBlockDesc, ep0 );
	ep0->stopped = 0;

	if (ctrl.bRequestType & USB_DIR_IN) 
	{		 
		ep0->dwc_ep.is_in = 1;
		pcd->ep0state = EP0_IN_DATA_PHASE;
	} 
	else 
	{
		ep0->dwc_ep.is_in = 0;
		pcd->ep0state = EP0_OUT_DATA_PHASE;
	}

	if(ctrl.wLength == 0)
	{
		ep0->dwc_ep.is_in = 1;
		pcd->ep0state = EP0_IN_STATUS_PHASE;
	}

	if ((ctrl.bRequestType & USB_TYPE_MASK) != USB_TYPE_STANDARD) 
	{
		/* handle non-standard (class/vendor) requests in the gadget driver */
		do_gadget_setup( psBlockDesc, &ctrl );
		return;
	}

	/* IMG_SYNOP_CR */
	/* Potential enhancement: handle bad setup packet */
	
	///////////////////////////////////////////
	//// --- Standard Request handling --- ////
	
	switch (ctrl.bRequest) 
	{
		case USB_REQ_GET_STATUS:
			do_get_status( psBlockDesc );
			break;

		case USB_REQ_CLEAR_FEATURE:
			do_clear_feature( psBlockDesc );
			break;

		case USB_REQ_SET_FEATURE:
			do_set_feature( psBlockDesc );
			break;
				
		case USB_REQ_SET_ADDRESS:
			do_set_address( psBlockDesc );
			break;

		case USB_REQ_SET_INTERFACE:
		case USB_REQ_SET_CONFIGURATION:
//			_pcd->request_config = 1;	/* Configuration changed */
			do_gadget_setup( psBlockDesc, &ctrl );
			break;
				
		case USB_REQ_SYNCH_FRAME:
			do_gadget_setup( psBlockDesc, &ctrl );
			break;

		default:
			/* Call the Gadget Driver's setup functions */		  
			do_gadget_setup( psBlockDesc, &ctrl );
			break;
	}
}

/**
 * This function completes the ep0 control transfer.
 */
static IMG_INT32 ep0_complete_request( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep )
{
	USB_DC_T				*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_core_if_t		*	core_if		= &(psContext->sOtgCoreIf);
	dwc_otg_dev_if_t		*	dev_if		= &(core_if->dev_if);
	dwc_otg_pcd_t			*	pcd			= &(psContext->sDevContext.sOtgPcd);
  #if !defined (USE_DDMA_ONLY)
	deptsiz0_data_t				deptsiz;
  #endif
	dev_dma_desc_sts_t 			desc_sts;
	dwc_otg_pcd_request_t	*	req;
	IMG_INT32					is_last		= 0;	

	#if !defined (USBD_NO_CBMAN_SUPPORT)
		if ( psContext->sDevContext.bLastControlRequestUnhandledByDriver == IMG_TRUE )
		{
			/* Call completion callback */
			CBMAN_ExecuteCallbackWithEventType (	psContext->sDevContext.ui32CallbackSlot,
													USBD_EVENT_UNHANDLED_CONTROL_MESSAGE_COMPLETE,
													0,
													IMG_NULL	);											
		}
	#endif
		
	if (pcd->ep0_pending && LST_empty(&ep->sRequestList))
	{
		if (ep->dwc_ep.is_in) 
		{
		  #if defined DEBUG_EP0
			DWC_DEBUGPL(DBG_PCDV, "Do setup OUT status phase\n");
		  #endif
			do_setup_out_status_phase( psBlockDesc );
		} 
		else 
		{
		  #if defined DEBUG_EP0
			DWC_DEBUGPL(DBG_PCDV, "Do setup IN status phase\n");
		  #endif
			do_setup_in_status_phase( psBlockDesc );
		}
		pcd->ep0_pending = 0;
		return 1;
	}

	if (LST_empty(&ep->sRequestList))
	{
		return 0;
	}
	req = (dwc_otg_pcd_request_t *) LST_first(&ep->sRequestList);

 
	if (pcd->ep0state == EP0_OUT_STATUS_PHASE || pcd->ep0state == EP0_IN_STATUS_PHASE) 
	{
		is_last = 1;						
	}
	else if (ep->dwc_ep.is_in) 
	{
	  #if !defined (USE_DDMA_ONLY)
		deptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0);
	  #endif
		if(core_if->dma_desc_enable != 0)
		{
			if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
			{
				psBlockDesc->psSystemDescriptor->pfn_FlushCache( (IMG_UINT32)dev_if->in_desc_addr, sizeof( dwc_otg_dev_dma_desc_t ) );
			}
			desc_sts = dev_if->in_desc_addr->status;
		}
	  #if defined DEBUG_EP0
		DWC_DEBUGPL(DBG_PCDV, "%s len=%d  xfersize=%d pktcnt=%d\n", 
						ep->ep.name, ep->dwc_ep.xfer_len,
						deptsiz.b.xfersize, deptsiz.b.pktcnt);
	  #endif
	  #if defined (USE_DDMA_ONLY)
	    if (desc_sts.b.bytes == 0)
	  #else
		if (((core_if->dma_desc_enable == 0) && (deptsiz.b.xfersize == 0)) || 
			((core_if->dma_desc_enable != 0) && (desc_sts.b.bytes == 0)))
	  #endif
		{
			req->actual = ep->dwc_ep.xfer_count;
			/* Is a Zero Len Packet needed? */
			if (req->zero) 
			{
		  #if defined DEBUG_EP0
				DWC_DEBUGPL(DBG_PCD, "Setup Rx ZLP\n");
		  #endif
				req->zero = 0;
			}
			do_setup_out_status_phase( psBlockDesc );
		}
	} 
	else 
	{
		/* ep0-OUT */
	  #if defined DEBUG_EP0
		deptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0);
		DWC_DEBUGPL(DBG_PCDV, "%s len=%d xsize=%d pktcnt=%d\n",
						ep->ep.name, ep->dwc_ep.xfer_len,
						deptsiz.b.xfersize, 
						deptsiz.b.pktcnt);
	  #endif
		req->actual = ep->dwc_ep.xfer_count;
		
		/* Is a Zero Len Packet needed? */
		if (req->zero) 
		{
	  #if defined DEBUG_EP0
			DWC_DEBUGPL(DBG_PCDV, "Setup Tx ZLP\n");
	  #endif
			req->zero = 0;
		}
	  #if !defined (USE_DDMA_ONLY)
		if(core_if->dma_desc_enable == 0)
		{
			do_setup_in_status_phase( psBlockDesc );
		}
	  #endif /* !USE_DDMA_ONLY */
	}
		
	/* Complete the request */
	if (is_last) 
	{
		request_done( psBlockDesc, ep, req, 0 );
		ep->dwc_ep.start_xfer_buff = 0;
		ep->dwc_ep.xfer_buff = 0;
		ep->dwc_ep.xfer_len = 0;
		return 1;
	}
	return 0;		
}

/**
 * This function completes the request for the EP.	If there are
 * additional requests for the EP in the queue they will be started.
 */
static IMG_VOID complete_ep( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep )
{
  #if !defined (USE_DDMA_ONLY) || !defined (USE_DMA_INTERNAL) 
	dwc_otg_core_if_t		*	core_if		= &(psContext->sOtgCoreIf);
	USB_DC_T				*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
  #endif
  #if !defined (USE_DDMA_ONLY)
	deptsiz_data_t				deptsiz;
  #endif
	dev_dma_desc_sts_t 			desc_sts;
	dwc_otg_pcd_request_t	*	req			= 0;
	dwc_otg_dev_dma_desc_t	*	dma_desc;
	IMG_UINT32					byte_count	= 0;
	IMG_INT32					is_last		= 0;

	
	DWC_DEBUGPL(DBG_PCDV,"%s() %s-%s\n", __func__, ep->ep.name, (_ep->dwc_ep.is_in?"IN":"OUT"));

	/* Get any pending requests */
	req = (dwc_otg_pcd_request_t *) LST_first(&ep->sRequestList);
	DWC_DEBUGPL(DBG_PCD, "Requests %d\n",ep->request_pending);

	if (ep->dwc_ep.is_in) 
	{
	  #if !defined (USE_DDMA_ONLY)
		deptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + ep->dwc_ep.num * DWC_EP_REG_OFFSET);
	  #endif
	  #if !defined USE_DMA_INTERNAL
		if (core_if->dma_enable) 
	  #endif /* USE_DMA_INTERNAL */
	  #if !defined (USE_DDMA_ONLY)
		{
			if (core_if->dma_desc_enable == 0)
			{
				if (deptsiz.b.xfersize == 0 &&
					deptsiz.b.pktcnt == 0)
				{
					byte_count = ep->dwc_ep.xfer_len - ep->dwc_ep.xfer_count;
					
					ep->dwc_ep.xfer_buff += byte_count;
					ep->dwc_ep.dma_addr += byte_count;	
					ep->dwc_ep.xfer_count += byte_count;

					DWC_DEBUGPL(DBG_PCDV, "%d-%s len=%d  xfersize=%d pktcnt=%d\n", 
						ep->dwc_ep.num, ep->ep.name, ep->dwc_ep.xfer_len,
						deptsiz.b.xfersize, deptsiz.b.pktcnt);
	
					if (ep->dwc_ep.xfer_len < ep->dwc_ep.total_len)
					{
						dwc_otg_ep_start_transfer( psBlockDesc, core_if, &ep->dwc_ep );
					}
					else if ( ep->dwc_ep.sent_zlp )
					{
						/*      
							 * This fragment of code should initiate 0 
							 * length trasfer in case if it is queued
							 * a trasfer with size divisible to EPs max 
							 * packet size and with usb_request zero field 
							 * is set, which means that after data is transfered, 
							 * it is also should be transfered 
							 * a 0 length packet at the end. For Slave and 
							 * Buffer DMA modes in this case SW has 
							 * to initiate 2 transfers one with transfer size, 
							 * and the second with 0 size. For Desriptor 
							 * DMA mode SW is able to initiate a transfer, 
							 * which will handle all the packets including 
							 * the last  0 legth.
							 */
						ep->dwc_ep.sent_zlp = 0;
						dwc_otg_ep_start_zl_transfer( psBlockDesc->ui32Base, core_if, &ep->dwc_ep );
					}
					else 
					{
						is_last = 1;
					}
				}
				else 
				{
					DWC_WARN("Incomplete transfer (%s-%s [siz=%d pkt=%d])\n", 
							 ep->ep.name, (ep->dwc_ep.is_in?"IN":"OUT"),
							 deptsiz.b.xfersize, deptsiz.b.pktcnt);
				}
			}
			else // DMA descriptor
	  #endif /* !USE_DDMA_ONLY */
			{
				IMG_INT i;
				dma_desc = ep->dwc_ep.desc_addr;
				if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
				{
					psBlockDesc->psSystemDescriptor->pfn_FlushCache( (IMG_UINT32)dma_desc, sizeof( dwc_otg_dev_dma_desc_t ) * ep->dwc_ep.desc_cnt );
				}
				byte_count = 0;
				ep->dwc_ep.sent_zlp = 0;
				for ( i = 0; i < ep->dwc_ep.desc_cnt; ++i )
				{
					desc_sts = dma_desc->status;
					byte_count += desc_sts.b.bytes;
					dma_desc++;
				}

				if (byte_count == 0) 
				{
					ep->dwc_ep.xfer_count = ep->dwc_ep.total_len;
					is_last = 1;
				}
				else
				{
					DWC_WARN("Incomplete transfer\n");
				}
			}
	  #if !defined (USE_DDMA_ONLY)
		}
	  #endif /* !USE_DDMA_ONLY */
	  #if !defined USE_DMA_INTERNAL
		else // Non DMA
		{
			if (deptsiz.b.xfersize == 0 && deptsiz.b.pktcnt == 0)
			{
				DWC_DEBUGPL(DBG_PCDV, "%d-%s len=%d  xfersize=%d pktcnt=%d\n", 
					ep->dwc_ep.num,
					ep->ep.name, ep->dwc_ep.xfer_len,
					deptsiz.b.xfersize, deptsiz.b.pktcnt);

				/*	Check if the whole transfer was completed,
				 *	if no, setup transfer for next portion of data
				 */
				if ( ep->dwc_ep.xfer_len < ep->dwc_ep.total_len )
				{
					dwc_otg_ep_start_transfer( psBlockDesc, &ep->dwc_ep );
				}
				else if (ep->dwc_ep.sent_zlp) 
				{
					/*      
					 * This fragment of code should initiate 0 
					 * length trasfer in case if it is queued
					 * a trasfer with size divisible to EPs max 
					 * packet size and with usb_request zero field 
					 * is set, which means that after data is transfered, 
					 * it is also should be transfered 
					 * a 0 length packet at the end. For Slave and 
					 * Buffer DMA modes in this case SW has 
					 * to initiate 2 transfers one with transfer size, 
					 * and the second with 0 size. For Desriptor 
					 * DMA mode SW is able to initiate a transfer, 
					 * which will handle all the packets including 
					 * the last  0 legth.
					 */
					ep->dwc_ep.sent_zlp = 0;
					dwc_otg_ep_start_zl_transfer(psContext, &ep->dwc_ep);
				} 
				else 
				{
					is_last = 1;
				}
			}
			else 
			{
				DWC_WARN("Incomplete transfer (-%s [siz=%d pkt=%d])\n", 
						 (_ep->dwc_ep.is_in?"IN":"OUT"),
						 deptsiz.b.xfersize, deptsiz.b.pktcnt);
			}
		} 
	  #endif /* USE_DMA_INTERNAL */
	}
	else  // Out ep
	{
		desc_sts.d32 = 0;
	  #if !defined USE_DMA_INTERNAL
		if ( core_if->dma_enable )
	  #endif /* USE_DMA_INTERNAL */
	  #if !defined (USE_DDMA_ONLY)
		{
			if ( core_if->dma_desc_enable )
	  #endif /* !USE_DDMA_ONLY */
			{
				IMG_INT32 i;
				dma_desc = ep->dwc_ep.desc_addr;
				if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
				{
					psBlockDesc->psSystemDescriptor->pfn_FlushCache( (IMG_UINT32)dma_desc, sizeof( dwc_otg_dev_dma_desc_t ) * ep->dwc_ep.desc_cnt );
				}
				byte_count = 0;
				ep->dwc_ep.sent_zlp = 0;
				for ( i = 0; i < ep->dwc_ep.desc_cnt; ++i )
				{
					desc_sts = dma_desc->status;
					byte_count += desc_sts.b.bytes;
					dma_desc++;
				}
				
				ep->dwc_ep.xfer_count = ep->dwc_ep.total_len - byte_count + ((4 - (ep->dwc_ep.total_len & 0x3)) & 0x3);
				is_last = 1;
			}
	  #if !defined (USE_DDMA_ONLY)
			else
			{
				deptsiz.d32 = 0;
				deptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + ep->dwc_ep.num*DWC_EP_REG_OFFSET);		

				byte_count = (ep->dwc_ep.xfer_len - ep->dwc_ep.xfer_count - deptsiz.b.xfersize);
				ep->dwc_ep.xfer_buff += byte_count;
				ep->dwc_ep.dma_addr += byte_count;
				ep->dwc_ep.xfer_count += byte_count;

				/*      Check if the whole transfer was completed,  
				 *      if no, setup transfer for next portion of data
				 */
				if (ep->dwc_ep.xfer_len < ep->dwc_ep.total_len) 
				{
					dwc_otg_ep_start_transfer( psBlockDesc, core_if, &ep->dwc_ep );
				} 
				else if (ep->dwc_ep.sent_zlp) 
				{
					/*      
					 * This fragment of code should initiate 0 
					 * length trasfer in case if it is queued
					 * a trasfer with size divisible to EPs max 
					 * packet size and with usb_request zero field 
					 * is set, which means that after data is transfered, 
					 * it is also should be transfered 
					 * a 0 length packet at the end. For Slave and 
					 * Buffer DMA modes in this case SW has 
					 * to initiate 2 transfers one with transfer size, 
					 * and the second with 0 size. For Desriptor 
					 * DMA mode SW is able to initiate a transfer, 
					 * which will handle all the packets including 
					 * the last  0 legth.
					 */
					ep->dwc_ep.sent_zlp = 0;
					dwc_otg_ep_start_zl_transfer( psBlockDesc->ui32Base, core_if, &ep->dwc_ep);
				} 
				else 
				{
					is_last = 1;
				}
			}
		}
	  #endif /* USE_DDMA_ONLY */
	  #if !defined USE_DMA_INTERNAL
		else // non DMA
		{
			/*      Check if the whole transfer was completed,  
			 *      if no, setup transfer for next portion of data
			 */
			if (ep->dwc_ep.xfer_len < ep->dwc_ep.total_len) 
			{
				dwc_otg_ep_start_transfer( psBlockDesc, core_if, &ep->dwc_ep );
			} 
			else if (ep->dwc_ep.sent_zlp) 
			{
				/*      
				 * This fragment of code should initiate 0 
				 * length trasfer in case if it is queued
				 * a trasfer with size divisible to EPs max 
				 * packet size and with usb_request zero field 
				 * is set, which means that after data is transfered, 
				 * it is also should be transfered 
				 * a 0 length packet at the end. For Slave and 
				 * Buffer DMA modes in this case SW has 
				 * to initiate 2 transfers one with transfer size, 
				 * and the second with 0 size. For Desriptor 
				 * DMA mode SW is able to initiate a transfer, 
				 * which will handle all the packets including 
				 * the last  0 legth.
				 */
				ep->dwc_ep.sent_zlp = 0;
				dwc_otg_ep_start_zl_transfer( psBlockDesc->ui32Base, core_if, &ep->dwc_ep);
			} 
			else 
			{
				is_last = 1;
			}
		}
	  #endif /* USE_DMA_INTERNAL */

#if defined USBD_DEBUG
		DWC_DEBUGPL(DBG_PCDV, "addr %p,	 %s len=%d cnt=%d xsize=%d pktcnt=%d\n",
						DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + ep->dwc_ep.num*DWC_EP_REG_OFFSET, _ep->ep.name, _ep->dwc_ep.xfer_len,
						ep->dwc_ep.xfer_count,
						deptsiz.b.xfersize, 
						deptsiz.b.pktcnt);
#endif
	}
		
	/* Complete the request */
	if (is_last) 
	{
		req->actual = ep->dwc_ep.xfer_count;

		request_done( psBlockDesc, ep, req, 0 );

		ep->dwc_ep.start_xfer_buff = 0;
		ep->dwc_ep.xfer_buff = 0;
		ep->dwc_ep.xfer_len = 0;

		/* If there is a request in the queue start it.*/
		start_next_request( psBlockDesc, ep );
	}
}


#if defined _EN_ISOC_

/**
 * This function BNA interrupt for Isochronous EPs
 *
 */
static IMG_VOID dwc_otg_pcd_handle_iso_bna( const img_uint32 ui32BaseAddress, dwc_otg_pcd_ep_t * ep)
{
	dwc_ep_t				*	dwc_ep = &ep->dwc_ep;
	img_uint32					addr;
	depctl_data_t				depctl = { 0 };
	dwc_otg_dev_dma_desc_t	*	dma_desc;
	IMG_INT32 i;

	dma_desc = dwc_ep->iso_desc_addr + dwc_ep->desc_cnt * (dwc_ep->proc_buf_num);

	if (dwc_ep->is_in) 
	{
		dev_dma_desc_sts_t sts = { 0 };
		for (i = 0; i < dwc_ep->desc_cnt; ++i, ++dma_desc) 
		{
			sts.d32 = dma_desc->status.d32;
			sts.b_iso_in.bs = BS_HOST_READY;
			dma_desc->status.d32 = sts.d32;
		}
	} 
	else 
	{
		dev_dma_desc_sts_t sts = { 0 };
		for (i = 0; i < dwc_ep->desc_cnt; ++i, ++dma_desc) 
		{
			sts.d32 = dma_desc->status.d32;
			sts.b_iso_out.bs = BS_HOST_READY;
			dma_desc->status.d32 = sts.d32;
		}
	}

	if (dwc_ep->is_in == 0) 
	{
		addr = DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + dwc_ep->num * DWC_EP_REG_OFFSET;
	} 
	else 
	{
		addr = DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + dwc_ep->num * DWC_EP_REG_OFFSET;
	}
	depctl.b.epena = 1;
	usb_ModifyReg32( ui32BaseAddress, addr, depctl.d32, depctl.d32);
}

/**
 * This function sets latest iso packet information(non-PTI mode)
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP to start the transfer on.
 *
 */
IMG_VOID set_current_pkt_info( const img_uint32 ui32BaseAddress, dwc_otg_core_if_t * core_if, dwc_ep_t * ep)
{
	deptsiz_data_t	deptsiz = {.d32 = 0 };
	img_uint32		dma_addr;
	img_uint32		offset;

	if (ep->proc_buf_num)
	{
		dma_addr = ep->dma_addr1;
	}
	else
	{
		dma_addr = ep->dma_addr0;
	}

	if (ep->is_in) 
	{
		deptsiz.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + ep->num * DWC_EP_REG_OFFSET );
		offset = ep->data_per_frame;
	} 
	else 
	{
		deptsiz.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + ep->num * DWC_EP_REG_OFFSET );
		offset = ep->data_per_frame + (0x4 & (0x4 - (ep->data_per_frame & 0x3)));
	}

	if (!deptsiz.b.xfersize) 
	{
		ep->pkt_info[ep->cur_pkt].length = ep->data_per_frame;
		ep->pkt_info[ep->cur_pkt].offset = ep->cur_pkt_dma_addr - dma_addr;
		ep->pkt_info[ep->cur_pkt].status = 0;
	} 
	else 
	{
		ep->pkt_info[ep->cur_pkt].length = ep->data_per_frame;
		ep->pkt_info[ep->cur_pkt].offset = ep->cur_pkt_dma_addr - dma_addr;
		ep->pkt_info[ep->cur_pkt].status = -USBD_ERR_NODATA;
	}
	ep->cur_pkt_addr += offset;
	ep->cur_pkt_dma_addr += offset;
	ep->cur_pkt++;
}

/**
 * This function sets latest iso packet information(DDMA mode)
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param dwc_ep The EP to start the transfer on.
 *
 */
static IMG_VOID set_ddma_iso_pkts_info( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_core_if_t * core_if, dwc_ep_t * dwc_ep)
{
	dwc_otg_dev_dma_desc_t	*	dma_desc;
	dev_dma_desc_sts_t			sts = { 0 };
	iso_pkt_info_t			*	iso_packet;
	img_uint32					data_per_desc;
	img_uint32					offset;
	int							i, j;

	iso_packet = dwc_ep->pkt_info;

	/** Reinit closed DMA Descriptors*/
	/** ISO OUT EP */
	if (dwc_ep->is_in == 0)
	{
		dma_desc = dwc_ep->iso_desc_addr + dwc_ep->desc_cnt * dwc_ep->proc_buf_num;
		offset = 0;

		// Flush the cache if needed
		if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
		{
			psBlockDesc->psSystemDescriptor->pfn_FlushCache( (img_uint32)dma_desc, sizeof( dwc_otg_dev_dma_desc_t ) * dwc_ep->desc_cnt );
		}

		for (i = 0; i < dwc_ep->desc_cnt - dwc_ep->pkt_per_frm; i += dwc_ep->pkt_per_frm) 
		{
			for (j = 0; j < dwc_ep->pkt_per_frm; ++j) 
			{
				data_per_desc = ((j + 1) * dwc_ep->maxpacket > dwc_ep->data_per_frame) ? dwc_ep->data_per_frame - j * dwc_ep->maxpacket : dwc_ep->maxpacket;
				data_per_desc += (data_per_desc % 4) ? (4 - data_per_desc % 4) : 0;

				sts.d32 = dma_desc->status.d32;

				/* Write status in iso_packet_decsriptor  */
				iso_packet->status = sts.b_iso_out.rxsts + (sts.b_iso_out.bs ^ BS_DMA_DONE);
				if (iso_packet->status) 
				{
					iso_packet->status = -USBD_ERR_NODATA;
				}

				/* Received data length */
				if (!sts.b_iso_out.rxbytes) 
				{
					iso_packet->length = data_per_desc - sts.b_iso_out.rxbytes;
				} 
				else 
				{
					iso_packet->length = data_per_desc - sts.b_iso_out.rxbytes + (4 - dwc_ep->data_per_frame % 4);
				}

				iso_packet->offset = offset;

				offset += data_per_desc;
				dma_desc++;
				iso_packet++;
			}
		}

		for (j = 0; j < dwc_ep->pkt_per_frm - 1; ++j) 
		{
			data_per_desc = ((j + 1) * dwc_ep->maxpacket > dwc_ep->data_per_frame) ? dwc_ep->data_per_frame - j * dwc_ep->maxpacket : dwc_ep->maxpacket;
			data_per_desc += (data_per_desc % 4) ? (4 - data_per_desc % 4) : 0;

			sts.d32 = dma_desc->status.d32;

			/* Write status in iso_packet_decsriptor  */
			iso_packet->status = sts.b_iso_out.rxsts + (sts.b_iso_out.bs ^ BS_DMA_DONE);
			if (iso_packet->status) 
			{
				iso_packet->status = -USBD_ERR_NODATA;
			}

			/* Received data length */
			iso_packet->length = dwc_ep->data_per_frame - sts.b_iso_out.rxbytes;

			iso_packet->offset = offset;

			offset += data_per_desc;
			iso_packet++;
			dma_desc++;
		}

		sts.d32 = dma_desc->status.d32;

		/* Write status in iso_packet_decsriptor  */
		iso_packet->status = sts.b_iso_out.rxsts + (sts.b_iso_out.bs ^ BS_DMA_DONE);
		if (iso_packet->status) 
		{
			iso_packet->status = -USBD_ERR_NODATA;
		}
		/* Received data length */
		if (!sts.b_iso_out.rxbytes) 
		{
			iso_packet->length = dwc_ep->data_per_frame - sts.b_iso_out.rxbytes;
		} 
		else 
		{
			iso_packet->length = dwc_ep->data_per_frame - sts.b_iso_out.rxbytes + (4 - dwc_ep->data_per_frame % 4);
		}

		iso_packet->offset = offset;
	} 
	else 
	{
/** ISO IN EP */

		dma_desc = dwc_ep->iso_desc_addr + dwc_ep->desc_cnt * dwc_ep->proc_buf_num;

		// Flush the cache if needed
		if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
		{
			psBlockDesc->psSystemDescriptor->pfn_FlushCache( (img_uint32)dma_desc, sizeof( dwc_otg_dev_dma_desc_t ) * dwc_ep->desc_cnt );
		}

		for (i = 0; i < dwc_ep->desc_cnt - 1; i++) 
		{
			sts.d32 = dma_desc->status.d32;

			/* Write status in iso packet descriptor */
			iso_packet->status = sts.b_iso_in.txsts + (sts.b_iso_in.bs ^ BS_DMA_DONE);
			if (iso_packet->status != 0) 
			{
				iso_packet->status = -USBD_ERR_NODATA;
			}
			/* Bytes has been transfered */
			iso_packet->length = dwc_ep->data_per_frame - sts.b_iso_in.txbytes;

			dma_desc++;
			iso_packet++;
		}

		sts.d32 = dma_desc->status.d32;
		while (sts.b_iso_in.bs == BS_DMA_BUSY) 
		{
			sts.d32 = dma_desc->status.d32;
		}

		/* Write status in iso packet descriptor? do be done with ERROR codes */
		iso_packet->status = sts.b_iso_in.txsts + (sts.b_iso_in.bs ^ BS_DMA_DONE);
		if (iso_packet->status != 0) 
		{
			iso_packet->status = -USBD_ERR_NODATA;
		}

		/* Bytes has been transfered */
		iso_packet->length = dwc_ep->data_per_frame - sts.b_iso_in.txbytes;
	}
}

/**
 * This function reinitialize DMA Descriptors for Isochronous transfer
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param dwc_ep The EP to start the transfer on.
 *
 */
static IMG_VOID reinit_ddma_iso_xfer(dwc_otg_core_if_t * core_if, dwc_ep_t * dwc_ep)
{
	int							i, j;
	dwc_otg_dev_dma_desc_t	*	dma_desc;
	img_uint32					dma_ad;
	dev_dma_desc_sts_t			sts = { 0 };
	img_uint32					data_per_desc;

	if (dwc_ep->proc_buf_num == 0) 
	{
		/** Buffer 0 descriptors setup */
		dma_ad = dwc_ep->dma_addr0;
	} 
	else 
	{
		/** Buffer 1 descriptors setup */
		dma_ad = dwc_ep->dma_addr1;
	}

	/** Reinit closed DMA Descriptors*/
	/** ISO OUT EP */
	if (dwc_ep->is_in == 0) 
	{
		dma_desc = dwc_ep->iso_desc_addr + dwc_ep->desc_cnt * dwc_ep->proc_buf_num;

		sts.b_iso_out.bs = BS_HOST_READY;
		sts.b_iso_out.rxsts = 0;
		sts.b_iso_out.l = 0;
		sts.b_iso_out.sp = 0;
		sts.b_iso_out.ioc = 0;
		sts.b_iso_out.pid = 0;
		sts.b_iso_out.framenum = 0;

		for (i = 0; i < dwc_ep->desc_cnt - dwc_ep->pkt_per_frm; i += dwc_ep->pkt_per_frm) 
		{
			for (j = 0; j < dwc_ep->pkt_per_frm; ++j) 
			{
				data_per_desc = ((j + 1) * dwc_ep->maxpacket > dwc_ep->data_per_frame) ? dwc_ep->data_per_frame - j * dwc_ep->maxpacket : dwc_ep->maxpacket;
				data_per_desc += (data_per_desc % 4) ? (4 - data_per_desc % 4) : 0;
				sts.b_iso_out.rxbytes = data_per_desc;
				dma_desc->buf = dma_ad;
				dma_desc->status.d32 = sts.d32;

				dma_ad += data_per_desc;
				dma_desc++;
			}
		}

		for (j = 0; j < dwc_ep->pkt_per_frm - 1; ++j) 
		{

			data_per_desc = ((j + 1) * dwc_ep->maxpacket > dwc_ep->data_per_frame) ? dwc_ep->data_per_frame - j * dwc_ep->maxpacket : dwc_ep->maxpacket;
			data_per_desc += (data_per_desc % 4) ? (4 - data_per_desc % 4) : 0;
			sts.b_iso_out.rxbytes = data_per_desc;

			dma_desc->buf = dma_ad;
			dma_desc->status.d32 = sts.d32;

			dma_desc++;
			dma_ad += data_per_desc;
		}

		sts.b_iso_out.ioc = 1;
		sts.b_iso_out.l = dwc_ep->proc_buf_num;

		data_per_desc = ((j + 1) * dwc_ep->maxpacket > dwc_ep->data_per_frame) ? dwc_ep->data_per_frame - j * dwc_ep->maxpacket : dwc_ep->maxpacket;
		data_per_desc += (data_per_desc % 4) ? (4 - data_per_desc % 4) : 0;
		sts.b_iso_out.rxbytes = data_per_desc;

		dma_desc->buf = dma_ad;
		dma_desc->status.d32 = sts.d32;
	} 
	else 
	{
/** ISO IN EP */

		dma_desc = dwc_ep->iso_desc_addr + dwc_ep->desc_cnt * dwc_ep->proc_buf_num;

		sts.b_iso_in.bs = BS_HOST_READY;
		sts.b_iso_in.txsts = 0;
		sts.b_iso_in.sp = (dwc_ep->data_per_frame % dwc_ep->maxpacket) ? 1 : 0;
		sts.b_iso_in.ioc = 0;
		sts.b_iso_in.pid = dwc_ep->pkt_per_frm;
		sts.b_iso_in.framenum = dwc_ep->next_frame;
		sts.b_iso_in.txbytes = dwc_ep->data_per_frame;
		sts.b_iso_in.l = 0;

		for (i = 0; i < dwc_ep->desc_cnt - 1; i++) 
		{
			dma_desc->buf = dma_ad;
			dma_desc->status.d32 = sts.d32;

			sts.b_iso_in.framenum += dwc_ep->bInterval;
			dma_ad += dwc_ep->data_per_frame;
			dma_desc++;
		}

		sts.b_iso_in.ioc = 1;
		sts.b_iso_in.l = dwc_ep->proc_buf_num;

		dma_desc->buf = dma_ad;
		dma_desc->status.d32 = sts.d32;

		dwc_ep->next_frame = sts.b_iso_in.framenum + dwc_ep->bInterval * 1;
	}
	dwc_ep->proc_buf_num = (dwc_ep->proc_buf_num ^ 1) & 0x1;
}

/**
 * This function is to handle Iso EP transfer complete interrupt
 * in case Iso out packet was dropped
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param dwc_ep The EP for wihich transfer complete was asserted
 *
 */
static IMG_UINT32 handle_iso_out_pkt_dropped( const img_uint32 ui32BaseAddress, dwc_otg_core_if_t * core_if, dwc_ep_t * dwc_ep)
{
	img_uint32		dma_addr;
	img_uint32		drp_pkt;
	img_uint32		drp_pkt_cnt;
	deptsiz_data_t	deptsiz = { 0 };
	depctl_data_t	depctl = { 0 };
	int				i;

	deptsiz.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + dwc_ep->num * DWC_EP_REG_OFFSET );

	drp_pkt = dwc_ep->pkt_cnt - deptsiz.b.pktcnt;
	drp_pkt_cnt = dwc_ep->pkt_per_frm - (drp_pkt % dwc_ep->pkt_per_frm);

	/* Setting dropped packets status */
	for (i = 0; i < drp_pkt_cnt; ++i) 
	{
		dwc_ep->pkt_info[drp_pkt].status = -USBD_ERR_NODATA;
		drp_pkt++;
		deptsiz.b.pktcnt--;
	}

	if (deptsiz.b.pktcnt > 0) 
	{
		deptsiz.b.xfersize = dwc_ep->xfer_len - (dwc_ep->pkt_cnt - deptsiz.b.pktcnt) * dwc_ep->maxpacket;
	} 
	else 
	{
		deptsiz.b.xfersize = 0;
		deptsiz.b.pktcnt = 0;
	}

	usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + dwc_ep->num * DWC_EP_REG_OFFSET, deptsiz.d32);

	if (deptsiz.b.pktcnt > 0) 
	{
		if (dwc_ep->proc_buf_num) 
		{
			dma_addr = dwc_ep->dma_addr1 + dwc_ep->xfer_len - deptsiz.b.xfersize;
		} 
		else 
		{
			dma_addr = dwc_ep->dma_addr0 + dwc_ep->xfer_len - deptsiz.b.xfersize;;
		}

		usb_WriteMemrefReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0 + dwc_ep->num * DWC_EP_REG_OFFSET, dma_addr );

		/** Re-enable endpoint, clear nak  */
		depctl.d32 = 0;
		depctl.b.epena = 1;
		depctl.b.cnak = 1;

		usb_ModifyReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + dwc_ep->num * DWC_EP_REG_OFFSET, depctl.d32, depctl.d32);
		return 0;
	} 
	else 
	{
		return 1;
	}
}

/**
 * This function sets iso packets information(PTI mode)
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP to start the transfer on.
 *
 */
static IMG_UINT32 set_iso_pkts_info( const img_uint32 ui32BaseAddress, dwc_otg_core_if_t * core_if, dwc_ep_t * ep)
{
	int					i, j;
	img_uint32			dma_ad;
	iso_pkt_info_t	*	packet_info = ep->pkt_info;
	img_uint32			offset;
	img_uint32			frame_data;
	deptsiz_data_t		deptsiz;

	if (ep->proc_buf_num == 0) 
	{
		/** Buffer 0 descriptors setup */
		dma_ad = ep->dma_addr0;
	} 
	else 
	{
		/** Buffer 1 descriptors setup */
		dma_ad = ep->dma_addr1;
	}

	if (ep->is_in) 
	{
		deptsiz.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + ep->num * DWC_EP_REG_OFFSET );
	} 
	else 
	{
		deptsiz.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + ep->num * DWC_EP_REG_OFFSET );
	}

	if (!deptsiz.b.xfersize) 
	{
		offset = 0;
		for (i = 0; i < ep->pkt_cnt; i += ep->pkt_per_frm) 
		{
			frame_data = ep->data_per_frame;
			for (j = 0; j < ep->pkt_per_frm; ++j) 
			{

				/* Packet status - is not set as initially 
				 * it is set to 0 and if packet was sent 
				 successfully, status field will remain 0*/

				/* Bytes has been transfered */
				packet_info->length = (ep->maxpacket < frame_data) ? ep->maxpacket : frame_data;

				/* Received packet offset */
				packet_info->offset = offset;
				offset += packet_info->length;
				frame_data -= packet_info->length;

				packet_info++;
			}
		}
		return 1;
	} 
	else 
	{
		/* This is a workaround for in case of Transfer Complete with 
		 * PktDrpSts interrupts merging - in this case Transfer complete 
		 * interrupt for Isoc Out Endpoint is asserted without PktDrpSts 
		 * set and with DOEPTSIZ register non zero. Investigations showed,
		 * that this happens when Out packet is dropped, but because of 
		 * interrupts merging during first interrupt handling PktDrpSts
		 * bit is cleared and for next merged interrupts it is not reset.
		 * In this case SW hadles the interrupt as if PktDrpSts bit is set.
		 */
		if (ep->is_in) 
		{
			return 1;
		} 
		else 
		{
			return handle_iso_out_pkt_dropped( ui32BaseAddress, core_if, ep);
		}
	}
}

/**
 * This function is to handle Iso EP transfer complete interrupt
 *
 * @param pcd The PCD
 * @param ep The EP for which transfer complete was asserted
 *
 */
static void complete_iso_ep( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t * ep)
{
	USB_DC_T			*	psContext = (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_core_if_t	*	core_if	= &(psContext->sOtgCoreIf);
	dwc_ep_t			*	dwc_ep	= &ep->dwc_ep;
	img_uint8				is_last = 0;

	if(ep->dwc_ep.next_frame == 0xffffffff) 
	{
		DWC_WARN("Next frame is not set!\n");
		return;
	}

  #if !defined USE_DMA_INTERNAL
	if (core_if->dma_enable)
  #endif /* USE_DMA_INTERNAL */
	{
		if (core_if->dma_desc_enable) 
		{
			set_ddma_iso_pkts_info( psBlockDesc, core_if, dwc_ep);
			reinit_ddma_iso_xfer(core_if, dwc_ep);
			is_last = 1;
		} 
		else 
		{
			if (core_if->pti_enh_enable) 
			{
				if (set_iso_pkts_info( psBlockDesc->ui32Base, core_if, dwc_ep)) 
				{
					dwc_ep->proc_buf_num = (dwc_ep->proc_buf_num ^ 1) & 0x1;
					dwc_otg_iso_ep_start_buf_transfer( psBlockDesc->ui32Base, core_if, dwc_ep);
					is_last = 1;
				}
			} 
			else 
			{
				set_current_pkt_info( psBlockDesc->ui32Base, core_if, dwc_ep);
				if (dwc_ep->cur_pkt >= dwc_ep->pkt_cnt) 
				{
					is_last = 1;
					dwc_ep->cur_pkt = 0;
					dwc_ep->proc_buf_num = (dwc_ep->proc_buf_num ^ 1) & 0x1;
					if (dwc_ep->proc_buf_num) 
					{
						dwc_ep->cur_pkt_addr = dwc_ep->xfer_buff1;
						dwc_ep->cur_pkt_dma_addr = dwc_ep->dma_addr1;
					} 
					else 
					{
						dwc_ep->cur_pkt_addr = dwc_ep->xfer_buff0;
						dwc_ep->cur_pkt_dma_addr = dwc_ep->dma_addr0;
					}

				}
				dwc_otg_iso_ep_start_frm_transfer( psBlockDesc->ui32Base, core_if, dwc_ep);
			}
		}
	} 
  #if !defined USE_DMA_INTERNAL
	else 
	{
		set_current_pkt_info( psBlockDesc->ui32Base, core_if, dwc_ep);
		if (dwc_ep->cur_pkt >= dwc_ep->pkt_cnt) 
		{
			is_last = 1;
			dwc_ep->cur_pkt = 0;
			dwc_ep->proc_buf_num = (dwc_ep->proc_buf_num ^ 1) & 0x1;
			if (dwc_ep->proc_buf_num) 
			{
				dwc_ep->cur_pkt_addr = dwc_ep->xfer_buff1;
				dwc_ep->cur_pkt_dma_addr = dwc_ep->dma_addr1;
			} 
			else 
			{
				dwc_ep->cur_pkt_addr = dwc_ep->xfer_buff0;
				dwc_ep->cur_pkt_dma_addr = dwc_ep->dma_addr0;
			}

		}
		dwc_otg_iso_ep_start_frm_transfer( psBlockDesc->ui32Base, core_if, dwc_ep);
	}
  #endif /* USE_DMA_INTERNAL */
	if (is_last)
	{
		dwc_otg_iso_buffer_done( psBlockDesc, ep, ep->iso_req );
	}
}
#endif				/* _EN_ISOC_ */

/**
 * This function handles EP0 Control transfers.	 
 *
 * The state of the control tranfers are tracked in
 * <code>ep0state</code>.
 */
static IMG_VOID handle_ep0(  const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	USB_DC_T			*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_pcd_t		*	pcd			= &(psContext->sDevContext.sOtgPcd);
	dwc_otg_core_if_t	*	core_if		= &(psContext->sOtgCoreIf);
	dwc_otg_pcd_ep_t	*	ep0			= &pcd->ep0;
	dev_dma_desc_sts_t 		desc_sts;
  #if !defined (USE_DDMA_ONLY)
	deptsiz0_data_t			deptsiz;
  #endif
	IMG_UINT32				byte_count;

#if defined DEBUG_EP0
	DWC_DEBUGPL(DBG_PCDV, "%s()\n", __func__);
	print_ep0_state( &(psContext->sDevContext) );
#endif

	switch (pcd->ep0state)
	{
		case EP0_DISCONNECT:
			break;
				
		case EP0_IDLE:
			pcd->request_config = 0;
				
			pcd_setup( psBlockDesc );
			break;

		case EP0_IN_DATA_PHASE:
		{
	  #if defined DEBUG_EP0
			DWC_DEBUGPL(DBG_PCD, "DATA_IN EP%d-%s: type=%d, mps=%d\n", 
						ep0->dwc_ep.num, (ep0->dwc_ep.is_in ?"IN":"OUT"),
						ep0->dwc_ep.type, ep0->dwc_ep.maxpacket );
	  #endif

	  	#if !defined USE_DMA_INTERNAL
			if (core_if->dma_enable)
	  	#endif /* USE_DMA_INTERNAL */
			{
				/* 
			 	* For EP0 we can only program 1 packet at a time so we
			 	* need to do the make calculations after each complete.
			 	* Call write_packet to make the calculations, as in
			 	* slave mode, and use those values to determine if we
			 	* can complete.
			 	*/
			  #if !defined (USE_DDMA_ONLY)
				if (core_if->dma_desc_enable == 0)
				{
					deptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 );
					byte_count = ep0->dwc_ep.xfer_len - deptsiz.b.xfersize;
				}
				else
				{
			  #endif /* !USE_DDMA_ONLY */
					if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
					{
						psBlockDesc->psSystemDescriptor->pfn_FlushCache( (IMG_UINT32)core_if->dev_if.in_desc_addr, sizeof( dwc_otg_dev_dma_desc_t ) );
					}
					desc_sts = core_if->dev_if.in_desc_addr->status;
					byte_count = ep0->dwc_ep.xfer_len - desc_sts.b.bytes;
			  #if !defined (USE_DDMA_ONLY)
				}
			  #endif /* !USE_DDMA_ONLY */
				ep0->dwc_ep.xfer_count += byte_count;
				ep0->dwc_ep.xfer_buff += byte_count;
				ep0->dwc_ep.dma_addr += byte_count;
			}

			if (ep0->dwc_ep.xfer_count < ep0->dwc_ep.total_len) 
			{
				dwc_otg_ep0_continue_transfer( psBlockDesc, core_if, &ep0->dwc_ep );
				DWC_DEBUGPL(DBG_PCD, "CONTINUE TRANSFER\n");
			}
			else if (ep0->dwc_ep.sent_zlp)
			{
				dwc_otg_ep0_continue_transfer( psBlockDesc, core_if, &ep0->dwc_ep );
				ep0->dwc_ep.sent_zlp = 0;
				DWC_DEBUGPL(DBG_PCD, "CONTINUE TRANSFER\n");
			}
			else 
			{		
				ep0_complete_request( psBlockDesc, ep0 );
				DWC_DEBUGPL(DBG_PCD, "COMPLETE TRANSFER\n"); 
			}
			break;
		}
		case EP0_OUT_DATA_PHASE:
		{
		  #if defined DEBUG_EP0
			DWC_DEBUGPL(DBG_PCD, "DATA_OUT EP%d-%s: type=%d, mps=%d\n", 
							ep0->dwc_ep.num, (ep0->dwc_ep.is_in ?"IN":"OUT"),
							ep0->dwc_ep.type, ep0->dwc_ep.maxpacket );
		  #endif
		  #if !defined USE_DMA_INTERNAL
			if (core_if->dma_enable != 0)
		  #endif /* USE_DMA_INTERNAL */
			{
			  #if !defined (USE_DDMA_ONLY)
				if (core_if->dma_desc_enable == 0)
				{
					deptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 );
					byte_count = ep0->dwc_ep.maxpacket - deptsiz.b.xfersize;
				}
				else
				{
			  #endif /* !USE_DDMA_ONLY */
					if ( psBlockDesc->psSystemDescriptor->pfn_FlushCache )
					{
						psBlockDesc->psSystemDescriptor->pfn_FlushCache( (IMG_UINT32)core_if->dev_if.out_desc_addr, sizeof( dwc_otg_dev_dma_desc_t ) );
					}
					desc_sts = core_if->dev_if.out_desc_addr->status;
					byte_count = ep0->dwc_ep.maxpacket - desc_sts.b.bytes;
			  #if !defined (USE_DDMA_ONLY)
				}
			  #endif /* USE_DDMA_ONLY */
				ep0->dwc_ep.xfer_count += byte_count;
				ep0->dwc_ep.xfer_buff += byte_count;
				ep0->dwc_ep.dma_addr += byte_count;
			}
			if (ep0->dwc_ep.xfer_count < ep0->dwc_ep.total_len) 
			{
				dwc_otg_ep0_continue_transfer( psBlockDesc, core_if, &ep0->dwc_ep );
				DWC_DEBUGPL(DBG_PCD, "CONTINUE TRANSFER\n"); 
			}
			else if(ep0->dwc_ep.sent_zlp) 
			{
				dwc_otg_ep0_continue_transfer( psBlockDesc, core_if, &ep0->dwc_ep );
				ep0->dwc_ep.sent_zlp = 0;
				DWC_DEBUGPL(DBG_PCD, "CONTINUE TRANSFER\n"); 
			}
			else 
			{		
				ep0_complete_request( psBlockDesc, ep0 );
				DWC_DEBUGPL(DBG_PCD, "COMPLETE TRANSFER\n"); 
			}	
			break;
				
		}
		case EP0_IN_STATUS_PHASE:
		case EP0_OUT_STATUS_PHASE:	
			DWC_DEBUGPL(DBG_PCD, "CASE: EP0_STATUS\n"); 
			ep0_complete_request( psBlockDesc, ep0 );
			pcd->ep0state = EP0_IDLE;
			ep0->stopped = 1;
			ep0->dwc_ep.is_in = 0;	/* OUT for next SETUP */
	
			/* Prepare for more SETUP Packets */
		  #if !defined USE_DMA_INTERNAL
			if (core_if->dma_enable) 
		  #endif /* USE_DMA_INTERNAL */
			{
				ep0_out_start( psBlockDesc );
			}
			break;
	
		case EP0_STALL:
			DWC_ERROR("EP0 STALLed, should not get here pcd_setup()\n");
			break;
		}
#if defined DEBUG_EP0
	print_ep0_state( psContext->psDevContext );
#endif
}

#if 0 // do not compile, as it is not used at the moment
/**
 * Restart transfer
 */
static IMG_VOID restart_transfer( USBD_DC_T	*	psContext, const IMG_UINT32 epnum)
{
	dwc_otg_core_if_t 	*core_if	= psContext->psOtgCoreIf;
	dwc_otg_dev_if_t	*dev_if		= core_if->dev_if;
	deptsiz_data_t		dieptsiz	= {0};
	desc_sts_data_t		desc_sts	= {0};
	dwc_otg_pcd_ep_t	*ep;

	ep = get_in_ep(epnum);

#if defined _EN_ISOC_
	if(ep->dwc_ep.type == DWC_OTG_EP_TYPE_ISOC) 
	{
		return;
	}
#endif /* _EN_ISOC_  */


	dieptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + epnum * DWC_EP_REG_OFFSET);
	if (core_if->dma_desc_enable != 0)
	{
		desc_sts.d32 = *(IMG_UINT32 *)ep->dwc_ep.desc_addr;
	}
	
  
	DWC_DEBUGPL(DBG_PCD,"xfer_buff=%p xfer_count=%0x xfer_len=%0x" " stopped=%d\n", ep->dwc_ep.xfer_buff, ep->dwc_ep.xfer_count, ep->dwc_ep.xfer_len, ep->stopped);
	/*
	 * If xfersize is 0 and pktcnt in not 0, resend the last packet.
	 */
	if (dieptsiz.b.pktcnt && dieptsiz.b.xfersize == 0 && ep->dwc_ep.start_xfer_buff != 0) 
	{
		if ( ep->dwc_ep.xfer_len <= ep->dwc_ep.maxpacket ) 
		{
			ep->dwc_ep.xfer_count = 0;
			ep->dwc_ep.xfer_buff = ep->dwc_ep.start_xfer_buff;
		} 
		else 
		{
			ep->dwc_ep.xfer_count -= ep->dwc_ep.maxpacket;
			/* convert packet size to dwords. */
			ep->dwc_ep.xfer_buff -= ep->dwc_ep.maxpacket;
		}
		ep->stopped = 0;
		DWC_DEBUGPL(DBG_PCD,"xfer_buff=%p xfer_count=%0x "
						"xfer_len=%0x stopped=%d\n", 
						ep->dwc_ep.xfer_buff,
						ep->dwc_ep.xfer_count, ep->dwc_ep.xfer_len ,
						ep->stopped
						);

		if (epnum == 0) 
		{
			dwc_otg_ep0_start_transfer(&ep->dwc_ep);
		} 
		else 
		{
			dwc_otg_ep_start_transfer(&ep->dwc_ep);
		}
	}
}
#endif

/**
 * handle the IN EP disable interrupt.
 */
static __USBD_INLINE__ IMG_VOID handle_in_ep_disable_intr( img_uint32 ui32BaseAddress, dwc_otg_core_if_t	*	core_if, dwc_otg_pcd_t * pcd, const IMG_UINT32 epnum )
{
//	deptsiz_data_t		dieptsiz	= {0};
//	dctl_data_t			dctl		= {0};
	dwc_otg_pcd_ep_t	*ep;
	dwc_ep_t			*dwc_ep;


	ep = get_in_ep( core_if, pcd, epnum );
	dwc_ep = &ep->dwc_ep;

#if defined DISABLE_IN_EP_FEATURE //bojan
	if (ep->disabling)
	{
		/* flush the fifo */
		dwc_otg_flush_tx_fifo( ui32BaseAddress, dwc_ep->tx_fifo_num );
		
		/* Deactivate the ep (turns off interrupts for the ep) */
		dwc_otg_ep_deactivate( ui32BaseAddress, core_if, dwc_ep );
		ep->desc = 0;
		ep->stopped = 1;

	  #if !defined (USE_MULTIPLE_TX_FIFO)
		release_perio_tx_fifo( core_if, dwc_ep->tx_fifo_num );
	  #endif
		release_tx_fifo( core_if, dwc_ep->tx_fifo_num );

		ep->disabling = 0;

		return;
	}
#endif

#if 0
	/* Bojan: I don't know exactly what this code is for... take it out for now */

	DWC_DEBUGPL(DBG_PCD,"diepctl%d=%0x\n", _epnum, usb_ReadReg32( psBlockDesc->ui32Base,  DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + _epnum * DWC_EP_REG_OFFSET));

	dieptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + _epnum * DWC_EP_REG_OFFSET);

	DWC_DEBUGPL(DBG_ANY, "pktcnt=%d size=%d\n", dieptsiz.b.pktcnt, dieptsiz.b.xfersize );


	if (ep->stopped) 
	{
		/* Flush the Tx FIFO */
		dwc_otg_flush_tx_fifo( dwc_ep->tx_fifo_num, psContext );
		/* Clear the Global IN NP NAK */
		dctl.d32 = 0;
		dctl.b.cgnpinnak = 1;
		usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, dctl.d32, 0);
		
		/* Restart the transaction */
		if (dieptsiz.b.pktcnt != 0 || dieptsiz.b.xfersize != 0) 
		{
			restart_transfer( epnum );
		}
	} 
	else 
	{
		/* Restart the transaction */
		if (dieptsiz.b.pktcnt != 0 || dieptsiz.b.xfersize != 0) 
		{
			restart_transfer( epnum );
		}
		DWC_DEBUGPL(DBG_ANY, "STOPPED!\n");
	}
#endif
}

/**
 * Handler for the IN EP timeout handshake interrupt. 
 */
static __USBD_INLINE__ IMG_VOID handle_in_ep_timeout_intr( img_uint32 ui32BaseAddress, dwc_otg_core_if_t	*	core_if, dwc_otg_pcd_t * pcd, const IMG_UINT32 epnum )
{
#if defined USBD_DEBUG
	deptsiz_data_t		dieptsiz	= {0};
	IMG_UINT32			epnum		= 0;
#endif
	dctl_data_t			dctl		= {0};
	dwc_otg_pcd_ep_t	*ep;
	gintmsk_data_t		intr_mask	= {0};

	ep = get_in_ep( core_if, pcd, epnum );	

#if !defined USE_DMA_INTERNAL
	/* Disable the NP Tx Fifo Empty Interrrupt */
	if (!core_if->dma_enable) 
	{
		intr_mask.b.nptxfempty = 1;
		usb_ModifyReg32( ui32BaseAddress,  DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, 0);
	}
#endif /* USE_DMA_INTERNAL */

	/* IMG_SYNOP_CR */
	/* Potential enhancement: check EP type.
	 * Implement for Periodic EPs */
	/* 
	 * Non-periodic EP 
	 */
	/* Enable the Global IN NAK Effective Interrupt */
	intr_mask.b.ginnakeff = 1;
	usb_ModifyReg32( ui32BaseAddress,  DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, 0, intr_mask.d32);

	/* Set Global IN NAK */
	dctl.b.sgnpinnak = 1;
	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, dctl.d32, dctl.d32);

	ep->stopped = 1;

#if defined USBD_DEBUG
	dieptsiz.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + epnum * DWC_EP_REG_OFFSET);
	DWC_DEBUGPL(DBG_ANY, "pktcnt=%d size=%d\n", dieptsiz.b.pktcnt, dieptsiz.b.xfersize );
#endif

#if defined DISABLE_PERIODIC_EP
	/*
	 * Set the NAK bit for this EP to
	 * start the disable process.
	 */
	diepctl.d32 = 0;
	diepctl.b.snak = 1;
	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + epnum * DWC_EP_REG_OFFSET, diepctl.d32, diepctl.d32);
	ep->disabling = 1;
	ep->stopped = 1;
#endif
}

/**
 * Handler for the IN EP NAK interrupt. 
 */
static __USBD_INLINE__ IMG_UINT32 handle_in_ep_nak_intr( img_uint32	ui32BaseAddress,  const IMG_UINT32 epnum)
{
	/* IMG_SYNOP_CR */
	diepmsk_data_t intr_mask = { 0 };

	DWC_PRINT("INTERRUPT Handler not implemented for %s\n", "IN EP NAK");
	intr_mask.b.nak = 1;

	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DIEPMSK_OFFSET, intr_mask.d32, 0);

	return 1;
}

/**
 * Handler for the OUT EP Babble interrupt. 
 */
static __USBD_INLINE__ IMG_UINT32 handle_out_ep_babble_intr( img_uint32 ui32BaseAddress,	const IMG_UINT32 epnum)
{
	/* IMG_SYNOP_CR */
	doepmsk_data_t intr_mask = { 0 };

	DWC_PRINT("INTERRUPT Handler not implemented for %s\n",
		   "OUT EP Babble");
	intr_mask.b.babble = 1;

	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DOEPMSK_OFFSET, intr_mask.d32, 0);

	return 1;
}

/**
 * Handler for the OUT EP NAK interrupt. 
 */
static __USBD_INLINE__ IMG_INT32 handle_out_ep_nak_intr( img_uint32 ui32BaseAddress, const IMG_UINT32 epnum)
{
	/* IMG_SYNOP_CR */
	doepmsk_data_t intr_mask = {.d32 = 0 };

	DWC_PRINT("INTERRUPT Handler not implemented for %s\n", "OUT EP NAK");
	intr_mask.b.nak = 1;

	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DOEPMSK_OFFSET, intr_mask.d32, 0);
	
	return 1;
}

/**
 * Handler for the OUT EP NYET interrupt. 
 */
static __USBD_INLINE__ IMG_INT32 handle_out_ep_nyet_intr( img_uint32 ui32BaseAddress, const IMG_UINT32 epnum)
{
	/* IMG_SYNOP_CR */
	doepmsk_data_t intr_mask = {.d32 = 0 };

	DWC_PRINT("INTERRUPT Handler not implemented for %s\n", "OUT EP NYET");
	intr_mask.b.nyet = 1;

	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DOEPMSK_OFFSET, intr_mask.d32, 0);

	return 1;
}

/**
 * This interrupt indicates that an IN EP has a pending Interrupt.
 * The sequence for handling the IN EP interrupt is shown below:
 * -#	Read the Device All Endpoint Interrupt register
 * -#	Repeat the following for each IN EP interrupt bit set (from
 *		LSB to MSB).
 * -#	Read the Device Endpoint Interrupt (DIEPINTn) register
 * -#	If "Transfer Complete" call the request complete function
 * -#	If "Endpoint Disabled" complete the EP disable procedure.
 * -#	If "AHB Error Interrupt" log error
 * -#	If "Time-out Handshake" log error
 * -#	If "IN Token Received when TxFIFO Empty" write packet to Tx
 *		FIFO.
 * -#	If "IN Token EP Mismatch" (disable, this is handled by EP
 *		Mismatch Interrupt)
 */
static IMG_INT32 dwc_otg_pcd_handle_in_ep_intr(  const ioblock_sBlockDescriptor	*	psBlockDesc )
{
#define CLEAR_IN_EP_INTR(__epnum,__intr) \
do { \
		diepint_data_t diepint = {0}; \
		diepint.b.__intr = 1; \
		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPINT0 + __epnum*DWC_EP_REG_OFFSET, diepint.d32); \
} while (0)
	USB_DC_T			*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_core_if_t	*	core_if		= &(psContext->sOtgCoreIf);
	dwc_otg_pcd_t		*	pcd			= &(psContext->sDevContext.sOtgPcd);
	diepint_data_t			diepint		= {0};
	dctl_data_t				dctl 		= {0};
	depctl_data_t			depctl		= {0};
	IMG_UINT32				ep_intr;
	IMG_UINT32				epnum		= 0;
	dwc_otg_pcd_ep_t	*	ep;
	dwc_ep_t			*	dwc_ep;
	
	DWC_DEBUGPL(DBG_PCDV, "%s(%p)\n", __func__, pcd);

	/* Read in the device interrupt bits */
	ep_intr = dwc_otg_read_dev_all_in_ep_intr( psBlockDesc->ui32Base );

	/* Service the Device IN interrupts for each endpoint */
	while( ep_intr ) 
	{
		if (ep_intr&0x1) 
		{	
			/* Get EP pointer */	   
			ep = get_in_ep( core_if, pcd, epnum );
			dwc_ep = &ep->dwc_ep;

			diepint.d32 = dwc_otg_read_dev_in_ep_intr( psBlockDesc->ui32Base, dwc_ep );
										   
			DWC_DEBUGPL(DBG_PCDV, "EP %d Interrupt Register - 0x%x\n", epnum, diepint.d32);
			
			/* Transfer complete */
			if ( diepint.b.xfercompl ) 
			{
				DWC_DEBUGPL(DBG_PCD,"EP%d IN Xfer Complete\n", epnum);
				
				/* Disable the NP Tx FIFO Empty
				 * Interrrupt */
			  #if !defined USE_MULTIPLE_TX_FIFO
				if(core_if->en_multiple_tx_fifo == 0)
				{
					gintmsk_data_t			intr_mask	= {0};
					intr_mask.b.nptxfempty = 1;
					usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, 0);
				}
				else
			  #endif
				{						
					/* Disable the Tx FIFO Empty Interrupt for this EP */
					IMG_UINT32 fifoemptymsk = 0x1 << dwc_ep->num;
					usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DTKNQR4_FIFOEMPTYMSK_OFFSET, fifoemptymsk, 0);
				}
				/* Clear the bit in DIEPINTn for this interrupt */
				CLEAR_IN_EP_INTR(epnum,xfercompl);

				/* Complete the transfer */
				if (epnum == 0) 
				{
					handle_ep0( psBlockDesc );
				}
			#if defined _EN_ISOC_
				else if (dwc_ep->type == DWC_OTG_EP_TYPE_ISOC)
				{
					if (!ep->stopped)
					{
						complete_iso_ep( psBlockDesc, ep );
					}
				}	
			#endif // _EN_ISOC_
			#if defined (_PER_IO_)
				else if (dwc_ep->type == DWC_OTG_EP_TYPE_ISOC)
				{
					if (!ep->stopped)
					{
						complete_xiso_ep( psBlockDesc, ep );
					}
				}
			#endif
				else 
				{
					complete_ep( psBlockDesc, ep );
				}
			}
			/* Endpoint disable	 */
			if ( diepint.b.epdisabled ) 
			{
				DWC_DEBUGPL(DBG_ANY,"EP%d IN disabled\n", epnum);
				handle_in_ep_disable_intr( psBlockDesc->ui32Base, core_if, pcd, epnum );

				/* Clear the bit in DIEPINTn for this interrupt */
				CLEAR_IN_EP_INTR(epnum,epdisabled);
			}
			/* AHB Error */
			if ( diepint.b.ahberr ) 
			{
				DWC_DEBUGPL(DBG_ANY,"EP%d IN AHB Error\n", epnum);
				/* Clear the bit in DIEPINTn for this interrupt */
				CLEAR_IN_EP_INTR(epnum,ahberr);
			}
			/* TimeOUT Handshake (non-ISOC IN EPs) */
			if ( diepint.b.timeout ) 
			{ 
				DWC_DEBUGPL(DBG_ANY,"EP%d IN Time-out\n", epnum);
				handle_in_ep_timeout_intr( psBlockDesc->ui32Base, core_if, pcd, epnum );

				CLEAR_IN_EP_INTR(epnum,timeout);
			}
			/** IN Token received with TxF Empty */
			if (diepint.b.intktxfemp)
			{
				DWC_DEBUGPL(DBG_ANY,"EP%d IN TKN TxFifo Empty\n", epnum);
				if (!ep->stopped && epnum != 0) 
				{
					diepmsk_data_t diepmsk = {0};
					diepmsk.b.intktxfemp = 1;
					usb_ModifyReg32( psBlockDesc->ui32Base,  DWC_OTG_DEV_GLOBAL_REGS_DIEPMSK_OFFSET, diepmsk.d32, 0 );		 
				}
				else if(core_if->dma_desc_enable && epnum == 0 && pcd->ep0state == EP0_OUT_STATUS_PHASE)
				{
					// EP0 IN set STALL
					depctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + epnum * DWC_EP_REG_OFFSET);

					/* set the disable and stall bits */
					if (depctl.b.epena)
					{
						depctl.b.epdis = 1;
					}
					depctl.b.stall = 1;
					usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + epnum * DWC_EP_REG_OFFSET, depctl.d32);
				}
				CLEAR_IN_EP_INTR(epnum,intktxfemp);
			}					
			/** IN Token Received with EP mismatch */
			if (diepint.b.intknepmis)
			{
				DWC_DEBUGPL(DBG_ANY,"EP%d IN TKN EP Mismatch\n", epnum);
				CLEAR_IN_EP_INTR(epnum,intknepmis);
			}
			/** IN Endpoint NAK Effective */
			if (diepint.b.inepnakeff)
			{
				DWC_DEBUGPL(DBG_ANY,"EP%d IN EP NAK Effective\n", epnum);
				/* Periodic EP */
				if (ep->disabling) 
				{
					depctl.d32 = 0;
					depctl.b.snak = 1;
					depctl.b.epdis = 1;
					usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + epnum * DWC_EP_REG_OFFSET, depctl.d32, depctl.d32);
				}
				CLEAR_IN_EP_INTR(epnum,inepnakeff);

			}
					
			/** IN EP Tx FIFO Empty Intr */
			if (diepint.b.emptyintr)
			{
			  #if !defined USE_DMA_INTERNAL
				DWC_DEBUGPL(DBG_ANY,"EP%d Tx FIFO Empty Intr \n", epnum);
				write_empty_tx_fifo( psBlockDesc->ui32Base, core_if, pcd, epnum );										

				CLEAR_IN_EP_INTR(epnum,emptyintr);
			  #else
				IMG_ASSERT(0);	//not expected when DMA is used
			  #endif
			}

			/** IN EP BNA Intr */
			if (diepint.b.bna)
			{
				CLEAR_IN_EP_INTR(epnum, bna);
				if (core_if->dma_desc_enable)
				{
				  #if defined _EN_ISOC_
					if(dwc_ep->type == DWC_OTG_EP_TYPE_ISOC)
					{
						/*
						 * This checking is performed to prevent first "false" BNA
						 * handling occuring right after reconnect
						 */
						if(dwc_ep->next_frame != 0xffffffff)
						{
							dwc_otg_pcd_handle_iso_bna( psBlockDesc->ui32Base, ep );
						}
					}
					else
				  #endif // _EN_ISOC_
					{
						dctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET );
					
						/* If Global Continue on BNA is disabled - disable EP */
						if(!dctl.b.gcontbna)
						{
							depctl.d32 = 0;
							depctl.b.snak = 1;
							depctl.b.epdis = 1;
							usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + epnum * DWC_EP_REG_OFFSET, depctl.d32, depctl.d32);
						}
						else
						{
							start_next_request( psBlockDesc, ep );
						}
					}
				}
			}
			/* NAK Interrupt */
			if (diepint.b.nak)
			{
				DWC_DEBUGPL(DBG_ANY, "EP%d IN NAK Interrupt\n", epnum);
				handle_in_ep_nak_intr( psBlockDesc->ui32Base, epnum );

				CLEAR_IN_EP_INTR(epnum, nak);
			}
		}
		epnum++;
		ep_intr >>=1;
	}

	return 1;
#undef CLEAR_IN_EP_INTR
}

/**
 * This interrupt indicates that an OUT EP has a pending Interrupt.
 * The sequence for handling the OUT EP interrupt is shown below:
 * -#	Read the Device All Endpoint Interrupt register
 * -#	Repeat the following for each OUT EP interrupt bit set (from
 *		LSB to MSB).
 * -#	Read the Device Endpoint Interrupt (DOEPINTn) register
 * -#	If "Transfer Complete" call the request complete function
 * -#	If "Endpoint Disabled" complete the EP disable procedure.
 * -#	If "AHB Error Interrupt" log error
 * -#	If "Setup Phase Done" process Setup Packet (See Standard USB
 *		Command Processing)
 */
static IMG_INT32 dwc_otg_pcd_handle_out_ep_intr(  const ioblock_sBlockDescriptor	*	psBlockDesc )
{
#define CLEAR_OUT_EP_INTR(__epnum,__intr) \
do { \
		doepint_data_t doepint = {0}; \
		doepint.b.__intr = 1; \
		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPINT0 + __epnum*DWC_EP_REG_OFFSET, doepint.d32); \
} while(0)
	USB_DC_T			*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	IMG_UINT32				ep_intr;
	dwc_otg_core_if_t	*	core_if		= &(psContext->sOtgCoreIf);
	dwc_otg_pcd_t		*	pcd			= &(psContext->sDevContext.sOtgPcd);
	doepint_data_t			doepint		= {0};
	dctl_data_t				dctl		= {0};
	depctl_data_t			doepctl		= {0};
	IMG_UINT32				epnum		= 0;
	dwc_otg_pcd_ep_t	*	ep;
	dwc_ep_t			*	dwc_ep;

	DWC_DEBUGPL(DBG_PCDV, "%s()\n", __func__);

	/* Read in the device interrupt bits */
	ep_intr = dwc_otg_read_dev_all_out_ep_intr(psBlockDesc->ui32Base);

	while( ep_intr ) 
	{
		if (ep_intr&0x1) 
		{
			/* Get EP pointer */	   
			ep = get_out_ep( core_if, pcd, epnum );
			dwc_ep = &ep->dwc_ep;

		  #if defined VERBOSE
			DWC_DEBUGPL(DBG_PCDV, "EP%d-%s: type=%d, mps=%d\n", dwc_ep->num, (dwc_ep->is_in ?"IN":"OUT"), dwc_ep->type, dwc_ep->maxpacket );
		  #endif
			doepint.d32 = dwc_otg_read_dev_out_ep_intr( psBlockDesc->ui32Base, dwc_ep );
				
			/* Transfer complete */
			if ( doepint.b.xfercompl ) 
			{
				DWC_DEBUGPL(DBG_PCD,"EP%d OUT Xfer Complete\n", epnum);

				if (epnum == 0) 
				{
					/* Clear the bit in DOEPINTn for this interrupt */
					CLEAR_OUT_EP_INTR(epnum,xfercompl);
					if (core_if->dma_desc_enable == 0 || pcd->ep0state != EP0_IDLE)
					{
						handle_ep0( psBlockDesc );
					}
			  #if defined _EN_ISOC_
				}
				else if(dwc_ep->type == DWC_OTG_EP_TYPE_ISOC)
				{
					if (doepint.b.pktdrpsts == 0)
					{
						/* Clear the bit in DOEPINTn for this interrupt */
						CLEAR_OUT_EP_INTR(epnum,xfercompl);
						complete_iso_ep( psBlockDesc, ep );
					}
					else
					{
						doepint_data_t doepint = { 0 };
						doepint.b.xfercompl = 1;
						doepint.b.pktdrpsts = 1;
						usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPINT0 + epnum * DWC_EP_REG_OFFSET, doepint.d32 );
						if (handle_iso_out_pkt_dropped( psBlockDesc->ui32Base, core_if, dwc_ep ) )
						{
							complete_iso_ep( psBlockDesc, ep );
						}
					}
			  #endif /* _EN_ISOC_ */
			  #if defined (_PER_IO_)
				} 
				else if (dwc_ep->type == DWC_OTG_EP_TYPE_ISOC)
				{
					CLEAR_OUT_EP_INTR(epnum, xfercompl);
					if (!ep->stopped)
					{
						complete_xiso_ep( psBlockDesc, ep );
					}
			  #endif /* _PER_IO_ */
				}
				else 
				{
					/* Clear the bit in DOEPINTn for this interrupt */
					CLEAR_OUT_EP_INTR(epnum,xfercompl);
					complete_ep( psBlockDesc, ep );
				}

			}

			/* Endpoint disable	 */
			if ( doepint.b.epdisabled ) 
			{
				DWC_DEBUGPL(DBG_PCD,"EP%d OUT disabled\n", epnum);
				/* Clear the bit in DOEPINTn for this interrupt */
				CLEAR_OUT_EP_INTR(epnum,epdisabled);
			}
			/* AHB Error */
			if ( doepint.b.ahberr ) 
			{
				DWC_DEBUGPL(DBG_PCD,"EP%d OUT AHB Error\n", epnum);
				DWC_DEBUGPL(DBG_PCD,"EP DMA REG	 %d \n", DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0 + epnum*DWC_EP_REG_OFFSET);													
				CLEAR_OUT_EP_INTR(epnum,ahberr);
			}
			/* Setup Phase Done (contorl EPs) */
			if ( doepint.b.setup ) 
			{
			  #if defined DEBUG_EP0
				DWC_DEBUGPL(DBG_PCD,"EP%d SETUP Done\n", epnum);
			  #endif
				CLEAR_OUT_EP_INTR(epnum,setup);
				handle_ep0( psBlockDesc );	
			}

			/** OUT EP BNA Intr */
			if (doepint.b.bna)
			{
				CLEAR_OUT_EP_INTR(epnum, bna);
				if(core_if->dma_desc_enable)
				{
				  #if defined _EN_ISOC_
					if(dwc_ep->type == DWC_OTG_EP_TYPE_ISOC)
					{
						/*
						 * This checking is performed to prevent first "false" BNA 
						 * handling occuring right after reconnect 
						 */ 
						if(dwc_ep->next_frame != 0xffffffff)
						{
							dwc_otg_pcd_handle_iso_bna( psBlockDesc->ui32Base, ep );
						}
					}
					else
				  #endif /* _EN_ISOC_ */
					{
						dctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET );
							
						/* If Global Continue on BNA is disabled - disable EP*/
						if(!dctl.b.gcontbna) 
						{
							doepctl.d32 = 0;
							doepctl.b.snak = 1;
							doepctl.b.epdis = 1;
							usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + epnum*DWC_EP_REG_OFFSET, doepctl.d32, doepctl.d32);
						}
						else
						{
							start_next_request( psBlockDesc, ep );
						}
					}
				}
			}
			if (doepint.b.stsphsercvd)
			{
				CLEAR_OUT_EP_INTR(epnum,stsphsercvd);
				if(core_if->dma_desc_enable)
				{
					do_setup_in_status_phase( psBlockDesc );
				}
			}
			/* Babble Interrupt */
			if (doepint.b.babble)
			{
				DWC_DEBUGPL(DBP_ANY, "EP%d OUT Babble\n", epnum);
				handle_out_ep_babble_intr(psBlockDesc->ui32Base, epnum);
				
				CLEAR_OUT_EP_INTR(epnum, babble);
			}
			/* NAK Interrupt */
			if (doepint.b.nak)
			{
				DWC_DEBUGPL(DBG_ANY, "EP%d OUT NAK\n", epnum);
				handle_out_ep_nak_intr(psBlockDesc->ui32Base, epnum);
				CLEAR_OUT_EP_INTR(epnum, nak);	
			}
			/* NYET Interrupt */
			if (doepint.b.nyet)
			{
				DWC_DEBUGPL(DBG_ANY, "EP%d OUT NYET\n", epnum);
				handle_out_ep_nyet_intr(psBlockDesc->ui32Base, epnum);

				CLEAR_OUT_EP_INTR(epnum, nyet);				
			}

		}

		epnum++;
		ep_intr >>=1;
	}

	return 1;

#undef CLEAR_OUT_EP_INTR
}

/**
 * Incomplete ISO IN Transfer Interrupt.
 * This interrupt indicates one of the following conditions occurred
 * while transmitting an ISOC transaction.
 * - Corrupted IN Token for ISOC EP.
 * - Packet not complete in FIFO.
 * The follow actions will be taken:
 *	-#	Determine the EP
 *	-#	Set incomplete flag in dwc_ep structure
 *	-#	Disable EP; when "Endpoint Disabled" interrupt is received
 *		Flush FIFO
 */
IMG_INT32 dwc_otg_pcd_handle_incomplete_isoc_in_intr(  const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	gintsts_data_t			gintsts;

#if defined _EN_ISOC_
	USB_DC_T			*	psContext = (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_core_if_t	*	core_if = &(psContext->sOtgCoreIf);
	dwc_otg_dev_if_t	*	dev_if = psContext->sDevContext.psOtgDevIf;
	dwc_otg_pcd_t		*	pcd = &(psContext->sDevContext.sOtgPcd);
	deptsiz_data_t			deptsiz = { 0 };
	depctl_data_t			depctl = { 0 };
	dsts_data_t				dsts = { 0 };
	dwc_ep_t			*	dwc_ep;
	int						i;

	for (i = 1; i <= dev_if->num_in_eps; ++i) 
	{
		dwc_ep = &pcd->in_ep[i].dwc_ep;
		if (dwc_ep->active && dwc_ep->type == DWC_OTG_EP_TYPE_ISOC) 
		{
			deptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + i * DWC_EP_REG_OFFSET );
			depctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + i * DWC_EP_REG_OFFSET );

			if (depctl.b.epdis && deptsiz.d32) 
			{
				set_current_pkt_info( psBlockDesc->ui32Base, core_if, dwc_ep);
				if (dwc_ep->cur_pkt >= dwc_ep->pkt_cnt) 
				{
					dwc_ep->cur_pkt = 0;
					dwc_ep->proc_buf_num = (dwc_ep->proc_buf_num ^ 1) & 0x1;

					if (dwc_ep->proc_buf_num) 
					{
						dwc_ep->cur_pkt_addr = dwc_ep->xfer_buff1;
						dwc_ep->cur_pkt_dma_addr = dwc_ep->dma_addr1;
					} 
					else 
					{
						dwc_ep->cur_pkt_addr = dwc_ep->xfer_buff0;
						dwc_ep->cur_pkt_dma_addr = dwc_ep->dma_addr0;
					}
	
				}

				dsts.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DSTS_OFFSET );
				dwc_ep->next_frame = dsts.b.soffn;

				dwc_otg_iso_ep_start_frm_transfer( psBlockDesc->ui32Base, core_if, dwc_ep );
			}
		}
	}

#else
	gintmsk_data_t intr_mask = { 0 };
	DWC_PRINT("INTERRUPT Handler not implemented for %s\n",
		   "IN ISOC Incomplete");

	intr_mask.b.incomplisoin = 1;
	usb_ModifyReg32( psBlockDesc->ui32Base,  DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, 0 );
#endif 	/* _EN_ISOC_ */

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.incomplisoin = 1;
	usb_WriteReg32 ( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}

/**
 * Incomplete ISO OUT Transfer Interrupt.  
 *
 * This interrupt indicates that the core has dropped an ISO OUT
 * packet.	The following conditions can be the cause:
 * - FIFO Full, the entire packet would not fit in the FIFO.
 * - CRC Error
 * - Corrupted Token
 * The follow actions will be taken:
 *	-#	Determine the EP
 *	-#	Set incomplete flag in dwc_ep structure
 *	-#	Read any data from the FIFO
 *	-#	Disable EP.	 when "Endpoint Disabled" interrupt is received
 *		re-enable EP.
 */
IMG_INT32 dwc_otg_pcd_handle_incomplete_isoc_out_intr(  const ioblock_sBlockDescriptor	*	psBlockDesc )
{

	gintsts_data_t gintsts;

#if defined _EN_ISOC_
	USB_DC_T			*	psContext = (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_core_if_t	*	core_if = &(psContext->sOtgCoreIf);
	dwc_otg_dev_if_t	*	dev_if = psContext->sDevContext.psOtgDevIf;
	dwc_otg_pcd_t		*	pcd = &(psContext->sDevContext.sOtgPcd);
	deptsiz_data_t			deptsiz = { 0 };
	depctl_data_t			depctl = { 0 };
	dsts_data_t				dsts = { 0 };
	dwc_ep_t			*	dwc_ep;
	int						i;

	for (i = 1; i <= dev_if->num_out_eps; ++i) 
	{
		dwc_ep = &pcd->in_ep[i].dwc_ep;
		if (pcd->out_ep[i].dwc_ep.active &&
		    pcd->out_ep[i].dwc_ep.type == DWC_OTG_EP_TYPE_ISOC) 
		{
			deptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + i * DWC_EP_REG_OFFSET ); 
			depctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + i * DWC_EP_REG_OFFSET );

			if (depctl.b.epdis && deptsiz.d32) 
			{
				set_current_pkt_info( psBlockDesc->ui32Base, core_if, &pcd->out_ep[i].dwc_ep);
				if (dwc_ep->cur_pkt >= dwc_ep->pkt_cnt) 
				{
					dwc_ep->cur_pkt = 0;
					dwc_ep->proc_buf_num = (dwc_ep->proc_buf_num ^ 1) & 0x1;

					if (dwc_ep->proc_buf_num) 
					{
						dwc_ep->cur_pkt_addr = dwc_ep->xfer_buff1;
						dwc_ep->cur_pkt_dma_addr = dwc_ep->dma_addr1;
					} 
					else 
					{
						dwc_ep->cur_pkt_addr = dwc_ep->xfer_buff0;
						dwc_ep->cur_pkt_dma_addr = dwc_ep->dma_addr0;
					}

				}

				dsts.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DSTS_OFFSET );
				dwc_ep->next_frame = dsts.b.soffn;

				dwc_otg_iso_ep_start_frm_transfer( psBlockDesc->ui32Base, core_if, dwc_ep);
			}
		}
	}
#else
	/* IMG_SYNOP_CR */
	gintmsk_data_t	intr_mask	= {0};


	DWC_PRINT("INTERRUPT Handler not implemented for %s\n", "OUT ISOC Incomplete");

	intr_mask.b.incomplisoout = 1;
	usb_ModifyReg32( psBlockDesc->ui32Base,  DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, 0 );

#endif				/* _EN_ISOC_ */

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.incomplisoout = 1;
	usb_WriteReg32 ( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}

/**
 * This function handles the Global IN NAK Effective interrupt.
 *	
 */
IMG_INT32 dwc_otg_pcd_handle_in_nak_effective(  const ioblock_sBlockDescriptor	*	psBlockDesc )
{	
	USB_DC_T			*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
	dwc_otg_dev_if_t	*	dev_if		= psContext->sDevContext.psOtgDevIf;
	depctl_data_t			diepctl		= {0};
	depctl_data_t			diepctl_rd	= {0};
	gintmsk_data_t			intr_mask	= {0};
	gintsts_data_t			gintsts;
	IMG_INT32				i;

	DWC_DEBUGPL(DBG_PCD, "Global IN NAK Effective\n");
	
	/* Disable all active IN EPs */
	diepctl.b.epdis = 1;
	diepctl.b.snak = 1;
	
	//Bojan: changed the <= to < in the statement below. I think this was a bug
	//for (i=0; i <= dev_if->num_in_eps; i++)
	for (i=0; i < dev_if->num_in_eps; i++)
	{
		diepctl_rd.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + i * DWC_EP_REG_OFFSET);
		if (diepctl_rd.b.epena) 
		{
			usb_WriteReg32( psBlockDesc->ui32Base,  DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + i * DWC_EP_REG_OFFSET, diepctl.d32 );	
		}
	}
	/* Disable the Global IN NAK Effective Interrupt */
	intr_mask.b.ginnakeff = 1;
	usb_ModifyReg32( psBlockDesc->ui32Base,  DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, 0);

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.ginnakeff = 1;
	usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}

/**
 * OUT NAK Effective.
 *
 */
IMG_INT32 dwc_otg_pcd_handle_out_nak_effective(  const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	gintmsk_data_t		intr_mask	= {0};
	gintsts_data_t		gintsts;

	DWC_PRINT("INTERRUPT Handler not implemented for %s\n", "Global IN NAK Effective\n");

#if defined DISABLE_OUT_EP_FEATURE //bojan
	{
		IMG_INT32				i;
		USB_DC_T			*	psContext	= (USB_DC_T *)psBlockDesc->pvAPIContext;
		dwc_otg_dev_if_t	*	dev_if		= psContext->sDevContext.psOtgDevIf;
		dwc_otg_pcd_t		*	pcd			= &(psContext->sDevContext.sOtgPcd);
		depctl_data_t			doepctl		= {0};
		depctl_data_t			doepctl_rd	= {0};
		dctl_data_t				dctl		= {0};
					

		doepctl.b.epdis = 1;
		doepctl.b.snak = 1;


		/* Set the epdis and snak bit for all the out endpoints we are trying to disable */
		for (i=0; i < dev_if->num_out_eps; i++) 
		{
			dwc_otg_pcd_ep_t *ep = &pcd->out_ep[i];
			
			if (ep->disabling)
			{
				doepctl_rd.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->dwc_ep.num * DWC_EP_REG_OFFSET);
				if (doepctl_rd.b.epena) 
				{
					usb_WriteReg32( psBlockDesc->ui32Base,  DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->dwc_ep.num * DWC_EP_REG_OFFSET, doepctl.d32 );	
				}

				ep->disabling = 0; //?
			}
		}

		/* clear goutnak - not quite right... should be done on a ep disabled interrupt.. but try here */
		dctl.b.cgoutnak = 1;
		usb_ModifyReg32( psBlockDesc->ui32Base,  DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, 0, dctl.d32 );

	}
#endif

	/* Disable the Global IN NAK Effective Interrupt */
	intr_mask.b.goutnakeff = 1;
	usb_ModifyReg32( psBlockDesc->ui32Base,  DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, 0);

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.goutnakeff = 1;
	usb_WriteReg32 ( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}

/**
 * PCD interrupt handler.
 *
 * The PCD handles the device interrupts.  Many conditions can cause a
 * device interrupt. When an interrupt occurs, the device interrupt
 * service routine determines the cause of the interrupt and
 * dispatches handling to the appropriate function. These interrupt
 * handling functions are described below.
 *
 * All interrupt registers are processed from LSB to MSB.
 * 
 */

IMG_INT32 dwc_otg_pcd_handle_intr(  const ioblock_sBlockDescriptor	*	psBlockDesc )
{
	gintsts_data_t		gintr_status;
	IMG_INT32			retval		= 0;

	SPIN_LOCK(&pcd->lock);

	gintr_status.d32 = dwc_otg_read_core_intr( psBlockDesc->ui32Base );

	if (!gintr_status.d32) 
	{
		return 0;
	}
	
	if (gintr_status.b.sofintr) 
	{
		retval |= dwc_otg_pcd_handle_sof_intr( psBlockDesc->ui32Base );
	}
	if (gintr_status.b.rxstsqlvl) 
	{
		#if !defined USE_DMA_INTERNAL
		retval |= dwc_otg_pcd_handle_rx_status_q_level_intr( psContext );
		#else
		IMG_ASSERT(0);	//not expected in slave mode
		#endif
	}
	if (gintr_status.b.nptxfempty) 
	{
		#if !defined USE_MULTIPLE_TX_FIFO
		retval |= dwc_otg_pcd_handle_np_tx_fifo_empty_intr( psBlockDesc );
		#else
		IMG_ASSERT(0);	//not expected in dedicated fifo mode
		#endif
	}
	if (gintr_status.b.ginnakeff) 
	{
		retval |= dwc_otg_pcd_handle_in_nak_effective( psBlockDesc );
	}
	if (gintr_status.b.goutnakeff) 
	{
		retval |= dwc_otg_pcd_handle_out_nak_effective( psBlockDesc );
	}
	if (gintr_status.b.i2cintr) 
	{
		#if defined USE_I2C
		retval |= dwc_otg_pcd_handle_i2c_intr( );
		#else
		IMG_ASSERT(0);
		#endif
	}
	if (gintr_status.b.erlysuspend) 
	{
		retval |= dwc_otg_pcd_handle_early_suspend_intr( psBlockDesc );
	}
	if (gintr_status.b.usbreset) 
	{
		retval |= dwc_otg_pcd_handle_usb_reset_intr( psBlockDesc );
	}
	if (gintr_status.b.enumdone) 
	{
		retval |= dwc_otg_pcd_handle_enum_done_intr( psBlockDesc );
	}
  #if defined USE_PERIODIC_EP
	if (gintr_status.b.isooutdrop) 
	{
		retval |= dwc_otg_pcd_handle_isoc_out_packet_dropped_intr( psBlockDesc->ui32Base );
	}
	if (gintr_status.b.eopframe) 
	{
		retval |= dwc_otg_pcd_handle_end_periodic_frame_intr( psBlockDesc->ui32Base );
	}
  #endif /* USE_PERIODIC_EP */
	if (gintr_status.b.epmismatch) 
	{
		#if !defined USE_MULTIPLE_TX_FIFO
		retval |= dwc_otg_pcd_handle_ep_mismatch_intr( psBlockDesc->ui32Base );
		#else
		IMG_ASSERT(0);
		#endif
	}
	if (gintr_status.b.inepint) 
	{
		retval |= dwc_otg_pcd_handle_in_ep_intr( psBlockDesc );
	}
	if (gintr_status.b.outepintr) 
	{
		retval |= dwc_otg_pcd_handle_out_ep_intr( psBlockDesc );
	}
	if (gintr_status.b.incomplisoin) 
	{
		retval |= dwc_otg_pcd_handle_incomplete_isoc_in_intr( psBlockDesc );
	}
	if (gintr_status.b.incomplisoout) 
	{
		retval |= dwc_otg_pcd_handle_incomplete_isoc_out_intr( psBlockDesc );
	}

	SPIN_UNLOCK(&pcd->lock);

	return retval;
}

#endif /* DWC_HOST_ONLY */
