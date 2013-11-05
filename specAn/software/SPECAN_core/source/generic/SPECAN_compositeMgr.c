/*!****************************************************************************
 @File          SPECAN_compositeMgr.c

 @Title         Spectrum analyser spectral compositing manager

 @Date          6/2/2013

 @Copyright     Copyright (C) Imagination Technologies Limited 2013

 @Description   Definitions and declarations shared amongst core files, but not
 	 	 	 	made public.

 ******************************************************************************/
#include "SPECAN_compositeMgr.h"
#include "maths_funcs.h"
#include <assert.h>

#ifndef MAX
#define MAX(A,B)	((A) >= (B) ? (A) : (B))
#endif

#ifndef MIN
#define MIN(A,B)	((A) <= (B) ? (A) : (B))
#endif

/* MATHS_log10 uses a Q15.16 format input and returns a Q15.16 value.
 * The format of pFragment is Q8.23 (since it comes from MCP) so add
 * (10*log10(2^-7))*2^16.
 * Result in Q15.16 */
#define TO_DB_Q15_16(A) ((10*MATHS_log10(A))-1380981)
#define OUTPUT_SHIFT	(MATHS_SHIFT_Q16-SPECAN_OUTPUT_FRAC_BITS)
#define ROUND2DB_T(A) (((A)+(1<<(OUTPUT_SHIFT-1))) >> OUTPUT_SHIFT)

#define MAX_PEAK_REGION (SA_REG_MAX_PEAK_WIDTH_MASK*2+1)
#define DB_FLOOR		((SPECAN_DB_T)(1<<(sizeof(SPECAN_DB_T)*8-1)))

/* Initialisation function with a buffer pointer and length. */
void SPECAN_compositeMgr_init(SPECAN_COMPOSITE_MGR_T *pCtx, SPECAN_DB_T *pSpectralBuff, unsigned len)
{
	unsigned i=len/sizeof(SPECAN_DB_T);

	pCtx->pSpectralBuffer  = pSpectralBuff;
	pCtx->pAddr            = pSpectralBuff;
	pCtx->szSpectralBuffer = len;
	pCtx->fftLen           = 0;
	pCtx->fragmentLength   = 0;
	pCtx->totalBins        = 0;
	pCtx->oldOverlapBinVal = 0;
	pCtx->isFirstFragment  = true;

	if (pSpectralBuff==NULL)
		return;

	/* Clear the buffer. May not be needed in final version. */
	while (i--)
	{
		*pSpectralBuff = 0;
		pSpectralBuff++;
	}
}

/* Configure the composite manager given the current scan range, tune step, sample rate and FFT size. */
SPECAN_COMPOSITE_MGR_RTN_E SPECAN_compositeMgr_config(
	SPECAN_COMPOSITE_MGR_T *pCtx,   /* Context pointer */
	unsigned scanRangeHz,
	unsigned tuneStepHz,
	unsigned sampleRate,
	unsigned sampleRate_fracBits,
	unsigned fftLen 				/* FFT length - length of any input pointer.*/
	)
{
	unsigned tuneStepBins = (unsigned)((fftLen * ((uint64_t)tuneStepHz<<sampleRate_fracBits)) / sampleRate);

	/* floor scan range to multiple of tune steps. */
	unsigned totalFreq = (scanRangeHz / tuneStepHz) * tuneStepHz;

	unsigned totalBins = (unsigned)((((uint64_t)totalFreq * fftLen)<<sampleRate_fracBits) / sampleRate) + tuneStepBins;

	/* Check the final buffer size. Is there enough memory available? */
	if (totalBins*sizeof(SPECAN_DB_T) > pCtx->szSpectralBuffer)
		return SPECAN_COMPOSITE_MGR_RTN_FAIL;

	/* Success, continue assigning values to context fields. */
	pCtx->fftLen           = fftLen;
	pCtx->fragmentLength   = tuneStepBins;
	pCtx->totalBins        = totalBins - (tuneStepBins/2); /* first fragment -ve frequency ignored. */
	pCtx->oldOverlapBinVal = 0;
	pCtx->pAddr			   = pCtx->pSpectralBuffer;
	pCtx->isFirstFragment  = true;

	return SPECAN_COMPOSITE_MGR_RTN_OK;
}

/* Add a spectral fragment from the MCP PSD buffer to the final spectrum vector.
 * pFragment points to a buffer of PSD values in Q8.23 format. */

