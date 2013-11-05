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
 * The CIL manages the memory map for the core so that the HCD and PCD
 * don't have to do this separately. It also handles basic tasks like
 * reading/writing the registers and data FIFOs in the controller.
 * Some of the data access functions provide encapsulation of several
 * operations required to perform a task, such as writing multiple
 * registers to start a transfer. Finally, the CIL performs basic
 * services that are not specific to either the host or device modes
 * of operation. These services include management of the OTG Host
 * Negotiation Protocol (HNP) and Session Request Protocol (SRP). A
 * Diagnostic API is also provided to allow testing of the controller
 * hardware.
 *
 * The Core Interface Layer has the following requirements:
 * - Provides basic controller operations.
 * - Minimal use of OS services.  
 * - The OS services used will be abstracted by using __USBD_INLINE__ functions
 *	 or macros.
 *
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
#include "usb_defs.h"


/**
 * This function frees the structures allocated by dwc_otg_cil_init().
 * 
 * @param[in] core_if The core interface pointer returned from
 * dwc_otg_cil_init().
 *
 */
IMG_VOID dwc_otg_cil_remove( const img_uint32 ui32BaseAddress )
{
	/* Disable all interrupts */
	usb_ModifyReg32( ui32BaseAddress,  DWC_OTG_CORE_GLOBAL_REGS_GAHBCFG_OFFSET, 1, 0);
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, 0);
}

/**
 * This function enables the controller's Global Interrupt in the AHB Config
 * register.
 *
 * @param[in] core_if Programming view of DWC_otg controller.
 */
extern IMG_VOID dwc_otg_enable_global_interrupts( const img_uint32 ui32BaseAddress  )
{
	gahbcfg_data_t ahbcfg = {0};
	ahbcfg.b.glblintrmsk = 1; /* Enable interrupts */
	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GAHBCFG_OFFSET, 0, ahbcfg.d32);
}

/**
 * This function disables the controller's Global Interrupt in the AHB Config
 * register.
 *
 * @param[in] core_if Programming view of DWC_otg controller.
 */
extern IMG_VOID dwc_otg_disable_global_interrupts( const img_uint32 ui32BaseAddress  )
{
	gahbcfg_data_t ahbcfg = {0};
	ahbcfg.b.glblintrmsk = 1; /* Enable interrupts */
	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GAHBCFG_OFFSET, ahbcfg.d32, 0);
}

/**
 * This function initializes the commmon interrupts, used in both
 * device and host modes.
 *
 * @param[in] core_if Programming view of the DWC_otg controller
 *
 */
static IMG_VOID dwc_otg_enable_common_interrupts( const img_uint32 ui32BaseAddress, dwc_otg_core_if_t	*	core_if )
{
	gintmsk_data_t				intr_mask		= {0};
	
	/* Clear any pending OTG Interrupts */
	usb_WriteReg32( ui32BaseAddress,  DWC_OTG_CORE_GLOBAL_REGS_GOTGINT_OFFSET, 0xFFFFFFFF); 
	
	/* Clear any pending interrupts */
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, 0xFFFFFFFF); 
	
	/* 
	 * Enable the interrupts in the GINTMSK. 
	 */
  #if !defined DWC_DEVICE_ONLY
	intr_mask.b.modemismatch = 1;
	intr_mask.b.otgintr = 1;
	intr_mask.b.conidstschng = 1;
	intr_mask.b.disconnect = 1;
	intr_mask.b.sessreqintr = 1;
  #endif

  #if !defined USE_DMA_INTERNAL	
	if (!core_if->dma_enable) 
	{
		intr_mask.b.rxstsqlvl = 1;
	}
  #endif /* USE_DMA_INTERNAL */

  #if !defined IGNORE_SUSPEND
	intr_mask.b.wkupintr = 1;
	intr_mask.b.usbsuspend = 1;
  #endif
	
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32);
}

/**
 * Initializes the FSLSPClkSel field of the HCFG register depending on the PHY
 * type.
 */
#if !defined DWC_DEVICE_ONLY
static void init_fslspclksel(dwc_otg_core_if_t *core_if)
{
	uint32_t	val;
	hcfg_data_t		hcfg;

	if (((core_if->hwcfg2.b.hs_phy_type == 2) &&
		 (core_if->hwcfg2.b.fs_phy_type == 1) &&
		 (core_if->core_params.ulpi_fs_ls)) ||
		(core_if->core_params.phy_type == DWC_PHY_TYPE_PARAM_FS)) {
		/* Full speed PHY */
		val = DWC_HCFG_48_MHZ;
	} 
	else {
		/* High speed PHY running at full speed or high speed */
		val = DWC_HCFG_30_60_MHZ;
	}

	DWC_DEBUGPL(DBG_CIL, "Initializing HCFG.FSLSPClkSel to 0x%1x\n", val);
	hcfg.d32 = usb_ReadReg32( ui32BaseAddress, &core_if->host_if->host_global_regs->hcfg);
	hcfg.b.fslspclksel = val;
	usb_WriteReg32( ui32BaseAddress, &core_if->host_if->host_global_regs->hcfg, hcfg.d32);
}
#endif

/**
 * Initializes the DevSpd field of the DCFG register depending on the PHY type
 * and the enumeration speed of the device.
 */
static IMG_VOID init_devspd( const img_uint32 ui32BaseAddress, dwc_otg_core_if_t	*	core_if )
{
	IMG_UINT32			val;
	dcfg_data_t			dcfg;

	if (((core_if->hwcfg2.b.hs_phy_type == 2) &&
		 (core_if->hwcfg2.b.fs_phy_type == 1) &&
		 (core_if->core_params.ulpi_fs_ls)) ||
		 (core_if->core_params.phy_type == DWC_PHY_TYPE_PARAM_FS)) 
	{
		/* Full speed PHY */
		val = 0x3;
	}
	else if (core_if->core_params.speed == DWC_SPEED_PARAM_FULL) 
	{
		/* High speed PHY running at full speed */
		val = 0x1;
	} 
	else 
	{
		/* High speed PHY running at high speed */
		val = 0x0;
	}

	DWC_DEBUGPL(DBG_CIL, "Initializing DCFG.DevSpd to 0x%1x\n", val);
	
	dcfg.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DCFG_OFFSET);
	dcfg.b.devspd = val;
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DCFG_OFFSET, dcfg.d32);
}

/**
 * This function calculates the number of IN EPS
 * using GHWCFG1 and GHWCFG2 registers values 
 *
 * @param _pcd the pcd structure.
 */
static IMG_UINT32 calc_num_in_eps( dwc_otg_core_if_t	*core_if )
{
	IMG_UINT32			num_in_eps		= 0;
	IMG_UINT32			num_eps			= core_if->hwcfg2.b.num_dev_ep;
	IMG_UINT32			hwcfg1			= core_if->hwcfg1.d32 >> 3;
	IMG_UINT32			num_tx_fifos	= core_if->hwcfg4.b.num_in_eps;
	IMG_INT32					i;
	
	
	for (i = 0; i < num_eps; ++i)
	{
		if(!(hwcfg1 & 0x1))
		{
			num_in_eps++;
		}
		hwcfg1 >>= 2;
	}
	
	if(core_if->hwcfg4.b.ded_fifo_en)
	{
		num_in_eps = (num_in_eps > num_tx_fifos) ? num_tx_fifos : num_in_eps; 
	}
	
	return num_in_eps;
}


/**
 * This function calculates the number of OUT EPS
 * using GHWCFG1 and GHWCFG2 registers values 
 *
 * @param _pcd the pcd structure.
 */
static IMG_UINT32 calc_num_out_eps( dwc_otg_core_if_t	*core_if  )
{
	IMG_UINT32			num_out_eps	= 0;
	IMG_UINT32			num_eps		= core_if->hwcfg2.b.num_dev_ep;
	IMG_UINT32			hwcfg1		= core_if->hwcfg1.d32 >> 2;
	IMG_INT32					i;
	
	for (i = 0; i < num_eps; ++i)
	{
		if(!(hwcfg1 & 0x1))
		{
			num_out_eps++;
		}

		hwcfg1 >>= 2;
	}
	return num_out_eps;
}
/**
 * This function initializes the DWC_otg controller registers and
 * prepares the core for device mode or host mode operation.
 *
 * @param core_if Programming view of the DWC_otg controller
 *
 */
IMG_VOID dwc_otg_core_init( const img_uint32 ui32BaseAddress, dwc_otg_core_if_t	*core_if ) 
{
	dwc_otg_dev_if_t            *dev_if			= &(core_if->dev_if);
	gahbcfg_data_t              ahbcfg			= {0};
	gusbcfg_data_t              usbcfg			= {0};
	dctl_data_t                 dctl 			= {0};

	DWC_DEBUGPL(DBG_CILV, "dwc_otg_core_init(%p)\n",core_if);

	/* Common Initialization */
	usbcfg.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET);

	/* Program the ULPI External VBUS bit if needed */
	usbcfg.b.ulpi_ext_vbus_drv = (core_if->core_params.phy_ulpi_ext_vbus == DWC_PHY_ULPI_EXTERNAL_VBUS) ? 1 : 0;

	/* Set external TS Dline pulsing */
	usbcfg.b.term_sel_dl_pulse = (core_if->core_params.ts_dline == 1) ? 1 : 0;
	usb_WriteReg32 ( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET, usbcfg.d32);

	/* Reset the Controller */
	dwc_otg_core_reset( ui32BaseAddress );

	/* Initialize parameters from Hardware configuration registers. */
	dev_if->num_in_eps = calc_num_in_eps( core_if );
	dev_if->num_out_eps = calc_num_out_eps( core_if );
	
	/* THW - we restrict the maximum number of endpoints to save space. This is our opportunity to ensure that the 	*/
	/*		 value required by the h/w does not exceed the amount of static storage we've allocated to endpoints.	*/
	IMG_ASSERT ( dev_if->num_in_eps <= (MAX_EPS_CHANNELS-1) );
	IMG_ASSERT ( dev_if->num_out_eps <= (MAX_EPS_CHANNELS-1) );
	
	DWC_DEBUGPL(DBG_CIL, "num_dev_perio_in_ep=%d\n",core_if->hwcfg4.b.num_dev_perio_in_ep);
	
/*
	//Bojan: perio_tx_fifo_size[] and tx_fifo_size[] are not used at all in the code... I will remove them for now

	for (i=0; i < core_if->hwcfg4.b.num_dev_perio_in_ep; i++) 
	{
		dev_if->perio_tx_fifo_size[i] = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_DPTXFSIZ_DIEPTXF_OFFSET + 4*i) >> 16;

		DWC_DEBUGPL(DBG_CIL, "Periodic Tx FIFO SZ #%d=0x%0x\n", i, dev_if->perio_tx_fifo_size[i]);	
	}
		
	for (i=0; i < core_if->hwcfg4.b.num_in_eps; i++) 
	{
		dev_if->tx_fifo_size[i] = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_DPTXFSIZ_DIEPTXF_OFFSET + 4*i) >> 16;

		DWC_DEBUGPL(DBG_CIL, "Tx FIFO SZ #%d=0x%0x\n", i, dev_if->perio_tx_fifo_size[i]);	
	}
*/

	core_if->total_fifo_size = core_if->hwcfg3.b.dfifo_depth;
	core_if->rx_fifo_size = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GRXFSIZ_OFFSET);
	core_if->nperio_tx_fifo_size = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GNPTXFSIZ_OFFSET) >> 16;
		
	DWC_DEBUGPL(DBG_CIL, "Total FIFO SZ=%d\n", core_if->total_fifo_size);
	DWC_DEBUGPL(DBG_CIL, "Rx FIFO SZ=%d\n", core_if->rx_fifo_size);
	DWC_DEBUGPL(DBG_CIL, "NP Tx FIFO SZ=%d\n", core_if->nperio_tx_fifo_size);

	/* This programming sequence needs to happen in FS mode before any other
	 * programming occurs */
	if ((core_if->core_params.speed == DWC_SPEED_PARAM_FULL) &&
		(core_if->core_params.phy_type == DWC_PHY_TYPE_PARAM_FS)) 
	{
		/* If FS mode with FS PHY */

		/* core_init() is now called on every switch so only call the
		 * following for the first time through. */
		if (!core_if->phy_init_done) 
		{
			core_if->phy_init_done = 1;
			DWC_DEBUGPL(DBG_CIL, "FS_PHY detected\n");
			usbcfg.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET);
			usbcfg.b.physel = 1;
			usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET, usbcfg.d32);

			/* Reset after a PHY select */
			dwc_otg_core_reset( ui32BaseAddress );
		}

		/* Program DCFG.DevSpd or HCFG.FSLSPclkSel to 48Mhz in FS.	Also
		 * do this on HNP Dev/Host mode switches (done in dev_init and
		 * host_init). */

	  #if !defined DWC_DEVICE_ONLY  //Bojan
		if (dwc_otg_is_host_mode()) 
		{
			init_fslspclksel(core_if);
		}
		else
	  #endif
		{
			init_devspd( ui32BaseAddress, core_if );
		}
	  #if defined USE_I2C
		if (core_if->core_params.i2c_enable) 
		{
			gi2cctl_data_t				i2cctl			= {0};

			DWC_DEBUGPL(DBG_CIL, "FS_PHY Enabling I2c\n");
			/* Program GUSBCFG.OtgUtmifsSel to I2C */
			usbcfg.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET);
			usbcfg.b.otgutmifssel = 1;
			usb_WriteReg32 (DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET, usbcfg.d32);
				
			/* Program GI2CCTL.I2CEn */
			i2cctl.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GI2CCTL_OFFSET);
			i2cctl.b.i2cdevaddr = 1;
			i2cctl.b.i2cen = 0;
			usb_WriteReg32 (DWC_OTG_CORE_GLOBAL_REGS_GI2CCTL_OFFSET, i2cctl.d32);
			i2cctl.b.i2cen = 1;
			usb_WriteReg32 (DWC_OTG_CORE_GLOBAL_REGS_GI2CCTL_OFFSET, i2cctl.d32);
		}
	  #else
		IMG_ASSERT(core_if->core_params.i2c_enable == 0); 
	  #endif
	} /* endif speed == DWC_SPEED_PARAM_FULL */
	else 
	{
		/* High speed PHY. */
		if (!core_if->phy_init_done) 
		{
			core_if->phy_init_done = 1;
			/* HS PHY parameters.  These parameters are preserved
			 * during soft reset so only program the first time.  Do
			 * a soft reset immediately after setting phyif.  */
			if ( core_if->core_params.phy_type == DWC_PHY_TYPE_PARAM_ULPI )
			{
				/* ULPI interface */
				usbcfg.b.ulpi_utmi_sel = 1;
				usbcfg.b.phyif = 0;
				usbcfg.b.ddrsel = core_if->core_params.phy_ulpi_ddr;
			} 
			else if ( core_if->core_params.phy_type == DWC_PHY_TYPE_PARAM_UTMI )
			{
				/* UTMI+ interface */
				usbcfg.b.ulpi_utmi_sel = 0;
				if (core_if->core_params.phy_utmi_width == 16) 
				{
					usbcfg.b.phyif = 1;
				} 
				else 
				{
					usbcfg.b.phyif = 0;
				}
			}
			else
			{
				DWC_ERROR("FS PHY TYPE\n");
			}

			usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET, usbcfg.d32);

			/* Reset after setting the PHY parameters */
			dwc_otg_core_reset( ui32BaseAddress );
		}
	}

	if ((core_if->hwcfg2.b.hs_phy_type == 2) &&
		(core_if->hwcfg2.b.fs_phy_type == 1) &&
		(core_if->core_params.ulpi_fs_ls)) 
	{
		DWC_DEBUGPL(DBG_CIL, "Setting ULPI FSLS\n");
		usbcfg.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET);
		usbcfg.b.ulpi_fsls = 1;
		usbcfg.b.ulpi_clk_sus_m = 1;
		usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET, usbcfg.d32);
	}
	else 
	{
		usbcfg.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET);
		usbcfg.b.ulpi_fsls = 0;
		usbcfg.b.ulpi_clk_sus_m = 0;
		usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET, usbcfg.d32);
	}

	/* Program the GAHBCFG Register.*/
	switch (core_if->hwcfg2.b.architecture)
	{
#if !defined USE_DMA_INTERNAL
	case DWC_SLAVE_ONLY_ARCH:
		DWC_DEBUGPL(DBG_CIL, "Slave Only Mode\n");
		ahbcfg.b.nptxfemplvl_txfemplvl = DWC_GAHBCFG_TXFEMPTYLVL_HALFEMPTY;
		ahbcfg.b.ptxfemplvl = DWC_GAHBCFG_TXFEMPTYLVL_HALFEMPTY;
		core_if->dma_enable = 0;
		core_if->dma_desc_enable = 0;
		break;

	case DWC_EXT_DMA_ARCH:
		DWC_DEBUGPL(DBG_CIL, "External DMA Mode\n");
		{
			IMG_UINT8	brst_sz = core_if->core_params.dma_burst_size;  
			ahbcfg.b.hburstlen = 0;
			while (brst_sz > 1)
			{
				ahbcfg.b.hburstlen++;
				brst_sz >>= 1;
			}
		}
		core_if->dma_enable = (core_if->core_params.dma_enable != 0);
		core_if->dma_desc_enable = (core_if->core_params.dma_desc_enable != 0);
		break;
#endif /* USE_DMA_INTERNAL */
	case DWC_INT_DMA_ARCH:
		DWC_DEBUGPL(DBG_CIL, "Internal DMA Mode\n");
		ahbcfg.b.hburstlen = DWC_GAHBCFG_INT_DMA_BURST_INCR;
		core_if->dma_enable = (core_if->core_params.dma_enable != 0);
		core_if->dma_desc_enable = (core_if->core_params.dma_desc_enable != 0);
		break;
	default:
		IMG_ASSERT(0);
		break;
	}

	ahbcfg.b.dmaenable = core_if->dma_enable;
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GAHBCFG_OFFSET, ahbcfg.d32);

	core_if->en_multiple_tx_fifo = core_if->hwcfg4.b.ded_fifo_en;
#if defined USE_MULTIPLE_TX_FIFO
	IMG_ASSERT(core_if->en_multiple_tx_fifo && core_if->core_params.en_multiple_tx_fifo);
#endif

	core_if->pti_enh_enable = core_if->core_params.pti_enable != 0;
	core_if->multiproc_int_enable = core_if->core_params.mpi_enable;
	DWC_PRINT("Periodic Transfer Interrupt Enhancement - %s\n",
		   ((core_if->pti_enh_enable) ? "enabled" : "disabled"));
	DWC_PRINT("Multiprocessor Interrupt Enhancement - %s\n",
		   ((core_if->multiproc_int_enable) ? "enabled" : "disabled"));
	
	/* 
	 * Program the GUSBCFG register. 
	 */
	usbcfg.d32 = usb_ReadReg32( ui32BaseAddress,  DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET );

#if defined DWC_SUPPORT_OTG
	switch (core_if->hwcfg2.b.op_mode) 
	{
	case DWC_MODE_HNP_SRP_CAPABLE:
		usbcfg.b.hnpcap = (core_if->core_params.otg_cap == DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE);
		usbcfg.b.srpcap = (core_if->core_params.otg_cap != DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE);
		break;

	case DWC_MODE_SRP_ONLY_CAPABLE:
		usbcfg.b.hnpcap = 0;
		usbcfg.b.srpcap = (core_if->core_params.otg_cap != DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE);
		break;

	case DWC_MODE_NO_HNP_SRP_CAPABLE:
		usbcfg.b.hnpcap = 0;
		usbcfg.b.srpcap = 0;
		break;

	case DWC_MODE_SRP_CAPABLE_DEVICE:
		usbcfg.b.hnpcap = 0;
		usbcfg.b.srpcap = (core_if->core_params.otg_cap != DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE);
		break;

	case DWC_MODE_NO_SRP_CAPABLE_DEVICE:
		usbcfg.b.hnpcap = 0;
		usbcfg.b.srpcap = 0;
		break;

	case DWC_MODE_SRP_CAPABLE_HOST:
		usbcfg.b.hnpcap = 0;
		usbcfg.b.srpcap = (core_if->core_params.otg_cap != DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE);
		break;

	case DWC_MODE_NO_SRP_CAPABLE_HOST:
		usbcfg.b.hnpcap = 0;
		usbcfg.b.srpcap = 0;
		break;
	}
#else
	if (core_if->hwcfg2.b.op_mode == DWC_MODE_NO_SRP_CAPABLE_DEVICE)
	{
		usbcfg.b.hnpcap = 0;
		usbcfg.b.srpcap = 0;
	}
	else if (core_if->hwcfg2.b.op_mode == DWC_MODE_HNP_SRP_CAPABLE)
	{
		usbcfg.b.hnpcap = 0;
		usbcfg.b.srpcap = 0;
	}
	else
	{
		IMG_ASSERT(0);	//no support for other modes
	}
#endif

	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GUSBCFG_OFFSET, usbcfg.d32);
	
	/* clear Soft Disconnect  - reproduces a fix to the initialization procedure implemented for the Boot Rom
	 * THIS CHANGE HAS NOT BEEN TESTED FOR THE GENERAL USE OF DRIVERS, ONLY FOR BOOT ROM !!!*/
	dctl.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET);
	dctl.b.sftdiscon = 0;
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, dctl.d32);

	/* Enable common interrupts */
	dwc_otg_enable_common_interrupts( ui32BaseAddress, core_if );

	/* Do device or host intialization based on mode during PCD
	 * and HCD initialization  */
  #if !defined DWC_DEVICE_ONLY   //Bojan
	if (dwc_otg_is_host_mode( )) 
	{
		DWC_DEBUGPL(DBG_ANY, "Host Mode\n" );
		core_if->op_state = A_HOST;
	}
	else
  #endif
	{
		DWC_DEBUGPL(DBG_ANY, "Device Mode\n" );
		core_if->op_state = B_PERIPHERAL;
	  #if defined DWC_DEVICE_ONLY
		dwc_otg_core_dev_init( ui32BaseAddress, core_if );
	  #endif
	}
}


/** 
 * This function enables the Device mode interrupts.
 *
 * @param core_if Programming view of DWC_otg controller
 */
IMG_VOID dwc_otg_enable_device_interrupts( const img_uint32 ui32BaseAddress, dwc_otg_core_if_t	*	core_if )
{
	gintmsk_data_t				intr_mask		= {0};


	DWC_DEBUGPL(DBG_CIL, "%s()\n", __func__);

	/* Disable all interrupts. */
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, 0);
	
	/* Clear any pending interrupts */
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, 0xFFFFFFFF); 

	/* Enable the common interrupts */
	dwc_otg_enable_common_interrupts( ui32BaseAddress, core_if );

	/* Enable interrupts */
	intr_mask.b.usbreset = 1;
	intr_mask.b.enumdone = 1;
	intr_mask.b.inepintr = 1;
	intr_mask.b.outepintr = 1;
#if !defined IGNORE_SUSPEND
	intr_mask.b.erlysuspend = 1;
#endif

#if !defined USE_MULTIPLE_TX_FIFO
	if(core_if->en_multiple_tx_fifo == 0)
	{
		intr_mask.b.epmismatch = 1;
	}
