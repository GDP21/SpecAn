/*!****************************************************************************
 @File          SPECAN_init.c

 @Title         Initialisation functions for the Spectrum Analyser core

 @Date          29 Nov 2012

 @Copyright     Copyright (C) Imagination Technologies Limited 2012

 @Description   Initialisation functions

 ******************************************************************************/
#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <assert.h>
#include "SPECAN_private.h"
#include "SPECAN_core_DCPM.h"
#include "SPECAN_core_RCD.h"
#include "scpDriver.h"
#include "SPECAN_mcposDriver_DCP.h"

#ifdef SCP_BYPASS_MODE
#define SYMBOLS_PER_FRAME   (10)
#define SYMBOL_LENGTH       (512)
#else
#define SYMBOL_LENGTH       (SCP_DEFAULT_DISCARD_LEN)
#endif

/* The coefficients below have been generated using the SCP configuration Matlab tool
 * (cvs:/mobileTV/common/Matlab/simulation/PHY_SCP/SCP_configuration.m) with the following settings:
 *
 *
 * scpConfig = [];
 * scpConfig.fsi               = 78.64e6;
 * scpConfig.fso               = {51.2e6, 64e6, 81.92e6};	% each of these
 * scpConfig.sysClk            = 166e6;
 * scpConfig.fc                = 0;
 * scpConfig.bw                = 45e6;
 * scpConfig.maxPbRipple       = 5;
 * scpConfig.minSbAtten        = 10;
 * scpConfig.stopbandEdgeFreq  = 40e6;
 * scpConfig.tradeoff          = 'pb';
 * scpConfig.enablePbFlattening= 1;
 *
 * [ctxScp,scpConfig] = SCP_configuration(scpConfig);
 * [pbRipple,sbAtten] = SCP_performance(scpConfig);
 *
 * */
#define SCP_RESAMPLER_COEFFS {28,  10, -20, -53, -77, -80, -51,  14, 110, 225, 341, 437, 511}	// Set 1
//#define SCP_RESAMPLER_COEFFS	{-57, -62, -55, -36,  -3,  43, 101, 167, 237, 307, 372, 430, 511},	// Set 2


/*--------------------------------------------------------------------------*/

