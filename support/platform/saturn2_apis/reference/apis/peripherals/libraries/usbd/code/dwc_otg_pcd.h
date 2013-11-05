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
#if !defined(__DWC_PCD_H__)
#define __DWC_PCD_H__

#include <MeOS.h>
#include <usb_spec.h>



#include "dwc_otg_cil.h"

/**
 * @file
 *
 * This file contains the structures, constants, and interfaces for
 * the Perpherial Contoller Driver (PCD).
 *
 * The Peripheral Controller Driver (PCD) for Linux will implement the
 * Gadget API, so that the existing Gadget drivers can be used.	 For
 * the Mass Storage Function driver the File-backed USB Storage Gadget
 * (FBS) driver will be used.  The FBS driver supports the
 * Control-Bulk (CB), Control-Bulk-Interrupt (CBI), and Bulk-Only
 * transports.
 *
 */

/** Max Transfer size for any EP */
#define DDMA_MAX_TRANSFER_SIZE	65535

/**
 * States of EP0.
 */
typedef enum ep0_state
{
	EP0_DISCONNECT,		/* no host */
	EP0_IDLE,
	EP0_IN_DATA_PHASE,
	EP0_OUT_DATA_PHASE,
	EP0_IN_STATUS_PHASE,
	EP0_OUT_STATUS_PHASE,
	EP0_STALL,
} ep0state_e;

/** Fordward declaration.*/
struct dwc_otg_pcd;

#if defined (_PER_IO_)

/**
 * This shall be the exact analogy of the same type structure defined in the
 * usb_gadget.h. Each descriptor contains
 */
struct dwc_iso_pkt_desc_port
{
	IMG_UINT32	offset;
	IMG_UINT32	length;	/* expected length */
	IMG_UINT32	actual_length;
	IMG_UINT32	status;
};

struct dwc_iso_xreq_port
{
	/** transfer/submission flag */
	IMG_UINT32 tr_sub_flags;
	/** Start the request ASAP */
#define DWC_EREQ_TF_ASAP	0x00000002
	/** Just enqueue the request w/o initiating a transfer */
#define DWC_EREQ_TF_ENQUEUE	0x00000004

	/**
	* count of ISO packets attached to this request - shall
	* not exceeed the pio_alloc_pkt_count
	*/
	IMG_UINT32 pio_pkt_count;
	/** count of ISO packets allocated for this request */
	IMG_UINT32 pio_alloc_pkt_count;
	/** number of ISO packet errors */
	IMG_UINT32 error_count;
	/** reserved for future extension */
	IMG_UINT32 res;
	/** Will be allocated and freed in the UTE gadget and based on the CFC value */
	struct dwc_iso_pkt_desc_port	per_io_frame_descs[MAX_ISO_PACKETS];
};

#endif




/**	  PCD EP structure.
 * This structure describes an EP, there is an array of EPs in the PCD
 * structure.
 */
typedef struct dwc_otg_pcd_ep
{
	/** USB EP Descriptor */
	const struct usb_endpoint_descriptor_t	*desc;

	/* queue of dwc_otg_pcd_requests. */
	LST_T		  sRequestList;

	IMG_UINT8 stopped;
	IMG_UINT8 disabling;
	IMG_UINT8 dma;
	IMG_UINT8 queue_sof;

	/** Count of pending Requests */
	IMG_UINT8	request_pending;

#ifdef _EN_ISOC_
	struct usb_iso_request	*	iso_req;

#endif // _EN_ISOC_

	/** DWC_otg ep data. */
	dwc_ep_t dwc_ep;

	/** Pointer to PCD */
	struct dwc_otg_pcd *pcd;
} dwc_otg_pcd_ep_t;



/** DWC_otg PCD Structure.
 * This structure encapsulates the data for the dwc_otg PCD.
 */
typedef struct dwc_otg_pcd
{
	enum usb_device_speed		speed;
	IMG_UINT8				is_dualspeed;
#ifdef DWC_SUPPORT_OTG
	IMG_UINT8				is_otg;
#endif
	/** The core if pointer. */
	//struct dwc_otg_core_if		*core_if;		/* IMG_SYNOP_CR */

	/** State of EP0 */
	ep0state_e					ep0state;
	/** EP0 Request is pending */
	IMG_UINT8				ep0_pending;
	/** Indicates when SET CONFIGURATION Request is in process */
	IMG_UINT8				request_config;
	/** The state of the Remote Wakeup Enable. */
	IMG_UINT8				remote_wakeup_enable;

#ifdef DWC_SUPPORT_OTG
	/** The state of the B-Device HNP Enable. */
	IMG_UINT8				b_hnp_enable;
	/** The state of A-Device HNP Support. */
	IMG_UINT8				a_hnp_support;
	/** The state of the A-Device Alt HNP support. */
	IMG_UINT8				a_alt_hnp_support;
#endif

	/** SETUP packet for EP0
	 * This structure is allocated as a DMA buffer on PCD initialization
	 * with enough space for up to 3 setup packets.
	 */
	union
	{
			struct usb_ctrlrequest_t	req;
			IMG_UINT32					d32[2];
	} *setup_pkt;

	IMG_UINT32					setup_pkt_dma_handle;

	/** 2-byte dma buffer used to return status from GET_STATUS */
	IMG_UINT16					*status_buf;
	IMG_UINT32					status_buf_dma_handle;

	/** EP0 */
	dwc_otg_pcd_ep_t			ep0;

	/** Array of IN EPs. */
	dwc_otg_pcd_ep_t			in_ep[ MAX_EPS_CHANNELS - 1];			/* IMG_SYNOP_CR */
	/** Array of OUT EPs. */
	dwc_otg_pcd_ep_t			out_ep[ MAX_EPS_CHANNELS - 1];			/* IMG_SYNOP_CR */

#ifdef DWC_SUPPORT_OTG
	/** Timer for SRP.	If it expires before SRP is successful
	 * clear the SRP. */
	struct timer_list			srp_timer;
#endif

	/** The test mode to enter when the tasklet is executed. */
	unsigned					test_mode;

} dwc_otg_pcd_t;