#endif

/* Enable the ignore frame number for ISOC xfers - MAS */
#if defined (_PER_IO_)
	if (core_if->dma_enable)
	{
		if (core_if->dma_desc_enable)
		{
			dctl_data_t	dctl1 = { 0 };
			dctl1.b.ifrmnum = 1;
			usb_ModifyReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, 0, dctl1.d32 );
		}
	}
#endif /* _PER_IO_ */
#if defined (_EN_ISOC_)
  #if !defined (USE_DMA_INTERNAL)
	if (core_if->dma_enable) 
  #endif /* USE_DMA_INTERNAL */
  #if !defined (USE_DDMA_ONLY)
	{
		if (core_if->dma_desc_enable == 0) 
		{
			if (core_if->pti_enh_enable) 
			{
				dctl_data_t dctl = {0};
				dctl.b.ifrmnum = 1;
				usb_ModifyReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, 0, dctl.d32 );
			} 
			else 
			{
				intr_mask.b.incomplisoin = 1;
				intr_mask.b.incomplisoout = 1;
			}
		}
	} 
  #endif /* USE_DDMA_ONLY */
  #if !defined USE_DMA_INTERNAL
	else 
	{
		intr_mask.b.incomplisoin = 1;
		intr_mask.b.incomplisoout = 1;
	}
  #endif /* USE_DMA_INTERNAL */
#endif				/* _EN_ISOC_ */

	/* IMG_SYNOP_CR */
	/** Potential enhancement: This could be made into a module parameter */
#if defined USE_PERIODIC_EP
	intr_mask.b.isooutdrop = 1;
	intr_mask.b.eopframe = 1;
	intr_mask.b.incomplisoin = 1;
	intr_mask.b.incomplisoout = 1;
#endif

	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, intr_mask.d32);

	DWC_DEBUGPL(DBG_CIL, "%s() gintmsk=%0x\n", __func__, usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET));
}

/**
 * This function initializes the DWC_otg controller registers for
 * device mode.
 * 
 * @param core_if Programming view of DWC_otg controller
 *
 */
IMG_VOID dwc_otg_core_dev_init( const img_uint32 ui32BaseAddress, dwc_otg_core_if_t	*	core_if )
{
	IMG_INT32					i;
	dwc_otg_dev_if_t			*dev_if			= &(core_if->dev_if);
	dwc_otg_core_params_t		*params			= &(core_if->core_params);
	dcfg_data_t					dcfg			= {0};
	grstctl_t					resetctl		= {0};
	IMG_UINT32					rx_fifo_size;
	fifosize_data_t				nptxfifosize;
	fifosize_data_t				txfifosize;
	dthrctl_data_t				dthrctl;
		
	/* Restart the Phy Clock */
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_PCGCCTL_OFFSET, 0);
	
	/* Device configuration register */
	init_devspd( ui32BaseAddress, core_if );
	dcfg.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DCFG_OFFSET);
	dcfg.b.descdma = (core_if->dma_desc_enable) ? 1 : 0;
	dcfg.b.perfrint = DWC_DCFG_FRAME_INTERVAL_80;
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DCFG_OFFSET, dcfg.d32 );

	/* Configure data FIFO sizes */
	if ( core_if->hwcfg2.b.dynamic_fifo && params->enable_dynamic_fifo ) 
	{
		DWC_DEBUGPL(DBG_CIL, "Total FIFO Size=%d\n", core_if->total_fifo_size);
		DWC_DEBUGPL(DBG_CIL, "Rx FIFO Size=%d\n", params->dev_rx_fifo_size);
		DWC_DEBUGPL(DBG_CIL, "NP Tx FIFO Size=%d\n", params->dev_nperio_tx_fifo_size);

		/* Rx FIFO */
		DWC_DEBUGPL(DBG_CIL, "initial grxfsiz=%08x\n", usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GFXSIZ_OFFSET ));
		
		rx_fifo_size = params->dev_rx_fifo_size;
		usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GRXFSIZ_OFFSET, rx_fifo_size );
		
		DWC_DEBUGPL(DBG_CIL, "new grxfsiz=%08x\n", usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GRXFSIZ_OFFSET));
		
		/** Set Periodic Tx FIFO Mask all bits 0 */
		core_if->p_tx_msk = 0;
		
		/** Set Tx FIFO Mask all bits 0 */
		core_if->tx_msk = 0;

#if !defined USE_MULTIPLE_TX_FIFO
		if(core_if->en_multiple_tx_fifo == 0)
		{
			fifosize_data_t				ptxfifosize;

			/* Non-periodic Tx FIFO */
			nptxfifosize.b.depth  = params->dev_nperio_tx_fifo_size;
			nptxfifosize.b.startaddr = params->dev_rx_fifo_size;
			
			usb_WriteReg32( ui32BaseAddress,  DWC_OTG_CORE_GLOBAL_REGS_GNPTXFSIZ_OFFSET, nptxfifosize.d32 );
			
			/* IMG_SYNOP_CR */
			/* Potential enhancement: Examine current strategy for period FIFO sizing */
			/*
			 * Periodic Tx FIFOs These FIFOs are numbered from 1 to 15.
			 * Indexes of the FIFO size module parameters in the
			 * dev_perio_tx_fifo_size array and the FIFO size registers in
			 * the dptxfsiz array run from 0 to 14.
			 */
			ptxfifosize.b.startaddr = nptxfifosize.b.startaddr + nptxfifosize.b.depth;
			for (i=0; i < core_if->hwcfg4.b.num_dev_perio_in_ep; i++) 
			{
				ptxfifosize.b.depth = params->dev_tx_fifo_size[i]; /*params->dev_perio_tx_fifo_size[i];*/
				usb_WriteReg32( ui32BaseAddress,  DWC_OTG_CORE_GLOBAL_REGS_DPTXFSIZ_DIEPTXF_OFFSET + 4*i, ptxfifosize.d32 );
				ptxfifosize.b.startaddr += ptxfifosize.b.depth;
			}
		}
		else
#endif //#if !defined USE_MULTIPLE_TX_FIFO
		{
			/*
			 * Tx FIFOs These FIFOs are numbered from 1 to 15.
			 * Indexes of the FIFO size module parameters in the
			 * dev_tx_fifo_size array and the FIFO size registers in
			 * the dptxfsiz_dieptxf array run from 0 to 14.
			 */
					
			/* Non-periodic Tx FIFO */
			nptxfifosize.b.depth  = params->dev_nperio_tx_fifo_size;
			nptxfifosize.b.startaddr = params->dev_rx_fifo_size;
			
			usb_WriteReg32( ui32BaseAddress,  DWC_OTG_CORE_GLOBAL_REGS_GNPTXFSIZ_OFFSET, nptxfifosize.d32 );
				
			txfifosize.b.startaddr = nptxfifosize.b.startaddr + nptxfifosize.b.depth;
			for (i=0; i < core_if->hwcfg4.b.num_in_eps; i++) 
			{
				txfifosize.b.depth = params->dev_tx_fifo_size[i];	
				usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_DPTXFSIZ_DIEPTXF_OFFSET + 4*i, txfifosize.d32 );
				txfifosize.b.startaddr += txfifosize.b.depth;
			}
		}
	}
	/* Flush the FIFOs */
	dwc_otg_flush_tx_fifo( ui32BaseAddress, 0x10); /* all Tx FIFOs */
	dwc_otg_flush_rx_fifo( ui32BaseAddress );

	/* Flush the Learning Queue. */
	resetctl.b.intknqflsh = 1;
	usb_WriteReg32( ui32BaseAddress,  DWC_OTG_CORE_GLOBAL_REGS_GRSTCTL_OFFSET, resetctl.d32 );

	/* Clear all pending Device Interrupts */
	usb_WriteReg32( ui32BaseAddress,  DWC_OTG_DEV_GLOBAL_REGS_DIEPMSK_OFFSET, 0 );
	usb_WriteReg32( ui32BaseAddress,  DWC_OTG_DEV_GLOBAL_REGS_DOEPMSK_OFFSET, 0 );
	usb_WriteReg32( ui32BaseAddress,  DWC_OTG_DEV_GLOBAL_REGS_DAINT_OFFSET, 0xFFFFFFFF );
	usb_WriteReg32( ui32BaseAddress,  DWC_OTG_DEV_GLOBAL_REGS_DAINTMSK_OFFSET, 0 );

	for (i = 0; i <= dev_if->num_in_eps; i++) 
	{
		depctl_data_t depctl;
		depctl.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + i*DWC_EP_REG_OFFSET);
		if (depctl.b.epena) 
		{
			depctl.d32 = 0;
			depctl.b.epdis = 1;
			depctl.b.snak = 1;
		} 
		else 
		{
			depctl.d32 = 0;
		}
		
		usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + i*DWC_EP_REG_OFFSET, depctl.d32);
		
	  #if !defined (USE_DDMA_ONLY)
		usb_WriteReg32( ui32BaseAddress,  DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + i*DWC_EP_REG_OFFSET, 0);
	  #endif
		usb_WriteMemrefReg32( ui32BaseAddress,  DWC_OTG_DEV_IN_EP_REGS_DIEPDMA0 + i*DWC_EP_REG_OFFSET, 0);
		usb_WriteReg32( ui32BaseAddress,  DWC_OTG_DEV_IN_EP_REGS_DIEPINT0 + i*DWC_EP_REG_OFFSET, 0xFF);
	}
	
	for (i=0; i <= dev_if->num_out_eps; i++) 
	{
		depctl_data_t depctl;
		depctl.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + i*DWC_EP_REG_OFFSET);
		if (depctl.b.epena) 
		{
			depctl.d32 = 0;
			depctl.b.epdis = 1;
			depctl.b.snak = 1;
		} 
		else 
		{
			depctl.d32 = 0;
		}
		
		usb_WriteReg32( ui32BaseAddress,  DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + i*DWC_EP_REG_OFFSET, depctl.d32);

	  #if !defined (USE_DDMA_ONLY)
		usb_WriteReg32( ui32BaseAddress,  DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + i*DWC_EP_REG_OFFSET, 0);
	  #endif
		usb_WriteReg32( ui32BaseAddress,  DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0 + i*DWC_EP_REG_OFFSET, 0);
		usb_WriteReg32( ui32BaseAddress,  DWC_OTG_DEV_OUT_EP_REGS_DOEPINT0 + i*DWC_EP_REG_OFFSET, 0xFF);
	}
#if defined USE_DMA_INTERNAL
  #if !defined USE_MULTIPLE_TX_FIFO
	if(core_if->en_multiple_tx_fifo)
  #endif
#else
  #if defined USE_MULTIPLE_TX_FIFO
	if(core_if->dma_enable)
  #else
	if(core_if->en_multiple_tx_fifo && core_if->dma_enable)
  #endif
#endif /* USE_DMA_INTERNAL */
	{
		dev_if->non_iso_tx_thr_en = params->thr_ctl & 0x1;
		dev_if->iso_tx_thr_en = (params->thr_ctl >> 1) & 0x1;
		dev_if->rx_thr_en = (params->thr_ctl >> 2) & 0x1;
			
		dev_if->rx_thr_length = params->rx_thr_length;
		dev_if->tx_thr_length = params->tx_thr_length;
			
		dev_if->setup_desc_index = 0;
			 
		dthrctl.d32 = 0;
		dthrctl.b.non_iso_thr_en = dev_if->non_iso_tx_thr_en;
		dthrctl.b.iso_thr_en = dev_if->iso_tx_thr_en;
		dthrctl.b.tx_thr_len = dev_if->tx_thr_length;
		dthrctl.b.rx_thr_en = dev_if->rx_thr_en;
		dthrctl.b.rx_thr_len = dev_if->rx_thr_length;
		dthrctl.b.ahb_thr_ratio = params->ahb_thr_ratio;
			
		usb_WriteReg32( ui32BaseAddress,  DWC_OTG_DEV_GLOBAL_REGS_DTKNQR3_DTHRCTL_OFFSET, dthrctl.d32);

		DWC_DEBUGPL(DBG_CIL, "Non ISO Tx Thr - %d\nISO Tx Thr - %d\nRx Thr - %d\nTx Thr Len - %d\nRx Thr Len - %d\n",
			dthrctl.b.non_iso_thr_en, dthrctl.b.iso_thr_en, dthrctl.b.rx_thr_en, dthrctl.b.tx_thr_len, dthrctl.b.rx_thr_len);

	}
		
	dwc_otg_enable_device_interrupts( ui32BaseAddress, core_if );		 

	{
		diepmsk_data_t msk = {0};
		msk.b.txfifoundrn = 1;
		usb_ModifyReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DIEPMSK_OFFSET, msk.d32, msk.d32);
	}
}


/** 
 * This function enables the Host mode interrupts.
 *
 * @param core_if Programming view of DWC_otg controller
 */
#if !defined DWC_DEVICE_ONLY
IMG_VOID dwc_otg_enable_host_interrupts( USB_DC_T	*	psContext )
{
	dwc_otg_core_if_t *core_if = psContext->psOtgCoreIf;
	gintmsk_data_t intr_mask = { .d32 = 0 };

	DWC_DEBUGPL(DBG_CIL, "%s()\n", __func__);

	/* Disable all interrupts. */
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, 0);

	/* Clear any pending interrupts. */
	usb_WriteReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, 0xFFFFFFFF); 

	/* Enable the common interrupts */
	dwc_otg_enable_common_interrupts(core_if);

	/*
	 * Enable host mode interrupts without disturbing common
	 * interrupts.
	 */
	 
	/* Do not need sof interrupt for Descriptor DMA*/ 
	if (!core_if->dma_desc_enable)
	{
		intr_mask.b.sofintr = 1;
	}
	intr_mask.b.portintr = 1;
	intr_mask.b.hcintr = 1;

	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, intr_mask.d32);
}

/** 
 * This function disables the Host Mode interrupts.
 *
 * @param core_if Programming view of DWC_otg controller
 */
IMG_VOID dwc_otg_disable_host_interrupts( USB_DC_T	*	psContext )
{
	dwc_otg_core_if_t *core_if = psContext->psOtgCoreIf;
	gintmsk_data_t intr_mask = { .d32 = 0 };

	DWC_DEBUGPL(DBG_CILV, "%s()\n", __func__);
		 
	/*
	 * Disable host mode interrupts without disturbing common
	 * interrupts.
	 */
	intr_mask.b.sofintr = 1;
	intr_mask.b.portintr = 1;
	intr_mask.b.hcintr = 1;
	intr_mask.b.ptxfempty = 1;
	intr_mask.b.nptxfempty = 1;
		
	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, 0);
}

/**
 * This function initializes the DWC_otg controller registers for
 * host mode.
 *
 * This function flushes the Tx and Rx FIFOs and it flushes any entries in the
 * request queues. Host channels are reset to ensure that they are ready for
 * performing transfers.
 *
 * @param core_if Programming view of DWC_otg controller
 *
 */
IMG_VOID dwc_otg_core_host_init( USB_DC_T	*	psContext )
{
	dwc_otg_core_if_t	*	core_if = psContext->psOtgCoreIf;
	dwc_otg_host_if_t	*	host_if = core_if->host_if;
	dwc_otg_core_params_t	*params = core_if->core_params;
	hprt0_data_t			hprt0 = { .d32 = 0 };
	fifosize_data_t			nptxfifosize;
	fifosize_data_t			ptxfifosize;
	int						i;
	hcchar_data_t			hcchar;
	hcfg_data_t				hcfg;
	dwc_otg_hc_regs_t	*	hc_regs;
	int						num_channels;
	gotgctl_data_t			gotgctl = { .d32 = 0 };

	DWC_DEBUGPL(DBG_CILV,"%s(%p)\n", __func__, core_if);

	/* Restart the Phy Clock */
	usb_WriteReg32( ui32BaseAddress, core_if->pcgcctl, 0);
	
	/* Initialize Host Configuration Register */
	init_fslspclksel(core_if);
	if (core_if->core_params.speed == DWC_SPEED_PARAM_FULL) 
	{
		hcfg.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_HOST_GLOBAL_REGS_HCFG_OFFSET);
		hcfg.b.fslssupp = 1;
		usb_WriteReg32( ui32BaseAddress, DWC_OTG_HOST_GLOBAL_REGS_HCFG_OFFSET, hcfg.d32);
	}

	if (core_if->core_params.dma_desc_enable) {
		uint8_t op_mode = core_if->hwcfg2.b.op_mode;	
		if (!(core_if->hwcfg4.b.desc_dma && (core_if->snpsid >= OTG_CORE_REV_2_90a) &&
				((op_mode == DWC_HWCFG2_OP_MODE_HNP_SRP_CAPABLE_OTG) ||
				(op_mode == DWC_HWCFG2_OP_MODE_SRP_ONLY_CAPABLE_OTG) || 
				(op_mode == DWC_HWCFG2_OP_MODE_NO_HNP_SRP_CAPABLE_OTG) ||
				(op_mode == DWC_HWCFG2_OP_MODE_SRP_CAPABLE_HOST) ||
				(op_mode == DWC_HWCFG2_OP_MODE_NO_SRP_CAPABLE_HOST)))) {
				
				DWC_ERROR("Host can't operate in Descriptor DMA mode.\n"
					  "Either core version is below 2.90a or "
					  "GHWCFG2, GHWCFG4 registers' values do not allow Descriptor DMA in host mode.\n"
					  "To run the driver in Buffer DMA host mode set dma_desc_enable "
					  "module parameter to 0.\n");
				return;
		}		
		hcfg.d32 = dwc_read_reg32(&host_if->host_global_regs->hcfg);
		hcfg.b.descdma = 1;
		dwc_write_reg32(&host_if->host_global_regs->hcfg, hcfg.d32);
	}

	/* Configure data FIFO sizes */
	if (core_if->hwcfg2.b.dynamic_fifo && params->enable_dynamic_fifo) {
		DWC_DEBUGPL(DBG_CIL,"Total FIFO Size=%d\n", core_if->total_fifo_size);
		DWC_DEBUGPL(DBG_CIL,"Rx FIFO Size=%d\n", params->host_rx_fifo_size);
		DWC_DEBUGPL(DBG_CIL,"NP Tx FIFO Size=%d\n", params->host_nperio_tx_fifo_size);
		DWC_DEBUGPL(DBG_CIL,"P Tx FIFO Size=%d\n", params->host_perio_tx_fifo_size);

		/* Rx FIFO */
		DWC_DEBUGPL(DBG_CIL,"initial grxfsiz=%08x\n", usb_ReadReg32( ui32BaseAddress, &global_regs->grxfsiz));
		usb_WriteReg32( ui32BaseAddress, &global_regs->grxfsiz, params->host_rx_fifo_size);
		DWC_DEBUGPL(DBG_CIL,"new grxfsiz=%08x\n", usb_ReadReg32( ui32BaseAddress, &global_regs->grxfsiz));

		/* Non-periodic Tx FIFO */
		DWC_DEBUGPL(DBG_CIL,"initial gnptxfsiz=%08x\n", usb_ReadReg32( ui32BaseAddress, &global_regs->gnptxfsiz));
		nptxfifosize.b.depth  = params->host_nperio_tx_fifo_size;
		nptxfifosize.b.startaddr = params->host_rx_fifo_size;
		usb_WriteReg32( ui32BaseAddress, &global_regs->gnptxfsiz, nptxfifosize.d32);
		DWC_DEBUGPL(DBG_CIL,"new gnptxfsiz=%08x\n", usb_ReadReg32( ui32BaseAddress, &global_regs->gnptxfsiz));
		
		/* Periodic Tx FIFO */
		DWC_DEBUGPL(DBG_CIL,"initial hptxfsiz=%08x\n", usb_ReadReg32( ui32BaseAddress, &global_regs->hptxfsiz));
		ptxfifosize.b.depth	 = params->host_perio_tx_fifo_size;
		ptxfifosize.b.startaddr = nptxfifosize.b.startaddr + nptxfifosize.b.depth;
		usb_WriteReg32( ui32BaseAddress, &global_regs->hptxfsiz, ptxfifosize.d32);
		DWC_DEBUGPL(DBG_CIL,"new hptxfsiz=%08x\n", usb_ReadReg32( ui32BaseAddress, &global_regs->hptxfsiz));
	}

	/* Clear Host Set HNP Enable in the OTG Control Register */
	gotgctl.b.hstsethnpen = 1;
	usb_ModifyReg32( ui32BaseAddress, &global_regs->gotgctl, gotgctl.d32, 0);

	/* Make sure the FIFOs are flushed. */
	dwc_otg_flush_tx_fifo(core_if, 0x10 /* all Tx FIFOs */);
	dwc_otg_flush_rx_fifo(core_if);

	if(!core_if->core_params.dma_desc_enable) 
	{
		/* Flush out any leftover queued requests. */
		num_channels = core_if->core_params.host_channels;
		for (i = 0; i < num_channels; i++) 
		{
			hc_regs = core_if->host_if->hc_regs[i];
			hcchar.d32 = usb_ReadReg32( ui32BaseAddress, &hc_regs->hcchar);
			hcchar.b.chen = 0;
			hcchar.b.chdis = 1;
			hcchar.b.epdir = 0;
			usb_WriteReg32( ui32BaseAddress, &hc_regs->hcchar, hcchar.d32);
		}
			   
		/* Halt all channels to put them into a known state. */
		for (i = 0; i < num_channels; i++) 
		{
			int count = 0;
			hc_regs = core_if->host_if->hc_regs[i];
			hcchar.d32 = usb_ReadReg32( ui32BaseAddress, &hc_regs->hcchar);
			hcchar.b.chen = 1;
			hcchar.b.chdis = 1;
			hcchar.b.epdir = 0;
			usb_WriteReg32( ui32BaseAddress, &hc_regs->hcchar, hcchar.d32);
			DWC_DEBUGPL(DBG_HCDV, "%s: Halt channel %d\n", __func__, i);
			do 
			{
				hcchar.d32 = usb_ReadReg32( ui32BaseAddress, &hc_regs->hcchar);
				if (++count > 1000) 
				{
					DWC_ERROR("%s: Unable to clear halt on channel %d\n",
						  __func__, i);
					break;
				}
				dwc_udelay(1);	
			} while (hcchar.b.chen);
		}
	}

	/* Turn on the vbus power. */
	DWC_PRINT("Init: Port Power? op_state=%d\n", core_if->op_state);
	if (core_if->op_state == A_HOST) {	
		hprt0.d32 = dwc_otg_read_hprt0(core_if);
		DWC_PRINT("Init: Power Port (%d)\n", hprt0.b.prtpwr);
		if (hprt0.b.prtpwr == 0) {
			hprt0.b.prtpwr = 1;
			usb_WriteReg32( ui32BaseAddress, host_if->hprt0, hprt0.d32);
		}  
	}

	dwc_otg_enable_host_interrupts(core_if);
}

/**
 * Prepares a host channel for transferring packets to/from a specific
 * endpoint. The HCCHARn register is set up with the characteristics specified
 * in _hc. Host channel interrupts that may need to be serviced while this
 * transfer is in progress are enabled.
 *
 * @param core_if Programming view of DWC_otg controller
 * @param hc Information needed to initialize the host channel
 */
