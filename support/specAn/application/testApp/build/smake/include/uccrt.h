/*!
 *****************************************************************************

 @file      uccrt.h
 @brief     UCC Runtime top level header file

 Copyright (c) Imagination Technologies Limited. 2010

 ****************************************************************************/

#ifndef _UCCRT_H_
#define _UCCRT_H_

/*-------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
/* Preliminaries - these definitions are used in more than one sub-include */

/**
 * SCP sample format selections
 */
/* Don't change these values - they are used directly as register fields */
typedef enum
{
    /** Two's complement */
    SCP_SAMPLE_2SCOMP = 0,
    /** Unsigned (offset binary) */
    SCP_SAMPLE_UNSIGNED = 1
} SCP_SAMPLE_FORMAT_T;
/*                        End of preliminaries                            */
//TODO - move nested includes into a subdir
//TODO - add prefix to nested include protection macros to minimise chance of name clashes with users
#include "uccStandards.h"
#include "uccLow.h"
#include "uccPdump.h"
#include "uccUtil.h"
#include "uccMem.h"
#include "dcpTypes.h"
#include "rcdTypes.h"
#include "rcd.h"
#include "edcConfigDefs.h"
#include "tbusPorts.h"
#include "efsDefs.h"
#include "scp.h"
#include "mcp.h"
#include "tdev.h"
#include "tdev16.h"
#include "tuner.h"
#include "tuner16.h"
#include "uccint.h"
#include "uccCompat.h"
#include "virtualregister.h"

/**
 * Define primitive interrupt management operations - these need to be done differently depending on
 * combinations of MTP/ATP interrupt system and MEOS or not
 */

#if defined(__MEOS__)

#define UCC_DISABLE_INTS() KRN_raiseIPL();
#define UCC_RESTORE_INTS(OLDIPL) KRN_restoreIPL(OLDIPL)
/* Discourage direct manipulation of TXMASKI */
#undef TBI_INTSX
#undef TBI_INTSOR
#undef TBI_INTSAND
#define TBI_INTSX(X) Direct_manipulation_of_interrupt_mask_not_permitted_when_using_MeOS
#define TBI_INTSOR(X) Direct_manipulation_of_interrupt_mask_not_permitted_when_using_MeOS
#define TBI_INTSAND(X) Direct_manipulation_of_interrupt_mask_not_permitted_when_using_MeOS
#else
#if defined(KRN_raiseIPL)
#undef KRN_raiseIPL
#endif
#if defined(KRN_restoreIPL)
#undef KRN_restoreIPL
#endif
#define KRN_raiseIPL() Can_not_use_KRN_raiseIPL_when_not_using_MeOS
#define KRN_restoreIPL(X) Can_not_use_KRN_restoreIPL_when_not_using_MeOS
#if defined(__META_ATP_INTERRUPTS__)

#if defined(__META_MTP_INTERRUPTS__)
#error "Both __META_ATP_INTERRUPTS and __META_MTP_INTERRUPTS__ are defined"
#else
/* define Non-MeOS ATP interrupt primitives */
inline static int UCC_DISABLE_INTS(void) {int oldipl=0; TBI_INTSX(oldipl); return oldipl;}
inline static void UCC_RESTORE_INTS(int oldipl) {TBI_INTSX(oldipl);}
#endif

#elif defined(__META_MTP_INTERRUPTS__)

#if defined(__META_ATP_INTERRUPTS__)
#error "Both __META_ATP_INTERRUPTS and __META_MTP_INTERRUPTS__ are defined"
#else
/* define non-MeOS MTP interrupt primitives */
#error "non-MeOS MTP interrupts not supported"
#endif

#else

#warning "Assuming Meta ATP interrupts. Define __META_ATP_INTERRUPTS__ or __MEOS__ to clear this warning"
/* defining non-MeOS ATP interrupt primitives (same as above) */
inline static int UCC_DISABLE_INTS(void) {int oldipl=0; TBI_INTSX(oldipl); return oldipl;}
#define UCC_RESTORE_INTS(OLDIPL) TBI_INTSX(OLDIPL)
#endif

#endif
/*-------------------------------------------------------------------------*/

/**
 * Type for representing absolute GRAM addresses
 */
typedef uint32_t UCCP_GRAM_ADDRESS_T;

/*
 * Types to represent GRAM views.
 */

/** Double view */
typedef uint32_t GRAM_DBL_T;

/** Sign-extended view */
typedef uint32_t GRAM_SXT_T;

/** Complex view */
typedef uint32_t GRAM_CPX_T;

/** Packed view */
typedef uint8_t GRAM_PKD_T;

/*-------------------------------------------------------------------------*/

/*
 * DCP functions.
 */

/** Special job number that means jobs are dynamically generated.
 *  (e.g. MCPOS jobs).
 */
#define DCP_DYN_JOB_NUM (255)

/** DCP data blocks are always accessed through the SXT GRAM view */
typedef GRAM_SXT_T DCP_DATA_T;

/**
 * Loads a DCP image.
 *
 * @param[in]      image  The DCP image to load
 * @param[in,out]  dest   The destination address to copy the image to. Can
 *                        be left NULL, in which case no copying will be
 *                        done (only applicable if the image has already been
 *                        placed in GRAM). Otherwise, the destination address
 *                        must be at a GRAM word aligned address, specified
 *                        through any of the four views.
 */
void
DCP_load(DCP_IMAGE_T *image, void *dest);

/**
 * Unloads the current DCP image.
 */
void
DCP_unload(void);

/**
 * Returns the "loaded" status of the DCP image.
 *
 * @return true if the a DCP image is loaded; false otherwise.
 */

bool
DCP_isImageLoaded(void);

/**
 * Starts a DCP group.
 *
 * @param[in] group  Pointer to the DCP group to start
 */
void
DCP_startGroup(DCP_GROUP_T *group);

/**
 * Stops a DCP group.
 *
 * @param[in] group  Pointer to the DCP group to stop
 */
void
DCP_stopGroup(DCP_GROUP_T *group);

/**
 * Starts a DCP device.
 *
 * @param[in] device  Pointer to the DCP device to start
 */
void
DCP_startDevice(DCP_DEVICE_T *device);

/**
 * Stops a DCP device.
 *
 * @param[in] device  Pointer to the DCP device to stop
 */
void
DCP_stopDevice(DCP_DEVICE_T *device);

/**
 * Starts all DCP devices.
 */
void
DCP_startAllDevices(void);

/**
 * Stops all DCP devices.
 */
void
DCP_stopAllDevices(void);

/**
 * Returns a pointer to the pipeline at a specified index in the current DCP image structure.
 *
 * @param[in]   index   The index of the pipeline
 * @return              A pointer to the DCP pipeline structure
 */
DCP_PIPELINE_T *
DCP_getImagePipeline(int index);

/**
 * Returns a pointer to the device group at a specified index in the current DCP image structure.
 *
 * @param[in]   index   The index of the group
 * @return              A pointer to the DCP group structure
 */
DCP_GROUP_T *
DCP_getImageGroup(int index);

/**
 * Returns a pointer to the device at a specified index in the current DCP image structure.
 *
 * @param[in]   index   The index of the device
 * @return              A pointer to the DCP device structure
 */
DCP_DEVICE_T *
DCP_getImageDevice(int index);

/**
 * Gets the address of a job-context data block.
 *
 * @param[in]   pipeline         Pointer to the DCP pipeline
 * @param[in]   useId            ID of the use device within the pipeline
 * @param[in]   dataBlockOffset  Offset of the data block
 * @param[in]   jobNum           The job number
 * @return                       The address of the data block
 */
DCP_DATA_T *
DCP_getJobDataBlock(DCP_PIPELINE_T *pipeline, int useId, int dataBlockOffset,
                    int jobNum);

/**
 * Gets the base of job-context data blocks for a specified job number.
 *
 * @param[in]  pipeline  Pointer to the DCP pipeline
 * @param[in]  useId     ID of the use device within the pipeline
 * @param[in]  jobNum    The job number
 * @return               The base address of use-context data blocks
 */
#define DCP_getJobDataBase(pipeline, useId, jobNum) DCP_getJobDataBlock(pipeline, useId, 0, jobNum)

/**
 * Gets the base of use-context data blocks.
 *
 * @param[in]  pipeline  Pointer to the DCP pipeline
 * @param[in]  useId     ID of the use device within the pipeline
 * @return               The base address of use-context data blocks
 */
#define DCP_getUseDataBase(pipeline, useId) DCP_getUseDataBlock(pipeline, useId, 0)

/**
 * Gets the address of a use-context data block.
 *
 * @param[in]   pipeline         Pointer to the DCP pipeline
 * @param[in]   useId            ID of the use device within the pipeline
 * @param[in]   dataBlockOffset  Offset of the data block
 * @return                       The address of the data block
 */
DCP_DATA_T *
DCP_getUseDataBlock(DCP_PIPELINE_T *pipeline, int useId, int dataBlockOffset);

/**
 * Gets the base of device-context data blocks.
 *
 * @param[in]   device  Pointer to the DCP device
 * @return              The base address of device-context data blocks
 */
#define DCP_getDeviceDataBase(device) DCP_getDeviceDataBlock(device, 0)

/**
 * Gets the address of a device-context data block.
 *
 * @param[in]   device           Pointer to the DCP device
 * @param[in]   dataBlockOffset  Offset of the data block
 * @return                       The address of the data block
 */
DCP_DATA_T *
DCP_getDeviceDataBlock(DCP_DEVICE_T *device, int dataBlockOffset);

/**
 * Gets the value of a use-context parameter.
 *
 * @param[in]  pipeline    Pointer to the DCP pipeline
 * @param[in]  useId       ID of the use device within the pipeline
 * @param[in]  useParamId  ID of the parameter
 * @return                 The ID of the use scope parameter
 */
DCP_PARAM_ID_T
DCP_getUseParam(DCP_PIPELINE_T *pipeline, int useId, int useParamId);

/**
 * Gets the value of a device-context parameter.
 *
 * @param[in]  device         Pointer to the DCP device
 * @param[in]  deviceParamId  ID of the parameter
 * @return                    The ID of the device scope parameter
 */
DCP_PARAM_ID_T
DCP_getDeviceParam(DCP_DEVICE_T *device, int deviceParamId);

/**
 * Gets a DCP device from a pipeline.
 *
 * @param[in]  pipeline  Pointer to the DCP pipeline
 * @param[in]  useId     ID of the use device within the pipeline
 * @return               The DCP device
 */
DCP_DEVICE_T *
DCP_getDevice(DCP_PIPELINE_T *pipeline, int useId);

/**
 * Gets a DCP channel from a device.
 *
 * @param[in]  device     Pointer to the DCP device
 * @param[in]  channelId  Logical ID of the channel
 * @return                The DCP channel
 */
DCP_CHANNEL_T *
DCP_getChannel(DCP_DEVICE_T *device, int channelId);

/**
 * Fills a data block.
 *
 * @param[in]  block      Pointer to the block
 * @param[in]  fillValue  Value to fill with
 * @param[in]  length     Length of the data block in words.
 */
void
DCP_fillDataBlock(DCP_DATA_T *block, DCP_DATA_T fillValue, int length);

/**
 * Fills a job-context data block.
 *
 * @param[in]  pipeline         Pointer to the DCP pipeline
 * @param[in]  useId            ID of the use device within the pipeline
 * @param[in]  dataBlockOffset  Offset of the data block
 * @param[in]  dataBlockLength  Length of the data block in words
 * @param[in]  jobNum           The job number
 * @param[in]  fillValue        Value to fill with
 */
void
DCP_fillJobDataBlockEx(DCP_PIPELINE_T *pipeline, int useId, int dataBlockOffset,
                       int dataBlockLength, int jobNum, DCP_DATA_T fillValue);

/**
 * Fills a use-context data block.
 *
 * @param[in]  pipeline         Pointer to the DCP pipeline
 * @param[in]  useId            ID of the use device within the pipeline
 * @param[in]  dataBlockOffset  Offset of the data block
 * @param[in]  dataBlockLength  Length of the data block in words
 * @param[in]  fillValue        Value to fill with
 */
void
DCP_fillUseDataBlockEx(DCP_PIPELINE_T *pipeline, int useId, int dataBlockOffset,
                       int dataBlockLength, DCP_DATA_T fillValue);

/**
 * Fills a device-context data block.
 *
 * @param[in]  device           Pointer to the DCP device
 * @param[in]  dataBlockOffset  Offset of the data block
 * @param[in]  dataBlockLength  Length of the data block in words
 * @param[in]  fillValue        Value to fill with
 */
void
DCP_fillDeviceDataBlockEx(DCP_DEVICE_T *device, int dataBlockOffset,
                          int dataBlockLength, DCP_DATA_T fillValue);

/**
 * Fills a job-context data block.
 *
 * @param[in]  pipeline         Pointer to the DCP pipeline
 * @param[in]  useId            ID of the use device within the pipeline
 * @param[in]  dataBlockOffset  Offset of the data block
 * @param[in]  jobNum           The job number
 * @param[in]  fillValue        Value to fill with
 */
#define DCP_fillJobDataBlock(pipeline, useId, dataBlockOffset, jobNum, fillValue) DCP_fillJobDataBlockEx(pipeline, useId, dataBlockOffset, dataBlockOffset##_length, jobNum, fillValue)

/**
 * Fills a use-context data block.
 *
 * @param[in]  pipeline         Pointer to the DCP pipeline
 * @param[in]  useId            ID of the use device within the pipeline
 * @param[in]  dataBlockOffset  Offset of the data block
 * @param[in]  fillValue        Value to fill with
 */
#define DCP_fillUseDataBlock(pipeline, useId, dataBlockOffset, fillValue) DCP_fillUseDataBlockEx(pipeline, useId, dataBlockOffset, dataBlockOffset##_length, fillValue)

/**
 * Fills a device-context data block.
 *
 * @param[in]  device           Pointer to the DCP device
 * @param[in]  dataBlockOffset  Offset of the data block
 * @param[in]  fillValue        Value to fill with
 */
#define DCP_fillDeviceDataBlock(device, dataBlockOffset, fillValue) DCP_fillDeviceDataBlockEx(device, dataBlockOffset, dataBlockOffset##_length, fillValue)

/**
 * Sets up channel linking. Original version where the destination channel
 * number is obtained from the DCP linker generated information.
 *
 * @param[in]      pipeline   Pointer to the DCP pipeline
 * @param[in]      useId      ID of the block within the pipeline
 * @param[in]      channelId  Logical ID of the channel
 * @param[in,out]  dataBlock  Pointer to the data block to setup
 */
void
DCP_setupChannelLinking(DCP_PIPELINE_T *pipeline, int useId, int channelId,
                        DCP_DATA_T *dataBlock);

/* Sets up channel linking. Extended version where the logical ID of the
 * destination channel is passed in.
 *
 * @param[in]      pipeline       Pointer to the DCP pipeline
 * @param[in]      useId          ID of the block within the pipeline
 * @param[in]      destChannelId  Logical ID of the destination channel to
 *                                link to
 * @param[in,out]  dataBlock      Pointer to the data block to setup
 */
void DCP_setupChannelLinkingEx(DCP_PIPELINE_T *pipeline, int useId,
                               int destChannelId, DCP_DATA_T *dataBlock);

/**
 * Sets up PA for the case where one channel is feeding offsets into another
 * channel.
 *
 * @param[in]      pipeline       Pointer to the DCP pipeline
 * @param[in]      useId          ID of the block within the pipeline
 * @param[in]      destChannelId  Logical ID of the destination channel
 *                                (the channel to which offsets will be fed)
 * @param[in,out]  dataBlock      Pointer to the data block to setup
 */
void
DCP_setupPaLinking(DCP_PIPELINE_T *pipeline, int useId, int destChannelId,
                   DCP_DATA_T *dataBlock);

/**
 * Initialises DCP device data blocks from a register image.
 *
 * @param[in]  device    The device which contains the blocks to initialise
 * @param[in]  imageSet  The register image set
 */
void
DCP_initDeviceDataBlocks(DCP_DEVICE_T *device, RCD_IMAGE_SET_T *imageSet);

/**
 * Initialises DCP use data blocks from a register image.
 *
 * @param[in]  pipeline  The pipeline which contains the blocks to initialise
 * @param[in]  useId     The use index of the device to initialise
 * @param[in]  imageSet  The register image set
 */
void
DCP_initUseDataBlocks(DCP_PIPELINE_T *pipeline, int useId,
                      RCD_IMAGE_SET_T *imageSet);

/**
 * Initialises DCP job data blocks from a register image.
 *
 * @param[in]  pipeline  The pipeline which contains the blocks to initialise
 * @param[in]  useId     The use index of the device to initialise
 * @param[in]  jobNum    The job number of the blocks to initialise
 * @param[in]  imageSet  The register image set
 */
void
DCP_initJobDataBlocks(DCP_PIPELINE_T *pipeline, int useId, int jobNum,
                      RCD_IMAGE_SET_T *imageSet);

/**
 * Initialises DCP job data blocks from a register image for all jobs in
 * a pipeline.
 *
 * @param[in]  pipeline  The pipeline which contains the blocks to initialise
 * @param[in]  useId     The use index of the device to initialise
 * @param[in]  imageSet  The register image set
 */
void
DCP_initAllJobDataBlocks(DCP_PIPELINE_T *pipeline, int useId,
                         RCD_IMAGE_SET_T *imageSet);

/**
 * Returns a job ID.
 *
 * @param[in]  pipeline  Pointer to the DCP pipeline
 * @param[in]  useId     The use ID
 * @param[in]  jobNum    The job number
 * @return               The job ID
 */
