/*!****************************************************************************
 * @File          hostif.h
 *
 * @Title         Host interface
 *
 * @Date          15 April 2011
 *
 * @Copyright     Copyright (C) Imagination Technologies Limited
 *
 * @Description   Public interface of the host interface module.
 *
 ******************************************************************************/

#ifndef _HOST_IF_H_
#define _HOST_IF_H_
#include <assert.h>
#include <MeOS.h>
#include "uccrt.h"

#ifdef HOSTIFTEST
#include "hostiftest.h"
#else
/*
 * These macros provide a skinny abstraction on top of the host port driver.
 * The purpose of this is to allow the host port driver to be replaced by a
 * test harness.
 *
 * The abstraction itself has little value.
 */
#define	HSTIF_init(BASEH, BASEU, BUFSIZE, POLLPRIORITY) {\
                                                         HP_RESULT_T e = HP_init(BASEH, BASEU, BUFSIZE, POLLPRIORITY);\
                                                         assert(e == HP_RESULT_SUCCESS);\
                                                         (void)e;\
                                                     }
#define HSTIF_queuetoRx(MSGDESC, MBOX) {\
                                          HP_RESULT_T e = HP_queueReadFromHost(MSGDESC, MBOX, KRN_INFWAIT);\
                                          assert(e == HP_RESULT_SUCCESS);\
                                          (void)e;\
                                      }
#define HSTIF_send(MESSAGE, MSGLEN) {\
                                         HP_RESULT_T e = HP_writeToHost(MESSAGE, MSGLEN, KRN_INFWAIT);\
                                         assert(e == HP_RESULT_SUCCESS);\
                                         (void)e;\
                                    }
#endif /* HOSTIFTEST */
#endif /* _HOST_IF_H_ */
