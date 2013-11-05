/*
** FILE NAME:   $RCSfile: common_agc.c,v $
**
** TITLE:       Common AGC and related control loop handling
**
** PROJECT:		UCCP Common
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: C file for common functions AGC.
**
**				Copyright (C) Imagination Technologies Ltd.
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include <assert.h>

#include "common_agc.h"
#include "uccrt.h"
#include "maths_funcs.h"

#include "agc_constants.h"

/* If we have a target clip count in PPM more than this the fixed point math might overflow */
#define MAX_CLIP_COUNT_PPM	8191
#define	ONE_MILLION 1000000UL

/* Initialises the AGC context and AGC parts of SCP */
void AGC_Init(SCP_T *scp, AGC_CONTEXT_T *context, TDEV_SCP_CONFIG_T *SCPConfig, TDEV_AGC_MODE_T agcMode, long complexIF)
{
    long agcPeriod = (agcMode == TDEV_RAPID_AGC) ? SCPConfig->rapidAGCPeriod : SCPConfig->normalAGCPeriod;

	unsigned n = sizeof(AGC_INPUT_FILT_U);
	char *pFilt = (char *)&context->DCOffset_inputFilt;

	/* Store parameters in the context */
	context->SCPConfig = SCPConfig;
	context->complexIF = complexIF;

	/* Init control loops to default values */
	context->gain_error_accum = 0;
	context->IQcorrectionAccum = 0;

	/* clear DC loop filter context memory */
	while (n--)
	{
	    pFilt[n] = 0;
	}

	context->DcOffsetI_Accum = 0;
	context->DcOffsetQ_Accum = 0;
	context->gain_accum = INITIAL_GAIN_ACCUM;
	context->agctarget_accum = INITIAL_TARGET_ACCUM;
    context->agcIQErrScale = AGC_IQ_SCALE_SHIFT;

	/* Setup for the AGC rate we are initialised to. Default configuration. */
	AGC_Configure(context, agcMode, NULL);

	/* Set up AGC part of SCP */
	SCP_setAGC(scp, agcPeriod, AGC_CLIP_LEVEL, AGC_THRESHOLD_VALUE);

	if (complexIF)
		SCP_setIQCorrelator(scp, true);

	return;
}

