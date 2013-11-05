/*!****************************************************************************
 @File          SPECAN_configure.c

 @Title         Spectrum Analyser core - configuration functions

 @Date          27 November 2012

 @Copyright     Copyright (C) Imagination Technologies Limited 2012

 ******************************************************************************/
#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <assert.h>
#include "SPECAN_private.h"
#include "SPECAN_core.h"
#include "mcpprogs.h"

#define CALCULATE_DECIMATION_STAGE			(1)
#define SELECT_FFT_BY_BW					(1)


#ifdef CALCULATE_DECIMATION_STAGE
#include "SPECAN_scpData.h"
#endif

extern const unsigned HammingWindow[];
extern const unsigned HanningWindow[];

/* TODO The resampler pass band has a very poor roll
 * so setting the tune step to be half the tuner bandwidth. */
#define BW2TUNE_STEP(BW)				    ((BW * 8) / 10)
//#define BW2TUNE_STEP(BW)				    (BW>>1)

#define RESOLUTION_FRAC_BITS				(4)

/*----------------------------------------------------------------------*/

/* This is a local copy of the static function _TV_griddedFrequency in tvcore.
This function is not made public from tvcore because eventually, the gridding is
not supposed to be done in this way.  Eventually there should be a mechanism for
the local tuner driver to communicate it's knowledge of the gridding, in which
case the TV_REG_TUNER_GRID_BASE and TV_REG_TUNER_GRID_INCR registers will be
applicable only to the remote tuner case (and the gridding will in all cases
be dealt with internally within tvcore).  */
static uint32_t TV_griddedFrequency(TV_INSTANCE_T *tvInstance, uint32_t f)
{
    uint32_t base, increment, remainder;
    int offset;
    bool negoff = false;

    increment = TVREG_read(tvInstance->coreInstance, TV_REG_TUNER_GRID_INCR);
    if (increment > 1)    {
        base = TVREG_read(tvInstance->coreInstance, TV_REG_TUNER_GRID_BASE);
        offset = f - base;
        if (offset < 0)
        {
            offset = -offset;
            negoff = true;
        }
        remainder = offset % increment;
        offset -= remainder;
        if (negoff)
        {
            if (remainder > (increment / 2))
                offset = offset + increment;
            return base - offset;
        }
        else
        {
            if (remainder >= (increment / 2))
                offset = offset + increment;
            return base + offset;
        }
    }
    else
        return f;
}

/*----------------------------------------------------------------------*/

void SA_retune(SPECAN_INSTANCE_CTX_T *SA_ctx, unsigned freq, unsigned bandwidth)
{
	/* Re-tune */
	TV_reTune(SA_ctx->tvInstance, freq, bandwidth, false);

	UCC_LOGMSG("SA re-tuned to %dMHz", freq / 1000000);
}

/*----------------------------------------------------------------------*/
uint32_t SA_snapToFrequency(SPECAN_INSTANCE_CTX_T *SA_ctx, uint32_t f)
{
	/* Simple wrap but means one place to change if the TV_griddedFrequency function is removed. */
	return TV_griddedFrequency(SA_ctx->tvInstance, f);
}

/*----------------------------------------------------------------------*/

/* Calculate an appropriate tuning step for given API settings */
static unsigned _SA_getTuningStep(SPECAN_INSTANCE_CTX_T *SA_ctx)
{
	TV_INSTANCE_T *tvInstance = SA_ctx->tvInstance;
	UFW_COREINSTANCE_T *coreInstance = tvInstance->coreInstance;
	unsigned val, bitMask, tuningStep;
	unsigned incr = TVREG_read(coreInstance, TV_REG_TUNER_GRID_INCR);

	/* Register value includes the "auto" control bit on MSB */
	val = TVREG_read(coreInstance, SA_TUNING_STEP);
	bitMask = 1 << SA_REG_AUTO_TUNE_STEP_BITSHIFT;
	if (val & bitMask)
	{
		/* Auto tuning step = tuner bandwith less 20% to provide overlap
		(TODO: for review).  Overlap is firstly to correct for AGC
		gain changes between different tunings, and secondly to allow upper frequency bins
		to be unused (due to encroaching on Nyquist filter response)  */
		tuningStep = BW2TUNE_STEP(SA_ctx->currentBW);

		tuningStep = MAX(tuningStep, incr); /* limit to minimum grip increment. */
	}
	else
	{
		tuningStep = val & ~bitMask; /* Manual tuning step. */
	}

	/* Snap tuningStep to grid. */
	tuningStep -= (tuningStep % incr);

	return tuningStep;
}

