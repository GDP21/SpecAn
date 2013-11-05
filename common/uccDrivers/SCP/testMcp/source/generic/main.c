/*!
 *****************************************************************************

 @file      main.c
 @brief     SCP driver for UCC320 Test Program

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

 ****************************************************************************/

#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <assert.h>
#include <string.h>
#include <stdint.h>

#include "uccrt.h"
#include "gramBuffers.h"
#include "scpDriver.h"
#include "rcdOutput.h"
#include "dcpOutput.h"
#include "scpMcpShimDriver_DCP.h"

#include "mcp_source.h"
#include "mcp_source_family_data.h"
#define MCP_SYMBOL(symbol)  (mcp_source_##symbol(mcp))

/*------------------------------------------------------------*/
/* Define USE_PLAYOUT if the SCP is to be fed with data from an external playout system (otherwise a MeOS task feeds data to the SCP) */
#define USE_PLAYOUT
//#define PDUMP

#if defined(__SIM__) && defined(__DEBUG__)
//#define PDUMP
#endif

/*------------------------------------------------------------*/

/* MeOS constants and variables */

#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES - 1)
#define NUM_VEC_DEVICES 64
#define MEOS_TIMER_STACK_SIZE (512)
#define MEOS_STACK_INIT_VALUE (0xDEADBEEF)
#define MEOS_TICK_LENGTH (1000) /* 1000 us */
#define MEOS_SCP_FEEDER_STACK_SIZE (512)

static KRN_SCHEDULE_T schedule;
static KRN_TASKQ_T taskQueues[MAX_PRIORITY + 1];
static KRN_TASKQ_T hibernateQ;

static QIO_DEVENTRY_T deviceTable[NUM_VEC_DEVICES];

static QIO_SYS_T qioSys;

static KRN_TASK_T scpFeederTask;
static KRN_TASK_T *timerTask;

static unsigned int timerStack[MEOS_TIMER_STACK_SIZE];
static unsigned int scpFeederStack[MEOS_SCP_FEEDER_STACK_SIZE];

/*--------------------------------------------------------------------*/
/* Test constants */

#define SYMBOLS_PER_FRAME       (10)    /* Arbitrary, but can be used to ensure the symbol number is output correctly */
#define SYMBOL_LENGTH           (768)   /* Size of SCP symbols (if too small host might not be able to queue jobs quickly enough) */
#define NUM_SCP_JOBS            (4)     /* Number of jobs to capture */
#define MAX_JOB_SIZE            (1024)  /* Maximum job size */

#define NUM_TESTS               (4)
#define TEST1_CAPTURE_LENGTH    (SYMBOL_LENGTH-64)
#define TEST2_CAPTURE_LENGTH    (SYMBOL_LENGTH)
#define TEST3_CAPTURE_LENGTH    (512)
#define TEST4_CAPTURE_LENGTH    (523)

#define FILL_VALUE              (0xff)  // Memory fill value for start of test
/* Definitions of the ramp signal send to the SCP */
#define TEST_SIGNAL_START_VALUE (-256<<2)
#define TEST_SIGNAL_END_VALUE   (255<<2)
#define TEST_SIGNAL_STEP        (+1<<2)
#define TEST_SIGNAL_LENGTH      ((TEST_SIGNAL_END_VALUE - TEST_SIGNAL_START_VALUE + TEST_SIGNAL_STEP)/TEST_SIGNAL_STEP)
#define TEST_SIGNAL_WRAP_STEP   (TEST_SIGNAL_END_VALUE - TEST_SIGNAL_START_VALUE + TEST_SIGNAL_STEP)

#define MAX_MISMATCHES_TO_SHOW  (10)

/* Buffer to be filled with SCP output data (this must be located in GRAM) */
#if SCP_OUTPUT_BUFFER_LEN < 3*(NUM_SCP_JOBS * MAX_JOB_SIZE)
#error SCP_OUTPUT_BUFFER_LEN needs increasing (minimum size is NUM_SCP_JOBS * MAX_JOB_SIZE)
#endif

