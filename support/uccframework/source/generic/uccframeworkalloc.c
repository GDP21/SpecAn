/*
 * uccframeworkalloc.c
 *
 * Resource allocation functions provided by the framework
 */
#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <uccrt.h>
#include "uccframework.h"

/*--------------------------------------------------------------------------------*/

typedef struct
{
    uint8_t *top;
    uint8_t *base;
    unsigned freeSize;
    unsigned peakUseBytes;
} DATA_POOL_T;

typedef struct
{
    uint32_t *top;
    uint32_t *base;
    unsigned freeSize;
    unsigned peakUseWords;
} GRAM_POOL_T;

/* Pool descriptors are public variables so that use levels can easily be inspected while debugging */

DATA_POOL_T UFW_normalPoolDescriptor;
DATA_POOL_T UFW_fastPoolDescriptor;
DATA_POOL_T UFW_uncachedPoolDescriptor;
GRAM_POOL_T UFW_gramPoolDescriptor;

typedef struct
{
    bool allocated;
    bool exclusive;
    unsigned shareCount;
} _ALLOCATOR_T;

static _ALLOCATOR_T uccAllocations[MAX_UCCS_PER_UCCP];
static _ALLOCATOR_T mcpAllocations[MAX_UCCS_PER_UCCP][MAX_MCPS_PER_UCC];
static _ALLOCATOR_T scpAllocations[MAX_UCCS_PER_UCCP][MAX_SCPS_PER_UCC];

#if (MAX_UCCS_PER_UCCP != 1)
#error "Broken design assumption"
#endif
/*
 * Pool data arrays - these are defined in dedicated sections so they can be explicitly
 * EXTRACTed and placed by the ldlk system build script (.IMG file).
 *
 * The data arrays are also contained in a a union with a 64-bit data object. This
 * makes the compiler align the pool on an 8-byte address boundary.
 */
static union
{
    uint64_t dummy;
    uint8_t data[UFW_FAST_POOL_BYTES];
} _UFW_fastPool;

static union
{
    uint64_t dummy;
    uint8_t data[UFW_NORMAL_POOL_BYTES];
}
_UFW_normalPool;

static union
{
    uint64_t dummy;
    uint8_t data[UFW_UNCACHED_POOL_BYTES];
}
_UFW_uncachedPool;

static union
{
    uint64_t dummy;
    uint32_t data[UFW_GRAM_POOL_WORDS];
}
_UFW_gramPool;

/*--------------------------------------------------------------------------------*/

static void
initDataPool(DATA_POOL_T *pool, uint8_t *start, unsigned size)
{
    pool->base = pool->top = start;
    pool->freeSize = size;
    /* Ensure allocations are made on 8-byte boundaries, sacrifice a few bytes if necessary  */
    while (((unsigned)pool->top) & 7)
    {
        assert(0); /* pool should be aligned by build process, so this correction should be unnecessary */
        pool->top++;
        pool->freeSize--;
    }
    pool->peakUseBytes = pool->top - pool->base;
}

/*--------------------------------------------------------------------------------*/

static void
initGRAMPool(GRAM_POOL_T *pool, uint32_t *start, unsigned size)
{
    pool->base = pool->top = start;
    pool->freeSize = size;
    /*
     * Ensure allocations are made on an 8 word boundary in  GRAM address space or
     * a 32 byte boundary in META address space. Sacrifice a few words if necessary
     */
    /* at least 4 byte alignment assumed */
    assert((((uint32_t)(pool->top)) & 0x7) == 0);
    while (((unsigned)pool->top) & 31)
    {
        pool->top++;
        pool->freeSize--;
    }
    pool->peakUseWords = pool->top - pool->base;
}

/*--------------------------------------------------------------------------------*/

