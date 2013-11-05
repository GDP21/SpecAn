/*
** FILE NAME:   $RCSfile: mxl_dtv_tuner.c,v $
**
** TITLE:       MaxLinear DTV tuner driver
**
** AUTHOR:      Imagination Technologies
**
** DESCRIPTION: Implementation of a MaxLinear DTV tuner driver
**
** NOTICE:      Copyright (C) Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#ifdef METAG
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif
#include <MeOS.h>
#include <assert.h>
#include <string.h>

#include "uccrt.h"

#include "mxl_dtv_tuner.h"
#include "mxl_port.h"

#ifdef MXL5007T
#include "MxL5007_API.h"
#else
#ifdef MXL201RF
#include "MxL201RF_API.h"
#else
#error "Not a supported tuner."
#endif
#endif


/* Define valid values of TRUE and FALSE that can be assigned to BOOLs */
#ifndef TRUE
#define TRUE    (1)
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

/* Define NULL pointer value */
#ifndef NULL
#define NULL    ((void *)0)
#endif

#ifndef IF_FREQ
#define	IF_FREQ			MxL_IF_44_MHZ	/* IF out of tuner. */
#endif

#define	SYNTH_GRID		(15625)	/* 1/64th MHz */
#define	UPDATE_MARGIN	(SYNTH_GRID * 2)

#define DEFAULT_RF_FREQ	(650000000UL)

#ifdef MXL5007T
#define XTAL_FREQ MxL_XTAL_24_MHZ
#endif

#ifdef MXL201RF
#define XTAL_FREQ MxL_XTAL_48_MHZ
#endif

typedef enum
{
	SET_FREQ_MESSAGE = 0,
	POWER_UP_MESSAGE,
	POWER_DOWN_MESSAGE,
	NUMBER_OF_MSG		/* The number of different messages we can have */
} MSG_T;

typedef struct
{
	KRN_POOLLINK;
	MSG_T messageType;		/* What do we need to do with this message */
	TDEV_T *pTuner;			/* Tuner instance */
	unsigned messageFreq;		/* Frequency to tune to if it is a SET_FREQ_MESSAGE */
	TDEV_COMPLETION_FUNC_T CompletionFunc;
	void *CompletionParamater;
} TUNER_MESSAGE_T;

	/* Size the pool to be more than the possible number of outstanding messages
	** incase we try and issue a new message just as we are completing one.
	** This assumes that only one of each message can occur at any time. */
#define MSG_POOL_SIZE	(NUMBER_OF_MSG + 1)
#define TASK_STACKSIZE (1024)

typedef struct
{
	/* Which port we should use to communicate with the tuner */
	int	portNumber;

	/* Bandwidth we are configured to operate at, in Hz.*/
	int signalBandwidth;
	UCC_STANDARD_T demodStandard;

	KRN_TASK_T task_tcb;
	unsigned int task_stack[TASK_STACKSIZE];
	TUNER_MESSAGE_T messagePoolDesc[MSG_POOL_SIZE];
	KRN_POOL_T	 messagePool;
	KRN_MAILBOX_T taskMbox;
	int poolEmptyCount;

#ifdef MXL5007T
	MxL5007_TunerConfigS TunerConfigStruct;
#endif

#ifdef MXL201RF
	MxL201RF_TunerConfigS TunerConfigStruct;
#endif
} MXL_WORKSPACE_T;

