/*!****************************************************************************
 @File          SPECAN_controlTask.c

 @Title         Control state machine for spectrum analyser core

 @Date          30 Nov 2012

 @Copyright     Copyright (C) Imagination Technologies Limited 2010

 ******************************************************************************/
#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <assert.h>
#include "SPECAN_private.h"
#include "SPECAN_logging.h"
#include "mcpprogs.h"

#define ROUND2INT(A)    ((A+(1<<(SPECAN_OUTPUT_FRAC_BITS-1)))>>SPECAN_OUTPUT_FRAC_BITS)

/*----------------------------------------------------------------------*/
void SPECAN_QctrlFSMmessage(SPECAN_INSTANCE_CTX_T *SA_ctx, SPECAN_CONTROL_MSG_T message, unsigned param)
{
    SPECAN_CONTROL_FSM_MESSAGE_T *stateChangeMsgPtr;

    /* The alternative function SLT_QctrlStateChange should be used for state change requests */
    assert(message != SPECAN_CHANGE_STATE);

    /* Note that when running this function from an ISR we are not allowed to block, so use a
    zero timeout when taking a message from the pool; but assert if we don't have a valid one.
    (NB need to ensure that there are sufficient in the pool such that we never hit this
    assert) */
    stateChangeMsgPtr = (SPECAN_CONTROL_FSM_MESSAGE_T *)KRN_takePool(&SA_ctx->ctrlMsgPool, 0);
    if (stateChangeMsgPtr != NULL)
    {
        stateChangeMsgPtr->newState = SPECAN_STATE_INVALID;
        stateChangeMsgPtr->message = message;
        stateChangeMsgPtr->code = param;
        KRN_putMbox(&SA_ctx->ctrlTaskMbox, (void *)stateChangeMsgPtr);
    }
    else
    {
        assert(!"Ran out of ctrl state change message descriptors");
    }
}

/*----------------------------------------------------------------------*/
void SPECAN_QctrlStateChange(SPECAN_INSTANCE_CTX_T *SA_ctx, SPECAN_CONTROL_STATE_T newState)
{
    SPECAN_CONTROL_FSM_MESSAGE_T *stateChangeMsgPtr;

    /* Note that when running this function from an ISR we are not allowed to block, so use a
    zero timeout when taking a message from the pool; but assert if we don't have a valid one.
    (NB need to ensure that there are sufficient in the pool such that we never hit this
    assert) */
    stateChangeMsgPtr = (SPECAN_CONTROL_FSM_MESSAGE_T *)KRN_takePool(&SA_ctx->ctrlMsgPool, 0);
    if (stateChangeMsgPtr != NULL)
    {
        stateChangeMsgPtr->newState = newState;
        stateChangeMsgPtr->message = SPECAN_CHANGE_STATE;
        KRN_putMbox(&SA_ctx->ctrlTaskMbox, (void *)stateChangeMsgPtr);
    }
    else
    {
        assert(!"Ran out of ctrl state change message descriptors");
    }
}


/*----------------------------------------------------------------------*/
void SPECAN_initiateScan(SPECAN_INSTANCE_CTX_T *SA_ctx)
{
    SPECAN_CONTROL_FSM_MESSAGE_T *msgPtr;

    msgPtr = KRN_takePool(&SA_ctx->ctrlMsgPool, -1);
    msgPtr->message = SPECAN_RUN_SCAN;
    KRN_putMbox(&SA_ctx->ctrlTaskMbox, (void *)msgPtr);
}

/*----------------------------------------------------------------------*/
/* Search the spectrum and write peaks to the API.
 */
