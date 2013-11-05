/**
 * ****************************************************************************
 * @file          tuner.h
 *
 * @brief         High(er) level tuner API
 *
 * @date          7 Oct 2010
 *
 * Copyright     Copyright (C) Imagination Technologies Limited 2010
 *
 */

#ifndef _TUNER_H_
#define _TUNER_H_
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
 * Forward declaration of TUNER_T
 */
typedef struct _TUNER_tag TUNER_T;

/**
 * AGC calculator function
 */
typedef void
TUNER_AGC_CACLULATOR_T(TDEV_AGCISR_HELPER_T *agcData,
                       TDEV_SCP_CONTROL_T *agcControls, void *context);

/** TUNER object for the higher level API */
struct _TUNER_tag
{
    /** Lock for API transaction protection */
    KRN_LOCK_T lock;
    /** Associated low level tuner device */
    TDEV_T tdev;
    /** Semaphore for signalling AGC update period events */
    KRN_SEMAPHORE_T agcSem;
    /** Data structure to hold AGC parameters for the tuner device driver */
    TDEV_AGCISR_HELPER_T agcData;
    /** Datat structure to hold SCP configuration updates returned by the tuner device driver */
    TDEV_SCP_CONTROL_T agcControls;
};
/** Tuner "use case".
 *
 * Defines a particular combination of tuner and SCP configuration
 */
typedef struct
{
    /** Pointer to associated tuner device use case */
    TDEV_USE_T *tdevUse;
    /** Tuner Use Id - the kind of tuner this item represents. Tuner use id's are system specific. They are normally defined
     * by the system architect and it is expected that a driver package will be constructed to deliver all the required uses.
     *
     * It is possible that several uses will share the same physical ucc/scp/tuner hardware.
     * It is up to system authors to ensure that conflicting uses are not activated concurrently
     */
    unsigned useId;
    /** Flag for management of tuner use case allocation */
    bool allocated;
} TUNER_USE_T;
/**
 * Tuner SCP config structure. This is just an alias for the equivalent \c TDEV_ structure.
 * It is defined simply to allow us to keep the naming at the \c TUNER_ API level consistent.
 */
typedef TDEV_SCP_CONFIG_T TUNER_SCP_CONFIG_T;
/**
 * Tuner active config structure. This is just an alias for the equivalent \c TDEV_ structure.
 * It is defined simply to allow us to keep the naming at the \c TUNER_ API level consistent.
 */
typedef TDEV_ACTIVE_CONFIG_T TUNER_ACTIVE_CONFIG_T;
/**
 * Allocate a tuner from a list of available tuners.
 *
 * @param[in] tunerList Pointer to an array ::TUNER_USE_T items.
 * @param[in] listSize Number of items in \c tunerList
 * @param[in] useId Kind of tuner use case to search for (values are system-specific).
 *
 * @returns Pointer to allocated use case or NULL if none can be found
 */
TUNER_USE_T *
TUNER_allocate(TUNER_USE_T *tunerList, unsigned listSize, unsigned useId);
/**
 * Free a previously allocated tuner.
 *
 * @param[in] tunerUse Particular tuner use case to free
 * @param[in] tunerList Pointer to the array ::TUNER_USE_T items from which \c tunerUse was allocated.
 * @param[in] listSize Number of items in \c tunerList
 */
void
TUNER_free(TUNER_USE_T *tunerUse, TUNER_USE_T *tunerList, unsigned listSize);
/**
 * Get the low(er) level \c TDEV_T object associated with a tuenr user case.
 *
 * Use this function to identify the tuner device use case, when you prefer to access the tuner via
 * the low level split-transaction TDEV API, rather than the blocking \c TUNER API.
 *
 * @param[in] useCase Pointer to tuner use case object.
 *
 * @returns Pointer to tuner device object.
 */
TDEV_USE_T *
TUNER_getDevice(TUNER_USE_T *useCase);
/**
 * Return the size of the workspace (in bytes) required to instantiate a tuner for the specified use.
 * Note that, although the workspace size is given in bytes, you should assume that a tuner may require a workspace to be aligned on an 8-byte boundary
 *
 * @param[in] useCase Pointer to tuner use case object.
 *
 * @returns Workspace size required (may be zero).
 */
