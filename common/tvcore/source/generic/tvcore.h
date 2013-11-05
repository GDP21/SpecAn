/*!****************************************************************************
 * @File          tvcore.h
 *
 * @Title         Header file for standard part of a TV core
 *
 * @Date          7 Jan 2011
 *
 * @Copyright     Copyright (C) Imagination Technologies Limited 2011
 *
 * @Description   Defines those parts of a TV core which are common to all TV standards
 *
 ******************************************************************************/

#ifndef _TVCORE_H_
#define _TVCORE_H_
#include <uccrt.h>
#include "uccframework.h"

/**
 * Type definition for a TV Core activation parameter
 * TV applications use the activation parameter to pass a list of available
 * tuners at run-time, so the core need not be rebuilt for different tuner
 * configurations.
 */
typedef struct
{
    /** Number of entries in tuner use list */
    int tunerUseCount;
    /** Pointer to an array of tuner use case definitions */
    TUNER_USE_T *tunerUseList;
} TV_ACTIVATION_PARAMETER_T;

/**
 * Common TV API version
 */

#define TV_API_MAJOR_VERSION 0x03
#define TV_API_MINOR_VERSION 0x01
#define TV_REG_API_ID_VALUE (((TV_API_MAJOR_VERSION) << 8) | (TV_API_MINOR_VERSION))

/**
 * Macro for constructing TV-specific demod Ids
 */

#define TV_DEMOD_ID(STD, MAJ, MIN) (((STD) << 16) | ((MAJ) << 8) | (MIN))
/**
 * Common TV register Ids - numbered consecutively from 0
 *
 * Since these registers define an external interface, and may be accessed from non-C environments,
 * it's important not to change them unnecessarily. Don't re-order
 */
enum
{
    /* Identification registers - the code depends on these two being first and in this order */
    TV_REG_API_ID = 0,
    TV_REG_DEMOD_ID,
    TV_REG_BUILD_ID,
    /* standard control/state registers */
    TV_REG_CONTROL,
    TV_REG_STATE,
    /* standard tuner control */
    TV_REG_TUNER_FREQ,
    TV_REG_TUNER_BW, TV_REG_TUNER_GRID_BASE, TV_REG_TUNER_GRID_INCR,
    /* standard external notch request registers */
    TV_REG_NOTCH_FREQ,
    TV_REG_NOTCH_BW,
    /* standard tuner status registers */
    TV_REG_ACTIVE_TUNER_FREQ,
    TV_REG_ACTIVE_TUNER_BW, TV_REG_ACTIVE_TUNER_GAIN, TV_REG_ACTIVE_TUNER_RSSI,
    /* AGC parameter register (multi-valued) */
    TV_REG_AGC_PARAMS,
    /* Front end SCP override registers */
    TV_REG_FE_GROUP,
    TV_REG_FE_CTRL, TV_REG_FE_ADCSAMP, TV_REG_FE_IF, TV_REG_FE_RS,
    TV_REG_FE_AGC_NORMAL, TV_REG_FE_AGC_FAST, TV_REG_FE_FIR_NARROW_0,
    TV_REG_FE_FIR_NARROW_1, TV_REG_FE_FIR_NARROW_2, TV_REG_FE_FIR_NARROW_3,
    TV_REG_FE_FIR_WIDE_0, TV_REG_FE_FIR_WIDE_1, TV_REG_FE_FIR_WIDE_2,
    TV_REG_FE_FIR_WIDE_3,
    /* total number of common TV API registers */
    TV_REG_NUM_COMMON_REG
};

/*
 * First register Id for use by TV standards
 */
#define TV_REG_FIRST_STD_ID 0x8000
/*
 * A reserved value to mean "no register"
 */
#define TV_REG_NULL_ID 0xFFFFFFFF
/*
 * Common TV control commands - numbered consecutively from 0 (NULL command)
 */
