/*!****************************************************************************
* @File          hostiftest.h
*
* @Title         Host interface test harness
*
* @Date          15 April 2011
*
* @Copyright     Copyright (C) Imagination Technologies Limited
*
* @Description   Public interface of the host interface module.
*
******************************************************************************/


#ifndef _HOST_IFTEST_H_
#define _HOST_IFTEST_H_
#include <MeOS.h>
#include "uccrt.h"
#include "img_tv_msg.h"

void HSTIF_queuetoRx(HP_MSG_DESCRIPTOR_T *msg, KRN_MAILBOX_T *mbox);
void HSTIF_init(UCCP_GRAM_ADDRESS_T baseH, UCCP_GRAM_ADDRESS_T baseU, int bufLen, KRN_PRIORITY_T pollPriority);
void HSTIF_send(uint8_t *message, int msglen);
#endif /* _HOST_IFTEST_H_ */