IMG_VOID dwc_otg_hc_init(dwc_otg_core_if_t *core_if, dwc_hc_t *hc)
{
	uint32_t intr_enable;
	hcintmsk_data_t hc_intr_mask;
	gintmsk_data_t gintmsk = { .d32 = 0 };
	hcchar_data_t hcchar;
	hcsplt_data_t hcsplt;

	uint8_t hc_num = hc->hc_num;
	dwc_otg_host_if_t *host_if = core_if->host_if;
	dwc_otg_hc_regs_t *hc_regs = host_if->hc_regs[hc_num];

	/* Clear old interrupt conditions for this host channel. */
	hc_intr_mask.d32 = 0xFFFFFFFF;
	hc_intr_mask.b.reserved14_31 = 0;
	usb_WriteReg32( ui32BaseAddress, &hc_regs->hcint, hc_intr_mask.d32);

	/* Enable channel interrupts required for this transfer. */
	hc_intr_mask.d32 = 0;
	hc_intr_mask.b.chhltd = 1;
	if (core_if->dma_enable) 
	{
		/* For Descriptor DMA mode core halts the channel on AHB error. Interrupt is not required */
		if (!core_if->dma_desc_enable)
		{
			hc_intr_mask.b.ahberr = 1;
		}
		else if (hc->ep_type == DWC_OTG_EP_TYPE_ISOC)
		{
			hc_intr_mask.b.xfercompl = 1;
		}
		
		if (hc->error_state && !hc->do_split &&
			hc->ep_type != DWC_OTG_EP_TYPE_ISOC) 
		{
			hc_intr_mask.b.ack = 1;
			if (hc->ep_is_in) 
			{
				hc_intr_mask.b.datatglerr = 1;
				if (hc->ep_type != DWC_OTG_EP_TYPE_INTR) 
				{
					hc_intr_mask.b.nak = 1;
				}
			}
		}
	} 
	else {
		switch (hc->ep_type) {
		case DWC_OTG_EP_TYPE_CONTROL:
		case DWC_OTG_EP_TYPE_BULK:
			hc_intr_mask.b.xfercompl = 1;
			hc_intr_mask.b.stall = 1;
			hc_intr_mask.b.xacterr = 1;
			hc_intr_mask.b.datatglerr = 1;
			if (hc->ep_is_in) {
				hc_intr_mask.b.bblerr = 1;
			} 
			else {
				hc_intr_mask.b.nak = 1;
				hc_intr_mask.b.nyet = 1;
				if (hc->do_ping) {
					hc_intr_mask.b.ack = 1;
				}
			}

			if (hc->do_split) {
				hc_intr_mask.b.nak = 1;
				if (hc->complete_split) {
					hc_intr_mask.b.nyet = 1;
				}
				else {
					hc_intr_mask.b.ack = 1;
				}
			}

			if (hc->error_state) {
				hc_intr_mask.b.ack = 1;
			}
			break;
		case DWC_OTG_EP_TYPE_INTR:
			hc_intr_mask.b.xfercompl = 1;
			hc_intr_mask.b.nak = 1;
			hc_intr_mask.b.stall = 1;
			hc_intr_mask.b.xacterr = 1;
			hc_intr_mask.b.datatglerr = 1;
			hc_intr_mask.b.frmovrun = 1;

			if (hc->ep_is_in) {
				hc_intr_mask.b.bblerr = 1;
			}
			if (hc->error_state) {
				hc_intr_mask.b.ack = 1;
			}
			if (hc->do_split) {
				if (hc->complete_split) {
					hc_intr_mask.b.nyet = 1;
				}
				else {
					hc_intr_mask.b.ack = 1;
				}
			}
			break;
		case DWC_OTG_EP_TYPE_ISOC:
			hc_intr_mask.b.xfercompl = 1;
			hc_intr_mask.b.frmovrun = 1;
			hc_intr_mask.b.ack = 1;

			if (hc->ep_is_in) {
				hc_intr_mask.b.xacterr = 1;
				hc_intr_mask.b.bblerr = 1;
			}
			break;
		}
	}
	usb_WriteReg32( ui32BaseAddress, &hc_regs->hcintmsk, hc_intr_mask.d32);

	/* Enable the top level host channel interrupt. */
	intr_enable = (1 << hc_num);
	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_HOST_GLOBAL_REGS_HAINTMSK_OFFSET, 0, intr_enable);

	/* Make sure host channel interrupts are enabled. */
	gintmsk.b.hcintr = 1;
	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_HOST_GLOBAL_REGS_GINTMSK_OFFSET, 0, gintmsk.d32);
	
	/*
	 * Program the HCCHARn register with the endpoint characteristics for
	 * the current transfer.
	 */
	hcchar.d32 = 0;
	hcchar.b.devaddr = hc->dev_addr;
	hcchar.b.epnum = hc->ep_num;
	hcchar.b.epdir = hc->ep_is_in;
	hcchar.b.lspddev = (hc->speed == DWC_OTG_EP_SPEED_LOW);
	hcchar.b.eptype = hc->ep_type;
	hcchar.b.mps = hc->max_packet;

	usb_WriteReg32( ui32BaseAddress, &host_if->hc_regs[hc_num]->hcchar, hcchar.d32);

	DWC_DEBUGPL(DBG_HCDV, "%s: Channel %d\n", __func__, hc->hc_num);
	DWC_DEBUGPL(DBG_HCDV, "	 Dev Addr: %d\n", hcchar.b.devaddr);
	DWC_DEBUGPL(DBG_HCDV, "	 Ep Num: %d\n", hcchar.b.epnum);
	DWC_DEBUGPL(DBG_HCDV, "	 Is In: %d\n", hcchar.b.epdir);
	DWC_DEBUGPL(DBG_HCDV, "	 Is Low Speed: %d\n", hcchar.b.lspddev);
	DWC_DEBUGPL(DBG_HCDV, "	 Ep Type: %d\n", hcchar.b.eptype);
	DWC_DEBUGPL(DBG_HCDV, "	 Max Pkt: %d\n", hcchar.b.mps);
	DWC_DEBUGPL(DBG_HCDV, "	 Multi Cnt: %d\n", hcchar.b.multicnt);

	/*
	 * Program the HCSPLIT register for SPLITs
	 */
	hcsplt.d32 = 0;
	if (hc->do_split) {
		DWC_DEBUGPL(DBG_HCDV, "Programming HC %d with split --> %s\n", hc->hc_num,
			   hc->complete_split ? "CSPLIT" : "SSPLIT");
		hcsplt.b.compsplt = hc->complete_split;
		hcsplt.b.xactpos = hc->xact_pos;
		hcsplt.b.hubaddr = hc->hub_addr;
		hcsplt.b.prtaddr = hc->port_addr;
		DWC_DEBUGPL(DBG_HCDV, "	  comp split %d\n", hc->complete_split);
		DWC_DEBUGPL(DBG_HCDV, "	  xact pos %d\n", hc->xact_pos);
		DWC_DEBUGPL(DBG_HCDV, "	  hub addr %d\n", hc->hub_addr);
		DWC_DEBUGPL(DBG_HCDV, "	  port addr %d\n", hc->port_addr);
		DWC_DEBUGPL(DBG_HCDV, "	  is_in %d\n", hc->ep_is_in);
		DWC_DEBUGPL(DBG_HCDV, "	  Max Pkt: %d\n", hcchar.b.mps);
		DWC_DEBUGPL(DBG_HCDV, "	  xferlen: %d\n", hc->xfer_len);		
	}
	usb_WriteReg32( ui32BaseAddress, &host_if->hc_regs[hc_num]->hcsplt, hcsplt.d32);

}

/**
 * Attempts to halt a host channel. This function should only be called in
 * Slave mode or to abort a transfer in either Slave mode or DMA mode. Under
 * normal circumstances in DMA mode, the controller halts the channel when the
 * transfer is complete or a condition occurs that requires application
 * intervention.
 *
 * In slave mode, checks for a free request queue entry, then sets the Channel
 * Enable and Channel Disable bits of the Host Channel Characteristics
 * register of the specified channel to intiate the halt. If there is no free
 * request queue entry, sets only the Channel Disable bit of the HCCHARn
 * register to flush requests for this channel. In the latter case, sets a
 * flag to indicate that the host channel needs to be halted when a request
 * queue slot is open.
 *
 * In DMA mode, always sets the Channel Enable and Channel Disable bits of the
 * HCCHARn register. The controller ensures there is space in the request
 * queue before submitting the halt request.
 *
 * Some time may elapse before the core flushes any posted requests for this
 * host channel and halts. The Channel Halted interrupt handler completes the
 * deactivation of the host channel.
 *
 * @param core_if Controller register interface.
 * @param hc Host channel to halt.
 * @param halt_status Reason for halting the channel.
 */
IMG_VOID dwc_otg_hc_halt(dwc_otg_core_if_t *core_if,
			 dwc_hc_t *hc,
			 dwc_otg_halt_status_e halt_status)
{
	gnptxsts_data_t			nptxsts;
	hptxsts_data_t			hptxsts;
	hcchar_data_t			hcchar;
	dwc_otg_hc_regs_t		*hc_regs;

	hc_regs = core_if->host_if->hc_regs[hc->hc_num];

	DWC_ASSERT(!(halt_status == DWC_OTG_HC_XFER_NO_HALT_STATUS),
		   "halt_status = %d\n", halt_status);

	if (halt_status == DWC_OTG_HC_XFER_URB_DEQUEUE ||
		halt_status == DWC_OTG_HC_XFER_AHB_ERR) {
		/*
		 * Disable all channel interrupts except Ch Halted. The QTD
		 * and QH state associated with this transfer has been cleared
		 * (in the case of URB_DEQUEUE), so the channel needs to be
		 * shut down carefully to prevent crashes.
		 */
		hcintmsk_data_t hcintmsk;
		hcintmsk.d32 = 0;
		hcintmsk.b.chhltd = 1;
		usb_WriteReg32( ui32BaseAddress, &hc_regs->hcintmsk, hcintmsk.d32);

		/*
		 * Make sure no other interrupts besides halt are currently
		 * pending. Handling another interrupt could cause a crash due
		 * to the QTD and QH state.
		 */
		usb_WriteReg32( ui32BaseAddress, &hc_regs->hcint, ~hcintmsk.d32);

		/*
		 * Make sure the halt status is set to URB_DEQUEUE or AHB_ERR
		 * even if the channel was already halted for some other
		 * reason.
		 */
		hc->halt_status = halt_status;

		hcchar.d32 = usb_ReadReg32( ui32BaseAddress, &hc_regs->hcchar);
		if (hcchar.b.chen == 0) {
			/*
			 * The channel is either already halted or it hasn't
			 * started yet. In DMA mode, the transfer may halt if
			 * it finishes normally or a condition occurs that
			 * requires driver intervention. Don't want to halt
			 * the channel again. In either Slave or DMA mode,
			 * it's possible that the transfer has been assigned
			 * to a channel, but not started yet when an URB is
			 * dequeued. Don't want to halt a channel that hasn't
			 * started yet.
			 */
			return;
		}
	}

	if (hc->halt_pending) {
		/*
		 * A halt has already been issued for this channel. This might
		 * happen when a transfer is aborted by a higher level in
		 * the stack.
		 */
#if defined DEBUG
		DWC_PRINT("*** %s: Channel %d, _hc->halt_pending already set ***\n",
			  __func__, hc->hc_num);

#endif		
		return;
	}

	hcchar.d32 = usb_ReadReg32( ui32BaseAddress, &hc_regs->hcchar);
	/* IMG_SYNOP_CR */
	/* No need to set the bit in DDMA for disabling the channel 	*/
	/* Potential enhancement: This bit could also be set wherever	*/
	/* channel is disabled.											*/
	if(!core_if->core_params.dma_desc_enable)
	{
		hcchar.b.chen = 1;
	}
	hcchar.b.chdis = 1;

	if (!core_if->dma_enable) {
		/* Check for space in the request queue to issue the halt. */
		if (hc->ep_type == DWC_OTG_EP_TYPE_CONTROL ||
			hc->ep_type == DWC_OTG_EP_TYPE_BULK) {
			nptxsts.d32 = usb_ReadReg32( ui32BaseAddress, &global_regs->gnptxsts);
			if (nptxsts.b.nptxqspcavail == 0) {
				hcchar.b.chen = 0;
			}
		} 
		else {
			hptxsts.d32 = usb_ReadReg32( ui32BaseAddress, &host_global_regs->hptxsts);
			if ((hptxsts.b.ptxqspcavail == 0) || (core_if->queuing_high_bandwidth)) {
				hcchar.b.chen = 0;
			}
		}
	}

	usb_WriteReg32( ui32BaseAddress, &hc_regs->hcchar, hcchar.d32);

	hc->halt_status = halt_status;

	if (hcchar.b.chen) {
		hc->halt_pending = 1;
		hc->halt_on_queue = 0;
	} 
	else {
		hc->halt_on_queue = 1;
	}

	DWC_DEBUGPL(DBG_HCDV, "%s: Channel %d\n", __func__, hc->hc_num);
	DWC_DEBUGPL(DBG_HCDV, "	 hcchar: 0x%08x\n", hcchar.d32);
	DWC_DEBUGPL(DBG_HCDV, "	 halt_pending: %d\n", hc->halt_pending);
	DWC_DEBUGPL(DBG_HCDV, "	 halt_on_queue: %d\n", hc->halt_on_queue);
	DWC_DEBUGPL(DBG_HCDV, "	 halt_status: %d\n", hc->halt_status);

	return;
}

/**
 * Clears the transfer state for a host channel. This function is normally
 * called after a transfer is done and the host channel is being released.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param hc Identifies the host channel to clean up.
 */
IMG_VOID dwc_otg_hc_cleanup(dwc_otg_core_if_t *core_if, dwc_hc_t *hc)
{
	dwc_otg_hc_regs_t *hc_regs;

	hc->xfer_started = 0;

	/*
	 * Clear channel interrupt enables and any unhandled channel interrupt
	 * conditions.
	 */
	hc_regs = core_if->host_if->hc_regs[hc->hc_num];
	usb_WriteReg32( ui32BaseAddress, &hc_regs->hcintmsk, 0);
	usb_WriteReg32( ui32BaseAddress, &hc_regs->hcint, 0xFFFFFFFF);

#if defined DEBUG
	del_timer(&core_if->hc_xfer_timer[hc->hc_num]);
#endif	
}

/**
 * Sets the channel property that indicates in which frame a periodic transfer
 * should occur. This is always set to the _next_ frame. This function has no
 * effect on non-periodic transfers.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param hc Identifies the host channel to set up and its properties.
 * @param hcchar Current value of the HCCHAR register for the specified host
 * channel.
 */
static inline IMG_VOID hc_set_even_odd_frame(dwc_otg_core_if_t *core_if,
					 dwc_hc_t *hc,
					 hcchar_data_t *hcchar)
{
	if (hc->ep_type == DWC_OTG_EP_TYPE_INTR ||
		hc->ep_type == DWC_OTG_EP_TYPE_ISOC) {
		hfnum_data_t	hfnum;
		hfnum.d32 = usb_ReadReg32( ui32BaseAddress, &core_if->host_if->host_global_regs->hfnum);
		
		/* 1 if _next_ frame is odd, 0 if it's even */
		hcchar->b.oddfrm = (hfnum.b.frnum & 0x1) ? 0 : 1;
#if defined DEBUG
		if (hc->ep_type == DWC_OTG_EP_TYPE_INTR && hc->do_split && !hc->complete_split) {
			switch (hfnum.b.frnum & 0x7) {
			case 7:
				core_if->hfnum_7_samples++;
				core_if->hfnum_7_frrem_accum += hfnum.b.frrem;
				break;
			case 0:
				core_if->hfnum_0_samples++;
				core_if->hfnum_0_frrem_accum += hfnum.b.frrem;
				break;
			default:
				core_if->hfnum_other_samples++;
				core_if->hfnum_other_frrem_accum += hfnum.b.frrem;
				break;
			}
		}
#endif		
	}
}

#if defined DEBUG
static IMG_VOID hc_xfer_timeout(unsigned long ptr)
{
	hc_xfer_info_t *xfer_info = (hc_xfer_info_t *)ptr;
	int hc_num = xfer_info->hc->hc_num;
	DWC_WARN("%s: timeout on channel %d\n", __func__, hc_num);
	DWC_WARN("	start_hcchar_val 0x%08x\n", xfer_info->core_if->start_hcchar_val[hc_num]);
}
#endif

IMG_VOID set_pid_isoc(dwc_hc_t * hc)
{
	/* Set up the initial PID for the transfer. */
	if (hc->speed == DWC_OTG_EP_SPEED_HIGH) {
		if (hc->ep_is_in) {
			if (hc->multi_count == 1) {
				hc->data_pid_start =
				    DWC_OTG_HC_PID_DATA0;
			} else if (hc->multi_count == 2) {
				hc->data_pid_start =
				    DWC_OTG_HC_PID_DATA1;
			} else {
				hc->data_pid_start =
				    DWC_OTG_HC_PID_DATA2;
			}
		} else {
			if (hc->multi_count == 1) {
				hc->data_pid_start =
				    DWC_OTG_HC_PID_DATA0;
			} else {
				hc->data_pid_start =
				    DWC_OTG_HC_PID_MDATA;
			}
		}
	} else {
		hc->data_pid_start = DWC_OTG_HC_PID_DATA0;
	}
}

/**
 * This function does the setup for a data transfer for a host channel and
 * starts the transfer. May be called in either Slave mode or DMA mode. In
 * Slave mode, the caller must ensure that there is sufficient space in the
 * request queue and Tx Data FIFO.
 *
 * For an OUT transfer in Slave mode, it loads a data packet into the
 * appropriate FIFO. If necessary, additional data packets will be loaded in
 * the Host ISR.
 *
 * For an IN transfer in Slave mode, a data packet is requested. The data
 * packets are unloaded from the Rx FIFO in the Host ISR. If necessary,
 * additional data packets are requested in the Host ISR.
 *
 * For a PING transfer in Slave mode, the Do Ping bit is set in the HCTSIZ
 * register along with a packet count of 1 and the channel is enabled. This
 * causes a single PING transaction to occur. Other fields in HCTSIZ are
 * simply set to 0 since no data transfer occurs in this case.
 *
 * For a PING transfer in DMA mode, the HCTSIZ register is initialized with
 * all the information required to perform the subsequent data transfer. In
 * addition, the Do Ping bit is set in the HCTSIZ register. In this case, the
 * controller performs the entire PING protocol, then starts the data
 * transfer.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param hc Information needed to initialize the host channel. The xfer_len
 * value may be reduced to accommodate the max widths of the XferSize and
 * PktCnt fields in the HCTSIZn register. The multi_count value may be changed
 * to reflect the final xfer_len value.
 */
IMG_VOID dwc_otg_hc_start_transfer(dwc_otg_core_if_t *core_if, dwc_hc_t *hc)
{
	hcchar_data_t hcchar;
	hctsiz_data_t hctsiz;
	uint16_t num_packets;
	uint32_t max_hc_xfer_size = core_if->core_params.max_transfer_size;
	uint16_t max_hc_pkt_count = core_if->core_params.max_packet_count;
	dwc_otg_hc_regs_t *hc_regs = core_if->host_if->hc_regs[hc->hc_num];

	hctsiz.d32 = 0;

	if (hc->do_ping) 
	{
		if (!core_if->dma_enable) 
		{
			dwc_otg_hc_do_ping(core_if, hc);
			hc->xfer_started = 1;
			return;
		} 
		else 
		{
			hctsiz.b.dopng = 1;
		}
	}

	if (hc->do_split) 
	{
		num_packets = 1;

		if (hc->complete_split && !hc->ep_is_in) 
		{
			/* For CSPLIT OUT Transfer, set the size to 0 so the
			 * core doesn't expect any data written to the FIFO */
			hc->xfer_len = 0;
		} 
		else if (hc->ep_is_in || (hc->xfer_len > hc->max_packet)) 
		{
			hc->xfer_len = hc->max_packet;
		} 
		else if (!hc->ep_is_in && (hc->xfer_len > 188)) 
		{
			hc->xfer_len = 188;
		}

		hctsiz.b.xfersize = hc->xfer_len;
	} 
	else 
	{
		/*
		 * Ensure that the transfer length and packet count will fit
		 * in the widths allocated for them in the HCTSIZn register.
		 */
		if (hc->ep_type == DWC_OTG_EP_TYPE_INTR ||
			hc->ep_type == DWC_OTG_EP_TYPE_ISOC) 
		{
			/*
			 * Make sure the transfer size is no larger than one
			 * (micro)frame's worth of data. (A check was done
			 * when the periodic transfer was accepted to ensure
			 * that a (micro)frame's worth of data can be
			 * programmed into a channel.)
			 */
			uint32_t max_periodic_len = hc->multi_count * hc->max_packet;
			if (hc->xfer_len > max_periodic_len) 
			{
				hc->xfer_len = max_periodic_len;
			} 
			else 
			{
			}
		} 
		else if (hc->xfer_len > max_hc_xfer_size) 
		{
			/* Make sure that xfer_len is a multiple of max packet size. */
			hc->xfer_len = max_hc_xfer_size - hc->max_packet + 1;
		}

		if (hc->xfer_len > 0) 
		{
			num_packets = (hc->xfer_len + hc->max_packet - 1) / hc->max_packet;
			if (num_packets > max_hc_pkt_count) 
			{
				num_packets = max_hc_pkt_count;
				hc->xfer_len = num_packets * hc->max_packet;
			}
		} 
		else 
		{
			/* Need 1 packet for transfer length of 0. */
			num_packets = 1;
		}

		if (hc->ep_is_in) 
		{
			/* Always program an integral # of max packets for IN transfers. */
			hc->xfer_len = num_packets * hc->max_packet;
		}

		if (hc->ep_type == DWC_OTG_EP_TYPE_INTR ||
			hc->ep_type == DWC_OTG_EP_TYPE_ISOC) 
		{
			/*
			 * Make sure that the multi_count field matches the
			 * actual transfer length.
			 */
			hc->multi_count = num_packets;
		}

		if (hc->ep_type == DWC_OTG_EP_TYPE_ISOC) 
			set_pid_isoc(hc);

		hctsiz.b.xfersize = hc->xfer_len;
	}

	hc->start_pkt_count = num_packets;
	hctsiz.b.pktcnt = num_packets;
	hctsiz.b.pid = hc->data_pid_start;
	usb_WriteReg32( ui32BaseAddress, &hc_regs->hctsiz, hctsiz.d32);

	DWC_DEBUGPL(DBG_HCDV, "%s: Channel %d\n", __func__, hc->hc_num);
	DWC_DEBUGPL(DBG_HCDV, "	 Xfer Size: %d\n", hctsiz.b.xfersize);
	DWC_DEBUGPL(DBG_HCDV, "	 Num Pkts: %d\n", hctsiz.b.pktcnt);
	DWC_DEBUGPL(DBG_HCDV, "	 Start PID: %d\n", hctsiz.b.pid);

	if (core_if->dma_enable) {
		dwc_dma_t dma_addr;
		if (hc->align_buff) {
			dma_addr = hc->align_buff;
		} else {
			dma_addr = (uint32_t)hc->xfer_buff;
		}
		dwc_write_reg32(&hc_regs->hcdma, dma_addr);
	}

	/* Start the split */
	if (hc->do_split) {
		hcsplt_data_t hcsplt;
		hcsplt.d32 = dwc_read_reg32 (&hc_regs->hcsplt);
		hcsplt.b.spltena = 1;
		usb_WriteReg32( ui32BaseAddress, &hc_regs->hcsplt, hcsplt.d32);
	}

	hcchar.d32 = usb_ReadReg32( ui32BaseAddress, &hc_regs->hcchar);
	hcchar.b.multicnt = hc->multi_count;
	hc_set_even_odd_frame(core_if, hc, &hcchar);
#if defined DEBUG
	core_if->start_hcchar_val[hc->hc_num] = hcchar.d32;
	if (hcchar.b.chdis) {
		DWC_WARN("%s: chdis set, channel %d, hcchar 0x%08x\n",
			 __func__, hc->hc_num, hcchar.d32);
	}
#endif	

	/* Set host channel enable after all other setup is complete. */
	hcchar.b.chen = 1;
	hcchar.b.chdis = 0;
	usb_WriteReg32( ui32BaseAddress, &hc_regs->hcchar, hcchar.d32);

	hc->xfer_started = 1;
	hc->requests++;

	if (!core_if->dma_enable && !hc->ep_is_in && hc->xfer_len > 0) {
		/* Load OUT packet into the appropriate Tx FIFO. */
		dwc_otg_hc_write_packet(core_if, hc);
	}
	hcchar_data_t hcchar;
	hctsiz_data_t hctsiz;
	hcdma_data_t  hcdma;
	
	hctsiz.d32 = 0;

	if (hc->do_ping && !hc->ep_is_in)
		hctsiz.b_ddma.dopng = 1;

	if (hc->ep_type == DWC_OTG_EP_TYPE_ISOC)
		set_pid_isoc(hc);
	
	/* Packet Count and Xfer Size are not used in Descriptor DMA mode */
	hctsiz.b_ddma.pid = hc->data_pid_start;
	hctsiz.b_ddma.ntd = hc->ntd - 1; /* 0 - 1 descriptor, 1 - 2 descriptors, etc. */
	hctsiz.b_ddma.schinfo = hc->schinfo; /* Non-zero only for high-speed interrupt endpoints */
	
	DWC_DEBUGPL(DBG_HCDV, "%s: Channel %d\n", __func__, hc->hc_num);
	DWC_DEBUGPL(DBG_HCDV, "	 Start PID: %d\n", hctsiz.b.pid);
	DWC_DEBUGPL(DBG_HCDV, "	 NTD: %d\n", hctsiz.b_ddma.ntd);	

	dwc_write_reg32(&hc_regs->hctsiz, hctsiz.d32);

	hcdma.d32 = 0;
	hcdma.b.dma_addr = ((uint32_t)hc->desc_list_addr) >> 11;
		
	/* Always start from first descriptor. */
	hcdma.b.ctd = 0;
	dwc_write_reg32(&hc_regs->hcdma, hcdma.d32);

	hcchar.d32 = dwc_read_reg32(&hc_regs->hcchar);
	hcchar.b.multicnt = hc->multi_count;
	
#if defined DEBUG
	core_if->start_hcchar_val[hc->hc_num] = hcchar.d32;
	if (hcchar.b.chdis) {
		DWC_WARN("%s: chdis set, channel %d, hcchar 0x%08x\n",
			 __func__, hc->hc_num, hcchar.d32);
	}
#endif

	/* Set host channel enable after all other setup is complete. */
	hcchar.b.chen = 1;
	hcchar.b.chdis = 0;
	
	dwc_write_reg32(&hc_regs->hcchar, hcchar.d32);

	hc->xfer_started = 1;
	hc->requests++;

#if defined DEBUG
	if ((hc->ep_type != DWC_OTG_EP_TYPE_INTR) && (hc->ep_type != DWC_OTG_EP_TYPE_ISOC)) {
	core_if->hc_xfer_info[hc->hc_num].core_if = core_if;
	core_if->hc_xfer_info[hc->hc_num].hc = hc;
		/* Start a timer for this transfer. */
		DWC_TIMER_SCHEDULE(core_if->hc_xfer_timer[hc->hc_num], 10000);
	}

#endif
	
}