/* TODO - first spectral fragment should read from centre frequency only. */
SPECAN_COMPOSITE_MGR_RTN_E SPECAN_compositeMgr_addFragment(SPECAN_COMPOSITE_MGR_T *pCtx, uint32_t *pFragment)
{
	uint32_t ui32L, ui32H, ui32v;
	int32_t i32LdB, i32HdB, i32offset_dB, i32vdB;
	unsigned i, buffOffset;

	SPECAN_DB_T *pBuffOut = pCtx->pAddr;
	unsigned binOffsetL   = pCtx->fragmentLength/2;
	unsigned binOffsetH   = pCtx->fragmentLength - binOffsetL;
	unsigned fftLen       = pCtx->fftLen;
	unsigned currentIdx   = pCtx->pAddr - pCtx->pSpectralBuffer;

	/* Validate there is enough space left in the buffer before continuing. */
	if (currentIdx+pCtx->fragmentLength > pCtx->totalBins)
		return SPECAN_COMPOSITE_MGR_RTN_FAIL;

	/* Read the low and high values from the fragment. */
	ui32L = pFragment[fftLen - binOffsetL];
	ui32H = pFragment[binOffsetH];

	/* convert to dB */
	i32LdB = TO_DB_Q15_16(ui32L);
	i32HdB = TO_DB_Q15_16(ui32H);

	/* calculate offset (0 for first iteration) */
	i32offset_dB = (pCtx->oldOverlapBinVal) ? (pCtx->oldOverlapBinVal - i32LdB) : 0;

	pCtx->oldOverlapBinVal = i32HdB+i32offset_dB;

	buffOffset = (pCtx->isFirstFragment) ? 0 : binOffsetL; /* Only do from DC to nyquist for the first fragment. */

	/* Positive frequency half. */
	for (i=0; i<binOffsetH; i++)
	{
		ui32v = pFragment[i];

		i32vdB = TO_DB_Q15_16(ui32v);	/* v to dB */

		i32vdB += i32offset_dB;	/* add offset */

		i32vdB = ROUND2DB_T(i32vdB); /* Shift down to byte */

		/* limit to min/max */
		i32vdB = MIN(i32vdB, MAX_DB_T);
		i32vdB = MAX(i32vdB, MIN_DB_T);

		/* set to output */
		pBuffOut[buffOffset+i] = (SPECAN_DB_T)i32vdB;
	}

	if (pCtx->isFirstFragment)
	{
		pCtx->pAddr += binOffsetH; /* Update the pointer. */

		/* Only do from DC to nyquist for the first fragment. */
		pCtx->isFirstFragment = false;
		return SPECAN_COMPOSITE_MGR_RTN_OK;
	}

	/* Negative frequency half. */
	for (i=1; i<=binOffsetL; i++)
	{
		ui32v = pFragment[fftLen-i];

		i32vdB = TO_DB_Q15_16(ui32v);	/* v to dB */

		i32vdB += i32offset_dB;	/* add offset */

		i32vdB = ROUND2DB_T(i32vdB); /* Shift down to byte */

		/* limit to min/max */
		i32vdB = MIN(i32vdB, MAX_DB_T);
		i32vdB = MAX(i32vdB, MIN_DB_T);

		/* set to output */
		pBuffOut[binOffsetL-i] = (SPECAN_DB_T)i32vdB;
	}

	pCtx->pAddr += pCtx->fragmentLength; /* Update the pointer. */

	return SPECAN_COMPOSITE_MGR_RTN_OK;
}

unsigned SPECAN_compositeMgr_getSpectrumSize(SPECAN_COMPOSITE_MGR_T *pCtx)
{
	return pCtx->totalBins;
}

/* Find the nPeaks largest peaks from the completed spectrum. */
void SPECAN_compositeMgr_findPeaks(
		SPECAN_COMPOSITE_MGR_T *pCtx,	/* Context pointer */
		SPECAN_PEAK_T *pPeaks, 			/* Pointer to array of peak structs to be filled. */
		unsigned nPeaks, 				/* Number of peaks to find. */
		unsigned peakWidthBins			/* Peak width in bins. */
	)
{
	unsigned i, j, k, currentIdx;
	SPECAN_DB_T currentMax;
	SPECAN_DB_T *pSpectrum;

	struct PEAK_DESCRIPTOR
	{
		unsigned startId;
		unsigned endId;
		SPECAN_DB_T values[MAX_PEAK_REGION];
	} peakDescriptor[SPECAN_MAX_PEAK_COUNT];

	assert(pCtx);
	assert(pPeaks);
	assert(nPeaks <= SPECAN_MAX_PEAK_COUNT);

	/* Clear the peak results first. */
	for (i=0; i<nPeaks; i++)
	{
		pPeaks[i].i   = 0;
		pPeaks[i].val = 0;
	}

	/* For each peak, search the spectral buffer.
	 * When a peak is found, copy the peak information, including region, to any empty workspace. */
	for (k=0; k<nPeaks; k++)
	{
		/* reset */
		currentMax= (1<<((sizeof(SPECAN_DB_T)*8)-1));
		currentIdx=0;
		pSpectrum = pCtx->pSpectralBuffer;

		/* find the peak. */
		for (i=0; i<pCtx->totalBins; i++)
		{
			if (*pSpectrum > currentMax)
			{
				currentMax = *pSpectrum;
				currentIdx = i;
			}

			pSpectrum++;
		}

		/* Write results to output array */
		pPeaks[k].val = currentMax;
		pPeaks[k].i   = currentIdx;

		/* Store peak to workspace. Care required for boundary peaks. */
		peakDescriptor[k].startId = (currentIdx < peakWidthBins)       			 ? 0 		       : currentIdx-peakWidthBins;
		peakDescriptor[k].endId   = (currentIdx+peakWidthBins > pCtx->totalBins) ? pCtx->totalBins : currentIdx+peakWidthBins;

		for (j=0, i=peakDescriptor[k].startId; i<=peakDescriptor[k].endId; j++, i++)
		{
			peakDescriptor[k].values[j] = pCtx->pSpectralBuffer[i];
			pCtx->pSpectralBuffer[i] = DB_FLOOR;		/* clear region for next search. */
		}
	}

	/* When completed, return the peak information to the spectral buffer. */
	while(nPeaks--)
	{
		for (j=0, i=peakDescriptor[nPeaks].startId; i<=peakDescriptor[nPeaks].endId; j++, i++)
		{
			if (peakDescriptor[nPeaks].values[j] != DB_FLOOR)
				pCtx->pSpectralBuffer[i] = peakDescriptor[nPeaks].values[j];
		}
	}
}
