/**
 * ****************************************************************************
 * @file          tuner16.h
 *
 * @brief         High(er) level tuner API - version for 16-bit SCP
 *
 * @date          25 06 2013
 *
 * Copyright     Copyright (C) Imagination Technologies Limited 2013
 *
 */

#ifndef _TUNER16_H_
#define _TUNER16_H_
#include <stdbool.h>
#include <MeOS.h>
#include <uccrt.h>
/*
 ************************************************************************************************************
 * Tuner high-level operational API
 *
 * The high level API hides the completion function mechanisms and provides a set of simple blocking functions.
 * The high level API also protects against multiple concurrent operations on the
 * same tuner - A task requesting an operation will block until any outstanding operations complete.
 *
 ************************************************************************************************************/

/**
 * Forward declaration of TUNER16_T
 */
typedef struct _TUNER16_tag TUNER16_T;

/**
 * AGC calculator function
 */
typedef void
TUNER16_AGC_CALCULATOR_T(TDEV16_AGCISR_HELPER_T *agcData,
                       TDEV16_SCP_CONTROL_T *agcControls, void *context);

/** TUNER object for the higher level API */
struct _TUNER16_tag
{
    /** Lock for API transaction protection */
    KRN_LOCK_T lock;
    /** Associated low level tuner device */
    TDEV16_T tdev;
    /** Semaphore for signalling AGC update period events */
    KRN_SEMAPHORE_T agcSem;
    /** Data structure to hold AGC parameters for the tuner device driver */
    TDEV16_AGCISR_HELPER_T agcData;
    /** Datat structure to hold SCP configuration updates returned by the tuner device driver */
    TDEV16_SCP_CONTROL_T agcControls;
};
/** Tuner "use case".
 *
 * Defines a particular combination of tuner and SCP configuration
 */
typedef struct
{
    /** Pointer to associated tuner device use case */
    TDEV16_USE_T *tdevUse;
    /** Tuner Use Id - the kind of tuner this item represents. Tuner use id's are system specific. They are normally defined
     * by the system architect and it is expected that a driver package will be constructed to deliver all the required uses.
     *
     * It is possible that several uses will share the same physical ucc/scp/tuner hardware.
     * It is up to system authors to ensure that conflicting uses are not activated concurrently
     */
    unsigned useId;
    /** Flag for management of tuner use case allocation */
    bool allocated;
} TUNER16_USE_T;
/**
 * Tuner SCP config structure. This is just an alias for the equivalent \c TDEV16_ structure.
 * It is defined simply to allow us to keep the naming at the \c TUNER16_ API level consistent.
 */
typedef TDEV16_SCP_CONFIG_T TUNER16_SCP_CONFIG_T;
/**
 * Tuner active config structure. This is just an alias for the equivalent \c TDEV16_ structure.
 * It is defined simply to allow us to keep the naming at the \c TUNER16_ API level consistent.
 */
typedef TDEV16_ACTIVE_CONFIG_T TUNER16_ACTIVE_CONFIG_T;
/**
 * Allocate a tuner from a list of available tuners.
 *
 * @param[in] tunerList Pointer to an array ::TUNER16_USE_T items.
 * @param[in] listSize Number of items in \c tunerList
 * @param[in] useId Kind of tuner use case to search for (values are system-specific).
 *
 * @returns Pointer to allocated use case or NULL if none can be found
 */
TUNER16_USE_T *
TUNER16_allocate(TUNER16_USE_T *tunerList, unsigned listSize, unsigned useId);
/**
 * Free a previously allocated tuner.
 *
 * @param[in] tunerUse Particular tuner use case to free
 * @param[in] tunerList Pointer to the array ::TUNER16_USE_T items from which \c tunerUse was allocated.
 * @param[in] listSize Number of items in \c tunerList
 */
void
TUNER16_free(TUNER16_USE_T *tunerUse, TUNER16_USE_T *tunerList, unsigned listSize);
/**
 * Get the low(er) level \c TDEV16_T object associated with a tuenr user case.
 *
 * Use this function to identify the tuner device use case, when you prefer to access the tuner via
 * the low level split-transaction TDEV API, rather than the blocking \c TUNER API.
 *
 * @param[in] useCase Pointer to tuner use case object.
 *
 * @returns Pointer to tuner device object.
 */
