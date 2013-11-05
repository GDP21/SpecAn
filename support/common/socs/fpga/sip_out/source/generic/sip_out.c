/*
** FILE NAME:   $RCSfile: sip_out.c,v $
**
** TITLE:       Output to Streaming IP port
**
** PROJECT:		UCCP Common
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Send data out to the streaming IP port.
**				DMA data out to the SIP port provided on our FPGA platforms.
**				This is usually connected to a USB module that allows the data
**				to be captured on a connected PC.
**
**				Copyright (C) 2009, Imagination Technologies Ltd.
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

/* Ensure that we inline the list functions */
#ifndef LST_INLINE
#define LST_INLINE
#endif
#include <meos/lst.h>

#include "sip_out.h"
#include "dmac_api.h"
#include "uccp_driver.h"

#define MAX_NUM_BLOCKS	10

#define DMA_REQUEST_NUM_SIP0_OUT	0

/* SIP 0 start here on Kuroi systems */
#define SIP_0_BASE_ADDRESS 0x00003800

/* Simple listable structure to hold the descriptions of the output jobs */
typedef struct
{
    LST_LINK;
	unsigned char *data;
	int dataSize;
} SIPOUT_JOB_DESC_T;

static LST_T descFreeList;
static SIPOUT_JOB_DESC_T descriptors[MAX_NUM_BLOCKS];
static SIPOUT_JOB_DESC_T *currentJob;

static LST_T JobQueue;

/* Copies of the initialised setup */
static int sysDMAChannelNumber;
static SIPOUT_CALLBACK_FUNCTION_T callbackFunction = NULL;

/* DMA context */
static DMAC_CONTEXT_T DmaOut;


/******************************************************************************
	setupDMA
******************************************************************************/
static void setupDMA(DMAC_CONTEXT_T *pDmaOut)
{
    DMAC_CONFIG_T  dmacConfig;  /* A config object that can be used for all DMAs */

    /* Set up the input DMAC */
    dmacConfig.eMode = DMAC_MODE_BUFFER;
    dmacConfig.rank = 0;
    dmacConfig.triggerNum = 0;
    dmacConfig.LIST_IEN = 0;
    dmacConfig.BSWAP = 0;
    dmacConfig.IEN = 1;
    dmacConfig.INCR = 0;
    dmacConfig.ACC_DEL = 0;
    dmacConfig.perHold = 0x07; /* Current default */
    dmacConfig.DIR = 0;     /* Direction from memory to peripheral */
    dmacConfig.baseAddress = SIP_0_BASE_ADDRESS;
    dmacConfig.PW = 2; /* 8 bit peripheral width */
    dmacConfig.BURST = 1;
    dmacConfig.channel = sysDMAChannelNumber;

    DMAC_init(pDmaOut, NULL, 0, 0, &dmacConfig);
    UCCP_SCR_setDmaChanSel(sysDMAChannelNumber, DMA_REQUEST_NUM_SIP0_OUT);

	return;
}


/******************************************************************************
	StartJob
******************************************************************************/
static void StartJob(void)
{
	/* No need to protect this function from interrupts as it is either called
		from interrupt context or with interrupts already disabled */

	/* If busy can't start a new job */
	if (currentJob != NULL)
		return;

	/* Get new job from queue */
	currentJob = (SIPOUT_JOB_DESC_T *)LST_removeHead(&JobQueue);

	/* If no queued jobs can't start */
	if (currentJob == NULL)
		return;

	/* Start DMA for this job */
	DMAC_upLoadBurst(&DmaOut, (unsigned int)(currentJob->data), (unsigned int)(currentJob->dataSize));
	DMAC_enable(&DmaOut);

	return;
}

/******************************************************************************
	SIPOut_Init
******************************************************************************/
void SIPOut_Init(int DMAChannel, SIPOUT_CALLBACK_FUNCTION_T callback)
{
	int i;

	/* Init internal state */
	currentJob = NULL;
	LST_init(&descFreeList);
	LST_init(&JobQueue);

	/* Queue all descriptors as free */
	for(i=0;i<MAX_NUM_BLOCKS;i++)
	{
		LST_add(&descFreeList, &descriptors[i]);
	}

	/* Copy setup to internal variables */
	sysDMAChannelNumber = DMAChannel;
	callbackFunction = callback;

	setupDMA(&DmaOut);

	return;
}

/******************************************************************************
	SIPOut_Cancel
******************************************************************************/
void SIPOut_Cancel(void)
{
	/* Protect from interrupts */
	int ipl = 0;
	TBI_INTSX(ipl);

	/* Return current job (if one) via callback */
	if (currentJob != NULL)
	{
		/* Stop current job */
		DMAC_reset(&DmaOut);

		if (callbackFunction != NULL)
			callbackFunction(currentJob->data, currentJob->dataSize);

		LST_add(&descFreeList, currentJob);
	}

	/* Issue callback for all currently queued jobs */
	currentJob = (SIPOUT_JOB_DESC_T *)LST_removeHead(&JobQueue);
	while(currentJob != NULL)
	{
		if (callbackFunction != NULL)
			callbackFunction(currentJob->data, currentJob->dataSize);

		LST_add(&descFreeList, currentJob);

		currentJob = (SIPOUT_JOB_DESC_T *)LST_removeHead(&JobQueue);
	}
	/* When we get out of the while loop we know that currentJob is NULL */

	/* Restore interrupt set up */
	TBI_INTSX(ipl);

	return;
}

/******************************************************************************
	SIPOut_DeInit
******************************************************************************/
void SIPOut_DeInit(void)
{
	/* Protect from interrupts */
	int ipl = 0;
	TBI_INTSX(ipl);

	/* Throw out all queued jobs */
	SIPOut_Cancel();

	/* Make sure we can't issue any more callbacks */
	callbackFunction = NULL;

	/* Restore interrupt set up */
	TBI_INTSX(ipl);

	return;
}


/******************************************************************************
	SIPOut_QueueData
******************************************************************************/
int SIPOut_QueueData(unsigned char *dataBlock, int dataBlockSize)
{
	SIPOUT_JOB_DESC_T *descPtr;

	/* Protect from interrupts */
	int ipl = 0;
	TBI_INTSX(ipl);

	/* Allocate a output job descriptor */
	descPtr = (SIPOUT_JOB_DESC_T *)LST_removeHead(&descFreeList);

	/* No descriptors available, so return error */
	if (descPtr == NULL)
	{
		TBI_INTSX(ipl);
		return(-1);
	}

	/* Fill in descriptor with this job and queue it */
	descPtr->data = dataBlock;
	descPtr->dataSize = dataBlockSize;

	LST_add(&JobQueue, descPtr);

	/* Start job (if we can) */
	StartJob();

	/* Restore interrupt set up */
	TBI_INTSX(ipl);

	return(0);
}


/******************************************************************************
	SIPOut_ISR
******************************************************************************/
void SIPOut_ISR(void)
{
	/* No need to protect this function from interrupts as it is called from interrupt context */

	/* When cancelling it is possible to have an outstanding interrupt and
		no current job, so to handle this case check we have a job in currentJob */
	if (currentJob != NULL)
	{
		/* Issue callback for the completed job and free descriptor */
		if (callbackFunction != NULL)
			callbackFunction(currentJob->data, currentJob->dataSize);

		LST_add(&descFreeList, currentJob);
		currentJob = NULL;
	}

	/* Start another job (if we can) */
	StartJob();

	return;
}