void
_UFW_initAllocators(void)
{
    initDataPool(&UFW_normalPoolDescriptor, _UFW_normalPool.data, UFW_NORMAL_POOL_BYTES);
    initDataPool(&UFW_fastPoolDescriptor, _UFW_fastPool.data, UFW_FAST_POOL_BYTES);
    initDataPool(&UFW_uncachedPoolDescriptor, _UFW_uncachedPool.data, UFW_UNCACHED_POOL_BYTES);
    initGRAMPool(&UFW_gramPoolDescriptor, _UFW_gramPool.data, UFW_GRAM_POOL_WORDS);
    memset(uccAllocations, 0, sizeof(uccAllocations));
    memset(mcpAllocations, 0, sizeof(mcpAllocations));
    memset(scpAllocations, 0, sizeof(scpAllocations));
}

/*--------------------------------------------------------------------------------*/
static DATA_POOL_T *
_UFW_dataPool(UFW_MEMORY_TYPE_T memType)
{
    if (memType == UFW_MEMORY_TYPE_NORMAL)
        return &UFW_normalPoolDescriptor;
    else if (memType == UFW_MEMORY_TYPE_FAST)
        return &UFW_fastPoolDescriptor;
    else if (memType == UFW_MEMORY_TYPE_UNCACHED)
        return &UFW_uncachedPoolDescriptor;
    else
        return NULL;
}

void *
UFW_memAlloc(unsigned memSize, UFW_MEMORY_TYPE_T memType)
{
    KRN_IPL_T oldipl;
    void *poolPtr = NULL;
    DATA_POOL_T *p;
    unsigned use;

    p = _UFW_dataPool(memType);

    /* round up size to preserve 8-byte alignment */
    assert(p);
    memSize = (memSize + 7) & ~7;
    oldipl = KRN_raiseIPL();
    if (memSize <= p->freeSize)
    {
        poolPtr = p->top;
        p->top += memSize;
        p->freeSize -= memSize;
        use = p->top - p->base;
        if (use > p->peakUseBytes)
            p->peakUseBytes = use;
    }
    KRN_restoreIPL(oldipl);
    if (poolPtr)
        memset(poolPtr, 0, memSize);
    return poolPtr;
}

/*--------------------------------------------------------------------------------*/

void
UFW_memFree(void *memPtr, unsigned memSize, UFW_MEMORY_TYPE_T memType)
{
    DATA_POOL_T *p;
    KRN_IPL_T oldipl;

    p = _UFW_dataPool(memType);
    assert(p);
    memSize = (memSize + 7) & ~7;
    oldipl = KRN_raiseIPL();
    if (p->top == (memPtr + memSize))
    {
        p->top -= memSize;
        p->freeSize += memSize;
    }
    else
    {
        /* we only currently support a stack based approach to allocation
         * An attempt to deallocate in the wrong order is simply ignored - the system
         * will eventually die when subsequent allocations fail
         */
        /* do nothing */;
    }
    KRN_restoreIPL(oldipl);
}

/*--------------------------------------------------------------------------------*/

void
UFW_memFreeToMark(UFW_DATAMARK_T *mark)
{
    KRN_IPL_T oldipl;
    DATA_POOL_T *p;

    p = _UFW_dataPool(mark->memType);
    assert(p);
    oldipl = KRN_raiseIPL();
    p->top = mark->top;
    p->freeSize = mark->freeSize;
    KRN_restoreIPL(oldipl);
}

/*--------------------------------------------------------------------------------*/

void
UFW_memMark(UFW_DATAMARK_T *mark, UFW_MEMORY_TYPE_T memType)
{

    KRN_IPL_T oldipl;
    DATA_POOL_T *p;

    p = _UFW_dataPool(memType);
    assert(p);
    oldipl = KRN_raiseIPL();
    mark->top = p->top;
    mark->freeSize = p->freeSize;
    mark->memType = memType;
    KRN_restoreIPL(oldipl);
}

/*--------------------------------------------------------------------------------*/

