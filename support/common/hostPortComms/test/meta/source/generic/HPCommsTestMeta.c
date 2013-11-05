/*!
******************************************************************************
 @file   HPCommsTestMeta.c

 @brief  Host Port Interface test harness, Server side.

 @Author Imagination Technologies

 @date   28/09/2010

         <b>Copyright 2010 by Imagination Technologies Limited.</b>\n
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

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

/* MeOS Library */
#include <MeOS.h>

#include "system.h"
#include "h2d_api.h"
#include "testData.h"

#define VERBOSE_LOGGING
#ifdef VERBOSE_LOGGING
#define VerboseLog __TBILogF
#else
void dummyLog() {}
#define VerboseLog dummyLog
#endif

#define MEOS_MAX_PRIORITY (0x6)
#define MEOS_STACK_INIT   (0x0)

KRN_SCHEDULE_T sched;
KRN_TASKQ_T    schedQueues[MEOS_MAX_PRIORITY + 1];
KRN_TASK_T     *mainTask;

/* Timer task */
#define TIMER_TICK                (1000)      /* 1ms (in microseconds) */
#define TIMER_STACK_SIZE_IN_WORDS (256)
static KRN_TASK_T *timerTask;
static unsigned int timerTaskStack[TIMER_STACK_SIZE_IN_WORDS];

#define NUM_DIRECT_DEVICES (16)

/*
#define QIO_NO_VECTORS          (40)
QIO_IVDESC_T QIO_META12xIVDesc = {
	{
		HWSTATEXT,
		HWSTATEXT2,
		HWSTATEXT4,
		HWSTATEXT6
	},
	{
		HWVEC0EXT,
		HWVEC20EXT,
		HWVEC40EXT,
		HWVEC60EXT
	},
	HWVECnEXT_STRIDE
};
*/
/* NB QIO_NO_VECTORS, QIO_MTXIVDesc and QIO_MTXLevelRegisters are from Comet SoC system header. */
static QIO_DEVENTRY_T devtab[QIO_NO_VECTORS];
//static QIO_DEVENTRY_T directDevTable[NUM_DIRECT_DEVICES];
static QIO_SYS_T      qioSys;

/*
** Data buffers for tests. They are 8 bytes larger than the maximum size
** that the comms module can handle.
*/
#define COMMAND_BUFFER_SIZE (DACOMMS_CMD_DATA_SIZE + DACOMMS_CMD_PAYLOAD_START + 8)
#define RESP_ASYNC_BUFFER_SIZE (DACOMMS_RSP_ASYNC_DATA_SIZE + DACOMMS_RSP_ASYNC_PAYLOAD_START + 8)
static unsigned char commandBuff[COMMAND_BUFFER_SIZE];
static unsigned int responseNumBytes;
static unsigned char responseBuff[RESP_ASYNC_BUFFER_SIZE];
static unsigned int asyncNumBytes0;
static unsigned char asyncBuff0[RESP_ASYNC_BUFFER_SIZE];
static unsigned int asyncNumBytes1;
static unsigned char asyncBuff1[RESP_ASYNC_BUFFER_SIZE];

static H2D_eCBResult test02Callback(H2D_sAsyncCallbackDesc *psAsyncDesc);
KRN_SEMAPHORE_T		g_sTest02CompleteSemaphore;
static unsigned int ui32Test02Stage;
static void queueTest02BufferDesc(H2D_sAsyncBufferDesc *psBufferDesc, unsigned int testStage);

static H2D_eCBResult test04Callback(H2D_sAsyncCallbackDesc *psAsyncDesc);
KRN_SEMAPHORE_T		g_sTest04CompleteSemaphore;
static unsigned int ui32Test04Stage;
static void queueTest04BufferDesc(H2D_sAsyncBufferDesc *psBufferDesc, unsigned int testStage);

