/*!
******************************************************************************
 @file   : gpio_test.c

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
         GPIO Test Application.

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

#include <gpio_api.h>

/*============================================================================
====	D E F I N E S
=============================================================================*/

/* Main task stack */
#define STACK_SIZE			(8092)

#define STACK_FILL			(0xDEADBEEF)

/* # GPIO blocks */
#define	NUM_GPIO_BLOCKS		(3)

/*
** VERBOSE_LOGGING definition used to restrict logging output
*/
#define	VERBOSE_LOGGING
#ifdef VERBOSE_LOGGING
#define VerboseLog __TBILogF
#else
void DummyLog() {}
#define VerboseLog DummyLog
#endif

/*============================================================================
====	E N U M S
=============================================================================*/


typedef enum
{
	GPIO_TEST_AUTO = 1,						/* 1 - Automated test of all functions. Pin A and Pin B must be connected together. No interaction is required for this test. */
	GPIO_TEST_OUTPUT_LOW,					/* 2 - Low output on first pin */
	GPIO_TEST_OUTPUT_HIGH,					/* 3 - High output on first pin */
	GPIO_TEST_OUTPUT_OSCILLATE,				/* 4 - Oscillate between low and high output on the first pin */
	GPIO_TEST_OUTPUT_OSCILLATE_REMOVE,		/* 5 - Oscillate between low and high output on the first pin with removing and re-adding the pin in-between */
	GPIO_TEST_FOLLOW_POLL,					/* 6 - Second pins output will follow the input of the first pin (using polling) */
	GPIO_TEST_FOLLOW_INTERRUPT,				/* 7 - Second pins output will follow the input of the first pin (using interrupts that are reconfigured after each interrupt) */
	GPIO_TEST_FOLLOW_INTERRUPT_TRANSITION,	/* 8 - Second pins output will follow the input of the first pin (using the dual edge interrupt) */
	GPIO_TEST_DISABLE_INTERRUPT,			/* 9 - Second pins output will follow the input of the first pin when the input of the third pin is high (using the dual edge interrupt) */
	NUM_TESTS_PLUS_ONE,						/* 10 */
} TEST_eTestMode;

#define NUM_TESTS (NUM_TESTS_PLUS_ONE - 1)

/*============================================================================
====	T Y P E D E F S
=============================================================================*/

typedef struct test_desc_tag
{
	img_uint32			ui32TestNum;
	img_uint32			ui32BlockIndex;
	img_uint32			ui32PinNumber;
	img_uint32			ui32BlockIndexB;
	img_uint32			ui32PinNumberB;
	img_uint32			ui32BlockIndexC;
	img_uint32			ui32PinNumberC;
} TEST_sTestDesc;

/*============================================================================
====	D A T A
=============================================================================*/

/* Parameter data strings (initialised to '\0' to enable simple calculation of argc) */
char argv1[32] = {'\0'};
char argv2[32] = {'\0'};
char argv3[32] = {'\0'};
char argv4[32] = {'\0'};
char argv5[32] = {'\0'};
char argv6[32] = {'\0'};
char argv7[32] = {'\0'};
char argv8[32] = {'\0'};
char argv9[32] = {'\0'};
char argv10[32] = {'\0'};
char argv11[32] = {'\0'};
char argv12[32] = {'\0'};
char argv13[32] = {'\0'};
char argv14[32] = {'\0'};
char argv15[32] = {'\0'};
char argv16[32] = {'\0'};

/* Array of arguments - the first element is unused, the other elements point to the argvXX[]
   strings that are initialised by script */
char *metag_argv[] = { "dummy", argv1, argv2, argv3, argv4, argv5, argv6, argv7, argv8, argv9, argv10,
						argv11, argv12, argv13, argv14, argv15, argv16};


/* Background Task Control Block */
KRN_TASK_T		*g_psBackgroundTask;

/* Timer Task Control Block */
KRN_TASK_T		*g_psTimerTask;

/* Stack for the Timer Task */
IMG_UINT32		g_aui32TimerStack[TIM_STACK_SIZE];

/* Main Task Control Block */
KRN_TASK_T		g_sMainTask;

/* Stack for the Main Task */
IMG_UINT32		g_aui32TaskStack[STACK_SIZE];

/* MEOS Scheduler */
KRN_SCHEDULE_T	g_sMeOSScheduler;

/* Scheduler queues */
KRN_TASKQ_T		g_asSchedQueues[MEOS_MAX_PRIORITY_LEVEL+1];

/* QIO structures */
QIO_DEVENTRY_T	g_aVectoredDevTable[QIO_NO_VECTORS];
QIO_SYS_T		g_sQio;

DQ_T			g_sTaskQueue;

/* Test structure */
static TEST_sTestDesc	g_sTestDescription;

/******************************************************************************
	Internal function prototypes
 ******************************************************************************/
static IMG_VOID ParseCommandLine(IMG_INT iArgc, IMG_CHAR *pszArgv[]);
static IMG_VOID Usage(IMG_CHAR *pszSelfName);
void MainTask();

