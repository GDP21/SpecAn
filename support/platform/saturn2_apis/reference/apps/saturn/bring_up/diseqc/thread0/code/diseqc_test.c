/*!
******************************************************************************
 @file   : diseqc_test.c

 @brief

 @Author Imagination Technologies

 @date   10/11/2010

         <b>Copyright 2011 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to third parties without the
         express written permission of Imagination Technologies
         Limited, Unit 8, HomePark Industrial Estate,
         King's Langley, Hertfordshire, WD4 8LZ, U.K.

 <b>Description:</b>\n
         DiSEqC Test Application.

 <b>Platform:</b>\n
	     Platform Independent

 @Version
	     1.0

******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log:


*****************************************************************************/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES

/*============================================================================
====	I N C L U D E S
=============================================================================*/

#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <MeOS.h>
#include <img_defs.h>
#include <ioblock_defs.h>
#include <system.h>
#include <sys_config.h>
#include <unistd.h>

#include <diseqc_api.h>

/*============================================================================
====	D E F I N E S
=============================================================================*/

/* Timer defines...*/
#define STACK_SIZE			(8092)

#define STACK_FILL			(0xDEADBEEF)

#define	ITER_INF	((IMG_UINT32)-1)

/*============================================================================
====	E N U M S
=============================================================================*/


typedef enum
{
	DISEQC_TEST_AUTO = 1,					/* 1 */
	DISEQC_TEST_CONTINUOUS,					/* 2 */
	DISEQC_TEST_CONTINUOUS_END,				/* 3 */
	DISEQC_TEST_TONE_BURST_A,				/* 4 */
	DISEQC_TEST_TONE_BURST_B,				/* 5 */
	DISEQC_TEST_SEND_MESSAGES,				/* 6 */
	DISEQC_TEST_SEND_RECEIVE_MESSAGES,		/* 7 */
	DISEQC_TEST_TONE_BURST_A_B,				/* 8 */
	DISEQC_TEST_CONTINUOUS_TONE_BURST_A,	/* 9 */
	DISEQC_TEST_CONTINUOUS_TONE_BURST_B,	/* 10 */
	NUM_TESTS_PLUS_ONE,						/* 11 */
} TEST_eTestMode;

#define NUM_TESTS (NUM_TESTS_PLUS_ONE - 1)

/*============================================================================
====	T Y P E D E F S
=============================================================================*/

typedef struct test_file_node
{
	IMG_UINT32			ui32Size;
	int					fd;
	IMG_BYTE			aui8Data[255];
}TEST_FILE_NODE;

typedef struct test_desc
{
	IMG_BOOL			bDoTest;
	IMG_BYTE			aui8SendBuf[255];
	IMG_UINT8			ui8SendSize;
	IMG_UINT32			ui32TestNum;
	IMG_UINT32			ui32BlockIndex;
	IMG_UINT32			ui32RXToneTolerance;
	IMG_UINT32			ui32RXMinDetections;
	IMG_UINT32			ui32RXChunkWidth;
	IMG_UINT32			ui32RXPostMesageSilence;
	IMG_UINT32			ui32SlaveResponseTimeout;
	IMG_UINT32			ui32TXShort;
	IMG_UINT32			ui32TXLong;
	IMG_UINT32			ui32MasterEmptyTimeout;
	IMG_UINT32			ui32PostTransactionSilence;
	IMG_BOOL			bOverrideFreq;
	IMG_UINT32			ui32TargetFreq;
	DISEQC_RECEIVE_T	Receive;
	DISEQC_TONE_T		Tone;
	DISEQC_POLARITY_T	InputPolarity;
	DISEQC_POLARITY_T	OutputPolarity;
	DISEQC_LOOPBACK_T	Loopback;
	IMG_UINT32			ui32Iterations;
	TEST_FILE_NODE		sFiles[32];
	IMG_UINT8			ui8NumMessages;
}TEST_DESC;

/*============================================================================
====	D A T A
=============================================================================*/

/* Parameter data strings */
char argv1[64] = {'\0'};
char argv2[64] = {'\0'};
char argv3[64] = {'\0'};
char argv4[64] = {'\0'};
char argv5[64] = {'\0'};
char argv6[64] = {'\0'};
char argv7[64] = {'\0'};
char argv8[64] = {'\0'};
char argv9[64] = {'\0'};
char argv10[64] = {'\0'};
char argv11[64] = {'\0'};
char argv12[64] = {'\0'};
char argv13[64] = {'\0'};
char argv14[64] = {'\0'};
char argv15[64] = {'\0'};
char argv16[64] = {'\0'};
char argv17[64] = {'\0'};
char argv18[64] = {'\0'};
char argv19[64] = {'\0'};
char argv20[64] = {'\0'};
char argv21[64] = {'\0'};
char argv22[64] = {'\0'};
char argv23[64] = {'\0'};
char argv24[64] = {'\0'};
char argv25[64] = {'\0'};
char argv26[64] = {'\0'};
char argv27[64] = {'\0'};
char argv28[64] = {'\0'};
char argv29[64] = {'\0'};
char argv30[64] = {'\0'};
char argv31[64] = {'\0'};
char argv32[64] = {'\0'};

/* Array of arguments - the first element is unused, the other elements point to the argvXX[]
** strings that are initialised by script */
char *metag_argv[] = { "dummy", argv1, argv2, argv3, argv4, argv5, argv6, argv7, argv8, argv9, argv10,
						argv11, argv12, argv13, argv14, argv15, argv16, argv17, argv18, argv19, argv20,
						argv21, argv22, argv23, argv24, argv25, argv26, argv27, argv28, argv29, argv30, argv31, argv32};
/* Argument count, updated by initialisation script. Useful for debug. */
int metag_argc = 0;

/* Background Task Control Block */
KRN_TASK_T		*g_psBackgroundTask;

/* Timer Task Control Block */
KRN_TASK_T		*g_psTimerTask;

/* MEOS Scheduler */
KRN_SCHEDULE_T	g_sMeOSScheduler;

/* Scheduler queues */
KRN_TASKQ_T		g_asSchedQueues[MEOS_MAX_PRIORITY_LEVEL+1];

/* Stack for the Timer Task */
IMG_UINT32		g_aui32TimerStack[TIM_STACK_SIZE];

/* Main Task Control Block */
KRN_TASK_T		g_sMainTask;

/* Stack for the Main Task */
IMG_UINT32		g_aui32TaskStack[STACK_SIZE];

/* QIO structures */
QIO_DEVENTRY_T	g_aVectoredDevTable[QIO_NO_VECTORS];
QIO_SYS_T		g_sQio;

