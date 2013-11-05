/*!
*****************************************************************************

 @file      gramBuffers.h
 @brief     UCC Runtime Test Program

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#ifndef _GRAM_BUFFERS_H_
#define _GRAM_BUFFERS_H_

#include "uccrt.h"

/*-------------------------------------------------------------------------*/

#define SCP_OUTPUT_BUFFER_LEN       (3*3*8192)
#define TRACE_BUFFER_LEN            (3*1024)

/*-------------------------------------------------------------------------*/

extern uint32_t scpOutputBufferSpace[SCP_OUTPUT_BUFFER_LEN/3];
extern uint32_t traceBufferSpace[TRACE_BUFFER_LEN/3];

/*-------------------------------------------------------------------------*/

#endif /* _GRAM_BUFFERS_H_ */

/*-------------------------------------------------------------------------*/
