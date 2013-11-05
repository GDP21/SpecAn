/*
** FILE NAME:   $RCSfile: max2172_tuner.c,v $
**
** TITLE:       MAX2172 DAB/FM tuner driver
**
** AUTHOR:      Imagination Technologies
**
** DESCRIPTION: Implementation of a Maxim MAX2172 tuner driver
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
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#include <MeOS.h>
#include <assert.h>

#include "uccrt.h"

#include "max2172_tuner.h"
#include "max_port.h"

#include "2172_Driver.h"

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

#ifndef abs
#define abs(x)  (((x) < 0) ? (-(x)) : (x))
#endif


/* Replace with whatever the actual RF requires. */
#define	IF_FREQ			2048000
#define	SYNTH_GRID		375
#define	UPDATE_MARGIN	24000

/* Which port we should use to communicate with the tuner */
static int	portNumber = 0;

/* High or Low side conversion */
static HI_LO_CONVERT_T hiLoConvertFlag = LOW_SIDE_CONVERT;

/* Bandwidth we are configured to operate at, in Hz.
** Initialised to zero as an invalid value.
** This will ensure that a bandwidth has been set before it is used. */
static unsigned signalBandwidth = 0;
static UCC_STANDARD_T demodStandard;

typedef enum
{
	SET_FREQ_MESSAGE = 0,
	NUMBER_OF_MSG		/* The number of different messages we can have */
} MSG_T;

typedef struct
{
	KRN_POOLLINK;
	MSG_T messageType;		/* What do we need to do with this message */
	unsigned messageFreq;		/* Frequency to tune to if it is a SET_FREQ_MESSAGE */
	TDEV16_COMPLETION_FUNC_T CompletionFunc;
	TDEV16_T *tuner;
	void *parameter;
} TUNER_MESSAGE_T;

	/* Size the pool to be more than the possible number of outstanding messages
	** incase we try and issue a new message just as we are completing one.
	** This assumes that only one of each message can occur at any time. */
#define MSG_POOL_SIZE	(NUMBER_OF_MSG + 3)
#define TASK_STACKSIZE (1024)

static KRN_TASK_T task_tcb;
static unsigned int task_stack[TASK_STACKSIZE];
static TUNER_MESSAGE_T messagePoolDesc[MSG_POOL_SIZE];
static KRN_POOL_T	 messagePool;
static KRN_MAILBOX_T taskMbox;
static int poolEmptyCount;


/* Static Functions for Tuner API structure */
static TDEV_RETURN_T MAX2172_Init(TDEV16_T *tuner, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter);
static TDEV_RETURN_T MAX2172_SetFrequency(TDEV16_T *tuner, unsigned freq, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter);
static TDEV_RETURN_T MAX2172_SetAGC(TDEV16_T *tuner, TDEV16_AGCISR_HELPER_T *pControl, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter);
static TDEV_RETURN_T MAX2172_Enable(TDEV16_T *tuner, unsigned muxID, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter);
static TDEV_RETURN_T MAX2172_Disable(TDEV16_T *tuner, unsigned muxID, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter);
static TDEV_RETURN_T MAX2172_Configure(TDEV16_T *tuner, UCC_STANDARD_T standard, unsigned bandwidthHz, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter);
static TDEV_RETURN_T MAX2172_Shutdown(TDEV16_T *tuner, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter);
static TDEV_RETURN_T MAX2172_powerSave(TDEV16_T *tuner, TDEV_RF_PWRSAV_T powerSaveState, unsigned control, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter);

/* dummy functions for Tuner API structure */
static TDEV_RETURN_T MAX2172_Reset(TDEV16_T *tuner, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter);
static TDEV_RETURN_T MAX2172_initAGC(TDEV16_T *tuner, unsigned muxID, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter);
static TDEV_RETURN_T MAX2172_readRFPower(TDEV16_T *tuner, TDEV16_RSSI_COMPLETION_FUNC_T pCompletionFunc, void *parameter);
static TDEV_RETURN_T MAX2172_setIFAGCTimeConstant(TDEV16_T *tuner, int timeConstuS, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter);

