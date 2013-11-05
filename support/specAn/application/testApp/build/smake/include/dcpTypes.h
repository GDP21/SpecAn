/*!
*****************************************************************************

 @file      dcpTypes.h

 @brief     DCP type definitions

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#ifndef _DCP_TYPES_H_
#define _DCP_TYPES_H_

/*-------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>

#include "rcdTypes.h"

/*-------------------------------------------------------------------------*/

/** Version of the DCP types */
#define DCP_TYPES_VERSION (19)

/*-------------------------------------------------------------------------*/

/* In Microsoft C disable warning about variable length array */
#ifdef _MSC_VER
#pragma warning(disable:4200)
#endif

/*-------------------------------------------------------------------------*/

/*
 * Image flags
 */
 
/** Image contains logging */
#define DCP_IMAGE_FLAGS_LOGGING (1 << 0)

/*-------------------------------------------------------------------------*/

/** Value to represent an invalid parameter ID */
#define DCP_INVALID_PARAM (0xffffffff)

/** An alias for DCP_INVALID_PARAM, where "invalid" really means "use the
 *  default".
 */
#define DCP_DEFAULT_PARAM (DCP_INVALID_PARAM)

/** Value to represent an invalid channel number */
#define DCP_INVALID_CHANNEL_NUM (-1)

/* Enumeration of target memory regions */
typedef enum
{
    DCP_MEM_REGION_SYS_MEM = 0,
    DCP_MEM_REGION_GRAM_DBL,
    DCP_MEM_REGION_GRAM_SXT,
    DCP_MEM_REGION_GRAM_CPX,
    DCP_MEM_REGION_GRAM_PKD
} DCP_MEM_REGION_T;

/*
 * Integer types
 */

/** Type to represent a word of the DCP machine */
typedef uint8_t  DCP_IMAGE_DATA8_T;
typedef uint32_t DCP_IMAGE_DATA32_T;

/** Type to represent an offset into a DCP image */
typedef uint32_t DCP_OFFSET_T;

/** Type to represent a parameter ID */
typedef unsigned int DCP_PARAM_ID_T;

/*
 * Structures
 */

/** Structure to describe an array of offsets into a DCP image */
typedef struct
{
    int          numElements;
    DCP_OFFSET_T offsets[];
} DCP_OFFSET_ARRAY_T;

/** Structure to describe an initialiser for an element of a data block */
typedef struct
{
    DCP_OFFSET_T       offset;
    DCP_IMAGE_DATA32_T value;
} DCP_DATA_INITIALISER_T;

/** Structure to describe an array of initialisers for elements of a data block */
typedef struct
{
    int                    numElements;
    DCP_DATA_INITIALISER_T initialisers[];
} DCP_DATA_INITIALISER_ARRAY_T;

/** Structure to describe DCP offsets information */
typedef struct
{
    int          numElements;
    int          base;
    DCP_OFFSET_T offsets[];
} DCP_OFFSET_INFO_T;

/** Structure to contain an array of DCP offsets information */
typedef struct
{
    /** The number of offsets */
    int numElements;

    /** The offsets information */
    DCP_OFFSET_INFO_T *offsetInfos[];
} DCP_OFFSET_INFO_ARRAY_T;

/** Structure to contain an array of parameter IDs */
typedef struct
{
    /** The number of parameter IDs */
    int numElements;

    /** The parameter IDs */
    DCP_PARAM_ID_T ids[];
} DCP_PARAM_ID_ARRAY_T;

/** Structure to contain information about device instances */
typedef struct
{
    /** Offset to the first use-context data block */
    DCP_OFFSET_T useDataBlockOffset;

    /** Size of use-context data blocks */
    uint32_t useDataBlockSize;

    /** Use-context code offsets, per channel **/
    DCP_OFFSET_ARRAY_T **useCodeOffsets;

    /** Offset to the first job-context data block */
    DCP_OFFSET_T jobDataBlockOffset;

    /** Size of job-context data blocks */
    uint32_t jobDataBlockSize;

    /** Job-context code offsets, per channel **/
    DCP_OFFSET_ARRAY_T **jobCodeOffsets;

    /** Number of jobs */
    int numJobs;

    /** Job queue ID */
    int jobQueue;

    /** Job queue depth */
    int jobQueueDepth;
} DCP_USE_INFO_T;

/** Structure to describe a DCP channel */
typedef struct
{
    /** The name of the channel */
    char *name;

    /** The channel number */
    int channelNum;

    /** Offset of the device code for this channel */
    DCP_OFFSET_T offset;

    /** The channel number that this channel is linked to */
    int linkedChannel;
} DCP_CHANNEL_T;

/** Structure to describe a DCP group.
 *  A group corresponds to a collection of channels that may be shared by
 *  one or more DCP devices.
 */
typedef struct
{
    /** The name of the group */
    char *name;

    /** The number of channels in this group */
    int numChannels;

    /** Information about the DCP channels used by this group */
    DCP_CHANNEL_T *channels;

    /** The offset of the group's data blocks within the data and code
     *  image
     */
    DCP_OFFSET_T blockBase;

    /** Size of the group's data blocks */
    DCP_OFFSET_T blocksSize;

    /** The size of the group's shared data blocks */
    int blockMemberStart;
} DCP_GROUP_T;