/**
 * This function continues a data transfer that was started by previous call
 * to <code>dwc_otg_hc_start_transfer</code>. The caller must ensure there is
 * sufficient space in the request queue and Tx Data FIFO. This function
 * should only be called in Slave mode. In DMA mode, the controller acts
 * autonomously to complete transfers programmed to a host channel.
 *
 * For an OUT transfer, a new data packet is loaded into the appropriate FIFO
 * if there is any data remaining to be queued. For an IN transfer, another
 * data packet is always requested. For the SETUP phase of a control transfer,
 * this function does nothing.
 *
 * @return 1 if a new request is queued, 0 if no more requests are required
 * for this transfer.
 */
int dwc_otg_hc_continue_transfer(dwc_otg_core_if_t *core_if, dwc_hc_t *hc)
{
	DWC_DEBUGPL(DBG_HCDV, "%s: Channel %d\n", __func__, hc->hc_num);

	if (hc->do_split) {
		/* SPLITs always queue just once per channel */
		return 0;
	} 
	else if (hc->data_pid_start == DWC_OTG_HC_PID_SETUP) {
		/* SETUPs are queued only once since they can't be NAKed. */
		return 0;
	} 
	else if (hc->ep_is_in) {
		/*
		 * Always queue another request for other IN transfers. If
		 * back-to-back INs are issued and NAKs are received for both,
		 * the driver may still be processing the first NAK when the
		 * second NAK is received. When the interrupt handler clears
		 * the NAK interrupt for the first NAK, the second NAK will
		 * not be seen. So we can't depend on the NAK interrupt
		 * handler to requeue a NAKed request. Instead, IN requests
		 * are issued each time this function is called. When the
		 * transfer completes, the extra requests for the channel will
		 * be flushed.
		 */
		hcchar_data_t hcchar;
		dwc_otg_hc_regs_t *hc_regs = core_if->host_if->hc_regs[hc->hc_num];

		hcchar.d32 = usb_ReadReg32( ui32BaseAddress, &hc_regs->hcchar);
		hc_set_even_odd_frame(core_if, hc, &hcchar);
		hcchar.b.chen = 1;
		hcchar.b.chdis = 0;
		DWC_DEBUGPL(DBG_HCDV, "	 IN xfer: hcchar = 0x%08x\n", hcchar.d32);
		usb_WriteReg32( ui32BaseAddress, &hc_regs->hcchar, hcchar.d32);
		hc->requests++;
		return 1;
	} 
	else {
		/* OUT transfers. */
		if (hc->xfer_count < hc->xfer_len) {
			if (hc->ep_type == DWC_OTG_EP_TYPE_INTR ||
				hc->ep_type == DWC_OTG_EP_TYPE_ISOC) {
				hcchar_data_t hcchar;
				dwc_otg_hc_regs_t *hc_regs;
				hc_regs = core_if->host_if->hc_regs[hc->hc_num];
				hcchar.d32 = usb_ReadReg32( ui32BaseAddress, &hc_regs->hcchar);
				hc_set_even_odd_frame(core_if, hc, &hcchar);
			}

			/* Load OUT packet into the appropriate Tx FIFO. */
			dwc_otg_hc_write_packet(core_if, hc);
			hc->requests++;
			return 1;
		} 
		else {
			return 0;
		}
	}
}

/**
 * Starts a PING transfer. This function should only be called in Slave mode.
 * The Do Ping bit is set in the HCTSIZ register, then the channel is enabled.
 */
IMG_VOID dwc_otg_hc_do_ping(dwc_otg_core_if_t *core_if, dwc_hc_t *hc)
{
	hcchar_data_t hcchar;
	hctsiz_data_t hctsiz;
	dwc_otg_hc_regs_t *hc_regs = core_if->host_if->hc_regs[hc->hc_num];

	DWC_DEBUGPL(DBG_HCDV, "%s: Channel %d\n", __func__, hc->hc_num);

	hctsiz.d32 = 0;
	hctsiz.b.dopng = 1;
	hctsiz.b.pktcnt = 1;
	usb_WriteReg32( ui32BaseAddress, &hc_regs->hctsiz, hctsiz.d32);

	hcchar.d32 = usb_ReadReg32( ui32BaseAddress, &hc_regs->hcchar);
	hcchar.b.chen = 1;
	hcchar.b.chdis = 0;
	usb_WriteReg32( ui32BaseAddress, &hc_regs->hcchar, hcchar.d32);
}

/*
 * This function writes a packet into the Tx FIFO associated with the Host
 * Channel. For a channel associated with a non-periodic EP, the non-periodic
 * Tx FIFO is written. For a channel associated with a periodic EP, the
 * periodic Tx FIFO is written. This function should only be called in Slave
 * mode.
 *
 * Upon return the xfer_buff and xfer_count fields in _hc are incremented by
 * then number of bytes written to the Tx FIFO.
 */
IMG_VOID dwc_otg_hc_write_packet(dwc_otg_core_if_t *core_if, dwc_hc_t *hc)
{
	uint32_t i;
	uint32_t remaining_count;
	uint32_t byte_count;
	uint32_t dword_count;

	uint32_t *data_buff = (uint32_t *)(hc->xfer_buff);
	uint32_t *data_fifo = core_if->data_fifo[hc->hc_num];

	remaining_count = hc->xfer_len - hc->xfer_count;
	if (remaining_count > hc->max_packet) {
		byte_count = hc->max_packet;
	} 
	else {
		byte_count = remaining_count;
	}

	dword_count = (byte_count + 3) / 4;

	if ((((unsigned long)data_buff) & 0x3) == 0) {
		/* xfer_buff is DWORD aligned. */
		for (i = 0; i < dword_count; i++, data_buff++) 
		{
			usb_WriteReg32( ui32BaseAddress, data_fifo, *data_buff);
		}
	} 
	else {
		/* xfer_buff is not DWORD aligned. */
		for (i = 0; i < dword_count; i++, data_buff++) 
		{
			IMG_UINT32	data;
			data = (data_buff[0] | data_buff[1] << 8 | data_buff[2] << 16 | data_buff[3] << 24);
			usb_WriteReg32( ui32BaseAddress, data_fifo, data);
		}
	}

	hc->xfer_count += byte_count;
	hc->xfer_buff += byte_count;
}

#endif
/**
 * Gets the current USB frame number. This is the frame number from the last 
 * SOF packet.	
 */
#if defined (_PER_IO_)
__USBD_INLINE__ IMG_UINT32 dwc_otg_get_frame_number( const img_uint32 ui32BaseAddress  )
{
	dsts_data_t			dsts;
	dsts.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DSTS_OFFSET);

	/* read current frame/microframe number from DSTS register */
	return dsts.b.soffn;
}
#endif

#if !defined USE_DMA_INTERNAL
/**
 * This function reads a setup packet from the Rx FIFO into the destination 
 * buffer.	This function is called from the Rx Status Queue Level (RxStsQLvl)
 * Interrupt routine when a SETUP packet has been received in Slave mode.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param dest Destination buffer for packet data.
 */
IMG_VOID dwc_otg_read_setup_packet( const img_uint32 ui32BaseAddress, IMG_UINT32 *dest )
{
	/* Get the 8 bytes of a setup transaction data */

	/* Pop 2 DWORDS off the receive data FIFO into memory */
	dest[0] = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DATA_FIFO_OFFSET);
	dest[1] = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DATA_FIFO_OFFSET);
}
#endif

/**
 * This function enables EP0 OUT to receive SETUP packets and configures EP0 
 * IN for transmitting packets.	 It is normally called when the
 * "Enumeration Done" interrupt occurs.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP0 data.
 */
IMG_VOID dwc_otg_ep0_activate( const img_uint32 ui32BaseAddress, dwc_ep_t *ep )
{
	dsts_data_t				dsts;
	depctl_data_t			diepctl;
	depctl_data_t			doepctl;
	dctl_data_t				dctl		= {0};		   

	/* Read the Device Status and Endpoint 0 Control registers */
	dsts.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DSTS_OFFSET);
	diepctl.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0);
	doepctl.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0);

	/* Set the MPS of the IN EP based on the enumeration speed */
	switch (dsts.b.enumspd) 
	{
	case DWC_DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ:
	case DWC_DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ:
	case DWC_DSTS_ENUMSPD_FS_PHY_48MHZ:
		diepctl.b.mps = DWC_DEP0CTL_MPS_64;
		break;
	case DWC_DSTS_ENUMSPD_LS_PHY_6MHZ:
		diepctl.b.mps = DWC_DEP0CTL_MPS_8;
		break;
	}

	usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0, diepctl.d32);

	/* Enable OUT EP for receive */
	doepctl.b.epena = 1;

	usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0, doepctl.d32);

#if defined VERBOSE
	DWC_DEBUGPL(DBG_PCDV,"doepctl0=%0x\n", usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0));
	DWC_DEBUGPL(DBG_PCDV,"diepctl0=%0x\n", usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0));		 
#endif
	
	dctl.b.cgnpinnak = 1;
	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET, dctl.d32, dctl.d32);
	
	DWC_DEBUGPL(DBG_PCDV,"dctl=%0x\n", usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DCTL_OFFSET));
}

/**
 * This function activates an EP.  The Device EP control register for
 * the EP is configured as defined in the ep structure.	 Note: This
 * function is not used for EP0.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP to activate.
 */
IMG_VOID dwc_otg_ep_activate( const img_uint32 ui32BaseAddress, dwc_ep_t *ep )
{
	depctl_data_t			depctl;
	IMG_UINT32				addr;
	daint_data_t			daintmsk	= {0};
	
	DWC_DEBUGPL(DBG_PCDV, "%s() EP%d-%s\n", __func__, ep->num, (ep->is_in?"IN":"OUT"));

#if defined (_PER_IO_)
	ep->xiso_frame_num = 0xFFFFFFFF;
	ep->xiso_active_xfers = 0;
	ep->xiso_queued_xfers = 0;
#endif /* _PER_IO_ */
	/* Read DEPCTLn register */
	if (ep->is_in == 1) 
	{
		addr = DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + ep->num * DWC_EP_REG_OFFSET;
		daintmsk.ep.in = 1<<ep->num;
	} 
	else 
	{
		addr = DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->num*DWC_EP_REG_OFFSET;
		daintmsk.ep.out = 1<<ep->num;
	}
		
	/* If the EP is already active don't change the EP Control
	 * register. */
	depctl.d32 = usb_ReadReg32( ui32BaseAddress, addr);

	if (!depctl.b.usbactep) 
	{
		depctl.b.mps = ep->maxpacket;
		depctl.b.eptype = ep->type;
		depctl.b.txfnum = ep->tx_fifo_num;
				
		if (ep->type == DWC_OTG_EP_TYPE_ISOC) 
		{
		    // bojan: setting set1pid instead does not make any difference 
			depctl.b.setd0pid = 1;	/* IMG_SYNOP_CR */
		} 
		else 
		{
			depctl.b.setd0pid = 1;
		}
		depctl.b.usbactep = 1;

		usb_WriteReg32( ui32BaseAddress, addr, depctl.d32);
		DWC_DEBUGPL(DBG_PCDV,"DEPCTL=%08x\n", usb_ReadReg32( ui32BaseAddress, addr));
	}
		
	/* Enable the Interrupt for this EP */
	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DAINTMSK_OFFSET, 0, daintmsk.d32);
		
	DWC_DEBUGPL(DBG_PCDV,"DAINTMSK=%0x\n", usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DAINTMSK_OFFSET));

	ep->stall_clear_flag = 0;
	return;
}

/**
 * This function deactivates an EP.	 This is done by clearing the USB Active 
 * EP bit in the Device EP control register.  Note: This function is not used 
 * for EP0. EP0 cannot be deactivated.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP to deactivate.
 */
IMG_VOID dwc_otg_ep_deactivate( const img_uint32 ui32BaseAddress, dwc_otg_core_if_t	*	core_if, dwc_ep_t *ep )
{
	depctl_data_t		depctl		= {0};
	IMG_UINT32			addr;
	daint_data_t		daintmsk	= {0};
#if defined (_PER_IO_)
	ep->xiso_frame_num = 0xFFFFFFFF;
	ep->xiso_active_xfers = 0;
	ep->xiso_queued_xfers = 0;
#endif /* _PER_IO */

	/* Read DEPCTLn register */
	if (ep->is_in == 1) 
	{
		addr = DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + ep->num*DWC_EP_REG_OFFSET;
		daintmsk.ep.in = 1<<ep->num;
	} 
	else 
	{
		addr = DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->num*DWC_EP_REG_OFFSET;
		daintmsk.ep.out = 1<<ep->num;
	}

	depctl.d32 = usb_ReadReg32( ui32BaseAddress, addr);

	depctl.b.usbactep = 0;
	if (ep->is_in)
	{
		depctl.b.txfnum = 0;
	}

#if !defined (USE_DDMA_ONLY)
	if (core_if->dma_desc_enable)
#endif /* USE_DDMA_ONLY */
	{
		depctl.b.epdis = 1;
	}

	usb_WriteReg32( ui32BaseAddress, addr, depctl.d32);

	/* Disable the Interrupt for this EP */
	usb_ModifyReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DAINTMSK_OFFSET, daintmsk.d32, 0);
}

/**
 * This function initializes dma descriptor chain.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP to start the transfer on.
 */
static IMG_VOID init_dma_desc_chain( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_ep_t * ep)
{
	dwc_otg_dev_dma_desc_t *dma_desc;
	IMG_UINT32 offset;
	IMG_UINT32 xfer_est;
	int i;

	ep->desc_cnt = (ep->total_len / ep->maxxfer) + ((ep->total_len % ep->maxxfer) ? 1 : 0);
	if (!ep->desc_cnt)
	{
		ep->desc_cnt = 1;
	}
	// Check we have enough descriptors as these are statically allocated.
	IMG_ASSERT( ep->desc_cnt <= DESCRIPTORS_PER_EP );

	dma_desc = ep->desc_addr;
	xfer_est = ep->total_len;
	offset = 0;
	for (i = 0; i < ep->desc_cnt; ++i) 
	{
		/** DMA Descriptor Setup */
		if (xfer_est > ep->maxxfer) 
		{
			dma_desc->status.b.bs = BS_HOST_BUSY;
			dma_desc->status.b.l = 0;
			dma_desc->status.b.ioc = 0;
			dma_desc->status.b.sp = 0;
			dma_desc->status.b.bytes = ep->maxxfer;
			dma_desc->buf = ep->dma_addr + offset;
			dma_desc->status.b.bs = BS_HOST_READY;

			xfer_est -= ep->maxxfer;
			offset += ep->maxxfer;
		} 
		else 
		{
			dma_desc->status.b.bs = BS_HOST_BUSY;
			dma_desc->status.b.l = 1;
			dma_desc->status.b.ioc = 1;
			if (ep->is_in) 
			{
				dma_desc->status.b.sp = (xfer_est % ep->maxpacket) ? 1 : ((ep->sent_zlp) ? 1 : 0);
				dma_desc->status.b.bytes = xfer_est;
			} 
			else 
			{
				dma_desc->status.b.bytes = xfer_est + ((4 - (xfer_est & 0x3)) & 0x3);
			}

			dma_desc->buf = ep->dma_addr + offset;
			dma_desc->status.b.bs = BS_HOST_READY;
		}
		dma_desc++;
	}
}

/**
 * This function does the setup for a data transfer for an EP and
 * starts the transfer.	 For an IN transfer, the packets will be
 * loaded into the appropriate Tx FIFO in the ISR. For OUT transfers,
 * the packets are unloaded from the Rx FIFO in the ISR.  the ISR.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP to start the transfer on.
 */