static unsigned totalSamplesInput = 0; /* Total number of samples input to the SCP */
/*------------------------------------------------------------*/

/* Handles */
static MCP_T *mcp;
static SCP_T *scp;

static void
initMeos()
{
    KRN_TASK_T* taskPtr;

    /* Reset system */
    DQ_init(taskQueues);
    DQ_init(&hibernateQ);
    KRN_reset(&schedule, taskQueues, MAX_PRIORITY, MEOS_STACK_INIT_VALUE, NULL,
              0);

    /* Start multi-tasking environment */
    taskPtr = KRN_startOS(NULL);

    /* Start timer task with 100us ticks */
    timerTask = KRN_startTimerTask("Timer Task", timerStack,
                                   MEOS_TIMER_STACK_SIZE, MEOS_TICK_LENGTH);

    /* Reset I/O system */
    QIO_reset(&qioSys, deviceTable, NUM_VEC_DEVICES, UCCP_ivDesc,
              TBID_SIGNUM_TR2(__TBIThreadId() >> TBID_THREAD_S), NULL, 0);
}

/*--------------------------------------------------------------------*/
/* Local function prototypes */
static bool
testScpCapture(SCP_T *scp, unsigned captureLength, unsigned discardLength, unsigned numJobs);
static bool
testCapturedData(unsigned captureLength, unsigned symbolLength,
                 unsigned numJobs);
static void
scpFeeder(void);
static void
scpOverflowHandler(SCP_T *scp, SCP_EVT_T event, void *parameter);

/*
 ** FUNCTION:    main()
 **
 ** DESCRIPTION: main entry point, platform setup etc
 **
 */
