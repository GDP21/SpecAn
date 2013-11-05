/*!****************************************************************************
 @File          SPECAN_private.h

 @Title         Private definitions for Spectrum Analyser core

 @Date          29 Nov 2012

 @Copyright     Copyright (C) Imagination Technologies Limited 2012

 @Description   Definitions and declarations shared amongst core files, but not
 	 	 	 	made public.

 ******************************************************************************/

#ifndef _SPECAN_PRIVATE_H_
#define _SPECAN_PRIVATE_H_

#include <MeOS.h>
#include "SPECAN_core.h"
#include "common_agc.h"
#include "SPECAN_logging.h"
#include "SPECAN_timestamp.h"
#include "SPECAN_dcOffsetHandling.h"
#include "SPECAN_compositeMgr.h"
#include "mcpos.h"

#define nSCP_BYPASS_MODE

#ifndef MAX
#define MAX(A,B)	((A) >= (B) ? (A) : (B))
#endif
#ifndef MIN
#define MIN(A,B)	((A) <= (B) ? (A) : (B))
#endif

/* size in words of main tvcore task stack ("TV Core main task") */
#define _SPECAN_MAINTASK_STACK_SIZE 	(256)
/* size in words of control task stack ("SpecAn Control Task") */
#define _SPECAN_CONTROL_TASK_STACK_SIZE	(512)
/* size in words of front end task stack ("SpecAn Front End Task") */
#define _SPECAN_FRONT_END_TASK_STACK_SIZE (512)

/* Size of MCP GRAM memory space in words */
#define _SPECAN_MCP_GRAM_SIZE 			(0x18000)

/* Trace buffer length */
#define TRACE_BUFFER_LEN_WORDS			(2048)

/* The SCP driver needs a default discard length, this is the length of a discard which will happen in the event
that it is starved of input jobs.  In a correctly working system this should never happen so the length is not
too important, but the driver behaves very strangely if a zero length is supplied */
#define SCP_DEFAULT_DISCARD_LEN         (512)

/* These useIDs can be random */
#define FRAGMENT_COMPLETED_USE_ID		(2)
#define CAPTURE_COMPLETE_MCPOS_USE_ID	(3)

/* Log2 value of SA_AVERAGING_PERIOD_N minimum. Used to offset calculation of log2 from the enum.
 * Make sure this is updated if changing SA_AVERAGING_PERIOD_N start point. */
#define MIN_SA_AVERAGING_PERIOD_LOG2	(1)


/* Default tuner frequency and bandwidth, these are unimportant as we will re-tune with
user-supplied settings before starting scan. */
#define SPECAN_DEFAULT_TUNER_FREQUENCY	(1000000000)	/* 1GHz */
// For DVB-S #define SPECAN_DEFAULT_TUNER_BW			(70000000)		/* 70MHz */
#define SPECAN_DEFAULT_TUNER_BW			(8000000)		/* 8MHz (for DVB-T tuner) */

/* Maximum supported FFT size */
#define MAX_FFT_SIZE_SUPPORTED			(8192)
/* Minimum supported FFT size */
#define MIN_FFT_SIZE_SUPPORTED			(64)

/* Total output spectrum size in bins. */
#define MAX_TOTAL_SPECTRAL_SIZE_SUPPORTED (8192)

/* Size of stored window functions: this contains half of a (symmetrical) window function */
#define STORED_WINDOW_SIZE				(2048)

/* Minimum SCP capture length */
#define MIN_SCP_CAPTURE_LEN				(512)

/* This defines the length of time that we run the AGC before measuring a spectral fragment. */
#define NUM_AGC_ISRS					(35)
#define AGC_MUL_UNITY                  	(1<<8)
#define AGC_GAIN_LOOP_SCALING           ((long)(0.33 * AGC_MUL_UNITY))

/* Ennumerate state types */
typedef enum
{
	SPECAN_STATE_INVALID,
	SPECAN_STATE_IDLE,
	SPECAN_STATE_START,
	SPECAN_STATE_TUNE,
	SPECAN_STATE_AGC_UPDATE,
	SPECAN_STATE_PROC_FRAGMENT,
	SPECAN_STATE_PROC_COMPLETE,
	SPECAN_STATE_MAX
} SPECAN_CONTROL_STATE_T;

