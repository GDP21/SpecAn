/*!
 *****************************************************************************

 @file      ucctypes.h
 @brief     Type definitions for UCC objects.

 These are in a separate file because the definitions are mutually recursive

 Copyright (c) Imagination Technologies Limited. 2010

 ****************************************************************************/

#ifndef _UCCTYPES_H_
#define _UCCTYPES_H_
#include <stdbool.h>
#include <stdint.h>

#include "uccDefs.h"
#include "uccint.h"
#include "dcpTypes.h"

#define MAX_UCCS_PER_UCCP (1)
#define MAX_MCPS_PER_UCC  (4)
#define MAX_SCPS_PER_UCC  (4)

#if (MAX_UCCS_PER_UCCP != 1)
#error "Current design assumes only one UCC"
#endif

typedef struct _MCP_TAG MCP_T;
typedef struct _SCP_TAG SCP_T;

/**
 *  Anonymous type describes a UCC
 */
typedef struct
{
    /** UCC id within a UCCP (1..num UCCs) */
    unsigned int id;

    /** Number of MCPs in this UCC */
    unsigned int numMCPs;

    /** Number of SCPs in this UCC */
    unsigned int numSCPs;

    /** Array of pointers to the MCPs belonging to this UCC */
    MCP_T **mcps;

    /** Array of pointers to the SCPs belonging to this UCC */
    SCP_T **scps;

    /** Array of external interrupt numbers for the MCPs belonging to this UCC */
    unsigned int *mcpExtIntNums;

    /** Array of external interrupt numbers for the SCPs belonging to this UCC */
    unsigned int *scpExtIntNums;

    /** Array of peripheral bus offsets for the MCPs belonging to this UCC */
    unsigned int *mcpPMROffsets;

    /** Array of system bus offsets for the MCPs belonging to this UCC */
    unsigned int *mcpSYSOffsets;

    /** Array of peripheral bus offsets for the SCPs belonging to this UCC */
    unsigned int *scpPMROffsets;
} UCC_T;

/**
 * A structure for describing a UCC platform in terms of its constituent UCCs.
 */
typedef struct
{
    /** Number of UCCs in the platform */
    unsigned int numUccs;

    /** Array of pointers to UCC descriptors */
    UCC_T **uccs;

    /** Array of pointers to UCC interrupt system descriptors */
    _UCCINT_SYS_DESC_T **intSysDescs;
} UCCP_T;

/**
 * A structure for conveying information about a UCC platform.
 */
typedef struct
{
    /** Name of the platform */
    char *name;

    /** Major revision number of core */
    uint16_t coreMajorRev;

    /** Minor revision number of core */
    uint16_t coreMinorRev;

    /** Configuration ID of core */
    uint16_t coreConfigId;

    /** Base address of system bus registers (REGSYSBUS) */
    uint32_t sysBusBase;

    /** Base address of global RAM, sign-extended view (MEMGBL_SXT) */
    uint32_t gramSxtBase;

    /** Base address of global RAM, packed view (MEMGBL_PKD) */
    uint32_t gramPkdBase;

    /** Size of global RAM, words */
    uint32_t gramSize;

    /** Size of external RAM */
    uint32_t extRamSize;
} UCCP_INFO_T;

/**
 * Enumeration of MCP waking event source to release the MCP from its WAIT instruction
 * Note, this should not be confused with MCP events which wake the host
 */
typedef enum
{
    /** Event Flag System */
    MCP_EVT_SRC_EFS = 0,
    /** Legacy Event Flag Cluster */
    MCP_EVT_SRC_LEGACY_EFC
} MCP_EVENT_SOURCE_T;

/**
 * Enumeration of MCP events - these are defined in terms of the MCP IRQ status
 * register bits, which allows us to make some internal coding economies by using
 * their values directly in interrupt masks etc.
 */
typedef enum
{
    /** Halt event */
    MCP_EVT_HALT = SYS_MCP_HALT_IEN_MASK, /** WAIT event */
    MCP_EVT_WAIT = SYS_MCP_WAIT_IEN_MASK,
    /** Signal event */
    MCP_EVT_SIGNAL = SYS_MCP_SIGNAL_IEN_MASK,
    /** Event flag event */
    MCP_EVT_EVENT = SYS_MCP_EVENT_IEN_MASK,
#if (__UCC__ == 320)
    /** SCP overrun event */
    MCP_EVT_SCPORUN = SYS_MCP_SCPORUN_IEN_MASK,
#endif /* (__UCC__ == 320) */
    /** MCP stack overflow event */
    MCP_EVT_STACKOFLOW = SYS_MCP_STACKOFLOW_IEN_MASK,
    /** MCP stack underflow event */
    MCP_EVT_STACKUFLOW = SYS_MCP_STACKUFLOW_IEN_MASK,
    /** AFI Overflow event */
    MCP_EVT_AFIFOOFLOW = SYS_MCP_AFIFOOFLOW_IEN_MASK,
    /** AFI Underflow event */
    MCP_EVT_AFIFOUFLOW = SYS_MCP_AFIFOUFLOW_IEN_MASK
} MCP_EVT_T;
/*
 * These two definitions are used internally by the library to optimise event dispatcher table sizes.
 * They are placed here to try to ensure that if the interrupt event numbers are changed, then they will
 * be adjusted accordingly
 */
