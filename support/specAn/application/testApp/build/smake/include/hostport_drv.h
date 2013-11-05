/*!
 ******************************************************************************
 @file   hostport_drv.h

 @brief  Host Port Interface Driver, receiver side.

 <b>Copyright Imagination Technologies Limited.</b>\n
 All rights reserved.  No part of this software, either
 material or conceptual may be copied or distributed,
 transmitted, transcribed, stored in a retrieval system
 or translated into any human or computer language in any
 form by any means, electronic, mechanical, manual or
 other-wise, or disclosed to the third parties without the
 express written permission of Imagination Technologies
 Limited, Home Park Estate, Kings Langley, Hertfordshire,
 WD4 8LZ, U.K.

 \n\n\n

 ******************************************************************************/

#ifndef __HOSTPORT_DRV_H__
#define __HOSTPORT_DRV_H__

#include <MeOS.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Low level message definitions - these values form part of the published host port messaging
 * interface, so should not be changed.
 */
typedef enum HP_MESSAGE_TYPE_T_tag
{
    HP_START_MESSAGE = 1,
    HP_DATA_MESSAGE,
    HP_READY_MESSAGE,
    HP_BUFBASEH_MESSAGE,
    HP_BUFBASEU_MESSAGE,
    HP_BUFREADY_MESSAGE
} HP_MESSAGE_TYPE_T;

#define HP_MESSAGE_TYPE_SHIFT                             (24)
#define HP_MESSAGE_TYPE_MASK                              (0x7F000000)

#define HP_PAYLOAD_BITS_SHIFT                             (0)
#define HP_PAYLOAD_BITS_MASK                              (0x00FFFFFF)

/* bits in flags to signal receipt of these low level messages */
#define	BUFBASEH_FLAG	1U
#define	BUFBASEU_FLAG	2U

/*
 * Data shared between Host port background code and QIO driver.
 * It's convenient to group these into a struct.
 */
typedef struct
{
    KRN_LOCK_T sendMutexLock;
    uint8_t *pBufBaseH;
    volatile uint32_t bufBaseFlags;
    bool useBufferedProtocol;
    UCCP_GRAM_ADDRESS_T ui32BufBaseU;
    UCCP_GRAM_ADDRESS_T ui32BufBaseH;
} HPCLIENT_SHARED_DATA_T;

/* Public interface to QIO driver */
extern const QIO_DRIVER_T HP_hostToMetaDriver;
extern QIO_DEVICE_T HP_hostToMetaDevice;
/* Data shared between QIO driver and API wrapper */
extern HPCLIENT_SHARED_DATA_T HP_sharedDriverData;

#endif /* __HOSTPORT_DRV_H__   */

