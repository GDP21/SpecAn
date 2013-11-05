/*!
*****************************************************************************

 @file      rcdTypes.h
 @brief     RCD type definitions

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#ifndef _RCD_TYPES_H_
#define _RCD_TYPES_H_

/*-------------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

/*-------------------------------------------------------------------------*/

/** Version of the RCD types */
#define RCD_TYPES_VERSION (7)

/*-------------------------------------------------------------------------*/

/* Microsoft C: disable warning about variable length array */
#ifdef _MSC_VER
#pragma warning(disable:4200)
#endif

/*-------------------------------------------------------------------------*/

/* Register types */
typedef enum
{
    RCD_TYPE_SYSTEM = 0,
    RCD_TYPE_PERIPH
} RCD_TYPE_T;

/* Enumeration of target memory regions */
typedef enum
{
    RCD_MEM_REGION_SYS_MEM = 0,
    RCD_MEM_REGION_GRAM_DBL,
    RCD_MEM_REGION_GRAM_SXT,
    RCD_MEM_REGION_GRAM_CPX,
    RCD_MEM_REGION_GRAM_PKD
} RCD_MEM_REGION_T;

/** Enumeration of DCP parameter contexts */
typedef enum
{
    /** Device context */
    DCP_DEVICE_CTX = 0,
    
    /** Use context */
    DCP_USE_CTX,
    
    /** Job context */
    DCP_JOB_CTX
} DCP_PARAM_CTX_T;

/*
 * Integer types
 */

typedef uint32_t RCD_OFFSET_T;
typedef uint32_t RCD_ADDR_T;
typedef uint8_t  RCD_VALUE_T;
typedef uint8_t  RCD_IMAGE_DATA8_T;
typedef uint32_t RCD_IMAGE_DATA32_T;

/*
 * Structures
 */

/** Structure to describe a single contiguous block within a 'device'
 *  image.
 */
typedef struct rcd_block_device_t
{
    /** The address of the first register within this block */
    RCD_ADDR_T addr;
    
    /** The register type */
    RCD_TYPE_T regType;
    
    /** The number of data values */
    uint32_t dataLength;
    
    /** The start offset within the image data */
    RCD_OFFSET_T offset;
    
    /** The size of the block in words */
    uint32_t sizeInWords;
} RCD_BLOCK_DEVICE_T;

/** Structure to represent a 'device' image */
typedef struct rcd_image_device_t
{
    /** The number of contiguous blocks that comprise this image */
    int numBlocks;
    
    /** A summary of all the contiguous blocks that comprise this image */
    RCD_BLOCK_DEVICE_T blocks[];
} RCD_IMAGE_DEVICE_T;

/** Structure to represent a 'device serial' image */
typedef struct rcd_image_device_serial_t
{
    /** The address of the address register */
    RCD_ADDR_T addrRegAddr;
    
    /** The address of the data register */
    RCD_ADDR_T dataRegAddr;
    
    /** The value to write to the address register */
    RCD_ADDR_T addr;
    
    /** The type of the data register */
    RCD_TYPE_T regType;
    
    /** The number of data values */
    uint32_t dataLength;
    
    /** The start offset within the image data */
    RCD_OFFSET_T offset;
    
    /** The size of the image in words */
    uint32_t sizeInWords;
} RCD_IMAGE_DEVICE_SERIAL_T;

/** Structure to describe a single contiguous block within a 'dcp data'
 *  image.
 */
typedef struct rcd_block_dcp_data_t
{
    /** The offset of the first register within this block */
    RCD_OFFSET_T regOffset;
    
    /** The number of data values */
    uint32_t dataLength;
    
    /** The start offset within the image data */
    RCD_OFFSET_T offset;
    
    /** The size of the block in words */
    uint32_t sizeInWords;
} RCD_BLOCK_DCP_DATA_T;

/** Structure to represent a 'dcp data' image */
typedef struct rcd_image_dcp_data_t
{
    /** The DCP parameter context of this data block */
    DCP_PARAM_CTX_T paramCtx;
    
    /** The job number for job context data blocks */
    int jobNum;
    
    /** The data block offset */
    RCD_OFFSET_T offset;
    
    /** The number of contiguous blocks that comprise this image */
    int numBlocks;
    
    /** A summary of all the contiguous blocks that comprise this image */
    RCD_BLOCK_DCP_DATA_T blocks[];
} RCD_IMAGE_DCP_DATA_T;

/** Structure to represent a 'table' image */
typedef struct rcd_image_table_t
{
    /** Are the values signed? */
    bool isSigned;
    
    /** The number of bytes in a data value */
    int numBytes;
    
    /** The number of data values */
    uint32_t dataLength;
    
    /** The start offset within the image data */
    RCD_OFFSET_T offset;
    
    /** The size of the image in words */
    uint32_t sizeInWords;
} RCD_IMAGE_TABLE_T;

/** Structure to represent a set of register images */
typedef struct rcd_image_set_t
{
    /** The version number of the register formatter that produced the
     *  the register images.
     */
    uint32_t regTypesVersion; 
    
    /** The target memory region */
    RCD_MEM_REGION_T memRegion;

    /** Pointer to the image data */
    void *imageData;

    /** The number of 'device' images */
    int numImagesDevice;
    
    /** A summary of all 'device' images */
    RCD_IMAGE_DEVICE_T **imagesDevice;
    
    /** The number of 'device serial' images */
    int numImagesDeviceSerial;
    
    /** A summary of all 'device serial' images */
    RCD_IMAGE_DEVICE_SERIAL_T **imagesDeviceSerial;
    
    /** The number of 'dcp data' images */
    int numImagesDcpData;
    
    /** A summary of all 'dcp data' images */
    RCD_IMAGE_DCP_DATA_T **imagesDcpData;
    
    /** The number of 'table' images */
    int numImagesTable;
    
    /** A summary of all 'table' images */
    RCD_IMAGE_TABLE_T **imagesTable;
} RCD_IMAGE_SET_T;

/*-------------------------------------------------------------------------*/

#endif /* _RCD_TYPES_H_ */

/*-------------------------------------------------------------------------*/