#ifdef CALCULATE_DECIMATION_STAGE
/*--------------------------------------------------------------------*/
/* Load appropriate FIR coefficients. */

void SA_loadFIRcoeffs(SPECAN_INSTANCE_CTX_T *SA_ctx)
{
	SA_SCP_COEFF_SET_E coeffSetIdx;
	FIR_COEFF_SET_T *pCoeffSet = NULL;

	unsigned dec = SA_ctx->CICdecimationFactor * SA_ctx->FIRdecimationFactor;

	unsigned i=0;

	/* find the decimation index. */
	while (SA_decimatorFactorTable.decimation[i] != dec && i<NUM_DECIMATION_FACTORS_AVAILABLE)
	    i++;

	coeffSetIdx = SA_decimatorFactorTable.coeffId[i];

	pCoeffSet = SA_FIRCoeffs[coeffSetIdx];
	SCP_setFIRCoeffs(SA_ctx->tvInstance->scp, (int16_t *)pCoeffSet);
}
#endif

#define SPECAN_FFTSHIFT_MEMSPACE (7)
/*----------------------------------------------------------------------*/

static bool _SA_configureSamplingParams(SPECAN_INSTANCE_CTX_T *SA_ctx)
{
	TV_INSTANCE_T *tvInstance = SA_ctx->tvInstance;
	UFW_COREINSTANCE_T *coreInstance = tvInstance->coreInstance;
	TUNER_ACTIVE_CONFIG_T *tunerConfig = TVTUNER_getActiveConfig(tvInstance, 0);
	TDEV_SCP_CONFIG_T *scpConfig;

	unsigned resolution = TVREG_read(coreInstance, SA_SCAN_RESOLUTION);
	unsigned tunerIncr  = TVREG_read(coreInstance, TV_REG_TUNER_GRID_INCR);
	uint32_t FFTsize;
	uint64_t ADCclkRate;
    unsigned RSfactor, i, j, binsPerTuneIncr;
    int fso;
#ifndef SELECT_FFT_BY_BW
    unsigned minimumEffectiveSampleRate;
#endif
#ifndef CALCULATE_DECIMATION_STAGE
    /* Available frequencies to snap to. */
    int fs[SPECAN_SCP_RATE_MAX] = {51200000, 64000000, 81920000};
    int freqGap, minGap = 0x7FFFFFFF;
#else
    unsigned wantedDecimationFactor, numDecimationFactorsAvailable, k;
#endif


	assert(tunerConfig);
    scpConfig = &(tunerConfig->scpConfig);
    ADCclkRate = (uint64_t)scpConfig->sampleRate;

	/* The frequency resolution for the scan is set by the effective sample
	rate divided by the chosen FFT size.  This needs to match the resolution
	requested by the API.
	To ensure that samples are taken within the tuner filter roll-off,
	the effective sample rate must exceed 20% of the tuner bandwidth.

	Note that since the tuning step must be a whole number of bins, the resolution will be adapted accordingly.
	Resolution needs to be an integer
	 */
    binsPerTuneIncr = (((tunerIncr<<1) / resolution)+1)>>1; /* round to nearest integer */

    /* Recalculate the resolution with RESOLUTION_FRAC_BITS fractional bits. */
    resolution = (tunerIncr<<RESOLUTION_FRAC_BITS) / binsPerTuneIncr;

#ifdef SELECT_FFT_BY_BW
    /* Chose the largest FFT that allows the criteria to be met, without exceeding the tuner bandwidth. */
    int fftBW;
    int tunerBwLimit = SA_ctx->currentBW;
    i = 5;
    for(FFTsize = MIN_FFT_SIZE_SUPPORTED; FFTsize <= (MAX_FFT_SIZE_SUPPORTED*2); FFTsize<<=1)
    {
    	fftBW = (resolution * FFTsize) >> RESOLUTION_FRAC_BITS;
    	if(fftBW >= tunerBwLimit)
    		break;
    	i++;
    }
    FFTsize >>= 1;	// Use the smaller size
#else

	/* Start by choosing an FFT size which allows the Nyquist criterion above
	to be met */
	/* Note: by the "effective" sample rate we mean the sample rate after resampling
	and decimation within the SCP. */
	minimumEffectiveSampleRate = SA_ctx->currentBW; /* Nyquist requirement */
	minimumEffectiveSampleRate = (minimumEffectiveSampleRate * 6)/5; /* Add 20% */
	FFTsize = (minimumEffectiveSampleRate<<RESOLUTION_FRAC_BITS) / resolution;	/* Resolution in Q27.4*/
	/* .. Round FFTsize up to a power of 2 between 16 and 8192 */
	i = 31;
	/* ...find most significant 1 in FFTsize */
	while (i > 5 && !(FFTsize & (1 << i)))
	{
		i--;
	}
	if (FFTsize != (uint32_t)(1 << i))
	{
		/* ...FFTsize is not already a power of 2.  Round up to next power of 2 */
		i++;
		FFTsize = (uint32_t)(1 << i);
	}

#endif
	if ((FFTsize < MIN_FFT_SIZE_SUPPORTED) || (FFTsize > MAX_FFT_SIZE_SUPPORTED))
	{
		/* Failure: the effective resolution has been set too small compared to the bandwidth, so we can't
		achieve it with the max FFT length available.  If we have chosen the bandwidth, this means we need
		to make it smaller and reduce the tuning step size accordingly.  If the bandwidth has been chosen by the user,
		this needs to be flagged as a user error. */
		return false;
	}

	SA_ctx->FFTsize = FFTsize;

	*SA_ctx->MCP_ptrs.pFFTlen = FFTsize;

	// Get the log2 of FFTsize (FFTsize is a power of 2)
	*SA_ctx->MCP_ptrs.pFFTlen_log2 = i;

	/* Fill MCP FFT shift values. i is log2 of FFTsize. */
	for(j=0; j<SPECAN_FFTSHIFT_MEMSPACE; j++)
	{
		SA_ctx->MCP_ptrs.pFFTshifts[j] = (j<(i-mcp_src_SPECAN_fftFunc_offset(mcp))) ? 0x40000 : 0;
	}


#ifndef SCP_BYPASS_MODE
	/* Now calculate an actual effective sample rate to go with that FFT size (effectiveSampleRate in Q27.4) */
	SA_ctx->q27p4_effectiveSampleRate = resolution * FFTsize;
#else
	SA_ctx->q27p4_effectiveSampleRate = ADCclkRate << RESOLUTION_FRAC_BITS;
#endif

	/* The platform setup should contain SCP configs for the sample rates defined in the core enum type.
	 * However, still need to calculate the resampler factor.
     */

	fso = (int)(SA_ctx->q27p4_effectiveSampleRate >> RESOLUTION_FRAC_BITS);	/* output sample rate (integer part). */

#ifdef CALCULATE_DECIMATION_STAGE
	/* Do not load pre-defined SCP configurations, calculate the configuration here. */

	/* Calculate nominal resample factor with 25 fractional bits, without taking decimation
	 * into account.  Use 64-bit values to give sufficient headroom (could eventually switch to
	 * using DSP divide instruction on Meta).  */
	RSfactor = (unsigned)((ADCclkRate << 25) / (uint64_t)(fso));

	/* Find a decimation setup that will give a resampling factor between 1 and 2. */
    for (k=0; k<NUM_DECIMATION_FACTORS_AVAILABLE; k++)
    {
        if (RSfactor <= (SA_decimatorFactorTable.decimation[k] * SA_SCP_FS_UNITY))
            break;
    }
    wantedDecimationFactor = SA_decimatorFactorTable.decimation[k];
    /* Find the nearest achievable decimation factor from the table */
	numDecimationFactorsAvailable = GET_NUM_DEC_FACTORS(RSfactor);
	i = 0;
	while (i < (numDecimationFactorsAvailable - 1) && SA_decimatorFactorTable.decimation[i] < wantedDecimationFactor)
		i++;

	/* Divide decimation factor between CIC and FIR decimators */
	SA_ctx->CICdecimationFactor = SA_decimatorFactorTable.cicDec[i];
	SA_ctx->FIRdecimationFactor = SA_decimatorFactorTable.firDec[i];

	/* Modify RSfactor according to decimation */
	RSfactor /= SA_decimatorFactorTable.decimation[i];

	/* Check RS factor within limits 0.5-1.999 */
	if (RSfactor < (SA_SCP_FS_UNITY/2) || RSfactor >= (2*SA_SCP_FS_UNITY))
	{
		assert(!"Fs out of range");
		return 2;
	}

	/* Set up the SCP with our new sampling rate and decimation factors */
	if (!SCP_setCICDec(tvInstance->scp, SA_ctx->CICdecimationFactor) || !SCP_setFIRDec(tvInstance->scp, SA_ctx->FIRdecimationFactor))
	{
		assert(!"decimation factor out of range");
		return 3;
	}
	SCP_setFs(tvInstance->scp, RSfactor);

	/* Re-load FIR coefficients, since these are dependent on RRC rolloff factor and FIR
	 * decimation factor. */
	SA_loadFIRcoeffs(SA_ctx);

#else
	/* Find the SCP config ID for the closest sample rate. */
	for (i=0; i<SPECAN_SCP_RATE_MAX; i++)
	{
		freqGap = fs[i] - fso;
		freqGap *= (freqGap < 0) ? -1 : 1;	/* Make abs */

		if (freqGap < minGap)
		{
			minGap = freqGap;
			j = i;				/* set the config ID. */
		}
	}

	TVTUNER_setSCP(tvInstance, j);		/* Set the SCP config. */

#if 1
	int16_t coeffs[] = {   -5,  -10,   -5,    8,   17,   10,  -13,  -31,  -18,   35,  108,  161};	/* Fs=51.2MHz  */
	SCP_setFIRCoeffs(tvInstance->scp, coeffs);
	SCP_setCICDec(tvInstance->scp, 1);
	SCP_setFIRDec(tvInstance->scp, 3);
#endif


	/* Read back the CIC/FIR decimation values. */
	SA_ctx->CICdecimationFactor = SCP_getCICDec(tvInstance->scp);
	SA_ctx->FIRdecimationFactor = SCP_getFIRDec(tvInstance->scp);

	/* Calculate the resampler factor */
	RSfactor = (unsigned)(((uint64_t)ADCclkRate << (25+RESOLUTION_FRAC_BITS)) / SA_ctx->q27p4_effectiveSampleRate);
	RSfactor /= (SA_ctx->CICdecimationFactor*SA_ctx->FIRdecimationFactor);

    /* Check RS factor within limits 0.5-1.999 */
    if (RSfactor < (1 << 24) || RSfactor >= ((1 << 26)))
    {
    	assert(!"Fs out of range");
    	return false;
    }

    SCP_setFs(tvInstance->scp, RSfactor);
    SA_ctx->RSfactor = RSfactor;
#endif

    /* Write the resolution back to the API. */
    resolution = (resolution+(1<<(RESOLUTION_FRAC_BITS-1))) >> RESOLUTION_FRAC_BITS; /* round to nearest Hertz. */
    TVREG_coreWrite(coreInstance, SA_SCAN_RESOLUTION, resolution);

	return true;
}