uint32_t
DCP_getJobId(DCP_PIPELINE_T *pipeline, int useId, int jobNum);

/**
 * Returns the identifier of the first use (device instance) in a pipeline.
 *
 * @param[in]  pipeline  Pointer to the DCP pipeline
 * @return               The identifier of the use
 */
#define DCP_getFirstUse(pipeline) ((pipeline)->firstUse)

/**
 * Returns the multiplicity of the first use (device instance) in a pipeline.
 *
 * @param[in]  pipeline  Pointer to the DCP pipeline
 * @return               The multiplicity of the use
 */
#define DCP_getNumJobs(pipeline) \
        DCP_getNumJobsEx(pipeline, DCP_getFirstUse(pipeline))
/**
 * Returns the multiplicity of a use (device instance) within a pipeline.
 *
 * @param[in]  pipeline  Pointer to the DCP pipeline
 * @param[in]  useId     The ID of the use within the pipeline
 * @return               The multiplicity of the use
 */
int
DCP_getNumJobsEx(DCP_PIPELINE_T *pipeline, int useId);

/**
 * Returns the identifier of the job queue of the first use (device instance)
 * in a specified pipeline.
 *
 * @param[in]  pipeline  Pointer to the DCP pipeline
 * @return               The identifier of the job queue of the first device
 *                       instance
 */
#define DCP_getJobQueue(pipeline) \
        DCP_getJobQueueEx(pipeline, DCP_getFirstUse(pipeline))

/**
 * Returns the identifier of the job queue of a specified use (device
 * instance) in a pipeline.
 *
 * @param[in]  pipeline  Pointer to the DCP pipeline
 * @param[in]  useId     The identifier of the device instance within the
 *                       pipeline
 * @return               The identifier of the job queue of the device instance
 */
int
DCP_getJobQueueEx(DCP_PIPELINE_T *pipeline, int useId);

/**
 * Returns the depth of the job queue of a specified use (device instance) in
 * a pipeline.
 *
 * @param[in]  pipeline  Pointer to the DCP pipeline
 * @param[in]  useId     The identifier of the device instance within the
 *                       pipeline
 * @return               The depth of the job queue of the device instance
 */
int
DCP_getJobQueueDepth(DCP_PIPELINE_T *pipeline, int useId);

/**
 * Returns the address of the job code of the first use (device instance) in
 * a pipeline.
 *
 * @param[in]  pipeline  Pointer to the DCP pipeline
 * @param[in]  jobNum    The job number
 * @return               The job address
 */
#define DCP_getJobAddr(pipeline, jobNum) \
        DCP_getJobAddrEx(pipeline, DCP_getFirstUse(pipeline), 0, jobNum)

/**
 * Returns the address of the job code of a specified use (device instance)
 * in a pipeline.
 *
 * @param[in]  pipeline    Pointer to the DCP pipeline
 * @param[in]  useId       The identifier of the device instance within the
 *                         pipeline
 * @param[in]  labelIndex  The index of the code label in the job code block
 * @param[in]  jobNum      The job number
 * @return                 The job address
 */
uint32_t
DCP_getJobAddrEx(DCP_PIPELINE_T *pipeline, int useId, int labelIndex,
                 int jobNum);

/**
 * Starts a job on the first use (device instance) of a pipeline.
 *
 * @param[in]  pipeline  Pointer to the DCP pipeline
 * @param[in]  jobNum    The job number to start
 */
#define DCP_startJob(pipeline, jobNum) \
        DCP_startJobEx(pipeline, DCP_getFirstUse(pipeline), 0, jobNum)

/**
 * Starts a job on the specified use (device instance) of a pipeline.
 *
 * @param[in]  pipeline    Pointer to the DCP pipeline
 * @param[in]  useId       The identifier of the device instance within the
 *                         pipeline
 * @param[in]  labelIndex  The index of the code label in the job code block
 * @param[in]  jobNum      The job number to start
 */
void
DCP_startJobEx(DCP_PIPELINE_T *pipeline, int useId, int labelIndex, int jobNum);

/**
 * Returns the base address of the current DCP code image.
 *
 * @return  The base address of the current DCP code image
 */
uint32_t
DCP_getImageBaseCodeView(void);

/**
 * Queries which pipeline and job has just completed.
 *
 * @param[out] jobNum  Updated with the completed job number.
 *                     This parameter can be left NULL, in which case it will
 *                     not be written to.
 * @param[out] useId   Updated with the completed use ID.
 *                     This parameter can be left NULL, in which case it will
 *                     not be written to.
 * @return             Pointer to the completed pipeline. NULL is returned if
 *                     no pipeline has completed.
 */
DCP_PIPELINE_T *
DCP_query(int *jobNum, int *useId);

/**
 * Polls for any job completing.
 *
 * @param[out] jobNum  Updated with the completed job number.
 *                     This parameter can be left NULL, in which case it will
 *                     not be written to.
 * @param[out] useId   Updated with the completed use ID.
 *                     This parameter can be left NULL, in which case it will
 *                     not be written to.
 * @return             Pointer to the completed pipeline.
 */
DCP_PIPELINE_T *
DCP_poll(int *jobNum, int *useId);

/**
 * Prototype for a DCP completion handler.
 *
 * @param[in] pipeline  The pipeline that has completed.
 * @param[in] context   User-specified context.
 * @param[in] jobNum    The job number of the completed job.
 * @param[in] useId     The ID of where within the pipeline the job was
 *                      completed from.
 */
typedef void
DCP_COMPLETION_HANDLER_T(DCP_PIPELINE_T *pipeline, void *context, int jobNum,
                         int useId);

/**
 * Installs/removes a DCP pipeline completion handler on a specified
 * pipeline.
 *
 * Removal of the current handler is achieved by setting the handler argument
 * to NULL. Otherwise, the specified handler will be installed.
 *
 * When installing, there must NOT be a handler currently installed.
 * When removing, there MUST be a handler currently installed.
 *
 * @param[in] pipeline  The pipeline on which to install/remove the
 *                      completion handler.
 * @param[in] handler   The handler to install. Set to NULL to remove the
 *                      currently installed handler.
 * @param[in] context   Handler specific context.
 */
void
DCP_installCompletionHandler(DCP_PIPELINE_T *pipeline,
                             DCP_COMPLETION_HANDLER_T *handler, void *context);

/*-------------------------------------------------------------------------*/

/*
 * Register Configuration Database (RCD) functions.
 */

/**
 * Loads registers from a register image set structure. This structure is
 * output by the register formatter tool.
 *
 * @param[in]  imageSet  The set of register images to load.
 */
void
RCD_load(RCD_IMAGE_SET_T *imageSet);

/**
 * Gets an individual register image of type 'device'
 *
 * @param[in]  imageSet  The set of images
 * @param[in]  imageId   The ID of the image to get
 */
RCD_IMAGE_DEVICE_T *
RCD_getDeviceImage(RCD_IMAGE_SET_T *imageSet, int imageId);

/**
 * Gets an individual register image of type 'device serial'
 *
 * @param[in]  imageSet  The set of images
 * @param[in]  imageId   The ID of the image to get
 */
RCD_IMAGE_DEVICE_SERIAL_T *
RCD_getDeviceSerialImage(RCD_IMAGE_SET_T *imageSet, int imageId);

/**
 * Gets an individual register image of type 'DCP data block'
 *
 * @param[in]  imageSet  The set of images
 * @param[in]  imageId   The ID of the image to get
 */
RCD_IMAGE_DCP_DATA_T *
RCD_getDcpDataBlockImage(RCD_IMAGE_SET_T *imageSet, int imageId);

/**
 * Gets an individual register image of type 'table'
 *
 * @param[in]  imageSet  The set of images
 * @param[in]  imageId   The ID of the image to get
 * @param[out] data      Updated with a pointer to the table's data, which
 *                       will always be in the GRAM sign-extended view.
 */
RCD_IMAGE_TABLE_T *
RCD_getTableImage(RCD_IMAGE_SET_T *imageSet, int imageId,
                  RCD_IMAGE_DATA32_T **data);

/*!
 * Obtain the contents of a particular register from within a peripheral configuration image containing
 * that register.\n\n
 * @param[in]	imagesDescriptor	Pointer to an image descriptor structure for the peripheral
 * 									configuration, as produced by the register formatter and declared
 * 									in rcdOutput.h.
 * @param[in]	regAddr				Absolute address of the register of interest, within Meta's address map
 * @param[out]	regContents			Output variable holding the register contents as obtained from the image.
 * 									The 24-bit register contents are presented LSB-aligned in the output word.
 * @param[in]	imageIndex			Index for use when there may be multiple images within imagesDescriptor.
 * 									Normally set to 0.
 * @return							Returns 0 on success, -1 for imageIndex out of range, 1 for register not
 * 									found within the image.
*/
int RCD_getRegFromDeviceImage(	RCD_IMAGE_SET_T *imagesDescriptor,
								unsigned regAddr,
								unsigned *regContents,
								int imageIndex);

/**
 * Initialises an array of image sets for use. If NULL is passed in, it will
 * attempt to initialise the default global array of image sets.
 *
 * @param[in]	imageSets			An array of image sets
 *
 */
void RCD_init(RCD_IMAGE_SET_T *imageSets[]);

/*-------------------------------------------------------------------------*/

/*
 * EDC Configuration functions.
 */

/** Structure to describe an EDC buffer */
typedef struct edc_buffer_t
{
    /** The type of memory in which the buffer resides */
    EDC_MEM_TYPE_T type;

    /** Byte address of the buffer.
     *  For GRAM buffers this will be in the packed view. Don't use
     *  GRAM_PKD_T explicitly, because that wouldn't seem right for EXTRAM
     *  buffers, but the type below should match this.
     */
    uint8_t *addr;

    /** Length of the buffer in bytes */
    uint32_t len;

    /** Byte address of the buffer, but in a view where this address is
     *  guarenteed to be 32-bit aligned. For EXTRAM buffers this will be the
     *  same as addr, but for GRAM buffers it will use one of the 32-bit
     *  views.
     */
    uint32_t *addr32;

    /** Word offset of the buffer. Here word refers to the size of the
     *  specified alignment.
     */
    uint32_t wordOffset;

    /** Length of the buffer in words */
    uint32_t wordLen;

    /** The byte alignment of the buffer */
    int alignment;

    /** For GRAM buffers, the addresses of the buffer as seen through the
     *  double, single-extended and complex views. For an external RAM buffer
     *  these fields will be set to NULL.
     */
    GRAM_DBL_T *dblAddr;
    GRAM_SXT_T *sxtAddr;
    GRAM_CPX_T *cpxAddr;

    /** For GRAM buffers only, the length of the buffer in 24-bit words. For
     *  external RAM buffers, this field will be set to 0.
     */
    uint32_t dblLen;
} EDC_BUFFER_T;

/**
 * Acknowledge the presence of an EDC buffer from an input point of view.
 *
 * @param[in]  buffer  The buffer to acknowledge
 */
#define EDC_BUFFER_IN(buffer) UCC_IN32((buffer)->addr32, (buffer)->wordLen)

/**
 * Acknowledge the presence of an EDC buffer from an output point of view.
 *
 * @param[in]  buffer  The buffer to acknowledge
 */
#define EDC_BUFFER_OUT(buffer) UCC_OUT32((buffer)->addr32, (buffer)->wordLen)

/**
 * Writes basic configuration into a DMA configuration block.
 *
 * @param[out] block   DMA configuration block
 * @param[in]  config  Basic configuration
 */
void
EDC_setBasicConfig(DCP_DATA_T *block, EDC_BASIC_CONFIG_T *config);

/**
 * Reads basic configuration from a DMA configuration block.
 *
 * @param[in]  block   DMA configuration block
 * @param[out] config  Basic configuration
 */
void
EDC_getBasicConfig(DCP_DATA_T *block, EDC_BASIC_CONFIG_T *config);

/**
 * Writes address generation configuration into a DMA configuration block.
 *
 * @param[out] block     DMA configuration block
 * @param[in]  config    Address generation configuration
 * @param[in]  clearAbo  true to clear address generator context
 */
void
EDC_setAgenConfig(DCP_DATA_T *block, EDC_AGEN_CONFIG_T *config, bool clearAbo);

/**
 * Reads address generation configuration from a DMA configuration block.
 *
 * @param[in]  block   DMA configuration block
 * @param[out] config  Address generation configuration
 */
void
EDC_getAgenConfig(DCP_DATA_T *block, EDC_AGEN_CONFIG_T *config);

/**
 * Writes the length field of a DMA configuration block.
 *
 * @param[out] block   DMA configuration block
 * @param[in]  length  Length information
 */
void
EDC_setLength(DCP_DATA_T *block, EDC_LENGTH_T *length);

/**
 * Reads the length field of a DMA configuration block.
 *
 * @param[in]  block   DMA configuration block
 * @param[out] length  Length information
 */
void
EDC_getLength(DCP_DATA_T *block, EDC_LENGTH_T *length);

/**
 * Writes the MA field of a DMA configuration block.
 *
 * @param[out] block   DMA configuration block
 * @param[in]  ma      MA value
 */
void
EDC_setMA(DCP_DATA_T *block, uint32_t ma);

/**
 * Reads the MA field of a DMA configuration block.
 *
 * @param[in]  block   DMA configuration block
 * @return     ma      MA value
 */
uint32_t
EDC_getMA(DCP_DATA_T *block);

/**
 * Aligns a buffer.
 *
 * @param[in]  addr                 Base address of the unaligned buffer
 *                                  space.
 * @param[in]  len                  Length in bytes of the unaligned buffer
 *                                  space. For GRAM, this is as seen in the
 *                                  logical address view pointed at by
 *                                  \c addr.
 *                                  So for non-packed GRAM views, the length
 *                                  is: UCC_PKD_TO_DBL_LEN(physicalSize).
 * @param[in]  type                 The memory type.
 * @param[in]  alignmentMultiplier  An additional multiplier by which to
 *                                  increase the alignment requirement.
 *                                  Specifying a multiplier of 0 is treated
 *                                  as a multiplier of 1.
 * @param[out] buffer               Pointer to a buffer descriptor, which is
 *                                  updated with information about the
 *                                  aligned buffer.
 */
void
EDC_alignBuffer(void *addr, uint32_t len, EDC_MEM_TYPE_T type,
                int alignmentMultiplier, EDC_BUFFER_T *buffer);

/**
 * Clears an EDC trace buffer.
 *
 * @param[in] traceBuffer  A pointer to the trace buffer that is to be cleared.
 */
void
EDC_clearTraceBuffer(EDC_BUFFER_T *traceBuffer);

/**
 * Configure the EDC trace buffer.
 *
 * @param[in]  traceBufferSpace     Unaligned trace buffer space
 * @param[in]  traceBufferSpaceLen  Length in bytes of the unaligned trace
 *                                  buffer.
 * @param[in]  timestampPeriod      The timestamp period in core clocks.
 *                                  If this value is zero, then timestamps
 *                                  are disabled.
 * @param[out] traceBuffer          The aligned trace buffer.
 */
void
EDC_configTraceBuffer(void *traceBufferSpace, uint32_t traceBufferSpaceLen,
                      unsigned int timestampPeriod, EDC_BUFFER_T *traceBuffer);

/**
 * Gets the current trace buffer.
 *
 * @return  If a trace buffer has been configured, then a pointer to its
 *          buffer descriptor is returned. Otherwise, returns NULL.
 */
EDC_BUFFER_T *
EDC_getTraceBuffer(void);

/**
 * Performs an EDC reset.
 * Should be used with caution, as this is an EDC wide reset, so is therefore
 * not multi-context safe.
 */
void
EDC_reset(void);

/**
 * Poll waiting for a channel to to idle.
 * @param[in]  channel         The identifier of the EDC channel to poll on
 */
void
EDC_pollForChannelIdle(DCP_CHANNEL_T *channel);

#if (__UCC__ >= 330)

/**
 * Configures the external RAM read ahead mode.
 *
 * @param[in]  enabled  true to enable read ahead, false to disabled it
 */
void EDC_configReadAhead(bool enabled);

/**
 * Queries the current external RAM read ahead mode.
 *
 * @return  bool  true if read ahead is enabled, false if it's disabled
 */
bool EDC_queryReadAhead(void);

#endif /* __UCC__ >= 330 */

/*-------------------------------------------------------------------------*/

/*
 * Queue Manager functions.
 */

/**
 * Flush a queue, resetting it back to its initial state.
 *
 * @param[in]  q    The ID of the queue to flush.
 */
void
QM_flush(int q);

/**
 * Posts into the tail of a queue.
 *
 * @param[in]  q    The ID of the queue to post into.
 * @param[in]  val  The value to post into the queue.
 */
void
QM_postTail(int q, uint32_t val);

/**
 * Read item from head of queue, popping that item from the queue.
 *
 * @param[in]  q    The ID of the queue to read.
 * @return   The value of the item read from the head of the queue.
 */
uint32_t
QM_popHead(int q);

/**
 * Read the value of the head of the queue without removing that item from the queue.
 *
 * @param[in]  q    The ID of the queue to query.
 * @return   The value of the item read at the head of the queue.
 */
uint32_t
QM_queryHead(int q);

/**
 * Returns the depth of a specified queue.
 *
 * @param[in]  queueId  The identifier of the queue whose depth is sought. 
 * @return     The depth of the queue.
 */
int
QM_getDepth(int queueId);

/*-------------------------------------------------------------------------*/

/*
 * Queue-monitoring functions
 */

/**
 * Query the count of the current queue (queue depth used)
 *
 * @param[in]  q        The ID of the queue to query.
 * @return     The count value of the queue.
 */
uint32_t
QueueMon_queryCount(int q);

