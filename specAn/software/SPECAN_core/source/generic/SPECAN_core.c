/*!****************************************************************************
 @File          SPECAN_core.c

 @Title         Spectrum Analyser core - top level

 @Date          27 November 2012

 @Copyright     Copyright (C) Imagination Technologies Limited 2010

 @Description   Implements the public interface to the core component.

 ******************************************************************************/
#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <assert.h>
#include "SPECAN_private.h"
#include "SPECAN_core_DCPM.h"

/* I know we are not supposed to have static data but the trace buffer system fundamentally
doesn't support multi-context operation at present.. */
uint32_t traceBufferSpace[TRACE_BUFFER_LEN_WORDS];	/* Placed in GRAM using ldscript */
static EDC_BUFFER_T traceBuffer;

/* Local prototypes */

static bool _SPECAN_activate(UFW_COREINSTANCE_T *coreInstance, void *coreParameter);
static bool _SPECAN_deActivate(UFW_COREINSTANCE_T *coreInstance, void *coreParameter);
static bool _SPECAN_enterInitialised(TV_INSTANCE_T *tvInst, TV_STATE_T previousState);
static bool _SPECAN_enterDetecting(TV_INSTANCE_T *tvInst, TV_STATE_T previousState);
static bool _SPECAN_leaveInitialised(TV_INSTANCE_T *tvInst, TV_STATE_T nextState);
static bool _SPECAN_leaveDetecting(TV_INSTANCE_T *tvInst, TV_STATE_T nextState);
static bool _SPECAN_eventHook(TV_INSTANCE_T *tvInstance, uint32_t event);
static bool _SPECAN_enterCompleted(TV_INSTANCE_T *tvInst, TV_STATE_T previousState);
static bool _SPECAN_leaveCompleted(TV_INSTANCE_T *tvInst, TV_STATE_T nextState);
static bool _SPECAN_enterRedetecting(TV_INSTANCE_T *tvInst, TV_STATE_T previousState);
static bool _SPECAN_leaveRedetecting(TV_INSTANCE_T *tvInst, TV_STATE_T nextState);
static bool _SPECAN_enterDemodulating(TV_INSTANCE_T *tvInst, TV_STATE_T previousState);
static bool _SPECAN_leaveDemodulating(TV_INSTANCE_T *tvInst, TV_STATE_T nextState);

/* Core descriptor object and tvInstanceExtensionData - OK to declare these statically as they are constant
for all instances */
static TV_COREDESC_EXTENSION_T _SPECAN_coreExtension = {
        sizeof(SPECAN_INSTANCE_CTX_T), /* size of SPECAN-specific context extension */
        _SPECAN_MAINTASK_STACK_SIZE,
        _SPECAN_MCP_GRAM_SIZE,
        UCC_STANDARD_DVBT, /* TV standard Id as signalled to tuner driver etc */
        _SPECAN_enterInitialised, /* state transition functions.... */
        _SPECAN_leaveInitialised,
        _SPECAN_enterDetecting,
        _SPECAN_leaveDetecting,
    	/* Note we have renamed the TV core state of "ACQUIRING" to "COMPLETED" */
        _SPECAN_enterCompleted,
        _SPECAN_leaveCompleted,
        _SPECAN_enterRedetecting,
        _SPECAN_leaveRedetecting,
        _SPECAN_enterDemodulating,
        _SPECAN_leaveDemodulating,
        _SPECAN_eventHook, /* main TV event handler extension hook */
        NULL
};

static UFW_COREDESC_T _SPECAN_coreDesc = {
        sizeof(TV_INSTANCE_T), /* initial startup data object */
        SPECAN_TOTALREG, /* number of registers, including the common ones */
        _SPECAN_initRegisterAPI,
        _SPECAN_activate,
        _SPECAN_deActivate,
        &_SPECAN_coreExtension};

/* Published pointer to core descriptor */
UFW_COREDESC_T *SPECANdescriptor = &_SPECAN_coreDesc;


/*--------------------------------------------------------------------------*/