struct usb_dc_t;

/** DWC_otg request structure.
 */
typedef struct dwc_otg_pcd_request
{
	LST_LINK;
	IMG_VOID		*buf;
	IMG_UINT32		length;
	IMG_UINT32		dma;
	IMG_UINT8		zero;
	IMG_VOID (*complete)( const struct ioblock_sBlockDescriptorTag *, IMG_VOID*, struct dwc_otg_pcd_request*);
	IMG_INT32		status;
	IMG_UINT32		actual;
#if defined (_PER_IO_)
	struct dwc_iso_xreq_port ext_req;
	//void *priv_ereq_nport; /* */
#endif
} dwc_otg_pcd_request_t;

#if defined (_EN_ISOC_)

/* ISO Packet descriptor */
typedef struct usb_iso_packet_descriptor
{
	IMG_UINT32		offset;
	IMG_UINT32		length; /* expected length */
	IMG_UINT32		actual;
	IMG_UINT32		status;
} usb_iso_packet_descriptor_t;

/* Iso request structure */
typedef struct usb_iso_request
{
	LST_LINK;
	IMG_VOID		*buf0;
	IMG_VOID		*buf1;
	IMG_UINT32		dma0;
	IMG_UINT32		dma1;

	IMG_UINT32		buf_proc_intrvl;

	IMG_UINT8		zero;

	IMG_UINT32		data_per_frame;
	IMG_UINT32		start_frame;
	IMG_UINT32		flags;

	IMG_VOID		(*process_buffer)(	IMG_UINT8						ui8BufferNum,
										IMG_UINT8					*	pui8Buffer,
										usb_iso_packet_descriptor_t	*	iso_packet );

	usb_iso_packet_descriptor_t		iso_packet_desc0[MAX_ISO_PACKETS];
	usb_iso_packet_descriptor_t		iso_packet_desc1[MAX_ISO_PACKETS];
} usb_iso_request_t;

#endif

#include "usb_drv.h"

/* Callback functions for the pcd layer */
extern IMG_INT32 usbd_Setup ( const ioblock_sBlockDescriptor	*	psBlockDesc, struct usb_ctrlrequest_t *ctrl );
extern IMG_VOID usbd_Disconnect( const ioblock_sBlockDescriptor	*	psBlockDesc );
extern IMG_VOID usbd_Suspend ( img_void	*	p );
extern IMG_VOID usbd_Resume ( img_void	*	p );
extern IMG_VOID usbd_BusReset ( const ioblock_sBlockDescriptor	*	psBlockDesc );
extern IMG_VOID usbd_EventNotify ( USB_DC_T	*	psContext, USBD_EVENT_T eEvent );


extern IMG_VOID		dwc_otg_pcd_init( const ioblock_sBlockDescriptor	*	psBlockDesc );
extern IMG_INT32	dwc_otg_pcd_handle_intr( const ioblock_sBlockDescriptor	*	psBlockDesc );

extern IMG_INT32	dwc_otg_pcd_ep_enable( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep, const struct usb_endpoint_descriptor_t *desc );
extern IMG_INT32	dwc_otg_pcd_ep_disable( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep );
extern IMG_INT32	dwc_otg_pcd_ep_queue( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep, struct dwc_otg_pcd_request *req );
extern IMG_INT32	dwc_otg_pcd_ep_is_queued( dwc_otg_pcd_ep_t *ep );
extern IMG_VOID		dwc_otg_pcd_ep_reset( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep );

#if defined (_PER_IO_)
extern IMG_INT32	dwc_otg_pcd_xiso_ep_queue( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t *ep, struct dwc_otg_pcd_request *req );
#endif /* _PER_IO_ */

extern IMG_VOID		release_perio_tx_fifo( dwc_otg_core_if_t	*	core_if, IMG_UINT32 fifo_num );
extern IMG_VOID		release_tx_fifo( dwc_otg_core_if_t	*	core_if, IMG_UINT32 fifo_num );

/* request object allocation functions */
extern IMG_VOID		dwc_otg_pcd_initialise_request_list ( );
extern struct dwc_otg_pcd_request *	dwc_otg_pcd_alloc_request( );
extern IMG_VOID		dwc_otg_pcd_free_request(dwc_otg_pcd_request_t *req);

#if defined(TIMER_NOT_AVAILABLE)
extern IMG_BOOL	bDoTestMode;
extern IMG_VOID do_test_mode_func( IMG_VOID	*	pParam );
extern IMG_BOOL	bStartXferTimer;
extern IMG_VOID start_xfer_func( IMG_VOID	*	pParam );
#else
extern KRN_TIMER_T	tStartXferTimer;
extern IMG_VOID		start_xfer_func (KRN_TIMER_T *timer, IMG_VOID * pParam );
#endif

#ifdef DWC_SUPPORT_OTG
extern IMG_VOID		dwc_otg_pcd_start_srp_timer( );
extern IMG_VOID		dwc_otg_pcd_initiate_srp( );
#endif

#if defined (_EN_ISOC_)
IMG_VOID dwc_otg_iso_buffer_done( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_pcd_ep_t * ep, usb_iso_request_t *req);
#endif

#endif
#endif /* DWC_HOST_ONLY */
