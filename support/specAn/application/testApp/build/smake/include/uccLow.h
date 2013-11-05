/*!
*****************************************************************************

 @file      uccLow.h
 @brief     UCC low level primitives

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#ifndef _UCC_LOW_H_
#define _UCC_LOW_H_

/*-------------------------------------------------------------------------*/

/** Log a message */
#if defined(__DEBUG__) && !defined(DISABLE_LOGGING)
#define UCC_LOGMSG(...) do { UCC_LOW_LOGMSG(__VA_ARGS__); UCC_OUTMSG("\n"); } while (0)
#else
#define UCC_LOGMSG(...)
#endif /* __DEBUG__ && !DISABLE_LOGGING */

/** Log a message of less importance */
#if defined(__DEBUG__) && defined(ENABLE_INFO_LOGGING)
#define INFO_LOGGING_ENABLED
#define UCC_LOGINFO(...) do { UCC_LOW_LOGMSG(__VA_ARGS__); UCC_OUTMSG("\n"); } while (0)
#else
#define UCC_LOGINFO(...)
#endif /* __DEBUG__ && ENABLE_INFO_LOGGING */

/*-------------------------------------------------------------------------*/

/** Log an error */
#define UCC_LOGERR(...) do { UCC_OUTMSG("ERROR: "); UCC_OUTMSG(__VA_ARGS__); UCC_OUTMSG("\n"); } while (0)

/*-------------------------------------------------------------------------*/

/** Value for an infinite poll */
#define UCC_INF_POLL (-1)

/*-------------------------------------------------------------------------*/

/** Enumeration of poll operations */
typedef enum
{
    /** Equals */
    UCC_POLL_EQ = 0,
    
    /** Less than */
    UCC_POLL_LT,
    
    /** Less than or equal */
    UCC_POLL_LE,
    
    /** Greater than */
    UCC_POLL_GT,
    
    /** Greater than or equals */
    UCC_POLL_GE,
    
    /** Not equals */
    UCC_POLL_NE
} UCC_POLL_OP_T;

/*-------------------------------------------------------------------------*/

#include "uccLowPrim.h"

/*-------------------------------------------------------------------------*/

/**
 * Read a 32-bit value from a memory mapped location.
 *
 * @param[in]  addr  Address
 *
 * @return 32-bit value read
 */
#define UCC_READ32      UCC_LOW_READ32

/**
 * Write a 32-bit value to a memory mapped location.
 *
 * @param[in]  addr  Address
 * @param[in]  val   32-bit value to write
 */
#define UCC_WRITE32     UCC_LOW_WRITE32

/**
 * Acknowledge the presence of a buffer.
 *
 * @param[in]  buffer  The buffer to acknowledge
 * @param[in]  count   The length of the buffer in 32-bit words
 */
#define UCC_IN32        UCC_LOW_IN32

/**
 * Acknowledge the presence of a buffer.
 *
 * @param[in]  buffer  The buffer to acknowledge
 * @param[in]  count   The length of the buffer in 32-bit words
 */
#define UCC_OUT32       UCC_LOW_OUT32

/**
 * Polls on a 32-bit value from a memory mapped location.
 *
 * @param[in]  addr       Address to poll
 * @param[in]  reqVal     Required value
 * @param[in]  enable     Enable mask
 * @param[in]  op         Polling operation
 * @param[in]  pollCount  Number of times to poll
 * @param[in]  delay      Delay between polls
 *
 * @return                true for suceeded, false for poll timed out
 */
#define UCC_POLL32      UCC_LOW_POLL32

/**
 * Read a 32-bit value from a location in the META address space.
 * This macro does not write to the PDUMP output.
 *
 * @param[in]  addr  Address
 *
 * @return 32-bit value read
 */
#define META_READ32     UCC_HW_READ32

/**
 * Write a 32-bit value to a location in the META address space.
 * This macro does not write to the PDUMP output.
 *
 * @param[in]  addr  Address
 * @param[in]  val   32-bit value to write
 */
#define META_WRITE32    UCC_HW_WRITE32

/*-------------------------------------------------------------------------*/

/* @cond DONT_DOXYGEN */
#define UCC_PERIP_SHIFT (8)
/* @endcond */

/**
 * Read from the peripheral bus.
 *
 * @param[in]  addr  Address
 *
 * @return 24-bit value read represented as a 32-bit right aligned value
 */
#define UCC_READ_PERIP(addr) (UCC_READ32(addr) >> UCC_PERIP_SHIFT)

/**
 * Write to the peripheral bus.
 *
 * @param[in]  addr  Address
 * @param[in]  val   24-bit value to write represented as a 32-bit right
 *                   aligned value
 */
#define UCC_WRITE_PERIP(addr, val) UCC_WRITE32((addr), (val) << UCC_PERIP_SHIFT)

/**
 * Polls on a location on the peripheral bus.
 *
 * @param[in]  addr       Address to poll
 * @param[in]  reqVal     Required value
 * @param[in]  enable     Enable mask
 * @param[in]  op         Polling operation
 * @param[in]  pollCount  Number of times to poll
 * @param[in]  delay      Delay between polls
 *
 * @return                true for suceeded, false for poll timed out
 */
#define UCC_POLL_PERIP(addr, reqVal, enable, op, pollCount, delay) UCC_POLL32((addr), (reqVal) << UCC_PERIP_SHIFT, (enable) << UCC_PERIP_SHIFT, (op), (pollCount), (delay))

/**
 * Macros to count (at compile time) the number of bits set in a 24-bit value
 * Typically used to derive the width of a field from its register mask.
 */
#define UCC_BIT_VALUE(X,B) ((X) & (1 << (B)) ? 1 : 0)
#define UCC_PERIP_BITS_SET(X) (\
        UCC_BIT_VALUE(X, 0) + \
        UCC_BIT_VALUE(X, 1) + \
        UCC_BIT_VALUE(X, 2) + \
        UCC_BIT_VALUE(X, 3) + \
        UCC_BIT_VALUE(X, 4) + \
        UCC_BIT_VALUE(X, 5) + \
        UCC_BIT_VALUE(X, 6) + \
        UCC_BIT_VALUE(X, 7) + \
        UCC_BIT_VALUE(X, 8) + \
        UCC_BIT_VALUE(X, 9) + \
        UCC_BIT_VALUE(X, 10) + \
        UCC_BIT_VALUE(X, 11) + \
        UCC_BIT_VALUE(X, 12) + \
        UCC_BIT_VALUE(X, 13) + \
        UCC_BIT_VALUE(X, 14) + \
        UCC_BIT_VALUE(X, 15) + \
        UCC_BIT_VALUE(X, 16) + \
        UCC_BIT_VALUE(X, 17) + \
        UCC_BIT_VALUE(X, 18) + \
        UCC_BIT_VALUE(X, 19) + \
        UCC_BIT_VALUE(X, 20) + \
        UCC_BIT_VALUE(X, 21) + \
        UCC_BIT_VALUE(X, 22) + \
        UCC_BIT_VALUE(X, 23) \
)
/*-------------------------------------------------------------------------*/

#endif /* _UCC_LOW_H_ */

/*-------------------------------------------------------------------------*/
