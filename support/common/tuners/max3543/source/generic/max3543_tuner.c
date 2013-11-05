/*
** FILE NAME:   $RCSfile: max3543_tuner.c,v $
**
** TITLE:       MAX3543 DTV tuner driver
**
** AUTHOR:      Imagination Technologies
**
** DESCRIPTION: Implementation of a Maxim MAX3543 tuner driver
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

#include "PHY_tuner.h"

#include "max3543_tuner.h"
#include "max_port.h"

#include "max3543_driver.h"

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


/* Replace with whatever the actual RF requires. */
#define	IF_FREQ			(36150000 - 29952000)	/* IF out of tuner sub-sampled, hence mixed down. */
#define	SYNTH_GRID		(1000000 / LOSCALE)
#define	UPDATE_MARGIN	(SYNTH_GRID * 2)

#define DEFAULT_RF_FREQ	(650000000UL)

/* Which port we should use to communicate with the tuner */
static int	portNumber = 1;

/* Bandwidth we are configured to operate at, in Hz.
** Initialised to zero as an invalid value.
** This will ensure that a bandwidth has been set before it is used. */
static long signalBandwidth = 0;
static PHY_TUNER_STANDARD_T demodStandard;

typedef enum
{
	SET_FREQ_MESSAGE = 0,
	CONFIGURE_MESSAGE,
	NUMBER_OF_MSG		/* The number of different messages we can have */
} MSG_T;

