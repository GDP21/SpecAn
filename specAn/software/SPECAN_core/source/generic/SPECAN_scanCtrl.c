/*!****************************************************************************
 @File          SPECAN_scanCtrl.c

 @Title         Spectrum Analyser core - control of scan operations

 @Date          27 November 2012

 @Copyright     Copyright (C) Imagination Technologies Limited 2012

 ******************************************************************************/
#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <assert.h>
#include "SPECAN_private.h"
#include "mcpprogs.h"

#define nTEST_SINGLE_FRAGMENT

void SPECAN_processFragment(SPECAN_INSTANCE_CTX_T *SA_ctx)
{
	TV_INSTANCE_T *tvInstance = SA_ctx->tvInstance;
	MCP_T         *mcp        = tvInstance->mcp;
	//UFW_COREINSTANCE_T *coreInstance = tvInstance->coreInstance;

	/* Re-load outer loop counter from API.  Note that this counter runs to zero, so we
	load count value - 1 to get the desired number of iterations.
	(The inner loop counter is maintained by MCP following its initial set-up in
	SPECAN_setUpForScan) */
	*SA_ctx->MCP_ptrs.averagingPeriod_outerLoopCount = SA_ctx->outerLoopCount - 1;

	/* Clear all buffers, intermediate and output. */
	MCP_fill32uint(mcp, mcp_src_intermediatePowBuf(mcp), 0, SA_ctx->FFTsize);
	MCP_fill32uint(mcp, mcp_src_outputPowBuf(mcp),       0, SA_ctx->FFTsize);

	/* Kick off fragment processing by queueing 2 jobs into the SCP, along with associated MCPOS jobs.  The MCPOS jobs go
	into a queue declared within our SPECAN_mcposDriver, so they are gated on SCP job completions. */
	/* ...job A */
	QM_postTail(SA_ctx->tvInstance->scp->driver.inCaptureLenQId, SA_ctx->SCPcaptureLen);
	QM_postTail(SA_ctx->tvInstance->scp->driver.inDiscardLenQId, 0);
	QM_postTail(SA_ctx->tvInstance->scp->driver.inAddrQId, SA_ctx->MCP_ptrs.SCPoutBufferA.wordOffset);
	MCPOS_startJob(&SA_ctx->mcposUse, SA_ctx->mcposSCPjobQID, mcp_src_SCP_CAPTURE_JOB_BUFA(SA_ctx->tvInstance->mcp));
	/* ...job B */
	QM_postTail(SA_ctx->tvInstance->scp->driver.inCaptureLenQId, SA_ctx->SCPcaptureLen);
	QM_postTail(SA_ctx->tvInstance->scp->driver.inDiscardLenQId, 0);
	QM_postTail(SA_ctx->tvInstance->scp->driver.inAddrQId, SA_ctx->MCP_ptrs.SCPoutBufferB.wordOffset);
	MCPOS_startJob(&SA_ctx->mcposUse, SA_ctx->mcposSCPjobQID, mcp_src_SCP_CAPTURE_JOB_BUFB(SA_ctx->tvInstance->mcp));

}

void SPECAN_processFragmentCompletion(SPECAN_INSTANCE_CTX_T *SA_ctx)
{
	uint32_t *pFragment = NULL;
  	GRAM_SXT_T *pMCPBuff = SA_ctx->MCP_ptrs.SpectralFragmentBuff.sxtAddr;
	SPECAN_CONTROL_STATE_T stateChangeRequest = SPECAN_STATE_TUNE;
	SPECAN_DC_OFFSET_T *pDcCtx = &SA_ctx->dcOffsetCtx;

	if (pDcCtx->enabled)
   	{	/* DC compensation. */
		if(SPECAN_dcOffsetComp_proc(pDcCtx, pMCPBuff, pDcCtx->pTmpBuf) == SPECAN_DC_COMP_RTN_COMPLETE)
			pFragment = pDcCtx->pTmpBuf;	/* DC compensation completed, assign pointer. */
   	}
   	else
   	{	/* No DC compensation, assign pointer. */
   		pFragment = pMCPBuff;
   	}

	if (pFragment)
	{
		/* Fragment available, add to the spectrum buffer. */
		SPECAN_COMPOSITE_MGR_RTN_E eRtn = SPECAN_compositeMgr_addFragment(&SA_ctx->compositeCtx, pFragment);

		assert(eRtn != SPECAN_COMPOSITE_MGR_RTN_FAIL);
		(void)eRtn;						// Remove warning from RELEASE build

#ifdef TEST_SINGLE_FRAGMENT
		return;
#endif

		if (SA_ctx->currentCentreFreq + SA_ctx->tuningStep > SA_ctx->finalTuneFreq)
		{
			/* No new bands, we're done. */
			stateChangeRequest = SPECAN_STATE_PROC_COMPLETE;
		}
	}

	SPECAN_QctrlStateChange(SA_ctx, stateChangeRequest);
}

/* Change the tuning frequency given the current context.
 *
 * If DC compensation is enabled, the tuner must retune to offset above or below a centre frequency.
 * If DC offset compensation is disabled or has been completed the tuner will move to the next available frequency.
 * If no new frequency is available, i.e. the scan has completed, return -1, else return 0.
 * */
int SPECAN_updateTuner(SPECAN_INSTANCE_CTX_T *SA_ctx)
{
	unsigned freq;
	SPECAN_DC_OFFSET_T *pDcCtx = &SA_ctx->dcOffsetCtx;

	if (pDcCtx->enabled && (pDcCtx->bandFragmentID == SPECAN_BAND_FRAGMENT_SECONDARY))
	{
		/* Stay in the same band, but tune to secondary half */
		freq = SA_ctx->currentCentreFreq + pDcCtx->compensationFreq;

		/* Re-centre the signal back to the current centre freq. */
		SCP_setFc(SA_ctx->tvInstance->scp, pDcCtx->mixerCompensation);
	}
	else
	{
		/* Tune to a new band for the next spectral fragment. */

		/* If this isn't the first band then get a new frequency */
		if (!SA_ctx->isStartOfScan)
			SA_ctx->currentCentreFreq = SA_snapToFrequency(SA_ctx, SA_ctx->currentCentreFreq + SA_ctx->tuningStep);
		else
			SA_ctx->isStartOfScan = false;	/* clear the flag */

		if(SA_ctx->currentCentreFreq > SA_ctx->finalTuneFreq) /* We shouldn't get here without checking this first */
			return -1;

		if (pDcCtx->enabled)
		{
#ifdef DOUBLE_GRID_DC_COMPENSATION // DC compensation uses Fc +/- gridStep
			/* Primary fragment, apply an offset. */
			freq = SA_ctx->currentCentreFreq - pDcCtx->compensationFreq;

			/* Re-centre the signal back to the current centre freq. */
			SCP_setFc(SA_ctx->tvInstance->scp, -pDcCtx->mixerCompensation);
#else 							// DC compensation uses Fc and Fc + gridStep
			freq = SA_ctx->currentCentreFreq;
			SCP_setFc(SA_ctx->tvInstance->scp, 0);
#endif
		}
		else
		{
			freq = SA_ctx->currentCentreFreq;
		}
	}

	SA_retune(SA_ctx, freq, SA_ctx->currentBW);

	return 0;
}
