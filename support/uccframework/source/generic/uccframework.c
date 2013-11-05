/*
 * uccframework.c
 *
 * UCC application framework - this is a minimal stub pending development of the
 * full multi-tasking framework.
 */
#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <MeOS.h>
#include <uccrt.h>
#include "uccframework.h"

/* Prototype for resource allocation initialisers */
void
_UFW_initAllocators(void);

/* MeOS and QIO globals */
static KRN_SCHEDULE_T sched;

static KRN_TASKQ_T schedQueues[UFW_MAX_MEOS_PRIORITY + 1];

static QIO_SYS_T qioSys;

static QIO_DEVENTRY_T qioDevTab[UFW_MAX_QIO_EXTINTNUM + 1];

#if UFW_MEOS_TIMER_STACK_SIZE > 0
static unsigned timStack[UFW_MEOS_TIMER_STACK_SIZE];
KRN_TASK_T timerTCB;
#endif

#if UFW_MEOS_TRACEBUF_SIZE > 0
static KRN_TRACE_T traceBuf[UFW_MEOS_TRACEBUF_SIZE];
#else
#define traceBuf NULL
#endif

void
UFW_init(void)
{
    /* Resource allocators */
    _UFW_initAllocators();
    /* MeOS */
    KRN_reset(&sched, schedQueues, UFW_MAX_MEOS_PRIORITY,
              UFW_MEOS_STACK_INIT_VALUE, traceBuf, UFW_MEOS_TRACEBUF_SIZE);
    KRN_startOS("UCCAPP-startup-task");
    /* timer task */
#if UFW_MEOS_TIMER_STACK_SIZE > 0
    KRN_startTimerTask("UCCAPP-timer-task", timStack,
                       UFW_MEOS_TIMER_STACK_SIZE, UFW_MEOS_TICK_LENGTH);
#endif
    /* QIO */
    QIO_reset(&qioSys, qioDevTab, UFW_MAX_QIO_EXTINTNUM + 1, UCCP_ivDesc,
              TBID_SIGNUM_TR2(__TBIThreadId() >> TBID_THREAD_S), NULL, 0);
    /* Interrupt chaining hack for Saturn 1 */
#ifdef __UCCP320_2__
     QIO_chainInterrupts(0  /* Parent HWSTATEXT index */,
                         14 /* Parent bit EXT1 */,
                         2  /* Child HWSTATEXT4 index */,
                         2  /* vector value */,
                         UCCP_levelRegs /* HWLEVELEXTn addresses */);
#endif
}

/*
 * This framework stub implements a crude version of the core activation interface
 * which can only activate one application core at a time.
 */
static UFW_DATAMARK_T fastDataMemMark;
static UFW_DATAMARK_T normalDataMemMark;
static UFW_DATAMARK_T uncachedDataMemMark;
static UFW_GRAMMARK_T gramMark;

static void
_UFW_freeInstanceMemory(void)
{
    UFW_memFreeToMark(&normalDataMemMark);
    UFW_memFreeToMark(&fastDataMemMark);
    UFW_memFreeToMark(&uncachedDataMemMark);
    UFW_gramFreeToMark(&gramMark);
}

bool
UFW_activateCore(UFW_COREDESC_T *coreDesc, KRN_PRIORITY_T priority,
                 UFW_COREINSTANCE_T *instance, void *activationParameter)
{

    /* snapshot state of memory pools */
    UFW_memMark(&fastDataMemMark, UFW_MEMORY_TYPE_FAST);
    UFW_memMark(&normalDataMemMark, UFW_MEMORY_TYPE_NORMAL);
    UFW_memMark(&uncachedDataMemMark, UFW_MEMORY_TYPE_UNCACHED);
    UFW_gramMark(&gramMark);
    /* allocate memory for initial data and for register interface block */
    instance->instanceExtensionData = UFW_memAlloc(coreDesc->instanceDataSize,
                                                   UFW_MEMORY_TYPE_NORMAL);
    instance->registerBlock = UFW_memAlloc(coreDesc->numRegisters * sizeof(VREG_T),
                                      UFW_MEMORY_TYPE_NORMAL);
    if (instance->instanceExtensionData && instance->registerBlock)
    {
        instance->coreDesc = coreDesc;
        instance->priority = priority;

        return (*(coreDesc->activateFunction))(instance, activationParameter);
    }
    _UFW_freeInstanceMemory();
    return false;
}

/*----------------------------------------------------------------------*/

bool
UFW_deactivateCore(UFW_COREINSTANCE_T *instance, void *deactivationParameter)
{
    bool status =
            (*(instance->coreDesc->deactivateFunction))(instance,
                                                        deactivationParameter);
    if (status)
        _UFW_freeInstanceMemory();
    return status;
}