IMG_VOID dwc_otg_ep_start_transfer( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_core_if_t	*	core_if, dwc_ep_t *ep )
{
	depctl_data_t				depctl;
#if !defined (USE_DDMA_ONLY)
	deptsiz_data_t				deptsiz;
#endif
#if !defined USE_DMA_INTERNAL
	gintmsk_data_t				intr_mask = {0};
#endif

#if defined CHECK_PACKET_COUNTER_WIDTH
	const IMG_UINT32				MAX_XFER_SIZE = core_if->core_params.max_transfer_size;
	const IMG_UINT32				MAX_PKT_COUNT = core_if->core_params.max_packet_count;
	IMG_UINT32						num_packets;
	IMG_UINT32						transfer_len;
	gnptxsts_data_t					txstatus;
	IMG_INT32						lvl		= SET_DEBUG_LEVEL(DBG_PCD);

	DWC_DEBUGPL(DBG_PCD, "ep%d-%s xfer_len=%d xfer_cnt=%d "
		"xfer_buff=%p start_xfer_buff=%p\n",
		ep->num, (ep->is_in?"IN":"OUT"), ep->xfer_len, 
		ep->xfer_count, ep->xfer_buff, ep->start_xfer_buff);

	transfer_len = ep->xfer_len - ep->xfer_count;

	if (transfer_len > MAX_XFER_SIZE) 
	{
		transfer_len = MAX_XFER_SIZE;
	}
	if (transfer_len == 0) 
	{
		num_packets = 1;
		/* OUT EP to recieve Zero-length packet set transfer
		 * size to maxpacket size. */
		if (!ep->is_in) 
		{
			transfer_len = ep->maxpacket;				  
		}
	} 
	else 
	{
		num_packets = (transfer_len + ep->maxpacket - 1) / ep->maxpacket;
		
		if (num_packets > MAX_PKT_COUNT) 
		{
			num_packets = MAX_PKT_COUNT;
		}
	}
	DWC_DEBUGPL(DBG_PCD, "transfer_len=%d #pckt=%d\n", transfer_len, num_packets);

#if !defined (USE_DDMA_ONLY)
	deptsiz.b.xfersize = transfer_len;
	deptsiz.b.pktcnt = num_packets;
#endif

	/* IN endpoint */
	if (ep->is_in == 1) 
	{
		depctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + ep->num*DWC_EP_REG_OFFSET);
	} 
	/* OUT endpoint */
	else 
	{
		depctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->num*DWC_EP_REG_OFFSET);
	}
		
	/* EP enable, IN data in FIFO */
	depctl.b.cnak = 1;
	depctl.b.epena = 1;
	/* IN endpoint */
	if (ep->is_in == 1) 
	{
		txstatus.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GNPTXSTS_OFFSET);
		
		if (txstatus.b.nptxqspcavail == 0) 
		{
			DWC_DEBUGPL(DBG_ANY, "TX Queue Full (0x%0x)\n", txstatus.d32);
			return;
		}
		
	  #if !defined (USE_DDMA_ONLY)
		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + ep->num*DWC_EP_REG_OFFSET, deptsiz.d32);
	  #endif
		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + ep->num*DWC_EP_REG_OFFSET;, depctl.d32);
		
		/** 
		 * Enable the Non-Periodic Tx FIFO empty interrupt, the
		 * data will be written into the fifo by the ISR.
		 */ 
	#if !defined USE_DMA_INTERNAL
		if (core_if->dma_enable) 
	#endif
		{
			usb_WriteReg32 (DWC_OTG_DEV_IN_EP_REGS_DIEPDMA0 + ep->num*DWC_EP_REG_OFFSET, (IMG_UINT32)ep->xfer_buff);
		}
	#if !defined USE_DMA_INTERNAL
		else 
		{	
		  #if !defined USE_MULTIPLE_TX_FIFO
			if(core_if->en_multiple_tx_fifo == 0)
			{
				intr_mask.b.nptxfempty = 1;
				usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, intr_mask.d32, 0);
				usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, intr_mask.d32);
			}
			else
		  #endif
			{						
				/* Enable the Tx FIFO Empty Interrupt for this EP */
				if(ep->xfer_len > 0 && ep->type != DWC_OTG_EP_TYPE_ISOC)
				{
					IMG_UINT32 fifoemptymsk = 0;
					fifoemptymsk = (0x1 << ep->num);
					usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DTKNQR4_FIFOEMPTYMSK_OFFSET, 0, fifoemptymsk);
				}
			}
		}
	#endif

	} 
	else 
	{ 
		/* OUT endpoint */
	  #if !defined (USE_DDMA_ONLY)
		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + ep->num*DWC_EP_REG_OFFSET, deptsiz.d32);
	  #endif
		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->num*DWC_EP_REG_OFFSET, depctl.d32);

	  #if !defined USE_DMA_INTERNAL
		if (core_if->dma_enable)
	  #endif
		{
			usb_WriteReg32 (DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0 + ep->num*DWC_EP_REG_OFFSET, (IMG_UINT32)ep->xfer_buff);
		}
	}
	DWC_DEBUGPL(DBG_PCD, "DOEPCTL=%08x DOEPTSIZ=%08x\n", usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->num*DWC_EP_REG_OFFSET), usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + ep->num*DWC_EP_REG_OFFSET));
	
	SET_DEBUG_LEVEL(lvl);
  #endif  //#if defined CHECK_PACKET_COUNTER_WIDTH

	DWC_DEBUGPL((DBG_PCDV | DBG_CILV), "%s()\n", __func__);
		
	DWC_DEBUGPL(DBG_PCD, "ep%d-%s xfer_len=%d xfer_cnt=%d "
		"xfer_buff=%p start_xfer_buff=%p\n",
		ep->num, (ep->is_in?"IN":"OUT"), ep->xfer_len, 
		ep->xfer_count, ep->xfer_buff, ep->start_xfer_buff);

	/* IN endpoint */
	if (ep->is_in == 1) 
	{
	  #if !defined USE_MULTIPLE_TX_FIFO
		gnptxsts_data_t gtxstatus;
					
		gtxstatus.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GNPTXSTS_OFFSET);

		if(core_if->en_multiple_tx_fifo == 0 && gtxstatus.b.nptxqspcavail == 0)
		{
		  #if defined USBD_DEBUG
			DWC_PRINT("TX Queue Full (0x%0x)\n", gtxstatus.d32);
		  #endif
			return;
		}
	  #endif /* !USE_MULTIPLE_TX_FIFO */
				
		depctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + ep->num * DWC_EP_REG_OFFSET);
	  #if !defined (USE_DDMA_ONLY)
		deptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + ep->num * DWC_EP_REG_OFFSET);
	  #endif
		ep->xfer_len += (ep->maxxfer < (ep->total_len - ep->xfer_len)) ? ep->maxxfer : (ep->total_len - ep->xfer_len);

	  #if !defined (USE_DDMA_ONLY)
		/* Zero Length Packet? */
		if ((ep->xfer_len - ep->xfer_count) == 0) 
		{
			deptsiz.b.xfersize = 0;
			deptsiz.b.pktcnt = 1;
		} 
		else 
		{
			/* Program the transfer size and packet count
			 *	as follows: xfersize = N * maxpacket +
			 *	short_packet pktcnt = N + (short_packet
			 *	exist ? 1 : 0)	
			 */
			deptsiz.b.xfersize = ep->xfer_len - ep->xfer_count;
			deptsiz.b.pktcnt = (ep->xfer_len - ep->xfer_count - 1 + ep->maxpacket) / ep->maxpacket;
		}
	  #endif

	  #if !defined (USE_DDMA_ONLY)
	  #if 1	/* bojan: this fixes occasional DATA2 packet creeping in when 1 packet transfers are used */
		if(ep->type == DWC_OTG_EP_TYPE_ISOC)
		{
			deptsiz.b.mc = 1;
		}
	  #endif
	  #endif
		/* Write the DMA register */
	#if !defined USE_DMA_INTERNAL
		if (core_if->dma_enable)
	#endif /* !USE_DMA_INTERNAL */
		{	
		  #if !defined USE_DDMA_ONLY
			if ( core_if->dma_desc_enable == 0 )
			{
				usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + ep->num * DWC_EP_REG_OFFSET, deptsiz.d32);
				usb_WriteMemrefReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPDMA0 + ep->num*DWC_EP_REG_OFFSET, ep->dma_addr);
			}
			else
		  #endif /* !USE_DDMA_ONLY */
			{
				init_dma_desc_chain( psBlockDesc, ep );
				/** DIEPDMAn Register write */
				usb_WriteMemrefReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPDMA0 + ep->num * DWC_EP_REG_OFFSET, ep->dma_desc_addr );
			}
		}
	#if !defined USE_DMA_INTERNAL
		else 
		{
			usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + ep->num * DWC_EP_REG_OFFSET, deptsiz.d32);
			if(ep->type != DWC_OTG_EP_TYPE_ISOC)
			{
				/** 
				 * Enable the Non-Periodic Tx FIFO empty interrupt,
				 * or the Tx FIFO epmty interrupt in dedicated Tx FIFO mode,
				 * the data will be written into the fifo by the ISR.
				 */ 
			#if !defined USE_MULTIPLE_TX_FIFO
				if(core_if->en_multiple_tx_fifo == 0)
				{
					intr_mask.b.nptxfempty = 1;
					usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, intr_mask.d32);
				}
				else
			#endif
				{						
					/* Enable the Tx FIFO Empty Interrupt for this EP */
					if(ep->xfer_len > 0)
					{
						IMG_UINT32 fifoemptymsk = 0;
						fifoemptymsk = 1 << ep->num;
						usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DTKNQR4_FIFOEMPTYMSK_OFFSET, 0, fifoemptymsk);
					}
				}
			}
		}
	#endif	 //#if !defined USE_DMA_INTERNAL

		/* EP enable, IN data in FIFO */
		depctl.b.cnak = 1;
		depctl.b.epena = 1;
		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + ep->num * DWC_EP_REG_OFFSET, depctl.d32);

		depctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0);
		depctl.b.nextep = ep->num;
		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0, depctl.d32);
	} 
	else 
	{
		/* OUT endpoint */
		depctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->num*DWC_EP_REG_OFFSET);
	  #if !defined (USE_DDMA_ONLY)
		deptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + ep->num*DWC_EP_REG_OFFSET);
	  #endif
		ep->xfer_len += (ep->maxxfer < (ep->total_len - ep->xfer_len)) ? ep->maxxfer : (ep->total_len - ep->xfer_len);

		/* Program the transfer size and packet count as follows:
		 * 
		 *	pktcnt = N										   
		 *	xfersize = N * maxpacket
		 */
		if ((ep->xfer_len - ep->xfer_count) == 0) 
		{
		  #if !defined (USE_DDMA_ONLY)
			/* Zero Length Packet */
			deptsiz.b.xfersize = ep->maxpacket;
			deptsiz.b.pktcnt = 1;
		  #endif
		} 
		else 
		{
		  #if !defined (USE_DDMA_ONLY)
			deptsiz.b.pktcnt = (ep->xfer_len - ep->xfer_count + (ep->maxpacket - 1)) / ep->maxpacket;
			ep->xfer_len = deptsiz.b.pktcnt * ep->maxpacket + ep->xfer_count;
			deptsiz.b.xfersize = ep->xfer_len - ep->xfer_count;
	      #else
			ep->xfer_len = ((ep->xfer_len - ep->xfer_count + (ep->maxpacket - 1)) / ep->maxpacket) * ep->maxpacket + ep->xfer_count;
		  #endif
		}

		DWC_DEBUGPL(DBG_PCDV, "ep%d xfersize=%d pktcnt=%d\n", ep->num, deptsiz.b.xfersize, deptsiz.b.pktcnt);

	#if !defined USE_DMA_INTERNAL
		if (core_if->dma_enable)
	#endif /* USE_DMA_INTERNAL */
		{
		  #if !defined (USE_DDMA_ONLY)
			if ( !core_if->dma_desc_enable )
			{
				usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + ep->num*DWC_EP_REG_OFFSET, deptsiz.d32);
				usb_WriteMemrefReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0 + ep->num*DWC_EP_REG_OFFSET, ep->dma_addr);
			}
			else
		  #endif /* !USE_DDMA_ONLY */
			{
				init_dma_desc_chain( psBlockDesc, ep );					
				/** DOEPDMAn Register write */
				usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0 + ep->num*DWC_EP_REG_OFFSET, ep->dma_desc_addr);
			}
		}
	#if !defined USE_DMA_INTERNAL
		else
		{
			usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + ep->num*DWC_EP_REG_OFFSET, deptsiz.d32);
		}
	#endif /* !USE_DMA_INTERNAL */

		/* EP enable */
		depctl.b.cnak = 1;
		depctl.b.epena = 1;

		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->num*DWC_EP_REG_OFFSET, depctl.d32);

		DWC_DEBUGPL(DBG_PCD, "DOEPCTL=%08x DOEPTSIZ=%08x\n", usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->num*DWC_EP_REG_OFFSET), usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + ep->num*DWC_EP_REG_OFFSET));
	}
}

/**
 * This function setup a zero length transfer in Buffer DMA and
 * Slave modes for usb requests with zero field set
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP to start the transfer on.
 *
 */

#if !defined (USE_DDMA_ONLY)
IMG_VOID dwc_otg_ep_start_zl_transfer( const img_uint32 ui32BaseAddress, dwc_otg_core_if_t	*	core_if, dwc_ep_t *ep)
{
 	depctl_data_t		depctl;
	deptsiz_data_t		deptsiz;
#if !defined USE_DMA_INTERNAL
	gintmsk_data_t		intr_mask = { .d32 = 0};
#endif

	DWC_DEBUGPL((DBG_PCDV | DBG_CILV), "%s()\n", __func__);
		
	/* IN endpoint */
	if (ep->is_in == 1) 
	{
		depctl.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + ep->num * DWC_EP_REG_OFFSET);
		deptsiz.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + ep->num * DWC_EP_REG_OFFSET);

		deptsiz.b.xfersize = 0;
		deptsiz.b.pktcnt = 1;

		/* Write the DMA register */
	  #if !defined USE_DMA_INTERNAL
		if (core_if->dma_enable) 
		{
	  #endif /* USE_DMA_INTERNAL */
			if (core_if->dma_desc_enable == 0) 
			{
				usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + ep->num * DWC_EP_REG_OFFSET, deptsiz.d32);
				usb_WriteMemrefReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPDMA0 + ep->num * DWC_EP_REG_OFFSET, (IMG_UINT32)ep->dma_addr);
			}
	  #if !defined USE_DMA_INTERNAL
		} 
		else 
		{
			usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + ep->num * DWC_EP_REG_OFFSET, deptsiz.d32);
			/** 
			 * Enable the Non-Periodic Tx FIFO empty interrupt,
			 * or the Tx FIFO epmty interrupt in dedicated Tx FIFO mode,
			 * the data will be written into the fifo by the ISR.
			 */ 
		  #if !defined USE_MULTIPLE_TX_FIFO
			if(core_if->en_multiple_tx_fifo == 0) 
		  
			{
				intr_mask.b.nptxfempty = 1;
				usb_ModifyReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, intr_mask.d32);
			}
		  #endif
			else 
			{						
				/* Enable the Tx FIFO Empty Interrupt for this EP */
				if(ep->xfer_len > 0) 
				{
					IMG_UINT32 fifoemptymsk = 0;
					fifoemptymsk = 1 << ep->num;
					usb_ModifyReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DTKNQR4_FIFOEMPTYMSK_OFFSET, 0, fifoemptymsk);
				}
			}
		}
	  #endif /* !USE_DMA_INTERNAL */
				
		/* EP enable, IN data in FIFO */
		depctl.b.cnak = 1;
		depctl.b.epena = 1;
		usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + ep->num * DWC_EP_REG_OFFSET, depctl.d32);

		depctl.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0);
		depctl.b.nextep = ep->num;
		usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0, depctl.d32);

	} 
	else 
	{
		/* OUT endpoint */
		depctl.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->num*DWC_EP_REG_OFFSET);
		deptsiz.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + ep->num*DWC_EP_REG_OFFSET);

		/* Zero Length Packet */
		deptsiz.b.xfersize = ep->maxpacket;
		deptsiz.b.pktcnt = 1;

	  #if !defined USE_DMA_INTERNAL
		if (core_if->dma_enable) 
		{
	  #endif /* USE_DMA_INTERNAL */
			if (!core_if->dma_desc_enable) 
			{
				usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + ep->num*DWC_EP_REG_OFFSET, deptsiz.d32);
				usb_WriteMemrefReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0 + ep->num*DWC_EP_REG_OFFSET, (IMG_UINT32)ep->dma_addr);
			}
	  #if !defined USE_DMA_INTERNAL
		}
		else 
		{
			usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + ep->num*DWC_EP_REG_OFFSET, deptsiz.d32);
		}
	  #endif /* USE_DMA_INTERNAL */

		/* EP enable */
		depctl.b.cnak = 1;
		depctl.b.epena = 1;

		usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->num*DWC_EP_REG_OFFSET, depctl.d32);
	}
}

#endif /* !USE_DDMA_ONLY */


/**
 * This function does the setup for a data transfer for EP0 and starts
 * the transfer.  For an IN transfer, the packets will be loaded into
 * the appropriate Tx FIFO in the ISR. For OUT transfers, the packets are
 * unloaded from the Rx FIFO in the ISR.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP0 data.
 */
IMG_VOID dwc_otg_ep0_start_transfer( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_core_if_t * core_if, dwc_ep_t *ep )
{
	depctl_data_t			depctl;
  #if !defined (USE_DDMA_ONLY)
	deptsiz0_data_t			deptsiz;
  #endif
  #if !defined USE_DMA_INTERNAL
	gintmsk_data_t			intr_mask = {0};
  #endif
	dwc_otg_dev_dma_desc_t	*dma_desc;

	DWC_DEBUGPL(DBG_PCD, "ep%d-%s xfer_len=%d xfer_cnt=%d "
	"xfer_buff=%p start_xfer_buff=%p \n",
	ep->num, (ep->is_in?"IN":"OUT"), ep->xfer_len, 
	ep->xfer_count, ep->xfer_buff, ep->start_xfer_buff);

	ep->total_len = ep->xfer_len;

	/* IN endpoint */
	if (ep->is_in == 1) 
	{ 
		gnptxsts_data_t gtxstatus;
					
		gtxstatus.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GNPTXSTS_OFFSET);

	#if defined USE_MULTIPLE_TX_FIFO
		if(gtxstatus.b.nptxqspcavail == 0)
	#else
		if(core_if->en_multiple_tx_fifo == 0 && gtxstatus.b.nptxqspcavail == 0)
	#endif
		{
		  #if defined USBD_DEBUG
			deptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0);

			DWC_DEBUGPL(DBG_PCD,"DIEPCTL0=%0x\n", usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0));
			DWC_DEBUGPL(DBG_PCD, "DIEPTSIZ0=%0x (sz=%d, pcnt=%d)\n", deptsiz.d32, deptsiz.b.xfersize, deptsiz.b.pktcnt);
			DWC_PRINT("TX Queue or FIFO Full (0x%0x)\n", gtxstatus.d32);
		  #endif
			return;
		}
	
	
		depctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0);
	  #if !defined (USE_DDMA_ONLY)
		deptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0);
	  #endif
	
	  #if !defined (USE_DDMA_ONLY)
		/* Zero Length Packet? */
		if (ep->xfer_len == 0) 
		{
			deptsiz.b.xfersize = 0;
			deptsiz.b.pktcnt = 1;
		} 
		else 
		{
			/* Program the transfer size and packet count
			 *	as follows: xfersize = N * maxpacket +
			 *	short_packet pktcnt = N + (short_packet
			 *	exist ? 1 : 0)	
			 */
			if (ep->xfer_len > ep->maxpacket) 
			{
				ep->xfer_len = ep->maxpacket;
				deptsiz.b.xfersize = ep->maxpacket;
			}
			else 
			{
				deptsiz.b.xfersize = ep->xfer_len;
			}
			deptsiz.b.pktcnt = 1;	
		}
	  #else
		if ( ep->xfer_len > ep->maxpacket )
		{
			ep->xfer_len = ep->maxpacket;
		}
	  #endif

		DWC_DEBUGPL(DBG_PCDV, "IN len=%d  xfersize=%d pktcnt=%d [%08x]\n", ep->xfer_len, deptsiz.b.xfersize, deptsiz.b.pktcnt, deptsiz.d32);
	
		/* Write the DMA register */
	#if !defined USE_DMA_INTERNAL
		if (core_if->dma_enable)
	#endif /* USE_DMA_INTERNAL */
	#if !defined (USE_DDMA_ONLY)
		{
			if( core_if->dma_desc_enable == 0 )
			{
				usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0, deptsiz.d32);
				usb_WriteMemrefReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPDMA0, (IMG_UINT32)ep->dma_addr);
			}
			else
			{
	#endif /* USE_DDMA_ONLY */
				dma_desc = core_if->dev_if.in_desc_addr;
	
				/** DMA Descriptor Setup */
				dma_desc->status.b.bs = BS_HOST_BUSY;
				dma_desc->status.b.l = 1;
				dma_desc->status.b.ioc = 1;
				dma_desc->status.b.sp = (ep->xfer_len == ep->maxpacket) ? 0 : 1;
				dma_desc->status.b.bytes = ep->xfer_len;
				dma_desc->buf = ep->dma_addr;
				dma_desc->status.b.bs = BS_HOST_READY;
				
				/** DIEPDMA0 Register write */
				usb_WriteMemrefReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPDMA0, core_if->dev_if.dma_in_desc_addr);
	#if !defined (USE_DDMA_ONLY)
			}
		}
	#endif /* !USE_DDMA_ONLY */
	#if !defined USE_DMA_INTERNAL
		else
		{
			usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0, deptsiz.d32);
		}
	#endif /* USE_DMA_INTERNAL */
	
		/* EP enable, IN data in FIFO */
		depctl.b.cnak = 1;
		depctl.b.epena = 1;
		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0, depctl.d32);
	
	#if !defined USE_DMA_INTERNAL
		/** 
		 * Enable the Non-Periodic Tx FIFO empty interrupt, the
		 * data will be written into the fifo by the ISR.
		 */
		if (!core_if->dma_enable) 
		{
		  #if !defined USE_MULTIPLE_TX_FIFO
			if(core_if->en_multiple_tx_fifo == 0)
			{
				intr_mask.b.nptxfempty = 1;
//				usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTSTS_OFFSET, intr_mask.d32, 0); MIKE: This was taken out in 2.71a
				
				usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, intr_mask.d32);
			}
			else
		  #endif
			{						
				/* Enable the Tx FIFO Empty Interrupt for this EP */
				if(ep->xfer_len > 0)
				{
					IMG_UINT32 fifoemptymsk = 0;
					fifoemptymsk |= 1 << ep->num;
					usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DTKNQR4_FIFOEMPTYMSK_OFFSET, 0, fifoemptymsk);
				}
			}
		}
	#endif /* USE_DMA_INTERNAL */
	} 
	else 
	{ 
		/* OUT endpoint */
		depctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0);
	  #if !defined (USE_DDMA_ONLY)
		deptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0);

		/* Program the transfer size and packet count as follows:
		 *	xfersize = N * (maxpacket + 4 - (maxpacket % 4))
		 *	pktcnt = N											*/
		/* Zero Length Packet */
		deptsiz.b.xfersize = ep->maxpacket;
		deptsiz.b.pktcnt = 1;
	  #endif
				
		DWC_DEBUGPL(DBG_PCDV, "len=%d  xfersize=%d pktcnt=%d\n", ep->xfer_len, deptsiz.b.xfersize, deptsiz.b.pktcnt);

	#if !defined USE_DMA_INTERNAL
		if (core_if->dma_enable)
	#endif /* USE_DMA_INTERNAL */
	#if !defined (USE_DDMA_ONLY)
		{
			if ( !core_if->dma_desc_enable )
			{
				usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0, deptsiz.d32);
				usb_WriteMemrefReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0, (IMG_UINT32)ep->dma_addr);
			}
			else
			{
	#endif /* !USE_DDMA_ONLY */
				dma_desc = core_if->dev_if.out_desc_addr;
	
				/** DMA Descriptor Setup */
				dma_desc->status.b.bs = BS_HOST_BUSY;
				dma_desc->status.b.l = 1;
				dma_desc->status.b.ioc = 1;
				dma_desc->status.b.bytes = ep->maxpacket;
				dma_desc->buf = ep->dma_addr;
				dma_desc->status.b.bs = BS_HOST_READY;

				/** DOEPDMA0 Register write */
				usb_WriteMemrefReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0, core_if->dev_if.dma_out_desc_addr);
	#if !defined (USE_DDMA_ONLY)
			}
		}
	#endif /* !USE_DDMA_ONLY */
	#if !defined USE_DMA_INTERNAL
		else
		{
			usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0, deptsiz.d32);
		}
	#endif /* USE_DMA_INTERNAL */

		/* EP enable */
		depctl.b.cnak = 1;
		depctl.b.epena = 1;
		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0, depctl.d32);
	}
}

/**
 * This function continues control IN transfers started by
 * dwc_otg_ep0_start_transfer, when the transfer does not fit in a
 * single packet.  NOTE: The DIEPCTL0/DOEPCTL0 registers only have one
 * bit for the packet count.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP0 data.
 */