/** Structure to describe a DCP device.
 *  A device corresponds with a physical piece of hardware which must be
 *  shared in a multi-context system.
 */
typedef struct
{
    /** The name of the device */
    char *name;

    /** Pointer to the group to which this device belongs */
    DCP_GROUP_T *group;

    /** The number of channels used by this device */
    int numChannels;

    /** The channel numbers of the channels used by this device */
    int *channelMap;

    /** The index of the channel that contains the job queue */
    int jobChannel;

    /** Information about device-context parameters declared by this group */
    DCP_PARAM_ID_ARRAY_T *deviceParamIds;

    /** The offset of the device's unshared data blocks within the data and
     *  code image
     */
    DCP_OFFSET_T blockBase;

    /** Size of the device's unshared data blocks */
    DCP_OFFSET_T blocksSize;

    /** Has this device been started? */
    bool started;
} DCP_DEVICE_T;

/** Structure to describe a DCP pipeline.
 *  A pipeline represents usage of DCP devices, where the usage is in a
 *  sequential order.
 */
typedef struct
{
    /** User specified handler and context assosiated with this pipeline.
     *  These fields may be used by a client of the UCC runtime to store a
     *  handler and its context.
     */
    void *handler;
    void *context;

    /** The ID of the pipeline */
    uint16_t id;

    /** The name of the pipeline */
    char *name;

    /** Total size of use-context data blocks */
    uint32_t useDataSize;

    /** Total size of job-context data blocks */
    uint32_t jobDataSize;

    /** Device-instance information */
    DCP_USE_INFO_T *useInfos;

    /** The number of devices used in the pipeline */
    int numUses;

    /** The index of the first device in the pipeline */
    int firstUse;

    /** The devices in the pipeline */
    DCP_DEVICE_T **devices;

    /** The number of use-context parameters */
    int numUseParamIds;

    /** Information about use-context parameters */
    DCP_PARAM_ID_ARRAY_T **useParamIds;
} DCP_PIPELINE_T;

/** Structure containing debug information */
typedef struct
{
    /** The version number of this structure */
    uint32_t version;

    /** The target memory region of the debug data */
    DCP_MEM_REGION_T memRegion;

    /** The number of entries in the code-section table of the code image */
    int imageNumCodeSections;

    /** The size in bytes of the code-section table of the code image */
    int imageCodeSectionTableSize;

    /** A pointer to the code-section table of the code image */
    void *imageCodeSectionTable;

    /** The number of entries in the parameter table of the code image */
    int imageNumParameters;

    /** The size in bytes of the parameter table of the code image */
    int imageParameterTableSize;

    /** A pointer to the parameter table of the code image */
    void *imageParameterTable;

    /** The size in bytes of the string-offset list of the code image */
    int imageStringListSize;

    /** A pointer to the string-offset list of the code image */
    void *imageStringList;

    /** The size in bytes of the string table of the code image */
    int imageStringTableSize;

    /** A pointer to the string table of the code image */
    void *imageStringTable;
} DCP_DEBUG_INFO_T;

/** Structure to describe an overall DCP image */
typedef struct
{
    /** The version number of this structure */
    uint32_t version;

    /** Flags */
    uint32_t flags;

    /** A pointer to the image data */
    void *imageData;

    /** A pointer to the code relocation table */
    DCP_OFFSET_ARRAY_T *codeRelocationOffsets;

    /** A pointer to the data relocation table */
    DCP_OFFSET_ARRAY_T *dataRelocationOffsets;

    /** A pointer to an array of data initialisers */
    DCP_DATA_INITIALISER_ARRAY_T *dataInitialisers;
    
    /* The base address for which this image has been generated */
    int32_t origin;
    
    /* Indicates whether this image has been relocated */
    bool relocated;

    /** A pointer to the EFS and QM RCD image set */
    RCD_IMAGE_SET_T *efsqm;

    /** The target memory region of the image data */
    DCP_MEM_REGION_T memRegion;

    /** The size of the data section of the DCP image in words */
    int dataSizeInWords;

    /** The total size of the DCP image in words */
    int sizeInWords;

    /** The total number of DCP groups */
    int numGroups;

    /** A summary of all DCP groups */
    DCP_GROUP_T **groups;

    /** The total number of DCP devices */
    int numDevices;

    /** A summary of all DCP devices */
    DCP_DEVICE_T **devices;

    /** The total number of DCP pipelines */
    int numPipelines;

    /** A summary of all DCP pipelines */
    DCP_PIPELINE_T **pipelines;

    /** The index of the IRQ flag */
    int irqFlagIndex;

    /** The ID of the IRQ flag */
    int irqFlagId;

    /** A pointer to debug information */
    DCP_DEBUG_INFO_T *debugInfo;
} DCP_IMAGE_T;

/*-------------------------------------------------------------------------*/

#endif /* _DCP_TYPES_H_ */

/*-------------------------------------------------------------------------*/