DQ_T			g_sTaskQueue;

static volatile TEST_DESC	g_sTestDescription;

/******************************************************************************
	Internal function prototypes
 ******************************************************************************/

static IMG_VOID ParseCommandLine(IMG_INT iArgc, IMG_CHAR *pszArgv[]);
static IMG_VOID Usage(IMG_CHAR *pszSelfName);
void MainTask();

/* Test the return values of API calls and output an appropriate error with file
   and line number if required */
#define	DISEQC_API_TEST(API)														\
	if((Status = API) != DISEQC_STATUS_OK)											\
	{																				\
		__TBILogF("ERROR: %s %s line %u\n",GetErrorText(Status),__FILE__,__LINE__);	\
		exit(-1);																	\
	}

IMG_UINT64			g_ui64RandX = 0;
IMG_UINT64			g_ui64RandXC = 0;

KRN_SEMAPHORE_T		g_AutotestSema;

IMG_UINT32			g_ui32AutotestUnexpectedNum = 0;

IMG_VOID SRand(IMG_UINT64 ui64X)
{
	g_ui64RandX = ui64X;

	g_ui64RandXC = g_ui64RandX>>1;
}

IMG_UINT32 Rand(IMG_UINT32 ui32Max)
{
	IMG_UINT64	ui64Temp;

	ui64Temp = ((g_ui64RandX * 1103515245) + g_ui64RandXC);
	g_ui64RandX = ui64Temp & 0xFFFFFFFF;
	g_ui64RandXC = ui64Temp >> 32;
	return (IMG_UINT32)(g_ui64RandX % (ui32Max+1));
}

IMG_VOID Random(IMG_BYTE *pui8Buf, IMG_UINT32 ui32Size)
{
	IMG_BYTE	*pui8End = pui8Buf + ui32Size;

	while(pui8Buf < pui8End)
	{
		*pui8Buf++ = Rand(255);
	}
}

/* ERROR text for all DiSEqC status */
IMG_CHAR *GetErrorText(DISEQC_STATUS_T Status)
{
	switch(Status)
	{
    case DISEQC_STATUS_OK:
		return "DISEQC_STATUS_OK";

	case DISEQC_STATUS_ERR_TIMEOUT:
		return "DISEQC_STATUS_ERR_TIMEOUT";

	case DISEQC_STATUS_ERR_CANCEL:
		return "DISEQC_STATUS_ERR_CANCEL";

	case DISEQC_STATUS_ERR_RECEIVE_TIMEOUT:
		return "DISEQC_STATUS_ERR_RECEIVE_TIMEOUT";

	case DISEQC_STATUS_ERR_SEND_TIMEOUT:
		return "DISEQC_STATUS_ERR_SEND_TIMEOUT";

	case DISEQC_STATUS_ERR_RECEIVE_OVERFLOW:
		return "DISEQC_STATUS_ERR_RECEIVE_OVERFLOW";

	case DISEQC_STATUS_ERR_RECEIVE:
		return "DISEQC_STATUS_ERR_RECEIVE";

	case DISEQC_STATUS_ERR_INITIALISED:
		return "DISEQC_STATUS_ERR_INITIALISED";

	case DISEQC_STATUS_ERR_UNINITIALISED:
		return "DISEQC_STATUS_ERR_UNINITIALISED";

	case DISEQC_STATUS_ERR_INVALIDBLOCK:
		return "DISEQC_STATUS_ERR_INVALIDBLOCK";

	case DISEQC_STATUS_ERR_NUM_IO_BLOCKS:
		return "DISEQC_STATUS_ERR_NUM_IO_BLOCKS";

	case DISEQC_STATUS_ERR_UNEXPECTED_SIZE:
		return "DISEQC_STATUS_ERR_UNEXPECTED_SIZE";

	case DISEQC_STATUS_ERR_IO_BLOCK:
		return "DISEQC_STATUS_ERR_IO_BLOCK";

	case DISEQC_STATUS_ERR_CONTINUOUS_TONE:
		return "DISEQC_STATUS_ERR_CONTINUOUS_TONE";

	default:
		return "UNKNOWN";
	}
}


IMG_VOID InterruptCompleteCallback(IMG_BYTE *pData, IMG_UINT8 ui8BytesReceived, DISEQC_STATUS_T Status, IMG_PVOID pContext)
{
	IMG_CHAR aui8Hex[7];
	IMG_CHAR aui8HexText[17] = "0123456789ABCDEF";
	IMG_CHAR aui8NumReceived[20];
	IMG_UINT32 i;
	IMG_UINT32 ui32Context = (IMG_UINT32)pContext;
	IMG_CHAR *Ptr;
	IMG_CHAR *PtrB;
	IMG_CHAR c;

	switch(Status)
	{
	case DISEQC_STATUS_OK:
		switch(ui32Context)
		{
		case 0:
			__TBILogF("Continuous tone complete\n");
			break;
		case 1:
			__TBILogF("Continuous tone end complete\n");
			break;
		case 2:
			__TBILogF("Tone burst A complete\n");
			break;
		case 3:
			__TBILogF("Tone burst B complete\n");
			break;
		case 4:
			__TBILogF("Send message no reply complete\n");
			break;
		case 5:
			__TBILogF("Send message with reply complete...");

			aui8Hex[0]='0';
			aui8Hex[1]='x';
			aui8Hex[4]=',';
			aui8Hex[5]=' ';
			aui8Hex[6]=0;
			__TBILogF("Expected bytes (");
			i = ui8BytesReceived;
			Ptr = aui8NumReceived;
			if(!i)
			{
				*Ptr++ = '0';
			}
			else
			{
				while(i)
				{
					*Ptr++ = (i%10)+'0';
					i /= 10;
				}
			}
			*Ptr-- = 0;
			PtrB = aui8NumReceived;
			while(PtrB<Ptr)
			{
				c = *Ptr;
				*Ptr-- = *PtrB;
				*PtrB++ = c;
			}
			__TBILogF(aui8NumReceived);
			__TBILogF("): ");
			for(i=0;i<ui8BytesReceived;i++)
			{
				aui8Hex[2] = aui8HexText[(pData[i]>>4)&0xF];
				aui8Hex[3] = aui8HexText[pData[i]&0xF];
				__TBILogF(aui8Hex);
			}
			__TBILogF("\n");
			break;
		}
		break;
	case DISEQC_STATUS_ERR_TIMEOUT:
		__TBILogF("ERROR: Operation timeout\n");
		break;
	case DISEQC_STATUS_ERR_CANCEL:
		__TBILogF("ERROR: Operation cancelled\n");
		break;
	case DISEQC_STATUS_ERR_RECEIVE_TIMEOUT:
		__TBILogF("ERROR: Receive timeout\n");
		break;
	case DISEQC_STATUS_ERR_SEND_TIMEOUT:
		__TBILogF("ERROR: Send timeout\n");
		break;
	case DISEQC_STATUS_ERR_RECEIVE_OVERFLOW:
		__TBILogF("ERROR: Receive overflow\n");
		break;
	case DISEQC_STATUS_ERR_RECEIVE:
		__TBILogF("ERROR: Receive FAIL\n");
		break;
	case DISEQC_STATUS_ERR_CONTINUOUS_TONE:
		__TBILogF("ERROR: Continuous tone present on port\n");
		break;
	default:
		__TBILogF("ERROR: UNKNOWN\n");
		break;
	}


}