#define _MCP_EVT_TABLE_SIZE 9
#define _MCP_EVT_TABLE_BASE SYS_MCP_AFIFOUFLOW_IEN_SHIFT /* lowest numbered MCP interrupt bit */

/**
 * Prototype for a MCP event handler function
 *
 * @param[in]   mcp        Handle of the MCP signalling the event
 * @param[in]   event      Event being signalled itself
 */

typedef void
MCP_EVENT_HANDLER_T(MCP_T *mcp, MCP_EVT_T event, void *parameter);

/**
 * MCP event handling dispatcher table entry
 */
typedef struct
{
    /** Pointer to handler function */
    MCP_EVENT_HANDLER_T *isr;
    /** Parameter for handler function */
    void *parameter;
} _MCP_EVENT_ENTRY_T;

/*
 * Enumeration for tracking MCP configuration and memory mapping.
 */
typedef enum
{
    /** Invalid or not set */
    MCP_MAP_INVALID = 0,
    /** MCP is configures in 310 compatibility mode (WIDE/NARROW memory) */
    MCP_MAP_310,
    /** MCP is mapped in 320 mode (all 24-bit memory) */
    MCP_MAP_320
} MCP_MAP_TYPE_T;
/**
 * Anonymous type describes a MCP
 */
struct _MCP_TAG
{
    /* configuration */
    /** pointer to "parent" or "owner" ucc */
    UCC_T *parentUCC;
    /** MCP id within the parent UCC (1..MCP number) */
    unsigned id;
    /* state values */
    /** Logical Id of currently loaded MCP program family member */
    unsigned activeImageId;
    /** Active MCP A region base address mapping value */
    uint32_t ABase;
    /** Active MCP B region base address mapping value */
    uint32_t BBase;
    /** Active MCP L region base address mapping value */
    uint32_t LBase;
    /** Active MCP GRAM mapping type */
    MCP_MAP_TYPE_T mapping;
    /** Unexpected interrupt count - for debugging */
    unsigned unexpectedInterrupts;
    /** Event dispatcher table */
    _MCP_EVENT_ENTRY_T eventTable[_MCP_EVT_TABLE_SIZE];
    /** System bus base address for this MCP */
    /* MCP clock gating control */
    /** Sytem bus address of clock gate control register */
    uint32_t regsClkGate;
    /** Clock gate control register mask */
    uint32_t maskClkGate;
    /** Clock gate control register field shift */
    uint32_t shiftClkGate;
    /* MCP register addresses */
    /** System bus base address for this MCP's control/status registers */
    uint32_t sBase;
    /** Peripheral bus address of GRAM A region base mapping register */
    uint32_t regpABase;
    /** Peripheral bus address of GRAM B region base mapping register */
    uint32_t regpBBase;
    /** Peripheral bus address of GRAM L region base mapping register */
    uint32_t regpLBase;
    /** Peripheral bus address of GRAM B region mapping control register */
    uint32_t regpRemapB;
    /** Peripheral bus address of MCP B region mapping control register */
    uint32_t regpMCPRemapB;
#if (__UCC__ == 320)
    /** Peripheral bus address of MCP wait / interrupt source control register */
    uint32_t regWaitIntSource;
#endif /* (__UCC__ == 320) */
};

/**
 * Enumeration of SCP variants
 */
typedef enum
{
    /** "Legacy" (pre-Volt) 12-bit SCP */
    SCP_VARIANT_LEGACY_12BIT,
    /** 12-bit SCP */
    SCP_VARIANT_12BIT,
    /** 16-bit SCP */
    SCP_VARIANT_16BIT
} SCP_VARIANT_T;

/**
 * Enumeration of SCP events - these are defined in terms of the SCP IRQ status
 * register bits, which allows us to make some internal coding economies by using
 * their values directly in interrupt masks etc.
 */
typedef enum
{
#if __UCC__ >= 420
    /** Energy monitor ready event */
    SCP_EVT_ENERGY = PMB_SCP_ENERGY_ENABLE_MASK,
#endif
    /** TBUS overflow event */
    SCP_EVT_TBUS_OVF = PMB_SCP_TBUS_OVERFLOW_ENABLE_MASK,
    /** Input overflow event */
    SCP_EVT_IOVF = PMB_SCP_OVERFLOW_ENABLE_MASK,
    /** Frame compare event */
    SCP_EVT_COMPARE = PMB_SCP_COMPARE_ENABLE_MASK,
    /** AGC count event */
    SCP_EVT_AGCCOUNT = PMB_SCP_AGC_COUNTS_ENABLE_MASK
} SCP_EVT_T;
/*
 * These two definitions are used internally by the library to optimise event dispatcher table sizes.
 * They are placed here to try to ensure that if the interrupt event numbers are changed, then they will
 * be adjusted accordingly
 */