typedef enum
{
    TV_CMD_NULL = 0, TV_CMD_STOP, TV_CMD_DETECT, TV_CMD_RUN, TV_CMD_UPDATE_RSSI,
    TV_CMD_RESET_COUNTERS, TV_CMD_UPDATE_COUNTERS, TV_CMD_SET_AGC,
    TV_CMD_UPDATE_AGC, TV_CMD_NUMCMDS
} TV_CMD_T;
/*
 * Common TV register acknowledge codes
 */
typedef enum
{
    TV_ACK_OK = 0xffffffff, TV_ACK_ERR = 0xefffffff, TV_ACK_INVALID = 0
} TV_ACK_T;
/**
 * Common TV state definitions - numbered consecutively from 0 (RESET)
 */
typedef enum
{
    TV_STATE_DORMANT = 0, // 0 /* TV Core state machine needs this value to be "0" */
    TV_STATE_INITIALISED, // 1
    TV_STATE_DETECTING, // 2
    TV_STATE_ACQUIRING, // 3
    TV_STATE_REDETECTING, // 4
    TV_STATE_DEMODULATING, // 5
    TV_NUMSTATES
} TV_STATE_T;
/**
 * Definitions for the TV_REG_NOTCH_STATUS register
 */
enum
{
    /** Notch can't be set */
    TV_NOTCH_INACTIVE = 0,
    /** requested notch is active */
    TV_NOTCH_ACTIVE
};
/**
 * AGC register index values
 */
typedef enum
{
    TV_AGCPAR_ERR_SCALEPOSMUL, TV_AGCPAR_ERR_SCALEPOSSHIFT,
    TV_AGCPAR_ERR_SCALENEGMUL, TV_AGCPAR_ERR_SCALENEGSHIFT, TV_AGCPAR_FILT_MODE,
    TV_AGCPAR_INTEGRATIONS_LOG2, TV_AGCPAR_SPFILT_COEFF_A,
    TV_AGCPAR_SPFILT_COEFF_B, TV_AGCPAR_SPFILT_INIT, TV_AGCPAR_DCGAIN_MULT,
    TV_AGCPAR_DCGAIN_SHIFT, TV_AGCPAR_DC_DEADBAND, TV_AGCPAR_NUMPARAMS
} TV_AGCPAR_T;
/**
 * Type definition for common TV core instance data
 */
typedef struct
{
    UFW_COREINSTANCE_T *coreInstance;
    unsigned state;
    KRN_TASK_T masterTask;
    unsigned *masterStack;
    KRN_FLAGCLUSTER_T coreFlags;
    KRN_FLAGCLUSTER_T userFlags;
    void * tunerWorkspace;
    TUNER_USE_T *tunerUse;
    TUNER_T tuner;
    KRN_LOCK_T tunerLock;
    UCC_T *ucc;
    MCP_T *mcp;
    SCP_T *scp;
    UCCP_GRAM_ADDRESS_T mcpGRAMBase; /* base address of GRAM allocated as MCP data memory */
    void *tvInstanceExtensionData;
    uint32_t hostFrequency; /* retain last requested value delayed re-tuning in _TV_leaveInit() */
    uint32_t hostBandwidth; /* retain last requested value for delayed re-tuning in _TV_leaveInit() */
    uint32_t activeFrequency;
    uint32_t activeBandwidth;
    uint32_t lastStdFreqRequest; /* for filtering std generated tuning requests */
    uint32_t lastStdBWRequest; /* for filtering std generated tuning requests */
    uint32_t cmdACK; /* scratch store for command ack code */
    uint32_t freqACK; /* scratch store for frequency ack code */
    uint32_t notchACK; /* scratch store for notch ack code */
    uint32_t feACK; /* scratch store for FE override ack code */
    uint32_t agcRegisterBlock[TV_AGCPAR_NUMPARAMS];
} TV_INSTANCE_T;