/* Structure used for interrupt callbacks */
typedef struct GPIO_TEST_PINS
{
	GPIO_PIN_T *psInputFollowPin;
	GPIO_PIN_T *psOutputPin;
	GPIO_PIN_T *psInputInterruptEnablePin;
}GPIO_TEST_PINS;

/* Test the return values of API calls and output an appropriate error with file
   and line number if required */
#define	GPIO_API_TEST(API)															\
	if((Status = API) != GPIO_STATUS_OK)											\
	{																				\
		__TBILogF("ERROR: %s %s line %u\n",GetErrorText(Status),__FILE__,__LINE__);	\
		exit(-1);																	\
	}

/* Globals used by interrupt callbacks */
GPIO_LEVEL_T		g_GPIOTestLevel;
KRN_SEMAPHORE_T		g_GPIOTestSema;

/* Error text for all GPIO status */
IMG_CHAR *GetErrorText(GPIO_STATUS_T Status)
{
	switch(Status)
	{
	case GPIO_STATUS_OK:
		return "NO ERROR";

	case GPIO_STATUS_ERR_INITIALISED:
		return "GPIO ALREADY INITIALISED";

	case GPIO_STATUS_ERR_UNINITIALISED:
		return "GPIO NOT INITIALISED";

	case GPIO_STATUS_ERR_ACTIVE:
		return "PIN ALREADY ACTIVE";

	case GPIO_STATUS_ERR_ACTIVEPINS:
		return "ACTIVE PINS, CANNOT DEINITIALISE";

	case GPIO_STATUS_ERR_INACTIVE:
		return "PIN INACTIVE";

	case GPIO_STATUS_ERR_REPEAT:
		return "PIN ALREADY REGISTERED";

	case GPIO_STATUS_INVALID_BLOCK:
		return "INVALID BLOCK INDEX";

	case GPIO_STATUS_INVALID_PIN:
		return "INVALID PIN NUMBER";

	case GPIO_STATUS_INVALID_DIRECTION:
		return "INVALID DIRECTION";

	case GPIO_STATUS_INVALID_LEVEL:
		return "INVALID LEVEL";

	case GPIO_STATUS_INVALID_PULLUP_PULLDOWN:
		return "INVALID PULLUP/PULLDOWN";

	case GPIO_STATUS_INVALID_DRIVE_STRENGTH:
		return "INVALID DRIVE STRENGTH";

	case GPIO_STATUS_INVALID_SLEW_RATE:
		return "INVALID SLEW RATE";

	case GPIO_STATUS_INVALID_SCHMITT:
		return "INVALID SCHMITT";

	case GPIO_STATUS_INVALID_INTERRUPT_TYPE:
		return "INVALID INTERRUPT TYPE";
	}
	return "UNKNOWN";
}

/* Manual polarity switching outside the GPIO API, update the output pin value to
   the level that caused the interrupt */
IMG_VOID InterruptCallback(IMG_PVOID pContext, GPIO_LEVEL_T Level)
{
	GPIO_INTERRUPT_TYPE_T InterruptType;
	GPIO_TEST_PINS *psPins = (GPIO_TEST_PINS *)pContext;
	GPIO_STATUS_T Status;

	if(Level==GPIO_LEVEL_LOW)
	{
		InterruptType = GPIO_INT_TYPE_LEVEL_HIGH;
	}
	else
	{
		InterruptType = GPIO_INT_TYPE_LEVEL_LOW;
	}

	GPIO_API_TEST(GPIOEnableInterrupt(psPins->psInputFollowPin,InterruptType,InterruptCallback,psPins));

	GPIO_API_TEST(GPIOSet(psPins->psOutputPin,Level));
}

/* Update the output pin value to the level that caused the interrupt */
IMG_VOID InterruptCallbackB(IMG_PVOID pContext, GPIO_LEVEL_T Level)
{
	GPIO_STATUS_T Status;

	GPIO_TEST_PINS *psPins = (GPIO_TEST_PINS *)pContext;

	GPIO_API_TEST(GPIOSet(psPins->psOutputPin,Level));
}

/* Control the interrupt triggering of the input pin to be followed based on
   the value of the interrupt enable pin */
IMG_VOID InterruptCallbackC(IMG_PVOID pContext, GPIO_LEVEL_T Level)
{
	GPIO_TEST_PINS *psPins = (GPIO_TEST_PINS *)pContext;
	GPIO_STATUS_T Status;

	if(Level==GPIO_LEVEL_HIGH)
	{
		GPIO_API_TEST(GPIOEnableInterrupt(psPins->psInputFollowPin,GPIO_INT_TYPE_EDGE_TRANSITION,InterruptCallbackB,psPins));
	}
	else
	{
		GPIO_API_TEST(GPIODisableInterrupt(psPins->psInputFollowPin));
	}
}

/* Update the global level and set the global semaphore. Also disable the interrupt */
IMG_VOID InterruptCallbackD(IMG_PVOID pContext, GPIO_LEVEL_T Level)
{
	GPIO_STATUS_T Status;

	g_GPIOTestLevel = Level;
	KRN_setSemaphore(&g_GPIOTestSema,1);

	GPIO_API_TEST(GPIODisableInterrupt((GPIO_PIN_T *)pContext));
}