IMG_VOID dwc_otg_ep0_continue_transfer( const ioblock_sBlockDescriptor	*	psBlockDesc, dwc_otg_core_if_t	 *	core_if, dwc_ep_t *ep )
{
	depctl_data_t				depctl;
  #if !defined (USE_DDMA_ONLY)
	deptsiz0_data_t				deptsiz;
  #endif
  #if !defined USE_DMA_INTERNAL
	gintmsk_data_t				intr_mask = {0};
  #endif
	dwc_otg_dev_dma_desc_t	*	dma_desc;

	if (ep->is_in == 1) 
	{
		gnptxsts_data_t				tx_status = {0};


		tx_status.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GNPTXSTS_OFFSET);
		/* IMG_SYNOP_CR */
		depctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0);
	  #if !defined (USE_DDMA_ONLY)
		deptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0);
	  #endif

		/* Program the transfer size and packet count
		 *	as follows: xfersize = N * maxpacket +
		 *	short_packet pktcnt = N + (short_packet
		 *	exist ? 1 : 0)	
		 */
	  #if !defined (USE_DDMA_ONLY)
		if ( !core_if->dma_desc_enable  )
		{
			deptsiz.b.xfersize = (ep->total_len - ep->xfer_count) > ep->maxpacket ? ep->maxpacket : (ep->total_len - ep->xfer_count);
			deptsiz.b.pktcnt = 1;
			if (core_if->dma_enable == 0)
			{
				ep->xfer_len += deptsiz.b.xfersize;
			}
			else
			{
				ep->xfer_len = deptsiz.b.xfersize;
			}
			usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0, deptsiz.d32);
		}
		else
		{
	  #endif /* !USE_DDMA_ONLY */
			ep->xfer_len = (ep->total_len - ep->xfer_count) > ep->maxpacket ? ep->maxpacket : (ep->total_len - ep->xfer_count);

			dma_desc = core_if->dev_if.in_desc_addr;

			/** DMA Descriptor Setup */
			dma_desc->status.b.bs = BS_HOST_BUSY;
			dma_desc->status.b.l = 1;
			dma_desc->status.b.ioc = 1;
			dma_desc->status.b.sp = (ep->xfer_len == ep->maxpacket) ? 0 : 1;
			dma_desc->status.b.bytes = ep->xfer_len;
			dma_desc->buf = ep->dma_addr;
			dma_desc->status.b.bs = BS_HOST_READY; 
				
			/** DIEPDMA0 Register write */
			usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPDMA0, core_if->dev_if.dma_in_desc_addr);
	  #if !defined (USE_DDMA_ONLY)
		}		
	  #endif /* !USE_DDMA_ONLY */

		DWC_DEBUGPL(DBG_PCDV, "IN len=%d  xfersize=%d pktcnt=%d [%08x]\n", ep->xfer_len, deptsiz.b.xfersize, deptsiz.b.pktcnt, deptsiz.d32);

	  #if !defined (USE_DDMA_ONLY)
		/* Write the DMA register */
		if (core_if->hwcfg2.b.architecture == DWC_INT_DMA_ARCH) 
		{
			if ( !core_if->dma_desc_enable )
			{
				usb_WriteMemrefReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPDMA0, (IMG_UINT32)ep->dma_addr);
			}
		}
	  #endif /* !USE_DDMA_ONLY */

		/* EP enable, IN data in FIFO */
		depctl.b.cnak = 1;
		depctl.b.epena = 1;
		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0, depctl.d32);

		#if !defined USE_DMA_INTERNAL
		/** 
		 * Enable the Non-Periodic Tx FIFO empty interrupt, the
		 * data will be written into the fifo by the ISR.
		 */ 
		if (!core_if->dma_enable) 
		{
			if ( core_if->en_multiple_tx_fifo == 0 )
			{
				/* First clear it from GINTSTS */
				intr_mask.b.nptxfempty = 1;
				usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_CORE_GLOBAL_REGS_GINTMSK_OFFSET, intr_mask.d32, intr_mask.d32);
			}
			else
			{
				/* Enable the TX FIFO Empty Interrupt for this EP */
				if ( ep->xfer_len > 0 )
				{
					IMG_UINT32 fifoemptymsk = 0;
					fifoemptymsk |= 1 << ep->num;
					usb_ModifyReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_GLOBAL_REGS_DTKNQR4_FIFOEMPTYMSK_OFFSET, 0, fifoemptymsk );
				}
			}
		}
		#endif /* USE_DMA_INTERNAL */
	}
	else
	{
		depctl.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0);
	  #if !defined (USE_DDMA_ONLY)
		deptsiz.d32 = usb_ReadReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0);

		/* Program the transfer size and packet count
		 *	as follows: xfersize = N * maxpacket +
		 *	short_packet pktcnt = N + (short_packet
		 *	exist ? 1 : 0)	
		 */
		deptsiz.b.xfersize = ep->maxpacket;
		deptsiz.b.pktcnt = 1;
	  #endif
		
	  #if !defined (USE_DDMA_ONLY)
		if (core_if->dma_desc_enable == 0) 
		{	
			usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0, deptsiz.d32);
		}
		else 
		{
	  #endif /* USE_DDMA_ONLY */
			dma_desc = core_if->dev_if.out_desc_addr;

			/** DMA Descriptor Setup */
			dma_desc->status.b.bs = BS_HOST_BUSY;
			dma_desc->status.b.l = 1;
			dma_desc->status.b.ioc = 1;
			dma_desc->status.b.bytes = ep->maxpacket;
			dma_desc->buf = ep->dma_addr;
			dma_desc->status.b.bs = BS_HOST_READY;
			
			/** DOEPDMA0 Register write */
			usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0, core_if->dev_if.dma_out_desc_addr);
	  #if !defined (USE_DDMA_ONLY)
		}
	  #endif /* !USE_DDMA_ONLY */
		
		DWC_DEBUGPL(DBG_PCDV, "IN len=%d  xfersize=%d pktcnt=%d [%08x]\n",
			ep->xfer_len, 
			deptsiz.b.xfersize, deptsiz.b.pktcnt, deptsiz.d32);

	  #if !defined (USE_DDMA_ONLY)
		/* Write the DMA register */
		if (core_if->hwcfg2.b.architecture == DWC_INT_DMA_ARCH) 
		{
			if(core_if->dma_desc_enable == 0)
			{
				usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0, (IMG_UINT32)ep->dma_addr);
			}
		}
	  #endif /* !USE_DDMA_ONLY */

		/* EP enable, IN data in FIFO */
		depctl.b.cnak = 1;
		depctl.b.epena = 1;
		usb_WriteReg32( psBlockDesc->ui32Base, DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0, depctl.d32);
	}
}


/**
 * This function writes a packet into the Tx FIFO associated with the
 * EP.	For non-periodic EPs the non-periodic Tx FIFO is written.  For
 * periodic EPs the periodic Tx FIFO associated with the EP is written
 * with all packets for the next micro-frame.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP to write packet for.
 * @param dma Indicates if DMA is being used.
 */
IMG_VOID dwc_otg_ep_write_packet( const img_uint32 ui32BaseAddress, dwc_ep_t *ep, IMG_INT32 dma )
{
	/**
	 * The buffer is padded to DWORD on a per packet basis in
	 * slave/dma mode if the MPS is not DWORD aligned.	The last
	 * packet, if short, is also padded to a multiple of DWORD.
	 *
	 * ep->xfer_buff always starts DWORD aligned in memory and is a 
	 * multiple of DWORD in length
	 *
	 * ep->xfer_len can be any number of bytes
	 *
	 * ep->xfer_count is a multiple of ep->maxpacket until the last 
	 *	packet
	 *
	 * FIFO access is DWORD */

	IMG_UINT32 i;
	IMG_UINT32 byte_count;
	IMG_UINT32 dword_count;
	IMG_UINT32 fifo;
	IMG_UINT32 *data_buff = (IMG_UINT32 *)ep->xfer_buff;
		

	if (ep->xfer_count >= ep->xfer_len) 
	{
		DWC_WARN("%s() No data for EP%d!\n", ep->num);
		return;				   
	}

	/* Find the byte length of the packet either short packet or MPS */
	if ((ep->xfer_len - ep->xfer_count) < ep->maxpacket) 
	{
		byte_count = ep->xfer_len - ep->xfer_count;
	}
	else 
	{
		byte_count = ep->maxpacket;
	}

	/* Find the DWORD length, padded by extra bytes as neccessary if MPS
	 * is not a multiple of DWORD */
	dword_count =  (byte_count + 3) / 4;

	/* IMG_SYNOP_CR */
	/* Potential enhancement: Check where the periodic Tx FIFO addresses are intialized */		
	fifo = DWC_OTG_DATA_FIFO_OFFSET + (ep->num * DWC_OTG_DATA_FIFO_SIZE);

	 DWC_DEBUGPL((DBG_PCDV|DBG_CILV), "fifo=%p buff=%p *p=%08x bc=%d\n", fifo, data_buff, *data_buff, byte_count);

	if (!dma) 
	{
		for (i=0; i<dword_count; i++, data_buff++) 
		{
			usb_WriteReg32( ui32BaseAddress,  fifo, *data_buff );
		}
	}

	ep->xfer_count += byte_count;
	ep->xfer_buff += byte_count;
	ep->dma_addr += byte_count;
}

/** 
 * Set the EP STALL.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP to set the stall on.
 */
IMG_VOID dwc_otg_ep_set_stall( const img_uint32 ui32BaseAddress, dwc_ep_t *ep )
{
	depctl_data_t			depctl;
	IMG_UINT32				depctl_addr;

	DWC_DEBUGPL(DBG_PCD, "%s ep%d-%s\n", __func__, ep->num, (ep->is_in?"IN":"OUT"));

	if (ep->is_in == 1) 
	{
		depctl_addr = DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + ep->num * DWC_EP_REG_OFFSET;
		depctl.d32 = usb_ReadReg32( ui32BaseAddress, depctl_addr);
	
		/* set the disable and stall bits */
		if (depctl.b.epena) 
		{
			depctl.b.epdis = 1;
		}
		depctl.b.stall = 1;
		usb_WriteReg32( ui32BaseAddress, depctl_addr, depctl.d32);
	} 
	else 
	{
		depctl_addr = DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->num * DWC_EP_REG_OFFSET;
		depctl.d32 = usb_ReadReg32( ui32BaseAddress, depctl_addr);

		/* set the stall bit */
		depctl.b.stall = 1;
		usb_WriteReg32( ui32BaseAddress, depctl_addr, depctl.d32);
	}
	
	DWC_DEBUGPL(DBG_PCD,"DEPCTL=%0x\n",usb_ReadReg32( ui32BaseAddress, depctl_addr));
	
	return;
}

/** 
 * Clear the EP STALL.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP to clear stall from.
 */
IMG_VOID dwc_otg_ep_clear_stall( const img_uint32 ui32BaseAddress, dwc_ep_t *ep )
{
	depctl_data_t			depctl;
	IMG_UINT32				depctl_addr;

	DWC_DEBUGPL(DBG_PCD, "%s ep%d-%s\n", __func__, ep->num, (ep->is_in?"IN":"OUT"));

	if (ep->is_in == 1) 
	{
		depctl_addr = DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + ep->num*DWC_EP_REG_OFFSET;
	} 
	else
	{
		depctl_addr = DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->num*DWC_EP_REG_OFFSET;
	}

	depctl.d32 = usb_ReadReg32( ui32BaseAddress, depctl_addr);

	/* clear the stall bits */
	depctl.b.stall = 0;

	/* 
	 * USB Spec 9.4.5: For endpoints using data toggle, regardless
	 * of whether an endpoint has the Halt feature set, a
	 * ClearFeature(ENDPOINT_HALT) request always results in the
	 * data toggle being reinitialized to DATA0.
	 */
	if (ep->type == DWC_OTG_EP_TYPE_INTR || ep->type == DWC_OTG_EP_TYPE_BULK) 
	{
		depctl.b.setd0pid = 1; /* DATA0 */
	}
		
	usb_WriteReg32( ui32BaseAddress, depctl_addr, depctl.d32);
	DWC_DEBUGPL(DBG_PCD,"DEPCTL=%0x\n",usb_ReadReg32( ui32BaseAddress, depctl_addr));
	return;
}


#if !defined USE_DMA_INTERNAL
/** 
 * This function reads a packet from the Rx FIFO into the destination
 * buffer.	To read SETUP data use dwc_otg_read_setup_packet.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param dest	  Destination buffer for the packet.
 * @param bytes  Number of bytes to copy to the destination.
 */
IMG_VOID dwc_otg_read_packet( const img_uint32 ui32BaseAddress, IMG_UINT8 *dest, IMG_UINT16 bytes )
{
	IMG_INT32		i;
	IMG_INT32		word_count	= (bytes + 3) / 4;
	IMG_UINT32		fifo		= DWC_OTG_DATA_FIFO_OFFSET;
	IMG_UINT32		*data_buff	= (IMG_UINT32 *)dest;
	
	/*
	 * Potential enhancement: Account for the case where _dest is not dword 
	 * aligned. This requires reading data from the FIFO into a IMG_UINT32 temp buffer,
	 * then moving it into the data buffer.
	 */
	/* IMG_SYNOP_CR */	 

	DWC_DEBUGPL((DBG_PCDV | DBG_CILV), "%s(%p,%p,%d)\n", __func__, core_if, dest, bytes);

	for (i = 0; i < word_count; i++, data_buff++)
	{
		*data_buff = usb_ReadReg32( ui32BaseAddress, fifo);
	}

	return;
}
#endif


/**
 * Flush a Tx FIFO.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param num Tx FIFO to flush.
 */
extern IMG_VOID dwc_otg_flush_tx_fifo( const img_uint32 ui32BaseAddress , const IMG_INT32 num ) 
{
	volatile grstctl_t			greset			= {0};
	IMG_INT32					count			= 0;
	
	DWC_DEBUGPL((DBG_CIL|DBG_PCDV), "Flush Tx FIFO %d\n", num);

	greset.b.txfflsh = 1;
	greset.b.txfnum = num;
	usb_WriteReg32( ui32BaseAddress,  DWC_OTG_CORE_GLOBAL_REGS_GRSTCTL_OFFSET, greset.d32 );

	do 
	{
		greset.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GRSTCTL_OFFSET);
		if (++count > 10000)
		{
			DWC_WARN("%s() HANG! GRSTCTL=%0x GNPTXSTS=0x%08x\n", greset.d32, usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GNPTXSTS_OFFSET));
			break;
		}
	} 
	while (greset.b.txfflsh == 1);
	/* Wait for 3 PHY Clocks*/
	UDELAY(1);
}

/**
 * Flush Rx FIFO.
 *
 * @param core_if Programming view of DWC_otg controller.
 */
extern IMG_VOID dwc_otg_flush_rx_fifo( const img_uint32 ui32BaseAddress ) 
{
	volatile grstctl_t			greset			= {0};
	IMG_INT32					count			= 0;
		
	DWC_DEBUGPL((DBG_CIL|DBG_PCDV), "%s\n", __func__);
	/*
	 * 
	 */
	greset.b.rxfflsh = 1;
	usb_WriteReg32( ui32BaseAddress,  DWC_OTG_CORE_GLOBAL_REGS_GRSTCTL_OFFSET, greset.d32 );

	do 
	{
		greset.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GRSTCTL_OFFSET);
		if (++count > 10000)
		{
			DWC_WARN("%s() HANG! GRSTCTL=%0x\n", greset.d32);
			break;
		}
	} 
	while (greset.b.rxfflsh == 1);		  
	/* Wait for 3 PHY Clocks*/
	UDELAY(1);
}

/**
 * Do core a soft reset of the core.  Be careful with this because it
 * resets all the internal state machines of the core.
 */
IMG_VOID dwc_otg_core_reset( const img_uint32		ui32BaseAddress  )
{
	volatile grstctl_t			greset			= {0};
	IMG_INT32					count			= 0;

	DWC_DEBUGPL(DBG_CILV, "%s\n", __func__);

	/* Wait for AHB master IDLE state. */
	do 
	{
		UDELAY(10);
		greset.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GRSTCTL_OFFSET);
		if (++count > 100000)
		{
			DWC_WARN("%s() HANG! AHB Idle GRSTCTL=%0x\n", greset.d32);
			return;
		}
	} 
	while (greset.b.ahbidle == 0);
		
	/* Core Soft Reset */
	count = 0;
	greset.b.csftrst = 1;
	usb_WriteReg32( ui32BaseAddress,  DWC_OTG_CORE_GLOBAL_REGS_GRSTCTL_OFFSET, greset.d32 );

	do 
	{
		greset.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_CORE_GLOBAL_REGS_GRSTCTL_OFFSET);
		if (++count > 10000)
		{
			DWC_WARN("%s() HANG! Soft Reset GRSTCTL=%0x\n", greset.d32);
			break;
		}
	} 
	while (greset.b.csftrst == 1);
	
	/* Wait for 3 PHY Clocks*/
	//DWC_PRINT("100ms\n");
	MDELAY(100);
}



/**
 * Register HCD callbacks.	The callbacks are used to start and stop
 * the HCD for interrupt processing.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param cb the HCD callback structure.
 * @param p pointer to be passed to callback function (usb_hcd*).
 */
#if !defined DWC_DEVICE_ONLY
IMG_VOID dwc_otg_cil_register_hcd_callbacks( USB_DC_T	*	psContext, dwc_otg_cil_callbacks_t *cb, IMG_VOID *p )
{
	dwc_otg_core_if_t	*	core_if = psContext->psOtgCoreIf;

	core_if->hcd_cb = cb;
	cb->p = p;
}
#endif

/**
 * Register PCD callbacks.	The callbacks are used to start and stop
 * the PCD for interrupt processing.
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param cb the PCD callback structure.
 * @param p pointer to be passed to callback function (pcd*).
 */
extern IMG_VOID dwc_otg_cil_register_pcd_callbacks( dwc_otg_core_if_t *core_if, dwc_otg_cil_callbacks_t *cb, IMG_VOID *p  )
{
	core_if->pcd_cb = cb;
	cb->p = p;
}

#if defined _EN_ISOC_

/**
 * This function writes isoc data per 1 (micro)frame into tx fifo
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP to start the transfer on.
 *
 */
#if !defined USE_DMA_INTERNAL

IMG_VOID write_isoc_frame_data( const img_uint32 ui32BaseAddress, dwc_otg_core_if_t * core_if, dwc_ep_t * ep)
{
	dtxfsts_data_t				txstatus = {0};
	img_uint32					len = 0;
	img_uint32					dwords;

	ep->xfer_len = ep->data_per_frame;
	ep->xfer_count = 0;

	len = ep->xfer_len - ep->xfer_count;

	if (len > ep->maxpacket) 
	{
		len = ep->maxpacket;
	}

	dwords = (len + 3) / 4;

	/* While there is space in the queue and space in the FIFO and
	 * More data to tranfer, Write packets to the Tx FIFO */
	txstatus.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DTXFSTS0 + ep->num * DWC_EP_REG_OFFSET );
	DWC_DEBUGPL(DBG_PCDV, "b4 dtxfsts[%d]=0x%08x\n", ep->num, txstatus.d32);

	while ( ( txstatus.b.txfspcavail > dwords ) &&
	        ( ep->xfer_count < ep->xfer_len ) && 
			( ep->xfer_len != 0 ) )
	{
		/* Write the FIFO */
		dwc_otg_ep_write_packet( ui32BaseAddress, ep, 0 );

		len = ep->xfer_len - ep->xfer_count;
		if (len > ep->maxpacket) 
		{
			len = ep->maxpacket;
		}

		dwords = (len + 3) / 4;
		txstatus.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DTXFSTS0 + ep->num * DWC_EP_REG_OFFSET )
		DWC_DEBUGPL(DBG_PCDV, "dtxfsts[%d]=0x%08x\n", ep->num, txstatus.d32);
	}
}

#endif /* USE_DMA_INTERNAL */

/**
 * This function initializes a descriptor chain for Isochronous transfer
 *
 * @param core_if Programming view of DWC_otg controller.
 * @param ep The EP to start the transfer on.
 *
 */
IMG_VOID dwc_otg_iso_ep_start_frm_transfer(	const img_uint32 ui32BaseAddress, dwc_otg_core_if_t * core_if, dwc_ep_t * ep)
{
	deptsiz_data_t			deptsiz	= { 0 };
	depctl_data_t			depctl	= { 0 };
	dsts_data_t				dsts	= { 0 };
	img_uint32				addr;

	if (ep->is_in) 
	{
		addr = DWC_OTG_DEV_IN_EP_REGS_DIEPCTL0 + ep->num * DWC_EP_REG_OFFSET;
	} 
	else 
	{
		addr = DWC_OTG_DEV_OUT_EP_REGS_DOEPCTL0 + ep->num * DWC_EP_REG_OFFSET;
	}

	ep->xfer_len = ep->data_per_frame;
	ep->xfer_count = 0;
	ep->xfer_buff = ep->cur_pkt_addr;
	ep->dma_addr = ep->cur_pkt_dma_addr;

	if (ep->is_in) 
	{
		/* Program the transfer size and packet count
		 *      as follows: xfersize = N * maxpacket +
		 *      short_packet pktcnt = N + (short_packet
		 *      exist ? 1 : 0)  
		 */
		deptsiz.b.xfersize = ep->xfer_len;
		deptsiz.b.pktcnt = (ep->xfer_len - 1 + ep->maxpacket) / ep->maxpacket;
		deptsiz.b.mc = deptsiz.b.pktcnt;

		usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPTSIZ0 + ep->num * DWC_EP_REG_OFFSET, deptsiz.d32 );

		/* Write the DMA register */
	  #if !defined USE_DMA_INTERNAL
		if (core_if->dma_enable) 
	  #endif /* USE_DMA_INTERNAL */
		{
			usb_WriteMemrefReg32( ui32BaseAddress, DWC_OTG_DEV_IN_EP_REGS_DIEPDMA0 + ep->num * DWC_EP_REG_OFFSET, ep->dma_addr );
		}
	} 
	else 
	{
		deptsiz.b.pktcnt = (ep->xfer_len + (ep->maxpacket - 1)) / ep->maxpacket;
		deptsiz.b.xfersize = deptsiz.b.pktcnt * ep->maxpacket;

		usb_WriteReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPTSIZ0 + ep->num * DWC_EP_REG_OFFSET, deptsiz.d32 );

	  #if !defined USE_DMA_INTERNAL
		if (core_if->dma_enable) 
	  #endif /* USE_DMA_INTERNAL */
		{
			usb_WriteMemrefReg32( ui32BaseAddress, DWC_OTG_DEV_OUT_EP_REGS_DOEPDMA0 + ep->num * DWC_EP_REG_OFFSET, ep->dma_addr );
		}
	}

	/** Enable endpoint, clear nak  */

	depctl.d32 = 0;
	if (ep->bInterval == 1) 
	{
		dsts.d32 = usb_ReadReg32( ui32BaseAddress, DWC_OTG_DEV_GLOBAL_REGS_DSTS_OFFSET );
		ep->next_frame = dsts.b.soffn + ep->bInterval;

		if (ep->next_frame & 0x1) 
		{
			depctl.b.setd1pid = 1;
		} 
		else 
		{
			depctl.b.setd0pid = 1;
		}
	} 
	else 
	{
		ep->next_frame += ep->bInterval;

		if (ep->next_frame & 0x1) 
		{
			depctl.b.setd1pid = 1;
		} 
		else 
		{
			depctl.b.setd0pid = 1;
		}
	}
	depctl.b.epena = 1;
	depctl.b.cnak = 1;

	usb_ModifyReg32( ui32BaseAddress, addr, 0, depctl.d32);
	depctl.d32 = usb_ReadReg32( ui32BaseAddress, addr );

  #if !defined USE_DMA_INTERNAL
	if (ep->is_in && core_if->dma_enable == 0) 
	{
		write_isoc_frame_data( ui32BaseAddress, core_if, ep);
	}
  #endif /* USE_DMA_INTERNAL */
}
#endif				/* _EN_ISOC */

#if 0
static IMG_VOID dwc_otg_set_uninitialized(int32_t * p, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		p[i] = -1;
	}
}

static int dwc_otg_param_initialized(int32_t val)
{
	return val != -1;
}

