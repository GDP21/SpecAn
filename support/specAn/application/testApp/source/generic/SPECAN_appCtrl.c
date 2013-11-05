/*!****************************************************************************
 * @File          SPECAN_appCtrl.c
 *
 * @Title         Application support - control functions
 *
 * @Date          03 Dec 2012
 *
 * @Copyright     Copyright (C) Imagination Technologies Limited 2012
 *
 *
 *******************************************************************************/

#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <MeOS.h>
#include "tvcore.h"
#include "SPECAN_appCtrl.h"
#include "SPECAN_core.h"

/*----------------------------------------------------------------------*/

/*
 * command register checker - waits for previous command to complete and
 * returns the response
 */
static uint32_t _SPECAN_cmdRegPoll(UFW_COREINSTANCE_T *coreInstance, uint32_t regId)
{
    uint32_t r = TVREG_read(coreInstance, regId);
    KRN_PRIORITY_T p;

    while ((r != TV_ACK_OK) && (r != TV_ACK_ERR)) {
        p = KRN_priority(NULL, KRN_LOWEST_PRIORITY);
        KRN_release();
        KRN_priority(NULL, p);
        r = TVREG_read(coreInstance, regId);
    }
    return r;
}

/*----------------------------------------------------------------------*/

void SA_APP_tune(UFW_COREINSTANCE_T *instance, unsigned frequency,
            unsigned bandwidth)
{

    _SPECAN_cmdRegPoll(instance, TV_REG_TUNER_FREQ); /* wait for any prior command to complete */
    TVREG_wrapperWrite(instance, TV_REG_TUNER_BW, bandwidth);
    TVREG_wrapperWrite(instance, TV_REG_TUNER_FREQ, frequency);
    // TEMP ..?
    _SPECAN_cmdRegPoll(instance, TV_REG_TUNER_FREQ);
}


/*----------------------------------------------------------------------*/

bool SA_APP_startScan(UFW_COREINSTANCE_T *instance,
        SPECAN_APP_EVENT_FLAG_T *stateFlag, int timeout)
{
    unsigned state;
    bool result = false;

    state = TVREG_read(instance, TV_REG_STATE);
    if ((state == TV_STATE_COMPLETED) || (state == TV_STATE_DETECTING))
    {
        result = true; /* already running */
    } else if (state == TV_STATE_INITIALISED)
    {
        _SPECAN_cmdRegPoll(instance, TV_REG_CONTROL); /* wait for any prior command to complete */
/*!!!*/ TVREG_wrapperWrite(instance, TV_REG_CONTROL, TV_CMD_DETECT);    // HERE THE SPECTRUM ANALYSERS ENTERS THE DETECTING STATE AND THE SCAN STARTS
        while (KRN_testFlags(stateFlag->flagCluster, stateFlag->flagMask,
                KRN_ALL, 1, timeout)) /* wait for a state change */
        {
            state = TVREG_read(instance, TV_REG_STATE);

            if ((state == TV_STATE_COMPLETED) || (state == TV_STATE_DETECTING))
            {
                /* desired operating state change has happened */
                result = true;
                break;
            }
        }
    }
    return result;
}

/*----------------------------------------------------------------------*/

static void _SA_APP_setEvent(VREG_T *r, void *p)
{
    SPECAN_APP_EVENT_FLAG_T *f = p;
    (void) r;
    KRN_setFlags(f->flagCluster, f->flagMask);
}

/*----------------------------------------------------------------------*/

void SA_APP_mapEF(UFW_COREINSTANCE_T *instance, unsigned regId,
            SPECAN_APP_EVENT_FLAG_T *eventFlag)
{
    TVREG_installWrapperHandler(instance, regId,
            eventFlag ? _SA_APP_setEvent : NULL, eventFlag);
}