static bool _SPECAN_startMCP(TV_INSTANCE_T *tvInstance)
{
	SPECAN_INSTANCE_CTX_T *SA_ctx = tvInstance->tvInstanceExtensionData;

	/* Initialise and load the MCP code */
	if (!SPECAN_initMCP(SA_ctx, _SPECAN_MCP_GRAM_SIZE))
	{
		assert(!"MCP init failed");
		return false;
	}

	/* Initialise MCPOS and build jobs */
	SPECAN_initMCPOS(SA_ctx);

	/* Run the MCP */
    MCP_run(tvInstance->mcp);

	return true;
}

/*--------------------------------------------------------------------------*/

static bool _SPECAN_activate(UFW_COREINSTANCE_T *coreInstance, void *coreParameter)
{
    TV_INSTANCE_T *tvInstance = coreInstance->instanceExtensionData;
    TV_ACTIVATION_PARAMETER_T *tvPar = coreParameter;
    SPECAN_INSTANCE_CTX_T *SAinstance;

	/* Note that the following bits of initialisation are really system-wide so
	they should be performed by something at a higher level, such as the framework.  Do
	them here to avoid burdening the user application, but note that we are assuming
	we have ownership of the whole system.  The same applies to DCP_startAllDevices. */
    /* ... Configure the EDC trace buffer */
    EDC_configTraceBuffer(traceBufferSpace, sizeof(traceBufferSpace), 1,
                          &traceBuffer);
    /* ... Load the DCP image */
    DCP_load(&DCP_image_default, NULL);

	/* Common TV app activation */
    /* ... Note here we assume that tunerUseList actually only has one entry */
    if (TV_activate(coreInstance, tvPar->tunerUseList->useId, tvPar))
    {
        /* Standard-specific initialisation */
        TVTUNER_configure(tvInstance, SPECAN_DEFAULT_TUNER_BW);
        TVTUNER_setSCP(tvInstance, 0); /* initial SCP setup - config 0 */

        SAinstance = tvInstance->tvInstanceExtensionData;

        /* ... Allocate resources and initialise our context space. */
        if (SPECAN_init(tvInstance))
        {
        	/* ... Start control task.  This will sit waiting on first message. */
        	KRN_startTask(SPECAN_controlTask, &SAinstance->controlTask,
        				SAinstance->controlTaskStack, _SPECAN_CONTROL_TASK_STACK_SIZE,
        	            tvInstance->coreInstance->priority, SAinstance, "SpecAn Control Task");


        	/* Start front end task.  This deals with analog AGC events. */
        	KRN_startTask(SPECAN_frontEndTask, &SAinstance->frontEndTask,
        			    SAinstance->frontEndTaskStack, _SPECAN_FRONT_END_TASK_STACK_SIZE,
        			    tvInstance->coreInstance->priority, SAinstance, "SpecAn Front End Task");

        	/* ... Initialise, load and start the MCP code */
        	if (_SPECAN_startMCP(tvInstance))
        	{
        		/* ... TV core start function */
				TV_start(coreInstance);
				/* success! */
				return true;
        	}
        }

    }
    /* If we get here then activation failed */
    UCC_LOGMSG("Activation failed");
    assert(!"Activation failed");
    return false;

}

/*--------------------------------------------------------------------------*/

static bool _SPECAN_deActivate(UFW_COREINSTANCE_T *coreInstance, void *coreParameter)
{
    TV_INSTANCE_T *tvInstance = coreInstance->instanceExtensionData;
    SPECAN_INSTANCE_CTX_T *SActx = tvInstance->tvInstanceExtensionData;

#ifdef LATER
	/* Stop all DCP devices */
	DCP_stopAllDevices();
#endif

	/* Remove all tasks except our master task, which is removed by TV_deActivate */
	KRN_removeTask(&SActx->controlTask);

	KRN_removeTask(&SActx->frontEndTask);

	/* No need to free memory as all memory from pools will be freed to mark by
	the framework. */

    /* Common TV de-activation.  This stops the MCP and shuts down the tuner. */
    TV_deActivate(coreInstance, coreParameter);

    return true;
}

