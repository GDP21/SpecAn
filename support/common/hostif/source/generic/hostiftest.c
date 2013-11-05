/*!****************************************************************************
 @File          hostiftest.c

 @Title         Host interface - simple test version

 @Date          15 April 2011

 @Copyright     Copyright (C) Imagination Technologies Limited

 @Description   Test version of the host interface. Sets up the system with a
 fixed set of requests, then allow the used, via CodeScape to
 issue more.

 ******************************************************************************/

#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif
#ifdef HOSTIFTEST
#include <assert.h>
#include "uccrt.h"
#include "img_tv_msg.h"
#include "hostif.h"

/* Pointer to mailbox to send filled in messages */
static KRN_MAILBOX_T *rxMbox;
/* Mail box to keep queued message buffers be fore we fill them in.
 ** Use a mailbox so we can block on it waiting for buffers if we run out
 */
static KRN_MAILBOX_T messageList;
/* Semaphore to block on waiting for the IMG_TV_READY status message */
static KRN_SEMAPHORE_T startSem;

/* View these in a CodeScape Watch Window... */
/* Fill your request message in here.. */
static IMG_TV_MSG_T commandMsg;
/* ...then set this non-zero. */
static int sendMsg = 0;

/* messge sending task data */
#define STACK_SIZE 1024
static unsigned int stack[STACK_SIZE];
static KRN_TASK_T tcb;

/* A list of messages to send to the Host access message processor at start up. */
#define NUM_INIT_MSGS	4

static IMG_TV_MSG_T initMsgs[NUM_INIT_MSGS] = {
        { /* activate */
                28,
                27,
                0,
                0,
                2,
                8,
                {
                        0,
                        0}},
        { /* auto on reg 1 */
                24,
                27,
                1,
                0,
                10,
                4,
                {
                        1,
                        0}},
        { /* auto on reg 2 */
                24,
                27,
                1,
                0,
                10,
                4,
                {
                        2,
                        0}},
        { /* set reg 0 : detect */
                28,
                27,
                1,
                0,
                6,
                8,
                {
                        0,
                        2}}};

static void
testingTask(void)
{
    int i;
    KRN_TASKQ_T queue;
    DQ_init(&queue);

    /* Wait for semaphore that indicates we have seen the ready from the message processor */
    KRN_testSemaphore(&startSem, 1, KRN_INFWAIT);

    for (i = 0; i < NUM_INIT_MSGS; i++)
    {
        POOLABLE_IMG_TV_MSG_T *msg = KRN_getMbox(&messageList, KRN_INFWAIT);

        initMsgs[i].messageId = i + 0x100; // auto generate Id, so that they increment

        msg->tvAPIMessage = initMsgs[i];

        KRN_putMbox(rxMbox, msg);
    }

    __TBILogF("Ready for user input.\n\n");

    for (;;)
    {
        KRN_hibernate(&queue, 10);

        if (sendMsg)
        {
            POOLABLE_IMG_TV_MSG_T *msg = KRN_getMbox(&messageList, KRN_INFWAIT);

            sendMsg = 0;

            msg->tvAPIMessage = commandMsg;

            KRN_putMbox(rxMbox, msg);
        }
    }
}

/* External interface functions */
void HSTIF_init(UCCP_GRAM_ADDRESS_T baseH, UCCP_GRAM_ADDRESS_T baseU, int bufLen,
           KRN_PRIORITY_T pollPriority)
{
    (void)baseH;
    (void)baseU;
    (void)bufLen;
    (void)pollPriority;
    __TBILogF("Host Port IF driver initialised.\n\n");
    rxMbox = NULL;
    KRN_initMbox(&messageList);
    KRN_initSemaphore(&startSem,0);

    KRN_startTask(testingTask, &tcb, stack, STACK_SIZE, KRN_LOWEST_PRIORITY, NULL, "Host Port IF test driver");
}

void HSTIF_queuetoRx(HP_MSG_DESCRIPTOR_T *msg, KRN_MAILBOX_T *mbox)
{
    KRN_putMbox(&messageList, msg);
    rxMbox = mbox; // assume all for the same Mbox...
}

void HSTIF_send(uint8_t *message, int msglen)
{
    IMG_TV_MSG_T *msg = (IMG_TV_MSG_T *)message;

    unsigned i;

    __TBILogF("Status Message: %02X\n",msg->messageFunction);
    __TBILogF("\tTotal Length:\t%d\n",msglen);
    __TBILogF("\tLength:\t%d\n",msg->length);
    __TBILogF("\tsourceId:\t%d\n",msg->sourceId);
    __TBILogF("\ttargetId:\t%d\n",msg->targetId);
    __TBILogF("\tmessageId(%s):\t%08X\n",(msg->messageId > 0xffffU ? "Auto status update" : "Normal status update"),msg->messageId);
    __TBILogF("\tdatalength:\t%d\n",msg->datalength);
    for(i=0;i<(msg->datalength/sizeof(uint32_t));i++)
        __TBILogF("\tPayload[%d]:\t%08X\n",i,msg->payload[i]);

    __TBILogF("---------------------------------\n\n");

    KRN_setSemaphore(&startSem, 1);

    return;
}
#endif
