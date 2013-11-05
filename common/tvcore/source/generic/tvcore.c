/*!****************************************************************************
 * @File          tvcore.c
 *
 * @Title         Standard part of a TV core
 *
 * @Date          7 Jan 2011
 *
 * @Copyright     Copyright (C) Imagination Technologies Limited 2011
 *
 * @Description   Implements those parts of a TV core which are common to all TV standards
 *
 ******************************************************************************/

#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <MeOS.h>
#include "uccframework.h"
#include "tvcore.h"

/*
 * shorthands for various event combinations
 */
#define TV_COMMAND_EVENTS_MASK ((1 << TV_EVENT_DETECT_COMMAND) + \
                              (1 << TV_EVENT_RUN_COMMAND) + \
                              (1 << TV_EVENT_STOP_COMMAND) + \
                              (1 << TV_EVENT_UPDATE_RSSI_COMMAND) +\
                              (1 << TV_EVENT_RESET_COUNTERS_COMMAND) +\
                              (1 << TV_EVENT_UPDATE_COUNTERS_COMMAND))
#define TV_STATE_CONTROL_EVENTS_MASK (TV_COMMAND_EVENTS_MASK - \
                                    (1 << TV_EVENT_UPDATE_RSSI_COMMAND) -\
                                    (1 << TV_EVENT_RESET_COUNTERS_COMMAND) - \
                                    (1 << TV_EVENT_UPDATE_COUNTERS_COMMAND) + \
                                    (1 << TV_EVENT_FOUND) +  \
                                    (1 << TV_EVENT_LOST) + \
                                    (1 << TV_EVENT_NOTHING_THERE) + \
                                    (1 << TV_EVENT_ACQFAIL) + \
                                    (1 << TV_EVENT_ACQUIRED))
#define TV_ALL_CONTROL_EVENTS_MASK (~(1 << TV_EVENT_SYNC))

/*
 * other constants
 */
#define MAX_RSSI 32767

/*
 * Local prototypes
 */
static bool
_TV_hostReTune(TV_INSTANCE_T *tvInstance, bool force);
static bool
_TV_localReTune(TV_INSTANCE_T *tvInstance, uint32_t frequency,
                uint32_t bandwidth, bool force);
static bool
_TV_feUpdate(UFW_COREINSTANCE_T *coreInstance);
static bool
_TV_enterInit(TV_INSTANCE_T *tvInst, TV_STATE_T previousState);
static bool
_TV_leaveInit(TV_INSTANCE_T *tvInst, TV_STATE_T nextState);
static bool
_TV_enterDetect(TV_INSTANCE_T *tvInst, TV_STATE_T previousState);
static bool
_TV_leaveDetect(TV_INSTANCE_T *tvInst, TV_STATE_T nextState);
static bool
_TV_leaveRedetecting(TV_INSTANCE_T *tvInst, TV_STATE_T nextState);
static bool
_TV_enterRedetecting(TV_INSTANCE_T *tvInst, TV_STATE_T previousState);
static bool
_TV_leaveAcquiring(TV_INSTANCE_T *tvInst, TV_STATE_T nextState);
static bool
_TV_enterAcquiring(TV_INSTANCE_T *tvInst, TV_STATE_T previousState);
static bool
_TV_leaveDemodulating(TV_INSTANCE_T *tvInst, TV_STATE_T nextState);
static bool
_TV_enterDemodulating(TV_INSTANCE_T *tvInst, TV_STATE_T previousState);
static uint32_t
_TV_griddedFrequency(TV_INSTANCE_T *tvInstance, uint32_t f);

TV_STATEFUNC_T *
getLeaveFunction(unsigned currentState);
TV_STATEFUNC_T *
getEnterFunction(unsigned newState);
int32_t
getTVCoreEvent(uint32_t controlEvents);

/*
 ** FUNCTION:      uffb
 **
 ** DESCRIPTION:   "Unsigned Find First Bit"  - helper function
 **                for fast find of an active flag bit
 **
 ** RETURNS:       number of the most significant "1" bit in n.
 */

#ifdef __META_FAMILY__
inline static int uffb(int n)
{
    /* META FFB doesn't do what we want for negative numbers */
    if (n < 0)
    return 31;
    __asm__ volatile("FFB %0,%1\n\t" : "=f" (n) : "f" (n));
    return n;
}
#else
/* C simulation */
#warning "Using Non-optimsed version of uffb()"
static int
uffb(int n)
{
    int m;
    for (m = 31; m >= 0; m--)
    {
        if (n & 0x80000000)
            return m;
        n <<= 1;
    }
    return -1;
}
#endif
/*----------------------------------------------------------------------------------------*/

/*
 * Local helper functions for main state machine
 */

static void
_TV_changeState(TV_INSTANCE_T *tvInstance, unsigned newState)
{
    /*
     * Work out which leave() and enter() function we need
     *
     *    leave() functions need to know where they are going to
     *
     *    enter() functions need to know where they have came from
     *
     */

    TV_STATEFUNC_T *leave = getLeaveFunction(tvInstance->state);
    TV_STATEFUNC_T *enter = getEnterFunction(newState);

    if (((leave == NULL) || leave(tvInstance, newState))
            && ((enter == NULL) || enter(tvInstance, tvInstance->state)))
    {
        tvInstance->state = newState;

        /*
         * Update state register unless DORMANT. (registers will disappear very soon after going
         * DORMANT, so we don't want to trigger any register processing code).
         */
        if (newState != TV_STATE_DORMANT)
            TVREG_coreWrite(tvInstance->coreInstance, TV_REG_STATE, newState);

        /*
         * sync flag set when we first enter the INITIALISED state - used to synchronise
         * activate/de-activate code with the TV task
         */
        if (newState == TV_STATE_INITIALISED)
            KRN_setFlags(&tvInstance->coreFlags, (1 << TV_EVENT_SYNC));
    }
    else
    {
        assert(0);
        exit(-1);
    }
}
/*
 * This helper function simply returns the appropriate  function pointer for the
 * particular state. This little function simplifies the state machine
 * in TV_mainTask()
 */