TDEV16_USE_T *
TUNER16_getDevice(TUNER16_USE_T *useCase);
/**
 * Return the size of the workspace (in bytes) required to instantiate a tuner for the specified use.
 * Note that, although the workspace size is given in bytes, you should assume that a tuner may require a workspace to be aligned on an 8-byte boundary
 *
 * @param[in] useCase Pointer to tuner use case object.
 *
 * @returns Workspace size required (may be zero).
 */
unsigned
TUNER16_getWorkspaceSize(TUNER16_USE_T *useCase);
/**
 * Initialise a tuner object.
 *
 * Normally called once only to prepare the tuner object for use.
 *
 * This function calls the low level tuner driver's \c init() function, but does not otherwise
 * access the hardware.
 *
 * @param[in] tuner Pointer to tuner instance object.
 * @param[in] useCase Pointer to tuner "use case" defining driver and SCP setup.
 * @param[in] activeConfigs Pointer to a caller maintained array of active tuner/SCP parameter sets. These will be populated
 * from the default parameter sets defined in \c useCase. Subsequently they may be modified by the application.
 * @param[in] numConfigs Number of items in the \c activeSCPConfigs array. If this value differs from that defined by \c useCase,
 * the function will return ::TDEV16_FAILURE.
 * @param[in] workSpace Pointer to workspace for this tuner instance.
 * @returns true: success. false: failure.
 */
bool
TUNER16_init(TUNER16_T *tuner, TUNER16_USE_T *useCase, TUNER16_ACTIVE_CONFIG_T *activeConfigs, unsigned numConfigs, void *workSpace);

/**
 * Reset a tuner.
 *
 * Restores tuner to a known operating state with a particular configuration set up.
 *
 * This function calls the low level tuner driver's \c reset() function. It also resets the tuner's SCP
 * and sets those SCP operating parameters defined by the tuner driver.
 *
 * @param[in] tuner Pointer to tuner instance object.
 * @param[in] config Configuration selector.
 * @returns true: success. false: failure.
 */
bool
TUNER16_reset(TUNER16_T *tuner, unsigned config);
/**
 * Configure a tuner.
 *
 * @param[in] tuner Pointer to tuner instance object.
 * @param[in] standard Demodulation standard.
 * @param[in] rfBandwidth RF bandwidch in Hz.
 * @returns true: success. false: failure.
 */
bool
TUNER16_configure(TUNER16_T *tuner, UCC_STANDARD_T standard, int rfBandwidth);
/**
 * Tune to desired RF frequency.
 *
 * @param[in] tuner Pointer to tuner instance object
 * @param[in] rfFrequency RF frequency in Hz.
 * @returns true: success. false: failure.
 */
bool
TUNER16_tune(TUNER16_T *tuner, int rfFrequency);
/**
 * Read power (RSSI)
 * @param[in] tuner Pointer to tuner instance object
 * @returns RSSI measurement value. A negative value indicates failure.
 */
int
TUNER16_readRFPower(TUNER16_T *tuner);
/**
 * Power up.
 *
 * Set the tuner to its full "power on"
 * state.
 *
 * @param[in] tuner Pointer to tuner instance object
 * @param[in] muxId Multiplex Id. Use of this parameter is system dependent.
 * @returns true: success. false: failure.
 */
bool
TUNER16_powerUp(TUNER16_T *tuner, unsigned muxId);
/**
 * Power down.
 *
 * Set the tuner to its full "power off"
 * state.
 *
 * @param[in] tuner Pointer to tuner instance object
 * @param[in] muxId Multiplex Id. Use of this parameter is system dependent.
 * @returns true: success. false: failure.
 */
bool
TUNER16_powerDown(TUNER16_T *tuner, unsigned muxId);
/**
 * Power save.
 *
 * Set the tuner to one of the intermediate "power saving"
 * states.
 *
 * @param[in] tuner Pointer to tuner instance object
 * @param[in] pwrSaveLevel Power save level.
 * @param[in] muxId Multiplex Id. Use of this parameter is system dependent.
 * @returns true: success. false: failure.
 */
bool
TUNER16_powerSave(TUNER16_T *tuner, TDEV_RF_PWRSAV_T pwrSaveLevel, unsigned muxId);
/**
 * Set IF AGC time constant.
 *
 * This function definition is reserved for future use. The function is not available.
 */
bool
TUNER16_setIFAGCTimeConstant(TUNER16_T *tuner, int tc);

