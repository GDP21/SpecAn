/*
** FILE NAME:   $RCSfile: si2153_tuner.c,v $
**
** TITLE:       SiLabs DTV tuner driver
**
** AUTHOR:      Imagination Technologies
**
** DESCRIPTION: Implementation of a SiLabs DTV tuner driver
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

	/*  Include the interface to the SiLabs supplied code. */
#include "si2153.h"

#include "si2153_tuner.h"
#include "si2153_port.h"


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


#define	IF_FREQ			(6000000)	/* IF out of tuner. */
#define	SYNTH_GRID		(31250)
#define	UPDATE_MARGIN	(SYNTH_GRID * 2)

#define DEFAULT_RF_FREQ	(794000000UL)

#define RSSI_CORRECTION_FACTOR  (-6)    /* Measurements show the reported RSSI is 6dB higher than the input power presented */

/* Static Functions for Tuner API structure */
static TDEV_RETURN_T SI2153_Initialise(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T SI2153_Reset(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T SI2153_Tune(TDEV_T *pTuner, unsigned freq, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T SI2153_SetAGC(TDEV_T *pTuner, TDEV_AGCISR_HELPER_T * pControl, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T SI2153_Enable(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T SI2153_Disable(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T SI2153_Configure(TDEV_T *pTuner, UCC_STANDARD_T standard, unsigned bandwidthHz, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T SI2153_Shutdown(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T SI2153_powerSave(TDEV_T *pTuner, TDEV_RF_PWRSAV_T powerSaveState, unsigned control, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);

/* dummy functions for Tuner API structure */
static TDEV_RETURN_T SI2153_initAGC(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T SI2153_readRFPower(TDEV_T *pTuner, TDEV_RSSI_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T SI2153_setIFAGCTimeConstant(TDEV_T *pTuner, int timeConstuS, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);

TDEV_CONFIG_T SI2153Tuner = {
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
    SI2153_Initialise,				/* function */
    SI2153_Reset,					/* function */
    SI2153_Configure,				/* function */
    SI2153_Tune,					/* function */
    SI2153_readRFPower,				/* dummy function */
    SI2153_Enable,					/* function */
    SI2153_Disable,					/* function */
    SI2153_powerSave,				/* function */
    SI2153_setIFAGCTimeConstant,	/* dummy function */
    SI2153_SetAGC,					/* function */
    SI2153_initAGC,					/* dummy function */
    SI2153_Shutdown					/* function */
};



/*
** FUNCTION:    HandleSetFrequency
**
** DESCRIPTION: Handle the tuning to a frequency.
**
** RETURNS:     success/failure
**
*/
static TDEV_RETURN_T HandleSetFrequency(TDEV_T *pTuner, unsigned frequency_Hz)
{
	SI2153_WORKSPACE_T *wrkspc = pTuner->workSpace;
	Si2153_CmdReplyObj reply;

	if(wrkspc->demodStandard == UCC_STANDARD_ATV)
	{
		if ( Si2153_ATVTune(&(wrkspc->l1_context),
				frequency_Hz,
				Si2153_ATV_VIDEO_MODE_PROP_VIDEO_SYS_I,
				Si2153_ATV_VIDEO_MODE_PROP_TRANS_TERRESTRIAL,
				Si2153_ATV_VIDEO_MODE_PROP_COLOR_PAL_NTSC,
				(SI2153Tuner.spectrumInverted ? Si2153_ATV_VIDEO_MODE_PROP_INVERT_SPECTRUM_INVERTED : Si2153_ATV_VIDEO_MODE_PROP_INVERT_SPECTRUM_NORMAL),
				Si2153_ATV_AFC_RANGE_PROP_RANGE_KHZ_2000_KHZ,
				&reply))
				return (TDEV_FAILURE);
		else
				return (TDEV_SUCCESS);
	}
	else
	{
		if ( Si2153_DTVTune(&(wrkspc->l1_context),
				frequency_Hz,
				wrkspc->bw,
				wrkspc->modulation,
				(SI2153Tuner.spectrumInverted ? Si2153_DTV_MODE_PROP_INVERT_SPECTRUM_INVERTED : Si2153_DTV_MODE_PROP_INVERT_SPECTRUM_NORMAL),
				&reply) )
				return (TDEV_FAILURE);
		else
				return (TDEV_SUCCESS);
	}

	return(TDEV_FAILURE);
}

/*
** FUNCTION:    PollForRSSI
**
** DESCRIPTION: Polls the tuner to read the RSSI
**
** RETURNS:     The RSSI read from the tuner
**
*/
static int PollForRSSI(TDEV_T *pTuner)
{
	SI2153_WORKSPACE_T *wrkspc = pTuner->workSpace;
	Si2153_CmdReplyObj reply;
	int rssi = -1; /* Negative value to indicate an error */

	if(Si2153_L1_TUNER_STATUS(&(wrkspc->l1_context),
								0 /* Si2153_waitForCTS */,
								1 /* Si2153_waitForResponse */,
								Si2153_DTV_STATUS_CMD_INTACK_OK,
								&reply) == NO_Si2153_ERROR)
	{
        rssi = reply.tuner_status.rssi + RSSI_CORRECTION_FACTOR; /* rssi from tuner in dBm, 8bit two‘s complement */
		rssi &= 0xFF;	/* Mask off to ensure the values is actually positive */
	}

	return rssi;
}



static int RSSI_Log = -2;
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
	SI2153_WORKSPACE_T *wrkspc = KRN_taskParameter(NULL);
	Si2153_CmdReplyObj cmdReply;

	Si2153_Configure(&(wrkspc->l1_context), &cmdReply);

	if(wrkspc->demodStandard == UCC_STANDARD_ATV)
	{
		Si2153_ATVConfig(&(wrkspc->l1_context));
	}
	else
	{
		Si2153_CommonConfig(&(wrkspc->l1_context));
		Si2153_DTVConfig(&(wrkspc->l1_context));
	}

    for(;;)
    {
		int rssi = 0;
		TDEV_RETURN_T	messageSuccess =  TDEV_SUCCESS;
            /* Wait for a message */
		TUNER_MESSAGE_T *msg = (TUNER_MESSAGE_T *)KRN_getMbox(&wrkspc->taskMbox, KRN_INFWAIT);

        switch(msg->messageType)
        {
            case(SET_FREQ_MESSAGE):
                messageSuccess = HandleSetFrequency(msg->pTuner, msg->messageFreq);
                break;
            case(POLL_RSSI):
            	RSSI_Log = rssi = PollForRSSI(msg->pTuner);
            	break;
            case(POWER_UP_MESSAGE):
//            	if (0 != Si2153_PowerUpWithPatch(&(wrkspc->l1_context)))
//            		messageSuccess = TDEV_FAILURE;
                break;
            case(POWER_DOWN_MESSAGE):
//            	if (MxL_OK != MxL_Stand_By(&wrkspc->TunerConfigStruct))
//            		messageSuccess = TDEV_FAILURE;
                break;
            default:
            	messageSuccess = TDEV_FAILURE;
                break;
        }

        /* Call appropriate completion function */
        if (msg->messageType != POLL_RSSI)
			msg->CompletionFunc(msg->pTuner, messageSuccess, msg->CompletionParamater);
		else
			msg->rssiCompletionFunc(msg->pTuner, rssi, msg->CompletionParamater);

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
static void taskInit(SI2153_WORKSPACE_T *wrkspc)
{
	KRN_initPool(&wrkspc->messagePool, wrkspc->messagePoolDesc, MSG_POOL_SIZE, sizeof(TUNER_MESSAGE_T));
	wrkspc->poolEmptyCount = 0;

	KRN_initMbox(&wrkspc->taskMbox);

	KRN_startTask(taskMain, &wrkspc->task_tcb, wrkspc->task_stack, TASK_STACKSIZE, wrkspc->task_prio, wrkspc, "Tuner Task");

	return;
}


/*
** FUNCTION:    SI2153_Tune
**
** DESCRIPTION: Send the set frequency request off to the tuner control task.
**
** RETURNS:     Success
**
*/
static volatile unsigned si2153LastFreq = 0;
static TDEV_RETURN_T SI2153_Tune(TDEV_T *pTuner, unsigned freq, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	SI2153_WORKSPACE_T *wrkspc = pTuner->workSpace;

	/* Debug - store the last tuned frequency */
	si2153LastFreq = freq;

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
** FUNCTION:    SI2153_Initialise
**
** DESCRIPTION: Initialises the Tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T SI2153_Initialise(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	SI2153_WORKSPACE_T *wrkspc = pTuner->workSpace;

	assert(SI2153_TUNER_DRIVER_WORKSPACE_SIZE >= sizeof(SI2153_WORKSPACE_T));

	/* Zero the context before we start */
	memset(wrkspc, 0, sizeof(SI2153_WORKSPACE_T));

	wrkspc->signalBandwidth = 8000000;
	wrkspc->demodStandard = UCC_STANDARD_DVBT;
        wrkspc->task_prio= D_SI2153_DTV_TASK_PRIO;

	if (!Si_setupPort(((SI2153_CONFIG_T *)(pTuner->tunerConfigExtension))->portNumber))
	{
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);

		return(TDEV_FAILURE);
	}

	Si2153_L1_API_Init(&(wrkspc->l1_context), &(wrkspc->l0_context), ((SI2153_CONFIG_T *)(pTuner->tunerConfigExtension))->address);
	Si2153_Init(&(wrkspc->l1_context));

    taskInit(wrkspc);

	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    SI2153_Reset
**
** DESCRIPTION: Resets the Tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T SI2153_Reset(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    SI2153_Shutdown()
**
** DESCRIPTION: Shutdown the tuner control
*/
static TDEV_RETURN_T SI2153_Shutdown(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	SI2153_WORKSPACE_T *wrkspc = pTuner->workSpace;

	/* Shutdown all we can and record failures along the way, but don't return early. */
	TDEV_RETURN_T retVal = TDEV_SUCCESS;

	KRN_removeTask(&wrkspc->task_tcb);

	if (!Si_shutdownPort())
	{
		retVal = TDEV_FAILURE;
	}

	pCompletionFunc(pTuner, retVal, completionParameter);
	return(retVal);
}

/*
** FUNCTION:    SI2153_SetAGC
**
** DESCRIPTION: Send the gain request off to the tuner control task.
**
** RETURNS:     Success
**
*/
static volatile int ForceAgc = 0;
static volatile int LogAgc = 0;
static TDEV_RETURN_T SI2153_SetAGC(TDEV_T *pTuner, TDEV_AGCISR_HELPER_T *pControl, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
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
** FUNCTION:    SI2153_Configure()
**
** DESCRIPTION: Log the standard and signal bandwidth.
*/
static TDEV_RETURN_T SI2153_Configure(TDEV_T *pTuner, UCC_STANDARD_T standard, unsigned bandwidthHz, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	SI2153_WORKSPACE_T *wrkspc = pTuner->workSpace;
	unsigned bandwidthMHz;
	TDEV_RETURN_T retVal = TDEV_SUCCESS;

        /* Save bandwidth ready for when it is needed to configure RF. */
    wrkspc->signalBandwidth = bandwidthHz;

    	/* Save the standard we are demodulating as we can treat some of them differently */
	wrkspc->demodStandard = standard;

		/* Workout which are valid and report. */
	switch(standard)
	{
		case UCC_STANDARD_ATSC:
			wrkspc->modulation = Si2153_DTV_MODE_PROP_MODULATION_ATSC;
			break;
		case UCC_STANDARD_DVBT:
		case UCC_STANDARD_DVBT2:
		case UCC_STANDARD_DVBH:
			wrkspc->modulation = Si2153_DTV_MODE_PROP_MODULATION_DVBT;
			break;
		case UCC_STANDARD_ISDBT_1SEG:
		case UCC_STANDARD_ISDBT_3SEG:
		case UCC_STANDARD_ISDBT_13SEG:
			wrkspc->modulation = Si2153_DTV_MODE_PROP_MODULATION_ISDBT;
			break;
		case UCC_STANDARD_DVBC:
			wrkspc->modulation = Si2153_DTV_MODE_PROP_MODULATION_DVBC;
			break;
		case UCC_STANDARD_J83B:
			wrkspc->modulation = Si2153_DTV_MODE_PROP_MODULATION_QAM_US;
			break;
		case UCC_STANDARD_ISDBC:
			wrkspc->modulation = Si2153_DTV_MODE_PROP_MODULATION_ISDBC;
			break;
		case UCC_STANDARD_GB206:
			wrkspc->modulation = Si2153_DTV_MODE_PROP_MODULATION_DTMB;
			break;
		case UCC_STANDARD_ATV:
			/* UCC_STANDARD_NOT_SIGNALLED is considered a supported standard by all drivers; this is used by
			the spectrum analyser core. */
		case UCC_STANDARD_NOT_SIGNALLED:
			/* Default to DVB-T */
			wrkspc->modulation = Si2153_DTV_MODE_PROP_MODULATION_DVBT;
			break;

		// ?? Si2153_DTV_MODE_PROP_MODULATION_DTMB

			/* The following standards are not supported, so signal an error. */
		case UCC_STANDARD_DAB:
		case UCC_STANDARD_FM:
		default:
			retVal = TDEV_FAILURE;
			break;
	}

	/* BW rounding (up) for standards (>10kHz) under the threshold */
	bandwidthHz+=990000;
	bandwidthMHz = bandwidthHz / 1000000;

	/* Record the signal bandwidth in a Si2153 format */
	switch(bandwidthMHz)
	{
		case 6:
			wrkspc->bw = Si2153_DTV_MODE_PROP_BW_BW_6MHZ;
			break;
		case 7:
			wrkspc->bw = Si2153_DTV_MODE_PROP_BW_BW_7MHZ;
			break;
		case 8:
			wrkspc->bw = Si2153_DTV_MODE_PROP_BW_BW_8MHZ;
			break;

		default:
			retVal = TDEV_FAILURE;
			break;
	}

	pCompletionFunc(pTuner, retVal, completionParameter);
    return(retVal);
}


/*
** FUNCTION:    SI2153_Enable
**
** DESCRIPTION: Enable the tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T SI2153_Enable(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	SI2153_WORKSPACE_T *wrkspc = pTuner->workSpace;

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
** FUNCTION:    SI2153_Disable
**
** DESCRIPTION: Disable tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T SI2153_Disable(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	SI2153_WORKSPACE_T *wrkspc = pTuner->workSpace;

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
** FUNCTION:    SI2153_readRFPower()
**
** DESCRIPTION: This tuner does not have an RF power sense - dummy function
*/
static TDEV_RETURN_T SI2153_readRFPower(TDEV_T *pTuner, TDEV_RSSI_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	SI2153_WORKSPACE_T *wrkspc = pTuner->workSpace;

	/* To send a message to Tuner Task */
	TUNER_MESSAGE_T *pollRSSIMsg = (TUNER_MESSAGE_T *) KRN_takePool(&wrkspc->messagePool, KRN_NOWAIT);

	if (pollRSSIMsg == NULL)
	{
		wrkspc->poolEmptyCount++;
		/* If we can't pass on message signal failure */
		pCompletionFunc(pTuner, -1 /* Error */, completionParameter);
		return(TDEV_FAILURE);
	}

	pollRSSIMsg->rssiCompletionFunc = pCompletionFunc;
	pollRSSIMsg->CompletionParamater = completionParameter;
	pollRSSIMsg->messageType = POLL_RSSI;
	pollRSSIMsg->pTuner = pTuner;

	KRN_putMbox(&wrkspc->taskMbox, pollRSSIMsg);

    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    SI2153_powerSave()
**
** DESCRIPTION: This tuner does not have a power save mode, power it down - dummy function
*/
static TDEV_RETURN_T SI2153_powerSave(TDEV_T *pTuner, TDEV_RF_PWRSAV_T powerSaveState, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
    (void)(muxID);			/* Remove compile warnings for unused arguments. */
    (void)(powerSaveState);	/* Remove compile warnings for unused arguments. */

	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
	return(TDEV_SUCCESS);
}


/*
** FUNCTION:    SI2153_setIFAGCTimeConstant()
**
** DESCRIPTION: The IF AGC is under software control! - dummy function
*/
static TDEV_RETURN_T SI2153_setIFAGCTimeConstant(TDEV_T *pTuner, int timeConstuS, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
    (void)timeConstuS;	/* Remove compile warnings for unused arguments. */

	pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
    return TDEV_FAILURE;
}


/*
** FUNCTION:    SI2153_initAGC()
**
** DESCRIPTION: The IF AGC is under software control! - dummy function
*/
static TDEV_RETURN_T SI2153_initAGC(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
    (void)muxID;		/* Remove compile warnings for unused arguments. */
	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}

