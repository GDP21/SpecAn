/*!
*****************************************************************************

 @file      mcpos.c
 @brief     MCPOS host component

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

/*-------------------------------------------------------------------------*/

#include <assert.h>
#include <stdbool.h>

#include "mcpos.h"
#include "mcposDefaultDriver_DCP.h"
#include "uccDefs.h"
#include "uccrt.h"

/*-------------------------------------------------------------------------*/

/* Initialises an MCPOS device */
void MCPOS_initDevice(MCPOS_DEVICE_T     *mcposDevice,
                      DCP_DEVICE_T       *dcpDevice,
                      MCP_T              *mcp,
                      MCP_DATA_ADDRESS_T  mcposWorkConsts)
{
#ifdef __UCC320__
    uint32_t reg;
#endif

    assert(mcposDevice);
    assert(dcpDevice);
    assert(mcp);

    /* Construction */
    mcposDevice->dcpDevice = dcpDevice;
    mcposDevice->mcp = mcp;

    /* Set the address of the mcposOr vector register */
    MCP_writeDcpDeviceParam(dcpDevice,
                            DCP_mcpos0DefaultDriver_mcposOr,
                            mcp,
                            mcposWorkConsts,
                            DCP_ATTR_OR_VECTOR);

#ifdef MCPOS_DCP_LOGGING
    /* Set the address of the mcposLogQ tail register */
    MCP_writeDcpDeviceParam(dcpDevice,
                            DCP_mcpos0DefaultDriver_mcposLogQ,
                            mcp,
                            mcposWorkConsts + 7, /* +7 because it's the 8th entry in the mcposWorkConsts block */
                            DCP_ATTR_Q_TAIL);
#endif

#ifdef __UCC320__
    /* Set MCP WAIT to non-legacy mode */
    /* TODO: Should this use MCP_configWaitSource? */
    reg = UCC_READ_PERIP(PMR_MCP + PMB_MCP_PERIP_WAIT_INT_SOURCE);
    reg = UCC_CLR_BITS(reg, PMB_MCP_EFC_LEGACY_WAIT_MASK);
    UCC_WRITE_PERIP(PMR_MCP + PMB_MCP_PERIP_WAIT_INT_SOURCE, reg);
#endif
}

/* Initialises an MCPOS use */
void MCPOS_initUse(MCPOS_USE_T    *mcposUse,
                   MCPOS_DEVICE_T *mcposDevice,
                   DCP_PIPELINE_T *dcpPipeline,
                   int             useId)
{
    assert(mcposUse);
    assert(mcposDevice);
    assert(dcpPipeline);

    /* Construction */
    mcposUse->mcposDevice = mcposDevice;
    mcposUse->dcpPipeline = dcpPipeline;
    mcposUse->jobQ   = DCP_getUseParam(dcpPipeline,
                                       useId,
                                       DCP_mcpos0DefaultDriver_jobQ);
    mcposUse->yieldQ = DCP_getUseParam(dcpPipeline,
                                       useId,
                                       DCP_mcpos0DefaultDriver_yieldQ);
    mcposUse->finalQ = DCP_getUseParam(dcpPipeline,
                                       useId,
                                       DCP_mcpos0DefaultDriver_finalQ);
}

/* Builds an MCPOS job */
void MCPOS_buildJob(MCPOS_USE_T         *mcposUse,
                    MCP_DATA_ADDRESS_T   thisJobAddr,
                    MCP_DATA_ADDRESS_T   ap7,
                    uint32_t             pc,
                    MCPOS_NEXT_ACTION_T  nextAction,
                    MCP_DATA_ADDRESS_T   nextJobAddr,
                    DCP_PARAM_ID_T       nextQ,
                    int                  useId)
{
    MCP_GRAM_INT_T *p;
    MCP_DATA_ADDRESS_T nextVal;
    DCP_PARAM_ID_T actualNextQ;
    uint32_t qTailAddr = 0;
    MCPOS_DEVICE_T *mcposDevice;

    assert(mcposUse);

    /* Retrieve the MCPOS device we're using */
    mcposDevice = mcposUse->mcposDevice;

    /*
     * Build the job.
     */
    p = MCP_addrInt2host(mcposDevice->mcp, thisJobAddr);

    /* AP7 */
    *p++ = ap7;

    /* PC */
    *p++ = pc;

    /* Next action */
    switch (nextAction)
    {
        default:
            assert(0);
            /* Deliberate drop through ... */

        case MCPOS_NONE:
            nextVal = nextJobAddr;
            actualNextQ = nextQ;
            break;

        case MCPOS_YIELD:
            nextVal = nextJobAddr;
            actualNextQ = mcposUse->yieldQ;
            break;

        case MCPOS_FINAL:
            nextVal = (MCP_DATA_ADDRESS_T)DCP_getJobId(mcposUse->dcpPipeline,
                                                       useId,
                                                       DCP_DYN_JOB_NUM);
            actualNextQ = mcposUse->finalQ;
            /* finalQ is delcared as virtual within the MCPOS group owner.
             * Assert that it has actually been connected to something.
             */
            assert(actualNextQ != DCP_INVALID_PARAM);
            break;

        case MCPOS_NULL:
            nextVal = 0;
            qTailAddr = (PMR_UCC_QM - REGPMRREG + PMB_QM_PERIP_BUILD_0) >> 2;
            break;
    }

    /* nextVal */
    *p++ = nextVal;

    /* nextQ */
    if (qTailAddr == 0)
    {
        qTailAddr = MCP_getDcpParamAttr(actualNextQ, DCP_ATTR_Q_TAIL);
    }
    *p++ = qTailAddr;
}

/* Builds an MCPOS job, where the next job is a DCP pipeline. Extended
 * version where posting to any block within the pipeline is required.
 */
void MCPOS_buildJobDcpEx(MCPOS_USE_T        *mcposUse,
                         MCP_DATA_ADDRESS_T  thisJobAddr,
                         MCP_DATA_ADDRESS_T  ap7,
                         uint32_t            pc,
                         DCP_PIPELINE_T     *pipeline,
                         int                 useId,
                         int                 jobNum)
{
    assert(pipeline);

    MCPOS_buildJob(mcposUse,
                   thisJobAddr,
                   ap7,
                   pc,
                   MCPOS_NONE,
                   DCP_getJobAddrEx(pipeline, useId, 0, jobNum),
                   DCP_getJobQueueEx(pipeline, useId),
                   0);
}

/* Starts an MCPOS job */
void MCPOS_startJob(MCPOS_USE_T        *mcposUse,
                    DCP_PARAM_ID_T      qId,
                    MCP_DATA_ADDRESS_T  jobAddr)
{
    /* Use the default job Q? */
    if (qId == DCP_DEFAULT_PARAM)
    {
        qId = mcposUse->jobQ;
    }

    /* Post into job Q */
    QM_postTail(qId, jobAddr);
}

/*-------------------------------------------------------------------------*/