TV_STATEFUNC_T *
getLeaveFunction(unsigned currentState)
{
    TV_STATEFUNC_T *leaveFunc = NULL;

    switch (currentState)
    {
    case TV_STATE_INITIALISED:
        leaveFunc = _TV_leaveInit;
        break;
    case TV_STATE_DETECTING:
        leaveFunc = _TV_leaveDetect;
        break;
    case TV_STATE_ACQUIRING:
        leaveFunc = _TV_leaveAcquiring;
        break;
    case TV_STATE_REDETECTING:
        leaveFunc = _TV_leaveRedetecting;
        break;
    case TV_STATE_DEMODULATING:
        leaveFunc = _TV_leaveDemodulating;
        break;
    default: /* handle not found case */
        break;
    }

    return *leaveFunc;
}
/*
 * This helper function simply returns the appropriate function pointer for the
 * particular state. This little function simplifies the state machine
 * in TV_mainTask()
 */

TV_STATEFUNC_T *
getEnterFunction(unsigned newState)
{
    TV_STATEFUNC_T *enterFunc = NULL;

    switch (newState)
    {
    case TV_STATE_INITIALISED:
        enterFunc = _TV_enterInit;
        break;
    case TV_STATE_DETECTING:
        enterFunc = _TV_enterDetect;
        break;
    case TV_STATE_ACQUIRING:
        enterFunc = _TV_enterAcquiring;
        break;
    case TV_STATE_REDETECTING:
        enterFunc = _TV_enterRedetecting;
        break;
    case TV_STATE_DEMODULATING:
        enterFunc = _TV_enterDemodulating;
        break;
    default: /* handle not found case */
        break;
    }

    return *enterFunc;
}

/*----------------------------------------------------------------------------------------*/

/*
 * Common TV activation code
 */

bool
TV_activate(UFW_COREINSTANCE_T *coreInstance, unsigned initialTunerUseId,
            TV_ACTIVATION_PARAMETER_T *tList)
{
    TV_INSTANCE_T *tvInstance = coreInstance->instanceExtensionData;
    TV_COREDESC_EXTENSION_T *tvExt = coreInstance->coreDesc->coreExtension;
    TUNER_ACTIVE_CONFIG_T *tunerConfigs = NULL;
    unsigned numConfigs = 0;

    tvInstance->coreInstance = coreInstance; /* pointer back to "parent" core instance */

    if (!((tvInstance->tvInstanceExtensionData
            = UFW_memAlloc(tvExt->instanceExtensionSize, UFW_MEMORY_TYPE_NORMAL))
            && (coreInstance->coreDesc->apiInitFunction(coreInstance, tList))))
        return false;

    /* sanity check API version setup */
    assert(TVREG_read(coreInstance, TV_REG_API_ID) == TV_REG_API_ID_VALUE);
    assert(TVREG_read(coreInstance, TV_REG_DEMOD_ID) != 0xFFFFFFFF);
    assert(TVREG_read(coreInstance, TV_REG_BUILD_ID) != 0xFFFFFFFF);


    tvInstance->masterStack = UFW_memAlloc(tvExt->mainStackSize
            * sizeof(unsigned), UFW_MEMORY_TYPE_FAST);

    /* allocate GRAM for MCP data area */
    tvInstance->mcpGRAMBase = UFW_gramAlloc(tvExt->mcpGramSize);
    /* allocate a tuner use case */
    tvInstance->tunerUse = TUNER_allocate(tList->tunerUseList,
                                          tList->tunerUseCount,
                                          initialTunerUseId);
    /* ... allocate workspaces for tuner driver */
    if (tvInstance->tunerUse)
    {
        tvInstance->tunerWorkspace = UFW_memAlloc(
                TVTUNER_getWorkspaceSize(tvInstance), UFW_MEMORY_TYPE_NORMAL);
        numConfigs = TVTUNER_getNumConfigs(tvInstance);
        tunerConfigs = UFW_memAlloc(sizeof(TUNER_ACTIVE_CONFIG_T) * numConfigs,
                                    UFW_MEMORY_TYPE_NORMAL);
        /*
         * Allocate UCC, SCP and MCP for use by the standard. At the moment we do simple
         * exclusive allocations. In future when we support multi-standard systems, UCC and MCP
         * may be allocated on a non-exclusive basis.
         */
        /* allocate UCC, and SCP associated with the tuner */
        tvInstance->ucc = UFW_allocUCC(TVTUNER_getUCCId(tvInstance), true);
        tvInstance->scp = UFW_allocSCP(tvInstance->ucc,
                                       TVTUNER_getSCPId(tvInstance), true);
        /* allocate the first MCP on the same ucc */
        tvInstance->mcp = UFW_allocMCP(tvInstance->ucc, 1, true);
    }
    /* check that all allocations were successful */
    if (tvInstance->masterStack && tvInstance->tvInstanceExtensionData
            && (tvInstance->mcpGRAMBase != 0xffffffff) && tvInstance->tunerUse
            && tunerConfigs && tvInstance->tunerWorkspace && tvInstance->ucc
            && tvInstance->mcp && tvInstance->scp)
    {
        /* Initialise context data... */
        KRN_initFlags(&tvInstance->coreFlags); /* common core event flag cluster */
        KRN_initFlags(&tvInstance->userFlags); /* user event flag cluster */
        tvInstance->state = TV_STATE_DORMANT; /* initial master state      */
        tvInstance->hostFrequency = 0;
        tvInstance->hostBandwidth = 0;
        tvInstance->activeFrequency = 0;
        tvInstance->activeBandwidth = 0;
        tvInstance->lastStdFreqRequest = 0;
        tvInstance->lastStdBWRequest = 0;
        /* Start up the tuner */
        TVTUNER_init(tvInstance, tunerConfigs, numConfigs); /* initialise driver */
        TVTUNER_reset(tvInstance, 0); /* reset tuner hardware to config 0 */
        SCP_setExtGainControl1(tvInstance->scp, 0); /* consistent initial values for external gain/offset controls */
        SCP_setExtGainControl2(tvInstance->scp, 0);
        SCP_setExtOffsetControl1(tvInstance->scp, 0);
        SCP_setExtOffsetControl2(tvInstance->scp, 0);
        TVTUNER_initAGC(tvInstance, 0); /* initialise (but don't start) AGC processing */
        return true;
    }
    else
        return false;
}