/**
 * Set AGC.
 *
 * This function is part of the high level API for AGC management.
 * It is intended for use in an AGC management task (not in an ISR or event handler).
 *
 * This function blocks until the next AGC update event. Then,
 * in addition to the \c IFGain value, AGC related SCP configuration
 * parameters are passed to the tuner driver and any updates calculated by the tuner driver are written to the SCP.
 *
 * @param[in] tuner Pointer to tuner instance object.
 * @param[in] inputSignalLevel Long term logarithmic indication of input signal level.
 * @param[in] muxId Multiplex id - use is system dependent.
 * @param[in] agcCalculator Caller supplied function to calculate updated AGC controls.
 * @param[in] agcCalcContext Caller supplied context data pointer for use by \c agcCalculator.
 * @returns true: success. false: failure.
 */

bool
TUNER16_setAGC(TUNER16_T *tuner, int inputSignalLevel, unsigned muxId,
             TUNER16_AGC_CALCULATOR_T *agcCalculator, void *agcCalcContext);
/**
 * Set AGC without waiting.
 *
 * This function is part of the high level API for AGC management.
 * It is intended for use in an AGC management task (not in an ISR or event handler).
 *
 * This function is identical to ::TUNER16_setAGC except that it does not wait for an AGC update but
 * acts immediately
 *
 * @param[in] tuner Pointer to tuner instance object.
 * @param[in] inputSignalLevel Long term logarithmic indication of input signal level.
 * @param[in] muxId Multiplex id - use is system dependent.
 * @param[in] agcCalculator Caller supplied function to calculate updated AGC controls.
 * @param[in] agcCalcContext Caller supplied context data pointer for use by \c agcCalculator.
 * @returns true: success. false: failure.
 */

bool
TUNER16_setAGCImmediate(TUNER16_T *tuner, int inputSignalLevel, unsigned muxId,
             TUNER16_AGC_CALCULATOR_T *agcCalculator, void *agcCalcContext);
/**
 * Poll for AGC update event.
 *
 * Test whether an AGC update event is pending.
 *
 * This function can be used to poll the AGC update event status.
 *
 * @param[in] tuner Pointer to tuner instance object
 * @returns true: AGC update event is pending. ::TUNER16_setAGC will not block. false: No AGC update event pending. ::TUNER16_setAGC will  block until the next event.
 *
 */
bool
TUNER16_pollAGC(TUNER16_T *tuner);

/**
 * Initialise AGC system.
 *
 * This function is part of the high level API for AGC management. It calls
 * the tuner device driver's initialisation function and waits for completion.
 *
 * Following tuner AGC initialisation, you must start AGC processing by calling
 * ::TUNER16_startAGC().
 *
 * @param[in] tuner Pointer to tuner instance object
 * @param[in] muxId Multiplex id - use is system dependent
 * @returns true: success. false: failure.
 */
bool
TUNER16_initAGC(TUNER16_T *tuner, unsigned muxId);
/**
 * Start AGC processing.
 *
 * This function is part of the high level API for AGC management.
 * It activates the tuner's SCP AGC update event and sets up the
 * event handling logic for ::TUNER16_setAGC().
 *
 * ::TUNER16_stopAGC() and ::TUNER16_startAGC() are typically used to temporarily suspend AGC processing
 * around major TV mode changes. Whether this is actually necessary depends on the particular TV decoder
 * design.
 *
 * @param[in] tuner Pointer to tuner instance object
 * @returns true: success. false: failure.
 */
void
TUNER16_startAGC(TUNER16_T *tuner);
/**
 * Stop AGC processing
 *
 * This function is part of the high level API for AGC management. It suspends AGC updates by disabling
 * the AGC update event handler. The rest of the AGC
 * system state is left unchanged.
 *
 * You can restart AGC processing by calling ::TUNER16_startAGC().
 * ::TUNER16_stopAGC() and ::TUNER16_startAGC() are typically used to temporarily suspend AGC processing
 * around major TV mode changes. Whether this is actually necessary depends on the particular TV decoder
 * design.
 *
 * @param[in] tuner Pointer to tuner instance object
 * @returns true: success. false: failure.
 */
void
TUNER16_stopAGC(TUNER16_T *tuner);
/**
 * Shutdown tuner.
 *
 * Normally called only once when an application shuts down. Once the shutdown is complete, the \c tuner object may be discarded or
 * re-used.
 *
 * @param[in] tuner Pointer to tuner instance object
 * @returns true: success. false: failure.
 */