/* Static Functions for Tuner API structure */
static TDEV_RETURN_T MXL_DTV_Initialise(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MXL_DTV_Reset(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MXL_DTV_Tune(TDEV_T *pTuner, unsigned freq, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MXL_DTV_SetAGC(TDEV_T *pTuner, TDEV_AGCISR_HELPER_T * pControl, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MXL_DTV_Enable(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MXL_DTV_Disable(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MXL_DTV_Configure(TDEV_T *pTuner, UCC_STANDARD_T standard, unsigned bandwidthHz, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MXL_DTV_Shutdown(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MXL_DTV_powerSave(TDEV_T *pTuner, TDEV_RF_PWRSAV_T powerSaveState, unsigned control, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);

/* dummy functions for Tuner API structure */
static TDEV_RETURN_T MXL_DTV_initAGC(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MXL_DTV_readRFPower(TDEV_T *pTuner, TDEV_RSSI_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MXL_DTV_setIFAGCTimeConstant(TDEV_T *pTuner, int timeConstuS, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);

TDEV_CONFIG_T MXL_DTVTuner = {
    TDEV_VERSION_I32,				/* Version number - check that tuner and API are built with the same header */
    FALSE,							/* The IF interface is Real */
    FALSE,							/* Set true if the IF spectrum is inverted */
    IF_FREQ,						/* The final IF frequency of the tuner (Hz) */
    SYNTH_GRID,						/* The stepsize of the tuner PLL (Hz)      */
    UPDATE_MARGIN,					/* The tuner PLL update margin (Hz)        */
    1000,							/* Settling time in uS from power down to power up */
    0,								/* Settling time in uS from power save level 1 to power up */
    0,								/* Settling time in uS from power save level 2 to power up */
    1000,							/* Settling time in uS from tune to stable output  */
    MXL_DTV_Initialise,				/* function */
    MXL_DTV_Reset,					/* function */
    MXL_DTV_Configure,				/* function */
    MXL_DTV_Tune,					/* function */
    MXL_DTV_readRFPower,			/* dummy function */
    MXL_DTV_Enable,					/* function */
    MXL_DTV_Disable,				/* function */
    MXL_DTV_powerSave,				/* function */
    MXL_DTV_setIFAGCTimeConstant,	/* dummy function */
    MXL_DTV_SetAGC,					/* function */
    MXL_DTV_initAGC,				/* dummy function */
    MXL_DTV_Shutdown				/* function */
};



/*
** FUNCTION:    HandleSetFrequency
**
** DESCRIPTION: Handle the tuning to a frequency.
**
** RETURNS:     void
**
*/
static TDEV_RETURN_T HandleSetFrequency(TDEV_T *pTuner, unsigned frequency_Hz)
{
	MXL_WORKSPACE_T *wrkspc = pTuner->workSpace;
	int actualSignalBandwidth;

	switch(wrkspc->demodStandard)
	{
		case UCC_STANDARD_DVBT:
		case UCC_STANDARD_DVBT2:
		case UCC_STANDARD_DVBH:
		case UCC_STANDARD_GB206:
			wrkspc->TunerConfigStruct.Mode = MxL_MODE_DVBT;
			break;

		case UCC_STANDARD_ATSC:
			wrkspc->TunerConfigStruct.Mode = MxL_MODE_ATSC;
			break;

		case UCC_STANDARD_ISDBT_1SEG:
		case UCC_STANDARD_ISDBT_3SEG:
		case UCC_STANDARD_ISDBT_13SEG:
#ifdef MXL5007T
			wrkspc->TunerConfigStruct.Mode = MxL_MODE_ISDBT;
#else
			assert(0);
			return(TDEV_FAILURE);
#endif
			break;

		/* Treat the cable standards the same */
		case UCC_STANDARD_DVBC:
		case UCC_STANDARD_J83B:
		case UCC_STANDARD_ISDBC:
#ifdef MXL5007T
			wrkspc->TunerConfigStruct.Mode = MxL_MODE_CABLE;
#else
#ifdef MXL201RF
			wrkspc->TunerConfigStruct.Mode = MxL_MODE_CAB_STD;
			//wrkspc->TunerConfigStruct.Mode = MxL_MODE_CAB_OPT1;	/* Cable option 1 */
#else
			assert(0);	/* Not supported */
			return(TDEV_FAILURE);
#endif
#endif
			break;

		/* Analog TV not really handled by the MaxLinear tuner, but try DVBT anyway... */
	    case UCC_STANDARD_ATV:
	    	wrkspc->TunerConfigStruct.Mode = MxL_MODE_DVBT;
			break;

			/* The following standards are not supported, so ignore. */
		case UCC_STANDARD_NOT_SIGNALLED:
		case UCC_STANDARD_DAB:
		case UCC_STANDARD_FM:
		default:
			assert(0);
			return(TDEV_FAILURE);
			break;
	}

	if (MxL_OK != MxL_Tuner_Init(&wrkspc->TunerConfigStruct))
		return(TDEV_FAILURE);

	/* Convert to MHz and round up to get at least the requested bandwidth */
	/* ceiling(a/b) = (a+b-1)/b */
	actualSignalBandwidth = ( wrkspc->signalBandwidth + MHz - 1 ) / MHz;
	if (MxL_OK != MxL_Tuner_RFTune(&wrkspc->TunerConfigStruct, frequency_Hz, actualSignalBandwidth))
		return(TDEV_FAILURE);

	return(TDEV_SUCCESS);
}


/*
** FUNCTION:    taskMain
**
** DESCRIPTION: Simple dispatcher task to send Frequency requests to tuner via SCBM device driver.
**
** RETURNS:     void
**
*/
static void taskMain(void)
{
	MXL_WORKSPACE_T *wrkspc = KRN_taskParameter(NULL);
		/* Init the configuration of the tuner */
	wrkspc->TunerConfigStruct.I2C_Addr = MxL_I2C_ADDR_96;
	wrkspc->TunerConfigStruct.Mode = MxL_MODE_DVBT;
	wrkspc->TunerConfigStruct.Xtal_Freq = XTAL_FREQ;
	wrkspc->TunerConfigStruct.IF_Freq = IF_FREQ;
	wrkspc->TunerConfigStruct.IF_Spectrum = MxL_NORMAL_IF;
	wrkspc->TunerConfigStruct.ClkOut_Setting = MxL_CLKOUT_DISABLE;
    wrkspc->TunerConfigStruct.ClkOut_Amp = MxL_CLKOUT_AMP_0;
	wrkspc->TunerConfigStruct.BW_MHz = MxL_BW_8MHz;
	wrkspc->TunerConfigStruct.RF_Freq_Hz = DEFAULT_RF_FREQ;

#ifdef MXL201RF
	wrkspc->TunerConfigStruct.Xtal_Cap = MxL_XTAL_CAP_14_PF;
#endif

#ifdef MXL5007T
	wrkspc->TunerConfigStruct.IF_Diff_Out_Level = 0;
#endif

		/* The init to defaults. Note the init function soft resets the tuner before setting it up. */
	(void)MxL_Tuner_Init(&wrkspc->TunerConfigStruct);

    for(;;)
    {
		TDEV_RETURN_T	messageSuccess =  TDEV_SUCCESS;
            /* Wait for a message */
		TUNER_MESSAGE_T *msg = (TUNER_MESSAGE_T *)KRN_getMbox(&wrkspc->taskMbox, KRN_INFWAIT);
        switch(msg->messageType)
        {
            case(SET_FREQ_MESSAGE):
                messageSuccess = HandleSetFrequency(msg->pTuner, msg->messageFreq);
                break;
            case(POWER_UP_MESSAGE):
            	if (MxL_OK != MxL_Wake_Up(&wrkspc->TunerConfigStruct))
            		messageSuccess = TDEV_FAILURE;
                break;
            case(POWER_DOWN_MESSAGE):
            	if (MxL_OK != MxL_Stand_By(&wrkspc->TunerConfigStruct))
            		messageSuccess = TDEV_FAILURE;
                break;
            default:
            	messageSuccess = TDEV_FAILURE;
                break;
        }
		msg->CompletionFunc(msg->pTuner, messageSuccess, msg->CompletionParamater);

        	/* Finished with message so release back to pool */
        KRN_returnPool(msg);
    }
}

/*
** FUNCTION:    taskInit
**
** DESCRIPTION: Initialise and start task.
**
** RETURNS:     void
**
*/
static void taskInit(MXL_WORKSPACE_T *wrkspc)
{
	KRN_initPool(&wrkspc->messagePool, wrkspc->messagePoolDesc, MSG_POOL_SIZE, sizeof(TUNER_MESSAGE_T));
	wrkspc->poolEmptyCount = 0;

	KRN_initMbox(&wrkspc->taskMbox);

    KRN_startTask(taskMain, &wrkspc->task_tcb, wrkspc->task_stack, TASK_STACKSIZE, 1, wrkspc, "Tuner Task");

    return;
}


/*
** FUNCTION:    MXL_DTV_Tune
**
** DESCRIPTION: Send the set frequency request off to the tuner control task.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MXL_DTV_Tune(TDEV_T *pTuner, unsigned freq, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	MXL_WORKSPACE_T *wrkspc = pTuner->workSpace;

	/* To send a message to Tuner Task */
	TUNER_MESSAGE_T *setFreqMsg = (TUNER_MESSAGE_T *) KRN_takePool(&wrkspc->messagePool, KRN_NOWAIT);

	if (setFreqMsg == NULL)
	{
		wrkspc->poolEmptyCount++;
		/* If we can't pass on message signal failure */
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
		return(TDEV_FAILURE);
	}

	setFreqMsg->messageFreq = freq;
	setFreqMsg->CompletionFunc = pCompletionFunc;
	setFreqMsg->CompletionParamater = completionParameter;
	setFreqMsg->messageType = SET_FREQ_MESSAGE;
	setFreqMsg->pTuner = pTuner;

	KRN_putMbox(&wrkspc->taskMbox, setFreqMsg);

    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    MXL_DTV_Initialise
**
** DESCRIPTION: Initialises the Tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MXL_DTV_Initialise(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	MXL_DTV_CONFIG_T *mxl_DTV_Config = (MXL_DTV_CONFIG_T *) pTuner->tunerConfigExtension;
	MXL_WORKSPACE_T *wrkspc = pTuner->workSpace;

	assert(MXL_DTV_TUNER_DRIVER_WORKSPACE_SIZE >= sizeof(MXL_WORKSPACE_T));

	/* Zero the context before we start */
	memset(wrkspc, 0, sizeof(MXL_WORKSPACE_T));

	wrkspc->signalBandwidth = 7000000;
	wrkspc->demodStandard = UCC_STANDARD_DVBT;
	wrkspc->portNumber = mxl_DTV_Config->portNumber;

	if (!MxL_setupPort(wrkspc->portNumber))
	{
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);

		return(TDEV_FAILURE);
	}

    taskInit(wrkspc);

	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    MXL_DTV_Reset
**
** DESCRIPTION: Resets the Tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MXL_DTV_Reset(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    MXL_DTV_Shutdown()
**
** DESCRIPTION: Shutdown the tuner control
*/
static TDEV_RETURN_T MXL_DTV_Shutdown(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	MXL_WORKSPACE_T *wrkspc = pTuner->workSpace;

		/* Shutdown all we can and record failures along the way, but don't return early. */
	TDEV_RETURN_T retVal = TDEV_SUCCESS;

	KRN_removeTask(&wrkspc->task_tcb);

	if (!MxL_shutdownPort())
	{
		retVal = TDEV_FAILURE;
	}

	pCompletionFunc(pTuner, retVal, completionParameter);
	return(retVal);
}

/*
** FUNCTION:    MXL_DTV_SetAGC
**
** DESCRIPTION: Send the gain request off to the tuner control task.
**
** RETURNS:     Success
**
*/
static volatile int ForceAgc = 0;
static volatile int LogAgc = 0;
static TDEV_RETURN_T MXL_DTV_SetAGC(TDEV_T *pTuner, TDEV_AGCISR_HELPER_T *pControl, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	LogAgc = pControl->IFgainValue;

	if (ForceAgc != 0)
		pControl->IFgainValue = ForceAgc;

	/* Send gain value out to PDM DAC */
    SCP_setExtGainControl1(pTuner->scp, (pControl->IFgainValue) >> 4); /* 16 to 12 bit conversion */
    SCP_setExtOffsetControl1(pTuner->scp, (pControl->IFgainValue) >> 4); /* 16 to 12 bit conversion */

	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    MXL_DTV_Configure()
**
** DESCRIPTION: Log the standard and signal bandwidth.
*/
static TDEV_RETURN_T MXL_DTV_Configure(TDEV_T *pTuner, UCC_STANDARD_T standard, unsigned bandwidthHz, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	MXL_WORKSPACE_T *wrkspc = pTuner->workSpace;
	TDEV_RETURN_T retVal;

        /* Save bandwidth ready for when it is needed to configure RF. */
    wrkspc->signalBandwidth = bandwidthHz;

    	/* Save the standard we are demodulating as we can treat some of them differently */
	wrkspc->demodStandard = standard;

		/* Workout which are valid and report. */
	switch(standard)
	{
		case UCC_STANDARD_ATSC:
		case UCC_STANDARD_DVBT:
		case UCC_STANDARD_DVBT2:
		case UCC_STANDARD_DVBH:
		case UCC_STANDARD_ISDBT_1SEG:
		case UCC_STANDARD_ISDBT_3SEG:
		case UCC_STANDARD_ISDBT_13SEG:
		case UCC_STANDARD_DVBC:
		case UCC_STANDARD_J83B:
		case UCC_STANDARD_ISDBC:
		case UCC_STANDARD_GB206:
		/* UCC_STANDARD_NOT_SIGNALLED is considered a supported standard by all drivers; this is used by
		the spectrum analyser core. */
		case UCC_STANDARD_NOT_SIGNALLED:
			retVal = TDEV_SUCCESS;
			break;

			/* The following standards are not supported, so signal an error. */
		case UCC_STANDARD_DAB:
		case UCC_STANDARD_FM:
		default:
			retVal = TDEV_FAILURE;
			break;
	}

	pCompletionFunc(pTuner, retVal, completionParameter);
    return(retVal);
}


/*
** FUNCTION:    MXL_DTV_Enable
**
** DESCRIPTION: Enable the tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MXL_DTV_Enable(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	MXL_WORKSPACE_T *wrkspc = pTuner->workSpace;

	/* To send a message to Tuner Task */
	TUNER_MESSAGE_T *powerMsg = (TUNER_MESSAGE_T *) KRN_takePool(&wrkspc->messagePool, KRN_NOWAIT);

	if (powerMsg == NULL)
	{
		wrkspc->poolEmptyCount++;
		/* If we can't pass on message signal failure */
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
		return(TDEV_FAILURE);
	}

	powerMsg->CompletionFunc = pCompletionFunc;
	powerMsg->CompletionParamater = completionParameter;
	powerMsg->messageType = POWER_UP_MESSAGE;
	powerMsg->pTuner = pTuner;

	KRN_putMbox(&wrkspc->taskMbox, powerMsg);

    (void)muxID;		/* Remove compile warning for unused arguments. */

    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    MXL_DTV_Disable
**
** DESCRIPTION: Disable tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MXL_DTV_Disable(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	MXL_WORKSPACE_T *wrkspc = pTuner->workSpace;

	/* To send a message to Tuner Task */
	TUNER_MESSAGE_T *powerMsg = (TUNER_MESSAGE_T *) KRN_takePool(&wrkspc->messagePool, KRN_NOWAIT);

	if (powerMsg == NULL)
	{
		wrkspc->poolEmptyCount++;
		/* If we can't pass on message signal failure */
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
		return(TDEV_FAILURE);
	}

	powerMsg->CompletionFunc = pCompletionFunc;
	powerMsg->CompletionParamater = completionParameter;
	powerMsg->messageType = POWER_DOWN_MESSAGE;
	powerMsg->pTuner = pTuner;

	KRN_putMbox(&wrkspc->taskMbox, powerMsg);

	(void)muxID;		/* Remove compile warning for unused arguments. */

    return(TDEV_SUCCESS);
}


/*
** FUNCTION:    MXL_DTV_readRFPower()
**
** DESCRIPTION: This tuner does not have an RF power sense - dummy function
*/
static TDEV_RETURN_T MXL_DTV_readRFPower(TDEV_T *pTuner, TDEV_RSSI_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	pCompletionFunc(pTuner, 0, completionParameter);
    return TDEV_FAILURE;
}

/*
** FUNCTION:    MXL_DTV_powerSave()
**
** DESCRIPTION: This tuner does not have a power save mode, power it down - dummy function
*/
static TDEV_RETURN_T MXL_DTV_powerSave(TDEV_T *pTuner, TDEV_RF_PWRSAV_T powerSaveState, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
    (void)(muxID);			/* Remove compile warnings for unused arguments. */
    (void)(powerSaveState);	/* Remove compile warnings for unused arguments. */

	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
	return(TDEV_SUCCESS);
}


/*
** FUNCTION:    MXL_DTV_setIFAGCTimeConstant()
**
** DESCRIPTION: The IF AGC is under software control! - dummy function
*/
static TDEV_RETURN_T MXL_DTV_setIFAGCTimeConstant(TDEV_T *pTuner, int timeConstuS, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
    (void)timeConstuS;	/* Remove compile warnings for unused arguments. */

	pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
    return TDEV_FAILURE;
}


/*
** FUNCTION:    MXL_DTV_initAGC()
**
** DESCRIPTION: The IF AGC is under software control! - dummy function
*/
static TDEV_RETURN_T MXL_DTV_initAGC(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
    (void)muxID;		/* Remove compile warnings for unused arguments. */
	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}

