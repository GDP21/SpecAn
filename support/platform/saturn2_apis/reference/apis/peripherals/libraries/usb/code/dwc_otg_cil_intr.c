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

/** @file 
 *
 * The Core Interface Layer provides basic services for accessing and
 * managing the DWC_otg hardware. These services are used by both the
 * Host Controller Driver and the Peripheral Controller Driver.
 *
 * This file contains the Common Interrupt handlers.
 */

#define METAG_ALL_VALUES
#define METAC_ALL_VALUES

#if defined (__META_MEOS__) || defined (__MTX_MEOS__)
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <img_defs.h>
#include <MeOS.h>
#include <ioblock_defs.h>

#include "usb_hal.h"
#include "dwc_otg_regs.h"
#include "dwc_otg_cil.h"

#if defined USBD_DEBUG
__USBD_INLINE__ const IMG_CHAR *op_state_str( dwc_otg_core_if_t *core_if ) 
{
        return (core_if->op_state==A_HOST?"a_host":
                (core_if->op_state==A_SUSPEND?"a_suspend":
                 (core_if->op_state==A_PERIPHERAL?"a_peripheral":
                  (core_if->op_state==B_PERIPHERAL?"b_peripheral":
                   (core_if->op_state==B_HOST?"b_host":
                    "unknown")))));
}
#endif

#if !defined DWC_DEVICE_ONLY
/** This function will log a debug message 
 *
 * @param core_if Programming view of DWC_otg controller.
 */
IMG_INT32 dwc_otg_handle_mode_mismatch_intr ( )
{
	gintsts_data_t		gintsts;
	
	DWC_WARN("Mode Mismatch Interrupt: currently in %s mode\n", dwc_otg_mode(core_if) ? "Host" : "Device");

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.modemismatch = 1;	
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);
	return 1;
}

/** Start the HCD.  Helper function for using the HCD callbacks.
 *
 * @param core_if Programming view of DWC_otg controller.
 */
static __USBD_INLINE__ IMG_VOID hcd_start(dwc_otg_core_if_t *core_if) 
{        
    if (core_if->hcd_cb && core_if->hcd_cb->start) 
	{
    	core_if->hcd_cb->start(core_if->hcd_cb->p);
    }
}
/** Stop the HCD.  Helper function for using the HCD callbacks. 
 *
 * @param core_if Programming view of DWC_otg controller.
 */
static __USBD_INLINE__ IMG_VOID hcd_stop(dwc_otg_core_if_t *core_if) 
{        
    if (core_if->hcd_cb && core_if->hcd_cb->stop) 
	{
    	core_if->hcd_cb->stop(core_if->hcd_cb->p);
    }
}
/** Disconnect the HCD.  Helper function for using the HCD callbacks.
 *
 * @param core_if Programming view of DWC_otg controller.
 */
static __USBD_INLINE__ IMG_VOID hcd_disconnect(dwc_otg_core_if_t *core_if) 
{
    if (core_if->hcd_cb && core_if->hcd_cb->disconnect) 
	{
    	core_if->hcd_cb->disconnect(core_if->hcd_cb->p);
    }
}
/** Inform the HCD the a New Session has begun.  Helper function for
 * using the HCD callbacks.
 *
 * @param core_if Programming view of DWC_otg controller.
 */
static __USBD_INLINE__ IMG_VOID hcd_session_start(dwc_otg_core_if_t *core_if) 
{
    if (core_if->hcd_cb && core_if->hcd_cb->session_start) 
	{
    	core_if->hcd_cb->session_start(core_if->hcd_cb->p);
    }
}

#endif /* !DWC_DEVICE_ONLY */

#if defined DWC_SUPPORT_OTG
/** Start the PCD.  Helper function for using the PCD callbacks.
 *
 * @param _core_if Programming view of DWC_otg controller.
 */
