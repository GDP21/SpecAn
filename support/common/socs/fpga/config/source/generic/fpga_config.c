/*
** FILE NAME:   $RCSfile: fpga_config.c,v $
**
** TITLE:       FPGA SOC config
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Interface for FPGA SOC configuration driver
**
** NOTICE:		Copyright (C) 2009, Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

#ifdef METAG
/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include "fpga_config.h"

/* Avnet KuroI board support */
#include "mdtv_proto1_drv.h"
#include "ak_bsp_drv.h"

#ifdef kuroi3
/* On Kuroi3 systems the OSC selection is performed with the MDTV proto driver */
#define MDTV_PROTO_CLOCK_SELECTION
#endif

#ifdef TDMB
/* T-DMB set up */

#ifdef MDTV_PROTO_CLOCK_SELECTION

/* Kuroi3 clock selection. Define for clock selection on this platform. */
#define MDTV_PROTO1_OSC_CLK_SEL		(SEL_OSC2_CLK)	/* 32.768MHz */
#define MDTV_PROTO1_OSC_CLK_DIVIDER	(8)	/* 4.096MHz from 32.768MHz so divide by 8 */

#else

/* Kuroi1/2 clock selection. Define for clock selection on these platforms. */
#define AK_SYNTH_CLK_SOURCE_PLAYOUT_VALUE        (AK_BSP_SRV_SCS_OSC3) /* SYNTH_CLK0_SEL 10: OSC3 - clk for playout */
#define AK_SYNTH_CLK_SOURCE_RF_VALUE	         (AK_BSP_SRV_SCS_OSC3) /* SYNTH_CLK0_SEL 10: OSC3 - clk for ADC */

#endif	// MDTV_PROTO_CLOCK_SELECTION

/* ADC FIFO configuration */
static ADC_FIFO_CONFIG_T adcFifoConfig =
{
    COMPLEX_SIGNAL,
    TWOS_COMPLEMENT_FORMAT,
    1,
    1
};
#endif	/* TDMB */


/* Macro for writing to HW registers */
#define WRITE(A,V)  (*((volatile unsigned int *)(A))=((unsigned int)V))

void FPGAConfig(void)
{
    /* Avnet KuroI specific setup */
    MDTV_PROTO1_DRV_setPlayoutModeAdc();
    MDTV_PROTO1_DRV_setAdcFifoConfig(&adcFifoConfig);

#ifdef MDTV_PROTO1_OSC_CLK_SEL
		/* Note MDTV_PROTO1_OSC_CLK_DIVIDER and MDTV_PROTO1_OSC_CLK_SEL must be defined for this to work.
		** If OSC clock selection not needed here it won't matter as then the bit in the register doesn't exist. */
	MDTV_PROTO1_DRV_setSynthClockDivider(MDTV_PROTO1_OSC_CLK_DIVIDER,MDTV_PROTO1_OSC_CLK_SEL);
#endif


#if defined(PLAYOUT)

    MDTV_PROTO1_DRV_setRfEnable(0);
#ifdef AK_SYNTH_CLK_SOURCE_PLAYOUT_VALUE
    AK_BSP_DRV_setSynthClkSrc(AK_SYNTH_CLK_SOURCE_PLAYOUT_VALUE);
#endif

#elif defined(RF)

    MDTV_PROTO1_DRV_setRfEnable(1);
#ifdef AK_SYNTH_CLK_SOURCE_RF_VALUE
    AK_BSP_DRV_setSynthClkSrc(AK_SYNTH_CLK_SOURCE_RF_VALUE);
#endif

#else
    #error "PLAYOUT or RF must be defined"
#endif

	return;
}



/* Definitions for TSO to USB
 * To enable the TSO to USB set the gate mirror (0x04803F0C) to 0x0002000
 * and
 * then enable USB input select (0x04803F08) with 0x0000001
 * */
#define VIRTEX_IV_ECP_TSO_CONTROL_0_MIRRORED        (0x4803F0C)
#define VIRTEX_IV_ECP_TSO_CONTROL_0_MIRRORED_VALUE(c)  ((((c)->dataOrder)<<14) | (((c)->clockGate) << 13) | (((c)->clockInvert)<<12) | (  ((c)->validInvert) << 11) | (((c)->errorInvert) << 10) | (((c)->streamSelect) << 9))
#define VIRTEX_IV_USB_INPUT_SELECTION               (0x4803F08)
#define VIRTEX_IV_USB_INPUT_SELECTION_VALUE         (0x0000001)

void FPGA_TSOConfig(const TSO_FPGA_CONFIG_T *config)
{

    /* Definitions for TSO to USB
    * To enable the TSO to USB set the gate mirror
    * and
    * then enable USB input select
    * */
    WRITE(VIRTEX_IV_ECP_TSO_CONTROL_0_MIRRORED,VIRTEX_IV_ECP_TSO_CONTROL_0_MIRRORED_VALUE(config));
    WRITE(VIRTEX_IV_USB_INPUT_SELECTION,VIRTEX_IV_USB_INPUT_SELECTION_VALUE);

	return;
}



