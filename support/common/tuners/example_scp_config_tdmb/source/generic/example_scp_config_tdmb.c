/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuners/example_scp_config_tdmb/source/generic/example_scp_config_tdmb.c,v $
**
** TITLE:       Example SCP Configuration T-DMB
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Implementation of example SCP configuration
**
** NOTICE:		Copyright (C) 2008, Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
**				This example SCP config is intended to be used as a template, a starting point for creating
**				tuner drivers. The tuner API software integration guide should be consulted for more information
**				on the requirements and capabilities of the tuner API.
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

#include "PHY_scpConfig.h"
#include "example_scp_config_tdmb.h"

/* FIR Filter coefficients */
static short narrowcoeffs[] = {-4, -12, -10, 9, 33, 28, -25, -88, -71, 84, 326, 511};
static short widecoeffs[] = {2, 6, 3, -12, -17, 15, 44, -5, -94, -44, 225, 504};


PHY_SCP_CONFIG_T exampleSCPConfig_TDMB =
{
	/*! Format of ADC samples */
	PHY_TUNER_ADC_2S_COMP,
    /* ADC (or input signal) sample rate in Hz*/
	4096000,
    /* CIC filter decimation rate */
    1,
    /* FIR filter decimation rate */
    2,
    /* 1/Resampler rate * 2^31 */
    0x80000000,
    /* 24 tap symmetrical FIR filter coefficients (12 coefficients)
    ** Narrow band FIR coefficients, normally used in acquisition   */
    narrowcoeffs,
    /* 24 tap symmetrical FIR filter coefficients (12 coefficients)
    ** Wide band FIR coefficients, normally used in normal operation */
    widecoeffs,
    /* Rapid AGC update period in ADC samples */
    8192,
    /* Normal operation AGC update period in ADC samples */
    8192
};