/**
 * Poll for queue being not empty - returns the number of items in the queue.
 * Note, there is no implicit locking around this transaction. If other QM_
 * functions are used in a multi-tasking environment locking is required.
 *
 * @param[in]  q        The ID of the queue to poll.
 * @return     The count value of the queue.
 */
uint32_t
QueueMon_poll(int q);

/*-------------------------------------------------------------------------*/

/*
 * PDUMP functions.
 */

/**
 * Opens the PDUMP stream.
 *
 * @return  true for success, false for failure.
 */

bool
UCC_openPdump(void);

/**
 * Closes the PDUMP stream.
 */
void
UCC_closePdump(void);

/*-------------------------------------------------------------------------*/
/* HOST PORT public interface
 * Host port support is implemented in two parts - a QIO driver for handling
 * incoming messages (from the host) which are interrupt driven and an "API"
 * module which handles outgoing messages (polled) and adds a convenience wrapper
 * around the driver. Since there is some interplay between incoming and outgoing
 * messages, the QIO driver is considered private, with the public interface being
 * limited to the functions defined here.
 */
/**
 * Host port API function return codes
 */
typedef enum
{
    HP_RESULT_SUCCESS,
    HP_RESULT_ILLEGAL_PARAM,
    HP_RESULT_READ_CANCEL,
    HP_RESULT_READ_TIMEOUT,
    HP_RESULT_WRITE_TIMEOUT,
} HP_RESULT_T;

/*
 * Host port received message descriptor
 * These objects are use to manage a number of in-flight host-read message requests.
 */
typedef struct
{
    /** first item is a QIO IOCB which makes the object poolable */
    QIO_IOCB_T iocb;
    /** length of message in bytes */
    unsigned msgLen;
    /** Pointer to message data */
    uint8_t *msgPtr;
} HP_MSG_DESCRIPTOR_T;
/**
 * Initialise the Host Port API and underlying driver.
 *
 * @param[in] bufH GRAM address of buffer to be used by driver for HOST->UCCP messages
 * @param[in] bufU GRAM address of buffer to be used by driver for UCCP->HOST messages
 * @param[in] bufLen length of buffers (in GRAM words) at \c bufH and \c bufU
 * @param[in] pollPriority  MeOS Priority to use while polling (busy waiting) for ACK on META->Host transfers.
 * @return    Completion status.
 *
 * \c bufH and \c bufU must be valid GRAM addresses and subject to the constraint that bufU - bufH == bufLen
 */
HP_RESULT_T
HP_init(UCCP_GRAM_ADDRESS_T bufH, UCCP_GRAM_ADDRESS_T bufU, unsigned bufLen,
        KRN_PRIORITY_T pollPriority);

/**
 * Queue a message buffer to receive a message from the host.
 *
 * @param[in]     msgDesc  Pointer to a message descriptor.
 * @param[in]     mbox   Pointer to mail box to receive message.
 * @param[in]     timeout Timeout (in scheduler clock ticks). KRN_INFWAIT for infinite wait, otherwise should be >0
 * @return         Completion status
 */
HP_RESULT_T
HP_queueReadFromHost(HP_MSG_DESCRIPTOR_T *msgDesc, KRN_MAILBOX_T *mbox,
                     int timeout);

/**
 *  Write a  message to the host.
 *
 *  @param[in]     message Pointer to the message buffer.
 *  @param[in]     msgLen Length of message (in bytes).
 *  @param[in]     timeout Timeout (in scheduler clock ticks). KRN_INFWAIT for infinite wait, otherwise should be >0.
 *  @return         Completion status.

 ******************************************************************************/
HP_RESULT_T
HP_writeToHost(uint8_t *message, unsigned msgLen, int timeout);

/*-------------------------------------------------------------------------*/
/* SCP public interface
 * Most SCP configuration is done via the register image system and REG_load()
 * Explicit SCP support is limited to functions for making run-time adjustments
 * to the SCP such as are typically done in AGC and tracking feedback loops.
 *------------------------------------------------------------------------*/

/**
 * Return hardware variant of a SCP
 *
 * @param[in]   scp     Pointer to SCP object
 * @return      SCP hardware variant
 */
SCP_VARIANT_T SCP_variant(SCP_T *scp);
/**
 * Return UCC "parent" or "owner" UCC of a SCP
 *
 * @param[in]   scp     Pointer to SCP object
 * @return      pointer to parent UCC object
 */
UCC_T *
SCP_parent(SCP_T *scp);
/**
 * Return Id number of a SCP
 *
 * @param[in]   mcp     Pointer to MCP object
 * @return      Id number of SCP
 */
unsigned
SCP_id(SCP_T *mcp);

/**
 * Reset a SCP hardware instance.
 *
 * @param[in]   scp Pointer to SCP object
 *
 */
void
SCP_reset(SCP_T *scp);

/**
 * Reset a SCP hardware instance.  This reset is less complete than the one
 * provided by SCP_reset(); it resets only the internal states.  This function is
 * provided for compatibility with legacy software and not recommended for new
 * designs.
 *
 * @param[in]   scp Pointer to SCP object
 *
 */
void
SCP_resetInternalState(SCP_T *scp);

/**
 * Enumeration of power codes used by SCP_getStatus() and SCP_configure().
 */
/* Don't change these values - they are used directly as register fields */
typedef enum
{
    /** Unchanged/Undefined */
    SCP_PWR_UNCHANGED = 0,
    /** Low Power operation */
    SCP_PWR_LOW = 1,
    /** Power on */
    SCP_PWR_ON = 2,
    /** Power off */
    SCP_PWR_OFF = 3

} SCP_PWR_T;
/**
 * Enumeration of SCP operating mode selections.
 */
/* Don't change these values - they are used directly as register fields */
typedef enum
{
    /** Normal */
    SCP_MODE_NORMAL = 0,
    /** Local Oscillator phase accumulator outpu to IQ */
    SCP_MODE_LO = 1,
    /** Bypass mode 1 - resampler results output on IQ */
    SCP_MODE_RSOUT = 2,
    /** Bypass mode 2 - mixer results output on IQ */
    SCP_MODE_MIXOUT = 3,
    /** SCP bypass - samples bypass SCP completely */
    SCP_MODE_BYPASS = 4
} SCP_MODE_T;

/**
 * SCP data source selection.
 */
/* Don't change these values - they are used directly as register fields */
typedef enum
{
    /** On-board ADC */
    SCP_SRC_ADC = 0,
    /** External pins */
    SCP_SRC_EXT = 1,
    /** Input sample register */
    SCP_SRC_INREG = 2
} SCP_SRC_T;

/**
 * SCP data destination selection
 */
/* Don't change these values - they are used directly as register fields */
typedef enum
{
    /** DMA1 */
    SCP_DST_DMA1 = 0,
    /** TBUS */
    SCP_DST_TBUS = 1,
} SCP_DST_T;

/**
 * SCP frame specifications
 */
typedef enum
{
    /** Current frame (or no change) */
    SCP_FRAME_NOCHANGE = 0,
    /** Frame A */
    SCP_FRAME_A = 2,
    /** Frame B */
    SCP_FRAME_B = 3
} SCP_FRAME_T;
/**
 * SCP Noise counter selections.
 */
typedef enum
{
    /** Impulse samples */
    SCP_NOISE_SAMPLE,
    /** Impulse bursts */
    SCP_NOISE_BURST,
} SCP_NOISE_T;

/**
 * SCP input type
 */
/* Don't change these values - they are used directly as register fields */
typedef enum
{
    /** real IF */
    SCP_INPUT_REAL = 0,
    /** IQ */
    SCP_INPUT_IQ = 1
} SCP_INPUT_T;
/**
 * SCP spectrum type
 */
/* Don't change these values - they are used directly as register fields */
typedef enum
{
    /** Inverted */
    SCP_SPECTRUM_INVERTED = 0,
    /** Normal */
    SCP_SPECTRUM_NORMAL = 1
} SCP_SPECTRUM_T;

/**
 * SCP update synchronisation type
 */
/* Don't change these values - they are used as register fields */
typedef enum
{
    /** No sync */
    SCP_UPDATE_NOSYNC = 0,
    /** Sync */
    SCP_UPDATE_SYNC = 1
} SCP_UPDATE_T;

/**
 * SCP frame counter operating modes
 */
typedef enum
{
    /** Normal, frame counters run normally */
    SCP_FRCMODE_NORMAL = 0,
    /** Frame counter runs in SCP low-power and power-on modes only when started by Wireless LAN ED-AGC */
    SCP_FRCMODE_EDAGC,
#if __UCC__ < 420
    /** Frame counter runs in SCP low-power and power-on modes only when started by DVB-H STC */
    SCP_FRCMODE_STC, /** NOTE: This feature is not available from UCC 420 onwards **/
#endif
} SCP_FRCMODE_T;

/**
 * Get SCP identifier
 *
 * The SCP Id is identified by three 8-bit numbers, known as the group, core and configuration
 * ids. This function simply reads these numbers directly from the SCP hardware registers.
 *
 * Beware - there may not be any obvious correspondence between the values returned by this function
 * and published SCP names.
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[out]  group           Group Id.
 * @param[out]  core            Core Id.
 * @param[out]  config          Configuration value
 */
void
SCP_getCoreId(SCP_T *scp, uint8_t *group, uint8_t *core, uint8_t *config);

/**
 * Configures the main operating parameters of a SCP. These parameters are not necessarily related to each other. Nor
 * do they represent the complete configuration of a SCP. This functional grouping is provided mainly as a convenience to
 * support historical programs which configured this group of parameters as a set.
 *
 * Each of the configuration parameters in this function can, alternatively, be set individually using
 * the appropriate \c SCP_setXXX() function.
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   source          Input data source selection.
 * @param[in]   destination     Output data destination.
 * @param[in]   sampleFormat    Sample format - offset binary or signed 2's complement.
 * @param[in]   inputMode       Input mode - real IF or IQ.
 * @param[in]   spectrum        Spectrum configuration - inverted or normal.
 * @param[in]   sync            Synchronised parameter updates - on or off.
 * @param[in]   mode            SCP operating mode.
 * @param[in]   activeFrame     Active frame selection.
 * @param[in]   adcPower        ADC power selection.
 * @param[in]   scpPower        SCP power selection.
 * @param[in]   powerMode       Lower power operation - normal or activated by Wireless LAN ED-AGC.
 * @param[in]   CICDec          First Stage (CIC) decimation filter ratio (1, 2, 3, 4, 6, 8, 12 or 16)
 * @param[in]   FIRDec          Final stage (FIR) decimation filter ratio (1, 2, or 3).
 * @param[in]   reset           true: Reset the SCP before configuring.
 * @param[in]   clrPhase        true: Clear Local Oscillator phase accumulator after configuring.
 *
 * @return      true: OK. false: Invalid configuration.
 */

bool
SCP_configure(SCP_T *scp, SCP_SRC_T source, SCP_DST_T destination,
              SCP_SAMPLE_FORMAT_T sampleFormat, SCP_INPUT_T inputMode,
              SCP_SPECTRUM_T spectrum, SCP_UPDATE_T sync, SCP_MODE_T mode,
              SCP_FRAME_T activeFrame, SCP_PWR_T adcPower, SCP_PWR_T scpPower,
              SCP_FRCMODE_T powerMode, unsigned CICDec, unsigned FIRDec,
              bool reset, bool clrPhase);

/**
 * Configure SCP data source.
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   source          Input data source selection.
 * @return      true: OK. false: Failure.
 */

bool
SCP_setSource(SCP_T *scp, SCP_SRC_T source);

/**
 * Configure SCP data destination.
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   destination     Output data destination.
 * @return      true: OK. false: Failure.
 */

bool
SCP_setDestination(SCP_T *scp, SCP_DST_T destination);

/**
 * Configure SCP spectrum inversion.
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   spectrum  Normal or inverted
 * @return      true: OK. false: Failure.
 */

bool
SCP_setSpectrumInversion(SCP_T *scp, SCP_SPECTRUM_T spectrum);

/**
 * Configure SCP update synchronisation.
 *
 * @param[in]   scp     Pointer to SCP object
 * @param[in]   update  Update method - normal or synchronised
 * @return      true:   OK. false: Failure.
 */

bool
SCP_setSync(SCP_T *scp, SCP_UPDATE_T update);

/**
 * Configure SCP operating mode setting.
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   mode            SCP operating mode.
 * @return      true: OK. false: Failure.
 */

bool
SCP_setMode(SCP_T *scp, SCP_MODE_T mode);

/**
 * Configure SCP active frame setting.
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   activeFrame     Active frame selection.
 * @return      true: OK. false: Failure.
 */

bool
SCP_setActiveFrame(SCP_T *scp, SCP_FRAME_T activeFrame);

/**
 * Configure SCP ADC power setting.
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   adcPower        SCP power selection.
 * @return      true: OK. false: Failure.
 */

bool
SCP_setADCPower(SCP_T *scp, SCP_PWR_T adcPower);

/**
 * Configure SCP power setting.
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   scpPower        SCP power selection.
 * @return      true: OK. false: Failure.
 */

bool
SCP_setSCPPower(SCP_T *scp, SCP_PWR_T scpPower);

/**
 * Configure frame counter mode.
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   frcMode         Frame counter mode - either normal operation, activated by Wireless
 *                              LAN ED-AGC or activated by DVB-H STC
 * @return      true: OK. false: Failure.
 */

bool
SCP_setFrCMode(SCP_T *scp, SCP_FRCMODE_T frcMode);

/**
 * Clear SCP phase accumulator.
 *
 * Sets and then clears the "clear phase accumulator" bit in the SCP configuration register
 *
 * @param[in]   scp             Pointer to SCP object
 */
void
SCP_clrPhase(SCP_T *scp);

/**
 * Configure SCP FIR decimation factor.
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   FIRDec          Final stage (FIR) decimation filter ratio (1, 2, or 3).
 * @return      true: OK. false: Failure.
 */

bool
SCP_setFIRDec(SCP_T *scp, unsigned FIRDec);

/**
 * Configure SCP CIC decimation factor.
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   CICDec          First Stage (CIC) decimation filter ratio (1, 2, 3, 4, 6, 8, 12 or 16)
 * @return      true: OK. false: Failure.
 */

bool
SCP_setCICDec(SCP_T *scp, unsigned CICDec);
/**
 * Set CIC bypass mode
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   bypass          true: bypass active, false: normal operation.
 *
 * @return      Previous bypass setting
 */

bool
SCP_setCICBypass(SCP_T *scp, bool bypass);
/**
 * Set DC Offsyte bypass mode
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   bypass          true: bypass active, false: normal operation.
 *
 * @return      Previous bypass setting
 */

bool
SCP_setDCOBypass(SCP_T *scp, bool bypass);
/**
 * Set Early Gain bypass mode
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   bypass          true: bypass active, false: normal operation.
 *
 * @return      Previous bypass setting
 */

bool
SCP_setEarlyGainBypass(SCP_T *scp, bool bypass);
/**
 * Set FIR bypass mode
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   bypass          true: bypass active, false: normal operation.
 *
 * @return      Previous bypass setting
 */

bool
SCP_setFIRBypass(SCP_T *scp, bool bypass);

/**
 * Set impulse filter bypass mode
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   bypass          true: bypass active, false: normal operation.
 *
 * @return      Previous bypass setting
 */

bool
SCP_setIMPBypass(SCP_T *scp, bool bypass);

/**
 * Configure SCP input mode.
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   inMode         Input mode - real IF or IQ.
 * @return      true: OK. false: Failure.
 */

bool
SCP_setInputMode(SCP_T *scp, SCP_INPUT_T inMode);

/**
 * Set Late Gain bypass mode
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   bypass          true: bypass active, false: normal operation.
 *
 * @return      Previous bypass setting
 */

bool
SCP_setLateGainBypass(SCP_T *scp, bool bypass);
/**
 * Set mixer bypass mode
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   bypass          true: bypass active, false: normal operation.
 *
 * @return      Previous bypass setting
 */

bool
SCP_setMixerBypass(SCP_T *scp, bool bypass);
/**
 * Set notch filter bypass mode
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   bypass          true: bypass active, false: normal operation.
 *
 * @return      Previous bypass setting
 */

bool
SCP_setNotchBypass(SCP_T *scp, bool bypass);
/**
 * Set notch filter continuous accumulation mode
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   continuous      true: notch filter accumulators run continuously, false: notch filers reset on register writes.
 *
 * @return      Previous bypass setting
 */

bool
SCP_setNotchAccum(SCP_T *scp, bool continuous);

/**
 * Configure SCP sample format.
 *
 * @param[in]   scp     Pointer to SCP object
 * @param[in]   format  Offset binary or signed 2's complement.
 * @return      true: OK. false: Failure.
 */

bool
SCP_setSampleFormat(SCP_T *scp, SCP_SAMPLE_FORMAT_T format);

/**
 * Set Resampler bypass mode
 *
 * @param[in]   scp             Pointer to SCP object
 * @param[in]   bypass          true: bypass active, false: normal operation.
 *
 * @return      Previous bypass setting
 */

bool
SCP_setResamplerBypass(SCP_T *scp, bool bypass);

/**
 * Read SCP data source configuration.
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      Input data source selection.
 */
SCP_SRC_T
SCP_getSource(SCP_T *scp);

/**
 * Read SCP data destination configuration.
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      Output data destination selection.
 */
SCP_DST_T
SCP_getDestination(SCP_T *scp);

/**
 * Read SCP spectrum inversion configuration.
 *
 * @param[in]   scp             Pointer to SCP object
 * returns  Spectrum configuration - inverted or normal.
 */
SCP_SPECTRUM_T
SCP_getSpectrumInversion(SCP_T *scp);