static __USBD_INLINE__ IMG_VOID pcd_start(  ) 
{
	dwc_otg_core_if_t *core_if = sUSBDDeviceContext.psOtgCoreIf;

	if (core_if->pcd_cb && core_if->pcd_cb->start )
	{
            core_if->pcd_cb->start( core_if->pcd_cb->p );
    }
}
/** Stop the PCD.  Helper function for using the PCD callbacks. 
 *
 * @param core_if Programming view of DWC_otg controller.
 */
static __USBD_INLINE__ IMG_VOID pcd_stop( ) 
{
	dwc_otg_core_if_t *core_if = sUSBDDeviceContext.psOtgCoreIf;

    if (core_if->pcd_cb && core_if->pcd_cb->stop )
	{
            core_if->pcd_cb->stop( core_if->pcd_cb->p );
    }
}
#endif /* DWC_SUPPORT_OTG */

/** Suspend the PCD.  Helper function for using the PCD callbacks. 
 *
 * @param core_if Programming view of DWC_otg controller.
 */
static __USBD_INLINE__ IMG_VOID pcd_suspend( dwc_otg_core_if_t *core_if ) 
{
    if (core_if->pcd_cb && core_if->pcd_cb->suspend )
	{
		core_if->pcd_cb->suspend( core_if->pcd_cb->p );
    }
}
/** Resume the PCD.  Helper function for using the PCD callbacks. 
 *
 * @param core_if Programming view of DWC_otg controller.
 */
static __USBD_INLINE__ IMG_VOID pcd_resume( dwc_otg_core_if_t *core_if  ) 
{
    if (core_if->pcd_cb && core_if->pcd_cb->resume_wakeup )
	{
		core_if->pcd_cb->resume_wakeup( core_if->pcd_cb->p );
    }
}

#if defined DWC_SUPPORT_OTG
/**
 * This function handles the OTG Interrupts. It reads the OTG
 * Interrupt Register (GOTGINT) to determine what interrupt has
 * occurred.
 *
 * @param core_if Programming view of DWC_otg controller.
 */