typedef struct
{
	KRN_POOLLINK;
	MSG_T messageType;		/* What do we need to do with this message */
	long messageFreq;		/* Frequency to tune to if it is a SET_FREQ_MESSAGE */
	TUNER_COMPLETION_FUNC_T CompletionFunc;
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
static PHY_TUNER_RETURN_T MAX3543_Initialise(TUNER_COMPLETION_FUNC_T pCompletionFunc);
static PHY_TUNER_RETURN_T MAX3543_Tune(long freq, TUNER_COMPLETION_FUNC_T pCompletionFunc);
static PHY_TUNER_RETURN_T MAX3543_SetAGC(TUNER_AGCISR_HELPER_T * pControl, TUNER_COMPLETION_FUNC_T pCompletionFunc);
static PHY_TUNER_RETURN_T MAX3543_Enable(unsigned long muxID,TUNER_COMPLETION_FUNC_T pCompletionFunc);
static PHY_TUNER_RETURN_T MAX3543_Disable(unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc);
static PHY_TUNER_RETURN_T MAX3543_Configure(PHY_TUNER_STANDARD_T standard, long bandwidthHz, TUNER_COMPLETION_FUNC_T pCompletionFunc);
static PHY_TUNER_RETURN_T MAX3543_Shutdown(TUNER_COMPLETION_FUNC_T pCompletionFunc);
static PHY_TUNER_RETURN_T MAX3543_powerSave(PHY_RF_PWRSAV_T powerSaveState, unsigned long control, TUNER_COMPLETION_FUNC_T pCompletionFunc);

/* dummy functions for Tuner API structure */
static PHY_TUNER_RETURN_T MAX3543_initAGC(unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc);
static long MAX3543_readRFPower(TUNER_COMPLETION_FUNC_T pCompletionFunc);
static PHY_TUNER_RETURN_T MAX3543_setIFAGCTimeConstant(long timeConstuS, TUNER_COMPLETION_FUNC_T pCompletionFunc);

TUNER_CONTROL_T MAX3543Tuner = {
    PHY_TUNER_VERSION_I32,			/* Version number - check that tuner and API are built with the same header */
    FALSE,							/* The IF interface is Real */
    IF_FREQ,						/* The final IF frequency of the tuner (Hz) */
    TRUE,							/* Set true if the IF spectrum is inverted */
    SYNTH_GRID,						/* The stepsize of the tuner PLL (Hz)      */
    UPDATE_MARGIN,					/* The tuner PLL update margin (Hz)        */
    1000,							/* Settling time in uS from power down to power up */
    1000,							/* Settling time in uS from power save level 1 to power up */
    0,								/* Settling time in uS from power save level 2 to power up */
    1000,							/* Settling time in uS from tune to stable output  */
    MAX3543_Initialise,				/* function */
    MAX3543_Configure,				/* function */
    MAX3543_Tune,					/* function */
    MAX3543_readRFPower,			/* dummy function */
    MAX3543_Enable,					/* function */
    MAX3543_Disable,				/* function */
    MAX3543_powerSave,				/* function */
    MAX3543_setIFAGCTimeConstant,	/* dummy function */
    MAX3543_SetAGC,					/* function */
    MAX3543_initAGC,				/* dummy function */
    MAX3543_Shutdown,				/* function */
    NULL,						   /* No standard specific info */
    NULL						   /* No tuner specific info */
};


/*
** FUNCTION:    HandleConfigure
**
** DESCRIPTION: Handle the standard/bandwidth configuration, to be called from within the MeOS task.
**
** RETURNS:     void
**
*/
static void HandleConfigure(void)
{
	switch(demodStandard)
	{
		case PHY_TUNER_DVBT:
			MAX3543_Standard(DVB_T, IFOUT1_DIFF_DTVOUT);
			/* Select the bandwidth from either 7 or 8MHz */
			if (signalBandwidth < 7500000)
				MAX3543_ChannelBW(BW7MHZ);
			else
				MAX3543_ChannelBW(BW8MHZ);
			break;

		case PHY_TUNER_ATSC:
			MAX3543_Standard(ATSC, IFOUT1_DIFF_DTVOUT);
			MAX3543_ChannelBW(BW7MHZ);
			break;

			/* The following standards are not supported, so ignore. */
		case PHY_TUNER_NOT_SIGNALLED:
		case PHY_TUNER_DVBH:
		case PHY_TUNER_ISDBT_1SEG:
		case PHY_TUNER_ISDBT_3SEG:
		case PHY_TUNER_ISDBT_13SEG:
		case PHY_TUNER_DAB:
		case PHY_TUNER_FM:
		default:
			assert(0);
			break;
	}
	return;
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
	MAX3543_Init(DEFAULT_RF_FREQ);

    for(;;)
    {
            /* Wait for a message */
		TUNER_MESSAGE_T *msg = (TUNER_MESSAGE_T *)KRN_getMbox(&taskMbox, KRN_INFWAIT);
        switch(msg->messageType)
        {
            case(SET_FREQ_MESSAGE):
                MAX3543_SetFrequency(msg->messageFreq);
                break;
            case(CONFIGURE_MESSAGE):
                HandleConfigure();
                break;
            default:
                break;
        }
		msg->CompletionFunc(PHY_TUNER_SUCCESS);

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
** FUNCTION:    MAX3543_Tune
**
** DESCRIPTION: Send the set frequency request off to the tuner control task.
**
** RETURNS:     Success
**
*/
static PHY_TUNER_RETURN_T MAX3543_Tune(long freq, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{

	/* To send a message to Tuner Task */
	TUNER_MESSAGE_T *setFreqMsg = (TUNER_MESSAGE_T *) KRN_takePool(&messagePool, KRN_NOWAIT);

	if (setFreqMsg == NULL)
	{
		poolEmptyCount++;
		/* If we can't pass on message signal failure */
		pCompletionFunc(PHY_TUNER_FAILURE);
		return(PHY_TUNER_FAILURE);
	}

	setFreqMsg->messageFreq = freq;
	setFreqMsg->CompletionFunc = pCompletionFunc;
	setFreqMsg->messageType = SET_FREQ_MESSAGE;

	KRN_putMbox(&taskMbox, setFreqMsg);

    return(PHY_TUNER_SUCCESS);
}

/*
** FUNCTION:    MAX3543_Initialise
**
** DESCRIPTION: Initialises the Tuner.
**
** RETURNS:     Success
**
*/
static PHY_TUNER_RETURN_T MAX3543_Initialise(TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	if (!MAX_setupPort(portNumber))
	{
		pCompletionFunc(PHY_TUNER_FAILURE);

		return(PHY_TUNER_FAILURE);
	}

    taskInit();

    pCompletionFunc(PHY_TUNER_SUCCESS);

    return(PHY_TUNER_SUCCESS);
}

/*
** FUNCTION:    MAX3543_Shutdown()
**
** DESCRIPTION: Shutdown the tuner control
*/
static PHY_TUNER_RETURN_T MAX3543_Shutdown(TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
		/* Shutdown all we can and record failures along the way, but don't return early. */
	PHY_TUNER_RETURN_T retVal = PHY_TUNER_SUCCESS;

	KRN_removeTask(&task_tcb);

	if (!MAX_shutdownPort())
	{
		retVal = PHY_TUNER_FAILURE;
	}

	pCompletionFunc(retVal);
	return(retVal);
}

/*
** FUNCTION:    MAX3543_SetAGC
**
** DESCRIPTION: Send the gain request off to the tuner control task.
**
** RETURNS:     Success
**
*/
static volatile int ForceAgc = 0;
static volatile int LogAgc = 0;
static PHY_TUNER_RETURN_T MAX3543_SetAGC(TUNER_AGCISR_HELPER_T * pControl, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	LogAgc = pControl->IFgainValue;

	if (ForceAgc != 0)
		pControl->IFgainValue = ForceAgc;

	/* Send gain value out to PDM DAC */
	Tuner_SetExtOffset1((pControl->IFgainValue)>>4); // 16 to 12 bit conversion

	pCompletionFunc(PHY_TUNER_SUCCESS);

    return(PHY_TUNER_SUCCESS);
}

/*
** FUNCTION:    MAX3543_Configure()
**
** DESCRIPTION: Log the standard signal bandwidth, then kick off setting up the tuner to this configuration.
*/
static PHY_TUNER_RETURN_T MAX3543_Configure(PHY_TUNER_STANDARD_T standard, long bandwidthHz, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	PHY_TUNER_RETURN_T retVal;

bandwidthHz = 7000000;
standard = PHY_TUNER_DVBT;

        /* Save bandwidth ready for when it is needed to configure RF. */
    signalBandwidth = bandwidthHz;

    	/* Save the standard we are demodulating as we can treat some of them differently */
	demodStandard = standard;

		/* Workout which are valid and hence are worth passing on to the tuner driver task. */
	switch(standard)
	{
		case PHY_TUNER_DVBT:
			retVal = PHY_TUNER_SUCCESS;
			break;

		case PHY_TUNER_ATSC:
			retVal = PHY_TUNER_SUCCESS;
			break;

			/* The following standards are not supported, so signal an error. */
		case PHY_TUNER_NOT_SIGNALLED:
		case PHY_TUNER_DVBH:
		case PHY_TUNER_ISDBT_1SEG:
		case PHY_TUNER_ISDBT_3SEG:
		case PHY_TUNER_ISDBT_13SEG:
		case PHY_TUNER_DAB:
		case PHY_TUNER_FM:
		default:
			retVal = PHY_TUNER_FAILURE;
			break;
	}


	if (retVal == PHY_TUNER_SUCCESS)
	{
		/* To send a message to Tuner Task */
		TUNER_MESSAGE_T *configMsg = (TUNER_MESSAGE_T *) KRN_takePool(&messagePool, KRN_NOWAIT);

		if (configMsg == NULL)
		{
			poolEmptyCount++;
			/* If we can't pass on message signal failure */
			retVal = PHY_TUNER_FAILURE;
		}
		else
		{
			configMsg->CompletionFunc = pCompletionFunc;
			configMsg->messageType = CONFIGURE_MESSAGE;

			KRN_putMbox(&taskMbox, configMsg);

				/* return without calling completion function, that will be called from the
				** tuner driver task when it has actually completed its action */
			return(retVal);
		}
	}

    pCompletionFunc(retVal);
    return(retVal);
}


/*
** FUNCTION:    MAX3543_Enable
**
** DESCRIPTION: Enable the tuner - currently only clears a few variables.
**
** RETURNS:     Success
**
*/
static PHY_TUNER_RETURN_T MAX3543_Enable(unsigned long muxID,TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
    (void)muxID;		/* Remove compile warning for unused arguments. */

	pCompletionFunc(PHY_TUNER_SUCCESS);
	return(PHY_TUNER_SUCCESS);
}

/*
** FUNCTION:    MAX3543_Disable
**
** DESCRIPTION: Disable tuner.
**
** RETURNS:     Success
**
*/
static PHY_TUNER_RETURN_T MAX3543_Disable(unsigned long muxID,TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	(void)muxID;		/* Remove compile warning for unused arguments. */

	pCompletionFunc(PHY_TUNER_SUCCESS);
	return(PHY_TUNER_SUCCESS);
}


/*
** FUNCTION:    MAX3543_readRFPower()
**
** DESCRIPTION: This tuner does not have an RF power sense - dummy function
*/
static long MAX3543_readRFPower(TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
    pCompletionFunc(PHY_TUNER_FAILURE);
    return PHY_TUNER_FAILURE;
}

/*
** FUNCTION:    MAX3543_powerSave()
**
** DESCRIPTION: This tuner does not have a power save mode, power it down - dummy function
*/
static PHY_TUNER_RETURN_T MAX3543_powerSave(PHY_RF_PWRSAV_T powerSaveState, unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
    (void)(muxID);			/* Remove compile warnings for unused arguments. */
    (void)(powerSaveState);	/* Remove compile warnings for unused arguments. */

	pCompletionFunc(PHY_TUNER_SUCCESS);
	return(PHY_TUNER_SUCCESS);
}


/*
** FUNCTION:    MAX3543_setIFAGCTimeConstant()
**
** DESCRIPTION: The IF AGC is under software control! - dummy function
*/
static PHY_TUNER_RETURN_T MAX3543_setIFAGCTimeConstant(long timeConstuS, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
    (void)timeConstuS;	/* Remove compile warnings for unused arguments. */

    pCompletionFunc(PHY_TUNER_FAILURE);
    return PHY_TUNER_FAILURE;
}


/*
** FUNCTION:    MAX3543_initAGC()
**
** DESCRIPTION: The IF AGC is under software control! - dummy function
*/
static PHY_TUNER_RETURN_T MAX3543_initAGC(unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
    (void)muxID;		/* Remove compile warnings for unused arguments. */
    pCompletionFunc(PHY_TUNER_SUCCESS);
    return PHY_TUNER_SUCCESS;
}


/*
** FUNCTION:    MAX3543Tuner_configure()
**
** DESCRIPTION: Configure various optional setup of driver
*/
void MAX3543Tuner_configure(MAX3543_CONFIG_T *config)
{
	portNumber = config->portNumber;
	return;
}
