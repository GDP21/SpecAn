/*!
*****************************************************************************

 @file      gramBuffers.c
 @brief     SCP legacy mode driver test program

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include "gramBuffers.h"

/*-------------------------------------------------------------------------*/

#ifdef __META_FAMILY__
#define BULK_BUFFERS_SECTION __attribute__((__section__(".bulkbuffers")))
#else
#define BULK_BUFFERS_SECTION
#endif /* __MTX__ */

/*-------------------------------------------------------------------------*/

BULK_BUFFERS_SECTION uint32_t scpOutputBufferSpace[SCP_OUTPUT_BUFFER_LEN/3];
BULK_BUFFERS_SECTION uint32_t traceBufferSpace[TRACE_BUFFER_LEN/3];