IMG_VOID InterruptUnexpectedCallback(IMG_BYTE *pData, IMG_UINT8 ui8BytesReceived, DISEQC_STATUS_T Status)
{
	IMG_CHAR	aui8Hex[7];
	IMG_CHAR	aui8HexText[17] = "0123456789ABCDEF";
	IMG_CHAR	aui8NumReceived[20];
	IMG_UINT32	i;
	IMG_CHAR	*Ptr;
	IMG_CHAR	*PtrB;
	IMG_CHAR	c;
	IMG_UINT8	ui8Len;
	IMG_BYTE	aui8Cmp[8];

	if(g_ui32AutotestUnexpectedNum)
	{
		if(Status != DISEQC_STATUS_OK)
		{
			__TBILogF("ERROR: Receive failure\n");
		}
		else
		{
			if(g_ui32AutotestUnexpectedNum==1)
			{
				if(ui8BytesReceived != 8)
				{
					__TBILogF("ERROR: Did not receive 8 bytes as expected\n");
				}
				else if(IMG_MEMCMP("\x00\x00\x00\x00\x00\x00\x00\x00",pData,ui8Len))
				{
					__TBILogF("ERROR: Data received not equal to expected data\n");
				}
				g_ui32AutotestUnexpectedNum++;
			}
			else if(g_ui32AutotestUnexpectedNum==2)
			{
				if(ui8BytesReceived != 8)
				{
					__TBILogF("ERROR: Did not receive 8 bytes as expected\n");
				}
				else if(IMG_MEMCMP("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF",pData,ui8Len))
				{
					__TBILogF("ERROR: Data received not equal to expected data\n");
				}
				g_ui32AutotestUnexpectedNum++;
			}
			else
			{
				ui8Len = Rand(7)+1;
				if(ui8BytesReceived != ui8Len)
				{
					__TBILogF("ERROR: Did not receive %u bytes as expected\n",(IMG_UINT32)ui8Len);
				}
				else
				{
					Random(aui8Cmp,ui8Len);
					if(IMG_MEMCMP(aui8Cmp,pData,ui8Len))
					{
						__TBILogF("ERROR: Data received not equal to expected data\n");
					}
					else
					{
						g_ui32AutotestUnexpectedNum++;
						if(g_ui32AutotestUnexpectedNum == 101)
						{
							KRN_setSemaphore(&g_AutotestSema,1);
						}
					}
				}
			}
		}
	}
	else
	{
		__TBILogF("Unexpected reply...");
		switch(Status)
		{
		case DISEQC_STATUS_OK:
			aui8Hex[0]='0';
			aui8Hex[1]='x';
			aui8Hex[4]=',';
			aui8Hex[5]=' ';
			aui8Hex[6]=0;
			__TBILogF("Unexpected bytes (");
			i = ui8BytesReceived;
			Ptr = aui8NumReceived;
			if(!i)
			{
				*Ptr++ = '0';
			}
			else
			{
				while(i)
				{
					*Ptr++ = (i%10)+'0';
					i /= 10;
				}
			}
			*Ptr-- = 0;
			PtrB = aui8NumReceived;
			while(PtrB<Ptr)
			{
				c = *Ptr;
				*Ptr-- = *PtrB;
				*PtrB++ = c;
			}
			__TBILogF(aui8NumReceived);
			__TBILogF("): ");
			for(i=0;i<ui8BytesReceived;i++)
			{
				aui8Hex[2] = aui8HexText[(pData[i]>>4)&0xF];
				aui8Hex[3] = aui8HexText[pData[i]&0xF];
				__TBILogF(aui8Hex);
			}
			__TBILogF("\n");
			break;
		case DISEQC_STATUS_ERR_RECEIVE_OVERFLOW:
			__TBILogF("ERROR: Receive overflow\n");
			break;
		case DISEQC_STATUS_ERR_RECEIVE:
			__TBILogF("ERROR: Receive FAIL\n");
			break;
		default:
			__TBILogF("ERROR: UNKNOWN\n");
			break;
		}
	}
}


/*!
******************************************************************************

 @Function				MainTask

******************************************************************************/



