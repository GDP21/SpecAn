/*!
*****************************************************************************

 @file      scpDriver.c
 @brief     SCP driver for UCC320

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "uccrt.h"
#include "scpDriver.h"

/* Second job fragment size - this must match that specified in DCP source file */
#define SECOND_FRAGMENT_SIZE    (256)

/* Configuration of SCP frame counters */
#define SCP_SYMBOLS_PER_FRAME   (1)
#define SCP_SYMBOL_DURATION     (0xFFFF)
/*
** FUNCTION:    SCP_resetDevice()
**
** DESCRIPTION: Resets an SCP device.
**              Any active jobs will be terminated and pending jobs flushed.
**              The output queues will also be flushed.
**
**              Note the EDC may be left in an indeterminate state after this function hence it is not safe to
**              restart the SCP driver without resetting the EDC after stopping the SCP driver.
**
** PARAMETERS:  device - Pointer to the SCP device
**
** RETURNS:     void
*/
void SCP_resetDevice(SCP_T *scp)
{
    if (scp->driver.device)
    {
        /* Set SCP output to TBUS - this is essential to flush any jobs out of EDC during reset */
        SCP_setDestination(scp, SCP_DST_TBUS);

        /* Stop the DCP device */
        DCP_stopDevice(scp->driver.device);

        /* Flush all the queues belonging to the DCP device */
        SCP_flushQueues(scp);
    }
}


/*
** FUNCTION:    SCP_restartDevice()
**
** DESCRIPTION: Restart the SCP driver, flushing and completed, pending and active jobs.
**
** PARAMETERS:  device - Pointer to the SCP device
**
** RETURNS:     void
*/
void SCP_restartDevice(SCP_T *scp)
{
    if (scp->driver.device)
    {
        /* Set SCP output to TBUS - this is essential to flush any jobs out of EDC during reset */
        SCP_setDestination(scp, SCP_DST_TBUS);

        /* Stop the DCP device */
        DCP_stopDevice(scp->driver.device);

        /* Flush all the queues belonging to the DCP device */
        SCP_flushQueues(scp);

        /* Start the DCP device */
        DCP_startDevice(scp->driver.device);

        /* Start a job to cause the SCP driver's use-context code to run */
        DCP_startJobEx(scp->driver.scpPipeline, scp->driver.useId, 0, 0);
    }
}


/*
** FUNCTION:    SCP_flushQueues()
**
** DESCRIPTION: Flush input and output job queues of SCP driver.
**
** PARAMETERS:  scp - Pointer to the SCP device
**
** RETURNS:     void
*/
void SCP_flushQueues(SCP_T *scp)
{
    /* Flush all the queues belonging to the DCP device */
    QM_flush(scp->driver.inAddrQId);
    QM_flush(scp->driver.inCaptureLenQId);
    QM_flush(scp->driver.inDiscardLenQId);
    if (scp->driver.outAddrQId != DCP_INVALID_PARAM)
        QM_flush(scp->driver.outAddrQId);
    if (scp->driver.outIscrQId != DCP_INVALID_PARAM)
        QM_flush(scp->driver.outIscrQId);
    if (scp->driver.outCaptureLenQId != DCP_INVALID_PARAM)
        QM_flush(scp->driver.outCaptureLenQId);
    if (scp->driver.outDiscardLenQId != DCP_INVALID_PARAM)
        QM_flush(scp->driver.outDiscardLenQId);
}