UCCP_GRAM_ADDRESS_T
UFW_gramAlloc(unsigned memSize)
{
    KRN_IPL_T oldipl;
    uint32_t *poolPtr = NULL;
    UCCP_GRAM_ADDRESS_T rv = 0xffffffff;
    unsigned use;

    oldipl = KRN_raiseIPL();
    /*
     * Round up to a multiple of 8 GRAM words, to ensure that the allocation is also
     * 32-byte aligned in the packed GRAM view of memory.
     */
    memSize = (memSize + 7) & ~7;
    if (memSize <= UFW_gramPoolDescriptor.freeSize)
    {
        poolPtr = UFW_gramPoolDescriptor.top;
        UFW_gramPoolDescriptor.top += memSize;
        UFW_gramPoolDescriptor.freeSize -= memSize;
        use = UFW_gramPoolDescriptor.top - UFW_gramPoolDescriptor.base;
        if (use > UFW_gramPoolDescriptor.peakUseWords)
            UFW_gramPoolDescriptor.peakUseWords = use;
    }
    KRN_restoreIPL(oldipl);
    /* convert to a GRAM address and zero the memory  */
    if (poolPtr)
    {
        rv = poolPtr - ((uint32_t *)MEMGBL_DBL);
        memset((void *)(MEMGBL_PKD + 3 * rv), 0, 3 * memSize);
    }
    return rv;
}

/*--------------------------------------------------------------------------------*/

void
UFW_gramFree(UCCP_GRAM_ADDRESS_T gramAddress, unsigned memSize)
{
    KRN_IPL_T oldipl;
    uint32_t *gramPtr;

    memSize = (memSize + 7) & ~7;
    /* convert from GRAM address to META address */
    gramPtr = ((uint32_t *)MEMGBL_DBL) + gramAddress;
    oldipl = KRN_raiseIPL();
    if (UFW_gramPoolDescriptor.top == (gramPtr + memSize))
    {
        UFW_gramPoolDescriptor.top -= memSize;
        UFW_gramPoolDescriptor.freeSize += memSize;
    }
    else
    {
        /* we only currently support a stack based approach to allocation
         * An attempt to deallocate in the wrong order is simply ignored - the system
         * will eventually die when subsequent allocations fail
         */
        /* do nothing */;
    }
    KRN_restoreIPL(oldipl);
}

/*--------------------------------------------------------------------------------*/

void
UFW_gramFreeToMark(UFW_GRAMMARK_T *mark)
{
    KRN_IPL_T oldipl;

    oldipl = KRN_raiseIPL();
    UFW_gramPoolDescriptor.top = mark->top;
    UFW_gramPoolDescriptor.freeSize = mark->freeSize;
    KRN_restoreIPL(oldipl);
}

/*--------------------------------------------------------------------------------*/

void
UFW_gramMark(UFW_GRAMMARK_T *mark)
{
    KRN_IPL_T oldipl;

    oldipl = KRN_raiseIPL();
    mark->top = UFW_gramPoolDescriptor.top;
    mark->freeSize = UFW_gramPoolDescriptor.freeSize;
    KRN_restoreIPL(oldipl);
}

/*--------------------------------------------------------------------------------*/

UCC_T *
UFW_allocUCC(unsigned uccNumber, bool exclusive)
{
    KRN_IPL_T oldipl;
    _ALLOCATOR_T *ua;
    UCC_T *ucc = NULL;
    if (uccNumber > 1)
        return NULL;
    ua = &uccAllocations[0];
    oldipl = KRN_raiseIPL();
    if (!ua->allocated)
    {
        ua->allocated = true;
        ua->exclusive = exclusive;
        ua->shareCount = 1;
        ucc = UCCP_getUCC(1);
    }
    else if (!(ua->exclusive || exclusive))
    {
        ua->shareCount++;
        ucc = UCCP_getUCC(1);
    }
    KRN_restoreIPL(oldipl);
    return ucc;
}