IMG_INT32 dwc_otg_handle_otg_intr( )
{

	dwc_otg_core_if_t			*core_if = sUSBDDeviceContext.psOtgCoreIf;
	gotgint_data_t				gotgint;
    gotgctl_data_t				gotgctl;
	gintmsk_data_t				gintmsk;

	gotgint.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GOTGINT_OFFSET);
    gotgctl.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GOTGCTL_OFFSET);
	
	DWC_DEBUGPL(DBG_CIL, "++OTG Interrupt gotgint=%0x [%s]\n", gotgint.d32, op_state_str(core_if));
   //DWC_DEBUGPL(DBG_CIL, "gotgctl=%08x\n", gotgctl.d32 );

	if (gotgint.b.sesenddet) 
	{
		DWC_DEBUGPL(DBG_ANY, " ++OTG Interrupt: ""Session End Detected++ (%s)\n", op_state_str(core_if));
                
		gotgctl.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GOTGCTL_OFFSET);

        if (core_if->op_state == B_HOST)
		{
            pcd_start( core_if );
            core_if->op_state = B_PERIPHERAL;
        } 
		else
		{
            /* If not B_HOST and Device HNP still set. HNP
             * Did not succeed!*/
            if (gotgctl.b.devhnpen) 
			{
                    DWC_DEBUGPL(DBG_ANY, "Session End Detected\n");
                    DWC_ERROR( "Device Not Connected/Responding!\n" );
            }

            /* If Session End Detected the B-Cable has
             * been disconnected. */
            /* Reset PCD and Gadget driver to a
             * clean state. */
			core_if->lx_state = DWC_OTG_L0;
            pcd_stop(core_if);
        }
        gotgctl.d32 = 0;
        gotgctl.b.devhnpen = 1;
        usb_ModifyReg32(DWC_OTG_CORE_GLOBAL_REGS_GOTGCTL_OFFSET, gotgctl.d32, 0);
    }
	if (gotgint.b.sesreqsucstschng)
	{
		DWC_DEBUGPL(DBG_ANY, " ++OTG Interrupt: ""Session Reqeust Success Status Change++\n");

        gotgctl.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GOTGCTL_OFFSET);
        if (gotgctl.b.sesreqscs) 
		{
		  #if defined USE_I2C
			if ((core_if->core_params->phy_type == DWC_PHY_TYPE_PARAM_FS) && 
			(core_if->core_params->i2c_enable)) 
			{
				core_if->srp_success = 1;
			}
			else
		  #endif
			{
				pcd_resume( core_if );
				/* Clear Session Request */
				gotgctl.d32 = 0;
				gotgctl.b.sesreq = 1;
				usb_ModifyReg32(DWC_OTG_CORE_GLOBAL_REGS_GOTGCTL_OFFSET, gotgctl.d32, 0);
			}
        }
	}
	if (gotgint.b.hstnegsucstschng) 
	{
        /* Print statements during the HNP interrupt handling
         * can cause it to fail.*/
        gotgctl.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GOTGCTL_OFFSET);
        if (gotgctl.b.hstnegscs) 
		{
            if (dwc_otg_is_host_mode() ) 
			{
                core_if->op_state = B_HOST;
				/*
				* Need to disable SOF interrupt immediately.
				* When switching from device to host, the PCD
				* interrupt handler won't handle the
				* interrupt if host mode is already set. The
				* HCD interrupt handler won't get called if
				* the HCD state is HALT. This means that the
				* interrupt does not get handled and Linux
				* complains loudly.
				*/
				gintmsk.d32 = 0;
				gintmsk.b.sofintr = 1;
				usb_ModifyReg32(DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, gintmsk.d32, 0);
			
                pcd_stop(core_if);
                /*
                 * Initialize the Core for Host mode.
                 */
                hcd_start( core_if );
                core_if->op_state = B_HOST;
            }
        } 
		else 
		{
            gotgctl.d32 = 0;
            gotgctl.b.hnpreq = 1;
            gotgctl.b.devhnpen = 1;
            usb_ModifyReg32(DWC_OTG_CORE_GLOBAL_REGS_GOTGCTL_OFFSET, gotgctl.d32, 0);
            DWC_DEBUGPL( DBG_ANY, "HNP Failed\n");
            DWC_ERROR( "Device Not Connected/Responding\n" );
        }
	}
	if (gotgint.b.hstnegdet) 
	{
        /* The disconnect interrupt is set at the same time as
		 * Host Negotiation Detected.  During the mode
		 * switch all interrupts are cleared so the disconnect
		 * interrupt handler will not get executed.
                 */
		DWC_DEBUGPL(DBG_ANY, " ++OTG Interrupt: ""Host Negotiation Detected++ (%s)\n", (dwc_otg_is_host_mode()?"Host":"Device"));
        if (dwc_otg_is_device_mode())
		{
            DWC_DEBUGPL(DBG_ANY, "a_suspend->a_peripheral (%d)\n",core_if->op_state);
            hcd_disconnect( core_if );
            pcd_start( core_if );
            core_if->op_state = A_PERIPHERAL;
        } 
		else 
		{
			/*
			* Need to disable SOF interrupt immediately. When
			* switching from device to host, the PCD interrupt
			* handler won't handle the interrupt if host mode is
			* already set. The HCD interrupt handler won't get
			* called if the HCD state is HALT. This means that
			* the interrupt does not get handled and Linux
			* complains loudly.
			*/
			gintmsk.d32 = 0;
			gintmsk.b.sofintr = 1;
			usb_ModifyReg32(DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, gintmsk.d32, 0);

			pcd_stop( core_if );
			hcd_start( core_if );
			core_if->op_state = A_HOST;
        }
	}
	if (gotgint.b.adevtoutchng) 
	{
		DWC_DEBUGPL(DBG_ANY, " ++OTG Interrupt: ""A-Device Timeout Change++\n");
	}
	if (gotgint.b.debdone) 
	{
		DWC_DEBUGPL(DBG_ANY, " ++OTG Interrupt: ""Debounce Done++\n");
	}

	/* Clear GOTGINT */
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GOTGINT_OFFSET, gotgint.d32);

	return 1;
}