TDEV16_CONFIG_T MAX2172Tuner = {
    TDEV_VERSION_I32,			/* Version number - check that tuner and API are built with the same header */
    FALSE,							/* The IF interface is Real */
    TRUE,							/* Set true if the IF spectrum is inverted - in this case IF is not inverted BUT this works around a suspected IQ swap */
    IF_FREQ,						/* The final IF frequency of the tuner (Hz) */
    SYNTH_GRID,						/* The stepsize of the tuner PLL (Hz)      */
    UPDATE_MARGIN,					/* The tuner PLL update margin (Hz)        */
    1000,							/* Settling time in uS from power down to power up */
    1000,							/* Settling time in uS from power save level 1 to power up */
    0,								/* Settling time in uS from power save level 2 to power up */
    1000,							/* Settling time in uS from tune to stable output  */
    MAX2172_Init,					/* function */
    MAX2172_Reset,					/* dummy function */
    MAX2172_Configure,				/* function */
    MAX2172_SetFrequency,			/* function */
    MAX2172_readRFPower,			/* dummy function */
    MAX2172_Enable,					/* function */
    MAX2172_Disable,				/* function */
    MAX2172_powerSave,				/* function */
    MAX2172_setIFAGCTimeConstant,	/* dummy function */
    MAX2172_SetAGC,					/* function */
    MAX2172_initAGC,				/* dummy function */
    MAX2172_Shutdown,				/* function */
};


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
	Max2172Init();

    for(;;)
    {
            /* Wait for a message */
		TUNER_MESSAGE_T *msg = (TUNER_MESSAGE_T *)KRN_getMbox(&taskMbox, KRN_INFWAIT);
        switch(msg->messageType)
        {
            case(SET_FREQ_MESSAGE):
				if (UCC_STANDARD_FM == demodStandard)
				{
					Max2172HighLow(LOW_SIDE_CONVERT == hiLoConvertFlag? 1 : 0);
					Max2172Finv(LOW_SIDE_CONVERT == hiLoConvertFlag? 0 : 1);
				}
				else
				{
					/* Default for DAB operation. */
					Max2172HighLow(1);
					Max2172Finv(0);
				}

                Max2172SetLO(msg->messageFreq);
                break;
            default:
                break;
        }
		msg->CompletionFunc(msg->tuner, TDEV_SUCCESS, msg->parameter);

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
static void taskInit(void)
{
	KRN_initPool(&messagePool, messagePoolDesc, MSG_POOL_SIZE, sizeof(TUNER_MESSAGE_T));
	poolEmptyCount = 0;

	KRN_initMbox(&taskMbox);

    KRN_startTask(taskMain, &task_tcb, task_stack, TASK_STACKSIZE, 1, NULL, "Tuner Task");

    return;
}


/*
** FUNCTION:    MAX2172_SetFrequency
**
** DESCRIPTION: Send the set frequency request off to the tuner control task.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MAX2172_SetFrequency(TDEV16_T *tuner, unsigned freq, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter)
{
	/* To send a message to Tuner Task */
	TUNER_MESSAGE_T *setFreqMsg = (TUNER_MESSAGE_T *) KRN_takePool(&messagePool, KRN_NOWAIT);

	if (setFreqMsg == NULL)
	{
		poolEmptyCount++;
		/* If we can't pass on message signal failure */
		pCompletionFunc(tuner, TDEV_FAILURE, parameter);
		return(TDEV_FAILURE);
	}

	setFreqMsg->messageType = SET_FREQ_MESSAGE;
	setFreqMsg->messageFreq = freq;
	setFreqMsg->CompletionFunc = pCompletionFunc;
	setFreqMsg->tuner = tuner;
	setFreqMsg->parameter = parameter;

	KRN_putMbox(&taskMbox, setFreqMsg);

	return(TDEV_SUCCESS);
}

/*
** FUNCTION:    MAX2172_Init
**
** DESCRIPTION: Initialises the Tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MAX2172_Init(TDEV16_T *tuner, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter)
{
	if (!MAX_setupPort(portNumber))
	{
		pCompletionFunc(tuner, TDEV_FAILURE, parameter);

		return(TDEV_FAILURE);
	}

    taskInit();

	pCompletionFunc(tuner, TDEV_SUCCESS, parameter);

	return(TDEV_SUCCESS);
}

/*
** FUNCTION:    MAX2172_Reset()
**
** DESCRIPTION: dummy function
*/
static TDEV_RETURN_T MAX2172_Reset(TDEV16_T *tuner, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter)
{
	pCompletionFunc(tuner, TDEV_SUCCESS, parameter);
	return TDEV_SUCCESS;
}

/*
** FUNCTION:    MAX2172_Shutdown()
**
** DESCRIPTION: Shutdown the tuner control
*/
static TDEV_RETURN_T MAX2172_Shutdown(TDEV16_T *tuner, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter)
{
		/* Shutdown all we can and record failures along the way, but don't return early. */
	TDEV_RETURN_T retVal = TDEV_SUCCESS;

	KRN_removeTask(&task_tcb);

	if (!MAX_shutdownPort())
	{
		retVal = TDEV_FAILURE;
	}

	pCompletionFunc(tuner, retVal, parameter);
	return(retVal);
}

/*
** FUNCTION:    MAX2172_SetAGC
**
** DESCRIPTION: Send the gain request off to the tuner control task.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MAX2172_SetAGC(TDEV16_T *tuner, TDEV16_AGCISR_HELPER_T *pControl, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter)
{
	/* Send gain value out to PDM DAC */
	SCP_setExtOffsetControl1(tuner->scp, (unsigned)((pControl->IFgainValue)>>4)); // 16 to 12 bit conversion

	pCompletionFunc(tuner, TDEV_SUCCESS, parameter);

	return(TDEV_SUCCESS);
}