#if __UCC__ >= 420
#define _SCP_EVT_TABLE_SIZE 5
#define _SCP_EVT_TABLE_BASE PMB_SCP_ENERGY_EVENT_SHIFT /* lowest numbered SCP interrupt bit */
#else
#define _SCP_EVT_TABLE_SIZE 4
#define _SCP_EVT_TABLE_BASE PMB_SCP_TBUS_OVERFLOW_EVENT_SHIFT /* lowest numbered SCP interrupt bit */
#endif
/**
 * Prototype for a SCP event handler function
 *
 * @param[in]   scp        Handle of the SCP signalling the event
 * @param[in]   event      Event being signalled itself
 */

typedef void
SCP_EVENT_HANDLER_T(SCP_T *scp, SCP_EVT_T event, void *parameter);

/**
 * SCP event handling dispatcher table entry
 */
typedef struct
{
    /** Pointer to handler function */
    SCP_EVENT_HANDLER_T *isr;
    /** Parameter for handler function */
    void *parameter;
} _SCP_EVENT_ENTRY_T;

/**
 * Type describing an SCP capture device associated with a specific SCP
 */
typedef struct
{
    /** associated DCP pipeline */
    DCP_PIPELINE_T *scpPipeline;
    /** associated use ID within pipeline */
    int useId;
    /** associated DCP device */
    DCP_DEVICE_T *device;
    /** input address queue ID */
    DCP_PARAM_ID_T inAddrQId;
    /** input capture length queue ID */
    DCP_PARAM_ID_T inCaptureLenQId;
    /** input discard length queue ID */
    DCP_PARAM_ID_T inDiscardLenQId;
    /** output address queue ID */
    DCP_PARAM_ID_T outAddrQId;
    /** output capture length queue ID */
    DCP_PARAM_ID_T outCaptureLenQId;
    /** output discard length queue ID */
    DCP_PARAM_ID_T outDiscardLenQId;
    /** output ISCR queue ID */
    DCP_PARAM_ID_T outIscrQId;
} _SCP_CAPTURE_DRIVER_T;

/**
 * Enumerations for SCP mixer gain
 */
typedef enum
{
    /** 0 dB */
    SCP_MIXER_OUT_GAIN_0DB = 0,
    /** +6 dB */
    SCP_MIXER_OUT_GAIN_6DB = 1,
} SCP_MIXER_GAIN_T;

/**
 * Enumerations for SCP resampler gain
 */
typedef enum
{
    /** -12 dB */
    SCP_RESAMPLE_OUT_GAIN_MINUS12DB = 0,
    /** -6 dB */
    SCP_RESAMPLE_OUT_GAIN_MINUS6DB = 1,
    /** 0 dB */
    SCP_RESAMPLE_OUT_GAIN_0DB = 2,
    /** +6 dB */
    SCP_RESAMPLE_OUT_GAIN_6DB = 3,
} SCP_RESAMPLER_GAIN_T;

/**
 * Enumerations for SCP mixer gain
 */
typedef enum
{
    /** -30 dB */
    SCP_FIR_OUT_GAIN_MINUS30DB = 0,
    /** -24 dB */
    SCP_FIR_OUT_GAIN_MINUS24DB = 1,
    /** -18 dB */
    SCP_FIR_OUT_GAIN_MINUS18DB = 2,
    /** -12 dB */
    SCP_FIR_OUT_GAIN_MINUS12DB = 3,
    /** -6 dB */
    SCP_FIR_OUT_GAIN_MINUS6DB = 4,
    /** 0 dB */
    SCP_FIR_OUT_GAIN_0DB = 5,
} SCP_FIR_GAIN_T;

/**
 * Enumeration for SCP DC Monitor source
 *
 * Don't change these values as they are used internally by SCP support code.
 */
typedef enum
{
    /** Output of DRM decimator */
    SCP_DCMON_DRM = 0,
    /** Output of DC Offset control */
    SCP_DCMON_DCOFF = 1

} SCP_DCMON_SOURCE_T;
/**
 * Anonymous type describes a SCP
 */
struct _SCP_TAG
{
    /* Configuration */
    /** Pointer to "parent" or "owner" UCC */
    UCC_T *parentUCC;
    /** SCP id within the parent UCC (1..SCP number) */
    unsigned id;
    /** associated capture driver */
    _SCP_CAPTURE_DRIVER_T driver;
    /* State values */
    /** use ID associated with capture driver */
    unsigned unexpectedInterrupts;
    /** Event dispatcher table */
    _SCP_EVENT_ENTRY_T eventTable[_SCP_EVT_TABLE_SIZE];
    /** Is it a 16-bit SCP ? */
    bool scp16;
    /** peripheral bus base/offset value for this SCP */
    uint32_t pBase;
    /** System bus address of reset control register */
    uint32_t regsRstCtrl;
    /** Reset bit mask for this SCP in reset control register */
    uint32_t maskRstCtrl;
};

#endif /* _UCCTYPES_H_ */
