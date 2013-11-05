/*!
*****************************************************************************

 @file      scpDriver.h
 @brief     SCP driver for UCC320

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "uccrt.h"

#include "scpDriver_DCP.h"


/*
 * Minimum capture / discard length in samples (note zero-length discard is allowed)
 *
 * Note this minimum is for guidance and is determined by the DCP code runtime
 * required to setup the SCP capture job. The reasonably large minimum values
 * are conservative for running on a UCC320 which has various problems causing
 * slow DCP code execution. Much shorted minimums should be possible on more
 * recent UCCs
 */
#define SCP_DRIVER_MINIMUM_DISCARD_LENGTH       (128)
#define SCP_DRIVER_MINIMUM_CAPTURE_LENGTH       (512)


/**
 * Resets usage of an SCP device from an arbitrary state.
 * The device will stop although currently pending tansfers will complete (without posting to the output queues).
 * Any pending jobs will be flushed. The output queues will also be flushed.
 *
 * Note the EDC may be left in an indeterminate state afther this function hence an EDC reset is required before restarting.
 *
 * @param[in]   scp             The SCP_T object whose device needs to be reset.
 * @return                      nothing
 */
void SCP_resetDevice(SCP_T *scp);


/**
 * Restart an SCP device.
 * All pending, completed and active jobs will be flushed and the driver will return to initial state, although
 * The default discard size is unaffected..
 *
 * @param[in]   scp             The SCP_T object whose device needs to be restarted.
 * @return                      nothing
 */
void SCP_restartDevice(SCP_T *scp);


/**
 * Prepare the SCP driver for use.
 * Must be called before using the SCP driver in order to bind the driver to the associated
 * SCP_T object.
 *
 * @param[in]   pipeline               Pointer to the DCP pipeline (for example &DCP_scpPipeline0).
 * @param[in]   useId                  ID of the use device within the pipeline (for example DCP_scpPipeline_scp).
 * @param[in]   imageSet               Register images to associate (for example &RCD_scpLegacyModeDriver_default_images).
 * @param[in]   scp                    The SCP_T object to associate the driver with.
 * @param[in]   defaultDiscardLength   The length to discard when no input job is available
 * @return                             nothing
 */
void SCP_configureUse(DCP_PIPELINE_T  *pipeline,
                      int              useId,
                      RCD_IMAGE_SET_T *imageSet,
                      SCP_T           *scp,
                      unsigned         defaultDiscardLength);


/**
 * Flush input and output job queues of SCP driver.
 *
 * @param[in]   scp             Identifier of SCP whose queues to flush.
 * @return                      nothing
 */
void SCP_flushQueues(SCP_T *scp);


/**
 * Post a job to the SCP driver.
 * This will cause the required length of data to be captured to the specified GRAM address
 * from the next symbol start.
 *
 * @param[in]   scp             Identifier of SCP to capture data from.
 * @param[in]   address         GRAM address (complex view) specifying start of buffer to capture data to.
 * @param[in]   captureLength   Number of samples to capture (minimum length is 9)
 * @param[in]   discardLength   Number of sample to discard.
 * @return                      nothing
 */
void SCP_postJob(SCP_T *scp, uint32_t *address, unsigned int captureLength, unsigned int discardLength);


/**
 * Post multiple jobs to the SCP driver.
 * Note the number of jobs should not be such that the queue becomes full (this is not checked)
 *
 * @param[in]   scp             identifier of SCP to which this job will be posted
 * @param[in]   addresses       An array of GRAM base addresses for SCP data to be written to for each job. Note this array may be modified.
 * @param[in]   captureLengths  An array of capture lengths.
 * @param[in]   discardLengths  An array of discard lengths.
 * @param[in]   nJobs           The number of jobs to post.
 * @return                      nothing
 */
void SCP_postMultipleJobs(SCP_T *scp, uint32_t *addresses[], unsigned int  captureLengths[], unsigned int discardLengths[], unsigned int nJobs);


/**
 * Query how many jobs exist in the output job queue.
 *
 * @param[in]   scp             Identifier of SCP to query.
 * @return                      Number of jobs in output queue
 */
unsigned int SCP_queryJobsCompleted(SCP_T *scp);


/**
 * Read a job from the output job queue.
 *
 * @param[in]   scp             Identifier of SCP to read job from.
 * @param[out]  address         Pointer to value to contain job address.
 * @param[out]  captureLength   Pointer to value to contain job capture length.
 * @param[out]  discardLength   Pointer to value to contain job discard length.
 * @param[out]  iscr            Pointer to value to contain ISCR at job start.
 * @param[in]   waitForJobCompletion    true = block (poll) waiting for job completion, false = return immediately.
 * @return                      true if a job was read, false otherwise.
 *
 * Note that an output parameter will only be written to if a job was read
 * and its associated virtual queue has been connected to a real queue.
 */
bool SCP_readJob(SCP_T *scp, uint32_t **address, unsigned int *captureLength, unsigned int *discardLength, unsigned int *iscr, bool waitForJobCompletion);


/**
 * Set the default discard length of the SCP.
 *
 * @param[in]   scp                    Identifier of SCP to modify
 * @param[in]   defaultDiscardLength   The length to discard when no input job is available
 * @return                             nothing
 */
void SCP_setDefaultDiscardLength(SCP_T *scp, unsigned int defaultDiscardLength);


/**
 * Read the count of default discard jobs scheduled by the SCP driver.
 * Note this is primarily intended for debug purposes
 *
 * @param[in]   scp             The SCP_T object whose count to query
 * @return                      The number of default discard jobs scheduled since last reset
 */
unsigned int SCP_readDefaultDiscardCount(SCP_T *scp);


/**
 * Reset the count of default discard jobs scheduled by the SCP driver.
 * Note this is primarily intended for debug purposes
 *
 * @param[in]   scp             The SCP_T object whose count to reset
 * @return                      nothing
 */
void SCP_resetDefaultDiscardCount(SCP_T *scp);
