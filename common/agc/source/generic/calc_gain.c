/*
** FILE NAME:   $RCSfile: calc_gain.c,v $
**
** TITLE:       Common AGC and related control loop handling
**
** PROJECT:		UCCP Common
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Function to implement all gain loop in the common AGC.
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

#include "common_agc.h"
#include "agc_constants.h"
#include "maths_funcs.h"

#define MULT_Q23xQ8(A,B)     MATHS_roundAndShift((A)*(B), 8)

/* Calc the gain control loop */
void AGC_CalcGainLoop(AGC_CONTEXT_T *context, TDEV_AGCISR_HELPER_T *SCPInfo)
{

{
    /* Perform the AGC loop update
    **      g' = g + k(e-g),
    **      g=gain, e=error, k=constant
    ** note, AGC sense is negative
    */
    long AGCCount;
    long error;
    long gain_accum = context->gain_accum;

	/* If complex IF then average I and Q, otherwise just use I value */
    if (context->complexIF)
    	AGCCount = (SCPInfo->AGCcount1I + SCPInfo->AGCcount1Q)>>1;
    else
    	AGCCount = SCPInfo->AGCcount1I;

    error = (((long)(SCPInfo->AGCupdatePeriod) >> 1) - AGCCount);

    if (error > 0)
    {
        error = MULT_Q23xQ8(error, context->agcErrScaleUpMul);
        error >>= context->agcErrScaleUpShift;    /* Gain too low  (slower time constant when increasing gain)   */
    }
    else
    {
        error = MULT_Q23xQ8(error, context->agcErrScaleDnMul);
        error >>= context->agcErrScaleDnShift;    /* Gain too high (more rapid time constant when reducing gain) */
    }

    gain_accum += error;

    /* Saturate AGC value */
    gain_accum = MATHS_restrictRange(gain_accum, ACG_IF_MIN_GAIN, ACG_IF_MAX_GAIN);

	context->gain_accum = gain_accum;
    SCPInfo->IFgainValue = gain_accum;
}

{
	/* Adapting the AGC threshold level - AGCTarget
	** the computation is done in integer format
	** error is given by the difference between clip-count and reference clip-count level (7)
	** if error is too large, AGCthreshold (or AGCTarget) is reduced.
	** AGCthreshold =AGCthreshold - error
	*/
	long AGCTarget;
	long error_thresh;
		/* Always use sum of both clip counts... Even if not a complex input?? Not an average?? */
	long ClipCount;
	long agctarget_accum = context->agctarget_accum;

	if (context->complexIF)
		ClipCount = (SCPInfo->AGCcount2I + SCPInfo->AGCcount2Q)>>1;
	else
		ClipCount = (SCPInfo->AGCcount2I);
	error_thresh = (ClipCount - context->targetClipCount) << AGC_INTSCALE;

	error_thresh = error_thresh >> AGC_TARGET_ERR_SCALE;
	error_thresh = MATHS_restrictRange(error_thresh, error_thresh, (AGC_MAX_ERR << AGC_INTSCALE)); // sets the limit on maximum error

	agctarget_accum -= error_thresh;
	agctarget_accum = MATHS_restrictRange(agctarget_accum, AGC_MIN_TGT << AGC_INTSCALE, AGC_MAX_TGT << AGC_INTSCALE);

	context->agctarget_accum = agctarget_accum;
	AGCTarget = (agctarget_accum + (1<<(AGC_INTSCALE-1)))>> AGC_INTSCALE;

	SCPInfo->pSCPcontrol->AGCthresh1 = AGCTarget;
	SCPInfo->pSCPcontrol->AGCthresh2 = AGC_CLIP_LEVEL;
}
	return;
}