/**
 * This function handles the Connector ID Status Change Interrupt.  It
 * reads the OTG Interrupt Register (GOTCTL) to determine whether this
 * is a Device to Host Mode transition or a Host Mode to Device
 * Transition.  
 *
 * This only occurs when the cable is connected/removed from the PHY
 * connector.
 *
 * @param core_if Programming view of DWC_otg controller.
 */
IMG_INT32 dwc_otg_handle_conn_id_status_change_intr( )
{
	dwc_otg_core_if_t	*core_if	= sUSBDDeviceContext.psOtgCoreIf;
    IMG_UINT32			count		= 0;
	gintsts_data_t		gintsts		= { 0 };
	gintmsk_data_t		gintmsk		= { 0 };
    gotgctl_data_t		gotgctl		= { 0 }; 

	/*
	 * Need to disable SOF interrupt immediately. If switching from device
	 * to host, the PCD interrupt handler won't handle the interrupt if
	 * host mode is already set. The HCD interrupt handler won't get
	 * called if the HCD state is HALT. This means that the interrupt does
	 * not get handled and Linux complains loudly.
	 */
	gintmsk.b.sofintr = 1;
	usb_ModifyReg32(DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, gintmsk.d32, 0);


	DWC_DEBUGPL(DBG_CIL, " ++Connector ID Status Change Interrupt++  (%s)\n", (dwc_otg_is_host_mode()?"Host":"Device"));
    
	gotgctl.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GOTGCTL_OFFSET);
	
	DWC_DEBUGPL(DBG_CIL, "gotgctl=%0x\n", gotgctl.d32);
	DWC_DEBUGPL(DBG_CIL, "gotgctl.b.conidsts=%d\n", gotgctl.b.conidsts);
        
    /* B-Device connector (Device Mode) */
    if (gotgctl.b.conidsts) 
	{
        /* Wait for switch to device mode. */
        while (!dwc_otg_is_device_mode() )
		{
            DWC_PRINT("Waiting for Peripheral Mode, Mode=%s\n", (dwc_otg_is_host_mode()?"Host":"Peripheral"));
            MDELAY(100);
            if (++count > 10000) *(IMG_UINT32*)IMG_NULL=0;
        }
        core_if->op_state = B_PERIPHERAL;
		dwc_otg_core_init();
		dwc_otg_enable_global_interrupts();
        pcd_start( core_if );
    } 
	else 
	{
        /* A-Device connector (Host Mode) */
        while (!dwc_otg_is_host_mode() ) 
		{
            DWC_PRINT("Waiting for Host Mode, Mode=%s\n", (dwc_otg_is_host_mode()?"Host":"Peripheral"));
            MDELAY(100);
            if (++count > 10000) *(IMG_UINT32*)IMG_NULL=0;
        }
        core_if->op_state = A_HOST;
        /*
         * Initialize the Core for Host mode.
         */
		dwc_otg_core_init();
		dwc_otg_enable_global_interrupts();
        hcd_start( core_if );
    }

	/* Set flag and clear interrupt */
	gintsts.b.conidstschng = 1;
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}

/** 
 * This interrupt indicates that a device is initiating the Session
 * Request Protocol to request the host to turn on bus power so a new
 * session can begin. The handler responds by turning on bus power. If
 * the DWC_otg controller is in low power mode, the handler brings the
 * controller out of low power mode before turning on bus power. 
 *
 * @param core_if Programming view of DWC_otg controller.
 */
IMG_INT32 dwc_otg_handle_session_req_intr( )
{
	dwc_otg_core_if_t	*core_if = sUSBDDeviceContext.psOtgCoreIf;
	gintsts_data_t		gintsts;
	

  #if !defined DWC_HOST_ONLY
	DWC_DEBUGPL(DBG_ANY, "++Session Request Interrupt++\n");	
/*
    if (dwc_otg_is_device_mode() ) 
	{
*/
		DWC_PRINT("SRP: Device mode\n");
/*        
	} 
	else 
	{
		DWC_PRINT("SRP: Host mode\n");

		// Turn on the port power bit.
		hprt0.d32 = dwc_otg_read_hprt0( core_if );
		hprt0.b.prtpwr = 1;
		usb_WriteReg32( ui32BaseAddress, core_if->host_if->hprt0, hprt0.d32);

		// Start the Connection timer. So a message can be displayed
		// if connect does not occur within 10 seconds. 
		hcd_session_start( core_if );
    }
*/
  #endif

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.sessreqintr = 1;
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}

