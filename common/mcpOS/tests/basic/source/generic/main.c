/*!
*****************************************************************************

 @file      main.c
 @brief     MCPOS Basic Test Program

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

static EDC_BUFFER_T traceBuffer;

/*-------------------------------------------------------------------------*/

int main(void)
{      
    MCPOS_DEVICE_T mcposDevice;
    MCPOS_USE_T mcposUse;
    MCP_T *mcp;
    uint32_t job;
    int i;

#ifdef PDUMP
    assert(UCC_openPdump());
#endif

    /*
     * Initialisation and setup.
     */

    /* Initialise the UCC runtime */
    UCCP_init();
    UCCP_reset();
    
    /* Load the DCP image */
    DCP_load(&DCP_image_default, NULL);  
    
    /* Configure the EDC trace buffer */
    EDC_configTraceBuffer(traceBufferSpace,
                          sizeof(traceBufferSpace),
                          0,
                          &traceBuffer);
    
    /* Load MCP image */
    mcp = UCC_getMCP(UCCP_getUCC(1), 1);
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
                  DCP_getImagePipeline(DCP_pipelineId_mcposPipeline0),
                  DCP_mcposPipeline_mcpos);

    /* Start all DCP devices (needed for MCPOS DCP logging thread) */
    DCP_startAllDevices();

    /* Run the MCP */
    MCP_run(mcp);
    
    /*
     * Run MCPOS job(s).
     */
    for (i = 0; i < NUM_JOBS_TO_RUN; i++)
    {
        /* Build the job */
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
    
        /* Start job */
        MCPOS_startJob(&mcposUse, DCP_DEFAULT_PARAM, job);
        
        /* Poll for completion */
        DCP_poll(NULL, NULL);
    }

#ifdef PDUMP
    UCC_closePdump();
#endif

    return 0;
}

/*-------------------------------------------------------------------------*/