/**
 * Type definition for TV state entry/exit functions.
 *
 * These functions return \c true or \c false to indicate success or failure.
 * Normally a \c true return value should be expected. A \c false return value will
 * be treated by the TV state machine as an unrecoverable failure
 * and the system will \c assert() or \c exit() in reponse.
 *
 * @param[in] tvInstance TV core instance handle
 * @parm[in] relatedState For state entry functions, this parameter indicates the previous state.
 *                        For state exit functions, this parameter indicates the next state.
 * @returns true:succss. false: failure.
 */
typedef bool
TV_STATEFUNC_T(TV_INSTANCE_T *tvInstance, TV_STATE_T relatedState);
/**
 * Type definition for TV main event handler extension hook function.
 * The main event handling loop calls the hook function for each event that it detects
 *
 * These functions return \c true or \c false.
 *
 * For post-event hooks, the return status indicates success or failure.
 * Normally a \c true return value should be expected. A \c false return value will
 * be treated by the TV state machine as an unrecoverable failure
 * and the system will \c assert() or \c exit() in response.
 *
 * For pre-event hooks, a true return indicates that normal processing should continue, whereas
 * a false return indicates that the event has been consumed and should not be the subject of
 * normal tv core processing
 *
 * @param[in] tvInstance TV core instance handle
 * @param[in] event Event Id
 * @returns true:succss. false: failure/event consumed.
 */
typedef bool
TV_EVENTHOOK_T(TV_INSTANCE_T *tvInstance, uint32_t event);

/**
 * Type definition for common TV standard extension to a framework core descriptor.
 * This defines the parameters for a particular TV standard within the common TV framework.
 */
typedef struct
{
    /** Size of standard-specific extension to the common TV instance object */
    unsigned instanceExtensionSize;
    /** Size of the stack (32-bit words) used by the main (startup) task in the TV standard */
    unsigned mainStackSize;
    /** Number of (24-bit) GRAM words required for use as MCP data RAM */
    unsigned mcpGramSize;
    /** Id of the TV standard implemented by this core */
    UCC_STANDARD_T tvStandard;
    /** Pointer to function called on entering the TV_STATE_INITIALISED state */
    TV_STATEFUNC_T *enterInit;
    /** Pointer to function called on leaving the TV_STATE_INITIALISED state */
    TV_STATEFUNC_T *leaveInit;
    /** Pointer to function called on entering the TV_STATE_DETECTING state */
    TV_STATEFUNC_T *enterDetect;
    /** Pointer to function called on leaving the TV_STATE_DETECTING state */
    TV_STATEFUNC_T *leaveDetect;
    /** Pointer to function called on entering the TV_STATE_ACQUIRING state */
    TV_STATEFUNC_T *enterAcquiring;
    /** Pointer to function called on leaving the TV_STATE_ACQUIRING state */
    TV_STATEFUNC_T *leaveAcquiring;
    /** Pointer to function called on entering the TV_STATE_REDETECTING state */
    TV_STATEFUNC_T *enterRedetecting;
    /** Pointer to function called on leaving the TV_STATE_REDETECTING state */
    TV_STATEFUNC_T *leaveRedetecting;
    /** Pointer to function called on entering the TV_STATE_DEMODULATING state */
    TV_STATEFUNC_T *enterDemodulating;
    /** Pointer to function called on leaving the TV_STATE_DEMODULATING state */
    TV_STATEFUNC_T *leaveDemodulating;
    /** (Possibly NULL) pointer to post event handler extension hook function */
    TV_EVENTHOOK_T *postEventHook;
    /** (Possibly NULL) pointer to  pre event handler extension hook function */
    TV_EVENTHOOK_T *preEventHook;
} TV_COREDESC_EXTENSION_T;

/*
 * Core event flag cluster usage.
 * 32 flags are reserved for use by the TV core
 * A further 32 flags are available for standard-specific events, which can be processed
 * by a ::TV_EVENTHOOK_T function
 */

/*
 * Event definitions:
 *
 * Events are defined by an integer Id
 *
 * Events numbered 0..31 are reserved for use by the common TV core
 * Events numbered 32..63 are "user" events available for use as required by demodulator authors
 */

