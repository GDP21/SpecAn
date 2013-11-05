/*!****************************************************************************
 @File          SPECAN_mcpCtrl.c

 @Title         Spectrum Analyser core - MCP initialisation and control functions

 @Date          27 November 2012

 @Copyright     Copyright (C) Imagination Technologies Limited 2012

 ******************************************************************************/
#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include "assert.h"
#include "SPECAN_private.h"
#include "mcpprogs.h"
#include "mcpprogs_family_data.h"
#include "SPECAN_core_DCPM.h"
#include "irqGenDefaultDriver_DCP.h"
#include "SPECAN_mcposDriver_DCP.h"
#include "scpDriver_DCP.h"

/* Set up MCP address space to GRAM mapping, load the code image and set
the PC ready to start.  Also load up the required data structures in MCP memory
for the MCP code to use. */
bool SPECAN_initMCP(SPECAN_INSTANCE_CTX_T *SA_ctx, unsigned GRAMsizeAllocated)
{
    unsigned aSize;
    unsigned bSize;
    unsigned lSize;
    UCCP_GRAM_ADDRESS_T dummy;
    TV_INSTANCE_T *tvInst = SA_ctx->tvInstance;
	MCP_T *mcp = tvInst->mcp;
	MCP_GRAM_INT_T *bufferAddr;
	uint32_t bufferLen, bufferLen_bytes, qAddr, jobID;
	DCP_PARAM_ID_T Qid;
	DCP_PIPELINE_T *SCPpipeline = DCP_getImagePipeline(DCP_pipelineId_SCPPipeline);

    /* The MCP_mapMemoryToGRAM function sets up the hardware mapping between MCP addresses
    and GRAM addresses.  Prior to doing this, check that our MCP image fits within the space
    which was reserved when TV_activate was run */
    MCP_imageMap(imageFamilyData, mcpImageDemod_IMAGE_ID, &dummy, &aSize, &dummy, &bSize, &dummy,
                 &lSize);
    if ((aSize + bSize + lSize) > GRAMsizeAllocated)
    {
    	assert(!"GRAM size overflow");
        return false;
    }
    /* Regions A, B and L are mapped to follow on contiguously from one another */
    MCP_mapMemoryToGRAM(mcp, tvInst->mcpGRAMBase,
                        tvInst->mcpGRAMBase + aSize,
                        tvInst->mcpGRAMBase + aSize + bSize);

    /* You cannot access any MCP variables from mcpprogs.h
     * until you call MCP_loadImage */
    MCP_loadImage(mcp, imageFamilyData, mcpImageDemod_IMAGE_ID, false);

    /* Set the PC to correct starting position (note we don't actually run yet) */
    MCP_setPC(mcp, mcp_src_MAIN(mcp));

    /* Initialise pointers to MCP variables in our context space */
    SA_ctx->MCP_ptrs.averagingPeriod_innerLoopCount = MCP_addrInt2host(mcp, mcp_src_averagingPeriod_innerLoopCount(mcp));
    SA_ctx->MCP_ptrs.averagingPeriod_innerLoop = MCP_addrInt2host(mcp, mcp_src_averagingPeriod_innerLoop(mcp));
    SA_ctx->MCP_ptrs.averagingPeriod_innerLoop_log2 = MCP_addrInt2host(mcp, mcp_src_averagingPeriod_innerLoop_log2(mcp));
    SA_ctx->MCP_ptrs.averagingPeriod_outerLoopCount = MCP_addrInt2host(mcp, mcp_src_averagingPeriod_outerLoopCount(mcp));
    SA_ctx->MCP_ptrs.averagingPeriod_outerLoop_mult = MCP_addrInt2host(mcp, mcp_src_averagingPeriod_outerLoop_mult(mcp));
    SA_ctx->MCP_ptrs.SCPcaptureLen = MCP_addrInt2host(mcp, mcp_src_SCPcaptureLen(mcp));

    SA_ctx->MCP_ptrs.pFFTlen = MCP_addrInt2host(mcp, mcp_src_FFTlen(mcp));
    SA_ctx->MCP_ptrs.pFFTlen_log2 = MCP_addrInt2host(mcp, mcp_src_FFTlen_log2(mcp));
    SA_ctx->MCP_ptrs.pFFTshifts = MCP_addrInt2host(mcp, mcp_src_SPECAN_fftShifts(mcp));

    /* Initialise buffer descriptors for MCP buffers: */
	bufferLen = mcp_src_SA_MAX_FFT_LEN(mcp);
	/* Note we are presenting EDC_alignBuffer with a non-packed GRAM address so it expects
	a non-packed length, i.e. 4 bytes per word */
	bufferLen_bytes = bufferLen * sizeof(uint32_t); // convert to bytes
	/* ...SCP out buffer A */
	bufferAddr = MCP_addrInt2host(mcp, mcp_src_SCPoutBufA(mcp));
    EDC_alignBuffer((void *)bufferAddr, bufferLen_bytes, EDC_GRAM, 0, &SA_ctx->MCP_ptrs.SCPoutBufferA);
	/* ...SCP out buffer B */
	bufferAddr = MCP_addrInt2host(mcp, mcp_src_SCPoutBufB(mcp));
    EDC_alignBuffer((void *)bufferAddr, bufferLen_bytes, EDC_GRAM, 0, &SA_ctx->MCP_ptrs.SCPoutBufferB);
	/* ...Power spectral density buffer */
	bufferAddr = MCP_addrInt2host(mcp, mcp_src_outputPowBuf(mcp));
    EDC_alignBuffer((void *)bufferAddr, bufferLen_bytes, EDC_GRAM, 0, &SA_ctx->MCP_ptrs.SpectralFragmentBuff);
    /* ... window function buffer */
	bufferAddr = MCP_addrInt2host(mcp, mcp_src_windowFunc(mcp));
    EDC_alignBuffer((void *)bufferAddr, bufferLen_bytes, EDC_GRAM, 0, &SA_ctx->MCP_ptrs.windowFunc);

    /* Initialise pointers in MCP memory */
    /* ..pointer to tail of SCP MCPOS job queue */
	qAddr = MCP_getDcpParamAttr(SA_ctx->mcposSCPjobQID, DCP_ATTR_Q_TAIL);
	bufferAddr = MCP_addrInt2host(mcp, mcp_src_mcposSCPjobQtailPtr(mcp));
	*bufferAddr = qAddr;

	/* .. pointer to tail of the queue which MCP needs to post into
	to provide Meta with messages (and interrupts) */
	Qid = DCP_getUseParam(SCPpipeline, DCP_SA_SCPPipeline_irqGen, DCP_irqGenDefaultDriver_finalQ);
	qAddr = MCP_getDcpParamAttr(Qid, DCP_ATTR_Q_TAIL);
	bufferAddr = MCP_addrInt2host(mcp, mcp_src_metaInterruptQptr(mcp));
	*bufferAddr = qAddr;

	/* .. pointer to tail of the SCP input address queue */
	Qid = DCP_getUseParam(SCPpipeline, DCP_SA_SCPPipeline_SCP, DCP_scpDriver_inputAddressQ);
	qAddr = MCP_getDcpParamAttr(Qid, DCP_ATTR_Q_TAIL);
	bufferAddr = MCP_addrInt2host(mcp, mcp_src_SCPIN_addressQtailPtr(mcp));
	*bufferAddr = qAddr;

	/* .. pointer to tail of the SCP input capture length queue */
	Qid = DCP_getUseParam(SCPpipeline, DCP_SA_SCPPipeline_SCP, DCP_scpDriver_inputCaptureLengthQ);
	qAddr = MCP_getDcpParamAttr(Qid, DCP_ATTR_Q_TAIL);
	bufferAddr = MCP_addrInt2host(mcp, mcp_src_SCPIN_captureLenQtailPtr(mcp));
	*bufferAddr = qAddr;

	/* .. pointer to tail of the SCP discard length queue */
	Qid = DCP_getUseParam(SCPpipeline, DCP_SA_SCPPipeline_SCP, DCP_scpDriver_inputDiscardLengthQ);
	qAddr = MCP_getDcpParamAttr(Qid, DCP_ATTR_Q_TAIL);
	bufferAddr = MCP_addrInt2host(mcp, mcp_src_SCPIN_discardLenQtailPtr(mcp));
	*bufferAddr = qAddr;

	/* .. pointer to head of the SCP output address queue */
	Qid = DCP_getUseParam(SCPpipeline, DCP_SA_SCPPipeline_mcpos, DCP_SPECAN_mcposDriver_scpCompletionQ);
	qAddr = MCP_getDcpParamAttr(Qid, DCP_ATTR_Q_HEAD);
	bufferAddr = MCP_addrInt2host(mcp, mcp_src_SCPOUT_addressQheadPtr(mcp));
	*bufferAddr = qAddr;

	/* Construct a job ID for the MCP to use for posting.  This consists of a pipeline ID, Job number and use ID.
	We specify job number DCP_DYN_JOB_NUM which means that the useId part becomes our
	"user data"; this we use to indicate the message being sent. */
	jobID = DCP_getJobId(SCPpipeline, FRAGMENT_COMPLETED_USE_ID, DCP_DYN_JOB_NUM);
	bufferAddr = MCP_addrInt2host(mcp, mcp_src_fragmentCompleteJobID(mcp));
	*bufferAddr = jobID;

	/* To post SCP jobs the MCP needs to know the absolute word offset of SCP buffers (different from
	the MCP address of these buffers) */
	bufferAddr = MCP_addrInt2host(mcp, mcp_src_SCPoutBufA_wordOffset(mcp));
	*bufferAddr = SA_ctx->MCP_ptrs.SCPoutBufferA.wordOffset;
	bufferAddr = MCP_addrInt2host(mcp, mcp_src_SCPoutBufB_wordOffset(mcp));
	*bufferAddr = SA_ctx->MCP_ptrs.SCPoutBufferB.wordOffset;

    return true;
}