/**
 * Read SCP update synchronisation configuration.
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      Update method - synchronised or normal.
 */
SCP_UPDATE_T
SCP_getSync(SCP_T *scp);

/**
 * Read  SCP operating mode configuration.
 *
 * @param[in]   scp             Pointer to SCP object
 * @return             SCP operating mode.
 */
SCP_MODE_T
SCP_getMode(SCP_T *scp);

/**
 * Read SCP active frame configuration.
 *
 * @param[in]   scp             Pointer to SCP object
 * @return  Active frame selection.
 */
SCP_FRAME_T
SCP_getActiveFrame(SCP_T *scp);

/**
 * Read SCP ADC power configuration.
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      SCP power selection.
 */
SCP_PWR_T
SCP_getADCPower(SCP_T *scp);

/**
 * Read SCP power configuration.
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      SCP power selection.
 */
SCP_PWR_T
SCP_getSCPPower(SCP_T *scp);

/**
 * Read SCP frame counter operation mode configuration.
 *
 * @param[in]   scp             Pointer to SCP object
 * @return       Frame counter mode - normal, activated by wireless
 *                              LAN ED-AGC or activated by DVB-H STC
 */
SCP_FRCMODE_T
SCP_getFrCMode(SCP_T *scp);

/**
 * Read SCP FIR decimation factor.
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      Final stage (FIR) decimation filter ratio (1, 2, or 3).
 */
unsigned
SCP_getFIRDec(SCP_T *scp);
/**
 * get CIC bypass state
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      true:           bypass active, false: normal operation.
 *
 * @return      Previous bypass setting
 */

bool
SCP_getCICBypass(SCP_T *scp);
/**
 * get DC Offset bypass state
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      true:           bypass active, false: normal operation.
 *
 */

bool
SCP_getDCOBypass(SCP_T *scp);
/**
 * get Early Gain bypass state
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      true:           bypass active, false: normal operation.
 *
 */

bool
SCP_getEarlyGainBypass(SCP_T *scp);
/**
 * get FIR bypass state
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      true:           bypass active, false: normal operation.
 *
 */

bool
SCP_getFIRBypass(SCP_T *scp);

/**
 * get Impule filter bypass state
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      true:           bypass active, false: normal operation.
 *
 */

bool
SCP_getIMPBypass(SCP_T *scp);

/**
 * get Late Gain bypass state
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      true:           bypass active, false: normal operation.
 *
 */

bool
SCP_getLateGainBypass(SCP_T *scp);

/**
 * get Mixer bypass state
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      true:           bypass active, false: normal operation.
 *
 */

bool
SCP_getMixerBypass(SCP_T *scp);

/**
 * get Notch filter bypass state
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      true:           bypass active, false: normal operation.
 *
 */

bool
SCP_getNotchBypass(SCP_T *scp);

/**
 * get Notch filter continuous accumulation state
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      true:           bypass active, false: normal operation.
 */

bool
SCP_getNotchAccum(SCP_T *scp);

/**
 * get Resampler bypass state
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      true:           continuous accumulation active, false: normal operation.
 *
 */

bool
SCP_getResamplerBypass(SCP_T *scp);

/**
 * Read SCP CIC decimation factor.
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      First Stage (CIC) decimation filter ratio (1, 2, 3, 4, 6, 8, 12 or 16)
 */
unsigned
SCP_getCICDec(SCP_T *scp);
/**
 * Read SCP input mode configuration.
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      Input mode - real IF or IQ.
 */
SCP_INPUT_T
SCP_getInputMode(SCP_T *scp);
/**
 * Read SCP sample format configuration.
 *
 * @param[in]   scp             Pointer to SCP object
 * @return      Sample format - offset binary or two's complement.
 */
SCP_SAMPLE_FORMAT_T
SCP_getSampleFormat(SCP_T *scp);

/**
 * Get current status of a SCP.
 *
 * @param[in]  scp              Pointer to SCP object
 * @param[out] activeFrame      Active frame - A or B.
 * @param[out] adcPower         Active ADC power setting
 * @param[out] scpPower         Active SCP power setting
 */

void
SCP_getStatus(SCP_T *scp, SCP_FRAME_T *activeFrame, SCP_PWR_T *adcPower,
              SCP_PWR_T *scpPower);
/**
 * Read the mixer local oscillator control value.
 *
 * The mixer local oscillator control value is a 26-bit number, defined
 * by the sum of two numbers: a signed "base" value, in which only bits 23:25
 * are significant, and a two's complement offset value in which bits 0:23
 * are significant (sign + 23 significant bits).
 *
 * @param[in]  scp              Pointer to SCP object
 * @return  Mixer local oscillator control value sign extended to 32bits.
 */
int
SCP_getFc(SCP_T *scp);
/**
 * Read the mixer local oscillator base value.
 *
 * See the description of SCP_getFc() for an explanation of local oscillator control,
 * base and offset values.
 *
 * @param[in]  scp              Pointer to SCP object.
 * @return  base value sign extended to 32 bits.
 */
int
SCP_getFcBase(SCP_T *scp);
/**
 * Read the mixer local oscillator offset value.
 *
 * See the description of SCP_getFc() for an explanation of local oscillator control,
 * base and offset values.
 *
 * @param[in]  scp              Pointer to SCP object.
 * @return  offset value sign extended to 32 bits.
 */
int
SCP_getFcOffset(SCP_T *scp);
/**
 * Set the mixer local oscillator control value. This function breaks down the desired control
 * value into base and offset components, in such a way as to place the desired value as close as
 * possible to the centre of the adjustment range achievable by programming the offset alone.
 *
 * See the description of SCP_getFc() for an explanation of local oscillator control,
 * base and offset values.
 *
 * The local oscillator value is a two's complement 26-bit value (sign extended to int in the value parameter).
 * The range of the control value is -Fs/2 to +Fs/2
 * where Fs is the ADC sample rate.
 *
 * @param[in]    scp     Pointer to the SCP object.
 * @param[in]    value   Local oscillator control value sign extended to int.
 */
void
SCP_setFc(SCP_T *scp, int value);
/**
 * Set the mixer local oscillator base value.
 *
 * See the description of SCP_getFc() for an explanation of local oscillator control,
 * base and offset values.
 *
 * @param[in]    scp     Pointer to the SCP object.
 * @param[in]    value   Local oscillator base value sign extended to int.
 */
void
SCP_setFcBase(SCP_T *scp, int value);
/**
 * Set the mixer local oscillator offset value.
 *
 * See the description of SCP_getFc() for an explanation of local oscillator control,
 * base and offset values.
 *
 * @param[in]    scp     Pointer to the SCP object
 * @param[in]    value   Local oscillator offset value sign extended to int.
 */
void
SCP_setFcOffset(SCP_T *scp, int value);
/**
 * Reads late gain stage settings. Refer to the description of ::SCP16_setLateGains() for
 * more information.
 *
 * @param[in] scp     Pointer to the SCP object
 * @param[in] fineGainI Fine gain control value (I) in Q0.31 format.
 * @param[in] fineGainQ Fine gain control value (Q) in Q0.31 format.
 * @param[in] coarseGain Coarse gain control (1 or 0).
 */
void
SCP16_getLateGains(SCP_T *scp, int32_t *fineGainI, int32_t *fineGainQ, int *coarseGain);
/**
 * Reads late gain stage settings. Refer to the description of SCP_setLateGains() for
 * more information.
 * @param[in]    scp     Pointer to the SCP object
 * @param[out]   fineGainI  Late fine gain (I) value sign extended to int.
 * @param[out]   fineGainQ  Late fine gain (Q) value sign extended to int.
 * @param[out]   coarseGain  Late coarse gain selection (1 or 0).
 */
void
SCP_getLateGains(SCP_T *scp, int *fineGainI, int *fineGainQ, int *coarseGain);
/**
 * Sets up the late gain stage of the SCP.
 *
 * Independent I and Q gains are allowed to compensate for gain
 * mismatch and a leakage term from the I to Q can be used to
 * compensate for quadrature error.
 *
 * The fine gains are signed quantities in Q0.31 format for a linear gain
 * multiplier where the 0dB gain level is set using a value of 0.25.
 * The fine gain range is therefore -inf to +12dB. Negative values
 * imply signal inversion.
 *
 * The coarse gain, when set, provides a further x4 boost.
 * *
 * The number of significant fraction bits in the fine gain and angle controls is 10
 * for both 16-bit and 12-bit SCPs.
 *
 * @param[in] scp     Pointer to the SCP object
 * @param[in] fineGainI Fine gain control value (I) in Q0.31 format.
 * @param[in] fineGainQ Fine gain control value (Q) in Q0.31 format.
 * @param[in] coarseGain Coarse gain control (non-zero: x4 boost).
 */
void
SCP16_setLateGains(SCP_T *scp, int32_t fineGainI, int32_t fineGainQ, int coarseGain);
/**
 * Sets up the late gain stage of the SCP.
 *
 * This is a legacy 12-bit SCP function. Gain values are provided as 11 bit 2's complement
 * integers, sign extended to 16 bits. ALthough a 16-bit SCP provides no more precision
 * for setting the late gains, you may wish to adopt a similar API style as for the early gains,
 * in which case should use ::SCP16_setLateGains().
 *
 * Independent I and Q gains are allowed to compensate for gain
 * mismatch and a leakage term from the I to Q can be used to
 * compensate for quadrature error.
 *
 * The fine gains are 11 bit signed quantities for a linear gain
 * multiplier where the 0dB gain level is set using a value of 256.
 * The fine gain range is therefore -inf to +12dB. Negative values
 * imply signal inversion.
 *
 * A non-zero value for coarse gain, provides a further x4 boost.
 *

 * @param[in] scp     Pointer to the SCP object
 * @param[in] fineGainI Fine gain control value (I).
 * @param[in] fineGainQ Fine gain control value (Q).
 * @param[in] coarseGain Coarse gain control (non-zero: x4 boost).
 */
void
SCP_setLateGains(SCP_T *scp, int fineGainI, int fineGainQ, int coarseGain);
/**
 * Reads early gain stage settings. Refer to the description of ::SCP_setEarlyGains() for
 * more information.
 *
 * This is a legacy 12-bit SCP function. Gain values are provided as 11 bit 2's complement
 * integers, sign extended to 16 bits. If you need the additional precision in gain values
 * provided by a 16-bit SCP, then you should use ::SCP16_getEarlyGains().
 *
 * @param[in] scp     Pointer to the SCP object
 * @param[in] fineGainI Fine gain control value (I).
 * @param[in] fineGainQ Fine gain control value (Q).
 * @param[in] coarseGain Coarse gain control (1 or 0).
 * @param[in] angleControl Angle control value.
 */
void
SCP_getEarlyGains(SCP_T *scp, int *fineGainI, int *fineGainQ, int *coarseGain,
                  int *angleControl);
/**
 * Reads early gain stage settings. Refer to the description of ::SCP16_setEarlyGains() for
 * more information.
 *
 * @param[in] scp     Pointer to the SCP object
 * @param[in] fineGainI Fine gain control value (I) in Q0.31 format.
 * @param[in] fineGainQ Fine gain control value (Q) in Q0.31 format.
 * @param[in] coarseGain Coarse gain control (1 or 0).
 * @param[in] angleControl Angle control value in Q0.31 format.
 */
void
SCP16_getEarlyGains(SCP_T *scp, int32_t *fineGainI, int32_t *fineGainQ, int *coarseGain,
                  int32_t *angleControl);
/**
 * Sets up the early gain stage of the SCP.
 *
 * Independent I and Q gains are allowed to compensate for gain
 * mismatch and a leakage term from the I to Q can be used to
 * compensate for quadrature error.
 *
 * The fine gains are signed quantities in Q0.31 format for a linear gain
 * multiplier where the 0dB gain level is set using a value of 0.25.
 * The fine gain range is therefore -inf to +12dB. Negative values
 * imply signal inversion.
 *
 * The coarse gain, when set, provides a further x4 boost.
 *
 * The angle control is a signed quantity in Q0.31 format.
 * Angle correction is applied by modifying Q according to:
 *
 * Q = Q + angleControl * I
 *
 * The number of significant fraction bits in the fine gain and angle controls is 13
 * for a 16-bit SCP and 10 for a 12-bit SCP.
 *
 * @param[in] scp     Pointer to the SCP object
 * @param[in] fineGainI Fine gain control value (I) in Q0.31 format.
 * @param[in] fineGainQ Fine gain control value (Q) in Q0.31 format.
 * @param[in] coarseGain Coarse gain control (non-zero: x4 boost).
 * @param[in] angleControl Angle control value in Q0.31 format.
 */
void
SCP16_setEarlyGains(SCP_T *scp, int32_t fineGainI, int32_t fineGainQ, int coarseGain,
                  int32_t angleControl);
/**
 * Sets up the early gain stage of the SCP.
 *
 * This is a legacy 12-bit SCP function. Gain values are provided as 11 bit 2's complement
 * integers, sign extended to 16 bits. If you need the additional precision in gain setting
 * provided by a 16-bit SCP, then you should use ::SCP16_setEarlyGains().
 *
 * Independent I and Q gains are allowed to compensate for gain
 * mismatch and a leakage term from the I to Q can be used to
 * compensate for quadrature error.
 *
 * The fine gains are 11 bit signed quantities for a linear gain
 * multiplier where the 0dB gain level is set using a value of 256.
 * The fine gain range is therefore -inf to +12dB. Negative values
 * imply signal inversion.
 *
 * The coarse gain, when set, provides a further x4 boost.
 *
 * The angle control is again a signed 11 bit quantity that is
 * linearly multiplied by the I component and the result added
 * to the Q component.
 *
 *
 * @param[in] scp     Pointer to the SCP object
 * @param[in] fineGainI Fine gain control value (I).
 * @param[in] fineGainQ Fine gain control value (Q).
 * @param[in] coarseGain Coarse gain control (non-zero: x4 boost).
 * @param[in] angleControl Angle control value.
 */
void
SCP_setEarlyGains(SCP_T *scp, int fineGainI, int fineGainQ, int coarseGain,
                  int angleControl);
/**
 * Sets up the gain of the mixer stage of the SCP.
 *
 * The mixer output gain can be set 2x in order to compensate for a
 * loss of signal power when using a real only input signal.
 *
 * The gains available are:
 *     SCP_MIXER_OUT_GAIN_0DB           gain of 1
 *     SCP_MIXER_OUT_GAIN_6DB           gain of 2
 *
 *
 * @param[in] scp     Pointer to the SCP object
 * @param[in] gain    Gain parameter (see above)
 */
void
SCP_setMixerGain(SCP_T *scp, SCP_MIXER_GAIN_T gain);
/**
 * Sets up the gain of the resampler stage of the SCP.
 *
 * The resampler output gain can be set to 0.25, 0.5, 1.0 or 2.0 by
 * specifying the gain parameter as follows:
 *     SCP_RESAMPLE_OUT_GAIN_MINUS12DB  gain of 1/4
 *     SCP_RESAMPLE_OUT_GAIN_MINUS6DB   gain of 1/2
 *     SCP_RESAMPLE_OUT_GAIN_0DB        gain of 1
 *     SCP_RESAMPLE_OUT_GAIN_6DB        gain of 2
 *
 * @param[in] scp     Pointer to the SCP object
 * @param[in] gain    Gain parameter (see above)
 */
void
SCP_setResamplerGain(SCP_T *scp, SCP_RESAMPLER_GAIN_T gain);
/**
 * Sets up the gain of the FIR stage of the SCP.
 *
 * The FIR gain can be set to the following values:
 *     SCP_FIR_OUT_GAIN_MINUS30DB       gain of 1/32
 *     SCP_FIR_OUT_GAIN_MINUS24DB       gain of 1/16
 *     SCP_FIR_OUT_GAIN_MINUS18DB       gain of 1/8
 *     SCP_FIR_OUT_GAIN_MINUS12DB       gain of 1/4
 *     SCP_FIR_OUT_GAIN_MINUS6DB        gain of 1/2
 *     SCP_FIR_OUT_GAIN_0DB             gain of 1
 *
 * @param[in] scp     Pointer to the SCP object
 * @param[in] gain    Gain parameter (see above)
 */
void
SCP_setFIRGain(SCP_T *scp, SCP_FIR_GAIN_T gain);
/**
 * Get the gain of the mixer stage of the SCP. Refer to the description of
 * SCP_setMixerGains() for more information.
 *
 * @param[in] scp     Pointer to the SCP object
 * @return  gain parameter (see description of SCP_setMixerGains()
 */
SCP_MIXER_GAIN_T
SCP_getMixerGain(SCP_T *scp);
/**
 * Get the gain of the resampler stage of the SCP. Refer to the description of
 * SCP_setResamplerGains() for more information.
 *
 * @param[in] scp     Pointer to the SCP object
 * @return  gain parameter (see description of SCP_setResamplerGains()
 */
SCP_RESAMPLER_GAIN_T
SCP_getResamplerGain(SCP_T *scp);
/**
 * Get the gain of the FIR stage of the SCP. Refer to the description of
 * SCP_setFIRGains() for more information.
 *
 * @param[in] scp     Pointer to the SCP object
 * @return  gain parameter (see description of SCP_setFIRGains()
 */
SCP_FIR_GAIN_T
SCP_getFIRGain(SCP_T *scp);