int
main(void)
{
    bool passed;
    int test, numPassed = 0;
    unsigned int captureLen[NUM_TESTS] = {
            TEST1_CAPTURE_LENGTH,
            TEST2_CAPTURE_LENGTH,
            TEST3_CAPTURE_LENGTH,
            TEST4_CAPTURE_LENGTH, };
    SCP_SRC_T scpSource;
    EDC_BUFFER_T traceBuffer;
    DCP_DEVICE_T *DCP_scpMcpShim0;
    DCP_PARAM_ID_T orFlag;
    DCP_PIPELINE_T *pipeline;
    int32_t data;
    int queueStride;

    __TBILogF("SCP capture driver unit test (built on %s).\n", __DATE__);
#ifdef USE_PLAYOUT
    __TBILogF(
              "Using playout - ensure playout is connected and playing ''rampTestPlayout.16c''\n");
    scpSource = SCP_SRC_ADC;
    (void)scpFeederStack;
    (void)scpFeeder;
    /* Write to Kuroi FPGA registers to setup playout */
#ifdef __UCCP320_0__
    *(volatile unsigned int *)0x04808400 = 0x00000201; /* Write to ADC_FIFO_CONTROL, set INPUT_MODE=1, BYPASS_MODE=1 */
    *(volatile unsigned int *)0x04808750 = 0x00008030; /* Write to OSCILLATOR_CLK_DIVIDER, set OSC_CLK=0, OSC_CLK_DIVIDER=0x30 */
    // HACK - allow us to poke the divider ratio to test different SCP output clock rates.
    //{
    //        static volatile unsigned int dbgDivRatio = 0;
    //        while (dbgDivRatio==0);
    //        *(volatile unsigned int *)0x04808750 = 0x000080200|dbgDivRatio; /* Write to OSCILLATOR_CLK_DIVIDER, set OSC_CLK=0, OSC_CLK_DIVIDER=programmable */
    //}
#else
    // Kuroi registers setup by .IMG file for Meta builds
    //    *(volatile unsigned int *)0x02008400 = 0x00000201; /* Write to ADC_FIFO_CONTROL, set INPUT_MODE=1, BYPASS_MODE=1 */
        *(volatile unsigned int *)0x02008750 = 0x00008060; /* Write to OSCILLATOR_CLK_DIVIDER, set OSC_CLK=0, OSC_CLK_DIVIDER=0x30 */
#endif
#else
    __TBILogF("Using MeOS task to provide SCP data - no playout system required.\n");
    scpSource = SCP_SRC_INREG;
#endif

#ifdef PDUMP
    assert(UCC_openPdump());
#endif

    /* Initialise MeOS */
    initMeos();
    KRN_setTimeSlice(1);

    /* Initialise and reset the UCC runtime */
    UCCP_init();
    UCCP_reset();

    /* Load the DCP image */
    DCP_load(&DCP_image_default, NULL);

    /* Configure the EDC trace buffer */
    EDC_configTraceBuffer(traceBufferSpace, TRACE_BUFFER_LEN, 1, &traceBuffer);

    /* Clear the trace buffer */
    memset(traceBuffer.addr, 0, traceBuffer.len);

    mcp = UCC_getMCP(UCCP_getUCC(1), 1);
    scp = UCC_getSCP(UCCP_getUCC(1), 1);


    /* Configure the SCP - we put the SCP into bypass mode for this test as we're only interested in the captured data */
    SCP_reset(scp);
    SCP_configure(scp, scpSource, SCP_DST_TBUS, false, true, false, false,
                  SCP_MODE_BYPASS, SCP_FRAME_A, SCP_PWR_ON, SCP_PWR_ON, false,
                  1, 1, false, false);
    SCP_setFrameSize(scp, SCP_FRAME_A, SYMBOLS_PER_FRAME, SYMBOL_LENGTH,
                     SYMBOL_LENGTH);
    SCP_installEventHandler(scp, scpOverflowHandler, SCP_EVT_IOVF, NULL, true);
    SCP_installEventHandler(scp, scpOverflowHandler, SCP_EVT_TBUS_OVF, NULL, true);

    /* Configure SCP capture device */
    pipeline = DCP_getImagePipeline(DCP_pipelineId_scpPipeline0);
    SCP_configureUse(pipeline, DCP_scpPipeline_scp, &RCD_scpDriver_default_images, scp, SYMBOL_LENGTH);

    /* Configure the MCP */
    MCP_reset(mcp);
    /* set up MCP mapping to GRAM */
    MCP_mapMemoryToGRAM(mcp, 0x8000, 0x8000, 0x9000);
    /* Load MCP program */
    MCP_loadImage(mcp, imageFamilyData, mcp_source_IMAGE_ID, false);

    /* Patch the OR flag */
    DCP_scpMcpShim0 = DCP_getImageDevice(DCP_deviceId_scpShimDevice);
    orFlag = DCP_getDeviceParam(DCP_scpMcpShim0, DCP_scpMcpShimDriver_scpOr);
    data = orFlag;
    MCP_write32int(mcp, MCP_SYMBOL(scpOrFlag), &data, 1);

    /*
    ** Set MCP WAIT to non-legacy mode (wait on EFS). Note, this is also setting the orFlag for the
    ** MCP WAIT, even though this is being done again in MCP code.
    */
    MCP_configWaitSource(mcp, MCP_EVT_SRC_EFS, orFlag);


    /* Patch queue and flag IDs down to MCP code */
    queueStride = MCP_SYMBOL(QM_PERIP_TAIL_1) - MCP_SYMBOL(QM_PERIP_TAIL_0);
    data = MCP_SYMBOL(QM_PERIP_TAIL_0) + queueStride*scp->driver.inCaptureLenQId;
    MCP_write32int(mcp, MCP_SYMBOL(scpDrvInputCapLenQ), &data, 1);
    data = MCP_SYMBOL(QM_PERIP_TAIL_0) + queueStride*scp->driver.inDiscardLenQId;
    MCP_write32int(mcp, MCP_SYMBOL(scpDrvInputDiscLenQ),  &data, 1);
    data = MCP_SYMBOL(QM_PERIP_TAIL_0) + queueStride*scp->driver.inAddrQId;
    MCP_write32int(mcp, MCP_SYMBOL(scpDrvInputAddressQ), &data, 1);

    queueStride = MCP_SYMBOL(QM_PERIP_HEAD_1) - MCP_SYMBOL(QM_PERIP_HEAD_0);
    data = MCP_SYMBOL(QM_PERIP_HEAD_0) + queueStride*scp->driver.outCaptureLenQId;
    MCP_write32int(mcp, MCP_SYMBOL(scpDrvOutputCapLenQ), &data, 1);
    data = MCP_SYMBOL(QM_PERIP_HEAD_0) + queueStride*scp->driver.outDiscardLenQId;
    MCP_write32int(mcp, MCP_SYMBOL(scpDrvOutputDiscLenQ), &data, 1);
    data = MCP_SYMBOL(QM_PERIP_HEAD_0) + queueStride*scp->driver.outAddrQId;
    MCP_write32int(mcp, MCP_SYMBOL(scpDrvOutputAddressQ), &data, 1);
    data = MCP_SYMBOL(QM_PERIP_HEAD_0) + queueStride*scp->driver.outIscrQId;
    MCP_write32int(mcp, MCP_SYMBOL(scpDrvOutputIscrQ), &data, 1);

    /* Start DCP code */
    DCP_startAllDevices();

    /* Start the SCP driver */
    DCP_startJob(pipeline, 0);

    /* Start the SCP sample feeder job if we're not using playout */
#ifndef USE_PLAYOUT
    KRN_priority(NULL, KRN_LOWEST_PRIORITY); // Main task needs to be lower priority than the feeder task.
    KRN_startTask(&scpFeeder, &scpFeederTask, scpFeederStack,
            MEOS_SCP_FEEDER_STACK_SIZE, KRN_LOWEST_PRIORITY+1, scp, "SCP Feeder Task");
#endif

    /*
     * Run tests
     */
    for (test = 0; test < NUM_TESTS; test++)
    {
        unsigned thisCaptureLen, thisDiscardLen;

        thisCaptureLen = captureLen[test];
        thisDiscardLen = SYMBOL_LENGTH - thisCaptureLen;

        __TBILogF("Test %d: (captureLength=%d, discardLength=%d, symbolLength=%d, jobs=%d)\n",
                  test + 1, thisCaptureLen, thisDiscardLen, SYMBOL_LENGTH, NUM_SCP_JOBS);
        passed = testScpCapture(scp, thisCaptureLen, thisDiscardLen, NUM_SCP_JOBS);
        __TBILogF("    Test %d %s\n", test + 1, passed ? "Passed" : "Failed");
        numPassed += passed ? 1 : 0;
    }

    __TBILogF("Summary: %d of %d passed\n", numPassed, NUM_TESTS);

    SCP_installEventHandler(scp, NULL, SCP_EVT_IOVF, NULL, true);
    SCP_installEventHandler(scp, NULL, SCP_EVT_TBUS_OVF, NULL, true);
    DCP_stopAllDevices();

#ifdef PDUMP
    UCC_closePdump();
#endif

    /* Stop MeOS */
    KRN_removeTask(timerTask);

    // TEMP: Non-zero return causes Sonic crash
    //return (numPassed == NUM_TESTS ? 0 : 1);
    return 0;
}

