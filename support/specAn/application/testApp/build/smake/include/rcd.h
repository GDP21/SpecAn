/*!
*****************************************************************************

 @file      rcd.h
 @brief     Internal RCD functions

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#ifndef _RCD_H_
#define _RCD_H_

/*-------------------------------------------------------------------------*/

#include "uccrt.h"

/*-------------------------------------------------------------------------*/

/* Convert image address */
RCD_IMAGE_DATA32_T *_RCD_convertAddr(RCD_IMAGE_SET_T *imageSet);
RCD_IMAGE_DATA32_T *_RCD_convertAddrToDBL(RCD_IMAGE_SET_T *imageSet);

/*-------------------------------------------------------------------------*/

#endif /* _RCD_H_ */

/*-------------------------------------------------------------------------*/