static void writePeaksToApi(SPECAN_INSTANCE_CTX_T *SA_ctx, UFW_COREINSTANCE_T *coreInstance)
{
    uint32_t i, reg;
    SPECAN_PEAK_T *pPeaks=SA_ctx->maxPeakArray;

    /* Update the API with Spectral peaks info. */
    SPECAN_compositeMgr_findPeaks(&SA_ctx->compositeCtx, pPeaks, SPECAN_MAX_PEAK_COUNT, SA_ctx->peakWidthBins);

    /* Write results to API. */
    for (i=0; i<SPECAN_MAX_PEAK_COUNT; i++)
    {
        /* Output register has only room for 8-bits so round down. */
        reg = ((ROUND2INT(pPeaks->val) & SA_MAX_POWER_N_MASK)   <<  SA_MAX_POWER_N_BITSHIFT) |
              ((pPeaks->i   & SA_MAX_POWER_N_INDEX_MASK) <<  SA_MAX_POWER_N_INDEX_BITSHIFT);

        TVREG_coreWrite(coreInstance, SA_MAX_POWER_REG_0+i, reg);

        pPeaks++;
    }
}


/*----------------------------------------------------------------------*/
void SPECAN_controlTask(void)
{
    int rtn;

    SPECAN_INSTANCE_CTX_T *SA_ctx = KRN_taskParameter(NULL);
    TV_INSTANCE_T         *tvInstance   = SA_ctx->tvInstance;
    UFW_COREINSTANCE_T    *coreInstance = tvInstance->coreInstance;

    uint32_t tuneIssueTime = 0;

    SPECAN_CONTROL_FSM_MESSAGE_T *msgPtr;
    SPECAN_CONTROL_MSG_T          msg;
    SPECAN_CONTROL_STATE_T        newState, stateChangeRequest;

    TIME_STAMP_T *pTimeStamps = &SA_ctx->timeStamps;

    SA_ctx->ctrlState = SPECAN_STATE_IDLE;  /* IDLE when we begin the task */

    while (1)
    {
        /* Wait on mailbox, indefinitely until a message is posted */
        msgPtr   = (SPECAN_CONTROL_FSM_MESSAGE_T *)KRN_getMbox(&SA_ctx->ctrlTaskMbox, -1);
        msg      = msgPtr->message;

        newState = SPECAN_STATE_INVALID;
        stateChangeRequest = SPECAN_STATE_INVALID; /* Default: no state change requested */

        /* Control Message handling. */
        switch(msg)
        {
        case SPECAN_RUN_SCAN:
            // move from IDLE to a running state
            UCC_LOGMSG("Running scan..");

            recordAcquisitionStart(pTimeStamps);

            newState = SPECAN_STATE_START;
            break;

        case SPECAN_STOP_SCAN:
            UCC_LOGMSG("Stopping scan..");
            newState = SPECAN_STATE_IDLE;   // move to an idle state
            break;

        case SPECAN_CHANGE_STATE:
            newState = msgPtr->newState;    // read the new state from the message structure
            break;

        case SPECAN_TUNE_EVENT:

            SA_ctx->currentTuneFreq = TV_getActiveFrequency(tvInstance); /* update current tune frequency field. */
            uint32_t tuneTime = TBI_GETREG(TXTIMER) - tuneIssueTime;
            uint32_t *pTuneTime = (uint32_t *)0xB71C3604;
            *pTuneTime = tuneTime;

            if (SA_ctx->ctrlState == SPECAN_STATE_TUNE)
            {
                /* This conditional allows us to handle other asynchronous event, e.g. we've gone to IDLE during a retune. */
                newState = SPECAN_STATE_AGC_UPDATE;
            }

            break;

        case SPECAN_FAIL:
            /* Failure message, stop. */
            newState = SPECAN_STATE_IDLE;
            /* Update any API messages here */
            TVREG_coreWrite(coreInstance, SA_FAILURE_CODE, msgPtr->code);

            /* The state machine doesn't provide for returning to an IDLE state. */

            break;
        default:
            assert(!"Unknown control message");
            break;
        }

        /* Return message to pool */
        KRN_returnPool((void *)msgPtr);

        if (newState != SPECAN_STATE_INVALID)
        {
            recordTimeStamp(pTimeStamps, newState);
        }

        /* Control State handling */
        switch(newState)
        {
        case SPECAN_STATE_INVALID:
            /* do nothing */
            break;

        case SPECAN_STATE_IDLE:
            UCC_LOGMSG("Control task: In IDLE...");
            SPECAN_LOG_EVENT(SPECAN_LOG_CATEGORY_CTRL_STATES, SPECAN_LOGEVENT_CTRLSTATE_IDLE);

            /* Stop the front-end AGC, this will re-start when we move to SCAN_STATE_AGC_UPDATE */
            TVTUNER_stopAGC(tvInstance);
            break;

        case SPECAN_STATE_START:
            UCC_LOGMSG("Control task: Start...");
            SPECAN_LOG_EVENT(SPECAN_LOG_CATEGORY_CTRL_STATES, SPECAN_LOGEVENT_CTRLSTATE_START);

            SPECAN_initAgc(SA_ctx);

            /* Set up for scan */
            if(!SPECAN_setUpForScan(SA_ctx))
            {
                stateChangeRequest = SPECAN_STATE_TUNE;
            }
            else
            {
                /* Flag a failure and update the FSM. */
                SPECAN_QctrlFSMmessage(SA_ctx, SPECAN_FAIL, SA_SCAN_SIZE_EXCEEDS_AVAILABLE_MEMORY);
            }
            break;

        case SPECAN_STATE_TUNE:
            UCC_LOGMSG("Control task: Tuning...");
            SPECAN_LOG_EVENT(SPECAN_LOG_CATEGORY_CTRL_STATES, SPECAN_LOGEVENT_CTRLSTATE_TUNE);
            {
                /* move to a new tuning band. */
                tuneIssueTime = TBI_GETREG(TXTIMER);
                rtn = SPECAN_updateTuner(SA_ctx);
                assert(rtn == 0);   /* A new tuning position should have been checked before getting to this state. */
            }
            break;

        case SPECAN_STATE_AGC_UPDATE:
            UCC_LOGMSG("Control task: Running AGC...");
            SPECAN_LOG_EVENT(SPECAN_LOG_CATEGORY_CTRL_STATES, SPECAN_LOGEVENT_CTRLSTATE_AGC_UPDATE);

            if ((SA_ctx->dcOffsetCtx.enabled && (SA_ctx->dcOffsetCtx.bandFragmentID == SPECAN_BAND_FRAGMENT_SECONDARY)) ||
                    SA_ctx->overrideIFGain)
            {
                /* A retune causes the front end gain to be set to maximum, so reset. */
                rtn = SPECAN_setFrontEndGain(SA_ctx, SA_ctx->frontEndGain);
                assert(rtn == 0);

                stateChangeRequest = SPECAN_STATE_PROC_FRAGMENT; /* Go directly to next state. */
            }
            else
            {
                /* We've retuned to a new band and AGC is enabled so rerun it. */

                /* Reset AGC to a known state */
                SPECAN_resetAGC(SA_ctx);

                /* Restart the AGC, state change triggered on completion. */
                TVTUNER_startAGC(tvInstance);
            }

            break;

        case SPECAN_STATE_PROC_FRAGMENT:
            UCC_LOGMSG("Control task: Processing spectral fragment...");
            SPECAN_LOG_EVENT(SPECAN_LOG_CATEGORY_CTRL_STATES, SPECAN_LOGEVENT_CTRLSTATE_PROC_FRAGMENT);

            SPECAN_processFragment(SA_ctx); /* Triggers the ISR on completion */
            break;

        case SPECAN_STATE_PROC_COMPLETE:
            UCC_LOGMSG("Control task: Spectrum assembly completed.");
            SPECAN_LOG_EVENT(SPECAN_LOG_CATEGORY_CTRL_STATES, SPECAN_LOGEVENT_CTRLSTATE_PROC_SPECTRUM);

            writePeaksToApi(SA_ctx, coreInstance);

            /* Spectrum assembled, pass event to TV API. */
            TV_deliverEvent(coreInstance, TV_EVENT_FOUND);

            recordAcquisitionEnd(pTimeStamps);
            break;

        default:
            assert(!"Unknown control task state");
            break;
        }

        /* Handle any changes to the state machine */
        if (newState != SPECAN_STATE_INVALID)
        {
            SA_ctx->ctrlState = newState;

            if (stateChangeRequest != SPECAN_STATE_INVALID)
            {
                SPECAN_QctrlStateChange(SA_ctx, stateChangeRequest);
            }
        }
    }
}