/* Ennumerate control messages */
typedef enum
{
	SPECAN_RUN_SCAN,
	SPECAN_STOP_SCAN,
	SPECAN_CHANGE_STATE,
	SPECAN_TUNE_EVENT,
	SPECAN_FAIL,
	SPECAN_CONTROL_MSG_MAX
} SPECAN_CONTROL_MSG_T;


/* Poolable message structure passed to the control FSM */
typedef struct {
    KRN_POOLLINK;
    SPECAN_CONTROL_MSG_T   message;
    SPECAN_CONTROL_STATE_T newState;
    unsigned code; /* Message specific code. */
} SPECAN_CONTROL_FSM_MESSAGE_T;

/* Number of control messages pooled.  Only 2 should ever be in use at once
(run and stop) */
#define NUM_CTRL_MSGS_POOLED			(3)

/* Structure holding pointers to items in MCP memory.  Names match the variable names in MCP code.
Pointers are in the LSB-justified GRAM view. */
typedef struct
{
	MCP_GRAM_INT_T *averagingPeriod_innerLoopCount;
	MCP_GRAM_INT_T *averagingPeriod_innerLoop;
	MCP_GRAM_INT_T *averagingPeriod_innerLoop_log2;
	MCP_GRAM_INT_T *averagingPeriod_outerLoopCount;
	MCP_GRAM_INT_T *averagingPeriod_outerLoop_mult;
	EDC_BUFFER_T SCPoutBufferA;
	EDC_BUFFER_T SCPoutBufferB;
	EDC_BUFFER_T SpectralFragmentBuff;
	MCP_GRAM_INT_T *SCPcaptureLen;
	EDC_BUFFER_T windowFunc;
	MCP_GRAM_INT_T *pFFTlen;
	MCP_GRAM_INT_T *pFFTlen_log2;
	MCP_GRAM_INT_T *pFFTshifts;
} SPECAN_MCP_PTR_T;

/* Structure to hold front end AGC context. */
typedef struct {

	bool		  isFirstReading;
    unsigned      agcCounter; 	/* counter */
    AGC_CONTEXT_T agcCtx;       /* The AGC context space. */
} FRONT_END_TASK_CTX_T;


/* Our main context space */
typedef struct
{
    TV_INSTANCE_T *tvInstance; /* Link back to parent structure */
    KRN_TASK_T controlTask; /* MeOS Context for control task */
    KRN_TASK_T frontEndTask; /* MeOS Context for front end task */
    unsigned *controlTaskStack; /* Stack for control task */
    unsigned *frontEndTaskStack; /* Stack for front end task */
    SPECAN_CONTROL_STATE_T ctrlState; /* Current control state */
    KRN_MAILBOX_T ctrlTaskMbox; /* Control task mailbox */
    SPECAN_CONTROL_FSM_MESSAGE_T ctrlMsgArray[NUM_CTRL_MSGS_POOLED]; /* Messgage structures for ctrlMsgPool */
    KRN_POOL_T ctrlMsgPool; /* Pool of control messages */
    MCPOS_DEVICE_T *mcpos; /* MCPOS device object, private to MCPOS */
    MCPOS_USE_T mcposUse; /* MCPOS use case, private to MCPOS */
    DCP_PARAM_ID_T mcposSCPjobQID; /* Job ID for use when queueing MCPOS SCP completion jobs via MCPOS_startJob() */
    KRN_FLAGCLUSTER_T SA_eventFlags; /* Event flags array */
    SPECAN_MCP_PTR_T MCP_ptrs; /* Pointers to variables in MCP space */
    FRONT_END_TASK_CTX_T frontEndCtx;	/* Analogue AGC context */

    bool     isStartOfScan;		/* flag to indicate scan is starting */
    unsigned currentCentreFreq; /* Current centre frequency in Hz. */
    unsigned currentTuneFreq; /* Current tune frequency in Hz. = currentCentreFreq+/-dcOffsetCompensationFreq */
    unsigned finalTuneFreq;	/* the final frequency in the scan in Hz. */
    unsigned currentBW; /* Tuner bandwidth in Hz */
    unsigned scanRange; /* Desired range of frequencies for scan in Hz */
    unsigned tuningStep; /* Step between tune positions in Hz */

    SPECAN_DC_OFFSET_T dcOffsetCtx;	/* DC offset compensation handling context. */

    SPECAN_COMPOSITE_MGR_T compositeCtx; /* Spectral compositing structure. */

    SPECAN_PEAK_T maxPeakArray[SPECAN_MAX_PEAK_COUNT];	/* Array of maximum peaks to be filled on completion. */
    int peakWidthBins;			/* Width of peak for spectral peaks reported. */

    uint32_t FFTsize; /* FFT size */
    unsigned SCPcaptureLen; /* Capture length, may be different from FFT length when FFT length is short */
    SPECAN_WINDOW_T windowFunc;
    unsigned RSfactor; /* Resample factor, 25 fractional bits */
    unsigned q27p4_effectiveSampleRate; /* Effective sample rate in samps/s after resampling and decimation in Q27.4 */
    int CICdecimationFactor; /* Decimation factor of SCP CIC filter */
    int FIRdecimationFactor; /* Decimation factor of SCP FIR filter */

    bool 	overrideIFGain;		/* If true, automatic gain control disabled. */
    long 	frontEndGain;		/* placeholder for last gain value or gain override. */

    unsigned innerLoopCount;	/* Averaging period loop 1 */
    unsigned outerLoopCount;	/* Averaging period loop 2 */

    SPECAN_LOG_CTX logCtx;		/* logging context */

    EDC_BUFFER_T outputBuffer;	/* The output buffer. */

    TIME_STAMP_T timeStamps;

} SPECAN_INSTANCE_CTX_T;


