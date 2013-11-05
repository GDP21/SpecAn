/*
** FILE NAME:   $RCSfile: stv6110_tuner.c,v $
**
** TITLE:       Satellite tuner driver
**
** AUTHOR:      Imagination Technologies
**
** DESCRIPTION: Implementation of a satellite tuner driver based upon the STV6110
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

#include "stv6110_tuner.h"
#include "stv6110_port.h"


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


#define	IF_FREQ			(0)	/* IF out of tuner. */
#define	SYNTH_GRID		(1000000)
#define	UPDATE_MARGIN	(SYNTH_GRID)

#define DEFAULT_RF_BW	(8000000UL)
#define DEFAULT_RF_FREQ	(1000000000UL)

#define RF_CONTROL_SIZE 12

#define HALF_MHZ		500000
#define	MHZ				1000000
#define	MIN_BANDWIDTH	(5000000 * 2)

#define	SINGLE_REGISTER_WRITE_SIZE	2
#define	DOUBLE_REGISTER_WRITE_SIZE	3
#define MAX_POLL_ATTEMPTS			1000000

#define LO_DIVIDER_THRESHOLD_MHZ	1300
#define PRESC32ON_THRESHOLD_MHZ		4096


/* Addresses of the registers */
#define	CTRL1_REG_ADDRESS		0x0
#define	CTRL2_REG_ADDRESS		0x1
#define	TUNING0_REG_ADDRESS		0x2
#define	TUNING1_REG_ADDRESS		0x3
#define	CTRL3_REG_ADDRESS		0x4
#define	STAT1_REG_ADDRESS		0x5

/* Register CTRL1 */
#define K_DIVIDER				(0<<3)	/* 0 for 16MHz Xtal */
#define SYN						0x01
#define RX						0x02
#define LPT						0x04

/* Register CTRL2 */
#define	CO_DIV1					0x80
#define	CO_DIV0					0x40
#define	REFOUTSEL				0x10
#define	BB_GAIN3_0_MASK			0x0F

#define CO_DIV_BY_1				(0)
#define CO_DIV_BY_2				(CO_DIV0)
#define CO_DIV_BY_4				(CO_DIV1)
#define CO_DIV_BY_8				(CO_DIV1 | CO_DIV0)

#define BB_GAIN_0DB				0
#define BB_GAIN_2DB				1
#define BB_GAIN_4DB				2
#define BB_GAIN_6DB				3
#define BB_GAIN_8DB				4
#define BB_GAIN_10DB			5
#define BB_GAIN_12DB			6
#define BB_GAIN_14DB			7
#define BB_GAIN_16DB			8


/* Register TUNING0 */
#define N_DIV7_0_MASK			0xFF

/* Register TUNING1 */
#define	R_DIV1					0x80
#define	R_DIV0					0x40
#define	PRESC32ON				0x20
#define	DIV4SEL					0x10
#define	N_DIV11_8_MASK			0x0F
#define	R_DIV_2					0
#define	R_DIV_4					R_DIV0
#define	R_DIV_8					R_DIV1
#define	R_DIV_16				(R_DIV1 | R_DIV0)

/* Register CTRL3 */
#define	DCLOOP_OFF				0x80
#define	ICP						0x20
#define	CF4_0_MASK				0x1F

/* Register STAT1 */
#define	CALVCO_STRT				0x04
#define	CALRC_STRT				0x02
#define	LOCK					0x01


typedef enum
{
	SET_FREQ_MESSAGE = 0,
	SET_CONFIG_MESSAGE,
	POWER_UP_MESSAGE,
	POWER_DOWN_MESSAGE,
	NUMBER_OF_MSG		/* The number of different messages we can have */
} MSG_T;

