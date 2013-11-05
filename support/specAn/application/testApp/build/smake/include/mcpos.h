/*!
*****************************************************************************

 @file      mcpos.h
 @brief     MCPOS host component header file

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#ifndef _MCPOS_H_
#define _MCPOS_H_

/*-------------------------------------------------------------------------*/

#include "uccrt.h"

/*-------------------------------------------------------------------------*/

/*
 * An MCPOS job is simple a block of MCP data memory with fields containing
 * job parameters. The size of this block and the offsets of the fields are
 * defined here. These definitions should be regarded as the specification
 * for an MCPOS job.
 */

/** The size of an MCPOS job in words. Note this should tie up with
 *  MCPOS_JOB_WORDS as defined in the MCP source. The reason for suffixing
 *  this definition with _HOST is to prevent a name clash with that
 *  contained in the MCP source.
 */
#define MCPOS_JOB_WORDS_HOST (6)

/** Offset of the AP7 field within a job */
#define MCPOS_JOB_AP7_OFFSET (0)

/** Offset of the PC field within a job */
#define MCPOS_JOB_PC_OFFSET (1)

/** Offset of the nextVal field within a job */
#define MCPOS_JOB_NEXT_VAL_OFFSET (2)

/** Offset of the nextQ field within a job */
#define MCPOS_JOB_NEXT_Q_OFFSET (3)

/** Offset of the iscrIn field within a job */
#define MCPOS_JOB_ISCR_IN_OFFSET (4)

/** Offset of the iscrOut field within a job */
#define MCPOS_JOB_ISCR_OUT_OFFSET (5)

/*-------------------------------------------------------------------------*/

/** Enumeration of next actions */
typedef enum
{
    /* No special action. Post nextJob into nextQ. */
    MCPOS_NONE = 0,

    /** Post nextJob into the yield Q */
    MCPOS_YIELD,

    /** Post JOBID into the final Q */
    MCPOS_FINAL,

    /** Post "don't care" into null Q */
    MCPOS_NULL
} MCPOS_NEXT_ACTION_T;

/** Structure to contain an MCPOS device */
typedef struct mcpos_device_t
{
    /** The underlying DCP device */
    DCP_DEVICE_T *dcpDevice;

    /** MCP object */
    MCP_T *mcp;
} MCPOS_DEVICE_T;

/** Structure to contain an MCPOS use */
typedef struct mcpos_use_t
{
    /** Pointer to the MCPOS device we're using */
    struct mcpos_device_t *mcposDevice;

    /** The underlying DCP pipeline */
    DCP_PIPELINE_T *dcpPipeline;

    /** ID of the jobQ within mcpos */
    DCP_PARAM_ID_T jobQ;

    /** ID of the yieldQ within mcpos */
    DCP_PARAM_ID_T yieldQ;

    /** ID of the finalQ within irqGen */
    DCP_PARAM_ID_T finalQ;
} MCPOS_USE_T;

/*-------------------------------------------------------------------------*/

/**
 * Initialises an MCPOS device.
 *
 * @param[in]  mcposDevice      MCPOS device to initialise
 * @param[in]  dcpDevice        The underlying DCP device
 * @param[in]  mcp              MCP instance handle
 * @param[in]  mcposWorkConsts  Offset of MCPOS work constants in the MCP
 *                              code
 */
void MCPOS_initDevice(MCPOS_DEVICE_T     *mcposDevice,
                      DCP_DEVICE_T       *dcpDevice,
                      MCP_T              *mcp,
                      MCP_DATA_ADDRESS_T  mcposWorkConsts);

/**
 * Initialises an MCPOS use.
 *
 * @param[in]  mcposUse     MCPOS use to initialise
 * @param[in]  mcposDevice  The MCPOS device to use
 * @param[in]  dcpPipeline  The underlying DCP pipeline
 * @param[in]  useId        Use ID of MCPOS within the pipeline
 */