void w_wakeup_detected(void *p)
{
	dwc_otg_core_if_t *core_if = (dwc_otg_core_if_t *) p;
	/*
	 * Clear the Resume after 70ms. (Need 20 ms minimum. Use 70 ms
	 * so that OPT tests pass with all PHYs).
	 */
	hprt0_data_t hprt0 = {.d32 = 0 };
#if 0
	pcgcctl_data_t pcgcctl = {.d32 = 0 };
	/* Restart the Phy Clock */
	pcgcctl.b.stoppclk = 1;
	dwc_modify_reg32(core_if->pcgcctl, pcgcctl.d32, 0);
	dwc_udelay(10);
#endif				//0
	hprt0.d32 = dwc_otg_read_hprt0(core_if);
	DWC_DEBUGPL(DBG_ANY, "Resume: HPRT0=%0x\n", hprt0.d32);
//      dwc_mdelay(70);
	hprt0.b.prtres = 0;	/* Resume */
	dwc_write_reg32(core_if->host_if->hprt0, hprt0.d32);
	DWC_DEBUGPL(DBG_ANY, "Clear Resume: HPRT0=%0x\n",
		    dwc_read_reg32(core_if->host_if->hprt0));

	hcd_resume(core_if);

	/** Change to L0 state*/
	core_if->lx_state = DWC_OTG_L0;

}

#endif /* DWC_SUPPORT_OTG */

/** 
 * This interrupt indicates that the DWC_otg controller has detected a
 * resume or remote wakeup sequence. If the DWC_otg controller is in
 * low power mode, the handler must brings the controller out of low
 * power mode. The controller automatically begins resume
 * signaling. The handler schedules a time to stop resume signaling.
 */
IMG_INT32 dwc_otg_handle_wakeup_detected_intr( const img_uint32 ui32BaseAddress, dwc_otg_core_if_t *core_if )
{
	gintsts_data_t		gintsts;

	DWC_DEBUGPL(DBG_ANY, "++Resume and Remote Wakeup Detected Interrupt++\n");

	if (dwc_otg_is_device_mode( ui32BaseAddress ))
	{
		dctl_data_t			dctl = {0};
    	DWC_DEBUGPL(DBG_PCD, "DSTS=0x%0x\n", usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DSTS_OFFSET));
#if defined(PARTIAL_POWER_DOWN)
   		if (core_if->hwcfg4.b.power_optimiz) 
		{
        	pcgcctl_data_t power = {0};

	        power.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_PCGCCTL_OFFSET);
	        DWC_DEBUGPL(DBG_CIL, "PCGCCTL=%0x\n", power.d32);
	
	        power.b.stoppclk = 0;
	        usb_WriteReg32( ui32BaseAddress, DWC_OTG_PCGCCTL_OFFSET, power.d32);
	
	        power.b.pwrclmp = 0;
	        usb_WriteReg32( ui32BaseAddress, DWC_OTG_PCGCCTL_OFFSET, power.d32);
	
	        power.b.rstpdwnmodule = 0;
	        usb_WriteReg32( ui32BaseAddress, DWC_OTG_PCGCCTL_OFFSET, power.d32);
	    }