/* Set parameters which depends on the AGC mode */
void AGC_Configure(AGC_CONTEXT_T *context, TDEV_AGC_MODE_T agcMode, AGC_CONFIG_T *pConfig)
{
    long agcPeriod = (agcMode == TDEV_RAPID_AGC) ? context->SCPConfig->rapidAGCPeriod : context->SCPConfig->normalAGCPeriod;
	unsigned long targetClipCount;

	targetClipCount = ((unsigned long)AGC_TGT_CLIPRATE_PPM) * agcPeriod;

	/* to convert ppm of AGC period divide by 10^6, but in two parts, so as to round up. */
	targetClipCount /= (ONE_MILLION / 2);
	targetClipCount++;
	targetClipCount /= 2;

	context->agcMode = agcMode;
	context->targetClipCount = (long)targetClipCount;
	//context->AGCSamplesShift = MATHS_log2Rnd(agcPeriod);
	//context->AGCPeriod = agcPeriod;

	if (pConfig == NULL)
	{
	    /* Default values */
	    if (agcMode == TDEV_RAPID_AGC)
	    {
	        context->agcErrScaleUpShift = AGC_SCALE_SHIFT_RAPID_UP;
	        context->agcErrScaleDnShift = AGC_SCALE_SHIFT_RAPID_DN;
	    }
	    else
	    {
	        context->agcErrScaleUpShift = AGC_SCALE_SHIFT_NORMAL_UP;
	        context->agcErrScaleDnShift = AGC_SCALE_SHIFT_NORMAL_DN;
	    }

	    context->agcErrScaleUpMul     = AGC_SCALE_MUL;
	    context->agcErrScaleDnMul     = AGC_SCALE_MUL;

	    /* Default to using the legacy integrator. */
	    context->DCOffset_filtMode    = AGC_FILT_INTEGRATOR;
	    context->DCOffset_inputFilt.integrator.numIntegrationsLog2 = SCP_DC_OFFSET_PRE_INTEGRATIONS_LOG2;
	    context->DCOffset_inputFilt.integrator.numIntegrations     = SCP_DC_OFFSET_PRE_INTEGRATIONS;

	    context->scpDcOffsetDeadBand  = SCP_DC_OFFSET_DEFAULT_DEADBAND;
	    context->scpDcOffsetGainShift = SCP_DC_OFFSET_DEFAULT_GAIN_SHIFT;
	    context->scpDcOffsetGainMult  = SCP_DC_OFFSET_DEFAULT_GAIN_MULT;
	}
	else
	{
	    /* overwrite default values with configuration ones */
		context->agcErrScaleUpShift   = pConfig->agcErrScaleUpShift;
		context->agcErrScaleDnShift   = pConfig->agcErrScaleDnShift;
        context->agcErrScaleUpMul     = pConfig->agcErrScaleUpMul;
        context->agcErrScaleDnMul     = pConfig->agcErrScaleDnMul;

        context->scpDcOffsetDeadBand  = pConfig->scpDcOffsetDeadBand;
        context->scpDcOffsetGainShift = pConfig->scpDcOffsetGainShift;
        context->scpDcOffsetGainMult  = pConfig->scpDcOffsetGainMult;

        context->DCOffset_filtMode    = pConfig->scpDcOffsetFiltMode;

        if (context->DCOffset_filtMode == AGC_FILT_SINGLE_POLE)
        {
            context->DCOffset_inputFilt.singlePole.init = pConfig->scpDcOffsetSinglePoleInit;
            context->DCOffset_inputFilt.singlePole.a = pConfig->scpDcOffsetSinglePoleLoopCoeff_a;
            context->DCOffset_inputFilt.singlePole.b = pConfig->scpDcOffsetSinglePoleLoopCoeff_b;
        }
        else    /* default to integrator */
        {
            context->DCOffset_inputFilt.integrator.numIntegrationsLog2 = pConfig->scpDcOffsetPreIntegrationsLog2;
            context->DCOffset_inputFilt.integrator.numIntegrations     = (long)(1<<pConfig->scpDcOffsetPreIntegrationsLog2);
        }
	}

	return;
}

/* Fill the config structure with the current configuration.
 * This function can be used with AGC_Configure for read/modify/write operations. */
void AGC_getConfig(AGC_CONTEXT_T *context, AGC_CONFIG_T *pConfig)
{
    pConfig->agcErrScaleUpShift   = context->agcErrScaleUpShift;
    pConfig->agcErrScaleDnShift   = context->agcErrScaleDnShift;
    pConfig->agcErrScaleUpMul     = context->agcErrScaleUpMul;
    pConfig->agcErrScaleDnMul     = context->agcErrScaleDnMul;

    pConfig->scpDcOffsetDeadBand  = context->scpDcOffsetDeadBand;
    pConfig->scpDcOffsetGainShift = context->scpDcOffsetGainShift;
    pConfig->scpDcOffsetGainMult  = context->scpDcOffsetGainMult;

    pConfig->scpDcOffsetFiltMode  = context->DCOffset_filtMode;

    if (pConfig->scpDcOffsetFiltMode == AGC_FILT_SINGLE_POLE)
    {
        pConfig->scpDcOffsetSinglePoleLoopCoeff_a = context->DCOffset_inputFilt.singlePole.a;
        pConfig->scpDcOffsetSinglePoleLoopCoeff_b = context->DCOffset_inputFilt.singlePole.b;
        pConfig->scpDcOffsetSinglePoleInit        = context->DCOffset_inputFilt.singlePole.init;
    }
    else    /* default mode is AGC_FILT_INTEGRATOR */
    {
        pConfig->scpDcOffsetPreIntegrationsLog2 = context->DCOffset_inputFilt.integrator.numIntegrationsLog2;
    }
}

/*
 * Fill the config structure for a default configuration with given AGC mode.
 */