/* Update the global level and set the global semaphore */
IMG_VOID InterruptCallbackE(IMG_PVOID pContext, GPIO_LEVEL_T Level)
{
	g_GPIOTestLevel = Level;
	KRN_setSemaphore(&g_GPIOTestSema,1);
}


/* Test to see if InterruptCallbackD/E has been called and if the expected level caused the interrupt.
   Also test the interrupt did not trigger more than once */
IMG_VOID TestInterrupt(GPIO_PIN_T *psPin, GPIO_LEVEL_T Level)
{
	if(!KRN_testSemaphore(&g_GPIOTestSema,1,10))
	{
		__TBILogF("ERROR: Expected interrupt did not occur");
		exit(-1);
	}
	if(g_GPIOTestLevel!=Level)
	{
		__TBILogF("ERROR: Expected level was not detected");
		exit(-1);
	}
	if(KRN_testSemaphore(&g_GPIOTestSema,1,10))
	{
		__TBILogF("ERROR: Unexpected interrupt");
		exit(-1);
	}
}


/* Test a pin for an expected value */
IMG_VOID TestPin(GPIO_PIN_T *psPin, GPIO_LEVEL_T Level)
{
	GPIO_LEVEL_T tLevel;
	GPIO_STATUS_T Status;

	KRN_hibernate(&g_sTaskQueue,10);

	GPIO_API_TEST(GPIORead(psPin,&tLevel));

	if(tLevel!=Level)
	{
		__TBILogF("ERROR: Expected level was not detected");
		exit(-1);
	}
}


/*!
******************************************************************************

 @Function				MainTask

******************************************************************************/