typedef enum
{
    TV_EVENT_SYNC = 0, TV_EVENT_DETECT_COMMAND, TV_EVENT_RUN_COMMAND,
    TV_EVENT_STOP_COMMAND, TV_EVENT_FOUND, TV_EVENT_LOST,
    TV_EVENT_NOTHING_THERE, TV_EVENT_NEW_FREQ, TV_EVENT_NOTCH,
    TV_EVENT_FE_UPDATE, TV_EVENT_ACQFAIL, TV_EVENT_ACQUIRED, TV_EVENT_TUNED,
    TV_EVENT_UPDATE_RSSI_COMMAND, TV_EVENT_RESET_COUNTERS_COMMAND,
    TV_EVENT_UPDATE_COUNTERS_COMMAND, TV_EVENT_UPDATE_AGC_COMMAND,
    TV_EVENT_SET_AGC_COMMAND, TV_EVENT_USER = 31 /* a pseudo event to signal that one or more user events is pending */
} TV_EVENT_ID_T;

typedef enum
{
    TV_EVENT_USER1 = 32, TV_EVENT_USER2 = 33, TV_EVENT_USER3 = 34,
    TV_EVENT_USER4 = 35, TV_EVENT_USER5 = 36, TV_EVENT_USER6 = 37,
    TV_EVENT_USER7 = 38, TV_EVENT_USER8 = 39, TV_EVENT_USER9 = 40,
    TV_EVENT_USER10 = 41, TV_EVENT_USER11 = 42, TV_EVENT_USER12 = 43,
    TV_EVENT_USER13 = 44, TV_EVENT_USER14 = 45, TV_EVENT_USER15 = 46,
    TV_EVENT_USER16 = 47, TV_EVENT_USER17 = 48, TV_EVENT_USER18 = 49,
    TV_EVENT_USER19 = 50, TV_EVENT_USER20 = 51, TV_EVENT_USER21 = 52,
    TV_EVENT_USER22 = 53, TV_EVENT_USER23 = 54, TV_EVENT_USER24 = 55,
    TV_EVENT_USER25 = 56, TV_EVENT_USER26 = 57, TV_EVENT_USER27 = 58,
    TV_EVENT_USER28 = 59, TV_EVENT_USER29 = 60, TV_EVENT_USER30 = 61,
    TV_EVENT_USER31 = 62, TV_EVENT_USER32 = 63
} TV_USEREVENT_ID_T;

/*
 * Common TV main task function
 */
void
TV_mainTask(void);

/*
 * Deliver common TV events to the main event handler.
 */
void
TV_deliverEvent(UFW_COREINSTANCE_T *coreInstance, uint32_t eventId);

/*
 * Common TV management
 */

bool
TV_activate(UFW_COREINSTANCE_T *coreInstance, unsigned initialTunerUseId,
            TV_ACTIVATION_PARAMETER_T *tList);
void
TV_initRegisterAPI(UFW_COREINSTANCE_T *coreInstance);
void
TV_start(UFW_COREINSTANCE_T *coreInstance);
void
TV_stop(UFW_COREINSTANCE_T *coreInstance);
void
TV_deActivate(UFW_COREINSTANCE_T *coreInstance, void *coreParameter);
uint32_t
TV_getActiveFrequency(TV_INSTANCE_T *tvInstance);
uint32_t
TV_getActiveBandwidth(TV_INSTANCE_T *tvInstance);
bool
TV_reTune(TV_INSTANCE_T *tvInstance, uint32_t frequency, uint32_t bandwidth,
          bool force);

/****************************************************************************************************************
 *
 * The TVREG_xxx system provides a skinny wrapper around the common VREG_xxx library to manage the discontinuous
 * range of register Id value assignments
 *
 ****************************************************************************************************************/
/*
 * Macros for translating between register Ids and VREG_T pointers
 */