/*----------------------------------------------------------------------*/

static void _SA_loadWindowFunc(SPECAN_INSTANCE_CTX_T *SA_ctx)
{
	/* Load data for window function into MCP data space */
	unsigned decimationFactor, i, interpSample;
	unsigned *refWindowFunction;
	unsigned prevSample, nextSample, scaledInputSample;
	int prevIndex, currentIndex, nextIndex, indexInc, destIndex;
	unsigned fullWindowSize = STORED_WINDOW_SIZE << 1; /* Only half the window is stored */

	if (SA_ctx->windowFunc == WINDOW_RECTANGULAR)
	{
		/* TODO: Set flag in MCP code to bypass windowing */
	}
	else
	{
		/* Pick desired window function */
		if (SA_ctx->windowFunc == WINDOW_HAMMING)
			refWindowFunction = (unsigned *)HammingWindow;
		else
			refWindowFunction = (unsigned *)HanningWindow;

		/* Divide doubled stored window size by FFT size, using one fractional
		bit, to determine required decimation / interpolation factor (the stored window
		size is doubled here because only half the window is stored) */
		decimationFactor = (STORED_WINDOW_SIZE << 2) / SA_ctx->FFTsize;

		if (decimationFactor == 1 /* 0.5 with 1 fractional bit */)
		{
			/* Up-sample the window data by linear interpolation (upsampling by
			a factor of 2 only is currently supported).  IndexInc causes us to traverse the
			half-size reference window in forward then backward direction. */
			destIndex = 0;
			for (indexInc = 1; indexInc >= -1; indexInc -= 2)
			{
				if (indexInc == 1)
				{
					/* Forward pass */
					prevIndex = -1; currentIndex = 0; nextIndex = 1;
				}
				else
				{
					/* Reverse pass */
					prevIndex = STORED_WINDOW_SIZE; currentIndex = STORED_WINDOW_SIZE - 1; nextIndex = STORED_WINDOW_SIZE - 2;
				}

				for (i = 0; i < STORED_WINDOW_SIZE; i++)
				{
					if (prevIndex < 0 || prevIndex >= STORED_WINDOW_SIZE)
						prevSample = refWindowFunction[currentIndex] & 0xFFF;
					else
						prevSample = refWindowFunction[prevIndex] & 0xFFF;
					if (nextIndex < 0 || nextIndex >= STORED_WINDOW_SIZE)
						nextSample = refWindowFunction[currentIndex] & 0xFFF;
					else
						nextSample = refWindowFunction[nextIndex] & 0xFFF;

					/* For each input sample we form 2 output samples spaced +/-1/4 input sample away */
					scaledInputSample = (refWindowFunction[currentIndex] & 0xFFF) * 3;
					scaledInputSample += 2; /* Rounding */
					scaledInputSample >>= 2; /* 3/4 of refWindowFunction[i], rounded. */
					interpSample = scaledInputSample + ((prevSample + 2) >> 2); /* (0.75 * input sample) + (0.25 * previous sample) */
					SA_ctx->MCP_ptrs.windowFunc.sxtAddr[destIndex++] = interpSample | (interpSample << 12); /* Same value on real and complex parts */
					interpSample = scaledInputSample + ((nextSample + 2) >> 2); /* (0.75 * input sample) + (0.25 * next sample) */
					SA_ctx->MCP_ptrs.windowFunc.sxtAddr[destIndex++] = interpSample | (interpSample << 12); /* Same value on real and complex parts */

					prevIndex += indexInc; currentIndex += indexInc; nextIndex += indexInc;
				}
			}
		}
		else if ((fullWindowSize >= SA_ctx->FFTsize) &&
				(fullWindowSize % SA_ctx->FFTsize == 0) /* Integer decimation factor,
												i.e. FFT size must divide exactly into the window length */)
		{
			/* Decimate the window data to match window size to FFT length */
			/* .. remove the fractional bit, this is then our decimation factor */
			decimationFactor >>= 1;

			destIndex = 0;
			/* IndexInc causes us to traverse the half-size reference window in forward then backward direction. */
			for (indexInc = 1; indexInc >= -1; indexInc -= 2)
			{
				if (indexInc == 1)
				{
					/* Forward pass */
					currentIndex = 0;
				}
				else
				{
					/* Reverse pass */
					currentIndex = STORED_WINDOW_SIZE - 1;
				}

				for (i = 0; i < STORED_WINDOW_SIZE; i += decimationFactor)
				{
					SA_ctx->MCP_ptrs.windowFunc.sxtAddr[destIndex++] = refWindowFunction[currentIndex];
					currentIndex += (indexInc * decimationFactor);
				}
			}
		}
		else
		{
			assert(!"Unsupported decimation factor");
		}
	}

	return;
}

