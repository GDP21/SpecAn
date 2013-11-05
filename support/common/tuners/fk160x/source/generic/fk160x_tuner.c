/*
** FILE NAME:   $RCSfile: fk160x_tuner.c,v $
**
** TITLE:       NuTune DTV tuner driver
**
** AUTHOR:      Imagination Technologies
**
** DESCRIPTION: Implementation of a NuTune DTV tuner driver
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

	/*  Include the interface to the NuTune supplied code. */
#include "fk160x.h"

#include "fk160x_tuner.h"
#include "fk160x_port.h"


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


#define	SYNTH_GRID		(1000)
#define	UPDATE_MARGIN	(SYNTH_GRID * 2)

#define DEFAULT_RF_FREQ	(650000000UL)


typedef enum
{
	SET_FREQ_MESSAGE = 0,
	POWER_UP_MESSAGE,
	POWER_DOWN_MESSAGE,
	CONFIGURE_MESSAGE,
	NUMBER_OF_MSG		/* The number of different messages we can have */
} MSG_T;

typedef struct
{
	KRN_POOLLINK;
	MSG_T messageType;		/* What do we need to do with this message */
	TDEV_T *pTuner;			/* Tuner instance */
	unsigned messageFreq;	/* Frequency to tune to if it is a SET_FREQ_MESSAGE */
	TDEV_COMPLETION_FUNC_T CompletionFunc;
	void *CompletionParamater;
} TUNER_MESSAGE_T;

	/* Size the pool to be more than the possible number of outstanding messages
	** incase we try and issue a new message just as we are completing one.
	** This assumes that only one of each message can occur at any time. */
#define MSG_POOL_SIZE	(NUMBER_OF_MSG + 1)
#define TASK_STACKSIZE	(1024)

typedef struct
{
	/* Bandwidth we are configured to operate at, in Hz.*/
	unsigned signalBandwidth;
	UCC_STANDARD_T demodStandard;

	/* State for the supplied TDA18273 driver */
	UInt32 TunerUnit;
	tmbslFrontEndDependency_t sSrvTunerFunc;

	KRN_TASK_T task_tcb;
	unsigned int task_stack[TASK_STACKSIZE];
	TUNER_MESSAGE_T messagePoolDesc[MSG_POOL_SIZE];
	KRN_POOL_T	 messagePool;
	KRN_MAILBOX_T taskMbox;
	int poolEmptyCount;

} FK160X_WORKSPACE_T;

