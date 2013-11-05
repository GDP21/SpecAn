/*
** FILE NAME:   $RCSfile: agc_test.c,v $
**
** TITLE:       Common AGC test application
**
** PROJECT:		UCCP Common
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Test application for for common AGC functions.
**
**				Copyright (C) Imagination Technologies Ltd.
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include "common_agc.h"
#include "uccrt.h"

#define AGC_TGT_CLIPRATE_PPM	3000

static AGC_CONTEXT_T agcContext;
static TDEV_AGCISR_HELPER_T SCPInfo;
static TDEV_SCP_CONTROL_T scpControl;
SCP_T *scp = NULL;

/* FIR Filter coefficients */
static TDEV_SCP_CONFIG_T SCPConfig =
{
	/*! Format of ADC samples */
	SCP_SAMPLE_2SCOMP,
    /*! ADC (or input signal) sample rate in Hz*/
	4096000,
    /*! CIC filter decimation rate */
    1,
    /*! FIR filter decimation rate */
    2,
    /*! 1/Resampler rate * 2^31 */
    0x80000000,
    /*! 24 tap symmetrical FIR filter coefficients (12 coefficients)
    **  Narrow band FIR coefficients, normally used in acquisition   */
    {-4, -12, -10, 9, 33, 28, -25, -88, -71, 84, 326, 511},
    /*! 24 tap symmetrical FIR filter coefficients (12 coefficients)
    **  Wide band FIR coefficients, normally used in normal operation */
    {2, 6, 3, -12, -17, 15, 44, -5, -94, -44, 225, 504},
    /*! Rapid AGC update period in IF sample */
    8192,
    /*! Normal operation AGC update period in IF sample */
    8192
};



static volatile int stopTest = 0;

/******************************************************************************
 AGC Interrupt Handler
******************************************************************************/
static void AGCHandler(SCP_T *scp, SCP_EVT_T event, void *parameter)
{
	(void)event;

	/* @ an AGC interrupt */
	if (AGC_ReadSCPRegs(scp, parameter, &SCPInfo))
	{
		/* Use all the common loop calculation functions */
		AGC_CalcLoops(parameter, &SCPInfo);

		/* Call Tuner API registered setAGC function with &SCPInfo */
		/* ... */

		/* Update SCP after AGC operation */
		AGC_WriteSCPRegs(scp, parameter, &SCPInfo);
	}

	return;
}

int main(void)
{
	long complex = 0;
	/* Init TBI... */
    TBIRES tbiResVal;                   /* Variable for TBI manipulations. */

    tbiResVal.Sig.pCtx = 0;
    tbiResVal.Sig.SaveMask = 0;
    tbiResVal.Sig.TrigMask = 0;

    /* Disable all interrupts and obtain TXMASK */
    TBI_INTSX(tbiResVal.Sig.TrigMask);

    /* Enable interrupts and initialise the interrupt subsystem */
    __TBIASyncTrigger(tbiResVal);


	/* Init. UCC and SCP */
	UCCP_init();

	SCP_installEventHandler(scp, AGCHandler, SCP_EVT_AGCCOUNT, &agcContext, true);

#ifdef COMPLEX_INPUT
	complex = 1;
#endif

	SCP_setFIRDec(scp, SCPConfig.FIRFactor);
	SCP_setCICDec(scp, SCPConfig.CICFactor);

	/* Init. AGC */
	SCPInfo.pSCPcontrol = &scpControl;
	AGC_Init(scp, &agcContext, &SCPConfig, TDEV_NORMAL_AGC, complex /* COMPLEX */);

	/* Spin waiting to be told to stop the test */
	while (!stopTest)
	{
	}

	/* @ system shutdown. */
	AGC_DeInit(scp, &agcContext);

	return(0);
}