/*
 ** FUNCTION:    scpFeeder()
 **
 ** DESCRIPTION: SCP Feeder task. Generates ramp input signal and feeds samples to the SCP
 **
 ** PARAMETERS:  void - retrieves SCP pointer from KRN_taskParameter()
 **
 ** RETURNS:     void - never returns
 */
static volatile unsigned scpFeederHibInterval = 32; /* Number of samples to post before hibernating */
static void
scpFeeder(void)
{
    int32_t sample = TEST_SIGNAL_START_VALUE;
    SCP_T *scp;

    scp = KRN_taskParameter(&scpFeederTask);

    while (true)
    {
        /* Write sample to SCP */
        SCP_writeSample(scp, sample, sample);
        totalSamplesInput++;

        /* Work out next sample value */
        sample += TEST_SIGNAL_STEP;
        if (sample > TEST_SIGNAL_END_VALUE)
        {
            sample = TEST_SIGNAL_START_VALUE;
        }

        /* De-schedule to slow the rate of input samples and allow the main task to execute*/
        if (totalSamplesInput % scpFeederHibInterval == 0)
        {
            KRN_hibernate(&hibernateQ, 1);
        }
    }
}

/*
 ** FUNCTION:    scpOverflowHandler()
 **
 ** DESCRIPTION: ISR hooked to SCP overflow event - this shouldn't trigger, but we need to be aware if it does.
 **
 ** PARAMETERS:  void - none
 **
 ** RETURNS:     void
 */