/**
 * Set coefficients of FIR decimation filter. Coefficients are signed fixed point values
 * in Q31 format. There are 9 significant fraction bits on a 12-bit SCP and 16 significant fraction bits
 * on a 16-bit SCP
 *
 * The filter is symmetric so, for n taps only n/2 coefficients are required.
 *
 * Up to 12 coefficients may be provided for a 12-bit SCP (24 tap filter) and up to 48
 * coefficients may be provided for a 16-bit SCP (96-tap filter).
 *
 * The function returns false if an invalid number of coefficients is provided.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] coeffs Pointer to array of coefficient values.
 * @param[in] numCoeffs length of the \c coeffs array (filter length/2)
 * @return true: Filter loaded. false:invalid filter length
 */
bool
SCP16_setFIRCoeffs(SCP_T *scp, int32_t *coeffs, unsigned numCoeffs);
/**
 * Set coefficients of FIR decimation filter. Coefficients are signed values limited to a 10-bit range (including the sign bit).
 *
 * The filter is a 24-tap symmetric filter, so only coefficients h0 to h11 are required.
 *
 * This is a legacy 12-bit SCP function. In the case of a 16-bit SCP, the filter coefficients are shifted
 * left by 7 bits. If the full precision of the filter in a 16-bit SCP is required, then use ::SCP16_setFIRCoeffs().
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] coeffs Pointer to array of 12 coefficient values.
 */
void
SCP_setFIRCoeffs(SCP_T *scp, int16_t *coeffs);
/**
 * Read coefficients of FIR decimation filter.
 *
 * This is a legacy 12-bit SCP function. It is not supported on 16 bit SCPs
 *
 * @param[in] scp Pointer to SCP object.
 * @param[out] coeffs Pointer to array of 12 coefficient values.
 * @return true: OK. false: Functionality not available ( 16-bit SCP)
 */
bool
SCP_getFIRCoeffs(SCP_T *scp, int16_t *coeffs);
/**
 * Set coefficients of resampler filter. Coefficients are signed fixed point values
 * in Q31 format. There are 9 significant fraction bits on a 12-bit SCP and 13 significant fraction bits
 * on a 16-bit SCP
 *
 * The filter is symmetric so, for n taps only n/2 coefficients are required.
 *
 * For a 12-bit SCP, 13 coefficients must be provided (defines a symmetric 25 tap filter).
 * For a 16-bit SCP, 33 coefficients must be provided (defines a symmetric 65 tap filter).
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] coeffs Pointer to array of coefficient values.
 * @param[in] numCoeffs Length of the \c coeffs array.
 * @return true:OK. false: Wrong number of coefficients for SCP variant
 */
bool
SCP16_setResamplerCoeffs(SCP_T *scp, int32_t *coeffs, unsigned numCoeffs);

/**
 * Set coefficients of resampler filter. Coefficients are signed values limited to a 10-bit range (including the sign bit).
 *
 * The filter is a 25-tap symmetric filter, so only coefficients h0 to h12 are required.
 *
 * This is a legacy 12-bit SCP function. It is not supported on 16 bit SCPs
 *
 * In test and debug builds this function will assert, rather than returning false.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] coeffs Pointer to array of 13 coefficient values.
 * @return true:OK. false: Functionality not available (16-bit SCP)
 */
bool
SCP_setResamplerCoeffs(SCP_T *scp, int16_t *coeffs);
/**
 * Read coefficients of the resampler filter.
 *
 * This is a legacy 12-bit SCP function. It is not supported on 16 bit SCPs
 *
 * @param[in] scp Pointer to SCP object.
 * @param[out] coeffs Pointer to array of 13 coefficient values.
 * @return true: OK. false: Functionality not available ( 16-bit SCP)
 */

bool
SCP_getResamplerCoeffs(SCP_T *scp, int16_t *coeffs);
/**
 * Read the resampler control value.
 *
 * The resampler control value is a 26-bit value, defined
 * by the sum of two numbers: an unsigned "base" value, in which only bits 23:25
 * are significant (3 significant bits) and a signed offset value in which bits 0:23
 * are significant (sign + 23 significant bits). There is a 1 bit overlap between base
 * and offset, so they are added (not OR'ed) to give the control value
 *
 * @param[in]  scp              Pointer to SCP object
 * @return  Resampler control value in the low 26 bits of an unsigned 32-bit integer .
 */
unsigned
SCP_getFs(SCP_T *scp);
/**
 * Read the resampler base value.
 *
 * See the description of SCP_getFs() for an explanation of resampling factor control,
 * base and offset values.
 *
 * @param[in]  scp              Pointer to SCP object.
 * @return  resampler control base value as a 32-bit unsigned integer.
 */
unsigned
SCP_getFsBase(SCP_T *scp);
/**
 * Read the resampler offset value.
 *
 * See the description of SCP_getFs() for an explanation of resampling factor control,
 * base and offset values.
 *
 * @param[in]  scp              Pointer to SCP object.
 * @return  resampler control offset value sign extended to 32 bits.
 */
int
SCP_getFsOffset(SCP_T *scp);
/**
 * Set the resampler conversion ratio (I/O).
 *
 * See the description of SCP_getFs() for an explanation of resampling factor control,
 * base and offset values.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] ratio Resampler conversion ratio.
 */
void
SCP_setFs(SCP_T *scp, unsigned ratio);
/**
 * Set the resampling factor base value.
 *
 * See the description of SCP_getFs() for an explanation of resampling factor control,
 * base and offset values.
 *
 * @param[in]    scp     Pointer to the SCP object.
 * @param[in]    value   Local oscillator base value sign extended to int.
 */
void
SCP_setFsBase(SCP_T *scp, unsigned value);
/**
 * Set the mixer local oscillator offset value.
 *
 * See the description of SCP_getFs() for an explanation of resampling factor control,
 * base and offset values.
 *
 * @param[in]    scp     Pointer to the SCP object
 * @param[in]    value   Local oscillator offset value sign extended to int.
 */
void
SCP_setFsOffset(SCP_T *scp, int value);
/**
 * Configure the SCP front end throw-away decimator.
 * The ratio parameter is a value between 1 and 16.
 *
 * @param[in] scp Pointer to SCP object
 * @param[in] enable true: Decimator enabled. false: Decimator disabled.
 * @param[in] ratio Decimation ratio.
 * @return  true: Configuration OK. false: Bad ratio value.
 */

bool
SCP_setDecimator(SCP_T *scp, bool enable, unsigned ratio);
/**
 * Read the SCP front end throw-away decimator configuration.
 * The ratio parameter is a value between 1 and 16.
 *
 * @param[in] scp Pointer to SCP object
 * @param[out] enable true: Decimator enabled. false: Decimator disabled.
 * @param[out] ratio Decimation ratio.
 */
void
SCP_getDecimator(SCP_T *scp, bool *enable, unsigned *ratio);
/**
 * Enable/Disable the IQ correlator.
 *
 * @param[in] scp Pointer to SCP object
 * @param[in] enable true: Correlator enabled. false: Correlator disabled.
 */
void
SCP_setIQCorrelator(SCP_T *scp, bool enable);
/**
 * Read IQ Correlator state
 *
 * @param[in] scp Pointer to SCP object.
 * @param[out] enabled true: IQ correlator is enabled.
 * @return  IQ Correlator value
 */
int
SCP_getIQCorrelator(SCP_T *scp, bool *enabled);
/**
 * Set the SCP reference clock
 *
 * The SCP produces a reference clock whose frequency is R x N / (M x 2)
 * where R is the SCP output sample rate.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] numerator Numerator (N).
 * @param[in] denominator Denominator (M).
 *
 */
void
SCP_setRefClk(SCP_T *scp, unsigned numerator, unsigned denominator);

/**
 * Read the SCP reference clock controls
 *
 * The SCP produces a reference clock whose frequency is R x N / (M x 2)
 * where R is the SCP output sample rate.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[out] numerator Numerator (N).
 * @param[out] denominator Denominator (M).
 *
 */
void
SCP_getRefClk(SCP_T *scp, unsigned *numerator, unsigned *denominator);

/**
 * Set up a notch filter.
 *
 * The SCP has 5 independently controllable notches that can be used for interference suppression.
 *
 * Filter bandwidth is determined by a combination of linear (L) and shift (S) values chosen from a table defined in the
 * SCP TRM.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] notchNum Filter identifier (0..4).
 * @param[in] enable true: Enable filter. false: Disable filter.
 * @param[in] frequency Centre frequency. 16-bit two's complement value scaled so that full range represents +/- FSOut/2.
 * @param[in] gainShift Bandwidth (S) term (0..10).
 * @param[in] gainLinear Bandwidth (L) term (0..7).
 */
void
SCP_setNotchFilter(SCP_T *scp, unsigned notchNum, bool enable, int frequency,
                   unsigned gainShift, unsigned gainLinear);
/**
 * Get a notch filter's setup.
 *
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] notchNum Filter identifier (0..4).
 * @param[out] enable true: Enable filter. false: Disable filter.
 * @param[out] frequency Centre frequency. 16-bit two's complement value scaled so that full range represents +/- FSOut/2.
 * @param[out] gainShift Bandwidth (S) term (0..10).
 * @param[out] gainLinear Bandwidth (L) term (0..7).
 */
void
SCP_getNotchFilter(SCP_T *scp, unsigned notchNum, bool *enable, int *frequency,
                   unsigned *gainShift, unsigned *gainLinear);

/**
 * Gets energy removed by the selected notch filter (or would be removed if enabled).
 *
 * I and Q energy values are provided as signed fixed point values in Q0.31 format,
 * with the range -1.0 to ~= +1.0 corresponding to the full scale range of the notch
 * energy detection hardware. Depending on the underlying SCP hardware there will be
 * 12 or 16 significant fraction bits with the least significant bits being zero.
 *
 * The return value is provided as a signed fixed point value in Q1.30 format,
 * covering the range -2.0 to ~= +2.0. In practice the value will always be positive
 *
 * @param[in] scp Pointer to SCP object
 * @param[in] notchNum Filter identifier (0..4).
 * @param[out] signalI I component of energy removed in Q0.31 format
 * @param[out] signalQ Q component of energy removed in Q0.31 format
 * @return  Total energy removed (signalI^2 + signalQ^2) in Q1.30 format
 */
int32_t
SCP16_getNotchEnergy(SCP_T *scp, unsigned notchNum, int32_t *signalI, int32_t *signalQ);

/**
 * Gets energy removed by the selected notch filter (or would be removed if enabled).
 *
 * This is a legacy 12-bit SCP function. Signal values are provided as 13 bit 2's complement
 * integers, sign extended to 16 bits.
 *
 * In the case of a 16-bit SCP, only the most significant 13 of the available 17 bits are
 * provided. I.e. this function simply discards the extra precision available from a 16-bit SCP
 *
 * @param[in] scp Pointer to SCP object
 * @param[in] notchNum Filter identifier (0..4).
 * @param[out] signalI I component of energy removed
 * @param[out] signalQ Q component of energy removed
 * @return  Total energy removed (signalI^2 + signalQ^2)
 */
unsigned
SCP_getNotchEnergy(SCP_T *scp, unsigned notchNum, int *signalI, int *signalQ);
/**
 * Set up SCP frame compare event logic.
 *
 * This function just sets up the event detection logic. You will also need to install
 * a handler for SCP_EVT_COMPARE events if you want to respond to the events when generated.
 *
 * @param[in] scp pointer to SCP object.
 * @param[in] symbol Symbol number.
 * @param[in] sample Sample number.
 * @param[in] activeFrame Active frame selection.
 * @param[in] adcPower ADC power selection.
 * @param[in] scpPower SCP power selection.
 */
void
SCP_setFrameCompare(SCP_T *scp, unsigned symbol, unsigned sample,
                    SCP_FRAME_T activeFrame, SCP_PWR_T adcPower,
                    SCP_PWR_T scpPower);
/**
 * Read SCP frame compare controls.
 *
 * This function just reads back the last-written control values. In the case
 * of "unchanged" power or frame controls, no information is provided about the actual resultant
 * setting.
 *
 * @param[in] scp pointer to SCP object.
 * @param[out] symbol Symbol number.
 * @param[out] sample Sample number.
 * @param[out] activeFrame Active frame selection.
 * @param[out] adcPower ADC power selection.
 * @param[out] scpPower SCP power selection.
 */
void
SCP_getFrameCompare(SCP_T *scp, unsigned *symbol, unsigned *sample,
                    SCP_FRAME_T *activeFrame, SCP_PWR_T *adcPower,
                    SCP_PWR_T *scpPower);
/**
 * Set up AGC count event logic.
 *
 * Clip and threshold levels are provided
 * as unsigned integers in the range 0..0x7FFF.
 *
 * In the case of 12-bit hardware the
 * clip and threshold levels are effectively shifted right by 4 bits, thus setting the levels at
 * roughly the same places relative to a full scale signal.
 *
 * This function just sets up the event detection logic. You will also need to install
 * a handler for ::SCP_EVT_AGCCOUNT events if you want to respond to the events when generated.
 *
 * @param[in] scp pointer to SCP object.
 * @param[in] period AGC period.
 * @param[in] clipLevel AGC clipLevel.
 * @param[in] thresholdLevel AGC threshold level.
 */
void
SCP16_setAGC(SCP_T *scp, unsigned period, unsigned clipLevel,
           unsigned thresholdLevel);
/**
 * Read AGC count event logic.
 *
 * This is a legacy 12-bit SCP function. Clip and threshold levels are returned
 * as unsigned integers in the range 0..0x7FFF.
 *
 * In the case of 12-bit hardware the
 * clip and threshold levels are shifted left by 4 bits, thus corresponding with
 * values provided to ::SCP16_setAGC().
 *
 * @param[in] scp pointer to SCP object.
 * @param[out] period AGC period.
 * @param[out] clipLevel AGC clipLevel.
 * @param[out] thresholdLevel AGC threshold level.
 */
void
SCP16_getAGC(SCP_T *scp, unsigned *period, unsigned *clipLevel,
           unsigned *thresholdLevel);
/**
 * Set up AGC count event logic.
 *
 * This is a legacy 12-bit SCP function. Clip and threshold levels are provided
 * as unsigned integers in the range 0..0x7FF.
 *
 * In the case of 16 bit hardware the
 * clip and threshold levels are shifted left by 4 bits, thus setting the levels at
 * roughly the same places relative to a full scale signal. If you need the full 15
 * bits of precision when setting the clip and threshold levels use ::SCP16_setAGC().
 *
 * This function just sets up the event detection logic. You will also need to install
 * a handler for ::SCP_EVT_AGCCOUNT events if you want to respond to the events when generated.
 *
 * @param[in] scp pointer to SCP object.
 * @param[in] period AGC period.
 * @param[in] clipLevel AGC clipLevel.
 * @param[in] thresholdLevel AGC threshold level.
 */
void
SCP_setAGC(SCP_T *scp, unsigned period, unsigned clipLevel,
           unsigned thresholdLevel);
/**
 * Read AGC count event logic.
 *
 * This is a legacy 12-bit SCP function. Clip and threshold levels are retured
 * as unsigned integers in the range 0..0x7FF.
 *
 * In the case of 16 bit hardware the
 * clip and threshold levels are shifted right by 4 bits, thus corresponding with
 * values provided to ::SCP_setAGC. If you need to see the full 15 bits of precision
 * use ::SCP16_setAGC().
 *
 * @param[in] scp pointer to SCP object.
 * @param[out] period AGC period.
 * @param[out] clipLevel AGC clipLevel.
 * @param[out] thresholdLevel AGC threshold level.
 */
void
SCP_getAGC(SCP_T *scp, unsigned *period, unsigned *clipLevel,
           unsigned *thresholdLevel);
/**
 * Get number of samples clipped in last AGC period.
 *
 * @param[in] scp pointer to SCP object.
 * @param[out] clipI I channel clip count.
 * @param[out] clipQ Q channel clip count.
 * @return  Total clip count (clipI + clipQ)
 */
unsigned
SCP_getAGCClipCount(SCP_T *scp, unsigned *clipI, unsigned *clipQ);
/**
 * Get number of samples with absolute values greater than AGC threshold in last AGC period.
 *
 * @param[in] scp pointer to SCP object.
 * @param[out] threshI I channel threshold count.
 * @param[out] threshQ Q channel threshold count.
 * @return  Total threshold count (threshI + threshQ)
 */
unsigned
SCP_getAGCThreshCount(SCP_T *scp, unsigned *threshI, unsigned *threshQ);
/**
 * Get DC monitor value (I channel).
 *
 * This is a legacy function.
 * It is more efficient to use ::SCP16_getDCMonitors() than to make separate calls
 * to \c SCP_getDCMonitorI() and \c SCP_getDCMonitorQ().
 *
 * The return value can span the full  32-bit range on 16-bit SCPs but
 * is limited to a 24-bit range on 12-bit SCPs.
 *
 * @param[in] scp pointer to SCP object.
 * @return  DC monitor value (I)
 */
int
SCP_getDCMonitorI(SCP_T *scp);
/**
 * Get DC monitor value (Q channel).
 *
 * This is a legacy function.
 * It is more efficient to use ::SCP16_getDCMonitors() than to make separate calls
 * to \c SCP_getDCMonitorI() and \c SCP_getDCMonitorQ().
 *
 * The return value can span the full  32-bit range on 16-bit SCPs but
 * is limited to a 24-bit range on 12-bit SCPs.
 *
 * @param[in] scp pointer to SCP object.
 * @return  DC monitor value (Q).
 */
int
SCP_getDCMonitorQ(SCP_T *scp);

/**
 * Get DC monitor values (I and Q channels).
 *
 * The output values are DC measurements accumulated across
 * the AGC period.  There are 32 significant bits in the case of a 16-bit SCP,
 * or 24 significant bits in the case of a 12-bit SCP.  For a 12-bit SCP the output
 * values are left-shifted by 4 places; this means that dividing the output value by the AGC
 * period gives an average DC measurement in Q16.15 format, (for either type of SCP)
 * where unity is defined as full scale at the SCP input.
 *
 * @param[in] scp pointer to SCP object.
 * @param [out] dcmonI Pointer to DC monitor I value.
 * @param [out] dcmonQ Pointer to DC monitor Q value.
 */