void MainTask()
{
	SYS_sConfig				sConfig;
	GPIO_PIN_T				sPin;
	GPIO_PIN_T				sPinB;
	GPIO_PIN_T				sPinC;
	GPIO_PIN_SETTINGS_T		sPinSettings;
	GPIO_PIN_SETTINGS_T		sPinSettingsB;
	GPIO_PIN_SETTINGS_T		sPinSettingsC;
	GPIO_LEVEL_T			PinValue;
	GPIO_INTERRUPT_TYPE_T	InterruptType;
	IMG_UINT32				n;
	GPIO_STATUS_T			Status;
	IMG_UINT32				ui32Temp;
	IMG_UINT32				i;
	IMG_UINT32				j;

	GPIO_TEST_PINS sTestPins;
	sTestPins.psInputFollowPin = &sPin;
	sTestPins.psOutputPin = &sPinB;
	sTestPins.psInputInterruptEnablePin = &sPinC;

	/* Configure the system */
	IMG_MEMSET(&sConfig, 0, sizeof(SYS_sConfig));
	SYS_Configure(&sConfig);

	/* Initialise, deinitialise, reinitialise */
	GPIO_API_TEST(GPIOInit());

	GPIO_API_TEST(GPIODeinit());

	GPIO_API_TEST(GPIOInit());

	/* Do the test */
	switch(g_sTestDescription.ui32TestNum)
	{
		/* Automated test of all functions. Pin A and Pin B must be connected together. No interaction is required for this test */
		case GPIO_TEST_AUTO:
		{

			__TBILogF("GPIO test auto...\n");

			for(i=0;i<2;i++)
			{
				for(n=0;n<4;n++)
				{
					/* Basic level changing and reading tests */

					sPin.bActive = IMG_FALSE;
					sPinSettings.Direction = GPIO_DIR_INPUT;
					sPinSettings.Input.PullupPulldown = GPIO_PU_PD_OFF;
					sPinSettings.Input.Schmitt = GPIO_SCHMITT_OFF;
					GPIO_API_TEST(GPIOAddPin(&sPin,g_sTestDescription.ui32BlockIndex,g_sTestDescription.ui32PinNumber,&sPinSettings));


					sPinB.bActive = IMG_FALSE;
					sPinSettingsB.Direction = GPIO_DIR_OUTPUT;
					sPinSettingsB.Output.Level = GPIO_LEVEL_LOW;
					sPinSettingsB.Output.DriveStrength = GPIO_DRIVE_2MA;
					sPinSettingsB.Output.Slew = GPIO_SLEW_FAST;
					GPIO_API_TEST(GPIOAddPin(&sPinB,g_sTestDescription.ui32BlockIndexB,g_sTestDescription.ui32PinNumberB,&sPinSettingsB));
					TestPin(&sPin,GPIO_LEVEL_LOW);


					GPIO_API_TEST(GPIOSet(&sPinB,GPIO_LEVEL_HIGH));
					TestPin(&sPin,GPIO_LEVEL_HIGH);


					sPinSettingsB.Output.Level = GPIO_LEVEL_LOW;
					GPIO_API_TEST(GPIOConfigure(&sPinB,&sPinSettingsB));
					TestPin(&sPin,GPIO_LEVEL_LOW);


					GPIO_API_TEST(GPIOConfigure(&sPinB,&sPinSettings));
					GPIO_API_TEST(GPIOConfigure(&sPin,&sPinSettingsB));
					TestPin(&sPinB,GPIO_LEVEL_LOW);


					GPIO_API_TEST(GPIOSet(&sPin,GPIO_LEVEL_HIGH));
					TestPin(&sPinB,GPIO_LEVEL_HIGH);


					GPIO_API_TEST(GPIOConfigure(&sPin,&sPinSettings));
					GPIO_API_TEST(GPIOConfigure(&sPinB,&sPinSettings));

					//sPinSettings.Input.PullupPulldown = GPIO_PU_PD_PULLDOWN;
					//GPIOConfigure(&sPinB,&sPinSettings);

					//TestPin(&sPinB,GPIO_LEVEL_LOW);
					//TestPin(&sPin,GPIO_LEVEL_LOW);

					//sPinSettings.Input.PullupPulldown = GPIO_PU_PD_PULLUP;
					//GPIOConfigure(&sPinB,&sPinSettings);

					//TestPin(&sPinB,GPIO_LEVEL_HIGH);
					//TestPin(&sPin,GPIO_LEVEL_HIGH);

					GPIO_API_TEST(GPIOConfigure(&sPinB,&sPinSettingsB));


					/* Test interruts with diasbling after each interrupt */
					KRN_initSemaphore(&g_GPIOTestSema,0);
					GPIO_API_TEST(GPIOEnableInterrupt(&sPin,GPIO_INT_TYPE_LEVEL_HIGH,InterruptCallbackD,&sPin));
					GPIO_API_TEST(GPIOSet(&sPinB,GPIO_LEVEL_HIGH));
					TestInterrupt(&sPin,GPIO_LEVEL_HIGH);


					KRN_initSemaphore(&g_GPIOTestSema,0);
					GPIO_API_TEST(GPIOEnableInterrupt(&sPin,GPIO_INT_TYPE_LEVEL_LOW,InterruptCallbackD,&sPin));
					GPIO_API_TEST(GPIOSet(&sPinB,GPIO_LEVEL_LOW));
					TestInterrupt(&sPin,GPIO_LEVEL_LOW);


					KRN_initSemaphore(&g_GPIOTestSema,0);
					GPIO_API_TEST(GPIOEnableInterrupt(&sPin,GPIO_INT_TYPE_EDGE_RISE,InterruptCallbackD,&sPin));
					GPIO_API_TEST(GPIOSet(&sPinB,GPIO_LEVEL_HIGH));
					TestInterrupt(&sPin,GPIO_LEVEL_HIGH);


					KRN_initSemaphore(&g_GPIOTestSema,0);
					GPIO_API_TEST(GPIOEnableInterrupt(&sPin,GPIO_INT_TYPE_EDGE_FALL,InterruptCallbackD,&sPin));
					GPIO_API_TEST(GPIOSet(&sPinB,GPIO_LEVEL_LOW));
					TestInterrupt(&sPin,GPIO_LEVEL_LOW);


					KRN_initSemaphore(&g_GPIOTestSema,0);
					GPIO_API_TEST(GPIOEnableInterrupt(&sPin,GPIO_INT_TYPE_EDGE_TRANSITION,InterruptCallbackD,&sPin));
					GPIO_API_TEST(GPIOSet(&sPinB,GPIO_LEVEL_HIGH));
					TestInterrupt(&sPin,GPIO_LEVEL_HIGH);


					KRN_initSemaphore(&g_GPIOTestSema,0);
					GPIO_API_TEST(GPIOEnableInterrupt(&sPin,GPIO_INT_TYPE_EDGE_TRANSITION,InterruptCallbackD,&sPin));
					GPIO_API_TEST(GPIOSet(&sPinB,GPIO_LEVEL_LOW));
					TestInterrupt(&sPin,GPIO_LEVEL_LOW);


					GPIO_API_TEST(GPIOSet(&sPinB,GPIO_LEVEL_HIGH));

					KRN_hibernate(&g_sTaskQueue,10);

					GPIO_API_TEST(GPIOSet(&sPinB,GPIO_LEVEL_LOW));

					if(KRN_testSemaphore(&g_GPIOTestSema,1,10))
					{
						__TBILogF("ERROR: Unexpected interrupt");
						exit(-1);
					}

					/* Test interrupts without disables (edge only, levels must be disabled when the callback is used, else the ISR will never end and keep calling the callback until the external level changes) */
					for(j=0;j<3;j++)
					{
						KRN_initSemaphore(&g_GPIOTestSema,0);

						switch(j)
						{
						case 0:
							GPIO_API_TEST(GPIOEnableInterrupt(&sPin,GPIO_INT_TYPE_EDGE_RISE,InterruptCallbackE,&sPin));
							break;
						case 1:
							GPIO_API_TEST(GPIOEnableInterrupt(&sPin,GPIO_INT_TYPE_EDGE_FALL,InterruptCallbackE,&sPin));
							break;
						case 2:
							GPIO_API_TEST(GPIOEnableInterrupt(&sPin,GPIO_INT_TYPE_EDGE_TRANSITION,InterruptCallbackE,&sPin));
							break;
						}

						GPIO_API_TEST(GPIOSet(&sPinB,GPIO_LEVEL_HIGH));
						switch(j)
						{
						case 0:
						case 2:
							TestInterrupt(&sPin,GPIO_LEVEL_HIGH);
							break;
						default:
							if(KRN_testSemaphore(&g_GPIOTestSema,1,10))
							{
								__TBILogF("ERROR: Unexpected interrupt");
								exit(-1);
							}
							break;
						}


						GPIO_API_TEST(GPIOSet(&sPinB,GPIO_LEVEL_LOW));
						switch(j)
						{
						case 1:
						case 2:
							TestInterrupt(&sPin,GPIO_LEVEL_LOW);
							break;
						default:
							if(KRN_testSemaphore(&g_GPIOTestSema,1,10))
							{
								__TBILogF("ERROR: Unexpected interrupt");
								exit(-1);
							}
							break;
						}

						GPIO_API_TEST(GPIOSet(&sPinB,GPIO_LEVEL_HIGH));
						switch(j)
						{
						case 0:
						case 2:
							TestInterrupt(&sPin,GPIO_LEVEL_HIGH);
							break;
						default:
							if(KRN_testSemaphore(&g_GPIOTestSema,1,10))
							{
								__TBILogF("ERROR: Unexpected interrupt");
								exit(-1);
							}
							break;
						}

						GPIO_API_TEST(GPIOSet(&sPinB,GPIO_LEVEL_LOW));
						switch(j)
						{
						case 1:
						case 2:
							TestInterrupt(&sPin,GPIO_LEVEL_LOW);
							break;
						default:
							if(KRN_testSemaphore(&g_GPIOTestSema,1,10))
							{
								__TBILogF("ERROR: Unexpected interrupt");
								exit(-1);
							}
							break;
						}
					}


					GPIO_API_TEST(GPIORemovePin(&sPin));
					GPIO_API_TEST(GPIORemovePin(&sPinB));

					ui32Temp = g_sTestDescription.ui32BlockIndex;
					g_sTestDescription.ui32BlockIndex = g_sTestDescription.ui32BlockIndexB;
					g_sTestDescription.ui32BlockIndexB = ui32Temp;

					ui32Temp = g_sTestDescription.ui32PinNumber;
					g_sTestDescription.ui32PinNumber = g_sTestDescription.ui32PinNumberB;
					g_sTestDescription.ui32PinNumberB = ui32Temp;
				}

				GPIO_API_TEST(GPIODeinit());

				GPIO_API_TEST(GPIOInit());
			}

			__TBILogF("SUCCESS!\n");
			break;

		}
		/* Low output on first pin */
		case GPIO_TEST_OUTPUT_LOW:
		{

			__TBILogF("GPIO test low...");

			sPin.bActive = IMG_FALSE;
			sPinSettings.Direction = GPIO_DIR_OUTPUT;
			sPinSettings.Output.Level = GPIO_LEVEL_LOW;
			sPinSettings.Output.DriveStrength = GPIO_DRIVE_2MA;
			sPinSettings.Output.Slew = GPIO_SLEW_FAST;

			GPIO_API_TEST(GPIOAddPin(&sPin,g_sTestDescription.ui32BlockIndex,g_sTestDescription.ui32PinNumber,&sPinSettings));

			__TBILogF("SUCCESS!\n");
			break;

		}
		/* High output on first pin */
		case GPIO_TEST_OUTPUT_HIGH:
		{

			__TBILogF("GPIO test high...");

			sPin.bActive = IMG_FALSE;
			sPinSettings.Direction = GPIO_DIR_OUTPUT;
			sPinSettings.Output.Level = GPIO_LEVEL_HIGH;
			sPinSettings.Output.DriveStrength = GPIO_DRIVE_2MA;
			sPinSettings.Output.Slew = GPIO_SLEW_FAST;

			GPIO_API_TEST(GPIOAddPin(&sPin,g_sTestDescription.ui32BlockIndex,g_sTestDescription.ui32PinNumber,&sPinSettings));

			__TBILogF("SUCCESS!\n");
			break;

		}
		/* Oscillate between low and high output on the first pin */
		case GPIO_TEST_OUTPUT_OSCILLATE:
		{

			__TBILogF("GPIO test oscillate...");

			sPin.bActive = IMG_FALSE;
			sPinSettings.Direction = GPIO_DIR_OUTPUT;
			sPinSettings.Output.Level = GPIO_LEVEL_LOW;
			sPinSettings.Output.DriveStrength = GPIO_DRIVE_2MA;
			sPinSettings.Output.Slew = GPIO_SLEW_FAST;

			GPIO_API_TEST(GPIOAddPin(&sPin,g_sTestDescription.ui32BlockIndex,g_sTestDescription.ui32PinNumber,&sPinSettings));

			while(IMG_TRUE)
			{
				GPIO_API_TEST(GPIOSet(&sPin,GPIO_LEVEL_HIGH));

				KRN_hibernate(&g_sTaskQueue,1000);

				GPIO_API_TEST(GPIOSet(&sPin,GPIO_LEVEL_LOW));

				KRN_hibernate(&g_sTaskQueue,1000);
			};

			__TBILogF("SUCCESS!\n");
			break;

		}
		/* Oscillate between low and high output on the first pin with removing and re-adding the pin in-between */
		case GPIO_TEST_OUTPUT_OSCILLATE_REMOVE:
		{

			__TBILogF("GPIO test oscillate (remove)...");

			sPin.bActive = IMG_FALSE;
			sPinSettings.Direction = GPIO_DIR_OUTPUT;
			sPinSettings.Output.Level = GPIO_LEVEL_LOW;
			sPinSettings.Output.DriveStrength = GPIO_DRIVE_2MA;
			sPinSettings.Output.Slew = GPIO_SLEW_FAST;

			GPIO_API_TEST(GPIOAddPin(&sPin,g_sTestDescription.ui32BlockIndex,g_sTestDescription.ui32PinNumber,&sPinSettings));

			PinValue = GPIO_LEVEL_HIGH;

			while(IMG_TRUE)
			{

				GPIO_API_TEST(GPIORemovePin(&sPin));

				sPinSettings.Direction = GPIO_DIR_OUTPUT;
				sPinSettings.Output.Level = PinValue;
				sPinSettings.Output.DriveStrength = GPIO_DRIVE_2MA;
				sPinSettings.Output.Slew = GPIO_SLEW_FAST;

				GPIO_API_TEST(GPIOAddPin(&sPin,g_sTestDescription.ui32BlockIndex,g_sTestDescription.ui32PinNumber,&sPinSettings));

				KRN_hibernate(&g_sTaskQueue,1000);

				if(PinValue==GPIO_LEVEL_LOW){PinValue=GPIO_LEVEL_HIGH;}
				else{PinValue=GPIO_LEVEL_LOW;}

			};

			__TBILogF("SUCCESS!\n");
			break;

		}
		/* Second pins output will follow the input of the first pin (using polling) */
		case GPIO_TEST_FOLLOW_POLL:
		{

			__TBILogF("GPIO test follow (poll)...");

			sPin.bActive = IMG_FALSE;
			sPinSettings.Direction = GPIO_DIR_INPUT;
			sPinSettings.Input.Schmitt = GPIO_SCHMITT_ON;
			sPinSettings.Input.PullupPulldown = GPIO_PU_PD_OFF;

			GPIO_API_TEST(GPIOAddPin(&sPin,g_sTestDescription.ui32BlockIndex,g_sTestDescription.ui32PinNumber,&sPinSettings));


			sPinB.bActive = IMG_FALSE;
			sPinSettingsB.Direction = GPIO_DIR_OUTPUT;
			sPinSettingsB.Output.Level = GPIO_LEVEL_LOW;
			sPinSettingsB.Output.DriveStrength = GPIO_DRIVE_2MA;
			sPinSettingsB.Output.Slew = GPIO_SLEW_FAST;

			GPIO_API_TEST(GPIOAddPin(&sPinB,g_sTestDescription.ui32BlockIndexB,g_sTestDescription.ui32PinNumberB,&sPinSettingsB));


			while(IMG_TRUE)
			{
				GPIO_API_TEST(GPIORead(&sPin,&PinValue));

				GPIO_API_TEST(GPIOSet(&sPinB,PinValue));

				KRN_hibernate(&g_sTaskQueue,10);
			};

			__TBILogF("SUCCESS!\n");
			break;

		}
		/* Second pins output will follow the input of the first pin (using interrupts that are reconfigured after each interrupt) */
		case GPIO_TEST_FOLLOW_INTERRUPT:
		{

			__TBILogF("GPIO test follow (interrupt level switching)...");

			KRN_initSemaphore(&g_GPIOTestSema,0);

			sPin.bActive = IMG_FALSE;
			sPinSettings.Direction = GPIO_DIR_INPUT;
			sPinSettings.Input.Schmitt = GPIO_SCHMITT_ON;
			sPinSettings.Input.PullupPulldown = GPIO_PU_PD_OFF;

			GPIO_API_TEST(GPIOAddPin(&sPin,g_sTestDescription.ui32BlockIndex,g_sTestDescription.ui32PinNumber,&sPinSettings));

			GPIO_API_TEST(GPIORead(&sPin,&PinValue));

			sPinB.bActive = IMG_FALSE;
			sPinSettingsB.Direction = GPIO_DIR_OUTPUT;
			sPinSettingsB.Output.Level = PinValue;
			sPinSettingsB.Output.DriveStrength = GPIO_DRIVE_2MA;
			sPinSettingsB.Output.Slew = GPIO_SLEW_FAST;

			GPIO_API_TEST(GPIOAddPin(&sPinB,g_sTestDescription.ui32BlockIndexB,g_sTestDescription.ui32PinNumberB,&sPinSettingsB));

			if(PinValue==GPIO_LEVEL_LOW){InterruptType = GPIO_INT_TYPE_LEVEL_HIGH;}
			else{InterruptType = GPIO_INT_TYPE_LEVEL_LOW;}

			GPIO_API_TEST(GPIOEnableInterrupt(&sPin,InterruptType,InterruptCallback,&sTestPins));

			while(IMG_TRUE)
			{
				KRN_hibernate(&g_sTaskQueue,10);
			};

			__TBILogF("SUCCESS!\n");
			break;

		}
		/* Second pins output will follow the input of the first pin (using the dual edge interrupt) */
		case GPIO_TEST_FOLLOW_INTERRUPT_TRANSITION:
		{

			__TBILogF("GPIO test follow (interrupt transition)...");

			KRN_initSemaphore(&g_GPIOTestSema,0);

			sPin.bActive = IMG_FALSE;
			sPinSettings.Direction = GPIO_DIR_INPUT;
			sPinSettings.Input.Schmitt = GPIO_SCHMITT_ON;
			sPinSettings.Input.PullupPulldown = GPIO_PU_PD_OFF;

			GPIO_API_TEST(GPIOAddPin(&sPin,g_sTestDescription.ui32BlockIndex,g_sTestDescription.ui32PinNumber,&sPinSettings));

			GPIO_API_TEST(GPIORead(&sPin,&PinValue));

			sPinB.bActive = IMG_FALSE;
			sPinSettingsB.Direction = GPIO_DIR_OUTPUT;
			sPinSettingsB.Output.Level = PinValue;
			sPinSettingsB.Output.DriveStrength = GPIO_DRIVE_2MA;
			sPinSettingsB.Output.Slew = GPIO_SLEW_FAST;

			GPIO_API_TEST(GPIOAddPin(&sPinB,g_sTestDescription.ui32BlockIndexB,g_sTestDescription.ui32PinNumberB,&sPinSettingsB));

			GPIO_API_TEST(GPIOEnableInterrupt(&sPin,GPIO_INT_TYPE_EDGE_TRANSITION,InterruptCallbackB,&sTestPins));

			while(IMG_TRUE)
			{
				KRN_hibernate(&g_sTaskQueue,10);
			};

			__TBILogF("SUCCESS!\n");
			break;

		}
		/* Second pins output will follow the input of the first pin when the input of the third pin is high (using the dual edge interrupt) */
		case GPIO_TEST_DISABLE_INTERRUPT:
		{

			__TBILogF("GPIO test disable interrupt...");

			KRN_initSemaphore(&g_GPIOTestSema,0);

			sPin.bActive = IMG_FALSE;
			sPinSettings.Direction = GPIO_DIR_INPUT;
			sPinSettings.Input.Schmitt = GPIO_SCHMITT_ON;
			sPinSettings.Input.PullupPulldown = GPIO_PU_PD_OFF;

			GPIO_API_TEST(GPIOAddPin(&sPin,g_sTestDescription.ui32BlockIndex,g_sTestDescription.ui32PinNumber,&sPinSettings));

			GPIO_API_TEST(GPIORead(&sPin,&PinValue));

			sPinB.bActive = IMG_FALSE;
			sPinSettingsB.Direction = GPIO_DIR_OUTPUT;
			sPinSettingsB.Output.Level = PinValue;
			sPinSettingsB.Output.DriveStrength = GPIO_DRIVE_2MA;
			sPinSettingsB.Output.Slew = GPIO_SLEW_FAST;

			GPIO_API_TEST(GPIOAddPin(&sPinB,g_sTestDescription.ui32BlockIndexB,g_sTestDescription.ui32PinNumberB,&sPinSettingsB));

			sPinC.bActive = IMG_FALSE;
			sPinSettingsC.Direction = GPIO_DIR_INPUT;
			sPinSettingsC.Input.Schmitt = GPIO_SCHMITT_ON;
			sPinSettingsC.Input.PullupPulldown = GPIO_PU_PD_OFF;

			GPIO_API_TEST(GPIOAddPin(&sPinC,g_sTestDescription.ui32BlockIndexC,g_sTestDescription.ui32PinNumberC,&sPinSettingsC));

			GPIO_API_TEST(GPIORead(&sPinC,&PinValue));

			if(PinValue==GPIO_LEVEL_HIGH)
			{
				GPIO_API_TEST(GPIOEnableInterrupt(&sPin,GPIO_INT_TYPE_EDGE_TRANSITION,InterruptCallbackB,&sTestPins));
			}

			GPIO_API_TEST(GPIOEnableInterrupt(&sPinC,GPIO_INT_TYPE_EDGE_TRANSITION,InterruptCallbackC,&sTestPins));


			while(IMG_TRUE)
			{
				KRN_hibernate(&g_sTaskQueue,10);
			};

			__TBILogF("SUCCESS!\n");
			break;

		}
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


	/* Codescape or Dashscript writes the test parameters into the metag_argv array.
	 We copy them to the C standard argc and *argv[] variables */
	argv = metag_argv;

	/* Get number of arguments provided */
	argc = (sizeof(metag_argv)/sizeof(char *));
	while(argv[argc-1][0]=='\0')
	{
		argc--;
	}

	pszSelfName = *argv++;

	/* Parse command line and update global test descriptor structure accordingly */
    VerboseLog("Parsing command line - %d arguments\n", argc);

	g_sTestDescription.ui32TestNum = 4;
	g_sTestDescription.ui32BlockIndex = 0;
	g_sTestDescription.ui32PinNumber = 4;
	g_sTestDescription.ui32BlockIndexB = 0;
	g_sTestDescription.ui32PinNumberB = 5;

    while (argc>1)
    {
        if (*argv[0]=='-')
        {
            pszOption = (*argv);

			if(strncmp(pszOption,"-test",5)==0)
            {
                argv++;
                argc--;
                sscanf(*argv,"%d",&g_sTestDescription.ui32TestNum);
				if (g_sTestDescription.ui32TestNum>NUM_TESTS)
				{
					VerboseLog("Error: incorrect test number. Should be from 1 to %d\n", NUM_TESTS);
					Usage(pszSelfName);
				}
                VerboseLog("Test number %d\n",g_sTestDescription.ui32TestNum);
            }
			else if(strncmp(pszOption,"-blockA",7)==0)
            {
                argv++;
                argc--;
                sscanf(*argv,"%d",&g_sTestDescription.ui32BlockIndex);
				if (g_sTestDescription.ui32BlockIndex>=NUM_GPIO_BLOCKS)
				{
					VerboseLog("Error: incorrect block index. Should be from 1 to %d\n", NUM_GPIO_BLOCKS-1);
					Usage(pszSelfName);
				}
                VerboseLog("Block index (A): %d\n",g_sTestDescription.ui32BlockIndex);
            }
			else if(strncmp(pszOption,"-blockB",7)==0)
            {
                argv++;
                argc--;
                sscanf(*argv,"%d",&g_sTestDescription.ui32BlockIndexB);
				if (g_sTestDescription.ui32BlockIndexB>=NUM_GPIO_BLOCKS)
				{
					VerboseLog("Error: incorrect block index. Should be from 1 to %d\n", NUM_GPIO_BLOCKS-1);
					Usage(pszSelfName);
				}
                VerboseLog("Block index (B): %d\n",g_sTestDescription.ui32BlockIndexB);
            }
			else if(strncmp(pszOption,"-blockC",7)==0)
            {
                argv++;
                argc--;
                sscanf(*argv,"%d",&g_sTestDescription.ui32BlockIndexC);
				if (g_sTestDescription.ui32BlockIndexC>=NUM_GPIO_BLOCKS)
				{
					VerboseLog("Error: incorrect block index. Should be from 1 to %d\n", NUM_GPIO_BLOCKS-1);
					Usage(pszSelfName);
				}
                VerboseLog("Block index (C): %d\n",g_sTestDescription.ui32BlockIndexC);
            }
			else if(strncmp(pszOption,"-pinA",5)==0)
            {
                argv++;
                argc--;
                sscanf(*argv,"%d",&g_sTestDescription.ui32PinNumber);
				if (g_sTestDescription.ui32PinNumber>=32)
				{
					VerboseLog("Error: incorrect pin number. Should be from 1 to 31\n");
					Usage(pszSelfName);
				}
                VerboseLog("Pin number (A) %d\n",g_sTestDescription.ui32PinNumber);
            }
			else if(strncmp(pszOption,"-pinB",5)==0)
            {
                argv++;
                argc--;
                sscanf(*argv,"%d",&g_sTestDescription.ui32PinNumberB);
				if (g_sTestDescription.ui32PinNumberB>=32)
				{
					VerboseLog("Error: incorrect pin number. Should be from 1 to 31\n");
					Usage(pszSelfName);
				}
                VerboseLog("Pin number (B) %d\n",g_sTestDescription.ui32PinNumberB);
            }
			else if(strncmp(pszOption,"-pinC",5)==0)
            {
                argv++;
                argc--;
                sscanf(*argv,"%d",&g_sTestDescription.ui32PinNumberC);
				if (g_sTestDescription.ui32PinNumberC>=32)
				{
					VerboseLog("Error: incorrect pin number. Should be from 1 to 31\n");
					Usage(pszSelfName);
				}
                VerboseLog("Pin number (C) %d\n",g_sTestDescription.ui32PinNumberC);
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
    __TBILogF("          -test N			Test number\n");
	__TBILogF("          -blockA N			Block Index (first pin)\n");
	__TBILogF("          -pinA N			Pin Number (first pin)\n");
	__TBILogF("          -blockB N			Block Index (second pin)\n");
	__TBILogF("          -pinB N			Pin Number (second pin)\n");
	__TBILogF("          -blockC N			Block Index (third pin)\n");
	__TBILogF("          -pinC N			Pin Number (third pin)\n");
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

/*volatile IMG_UINT32 g_test;

int main(int argc, char **argv)
{
	while(IMG_TRUE)
	{
		g_test = 10;
	}
}*/

#else

#error CPU and OS not recognised

#endif