/*--------------------------------------------------------------------------------*/

void
UFW_freeUCC(UCC_T *ucc)
{
    KRN_IPL_T oldipl;
    _ALLOCATOR_T *ua;
    int n = UCC_id(ucc) - 1;

    (void)ucc;
    ua = &uccAllocations[n];
    oldipl = KRN_raiseIPL();
    if (--(ua->shareCount) == 0)
    {
        ua->allocated = false;
        ua->exclusive = false;
    }
    KRN_restoreIPL(oldipl);
}

/*--------------------------------------------------------------------------------*/

MCP_T *
UFW_allocMCP(UCC_T *ucc, unsigned mcpNumber, bool exclusive)
{
    KRN_IPL_T oldipl;
    _ALLOCATOR_T *m;
    int n, u;
    int nMin, nMax;

    if (mcpNumber)
    {
        nMin = nMax = mcpNumber - 1;
    }
    else
    {
        nMin = 0;
        nMax = MAX_SCPS_PER_UCC - 1;
    }
    mcpNumber = 0;
    u = UCC_id(ucc) - 1;

    for (n = nMin; (n <= nMax) && !mcpNumber; n++)
    {
        m = &mcpAllocations[u][n];
        oldipl = KRN_raiseIPL();
        if (!m->allocated)
        {
            m->allocated = true;
            m->exclusive = exclusive;
            m->shareCount = 1;
            mcpNumber = n + 1;
        }
        else if (!(m->exclusive || exclusive))
        {
            m->shareCount++;
            mcpNumber = n + 1;
        }
        KRN_restoreIPL(oldipl);
    }
    return UCC_getMCP(ucc, mcpNumber);
}

/*--------------------------------------------------------------------------------*/

void
UFW_freeMCP(MCP_T *mcp)
{
    KRN_IPL_T oldipl;
    int n = MCP_id(mcp) - 1;
    int u = UCC_id(MCP_parent(mcp)) - 1;
    _ALLOCATOR_T *m = &mcpAllocations[u][n];

    oldipl = KRN_raiseIPL();
    if (--(m->shareCount) == 0)
    {
        m->allocated = false;
        m->exclusive = false;
    }
    KRN_restoreIPL(oldipl);
}

/*--------------------------------------------------------------------------------*/

SCP_T *
UFW_allocSCP(UCC_T *ucc, unsigned scpNumber, bool exclusive)
{
    KRN_IPL_T oldipl;
    _ALLOCATOR_T *s;
    int n, u;
    int nMin, nMax;

    if (scpNumber)
    {
        nMin = nMax = scpNumber - 1;
    }
    else
    {
        nMin = 0;
        nMax = MAX_SCPS_PER_UCC - 1;
    }
    scpNumber = 0;
    u = UCC_id(ucc) - 1;

    for (n = nMin; (n <= nMax) && !scpNumber; n++)
    {
        s = &scpAllocations[u][n];
        oldipl = KRN_raiseIPL();
        if (!s->allocated)
        {
            s->allocated = true;
            s->exclusive = exclusive;
            s->shareCount = 1;
            scpNumber = n + 1;
        }
        else if (!(s->exclusive || exclusive))
        {
            s->shareCount++;
            scpNumber = n + 1;
        }
        KRN_restoreIPL(oldipl);
    }
    return UCC_getSCP(ucc, scpNumber);
}

/*--------------------------------------------------------------------------------*/

void
UFW_freeSCP(SCP_T *scp)
{
    KRN_IPL_T oldipl;
    int n = SCP_id(scp) - 1;
    int u = UCC_id(SCP_parent(scp)) - 1;
    _ALLOCATOR_T *s = &scpAllocations[u][n];

    oldipl = KRN_raiseIPL();
    if (--(s->shareCount) == 0)
    {
        s->allocated = false;
        s->exclusive = false;
    }
    KRN_restoreIPL(oldipl);
}

