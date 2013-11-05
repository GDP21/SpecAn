/*!
******************************************************************************
 @file   HPCommsTestMtx.c

 @brief  Host Port Interface test harness, Client side.

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
#include "d2h_api.h"
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
#define QIO_NO_VECTORS          (128)
QIO_IVDESC_T QIO_MTXIVDesc =
{
	{ HWSTATEXT, CR_PERIP_HWSTATEXT, 0, 0 },
	{ HWVEC0EXT, CR_PERIP_HWVEC0EXT, 0, 0 },
	( 4 + ( 1 << 16 ) )
};
IMG_UINT32 QIO_MTXLevelRegisters[] = { HWLEVELEXT, CR_PERIP_HWLEVELEXT, 0, 0 };

/* NB QIO_NO_VECTORS, QIO_MTXIVDesc and QIO_MTXLevelRegisters are from Comet SoC system header. */
static QIO_DEVENTRY_T devtab[QIO_NO_VECTORS];
static QIO_DEVENTRY_T directDevTable[NUM_DIRECT_DEVICES];
static QIO_SYS_T      qioSys;


/*
** Data buffers for tests. They are 8 bytes larger than the maximum size
** that the comms module can handle.
*/
#define COMMAND_BUFFER_SIZE (DACOMMS_CMD_DATA_SIZE + 8)
#define RESP_ASYNC_BUFFER_SIZE (DACOMMS_RSP_ASYNC_DATA_SIZE + 8)
static unsigned int commandNumBytes;
static unsigned char commandBuff[COMMAND_BUFFER_SIZE];
static unsigned int responseNumBytes;
static unsigned char responseBuff[RESP_ASYNC_BUFFER_SIZE];
static unsigned int asyncNumBytes0;
static unsigned char asyncBuff0[RESP_ASYNC_BUFFER_SIZE];
static unsigned int asyncNumBytes1;
static unsigned char asyncBuff1[RESP_ASYNC_BUFFER_SIZE];

/*
** For test 03
*/
static D2H_eCBResult asyncCallback(
    D2H_eAsyncEvent				eEvent,
    img_uint8	*				pui8Buff,
    img_uint32					ui32BuffNumBytes
);
static KRN_SEMAPHORE_T		sAsyncCallbackSemaphore;