/*
** FUNCTION:    MAX2172_Configure()
**
** DESCRIPTION: This tuner does very little to configure the tuner for a specific standard/bandwidth modes, it is all driven from the carrier frequency.
*/
static TDEV_RETURN_T MAX2172_Configure(TDEV16_T *tuner, UCC_STANDARD_T standard, unsigned bandwidthHz, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter)
{
        /* Save bandwidth ready for when it is needed to configure RF. */
    signalBandwidth = bandwidthHz;

    	/* Save the standard we are demodulating as we can treat some of them differently */
	demodStandard = standard;

	if (UCC_STANDARD_FM == standard)
	{	/* For FM operation we shift the final IF as required. */
		/* assume TDEV RF struct is NOT const so can do following ... */
		MAX2172Tuner.frequencyIF = IF_FREQ - abs(Max2172GetFMCenterFreq());
	}
	else
	{
		/* ... but safe to skip following since IF_FREQ is default
		MAX2172Tuner.frequencyIF = IF_FREQ;
		*/
	}

	pCompletionFunc(tuner, TDEV_SUCCESS, parameter);
	return(TDEV_SUCCESS);
}


/*
** FUNCTION:    MAX2172_Enable
**
** DESCRIPTION: Enable the tuner - currently only clears a few variables.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MAX2172_Enable(TDEV16_T *tuner, unsigned muxID, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter)
{
    (void)muxID;		/* Remove compile warning for unused arguments. */

	if (!MAX_enable())
	{
		pCompletionFunc(tuner, TDEV_FAILURE, parameter);
		return(TDEV_FAILURE);
	}
	else
	{
		pCompletionFunc(tuner, TDEV_SUCCESS, parameter);
		return(TDEV_SUCCESS);
	}
}

/*
** FUNCTION:    MAX2172_Disable
**
** DESCRIPTION: Disable tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MAX2172_Disable(TDEV16_T *tuner, unsigned muxID, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter)
{
	(void)muxID;		/* Remove compile warning for unused arguments. */

	if (!MAX_standby())
	{
		pCompletionFunc(tuner, TDEV_FAILURE, parameter);
		return(TDEV_FAILURE);
	}
	else
	{
		pCompletionFunc(tuner, TDEV_SUCCESS, parameter);
		return(TDEV_SUCCESS);
	}
}


/*
** FUNCTION:    MAX2172_readRFPower()
**
** DESCRIPTION: This tuner does not have an RF power sense - dummy function
*/
static TDEV_RETURN_T MAX2172_readRFPower(TDEV16_T *tuner, TDEV16_RSSI_COMPLETION_FUNC_T pCompletionFunc, void *parameter)
{
	pCompletionFunc(tuner, 0, parameter);
	return TDEV_FAILURE;
}

/*
** FUNCTION:    MAX2172_powerSave()
**
** DESCRIPTION: This tuner does not have a power save mode, power it down - dummy function
*/
static TDEV_RETURN_T MAX2172_powerSave(TDEV16_T *tuner, TDEV_RF_PWRSAV_T powerSaveState, unsigned muxID, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter)
{
	int retVal;

    (void)(muxID);		/* Remove compile warnings for unused arguments. */

	if (powerSaveState == TDEV_RF_PWRSAV_OFF)
		retVal = MAX_enable();
	else
		retVal = MAX_standby();

	if (!retVal)
	{
		pCompletionFunc(tuner, TDEV_FAILURE, parameter);
		return(TDEV_FAILURE);
	}
	else
	{
		pCompletionFunc(tuner, TDEV_SUCCESS, parameter);
		return(TDEV_SUCCESS);
	}
}


/*
** FUNCTION:    MAX2172_setIFAGCTimeConstant()
**
** DESCRIPTION: The IF AGC is under software control! - dummy function
*/
static TDEV_RETURN_T MAX2172_setIFAGCTimeConstant(TDEV16_T *tuner, int timeConstuS, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter)
{
    (void)timeConstuS;	/* Remove compile warnings for unused arguments. */

	pCompletionFunc(tuner, TDEV_FAILURE, parameter);
	return TDEV_FAILURE;
}


/*
** FUNCTION:    MAX2172_initAGC()
**
** DESCRIPTION: The IF AGC is under software control! - dummy function
*/
static TDEV_RETURN_T MAX2172_initAGC(TDEV16_T *tuner, unsigned muxID, TDEV16_COMPLETION_FUNC_T pCompletionFunc, void *parameter)
{
    (void)muxID;		/* Remove compile warnings for unused arguments. */

	pCompletionFunc(tuner, TDEV_SUCCESS, parameter);
	return TDEV_SUCCESS;
}


/*
** FUNCTION:    MAX2172Tuner_configure()
**
** DESCRIPTION: Configure various optional setup of driver
*/
void MAX2172Tuner_configure(MAX2172_CONFIG_T *config)
{
	portNumber = config->portNumber;

	return;
}

void MAX2172Tuner_reconfigure(HI_LO_CONVERT_T reqHiLoConvertFlag)
{
	hiLoConvertFlag = reqHiLoConvertFlag;

	return;
}
