/*!
******************************************************************************

 @file si2153_tuner.h

 @brief SiLabs DTV tuner driver

 @Author Imagination Technologies

	<b>Copyright (C) Imagination Technologies Ltd.</b>\n

	This software is supplied under a licence agreement.
	It must not be copied or distributed except under the
	terms of that agreement.

 \n<b>Description:</b>\n
	Interface for the SiLabs DTV tuner driver.

******************************************************************************/
/*! \mainpage SiLabs DTV tuner driver
*******************************************************************************
 \section intro Introduction

 This is a driver for the SiLabs DTV tuner driver.

 <b>Feedback</b>

 If you have any comments regarding this document, please contact your
 Imagination Technologies representative.
 Please provide the document title and revision with a description of the problem
 or suggestion for improvement.

*******************************************************************************/

#ifndef SI2153_TUNER_H
/* @cond DONT_DOXYGEN */
#define SI2153_TUNER_H

#include "uccrt.h"
#include "si2153.h"
/* @endcond */

/*!
*******************************************************************************
 SiLabs tuner driver configuration.
 The various aspect of the driver that can be configured at run time.
*******************************************************************************/
#define D_SI2153_DTV_TASK_PRIO (4)
	/* Size the pool to be more than the possible number of outstanding messages
	** incase we try and issue a new message just as we are completing one.
	** This assumes that only one of each message can occur at any time. */
#define MSG_POOL_SIZE	(NUMBER_OF_MSG + 1)
#define TASK_STACKSIZE (1024)

typedef struct
{
	/*! Serial port number.
	    The exact meaning of this is dependant upon the SOC and its I2C driver.
	*/
	int portNumber;
	/*! I2C address of tuner
	*/
	int address;
} SI2153_CONFIG_T;

typedef enum
{
	SET_FREQ_MESSAGE = 0,
	POWER_UP_MESSAGE,
	POWER_DOWN_MESSAGE,
	POLL_RSSI,
	NUMBER_OF_MSG		/* The number of different messages we can have */
} MSG_T;

typedef struct
{
	KRN_POOLLINK;
	MSG_T messageType;		/* What do we need to do with this message */
	TDEV_T *pTuner;			/* Tuner instance */
	unsigned messageFreq;	/* Frequency to tune to if it is a SET_FREQ_MESSAGE */
	TDEV_COMPLETION_FUNC_T CompletionFunc;
	TDEV_RSSI_COMPLETION_FUNC_T rssiCompletionFunc;
	void *CompletionParamater;
} TUNER_MESSAGE_T;

typedef struct
{
	/* Bandwidth we are configured to operate at, in Hz.*/
	unsigned signalBandwidth;
	UCC_STANDARD_T demodStandard;
	/* Bandwidth and modulation in Si2153 DTV_MODE property format */
	unsigned char bw;
	unsigned char modulation;

	L0_Context			l0_context;
	L1_Si2153_Context	l1_context;

	KRN_TASK_T task_tcb;
	unsigned int task_prio;
	unsigned int task_stack[TASK_STACKSIZE];
	TUNER_MESSAGE_T messagePoolDesc[MSG_POOL_SIZE];
	KRN_POOL_T	 messagePool;
	KRN_MAILBOX_T taskMbox;
	int poolEmptyCount;

} SI2153_WORKSPACE_T;

/*! Size in bytes of the workspace that needs to be allocated for this tuner driver. */
#define SI2153_TUNER_DRIVER_WORKSPACE_SIZE	(sizeof(SI2153_WORKSPACE_T))

/*! Exported tuner control structure for SiLabs DTV tuner driver */
extern TDEV_CONFIG_T SI2153Tuner;

#endif