/*
** FUNCTION:    SCP_configureUse
**
** DESCRIPTION: Initialise the SCP pipeline and bind the DCP_DEVICE_T to the SCP_T
**
** PARAMETERS:  pipeline             - DCP pipeline for the SCP
**              useId                - the identifier of this use
**              imageSet             - the register image set for the SCP driver
**              scp                  - SCP_T identifying this SCP
**              defaultDiscardLength - the length to discard when no input job is available
**
** RETURNS:     void
*/
void SCP_configureUse(DCP_PIPELINE_T  *pipeline,
                      int              useId,
                      RCD_IMAGE_SET_T *imageSet,
                      SCP_T           *scp,
                      unsigned int     defaultDiscardLength)
{
	DCP_DATA_T *useData;
	DCP_DATA_T *captureConfigBlock;
	DCP_DATA_T *discardConfigBlock;
	EDC_BASIC_CONFIG_T captureBasicConfig;
	EDC_BASIC_CONFIG_T discardBasicConfig;

	/* Bind the DCP pipeline, use ID and device to the SCP pipeline, use ID and device */
    scp->driver.scpPipeline = pipeline;
    scp->driver.useId = useId;
    scp->driver.device = DCP_getDevice(pipeline, useId);

    /* Get queue indices */
    scp->driver.inAddrQId        = DCP_getUseParam(pipeline, useId, DCP_scpDriver_inputAddressQ);
    scp->driver.inCaptureLenQId  = DCP_getUseParam(pipeline, useId, DCP_scpDriver_inputCaptureLengthQ);
    scp->driver.inDiscardLenQId  = DCP_getUseParam(pipeline, useId, DCP_scpDriver_inputDiscardLengthQ);
    scp->driver.outIscrQId       = DCP_getUseParam(pipeline, useId, DCP_scpDriver_outputIscrQ);
    scp->driver.outAddrQId       = DCP_getUseParam(pipeline, useId, DCP_scpDriver_outputAddressQ);
    scp->driver.outCaptureLenQId = DCP_getUseParam(pipeline, useId, DCP_scpDriver_outputCaptureLengthQ);
    scp->driver.outDiscardLenQId = DCP_getUseParam(pipeline, useId, DCP_scpDriver_outputDiscardLengthQ);

    /* Initialise DCP data blocks */
    DCP_initUseDataBlocks(pipeline, useId, imageSet);

    /* Set frame counters - we need to set a frame of 1 long symbol such that the DCP code can poke this to mimic the SYNCHRONIZE_UPDATES feature of previous UCCs */
    SCP_setFrameSize(scp, SCP_FRAME_A, SCP_SYMBOLS_PER_FRAME, SCP_SYMBOL_DURATION, SCP_SYMBOL_DURATION);

    /* Adjust the DCP data blocks based on the SCP_id that we are using so that the
     * TBUS transfer is linked to the correct SCP hardware.
     */
    useData = DCP_getUseDataBase(pipeline, useId);
    captureConfigBlock = &useData[DCP_scpDriver_captureConfig];
    discardConfigBlock = &useData[DCP_scpDriver_discardConfig];

    EDC_getBasicConfig(captureConfigBlock, &captureBasicConfig);
    EDC_getBasicConfig(discardConfigBlock, &discardBasicConfig);
    captureBasicConfig.pa = captureBasicConfig.pa + SCP_id(scp) - 1;
    discardBasicConfig.pa = discardBasicConfig.pa + SCP_id(scp) - 1;
    EDC_setBasicConfig(captureConfigBlock, &captureBasicConfig);
    EDC_setBasicConfig(discardConfigBlock, &discardBasicConfig);

    switch (SCP_id(scp))
	{
	case 1:
		useData[DCP_scpDriver_scpTbusControl] = (ABS_PMB_SCP_TBUS_CONTROL - ABS_PMB_MCP_PERIP_APC) / 4;
		useData[DCP_scpDriver_scpIscrControl] = (ABS_PMB_SCP_ISCR_CONTROL - ABS_PMB_MCP_PERIP_APC) / 4;
		useData[DCP_scpDriver_scpIscrCounter] = (ABS_PMB_SCP_ISCR_COUNTER0 - ABS_PMB_MCP_PERIP_APC) / 4;
		useData[DCP_scpDriver_scpFinalPeriodA] = (ABS_PMB_SCP_FINAL_PERIOD_A - ABS_PMB_MCP_PERIP_APC) / 4;
		break;
	case 2:
		useData[DCP_scpDriver_scpTbusControl] = (ABS_PMB_SCP2_TBUS_CONTROL - ABS_PMB_MCP_PERIP_APC) / 4;
		useData[DCP_scpDriver_scpIscrControl] = (ABS_PMB_SCP2_ISCR_CONTROL - ABS_PMB_MCP_PERIP_APC) / 4;
		useData[DCP_scpDriver_scpIscrCounter] = (ABS_PMB_SCP2_ISCR_COUNTER0 - ABS_PMB_MCP_PERIP_APC) / 4;
		useData[DCP_scpDriver_scpFinalPeriodA] = (ABS_PMB_SCP2_FINAL_PERIOD_A - ABS_PMB_MCP_PERIP_APC) / 4;
		break;
	case 3:
		useData[DCP_scpDriver_scpTbusControl] = (ABS_PMB_SCP3_TBUS_CONTROL - ABS_PMB_MCP_PERIP_APC) / 4;
		useData[DCP_scpDriver_scpIscrControl] = (ABS_PMB_SCP3_ISCR_CONTROL - ABS_PMB_MCP_PERIP_APC) / 4;
		useData[DCP_scpDriver_scpIscrCounter] = (ABS_PMB_SCP3_ISCR_COUNTER0 - ABS_PMB_MCP_PERIP_APC) / 4;
		useData[DCP_scpDriver_scpFinalPeriodA] = (ABS_PMB_SCP3_FINAL_PERIOD_A - ABS_PMB_MCP_PERIP_APC) / 4;
		break;
	case 4:
		useData[DCP_scpDriver_scpTbusControl] = (ABS_PMB_SCP4_TBUS_CONTROL - ABS_PMB_MCP_PERIP_APC) / 4;
		useData[DCP_scpDriver_scpIscrControl] = (ABS_PMB_SCP4_ISCR_CONTROL - ABS_PMB_MCP_PERIP_APC) / 4;
		useData[DCP_scpDriver_scpIscrCounter] = (ABS_PMB_SCP4_ISCR_COUNTER0 - ABS_PMB_MCP_PERIP_APC) / 4;
		useData[DCP_scpDriver_scpFinalPeriodA] = (ABS_PMB_SCP4_FINAL_PERIOD_A - ABS_PMB_MCP_PERIP_APC) / 4;
		break;
	default:
    	// SCP_id not catered for
    	assert(0);
    }

    /* Initialise discard length and reset discard count */
    SCP_setDefaultDiscardLength(scp, defaultDiscardLength);
    SCP_resetDefaultDiscardCount(scp);
}