/* Static Functions for Tuner API structure */
static TDEV_RETURN_T FK160x_Initialise(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T FK160x_Reset(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T FK160x_Tune(TDEV_T *pTuner, unsigned freq, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T FK160x_SetAGC(TDEV_T *pTuner, TDEV_AGCISR_HELPER_T * pControl, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T FK160x_Enable(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T FK160x_Disable(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T FK160x_Configure(TDEV_T *pTuner, UCC_STANDARD_T standard, unsigned bandwidthHz, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T FK160x_Shutdown(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T FK160x_powerSave(TDEV_T *pTuner, TDEV_RF_PWRSAV_T powerSaveState, unsigned control, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);

/* dummy functions for Tuner API structure */
static TDEV_RETURN_T FK160x_initAGC(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T FK160x_readRFPower(TDEV_T *pTuner, TDEV_RSSI_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T FK160x_setIFAGCTimeConstant(TDEV_T *pTuner, int timeConstuS, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);

TDEV_CONFIG_T FK160XTunerDVBC = {
    TDEV_VERSION_I32,				/* Version number - check that tuner and API are built with the same header */
    FALSE,							/* The IF interface is Real */
    FALSE,							/* Set true if the IF spectrum is inverted */
    QAM_8MHz_IF,					/* The final IF frequency of the tuner (Hz) */
    SYNTH_GRID,						/* The stepsize of the tuner PLL (Hz)      */
    UPDATE_MARGIN,					/* The tuner PLL update margin (Hz)        */
    1000,							/* Settling time in uS from power down to power up */
    0,								/* Settling time in uS from power save level 1 to power up */
    0,								/* Settling time in uS from power save level 2 to power up */
    1000,							/* Settling time in uS from tune to stable output  */
    FK160x_Initialise,				/* function */
    FK160x_Reset,					/* function */
    FK160x_Configure,				/* function */
    FK160x_Tune,					/* function */
    FK160x_readRFPower,				/* dummy function */
    FK160x_Enable,					/* function */
    FK160x_Disable,					/* function */
    FK160x_powerSave,				/* function */
    FK160x_setIFAGCTimeConstant,	/* dummy function */
    FK160x_SetAGC,					/* function */
    FK160x_initAGC,					/* dummy function */
    FK160x_Shutdown					/* function */
};

TDEV_CONFIG_T FK160XTunerJ83B = {
    TDEV_VERSION_I32,				/* Version number - check that tuner and API are built with the same header */
    FALSE,							/* The IF interface is Real */
    FALSE,							/* Set true if the IF spectrum is inverted */
    QAM_6MHz_IF,					/* The final IF frequency of the tuner (Hz) */
    SYNTH_GRID,						/* The stepsize of the tuner PLL (Hz)      */
    UPDATE_MARGIN,					/* The tuner PLL update margin (Hz)        */
    1000,							/* Settling time in uS from power down to power up */
    0,								/* Settling time in uS from power save level 1 to power up */
    0,								/* Settling time in uS from power save level 2 to power up */
    1000,							/* Settling time in uS from tune to stable output  */
    FK160x_Initialise,				/* function */
    FK160x_Reset,					/* function */
    FK160x_Configure,				/* function */
    FK160x_Tune,					/* function */
    FK160x_readRFPower,				/* dummy function */
    FK160x_Enable,					/* function */
    FK160x_Disable,					/* function */
    FK160x_powerSave,				/* function */
    FK160x_setIFAGCTimeConstant,	/* dummy function */
    FK160x_SetAGC,					/* function */
    FK160x_initAGC,					/* dummy function */
    FK160x_Shutdown					/* function */
};

TDEV_CONFIG_T FK160XTunerDVBT = {
    TDEV_VERSION_I32,				/* Version number - check that tuner and API are built with the same header */
    FALSE,							/* The IF interface is Real */
    FALSE,							/* Set true if the IF spectrum is inverted */
    DVBT_8MHz_IF,					/* The final IF frequency of the tuner (Hz) */
    SYNTH_GRID,						/* The stepsize of the tuner PLL (Hz)      */
    UPDATE_MARGIN,					/* The tuner PLL update margin (Hz)        */
    1000,							/* Settling time in uS from power down to power up */
    0,								/* Settling time in uS from power save level 1 to power up */
    0,								/* Settling time in uS from power save level 2 to power up */
    1000,							/* Settling time in uS from tune to stable output  */
    FK160x_Initialise,				/* function */
    FK160x_Reset,					/* function */
    FK160x_Configure,				/* function */
    FK160x_Tune,					/* function */
    FK160x_readRFPower,				/* dummy function */
    FK160x_Enable,					/* function */
    FK160x_Disable,					/* function */
    FK160x_powerSave,				/* function */
    FK160x_setIFAGCTimeConstant,	/* dummy function */
    FK160x_SetAGC,					/* function */
    FK160x_initAGC,					/* dummy function */
    FK160x_Shutdown					/* function */
};

TDEV_CONFIG_T FK160XTunerATSC = {
    TDEV_VERSION_I32,				/* Version number - check that tuner and API are built with the same header */
    FALSE,							/* The IF interface is Real */
    FALSE,							/* Set true if the IF spectrum is inverted */
    ATSC_6MHz_IF,					/* The final IF frequency of the tuner (Hz) */
    SYNTH_GRID,						/* The stepsize of the tuner PLL (Hz)      */
    UPDATE_MARGIN,					/* The tuner PLL update margin (Hz)        */
    1000,							/* Settling time in uS from power down to power up */
    0,								/* Settling time in uS from power save level 1 to power up */
    0,								/* Settling time in uS from power save level 2 to power up */
    1000,							/* Settling time in uS from tune to stable output  */
    FK160x_Initialise,				/* function */
    FK160x_Reset,					/* function */
    FK160x_Configure,				/* function */
    FK160x_Tune,					/* function */
    FK160x_readRFPower,				/* dummy function */
    FK160x_Enable,					/* function */
    FK160x_Disable,					/* function */
    FK160x_powerSave,				/* function */
    FK160x_setIFAGCTimeConstant,	/* dummy function */
    FK160x_SetAGC,					/* function */
    FK160x_initAGC,					/* dummy function */
    FK160x_Shutdown					/* function */
};

TDEV_CONFIG_T FK160XTunerISDBT = {
    TDEV_VERSION_I32,				/* Version number - check that tuner and API are built with the same header */
    FALSE,							/* The IF interface is Real */
    FALSE,							/* Set true if the IF spectrum is inverted */
    ISDBT_6MHz_IF,					/* The final IF frequency of the tuner (Hz) */
    SYNTH_GRID,						/* The stepsize of the tuner PLL (Hz)      */
    UPDATE_MARGIN,					/* The tuner PLL update margin (Hz)        */
    1000,							/* Settling time in uS from power down to power up */
    0,								/* Settling time in uS from power save level 1 to power up */
    0,								/* Settling time in uS from power save level 2 to power up */
    1000,							/* Settling time in uS from tune to stable output  */
    FK160x_Initialise,				/* function */
    FK160x_Reset,					/* function */
    FK160x_Configure,				/* function */
    FK160x_Tune,					/* function */
    FK160x_readRFPower,				/* dummy function */
    FK160x_Enable,					/* function */
    FK160x_Disable,					/* function */
    FK160x_powerSave,				/* function */
    FK160x_setIFAGCTimeConstant,	/* dummy function */
    FK160x_SetAGC,					/* function */
    FK160x_initAGC,					/* dummy function */
    FK160x_Shutdown					/* function */
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
	FK160X_WORKSPACE_T *wrkspc = pTuner->workSpace;
	tmErrorCode_t err = TM_OK;

	err = tmbslTDA18273_SetRF(wrkspc->TunerUnit, (UInt32)frequency_Hz);

#ifndef NDEBUG
	if (err == TM_OK)
	{
		UInt32 uIF = 0;
		err = tmbslTDA18273_GetIF(wrkspc->TunerUnit, &uIF);
	switch(wrkspc->demodStandard)
	{
		case UCC_STANDARD_ATSC:
			assert(uIF == (UInt32)FK160XTunerATSC.frequencyIF);
			break;
		case UCC_STANDARD_DVBT:
		case UCC_STANDARD_DVBT2:
		case UCC_STANDARD_DVBH:
			assert(uIF == (UInt32)FK160XTunerDVBT.frequencyIF);
			break;
		case UCC_STANDARD_ISDBT_1SEG:
		case UCC_STANDARD_ISDBT_3SEG:
		case UCC_STANDARD_ISDBT_13SEG:
			assert(uIF == (UInt32)FK160XTunerISDBT.frequencyIF);
			break;
		case UCC_STANDARD_DVBC:
		case UCC_STANDARD_ISDBC:
			assert(uIF == (UInt32)FK160XTunerDVBC.frequencyIF);
			break;
		case UCC_STANDARD_J83B:
			assert(uIF == (UInt32)FK160XTunerJ83B.frequencyIF);
			break;
			/* The following standards are not supported, so signal an error. */
		case UCC_STANDARD_NOT_SIGNALLED:
		case UCC_STANDARD_DAB:
		case UCC_STANDARD_FM:
		default:
			err = 1;
			break;
		}
	}
#endif

	if (err == TM_OK)
		return(TDEV_SUCCESS);
	else
		return(TDEV_FAILURE);
}


/*
** FUNCTION:    HandleConfigure
**
** DESCRIPTION: Handle configuring the bandwidth and standard.
**
** RETURNS:     void
**
*/
static TDEV_RETURN_T HandleConfigure(TDEV_T *pTuner)
{
	FK160X_WORKSPACE_T *wrkspc = pTuner->workSpace;
	tmErrorCode_t err = TM_OK;
	tmTDA18273StandardMode_t TDA18273StdMode = tmTDA18273_DVBT_8MHz;

		/* Workout which are valid. */
	switch(wrkspc->demodStandard)
	{
		case UCC_STANDARD_ATSC:
			TDA18273StdMode = tmTDA18273_ATSC_6MHz;
			break;
		case UCC_STANDARD_DVBT:
		case UCC_STANDARD_DVBT2:
		case UCC_STANDARD_DVBH:
			switch(wrkspc->signalBandwidth)
			{
				case 6000000:
					TDA18273StdMode = tmTDA18273_DVBT_6MHz;
					break;
				case 7000000:
					TDA18273StdMode = tmTDA18273_DVBT_7MHz;
					break;
				case 8000000:
					TDA18273StdMode = tmTDA18273_DVBT_8MHz;
					break;
				case 10000000:
					TDA18273StdMode = tmTDA18273_DVBT_10MHz;
					break;

				default:
					err = 1;
					break;
			}
			break;
		case UCC_STANDARD_ISDBT_1SEG:
		case UCC_STANDARD_ISDBT_3SEG:
		case UCC_STANDARD_ISDBT_13SEG:
			TDA18273StdMode = tmTDA18273_ISDBT_6MHz;
			break;
		case UCC_STANDARD_DVBC:
			TDA18273StdMode = tmTDA18273_QAM_8MHz;
			break;
		case UCC_STANDARD_J83B:
			TDA18273StdMode = tmTDA18273_QAM_6MHz;
			break;
		case UCC_STANDARD_ISDBC:
			TDA18273StdMode = tmTDA18273_QAM_6MHz;
			break;

			/* The following standards are not supported, so signal an error. */
		case UCC_STANDARD_NOT_SIGNALLED:
		case UCC_STANDARD_DAB:
		case UCC_STANDARD_FM:
		default:
			err = 1;
			break;
	}

		/* If a valid combination pass onto tuner */
	if (err == TM_OK)
		err = tmbslTDA18273_SetStandardMode(wrkspc->TunerUnit, TDA18273StdMode);

	if (err == TM_OK)
		return(TDEV_SUCCESS);
	else
		return(TDEV_FAILURE);
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
	FK160X_WORKSPACE_T *wrkspc = KRN_taskParameter(NULL);

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
            case(CONFIGURE_MESSAGE):
                messageSuccess = HandleConfigure(msg->pTuner);
                break;
            case(POWER_UP_MESSAGE):
				if (tmbslTDA18273_SetPowerState(wrkspc->TunerUnit, tmPowerOn))
					messageSuccess = TDEV_FAILURE;
                break;
            case(POWER_DOWN_MESSAGE):
				if (tmbslTDA18273_SetPowerState(wrkspc->TunerUnit, tmPowerStandby))
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
static void taskInit(FK160X_WORKSPACE_T *wrkspc)
{
	KRN_initPool(&wrkspc->messagePool, wrkspc->messagePoolDesc, MSG_POOL_SIZE, sizeof(TUNER_MESSAGE_T));
	wrkspc->poolEmptyCount = 0;

	KRN_initMbox(&wrkspc->taskMbox);

    KRN_startTask(taskMain, &wrkspc->task_tcb, wrkspc->task_stack, TASK_STACKSIZE, 1, wrkspc, "Tuner Task");

    return;
}


/*
** FUNCTION:    FK160x_Tune
**
** DESCRIPTION: Send the set frequency request off to the tuner control task.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T FK160x_Tune(TDEV_T *pTuner, unsigned freq, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	FK160X_WORKSPACE_T *wrkspc = pTuner->workSpace;

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
** FUNCTION:    FK160x_Initialise
**
** DESCRIPTION: Initialises the Tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T FK160x_Initialise(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	FK160X_WORKSPACE_T *wrkspc = pTuner->workSpace;
	tmErrorCode_t err = TM_OK;

	assert(FK160X_TUNER_DRIVER_WORKSPACE_SIZE >= sizeof(FK160X_WORKSPACE_T));

	/* Zero the context before we start */
	memset(wrkspc, 0, sizeof(FK160X_WORKSPACE_T));

	wrkspc->signalBandwidth = 7000000;
	wrkspc->demodStandard = UCC_STANDARD_DVBT;

	if (!FK_setupPort(((FK160X_CONFIG_T *)(pTuner->tunerConfigExtension))->portNumber))
	{
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);

		return(TDEV_FAILURE);
	}

	wrkspc->TunerUnit = 0;
	wrkspc->sSrvTunerFunc.sIo.Write = FK_I2CWrite;
	wrkspc->sSrvTunerFunc.sIo.Read = FK_I2CRead;
	wrkspc->sSrvTunerFunc.sTime.Get = Null;
	wrkspc->sSrvTunerFunc.sTime.Wait = FK_Wait;
	wrkspc->sSrvTunerFunc.sDebug.Print = FK_Print;
	wrkspc->sSrvTunerFunc.sMutex.Open = Null;
	wrkspc->sSrvTunerFunc.sMutex.Close = Null;
	wrkspc->sSrvTunerFunc.sMutex.Acquire = Null;
	wrkspc->sSrvTunerFunc.sMutex.Release = Null;
	wrkspc->sSrvTunerFunc.dwAdditionalDataSize = 0;
	wrkspc->sSrvTunerFunc.pAdditionalData = Null;

	/* Open TDA18273 driver instance */
	err = tmbslTDA18273_Open(wrkspc->TunerUnit, &(wrkspc->sSrvTunerFunc));
	if(err == TM_OK)
	{
		/* TDA18273 Hardware initialization */
		err = tmbslTDA18273_HwInit(wrkspc->TunerUnit);
	}

	if(err == TM_OK)
		err = tmbslTDA18273_CheckHWVersion(wrkspc->TunerUnit);

	if (err != TM_OK)
	{
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);

		return(TDEV_FAILURE);
	}

    taskInit(wrkspc);

	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    FK160x_Reset
**
** DESCRIPTION: Resets the Tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T FK160x_Reset(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    FK160x_Shutdown()
**
** DESCRIPTION: Shutdown the tuner control
*/
static TDEV_RETURN_T FK160x_Shutdown(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	FK160X_WORKSPACE_T *wrkspc = pTuner->workSpace;

		/* Shutdown all we can and record failures along the way, but don't return early. */
	TDEV_RETURN_T retVal = TDEV_SUCCESS;

	/* Close TDA18273 driver instance */
	if (tmbslTDA18273_Close(wrkspc->TunerUnit))
		retVal = TDEV_FAILURE;

	KRN_removeTask(&wrkspc->task_tcb);

	if (!FK_shutdownPort())
	{
		retVal = TDEV_FAILURE;
	}

	pCompletionFunc(pTuner, retVal, completionParameter);
	return(retVal);
}

/*
** FUNCTION:    FK160x_SetAGC
**
** DESCRIPTION: Send the gain request off to the tuner control task.
**
** RETURNS:     Success
**
*/
static volatile int ForceAgc = 0;
static volatile int LogAgc = 0;
static TDEV_RETURN_T FK160x_SetAGC(TDEV_T *pTuner, TDEV_AGCISR_HELPER_T *pControl, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
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
** FUNCTION:    FK160x_Configure()
**
** DESCRIPTION: Log the standard and signal bandwidth.
*/
static TDEV_RETURN_T FK160x_Configure(TDEV_T *pTuner, UCC_STANDARD_T standard, unsigned bandwidthHz, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	FK160X_WORKSPACE_T *wrkspc = pTuner->workSpace;
	/* To send a message to Tuner Task */
	TUNER_MESSAGE_T *configureMsg = (TUNER_MESSAGE_T *) KRN_takePool(&wrkspc->messagePool, KRN_NOWAIT);

        /* Save bandwidth ready for when it is needed to configure RF. */
    wrkspc->signalBandwidth = bandwidthHz;

    	/* Save the standard we are demodulating as we can treat some of them differently */
	wrkspc->demodStandard = standard;

	if (configureMsg == NULL)
	{
		wrkspc->poolEmptyCount++;
		/* If we can't pass on message signal failure */
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
		return(TDEV_FAILURE);
	}

	configureMsg->CompletionFunc = pCompletionFunc;
	configureMsg->CompletionParamater = completionParameter;
	configureMsg->messageType = CONFIGURE_MESSAGE;
	configureMsg->pTuner = pTuner;

	KRN_putMbox(&wrkspc->taskMbox, configureMsg);

    return(TDEV_SUCCESS);

}


/*
** FUNCTION:    FK160x_Enable
**
** DESCRIPTION: Enable the tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T FK160x_Enable(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	FK160X_WORKSPACE_T *wrkspc = pTuner->workSpace;

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
** FUNCTION:    FK160x_Disable
**
** DESCRIPTION: Disable tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T FK160x_Disable(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	FK160X_WORKSPACE_T *wrkspc = pTuner->workSpace;

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
** FUNCTION:    FK160x_readRFPower()
**
** DESCRIPTION: This tuner does not have an RF power sense - dummy function
*/
static TDEV_RETURN_T FK160x_readRFPower(TDEV_T *pTuner, TDEV_RSSI_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
//
//tmErrorCode_t err;
//UInt8 uValue = 0;
//Decimal PowerLevel;
//err = tmbslTDA18273_GetPowerLevel(wrkspc->TunerUnit, &uValue);
//PowerLevel = uValue * 0.5;	/* in dBuV */
	pCompletionFunc(pTuner, 0, completionParameter);
    return TDEV_FAILURE;
}

/*
** FUNCTION:    FK160x_powerSave()
**
** DESCRIPTION: This tuner does not have a power save mode, power it down - dummy function
*/
static TDEV_RETURN_T FK160x_powerSave(TDEV_T *pTuner, TDEV_RF_PWRSAV_T powerSaveState, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
    (void)(muxID);			/* Remove compile warnings for unused arguments. */
    (void)(powerSaveState);	/* Remove compile warnings for unused arguments. */

	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
	return(TDEV_SUCCESS);
}


/*
** FUNCTION:    FK160x_setIFAGCTimeConstant()
**
** DESCRIPTION: The IF AGC is under software control! - dummy function
*/
static TDEV_RETURN_T FK160x_setIFAGCTimeConstant(TDEV_T *pTuner, int timeConstuS, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
    (void)timeConstuS;	/* Remove compile warnings for unused arguments. */

	pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
    return TDEV_FAILURE;
}


/*
** FUNCTION:    FK160x_initAGC()
**
** DESCRIPTION: The IF AGC is under software control! - dummy function
*/
static TDEV_RETURN_T FK160x_initAGC(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
    (void)muxID;		/* Remove compile warnings for unused arguments. */
	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}

