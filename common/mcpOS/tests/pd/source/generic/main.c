/*!
*****************************************************************************

 @file      main.c
 @brief     MCPOS Basic Test Program
            ... modified to use the pipeline driver

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

#include "uccrt.h"
#include "pipelineDriver.h"
#include "mcpos.h"
#include "mcp_source.h"
#include "mcp_source_family_data.h"
#include "irqGenDefaultDriver_DCP.h"
#include "dcpOutput.h"
#include "gramBuffers.h"

/*-------------------------------------------------------------------------*/

#if defined(__SIM__) && defined(__DEBUG__)
#define PDUMP
#endif

/*-------------------------------------------------------------------------*/

/* The number of MCPOS jobs to run */
#define NUM_JOBS_TO_RUN     (1)

/*-------------------------------------------------------------------------*/

/* MeOS constants and variables */

#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES - 1)

#define NUM_VEC_DEVICES 64

static KRN_SCHEDULE_T	schedule;
static KRN_TASKQ_T		taskQueues[MAX_PRIORITY + 1];

static QIO_DEVENTRY_T	deviceTable[NUM_VEC_DEVICES];

static QIO_SYS_T		qioSys;

/*-------------------------------------------------------------------------*/

static EDC_BUFFER_T traceBuffer;
static PD_SYSTEM_T pdSystem;

/*-------------------------------------------------------------------------*/

/* Logging */

#define LOG_RECORDS_SIZE (4096)
#define LOG_STRINGS_SIZE (1024)
#define CLOCK_FREQUENCY  (1000000)
#define MAX_TIME_STAMP   (4000000)
 
static LOG_CONTEXT_T GLB_eventLogCtx;
static unsigned int  logRecords[LOG_RECORDS_SIZE/4];

/*-------------------------------------------------------------------------*/

static void initMeos()
{
	KRN_TASK_T* taskPtr;

	/* Reset system */
	KRN_reset(&schedule, taskQueues, MAX_PRIORITY, 0, NULL, 0);

	/* Start multi-tasking environment */
	taskPtr = KRN_startOS(NULL);

	/* Reset I/O system */
	QIO_reset(&qioSys, deviceTable, NUM_VEC_DEVICES, UCCP_ivDesc,
			  TBID_SIGNUM_TR2(__TBIThreadId() >> TBID_THREAD_S), NULL, 0);
}

/*-------------------------------------------------------------------------*/

int main(void)
{      
    MCPOS_DEVICE_T mcposDevice;
    MCPOS_USE_T mcposUse;
    uint32_t job;
    PD_DEVICE_T pipelineDevice;
    MCP_T *mcp;
    DCP_PIPELINE_T *pipeline;
    int i;

#ifdef PDUMP
    assert(UCC_openPdump());
#endif

    /*
     * Initialisation and setup.
     */

    /* Initialise logging and switch it on */
    LOG_init(&GLB_eventLogCtx,
             logRecords,
             sizeof(logRecords),
             LOG_STRINGS_SIZE,
             CLOCK_FREQUENCY,
             MAX_TIME_STAMP);
    LOG_on();
     
    /* Initialise MeOS */
    initMeos();

    /* Initialise the UCC runtime */
    UCCP_init();
    UCCP_reset();
    
    /* Initialise pipeline driver system */
    PD_init(&pdSystem);
    
    /* Load the DCP image */
    DCP_load(&DCP_image_default, NULL);

    /* Configure the EDC trace buffer */
    EDC_configTraceBuffer(traceBufferSpace,
                          sizeof(traceBufferSpace),
                          0,
                          &traceBuffer);

    /* Get the pipeline */
    pipeline = DCP_getImagePipeline(DCP_pipelineId_mcposPipeline0);
 
    /* Load MCP image */
    mcp = UCC_getMCP(UCCP_getUCC(1),1);
    if (!MCP_loadImage(mcp,
                       imageFamilyData,
                       mcp_source_IMAGE_ID,
                       true))
    {
        UCC_LOGERR("Failed to load MCP image");
        return 1;
    }
    
    /* Initialise the MCPOS device.
     * Must do this after the MCP code has been loaded, as it does some
     * patching.
     */
    MCPOS_initDevice(&mcposDevice,
                     DCP_getImageDevice(DCP_deviceId_mcpos0),
                     mcp,
                     mcp_source_mcposWorkConsts(mcp));
                     
    /* Initialise the MCPOS use */
    MCPOS_initUse(&mcposUse,
                  &mcposDevice,
                  pipeline,
                  DCP_mcposPipeline_mcpos);

    /* Start all DCP devices (needed for MCPOS DCP logging thread) */
    DCP_startAllDevices();

    /* Run the MCP */
    MCP_run(mcp);

    /* Open pipeline */
    PD_openMcpos(&pipelineDevice, &mcposUse);

    /*
     * Run MCPOS job(s).
     */
    for (i = 0; i < NUM_JOBS_TO_RUN; i++)
    {
        /* Build a chain of jobs */
        job = mcp_source_JOB_DATA(mcp);
        MCPOS_buildJob(&mcposUse,
                       job,
                       mcp_source_dpArguments(mcp),
                       mcp_source_dotProductJob(mcp),
                       MCPOS_YIELD,
                       job + MCPOS_JOB_WORDS_HOST,
                       0,
                       0);
        MCPOS_buildJob(&mcposUse,
                       job + MCPOS_JOB_WORDS_HOST,
                       mcp_source_dpArguments(mcp),
                       mcp_source_dotProductJob(mcp),
                       MCPOS_FINAL,
                       0,
                       0,
                       1);                       

    	/* Run job */
    	PD_runJob(&pipelineDevice, job, NULL, 1, NULL, NULL);
        UCC_LOGMSG("Job finished");
    }

	/* Close pipeline */
	PD_close(&pipelineDevice);

#ifdef PDUMP
    UCC_closePdump();
#endif

    return 0;
}

/*-------------------------------------------------------------------------*/