/* Initialise signal processing pipeline */
static void SPECAN_initPipeline(SPECAN_INSTANCE_CTX_T *SA_ctx)
{
    TV_INSTANCE_T *tvInst = SA_ctx->tvInstance;
    SCP_T *scp = tvInst->scp;
	DCP_PIPELINE_T *SCPPipeline = DCP_getImagePipeline(DCP_pipelineId_SCPPipeline);

    /* SCP resampler coefficients, supplied by Adrian 9/9/2011 */
    //int16_t SCPResamplerCoeffs[] = {2, 9, 22, 43, 72, 110, 155, 204, 254, 299, 335, 359, 367};
	int16_t SCPResamplerCoeffs[] = SCP_RESAMPLER_COEFFS;

	/*
	 * Static SCP configuration
	 */
	SCP_configureUse(SCPPipeline,
					DCP_SA_SCPPipeline_SCP,
					&RCD_scpDriver_default_images,
					scp,
					SYMBOL_LENGTH);
	/* Configure SCP resampler coeffients */
	SCP_setResamplerCoeffs(scp, SCPResamplerCoeffs);

	/* Set the gain normalisation controls following the mixer, resampler and FIR filter
	(these are new and not modelled in the UNISIM SCP model).  These are set in conjunction
	with the filter coefficients to maintain best use of the number range through the SCP */
	SCP_setMixerGain(scp, SCP_MIXER_OUT_GAIN_0DB);
	//SCP_setResamplerGain(scp, SCP_RESAMPLE_OUT_GAIN_0DB);
	//SCP_setFIRGain(scp, SCP_FIR_OUT_GAIN_MINUS6DB);
	/* TEMP: for now set 0dB gain for FIR, as the _TDEV_configureScpMixer function changes the above
	settings, setting -6dB for the resampler gain.  Need to sort this out as not following an optimal
	gain plan for the current coefficient sets. */
	SCP_setResamplerGain(scp, SCP_RESAMPLE_OUT_GAIN_MINUS6DB);
	SCP_setFIRGain(scp, SCP_FIR_OUT_GAIN_0DB);

	/* Disable the DRM decimator (note this is on the SCP input; different to the CIC / FIR decimators on
	at the back-end) */
	SCP_setDecimator(scp, 0, 1);
	/* Disable the I/Q correlator */
	SCP_setIQCorrelator(scp, 0);
	/* Disable the notch filter.  This is important to allow us to run at high IF clock rate without
	i/p synchroniser overflow. */
	SCP_setNotchBypass(scp, 1);
	/* Disable ISCR counter, we don't use this */
	SCP_disableIscr(scp);
	/* Disable the impulsive noise filter.  This is important to allow us to run at high IF clock rate without
	i/p synchroniser overflow. */
	SCP_setIMPBypass(scp, 1);
	/* Zero DC offsets */
	SCP_setDCOffsets(scp, 0, 0);
	/* Set unity resample factor prior to running SCP_configure, necessary to prevent SCP i/p synchroniser overflow */
	SCP_setFs(scp, 0x2000000); /* Unity in Q1.25 */

	/* Configure SCP
	 * NOTE: maybe some of this static stuff would be better done from an XML file? */
#ifdef SCP_BYPASS_MODE
	SCP_configure(scp,
			  SCP_SRC_ADC,
			  SCP_DST_TBUS,
			  false,
			  true,
			  false,
			  false,
			  SCP_MODE_BYPASS,
			  SCP_FRAME_A,
			  SCP_PWR_ON,
			  SCP_PWR_ON,
			  false,
			  1,
			  1,
			  false,
			  false);

	SCP_setFrameSize(scp,
			SCP_FRAME_A,
			SYMBOLS_PER_FRAME,
			SYMBOL_LENGTH,
			SYMBOL_LENGTH);

#else
	SCP_configure(scp,            				/* SCP object */
				  SCP_SRC_ADC,
				  SCP_DST_TBUS,                 /* SCP data destination */
				  SCP_SAMPLE_2SCOMP,            /* SCP sample format */
				  SCP_INPUT_IQ,                 /* SCP input mode */
				  SCP_SPECTRUM_NORMAL,			/* SCP spectrum inversion setup */
				  SCP_UPDATE_NOSYNC,            /* SCP update sync */
				  SCP_MODE_NORMAL,              /* SCP operating mode */
				  SCP_FRAME_A,                  /* SCP active frame */
				  SCP_PWR_ON,                   /* ADC power */
				  SCP_PWR_ON,                   /* SCP power */
				  SCP_FRCMODE_NORMAL,           /* SCP lower power operation */
				  /* We initialise with a CIC decimation factor of 4 because on 1st generation Saturn
				  we cannot run at maximum bode rate so this avoids a potential SCP TBUS overflow.  The
				  actual decimation factor will be set later according to the baud rate we are running at. */
				  4,                            /* CIC decimation factor */
				  1,                            /* FIR decimation factor */
				  false,                         /* Don't reset the SCP before configuring (if you do, we seem to get
				   	   	   	   	   	   	   	   problems with input synchroniser overflow). */
				  true);                        /* Clear Local Oscillator phase accumulator after configuring */
#endif

	/* Start DCP devices.  Note this should really be a system-wide function. */
	DCP_startAllDevices();

	/* Start DCP job on SCP.  This job runs indefinitely. From here on the SCP will be waiting on its input queues */
	DCP_startJobEx(SCPPipeline, DCP_SA_SCPPipeline_SCP, 0, 0);

}

/*--------------------------------------------------------------------------*/