#endif

    	/* Clear the Remote Wakeup Signalling */
    	dctl.b.rmtwkupsig = 1;
    	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, dctl.d32, 0 );

    	if (core_if->pcd_cb && core_if->pcd_cb->resume_wakeup) 
		{
			core_if->pcd_cb->resume_wakeup( core_if->pcd_cb->p );
	    }
	}
	else
	{
		pcgcctl_data_t	pcgcctl = { 0 };

		/* Restart the Phy Clock */
		pcgcctl.b.stoppclk = 1;
		usb_ModifyReg32( ui32BaseAddress, DWC_OTG_PCGCCTL_OFFSET, pcgcctl.d32, 0);
	}

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.wkupintr = 1;
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}

#if !defined DWC_DEVICE_ONLY
/** 
 * This interrupt indicates that a device has been disconnected from
 * the root port. 
 */
IMG_INT32 dwc_otg_handle_disconnect_intr( USB_DC_T	*	psContext )
{

	dwc_otg_core_if_t *core_if = psContext->psOtgCoreIf;
	gintsts_data_t gintsts;

	DWC_DEBUGPL(DBG_ANY, "++Disconnect Detected Interrupt++ (%s) %s\n", 
                    (dwc_otg_is_host_mode(core_if)?"Host":"Device"), 
                    op_state_str(core_if));

		/* IMG_SYNOP_CR */
		/** Potential enhancement - consolidate this 'if' statement */
        if (core_if->op_state == B_HOST) 
		{
            /* If in device mode Disconnect and stop the HCD, then
        	 * start the PCD. */
            hcd_disconnect( core_if );
            pcd_start( core_if );
            core_if->op_state = B_PERIPHERAL;
        } 
		else if (dwc_otg_is_device_mode(core_if))
		{
            gotgctl_data_t gotgctl = { 0 }; 
            gotgctl.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GOTGCTL_OFFSET);
            if (gotgctl.b.hstsethnpen==1) 
			{
                /* Do nothing, if HNP in process the OTG
                 * interrupt "Host Negotiation Detected"
                 * interrupt will do the mode switch.
                 */
            } 
			else if (gotgctl.b.devhnpen == 0) 
			{
                /* If in device mode Disconnect and stop the HCD, then
                 * start the PCD. */
                hcd_disconnect( core_if );
                pcd_start( core_if );
                core_if->op_state = B_PERIPHERAL;
            } 
			else 
			{
            	DWC_DEBUGPL(DBG_ANY,"!a_peripheral && !devhnpen\n");
            }
        }
		else 
		{
                if (core_if->op_state == A_HOST) 
				{
                    // A-Cable still connected but device disconnected.
                    hcd_disconnect( core_if );
                }
        }
   
	gintsts.d32 = 0;
	gintsts.b.disconnect = 1;
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}
#endif /* !DWC_DEVICE_ONLY */

/**
 * This interrupt indicates that SUSPEND state has been detected on
 * the USB.
 * 
 * For HNP the USB Suspend interrupt signals the change from
 * "a_peripheral" to "a_host".
 *
 * When power management is enabled the core will be put in low power
 * mode.
 */
IMG_INT32 dwc_otg_handle_usb_suspend_intr( const img_uint32 ui32BaseAddress, dwc_otg_core_if_t *core_if )
{
    dsts_data_t dsts;
    gintsts_data_t gintsts;

    DWC_DEBUGPL(DBG_ANY,"USB SUSPEND\n");

	if (dwc_otg_is_device_mode( ui32BaseAddress ))
	{
    	/* Check the Device status register to determine if the Suspend
		* state is active. */
    	dsts.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DSTS_OFFSET);

    	DWC_DEBUGPL(DBG_PCD, "DSTS=0x%0x\n", dsts.d32);
    	DWC_DEBUGPL(DBG_PCD, "DSTS.Suspend Status=%d " "HWCFG4.power Optimize=%d\n", dsts.b.suspsts, core_if->hwcfg4.b.power_optimiz);


  #if defined(PARTIAL_POWER_DOWN)
  	/* IMG_SYNOP_CR */
	/* Potential enhancement - add a module parameter for power management. */

	    if (dsts.b.suspsts && core_if->hwcfg4.b.power_optimiz) 
		{
			pcgcctl_data_t power = {0};
	        DWC_DEBUGPL(DBG_CIL, "suspend\n");
	
	        power.b.pwrclmp = 1;
	        usb_WriteReg32( ui32BaseAddress, DWC_OTG_PCGCCTL_OFFSET, power.d32);
	
	        power.b.rstpdwnmodule = 1;
	        usb_ModifyReg32(DWC_OTG_PCGCCTL_OFFSET, 0, power.d32);
	
	        power.b.stoppclk = 1;
	        usb_ModifyReg32(DWC_OTG_PCGCCTL_OFFSET, 0, power.d32);
	    
	    } 
		else 
		{
			DWC_DEBUGPL(DBG_ANY,"disconnect?\n");
	    }
  #endif

		/* PCD callback for suspend. */
		pcd_suspend( core_if );
	}