#define TVREG_OFFSET(ID) (ID < TV_REG_FIRST_STD_ID ? (ID) :  (ID) + TV_REG_NUM_COMMON_REG - TV_REG_FIRST_STD_ID )
#define TVREG_ID2PTR(CI, ID) ((CI)->registerBlock + TVREG_OFFSET(ID))
#define TVREG_ID(OFFSET) ((OFFSET) < TV_REG_NUM_COMMON_REG ? (OFFSET) : (OFFSET) - TV_REG_NUM_COMMON_REG + TV_REG_FIRST_STD_ID)
#define TVREG_PTR2OFFSET(CI, P) (P - (CI)->registerBlock)
#define TVREG_PTR2ID(CI, P) TVREG_ID(TVREG_PTR2OFFSET((CI), (P)))
/**
 * Read a register.
 *
 * In the case of a multi-valued register, the item with index zero is returned.
 *
 * @param[in]   coreInstance Pointer to core instance
 * @param[in]   reg          Register Id
 * @returns                  Register value
 */
uint32_t
TVREG_read(UFW_COREINSTANCE_T *coreInstance, uint32_t reg);
/**
 * Write a register from the core.
 *
 * In the case of a multi-valued register, the item with index zero is written.
 *
 * @param[in]   coreInstance Pointer to core instance
 * @param[in]   reg          Register Id
 * @param[in]   value        Register value
 */
void
TVREG_coreWrite(UFW_COREINSTANCE_T *coreInstance, uint32_t reg, uint32_t value);
/**
 * Write a register from the wrapper.
 *
 * In the case of a multi-valued register, the item with index zero is written.
 *
 * @param[in]   coreInstance Pointer to core instance
 * @param[in]   reg          Register Id
 * @param[in]   value        Register value
 */
void
TVREG_wrapperWrite(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                   uint32_t value);
/**
 * Install an event handler in the wrapper application to respond to register updates.
 *
 * @param[in]   coreInstance Pointer to core instance
 * @param[in]   reg          Register Id
 * @param[in]   handler      Pointer to handler function (or NULL to remove handler)
 * @param[in]   handlerParameter Parameter to passed to handler.
 */
void
TVREG_installWrapperHandler(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                            VREG_HANDLER_T *handler, void *handlerParameter);
/**
 * Install an event handler in the core IP to respond to register updates.
 *
 * @param[in]   coreInstance Pointer to core instance
 * @param[in]   reg          Register Id
 * @param[in]   handler      Pointer to handler function (or NULL to remove handler)
 * @param[in]   handlerParameter Parameter to passed to handler.
 */
void
TVREG_installCoreHandler(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                         VREG_HANDLER_T *handler, void *handlerParameter);
/**
 * Initialise a register object for use as a simple, possibly "mastered", value register.
 *
 * Register initialisation can fail if a master register is itself mastered - mastering can not be chained.
 *
 * @param[in] coreInstance Pointer to core instance
 * @param[in] reg          Register Id
 * @param[in] master       Id of register acting as synchronisation master for this register (or TV_REG_NULL_ID)
 * @param[in] resetValue   Register's initial value following ::TVREG_initValue().
 * @returns true: Register initialised successfully. false: Register initialise failed
 */
bool
TVREG_initValue(UFW_COREINSTANCE_T *coreInstance, uint32_t reg, uint32_t master,
                uint32_t resetValue);
/**
 * Initialise a register object for use as a simple, possibly "masked", bit-set register.
 *
 * Register initialisation can fail if a mask register is itself masked - masking can not be chained.
 *
 * @param[in] coreInstance Pointer to core instance
 * @param[in] reg          Register Id
 * @param[in] mask         Id of register acting as mask for this register (or TV_REG_NULL_ID).
 * @param[in] resetValue   Register's initial value following ::TVREG_initBits().
 * @returns true: Register initialised successfully. false: Register initialise failed
 */