bool SPECAN_init(TV_INSTANCE_T *tvInstance)
{
	unsigned nBytes, *pMem = NULL;

	/* Obtain pointer to our own context structure, then initialise this structure */
	SPECAN_INSTANCE_CTX_T *SA_ctx = tvInstance->tvInstanceExtensionData;

	/* Fill in a pointer back to tvInstance, used to reference our allocated SCP etc. */
	SA_ctx->tvInstance = tvInstance;

    /* Allocate memory for control task stack.  This stack goes in core on all architectures. */
	SA_ctx->controlTaskStack = UFW_memAlloc(_SPECAN_CONTROL_TASK_STACK_SIZE * sizeof(unsigned),
	                                                  UFW_MEMORY_TYPE_FAST);
	if (!SA_ctx->controlTaskStack)
	{
		assert(!"Control task stack allocation failed");
		return false;
	}

	/* Allocate memory for front end task stack. */
	SA_ctx->frontEndTaskStack = UFW_memAlloc(_SPECAN_FRONT_END_TASK_STACK_SIZE * sizeof(unsigned),
	                                                  UFW_MEMORY_TYPE_FAST);
	if (!SA_ctx->frontEndTaskStack)
	{
		assert(!"Front end task stack allocation failed");
		return false;
	}


	KRN_initMbox(&SA_ctx->ctrlTaskMbox);

	/* Initialise the message pool object.  No particular need to initialise the pooled items
	as these will all be configured before use. */
	KRN_initPool(&SA_ctx->ctrlMsgPool, SA_ctx->ctrlMsgArray, NUM_CTRL_MSGS_POOLED,
				sizeof(SPECAN_CONTROL_FSM_MESSAGE_T));

	/* Allocate memory for mcpos device */
	SA_ctx->mcpos = UFW_memAlloc(sizeof(MCPOS_DEVICE_T), UFW_MEMORY_TYPE_NORMAL);
	if (!SA_ctx->mcpos)
	{
		assert(!"MCPOS device allocation failed");
		return false;
	}

	KRN_initFlags(&SA_ctx->SA_eventFlags);

	/* Just initialise these to 0, doesn't matter as they will be over-written from the
	API prior to use */
	SA_ctx->isStartOfScan = false;
	SA_ctx->currentTuneFreq = 0;
	SA_ctx->currentCentreFreq = 0;
	SA_ctx->currentBW = 0;
	SA_ctx->scanRange = 0;
	SA_ctx->tuningStep = 0;
	SA_ctx->FFTsize = 0;
	SA_ctx->SCPcaptureLen = 0;
	SA_ctx->RSfactor = 0;
	SA_ctx->q27p4_effectiveSampleRate = 0;
	SA_ctx->CICdecimationFactor  = 0;
	SA_ctx->FIRdecimationFactor = 0;

	pMem = UFW_memAlloc(MAX_FFT_SIZE_SUPPORTED * sizeof(unsigned), UFW_MEMORY_TYPE_NORMAL);
	if (!pMem)
	{
		assert(!"Workspace memory allocation failed.");
		return false;
	}

	/* Initialise the DC compensation module. */
	SPECAN_dcOffsetComp_init(&SA_ctx->dcOffsetCtx, (uint32_t *)pMem, MAX_FFT_SIZE_SUPPORTED);

	/* Allocate the composite spectral buffer memory. */
	nBytes = MAX_TOTAL_SPECTRAL_SIZE_SUPPORTED * sizeof(SPECAN_DB_T);
	pMem = UFW_memAlloc(nBytes, UFW_MEMORY_TYPE_NORMAL);

	if (!pMem)
	{
		assert(!"Workspace memory allocation failed.");
		return false;
	}

	/* Align into GRAM. */
	EDC_alignBuffer(pMem, nBytes, EDC_EXTRAM, 0, &SA_ctx->outputBuffer);

	/* Initialise the compositing module. */
	SPECAN_compositeMgr_init(&SA_ctx->compositeCtx, (SPECAN_DB_T *)SA_ctx->outputBuffer.addr, nBytes);

	/* Initialise signal processing pipeline */
	SPECAN_initPipeline(SA_ctx);

	/* Job ID for use when queueing MCPOS SCP completion jobs via MCPOS_startJob() */
	SA_ctx->mcposSCPjobQID = DCP_getUseParam(DCP_getImagePipeline(DCP_pipelineId_SCPPipeline),
			DCP_SA_SCPPipeline_mcpos, DCP_SPECAN_mcposDriver_scp_mcposJobQ);

	/* Install interrupt handlers */
	SPECAN_installInterruptHandlers(SA_ctx);

#ifndef DISABLE_META_LOGGING
    /* Initialise our logging system */
    SPECAN_initLogging(&SA_ctx->logCtx);
#endif

	return true;
}
