/*!****************************************************************************
 @File          SPECAN_isr.c

 @Title         Spectrum Analyser core - interrupt-related functions

 @Date          27 November 2012

 @Copyright     Copyright (C) Imagination Technologies Limited 2012

 ******************************************************************************/
#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <assert.h>
#include "SPECAN_private.h"
#include "SPECAN_core_DCPM.h"

/* Completion function that will be installed to run in response
to MCP message interrupts */
static void _SPECAN_MCPmessageHandler(DCP_PIPELINE_T *pipeline,
                                   void           *context,
                                   int             jobNum,
                                   int             useId)
{
	SPECAN_INSTANCE_CTX_T *SA_ctx = (SPECAN_INSTANCE_CTX_T *)context;
    (void)jobNum;

#ifdef __RELEASE__
	(void)pipeline;
#endif

	/* This handler is only installed for completions on the SCP pipeline */
	assert(pipeline == DCP_getImagePipeline(DCP_pipelineId_SCPPipeline));

	switch(useId)
	{
	case CAPTURE_COMPLETE_MCPOS_USE_ID:
		UCC_LOGMSG("MCPOS job completed");
		break;
	case FRAGMENT_COMPLETED_USE_ID:
		UCC_LOGMSG("Fragment completed");

		SPECAN_processFragmentCompletion(SA_ctx);
		break;
	default:
		assert(!"Unknown MCP message");
		break;
	}
}


void SPECAN_installInterruptHandlers(SPECAN_INSTANCE_CTX_T *SA_ctx)
{
	/* The irqGen that we are using to provide a Q for MCP messages is located
	on the DFE pipeline, for no good reason. */
	DCP_installCompletionHandler(DCP_getImagePipeline(DCP_pipelineId_SCPPipeline),
					_SPECAN_MCPmessageHandler, (void *)SA_ctx);
}