bool
TVREG_initBits(UFW_COREINSTANCE_T *coreInstance, uint32_t reg, uint32_t mask,
               uint32_t resetValue);
/**
 * Initialise a register object for use as a, possibly "masked", toggling bit-set register.
 * A toggling bit-set register differs from a simple bit-set register in that a value written
 * to the register is XORed with the previous value, rather than replacing it.
 *
 * Register initialisation can fail if a mask register is itself masked - masking can not be chained.
 *
 * @param[in] coreInstance Pointer to core instance
 * @param[in] reg          Register Id
 * @param[in] mask         Id of register acting as mask for this register (or TV_REG_NULL_ID).
 * @param[in] resetValue   Register's initial value following ::TVREG_initToggleBits().
 * @returns true: Register initialised successfully. false: Register initialise failed
 */
bool
TVREG_initToggleBits(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                     uint32_t mask, uint32_t resetValue);
/**
 * Initialise a register object as a multi-value register.
 *
 * @param[in] coreInstance Pointer to core instance
 * @param[in] reg          Register Id
 * @param[in] numValues    Size of the register's valueArray
 * @param[in] valueArray   Data array to hold the set of values associated with the register
 * @param[in] resetValue   Register's initial value following ::TVREG_initMultiValue(). This value is applied to all the
 * items in \c valueArray.
 */
void
TVREG_initMultiValue(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                     unsigned numValues, uint32_t *valueArray,
                     uint32_t resetValue);
/**
 * Read indexed value from a multi-value register.
 *
 * This function is normally only used with multi-value registers. If it is called for other register types
 * it is equivalent to ::TVREG_read() - \c index will be ignored.
 *
 * @param[in] coreInstance Pointer to core instance
 * @param[in] reg          Register Id
 * @param[in] index        Value index.
 *
 * @returns Indexed register value.
 */
uint32_t
TVREG_readIndexed(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                  unsigned index);
/**
 * Write indexed value to a multi-value register from the core.
 *
 * This function is normally only used with multi-value registers. If it is called for other register types
 * it is equivalent to ::TVREG_coreWrite() - \c index will be ignored.
 *
 * @param[in] coreInstance Pointer to core instance
 * @param[in] reg          Register Id
 * @param[in] index        Value index.
 * @param[in] value        Register value.
 */
void
TVREG_coreWriteIndexed(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                       unsigned index, uint32_t value);
/**
 * Write indexed value to a multi-value register from the wrapper.
 *
 * This function is normally only used with multi-value registers. If it is called for other register types
 * it is equivalent to ::TVREG_wrapperWrite() - \c index will be ignored.
 *
 * @param[in] coreInstance Pointer to core instance
 * @param[in] reg          Register Id
 * @param[in] index        Value index.
 * @param[in] value        Register value.
 */
void
TVREG_wrapperWriteIndexed(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                          unsigned index, uint32_t value);
/**
 * Get the size (number of value elements) of a register.
 *
 * This is normally used to retrieve the \c numValues argument passed to a call to ::TVREG_initMultiValue().
 * For normal registers, the function always returns 1.
 *
 * @param[in] coreInstance Pointer to core instance
 * @param[in] reg          Register Id
 * @returns Number of values held in the register.
 */
unsigned
TVREG_getValueCount(UFW_COREINSTANCE_T *coreInstance, uint32_t reg);
/**
 * Get a pointer to the effective data values held in a register
 *
 * This is normally used to retrieve a pointer to the data array associated with a multi-valued register.
 * This sometimes provides a more efficient way to read the entire contents of a multi-valued register.
 *
 * Writing to a multi-valued register via a pointer is <i>not recommended</i>, since to do so, would circumvent
 * the normal register event generation system.
 *
 * @param[in] coreInstance Pointer to core instance
 * @param[in] reg          Register Id
 */