void MCPOS_initUse(MCPOS_USE_T    *mcposUse,
                   MCPOS_DEVICE_T *mcposDevice,
                   DCP_PIPELINE_T *dcpPipeline,
                   int             useId);

/**
 * Builds an MCPOS job.
 *
 * @param[in]  mcposUse     MCPOS use
 * @param[in]  thisJobAddr  Address of the job to be built
 * @param[in]  ap7          AP7 value
 * @param[in]  pc           PC value
 * @param[in]  nextAction   Next action to take after this job
 * @param[in]  nextJobAddr  Address of the next job in the chain
 * @param[in]  nextQ        Next queue in the chain
 * @param[in]  useId        The use ID to use when generating a job ID.
 *                          Only relevant when nextAction is MCPOS_FINAL.
 */
void MCPOS_buildJob(MCPOS_USE_T         *mcposUse,
                    MCP_DATA_ADDRESS_T   thisJobAddr,
                    MCP_DATA_ADDRESS_T   ap7,
                    uint32_t             pc,
                    MCPOS_NEXT_ACTION_T  nextAction,
                    MCP_DATA_ADDRESS_T   nextJobAddr,
                    DCP_PARAM_ID_T       nextQ,
                    int                  useId);

/**
 * Builds an MCPOS job, where the next job is a DCP pipeline.
 *
 * @param[in]  mcposUse     MCPOS use
 * @param[in]  thisJobAddr  Address of the job to be built
 * @param[in]  ap7          AP7 value
 * @param[in]  pc           PC value
 * @param[in]  pipeline     The DCP pipeline to post into next
 * @param[in]  jobNum       The job number to post into the above pipeline.
 */
#define MCPOS_buildJobDcp(mcposUse, thisJobAddr, ap7, pc, pipeline, jobNum) \
    MCPOS_buildJobDcpEx(mcposUse, thisJobAddr, ap7, pc, pipeline, DCP_getFirstUse(pipeline), jobNum)

/**
 * Builds an MCPOS job, where the next job is a DCP pipeline. Extended
 * version where posting to any block within the pipeline is required.
 *
 * @param[in]  mcposUse     MCPOS use
 * @param[in]  thisJobAddr  Address of the job to be built
 * @param[in]  ap7          AP7 value
 * @param[in]  pc           PC value
 * @param[in]  pipeline     The DCP pipeline to post into next
 * @param[in]  useId        The ID of the block within the pipeline to post
 *                          into.
 * @param[in]  jobNum       The job number to post into the above pipeline.
 */
void MCPOS_buildJobDcpEx(MCPOS_USE_T        *mcposUse,
                         MCP_DATA_ADDRESS_T  thisJobAddr,
                         MCP_DATA_ADDRESS_T  ap7,
                         uint32_t            pc,
                         DCP_PIPELINE_T     *pipeline,
                         int                 useId,
                         int                 jobNum);

/**
 * Starts an MCPOS job.
 *
 * @param[in]  mcposUse  MCPOS use
 * @param[in]  qId       The ID of the Q to post to. Set to
 *                       DCP_DEFAULT_PARAM to post to the default job Q.
 * @param[in]  jobAddr       Address of the job to post
 */
void MCPOS_startJob(MCPOS_USE_T        *mcposUse,
                    DCP_PARAM_ID_T      qId,
                    MCP_DATA_ADDRESS_T  jobAddr);

/**
 * Version of MCPOS_startJob where only the DCP pipeline is supplied.
 *
 * @param[in]  pipeline  Pointer to the DCP pipeline
 * @param[in]  qId       The ID of the Q to post to. Set to
 *                       DCP_DEFAULT_PARAM to post to the default job Q.
 * @param[in]  jobAddr   Address of the job to post
 */
void MCPOS_startJobEx(DCP_PIPELINE_T     *pipeline,
                      DCP_PARAM_ID_T     qId,
                      MCP_DATA_ADDRESS_T jobAddr);

/*-------------------------------------------------------------------------*/

#endif /* _MCPOS_H_ */

/*-------------------------------------------------------------------------*/