/*
 * Private function declarations
 */
/* Initialise the API register set */
bool _SPECAN_initRegisterAPI(UFW_COREINSTANCE_T *coreInstance, void *parameter);

/* Initialise the spectrum analyser core: run on activation */
bool SPECAN_init(TV_INSTANCE_T *tvInstance);

/* Control task */
void SPECAN_controlTask(void);

/* Queue a control task state change */
void SPECAN_QctrlStateChange(SPECAN_INSTANCE_CTX_T *SA_ctx, SPECAN_CONTROL_STATE_T newState);

/* Front end task */
void SPECAN_frontEndTask(void);

/* Front end AGC init function */
void SPECAN_initAgc(SPECAN_INSTANCE_CTX_T *SA_ctx);

/* Set the AGC back to a known state prior to running */
void SPECAN_resetAGC(SPECAN_INSTANCE_CTX_T *SA_ctx);

/* Set the front end gain to the given value. Return 0 for success, else -1 */
int SPECAN_setFrontEndGain(SPECAN_INSTANCE_CTX_T *SA_ctx, long gain);


/* Post a "run scan" message to control task */
void SPECAN_initiateScan(SPECAN_INSTANCE_CTX_T *SActx);

/* Initialise and load the MCP code; run during activate function only */
bool SPECAN_initMCP(SPECAN_INSTANCE_CTX_T *SA_ctx, unsigned GRAMsizeAllocated);

/* Initialise MCPOS and build jobs; run during activate function only */
void SPECAN_initMCPOS(SPECAN_INSTANCE_CTX_T *SA_ctx);

/* Set-up parameters prior to a scan. Return 0 for success, else -1 */
int SPECAN_setUpForScan(SPECAN_INSTANCE_CTX_T *SA_ctx);

/* Move to next tuning position. Return 0 on success, else -1. */
int SPECAN_updateTuner(SPECAN_INSTANCE_CTX_T *SA_ctx);

/* Return a snapped-to-tuning-grid frequency for f in Hz. */
uint32_t SA_snapToFrequency(SPECAN_INSTANCE_CTX_T *SA_ctx, uint32_t f);

/* Re-tune */
void SA_retune(SPECAN_INSTANCE_CTX_T *SA_ctx, unsigned freq, unsigned bandwidth);

/* Install interrupt handlers */
void SPECAN_installInterruptHandlers(SPECAN_INSTANCE_CTX_T *SA_ctx);

/* Kick-off MCP code to process a spectral fragment */
void SPECAN_processFragment(SPECAN_INSTANCE_CTX_T *SA_ctx);

/* Complete the processing of a spectral fragment on an MSG interrupt. */
void SPECAN_processFragmentCompletion(SPECAN_INSTANCE_CTX_T *SA_ctx);

#endif /* _SPECAN_PRIVATE_H_ */