uint32_t *
TVREG_getValuePointer(UFW_COREINSTANCE_T *coreInstance, uint32_t reg);
/****************************************************************************************************************
 *
 * The TVTUNERxxx functions provide a skinny wrapper around the common TUNER_xxx library to allow
 *
 * i) Accessing the tuner by tvInstance pointer rather than coreInstance pointer
 * ii) Encapsulation of common TV functionality such as updating mirror registers
 *     and managing protocols for "host-operated" tuners
 *
 ****************************************************************************************************************/
bool
TVTUNER_tune(TV_INSTANCE_T *tvInstance, int rfFrequency);
bool
TVTUNER_configure(TV_INSTANCE_T *tvInstance, int rfBandwidth);
bool
TVTUNER_setSCP(TV_INSTANCE_T *tvInstance, unsigned config);
bool
TVTUNER_init(TV_INSTANCE_T *tvInstance, TUNER_ACTIVE_CONFIG_T *activeConfigs,
             unsigned numConfigs);
bool
TVTUNER_reset(TV_INSTANCE_T *tvInstance, unsigned config);
bool
TVTUNER_initAGC(TV_INSTANCE_T *tvInstance, unsigned muxId);
unsigned
TVTUNER_getNumConfigs(TV_INSTANCE_T *tvInstance);
unsigned
TVTUNER_getWorkspaceSize(TV_INSTANCE_T *tvInstance);
unsigned
TVTUNER_getSCPId(TV_INSTANCE_T *tvInstance);
unsigned
TVTUNER_getUCCId(TV_INSTANCE_T *tvInstance);
TDEV_USE_T *
TVTUNER_getDevice(TV_INSTANCE_T *tvInstance);
int
TVTUNER_readRFPower(TV_INSTANCE_T *tvInstance);
int
TVTUNER_pollAGC(TV_INSTANCE_T *tvInstance);
void
TVTUNER_startAGC(TV_INSTANCE_T *tvInstance);
void
TVTUNER_stopAGC(TV_INSTANCE_T *tvInstance);
int
TVTUNER_shutdown(TV_INSTANCE_T *tvInstance);
bool
TVTUNER_powerUp(TV_INSTANCE_T *tvInstance, unsigned muxId);
bool
TVTUNER_powerDown(TV_INSTANCE_T *tvInstance, unsigned muxId);
bool
TVTUNER_powerSave(TV_INSTANCE_T *tvInstance, TDEV_RF_PWRSAV_T pwrSaveLevel,
                  unsigned muxId);
bool
TVTUNER_setBandWide(TV_INSTANCE_T *tvInstance, unsigned config);
bool
TVTUNER_setBandNarrow(TV_INSTANCE_T *tvInstance, unsigned config);
bool
TVTUNER_setAGCRapid(TV_INSTANCE_T *tvInstance, unsigned config);
bool
TVTUNER_setAGCNormal(TV_INSTANCE_T *tvInstance, unsigned config);
bool
TVTUNER_setIFAGCTimeConstant(TV_INSTANCE_T *tvInstance, int tc);
bool
TVTUNER_setAGC(TV_INSTANCE_T *tvInstance, int inputSignalLevel, unsigned muxId,
               TUNER_AGC_CACLULATOR_T *agcCalculator, void *agcCalcContext);
bool
TVTUNER_setAGCImmediate(TV_INSTANCE_T *tvInstance, int inputSignalLevel,
                        unsigned muxId, TUNER_AGC_CACLULATOR_T *agcCalculator,
                        void *agcCalcContext);
bool
TVTUNER_updateActiveConfig(TV_INSTANCE_T *tvInstance, unsigned config,
                           TUNER_ACTIVE_CONFIG_T *newConfig);
TUNER_ACTIVE_CONFIG_T *
TVTUNER_getActiveConfig(TV_INSTANCE_T *tvInstance, unsigned config);
SCP_T *
TVTUNER_getSCP(TV_INSTANCE_T *tvInstance);
bool
TVTUNER_tuneIsImplemented(TV_INSTANCE_T *tvInstance);

#endif /* _TVCORE_H_ */
