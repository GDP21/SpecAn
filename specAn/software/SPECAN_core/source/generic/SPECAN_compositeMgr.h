/*!****************************************************************************
 @File          SPECAN_compositeMgr.h

 @Title         Spectrum analyser spectral compositing manager

 @Date          6/2/2013

 @Copyright     Copyright (C) Imagination Technologies Limited 2013

 @Description   Definitions and declarations shared amongst core files, but not
 	 	 	 	made public.

 ******************************************************************************/
#ifndef SPECAN_COMPOSITE_MGR_H
#define SPECAN_COMPOSITE_MGR_H

#include "uccrt.h"
#include "SPECAN_core.h"


#define SPECAN_WORD_SIZE (sizeof(SPECAN_DB_T)<<3)

#define MAX_DB_T	((1<<(SPECAN_WORD_SIZE-1))-1)
#define MIN_DB_T	(-(1<<(SPECAN_WORD_SIZE-1)))

/* Compositing manager context */
typedef struct
{
	SPECAN_DB_T *pSpectralBuffer;	/* The spectral buffer. */
	
	SPECAN_DB_T *pAddr;				/* Current spectral buffer address. */
	
	unsigned szSpectralBuffer;		/* Spectral Buffer size in bytes. */

	unsigned fftLen;				/* FFT length (the size of input spectral fragments). */
	
	unsigned fragmentLength;		/* Number of bins in a processed fragment. */
	
	unsigned totalBins;				/* Total number of points in the spectral buffer. */
	
	int32_t	oldOverlapBinVal;		/* Target value to offset the new fragment against. */
	
	bool isFirstFragment;			/* Boolean indicates first fragment. */

} SPECAN_COMPOSITE_MGR_T;

/* Structure to hold peak information. */
typedef struct
{
	SPECAN_DB_T	val;				/* peak value. */
	unsigned    i;					/* peak index. */
} SPECAN_PEAK_T;

typedef enum
{
	SPECAN_COMPOSITE_MGR_RTN_FAIL=-10,	/* Error */
	SPECAN_COMPOSITE_MGR_RTN_OK=0,		/* Fragment added successfully. */
	SPECAN_COMPOSITE_MGR_RTN_COMPLETE	/* Spectrum complete. */
} SPECAN_COMPOSITE_MGR_RTN_E;

/* Initialisation function. */
void SPECAN_compositeMgr_init(
		SPECAN_COMPOSITE_MGR_T *pCtx, /* Context pointer. */
		SPECAN_DB_T *pSpectralBuff,   /* Pointer to a buffer for use as the composited spectrum. */
		unsigned len				  /* Total length of the buffer in bytes. */
	);

/* Configure the composite manager. */
SPECAN_COMPOSITE_MGR_RTN_E SPECAN_compositeMgr_config(
		SPECAN_COMPOSITE_MGR_T *pCtx,   /* Context pointer */
		unsigned scanRangeHz,			/* Scan range in Hertz. */
		unsigned tuneStepHz,			/* Tune step in Hertz. */
		unsigned sampleRate,			/* Sample rate in Hertz with sampleRate_fracBits fractional bits. */
		unsigned sampleRate_fracBits,	/* Number of fractional bits in sampleRate argument. */
		unsigned fftLen 				/* FFT length (sets input buffer length for pFragment). */
	);
	
/* Add a spectral fragment from the MCP PSD buffer to the final spectrum vector.
 * pFragment points to a buffer of PSD values in Q8.23 format. */
SPECAN_COMPOSITE_MGR_RTN_E SPECAN_compositeMgr_addFragment(
		SPECAN_COMPOSITE_MGR_T *pCtx,
		uint32_t *pFragment
	);

/* Return the size of the spectral buffer in words. */
unsigned SPECAN_compositeMgr_getSpectrumSize(SPECAN_COMPOSITE_MGR_T *pCtx);

/* Find the nPeaks largest peaks from the completed spectrum. */
void SPECAN_compositeMgr_findPeaks(
		SPECAN_COMPOSITE_MGR_T *pCtx,	/* Context pointer */
		SPECAN_PEAK_T *pPeaks, 			/* Pointer to array of peak structs to be filled. */
		unsigned nPeaks, 				/* Number of peaks to find. */
		unsigned peakWidthBins			/* Peak width in bins. */
	);

#endif /* SPECAN_COMPOSITE_MGR_H */