void MainTask()
{
	SYS_sConfig					sConfig;
	SYS_sDISEQCConfig			sDISEQCConfig;
	DISEQC_PORT_T				sPort;
	DISEQC_PORT_SETTINGS_T		sPortSettings;
	DISEQC_IO_BLOCK_T			asIOBlocks[10];
	IMG_BYTE					aui8UnexpectedBuf[255];
	IMG_BYTE					aui8RecvBuf[255];
	IMG_BYTE					aui8SendBuf[255];
	IMG_UINT32					i;
	DISEQC_RECEIVE_TIMINGS_T	sReceiveTimings;
	DISEQC_SEND_TIMINGS_T		sSendTimings;
	IMG_UINT32					n;
	DISEQC_STATUS_T				Status;
	IMG_UINT8					ui8Received;
	IMG_UINT8					ui8Len;

	IMG_MEMSET(&sConfig, 0, sizeof(SYS_sConfig));
	SYS_Configure(&sConfig);

	sDISEQCConfig.asBlockConfig[0].bConfigure = IMG_TRUE;
	sDISEQCConfig.asBlockConfig[0].bEnable = IMG_TRUE;
	sDISEQCConfig.asBlockConfig[1].bConfigure = IMG_TRUE;
	sDISEQCConfig.asBlockConfig[1].bEnable = IMG_FALSE;
	SYS_ConfigureDISEQC(&sDISEQCConfig);

	sPort.State = DISEQC_PORT_STATE_UNINITIALISED;

	sPortSettings.ui32BlockIndex = g_sTestDescription.ui32BlockIndex;
	sPortSettings.bOverrideFreq = g_sTestDescription.bOverrideFreq;
	sPortSettings.ui32TargetFreq = g_sTestDescription.ui32TargetFreq;

	sReceiveTimings.ui16ToneTolerance = g_sTestDescription.ui32RXToneTolerance;
	sReceiveTimings.ui8ChunkWidth = g_sTestDescription.ui32RXChunkWidth;
	sReceiveTimings.ui8MinDetections = g_sTestDescription.ui32RXMinDetections;
	sReceiveTimings.ui16SlaveResponseTimeout = g_sTestDescription.ui32SlaveResponseTimeout;
	sReceiveTimings.ui8PostMessageSilence = g_sTestDescription.ui32RXPostMesageSilence;
	sPortSettings.psReceiveTimings = &sReceiveTimings;

	sSendTimings.ui8ShortChunkWidth = g_sTestDescription.ui32TXShort;
	sSendTimings.ui8LongChunkWidth = g_sTestDescription.ui32TXLong;
	sSendTimings.ui16MasterEmptyTimeout = g_sTestDescription.ui32MasterEmptyTimeout;
	sSendTimings.ui16PostTransactionSilence = g_sTestDescription.ui32PostTransactionSilence;
	sPortSettings.psSendTimings = &sSendTimings;

	sPortSettings.Receive = g_sTestDescription.Receive;
	sPortSettings.Tone = g_sTestDescription.Tone;
	sPortSettings.InputPolarity = g_sTestDescription.InputPolarity;
	sPortSettings.OutputPolarity = g_sTestDescription.OutputPolarity;
	sPortSettings.Loopback = g_sTestDescription.Loopback;
	sPortSettings.pfnReadUnexpectedCallback = InterruptUnexpectedCallback;
	sPortSettings.pui8UnexpectedData = aui8UnexpectedBuf;
	sPortSettings.ui8UnexpectedDataMaxSize = 255;

	/* Initialise DiSEqC */
	DISEQC_API_TEST(DISEQCInit(&sPort,&sPortSettings,asIOBlocks,10));

	while(1)
	{
		while(g_sTestDescription.bDoTest == IMG_FALSE)
		{
			KRN_hibernate(&g_sTaskQueue,10);
		}

		/* perform test */
		switch(g_sTestDescription.ui32TestNum)
		{
			/************************************************/
			case DISEQC_TEST_AUTO:
			{
				__TBILogF("DiSEqC test auto...\n");

				__TBILogF("Continuous tone (5s)...\n");

				DISEQC_API_TEST(DISEQCStartContinuousTone(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,IMG_NULL,IMG_NULL));

				KRN_hibernate(&g_sTaskQueue,MILLISECOND_TO_TICK(5000));

				DISEQC_API_TEST(DISEQCEndContinuousTone(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,IMG_NULL,IMG_NULL));

				KRN_hibernate(&g_sTaskQueue,MILLISECOND_TO_TICK(100));

				__TBILogF("Tone burst A (x100)...\n");

				for(i=0;i<100;i++)
				{
					DISEQC_API_TEST(DISEQCToneBurstA(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,IMG_NULL,IMG_NULL));
				}

				KRN_hibernate(&g_sTaskQueue,MILLISECOND_TO_TICK(100));

				__TBILogF("Tone burst B (x100)...\n");

				for(i=0;i<100;i++)
				{
					DISEQC_API_TEST(DISEQCToneBurstB(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,IMG_NULL,IMG_NULL));
				}

				KRN_hibernate(&g_sTaskQueue,MILLISECOND_TO_TICK(100));

				__TBILogF("Messages without replies...\n");

				DISEQC_API_TEST(DISEQCSendMessage(&sPort,(IMG_BYTE *)"\x00\x00\x00\x00\x00\x00\x00\x00",8,IMG_NULL,0,IMG_NULL,DISEQC_SYNC_SYNC,KRN_INFWAIT,IMG_NULL,IMG_NULL));

				DISEQC_API_TEST(DISEQCSendMessage(&sPort,(IMG_BYTE *)"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF",8,IMG_NULL,0,IMG_NULL,DISEQC_SYNC_SYNC,KRN_INFWAIT,IMG_NULL,IMG_NULL));

				SRand(0x12345678);
				for(i=0;i<98;i++)
				{
					ui8Len = Rand(7)+1;
					Random(aui8SendBuf,ui8Len);
					DISEQC_API_TEST(DISEQCSendMessage(&sPort,aui8SendBuf,ui8Len,IMG_NULL,0,IMG_NULL,DISEQC_SYNC_SYNC,KRN_INFWAIT,IMG_NULL,IMG_NULL));
				}

				KRN_hibernate(&g_sTaskQueue,MILLISECOND_TO_TICK(100));

				__TBILogF("Message with replies...\n");

				DISEQC_API_TEST(DISEQCSendMessage(&sPort,(IMG_BYTE *)"\x00\x00\x00\x00\x00\x00\x00\x00",8,aui8RecvBuf,8,&ui8Received,DISEQC_SYNC_SYNC,KRN_INFWAIT,IMG_NULL,IMG_NULL));

				if(ui8Received!=8)
				{
					__TBILogF("ERROR: Did not receive 8 bytes as expected\n");
					exit(-1);
				}
				if(IMG_MEMCMP(aui8RecvBuf,"\x00\x00\x00\x00\x00\x00\x00\x00",8))
				{
					__TBILogF("ERROR: Data received not equal to expected data\n");
					exit(-1);
				}

				DISEQC_API_TEST(DISEQCSendMessage(&sPort,(IMG_BYTE *)"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF",8,aui8RecvBuf,8,&ui8Received,DISEQC_SYNC_SYNC,KRN_INFWAIT,IMG_NULL,IMG_NULL));

				if(ui8Received!=8)
				{
					__TBILogF("ERROR: Did not receive 8 bytes as expected\n");
					exit(-1);
				}
				if(IMG_MEMCMP(aui8RecvBuf,"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF",8))
				{
					__TBILogF("ERROR: Data received not equal to expected data\n");
					exit(-1);
				}

				SRand(0x87654321);
				for(i=0;i<98;i++)
				{
					ui8Len = Rand(7)+1;
					Random(aui8SendBuf,ui8Len);
					DISEQC_API_TEST(DISEQCSendMessage(&sPort,aui8SendBuf,ui8Len,aui8RecvBuf,8,&ui8Received,DISEQC_SYNC_SYNC,KRN_INFWAIT,IMG_NULL,IMG_NULL));

					ui8Len = Rand(7)+1;
					Random(aui8SendBuf,ui8Len);

					if(ui8Received!=ui8Len)
					{
						__TBILogF("ERROR: Did not receive %u bytes as expected\n",(IMG_UINT32)ui8Len);
						exit(-1);
					}
					if(IMG_MEMCMP(aui8RecvBuf,aui8SendBuf,ui8Len))
					{
						__TBILogF("ERROR: Data received not equal to expected data\n");
						exit(-1);
					}
				}

				__TBILogF("Unexpected messages...\n");

				g_ui32AutotestUnexpectedNum = 1;
				KRN_initSemaphore(&g_AutotestSema,0);
				SRand(0x55555555);

				KRN_testSemaphore(&g_AutotestSema,1,KRN_INFWAIT);

				__TBILogF("SUCCESS!\n");
				break;
			}

			case DISEQC_TEST_CONTINUOUS:
			{
				__TBILogF("DiSEqC test continuous...\n");

				DISEQC_API_TEST(DISEQCStartContinuousTone(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)0));

				for(n=0;n<g_sTestDescription.ui32Iterations;n++)
				{
					if(g_sTestDescription.ui32Iterations==ITER_INF)
					{
						if(g_sTestDescription.bDoTest==IMG_FALSE){break;}
						n=0;
					}
					KRN_hibernate(&g_sTaskQueue,1000);
				}

				DISEQC_API_TEST(DISEQCEndContinuousTone(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)1));

				__TBILogF("SUCCESS!\n");
				break;
			}

			case DISEQC_TEST_CONTINUOUS_END:
			{
				__TBILogF("DiSEqC test continuous end...\n");

				for(n=0;n<g_sTestDescription.ui32Iterations;n++)
				{
					if(g_sTestDescription.ui32Iterations==ITER_INF)
					{
						if(g_sTestDescription.bDoTest==IMG_FALSE){break;}
						n=0;
					}

					DISEQC_API_TEST(DISEQCStartContinuousTone(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)0));
					KRN_hibernate(&g_sTaskQueue,1000);
					DISEQC_API_TEST(DISEQCEndContinuousTone(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)1));
					KRN_hibernate(&g_sTaskQueue,1000);
				}

				__TBILogF("SUCCESS!\n");
				break;
			}

			case DISEQC_TEST_TONE_BURST_A:
			{
				__TBILogF("DiSEqC test tone burst A...\n");

				for(n=0;n<g_sTestDescription.ui32Iterations;n++)
				{
					if(g_sTestDescription.ui32Iterations==ITER_INF)
					{
						if(g_sTestDescription.bDoTest==IMG_FALSE){break;}
						n=0;
					}
					DISEQC_API_TEST(DISEQCToneBurstA(&sPort,DISEQC_SYNC_ASYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)2));
				}
				__TBILogF("SUCCESS!\n");
				break;
			}

			case DISEQC_TEST_TONE_BURST_B:
			{
				__TBILogF("DiSEqC test tone burst B...\n");

				for(n=0;n<g_sTestDescription.ui32Iterations;n++)
				{
					if(g_sTestDescription.ui32Iterations==ITER_INF)
					{
						if(g_sTestDescription.bDoTest==IMG_FALSE){break;}
						n=0;
					}
					DISEQC_API_TEST(DISEQCToneBurstB(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)3));
				}


				__TBILogF("SUCCESS!\n");
				break;
			}

			case DISEQC_TEST_SEND_MESSAGES:
			{
				__TBILogF("DiSEqC test send messages...\n");

				for(n=0;n<g_sTestDescription.ui32Iterations;n++)
				{
					if(g_sTestDescription.ui32Iterations==ITER_INF)
					{
						if(g_sTestDescription.bDoTest==IMG_FALSE){break;}
						n=0;
					}
					for(i=0;i<g_sTestDescription.ui8NumMessages;i++)
					{
						DISEQC_API_TEST(DISEQCSendMessage(&sPort,(IMG_BYTE *)g_sTestDescription.sFiles[i].aui8Data,g_sTestDescription.sFiles[i].ui32Size,
										IMG_NULL,0,IMG_NULL,DISEQC_SYNC_SYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)4));
					}
				}

				__TBILogF("SUCCESS!\n");
				break;
			}

			case DISEQC_TEST_SEND_RECEIVE_MESSAGES:
			{
				__TBILogF("DiSEqC test send receive messages...\n");

				for(n=0;n<g_sTestDescription.ui32Iterations;n++)
				{
					if(g_sTestDescription.ui32Iterations==ITER_INF)
					{
						if(g_sTestDescription.bDoTest==IMG_FALSE){break;}
						n=0;
					}
					for(i=0;i<g_sTestDescription.ui8NumMessages;i++)
					{
						DISEQC_API_TEST(DISEQCSendMessage(&sPort,(IMG_BYTE *)g_sTestDescription.sFiles[i].aui8Data,g_sTestDescription.sFiles[i].ui32Size,
										aui8RecvBuf,255,IMG_NULL,DISEQC_SYNC_SYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)5));
					}
				}

				__TBILogF("SUCCESS!\n");
				break;
			}
			case DISEQC_TEST_TONE_BURST_A_B:
			{
				__TBILogF("DiSEqC test tone burst A B...\n");

				for(n=0;n<g_sTestDescription.ui32Iterations;n++)
				{
					if(g_sTestDescription.ui32Iterations==ITER_INF)
					{
						if(g_sTestDescription.bDoTest==IMG_FALSE){break;}
						n=0;
					}
					DISEQC_API_TEST(DISEQCToneBurstA(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)3));
					DISEQC_API_TEST(DISEQCToneBurstB(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)3));
				}


				__TBILogF("SUCCESS!\n");
				break;
			}
			case DISEQC_TEST_CONTINUOUS_TONE_BURST_A:
			{
				__TBILogF("DiSEqC test tone burst A (continuous)...\n");

				for(n=0;n<g_sTestDescription.ui32Iterations;n++)
				{
					if(g_sTestDescription.ui32Iterations==ITER_INF)
					{
						if(g_sTestDescription.bDoTest==IMG_FALSE){break;}
						n=0;
					}

					DISEQC_API_TEST(DISEQCStartContinuousTone(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)0));
					KRN_hibernate(&g_sTaskQueue,1000);
					DISEQC_API_TEST(DISEQCEndContinuousTone(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)1));
					DISEQC_API_TEST(DISEQCToneBurstA(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)2));
				}

				__TBILogF("SUCCESS!\n");
				break;
			}
			case DISEQC_TEST_CONTINUOUS_TONE_BURST_B:
			{
				__TBILogF("DiSEqC test tone burst B (continuous)...\n");

				for(n=0;n<g_sTestDescription.ui32Iterations;n++)
				{
					if(g_sTestDescription.ui32Iterations==ITER_INF)
					{
						if(g_sTestDescription.bDoTest==IMG_FALSE){break;}
						n=0;
					}

					DISEQC_API_TEST(DISEQCStartContinuousTone(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)0));
					KRN_hibernate(&g_sTaskQueue,1000);
					DISEQC_API_TEST(DISEQCEndContinuousTone(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)1));
					DISEQC_API_TEST(DISEQCToneBurstB(&sPort,DISEQC_SYNC_SYNC,KRN_INFWAIT,InterruptCompleteCallback,(IMG_PVOID)2));
				}

				__TBILogF("SUCCESS!\n");
				break;
			}
			/************************************************/
		}
		g_sTestDescription.bDoTest = IMG_FALSE;
	}

	return; /* End of Main Task */
}