static void
scpOverflowHandler(SCP_T *scp, SCP_EVT_T event, void *parameter)
{
    (void)scp;
    (void)parameter;

    if (event == SCP_EVT_TBUS_OVF)
    {
        __TBILogF("ERROR: SCP TBUS overflow occurred.\n");
    }
    else if (event == SCP_EVT_IOVF)
    {
        __TBILogF("ERROR: SCP input synchroniser overflow occurred.\n");
    }
    else
    {
        __TBILogF("ERROR: unknown SCP event occurred.\n");
    }
#ifndef USE_PLAYOUT
    __TBILogF("       %d samples fed by SCP feeder task.\n", totalSamplesInput);
#endif

    DCP_stopAllDevices();
    assert(0);
}

/*
 ** FUNCTION:    testScpCapture()
 **
 ** DESCRIPTION: Test the SCP capture driver for the specified jobs size
 **
 ** PARAMETERS:  captureLength - Number of samples to capture per job
 **              discardLength - Number of samples to discard per job
 **              numJobs - number of jobs to run.
 **
 ** RETURNS:     TRUE - is test passed, FALSE if it failed.
 */
static bool
testScpCapture(SCP_T *scp, unsigned captureLength, unsigned discardLength, unsigned numJobs)
{
    EDC_BUFFER_T outputBuffer;
    bool passed;
    uint32_t outputPtr;
    int32_t data;

    (void) scp;

    /* Obtain GRAM address in complex region for SCP output buffer */
    EDC_alignBuffer(scpOutputBufferSpace, SCP_OUTPUT_BUFFER_LEN, EDC_GRAM, 0,
                    &outputBuffer);

    /* clear the output buffer */
    memset(outputBuffer.addr, FILL_VALUE, numJobs * captureLength * 3);
    outputPtr = outputBuffer.wordOffset;    /* GRAM locations from start of GRAM (not MCP view) */

    /* Patch parameters for job A */
    data = captureLength;
    MCP_write32int(mcp, MCP_SYMBOL(postJobA_DOT_capLength), &data, 1);
    data = discardLength;
    MCP_write32int(mcp, MCP_SYMBOL(postJobA_DOT_discardLength), &data, 1);
    data = outputPtr;
    MCP_write32int(mcp, MCP_SYMBOL(postJobA_DOT_capAddress), &data, 1);
    outputPtr += captureLength;

    /* Patch parameters for job B */
    data = captureLength;
    MCP_write32int(mcp, MCP_SYMBOL(postJobB_DOT_capLength), &data, 1);
    data = discardLength;
    MCP_write32int(mcp, MCP_SYMBOL(postJobB_DOT_discardLength), &data, 1);
    data = outputPtr;
    MCP_write32int(mcp, MCP_SYMBOL(postJobB_DOT_capAddress), &data, 1);

    /* Patch parameters for job N - Note, no address found */
    data = captureLength;
    MCP_write32int(mcp, MCP_SYMBOL(postJobN_DOT_capLength), &data, 1);
    data = discardLength;
    MCP_write32int(mcp, MCP_SYMBOL(postJobN_DOT_discardLength), &data, 1);

    /* Patch 2 x capLength for generating remaining job addresses in MCP */
    data = captureLength * 2;
    MCP_write32int(mcp, MCP_SYMBOL(twoTimesCapLen), &data, 1);

    /* Start MCP running */
    MCP_run(mcp);

    /* Wait for Halt */
    MCP_pollForHalt(mcp);

    /* Now test the captured data matches with what we expect */
    passed = testCapturedData(captureLength, SYMBOL_LENGTH, numJobs);

    return passed;

}