H2D_sAsyncBufferDesc g_sBufferDesc;
/*!
******************************************************************************

 @Function              main

 @Description

 @Input     None.

 @Return    None.

******************************************************************************/
int main(int argc, char *argv[])
{
	volatile int waitForAync = 1;
    (void)argc;     /* Remove warnings about unused parameters */
    (void)argv;     /* Remove warnings about unused parameters */

	/*
	** MEOS Setup
	*/

    /* Start up MeOS */
    if (!KRN_reset(&sched,
                   schedQueues,
                   MEOS_MAX_PRIORITY,
                   MEOS_STACK_INIT,
                   NULL,
                   0))
    {
        assert(0);
    }

    mainTask = KRN_startOS("Main Task");

    QIO_reset(&qioSys,
        devtab,
        QIO_NO_VECTORS,
        &QIO_META12xIVDesc,
        TBID_SIGNUM_TR2(__TBIThreadId() >> TBID_THREAD_S),
        IMG_NULL,
        0);

	/* Start the timer task */
    timerTask = KRN_startTimerTask("Timer Task",
                                   timerTaskStack,
                                   TIMER_STACK_SIZE_IN_WORDS,
                                   TIMER_TICK);

	H2D_Initialise(MEOS_MAX_PRIORITY - 1);

	/******************************************
	**
	** Test 01: synchronous command/response
	**
	*******************************************/

	/*
	** a) Basic command respose. 32 byte command & response.
	*/

	fillTestBuffer(commandBuff, COMMAND_BUFFER_SIZE, 0);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (H2D_SUCCESS != H2D_SendCmdGetRsp(commandBuff, 32, responseBuff,
									32, 0, &responseNumBytes))
	{
		VerboseLog("Test 01-a: FAIL. H2D_SendCmdGetRsp error.\n");
		assert(0);
	}

	if (responseNumBytes != 32)
	{
		VerboseLog("Test 01-a: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(responseBuff, 32, 10))
	{
		VerboseLog("Test 01-a: FAIL. Incorrect data.\n");
		assert(0);
	}
	else
		VerboseLog("Test 01-a: PASS.\n");

	/*
	** b) Max sized buffers & data.
	*/

	fillTestBuffer(commandBuff, COMMAND_BUFFER_SIZE, 1);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (H2D_SUCCESS != H2D_SendCmdGetRsp(commandBuff, DACOMMS_CMD_DATA_SIZE, responseBuff,
	                                DACOMMS_RSP_ASYNC_DATA_SIZE,
									0, &responseNumBytes))
	{
		VerboseLog("Test 01-b: FAIL. H2D_SendCmdGetRsp error.\n");
		assert(0);
	}

	if (responseNumBytes != DACOMMS_RSP_ASYNC_DATA_SIZE)
	{
		VerboseLog("Test 01-b: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(responseBuff, DACOMMS_RSP_ASYNC_DATA_SIZE, 11))
	{
		VerboseLog("Test 01-b: FAIL. Incorrect data.\n");
		assert(0);
	}
	else
		VerboseLog("Test 01-b: PASS.\n");

	/*
	** c) Max sized data. Oversized buffers.
	*/

	fillTestBuffer(commandBuff, COMMAND_BUFFER_SIZE, 2);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (H2D_SUCCESS != H2D_SendCmdGetRsp(commandBuff, DACOMMS_CMD_DATA_SIZE, responseBuff,
	                                RESP_ASYNC_BUFFER_SIZE, 0, &responseNumBytes))
	{
		VerboseLog("Test 01-c: FAIL. H2D_SendCmdGetRsp error.\n");
		assert(0);
	}

	if (responseNumBytes != DACOMMS_RSP_ASYNC_DATA_SIZE)
	{
		VerboseLog("Test 01-c: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(responseBuff, DACOMMS_RSP_ASYNC_DATA_SIZE, 12))
	{
		VerboseLog("Test 01-c: FAIL. Incorrect data.\n");
		assert(0);
	}
	else
		VerboseLog("Test 01-c: PASS.\n");

	/*
	** d) Too much data in command buffer (Host side).
	*/

	fillTestBuffer(commandBuff, COMMAND_BUFFER_SIZE, 3);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (H2D_ILLEGAL_PARAM != H2D_SendCmdGetRsp(commandBuff, COMMAND_BUFFER_SIZE, responseBuff,
	                                RESP_ASYNC_BUFFER_SIZE, 0, &responseNumBytes))
	{
		VerboseLog("Test 01-d: FAIL. H2D_SendCmdGetRsp error.\n");
		assert(0);
	}
	else
		VerboseLog("Test 01-d: PASS.\n");

	/*
	** e) Too much data in command buffer (Device side).
	*/

	fillTestBuffer(commandBuff, COMMAND_BUFFER_SIZE, 4);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (H2D_SUCCESS != H2D_SendCmdGetRsp(commandBuff, DACOMMS_CMD_DATA_SIZE, responseBuff,
	                                RESP_ASYNC_BUFFER_SIZE, 0, &responseNumBytes))
	{
		VerboseLog("Test 01-e: FAIL. H2D_SendCmdGetRsp error.\n");
		assert(0);
	}

	if (responseNumBytes != DACOMMS_RSP_ASYNC_DATA_SIZE)
	{
		VerboseLog("Test 01-e: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(responseBuff, DACOMMS_RSP_ASYNC_DATA_SIZE, 14))
	{
		VerboseLog("Test 01-e: FAIL. Incorrect data.\n");
		assert(0);
	}
	else
		VerboseLog("Test 01-e: PASS.\n");

	/*
	** f) Too much data in response buffer (Host side).
	*/

	fillTestBuffer(commandBuff, COMMAND_BUFFER_SIZE, 5);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (H2D_RSP_BUFF_OVERFLOW != H2D_SendCmdGetRsp(commandBuff, DACOMMS_CMD_DATA_SIZE, responseBuff,
	                                DACOMMS_RSP_ASYNC_DATA_SIZE - 1, 0, &responseNumBytes))
	{
		VerboseLog("Test 01-f: FAIL. H2D_SendCmdGetRsp error.\n");
		assert(0);
	}
		VerboseLog("Test 01-f: PASS.\n");

	/*
	** g) Too much data in response buffer (Device side).
	*/

	fillTestBuffer(commandBuff, COMMAND_BUFFER_SIZE, 6);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (H2D_SUCCESS != H2D_SendCmdGetRsp(commandBuff, DACOMMS_CMD_DATA_SIZE, responseBuff,
	                                RESP_ASYNC_BUFFER_SIZE, 0, &responseNumBytes))
	{
		VerboseLog("Test 01-g: FAIL. H2D_SendCmdGetRsp error.\n");
		assert(0);
	}

	if (responseNumBytes != DACOMMS_RSP_ASYNC_DATA_SIZE)
	{
		VerboseLog("Test 01-g: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(responseBuff, DACOMMS_RSP_ASYNC_DATA_SIZE, 16))
	{
		VerboseLog("Test 01-g: FAIL. Incorrect data.\n");
		assert(0);
	}
	else
		VerboseLog("Test 01-g: PASS.\n");

	/******************************************
	**
	** Test 02: asynchronous message sending, single group.
	**
	*******************************************/

	/*
	** Add the callback.
	*/
	if (H2D_SUCCESS != H2D_AddCallback(test02Callback))
	{
		VerboseLog("Test 02: FAIL. H2D_AddCallback error.\n");
		assert(0);
	}
	KRN_initSemaphore( &g_sTest02CompleteSemaphore, 0 );

	/*
	** Queue up first async descriptor.
	*/
	ui32Test02Stage = 1;
	queueTest02BufferDesc(&g_sBufferDesc, ui32Test02Stage);

	/*
	** Signal the Device to start sending async messages.
	*/
	fillTestBuffer(commandBuff, COMMAND_BUFFER_SIZE, 0);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (H2D_SUCCESS != H2D_SendCmdGetRsp(commandBuff, 32, responseBuff,
									32, 0, &responseNumBytes))
	{
		VerboseLog("Test 02: FAIL. H2D_SendCmdGetRsp error.\n");
		assert(0);
	}

	if (responseNumBytes != 32)
	{
		VerboseLog("Test 02: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(responseBuff, 32, 10))
	{
		VerboseLog("Test 02: FAIL. Incorrect data.\n");
		assert(0);
	}

	/*
	** Wait until the test is complete
	*/
	KRN_testSemaphore( &g_sTest02CompleteSemaphore, 1, KRN_INFWAIT );

	/*
	** Remove the callback.
	*/
	if (H2D_SUCCESS != H2D_RemoveCallback())
	{
		VerboseLog("Test 02: FAIL. H2D_RemoveCallback error.\n");
		assert(0);
	}

	/******************************************
	**
	** Test 03: asynchronous command/response.
	**
	*******************************************/

	/*
	** a) Basic command respose. 32 byte command & response.
	*/

	fillTestBuffer(commandBuff, COMMAND_BUFFER_SIZE, 0);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (H2D_SUCCESS != H2D_SendCmdGetRsp(commandBuff, 32, responseBuff,
									32, 0, &responseNumBytes))
	{
		VerboseLog("Test 03-a: FAIL. H2D_SendCmdGetRsp error.\n");
		assert(0);
	}

	if (responseNumBytes != 32)
	{
		VerboseLog("Test 03-a: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(responseBuff, 32, 10))
	{
		VerboseLog("Test 03-a: FAIL. Incorrect data.\n");
		assert(0);
	}
	else
		VerboseLog("Test 03-a: PASS.\n");

	/******************************************
	**
	** Test 04: asynchronous message sending, multiple group.
	**
	*******************************************/

	/*
	** Add the callback.
	*/
	if (H2D_SUCCESS != H2D_AddCallback(test04Callback))
	{
		VerboseLog("Test 04: FAIL. H2D_AddCallback error.\n");
		assert(0);
	}
	KRN_initSemaphore( &g_sTest04CompleteSemaphore, 0 );

	/*
	** Queue up first async descriptor.
	*/
	ui32Test04Stage = 1;
	queueTest04BufferDesc(&g_sBufferDesc, ui32Test04Stage);

	/*
	** Signal the Device to start sending async messages.
	*/
	fillTestBuffer(commandBuff, COMMAND_BUFFER_SIZE, 0);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (H2D_SUCCESS != H2D_SendCmdGetRsp(commandBuff, 32, responseBuff,
									32, 0, &responseNumBytes))
	{
		VerboseLog("Test 04: FAIL. H2D_SendCmdGetRsp error.\n");
		assert(0);
	}

	if (responseNumBytes != 32)
	{
		VerboseLog("Test 04: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(responseBuff, 32, 10))
	{
		VerboseLog("Test 04: FAIL. Incorrect data.\n");
		assert(0);
	}

	/*
	** Wait until the test is complete
	*/
	KRN_testSemaphore( &g_sTest04CompleteSemaphore, 1, KRN_INFWAIT );

	/*
	** Remove the callback.
	*/
	if (H2D_SUCCESS != H2D_RemoveCallback())
	{
		VerboseLog("Test 04: FAIL. H2D_RemoveCallback error.\n");
		assert(0);
	}

/*
	H2D_Deinitialise();
	H2D_Initialise(MEOS_MAX_PRIORITY - 1);

	H2D_Disable();
	H2D_Enable();
*/
/*
use both buffers
queue to many buffers
no buffers queued
data
disable reenable
Test host disable
*/

	/************************************************************
	** Finished testing
	*************************************************************/

	__TBILogF("Finito!\n");

	while (waitForAync){};

    return 0; /* End of Main Task */
}

static void queueTest02BufferDesc(H2D_sAsyncBufferDesc *psBufferDesc, unsigned int testStage)
{
	switch (testStage)
	{
		case 1:
		{
			/*
			** a) Basic async message.
			*/
			psBufferDesc->eBuffType = H2D_BT_MSG;
			psBufferDesc->ui32DataBufferNum = 0;
			psBufferDesc->apui8Buffer[0] = asyncBuff0;
			psBufferDesc->apui8Buffer[1] = IMG_NULL;
			psBufferDesc->aui32BufferMaxBytes[0] = 32;
			psBufferDesc->aui32BufferMaxBytes[1] = 0;
			break;
		}
		case 2:
		{
			/*
			** b) Max sized buffers & data.
			*/
			psBufferDesc->eBuffType = H2D_BT_MSG;
			psBufferDesc->ui32DataBufferNum = 0;
			psBufferDesc->apui8Buffer[0] = asyncBuff0;
			psBufferDesc->apui8Buffer[1] = IMG_NULL;
			psBufferDesc->aui32BufferMaxBytes[0] = DACOMMS_RSP_ASYNC_DATA_SIZE;
			psBufferDesc->aui32BufferMaxBytes[1] = 0;
			break;
		}
		case 3:
		{
			/*
			** c) Max sized data. Oversized buffers.
			*/
			psBufferDesc->eBuffType = H2D_BT_MSG;
			psBufferDesc->ui32DataBufferNum = 0;
			psBufferDesc->apui8Buffer[0] = asyncBuff0;
			psBufferDesc->apui8Buffer[1] = IMG_NULL;
			psBufferDesc->aui32BufferMaxBytes[0] = RESP_ASYNC_BUFFER_SIZE;
			psBufferDesc->aui32BufferMaxBytes[1] = 0;
			break;
		}
		case 4:
		{
			/*
			** d) Too much data in async. (Host side)
			*/
			psBufferDesc->eBuffType = H2D_BT_MSG;
			psBufferDesc->ui32DataBufferNum = 0;
			psBufferDesc->apui8Buffer[0] = asyncBuff0;
			psBufferDesc->apui8Buffer[1] = IMG_NULL;
			psBufferDesc->aui32BufferMaxBytes[0] = DACOMMS_RSP_ASYNC_DATA_SIZE - 1;
			psBufferDesc->aui32BufferMaxBytes[1] = 0;
			break;
		}
		case 5:
		{
			/*
			** e) Too much data in async. (Device side). Do not queue.
			*/
			psBufferDesc->eBuffType = H2D_BT_MSG;
			psBufferDesc->ui32DataBufferNum = 0;
			psBufferDesc->apui8Buffer[0] = asyncBuff0;
			psBufferDesc->apui8Buffer[1] = IMG_NULL;
			psBufferDesc->aui32BufferMaxBytes[0] = DACOMMS_RSP_ASYNC_DATA_SIZE;
			psBufferDesc->aui32BufferMaxBytes[1] = 0;
			break;
		}
		case 6:
		{
			/*
			** f) Alternate data buffer number sent from device. Buffer number should be ignored.
			*/
			psBufferDesc->eBuffType = H2D_BT_MSG;
			psBufferDesc->ui32DataBufferNum = 0;
			psBufferDesc->apui8Buffer[0] = asyncBuff0;
			psBufferDesc->apui8Buffer[1] = IMG_NULL;
			psBufferDesc->aui32BufferMaxBytes[0] = 32;
			psBufferDesc->aui32BufferMaxBytes[1] = 0;
			break;
		}
		case 7:
		{
			/*
			** g) Incorrect data buffer number queued on host. Buffer number should be ignored.
			*/
			psBufferDesc->eBuffType = H2D_BT_MSG;
			psBufferDesc->ui32DataBufferNum = 1;
			psBufferDesc->apui8Buffer[0] = asyncBuff0;
			psBufferDesc->apui8Buffer[1] = IMG_NULL;
			psBufferDesc->aui32BufferMaxBytes[0] = 32;
			psBufferDesc->aui32BufferMaxBytes[1] = 0;
			break;
		}

		default:
		{
			assert(0);
			break;
		}
	}
		IMG_MEMSET(asyncBuff0, 0, RESP_ASYNC_BUFFER_SIZE);
		H2D_QueueBuff(psBufferDesc);
}

static H2D_eCBResult test02Callback(H2D_sAsyncCallbackDesc *psAsyncDesc)
{
	switch (ui32Test02Stage)
	{
		case 1:
		{
			/*
			** a) Basic async message.
			*/
			if ( ((psAsyncDesc->eEvent) != H2D_AE_MSG) ||
			     ((psAsyncDesc->ui32DataBufferNum) != 0) ||
			     ((psAsyncDesc->apui8Buff[0]) != asyncBuff0) ||
			     ((psAsyncDesc->apui8Buff[1]) != IMG_NULL) ||
			     ((psAsyncDesc->aui32BuffNumBytes[0]) != 32) ||
			     ((psAsyncDesc->aui32BuffNumBytes[1]) != 0)
			)
			{
				VerboseLog("Test 02-a: FAIL. Incorrect callback descriptor.\n");
				assert(0);
			}
			else if (DATA_FAIL == checkTestBuffer(psAsyncDesc->apui8Buff[0], 32, 0))
			{
				VerboseLog("Test 02-a: FAIL. Incorrect data.\n");
				assert(0);
			}
			else
				VerboseLog("Test 02-a: PASS.\n");
			break;
		}
		case 2:
		{
			/*
			** b) Max sized buffers & data.
			*/
			if ( ((psAsyncDesc->eEvent) != H2D_AE_MSG) ||
			     ((psAsyncDesc->ui32DataBufferNum) != 0) ||
			     ((psAsyncDesc->apui8Buff[0]) != asyncBuff0) ||
			     ((psAsyncDesc->apui8Buff[1]) != IMG_NULL) ||
			     ((psAsyncDesc->aui32BuffNumBytes[0]) != DACOMMS_RSP_ASYNC_DATA_SIZE) ||
			     ((psAsyncDesc->aui32BuffNumBytes[1]) != 0)
			)
			{
				VerboseLog("Test 02-b: FAIL. Incorrect callback descriptor.\n");
				assert(0);
			}
			else if (DATA_FAIL == checkTestBuffer(psAsyncDesc->apui8Buff[0], DACOMMS_RSP_ASYNC_DATA_SIZE, 1))
			{
				VerboseLog("Test 02-b: FAIL. Incorrect data.\n");
				assert(0);
			}
			else
				VerboseLog("Test 02-b: PASS.\n");
			break;
		}
		case 3:
		{
			/*
			** c) Max sized data. Oversized buffers.
			*/
			if ( ((psAsyncDesc->eEvent) != H2D_AE_MSG) ||
			     ((psAsyncDesc->ui32DataBufferNum) != 0) ||
			     ((psAsyncDesc->apui8Buff[0]) != asyncBuff0) ||
			     ((psAsyncDesc->apui8Buff[1]) != IMG_NULL) ||
			     ((psAsyncDesc->aui32BuffNumBytes[0]) != DACOMMS_RSP_ASYNC_DATA_SIZE) ||
			     ((psAsyncDesc->aui32BuffNumBytes[1]) != 0)
			)
			{
				VerboseLog("Test 02-c: FAIL. Incorrect callback descriptor.\n");
				assert(0);
			}
			else if (DATA_FAIL == checkTestBuffer(psAsyncDesc->apui8Buff[0], DACOMMS_RSP_ASYNC_DATA_SIZE, 2))
			{
				VerboseLog("Test 02-c: FAIL. Incorrect data.\n");
				assert(0);
			}
			else
				VerboseLog("Test 02-c: PASS.\n");
			break;
		}
		case 4:
		{
			/*
			** d) Too much data in async. (Host side)
			*/
			if ((psAsyncDesc->eEvent) != H2D_AE_BUFFER_UNAVAILABLE)
			{
				VerboseLog("Test 02-d: FAIL. Incorrect callback descriptor.\n");
				assert(0);
			}
			break;
		}
		case 5:
		{
			/*
			** d) Too much data in async. (Host side). Need to clear buffer of queue.
			*/
			if ( ((psAsyncDesc->eEvent) != H2D_AE_MSG) ||
			     ((psAsyncDesc->ui32DataBufferNum) != 0) ||
			     ((psAsyncDesc->apui8Buff[0]) != asyncBuff0) ||
			     ((psAsyncDesc->apui8Buff[1]) != IMG_NULL) ||
			     ((psAsyncDesc->aui32BuffNumBytes[0]) != DACOMMS_RSP_ASYNC_DATA_SIZE - 1) ||
			     ((psAsyncDesc->aui32BuffNumBytes[1]) != 0)
			)
			{
				VerboseLog("Test 02-d: FAIL. Incorrect callback descriptor.\n");
				assert(0);
			}
			else if (DATA_FAIL == checkTestBuffer(psAsyncDesc->apui8Buff[0], DACOMMS_RSP_ASYNC_DATA_SIZE - 1, 13))
			{
				VerboseLog("Test 02-d: FAIL. Incorrect data.\n");
				assert(0);
			}
			else
				VerboseLog("Test 02-d: PASS.\n");
			break;
		}
			/*
			** e) Too much data in async. (Device side)
			*/
			// ASYNC WILL NOT BE CALLED
		case 6:
		{
			/*
			** f) Alternate data buffer number sent from device. Buffer number should be ignored.
			*/
			if ( ((psAsyncDesc->eEvent) != H2D_AE_MSG) ||
			     ((psAsyncDesc->ui32DataBufferNum) != 0) ||
			     ((psAsyncDesc->apui8Buff[0]) != asyncBuff0) ||
			     ((psAsyncDesc->apui8Buff[1]) != IMG_NULL) ||
			     ((psAsyncDesc->aui32BuffNumBytes[0]) != 32) ||
			     ((psAsyncDesc->aui32BuffNumBytes[1]) != 0)
			)
			{
				VerboseLog("Test 02-f: FAIL. Incorrect callback descriptor.\n");
				assert(0);
			}
			else if (DATA_FAIL == checkTestBuffer(psAsyncDesc->apui8Buff[0], 32, 5))
			{
				VerboseLog("Test 02-f: FAIL. Incorrect data.\n");
				assert(0);
			}
			else
				VerboseLog("Test 02-f: PASS.\n");
			break;
		}
		case 7:
		{
			/*
			** g) Incorrect data buffer number queued on host. Buffer number should be ignored.
			*/
			if ( ((psAsyncDesc->eEvent) != H2D_AE_MSG) ||
			     ((psAsyncDesc->ui32DataBufferNum) != 0) ||
			     ((psAsyncDesc->apui8Buff[0]) != asyncBuff0) ||
			     ((psAsyncDesc->apui8Buff[1]) != IMG_NULL) ||
			     ((psAsyncDesc->aui32BuffNumBytes[0]) != 32) ||
			     ((psAsyncDesc->aui32BuffNumBytes[1]) != 0)
			)
			{
				VerboseLog("Test 02-g: FAIL. Incorrect callback descriptor.\n");
				assert(0);
			}
			else if (DATA_FAIL == checkTestBuffer(psAsyncDesc->apui8Buff[0], 32, 6))
			{
				VerboseLog("Test 02-g: FAIL. Incorrect data.\n");
				assert(0);
			}
			else
				VerboseLog("Test 02-g: PASS.\n");
			break;
		}
		default:
		{
			assert(0);
			break;
		}
	}

	if (ui32Test02Stage == 7)
		//if we are done, then signal main task
		KRN_setSemaphore( &g_sTest02CompleteSemaphore, 1 );
	else
	{
		//queue up next async buffer
		ui32Test02Stage++;
		if (ui32Test02Stage != 5)
			queueTest02BufferDesc(&g_sBufferDesc, ui32Test02Stage);
	}

	return H2D_CB_SUCCESS;
}

static void queueTest04BufferDesc(H2D_sAsyncBufferDesc *psBufferDesc, unsigned int testStage)
{
	switch (testStage)
	{
		case 1:
		{
			/*
			** a) Basic async message.
			*/
			psBufferDesc->eBuffType = H2D_BT_MSG;
			psBufferDesc->ui32DataBufferNum = 0;
			psBufferDesc->apui8Buffer[0] = asyncBuff0;
			psBufferDesc->apui8Buffer[1] = asyncBuff1;
			psBufferDesc->aui32BufferMaxBytes[0] = 32;
			psBufferDesc->aui32BufferMaxBytes[1] = 32;
			break;
		}
		case 2:
		{
			/*
			** b) Max sized buffers & data.
			*/
			psBufferDesc->eBuffType = H2D_BT_MSG;
			psBufferDesc->ui32DataBufferNum = 0;
			psBufferDesc->apui8Buffer[0] = asyncBuff0;
			psBufferDesc->apui8Buffer[1] = asyncBuff1;
			psBufferDesc->aui32BufferMaxBytes[0] = DACOMMS_RSP_ASYNC_DATA_SIZE;
			psBufferDesc->aui32BufferMaxBytes[1] = DACOMMS_RSP_ASYNC_DATA_SIZE;
			break;
		}
		case 3:
		{
			/*
			** c) Too much data in async. (Host side)
			*/
			psBufferDesc->eBuffType = H2D_BT_MSG;
			psBufferDesc->ui32DataBufferNum = 0;
			psBufferDesc->apui8Buffer[0] = asyncBuff0;
			psBufferDesc->apui8Buffer[1] = asyncBuff1;
			psBufferDesc->aui32BufferMaxBytes[0] = ((DACOMMS_RSP_ASYNC_DATA_SIZE/2)-1);
			psBufferDesc->aui32BufferMaxBytes[1] = ((DACOMMS_RSP_ASYNC_DATA_SIZE/2)-1);
			break;
		}
		case 4:
		{
			/*
			** d) Too much data in async. (Device side). Do not queue.
			*/
			psBufferDesc->eBuffType = H2D_BT_MSG;
			psBufferDesc->ui32DataBufferNum = 0;
			psBufferDesc->apui8Buffer[0] = asyncBuff0;
			psBufferDesc->apui8Buffer[1] = asyncBuff1;
			psBufferDesc->aui32BufferMaxBytes[0] = DACOMMS_RSP_ASYNC_DATA_SIZE;
			psBufferDesc->aui32BufferMaxBytes[1] = DACOMMS_RSP_ASYNC_DATA_SIZE;
			break;
		}
		case 5:
		{
			/*
			** e) Basic async message.
			*/
			psBufferDesc->eBuffType = H2D_BT_MSG;
			psBufferDesc->ui32DataBufferNum = 0;
			psBufferDesc->apui8Buffer[0] = asyncBuff0;
			psBufferDesc->apui8Buffer[1] = asyncBuff1;
			psBufferDesc->aui32BufferMaxBytes[0] = 32;
			psBufferDesc->aui32BufferMaxBytes[1] = 32;
			break;
		}
		default:
		{
			assert(0);
			break;
		}
	}
		IMG_MEMSET(asyncBuff0, 0, RESP_ASYNC_BUFFER_SIZE);
		IMG_MEMSET(asyncBuff1, 0, RESP_ASYNC_BUFFER_SIZE);
		H2D_QueueBuff(psBufferDesc);
}

static H2D_eCBResult test04Callback(H2D_sAsyncCallbackDesc *psAsyncDesc)
{
	switch (ui32Test04Stage)
	{
		case 1:
		{
			/*
			** a) Basic async message.
			*/
			if ( ((psAsyncDesc->eEvent) != H2D_AE_MSG) ||
			     ((psAsyncDesc->ui32DataBufferNum) != 0) ||
			     ((psAsyncDesc->apui8Buff[0]) != asyncBuff0) ||
			     ((psAsyncDesc->apui8Buff[1]) != asyncBuff1) ||
			     ((psAsyncDesc->aui32BuffNumBytes[0]) != 32) ||
			     ((psAsyncDesc->aui32BuffNumBytes[1]) != 32)
			)
			{
				VerboseLog("Test 04-a: FAIL. Incorrect callback descriptor.\n");
				assert(0);
			}
			else if (DATA_FAIL == checkTestBuffer(psAsyncDesc->apui8Buff[0], 32, 0))
			{
				VerboseLog("Test 04-a: FAIL. Incorrect data.\n");
				assert(0);
			}
			else if (DATA_FAIL == checkTestBuffer(psAsyncDesc->apui8Buff[1], 32, 20))
			{
				VerboseLog("Test 04-a: FAIL. Incorrect data.\n");
				assert(0);
			}
			else
				VerboseLog("Test 04-a: PASS.\n");
			break;
		}
		case 2:
		{
			/*
			** b) Max sized buffer and data.
			**    Both goups have to fit in a buffer of DACOMMS_RSP_ASYNC_DATA_SIZE
			*/
			if ( ((psAsyncDesc->eEvent) != H2D_AE_MSG) ||
			     ((psAsyncDesc->ui32DataBufferNum) != 0) ||
			     ((psAsyncDesc->apui8Buff[0]) != asyncBuff0) ||
			     ((psAsyncDesc->apui8Buff[1]) != asyncBuff1) ||
			     ((psAsyncDesc->aui32BuffNumBytes[0]) != (DACOMMS_RSP_ASYNC_DATA_SIZE/2)) ||
			     ((psAsyncDesc->aui32BuffNumBytes[1]) != (DACOMMS_RSP_ASYNC_DATA_SIZE/2))
			)
			{
				VerboseLog("Test 04-b: FAIL. Incorrect callback descriptor.\n");
				assert(0);
			}
			else if (DATA_FAIL == checkTestBuffer(psAsyncDesc->apui8Buff[0], (DACOMMS_RSP_ASYNC_DATA_SIZE/2), 1))
			{
				VerboseLog("Test 04-b: FAIL. Incorrect data.\n");
				assert(0);
			}
			else if (DATA_FAIL == checkTestBuffer(psAsyncDesc->apui8Buff[1], (DACOMMS_RSP_ASYNC_DATA_SIZE/2), 21))
			{
				VerboseLog("Test 04-b: FAIL. Incorrect data.\n");
				assert(0);
			}
			else
				VerboseLog("Test 04-b: PASS.\n");
			break;
		}
		case 3:
		{
			/*
			** c) Too much data in async. (Host side)
			*/
			if ((psAsyncDesc->eEvent) != H2D_AE_BUFFER_UNAVAILABLE)
			{
				VerboseLog("Test 04-c: FAIL. Incorrect callback descriptor.\n");
				assert(0);
			}
			break;
		}
		case 4:
		{
			/*
			** c) Too much data in async. (Host side). Need to clear buffer queue.
			*/
			if ( ((psAsyncDesc->eEvent) != H2D_AE_MSG) ||
			     ((psAsyncDesc->ui32DataBufferNum) != 0) ||
			     ((psAsyncDesc->apui8Buff[0]) != asyncBuff0) ||
			     ((psAsyncDesc->apui8Buff[1]) != asyncBuff1) ||
			     ((psAsyncDesc->aui32BuffNumBytes[0]) != ((DACOMMS_RSP_ASYNC_DATA_SIZE/2)-1)) ||
			     ((psAsyncDesc->aui32BuffNumBytes[1]) != ((DACOMMS_RSP_ASYNC_DATA_SIZE/2)-1))
			)
			{
				VerboseLog("Test 04-c: FAIL. Incorrect callback descriptor.\n");
				assert(0);
			}
			else if (DATA_FAIL == checkTestBuffer(psAsyncDesc->apui8Buff[0], ((DACOMMS_RSP_ASYNC_DATA_SIZE/2)-1), 12))
			{
				VerboseLog("Test 04-c: FAIL. Incorrect data.\n");
				assert(0);
			}
			else if (DATA_FAIL == checkTestBuffer(psAsyncDesc->apui8Buff[1], ((DACOMMS_RSP_ASYNC_DATA_SIZE/2)-1), 32))
			{
				VerboseLog("Test 04-c: FAIL. Incorrect data.\n");
				assert(0);
			}
			else
				VerboseLog("Test 04-c: PASS.\n");
			break;
		}
			/*
			** d) Too much data in async. (Device side)
			*/
			// ASYNC WILL NOT BE CALLED
		case 5:
		{
			/*
			** e) Basic async message.
			*/
			if ( ((psAsyncDesc->eEvent) != H2D_AE_MSG) ||
			     ((psAsyncDesc->ui32DataBufferNum) != 0) ||
			     ((psAsyncDesc->apui8Buff[0]) != asyncBuff0) ||
			     ((psAsyncDesc->apui8Buff[1]) != asyncBuff1) ||
			     ((psAsyncDesc->aui32BuffNumBytes[0]) != 32) ||
			     ((psAsyncDesc->aui32BuffNumBytes[1]) != 32)
			)
			{
				VerboseLog("Test 04-e: FAIL. Incorrect callback descriptor.\n");
				assert(0);
			}
			else if (DATA_FAIL == checkTestBuffer(psAsyncDesc->apui8Buff[0], 32, 4))
			{
				VerboseLog("Test 04-e: FAIL. Incorrect data.\n");
				assert(0);
			}
			else if (DATA_FAIL == checkTestBuffer(psAsyncDesc->apui8Buff[1], 32, 24))
			{
				VerboseLog("Test 04-e: FAIL. Incorrect data.\n");
				assert(0);
			}
			else
				VerboseLog("Test 04-e: PASS.\n");
			break;
		}

		default:
		{
			assert(0);
			break;
		}
	}

	if (ui32Test04Stage == 5)
		//if we are done, then signal main task
		KRN_setSemaphore( &g_sTest04CompleteSemaphore, 1 );
	else
	{
		//queue up next async buffer
		ui32Test04Stage++;
		if (ui32Test04Stage != 4)
			queueTest04BufferDesc(&g_sBufferDesc, ui32Test04Stage);
	}

	return H2D_CB_SUCCESS;
}