/*--------------------------------------------------------------------------*/

/* Initialise the MCPOS system.
Must do this after the MCP code has been loaded, as it does some patching. */
void SPECAN_initMCPOS(SPECAN_INSTANCE_CTX_T *SA_ctx)
{
    int ap7, pc;
    unsigned thisJob;
    TV_INSTANCE_T *tvInst = SA_ctx->tvInstance;
    MCP_T *mcp = tvInst->mcp;
    DCP_DEVICE_T *mcpos_deviceId = DCP_getImageDevice(DCP_deviceId_mcpos0);
    DCP_PIPELINE_T *mcpos_pipelineId = DCP_getImagePipeline(DCP_pipelineId_SCPPipeline);
    int mcpos_useId = DCP_SA_SCPPipeline_mcpos;

	/* Initialise MCPOS */
	MCPOS_initDevice(SA_ctx->mcpos,
					 mcpos_deviceId,
					 mcp,
					 mcp_src_mcposWorkConsts(mcp));

	/* ... only one MCPOS use case to initialise */
	MCPOS_initUse(&SA_ctx->mcposUse,
					SA_ctx->mcpos,
					mcpos_pipelineId,
					mcpos_useId);

	/* Build MCPOS jobs.  Note we specify MCPOS_NULL rather than MCPOS_FINAL so these jobs do not
	lead to an interrupt, and the useId field is unused. */
	/* ...SCP buffer A completion */
    pc = mcp_src_SPECAN_SCPoutBufferProc(mcp);
	thisJob = mcp_src_SCP_CAPTURE_JOB_BUFA(mcp);
	ap7 = mcp_src_SCPoutBufferProc_argChainA(mcp);
	/* This job interrupts Meta to flag that it has completed. */
	MCPOS_buildJob(&SA_ctx->mcposUse, thisJob, ap7, pc, MCPOS_NULL, 0, 0, CAPTURE_COMPLETE_MCPOS_USE_ID);

	/* ...SCP buffer B completion */
    pc = mcp_src_SPECAN_SCPoutBufferProc(mcp);
	thisJob = mcp_src_SCP_CAPTURE_JOB_BUFB(mcp);
	ap7 = mcp_src_SCPoutBufferProc_argChainB(mcp);
	/* This job interrupts Meta to flag that it has completed. */
	MCPOS_buildJob(&SA_ctx->mcposUse, thisJob, ap7, pc, MCPOS_NULL, 0, 0, CAPTURE_COMPLETE_MCPOS_USE_ID);


    // Call Other/Our MCP code
    
    
	/* Prevent warning: mcp is actually unused, as the MCP parameter to the macros
	above ends up unused. */
	ap7 = (int)mcp;
}
