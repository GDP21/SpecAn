/*!****************************************************************************
 @File          SPECAN_dcOffsetHandling.c

 @Title         Spectrum analyser DC offset handling module

 @Date          1/2/2013

 @Copyright     Copyright (C) Imagination Technologies Limited 2013

 @Description   Definitions and declarations shared amongst core files, but not
 	 	 	 	made public.

 ******************************************************************************/
#include <assert.h>
#include "SPECAN_dcOffsetHandling.h"
#include "SPECAN_private.h"

#define FREQ_TO_LO_OFFSET(FREQ, SAMPLE_RATE)		((unsigned)((((int64_t)(FREQ) << 25)+(1<<24)) / ((SAMPLE_RATE)/2)))

#ifndef MAX
#define MAX(A,B)	((A) >= (B) ? (A) : (B))
#endif

/*----------------------------------------------------------------------*/
void SPECAN_dcOffsetComp_init(SPECAN_DC_OFFSET_T *pCtx, uint32_t *pWorkspace, unsigned workspaceSz)
{
	pCtx->enabled = false;		/* flag to indicate DC offset compensation is enabled */
	pCtx->compensationFreq = 0;  	/* Frequency added or subtracted in each spectral fragment measurement to eliminate DC bias. */
	pCtx->mixerCompensation = 0; 	/* Compensation applied to the mixer in Q.25 */
	pCtx->offsetBin = 0;			/* Offset to DC bin, non zero when currentCentreFreq and currentTuneFreq differ enough. */
	pCtx->bandFragmentID = 0;		/* For DC compensation, two passes of each band are required. */
	pCtx->fftSize = 0;
	pCtx->pTmpBuf = pWorkspace;
	pCtx->tmpBufSize = workspaceSz;
}

/*----------------------------------------------------------------------*/
/* Set up for operation. */
void SPECAN_dcOffsetComp_configure(SPECAN_DC_OFFSET_T *pCtx,
		bool isEnabled,
		unsigned tunerIncr,
		unsigned ADCclkRate,
		unsigned fftSize,
		unsigned sampleRate,
		unsigned sampleRate_fracBits)
{
	pCtx->enabled = isEnabled;

	if (!pCtx->enabled)
		return;

	assert(fftSize <= pCtx->tmpBufSize);	/* check buffer size. */

	/* Set up the DC offset fields */
	pCtx->fftSize = fftSize;

	pCtx->compensationFreq = SPECAN_DC_OFFSET_GRID_POSITIONS * tunerIncr;

	/* An SCP LO modification must be applied to compensate for the tuner change
	 * This is a Q.25 value and is the ratio of freq/(Fs/2). */
	pCtx->mixerCompensation = FREQ_TO_LO_OFFSET(pCtx->compensationFreq, ADCclkRate);

	pCtx->offsetBin = (pCtx->compensationFreq << sampleRate_fracBits) /
			(sampleRate / fftSize); /* resolution in Q27.4 */

	pCtx->bandFragmentID = SPECAN_BAND_FRAGMENT_PRIMARY; /* DC compensation, primary band */
}

/*----------------------------------------------------------------------*/
/*
 * Copy all of pPBuf to an internal workspace and update fragment ID.
 */
static void _procPrimaryFragment(SPECAN_DC_OFFSET_T *pCtx, GRAM_SXT_T *pPBuf)
{
	unsigned i = pCtx->fftSize;
	GRAM_SXT_T *p1 = pCtx->pTmpBuf;

	/* Copy MCP Power spectral density buffer to temporary workspace */
	while (i--)
	{
		*p1 = *pPBuf;
		p1++;
		pPBuf++;
	}

	pCtx->bandFragmentID = SPECAN_BAND_FRAGMENT_SECONDARY;
}