/*--------------------------------------------------------------------------*/

static bool _SPECAN_enterInitialised(TV_INSTANCE_T *tvInst, TV_STATE_T previousState)
{
	/* Nothing to do here */
	(void)tvInst;
	(void)previousState;

	return true;
}

/*--------------------------------------------------------------------------*/

static bool _SPECAN_enterDetecting(TV_INSTANCE_T *tvInst, TV_STATE_T previousState)
{
	assert(previousState == TV_STATE_INITIALISED);
	(void)previousState;			// Remove warning from RELEASE build
	/* Kick off a scan */
	SPECAN_initiateScan((SPECAN_INSTANCE_CTX_T *)tvInst->tvInstanceExtensionData);

	return true;
}

/*--------------------------------------------------------------------------*/

static bool _SPECAN_leaveInitialised(TV_INSTANCE_T *tvInst, TV_STATE_T nextState)
{
	/* Nothing to do here */
	(void)tvInst;
	(void)nextState;
	return true;
}

/*--------------------------------------------------------------------------*/

static bool _SPECAN_leaveDetecting(TV_INSTANCE_T *tvInst, TV_STATE_T nextState)
{
	/* Nothing to do here */
	(void)tvInst;
	(void)nextState;
	return true;
}

/*--------------------------------------------------------------------------*/

static bool _SPECAN_eventHook(TV_INSTANCE_T *tvInstance, uint32_t event)
{
	SPECAN_INSTANCE_CTX_T *SA_ctx = tvInstance->tvInstanceExtensionData;
    SPECAN_CONTROL_FSM_MESSAGE_T *msgPtr;

	if (event == TV_EVENT_TUNED)
	{
		/* On tuned event, send a message to control task to signal completion. */
		msgPtr = KRN_takePool(&SA_ctx->ctrlMsgPool, -1);
		msgPtr->message = SPECAN_TUNE_EVENT;
		KRN_putMbox(&SA_ctx->ctrlTaskMbox, (void *)msgPtr);
	}

	return true;
}

/*--------------------------------------------------------------------------*/

static bool _SPECAN_enterCompleted(TV_INSTANCE_T *tvInst, TV_STATE_T previousState)
{
	/* Nothing to do here.  Note we have renamed the TV core state of "ACQUIRING" to "COMPLETED" */
	(void)tvInst;
	(void)previousState;

	return true;
}

/*--------------------------------------------------------------------------*/

static bool _SPECAN_leaveCompleted(TV_INSTANCE_T *tvInst, TV_STATE_T nextState)
{
	/* Nothing to do here.  Note we have renamed the TV core state of "ACQUIRING" to "COMPLETED" */
	(void)tvInst;
	(void)nextState;

	return true;
}

/*--------------------------------------------------------------------------*/

static bool _SPECAN_enterRedetecting(TV_INSTANCE_T *tvInst, TV_STATE_T previousState)
{
	/* Nothing to do here */
	(void)tvInst;
	(void)previousState;
	assert(!"Unexpected state - RE-DETECTING");

	return true;
}

/*--------------------------------------------------------------------------*/

static bool _SPECAN_leaveRedetecting(TV_INSTANCE_T *tvInst, TV_STATE_T nextState)
{
	/* Nothing to do here */
	(void)tvInst;
	(void)nextState;

	return true;
}

/*--------------------------------------------------------------------------*/

static bool _SPECAN_enterDemodulating(TV_INSTANCE_T *tvInst, TV_STATE_T previousState)
{
	/* Nothing to do here */
	(void)tvInst;
	(void)previousState;
	assert(!"Unexpected state - DEMODULATING");

	return true;
}

/*--------------------------------------------------------------------------*/

static bool _SPECAN_leaveDemodulating(TV_INSTANCE_T *tvInst, TV_STATE_T nextState)
{
	/* Nothing to do here */
	(void)tvInst;
	(void)nextState;

	return true;
}