void AGC_getDefaultConfig(AGC_CONFIG_T *pConfig, TDEV_AGC_MODE_T agcMode)
{
    if (agcMode == TDEV_RAPID_AGC)
    {
        pConfig->agcErrScaleUpShift = AGC_SCALE_SHIFT_RAPID_UP;
        pConfig->agcErrScaleDnShift = AGC_SCALE_SHIFT_RAPID_DN;
    }
    else
    {
        pConfig->agcErrScaleUpShift = AGC_SCALE_SHIFT_NORMAL_UP;
        pConfig->agcErrScaleDnShift = AGC_SCALE_SHIFT_NORMAL_DN;
    }

    pConfig->agcErrScaleUpMul               = AGC_SCALE_MUL;
    pConfig->agcErrScaleDnMul               = AGC_SCALE_MUL;

    pConfig->scpDcOffsetFiltMode            = AGC_FILT_INTEGRATOR;
    pConfig->scpDcOffsetPreIntegrationsLog2 = SCP_DC_OFFSET_PRE_INTEGRATIONS_LOG2;

    pConfig->scpDcOffsetDeadBand            = SCP_DC_OFFSET_DEFAULT_DEADBAND;
    pConfig->scpDcOffsetGainShift           = SCP_DC_OFFSET_DEFAULT_GAIN_SHIFT;
    pConfig->scpDcOffsetGainMult            = SCP_DC_OFFSET_DEFAULT_GAIN_MULT;
}



/* De-initialises the AGC context and AGC parts of SCP */
void AGC_DeInit(SCP_T *scp, AGC_CONTEXT_T *context)
{
	SCP_setAGC(scp, context->SCPConfig->normalAGCPeriod, 0, 0);

	SCP_setIQCorrelator(scp, false);

	/* Forget SCP set up */
	context->SCPConfig = NULL;

	return;
}



/* Read signal monitoring registers in SCP */
int AGC_ReadSCPRegs(SCP_T *scp, AGC_CONTEXT_T *context, TDEV_AGCISR_HELPER_T *SCPInfo)
{
	bool IQCorrelatorEnabled;
	/* Set up from context/constant values */
	SCPInfo->AGCMode = context->agcMode;
	SCPInfo->AGCupdatePeriod = (context->agcMode == TDEV_RAPID_AGC) ? context->SCPConfig->rapidAGCPeriod : context->SCPConfig->normalAGCPeriod;
	SCPInfo->sampleRate = (context->SCPConfig)->sampleRate;

	/* The actual reading from SCP registers */
	SCP_getAGCThreshCount(scp, &SCPInfo->AGCcount1I, &SCPInfo->AGCcount1Q);
	SCP_getAGCClipCount(scp, &SCPInfo->AGCcount2I, &SCPInfo->AGCcount2Q);

	/* Correlation between I and Q streams gives a measure of sin(theta), where theta is phase imbalance */
	SCPInfo->IQphaseError = SCP_getIQCorrelator(scp, &IQCorrelatorEnabled);

	SCPInfo->DCoffsetI = SCP_getDCMonitorI(scp);
	SCPInfo->DCoffsetQ = SCP_getDCMonitorQ(scp);

	return(1);
}



/* Write to the appropriate SCP registers */
void AGC_WriteSCPRegs(SCP_T *scp, AGC_CONTEXT_T *context, TDEV_AGCISR_HELPER_T *SCPInfo)
{
	int coarseGain;
	int dummy;

	(void)context;

	/* sets the updated threshold level in the AGC. clip level is constant */
	SCP_setAGC(scp, SCPInfo->AGCupdatePeriod, SCPInfo->pSCPcontrol->AGCthresh2, SCPInfo->pSCPcontrol->AGCthresh1);

	/* set the gain and phase correction SCP registers */
	SCP_getEarlyGains(scp, &dummy, &dummy, &coarseGain, &dummy);

	SCP_setEarlyGains(scp, SCPInfo->pSCPcontrol->fineGainI, SCPInfo->pSCPcontrol->fineGainQ, coarseGain, SCPInfo->pSCPcontrol->IQcorrection);

	/* set the DC offset correction SCP registers */
	SCP_setDCOffsets(scp, SCPInfo->pSCPcontrol->DCoffsetI, SCPInfo->pSCPcontrol->DCoffsetQ);

	return;
}