bool
TUNER16_shutdown(TUNER16_T *tuner);

/**
 * Set up the tuner's SCP using the parameters defined by the tuner's "use case" for a particular configuration.
 *
 * Note that this function sets up <em>only</em> those items defined by the tuner's "user case":
 * - ADC format (2's comp/offfset binary)
 * - CIC factor
 * - FIR factor
 * - Input mode (real/imag)
 * - Spectrum inversion
 * - SCP mixer setup (carrier frequency and gain)
 * - AGC period - set to the "normal" value defined by the tuner driver.
 * - RF bandwidth -set to the "narrow" value defined by the tuner driver.

 * SCP mixer output gain is set to 0dB.\n
 * Resampler output gain is set to -6dB or 0db for complex or real IF respectively.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector.
 * @returns true: OK. false: Failure (bad config value)
 */
bool
TUNER16_setSCP(TUNER16_T *tuner, unsigned config);

/**
 * Set up the tuner's SCP for Wide Band operation.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector.
 * @returns true: OK. false: Failure (bad config value)
 */
bool
TUNER16_setBandWide(TUNER16_T *tuner, unsigned config);
/**
 * Set up the tuner's SCP for Narrow Band operation.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector.
 * @returns true: OK. false: Failure (bad config value)
 */
bool
TUNER16_setBandNarrow(TUNER16_T *tuner, unsigned config);
/**
 * Set up the tuner's AGC for rapid operation.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector.
 * @returns true: OK. false: Failure (bad config value)
 */
bool
TUNER16_setAGCRapid(TUNER16_T *tuner, unsigned config);
/**
 * Set up the tuner's AGC for normal operation.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector.
 * @returns true: OK. false: Failure (bad config value)
 */
bool
TUNER16_setAGCNormal(TUNER16_T *tuner, unsigned config);
/**
 * Get handle to the SCP associated with a tuner use
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @returns Pointer to SCP object "belongin" to the tuner.
 */
SCP_T *
TUNER16_getSCP(TUNER16_T *tuner);
/**
 * Get id number of SCP associated with a particular tuner use case
 *
 * @param[in] tunerUse Pointer to tuner use case
 * @returns Id number of SCP associated with the tuner use case
 */
unsigned
TUNER16_getSCPId(TUNER16_USE_T *tunerUse);
/**
 * Get id number of UCC associated with a particular tuner use case
 *
 * @param[in] tunerUse Pointer to tuner use case
 * @returns Id number of UCC associated with the tuner use case
 */
unsigned
TUNER16_getUCCId(TUNER16_USE_T *tunerUse);

/**
 * Get id number of SCP configurations associated with a particular tuner use case
 *
 * @param[in] tunerUse Pointer to tuner use case
 * @returns number of SCP configs
 */
unsigned
TUNER16_getNumConfigs(TUNER16_USE_T *tunerUse);

/**
 * Get a pointer to the active config data associated with a particular tuner configuration.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector.
 * @returns Pointer to configuration data structure or \c NULL on failure (bad \c config).
 *
 */
TUNER16_ACTIVE_CONFIG_T *TUNER16_getActiveConfig(TUNER16_T *tuner, unsigned config);

/**
 * Update the active config data associated with a particular tuner configuration.
 * This function copies \c newConfig into the tuner's configuration data store but does not
 * write any changes to the SCP hardware. Following a config update, it would normally make sense
 * to update the hardware using ::TUNER16_setSCP() etc.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector.
 * @param[in] newConfig New configuration parameter set.
 * @returns true:success. false: failure (bad \c config)
 *
 */
bool TUNER16_updateActiveConfig(TUNER16_T *tuner, unsigned config, TUNER16_ACTIVE_CONFIG_T *newConfig);
/**
 * Test whether a tuner's underlying driver code actually implements RF tuning functionality
 * or whether the code is missing (leading to dummy behaviour).
 *
 * This function is typically used by demodulators to determine whether tuning is locally managed
 * by the UCCP Meta, or managed by another processor in the system.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @returns true: Local implementation exists. false: Dummy
 */
bool TUNER16_tuneIsImplemented(TUNER16_T *tuner);

#endif /* _TUNER16_H_ */