/*!
******************************************************************************

 @Function              main

 @Description

 @Input     None.

 @Return    None.

******************************************************************************/
int main(int argc, char *argv[])
{
	unsigned int commandNumBytes;
	D2H_sAsyncBufferDesc sBufferDescriptor;
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
        &QIO_MTXIVDesc,
        TBID_SIGNUM_TR2(__TBIThreadId() >> TBID_THREAD_S),
        directDevTable,
        NUM_DIRECT_DEVICES);

    /* Map the peripheral interrupts to EXT0 interrupt of MTX */
	QIO_chainInterrupts( 0, 13, 1, 1, QIO_MTXLevelRegisters);

	/* Start the timer task */
    timerTask = KRN_startTimerTask("Timer Task",
                                   timerTaskStack,
                                   TIMER_STACK_SIZE_IN_WORDS,
                                   TIMER_TICK);

	D2H_Initialise(MEOS_MAX_PRIORITY - 1, (IMG_UINT32) NULL);

	/******************************************
	**
	** Test 01: synchronous command/response
	**
	*******************************************/

	/*
	** a) Basic command respose. 32 byte command & response.
	*/

	IMG_MEMSET(commandBuff, 0, COMMAND_BUFFER_SIZE);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (D2H_SUCCESS != D2H_ReadCmdBlocking(commandBuff, 32, &commandNumBytes))
	{
		VerboseLog("Test 01-a: FAIL. D2H_ReadCmdBlocking error.\n");
		assert(0);
	}

	if (commandNumBytes != 32)
	{
		VerboseLog("Test 01-a: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(commandBuff, 32, 0))
	{
		VerboseLog("Test 01-a: FAIL. Incorrect data.\n");
		assert(0);
	}

	fillTestBuffer(responseBuff, RESP_ASYNC_BUFFER_SIZE, 10);

	if(D2H_SUCCESS != D2H_SendRsp(responseBuff, 32))
	{
		VerboseLog("Test 01-a: FAIL. D2H_SendRsp error.\n");
		assert(0);
	}

	/*
	** b) Max sized buffers & data.
	*/

	IMG_MEMSET(commandBuff, 0, COMMAND_BUFFER_SIZE);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (D2H_SUCCESS != D2H_ReadCmdBlocking(commandBuff, DACOMMS_CMD_DATA_SIZE, &commandNumBytes))
	{
		VerboseLog("Test 01-b: FAIL. D2H_ReadCmdBlocking error.\n");
		assert(0);
	}

	if (commandNumBytes != DACOMMS_CMD_DATA_SIZE)
	{
		VerboseLog("Test 01-b: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(commandBuff, DACOMMS_CMD_DATA_SIZE, 1))
	{
		VerboseLog("Test 01-b: FAIL. Incorrect data.\n");
		assert(0);
	}

	fillTestBuffer(responseBuff, RESP_ASYNC_BUFFER_SIZE, 11);

	if(D2H_SUCCESS != D2H_SendRsp(responseBuff, DACOMMS_RSP_ASYNC_DATA_SIZE))
	{
		VerboseLog("Test 01-b: FAIL. D2H_SendRsp error.\n");
		assert(0);
	}

	/*
	** c) Max sized data. Oversized buffers.
	*/

	IMG_MEMSET(commandBuff, 0, COMMAND_BUFFER_SIZE);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (D2H_SUCCESS != D2H_ReadCmdBlocking(commandBuff, COMMAND_BUFFER_SIZE, &commandNumBytes))
	{
		VerboseLog("Test 01-c: FAIL. D2H_ReadCmdBlocking error.\n");
		assert(0);
	}

	if (commandNumBytes != DACOMMS_CMD_DATA_SIZE)
	{
		VerboseLog("Test 01-c: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(commandBuff, DACOMMS_CMD_DATA_SIZE, 2))
	{
		VerboseLog("Test 01-c: FAIL. Incorrect data.\n");
		assert(0);
	}

	fillTestBuffer(responseBuff, RESP_ASYNC_BUFFER_SIZE, 12);

	if(D2H_SUCCESS != D2H_SendRsp(responseBuff, DACOMMS_RSP_ASYNC_DATA_SIZE))
	{
		VerboseLog("Test 01-c: FAIL. D2H_SendRsp error.\n");
		assert(0);
	}

	/*
	** d) Too much data in command buffer (Host side).
	*/

	//nothing to do

	/*
	** e) Too much data in command buffer (Device side).
	*/

	IMG_MEMSET(commandBuff, 0, COMMAND_BUFFER_SIZE);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (D2H_CMD_BUFF_OVERFLOW != D2H_ReadCmdBlocking(commandBuff, DACOMMS_CMD_DATA_SIZE - 1, &commandNumBytes))
	{
		VerboseLog("Test 01-e: FAIL. D2H_ReadCmdBlocking error.\n");
		assert(0);
	}

	if (commandNumBytes != 0)
	{
		VerboseLog("Test 01-a: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	fillTestBuffer(responseBuff, RESP_ASYNC_BUFFER_SIZE, 14);

	if(D2H_SUCCESS != D2H_SendRsp(responseBuff, DACOMMS_RSP_ASYNC_DATA_SIZE))
	{
		VerboseLog("Test 01-e: FAIL. D2H_SendRsp error.\n");
		assert(0);
	}

	/*
	** f) Too much data in response buffer (Host side).
	*/

	IMG_MEMSET(commandBuff, 0, COMMAND_BUFFER_SIZE);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (D2H_SUCCESS != D2H_ReadCmdBlocking(commandBuff, COMMAND_BUFFER_SIZE, &commandNumBytes))
	{
		VerboseLog("Test 01-f: FAIL. D2H_ReadCmdBlocking error.\n");
		assert(0);
	}

	if (commandNumBytes != DACOMMS_CMD_DATA_SIZE)
	{
		VerboseLog("Test 01-f: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(commandBuff, DACOMMS_CMD_DATA_SIZE, 5))
	{
		VerboseLog("Test 01-f: FAIL. Incorrect data.\n");
		assert(0);
	}

	fillTestBuffer(responseBuff, RESP_ASYNC_BUFFER_SIZE, 15);

	if(D2H_SUCCESS != D2H_SendRsp(responseBuff, DACOMMS_RSP_ASYNC_DATA_SIZE))
	{
		VerboseLog("Test 01-f: FAIL. D2H_SendRsp error.\n");
		assert(0);
	}

	/*
	** g) Too much data in response buffer (Device side).
	*/

	IMG_MEMSET(commandBuff, 0, COMMAND_BUFFER_SIZE);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (D2H_SUCCESS != D2H_ReadCmdBlocking(commandBuff, COMMAND_BUFFER_SIZE, &commandNumBytes))
	{
		VerboseLog("Test 01-g: FAIL. D2H_ReadCmdBlocking error.\n");
		assert(0);
	}

	if (commandNumBytes != DACOMMS_CMD_DATA_SIZE)
	{
		VerboseLog("Test 01-g: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(commandBuff, DACOMMS_CMD_DATA_SIZE, 6))
	{
		VerboseLog("Test 01-g: FAIL. Incorrect data.\n");
		assert(0);
	}

	fillTestBuffer(responseBuff, RESP_ASYNC_BUFFER_SIZE, 0);

	if(D2H_COMMS_ERROR != D2H_SendRsp(responseBuff, RESP_ASYNC_BUFFER_SIZE))
	{
		VerboseLog("Test 01-g: FAIL. D2H_SendRsp error.\n");
		assert(0);
	}

	fillTestBuffer(responseBuff, RESP_ASYNC_BUFFER_SIZE, 16);

	if(D2H_SUCCESS != D2H_SendRsp(responseBuff, DACOMMS_RSP_ASYNC_DATA_SIZE))
	{
		VerboseLog("Test 01-g: FAIL. D2H_SendRsp error.\n");
		assert(0);
	}

	/******************************************
	**
	** Test 02: asynchronous message sending, single group.
	**
	*******************************************/

	IMG_MEMSET(commandBuff, 0, COMMAND_BUFFER_SIZE);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (D2H_SUCCESS != D2H_ReadCmdBlocking(commandBuff, 32, &commandNumBytes))
	{
		VerboseLog("Test 02: FAIL. D2H_ReadCmdBlocking error.\n");
		assert(0);
	}

	if (commandNumBytes != 32)
	{
		VerboseLog("Test 02: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(commandBuff, 32, 0))
	{
		VerboseLog("Test 02: FAIL. Incorrect data.\n");
		assert(0);
	}

	fillTestBuffer(responseBuff, RESP_ASYNC_BUFFER_SIZE, 10);

	if(D2H_SUCCESS != D2H_SendRsp(responseBuff, 32))
	{
		VerboseLog("Test 02: FAIL. D2H_SendRsp error.\n");
		assert(0);
	}

	/*
	** a) Basic async message.
	*/

	sBufferDescriptor.eBuffType = D2H_BT_MSG;
	sBufferDescriptor.ui32DataBufferNum = 0;
	sBufferDescriptor.apui8Buffer[0] = asyncBuff0;
	sBufferDescriptor.apui8Buffer[1] = IMG_NULL;
	sBufferDescriptor.aui32BufferNumBytes[0] = 32;
	sBufferDescriptor.aui32BufferNumBytes[1] = 0;

	fillTestBuffer(asyncBuff0, RESP_ASYNC_BUFFER_SIZE, 0);

	if(D2H_SUCCESS != D2H_SendAsyncBuff(&sBufferDescriptor))
	{
		VerboseLog("Test 02-a: FAIL. D2H_SendAsyncBuff error.\n");
		assert(0);
	}

	/*
	** b) Max sized buffers & data.
	*/

	sBufferDescriptor.eBuffType = D2H_BT_MSG;
	sBufferDescriptor.ui32DataBufferNum = 0;
	sBufferDescriptor.apui8Buffer[0] = asyncBuff0;
	sBufferDescriptor.apui8Buffer[1] = IMG_NULL;
	sBufferDescriptor.aui32BufferNumBytes[0] = DACOMMS_RSP_ASYNC_DATA_SIZE;
	sBufferDescriptor.aui32BufferNumBytes[1] = 0;

	fillTestBuffer(asyncBuff0, RESP_ASYNC_BUFFER_SIZE, 1);

	if(D2H_SUCCESS != D2H_SendAsyncBuff(&sBufferDescriptor))
	{
		VerboseLog("Test 02-b: FAIL. D2H_SendAsyncBuff error.\n");
		assert(0);
	}

	/*
	** c) Max sized data. Oversized buffers.
	*/

	sBufferDescriptor.eBuffType = D2H_BT_MSG;
	sBufferDescriptor.ui32DataBufferNum = 0;
	sBufferDescriptor.apui8Buffer[0] = asyncBuff0;
	sBufferDescriptor.apui8Buffer[1] = IMG_NULL;
	sBufferDescriptor.aui32BufferNumBytes[0] = DACOMMS_RSP_ASYNC_DATA_SIZE;
	sBufferDescriptor.aui32BufferNumBytes[1] = 0;

	fillTestBuffer(asyncBuff0, RESP_ASYNC_BUFFER_SIZE, 2);

	if(D2H_SUCCESS != D2H_SendAsyncBuff(&sBufferDescriptor))
	{
		VerboseLog("Test 02-c: FAIL. D2H_SendAsyncBuff error.\n");
		assert(0);
	}

	/*
	** d) Too much data in async. (Host side)
	*/

	sBufferDescriptor.eBuffType = D2H_BT_MSG;
	sBufferDescriptor.ui32DataBufferNum = 0;
	sBufferDescriptor.apui8Buffer[0] = asyncBuff0;
	sBufferDescriptor.apui8Buffer[1] = IMG_NULL;
	sBufferDescriptor.aui32BufferNumBytes[0] = DACOMMS_RSP_ASYNC_DATA_SIZE;
	sBufferDescriptor.aui32BufferNumBytes[1] = 0;

	fillTestBuffer(asyncBuff0, RESP_ASYNC_BUFFER_SIZE, 3);

	if(D2H_SUCCESS != D2H_SendAsyncBuff(&sBufferDescriptor))
	{
		VerboseLog("Test 02-d: FAIL. D2H_SendAsyncBuff error.\n");
		assert(0);
	}

	sBufferDescriptor.eBuffType = D2H_BT_MSG;
	sBufferDescriptor.ui32DataBufferNum = 0;
	sBufferDescriptor.apui8Buffer[0] = asyncBuff0;
	sBufferDescriptor.apui8Buffer[1] = IMG_NULL;
	sBufferDescriptor.aui32BufferNumBytes[0] = DACOMMS_RSP_ASYNC_DATA_SIZE - 1;
	sBufferDescriptor.aui32BufferNumBytes[1] = 0;

	fillTestBuffer(asyncBuff0, RESP_ASYNC_BUFFER_SIZE, 13);

	if(D2H_SUCCESS != D2H_SendAsyncBuff(&sBufferDescriptor))
	{
		VerboseLog("Test 02-d: FAIL. D2H_SendAsyncBuff error.\n");
		assert(0);
	}

	/*
	** e) Too much data in async. (Device side)
	*/

	sBufferDescriptor.eBuffType = D2H_BT_MSG;
	sBufferDescriptor.ui32DataBufferNum = 0;
	sBufferDescriptor.apui8Buffer[0] = asyncBuff0;
	sBufferDescriptor.apui8Buffer[1] = IMG_NULL;
	sBufferDescriptor.aui32BufferNumBytes[0] = RESP_ASYNC_BUFFER_SIZE;
	sBufferDescriptor.aui32BufferNumBytes[1] = 0;

	fillTestBuffer(asyncBuff0, RESP_ASYNC_BUFFER_SIZE, 4);

	if(D2H_COMMS_ERROR != D2H_SendAsyncBuff(&sBufferDescriptor))
	{
		VerboseLog("Test 02-e: FAIL. D2H_SendAsyncBuff error.\n");
		assert(0);
	}

	/*
	** f) Alternate data buffer number sent from device. Buffer number should be ignored.
	*/

	sBufferDescriptor.eBuffType = D2H_BT_MSG;
	sBufferDescriptor.ui32DataBufferNum = 1;
	sBufferDescriptor.apui8Buffer[0] = asyncBuff0;
	sBufferDescriptor.apui8Buffer[1] = IMG_NULL;
	sBufferDescriptor.aui32BufferNumBytes[0] = 32;
	sBufferDescriptor.aui32BufferNumBytes[1] = 0;

	fillTestBuffer(asyncBuff0, RESP_ASYNC_BUFFER_SIZE, 5);

	if(D2H_SUCCESS != D2H_SendAsyncBuff(&sBufferDescriptor))
	{
		VerboseLog("Test 02-f: FAIL. D2H_SendAsyncBuff error.\n");
		assert(0);
	}

	/*
	** g) Incorrect data buffer number queued on host. Buffer number should be ignored.
	*/

	sBufferDescriptor.eBuffType = D2H_BT_MSG;
	sBufferDescriptor.ui32DataBufferNum = 0;
	sBufferDescriptor.apui8Buffer[0] = asyncBuff0;
	sBufferDescriptor.apui8Buffer[1] = IMG_NULL;
	sBufferDescriptor.aui32BufferNumBytes[0] = 32;
	sBufferDescriptor.aui32BufferNumBytes[1] = 0;

	fillTestBuffer(asyncBuff0, RESP_ASYNC_BUFFER_SIZE, 6);

	if(D2H_SUCCESS != D2H_SendAsyncBuff(&sBufferDescriptor))
	{
		VerboseLog("Test 02-g: FAIL. D2H_SendAsyncBuff error.\n");
		assert(0);
	}

	/******************************************
	**
	** Test 03: asynchronous command/response.
	**
	*******************************************/

	/*
	** Setup callback
	*/
	KRN_initSemaphore( &sAsyncCallbackSemaphore, 0 );
	if (D2H_SUCCESS != D2H_AddCallback(asyncCallback))
	{
		VerboseLog("Test 03-a: FAIL. D2H_AddCallback error.\n");
		assert(0);
	}

	/*
	** a) Basic command respose. 32 byte command & response.
	*/

	IMG_MEMSET(commandBuff, 0, COMMAND_BUFFER_SIZE);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (D2H_SUCCESS != D2H_ReadCmdNonBlocking(commandBuff, 32))
	{
		VerboseLog("Test 03-a: FAIL. D2H_ReadCmdBlocking error.\n");
		assert(0);
	}

	/*
	** Wait for callback to set semaphore
	*/
	KRN_testSemaphore( &sAsyncCallbackSemaphore, 1, KRN_INFWAIT );

	fillTestBuffer(responseBuff, RESP_ASYNC_BUFFER_SIZE, 10);

	if(D2H_SUCCESS != D2H_SendRsp(responseBuff, 32))
	{
		VerboseLog("Test 03-a: FAIL. D2H_SendRsp error.\n");
		assert(0);
	}

	/******************************************
	**
	** Test 04: asynchronous message sending, single group.
	**
	*******************************************/

	IMG_MEMSET(commandBuff, 0, COMMAND_BUFFER_SIZE);
	IMG_MEMSET(responseBuff, 0, RESP_ASYNC_BUFFER_SIZE);

	if (D2H_SUCCESS != D2H_ReadCmdBlocking(commandBuff, 32, &commandNumBytes))
	{
		VerboseLog("Test 04: FAIL. D2H_ReadCmdBlocking error.\n");
		assert(0);
	}

	if (commandNumBytes != 32)
	{
		VerboseLog("Test 04: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(commandBuff, 32, 0))
	{
		VerboseLog("Test 04: FAIL. Incorrect data.\n");
		assert(0);
	}

	fillTestBuffer(responseBuff, RESP_ASYNC_BUFFER_SIZE, 10);

	if(D2H_SUCCESS != D2H_SendRsp(responseBuff, 32))
	{
		VerboseLog("Test 04: FAIL. D2H_SendRsp error.\n");
		assert(0);
	}

	/*
	** a) Basic async message.
	*/

	sBufferDescriptor.eBuffType = D2H_BT_MSG;
	sBufferDescriptor.ui32DataBufferNum = 0;
	sBufferDescriptor.apui8Buffer[0] = asyncBuff0;
	sBufferDescriptor.apui8Buffer[1] = asyncBuff1;
	sBufferDescriptor.aui32BufferNumBytes[0] = 32;
	sBufferDescriptor.aui32BufferNumBytes[1] = 32;

	fillTestBuffer(asyncBuff0, RESP_ASYNC_BUFFER_SIZE, 0);
	fillTestBuffer(asyncBuff1, RESP_ASYNC_BUFFER_SIZE, 20);

	if(D2H_SUCCESS != D2H_SendAsyncBuff(&sBufferDescriptor))
	{
		VerboseLog("Test 04-a: FAIL. D2H_SendAsyncBuff error.\n");
		assert(0);
	}

	/*
	** b) Max sized buffer and data.
	**    Both goups have to fit in a buffer of DACOMMS_RSP_ASYNC_DATA_SIZE
	*/

	sBufferDescriptor.eBuffType = D2H_BT_MSG;
	sBufferDescriptor.ui32DataBufferNum = 0;
	sBufferDescriptor.apui8Buffer[0] = asyncBuff0;
	sBufferDescriptor.apui8Buffer[1] = asyncBuff1;
	sBufferDescriptor.aui32BufferNumBytes[0] = (DACOMMS_RSP_ASYNC_DATA_SIZE/2);
	sBufferDescriptor.aui32BufferNumBytes[1] = (DACOMMS_RSP_ASYNC_DATA_SIZE/2);

	fillTestBuffer(asyncBuff0, RESP_ASYNC_BUFFER_SIZE, 1);
	fillTestBuffer(asyncBuff1, RESP_ASYNC_BUFFER_SIZE, 21);

	if(D2H_SUCCESS != D2H_SendAsyncBuff(&sBufferDescriptor))
	{
		VerboseLog("Test 04-b: FAIL. D2H_SendAsyncBuff error.\n");
		assert(0);
	}

	/*
	** c) Too much data in async. (Host side)
	*/

	sBufferDescriptor.eBuffType = D2H_BT_MSG;
	sBufferDescriptor.ui32DataBufferNum = 0;
	sBufferDescriptor.apui8Buffer[0] = asyncBuff0;
	sBufferDescriptor.apui8Buffer[1] = asyncBuff1;
	sBufferDescriptor.aui32BufferNumBytes[0] = (DACOMMS_RSP_ASYNC_DATA_SIZE/2);
	sBufferDescriptor.aui32BufferNumBytes[1] = (DACOMMS_RSP_ASYNC_DATA_SIZE/2);

	fillTestBuffer(asyncBuff0, RESP_ASYNC_BUFFER_SIZE, 2);
	fillTestBuffer(asyncBuff1, RESP_ASYNC_BUFFER_SIZE, 22);

	if(D2H_SUCCESS != D2H_SendAsyncBuff(&sBufferDescriptor))
	{
		VerboseLog("Test 04-c: FAIL. D2H_SendAsyncBuff error.\n");
		assert(0);
	}

	sBufferDescriptor.eBuffType = D2H_BT_MSG;
	sBufferDescriptor.ui32DataBufferNum = 0;
	sBufferDescriptor.apui8Buffer[0] = asyncBuff0;
	sBufferDescriptor.apui8Buffer[1] = asyncBuff1;
	sBufferDescriptor.aui32BufferNumBytes[0] = ((DACOMMS_RSP_ASYNC_DATA_SIZE/2)-1);
	sBufferDescriptor.aui32BufferNumBytes[1] = ((DACOMMS_RSP_ASYNC_DATA_SIZE/2)-1);

	fillTestBuffer(asyncBuff0, RESP_ASYNC_BUFFER_SIZE, 12);
	fillTestBuffer(asyncBuff1, RESP_ASYNC_BUFFER_SIZE, 32);

	if(D2H_SUCCESS != D2H_SendAsyncBuff(&sBufferDescriptor))
	{
		VerboseLog("Test 04-c: FAIL. D2H_SendAsyncBuff error.\n");
		assert(0);
	}

	/*
	** d) Too much data in async. (Device side)
	*/

	sBufferDescriptor.eBuffType = D2H_BT_MSG;
	sBufferDescriptor.ui32DataBufferNum = 0;
	sBufferDescriptor.apui8Buffer[0] = asyncBuff0;
	sBufferDescriptor.apui8Buffer[1] = asyncBuff1;
	sBufferDescriptor.aui32BufferNumBytes[0] = DACOMMS_RSP_ASYNC_DATA_SIZE;
	sBufferDescriptor.aui32BufferNumBytes[1] = DACOMMS_RSP_ASYNC_DATA_SIZE;

	fillTestBuffer(asyncBuff0, RESP_ASYNC_BUFFER_SIZE, 3);
	fillTestBuffer(asyncBuff1, RESP_ASYNC_BUFFER_SIZE, 23);

	if(D2H_COMMS_ERROR != D2H_SendAsyncBuff(&sBufferDescriptor))
	{
		VerboseLog("Test 04-d: FAIL. D2H_SendAsyncBuff error.\n");
		assert(0);
	}

	/*
	** e) Basic async message.
	*/

	sBufferDescriptor.eBuffType = D2H_BT_MSG;
	sBufferDescriptor.ui32DataBufferNum = 0;
	sBufferDescriptor.apui8Buffer[0] = asyncBuff0;
	sBufferDescriptor.apui8Buffer[1] = asyncBuff1;
	sBufferDescriptor.aui32BufferNumBytes[0] = 32;
	sBufferDescriptor.aui32BufferNumBytes[1] = 32;

	fillTestBuffer(asyncBuff0, RESP_ASYNC_BUFFER_SIZE, 4);
	fillTestBuffer(asyncBuff1, RESP_ASYNC_BUFFER_SIZE, 24);

	if(D2H_SUCCESS != D2H_SendAsyncBuff(&sBufferDescriptor))
	{
		VerboseLog("Test 04-e: FAIL. D2H_SendAsyncBuff error.\n");
		assert(0);
	}

	while (1) {};

	return 0;
}

/*!
******************************************************************************

 @Function              asyncCallback

******************************************************************************/
static D2H_eCBResult asyncCallback(
    D2H_eAsyncEvent				eEvent,
    img_uint8	*				pui8Buff,
    img_uint32					ui32BuffNumBytes
)
{
	/*
	** a) Basic async message.
	*/
	if ( (eEvent != D2H_BT_MSG) ||
		 (pui8Buff != commandBuff) ||
		 (ui32BuffNumBytes != 32)
	)
	{
		VerboseLog("Test 03-a: FAIL. Incorrect number of bytes read.\n");
		assert(0);
	}

	if (DATA_FAIL == checkTestBuffer(commandBuff, 32, 0))
	{
		VerboseLog("Test 03-a: FAIL. Incorrect data.\n");
		assert(0);
	}

	KRN_setSemaphore( &sAsyncCallbackSemaphore, 1 );

	return D2H_CB_SUCCESS;
}