#if !defined DWC_DEVICE_ONLY
	else if ( core_if->op_state == A_PERIPHERAL )
	{
		DWC_DEBUGPL(DBG_ANY,"a_peripheral->a_host\n");
  		/* Clear the a_peripheral flag, back to a_host. */
        pcd_stop(core_if);
        hcd_start(core_if);
        core_if->op_state = A_HOST;
	}
#endif

	/* Change to L2(suspend) state */
	core_if->lx_state = DWC_OTG_L2;
        
	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.usbsuspend = 1;
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);

	return 1;
}


/**
 * Common interrupt handler.
 *
 * The common interrupts are those that occur in both Host and Device mode. 
 * This handler handles the following interrupts:
 * - Mode Mismatch Interrupt
 * - Disconnect Interrupt
 * - OTG Interrupt
 * - Connector ID Status Change Interrupt
 * - Session Request Interrupt.
 * - Resume / Remote Wakeup Detected Interrupt.
 * 
 */
 
extern IMG_INT32 dwc_otg_handle_common_intr( const img_uint32 ui32BaseAddress, dwc_otg_core_if_t	*	core_if )
{
#define GINTMASK_COMMON		(0xF1000806)		/* mask constructed from the code in dwc_otg_read_common_intr */

	IMG_INT32			retval = 0;
	gintsts_data_t		gintsts;
	gintmsk_data_t		gintmsk;

	gintsts.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET);
	gintmsk.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET);

	gintsts.d32 = ((gintsts.d32 & gintmsk.d32 ) & GINTMASK_COMMON);
	
	if (!gintsts.d32)
	{
		return 0;
	}

  #if !defined DWC_DEVICE_ONLY
    if (gintsts.b.modemismatch)
	{
		retval |= dwc_otg_handle_mode_mismatch_intr( );		
    }
  #endif

  #if defined DWC_SUPPORT_OTG 
	if (gintsts.b.otgintr)
	{
		retval |= dwc_otg_handle_otg_intr( );
    }
  #endif

  #if defined DWC_SUPPORT_OTG
    if (gintsts.b.conidstschng)
	{
		retval |= dwc_otg_handle_conn_id_status_change_intr( );
    }
  #endif

  #if !defined DWC_DEVICE_ONLY
    if (gintsts.b.disconnect)
	{
		retval |= dwc_otg_handle_disconnect_intr( );
    }
  #endif

  #if defined DWC_SUPPORT_OTG
    if (gintsts.b.sessreqintr)
	{		
		retval |= dwc_otg_handle_session_req_intr( );
    }
  #endif
    if (gintsts.b.wkupintr)
	{
            retval |= dwc_otg_handle_wakeup_detected_intr( ui32BaseAddress, core_if );
    }
    if (gintsts.b.usbsuspend)
	{
            retval |= dwc_otg_handle_usb_suspend_intr( ui32BaseAddress, core_if );
    }
    if (gintsts.b.portintr /*&& dwc_otg_is_device_mode() commented out by Bojan */) 
	{
            /* The port interrupt occurs while in device mode with HPRT0
                * Port Enable/Disable.
                */
            gintsts.d32 = 0;
            gintsts.b.portintr = 1;
            usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, gintsts.d32);
            retval |= 1;
            
    }

    return retval;
}
