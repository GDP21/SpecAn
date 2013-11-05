/*!****************************************************************************
 @File          SPECAN_dcOffsetHandling.h

 @Title         Spectrum analyser DC offset handling module

 @Date          1/2/2013

 @Copyright     Copyright (C) Imagination Technologies Limited 2013

 @Description   Definitions and declarations shared amongst core files, but not
 	 	 	 	made public.

 ******************************************************************************/
#ifndef SPECAN_DC_OFFSET_HANDLING_H
#define SPECAN_DC_OFFSET_HANDLING_H

#include "uccrt.h"

/* DC Tuning offset in gridded frequency positions */
#define SPECAN_DC_OFFSET_GRID_POSITIONS (1)

/* Each band is processed in two halfs to avoid DC problems */
typedef enum
{
	SPECAN_BAND_FRAGMENT_PRIMARY,
	SPECAN_BAND_FRAGMENT_SECONDARY
} SPECAN_BAND_FRAGMENT_ID;

typedef enum
{
	SPECAN_DC_COMP_RTN_NOT_COMPLETE=0,
	SPECAN_DC_COMP_RTN_COMPLETE
} SPECAN_DC_COMP_RTN_E;

typedef struct
{
    bool 	 enabled;						/* flag to indicate DC offset compensation is enabled */
    unsigned compensationFreq;  			/* Frequency added or subtracted in each spectral fragment measurement to eliminate DC bias. */
    unsigned mixerCompensation; 			/* Compensation applied to the mixer in Q.25 */
    unsigned offsetBin;						/* Offset to DC bin, non zero when currentCentreFreq and currentTuneFreq differ enough. */
    unsigned fftSize;						/* FFT size */
    SPECAN_BAND_FRAGMENT_ID bandFragmentID;	/* For DC compensation, two passes of each band are required. */
    uint32_t *pTmpBuf;						/* temporary workspace buffer for MCP data. */
    unsigned tmpBufSize;
} SPECAN_DC_OFFSET_T;

/*
 * Initialise the DC offset context.
 * pWorkspace is a pointer to buffer large enough for the maximum fft size. */
void SPECAN_dcOffsetComp_init(SPECAN_DC_OFFSET_T *pDcOffset, uint32_t *pWorkspace, unsigned);

/* Set up DC compensation context for operation. */
void SPECAN_dcOffsetComp_configure(SPECAN_DC_OFFSET_T *pCtx,
		bool isEnabled,					/* flag to enable. */
		unsigned tunerIncr,				/* tuner increment in Hz. */
		unsigned ADCclkRate,			/* Front-end ADC sampling rate */
		unsigned fftSize,				/* FFT size */
		unsigned sampleRate,			/* Effective sampling rate (Q-format dependent on sampleRate_fracBits). */
		unsigned sampleRate_fracBits	/* Fractional bit count for sampleRate.*/
		);

/* Process a spectral fragment in pBuf to apply DC compensation.
 *
 * This function should be called twice, once for each spectral fragment.
 * The second call will write the resulting compensated buffer to pOut.
 * Fragment ID is managed internally using the bandFragmentID field of the context.
 *
 * Returns:
 *  SPECAN_DC_COMP_RTN_NOT_COMPLETE after processing the first spectral fragment.
 *  SPECAN_DC_COMP_RTN_COMPLETE if DC compensation not enabled or on completing the second fragment processing,  */
SPECAN_DC_COMP_RTN_E SPECAN_dcOffsetComp_proc(SPECAN_DC_OFFSET_T *pCtx, GRAM_SXT_T *pBuf, uint32_t *pOut);


#endif // SPECAN_DC_OFFSET_HANDLING_H