static int dwc_otg_setup_params(dwc_otg_core_if_t * core_if)
{
	int i;
	core_if->core_params = DWC_ALLOC(sizeof(*core_if->core_params));
	if (!core_if->core_params) {
		return -DWC_E_NO_MEMORY;
	}
	dwc_otg_set_uninitialized((int32_t *) core_if->core_params,
				  sizeof(*core_if->core_params) /
				  sizeof(int32_t));
	DWC_PRINTF("Setting default values for core params\n");
	dwc_otg_set_param_otg_cap(core_if, dwc_param_otg_cap_default);
	dwc_otg_set_param_dma_enable(core_if, dwc_param_dma_enable_default);
	dwc_otg_set_param_dma_desc_enable(core_if,
					  dwc_param_dma_desc_enable_default);
	dwc_otg_set_param_opt(core_if, dwc_param_opt_default);
	dwc_otg_set_param_dma_burst_size(core_if,
					 dwc_param_dma_burst_size_default);
	dwc_otg_set_param_host_support_fs_ls_low_power(core_if,
						       dwc_param_host_support_fs_ls_low_power_default);
	dwc_otg_set_param_enable_dynamic_fifo(core_if,
					      dwc_param_enable_dynamic_fifo_default);
	dwc_otg_set_param_data_fifo_size(core_if,
					 dwc_param_data_fifo_size_default);
	dwc_otg_set_param_dev_rx_fifo_size(core_if,
					   dwc_param_dev_rx_fifo_size_default);
	dwc_otg_set_param_dev_nperio_tx_fifo_size(core_if,
						  dwc_param_dev_nperio_tx_fifo_size_default);
	dwc_otg_set_param_host_rx_fifo_size(core_if,
					    dwc_param_host_rx_fifo_size_default);
	dwc_otg_set_param_host_nperio_tx_fifo_size(core_if,
						   dwc_param_host_nperio_tx_fifo_size_default);
	dwc_otg_set_param_host_perio_tx_fifo_size(core_if,
						  dwc_param_host_perio_tx_fifo_size_default);
	dwc_otg_set_param_max_transfer_size(core_if,
					    dwc_param_max_transfer_size_default);
	dwc_otg_set_param_max_packet_count(core_if,
					   dwc_param_max_packet_count_default);
	dwc_otg_set_param_host_channels(core_if,
					dwc_param_host_channels_default);
	dwc_otg_set_param_dev_endpoints(core_if,
					dwc_param_dev_endpoints_default);
	dwc_otg_set_param_phy_type(core_if, dwc_param_phy_type_default);
	dwc_otg_set_param_speed(core_if, dwc_param_speed_default);
	dwc_otg_set_param_host_ls_low_power_phy_clk(core_if,
						    dwc_param_host_ls_low_power_phy_clk_default);
	dwc_otg_set_param_phy_ulpi_ddr(core_if, dwc_param_phy_ulpi_ddr_default);
	dwc_otg_set_param_phy_ulpi_ext_vbus(core_if,
					    dwc_param_phy_ulpi_ext_vbus_default);
	dwc_otg_set_param_phy_utmi_width(core_if,
					 dwc_param_phy_utmi_width_default);
	dwc_otg_set_param_ts_dline(core_if, dwc_param_ts_dline_default);
	dwc_otg_set_param_i2c_enable(core_if, dwc_param_i2c_enable_default);
	dwc_otg_set_param_ulpi_fs_ls(core_if, dwc_param_ulpi_fs_ls_default);
	dwc_otg_set_param_en_multiple_tx_fifo(core_if,
					      dwc_param_en_multiple_tx_fifo_default);
	for (i = 0; i < 15; i++) {
		dwc_otg_set_param_dev_perio_tx_fifo_size(core_if,
							 dwc_param_dev_perio_tx_fifo_size_default,
							 i);
	}

	for (i = 0; i < 15; i++) {
		dwc_otg_set_param_dev_tx_fifo_size(core_if,
						   dwc_param_dev_tx_fifo_size_default,
						   i);
	}
	dwc_otg_set_param_thr_ctl(core_if, dwc_param_thr_ctl_default);
	dwc_otg_set_param_mpi_enable(core_if, dwc_param_mpi_enable_default);
	dwc_otg_set_param_pti_enable(core_if, dwc_param_pti_enable_default);
	dwc_otg_set_param_lpm_enable(core_if, dwc_param_lpm_enable_default);
	dwc_otg_set_param_ic_usb_cap(core_if, dwc_param_ic_usb_cap_default);
	dwc_otg_set_param_tx_thr_length(core_if,
					dwc_param_tx_thr_length_default);
	dwc_otg_set_param_rx_thr_length(core_if,
					dwc_param_rx_thr_length_default);
	dwc_otg_set_param_ahb_thr_ratio(core_if, dwc_param_ahb_thr_ratio_default);
	return 0;
}

uint8_t dwc_otg_is_dma_enable(dwc_otg_core_if_t * core_if)
{
	return core_if->dma_enable;
}

/* Checks if the parameter is outside of its valid range of values */
#define DWC_OTG_PARAM_TEST(_param_, _low_, _high_) \
		(((_param_) < (_low_)) || \
		((_param_) > (_high_)))

/* Parameter access functions */
int dwc_otg_set_param_otg_cap(dwc_otg_core_if_t * core_if, int32_t val)
{
	int valid;
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 2)) {
		DWC_WARN("Wrong value for otg_cap parameter\n");
		DWC_WARN("otg_cap parameter must be 0,1 or 2\n");
		retval = -DWC_E_INVALID;
		goto out;
	}

	valid = 1;
	switch (val) {
	case DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE:
		if (core_if->hwcfg2.b.op_mode !=
		    DWC_HWCFG2_OP_MODE_HNP_SRP_CAPABLE_OTG)
			valid = 0;
		break;
	case DWC_OTG_CAP_PARAM_SRP_ONLY_CAPABLE:
		if ((core_if->hwcfg2.b.op_mode !=
		     DWC_HWCFG2_OP_MODE_HNP_SRP_CAPABLE_OTG)
		    && (core_if->hwcfg2.b.op_mode !=
			DWC_HWCFG2_OP_MODE_SRP_ONLY_CAPABLE_OTG)
		    && (core_if->hwcfg2.b.op_mode !=
			DWC_HWCFG2_OP_MODE_SRP_CAPABLE_DEVICE)
		    && (core_if->hwcfg2.b.op_mode !=
		      DWC_HWCFG2_OP_MODE_SRP_CAPABLE_HOST)) {
			valid = 0;
		}
		break;
	case DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE:
		/* always valid */
		break;
	}
	if (!valid) {
		if (dwc_otg_param_initialized(core_if->core_params.otg_cap)) {
			DWC_ERROR
			    ("%d invalid for otg_cap paremter. Check HW configuration.\n",
			     val);
		}
		val =
		    (((core_if->hwcfg2.b.op_mode ==
		       DWC_HWCFG2_OP_MODE_HNP_SRP_CAPABLE_OTG)
		      || (core_if->hwcfg2.b.op_mode ==
			  DWC_HWCFG2_OP_MODE_SRP_ONLY_CAPABLE_OTG)
		      || (core_if->hwcfg2.b.op_mode ==
			  DWC_HWCFG2_OP_MODE_SRP_CAPABLE_DEVICE)
		      || (core_if->hwcfg2.b.op_mode ==
			  DWC_HWCFG2_OP_MODE_SRP_CAPABLE_HOST)) ?
		     DWC_OTG_CAP_PARAM_SRP_ONLY_CAPABLE :
		     DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.otg_cap = val;
      out:
	return retval;
}

int32_t dwc_otg_get_param_otg_cap(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.otg_cap;
}

int dwc_otg_set_param_opt(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for opt parameter\n");
		return -DWC_E_INVALID;
	}
	core_if->core_params.opt = val;
	return 0;
}

int32_t dwc_otg_get_param_opt(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.opt;
}

int dwc_otg_set_param_dma_enable(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for dma enable\n");
		return -DWC_E_INVALID;
	}

	if ((val == 1) && (core_if->hwcfg2.b.architecture == 0)) {
		if (dwc_otg_param_initialized(core_if->core_params.dma_enable)) {
			DWC_ERROR
			    ("%d invalid for dma_enable paremter. Check HW configuration.\n",
			     val);
		}
		val = 0;
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.dma_enable = val;
	if (val == 0) {
		dwc_otg_set_param_dma_desc_enable(core_if, 0);
	}
	return retval;
}

int32_t dwc_otg_get_param_dma_enable(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.dma_enable;
}

int dwc_otg_set_param_dma_desc_enable(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for dma_enable\n");
		DWC_WARN("dma_desc_enable must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	if ((val == 1)
	    && ((dwc_otg_get_param_dma_enable(core_if) == 0)
		|| (core_if->hwcfg4.b.desc_dma == 0))) {
		if (dwc_otg_param_initialized
		    (core_if->core_params.dma_desc_enable)) {
			DWC_ERROR
			    ("%d invalid for dma_desc_enable paremter. Check HW configuration.\n",
			     val);
		}
		val = 0;
		retval = -DWC_E_INVALID;
	}
	core_if->core_params.dma_desc_enable = val;
	return retval;
}

int32_t dwc_otg_get_param_dma_desc_enable(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.dma_desc_enable;
}

int dwc_otg_set_param_host_support_fs_ls_low_power(dwc_otg_core_if_t * core_if,
						   int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for host_support_fs_low_power\n");
		DWC_WARN("host_support_fs_low_power must be 0 or 1\n");
		return -DWC_E_INVALID;
	}
	core_if->core_params.host_support_fs_ls_low_power = val;
	return 0;
}

int32_t dwc_otg_get_param_host_support_fs_ls_low_power(dwc_otg_core_if_t *
						       core_if)
{
	return core_if->core_params.host_support_fs_ls_low_power;
}

int dwc_otg_set_param_enable_dynamic_fifo(dwc_otg_core_if_t * core_if,
					  int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for enable_dynamic_fifo\n");
		DWC_WARN("enable_dynamic_fifo must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	if ((val == 1) && (core_if->hwcfg2.b.dynamic_fifo == 0)) {
		if (dwc_otg_param_initialized
		    (core_if->core_params.enable_dynamic_fifo)) {
			DWC_ERROR
			    ("%d invalid for enable_dynamic_fifo paremter. Check HW configuration.\n",
			     val);
		}
		val = 0;
		retval = -DWC_E_INVALID;
	}
	core_if->core_params.enable_dynamic_fifo = val;
	return retval;
}

int32_t dwc_otg_get_param_enable_dynamic_fifo(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.enable_dynamic_fifo;
}

int dwc_otg_set_param_data_fifo_size(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 32, 32768)) {
		DWC_WARN("Wrong value for data_fifo_size\n");
		DWC_WARN("data_fifo_size must be 32-32768\n");
		return -DWC_E_INVALID;
	}

	if (val > core_if->hwcfg3.b.dfifo_depth) {
		if (dwc_otg_param_initialized
		    (core_if->core_params.data_fifo_size)) {
			DWC_ERROR
			    ("%d invalid for data_fifo_size parameter. Check HW configuration.\n",
			     val);
		}
		val = core_if->hwcfg3.b.dfifo_depth;
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.data_fifo_size = val;
	return retval;
}

int32_t dwc_otg_get_param_data_fifo_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.data_fifo_size;
}

int dwc_otg_set_param_dev_rx_fifo_size(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 16, 32768)) {
		DWC_WARN("Wrong value for dev_rx_fifo_size\n");
		DWC_WARN("dev_rx_fifo_size must be 16-32768\n");
		return -DWC_E_INVALID;
	}

	if (val > dwc_read_reg32(&core_if->core_global_regs->grxfsiz)) {
		if(dwc_otg_param_initialized(core_if->core_params.dev_rx_fifo_size)) {
		DWC_WARN("%d invalid for dev_rx_fifo_size parameter\n", val);
		}
		val = dwc_read_reg32(&core_if->core_global_regs->grxfsiz);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.dev_rx_fifo_size = val;
	return retval;
}

int32_t dwc_otg_get_param_dev_rx_fifo_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.dev_rx_fifo_size;
}

int dwc_otg_set_param_dev_nperio_tx_fifo_size(dwc_otg_core_if_t * core_if,
					      int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 16, 32768)) {
		DWC_WARN("Wrong value for dev_nperio_tx_fifo\n");
		DWC_WARN("dev_nperio_tx_fifo must be 16-32768\n");
		return -DWC_E_INVALID;
	}

	if (val > (dwc_read_reg32(&core_if->core_global_regs->gnptxfsiz) >> 16)) {
		if (dwc_otg_param_initialized
		    (core_if->core_params.dev_nperio_tx_fifo_size)) {
			DWC_ERROR
			    ("%d invalid for dev_nperio_tx_fifo_size. Check HW configuration.\n",
			     val);
		}
		val =
		    (dwc_read_reg32(&core_if->core_global_regs->gnptxfsiz) >>
		     16);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.dev_nperio_tx_fifo_size = val;
	return retval;
}

int32_t dwc_otg_get_param_dev_nperio_tx_fifo_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.dev_nperio_tx_fifo_size;
}

int dwc_otg_set_param_host_rx_fifo_size(dwc_otg_core_if_t * core_if,
					int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 16, 32768)) {
		DWC_WARN("Wrong value for host_rx_fifo_size\n");
		DWC_WARN("host_rx_fifo_size must be 16-32768\n");
		return -DWC_E_INVALID;
	}

	if (val > dwc_read_reg32(&core_if->core_global_regs->grxfsiz)) {
		if (dwc_otg_param_initialized
		    (core_if->core_params.host_rx_fifo_size)) {
			DWC_ERROR
			    ("%d invalid for host_rx_fifo_size. Check HW configuration.\n",
			     val);
		}
		val = dwc_read_reg32(&core_if->core_global_regs->grxfsiz);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.host_rx_fifo_size = val;
	return retval;

}

int32_t dwc_otg_get_param_host_rx_fifo_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.host_rx_fifo_size;
}

int dwc_otg_set_param_host_nperio_tx_fifo_size(dwc_otg_core_if_t * core_if,
					       int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 16, 32768)) {
		DWC_WARN("Wrong value for host_nperio_tx_fifo_size\n");
		DWC_WARN("host_nperio_tx_fifo_size must be 16-32768\n");
		return -DWC_E_INVALID;
	}

	if (val > (dwc_read_reg32(&core_if->core_global_regs->gnptxfsiz) >> 16)) {
		if (dwc_otg_param_initialized
		    (core_if->core_params.host_nperio_tx_fifo_size)) {
			DWC_ERROR
			    ("%d invalid for host_nperio_tx_fifo_size. Check HW configuration.\n",
			     val);
		}
		val =
		    (dwc_read_reg32(&core_if->core_global_regs->gnptxfsiz) >>
		     16);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.host_nperio_tx_fifo_size = val;
	return retval;
}

int32_t dwc_otg_get_param_host_nperio_tx_fifo_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.host_nperio_tx_fifo_size;
}

int dwc_otg_set_param_host_perio_tx_fifo_size(dwc_otg_core_if_t * core_if,
					      int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 16, 32768)) {
		DWC_WARN("Wrong value for host_perio_tx_fifo_size\n");
		DWC_WARN("host_perio_tx_fifo_size must be 16-32768\n");
		return -DWC_E_INVALID;
	}

	if (val >
	    ((dwc_read_reg32(&core_if->core_global_regs->hptxfsiz) >> 16))) {
		if (dwc_otg_param_initialized
		    (core_if->core_params.host_perio_tx_fifo_size)) {
			DWC_ERROR
			    ("%d invalid for host_perio_tx_fifo_size. Check HW configuration.\n",
			     val);
		}
		val =
		    (dwc_read_reg32(&core_if->core_global_regs->hptxfsiz) >>
		     16);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.host_perio_tx_fifo_size = val;
	return retval;
}

int32_t dwc_otg_get_param_host_perio_tx_fifo_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.host_perio_tx_fifo_size;
}

int dwc_otg_set_param_max_transfer_size(dwc_otg_core_if_t * core_if,
					int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 2047, 524288)) {
		DWC_WARN("Wrong value for max_transfer_size\n");
		DWC_WARN("max_transfer_size must be 2047-524288\n");
		return -DWC_E_INVALID;
	}

	if (val >= (1 << (core_if->hwcfg3.b.xfer_size_cntr_width + 11))) {
		if (dwc_otg_param_initialized
		    (core_if->core_params.max_transfer_size)) {
			DWC_ERROR
			    ("%d invalid for max_transfer_size. Check HW configuration.\n",
			     val);
		}
		val =
		    ((1 << (core_if->hwcfg3.b.packet_size_cntr_width + 11)) -
		     1);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.max_transfer_size = val;
	return retval;
}

int32_t dwc_otg_get_param_max_transfer_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.max_transfer_size;
}

int dwc_otg_set_param_max_packet_count(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 15, 511)) {
		DWC_WARN("Wrong value for max_packet_count\n");
		DWC_WARN("max_packet_count must be 15-511\n");
		return -DWC_E_INVALID;
	}

	if (val > (1 << (core_if->hwcfg3.b.packet_size_cntr_width + 4))) {
		if (dwc_otg_param_initialized
		    (core_if->core_params.max_packet_count)) {
			DWC_ERROR
			    ("%d invalid for max_packet_count. Check HW configuration.\n",
			     val);
		}
		val =
		    ((1 << (core_if->hwcfg3.b.packet_size_cntr_width + 4)) - 1);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.max_packet_count = val;
	return retval;
}

int32_t dwc_otg_get_param_max_packet_count(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.max_packet_count;
}

int dwc_otg_set_param_host_channels(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 1, 16)) {
		DWC_WARN("Wrong value for host_channels\n");
		DWC_WARN("host_channels must be 1-16\n");
		return -DWC_E_INVALID;
	}

	if (val > (core_if->hwcfg2.b.num_host_chan + 1)) {
		if (dwc_otg_param_initialized
		    (core_if->core_params.host_channels)) {
			DWC_ERROR
			    ("%d invalid for host_channels. Check HW configurations.\n",
			     val);
		}
		val = (core_if->hwcfg2.b.num_host_chan + 1);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.host_channels = val;
	return retval;
}

int32_t dwc_otg_get_param_host_channels(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.host_channels;
}

int dwc_otg_set_param_dev_endpoints(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 1, 15)) {
		DWC_WARN("Wrong value for dev_endpoints\n");
		DWC_WARN("dev_endpoints must be 1-15\n");
		return -DWC_E_INVALID;
	}

	if (val > (core_if->hwcfg2.b.num_dev_ep)) {
		if (dwc_otg_param_initialized
		    (core_if->core_params.dev_endpoints)) {
			DWC_ERROR
			    ("%d invalid for dev_endpoints. Check HW configurations.\n",
			     val);
		}
		val = core_if->hwcfg2.b.num_dev_ep;
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.dev_endpoints = val;
	return retval;
}

int32_t dwc_otg_get_param_dev_endpoints(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.dev_endpoints;
}

int dwc_otg_set_param_phy_type(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	int valid = 0;

	if (DWC_OTG_PARAM_TEST(val, 0, 2)) {
		DWC_WARN("Wrong value for phy_type\n");
		DWC_WARN("phy_type must be 0,1 or 2\n");
		return -DWC_E_INVALID;
	}
#if !defined NO_FS_PHY_HW_CHECKS
	if ((val == DWC_PHY_TYPE_PARAM_UTMI) &&
	    ((core_if->hwcfg2.b.hs_phy_type == 1) ||
	     (core_if->hwcfg2.b.hs_phy_type == 3))) {
		valid = 1;
	} else if ((val == DWC_PHY_TYPE_PARAM_ULPI) &&
		   ((core_if->hwcfg2.b.hs_phy_type == 2) ||
		    (core_if->hwcfg2.b.hs_phy_type == 3))) {
		valid = 1;
	} else if ((val == DWC_PHY_TYPE_PARAM_FS) &&
		   (core_if->hwcfg2.b.fs_phy_type == 1)) {
		valid = 1;
	}
	if (!valid) {
		if (dwc_otg_param_initialized(core_if->core_params.phy_type)) {
			DWC_ERROR
			    ("%d invalid for phy_type. Check HW configurations.\n",
			     val);
		}
		if (core_if->hwcfg2.b.hs_phy_type) {
			if ((core_if->hwcfg2.b.hs_phy_type == 3) ||
			    (core_if->hwcfg2.b.hs_phy_type == 1)) {
				val = DWC_PHY_TYPE_PARAM_UTMI;
			} else {
				val = DWC_PHY_TYPE_PARAM_ULPI;
			}
		}
		retval = -DWC_E_INVALID;
	}
#endif
	core_if->core_params.phy_type = val;
	return retval;
}

int32_t dwc_otg_get_param_phy_type(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.phy_type;
}

int dwc_otg_set_param_speed(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for speed parameter\n");
		DWC_WARN("max_speed parameter must be 0 or 1\n");
		return -DWC_E_INVALID;
	}
	if ((val == 0)
	    && dwc_otg_get_param_phy_type(core_if) == DWC_PHY_TYPE_PARAM_FS) {
		if (dwc_otg_param_initialized(core_if->core_params.speed)) {
			DWC_ERROR
			    ("%d invalid for speed paremter. Check HW configuration.\n",
			     val);
		}
		val =
		    (dwc_otg_get_param_phy_type(core_if) ==
		     DWC_PHY_TYPE_PARAM_FS ? 1 : 0);
		retval = -DWC_E_INVALID;
	}
	core_if->core_params.speed = val;
	return retval;
}

int32_t dwc_otg_get_param_speed(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.speed;
}

