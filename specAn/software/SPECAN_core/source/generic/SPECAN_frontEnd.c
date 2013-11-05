/*!****************************************************************************
 @File          SPECAN_frontEnd.c

 @Title         Functions to control the analog front end (gain and offset)

 @Date          9 Jan 2012

 @Copyright     Copyright (C) Imagination Technologies Limited 2013

 ******************************************************************************/
#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include "SPECAN_private.h"			/* pulls in common_agc.h */
#include "SPECAN_core.h"

#define MUX_ID  (0)
#define INIT_IF_GAIN_VALUE		(43690>>1)	/* if DISABLE_AGC defined this value passed to tuner */


/*--------------------------------------------------------------------*/

static void agcCalculator(TDEV_AGCISR_HELPER_T *agcData,
                   TDEV_SCP_CONTROL_T *agcControls,
                   void *agcContext)
{
    (void) agcControls;

    /* For the time being, only update AGC loops when there is a real tuner (so the
    analog gain control has an effect).  It should be safe to update them even when there
    is no real tuner, but currently a question mark over the operation of the I/Q gain control
    loop.  Needs review. */
#if !(defined(DUMMY_TUNER) || defined(EXAMPLE_TUNER))
    AGC_CalcLoops(agcContext, agcData); /* runs all three AGC loops,  counter reset in the task. */
#else
    (void)agcData;
    (void)agcContext;
#endif

    return;
}

/* Set the IF Gain value only */
static void initAgcGain(TDEV_AGCISR_HELPER_T *agcData,
                    TDEV_SCP_CONTROL_T *agcControls,
                    void *agcContext)
{
    (void)agcControls;

    agcData->IFgainValue = ((AGC_CONTEXT_T *)agcContext)->gain_accum;

    return;
}

/*--------------------------------------------------------------------*/

static int initSignalLevel(TV_INSTANCE_T *tvInstance)
{
    (void)tvInstance;
    return 0;
    /*
     * Standard-specific
     */
}

void SPECAN_frontEndTask(void)
{
	SPECAN_INSTANCE_CTX_T *SA_ctx = KRN_taskParameter(NULL);
    FRONT_END_TASK_CTX_T *frontEndCtx   = &SA_ctx->frontEndCtx;
	TV_INSTANCE_T        *tvInstance    = SA_ctx->tvInstance;
    TDEV_SCP_CONFIG_T    *scpConfig     = &(tvInstance->tuner.tdev.activeConfigs->scpConfig);

    AGC_CONTEXT_T        *agcCtx        = &frontEndCtx->agcCtx;

    /* establish an initial value for the long term average signal level.  Note the signalLevel and
    muxId arguments to TUNER_setAGC are currently not used.  Potentially they could be used by a tuner driver. */
    int signalLevel = initSignalLevel(tvInstance);

    AGC_Init(tvInstance->scp, agcCtx, scpConfig, TDEV_RAPID_AGC, 1);

    /* Running TVTUNER_stopAGC here prevents everything from kicking off immediately.  We will then block in
	TVTUNER_setAGC below until we run TVTUNER_startAGC (when starting scan) */
    TVTUNER_stopAGC(tvInstance);


    for (;;)
    {
    	TVTUNER_setAGC(tvInstance, signalLevel, MUX_ID, agcCalculator, agcCtx);

    	SPECAN_LOG_VALUE(SPECAN_LOG_CATEGORY_AGC, SPECAN_LOGVAL_FRONT_END_GAIN, agcCtx->gain_accum);

    	if (frontEndCtx->agcCounter)
    	{
    		frontEndCtx->agcCounter--;

    		if (!frontEndCtx->agcCounter)
    		{
                TVTUNER_stopAGC(tvInstance);  /* disable interrupt handling */

            	SA_ctx->frontEndGain = agcCtx->gain_accum;	/* placeholder for last gain value */

            	if (frontEndCtx->isFirstReading)
            	{
            		/* Update the API to indicate the reference level. */
            		TVREG_coreWrite(tvInstance->coreInstance, SA_REF_IF_GAIN_REG, SA_ctx->frontEndGain & 0xFFFF);
            		frontEndCtx->isFirstReading = false;
            	}

                // Send a message to the control task to indicate completion of the agc.
                SPECAN_QctrlStateChange(SA_ctx, SPECAN_STATE_PROC_FRAGMENT);
    		}
    	}
    }
}

/* Set the AGC back to a known state prior to running */
void SPECAN_resetAGC(SPECAN_INSTANCE_CTX_T *SA_ctx)
{
    FRONT_END_TASK_CTX_T *frontEndCtx = &SA_ctx->frontEndCtx;
	TV_INSTANCE_T        *tvInstance  = SA_ctx->tvInstance;

	frontEndCtx->agcCounter = NUM_AGC_ISRS;

    /* Initialise the AGC */
    TVTUNER_setAGCImmediate(tvInstance, initSignalLevel(tvInstance), MUX_ID, initAgcGain, &frontEndCtx->agcCtx);
}

/* Initialises the front end AGC to use a rapid update rate. */
void SPECAN_initAgc(SPECAN_INSTANCE_CTX_T *SA_ctx)
{
	TV_INSTANCE_T        *tvInstance    = SA_ctx->tvInstance;
    TDEV_SCP_CONFIG_T    *scpConfig     = &(tvInstance->tuner.tdev.activeConfigs->scpConfig);
	AGC_CONTEXT_T 		 *agcCtx        = &SA_ctx->frontEndCtx.agcCtx;
    AGC_CONFIG_T   		  agcConfig;


	TVTUNER_setAGCRapid(tvInstance, 0);
	/* Note: run AGC_Init instead of AGC_Configure as this will re-initialise the context space. */
	AGC_Init(tvInstance->scp, agcCtx, scpConfig, TDEV_RAPID_AGC, 1);

    /* Read-modify-write of the DC offset loop gain. */
    AGC_getDefaultConfig(&agcConfig, TDEV_RAPID_AGC);

    /* Modify the default rapid acquisition values to use a lower gain. */
    agcConfig.agcErrScaleUpMul  = AGC_GAIN_LOOP_SCALING;
    agcConfig.agcErrScaleDnMul  = AGC_GAIN_LOOP_SCALING;

    /* disable DC-offset compensation */
    agcConfig.scpDcOffsetGainMult = 0;

    AGC_Configure(agcCtx, TDEV_RAPID_AGC, &agcConfig);

    SPECAN_resetAGC(SA_ctx);

    SA_ctx->frontEndCtx.isFirstReading = true;

    SA_ctx->frontEndGain = INIT_IF_GAIN_VALUE;	/* Set to a known value */
}

/* empty completion function */
static void tdevCompletionFunc(TDEV_T *tuner, TDEV_RETURN_T status, void *parameter)
{
	(void)tuner;
	(void)status;
	(void)parameter;

	return;
}

/* Set the front end gain to the given value. */
int SPECAN_setFrontEndGain(SPECAN_INSTANCE_CTX_T *SA_ctx, long gain)
{
	TV_INSTANCE_T *tvInstance = SA_ctx->tvInstance;
	TUNER_T       *tuner	  = &tvInstance->tuner;
	TDEV_RETURN_T  rtn;

	tuner->agcData.IFgainValue = gain;

	rtn = TDEV_setAGC(&tuner->tdev, &tuner->agcData, tdevCompletionFunc, NULL);

	return (rtn == TDEV_SUCCESS) ? 0 : -1;
}
