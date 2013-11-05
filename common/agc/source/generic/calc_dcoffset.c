/*
** FILE NAME:   $RCSfile: calc_dcoffset.c,v $
**
** TITLE:       Common AGC and related control loop handling
**
** PROJECT:		UCCP Common
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Function to implement DC offset loop in the common AGC.
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


inline long updateControl(AGC_CONTEXT_T *context, long control, long offset)
{
    /*
    ** Although the SCP DC canceller is feed-forward (monitor before canceller) we run a control loop
    ** to provide some further noise attenuation on the DC cancellation value
    **
    ** Need to divide offset by DCSamples amd multiply the result by 2^SCP_DC_OFFSET_CTRL_FRACT_BITS
    ** to provide a reference signal that can be compared against the current canceller level. Since the
    ** loop is feed-forward maintaining precision in this calculation is critical
    */
    long error, update;

    error  = MATHS_core(control + offset, context->scpDcOffsetDeadBand);            /* dead-band to prevent excess dither */

    update = MULT_Q23xQ8(error, context->scpDcOffsetGainMult);
    update = MATHS_roundAndShift(update, context->scpDcOffsetGainShift);

    control -= update;

    /* Clip to maximum range */
    return MATHS_restrictRange(control, SCP_DC_OFFSET_CTRL_MIN_VALUE, SCP_DC_OFFSET_CTRL_MAX_VALUE);
}

/* Calc the DC Offset control loop */
void AGC_CalcDCOffsetLoop(AGC_CONTEXT_T *context, TDEV_AGCISR_HELPER_T *SCPInfo)
{
    (void)SCPInfo;

    long offsetI, offsetQ;

    /*
     ** Although the SCP DC canceller is feed-forward (monitor before canceller) we run a control loop
     ** to provide some further noise attenuation on the DC cancellation value.
     **/

    if (context->DCOffset_filtMode == AGC_FILT_SINGLE_POLE)
    {
        AGC_SINGLE_POLE_T *spFilt = &context->DCOffset_inputFilt.singlePole;

        /* shift down to properly scale */
        offsetI = (SCPInfo->DCoffsetI / (int)SCPInfo->AGCupdatePeriod) << SCP_DC_OFFSET_CTRL_FRACT_BITS;
        offsetQ = (SCPInfo->DCoffsetQ / (int)SCPInfo->AGCupdatePeriod) << SCP_DC_OFFSET_CTRL_FRACT_BITS;

        /* apply single pole filter */
        if (spFilt->init == false)
        {
            offsetI = MULT_Q23xQ8(offsetI, spFilt->a) + MULT_Q23xQ8(spFilt->y_I, spFilt->b);
            offsetQ = MULT_Q23xQ8(offsetQ, spFilt->a) + MULT_Q23xQ8(spFilt->y_Q, spFilt->b);
        }
        else
        {
            spFilt->init = false;   /* the first time, output = input */
        }

        spFilt->y_I = offsetI;
        spFilt->y_Q = offsetQ;
    }
    else    /* default integrator mode */
    {
        AGC_PRE_INTEGRATOR_T *integrator = &context->DCOffset_inputFilt.integrator;

        /* Run integrator to extend averaging period and reduce noise on the metric */
        integrator->valI += SCPInfo->DCoffsetI;
        integrator->valQ += SCPInfo->DCoffsetQ;

        /* Once the integrator has integrated enough run the control loops */
        if (++(integrator->cnt) >= integrator->numIntegrations)
        {
            /*
             ** Need to divide integratorI by DCSamples amd multiply the result by 2^SCP_DC_OFFSET_CTRL_FRACT_BITS
             ** to provide a reference signal that can be compared against the current canceller level. Since the
             ** loop is feed-forward maintaining precision in this calculation is critical
             */
            offsetI = (integrator->valI / (int)SCPInfo->AGCupdatePeriod) << SCP_DC_OFFSET_CTRL_FRACT_BITS;
            offsetQ = (integrator->valQ / (int)SCPInfo->AGCupdatePeriod) << SCP_DC_OFFSET_CTRL_FRACT_BITS;

            offsetI >>= (integrator->numIntegrationsLog2);
            offsetQ >>= (integrator->numIntegrationsLog2);

            /* Reset the integrators */
            integrator->valI = 0;
            integrator->valQ = 0;
            integrator->cnt = 0;
        }
        else
        {
            return;
        }
    }

    /* apply update */
    context->DcOffsetI_Accum = updateControl(context, context->DcOffsetI_Accum, offsetI);
    context->DcOffsetQ_Accum = updateControl(context, context->DcOffsetQ_Accum, offsetQ);

    SCPInfo->pSCPcontrol->DCoffsetI = context->DcOffsetI_Accum;
    SCPInfo->pSCPcontrol->DCoffsetQ = context->DcOffsetQ_Accum;

	return;
}