/*--------------------------------------------------------------------------*/

void
TV_start(UFW_COREINSTANCE_T *coreInstance)
{
    TV_INSTANCE_T *tvInstance = coreInstance->instanceExtensionData;
    TV_COREDESC_EXTENSION_T *tvExt = coreInstance->coreDesc->coreExtension;

    /* clear the synchronisation flag */

    KRN_clearFlags(&tvInstance->coreFlags, (1 << TV_EVENT_SYNC));

    /* start new TV instance main task */

    KRN_startTask(TV_mainTask, &(tvInstance->masterTask),
                  tvInstance->masterStack, tvExt->mainStackSize,
                  coreInstance->priority, coreInstance, "TV Core main task");

    /*
     * Wait for the synchronisation flag to indicate that the main task has
     * actually started (it's set on entry to INITIALISED state)
     */

    KRN_testFlags(&tvInstance->coreFlags, (1 << TV_EVENT_SYNC), KRN_ANY, 1,
                  KRN_INFWAIT);

}
/*--------------------------------------------------------------------------*/

void
TV_stop(UFW_COREINSTANCE_T *coreInstance)
{
    TV_INSTANCE_T *tvInstance = coreInstance->instanceExtensionData;

    if (tvInstance->state == TV_STATE_DORMANT)
        return; /* nothing to do */

    if (tvInstance->state != TV_STATE_INITIALISED)
    {
        /* clear the synchronisation flag */

        KRN_clearFlags(&tvInstance->coreFlags, (1 << TV_EVENT_SYNC));

        /* Command the TV instance to stop */

        TV_deliverEvent(coreInstance, TV_EVENT_STOP_COMMAND);

        /* wait for the STOP command to be processed */

        KRN_testFlags(&tvInstance->coreFlags, (1 << TV_EVENT_SYNC), KRN_ANY, 1,
                      KRN_INFWAIT);
    }

    /* stop the TV instance master task */

    KRN_removeTask(&(tvInstance->masterTask));

    /* Put the TV instance into the DORMANT state */

    _TV_changeState(tvInstance, TV_STATE_DORMANT);

    return;
}

/*--------------------------------------------------------------------------*/

static void
_TV_releaseResources(UFW_COREINSTANCE_T *coreInstance,
                     TV_ACTIVATION_PARAMETER_T *tList)
{
    /*
     * We can rely on the framework to release memory,
     * so this function just needs to deal with non-memory resources
     */
    TV_INSTANCE_T *tvInstance = coreInstance->instanceExtensionData;
    if (tvInstance->scp)
        UFW_freeSCP(tvInstance->scp);
    if (tvInstance->mcp)
        UFW_freeMCP(tvInstance->mcp);
    if (tvInstance->ucc)
        UFW_freeUCC(tvInstance->ucc);
    if (tvInstance->tunerUse)
        TUNER_free(tvInstance->tunerUse, tList->tunerUseList,
                   tList->tunerUseCount);
    return;
}
/*--------------------------------------------------------------------------*/

void
TV_deActivate(UFW_COREINSTANCE_T *coreInstance, void *coreParameter)
{
    TV_INSTANCE_T *tvInstance = coreInstance->instanceExtensionData;
    TV_ACTIVATION_PARAMETER_T *tList = coreParameter;

    MCP_stop(tvInstance->mcp);
    TVTUNER_shutdown(tvInstance);
    _TV_releaseResources(coreInstance, tList);

    /* We can rely on the framework to release memory after this function returns */

    return;
}

/*****************************************************************************
 * The next 6 functions, are all used by the _TV_changeState() function, to
 * leave and enter the various states. There is a distinction made between
 * common code and standard-specific code. Initially, most code will be
 * implemented on a per standard basis, but, with any common code to be added
 * into the appShell tvcore code.
 *****************************************************************************/
static bool
_TV_leaveInit(TV_INSTANCE_T *tvInst, TV_STATE_T nextState)
{
    TV_COREDESC_EXTENSION_T *tv = tvInst->coreInstance->coreDesc->coreExtension;
    bool status = false;

    /* execute standard-specific code first */
    status = tv->leaveInit(tvInst, nextState);

    /* execute common code here */
    if (nextState != TV_STATE_DORMANT)
    {
        /* As the next state is not TV_STATE_RESET, start up the TUNER,
         * ADC and SCP */
        TVTUNER_powerUp(tvInst, 0);
        SCP_setADCPower(tvInst->scp, SCP_PWR_ON);
        SCP_setSCPPower(tvInst->scp, SCP_PWR_ON);

        /*
         * Always reconfigure and retune the tuner with the last known values
         * when leaving the INITIALISED state.
         * This is because some tuners "forget" their settings when powered down
         */
        _TV_hostReTune(tvInst, true);
        /* Set the SCP to a known state */
        TVTUNER_setSCP(tvInst, 0); /* setup SCP to config 0 */
    }

    return status;
}

