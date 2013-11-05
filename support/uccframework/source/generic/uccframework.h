#ifndef _UCCFRAMEWORK_H_
#define _UCCFRAMEWORK_H_
/*
 * uccframework.h
 *
 * UCC application framework header file.
 */
#include <stdint.h>
#include <stdbool.h>
#include <uccrt.h>
#include <MeOS.h>

#include "ufwconfig.h"

#ifndef UFW_NORMAL_POOL_BYTES
#error UCC Framework configuration constant UFW_NORMAL_POOL_BYTES not defined.
#endif
#ifndef UFW_FAST_POOL_BYTES
#error UCC Framework configuration constant UFW_FAST_POOL_BYTES not defined.
#endif
#ifndef UFW_UNCACHED_POOL_BYTES
#error UCC Framework configuration constant UFW_UNCACHED_POOL_BYTES not defined.
#endif
#ifndef UFW_GRAM_POOL_WORDS
#error UCC Framework configuration constant UFW_GRAM_POOL_WORDS not defined.
#endif
#ifndef UFW_MAX_MEOS_PRIORITY
#error UCC Framework configuration constant UFW_MAX_MEOS_PRIORITY not defined.
#endif
#ifndef UFW_MAX_QIO_EXTINTNUM
#error UCC Framework configuration constant UFW_MAX_QIO_EXTINTNUM not defined.
#endif
#ifndef UFW_MEOS_TRACEBUF_SIZE
#error UCC Framework configuration constant UFW_MEOS_TRACEBUF_SIZE not defined.
#endif
#ifndef UFW_MEOS_TIMER_STACK_SIZE
#error UCC Framework configuration constant UFW_MEOS_TIMER_STACK_SIZE not defined.
#endif
#ifndef UFW_MEOS_STACK_INIT_VALUE
#error UCC Framework configuration constant UFW_MEOS_STACK_INIT_VALUE not defined.
#endif
#ifndef UFW_MEOS_TICK_LENGTH
#error UCC Framework configuration constant UFW_MEOS_TICK_LENGTH not defined.
#endif

#define SEC2TICKS(S) (((S) * 1000000)/UFW_MEOS_TICK_LENGTH)
#define MS2TICKS(S) (((S) * 1000)/UFW_MEOS_TICK_LENGTH)

void
UFW_init(void);

/* data types for pool state "marks" */
typedef enum
{
    UFW_MEMORY_TYPE_FAST, UFW_MEMORY_TYPE_NORMAL, UFW_MEMORY_TYPE_UNCACHED
} UFW_MEMORY_TYPE_T;

typedef struct
{
    UFW_MEMORY_TYPE_T memType;
    uint8_t *top;
    unsigned freeSize;
} UFW_DATAMARK_T;

typedef struct
{
    uint32_t *top;
    unsigned freeSize;
} UFW_GRAMMARK_T;

/*
 * Simple memory allocator functions. At the moment, memory of each type  must
 * be released strictly in the opposite order to which it is allocated (stack-wise).
 *
 * A group of consecutive allocations may be released in one go by using
 * UFW_memMark and UFW_memFreeToMark.
 *
 * Prior to starting an application core, the framework "marks" the memory pools
 * and after shutting down an app core, the framework does a "free to mark" on each pool.
 * So, a simple application simply needs to allocate memory at startup and can rely on the
 * framework to release the memory on completion.
 */
void *
UFW_memAlloc(unsigned memSize, UFW_MEMORY_TYPE_T memType);
void
UFW_memFree(void *memPtr, unsigned memSize, UFW_MEMORY_TYPE_T memType);
void
UFW_memFreeToMark(UFW_DATAMARK_T *mark);
void
UFW_memMark(UFW_DATAMARK_T *mark, UFW_MEMORY_TYPE_T memType);
/*
 * Corresponding set of allocation functions for GRAM.
 *
 * Note that the memory size specified to these functions is the number of GRAM *words*
 * (unlike the UFW_memxxx functions, in which memory size is specified in bytes).
 *
 * Also note that while UFW_memAlloc returns a C pointer, UFW_gramAlloc returns a UCCP_GRAM_ADDRESS_T,
 * which is an integer type representing a zero-based GRAM address - I.e. an offset in words from
 * the start of GRAM.
 */
UCCP_GRAM_ADDRESS_T
UFW_gramAlloc(unsigned memSize);
void
UFW_gramFree(UCCP_GRAM_ADDRESS_T memPtr, unsigned memSize);
void
UFW_gramFreeToMark(UFW_GRAMMARK_T *mark);
void
UFW_gramMark(UFW_GRAMMARK_T *mark);

/* simple resource allocator/deallocator functions for MCP,SCP/TUNER */
/*
 * Allocate a particular UCC - the API allows the caller to specify a particular UCC
 * or, to let the framework choose by specifying uccNumber = 0. The UCC may be reserved
 * for exclusive use by the core or sharing may be permitted - for example when multiple
 * application cores are running.
 */
UCC_T *
UFW_allocUCC(unsigned uccNumber, bool exclusive);
/*
 * Release a UCC.
 *
 * Note - this does NOT recursively release any allocated MCPs or SCPs - these should be
 * explicitly freed first.
 */
void
UFW_freeUCC(UCC_T *ucc);
/*
 * Allocate a particular MCP - the API allows the caller to specify a particular MCP
 * or, to let the framework choose by specifying mcpNumber = 0. The MCP may be reserved
 * for exclusive use by the core or sharing may be permitted - for example when the MCP software
 * multi-tasks to support more than one application core.
 */
MCP_T *
UFW_allocMCP(UCC_T *ucc, unsigned mcpNumber, bool exclusive);
/*
 * Release a MCP.
 */
void
UFW_freeMCP(MCP_T *mcp);
/*
 * Allocate a particular SCP - the API allows the caller to specify a particular SCP
 * or, to let the framework choose by specifying scpNumber = 0. The SCP may be reserved
 * for exclusive use by the core or sharing may be permitted. In practice, it is not
 * often sensible to share a SCP.
 */
SCP_T *
UFW_allocSCP(UCC_T *ucc, unsigned scpNumber, bool exclusive);
/*
 * Release a SCP.
 */
void
UFW_freeSCP(SCP_T *scp);

/* Support for the CORE+WRAPPER application model */

typedef struct _UFW_coreInstanceTag UFW_COREINSTANCE_T;

typedef bool
UFW_CONTROL_T(UFW_COREINSTANCE_T *appInstance, void *coreSpecificParameter);

typedef const struct
{
    unsigned instanceDataSize;
    unsigned numRegisters;
    UFW_CONTROL_T *apiInitFunction;
    UFW_CONTROL_T *activateFunction;
    UFW_CONTROL_T *deactivateFunction;
    void *coreExtension;
} UFW_COREDESC_T;

struct _UFW_coreInstanceTag
{
    UFW_COREDESC_T *coreDesc;
    void *instanceExtensionData;
    VREG_T *registerBlock;
    KRN_PRIORITY_T priority;
};

bool
UFW_activateCore(UFW_COREDESC_T *coreDesc, KRN_PRIORITY_T priority,
                 UFW_COREINSTANCE_T *instance, void *activationParameter);

bool
UFW_deactivateCore(UFW_COREINSTANCE_T *coreInstance, void *deactivationParameter);

#endif /* _UCCFRAMEWORK_H_ */