unsigned
TUNER_getWorkspaceSize(TUNER_USE_T *useCase);
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
 * the function will return ::TDEV_FAILURE.
 * @param[in] workSpace Pointer to workspace for this tuner instance.
 * @returns true: success. false: failure.
 */
bool
TUNER_init(TUNER_T *tuner, TUNER_USE_T *useCase, TUNER_ACTIVE_CONFIG_T *activeConfigs, unsigned numConfigs, void *workSpace);

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
TUNER_reset(TUNER_T *tuner, unsigned config);
/**
 * Configure a tuner.
 *
 * @param[in] tuner Pointer to tuner instance object.
 * @param[in] standard Demodulation standard.
 * @param[in] rfBandwidth RF bandwidch in Hz.
 * @returns true: success. false: failure.
 */
bool
TUNER_configure(TUNER_T *tuner, UCC_STANDARD_T standard, int rfBandwidth);
/**
 * Tune to desired RF frequency.
 *
 * @param[in] tuner Pointer to tuner instance object
 * @param[in] rfFrequency RF frequency in Hz.
 * @returns true: success. false: failure.
 */
bool
TUNER_tune(TUNER_T *tuner, int rfFrequency);
/**
 * Read power (RSSI)
 * @param[in] tuner Pointer to tuner instance object
 * @returns RSSI measurement value. A negative value indicates failure.
 */
int
TUNER_readRFPower(TUNER_T *tuner);
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
TUNER_powerUp(TUNER_T *tuner, unsigned muxId);
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
TUNER_powerDown(TUNER_T *tuner, unsigned muxId);
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
TUNER_powerSave(TUNER_T *tuner, TDEV_RF_PWRSAV_T pwrSaveLevel, unsigned muxId);
/**
 * Set IF AGC time constant.
 *
 * This function definition is reserved for future use. The function is not available.
 */
bool
TUNER_setIFAGCTimeConstant(TUNER_T *tuner, int tc);

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
TUNER_setAGC(TUNER_T *tuner, int inputSignalLevel, unsigned muxId,
             TUNER_AGC_CACLULATOR_T *agcCalculator, void *agcCalcContext);
/**
 * Set AGC without waiting.
 *
 * This function is part of the high level API for AGC management.
 * It is intended for use in an AGC management task (not in an ISR or event handler).
 *
 * This function is identical to ::TUNER_setAGC except that it does not wait for an AGC update but
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
TUNER_setAGCImmediate(TUNER_T *tuner, int inputSignalLevel, unsigned muxId,
             TUNER_AGC_CACLULATOR_T *agcCalculator, void *agcCalcContext);
/**
 * Poll for AGC update event.
 *
 * Test whether an AGC update event is pending.
 *
 * This function can be used to poll the AGC update event status.
 *
 * @param[in] tuner Pointer to tuner instance object
 * @returns true: AGC update event is pending. ::TUNER_setAGC will not block. false: No AGC update event pending. ::TUNER_setAGC will  block until the next event.
 *
 */
bool
TUNER_pollAGC(TUNER_T *tuner);

/**
 * Initialise AGC system.
 *
 * This function is part of the high level API for AGC management. It calls
 * the tuner device driver's initialisation function and waits for completion.
 *
 * Following tuner AGC initialisation, you must start AGC processing by calling
 * ::TUNER_startAGC().
 *
 * @param[in] tuner Pointer to tuner instance object
 * @param[in] muxId Multiplex id - use is system dependent
 * @returns true: success. false: failure.
 */
bool
TUNER_initAGC(TUNER_T *tuner, unsigned muxId);
/**
 * Start AGC processing.
 *
 * This function is part of the high level API for AGC management.
 * It activates the tuner's SCP AGC update event and sets up the
 * event handling logic for ::TUNER_setAGC().
 *
 * ::TUNER_stopAGC() and ::TUNER_startAGC() are typically used to temporarily suspend AGC processing
 * around major TV mode changes. Whether this is actually necessary depends on the particular TV decoder
 * design.
 *
 * @param[in] tuner Pointer to tuner instance object
 * @returns true: success. false: failure.
 */