/*----------------------------------------------------------------------*/

int SPECAN_setUpForScan(SPECAN_INSTANCE_CTX_T *SA_ctx)
{
	TV_INSTANCE_T         *tvInstance   = SA_ctx->tvInstance;
	UFW_COREINSTANCE_T    *coreInstance = tvInstance->coreInstance;
	TUNER_ACTIVE_CONFIG_T *tunerConfig  = TVTUNER_getActiveConfig(tvInstance, 0);
	TDEV_SCP_CONFIG_T     *scpConfig    = &(tunerConfig->scpConfig);

	int64_t ADCclkRate = (int64_t)scpConfig->sampleRate;

	unsigned val, loopNlog2;

	/* Initialise our working parameters from API settings */
	SA_ctx->isStartOfScan = true;

	/* The controlling application will have tuned us to the starting frequency
	and bandwidth */
	SA_ctx->currentTuneFreq = TV_getActiveFrequency(tvInstance);
	SA_ctx->currentBW = TV_getActiveBandwidth(tvInstance);

	/* TODO: this to be removed to separate tuning functions */
	/* Our tune frequency must be on the defined tuning grid.  In the case of a remote
	tuner the gridding is performed within the tvcore tune process; but for a local tuner
	this is not the case.  Pick up an off-grid tuned frequency and re-tune if necessary. */
	val = TV_griddedFrequency(tvInstance, SA_ctx->currentTuneFreq);
	if (val != SA_ctx->currentTuneFreq)
	{
		SA_retune(SA_ctx, val, SA_ctx->currentBW);
		SA_ctx->currentTuneFreq = val;
	}
	SA_ctx->currentCentreFreq = SA_ctx->currentTuneFreq; /* Initially these are equal. */

	SA_ctx->scanRange = TVREG_read(coreInstance, SA_SCAN_RANGE);

	/* Set a final frequency target for the tuner */
	SA_ctx->finalTuneFreq = SA_ctx->currentTuneFreq + SA_ctx->scanRange;

	/* Work out tuning step */
	SA_ctx->tuningStep = _SA_getTuningStep(SA_ctx);

	/* Set up resample factor, decimation factors and FFT size for required
	scan resolution */
	if (!_SA_configureSamplingParams(SA_ctx))
	{
		/* TODO: deal with failure without asserting */
		assert(!"Unable to configure sampling params");
	}

	/* SCP capture length is equal to the FFT length, but subject to a minimum */
	if (SA_ctx->FFTsize < MIN_SCP_CAPTURE_LEN)
		SA_ctx->SCPcaptureLen = MIN_SCP_CAPTURE_LEN;
	else
		SA_ctx->SCPcaptureLen = SA_ctx->FFTsize;

	/* Record settings from SA_MEASUREMENT_CONTROL register */
	val = TVREG_read(coreInstance, SA_MEASUREMENT_CONTROL);
	SA_ctx->windowFunc = (val >> SA_REG_WINDOW_TYPE_BITSHIFT) & SA_REG_WINDOW_TYPE_MASK;

	/* Read SA_MAX_PEAK_WIDTH values. */
	SA_ctx->peakWidthBins = (val >> SA_REG_MAX_PEAK_WIDTH_BITSHIFT) & SA_REG_MAX_PEAK_WIDTH_MASK;

#ifndef SCP_BYPASS_MODE
	SPECAN_dcOffsetComp_configure(&SA_ctx->dcOffsetCtx,
			(val >> SA_REG_ENABLE_DC_COMP_BITSHIFT) & SA_REG_ENABLE_DC_COMP_MASK,	/* DC compensation bit */
			TVREG_read(coreInstance, TV_REG_TUNER_GRID_INCR),	/* tuner grid increment */
			ADCclkRate,
			SA_ctx->FFTsize,
			SA_ctx->q27p4_effectiveSampleRate,
			RESOLUTION_FRAC_BITS);
#else
	UCC_LOGMSG("SCP bypassed - DC compensation disabled.");
#endif

	/* Configure spectral compositing */
	{
		uint32_t len;

		SPECAN_COMPOSITE_MGR_RTN_E eRtn;

		eRtn = SPECAN_compositeMgr_config(&SA_ctx->compositeCtx,
			SA_ctx->scanRange,
			SA_ctx->tuningStep,
			SA_ctx->q27p4_effectiveSampleRate,
			RESOLUTION_FRAC_BITS,
			SA_ctx->FFTsize);

		if(eRtn != SPECAN_COMPOSITE_MGR_RTN_OK)
			return -1;

		val  = (SA_ctx->outputBuffer.type << SA_REG_OUT_SPEC_MEM_TYPE_BITSHIFT) |
				((unsigned)SA_ctx->outputBuffer.wordOffset & SA_REG_OUT_SPEC_PTR_MASK);
		len = SPECAN_compositeMgr_getSpectrumSize(&SA_ctx->compositeCtx);

		/* Update the API registers. */
		TVREG_coreWrite(coreInstance, SA_REG_OUT_SPECTRUM_PTR, val);
		TVREG_coreWrite(coreInstance, SA_OUT_SPECTRUM_LEN, len);
	}

	/* Set up inner loop counter in MCP code.  Note that the outer loop counter
	is re-loaded from Meta code in SPECAN_processFragment.  After each fragment processing the
	inner loop counter returns to its starting value so doesn't need to be touched by Meta after this.
	Note that this counter runs to zero, so we load count value - 1 to get the desired number of iterations. */
	val = TVREG_read(coreInstance, SA_AVERAGING_PERIOD);

	SA_ctx->outerLoopCount = (val >> 8) & 0xFF;

	/* Get inner loop count as log2 */
	loopNlog2 = (val & 0xFF);
	loopNlog2 = MIN(loopNlog2, SA_AVERAGING_PERIOD_N_MAX-1);
	loopNlog2 += MIN_SA_AVERAGING_PERIOD_LOG2; /*add an offset since SA_AVERAGING_PERIOD_N_2=0 */
	/* DC offset compensation means running each spectral fragment twice and averaging,
	 * therefore, half the inner loop count. */
	loopNlog2 -= (SA_ctx->dcOffsetCtx.enabled) ? 1 : 0;
	/* The SCP job queue needs a minimum of 2 jobs, therefore restrict the inner loop to 2. */
	loopNlog2 = MAX(loopNlog2, 1);

	SA_ctx->innerLoopCount = (1<<loopNlog2);	/* convert averaging period value to count */
	/* convert loop counter to scaling constants (place shift value in top 6 bits of the register) */
	*SA_ctx->MCP_ptrs.averagingPeriod_innerLoop_log2 = loopNlog2 << 18;
	*SA_ctx->MCP_ptrs.averagingPeriod_innerLoopCount = SA_ctx->innerLoopCount - 1;
	*SA_ctx->MCP_ptrs.averagingPeriod_innerLoop      = SA_ctx->innerLoopCount - 1;
	*SA_ctx->MCP_ptrs.averagingPeriod_outerLoopCount = SA_ctx->outerLoopCount - 1;
	*SA_ctx->MCP_ptrs.averagingPeriod_outerLoop_mult = 0x7FFFFF / SA_ctx->outerLoopCount;

	/* Record settings from SA_IF_GAIN_OVERRIDE */
#ifndef SCP_BYPASS_MODE
	val = TVREG_read(coreInstance, SA_IF_GAIN_OVERRIDE);
	SA_ctx->overrideIFGain = (val >> SA_REG_OVERRIDE_IF_GAIN_BITSHIFT) & 0x1;

	if (SA_ctx->overrideIFGain)
	{
		/* AGC is disabled, read the fixed gain value. */
		SA_ctx->frontEndGain = ((val >> SA_REG_IF_GAIN_BITSHIF) & 0xFFFF);
	}
#else
	SA_ctx->overrideIFGain = true;
#endif

	/* Set capture length and FFT length in MCP data */
	*SA_ctx->MCP_ptrs.SCPcaptureLen = SA_ctx->SCPcaptureLen;

	/* Load window function to match our FFT size */
	_SA_loadWindowFunc(SA_ctx);

	return 0;
}