void
SCP16_getDCMonitors(SCP_T *scp, int32_t *dcmonI, int32_t *dcmonQ);

/**
 * Configure DC monitor source.
 *
 * @param[in] scp pointer to SCP object.
 * @param [in] source  ::SCP_DCMON_DRM or ::SCP_DCMON_DCOFF
 * @return true: OK. false: Bad \c source value.
 */

bool SCP_setDCMonitorSource(SCP_T *scp, SCP_DCMON_SOURCE_T source);
/**
 * Get DC monitor source configuration .
 *
 * @param[in] scp pointer to SCP object.
 * @return Active DC monitor source selection
 */

SCP_DCMON_SOURCE_T SCP_getDCMonitorSource(SCP_T *scp);

/**
 * Set frame size.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] frame Frame A (SCP_FRAME_A) or frame B (SCP_FRAME_B)
 * @param[in] symbolsPerFrame Symbols per frame (0..1023).
 * @param[in] symbolPeriod Number of samples in normal symbols (0..65535).
 * @param[in] finalSymbolPeriod number of samples in last symbol of frame (0..65535).
 */
void
SCP_setFrameSize(SCP_T *scp, SCP_FRAME_T frame, unsigned symbolsPerFrame,
                 unsigned symbolPeriod, unsigned finalSymbolPeriod);
/**
 * Read frame size settings.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] frame Frame A (SCP_FRAME_A) or frame B (SCP_FRAME_B)
 * @param[out] symbolsPerFrame Symbols per frame (0..1023).
 * @param[out] symbolPeriod Number of samples in normal symbols (0..65535).
 * @param[out] finalSymbolPeriod number of samples in last symbol of frame (0..65535).
 */
void
SCP_getFrameSize(SCP_T *scp, SCP_FRAME_T frame, unsigned *symbolsPerFrame,
                 unsigned *symbolPeriod, unsigned *finalSymbolPeriod);
/**
 * Adjust the size of the current frame.
 *
 * If the frame counter has exceeded the adjusted size,
 * alignment may be lost. Perform the adjustment on a frame
 * compare event close to the start of frame to avoid this.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] frame Frame A (SCP_FRAME_A) or frame B (SCP_FRAME_B)
 * @param[in] symbolsPerFrame Symbols per frame (0..1023).
 * @param[in] finalSymbolPeriod number of samples in last symbol of frame (0..65535).
 */
void
SCP_adjustFrameSize(SCP_T *scp, SCP_FRAME_T frame, unsigned symbolsPerFrame,
                    unsigned finalSymbolPeriod);
/**
 * Adjust the size of the next regular (not final) symbol.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] frame Frame A (SCP_FRAME_A) or frame B (SCP_FRAME_B)
 * @param[in] symbolPeriod Number of samples in normal symbols (0..65535).
 */
void
SCP_adjustSymbolSize(SCP_T *scp, SCP_FRAME_T frame, unsigned symbolPeriod);
/**
 * Synchronise parameters.
 *
 * Synchronise parameter updates to a specific sample of each
 * symbol. This ensures that all signal processing updates such
 * as gain, phase angle, resampler, carrier offset modifications
 * take place at a known time. This function is used to specify
 * the sample number on which the update occurs. To enable
 * synchronisation set the syncUpdates parameter when calling SCP_configure().
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] frame Frame A (SCP_FRAME_A) or frame B (SCP_FRAME_B)
 * @param[in] sampleNumber Sample number within symbol when updates are applied.
 */
void
SCP_synchroniseParameters(SCP_T *scp, SCP_FRAME_T frame, unsigned sampleNumber);
/**
 * Read symbol counter from a frame counter
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] frame Frame A (SCP_FRAME_A), frame B (SCP_FRAME_B) or current frame (SCP_FRAME_NOCHANGE).
 * @return  Symbol number.
 **/
unsigned
SCP_getSymbolNum(SCP_T *scp, SCP_FRAME_T frame);
/**
 * Read sample counter from a frame counter
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] frame Frame A (SCP_FRAME_A), frame B (SCP_FRAME_B) or current frame (SCP_FRAME_NOCHANGE).
 * @return  Sample number.
 **/
unsigned
SCP_getSampleNum(SCP_T *scp, SCP_FRAME_T frame);
/**
 * Enable the ISCR counter.
 *
 * @param[in] scp Pointer to SCP object.
 *
 */
void
SCP_enableIscr(SCP_T *scp);
/**
 * Disable the ISCR counter.
 *
 * @param[in] scp Pointer to SCP object.
 *
 */
void
SCP_disableIscr(SCP_T *scp);
/**
 * Set the ISCR counter.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[out] iscrHigh 24 bits of ISCR counter.
 * @param[out] iscrLow 24 bits of ISCR counter.
 *
 */
void
SCP_setIscr(SCP_T *scp, unsigned iscrHigh, unsigned iscrLow);
/**
 * Read the value of the ISCR counter.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[out] iscrHigh 24 bits of ISCR counter.
 * @param[out] iscrLow 24 bits of ISCR counter.
 *
 */
void
SCP_getIscr(SCP_T *scp, unsigned *iscrHigh, unsigned *iscrLow);
/**
 * Read the value of the ISCR latched on the last symbol start
 *
 * @param[in] scp Pointer to SCP object.
 * @param[out] iscrHigh 24 bits of latched ISCR counter.
 * @param[out] iscrLow 24 bits of latched ISCR counter.
 *
 */
void
SCP_getIscrLatched(SCP_T *scp, unsigned *iscrHigh, unsigned *iscrLow);
/**
 * Set up impulse filter.
 *
 * Set thresholds for impulsive noise filter windows.
 * Sixteen 8-bit thresholds can be set corresponding
 * to impulses of 1 to 16 samples duration.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] enable true: enable impulse filtering. false: disable impulse filtering.
 * @param[in] thresholds Pointer to array of thresholds.
 *
 */
void
SCP_setImpulseFilter(SCP_T *scp, bool enable, uint8_t *thresholds);
/**
 * Read impulse filter configuration.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[out] enabled true: impulse filtering enabled. false: impulse filtering disabled.
 * @param[out] thresholds Pointer to array of thresholds.
 *
 */
void
SCP_getImpulseFilter(SCP_T *scp, bool *enabled, uint8_t *thresholds);
/**
 * Read impulsive noise filter energy removed.
 *
 * @param[in] scp Pointer to SCP object.
 * @return  The sum of the energy in all the IQ samples deleted.
 */
unsigned
SCP_getNoiseEnergy(SCP_T *scp);
/**
 * Read impulsive noise filter counters.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] noiseType
 * @return  Impulsive noise filter counter.
 */
unsigned
SCP_getImpulseCounts(SCP_T *scp, SCP_NOISE_T noiseType);
/**
 * Process sample as though it had originated from the ADC.
 * I and Q samples are two's complement values limited to a 12-bit
 * or 16-bit range for 12-bit or 16-bit SCPs respectively.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] sampleI Sample value (I)
 * @param[in] sampleQ Sample value (Q)
 */
void
SCP_writeSample(SCP_T *scp, int32_t sampleI, int32_t sampleQ);
/**
 * Read the AGC sample counter.
 *
 * @param[in] scp Pointer to SCP object.
 * @return  AGC sample counter.
 */
unsigned
SCP_getAGCSampleCount(SCP_T *scp);
/**
 * Set the DC offset control for I and Q channels.
 *
 * This applies a correction for DC offset in the digital domain.
 * It is intended for short-time offset correction where an external
 * analogue correction will take time to take effect. Setting non-
 * zero DC offsets here will reduce headroom through the SCP.
 *
 * The control values are 24-bit signed values,
 * which are added to the input values from the ADC.
 * The values are interpreted as having a 12-bit integer part
 * and a 12-bit fractional part
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] offsetI I channel offset value.
 * @param[in] offsetQ Q channel offset value.
 */
void
SCP_setDCOffsets(SCP_T *scp, int offsetI, int offsetQ);
/**
 * Read the DC offset controls for I and Q channels.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[out] offsetI I channel offset value.
 * @param[out] offsetQ Q channel offset value.
 */
void
SCP_getDCOffsets(SCP_T *scp, int *offsetI, int *offsetQ);

/**
 * Set the DC offset control for I and Q channels.
 *
 * This applies a correction for DC offset in the digital domain.
 * It is intended for short-time offset correction where an external
 * analogue correction will take time to take effect. Setting non-
 * zero DC offsets here will reduce headroom through the SCP.
 *
 * The offset value may be interpreted as Q0.31 where unity represents
 * full-scale data in the SCP.
 *
 * The most significant 12 bits (in the case of a 12-bit SCP) or
 * 16 bits (in the case of a 16-bit SCP) of the offset value will be added to the data as
 * it flows through the SCP.  It is also possible to use up to 12 additional
 * fractional bits (for a 12-bit SCP, or 8 for a 16-bit SCP) in the offset value.
 * These fractional bits are used in an accumulator to produce a dithering of the LSB
 * in the offset adder, so as to produce sub-LSB average offset adjustments.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] offsetI I channel offset value.
 * @param[in] offsetQ Q channel offset value.
 */
void
SCP16_setDCOffsets(SCP_T *scp, int32_t offsetI, int32_t offsetQ);

/**
 * Read the DC offset controls for I and Q channels.  Offset values are as defined for
 * SCP16_setDCOffsets
 *
 * @param[in] scp Pointer to SCP object.
 * @param[out] offsetI I channel offset value.
 * @param[out] offsetQ Q channel offset value.
 */
void
SCP16_getDCOffsets(SCP_T *scp, int32_t *offsetI, int32_t *offsetQ);

/**
 * Set external gain control for channel 1.
 *
 * This is a 12-bit  quantity that is presented on a
 * digital output port of the SCP. It is typically coupled to a PDM
 * DAC to drive external AGC circuitry.
 *
 * The gain value is treated as unsigned by the driver - it's actual interpretation
 * depends on the externally connected hardware.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] gain Gain value.
 */
void
SCP_setExtGainControl1(SCP_T *scp, unsigned gain);
/**
 * Read external gain control for channel 1.
 *
 * @param[in] scp Pointer to SCP object.
 * @return  Gain value.
 */
unsigned
SCP_getExtGainControl1(SCP_T *scp);
/**
 * Set external gain control for channel 2.
 *
 * This is a 12-bit  quantity that is presented on a
 * digital output port of the SCP. It is typically coupled to a PDM
 * DAC to drive external AGC circuitry.
 *
 * The gain value is treated as unsigned by the driver - it's actual interpretation
 * depends on the externally connected hardware.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] gain Gain value.
 */
void
SCP_setExtGainControl2(SCP_T *scp, unsigned gain);
/**
 * Read external gain control for channel 2.
 *
 * @param[in] scp Pointer to SCP object.
 * @return  Gain value.
 */
unsigned
SCP_getExtGainControl2(SCP_T *scp);
/**
 * Set external offset control for channel 1.
 *
 * This is a 12-bit  quantity that is presented on a
 * digital output port of the SCP. It is typically coupled to a PDM
 * DAC to drive external AGC circuitry.
 * DAC to drive external analogue DC offset compensation
 * circuitry.
 *
 * The offset value is treated as unsigned by the driver - it's actual interpretation
 * depends on the externally connected hardware.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] offset Offset value.
 */
void
SCP_setExtOffsetControl1(SCP_T *scp, unsigned offset);
/**
 * Read external offset control for channel 1.
 *
 * @param[in] scp Pointer to SCP object.
 * @return  Offset value.
 */
unsigned
SCP_getExtOffsetControl1(SCP_T *scp);

/**
 * Set external offset control for channel 2.
 *
 * This is a 12-bit  quantity that is presented on a
 * digital output port of the SCP. It is typically coupled to a PDM
 * DAC to drive external AGC circuitry.
 * DAC to drive external analogue DC offset compensation
 * circuitry.
 *
 * The offset value is treated as unsigned by the driver - it's actual interpretation
 * depends on the externally connected hardware.
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] offset Gain value.
 */
void
SCP_setExtOffsetControl2(SCP_T *scp, unsigned offset);
/**
 * Read external offset control for channel 2.
 *
 * @param[in] scp Pointer to SCP object.
 * @return  Offset value.
 */
unsigned
SCP_getExtOffsetControl2(SCP_T *scp);
/*----------------------------- SCP Event Handling --------------------*/

/**
 * Install/remove an event handler for a SCP.
 * NB Installing an event handler implicitly enables handling of that event
 *
 * @param[in]   scp        SCP whose events are to be handled
 * @param[in]   handler    Pointer to handler function (NULL to remove the current handler)
 * @param[in]   event      Event to be handled (a single SCP_EVT_T value)
 * @param[in]   parameter  Arbitrary parameter to be passed to the handler when invoked
 * @param[in]   clear      true: Clear any pending event interrupt before the new handler is installed
 *
 * @return      Pointer to previous handler (or NULL)
 */
void
SCP_installEventHandler(SCP_T *scp, SCP_EVENT_HANDLER_T *handler,
                        SCP_EVT_T event, void *parameter, bool clear);
/**
 * Globally disable event handling on a SCP. (individual events can be disabled by
 * removing their handlers).
 *
 * @param[in]   scp     Pointer to SCP object whose event handling is  to be disabled
 *
 * @return      Anonymous "mask" value for use by SCP_restoreEventHandling
 */

uint32_t
SCP_disableEventHandling(SCP_T *scp);
/**
 * Globally re-enable event handling on a SCP.
 *
 * @param[in]   scp     Pointer to SCP object whose event handling is  to be restored
 * @param[in]   mask    Enable mask returned by a previous call to SCP_disableEventHandling
 * @param[in]   clear   true: Clear any pending masked interrupts before they are unmasked
 */
void
SCP_restoreEventHandling(SCP_T *scp, uint32_t mask, bool clear);
#if __UCC__ >= 420
/**
 * Enable/Disable the Mixer metric.
 *
 * @param[in] scp Pointer to SCP object
 * @param[in] enable true: Mixer metric enabled. false: Mixer Metric disabled.
 */
void
SCP_setMixerMetric(SCP_T *scp, bool enable);
/**
 * Read Mixer Metric value
 *
 * @param[in] scp Pointer to SCP object.
 * @param[out] enabled true: Mixer Metric is enabled.
 * @return  IQ Correlator value in Q31 format (21 significant fraction bits)
 */
int32_t
SCP_getMixerMetric(SCP_T *scp, bool *enabled);
/**
 * Set energy monitor controls.
 *
 * This function is not available on UCCs earlier than Volt
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] blockSize Block size for energy monitor. Must be a power of two in the range 2^6..2^20.
 * @param[in] CICEnable true: Enable CIC energy monitor. false: Disable CIC energy monitor.
 * @param[in] lateGainEnable true: Enable late gain energy monitor. false: Disable late gain energy monitor.
 *
 * @return true: OK. false: Function failed - invalid \c blockSize.
 */

bool SCP_setEnergyControls(SCP_T *scp, unsigned blockSize, bool CICEnable, bool lateGainEnable);
/**
 * Read energy monitor controls. Reads back the values set by ::SCP_setEnergyControls().
 *
 * This function is not available on UCCs earlier than Volt
 *
 * @param[in] scp Pointer to SCP object.
 * @param[in] blockSize Block size for energy monitor.
 * @param[in] CICEnable true: CIC energy monitor enabled. false: CIC energy monitor disabled.
 * @param[in] lateGainEnable true: Late gain energy monitor enabled. false: Late gain energy monitor disabled.
 *
 */
void SCP_getEnergyControls(SCP_T *scp, unsigned *blockSize, bool *CICEnable, bool *lateGainEnable);

/**
 * Read CIC energy monitor value.
 *
 * The energy value is updated and the function returns true if  the CIC energy monitor is enabled.
 * Otherwise the function returns fails and the energy value is not updated.
 *
 * The energy value is provided as a signed fixed point value in Q31 format with the value <~1.0 representing full scale.
 * Only positive values 0.0 to + 1.0 are returned. There are 24 significant fraction bits
 *
 * This function is not available on UCCs earlier than Volt
 *
 * @param[in] scp Pointer to SCP object.
 * @param[out] energy Updated with CIC energy value. Q31 format.
 * @return true: OK. false: CIC energy monitor not enabled
 */

bool SCP_getCICEnergyValue(SCP_T *scp, int32_t *energy);
/**
 * Read late gain energy monitor value.
 *
 * The energy value is updated and the function returns true if the late gain energy monitor is enabled.
 * Otherwise the function returns fails and the energy value is not updated.
 *
 * The energy value is provided as a signed fixed point value in Q31 format with the value <~1.0 representing full scale.
 * Only positive values 0.0 to + 1.0 are returned. There are 24 significant fraction bits
 *
 * This function is not available on UCCs earlier than Volt
 *
 * @param[in] scp Pointer to SCP object.
 * @param[out] energy Updated with CIC energy value. Q31 format.
 * @return true: OK. false: Late gain energy monitor not enabled.
 */

bool SCP_getLateGainEnergyValue(SCP_T *scp, int32_t *energy);
#endif
/*-------------------------------------------------------------------------*/
/* MCP public interface                                                    */
/*------------------------------------------------------------------------*/

/**
 * Type for representing MCP data addresses
 */
typedef uint32_t MCP_DATA_ADDRESS_T;
/**
 * Type for GRAM word in the region which maps MCP data to host data by sign extending to 32 bits.
 * This is normally used for accessing integer data in MCP data memory.
 *
 * This is volatile, because the MCP may change data in this region without the knowledge of
 * the host program.
 */