/*
** FUNCTION:    SCP_postJob()
**
** DESCRIPTION: Post a new job to the SCP driver.
**
** PARAMETERS:  scp - identifier of SCP to which this job will be posted
**              address - GRAM base address for SCP data to be written to.
**              length - Number of samples to capture.
**              discardLength - Number of samples to discard.
**
** RETURNS:     void
*/
void SCP_postJob(SCP_T        *scp,
                 uint32_t     *address,
                 unsigned int  captureLength,
                 unsigned int  discardLength)
{
    EDC_BUFFER_T gramBuffer;

    /* Check the discard length is bigger than the minimum allowed */
    assert((discardLength == 0) || (discardLength >= SCP_DRIVER_MINIMUM_DISCARD_LENGTH));

    /* Check the capture length and discard length are not both zero  */
    assert(!((discardLength == 0) && (captureLength == 0)));

    /* Obtain GRAM address in complex region for SCP output buffer */
    EDC_alignBuffer(address, captureLength, EDC_GRAM, 0, &gramBuffer);

    QM_postTail(scp->driver.inCaptureLenQId, captureLength);
    QM_postTail(scp->driver.inDiscardLenQId, discardLength);
    QM_postTail(scp->driver.inAddrQId,       gramBuffer.wordOffset);
}


/*
** FUNCTION:    SCP_postMultipleJobs()
**
** DESCRIPTION: Post multiple jobs to the SCP driver.
**              Note the number of jobs should not be such that the queue becomes full (this is not checked)
**
** PARAMETERS:  scp - identifier of SCP to which this job will be posted
**              addresses - An array of GRAM base addresses for SCP data to be written to for each job. Note this array may be modified.
**              captureLengths - An array of capture lengths.
**              discardLengths - An array of discard lengths.
**              nJobs - The number of jobs to post.
**
** RETURNS:     void
*/
void SCP_postMultipleJobs(SCP_T        *scp,
                          uint32_t     *addresses[],
                          unsigned int  captureLengths[],
                          unsigned int  discardLengths[],
                          unsigned int  nJobs)
{
    unsigned int i;
    EDC_BUFFER_T gramBuffer;

    /* Pre-process all addresses and convert to wordOffsets required for posting into the queues */
    for (i = 0; i < nJobs; i++)
    {

        /* Obtain GRAM word offsets for SCP output buffer */
        EDC_alignBuffer(addresses[i], captureLengths[i], EDC_GRAM, 0, &gramBuffer);
        addresses[i] = (uint32_t *)gramBuffer.wordOffset;
    }

    /* Post all jobs to the queues */
    for (i = 0; i < nJobs; i++)
    {
        QM_postTail(scp->driver.inCaptureLenQId, captureLengths[i]);
        QM_postTail(scp->driver.inDiscardLenQId, discardLengths[i]);
        QM_postTail(scp->driver.inAddrQId,       (uint32_t)(addresses[i]));
    }
}