/*!
******************************************************************************

 @Function				ParseCommandLine

******************************************************************************/
static void ParseCommandLine(int argc, char *argv[])
{
    char *pszSelfName;
    char *pszOption;

	/*
	** Codescape or Dashscript writes the test parameters into the metag_argv array.
	** We copy them to the C standard argc and *argv[] variables.
	*/
	argv = metag_argv;

	/* Assume that all arguments are used - argc can be determined from the size of metag_argv */
	argc = (sizeof(metag_argv)/sizeof(char *));
	while ((argv[argc-1][0] == '\0') || (argv[argc-1][0] == '@'))
	{
		/*
		 * If metag_argv is "@" or "\0", then this argument is unused, so
		 * we must decrement argc as one less argument has been provided.
		 */
		argc--;
	}

	pszSelfName = *argv++;

    __TBILogF("Parsing command line - %d arguments\n", argc);

	g_sTestDescription.ui32TestNum = 1;
	g_sTestDescription.ui32BlockIndex = 0;

	g_sTestDescription.bDoTest = IMG_TRUE;

	g_sTestDescription.ui8NumMessages = 0;

	g_sTestDescription.ui32RXToneTolerance = 500;
	g_sTestDescription.ui32RXMinDetections = 9;
	g_sTestDescription.ui32RXChunkWidth = 12;
	g_sTestDescription.ui32RXPostMesageSilence = 0x84;
	g_sTestDescription.ui32SlaveResponseTimeout = 0xCE4;

	g_sTestDescription.ui32TXShort = 11;
	g_sTestDescription.ui32TXLong = 22;
	g_sTestDescription.ui32MasterEmptyTimeout = 0x84;
	g_sTestDescription.ui32PostTransactionSilence = 0x14A;

	g_sTestDescription.bOverrideFreq = IMG_FALSE;
	g_sTestDescription.ui32TargetFreq = 0;
	g_sTestDescription.Receive = DISEQC_RECEIVE_ALWAYS;
	g_sTestDescription.Tone = DISEQC_TONE_SQUARE_WAVE;
	g_sTestDescription.InputPolarity = DISEQC_POLARITY_HIGH;
	g_sTestDescription.OutputPolarity = DISEQC_POLARITY_HIGH;
	g_sTestDescription.ui32Iterations = 1;
	g_sTestDescription.Loopback = DISEQC_LOOPBACK_OFF;

    while (argc>1)
    {
        if (*argv[0]=='-')
        {
            pszOption = (*argv);

			if(strncmp(pszOption,"-test",5)==0)
            {
                argv++;
                argc--;
                sscanf(*argv,"%u",&g_sTestDescription.ui32TestNum);
				if (g_sTestDescription.ui32TestNum>NUM_TESTS)
				{
					__TBILogF("ERROR: incorrect test number. Should be from 1 to %u\n", NUM_TESTS);
					Usage(pszSelfName);
				}
                __TBILogF("Test number %u\n",g_sTestDescription.ui32TestNum);
            }
			else if(strncmp(pszOption,"-block",6)==0)
            {
                argv++;
                argc--;
                sscanf(*argv,"%u",&g_sTestDescription.ui32BlockIndex);
				if (g_sTestDescription.ui32BlockIndex>=DISEQC_NUM_BLOCKS)
				{
					__TBILogF("ERROR: incorrect block index. Should be from 0 to %u\n", DISEQC_NUM_BLOCKS-1);
					Usage(pszSelfName);
				}
                __TBILogF("Block index: %u\n",g_sTestDescription.ui32BlockIndex);
            }
			else if(strncmp(pszOption,"-wait",5)==0)
			{
				g_sTestDescription.bDoTest = IMG_FALSE;
                __TBILogF("Wait\n");
            }

			else if(strncmp(pszOption,"-rxtonetolerance",16)==0)
            {
                argv++;
                argc--;
				sscanf(*argv,"%u",&g_sTestDescription.ui32RXToneTolerance);
				if (g_sTestDescription.ui32RXToneTolerance>1000)
				{
					__TBILogF("ERROR: incorrect RX tone tolerance. Should be from 0 to 1000\n");
					Usage(pszSelfName);
				}
                __TBILogF("RX tone tolerance: %u\n",g_sTestDescription.ui32RXToneTolerance);
            }
			else if(strncmp(pszOption,"-rxmindetections",16)==0)
            {
                argv++;
                argc--;
				sscanf(*argv,"%u",&g_sTestDescription.ui32RXMinDetections);
				if (g_sTestDescription.ui32RXMinDetections>=256)
				{
					__TBILogF("ERROR: incorrect RX min detections. Should be from 0 to 255\n");
					Usage(pszSelfName);
				}
                __TBILogF("RX min detections: %u\n",g_sTestDescription.ui32RXMinDetections);
            }
			else if(strncmp(pszOption,"-rxchunkwidth",13)==0)
            {
                argv++;
                argc--;
				sscanf(*argv,"%u",&g_sTestDescription.ui32RXChunkWidth);
				if (g_sTestDescription.ui32RXChunkWidth>=256)
				{
					__TBILogF("ERROR: incorrect RX chunk width. Should be from 0 to 255\n");
					Usage(pszSelfName);
				}
                __TBILogF("RX chunk width: %u\n",g_sTestDescription.ui32RXChunkWidth);
            }
			else if(strncmp(pszOption,"-rxpostmessagesilence",21)==0)
            {
                argv++;
                argc--;
				sscanf(*argv,"%u",&g_sTestDescription.ui32RXPostMesageSilence);
				if (g_sTestDescription.ui32RXPostMesageSilence>=256)
				{
					__TBILogF("ERROR: incorrect RX post message silence. Should be from 0 to 255\n");
					Usage(pszSelfName);
				}
                __TBILogF("RX post message silence: %u\n",g_sTestDescription.ui32RXPostMesageSilence);
            }
			else if(strncmp(pszOption,"-slaveresponsetimeout",21)==0)
            {
                argv++;
                argc--;
				sscanf(*argv,"%u",&g_sTestDescription.ui32SlaveResponseTimeout);
				if (g_sTestDescription.ui32SlaveResponseTimeout>=65536)
				{
					__TBILogF("ERROR: incorrect slave response timeout. Should be from 0 to 65536\n");
					Usage(pszSelfName);
				}
                __TBILogF("Slave response timeout: %u\n",g_sTestDescription.ui32SlaveResponseTimeout);
            }

			else if(strncmp(pszOption,"-txshortchunkwidth",18)==0)
            {
                argv++;
                argc--;
				sscanf(*argv,"%u",&g_sTestDescription.ui32TXShort);
				if (g_sTestDescription.ui32TXShort>=256)
				{
					__TBILogF("ERROR: incorrect TX short chunk width. Should be from 0 to 255\n");
					Usage(pszSelfName);
				}
                __TBILogF("TX short chunk width: %u\n",g_sTestDescription.ui32TXShort);
            }
			else if(strncmp(pszOption,"-txlongchunkwidth",17)==0)
            {
                argv++;
                argc--;
				sscanf(*argv,"%u",&g_sTestDescription.ui32TXLong);
				if (g_sTestDescription.ui32TXLong>=256)
				{
					__TBILogF("ERROR: incorrect TX long chunk width. Should be from 0 to 255\n");
					Usage(pszSelfName);
				}
                __TBILogF("TX long chunk width: %u\n",g_sTestDescription.ui32TXLong);
            }
			else if(strncmp(pszOption,"-masteremptytimeout",19)==0)
            {
                argv++;
                argc--;
				sscanf(*argv,"%u",&g_sTestDescription.ui32MasterEmptyTimeout);
				if (g_sTestDescription.ui32MasterEmptyTimeout>=65536)
				{
					__TBILogF("ERROR: incorrect master empty timeout. Should be from 0 to 65536\n");
					Usage(pszSelfName);
				}
                __TBILogF("Master empty timeout: %u\n",g_sTestDescription.ui32MasterEmptyTimeout);
            }
			else if(strncmp(pszOption,"-posttransactionsilence",23)==0)
            {
                argv++;
                argc--;
				sscanf(*argv,"%u",&g_sTestDescription.ui32PostTransactionSilence);
				if (g_sTestDescription.ui32PostTransactionSilence>=65536)
				{
					__TBILogF("ERROR: incorrect post transaction silence. Should be from 0 to 65536\n");
					Usage(pszSelfName);
				}
                __TBILogF("Post transaction silence: %u\n",g_sTestDescription.ui32PostTransactionSilence);
            }
			else if(strncmp(pszOption,"-targetfreq",11)==0)
            {
                argv++;
                argc--;
				sscanf(*argv,"%u",&g_sTestDescription.ui32TargetFreq);
				g_sTestDescription.bOverrideFreq = IMG_TRUE;
                __TBILogF("Target frequency: %u\n",g_sTestDescription.ui32TargetFreq);
            }
			else if(strncmp(pszOption,"-receiveexpected",16)==0)
            {
				g_sTestDescription.Receive = DISEQC_RECEIVE_EXPECTED;
                __TBILogF("Receive when expected only\n");
            }
			else if(strncmp(pszOption,"-tonelogic",13)==0)
            {
				g_sTestDescription.Tone = DISEQC_TONE_LOGIC;
                __TBILogF("Tone logic\n");
            }
			else if(strncmp(pszOption,"-inputpolaritylow",17)==0)
            {
				g_sTestDescription.InputPolarity = DISEQC_POLARITY_LOW;
                __TBILogF("Low input polarity\n");
            }
			else if(strncmp(pszOption,"-outputpolaritylow",18)==0)
            {
				g_sTestDescription.OutputPolarity = DISEQC_POLARITY_LOW;
                __TBILogF("Low output polarity\n");
            }
			else if(strncmp(pszOption,"-loopback",9)==0)
            {
				g_sTestDescription.Loopback = DISEQC_LOOPBACK_ON;
                __TBILogF("Loopback\n");
            }
			else if(strncmp(pszOption,"-messages",9)==0)
            {
                argv++;
                argc--;

				while((argc>1)&&((*argv)[0]!='-'))
				{

					IMG_UINT32 ui32Size = 0;
					IMG_CHAR ui8C;
					IMG_CHAR *pui8SrcPtr = (*argv);
					IMG_BYTE *pui8DstPtr = (IMG_BYTE *)g_sTestDescription.sFiles[g_sTestDescription.ui8NumMessages].aui8Data;

					while(*pui8SrcPtr)
					{
						if(ui32Size==255){ui32Size=256;break;}

						ui8C = *pui8SrcPtr++;
						if((ui8C>='0')&&(ui8C<='9')){*pui8DstPtr = (ui8C-'0')<<4;}
						else if((ui8C>='a')&&(ui8C<='f')){*pui8DstPtr = (10+(ui8C-'a'))<<4;}
						else if((ui8C>='A')&&(ui8C<='F')){*pui8DstPtr = (10+(ui8C-'A'))<<4;}
						else
						{
							__TBILogF("ERROR: Bad message format: %s\n",(*argv));
							Usage(pszSelfName);
						}

						ui8C = *pui8SrcPtr++;
						if((ui8C>='0')&&(ui8C<='9')){*pui8DstPtr |= (ui8C-'0');}
						else if((ui8C>='a')&&(ui8C<='f')){*pui8DstPtr |= (10+(ui8C-'a'));}
						else if((ui8C>='A')&&(ui8C<='F')){*pui8DstPtr |= (10+(ui8C-'A'));}
						else
						{
							__TBILogF("ERROR: Bad message format: %s\n",(*argv));
							Usage(pszSelfName);
						}
						ui32Size++;
						pui8DstPtr++;
					}
					g_sTestDescription.sFiles[g_sTestDescription.ui8NumMessages].ui32Size = ui32Size;

					if(g_sTestDescription.sFiles[g_sTestDescription.ui8NumMessages].ui32Size<1)
					{
						__TBILogF("ERROR: Message too small (1 byte min): %s\n",(*argv));
						Usage(pszSelfName);
					}
					if(g_sTestDescription.sFiles[g_sTestDescription.ui8NumMessages].ui32Size>255)
					{
						__TBILogF("ERROR: Message too large (255 bytes max): %s\n",(*argv));
						Usage(pszSelfName);
					}

					__TBILogF("Message %u: %s\n",(IMG_UINT32)g_sTestDescription.ui8NumMessages,(*argv));

					g_sTestDescription.ui8NumMessages++;

					argv++;
					argc--;
				}
				continue;
            }
			else if(strncmp(pszOption,"-messagesf",10)==0)
            {
                argv++;
                argc--;

				while((argc>1)&&((*argv)[0]!='-'))
				{
					__TBILogF("Message %u: %s\n",(IMG_UINT32)g_sTestDescription.ui8NumMessages,(*argv));

					struct stat s;
					stat((*argv),&s);
					g_sTestDescription.sFiles[g_sTestDescription.ui8NumMessages].ui32Size = s.st_size;
					g_sTestDescription.sFiles[g_sTestDescription.ui8NumMessages].fd = open((*argv),O_RDONLY,0744);

					if(g_sTestDescription.sFiles[g_sTestDescription.ui8NumMessages].fd==-1)
					{
						__TBILogF("ERROR: Cannot open file: %s\n",(*argv));
						Usage(pszSelfName);
					}
					if(g_sTestDescription.sFiles[g_sTestDescription.ui8NumMessages].ui32Size<1)
					{
						__TBILogF("ERROR: File too small (1 byte min): %s\n",(*argv));
						Usage(pszSelfName);
					}
					if(g_sTestDescription.sFiles[g_sTestDescription.ui8NumMessages].ui32Size>255)
					{
						__TBILogF("ERROR: File too large (255 bytes max): %s\n",(*argv));
						Usage(pszSelfName);
					}

					read(g_sTestDescription.sFiles[g_sTestDescription.ui8NumMessages].fd,(IMG_BYTE *)g_sTestDescription.sFiles[g_sTestDescription.ui8NumMessages].aui8Data,g_sTestDescription.sFiles[g_sTestDescription.ui8NumMessages].ui32Size);
					close(g_sTestDescription.sFiles[g_sTestDescription.ui8NumMessages].fd);

					g_sTestDescription.ui8NumMessages++;

					argv++;
					argc--;
				}
				continue;
            }
			else if(strncmp(pszOption,"-iter",5)==0)
            {
                argv++;
                argc--;
                sscanf(*argv,"%u",&g_sTestDescription.ui32Iterations);
                __TBILogF("Iterations: %u\n",g_sTestDescription.ui32Iterations);
				if(g_sTestDescription.ui32Iterations==0){g_sTestDescription.ui32Iterations=ITER_INF;}
            }
            else
            {
                Usage(pszSelfName);
            }
        }
		else
		{
			Usage(pszSelfName);
		}
        argv++;
        argc--;
    }
}