static bool
_TV_enterInit(TV_INSTANCE_T *tvInst, TV_STATE_T previousState)
{
    TV_COREDESC_EXTENSION_T *tv = tvInst->coreInstance->coreDesc->coreExtension;

    /* execute common code first */

    /* reset the frequency request filters */

    tvInst->lastStdBWRequest = 0;
    tvInst->lastStdFreqRequest = 0;

    /* TUNER/SCP/ADC are powered down on entering the initialised state */
    TVTUNER_powerDown(tvInst, 0);
    SCP_setADCPower(tvInst->scp, SCP_PWR_OFF);
    SCP_setSCPPower(tvInst->scp, SCP_PWR_OFF);

    /* execute standard-specific code */
    if (tv->enterInit(tvInst, previousState))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool
_TV_leaveDetect(TV_INSTANCE_T *tvInst, TV_STATE_T nextState)
{
    TV_COREDESC_EXTENSION_T *tv = tvInst->coreInstance->coreDesc->coreExtension;
    bool status = false;

    /* execute standard-specific code first */
    status = tv->leaveDetect(tvInst, nextState);

    /* execute common code here */
    /* _NO_COMMON_CODE_YET_(); */

    return status;
}

static bool
_TV_enterDetect(TV_INSTANCE_T *tvInst, TV_STATE_T previousState)
{
    TV_COREDESC_EXTENSION_T *tv = tvInst->coreInstance->coreDesc->coreExtension;

    /* execute common code first */
    /* _NO_COMMON_CODE_YET_(); */

    /* execute standard-specific code */
    return (tv->enterDetect(tvInst, previousState));
}

static bool
_TV_leaveRedetecting(TV_INSTANCE_T *tvInst, TV_STATE_T nextState)
{
    TV_COREDESC_EXTENSION_T *tv = tvInst->coreInstance->coreDesc->coreExtension;
    bool status = false;

    /* execute standard-specific code first */
    status = tv->leaveRedetecting(tvInst, nextState);

    /* execute common code here */
    /* _NO_COMMON_CODE_YET_(); */

    return status;
}

static bool
_TV_enterRedetecting(TV_INSTANCE_T *tvInst, TV_STATE_T previousState)
{
    TV_COREDESC_EXTENSION_T *tv = tvInst->coreInstance->coreDesc->coreExtension;

    /* execute common code first */
    /* _NO_COMMON_CODE_YET_(); */

    /* execute standard-specific code */
    return (tv->enterRedetecting(tvInst, previousState));
}

static bool
_TV_leaveAcquiring(TV_INSTANCE_T *tvInst, TV_STATE_T nextState)
{
    TV_COREDESC_EXTENSION_T *tv = tvInst->coreInstance->coreDesc->coreExtension;
    bool status = false;

    /* execute standard-specific code first */
    status = tv->leaveAcquiring(tvInst, nextState);

    /* execute common code here */
    /* _NO_COMMON_CODE_YET_(); */

    return status;
}

static bool
_TV_enterAcquiring(TV_INSTANCE_T *tvInst, TV_STATE_T previousState)
{
    TV_COREDESC_EXTENSION_T *tv = tvInst->coreInstance->coreDesc->coreExtension;

    /* execute common code first */
    /* _NO_COMMON_CODE_YET_(); */

    /* execute standard-specific code */
    return (tv->enterAcquiring(tvInst, previousState));
}

static bool
_TV_leaveDemodulating(TV_INSTANCE_T *tvInst, TV_STATE_T nextState)
{
    TV_COREDESC_EXTENSION_T *tv = tvInst->coreInstance->coreDesc->coreExtension;
    bool status = false;

    /* execute standard-specific code first */
    status = tv->leaveDemodulating(tvInst, nextState);

    /* execute common code here */
    /* _NO_COMMON_CODE_YET_(); */

    return status;
}

static bool
_TV_enterDemodulating(TV_INSTANCE_T *tvInst, TV_STATE_T previousState)
{
    TV_COREDESC_EXTENSION_T *tv = tvInst->coreInstance->coreDesc->coreExtension;

    /* execute common code first */
    /* _NO_COMMON_CODE_YET_(); */

    /* execute standard-specific code */
    return (tv->enterDemodulating(tvInst, previousState));
}

/*--------------------------------------------------------------------------*/

/*
 * Local helper function to call a hook function for each event in a 32-bit cluster.
 * Return value is the (sub)set of events that were considered "cleared" by the hook
 * function calls.
 */
static uint32_t
_TV_runEventHooks(TV_INSTANCE_T *tvInstance, TV_EVENTHOOK_T hookFunction,
                  uint32_t events, int clusterIdOffset)
{
    int event;
    uint32_t clearedEvents = 0;

    /* fast way to find active core events (usually there will only be one) */
    while ((event = uffb(events)) >= 0)
    {
        if (!(hookFunction(tvInstance, event + clusterIdOffset)))
            clearedEvents |= (1 << event);
        events ^= (1 << event);
    }
    return clearedEvents;
}
/*--------------------------------------------------------------------------*/

/*
 * Main task deals with control events including
 * a) State change requests
 * b) Tuner frequency changes
 * c) RSSI update
 * etc
 *
 * Other events are passed on the the core, either implicitly via state changes
 * or explicitly via pre/popst "hook functions
 */

/*--------------------------------------------------------------------------*/

/* COMMANDS:
 *             STOP     is valid in: DETECTING,
 *                                   RE-DETECTING,
 *                                   ACQUIRING,
 *                                   DEMODULATING.
 *
 *             RUN      is valid in: INITIALISED.
 *
 *             DETECT   is valid in: INITIALISED.
 *
 *  EVENTS:
 *             FOUND    is valid in: DETECTING, or
 *                                   RE-DETECTING.
 *
 *             LOST     is valid in: DEMODULATING
 *
 *             NOTHING  is valid in: DETECTING
 *              THERE
 *
 *             ACQFAIL  is valid in: ACQUIRING
 *
 *             ACQUIRED is valid in: ACQUIRING
 *
 */

void
TV_mainTask(void)
{
    UFW_COREINSTANCE_T *coreInstance = KRN_taskParameter(NULL);
    TV_INSTANCE_T *tvInstance = coreInstance->instanceExtensionData;
    TV_COREDESC_EXTENSION_T *tv = coreInstance->coreDesc->coreExtension;

    uint32_t events;
    uint32_t userEvents;
    uint32_t controlEvents;
    int32_t eventFound;

    unsigned newState = 0;

    /* Put the TV instance into the INITIALISED state */

    _TV_changeState(tvInstance, TV_STATE_INITIALISED);

    for (;;)
    {
        tvInstance->cmdACK = tvInstance->freqACK = tvInstance->notchACK = tvInstance->feACK = TV_ACK_INVALID;
        /* test for events in the core cluster */
        events = KRN_testFlags(&tvInstance->coreFlags,
                               TV_ALL_CONTROL_EVENTS_MASK, KRN_ANY, 1,
                               KRN_INFWAIT) & TV_ALL_CONTROL_EVENTS_MASK;
        /* If necessary test for events in the user cluster */
        if (events & (1 << TV_EVENT_USER))
        {
            /* TV_EVENT_USER isn't a real event, so clear it to exclude it from further processing. */
            events ^= (1 << TV_EVENT_USER);
            userEvents = KRN_testFlags(&tvInstance->userFlags, 0xffffffff,
                                       KRN_ANY, 1, KRN_NOWAIT);
        }
        else
        {
            userEvents = 0;
        }

        /* Run pre-event hooks and clear any that have been consumed */
        if (tv->preEventHook)
        {
            events ^= _TV_runEventHooks(tvInstance, tv->preEventHook, events,
                                        0);
            userEvents ^= _TV_runEventHooks(tvInstance, tv->preEventHook,
                                            userEvents, TV_EVENT_USER1);
        }

        controlEvents = events & TV_STATE_CONTROL_EVENTS_MASK;
        while (controlEvents)
        {
            /*
             * It's possible that more than one state change event could appear
             * at the same time. Processing is prioritised to favour those from
             * the controlling application. Beyond that, the favoured transition
             * is the one that takes the system farthest from Acquiring to
             * minimise the chance that the TV is doing something different from
             * what the controlling application believes is happening
             *
             * The event processing is coded to prioritise as follows
             * 1) STOP - from any running state (not Initialised) -> Initialised
             * 2) DETECT from stopped -> detecting
             * 3) RUN from stopped -> Acquiring
             * 4) NOTHING THERE from Detecting -> Re-Detecting
             * 5) LOST from Demodulating -> Acquiring
             * 6) FOUND from Detecting/Re-Detecting -> Acquiring
             * 7) ACQFAIL from Acquiring -> Re-Detecting
             * 8) ACQUIRED from Acquiring -> Demodulating
             */

            eventFound = uffb(controlEvents);
            newState = 0;

            switch (eventFound)
            {
            case TV_EVENT_STOP_COMMAND:
                if (tvInstance->state != TV_STATE_INITIALISED)
                    newState = TV_STATE_INITIALISED;
                break;
            case TV_EVENT_DETECT_COMMAND:
                if (tvInstance->state == TV_STATE_INITIALISED)
                    newState = TV_STATE_DETECTING;
                break;
            case TV_EVENT_RUN_COMMAND:
                if (tvInstance->state == TV_STATE_INITIALISED)
                    newState = TV_STATE_ACQUIRING;
                break;
            case TV_EVENT_FOUND:
                if ((tvInstance->state == TV_STATE_DETECTING)
                        || (tvInstance->state == TV_STATE_REDETECTING))
                    newState = TV_STATE_ACQUIRING;
                break;
            case TV_EVENT_LOST:
                if (tvInstance->state == TV_STATE_DEMODULATING)
                    newState = TV_STATE_ACQUIRING;
                break;
            case TV_EVENT_NOTHING_THERE:
                if (tvInstance->state == TV_STATE_DETECTING)
                    newState = TV_STATE_REDETECTING;
                break;
            case TV_EVENT_ACQFAIL:
                if (tvInstance->state == TV_STATE_ACQUIRING)
                    newState = TV_STATE_REDETECTING;
                break;
            case TV_EVENT_ACQUIRED:
                if (tvInstance->state == TV_STATE_ACQUIRING)
                    newState = TV_STATE_DEMODULATING;
                break;
            default:
                newState = 0; /* Although declared as 0, it
                 * assures us that an TV_ACK_ERR
                 * returns to the controller app */
                break;
            }

            /*
             * Acknowledge the command sent, as required by the command
             * register protocol.
             */
            if (controlEvents & TV_COMMAND_EVENTS_MASK)
                tvInstance->cmdACK = newState ? TV_ACK_OK : TV_ACK_ERR;

            /* if, and only if, the command or event is valid, then approve the
             * state transition...
             */
            if (newState)
            {
                _TV_changeState(tvInstance, newState);
                controlEvents ^= (1 << eventFound);
            }
            else
            { /* ...otherwise, clear all commands and events, because the
             *  behaviour is no longer predictable.
             */
                controlEvents &= ~TV_STATE_CONTROL_EVENTS_MASK;
            }
        }

        if (events & (1 << TV_EVENT_UPDATE_RSSI_COMMAND))
        {
            int rssi;
            int result = TV_ACK_ERR;

            if (tvInstance->state != TV_STATE_INITIALISED)
            {
                rssi = TVTUNER_readRFPower(tvInstance);
                if (rssi < 0)
                    rssi = -1;
                else if (rssi > MAX_RSSI)
                    rssi = MAX_RSSI;
                TVREG_coreWrite(coreInstance, TV_REG_ACTIVE_TUNER_RSSI, rssi);
                result = TV_ACK_OK;
            }
            tvInstance->cmdACK = result;
        }

        if (events & (1 << TV_EVENT_NEW_FREQ))
        {
            /* desired frequency has already been captured from the
             * register before the event is signalled */
            if (tvInstance->state != TV_STATE_INITIALISED)
                _TV_hostReTune(tvInstance, false);

            /* else re-tuning is deferred till _TV_leaveInit() because tuner is
             * currently powered down */

            /*
             * ACK for tune request - we can't reliably report success or failure as
             * ACK_OK/ACK_ERR because re-tuning is sometimes deferred. However,
             * the end-user can always inspect the TV_REG_ACTIVE_TUNER_xx
             * registers, to see what has actually been sent to the tuner.
             */
            tvInstance->freqACK = TV_ACK_OK;
        }

        if (events & (1 << TV_EVENT_FE_UPDATE))
            tvInstance->feACK = _TV_feUpdate(coreInstance) ? TV_ACK_OK : TV_ACK_ERR;

        /*
         * Common core does nothing with a NOTCH request - these are only of interest to the specific TV core
         */
        if (events & (1 << TV_EVENT_NOTCH))
            tvInstance->notchACK = TV_ACK_OK; /* default response standard can override this */

        /*
         * Common core does nothing with these requests - these are simply passed on the the std-specific core
         */
        if ((events & (1 << TV_EVENT_RESET_COUNTERS_COMMAND)) ||
                (events & (1 << TV_EVENT_UPDATE_COUNTERS_COMMAND)) ||
                (events & (1 << TV_EVENT_UPDATE_AGC_COMMAND)) ||
                (events & (1 << TV_EVENT_SET_AGC_COMMAND)))
            tvInstance->cmdACK =  TV_ACK_OK; /* default response standard can override this */
        /*
         * Common core does nothing with a TUNED event - these are only of interest to the specific TV core
         */
#if 0
        if (events & (1 << TV_EVENT_TUNED))
        /* do nothing */;
#endif
        /* call standard-specific extension to post event handling loop */
        if (tv->postEventHook)
            if ((_TV_runEventHooks(tvInstance, tv->postEventHook, events, 0)
                    || _TV_runEventHooks(tvInstance, tv->postEventHook,
                                         userEvents, TV_EVENT_USER1)))
            {
                assert(0);
                exit(-1);
            }
        /* issue any pending command [N]ACK */
        if (tvInstance->cmdACK)
            TVREG_coreWrite(coreInstance, TV_REG_CONTROL,tvInstance->cmdACK);
        if (tvInstance->freqACK)
            TVREG_coreWrite(coreInstance, TV_REG_TUNER_FREQ,tvInstance->freqACK);
        if (tvInstance->notchACK)
            TVREG_coreWrite(coreInstance, TV_REG_NOTCH_FREQ,tvInstance->notchACK);
        if (tvInstance->feACK)
            TVREG_coreWrite(coreInstance, TV_REG_FE_GROUP,tvInstance->cmdACK);
    }
}

/*--------------------------------------------------------------------------*/

static bool
_TV_localReTune(TV_INSTANCE_T *tvInstance, uint32_t frequency,
                uint32_t bandwidth, bool force)
{
    bool result1 = true;
    bool result2 = true;
    /*
     * Local retune function.
     *
     * It's a bit inconvenient that setting the tuner bandwidth is done ultimately
     * via a call to TUNER_configure, which depending on the tuner could be disruptive.
     * At the same time we want to link tuner freq changes to bw changes when they do happen
     * so that the tuner isn't left in an inconsistent state.
     * This is all rather annoying as the bandwidth is unlikely to change after initial setup.
     *
     * Unless "force" is specified, we minimise disruption by optimising away repeated writes of the
     * same value to the tuner frequency and bandwidth controls
     *
     */
    if (force || (bandwidth != tvInstance->activeBandwidth))
        result1 = TVTUNER_configure(tvInstance, bandwidth); /* sets active bandwidth */
    if (force || (frequency != tvInstance->activeFrequency))
        result2 = TVTUNER_tune(tvInstance, frequency); /* sets active frequency */
    TV_deliverEvent(tvInstance->coreInstance, TV_EVENT_TUNED);
    return result1 && result2;
}

/*--------------------------------------------------------------------------*/

static bool
_TV_hostReTune(TV_INSTANCE_T *tvInstance, bool force)
{
    /*
     * Called in response to an update of the tuner frequency and possibly bandwidth
     * register by the host
     */
    if (TVTUNER_tuneIsImplemented(tvInstance))
    {
        /* the tuner is locally managed, so hand over to the local tuner update function */
        return _TV_localReTune(tvInstance, tvInstance->hostFrequency,
                               tvInstance->hostBandwidth, force);
    }
    else
    {
        /*
         * The tuner is (presumably) managed by the host, so just update the active frequency/bw
         * and deliver the TUNED event
         */
        tvInstance->activeBandwidth = tvInstance->hostBandwidth;
        tvInstance->activeFrequency = tvInstance->hostFrequency;
        TV_deliverEvent(tvInstance->coreInstance, TV_EVENT_TUNED);
        return true;
    }
}

/*--------------------------------------------------------------------------*/
static int
sext9(uint32_t x)
{
    if (x & (1 << 9))
        return x | (0xffffffff << 9);
    else
        return x & ~(0xffffffff << 9);
}
static bool
unpackCoeffs(VREG_T *in, int16_t *out)
{
    bool rv = true;
    uint32_t reg;
    int n, m, nn;

    for (n = 0; n < 4; n++)
    {
        reg = VREG_read(in++);
        for (nn = 0; nn < 3; nn++)
        {
            m = sext9(reg);
            *out++ = m;
            reg >>= 10;
            if ((m < -512) || (m > 511))
                rv = false; /* range error */
        }
    }
    return rv;
}
static bool
_TV_feUpdate(UFW_COREINSTANCE_T *coreInstance)
{
    TV_INSTANCE_T *tvInstance = coreInstance->instanceExtensionData;
    unsigned configId = TVREG_read(coreInstance, TV_REG_FE_GROUP);
    TUNER_ACTIVE_CONFIG_T newConfig;
    TUNER_ACTIVE_CONFIG_T *currentConfig;
    unsigned reg;
    unsigned n;
    bool err = false;

    currentConfig = TVTUNER_getActiveConfig(tvInstance, configId);
    if (!currentConfig)
    {
        err = true;
    }
    else
    {
        newConfig = *currentConfig;
        /* read and unpack TVREG_FE_CTRL...*/
        reg = TVREG_read(coreInstance, TV_REG_FE_CTRL);
        newConfig.scpConfig.ADCFormat = reg & 1;
        newConfig.complexIF = (reg >> 8) & 0x1;
        newConfig.spectrumInverted = (reg >> 9) & 0x1;
        /* ...CIC factor needs value checks */
        n = (reg >> 1) & 0xf;
        if (!((n == 1) || (n == 2) || (n == 3) || (n == 4) || (n == 6)
                || (n == 8) || (n == 12) || (n == 16)))
            err = true;
        newConfig.scpConfig.CICFactor = n;
        /* ...FIR Factor needs value checks*/
        n = (reg >> 6) & 0x3;
        if ((n < 1) || (n > 3))
            err = true;
        newConfig.scpConfig.FIRFactor = n;

        /* ADC sample rate */
        newConfig.scpConfig.sampleRate = TVREG_read(coreInstance,
                                                    TV_REG_FE_ADCSAMP);

        /* IF Frequency */
        newConfig.frequencyIF = TVREG_read(coreInstance, TV_REG_FE_IF);

        /* Resampler control value */
        newConfig.scpConfig.resamplerValue = TVREG_read(coreInstance,
                                                        TV_REG_FE_RS);

        /* AGC periods */
        newConfig.scpConfig.normalAGCPeriod = TVREG_read(coreInstance,
                                                         TV_REG_FE_AGC_NORMAL);
        newConfig.scpConfig.rapidAGCPeriod = TVREG_read(coreInstance,
                                                        TV_REG_FE_AGC_FAST);

        /* FIR filter coeffs */
        if (!unpackCoeffs(TVREG_ID2PTR(coreInstance, TV_REG_FE_FIR_NARROW_0),
                          newConfig.scpConfig.narrowBandFIRCoeffs))
            err = true;
        if (!unpackCoeffs(TVREG_ID2PTR(coreInstance, TV_REG_FE_FIR_WIDE_0),
                          newConfig.scpConfig.wideBandFIRCoeffs))
            err = true;
    }
    return err || TVTUNER_updateActiveConfig(tvInstance, configId, &newConfig);
}

/*--------------------------------------------------------------------------*/

/*
 * Event handler for control register updates
 */
static void
controlHandler(VREG_T *reg, void *parameter)
{
    uint32_t rv = VREG_read(reg);
    uint32_t event;

    switch (rv)
    {
    case TV_CMD_STOP:
        event = TV_EVENT_STOP_COMMAND;
        break;
    case TV_CMD_DETECT:
        event = TV_EVENT_DETECT_COMMAND;
        break;
    case TV_CMD_RUN:
        event = TV_EVENT_RUN_COMMAND;
        break;
    case TV_CMD_UPDATE_RSSI:
        event = TV_EVENT_UPDATE_RSSI_COMMAND;
        break;
    case TV_CMD_RESET_COUNTERS:
        event = TV_EVENT_RESET_COUNTERS_COMMAND;
        break;
    case TV_CMD_UPDATE_COUNTERS:
        event = TV_EVENT_UPDATE_COUNTERS_COMMAND;
        break;
    case TV_CMD_UPDATE_AGC:
        event = TV_EVENT_UPDATE_AGC_COMMAND;
        break;
    case TV_CMD_SET_AGC:
        event = TV_EVENT_SET_AGC_COMMAND;
        break;
    default:
        VREG_coreWrite(reg, TV_ACK_ERR);
        return; /* do nothing */
    }
    TV_deliverEvent(parameter, event);
}

/*--------------------------------------------------------------------------*/

static void
tunerFreqHandler(VREG_T *reg, void *parameter)
{
    TV_INSTANCE_T *tvInstance =
            ((UFW_COREINSTANCE_T *)parameter)->instanceExtensionData;

    /* capture the requested tuner frequency value */
    tvInstance->hostFrequency = VREG_read(reg);
    /* signal tune request to background control handling task */
    TV_deliverEvent(parameter, TV_EVENT_NEW_FREQ);
}

/*--------------------------------------------------------------------------*/

static void
tunerBWHandler(VREG_T *reg, void *parameter)
{
    TV_INSTANCE_T *tvInstance =
            ((UFW_COREINSTANCE_T *)parameter)->instanceExtensionData;

    /* capture the requested bandwidth value */
    tvInstance->hostBandwidth = VREG_read(reg);
}

/*--------------------------------------------------------------------------*/

static void
notchHandler(VREG_T *reg, void *parameter)
{
    (void)reg;
    TV_deliverEvent(parameter, TV_EVENT_NOTCH);
}

/*--------------------------------------------------------------------------*/

static void
feUpdateHandler(VREG_T *reg, void *parameter)
{
    (void)reg;
    TV_deliverEvent(parameter, TV_EVENT_FE_UPDATE);
}

/*--------------------------------------------------------------------------*/

void
TV_initRegisterAPI(UFW_COREINSTANCE_T *coreInstance)
{
    int n;
    TV_INSTANCE_T *tvInstance = coreInstance->instanceExtensionData;

    /*
     * Initialise common TV register behaviour and initial values.
     */

    /* API Id/version register */
    TVREG_initValue(coreInstance, TV_REG_API_ID, TV_REG_NULL_ID,
                    TV_REG_API_ID_VALUE);
    /*
     * Demod Id/version register
     *
     * A deliberately bad value in expectation that the core will reinitialise this
     * register. Allows us to test (with an assert) that the core actually sets up its own id
     */
    TVREG_initValue(coreInstance, TV_REG_DEMOD_ID, TV_REG_NULL_ID, 0xFFFFFFFF);

    /*
     * Build Id/version register
     *
     * A deliberately bad value in expectation that the core will reinitialise this
     * register. Allows us to test (with an assert) that the core actually sets up its own id
     */
    TVREG_initValue(coreInstance, TV_REG_BUILD_ID, TV_REG_NULL_ID, 0xFFFFFFFF);

    /* common TV control/status registers */

    TVREG_initValue(coreInstance, TV_REG_CONTROL, TV_REG_NULL_ID, TV_ACK_OK);
    TVREG_initValue(coreInstance, TV_REG_STATE, TV_REG_NULL_ID,
                    TV_STATE_DORMANT);
    TVREG_initValue(coreInstance, TV_REG_TUNER_FREQ, TV_REG_NULL_ID, TV_ACK_OK);
    TVREG_initValue(coreInstance, TV_REG_TUNER_BW, TV_REG_TUNER_FREQ, 0);
    TVREG_initValue(coreInstance, TV_REG_TUNER_GRID_BASE, TV_REG_NULL_ID, 0);
    TVREG_initValue(coreInstance, TV_REG_TUNER_GRID_INCR, TV_REG_NULL_ID, 0);
    TVREG_initValue(coreInstance, TV_REG_NOTCH_FREQ, TV_REG_NULL_ID, TV_ACK_OK);
    TVREG_initValue(coreInstance, TV_REG_NOTCH_BW, TV_REG_NOTCH_FREQ, 0);
    TVREG_initValue(coreInstance, TV_REG_ACTIVE_TUNER_FREQ, TV_REG_NULL_ID, 0);
    TVREG_initValue(coreInstance, TV_REG_ACTIVE_TUNER_BW, TV_REG_NULL_ID, 0);
    TVREG_initValue(coreInstance, TV_REG_ACTIVE_TUNER_GAIN, TV_REG_NULL_ID, 0);
    TVREG_initValue(coreInstance, TV_REG_ACTIVE_TUNER_RSSI, TV_REG_NULL_ID, 0);
    TVREG_initMultiValue(coreInstance, TV_REG_AGC_PARAMS, TV_AGCPAR_NUMPARAMS, tvInstance->agcRegisterBlock, 0);

    /* Front end override registers */
    TVREG_initValue(coreInstance, TV_REG_FE_GROUP, TV_REG_NULL_ID, TV_ACK_OK);
    for (n = TV_REG_FE_CTRL; n <= TV_REG_FE_FIR_WIDE_3; n++)
        TVREG_initValue(coreInstance, n, TV_REG_FE_GROUP, 0);

    /*
     * Set up register event handlers for use within the core.
     * (The wrapper/app sets up its own handlers)
     */
    TVREG_installCoreHandler(coreInstance, TV_REG_CONTROL, controlHandler,
                             coreInstance);
    TVREG_installCoreHandler(coreInstance, TV_REG_TUNER_FREQ, tunerFreqHandler,
                             coreInstance);
    TVREG_installCoreHandler(coreInstance, TV_REG_TUNER_BW, tunerBWHandler,
                             coreInstance);
    TVREG_installCoreHandler(coreInstance, TV_REG_NOTCH_FREQ, notchHandler,
                             coreInstance);
    TVREG_installCoreHandler(coreInstance, TV_REG_FE_GROUP, feUpdateHandler,
                             coreInstance);
}

/*--------------------------------------------------------------------------*/

void
TV_deliverEvent(UFW_COREINSTANCE_T *coreInstance, uint32_t eventId)
{
    TV_INSTANCE_T *tvInstance = coreInstance->instanceExtensionData;

    if (eventId < 32)
    {
        KRN_setFlags(&tvInstance->coreFlags, 1 << eventId);
    }
    else if (eventId < 64)
    {
        KRN_setFlags(&tvInstance->userFlags, 1 << (eventId - 32));
        KRN_setFlags(&tvInstance->coreFlags, 1 << TV_EVENT_USER);
    }
    else
        assert(0);
}

/*--------------------------------------------------------------------------*/
uint32_t
TV_getActiveFrequency(TV_INSTANCE_T *tvInstance)
{
    return tvInstance->activeFrequency;
}

/*--------------------------------------------------------------------------*/
uint32_t
TV_getActiveBandwidth(TV_INSTANCE_T *tvInstance)
{
    return tvInstance->activeBandwidth;
}

/*--------------------------------------------------------------------------*/

static uint32_t
_TV_griddedFrequency(TV_INSTANCE_T *tvInstance, uint32_t f)
{
    uint32_t base, increment, remainder;
    int offset;
    bool negoff = false;

    increment = TVREG_read(tvInstance->coreInstance, TV_REG_TUNER_GRID_INCR);
    if (increment > 1)
    {
        base = TVREG_read(tvInstance->coreInstance, TV_REG_TUNER_GRID_BASE);
        offset = f - base;
        if (offset < 0)
        {
            offset = -offset;
            negoff = true;
        }
        remainder = offset % increment;
        offset -= remainder;
        if (negoff)
        {
            if (remainder > (increment / 2))
                offset = offset + increment;
            return base - offset;
        }
        else
        {
            if (remainder >= (increment / 2))
                offset = offset + increment;
            return base + offset;
        }
    }
    else
        return f;
}

/*--------------------------------------------------------------------------*/

bool
TV_reTune(TV_INSTANCE_T *tvInstance, uint32_t frequency, uint32_t bandwidth,
          bool force)
{
    if (TVTUNER_tuneIsImplemented(tvInstance))
    {
        /* just locally retune */
        return _TV_localReTune(tvInstance, frequency, bandwidth, force);
    }
    else
    {
        uint32_t gf;

        gf = _TV_griddedFrequency(tvInstance, frequency);

        /*
         * In the case of externally managed tuners, we have to maintain our own "previous values"
         * for filtering duplicated requests. This is because updates to the tvInstance activeFrequency
         * and activeBandwidth fields can be substantially delayed, which would lead to "request storms".
         *
         * Note that if EITHER frequency OR bandwidth changes, BOTH are updated and BW is updated first.
         * This means that a host needs to only to monitor the ACTIVER_TUNER_FREQUENCY register to be sure
         * of getting updates to both.
         */
        if (force || gf != tvInstance->lastStdFreqRequest
                || bandwidth != tvInstance->lastStdBWRequest)
        {
            tvInstance->lastStdFreqRequest = gf;
            tvInstance->lastStdBWRequest = bandwidth;
            TVREG_coreWrite(tvInstance->coreInstance, TV_REG_ACTIVE_TUNER_BW,
                            bandwidth);
            TVREG_coreWrite(tvInstance->coreInstance, TV_REG_ACTIVE_TUNER_FREQ,
                            gf);
        }
        else
        {
            /* locally fake a TUNED event to match the filtered request */
            TV_deliverEvent(tvInstance->coreInstance, TV_EVENT_TUNED);
        }
        return true;
    }
}

/*--------------------------------------------------------------------------*/