typedef struct
{
	KRN_POOLLINK;
	MSG_T messageType;		/* What do we need to do with this message */
	TDEV_T *pTuner;			/* Tuner instance */
	unsigned param;			/* Frequency to tune to if it is a SET_FREQ_MESSAGE, or Bandwidth for SET_CONFIG_MESSAGE */
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
	unsigned frequency;

	/* Bandwidth we are configured to operate at, in Hz.*/
	unsigned signalBandwidth;
	UCC_STANDARD_T demodStandard;

	int i2cAddresss;
	unsigned char rfCmdArray[RF_CONTROL_SIZE];


	KRN_TASK_T task_tcb;
	unsigned int task_stack[TASK_STACKSIZE];
	TUNER_MESSAGE_T messagePoolDesc[MSG_POOL_SIZE];
	KRN_POOL_T	 messagePool;
	KRN_MAILBOX_T taskMbox;
	int poolEmptyCount;

} STV_WORKSPACE_T;

static STV_WORKSPACE_T *dbgTunerWrkspc = NULL;

/* Static Functions for Tuner API structure */
static TDEV_RETURN_T STV_Initialise(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T STV_Reset(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T STV_Tune(TDEV_T *pTuner, unsigned freq, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T STV_SetAGC(TDEV_T *pTuner, TDEV_AGCISR_HELPER_T * pControl, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T STV_Enable(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T STV_Disable(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T STV_Configure(TDEV_T *pTuner, UCC_STANDARD_T standard, unsigned bandwidthHz, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T STV_Shutdown(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T STV_powerSave(TDEV_T *pTuner, TDEV_RF_PWRSAV_T powerSaveState, unsigned control, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);

/* dummy functions for Tuner API structure */
static TDEV_RETURN_T STV_initAGC(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T STV_readRFPower(TDEV_T *pTuner, TDEV_RSSI_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T STV_setIFAGCTimeConstant(TDEV_T *pTuner, int timeConstuS, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);

TDEV_CONFIG_T STV6110Tuner = {
    TDEV_VERSION_I32,				/* Version number - check that tuner and API are built with the same header */
    TRUE,							/* The IF interface is Complex */
    FALSE,							/* Set true if the IF spectrum is inverted */
    IF_FREQ,						/* The final IF frequency of the tuner (Hz) */
    SYNTH_GRID,						/* The stepsize of the tuner PLL (Hz)      */
    UPDATE_MARGIN,					/* The tuner PLL update margin (Hz)        */
    1000,							/* Settling time in uS from power down to power up */
    0,								/* Settling time in uS from power save level 1 to power up */
    0,								/* Settling time in uS from power save level 2 to power up */
    1000,							/* Settling time in uS from tune to stable output  */
    STV_Initialise,				/* function */
    STV_Reset,					/* function */
    STV_Configure,				/* function */
    STV_Tune,					/* function */
    STV_readRFPower,			/* dummy function */
    STV_Enable,					/* function */
    STV_Disable,				/* function */
    STV_powerSave,				/* function */
    STV_setIFAGCTimeConstant,	/* dummy function */
    STV_SetAGC,					/* function */
    STV_initAGC,				/* dummy function */
    STV_Shutdown				/* function */
};


/*
** FUNCTION:    SetFrequency
**
** DESCRIPTION: Tuning to a frequency.
**
** RETURNS:     void
**
*/
static TDEV_RETURN_T SetFrequency(STV_WORKSPACE_T *wrkspc, unsigned frequency_Hz)
{
	unsigned vco_frequency, frequency_MHz = (frequency_Hz + HALF_MHZ)/ 1000000;
	unsigned char stat1_register, tuning2_bits = 0;
	int pollAttempts = 0;
	unsigned n_div;

	if (frequency_MHz >= LO_DIVIDER_THRESHOLD_MHZ)
	{
		vco_frequency = frequency_MHz * 2;
		tuning2_bits = R_DIV_8;
	}
	else
	{
		vco_frequency = frequency_MHz * 4;
		tuning2_bits = DIV4SEL | R_DIV_4;
	}

	if (vco_frequency >= PRESC32ON_THRESHOLD_MHZ)
	{
		tuning2_bits |= PRESC32ON;
	}

	n_div = frequency_MHz;	/* as freq LO step is 1MHz */

	/* set LO frequency */
	wrkspc->rfCmdArray[0] = TUNING0_REG_ADDRESS;			/* [0] register address */
	wrkspc->rfCmdArray[1] = n_div & N_DIV7_0_MASK;			/* [1] TUNING0 register contents */
	wrkspc->rfCmdArray[2] = ((n_div >> 8) & N_DIV11_8_MASK) | tuning2_bits;	/* [1] TUNING1 register contents */


	if(!STV_writeMessage(wrkspc->i2cAddresss, wrkspc->rfCmdArray, DOUBLE_REGISTER_WRITE_SIZE))
	{
		return(TDEV_FAILURE);
	}


	/* Start Cal of VCO frequency */
	wrkspc->rfCmdArray[0] = STAT1_REG_ADDRESS;					/* [0] register address */
	wrkspc->rfCmdArray[1] = CALVCO_STRT;						/* [1] register contents */

	if(!STV_writeMessage(wrkspc->i2cAddresss, wrkspc->rfCmdArray, SINGLE_REGISTER_WRITE_SIZE))
	{
		return(TDEV_FAILURE);
	}

	/* wait for 'CALVCO_STRT' bit to be cleared, hence cal complete */
	do
	{
			/* Don't poll for ever, fail after too many attempts */
		if (++pollAttempts > MAX_POLL_ATTEMPTS)
			return(TDEV_FAILURE);

		if(!STV_readMessage(wrkspc->i2cAddresss, STAT1_REG_ADDRESS, &stat1_register, 1))
			return(TDEV_FAILURE);

	} while (stat1_register & CALVCO_STRT);


	return(TDEV_SUCCESS);
}

/*
** FUNCTION:    SetBandwidth
**
** DESCRIPTION: Set up RF for given bandwidth.
**
** RETURNS:     void
**
*/
static TDEV_RETURN_T SetBandwidth(STV_WORKSPACE_T *wrkspc, unsigned bandwidth_Hz)
{
	unsigned char stat1_register, ctrl3_register;
	int bandwidth_index = ((signed)bandwidth_Hz + MHZ - MIN_BANDWIDTH) / (2 * MHZ);
	int pollAttempts = 0;

	/* Limit the bandwidth index */
	if (bandwidth_index < 0)
		bandwidth_index = 0;
	if (bandwidth_index > 31)
		bandwidth_index = 31;

	/* Read Ctrl3 and clear bits to be set*/
	if(!STV_readMessage(wrkspc->i2cAddresss, CTRL3_REG_ADDRESS, &ctrl3_register, 1))
	{
		return(TDEV_FAILURE);
	}
	ctrl3_register &= ~CF4_0_MASK;

	/* set baseband filter cut of frequency */
	ctrl3_register |= (bandwidth_index & CF4_0_MASK);
	wrkspc->rfCmdArray[0] = CTRL3_REG_ADDRESS;			/* [0] register address */
	wrkspc->rfCmdArray[1] = ctrl3_register;				/* [1] register contents */

	if(!STV_writeMessage(wrkspc->i2cAddresss, wrkspc->rfCmdArray, SINGLE_REGISTER_WRITE_SIZE))
	{
		return(TDEV_FAILURE);
	}

	/* Start Cal of LPF */
	wrkspc->rfCmdArray[0] = STAT1_REG_ADDRESS;					/* [0] register address */
	wrkspc->rfCmdArray[1] = CALRC_STRT;							/* [1] register contents */

	if(!STV_writeMessage(wrkspc->i2cAddresss, wrkspc->rfCmdArray, SINGLE_REGISTER_WRITE_SIZE))
	{
		return(TDEV_FAILURE);
	}


	/* wait for 'CALRC_STRT' bit to be cleared, hence cal complete */
	do
	{
			/* Down poll for ever, fail after too many attempts */
		if (++pollAttempts > MAX_POLL_ATTEMPTS)
			return(TDEV_FAILURE);

		if(!STV_readMessage(wrkspc->i2cAddresss, STAT1_REG_ADDRESS, &stat1_register, 1))
			return(TDEV_FAILURE);

	} while (stat1_register & CALRC_STRT);


	return(TDEV_SUCCESS);
}


/*
** FUNCTION:    HandlePowerUp
**
** DESCRIPTION: Power up the tuner.
**
** RETURNS:     void
**
*/
static TDEV_RETURN_T HandlePowerUp(STV_WORKSPACE_T *wrkspc)
{
	TDEV_RETURN_T result;

	/* Power up tuner */
	/* set operating level to synth and Rx chain on */
	wrkspc->rfCmdArray[0] = CTRL1_REG_ADDRESS;		/* [0] register address */
	wrkspc->rfCmdArray[1] = K_DIVIDER | RX | SYN;	/* [1] register contents */
	wrkspc->rfCmdArray[2] = BB_GAIN_6DB;	/* [2] CTRL2 register contents */

	if(!STV_writeMessage(wrkspc->i2cAddresss, wrkspc->rfCmdArray, DOUBLE_REGISTER_WRITE_SIZE))
	{
		return(TDEV_FAILURE);
	}

	/* Set LO and bandwidth as previously set */
	result  = SetBandwidth(wrkspc, wrkspc->signalBandwidth);
	result |= SetFrequency(wrkspc, wrkspc->frequency);

	return(result);
}

/*
** FUNCTION:    HandlePowerDown
**
** DESCRIPTION: Power down the tuner.
**
** RETURNS:     void
**
*/
static TDEV_RETURN_T HandlePowerDown(STV_WORKSPACE_T *wrkspc)
{
#if 0
	/* set operating level to all off */
	wrkspc->rfCmdArray[0] = CTRL1_REG_ADDRESS;	/* [0] register address */
	wrkspc->rfCmdArray[1] = K_DIVIDER;			/* [1] register contents */

	if(!STV_writeMessage(wrkspc->i2cAddresss, wrkspc->rfCmdArray, SINGLE_REGISTER_WRITE_SIZE))
	{
		return(TDEV_FAILURE);
	}
#else
	(void)wrkspc;
#endif
	(void)(wrkspc);
	return(TDEV_SUCCESS);
}


/*
** FUNCTION:    taskMain
**
** DESCRIPTION: Simple dispatcher task to send Frequency requests etc. to tuner via SCBM device driver.
**
** RETURNS:     void
**
*/
static void taskMain(void)
{
	STV_WORKSPACE_T *wrkspc = KRN_taskParameter(NULL);

	(void)HandlePowerUp(wrkspc); // hack...

    for(;;)
    {
		TDEV_RETURN_T	messageSuccess =  TDEV_SUCCESS;
            /* Wait for a message */
		TUNER_MESSAGE_T *msg = (TUNER_MESSAGE_T *)KRN_getMbox(&wrkspc->taskMbox, KRN_INFWAIT);

        switch(msg->messageType)
        {
            case(SET_CONFIG_MESSAGE):
                wrkspc->signalBandwidth = msg->param;
                messageSuccess = SetBandwidth(wrkspc, msg->param);
                break;
            case(SET_FREQ_MESSAGE):
                wrkspc->frequency = msg->param;
                messageSuccess = SetFrequency(wrkspc, msg->param);
                break;
            case(POWER_UP_MESSAGE):
                messageSuccess = HandlePowerUp(wrkspc);
                break;
            case(POWER_DOWN_MESSAGE):
                messageSuccess = HandlePowerDown(wrkspc);
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
static void taskInit(STV_WORKSPACE_T *wrkspc)
{
	KRN_initPool(&wrkspc->messagePool, wrkspc->messagePoolDesc, MSG_POOL_SIZE, sizeof(TUNER_MESSAGE_T));
	wrkspc->poolEmptyCount = 0;

	KRN_initMbox(&wrkspc->taskMbox);

    KRN_startTask(taskMain, &wrkspc->task_tcb, wrkspc->task_stack, TASK_STACKSIZE, 1, wrkspc, "Tuner Task");

    return;
}

/* Fix up a suitable analog gain setting, for temporary purposes on Satellite project until
we get around to controlling this properly */
//#define DEFAULT_ANALOG_GAIN_SETTING 0x0fff
#define DEFAULT_ANALOG_GAIN_SETTING 0x0480

/*
** FUNCTION:    STV_Tune
**
** DESCRIPTION: Send the set frequency request off to the tuner control task.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T STV_Tune(TDEV_T *pTuner, unsigned freq, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	STV_WORKSPACE_T *wrkspc = pTuner->workSpace;

	/* To send a message to Tuner Task */
	TUNER_MESSAGE_T *setFreqMsg = (TUNER_MESSAGE_T *) KRN_takePool(&wrkspc->messagePool, KRN_NOWAIT);

	if (setFreqMsg == NULL)
	{
		wrkspc->poolEmptyCount++;
		/* If we can't pass on message signal failure */
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
		return(TDEV_FAILURE);
	}

	setFreqMsg->param = freq;
	setFreqMsg->CompletionFunc = pCompletionFunc;
	setFreqMsg->CompletionParamater = completionParameter;
	setFreqMsg->messageType = SET_FREQ_MESSAGE;
	setFreqMsg->pTuner = pTuner;

	KRN_putMbox(&wrkspc->taskMbox, setFreqMsg);

	/* Send Max. gain value out to PDM DAC */
    SCP_setExtOffsetControl1(pTuner->scp, DEFAULT_ANALOG_GAIN_SETTING);

    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    STV_Initialise
**
** DESCRIPTION: Initialises the Tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T STV_Initialise(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	STV_WORKSPACE_T *wrkspc = pTuner->workSpace;

	assert(STV6110_TUNER_DRIVER_WORKSPACE_SIZE >= sizeof(STV_WORKSPACE_T));

	dbgTunerWrkspc = wrkspc;

	/* Zero the context before we start */
	memset(wrkspc, 0, sizeof(STV_WORKSPACE_T));

	wrkspc->signalBandwidth = DEFAULT_RF_BW;
	wrkspc->frequency = DEFAULT_RF_FREQ;
	wrkspc->demodStandard = UCC_STANDARD_DVBS2;
	wrkspc->i2cAddresss = ((STV6110_CONFIG_T *)(pTuner->tunerConfigExtension))->address;

	if (!STV_setupPort(((STV6110_CONFIG_T *)(pTuner->tunerConfigExtension))->portNumber))
	{
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);

		return(TDEV_FAILURE);
	}

    taskInit(wrkspc);

	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    STV_Reset
**
** DESCRIPTION: Resets the Tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T STV_Reset(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    STV_Shutdown()
**
** DESCRIPTION: Shutdown the tuner control
*/
static TDEV_RETURN_T STV_Shutdown(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	STV_WORKSPACE_T *wrkspc = pTuner->workSpace;

		/* Shutdown all we can and record failures along the way, but don't return early. */
	TDEV_RETURN_T retVal = TDEV_SUCCESS;

	KRN_removeTask(&wrkspc->task_tcb);

	if (!STV_shutdownPort())
	{
		retVal = TDEV_FAILURE;
	}

	pCompletionFunc(pTuner, retVal, completionParameter);
	return(retVal);
}

/*
** FUNCTION:    STV_SetAGC
**
** DESCRIPTION: Send the gain request off to the tuner control task.
**
** RETURNS:     Success
**
*/
static volatile int ForceAgc = 0;
static volatile int LogAgc = 0;
static TDEV_RETURN_T STV_SetAGC(TDEV_T *pTuner, TDEV_AGCISR_HELPER_T *pControl, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	LogAgc = pControl->IFgainValue;

	if (ForceAgc != 0)
		pControl->IFgainValue = ForceAgc;

	/* Send gain value out to PDM DAC */
    SCP_setExtOffsetControl1(pTuner->scp, (pControl->IFgainValue) >> 4); /* 16 to 12 bit conversion */

	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    STV_Configure()
**
** DESCRIPTION: Log the standard and signal bandwidth.
*/
static TDEV_RETURN_T STV_Configure(TDEV_T *pTuner, UCC_STANDARD_T standard, unsigned bandwidthHz, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	STV_WORKSPACE_T *wrkspc = pTuner->workSpace;

        /* Save bandwidth ready for when it is needed to configure RF. */
    wrkspc->signalBandwidth = bandwidthHz;

    	/* Save the standard we are demodulating as we can treat some of them differently */
	wrkspc->demodStandard = standard;

		/* Workout which are valid and report. */
	switch(standard)
	{
		case UCC_STANDARD_DVBS:
		case UCC_STANDARD_DVBS2:
		case UCC_STANDARD_ISDBS:
		/* UCC_STANDARD_NOT_SIGNALLED is considered a supported standard by all drivers; this is used by
		the spectrum analyser core. */
		case UCC_STANDARD_NOT_SIGNALLED:
			break;

			/* The following standards are not supported, so signal an error. */
		case UCC_STANDARD_ATSC:
		case UCC_STANDARD_DAB:
		case UCC_STANDARD_DVBC:
		case UCC_STANDARD_DVBH:
		case UCC_STANDARD_DVBT2:
		case UCC_STANDARD_DVBT:
		case UCC_STANDARD_FM:
		case UCC_STANDARD_ISDBC:
		case UCC_STANDARD_ISDBT_13SEG:
		case UCC_STANDARD_ISDBT_1SEG:
		case UCC_STANDARD_ISDBT_3SEG:
		case UCC_STANDARD_J83B:
		default:
			pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
			return(TDEV_FAILURE);
			break;
	}

	/* To send a message to Tuner Task */
	TUNER_MESSAGE_T *setConfigMsg = (TUNER_MESSAGE_T *) KRN_takePool(&wrkspc->messagePool, KRN_NOWAIT);

	if (setConfigMsg == NULL)
	{
		wrkspc->poolEmptyCount++;
		/* If we can't pass on message signal failure */
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
		return(TDEV_FAILURE);
	}

	setConfigMsg->param = bandwidthHz;
	setConfigMsg->CompletionFunc = pCompletionFunc;
	setConfigMsg->CompletionParamater = completionParameter;
	setConfigMsg->messageType = SET_CONFIG_MESSAGE;
	setConfigMsg->pTuner = pTuner;

	KRN_putMbox(&wrkspc->taskMbox, setConfigMsg);

	return(TDEV_SUCCESS);
}


/*
** FUNCTION:    STV_Enable
**
** DESCRIPTION: Enable the tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T STV_Enable(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	STV_WORKSPACE_T *wrkspc = pTuner->workSpace;

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
** FUNCTION:    STV_Disable
**
** DESCRIPTION: Disable tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T STV_Disable(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	STV_WORKSPACE_T *wrkspc = pTuner->workSpace;

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
** FUNCTION:    STV_readRFPower()
**
** DESCRIPTION: This tuner does not have an RF power sense - dummy function
*/
static TDEV_RETURN_T STV_readRFPower(TDEV_T *pTuner, TDEV_RSSI_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	pCompletionFunc(pTuner, 0, completionParameter);
    return TDEV_FAILURE;
}

/*
** FUNCTION:    STV_powerSave()
**
** DESCRIPTION: This tuner does not have a power save mode, power it down - dummy function
*/
static TDEV_RETURN_T STV_powerSave(TDEV_T *pTuner, TDEV_RF_PWRSAV_T powerSaveState, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
    (void)(muxID);			/* Remove compile warnings for unused arguments. */
    (void)(powerSaveState);	/* Remove compile warnings for unused arguments. */

	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
	return(TDEV_SUCCESS);
}


/*
** FUNCTION:    STV_setIFAGCTimeConstant()
**
** DESCRIPTION: The IF AGC is under software control! - dummy function
*/
static TDEV_RETURN_T STV_setIFAGCTimeConstant(TDEV_T *pTuner, int timeConstuS, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
    (void)timeConstuS;	/* Remove compile warnings for unused arguments. */

	pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
    return TDEV_FAILURE;
}


/*
** FUNCTION:    STV_initAGC()
**
** DESCRIPTION: The IF AGC is under software control! - dummy function
*/
static TDEV_RETURN_T STV_initAGC(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
    (void)muxID;		/* Remove compile warnings for unused arguments. */

	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}