/*----------------------------------------------------------------------*/
/* Complete DC offset compensation and writing output to pOut */
static void _procSecondaryFragment(SPECAN_DC_OFFSET_T *pCtx, GRAM_SXT_T *pSBuf, uint32_t *pOut)
{
	int i;
	uint32_t a1, a2, b1, b2;
	int32_t dcEst;

	unsigned offsetBin = pCtx->offsetBin;
	unsigned fftSize   = pCtx->fftSize;

	GRAM_SXT_T *pPBuf = pCtx->pTmpBuf;	/* primary fragment buffer. */

	/* DC offset compensation is applied to DC bin and the bins either side. */
#ifdef DOUBLE_GRID_DC_COMPENSATION	// DC compensation uses Fc +/- gridStep
	for (i=-1; i<=1; i++)
	{
		a1 = pPBuf[fftSize-offsetBin+i];	/* Corrupted bin from primary buffer. */
		a2 = pSBuf[fftSize-offsetBin+i];	/* Uncorrupted bin from secondary buffer. */
		b1 = pPBuf[offsetBin+i];			/* Uncorrupted bin from primary buffer. */
		b2 = pSBuf[offsetBin+i];			/* Corrupted bin from secondary buffer. */

		/* Calculate DC estimate */
		dcEst = ((((int32_t)a1-a2) + ((int32_t)b2-b1)) >> 1);

		dcEst = MAX(dcEst, 0);	/* ensure dcEst is non-negative. */

		/* Remove the DC offset. Ensure result is positive, else use uncorrupted bin. */
		if ((dcEst < (int32_t)a1) && (dcEst < (int32_t)b2))
		{
			pPBuf[fftSize-offsetBin+i] = a1 - dcEst;
			pSBuf[offsetBin+i] 		   = b2 - dcEst;
		}
		else
		{
			/* DC estimate is unreliable. Use uncorrupted bin.
			 * This can happen with very low signal powers.  */
			pPBuf[fftSize-offsetBin+i] = a2;
			pSBuf[offsetBin+i] 		   = b1;
		}
#else // DC compensation uses Fc and Fc + gridStep
	uint32_t primaryDcIdx  = fftSize - 1;
	uint32_t secondaryDcIdx = fftSize + offsetBin - 1;

	for (i=-1; i<=1; i++)
	{
		/* Wrap the indexes. */
		primaryDcIdx &= (fftSize-1);
		secondaryDcIdx &= (fftSize-1);

		a1 = pPBuf[primaryDcIdx];		/* Corrupted bin from primary buffer. */
		a2 = pSBuf[primaryDcIdx];		/* Uncorrupted bin from secondary buffer. */
		b1 = pPBuf[secondaryDcIdx];		/* Uncorrupted bin from primary buffer. */
		b2 = pSBuf[secondaryDcIdx];		/* Corrupted bin from secondary buffer. */

		/* Calculate DC estimate */
		dcEst = ((((int32_t)a1-a2) + ((int32_t)b2-b1)) >> 1);

		dcEst = MAX(dcEst, 0);	/* ensure dcEst is non-negative. */

		/* Remove the DC offset. Ensure result is positive, else use uncorrupted bin. */
		if ((dcEst < (int32_t)a1) && (dcEst < (int32_t)b2))
		{
			pPBuf[primaryDcIdx]   = a1 - dcEst;
			pSBuf[secondaryDcIdx] = b2 - dcEst;
		}
		else
		{
			/* DC estimate is unreliable. Use uncorrupted bin.
			 * This can happen with very low signal powers.  */
			pPBuf[primaryDcIdx]   = a2;
			pSBuf[secondaryDcIdx] = b1;
		}
		primaryDcIdx++;
		secondaryDcIdx++;
#endif
	}

	/* Write adjusted PSD to pOut. */
	while (fftSize--)
	{
		*pOut = (*pPBuf+*pSBuf) / 2;
		pOut++;
		pPBuf++;
		pSBuf++;
	}

	pCtx->bandFragmentID = SPECAN_BAND_FRAGMENT_PRIMARY;
}

/* Process a spectral fragment in pBuf to apply DC compensation.
 *
 * This function should be called twice, once for each spectral fragment.
 * The second call will write the resulting compensated buffer to pOut.
 * Fragment ID is managed internally using the bandFragmentID field of the context.
 *
 * Returns:
 *  SPECAN_DC_COMP_RTN_NOT_COMPLETE after processing the first spectral fragment.
 *  SPECAN_DC_COMP_RTN_COMPLETE if DC compensation not enabled or on completing the second fragment processing,  */
SPECAN_DC_COMP_RTN_E SPECAN_dcOffsetComp_proc(SPECAN_DC_OFFSET_T *pCtx, GRAM_SXT_T *pBuf, uint32_t *pOut)
{
	SPECAN_DC_COMP_RTN_E eRtn = SPECAN_DC_COMP_RTN_NOT_COMPLETE;

	if (!pCtx->enabled)
		return SPECAN_DC_COMP_RTN_COMPLETE;	/* Not enabled, then we're done. */

	if(pCtx->bandFragmentID == SPECAN_BAND_FRAGMENT_PRIMARY)
	{
		/* Process the primary fragment - copy to internal workspace. */
		_procPrimaryFragment(pCtx, pBuf);
	}
	else
	{
		/* Process the secondard fragment - calculate a resulting buffer minus any DC. */
		_procSecondaryFragment(pCtx, pBuf, pOut);

		eRtn = SPECAN_DC_COMP_RTN_COMPLETE;
	}

	return eRtn;
}