/*!
******************************************************************************

 @Function				Usage

******************************************************************************/
static void Usage(char *cmd)
{
    __TBILogF("\nUsage:  %s <..options..>\n", cmd);
    __TBILogF("          -test N					Test Number (Default 1)\n");
	__TBILogF("          -block N					Block Index (Default 0)\n");
	__TBILogF("          -wait						Wait (modify variables to perform test) (Default No Wait)\n");
	__TBILogF("          -rxtonetolerance N			Tone Tolerance (0-1000) (Default 600)\n");
	__TBILogF("          -rxmindetections N			Min Detections Per Chunk (Default 9)\n");
	__TBILogF("          -rxchunkwidth N			Receive Chunk Width (Default 11 (0.5ms))\n");
	__TBILogF("          -rxpostmessagesilence N	Receive Post Message Silence (Default 0x84 (6ms))\n");
	__TBILogF("          -slaveresponsetimeout N	Slave Response Timeout (Default 0xCE4 (150ms))\n");
	__TBILogF("          -txshortchunkwidth N		Send Short Chunk Width (Default 11 (0.5ms))\n");
	__TBILogF("          -txlongchunkwidth N		Send Long Chunk Width (Default 22 (1ms))\n");
	__TBILogF("          -masteremptytimeout N		Master Empty Timeout (Default 0x84 (6ms))\n");
	__TBILogF("          -posttransactionsilence N	Post Transaction Silence (Default 0x14A (15ms))\n");
	__TBILogF("          -targetfreq N				Target Frequency Q12.20 mhz (Default 0x5A1D (22khz))\n");
	__TBILogF("          -receiveexpected			Receive Only When Data Is Expected (Default Always Receive)\n");
	__TBILogF("          -tonelogic					Input/Output Is Polarity (Default Input/Output Is Tone Wave)\n");
	__TBILogF("          -inputpolaritylow 			Input Polarity Low (Default Input Polarity High)\n");
	__TBILogF("          -outputpolaritylow			Output Polarity Low (Default Output Polarity High)\n");
	__TBILogF("          -loopback					Loopback On (Default Off)\n");
	__TBILogF("          -messages X ...			Messages (Variable Number Of Messages In Hex eg -messages E00000 E03168)\n");
	__TBILogF("          -iter N					Iterations For Test (Default 0xFFFFFFFF (Infinite))");

    exit(-1);
}