/*
 ** FUNCTION:    testCapturedData()
 **
 ** DESCRIPTION: Test the buffer of captured data is valid.
 **              The data streamed through the SCP is a ramp so the difference between samples is
 **              +1 except where the ramp wraps. Note in this test we only look at the I channel
 **              of the SCP output, assuming the Q channel is correctly associated;
 **              We detect this as well as the step due to the skipped part of the symbol.
 **              Note we allow an arbitrary start of the ramp because we could be injecting from
 **              playout where there is no symchronisation to the start of the test.
 **
 ** PARAMETERS:  captureLength - Number of samples captured per job
 **              symbolLength - Number of samples in 1 symbol of SCP frame counters
 **              numJobs - number of jobs run.
 **
 ** RETURNS:     TRUE - is test passed, FALSE if it failed.
 */
static bool
testCapturedData(unsigned captureLength, unsigned symbolLength,
                 unsigned numJobs)
{

    unsigned job, sample;
    int16_t currentValue, nextValue;
    int32_t samplesSkipped, extrapNextValue;
    MCP_GRAM_CMPLX_T *capturePtr;
    EDC_BUFFER_T outputBuffer;
    int mismatches = 0;

    /* Setup GRAM pointer to complex view */
    EDC_alignBuffer(scpOutputBufferSpace, SCP_OUTPUT_BUFFER_LEN, EDC_GRAM, 0,
                    &outputBuffer);
    capturePtr = outputBuffer.cpxAddr;

    /* Evaluate the number of sample skipped at a job boundary */
    samplesSkipped = symbolLength - captureLength;
    while (samplesSkipped < 0)
    {
        samplesSkipped += symbolLength;
    }

    /* Get first item in capture buffer (real part only)*/
    currentValue = ((int16_t)((*capturePtr) >> (16))) >> 4;
    nextValue = currentValue;

    for (job = 0; job < numJobs; job++)
    {
        for (sample = 0; sample < captureLength; sample++, capturePtr++)
        {
            /* Extract real part */
            currentValue = ((int16_t)((*capturePtr) >> (16))) >> 4;

            /* Compare */
            if (currentValue != nextValue)
            {
                if (mismatches < MAX_MISMATCHES_TO_SHOW)
                {
                    __TBILogF(
                              "    mismatch: job=%d, sample=%d, expected 0x%03x, found 0x%03x\n",
                              job, sample, nextValue & 0xfff, currentValue
                                      & 0xfff);
                }
                mismatches++;
            }

            /* Evaluate next value */
            nextValue = currentValue + TEST_SIGNAL_STEP;
            if (currentValue == TEST_SIGNAL_END_VALUE)
            {
                nextValue = TEST_SIGNAL_START_VALUE;
            }
        }

        /* Job boundary, next value will be different if the job length is not the same as the capture length */
        extrapNextValue = nextValue + (samplesSkipped * TEST_SIGNAL_STEP);
        while (extrapNextValue > TEST_SIGNAL_END_VALUE)
        {
            extrapNextValue -= TEST_SIGNAL_WRAP_STEP;
        }
        nextValue = (int16_t)extrapNextValue;
    }

    if (mismatches > 0)
    {
        __TBILogF(
                  "    Test failed: (captureLenght=%d, symbolLength=%d, jobs=%d) %d mismatches\n",
                  captureLength, symbolLength, numJobs, mismatches);
    }

    return (mismatches == 0 ? true : false);
}