typedef volatile int32_t MCP_GRAM_INT_T;
/**
 * Type for GRAM word in the region which maps MCP data to host data by left justifying in 32 bits.
 * This is normally used for accessing fixed point (MCP DBL type) data in MCP data memory.
 *
 * This is volatile, because the MCP may change data in this region without the knowledge of
 * the host program.
 */
typedef volatile uint32_t MCP_GRAM_DBL_T;
/**
 * Type for GRAM word in the region which maps MCP data to host data as a packed array of 24 bit words.
 * From the point of view of the host programmer we have to treat this area as a byte array, otherwise
 * the alignment assumptions in typical C compilers cause invalid accesses.
 *
 * This is volatile, because the MCP may change data in this region without the knowledge of
 * the host program.
 */
typedef volatile uint8_t MCP_GRAM_PKD_T;

/**
 * Type for GRAM word in the region which maps MCP data to host data as 32-bit complex.
 * This is normally used for accessing fixed point complex (MCP CMPLX type) data in MCP data memory.
 *
 * Note that this does not allow separate access to the real and imaginary components, because in the complex
 * mapped region of GRAM only aligned 32-bit accesses are permitted.
 *
 * This is volatile, because the MCP may change data in this region without the knowledge of
 * the host program.
 */

typedef volatile uint32_t MCP_GRAM_CMPLX_T;
/**
 * Return UCC "parent" or "owner" UCC of an MCP
 *
 * @param[in]   mcp     Pointer to MCP object
 * @return      pointer to parent UCC object
 */
UCC_T *
MCP_parent(MCP_T *mcp);
/**
 * Return Id number of an MCP
 *
 * @param[in]   mcp     Pointer to MCP object
 * @return      Id number of MCP
 */
unsigned
MCP_id(MCP_T *mcp);

/**
 * Reset a MCP hardware instance.
 *
 * @param[in]   mcp     Pointer to MCP object
 *
 */
void
MCP_reset(MCP_T *mcp);
/**
 * Get MCP identifier
 *
 * The MCP Id is identified by two 8-bit numbers, known as the group and core ids, along
 * with a 16-bit configuration value. This function simply reads these numbers directly from the
 * MCP hardware registers.
 *
 * Beware - there may not be any obvious correspondence between the values returned by this function
 * and published MCP names.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[out]  group           Group Id.
 * @param[out]  core            Core Id.
 * @param[out]  config          Configuration value
 */
void
MCP_getCoreId(MCP_T *mcp, uint8_t *group, uint8_t *core, uint16_t *config);

/**
 * Run a MCP
 *
 * @param[in]   mcp     Pointer to MCP object
 */
void
MCP_run(MCP_T *mcp);
/**
 * Stop a MCP
 *
 * @param[in]   mcp     Pointer to MCP object
 */
void
MCP_stop(MCP_T *mcp);
/**
 * Poll a MCP until it halts for any reason.
 *
 * @param[in]   mcp     Pointer to MCP object
 */
void
MCP_pollForHalt(MCP_T *mcp);
void
/**
 * Poll a MCP until it signals.
 *
 * @param[in]   mcp     Pointer to MCP object
 */
MCP_pollForSignal(MCP_T *mcp);
void
/**
 * Poll a MCP until it halts due to executing a HALT instruction.
 *
 * @param[in]   mcp     Pointer to MCP object
 */
MCP_pollForMCPIssuedHalt(MCP_T *mcp);

/**
 *  Acknowledge (clear) a MCP signal. This function should be used after polling for a signal.
 *  It is not necessary to acknowledge a signal if an event handler is installed. In such cases
 *  the event handling system performs the acknowledge automatically.
 *
 * @param[in]   mcp     Pointer to MCP object
 */
void
MCP_ackSignal(MCP_T *mcp);

/**
 *  Configure the source of events that release the MCP from a WAIT instruction.
 *  Note, this should not be confused with MCP events which wake the host
 *
 * @param[in]   mcp         Pointer to MCP object
 * @param[in]   source      Source identifier, either MCP_EVT_SRC_EFS or MCP_EVT_SRC_LEGACY_EFC
 * @param[in]   orFlagNum   OR flag number to vector to release MCP from WAIT (only used for MCP_EVT_SRC_EFS)
 */
void
MCP_configWaitSource(MCP_T *mcp, MCP_EVENT_SOURCE_T source, unsigned orFlagNum);

/**
 *  Configure the source of events that interrupt the MCP.
 *  Note, this should not be confused with MCP events which interrupt the host
 *
 * @param[in]   mcp         Pointer to MCP object
 * @param[in]   source      Source identifier, either MCP_EVT_SRC_EFS or MCP_EVT_SRC_LEGACY_EFC
 * @param[in]   priority    Priority number 1-7 (1 is highest) (only used for MCP_EVT_SRC_EFS)
 * @param[in]   orFlagNum   OR flag number to vector to interrupt MCP (only used for MCP_EVT_SRC_EFS)
 */
void
MCP_configInterruptSource(MCP_T *mcp, MCP_EVENT_SOURCE_T source,
                          unsigned priority, unsigned orFlagNum);

/**
 * Enumeration of MCP halt reasons returned by MCP_getStatus()
 */
typedef enum
{
    /** Halt due to MCP reset*/
    MCP_HALT_RESET,
    /** Halt due to host command*/
    MCP_HALT_HOST,
    /** HALT due to MCP executing HALT instruction */
    MCP_HALT_HALTINST,
    /** Halt due to single stepping */
    MCP_HALT_SINGLESTEP
} MCP_HALT_T;
/**
 * Get status of a MCP.
 *
 * @param[in]   mcp     Pointer to MCP object.
 * @param[out]  running true: MCP is running. false MCP is not running.
 * @param[out] waiting  true: MCP is waiting (due to WAIT instruction). false: MCP is not waiting.
 * @param[out] lastHalt Reason fo the last MCP halt
 */
void
MCP_getStatus(MCP_T *mcp, bool *running, bool *waiting, MCP_HALT_T *lastHalt);
/**
 * Set up mapping from MCP address space to GRAM - For the purposes of this function
 * GRAM addresses are considered to be 24-bit word addresses starting at zero. I.e. this function
 * requires addresses in the GRAM's own address space, not host processor addresses.
 *
 * @param[in]  mcp     Pointer to MCP object
 * @param[in]  baseA     GRAM address (based at 0) for region A.
 * @param[in]  baseB     GRAM address (based at 0) for region B.
 * @param[in]  baseL     GRAM address (based at 0) for region L.
 *
 */
void
MCP_mapMemoryToGRAM(MCP_T *mcp, UCCP_GRAM_ADDRESS_T baseA,
                    UCCP_GRAM_ADDRESS_T baseB, UCCP_GRAM_ADDRESS_T baseL);
/**
 *  Read MCP program counter
 *
 * @param[in]   mcp     Pointer to MCP object
 *
 * @return  MCP program counter value
 */
unsigned
MCP_getPC(MCP_T *mcp);
/**
 *  Set MCP program counter
 *
 * @param[in]   mcp     Pointer to MCP object
 * @param[in]   pc     program counter value
 *
 */
void
MCP_setPC(MCP_T *mcp, unsigned pc);
/**
 *  Reset MCP cycle counter to zero
 *
 * @param[in]   mcp     Pointer to MCP object
 */
void
MCP_resetCycleCount(MCP_T *mcp);
/**
 *  Reset MCP instruction issue counter to zero
 *
 * @param[in]   mcp     Pointer to MCP object
 */
void
MCP_resetIssueCount(MCP_T *mcp);
/**
 * Reset MCP wait counter to zero
 *
 * @param[in]   mcp     Pointer to MCP object
 */
void
MCP_resetWaitCount(MCP_T *mcp);
/**
 *  Read MCP cycle counter
 *
 * @param[in]   mcp     Pointer to MCP object
 *
 * @return  MCP cycle counter value
 */
unsigned
MCP_getCycleCount(MCP_T *mcp);
/**
 *  Read MCP instruction issue counter
 *
 * @param[in]   mcp     Pointer to MCP object
 *
 * @return  MCP instruction issue counter value
 */
unsigned
MCP_getIssueCount(MCP_T *mcp);
/**
 *  Read MCP wait counter
 *
 * @param[in]   mcp     Pointer to MCP object
 *
 * @return  MCP wait counter value
 */
unsigned
MCP_getWaitCount(MCP_T *mcp);

/**
 * Load a program image from a family stream onto a MCP
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   streamPtr       Pointer to image family stream
 * @param[in]   imageId         Logical id of image to load
 * @param[in]   useMap          Update the MCP GRAM map with that found in the image stream
 *
 * @return  true for success, false for failure.
 */

bool
MCP_loadImage(MCP_T *mcp, uint8_t *streamPtr, int imageId, bool useMap);
/**
 * Load a program image's code from a family stream onto a MCP
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   streamPtr       Pointer to image family stream
 * @param[in]   imageId         Logical id of image code to load
 * @param[in]   useMap          Update the MCP GRAM map with that found in the image stream
 *
 * @return  true for success, false for failure.
 */

bool
MCP_loadImageCode(MCP_T *mcp, uint8_t *streamPtr, int imageId, bool useMap);
/**
 * Load a program image's data from a family stream onto a MCP
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   streamPtr       Pointer to image family stream
 * @param[in]   imageId         Logical id of image data to load
 * @param[in]    useMap         Update the MCP GRAM map with that found in the image stream
 *
 * @return  true for success, false for failure.
 */

bool
MCP_loadImageData(MCP_T *mcp, uint8_t *streamPtr, unsigned imageId, bool useMap);
/**
 * Query a family stream for information about the logical images it contains
 *
 * @param[in]   streamPtr       Pointer to image family stream
 * @param[out]  familySize      Length of the stream in bytes
 * @param[out]  maxImageId      Maximum logical image id in the stream
 *
 * @return  true for success, false for failure(bad stream Id).
 */

bool
MCP_imageInfo(uint8_t *streamPtr, unsigned *familySize, unsigned *maxImageId);
/**
 * Query a family stream for the GRAM memory mapping information associated with a particular image
 *
 * @param[in]   streamPtr       Pointer to image family stream
 * @param[in]   imageId         Logical image Id
 * @param[out]  baseA           GRAM location of MCP A memory region
 * @param[out]  sizeA           Configured size MCP A memory region
 * @param[out]  baseB           GRAM location of MCP B memory region
 * @param[out]  sizeB           Configured size MCP B memory region
 * @param[out]  baseL           GRAM location of MCP L memory region
 * @param[out]  sizeL           Configured size MCP L memory region
 *
 * @return  true for success, false for failure(bad imageId).
 */

bool
MCP_imageMap(uint8_t *streamPtr, unsigned imageId, UCCP_GRAM_ADDRESS_T *baseA,
             unsigned *sizeA, UCCP_GRAM_ADDRESS_T *baseB, unsigned *sizeB,
             UCCP_GRAM_ADDRESS_T *baseL, unsigned *sizeL);

/*
 * MCP data access functions (1) - copying between host memory and MCP memory
 *
 * These are all array based. There are no equivalent "single word" copy functions
 * since the saving would be slight. With short arrays, the processing is dominated by the
 * address mapping calculations.
 */
/**
 * Copy an array of 8-bit fixed point values to 24-bit MCP memory. Each input byte will be
 * left justified in a 24-bit MCP memory word, with the least significant 16 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_write8dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint8_t *data,
              unsigned count);
/**
 * Copy an array of 8-bit signed values to 24-bit MCP memory. Each input byte will be
 * sign extended into a 24-bit MCP memory word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @return                      Number of items actually copied.
 * @param[in]   count           Number of data values to copy
 */
unsigned
MCP_write8int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int8_t *data,
              unsigned count);
/**
 * Copy an array of 8-bit unsigned integer values to 24-bit MCP memory. Each input byte will be
 * right justified in a 24-bit MCP memory word, with the most significant 16 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_write8uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint8_t *data,
               unsigned count);
/**
 * Copy an array of 16-bit fixed point values to 24-bit MCP memory. Each input word will be
 * left justified in a 24-bit MCP memory word, with the least significant 8 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_write16dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t *data,
               unsigned count);
/**
 * Copy an array of 16-bit signed values to 24-bit MCP memory. Each input word will be
 * sign extended into a 24-bit MCP memory word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_write16int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int16_t *data,
               unsigned count);
/**
 * Copy an array of 16-bit unsigned integer values to 24-bit MCP memory. Each input word will be
 * right justified in a 24-bit MCP memory word, with the most significant 16 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_write16uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t *data,
                unsigned count);
/**
 * Copy an array of 16-bit fixed point complex values to 24-bit MCP memory.
 * Bits [0..7] of each input word are copied to bits [4..11] of the MCP word
 * Bits [8..15] of each input word are copied to bits [16..23] of the MCP word
 * Bits [0..3] and [12..15] of the MCP word are zeroed.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_write16cpx(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t *data,
               unsigned count);
/**
 * Copy an array of 32-bit fixed point values to 24-bit MCP memory.
 * Bits [8..31] of each input word are copied to the corresponding MCP word
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_write32dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t *data,
               unsigned count);
/**
 * Copy an array of 32-bit signed integer values to 24-bit MCP memory.
 * Bits [0..23] of each input word are copied to the corresponding MCP word
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_write32int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int32_t *data,
               unsigned count);
/**
 * Copy an array of 32-bit unsigned integer values to 24-bit MCP memory.
 * Bits [0..23] of each input word are copied to the corresponding MCP word
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_write32uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t *data,
                unsigned count);
/**
 * Copy an array of 32-bit fixed point complex values to 24-bit MCP memory.
 * Bits [4..15] of each input word are copied to bits [0..11] of the MCP word
 * Bits [20..31] of each input word are copied to bits [12..23] of the MCP word
 * Bits [0..3] and [16..19] of the input word are ignored.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_write32cpx(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t *data,
               unsigned count);

/**
 * Copy multiple times a single 8-bit fixed point value to an array of consecutive 24-bit MCP memory. Each input byte will be
 * left justified in a 24-bit MCP memory word, with the least significant 16 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            data
 * @param[in]   count           Length of the array in the MCP memory
 * @return                      Number of items actually copied.
 */
unsigned
MCP_fill8dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint8_t data,
             unsigned count);
/**
 * Copy multiple times a single 8-bit signed value to an array of consecutive 24-bit MCP memory. Each input byte will be
 * sign extended into a 24-bit MCP memory word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            data
 * @return                      Length of the array in the MCP memory
 * @param[in]   count           Number of data values to copy.
 */
unsigned
MCP_fill8int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int8_t data,
             unsigned count);
/**
 * Copy multiple times a single 8-bit unsigned integer value to an array of consecutive 24-bit MCP memory. Each input byte will be
 * right justified in a 24-bit MCP memory word, with the most significant 16 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            data
 * @param[in]   count           Length of the array in the MCP memory
 * @return                      Number of items actually copied.
 */
unsigned
MCP_fill8uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint8_t data,
              unsigned count);
/**
 * Copy multiple times a single 16-bit fixed point value to an array of consecutive 24-bit MCP memory. Each input word will be
 * left justified in a 24-bit MCP memory word, with the least significant 8 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            data
 * @param[in]   count           Length of the array in the MCP memory
 * @return                      Number of items actually copied.
 */
unsigned
MCP_fill16dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t data,
              unsigned count);
/**
 * Copy multiple times a single 16-bit signed value to an array of consecutive 24-bit MCP memory. Each input word will be
 * sign extended into a 24-bit MCP memory word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            data
 * @param[in]   count           Length of the array in the MCP memory
 * @return                      Number of items actually copied.
 */
unsigned
MCP_fill16int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int16_t data,
              unsigned count);
/**
 * Copy multiple times a single 16-bit unsigned integer value to an array of consecutive 24-bit MCP memory. Each input word will be
 * right justified in a 24-bit MCP memory word, with the most significant 16 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            data
 * @param[in]   count           Length of the array in the MCP memory
 * @return                      Number of items actually copied.
 */
unsigned
MCP_fill16uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t data,
               unsigned count);
/**
 * Copy multiple times a single 16-bit fixed point complex value to an array of consecutive 24-bit MCP memory.
 * Bits [0..7] of each input word are copied to bits [4..11] of the MCP word
 * Bits [8..15] of each input word are copied to bits [16..23] of the MCP word
 * Bits [0..3] and [12..15] of the MCP word are zeroed.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            data
 * @param[in]   count           Length of the array in the MCP memory
 * @return                      Number of items actually copied.
 */
unsigned
MCP_fill16cpx(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t data,
              unsigned count);
/**
 * Copy multiple times a single 32-bit fixed point value to an array of consecutive 24-bit MCP memory.
 * Bits [8..31] of each input word are copied to the corresponding MCP word
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            data
 * @param[in]   count           Length of the array in the MCP memory
 * @return                      Number of items actually copied.
 */
unsigned
MCP_fill32dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t data,
              unsigned count);
/**
 * Copy multiple times a single 32-bit signed integer value to an array of consecutive 24-bit MCP memory.
 * Bits [0..23] of each input word are copied to the corresponding MCP word
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            data
 * @param[in]   count           Length of the array in the MCP memory
 * @return                      Number of items actually copied.
 */
unsigned
MCP_fill32int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int32_t data,
              unsigned count);
/**
 * Copy multiple times a single 32-bit unsigned integer value to an array of consecutive 24-bit MCP memory.
 * Bits [0..23] of each input word are copied to the corresponding MCP word
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            data
 * @param[in]   count           Length of the array in the MCP memory
 * @return                      Number of items actually copied.
 */