int dwc_otg_set_param_host_ls_low_power_phy_clk(dwc_otg_core_if_t * core_if,
						int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN
		    ("Wrong value for host_ls_low_power_phy_clk parameter\n");
		DWC_WARN("host_ls_low_power_phy_clk must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	if ((val == DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_48MHZ)
	    && (dwc_otg_get_param_phy_type(core_if) == DWC_PHY_TYPE_PARAM_FS)) {
		if(dwc_otg_param_initialized(core_if->core_params.host_ls_low_power_phy_clk)) {
			DWC_ERROR("%d invalid for host_ls_low_power_phy_clk. Check HW configuration.\n",
		     val);
		}
		val =
		    (dwc_otg_get_param_phy_type(core_if) ==
		     DWC_PHY_TYPE_PARAM_FS) ?
		    DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_6MHZ :
		    DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_48MHZ;
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.host_ls_low_power_phy_clk = val;
	return retval;
}

int32_t dwc_otg_get_param_host_ls_low_power_phy_clk(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.host_ls_low_power_phy_clk;
}

int dwc_otg_set_param_phy_ulpi_ddr(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for phy_ulpi_ddr\n");
		DWC_WARN("phy_upli_ddr must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	core_if->core_params.phy_ulpi_ddr = val;
	return 0;
}

int32_t dwc_otg_get_param_phy_ulpi_ddr(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.phy_ulpi_ddr;
}

int dwc_otg_set_param_phy_ulpi_ext_vbus(dwc_otg_core_if_t * core_if,
					int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong valaue for phy_ulpi_ext_vbus\n");
		DWC_WARN("phy_ulpi_ext_vbus must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	core_if->core_params.phy_ulpi_ext_vbus = val;
	return 0;
}

int32_t dwc_otg_get_param_phy_ulpi_ext_vbus(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.phy_ulpi_ext_vbus;
}

int dwc_otg_set_param_phy_utmi_width(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 8, 8) && DWC_OTG_PARAM_TEST(val, 16, 16)) {
		DWC_WARN("Wrong valaue for phy_utmi_width\n");
		DWC_WARN("phy_utmi_width must be 8 or 16\n");
		return -DWC_E_INVALID;
	}

	core_if->core_params.phy_utmi_width = val;
	return 0;
}

int32_t dwc_otg_get_param_phy_utmi_width(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.phy_utmi_width;
}

int dwc_otg_set_param_ulpi_fs_ls(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong valaue for ulpi_fs_ls\n");
		DWC_WARN("ulpi_fs_ls must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	core_if->core_params.ulpi_fs_ls = val;
	return 0;
}

int32_t dwc_otg_get_param_ulpi_fs_ls(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.ulpi_fs_ls;
}

int dwc_otg_set_param_ts_dline(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong valaue for ts_dline\n");
		DWC_WARN("ts_dline must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	core_if->core_params.ts_dline = val;
	return 0;
}

int32_t dwc_otg_get_param_ts_dline(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.ts_dline;
}

int dwc_otg_set_param_i2c_enable(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong valaue for i2c_enable\n");
		DWC_WARN("i2c_enable must be 0 or 1\n");
		return -DWC_E_INVALID;
	}
#if !defined NO_FS_PHY_HW_CHECK
	if (val == 1 && core_if->hwcfg3.b.i2c == 0) {
		if(dwc_otg_param_initialized(core_if->core_params.i2c_enable)) {
			DWC_ERROR("%d invalid for i2c_enable. Check HW configuration.\n",
		     val);
		}
		val = 0;
		retval = -DWC_E_INVALID;
	}
#endif

	core_if->core_params.i2c_enable = val;
	return retval;
}

int32_t dwc_otg_get_param_i2c_enable(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.i2c_enable;
}

int dwc_otg_set_param_dev_perio_tx_fifo_size(dwc_otg_core_if_t * core_if,
					     int32_t val, int fifo_num)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 4, 768)) {
		DWC_WARN("Wrong value for dev_perio_tx_fifo_size\n");
		DWC_WARN("dev_perio_tx_fifo_size must be 4-768\n");
		return -DWC_E_INVALID;
	}

	if (val > (dwc_read_reg32(&core_if->core_global_regs->dptxfsiz_dieptxf[fifo_num]))) {
		if(dwc_otg_param_initialized(core_if->core_params.dev_perio_tx_fifo_size[fifo_num])) {
			DWC_ERROR("`%d' invalid for parameter `dev_perio_fifo_size_%d'. Check HW configuration.\n",
		     val, fifo_num);
		}
		val = (dwc_read_reg32(&core_if->core_global_regs->dptxfsiz_dieptxf[fifo_num]));
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.dev_perio_tx_fifo_size[fifo_num] = val;
	return retval;
}

int32_t dwc_otg_get_param_dev_perio_tx_fifo_size(dwc_otg_core_if_t * core_if,
						 int fifo_num)
{
	return core_if->core_params.dev_perio_tx_fifo_size[fifo_num];
}

int dwc_otg_set_param_en_multiple_tx_fifo(dwc_otg_core_if_t * core_if,
					  int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong valaue for en_multiple_tx_fifo,\n");
		DWC_WARN("en_multiple_tx_fifo must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	if (val == 1 && core_if->hwcfg4.b.ded_fifo_en == 0) {
		if(dwc_otg_param_initialized(core_if->core_params.en_multiple_tx_fifo)) {
			DWC_ERROR("%d invalid for parameter en_multiple_tx_fifo. Check HW configuration.\n",
		     val);
		}
		val = 0;
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.en_multiple_tx_fifo = val;
	return retval;
}

int32_t dwc_otg_get_param_en_multiple_tx_fifo(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.en_multiple_tx_fifo;
}

int dwc_otg_set_param_dev_tx_fifo_size(dwc_otg_core_if_t * core_if, int32_t val,
				       int fifo_num)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 4, 768)) {
		DWC_WARN("Wrong value for dev_tx_fifo_size\n");
		DWC_WARN("dev_tx_fifo_size must be 4-768\n");
		return -DWC_E_INVALID;
	}

	if (val > (dwc_read_reg32(&core_if->core_global_regs->dptxfsiz_dieptxf[fifo_num]))) {
		if(dwc_otg_param_initialized(core_if->core_params.dev_tx_fifo_size[fifo_num])) {
			DWC_ERROR("`%d' invalid for parameter `dev_tx_fifo_size_%d'. Check HW configuration.\n",
		     val, fifo_num);
		}
		val = (dwc_read_reg32(&core_if->core_global_regs->dptxfsiz_dieptxf[fifo_num]));
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.dev_tx_fifo_size[fifo_num] = val;
	return retval;
}

int32_t dwc_otg_get_param_dev_tx_fifo_size(dwc_otg_core_if_t * core_if,
					   int fifo_num)
{
	return core_if->core_params.dev_tx_fifo_size[fifo_num];
}

int dwc_otg_set_param_thr_ctl(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 0, 7)) {
		DWC_WARN("Wrong value for thr_ctl\n");
		DWC_WARN("thr_ctl must be 0-7\n");
		return -DWC_E_INVALID;
	}

	if ((val != 0) &&
	    (!dwc_otg_get_param_dma_enable(core_if) ||
	     !core_if->hwcfg4.b.ded_fifo_en)) {
		if(dwc_otg_param_initialized(core_if->core_params.thr_ctl)) {
			DWC_ERROR("%d invalid for parameter thr_ctl. Check HW configuration.\n",
		     val);
		}
		val = 0;
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.thr_ctl = val;
	return retval;
}

int32_t dwc_otg_get_param_thr_ctl(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.thr_ctl;
}

int dwc_otg_set_param_lpm_enable(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for lpm_enable\n");
		DWC_WARN("lpm_enable must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	if (val && !core_if->hwcfg3.b.otg_lpm_en) {
		if(dwc_otg_param_initialized(core_if->core_params.lpm_enable)) {
			DWC_ERROR("%d invalid for parameter lpm_enable. Check HW configuration.\n",
		     val);
		}
		val = 0;
		retval = -DWC_E_INVALID;
	}

	core_if->core_params.lpm_enable = val;
	return retval;
}

int32_t dwc_otg_get_param_lpm_enable(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.lpm_enable;
}

int dwc_otg_set_param_tx_thr_length(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 8, 128)) {
		DWC_WARN("Wrong valaue for tx_thr_length\n");
		DWC_WARN("tx_thr_length must be 8 - 128\n");
		return -DWC_E_INVALID;
	}

	core_if->core_params.tx_thr_length = val;
	return 0;
}

int32_t dwc_otg_get_param_tx_thr_length(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.tx_thr_length;
}

int dwc_otg_set_param_rx_thr_length(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 8, 128)) {
		DWC_WARN("Wrong valaue for rx_thr_length\n");
		DWC_WARN("rx_thr_length must be 8 - 128\n");
		return -DWC_E_INVALID;
	}

	core_if->core_params.rx_thr_length = val;
	return 0;
}

int32_t dwc_otg_get_param_rx_thr_length(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.rx_thr_length;
}

int dwc_otg_set_param_dma_burst_size(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 1, 1) &&
	    DWC_OTG_PARAM_TEST(val, 4, 4) &&
	    DWC_OTG_PARAM_TEST(val, 8, 8) &&
	    DWC_OTG_PARAM_TEST(val, 16, 16) &&
	    DWC_OTG_PARAM_TEST(val, 32, 32) &&
	    DWC_OTG_PARAM_TEST(val, 64, 64) &&
	    DWC_OTG_PARAM_TEST(val, 128, 128) &&
	    DWC_OTG_PARAM_TEST(val, 256, 256)) {
		DWC_WARN("`%d' invalid for parameter `dma_burst_size'\n", val);
		return -DWC_E_INVALID;
	}
	core_if->core_params.dma_burst_size = val;
	return 0;
}

int32_t dwc_otg_get_param_dma_burst_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.dma_burst_size;
}

int dwc_otg_set_param_pti_enable(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("`%d' invalid for parameter `pti_enable'\n", val);
		return -DWC_E_INVALID;
	}
	if (val && (core_if->snpsid < OTG_CORE_REV_2_72a)) {
		if (dwc_otg_param_initialized(core_if->core_params.pti_enable)) {
			DWC_ERROR("%d invalid for parameter pti_enable. Check HW configuration.\n",
			     val);
		}
		retval = -DWC_E_INVALID;
		val = 0;
	}
	core_if->core_params.pti_enable = val;
	return retval;
}

int32_t dwc_otg_get_param_pti_enable(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.pti_enable;
}

int dwc_otg_set_param_mpi_enable(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("`%d' invalid for parameter `mpi_enable'\n", val);
		return -DWC_E_INVALID;
	}
	if (val && (core_if->hwcfg2.b.multi_proc_int == 0)) {
		if (dwc_otg_param_initialized(core_if->core_params.mpi_enable)) {
			DWC_ERROR("%d invalid for parameter mpi_enable. Check HW configuration.\n",
			     val);
		}
		retval = -DWC_E_INVALID;
		val = 0;
	}
	core_if->core_params.mpi_enable = val;
	return retval;
}

int32_t dwc_otg_get_param_mpi_enable(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.mpi_enable;
}

int dwc_otg_set_param_ic_usb_cap(dwc_otg_core_if_t * core_if,
					int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("`%d' invalid for parameter `ic_usb_cap'\n", val);
		DWC_WARN("ic_usb_cap must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	if (val && (core_if->hwcfg3.b.otg_enable_ic_usb == 0)) {
		if (dwc_otg_param_initialized(core_if->core_params.ic_usb_cap)) {
			DWC_ERROR("%d invalid for parameter ic_usb_cap. Check HW configuration.\n",
			     val);
		}
		retval = -DWC_E_INVALID;
		val = 0;
	}
	core_if->core_params.ic_usb_cap = val;
	return retval;
}
int32_t dwc_otg_get_param_ic_usb_cap(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.ic_usb_cap;
}

int dwc_otg_set_param_ahb_thr_ratio(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	int valid = 1;

	if(DWC_OTG_PARAM_TEST(val, 0, 3)) {
		DWC_WARN("`%d' invalid for parameter `ahb_thr_ratio'\n", val);
		DWC_WARN("ahb_thr_ratio must be 0 - 3\n");
		return -DWC_E_INVALID;
	}

	if(val && (core_if->snpsid < OTG_CORE_REV_2_81a || !dwc_otg_get_param_thr_ctl(core_if))) {
		valid = 0;
	} else if(val && ((dwc_otg_get_param_tx_thr_length(core_if) / (1 << val)) < 4)) {
		valid = 0;
	}
	if(valid == 0) {
		if(dwc_otg_param_initialized(core_if->core_params.ahb_thr_ratio)) {
			DWC_ERROR("%d invalid for parameter ahb_thr_ratio. Chack HW configuration.\n", val);
		}
		retval = -DWC_E_INVALID;
		val = 0;
	}

	core_if->core_params.ahb_thr_ratio = val;
	return retval;
}
int32_t dwc_otg_get_param_ahb_thr_ratio(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params.ahb_thr_ratio;
}


uint32_t dwc_otg_get_hnpstatus(dwc_otg_core_if_t * core_if)
{
	gotgctl_data_t otgctl;
	otgctl.d32 = dwc_read_reg32(&core_if->core_global_regs->gotgctl);
	return otgctl.b.hstnegscs;
}

uint32_t dwc_otg_get_srpstatus(dwc_otg_core_if_t * core_if)
{
	gotgctl_data_t otgctl;
	otgctl.d32 = dwc_read_reg32(&core_if->core_global_regs->gotgctl);
	return otgctl.b.sesreqscs;
}

IMG_VOID dwc_otg_set_hnpreq(dwc_otg_core_if_t * core_if, uint32_t val)
{
	gotgctl_data_t otgctl;
	otgctl.d32 = dwc_read_reg32(&core_if->core_global_regs->gotgctl);
	otgctl.b.hnpreq = val;
	dwc_write_reg32(&core_if->core_global_regs->gotgctl, otgctl.d32);
}

uint32_t dwc_otg_get_gsnpsid(dwc_otg_core_if_t * core_if)
{
	return core_if->snpsid;
}

uint32_t dwc_otg_get_mode(dwc_otg_core_if_t * core_if)
{
	gotgctl_data_t otgctl;
	otgctl.d32 = dwc_read_reg32(&core_if->core_global_regs->gotgctl);
	return otgctl.b.currmod;
}

uint32_t dwc_otg_get_hnpcapable(dwc_otg_core_if_t * core_if)
{
	gusbcfg_data_t usbcfg;
	usbcfg.d32 = dwc_read_reg32(&core_if->core_global_regs->gusbcfg);
	return usbcfg.b.hnpcap;
}

IMG_VOID dwc_otg_set_hnpcapable(dwc_otg_core_if_t * core_if, uint32_t val)
{
	gusbcfg_data_t usbcfg;
	usbcfg.d32 = dwc_read_reg32(&core_if->core_global_regs->gusbcfg);
	usbcfg.b.hnpcap = val;
	dwc_write_reg32(&core_if->core_global_regs->gusbcfg, usbcfg.d32);
}

uint32_t dwc_otg_get_srpcapable(dwc_otg_core_if_t * core_if)
{
	gusbcfg_data_t usbcfg;
	usbcfg.d32 = dwc_read_reg32(&core_if->core_global_regs->gusbcfg);
	return usbcfg.b.srpcap;
}

IMG_VOID dwc_otg_set_srpcapable(dwc_otg_core_if_t * core_if, uint32_t val)
{
	gusbcfg_data_t usbcfg;
	usbcfg.d32 = dwc_read_reg32(&core_if->core_global_regs->gusbcfg);
	usbcfg.b.srpcap = val;
	dwc_write_reg32(&core_if->core_global_regs->gusbcfg, usbcfg.d32);
}

uint32_t dwc_otg_get_devspeed(dwc_otg_core_if_t * core_if)
{
	dcfg_data_t dcfg;
	dcfg.d32 = dwc_read_reg32(&core_if->dev_if->dev_global_regs->dcfg);
	return dcfg.b.devspd;
}

IMG_VOID dwc_otg_set_devspeed(dwc_otg_core_if_t * core_if, uint32_t val)
{
	dcfg_data_t dcfg;
	dcfg.d32 = dwc_read_reg32(&core_if->dev_if->dev_global_regs->dcfg);
	dcfg.b.devspd = val;
	dwc_write_reg32(&core_if->dev_if->dev_global_regs->dcfg, dcfg.d32);
}

uint32_t dwc_otg_get_busconnected(dwc_otg_core_if_t * core_if)
{
	hprt0_data_t hprt0;
	hprt0.d32 = dwc_read_reg32(core_if->host_if->hprt0);
	return hprt0.b.prtconnsts;
}

uint32_t dwc_otg_get_enumspeed(dwc_otg_core_if_t * core_if)
{
	dsts_data_t dsts;
	dsts.d32 = dwc_read_reg32(&core_if->dev_if->dev_global_regs->dsts);
	return dsts.b.enumspd;
}

uint32_t dwc_otg_get_prtpower(dwc_otg_core_if_t * core_if)
{
	hprt0_data_t hprt0;
	hprt0.d32 = dwc_read_reg32(core_if->host_if->hprt0);
	return hprt0.b.prtpwr;

}

IMG_VOID dwc_otg_set_prtpower(dwc_otg_core_if_t * core_if, uint32_t val)
{
	hprt0_data_t hprt0;
	hprt0.d32 = dwc_read_reg32(core_if->host_if->hprt0);
	hprt0.b.prtpwr = val;
	dwc_write_reg32(core_if->host_if->hprt0, val);
}

uint32_t dwc_otg_get_prtsuspend(dwc_otg_core_if_t * core_if)
{
	hprt0_data_t hprt0;
	hprt0.d32 = dwc_read_reg32(core_if->host_if->hprt0);
	return hprt0.b.prtsusp;

}

IMG_VOID dwc_otg_set_prtsuspend(dwc_otg_core_if_t * core_if, uint32_t val)
{
	hprt0_data_t hprt0;
	hprt0.d32 = dwc_read_reg32(core_if->host_if->hprt0);
	hprt0.b.prtsusp = val;
	dwc_write_reg32(core_if->host_if->hprt0, val);
}

IMG_VOID dwc_otg_set_prtresume(dwc_otg_core_if_t * core_if, uint32_t val)
{
	hprt0_data_t hprt0;
	hprt0.d32 = dwc_read_reg32(core_if->host_if->hprt0);
	hprt0.b.prtres = val;
	dwc_write_reg32(core_if->host_if->hprt0, val);
}

uint32_t dwc_otg_get_remotewakesig(dwc_otg_core_if_t * core_if)
{
	dctl_data_t dctl;
	dctl.d32 = dwc_read_reg32(&core_if->dev_if->dev_global_regs->dctl);
	return dctl.b.rmtwkupsig;
}

uint32_t dwc_otg_get_lpm_portsleepstatus(dwc_otg_core_if_t * core_if)
{
	glpmcfg_data_t lpmcfg;
	lpmcfg.d32 = dwc_read_reg32(&core_if->core_global_regs->glpmcfg);

	DWC_ASSERT(!
		   ((core_if->lx_state == DWC_OTG_L1) ^ lpmcfg.b.prt_sleep_sts),
		   "lx_state = %d, lmpcfg.prt_sleep_sts = %d\n",
		   core_if->lx_state, lpmcfg.b.prt_sleep_sts);

	return lpmcfg.b.prt_sleep_sts;
}

uint32_t dwc_otg_get_lpm_remotewakeenabled(dwc_otg_core_if_t * core_if)
{
	glpmcfg_data_t lpmcfg;
	lpmcfg.d32 = dwc_read_reg32(&core_if->core_global_regs->glpmcfg);
	return lpmcfg.b.rem_wkup_en;
}

uint32_t dwc_otg_get_lpmresponse(dwc_otg_core_if_t * core_if)
{
	glpmcfg_data_t lpmcfg;
	lpmcfg.d32 = dwc_read_reg32(&core_if->core_global_regs->glpmcfg);
	return lpmcfg.b.appl_resp;
}

IMG_VOID dwc_otg_set_lpmresponse(dwc_otg_core_if_t * core_if, uint32_t val)
{
	glpmcfg_data_t lpmcfg;
	lpmcfg.d32 = dwc_read_reg32(&core_if->core_global_regs->glpmcfg);
	lpmcfg.b.appl_resp = val;
	dwc_write_reg32(&core_if->core_global_regs->glpmcfg, lpmcfg.d32);
}

uint32_t dwc_otg_get_hsic_connect(dwc_otg_core_if_t * core_if)
{
	glpmcfg_data_t lpmcfg;
	lpmcfg.d32 = dwc_read_reg32(&core_if->core_global_regs->glpmcfg);
	return lpmcfg.b.hsic_connect;
}

IMG_VOID dwc_otg_set_hsic_connect(dwc_otg_core_if_t * core_if, uint32_t val)
{
	glpmcfg_data_t lpmcfg;
	lpmcfg.d32 = dwc_read_reg32(&core_if->core_global_regs->glpmcfg);
	lpmcfg.b.hsic_connect = val;
	dwc_write_reg32(&core_if->core_global_regs->glpmcfg, lpmcfg.d32);
}

uint32_t dwc_otg_get_inv_sel_hsic(dwc_otg_core_if_t * core_if)
{
	glpmcfg_data_t lpmcfg;
	lpmcfg.d32 = dwc_read_reg32(&core_if->core_global_regs->glpmcfg);
	return lpmcfg.b.inv_sel_hsic;

}

IMG_VOID dwc_otg_set_inv_sel_hsic(dwc_otg_core_if_t * core_if, uint32_t val)
{
	glpmcfg_data_t lpmcfg;
	lpmcfg.d32 = dwc_read_reg32(&core_if->core_global_regs->glpmcfg);
	lpmcfg.b.inv_sel_hsic = val;
	dwc_write_reg32(&core_if->core_global_regs->glpmcfg, lpmcfg.d32);
}

uint32_t dwc_otg_get_gotgctl(dwc_otg_core_if_t * core_if)
{
	return dwc_read_reg32(&core_if->core_global_regs->gotgctl);
}

IMG_VOID dwc_otg_set_gotgctl(dwc_otg_core_if_t * core_if, uint32_t val)
{
	dwc_write_reg32(&core_if->core_global_regs->gotgctl, val);
}

uint32_t dwc_otg_get_gusbcfg(dwc_otg_core_if_t * core_if)
{
	return dwc_read_reg32(&core_if->core_global_regs->gusbcfg);
}

IMG_VOID dwc_otg_set_gusbcfg(dwc_otg_core_if_t * core_if, uint32_t val)
{
	dwc_write_reg32(&core_if->core_global_regs->gusbcfg, val);
}

uint32_t dwc_otg_get_grxfsiz(dwc_otg_core_if_t * core_if)
{
	return dwc_read_reg32(&core_if->core_global_regs->grxfsiz);
}

IMG_VOID dwc_otg_set_grxfsiz(dwc_otg_core_if_t * core_if, uint32_t val)
{
	dwc_write_reg32(&core_if->core_global_regs->grxfsiz, val);
}

uint32_t dwc_otg_get_gnptxfsiz(dwc_otg_core_if_t * core_if)
{
	return dwc_read_reg32(&core_if->core_global_regs->gnptxfsiz);
}

IMG_VOID dwc_otg_set_gnptxfsiz(dwc_otg_core_if_t * core_if, uint32_t val)
{
	dwc_write_reg32(&core_if->core_global_regs->gnptxfsiz, val);
}

uint32_t dwc_otg_get_gpvndctl(dwc_otg_core_if_t * core_if)
{
	return dwc_read_reg32(&core_if->core_global_regs->gpvndctl);
}

IMG_VOID dwc_otg_set_gpvndctl(dwc_otg_core_if_t * core_if, uint32_t val)
{
	dwc_write_reg32(&core_if->core_global_regs->gpvndctl, val);
}

uint32_t dwc_otg_get_ggpio(dwc_otg_core_if_t * core_if)
{
	return dwc_read_reg32(&core_if->core_global_regs->ggpio);
}

IMG_VOID dwc_otg_set_ggpio(dwc_otg_core_if_t * core_if, uint32_t val)
{
	dwc_write_reg32(&core_if->core_global_regs->ggpio, val);
}

uint32_t dwc_otg_get_hprt0(dwc_otg_core_if_t * core_if)
{
	return dwc_read_reg32(core_if->host_if->hprt0);

}

IMG_VOID dwc_otg_set_hprt0(dwc_otg_core_if_t * core_if, uint32_t val)
{
	dwc_write_reg32(core_if->host_if->hprt0, val);
}

uint32_t dwc_otg_get_guid(dwc_otg_core_if_t * core_if)
{
	return dwc_read_reg32(&core_if->core_global_regs->guid);
}

IMG_VOID dwc_otg_set_guid(dwc_otg_core_if_t * core_if, uint32_t val)
{
	dwc_write_reg32(&core_if->core_global_regs->guid, val);
}

uint32_t dwc_otg_get_hptxfsiz(dwc_otg_core_if_t * core_if)
{
	return dwc_read_reg32(&core_if->core_global_regs->hptxfsiz);
}

#endif
