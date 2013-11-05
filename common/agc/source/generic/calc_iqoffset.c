/*
** FILE NAME:   $RCSfile: calc_iqoffset.c,v $
**
** TITLE:       Common AGC and related control loop handling
**
** PROJECT:		UCCP Common
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Function to implement IQ Offset loop in the common AGC.
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

/* Keep the IQ gain error accumulator within these values */
#define MIN_IQGAIN_MISMATCH_ACCUM   (-128)
#define MAX_IQGAIN_MISMATCH_ACCUM   (128)


/* Calc the IQ Offset control loop */
void AGC_CalcIQOffsetLoop(AGC_CONTEXT_T *context, TDEV_AGCISR_HELPER_T *SCPInfo)
{
	if (context->complexIF)
	{
		long earlyGainI, earlyGainQ, gain_error;

		long earlyGainIQLeakage = context->IQcorrectionAccum;
        /*  update the phase offset control loop, calculated value equivalent to tan(theta), or sin(theta) for small angles*/
        earlyGainIQLeakage -= (SCPInfo->IQphaseError >> IQ_CORR_SCALE_SHIFT);
        earlyGainIQLeakage = MATHS_restrictRange(earlyGainIQLeakage, MIN_IQ_CORRECTION, MAX_IQ_CORRECTION);
        /* Store back in context */
        context->IQcorrectionAccum = earlyGainIQLeakage;
        SCPInfo->pSCPcontrol->IQcorrection = earlyGainIQLeakage;

		/* error is given by the scaled difference in the threshold counter of I and Q streams*/
		/* The casts to long are to allow signed maths upon what are unsigned values */
        gain_error = (((long)(SCPInfo->AGCcount1I) - (long)(SCPInfo->AGCcount1Q))<<(IQGAIN_ERR_SHIFT))/((long)(SCPInfo->AGCupdatePeriod)); /* update the gain offset control loop */
        context->gain_error_accum += (gain_error >> context->agcIQErrScale);

		/* Ensure that the accum does not go too low oe high */
        context->gain_error_accum = MATHS_restrictRange(context->gain_error_accum, MIN_IQGAIN_MISMATCH_ACCUM, MAX_IQGAIN_MISMATCH_ACCUM);

        /* early gain values are shifted by 8 bits, 256 is equivalent to 1 */
        earlyGainI = (1<<(IQGAIN_ERR_SHIFT+AGC_EARLY_GAIN_SHIFT))/((1<<IQGAIN_ERR_SHIFT)+(context->gain_error_accum));
        earlyGainQ = (1<<(IQGAIN_ERR_SHIFT+AGC_EARLY_GAIN_SHIFT))/((1<<IQGAIN_ERR_SHIFT)-(context->gain_error_accum));

        SCPInfo->pSCPcontrol->fineGainI = MATHS_restrictRange(earlyGainI, MIN_EARLY_GAIN, MAX_EARLY_GAIN);
        SCPInfo->pSCPcontrol->fineGainQ = MATHS_restrictRange(earlyGainQ, MIN_EARLY_GAIN, MAX_EARLY_GAIN);
	}
	else
	{
		SCPInfo->pSCPcontrol->IQcorrection = 0;
		SCPInfo->pSCPcontrol->fineGainI = (1<<AGC_EARLY_GAIN_SHIFT);
		SCPInfo->pSCPcontrol->fineGainQ = (1<<AGC_EARLY_GAIN_SHIFT);
	}

	return;
}