unsigned
MCP_fill32uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t data,
               unsigned count);
/**
 * Copy multiple times a single fixed point complex value to an array of consecutive 24-bit MCP memory.
 * Bits [4..15] of each input word are copied to bits [0..11] of the MCP word
 * Bits [20..31] of each input word are copied to bits [12..23] of the MCP word
 * Bits [0..3] and [16..19] of the input word are ignored.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            data
 * @param[in]   count           Length of the array in the MCP memory
 * @return                      Number of items actually copied.
 */
unsigned
MCP_fill32cpx(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t data,
              unsigned count);

/**
 * Copy an array of fixed point values from 24-bit MCP memory to 8-bit host memory.
 * Bits [16..23] of each MCP word are copied to the corresponding output byte.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_read8dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint8_t *data,
             unsigned count);
/**
 * Copy an array of integer values from 24-bit MCP memory to 8-bit host memory.
 * Bits [0..7] of each MCP word are copied to the corresponding output byte.
 * No rounding is performed.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_read8int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int8_t *data,
             unsigned count);
/**
 * Copy an array of integer values from 24-bit MCP memory to 8-bit host memory.
 * Bits [0..7] of each MCP word are copied to the corresponding output byte.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_read8uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint8_t *data,
              unsigned count);
/**
 * Copy an array of fixed point values from 24-bit MCP memory to 16-bit host memory.
 * Bits [8..23] of each MCP word are copied to the corresponding output word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_read16dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t *data,
              unsigned count);
/**
 * Copy an array of integer values from 24-bit MCP memory to 16-bit host memory.
 * Bits [0..15] of each MCP word are copied to the corresponding output word.
 * No rounding is performed.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_read16int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int16_t *data,
              unsigned count);
/**
 * Copy an array of integer values from 24-bit MCP memory to 16-bit host memory.
 * Bits [0..15] of each MCP word are copied to the corresponding output word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_read16uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t *data,
               unsigned count);
/**
 * Copy an array of fixed point complex values from MCP memory to 16-bit host memory.
 * Bits [4..11] of each input word are copied to bits [0..7] of the host word
 * Bits [12..23] of each input word are copied to bits [8..15] of the host word
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_read16cpx(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t *data,
              unsigned count);
/**
 * Copy an array of fixed point values from 24-bit MCP memory to 32-bit host memory.
 * Each MCP word are copied to bits [8..31] of the corresponding output word.
 * Bits [0..7] of the output word are zeroed.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_read32dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t *data,
              unsigned count);
/**
 * Copy an array of integer values from 24-bit MCP memory to 32-bit host memory.
 * Bits [0..23] of each MCP word are sign-extended to the corresponding output word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_read32int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int32_t *data,
              unsigned count);
/**
 * Copy an array of integer values from 24-bit MCP memory to 32-bit host memory.
 * Bits [0..23] of each MCP word are zero-extended to the corresponding output word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_read32uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t *data,
               unsigned count);
/**
 * Copy an array of fixed point complex values from MCP memory to 32-bit host memory.
 * Bits [0..11] of each input word are copied to bits [4..15] of the host word
 * Bits [12..23] of each input word are copied to bits [20..31] of the host word
 * Bits [0..3] and [16..19] of the host word are zeroed.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @return                      Number of items actually copied.
 */
unsigned
MCP_read32cpx(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t *data,
              unsigned count);

/*
 * MCP data access functions (2) - direct access to MCP memory as it appears in GRAM
 */

/**
 * Convert a MCP data address to an address in host memory suitable for accessing the
 * MCP data as a (sign-extended) integer. The integer range is limited to the low 24 bits
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 *
 * @return      Pointer to host mapped GRAM (or NULL if conversion fails)
 */
MCP_GRAM_INT_T *
MCP_addrInt2host(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress);
/**
 * Convert a MCP data address to an address in host memory suitable for accessing the
 * MCP data as a fixed point (MCP double) value. The fixed point precision is limited
 * to the high 24 bits.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 *
 * @return      Pointer to host mapped GRAM (or NULL if conversion fails)
 */
MCP_GRAM_DBL_T *
MCP_addrDbl2host(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress);
/**
 * Convert a MCP data address to an address in host memory suitable for accessing the
 * MCP data as a fixed point complex value. The precision of each complex component is
 * limited to the high 12 bits.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 *
 * @return      Pointer to host mapped GRAM (or NULL if conversion fails)
 */
MCP_GRAM_CMPLX_T *
MCP_addrCpx2host(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress);
/**
 * Convert a MCP data address to an address in host memory suitable for accessing the
 * MCP data as a packed byte sequence.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 *
 * @return      Pointer to host mapped GRAM (or NULL if conversion fails)
 */
MCP_GRAM_PKD_T *
MCP_addrPkd2host(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress);

/**
 * Convert an address in host memory stored as a (sign-extended) integer to an address
 * of MCP memory in the appropriate region. The integer range is limited to the low 24 bits.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   gramAddress     Target address in GRAM address space
 *
 * @return      Pointer to host mapped GRAM (or NULL if conversion fails)
 */
MCP_DATA_ADDRESS_T
MCP_hostInt2Mcp(MCP_T *mcp, void *gramAddress);
/**
 * Convert an address in host memory stored as a fixed point (MCP double) value to an address
 * of MCP memory in the appropriate region. The fixed point precision is limited
 * to the high 24 bits.a packed byte sequence
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   gramAddress     Target address in GRAM address space
 *
 * @return      Pointer to host mapped GRAM (or NULL if conversion fails)
 */

MCP_DATA_ADDRESS_T
MCP_hostDbl2Mcp(MCP_T *mcp, void *gramAddress);
/**
 * Convert an address in host memory stored as a fixed point complex value to an address
 * of MCP memory in the appropriate region. The precision of each complex component is
 * limited to the high 12 bits
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   gramAddress     Target address in GRAM address space
 *
 * @return      Pointer to host mapped GRAM (or NULL if conversion fails)
 */

MCP_DATA_ADDRESS_T
MCP_hostCpx2Mcp(MCP_T *mcp, void *gramAddress);
/**
 * Convert an address in host memory stored as a packed byte sequence to an address
 * of MCP memory in the appropriate region.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   gramAddress     Target address in GRAM address space
 *
 * @return      Pointer to host mapped GRAM (or NULL if conversion fails)
 */
MCP_DATA_ADDRESS_T
MCP_hostPkd2Mcp(MCP_T *mcp, void *gramAddress);

/*----------------------------- MCP Event Handling --------------------*/

/**
 * Install an event handler for a MCP.
 * NB Installing an event handler implicitly enables handling of that event
 *
 * @param[in]   mcp        MCP whose events are to be handled
 * @param[in]   handler    Pointer to handler function (NULL to remove the current handler)
 * @param[in]   event      Event to be handled (a single MCP_EVT_T value)
 * @param[in]   parameter  Arbitrary parameter to be passed to the handler when invoked
 * @param[in]   clear      true: Clear any pending event interrupt before the new handler is installed
 *
 */
void
MCP_installEventHandler(MCP_T *mcp, MCP_EVENT_HANDLER_T *handler,
                        MCP_EVT_T event, void *parameter, bool clear);
/**
 * Globally disable event handling on a MCP. (individual events can be disabled by
 * removing their handlers).
 *
 * @param[in]   mcp     Pointer MCP object, for which event handling to be disabled
 *
 * @return      Anonymous "mask" value for use by MCP_restoreEventHandling()
 */

uint32_t
MCP_disableEventHandling(MCP_T *mcp);
/**
 * Globally re-enable event handling on an MCP.
 *
 * @param[in]   mcp     Pointer MCP object, for which event handling to be restored
 * @param[in]   mask    Enable mask returned by a previous call to MCP_disableEventHandling()
 * @param[in]   clear   true: Clear any pending masked interrupts before they are unmasked
 */
void
MCP_restoreEventHandling(MCP_T *mcp, uint32_t mask, bool clear);

/*----------------------------- MCP Clock Gating control --------------------*/
/**
 * Disable MCP clock
 *
 * @param[in]   mcp     Pointer to MCP object
 * @return      Previous clock control value - for use only with ::MCP_restoreClock
 */
uint32_t
MCP_disableClock(MCP_T *mcp);
/**
 * Enable MCP clock
 *
 * @param[in]   mcp     Pointer to MCP object
 * @return      Previous clock control value - for use only with ::MCP_restoreClock
 */
uint32_t
MCP_enableClock(MCP_T *mcp);
/**
 * Set MCP clock gating to automatic mode
 *
 * @param[in]   mcp     Pointer to MCP object
 * @return      Previous clock control value - for use only with ::MCP_restoreClock
 */
uint32_t
MCP_autogateClock(MCP_T *mcp);
/**
 * Restore MCP clock gating to a previous mode
 *
 * @param[in]   mcp     Pointer to MCP object
 * @param[in]   oldClk Previous clock control mode
 * @return      Previous clock control value - for use only with ::MCP_restoreClock
 */
uint32_t
MCP_restoreClock(MCP_T *mcp, uint32_t oldClk);
/*
 *  310 emulation mode functions - included from a separate file since these
 * will eventually be dropped
 */
#include "mcp310.h"
/*-------------------------------------------------------------------------*/
/* UCC public interface */
/*-------------------------------------------------------------------------*/
/**
 * Get a UCC's Id number
 *
 * @param[in]   ucc     pointer to UCC object
 * @return  UCC Id number (1..n)
 */
unsigned
UCC_id(UCC_T *ucc);
/**
 * Recursively reset a UCC hardware instance and all the peripherals that belong to it
 *
 * @param[in]   ucc     pointer to UCC object
 *
 */
void
UCC_reset(UCC_T *ucc);
/**
 * Get number of MCPs  belonging to a UCC
 *
 * @param[in]   ucc       pointer to UCC object
 * @return      Number of MCPs in the UCC
 *
 */
unsigned
UCC_getNumMCPs(UCC_T *ucc);
/**
 * Get pointer to a particular MCP  belonging to a UCC
 *
 * @param[in]   ucc       pointer to UCC object
 * @param[in]   mcpNumber MCP number (1..n)
 *
 * @return      Pointer to MCP object or NULL if mcpNumber out of range
 *
 */
MCP_T *
UCC_getMCP(UCC_T *ucc, unsigned mcpNumber);
/**
 * Get number of SCPs belonging to a UCC
 *
 * @param[in]   ucc       pointer to UCC object
 * @return     Number of SCPs in the UCC
 *
 */
unsigned
UCC_getNumSCPs(UCC_T *ucc);
/**
 * Get pointer to a particular SCP  belonging to a UCC
 *
 * @param[in]   ucc       pointer to UCC object
 * @param[in]   scpNumber SCP number (1..n)
 *
 * @return      Pointer to SCP object or NULL if scpNumber out of range
 *
 */
SCP_T *
UCC_getSCP(UCC_T *ucc, unsigned scpNumber);

/*-------------------------------------------------------------------------*/
/* Standard UCCP platform interface */
/*-------------------------------------------------------------------------*/

/**
 * Initialise the UCCP platform software.
 *
 * This function forms part of the standard platform interface which is implemented in a hardware-specific
 * way for each supported platform implementation.
 *
 * It is called once only, after the application framework startup.
 */
void
UCCP_init(void);

/**
 * Recursively reset all the UCC devices and their sub-components (MCPs, SCPs etc) on the platform.
 *
 * This function forms part of the standard platform interface which is implemented in a hardware-specific
 * way for each supported platform implementation.
 */
void
UCCP_reset(void);
/**
 * Shut down all activity on a UCCP by calling UCCP_reset() and then stopping the Meta thread.
 *
 * This is designed for putting the UCCP into a safe state before resetting the platform and rebooting
 * a new application. The function never returns.
 *
 * Note that activity on other Meta threads may also need to be shut down, but this function
 * doesn't attempt to do that.
 */
void UCCP_shutdown(void);

/**
 * Get pointer to a particular UCC in the UCCP platform.
 *
 * This function forms part of the standard platform interface which is implemented in a hardware-specific
 * way for each supported platform implementation.
 *
 * @param[in]   uccNumber UCC number (1..n)
 *
 * @return      Pointer to UCC object or NULL if uccNumber out of range

 */
UCC_T *
UCCP_getUCC(unsigned uccNumber);

/**
 * Get UCCP identifier
 *
 * This function forms part of the standard platform interface which is implemented in a hardware-specific
 * way for each supported platform implementation.
 *
 * The UCCP is identified by two 8-bit numbers, known as the group and core ids, along
 * with a 16-bit configuration value. This function simply reads these numbers directly from the
 * MCP hardware registers.
 *
 * Beware - there may not be any obvious correspondence between the values returned by this function
 * and published UCCP names.
 *
 * @param[out]  group           Group Id.
 * @param[out]  core            Core Id.
 * @param[out]  config          Configuration value.
 */
void
UCCP_getCoreId(uint8_t *group, uint8_t *core, uint16_t *config);

/**
 * Get UCCP revision
 *
 * This function forms part of the standard platform interface which is implemented in a hardware-specific
 * way for each supported platform implementation.
 *
 * The UCCP revision is identified by four 8-bit numbers, known as the designer, major, minor and
 * maintenance ids. This function simply reads these numbers directly from the
 * UCCP hardware registers.
 *
 * Beware - there may not be any obvious correspondence between the values returned by this function
 * and published UCCP names. In particular, the designer value has no significance outside Imagination's
 * internal product development organisation.
 *
 * @param[out]  designer     Designer revision value.
 * @param[out]  major        Major revision value.
 * @param[out]  minor        Minor revision value.
 * @param[out]  maintenance  Maintenance revision value.
 */
void
UCCP_getCoreRev(uint8_t *designer, uint8_t *major, uint8_t *minor,
                uint8_t *maintenance);

/**
 * Returns a pointer to a UCC platform information structure.
 *
 * The platform information structure contains the following items of information:
 *   platform name
 *   core major revision number
 *   core minor revision number
 *   core configuration ID
 *   size of global RAM in 24-bit words
 *   size of external RAM in bytes
 *
 * @return  a pointer to a UCC platform information structure.
 */
UCCP_INFO_T *
UCCP_getPlatformInfo(void);

/**
 * Returns the core version (major revision, minor revision and, optionally, configuration ID) from the
 * appropriate UCCP_CORE registers.
 *
 * @param[in]  includeConfigId  If true, include the core configuration ID in the returned value.
 *
 * @return  the core version from the UCCP_CORE registers.
 */
uint32_t
UCCP_getHWCoreVersion(bool includeConfigId);

/**
 * Returns the core version (major revision, minor revision and, optionally, configuration ID) from the
 * platform information structure that is returned by UCCP_getPlatformInfo().
 *
 * @param[in]  includeConfigId  If true, include the core configuration ID in the returned value.
 *
 * @return  the core version from the platform information structure.
 */
uint32_t
UCCP_getSWCoreVersion(bool includeConfigId);

/**
 * Pointer to platform-specific interrupt hardware descriptor. This is used by application framework and event
 * handling code to set up external interrupt vectoring.
 *
 * This item forms part of the standard platform interface which is implemented in a hardware-specific
 * way for each supported platform implementation.
 *
 */
extern QIO_IVDESC_T *UCCP_ivDesc;

/**
 * Pointer to platform-specific array of HWLEVELEXT register addresses.
 * This is used by event handling code to set up external interrupt processing.
 *
 * This item forms part of the standard platform interface which is implemented in a hardware-specific
 * way for each supported platform implementation.
 *
 */
extern unsigned *UCCP_levelRegs;

/*
 * DCP -> MCP helper functions.
 */

/** Attributes for ID based DCP parameters.
 *  This is akin to doing, for example, $q.head in the DCP tools.
 */
typedef enum
{
    /** Address of Q's head register */
    DCP_ATTR_Q_HEAD = 0,

    /** Address of Q's tail register */
    DCP_ATTR_Q_TAIL,

    /** Address of OR flag's vector register */
    DCP_ATTR_OR_VECTOR
} DCP_PARAM_ATTR_T;

/**
 * Returns an attribute of a DCP parameter.
 *
 * @param[in]  id    The ID of the parameter.
 * @param[in]  attr  The attribute to get.
 *
 * @return The attribute value
 */
uint32_t
MCP_getDcpParamAttr(DCP_PARAM_ID_T id, DCP_PARAM_ATTR_T attr);

/**
 * Writes an attribute of a device-context DCP parameter to an MCP location.
 *
 * @param[in]  device         The DCP device.
 * @param[in]  deviceParamId  The logical ID of the parameter.
 * @param[in]  mcp            The MCP to write to.
 * @param[in]  mcpAddr        The MCP location to write to.
 * @param[in]  attr           The attribute of the parameter to write.
 */
void
MCP_writeDcpDeviceParam(DCP_DEVICE_T *device, int deviceParamId, MCP_T *mcp,
                        MCP_DATA_ADDRESS_T mcpAddr, DCP_PARAM_ATTR_T attr);

/**
 * Writes an attribute of a use-context DCP parameter to an MCP location.
 *
 * @param[in]  pipeline    The DCP pipeline.
 * @param[in]  useId       The use ID within the pipeline.
 * @param[in]  useParamId  The logical ID of the parameter.
 * @param[in]  mcp         The MCP to write to.
 * @param[in]  mcpAddr     The MCP location to write to.
 * @param[in]  attr        The attribute of the parameter to write.
 */
void
MCP_writeDcpUseParam(DCP_PIPELINE_T *pipeline, int useId, int useParamId,
                     MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddr,
                     DCP_PARAM_ATTR_T attr);

/*-------------------------------------------------------------------------*/

#endif /* _UCCRT_H_ */

/*-------------------------------------------------------------------------*/