/*
** FUNCTION:    SCP_queryJobsCompleted()
**
** DESCRIPTION: Query how many completed jobs exist in the SCP output queue
**
** PARAMETERS:  scp - identifier of SCP to query
**
** RETURNS:     unsigned int - the number of jobs that exist in the output queue
*/
unsigned int SCP_queryJobsCompleted(SCP_T *scp)
{
    return QueueMon_queryCount(scp->driver.outAddrQId);
}


/*
** FUNCTION:    SCP_readJob()
**
** DESCRIPTION: Read a completed job from the specified SCP and pop that job from the queues
**
** PARAMETERS:  scpDevice - identifier of SCP to read from
**              address - will contain the job start address if a job was read
**              captureLength - will contain the capture job length if a job was read
**              discardLength - will contain the discard job length if a job was read
**              iscr - will contain the ISCR at the capture start if a job was read
**              waitForCompletion - set true to block until a job completes
**
** RETURNS:     bool - true if a job was read, false otherwise
**
** Note that an output parameter will only be written to if its associated
** virtual queue has been connected to a real queue.
*/
bool SCP_readJob(SCP_T         *scp,
                 uint32_t     **address,
                 unsigned int  *captureLength,
                 unsigned int  *discardLength,
                 unsigned int  *iscr,
                 bool           waitForJobCompletion)
{
    if (waitForJobCompletion)
    {
        QueueMon_poll(scp->driver.outAddrQId);
    }
    else
    {
        if (QueueMon_queryCount(scp->driver.outAddrQId)==0)
            return false;
    }

    /* All of the output queues are virtual, so they may not be connected
     * to anything.  For each queue, check whether it is valid, and only
     * output if it is.
     */
    if (scp->driver.outCaptureLenQId != DCP_INVALID_PARAM)
    {
        *captureLength = QM_popHead(scp->driver.outCaptureLenQId);
    }
    if (scp->driver.outDiscardLenQId != DCP_INVALID_PARAM)
    {
        *discardLength = QM_popHead(scp->driver.outDiscardLenQId);
    }
    if (scp->driver.outIscrQId != DCP_INVALID_PARAM)
    {
        *iscr = QM_popHead(scp->driver.outIscrQId);
    }
    if (scp->driver.outAddrQId != DCP_INVALID_PARAM)
    {
        *address = UCC_GRAM_OFFSET_TO_CPX_ADDR(QM_popHead(scp->driver.outAddrQId));
    }

    return true;
}


/*
** FUNCTION:    SCP_setDefaultDiscardLength()
**
** DESCRIPTION: Modify the default discard size that the SCP uses when no capture jobs are present
**
** PARAMETERS:  scp                  - Pointer to the SCP device
**              defaultDiscardLength - The length to discard when no input job is available
**
** RETURNS:     void
*/
void SCP_setDefaultDiscardLength(SCP_T *scp, unsigned int defaultDiscardLength)
{
    DCP_DATA_T *data = DCP_getUseDataBase(scp->driver.scpPipeline, scp->driver.useId);
    data[DCP_scpDriver_defaultDiscardLengthFragment1] = defaultDiscardLength - SECOND_FRAGMENT_SIZE;
}


/*
** FUNCTION:    SCP_readDefaultDiscardCount()
**
** DESCRIPTION: Read the count of default discard jobs scheduled by the SCP driver.
**              Note this is primarily intended for debug purposes
**
** PARAMETERS:  scp - identifier of SCP to query
**
** RETURNS:     void
*/
unsigned int SCP_readDefaultDiscardCount(SCP_T *scp)
{
    DCP_DATA_T *data = DCP_getUseDataBase(scp->driver.scpPipeline, scp->driver.useId);
    return data[DCP_scpDriver_defaultDiscardCount];
}


/*
** FUNCTION:    SCP_resetDefaultDiscardCount()
**
** DESCRIPTION: Reset the count of default discard jobs scheduled by the SCP driver to 0
**              Note this is primarily intended for debug purposes
**
** PARAMETERS:  scp - identifier of SCP whose count to reset
**
** RETURNS:     void
*/
void SCP_resetDefaultDiscardCount(SCP_T *scp)
{
    DCP_DATA_T *data = DCP_getUseDataBase(scp->driver.scpPipeline, scp->driver.useId);
    data[DCP_scpDriver_defaultDiscardCount] = 0;
}
