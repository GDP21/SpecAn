/*!
*****************************************************************************

 @file      uccCompat.h
 @brief     Backward compatability for deprecated APIs
 
 ** These will be removed once everything has been migrated **

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

/* @cond DONT_DOXYGEN */

#ifndef _UCC_COMPAT_H_
#define _UCC_COMPAT_H_

/*-------------------------------------------------------------------------*/

typedef RCD_IMAGE_SET_T           UCC_REGISTER_IMAGES_T;
typedef RCD_IMAGE_DCP_DATA_T      UCC_REGISTER_IMAGE_SUMMARY_DCP_DATA_BLOCK_T;
typedef RCD_BLOCK_DCP_DATA_T      UCC_REGISTER_IMAGE_DCP_DATA_BLOCK_T;
typedef RCD_IMAGE_DEVICE_SERIAL_T UCC_REGISTER_IMAGE_DEVICE_SERIAL_T;
typedef RCD_IMAGE_DEVICE_T        UCC_REGISTER_IMAGE_ARRAY_DEVICE_T;
typedef RCD_BLOCK_DEVICE_T        UCC_REGISTER_IMAGE_DEVICE_T;

typedef RCD_IMAGE_SET_T           RCD_IMAGES_T;

#define REG_load RCD_load

#define DCP_getBlock(pipeline, useId, dataBlockOffset, jobNum, len) \
        DCP_getJobDataBlock(pipeline, useId, dataBlockOffset, jobNum)

/*-------------------------------------------------------------------------*/

#endif /* _UCC_COMPAT_H_ */

/*-------------------------------------------------------------------------*/

/* @endcond */

/*-------------------------------------------------------------------------*/