void
TUNER_startAGC(TUNER_T *tuner);
/**
 * Stop AGC processing
 *
 * This function is part of the high level API for AGC management. It suspends AGC updates by disabling
 * the AGC update event handler. The rest of the AGC
 * system state is left unchanged.
 *
 * You can restart AGC processing by calling ::TUNER_startAGC().
 * ::TUNER_stopAGC() and ::TUNER_startAGC() are typically used to temporarily suspend AGC processing
 * around major TV mode changes. Whether this is actually necessary depends on the particular TV decoder
 * design.
 *
 * @param[in] tuner Pointer to tuner instance object
 * @returns true: success. false: failure.
 */
void
TUNER_stopAGC(TUNER_T *tuner);
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
TUNER_shutdown(TUNER_T *tuner);

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
TUNER_setSCP(TUNER_T *tuner, unsigned config);

/**
 * Set up the tuner's SCP for Wide Band operation.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector.
 * @returns true: OK. false: Failure (bad config value)
 */
bool
TUNER_setBandWide(TUNER_T *tuner, unsigned config);
/**
 * Set up the tuner's SCP for Narrow Band operation.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector.
 * @returns true: OK. false: Failure (bad config value)
 */
bool
TUNER_setBandNarrow(TUNER_T *tuner, unsigned config);
/**
 * Set up the tuner's AGC for rapid operation.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector.
 * @returns true: OK. false: Failure (bad config value)
 */
bool
TUNER_setAGCRapid(TUNER_T *tuner, unsigned config);
/**
 * Set up the tuner's AGC for normal operation.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector.
 * @returns true: OK. false: Failure (bad config value)
 */
bool
TUNER_setAGCNormal(TUNER_T *tuner, unsigned config);
/**
 * Get handle to the SCP associated with a tuner use
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @returns Pointer to SCP object "belongin" to the tuner.
 */
SCP_T *
TUNER_getSCP(TUNER_T *tuner);
/**
 * Get id number of SCP associated with a particular tuner use case
 *
 * @param[in] tunerUse Pointer to tuner use case
 * @returns Id number of SCP associated with the tuner use case
 */
unsigned
TUNER_getSCPId(TUNER_USE_T *tunerUse);
/**
 * Get id number of UCC associated with a particular tuner use case
 *
 * @param[in] tunerUse Pointer to tuner use case
 * @returns Id number of UCC associated with the tuner use case
 */
unsigned
TUNER_getUCCId(TUNER_USE_T *tunerUse);

/**
 * Get id number of SCP configurations associated with a particular tuner use case
 *
 * @param[in] tunerUse Pointer to tuner use case
 * @returns number of SCP configs
 */
unsigned
TUNER_getNumConfigs(TUNER_USE_T *tunerUse);

/**
 * Get a pointer to the active config data associated with a particular tuner configuration.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector.
 * @returns Pointer to configuration data structure or \c NULL on failure (bad \c config).
 *
 */
TUNER_ACTIVE_CONFIG_T *TUNER_getActiveConfig(TUNER_T *tuner, unsigned config);

/**
 * Update the active config data associated with a particular tuner configuration.
 * This function copies \c newConfig into the tuner's configuration data store but does not
 * write any changes to the SCP hardware. Following a config update, it would normally make sense
 * to update the hardware using ::TUNER_setSCP() etc.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector.
 * @param[in] newConfig New configuration parameter set.
 * @returns true:success. false: failure (bad \c config)
 *
 */
bool TUNER_updateActiveConfig(TUNER_T *tuner, unsigned config, TUNER_ACTIVE_CONFIG_T *newConfig);
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
bool TUNER_tuneIsImplemented(TUNER_T *tuner);

#endif /* _TUNER_H_ */