/*!
******************************************************************************

 @Function				RunTest

******************************************************************************/
static img_void RunTest()
{
	/* Reset the Kernel */
    KRN_reset(&g_sMeOSScheduler,g_asSchedQueues,MEOS_MAX_PRIORITY_LEVEL,STACK_FILL,IMG_NULL,0);

	/* Reset QIO */
    QIO_reset(&g_sQio,g_aVectoredDevTable,QIO_NO_VECTORS,&QIO_MTPIVDesc,TBID_SIGNUM_TR2(__TBIThreadId() >> TBID_THREAD_S),IMG_NULL,0);

	/* Start Meos Main Task - no background task if MeOS Abstraction Layer */
	g_psBackgroundTask = KRN_startOS("Background Task");

	/* Start Meos Main Timer Task...*/
	g_psTimerTask = KRN_startTimerTask("Timer Task", g_aui32TimerStack, TIM_STACK_SIZE, TIM_TICK_PERIOD);

	DQ_init( &g_sTaskQueue );

    /* Start the main task...*/
    KRN_startTask(MainTask, &g_sMainTask, g_aui32TaskStack, STACK_SIZE, KRN_LOWEST_PRIORITY+1, IMG_NULL, "MainTask");

	return;
}

/*!
******************************************************************************

 @Function				main

******************************************************************************/
#if defined (__META_MEOS__)

int main(int argc, char **argv)
{
	KRN_TASKQ_T	HibernateQueue;

	ParseCommandLine(argc,argv);

	RunTest();

	DQ_init(&HibernateQueue);
	KRN_hibernate(&HibernateQueue, KRN_INFWAIT);
    return 1;
}

#else

#error CPU and OS not recognised

#endif
