/**
 * @file          tdev.h
 *
 * @brief         Common low level tuner driver support in the UCC RT
 *
 * @date          7 Oct 2010
 *
 * Copyright     Copyright (C) Imagination Technologies Limited 2010
 *
 */

#ifndef _TDEV_H_
#define _TDEV_H_
#include <stdbool.h>
#include <stdint.h>
#include <uccrt.h>

/** TDEV major version number */
#define TDEV_VERSION_MAJOR       (3)
/** TDEV minor version number */
#define TDEV_VERSION_MINOR       (0)
/** TDEV version number */
#define TDEV_VERSION_I32         ((TDEV_VERSION_MAJOR<<16) | TDEV_VERSION_MINOR)

/*
 **************************************************************************************************
 * Definitions for the SCP configuration parameters associated with a tuner
 *************************************************************************************************
 */

/**
 **     This structure defines signal characteristics and the related SCP configuration parameters
 **     associated with a tuner to implement a configuration within a "Tuner Use Case" - ::TDEV_USE_T.
 **
 **     The SCP includes a 24-tap symmetrical FIR filter; due to the symmetry,
 **     only 12 coefficients need be provided to configure this filter. This structure allows you
 **     to define two configurations for the FIR filter (ostensibly narrow band and wide
 **     band implementations) though the use of these is determined by the particular
 **     communication standard in use.
 **
 **     To support demodulation schemes that use a two-stage AGC, "rapid" and "normal"
 **     update rates can be specified.
 **
 **     The \c sampleRate, \c CICFactor, \c FIRFactor and \c resamplerValue fields combine to
 **     determine the final baseband sample rate at the SCP output and so should be
 **     configured appropriately for the communication standard in use.
 **
 **     The input sample rate (\c sampleRate) is decimated first by the \c CICFactor and
 **     then by the \c FIRFactor before being multiplied by the resampling rate (R, as
 **     defined by \c resamplerValue = 1/R * 2<SUP>31</SUP>) to obtain the final output
 **     sample rate.
 **
 **     <i>The values defined in a structure of this type are specific to a particular
 **     tuner and communication standard combination. they are used to configure the SCP to
 **     provide samples at the required baseband sample rate for that standard.</i>
 */
typedef struct
{
    /** Format of ADC samples */
    SCP_SAMPLE_FORMAT_T ADCFormat;
    /** Input signal (ADC) sample rate in Hz. */
    unsigned sampleRate;
    /*! CIC filter decimation factor.\n
     ** Valid values are: 1, 2, 3, 4, 6, 8, 12 and 16. */
    unsigned CICFactor;
    /** FIR filter decimation factor.\n
     ** Valid values are: 1, 2 and 3. */
    unsigned FIRFactor;
    /*! SCP resampler configuration setting.\n
     ** The value provided here is determined from the desired resampling rate
     ** according to the following formula:
     ** <tt>resamplerValue = 1/R * 2<SUP>31</SUP></tt>,
     ** where <tt>R</tt> is the desired resampling rate.*/
    unsigned resamplerValue;
    //TBD replace "12" with a #define
    /** Array of 12 coefficients used to configure the FIR filter
     **  for a narrow band implementation.\n
     ** The valid range for the coefficients in this array is: -512 to +511,
     ** where 256 is equivalent to 1.0. */
    int16_t narrowBandFIRCoeffs[12];
    /** Pointer to an array of 12 coefficients used to configure the FIR filter
     **  for a wide band implementation.\n
     ** The valid range for the coefficients in this array is: -512 to +511,
     ** where 256 is equivalent to 1.0. */
    int16_t wideBandFIRCoeffs[12];
    /** Rapid AGC update period (in ADC samples)\n
     ** The valid range for this value is: 0 to 2<SUP>17</SUP>-1. */
    unsigned rapidAGCPeriod;
    /** Normal operation AGC update period (in ADC samples)\n
     ** The valid range for this value is: 0 to 2<SUP>17</SUP>-1. */
    unsigned normalAGCPeriod;
} TDEV_SCP_CONFIG_T;

/**
 * This structure defines a set of active tuner configuration parameters.
 * These are used to hold copies of various tuner and SCP configuration parameters.
 * The copies are originally populated from default values, but may subsequently be modified
 * by application specific "front-end" override commands.
 */
typedef struct {
    /** A non-zero value indicates complex output from the RF. */
    int complexIF;
    /** A non-zero value indicates that the IF spectrum is inverted */
    int spectrumInverted;
    /** The final IF frequency of the signal coming into the SCP (Hz) */
    int frequencyIF;
    /** the complete set of tuner-related SCP parameters */
    TDEV_SCP_CONFIG_T scpConfig;
} TDEV_ACTIVE_CONFIG_T;
/*
 **************************************************************************************************
 * Definitions for describing a tuner and its drivers configuration parameters associated with a tuner
 **************************************************************************************************/

/** Maximum value that can be used for the IF gain parameter of the
 ::TDEV_CONFIG_T::setAGC function (::TDEV_AGCISR_HELPER_T.IFgainValue). */
#define TDEV_MAX_IF_GAIN     (65535)

/*
 ** types and structures
 */
/* Forward reference to TDEV_T definition */
typedef struct _TDEV_TAG TDEV_T;

/** Return type for the application-provided driver functions. */
typedef enum
{
    /** Failure. */
    TDEV_FAILURE = 0,
    /** Success. */
    TDEV_SUCCESS,
} TDEV_RETURN_T;

/** AGC Mode*/
typedef enum
{
    /** AGC is in rapid mode. Typically, fast operation for acquisition. */
    TDEV_RAPID_AGC = 0,
    /** AGC is in normal mode. Typically, slower operation used for demodulation. */
    TDEV_NORMAL_AGC,
} TDEV_AGC_MODE_T;

/** Some tuner (RF) hardware modules provide multiple stage power control to allow
 **     for lowest possible power consumption in a burst data reception environment (for
 **     example when demodulating DVB-H).\n
 **     The API provides for three levels of power saving:
 **     <ul><li>Off (RF is fully powered-up)</li>
 **     <li>Level 1 (some parts of RF are powered down)</li>
 **     <li>Level 2 (RF is fully powered-down)</li></ul>
 **     Typical usage for this scheme would be for the RF to be powered on in two stages,
 **     allowing for settling times, so as to minimise power consumption by reducing the
 **     "full power" period for each burst.\n
 */
typedef enum
{
    /** Power saving level 0 (i.e. powered up) */
    TDEV_RF_PWRSAV_OFF = 0,
    /** Power saving level 1 */
    TDEV_RF_PWRSAV_LEVEL1,
    /** Power saving level 2 */
    TDEV_RF_PWRSAV_LEVEL2,
    /** Invalid value used for initialisation */
    TDEV_RF_PWRSAV_INVALID
} TDEV_RF_PWRSAV_T;

/** This structure is used to communicate control parameters for the SCP,
 *  from a tuner driver's \c setAGC() function to the application.
 *  The driver's \c setAGC() function may calculate updated values for
 *  these parameters which, typically, will be programmed to the SCP hardware by the application, following
 *  completion of the drive's \c setAGC() operation.
 */
typedef struct
{
    /** Early Fine Gain Value for I.\n
     ** Configures the early gain block control value for the I data path in
     ** the SCP. A value of 256 corresponds to 0dB.\n
     ** Valid range: -1024 to +1023 */
    int fineGainI;
    /** Early Fine Gain Value for Q\n
     ** Configures the early gain block control value for the Q data path in
     ** the SCP. A value of 256 corresponds to 0dB.\n
     ** Valid range: -1024 to +1023 */
    int fineGainQ;
    /** IQ Phase Correction\n
     ** IQ phase imbalance compensation.\n
     ** Valid range: -1024 to +1023\n
     ** The output (<tt>Iout/Qout</tt>) signals are derived from the input
     ** (<tt>Iin/Qin</tt>) as follows:
     ** <ul> <li><tt>Qout = Qin + Iin * IQcorrection/1024</tt> <li><tt>Iout = Iin</tt></ul> */
    int IQcorrection;
    /** AGC Counter 1 Threshold Value.\n
     ** Input samples are compared with this value to determine the values recorded
     ** in \c TDEV_AGCISR_HELPER_T::AGCcount1I and \c TDEV_AGCISR_HELPER_T::AGCcount1Q.
     ** Valid range: 0 to 2047 */
    unsigned AGCthresh1;
    /** AGC Counter 2 Threshold Value.\n
     ** Input samples are compared with this value to determine the values recorded
     ** in \c TDEV_AGCISR_HELPER_T::AGCcount2I and \c TDEV_AGCISR_HELPER_T::AGCcount2Q.
     ** Valid range: 0 to 2047 */
    unsigned AGCthresh2;
    /** DC Offset Cancellation Value for I.\n
     **  Added to all input samples on the I channel in an attempt to remove
     **  any DC offset present. The actual offset added is \c DCoffsetI/4096, to achieve a
     **  precision significantly less than one bit.
     ** Valid range: -2<SUP>23</SUP> to +2<SUP>23</SUP>-1 */
    int DCoffsetI;
    /** DC Offset Cancellation Value for Q.\n
     **  Added to all input samples on the Q channel in an attempt to remove
     **  any DC offset present. The actual offset added is \c DCoffsetQ/4096, to achieve a
     **  precision significantly less than one bit.
     ** Valid range: -2<SUP>23</SUP> to +2<SUP>23</SUP>-1 */
    int DCoffsetQ;
    /** This value is used to perform a one-off correction to the
     ** Digital Local Oscillator frequency.\n
     ** It is used by the application software only after the first call
     ** to TDEV_CONFIG_T::setAGC (usually during initialisation).\n
     ** The correction value is <tt>(CFOcorrection / 2<SUP>31</SUP>) * sampleRate</tt> [Hz] */
    int CFOcorrection;
} TDEV_SCP_CONTROL_T;

/** This structure is used to pass a (fairly large) set of parameter values describing the
 * status of the SCP, to a tuner driver's \c setAGC() function.\n
 **     It contains status information from the SCP that
 **     may be useful to the AGC control software, for example, to calculate tuner-related
 **     control values.\n
 **     A pointer to a \c TDEV_SCP_CONTROL_T object is provided for the driver's \c setAGC() function
 **      the AGC to return updated SCP configuration parameters. These are used by the application to
 **      update the SCP - the tuner driver does not update the SCP directly. This method allows the application to
 **     determine which SCP control values are to be used and to ensure  that updates are performed
 **     at the appropriate time.\n
 */
typedef struct
{
    /**  Reports the number of ADC samples that exceeded the last programmed
     **  \c threshold1 on the I channel during the previous AGC update period.\n
     **  This value is used to measure the signal level on the I axis.\n
     ** Valid range: 0 to 2<SUP>17</SUP>-1 */
    unsigned AGCcount1I;
    /**  Reports the number of ADC samples that exceeded the previous programmed
     **  \c threshold1 on the Q channel during the last AGC update period.\n
     **  This value is used to measure the signal level on the Q axis.\n
     */
    unsigned AGCcount1Q;
    /**  Reports the number of ADC samples that exceeded the previous programmed
     **  \c threshold2 on the I channel during the last AGC update period.\n
     **  This value is used to measure the signal level on the I axis.\n
     ** Valid range: 0 to 2<SUP>17</SUP>-1 */
    unsigned AGCcount2I;
    /**  Reports the number of ADC samples that exceeded the previous programmed
     **  \c threshold2 on the Q channel during the last AGC update period.\n
     **  This value is used to measure the signal level on the Q axis.\n
     ** Valid range: 0 to 2<SUP>17</SUP>-1 */
    unsigned AGCcount2Q;
    /** Input Signal Level \n
     **  Reports the signal level at the input to the SCP. The value is a
     **  logarithmic indication of long term input signal power relative to a full scale input.
     **  It has units of 0.1dB and will always be negative. */
    int inputSignalLevel;
    /**  The sum of ADC samples on the I channel over the previous AGC update period.\n
     **  To obtain the DC offset from this value, divide by the total number of ADC samples in
     **  the AGC update period.\n
     ** Valid range: -2<SUP>23</SUP> to 2<SUP>23</SUP>-1 */
    int DCoffsetI;
    /**  The sum of ADC samples on the Q channel over the previous AGC update period.\n
     **  To obtain the DC offset from this value, divide by the total number of ADC samples in
     **  the AGC update period.\n
     ** Valid range: -2<SUP>23</SUP> to 2<SUP>23</SUP>-1 */
    int DCoffsetQ;
    /** IQ Phase Error Value. This is calculated by correlating the I and Q signals at the SCP input
     **  and accumulating the result over the AGC update period. The I and Q signals are
     **  divided by 256 before the correlation to reduce the word width. \n
     ** Valid range: -2<SUP>20</SUP> to 2<SUP>20</SUP>-1 */
    int IQphaseError;
    /** Digital Local Oscillator frequency \n
     **  The SCP input signal is down-converted by this frequency. The Digital Local Oscillator
     **  frequency is given by: \n
     ** <tt>DCOvalue / 2^31 * sampleRate [Hz]</tt> \n
     ** Valid range: -2<SUP>31</SUP> to 2<SUP>31</SUP>-1 */
    int DLOvalue;
    /**  Current AGC mode - either rapid (::TDEV_RAPID_AGC) or normal (::TDEV_NORMAL_AGC).
     */
    TDEV_AGC_MODE_T AGCMode;
    /** The number of ADC samples defining the AGC update period.
     ** A period of 0 means that the AGC counts (1I, 1Q, 2I, 2Q), DC Offsets (I, Q)
     **  and IQ Phase Error will not be available to read.\n
     ** Valid range: 0 to 2<SUP>17</SUP>-1 */
    unsigned AGCupdatePeriod;
    /** Input signal sample rate \n
     **  This is the sample rate of the ADC samples passed into the SCP, expressed in Hz.
     */
    unsigned sampleRate;
    /** Calculated IF Gain Value \n
     **  This is the IF gain value, as calculated by the application.\n
     ** Valid range: 0 to ::TDEV_MAX_IF_GAIN */
    int IFgainValue;
    /** Multiplexer Identifier \n
     ** This value is used where the tuner may require unique context information to support
     ** interleaved tuning to more than one RF channel. Use of this is standard specific.
     ** It is used as an index to store or recall the RF related information within the
     ** user application.\n
     ** It is provided to several driver functions to indicate which
     ** tuner context is expected for the operation. */
    unsigned muxID;
    /** Pointer to ::TDEV_SCP_CONTROL_T object.\n
     ** This is be used by the driver's \c setAGC() function to return recalculated AGC-related SCP parameters
     ** configuration values to the application. */
    TDEV_SCP_CONTROL_T *pSCPcontrol;
} TDEV_AGCISR_HELPER_T;

/**
 *     Tuner completion function type.
 *
 *     Each function registered in the TDEV_CONFIG_T tuner configuration takes
 *     this common function type as a parameter to support the split-transaction model.
 *     When the driver functions are called, a completion function of this type is
 *     provided by the calling software.
 *
 *     This allows the driver function to run and return quickly,
 *     whilst performing any operations that take a longer time to complete (for example,
 *     communication with the RF hardware) without blocking the progress of the caller.
 *     The completion function is called once all operations
 *     related to the original driver function have completed.
 */
typedef void
(*TDEV_COMPLETION_FUNC_T)(TDEV_T *tuner, TDEV_RETURN_T status, void *parameter);
/**
 *     Completion function type for use with readRFPower.
 *
 *      This differs from the the standard ::TDEV_COMPLETION_FUNC_T only in that it accepts a power value parameter
 *      rather than a ::TDEV_RETURN_T status.
 */
typedef void
(*TDEV_RSSI_COMPLETION_FUNC_T)(TDEV_T *tuner, int rssiValue, void *parameter);

/**
 * Tuner driver definition structure.
 *
 * This structure is declared, initialised and exported by a tuner driver author.
 * It contains key operating parameters and pointers to a standard set of tuner-specific
 * control and status functions.
 *
 * <i>The values in this structure are specific to a particular
 *  RF module and so may be common to more than one standard in a multi-standard product (e.g. an RF module that
 * supports both T-DMB/DAB and DVB-H operation).</i>
 */
typedef struct
{
    /**
     * Tuner API version number in the format ::TDEV_VERSION_I32.
     This can be used by the application to confirm that the tuner driver's version is compatible. */
    int versionNumber;
    /** A non-zero value indicates complex output from the RF. */
    int complexIF;
    /** A non-zero value indicates that the IF spectrum is inverted */
    int spectrumInverted;
    /** The final IF frequency of the signal coming into the SCP (Hz) */
    int frequencyIF;
    /** The step size of the tuner PLL (Hz) */
    int PLLStepSize;
    /** The offset margin (Hz) required for coarse retune of the tuner PLL */
    int PLLUpdateMargin;
    /** Settling time from power down to power up, given in microseconds (uS).*/
    int powerUpSettlingTimeuS;
    /** Settling time from ::TDEV_RF_PWRSAV_LEVEL1 to power up (::TDEV_RF_PWRSAV_OFF), given in microseconds (uS).
     *
     * If the tuner hardware or communication standard software does not support dynamic power
     * saving, this parameter should be set to 0.*/
    int powerSvLvl1SettlingTimeuS;
    /** Settling time from ::TDEV_RF_PWRSAV_LEVEL2 to power up (::TDEV_RF_PWRSAV_OFF), given in microseconds (uS).
     *
     * If the tuner hardware or communication standard software does not support dynamic power
     * saving, this parameter should be set to 0.*/
    int powerSvLvl2SettlingTimeuS;
    /** Settling time from tune to stable output, given in microseconds (uS). */
    int tuneSettlingTimeuS;

    /** Initialise tuner.
     *
     ** This function initialises a new tuner object. It is normally called only once,
     ** before any other tuner driver functions are used. */
    TDEV_RETURN_T
    (*init)(TDEV_T *, TDEV_COMPLETION_FUNC_T, void*);
    /** Reset tuner.
     *
     ** This function resets the tuner hardware to a known state. It may be called repeatedly during
     ** system operation. */
    TDEV_RETURN_T
    (*reset)(TDEV_T *, TDEV_COMPLETION_FUNC_T, void*);
    /** Configure tuner.
     *
     * This function is called to configure the tuner to the broadcast standard being demodulated
     *  and to set the RF bandwidth (Hz).
     */
    TDEV_RETURN_T
    (*configure)(TDEV_T *, UCC_STANDARD_T, unsigned, TDEV_COMPLETION_FUNC_T, void *);
    /** Tune request.
     *
     * This function is called to request that the RF module is tuned to a given frequency (Hz).
     */
    TDEV_RETURN_T
    (*tune)(TDEV_T *, unsigned, TDEV_COMPLETION_FUNC_T, void *);
    /** Measure RF power.
     *
     * This function is called to request the signal power at RF (RSSI).
     *
     * A positive RF power value indicates successful completion. A negative number is reported if there
     * is a failure to read the power for some reason.
     *
     ** Note that the measured power value is returned by the completion function, <em>not</em> by this function itself.
     **/
    TDEV_RETURN_T
    (*readRFPower)(TDEV_T *, TDEV_RSSI_COMPLETION_FUNC_T, void *);
    /** Power up.
     *
     * This function is called to request that the tuner is configured to its full "power on"
     * state.
     *
     * <i>The particular use of this functionality is dependent upon the communication standard.</i>*/
    TDEV_RETURN_T
    (*powerUp)(TDEV_T *, unsigned, TDEV_COMPLETION_FUNC_T, void *);
    /** Power down.
     *
     * This function is called to request that the tuner is configured into its lowest power
     * state.
     *
     *<i>The particular use of this functionality is dependent upon the communication standard.</i>*/
    TDEV_RETURN_T
    (*powerDown)(TDEV_T *, unsigned, TDEV_COMPLETION_FUNC_T, void *);
    /** Power save.
     *
     * This function is called request that the tuner is configured to a specific power state
     * (::TDEV_RF_PWRSAV_T). It can be called from any other power state. \n
     * If the tuner hardware does not support power saving, both of the power saving settling times should be zero and
     * this field should be set to NULL.
     *
     * <i>The particular use of this functionality is dependent upon the communication standard.</i>*/
    TDEV_RETURN_T
    (*powerSave)(TDEV_T *, TDEV_RF_PWRSAV_T, unsigned, TDEV_COMPLETION_FUNC_T,
                 void *);
    /**
     * Set Time Constant.
     *
     * This function is reserved for future use and should be set to NULL. */
    TDEV_RETURN_T
    (*setIFAGCTimeConstant)(TDEV_T *, int, TDEV_COMPLETION_FUNC_T, void *);
    /** Configure AGC.
     *
     * This function is called to configure features of the Automatic Gain Control sub-system
     * (for example, to set IF gain) and also to perform certain tuner specific configuration (for example
     * the DC canceller and I/Q imbalance canceller).
     * The ::TDEV_AGCISR_HELPER_T structure is provided by the caller
     * giving sufficient information about the current state of the SCP to perform
     * AGC calculations and decision making.
     * Updated SCP register values are returned via
     * TDEV_AGCISR_HELPER_T::pSCPcontrol. <i>Note that this update is performed on return from the
     * TDEV_CONFIG_T::setAGC function and not on calling of the completion function.</i>
     *
     * This function is typically called at the end of the SCP initialisation sequence and then regularly on
     * completion of each AGC update period (see TDEV_AGCISR_HELPER_T::AGCupdatePeriod). */
    TDEV_RETURN_T
    (*setAGC)(TDEV_T *, TDEV_AGCISR_HELPER_T *, TDEV_COMPLETION_FUNC_T, void *);
    /** Initialise AGC.
     *
     * This function is called to initialise the Automatic Gain Control sub-system.
     *
     * This function is generally called once only, at the
     * start of the SCP initialisation sequence.
     *
     * <i>Note that there is no capability here for the user application to modify SCP registers at this time -
     * such modification is performed via the TDEV_CONFIG_T::setAGC callback.</i> */
    TDEV_RETURN_T
    (*initAGC)(TDEV_T *, unsigned, TDEV_COMPLETION_FUNC_T, void *);
    /** Shutdown tuner.
     *
     ** This function is called to shutdown the tuner.
     **  It is called only once, when a demodulator is shutdown. */
    TDEV_RETURN_T
    (*shutdown)(TDEV_T *, TDEV_COMPLETION_FUNC_T, void *);
} TDEV_CONFIG_T;

/**
 * Tuner use descriptor. This contains pointers to fixed configuration parameters and driver code associated with
 * a particular use of a physical tuner
 *
 * In the case of a flexible hardware tuner, more than one tuner definition may share the same UCC/SCP
 * hardware (perhaps with different driver code and SCP parameters). In such cases, it is up to the system designer
 * to ensure that a given SCP is not put to multiple conflicting uses at any particular time.
 */
typedef const struct
{
    /** Number of SCP configuration parameter sets */
    unsigned numConfigs;
    /** Pointer to the tuner driver definition */
    TDEV_CONFIG_T *tunerControl;
    /** Pointer to the array of default SCP configuration parameter sets */
    TDEV_SCP_CONFIG_T *defaultSCPConfigs;
    /** Workspace size associated with TDEV use case */
    unsigned tunerWorkspaceSize;
    /** Id of the UCC associated with the tuner [1..n] */
    unsigned uccId;
    /** Id of the SCP (on the specified UCC) associated with the tuner */
    unsigned scpId;
    /** Arbitrary tuner-specific configuration extension */
    void *tunerConfigExtension;
} TDEV_USE_T;

/**
 * A tuner instance object.
 */
struct _TDEV_TAG
{
    /** Number of SCP configuration parameter sets */
    unsigned numConfigs;
    /** Pointer to the SCP used by this tuner instance */
    SCP_T *scp;
    /** Pointer to the tuner driver definition */
    TDEV_CONFIG_T *tunerConfig;
    /** Pointer to array of active configuration parameter sets */
    TDEV_ACTIVE_CONFIG_T *activeConfigs;
    /** Tuner Use Id */
    unsigned tunerUseId;
    /** pointer to the arbitrary use case extension data */
    void *tunerConfigExtension;
    /** Pointer to the workspace (if any) used by this tuner instance. */
    void *workSpace;
};

/*
 **********************************************************************************************************
 * Tuner management API
 ***********************************************************************************************************/
/**
 * Return the size of the workspace (in bytes) required to instantiate a tuner for the specified use.
 * Note that, although the workspace size is given in bytes, you should assume that a tuner may require a workspace to be aligned on an 8-byte boundary
 *
 * @returns Workspace size required (may be zero).
 */
unsigned
TDEV_getWorkspaceSize(TDEV_USE_T *useCase);

/*
 ***********************************************************************************************************
 * Tuner low-level operational API
 *
 * The low level API gives direct access to the tuner driver primitives.
 * It gives maximum flexibility and allows direct use of the completion function mechanism
 *
 * If you use the low level API, it is your responsibility to manage the possibility that more than one
 * operation could be outstanding at any time (which may not make sense)
 ***********************************************************************************************************
 */
/**
 * Initialise a tuner instance and call the tuner's driver \c init() function.
 *
 * This is normally called once (either directly or
 * indirectly via a higher level API) before the tuner is first used.
 *
 * This function is part of the low level tuner driver API, which provides direct access
 * the the driver functions described by the ::TDEV_CONFIG_T object belonging to a particular tuner instance.
 *
 * @param[in] tuner Pointer to the tuner instance object to be initialised
 * @param[in] useCase Pointer to the tuner use case descriptor
 * @param[in] activeConfigs Pointer to a caller maintained array of active tuner/SCP parameter sets. These will be populated
 * from the default parameter sets defined in \c useCase. Subsequently they may be modified by the application.
 * @param[in] numConfigs Number of items in the \c activeSCPConfigs array. If this value differs from that defined by \c useCase,
 * the function will return ::TDEV_FAILURE.
 * @param[in] workSpace Pointer to workspace data area for use by this tuner instance.
 * @param[in] completionFunction Pointer to the completion function, to be called when the driver operation completes.
 * @param[in] completionParameter Application specified parameter, which will be passed by the tuner driver to
 *            the completion function.
 * @returns ::TDEV_SUCCESS or ::TDEV_FAILURE. Note that ::TDEV_SUCCESS simply means that the driver
 *                  function was able to accept the request. Success or failure of the operation itself must be
 *                  signalled to the application in some way by the completion function.
 */
TDEV_RETURN_T
TDEV_init(TDEV_T *tuner, TDEV_USE_T *useCase, TDEV_ACTIVE_CONFIG_T *activeConfigs, unsigned numConfigs,
          void *workSpace, TDEV_COMPLETION_FUNC_T completionFunction, void *completionParameter);

/**
 * Call the tuner's driver \c reset() function. T
 *
 * This function is part of the low level tuner driver API, which provides direct access
 * the the driver functions described by the ::TDEV_CONFIG_T object belonging to a particular tuner instance.
 *
 * @param[in] tuner Pointer to the particular tuner instance whose driver function is to be called.
 * @param[in] completionFunction Pointer to the completion function, to be called when the driver operation completes.
 * @param[in] completionParameter Application specified parameter, which will be passed by the tuner driver to
 *            the completion function.
 * @returns ::TDEV_SUCCESS or ::TDEV_FAILURE. Note that ::TDEV_SUCCESS simply means that the driver
 *                  function was able to accept the request. Success or failure of the operation itself must be
 *                  signalled to the application in some way by the completion function.
 */
TDEV_RETURN_T
TDEV_reset(TDEV_T *tuner, TDEV_COMPLETION_FUNC_T completionFunction,
           void *completionParameter);
/**
 * Call the tuner's driver \c configure() function.
 *
 * This function is part of the low level tuner driver API, which provides direct access
 * the the driver functions described by the ::TDEV_CONFIG_T object belonging to a particular tuner instance.
 *
 * @param[in] tuner Pointer to the particular tuner instance whose driver function is to be called.
 * @param[in] standard Indicates which demodulation standard is using the tuner.
 * @param[in] rfBandwidth Required RF bandwidth (in Hz).
 * @param[in] completionFunction Pointer to the completion function, to be called when the driver operation completes.
 * @param[in] completionParameter Application specified parameter, which will be passed by the tuner driver to
 *            the completion function.
 * @returns ::TDEV_SUCCESS or ::TDEV_FAILURE. Note that ::TDEV_SUCCESS simply means that the driver
 *                  function was able to accept the request. Success or failure of the operation itself must be
 *                  signalled to the application in some way by the completion function.
 */
TDEV_RETURN_T
TDEV_configure(TDEV_T *tuner, UCC_STANDARD_T standard, unsigned rfBandwidth,
               TDEV_COMPLETION_FUNC_T completionFunction,
               void *completionParameter);
/**
 * Call the tuner's driver \c tune() function.
 *
 * This function is part of the low level tuner driver API, which provides direct access
 * the the driver functions described by the ::TDEV_CONFIG_T object belonging to a particular tuner instance.
 *
 * @param[in] tuner Pointer to the particular tuner instance whose driver function is to be called.
 * @param[in] rfFrequency RF frequency to tune (in Hz).
 * @param[in] completionFunction Pointer to the completion function, to be called when the driver operation completes.
 * @param[in] completionParameter Application specified parameter, which will be passed by the tuner driver to
 *            the completion function.
 * @returns ::TDEV_SUCCESS or ::TDEV_FAILURE. Note that ::TDEV_SUCCESS simply means that the driver
 *                  function was able to accept the request. Success or failure of the operation itself must be
 *                  signalled to the application in some way by the completion function.
 */
TDEV_RETURN_T
TDEV_tune(TDEV_T *tuner, unsigned rfFrequency,
          TDEV_COMPLETION_FUNC_T completionFunction, void *completionParameter);
/**
 * Call the tuner's driver \c readRFPower() function.
 *
 * Note that this does <em>not</em> return the RSSI value. Rather, the caller-provide completion function must
 * signal the RSSI value to the application i some way.
 *
 * This function is part of the low level tuner driver API, which provides direct access
 * the the driver functions described by the ::TDEV_CONFIG_T object belonging to a particular tuner instance.
 *
 * @param[in] tuner Pointer to the particular tuner instance whose driver function is to be called.
 * @param[in] completionFunction Pointer to the completion function, to be called when the driver operation completes.
 * @param[in] completionParameter Application specified parameter, which will be passed by the tuner driver to
 *            the completion function.
 * @returns ::TDEV_SUCCESS or ::TDEV_FAILURE. Note that ::TDEV_SUCCESS simply means that the driver
 *                  function was able to accept the request. Success or failure of the operation itself must be
 *                  signalled to the application in some way by the completion function.
 */
TDEV_RETURN_T
TDEV_readRFPower(TDEV_T *tuner, TDEV_RSSI_COMPLETION_FUNC_T completionFunction,
                 void *completionParameter);

/**
 * Call the tuner's driver \c powerUp() function.
 *
 * This function is part of the low level tuner driver API, which provides direct access
 * the the driver functions described by the ::TDEV_CONFIG_T object belonging to a particular tuner instance.
 *
 * @param[in] tuner Pointer to the particular tuner instance whose driver function is to be called.
 * @param[in] muxId Multiplex Id.
 * @param[in] completionFunction Pointer to the completion function, to be called when the driver operation completes.
 * @param[in] completionParameter Application specified parameter, which will be passed by the tuner driver to
 *            the completion function.
 * @returns ::TDEV_SUCCESS or ::TDEV_FAILURE. Note that ::TDEV_SUCCESS simply means that the driver
 *                  function was able to accept the request. Success or failure of the operation itself must be
 *                  signalled to the application in some way by the completion function.
 */
TDEV_RETURN_T
TDEV_powerUp(TDEV_T *tuner, unsigned muxId,
             TDEV_COMPLETION_FUNC_T completionFunction,
             void *completionParameter);
/**
 * Call the tuner's driver \c powerDown() function.
 *
 * This function is part of the low level tuner driver API, which provides direct access
 * the the driver functions described by the ::TDEV_CONFIG_T object belonging to a particular tuner instance.
 *
 * @param[in] tuner Pointer to the particular tuner instance whose driver function is to be called.
 * @param[in] muxId Multiplex Id.
 * @param[in] completionFunction Pointer to the completion function, to be called when the driver operation completes.
 * @param[in] completionParameter Application specified parameter, which will be passed by the tuner driver to
 *            the completion function.
 * @returns ::TDEV_SUCCESS or ::TDEV_FAILURE. Note that ::TDEV_SUCCESS simply means that the driver
 *                  function was able to accept the request. Success or failure of the operation itself must be
 *                  signalled to the application in some way by the completion function.
 */
TDEV_RETURN_T
TDEV_powerDown(TDEV_T *tuner, unsigned muxId,
               TDEV_COMPLETION_FUNC_T completionFunction,
               void *completionParameter);
/**
 * Call the tuner's driver \c powerSave() function.
 *
 * This function is part of the low level tuner driver API, which provides direct access
 * the the driver functions described by the ::TDEV_CONFIG_T object belonging to a particular tuner instance.
 *
 * @param[in] tuner Pointer to the particular tuner instance whose driver function is to be called.
 * @param[in] pwrSaveLevel Power save level selection.
 * @param[in] muxId Multiplex Id.
 * @param[in] completionFunction Pointer to the completion function, to be called when the driver operation completes.
 * @param[in] completionParameter Application specified parameter, which will be passed by the tuner driver to
 *            the completion function.
 * @returns ::TDEV_SUCCESS or ::TDEV_FAILURE. Note that ::TDEV_SUCCESS simply means that the driver
 *                  function was able to accept the request. Success or failure of the operation itself must be
 *                  signalled to the application in some way by the completion function.
 */
TDEV_RETURN_T
TDEV_powerSave(TDEV_T *tuner, TDEV_RF_PWRSAV_T pwrSaveLevel, unsigned muxId,
               TDEV_COMPLETION_FUNC_T completionFunction,
               void *completionParameter);
/**
 * Call the tuner's driver \c setIFAGCTimeConstant() function.
 *
 * This function is part of the low level tuner driver API, which provides direct access
 * the the driver functions described by the ::TDEV_CONFIG_T object belonging to a particular tuner instance.
 *
 * @param[in] tuner Pointer to the particular tuner instance whose driver function is to be called.
 * @param[in] tc Time constant value.
 * @param[in] completionFunction Pointer to the completion function, to be called when the driver operation completes.
 * @param[in] completionParameter Application specified parameter, which will be passed by the tuner driver to
 *            the completion function.
 * @returns ::TDEV_SUCCESS or ::TDEV_FAILURE. Note that ::TDEV_SUCCESS simply means that the driver
 *                  function was able to accept the request. Success or failure of the operation itself must be
 *                  signalled to the application in some way by the completion function.
 */
TDEV_RETURN_T
TDEV_setIFAGCTimeConstant(TDEV_T *tuner, int tc,
                          TDEV_COMPLETION_FUNC_T completionFunction,
                          void *completionParameter);
/**
 * Call the tuner's driver \c setAGC() function.
 *
 * This function is part of the low level tuner driver API, which provides direct access
 * the the driver functions described by the ::TDEV_CONFIG_T object belonging to a particular tuner instance.
 *
 * @param[in] tuner Pointer to the particular tuner instance whose driver function is to be called.
 * @param[in] agcData Pointer to a structure containing data describing the current state of the SCP's AGC system.
 * @param[in] completionFunction Pointer to the completion function, to be called when the driver operation completes.
 * @param[in] completionParameter Application specified parameter, which will be passed by the tuner driver to
 *            the completion function.
 * @returns ::TDEV_SUCCESS or ::TDEV_FAILURE. Note that ::TDEV_SUCCESS simply means that the driver
 *                  function was able to accept the request. Success or failure of the operation itself must be
 *                  signalled to the application in some way by the completion function.
 */
TDEV_RETURN_T
TDEV_setAGC(TDEV_T *tuner, TDEV_AGCISR_HELPER_T *agcData,
            TDEV_COMPLETION_FUNC_T completionFunction,
            void *completionParameter);
/**
 * Call the tuner's driver \c initAGC() function.
 *
 * This function is part of the low level tuner driver API, which provides direct access
 * the the driver functions described by the ::TDEV_CONFIG_T object belonging to a particular tuner instance.
 *
 * @param[in] tuner Pointer to the particular tuner instance whose driver function is to be called.
 * @param[in] muxId Multiplex Id.
 * @param[in] completionFunction Pointer to the completion function, to be called when the driver operation completes.
 * @param[in] completionParameter Application specified parameter, which will be passed by the tuner driver to
 *            the completion function.
 * @returns ::TDEV_SUCCESS or ::TDEV_FAILURE. Note that ::TDEV_SUCCESS simply means that the driver
 *                  function was able to accept the request. Success or failure of the operation itself must be
 *                  signalled to the application in some way by the completion function.
 */
TDEV_RETURN_T
TDEV_initAGC(TDEV_T *tuner, unsigned muxId,
             TDEV_COMPLETION_FUNC_T completionFunction,
             void *completionParameter);
/**
 * Call the tuner's driver \c shutdown() function. This is normally called once only (either directly or
 * indirectly via a higher level API) just before releasing the tuner as part of an application's close down.
 *
 * This function is part of the low level tuner driver API, which provides direct access
 * the the driver functions described by the ::TDEV_CONFIG_T object belonging to a particular tuner instance.
 *
 * @param[in] tuner Pointer to the particular tuner instance whose driver function is to be called
 * @param[in] completionFunction Pointer to the completion function, to be called when the driver operation completes.
 * @param[in] completionParameter Application specified parameter, which will be passed by the tuner driver to
 *            the completion function.
 * @returns ::TDEV_SUCCESS or ::TDEV_FAILURE. Note that ::TDEV_SUCCESS simply means that the driver
 *                  function was able to accept the request. Success or failure of the operation itself must be
 *                  signalled to the application in some way by the completion function.
 */
TDEV_RETURN_T
TDEV_shutdown(TDEV_T *tuner, TDEV_COMPLETION_FUNC_T completionFunction,
              void *completionParameter);
/**
 * Set up the SCP using the parameters in the tuner's ::TDEV_SCP_CONFIG_T definition.
 *
 * Note that this function sets up <em>only</em> those items derived from definitions in ::TDEV_SCP_CONFIG_T object:
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
 * @param[in] config Configuration selector
 * @returns true: Operation successful. false: Failure - bad \c config.
 */
bool
TDEV_setSCP(TDEV_T *tuner, unsigned config);

/**
 * Set up the tuner's SCP for Wide Band operation.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector
 * @returns true: Operation successful. false: Failure - bad \c config.
 */
bool
TDEV_setBandWide(TDEV_T *tuner, unsigned config);
/**
 * Set up the tuner's SCP for Narrow Band operation.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector
 * @returns true: Operation successful. false: Failure - bad \c config.
 */
bool
TDEV_setBandNarrow(TDEV_T *tuner, unsigned config);
/**
 * Set up the tuner's AGC for rapid operation.
 *
 * This function simply sets the SCP's AGC period. It does not modify the AGC interrupt setting
 * or install/remove AGC event handlers.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector
 * @returns true: Operation successful. false: Failure - bad \c config.
 */
bool
TDEV_setAGCRapid(TDEV_T *tuner, unsigned config);
/**
 * Set up the tuner's AGC for normal operation as defined by the selected configuration
 *
 * This function simply sets the SCP's AGC period. It does not modify the AGC interrupt setting
 * or install/remove AGC event handlers.
 *
 * @param[in] tuner Pointer to the particular tuner instance.
 * @param[in] config Configuration selector
 * @returns true: Operation successful. false: Failure - bad \c config.
 */
bool
TDEV_setAGCNormal(TDEV_T *tuner, unsigned config);
/*
 * Macros for accessing tuner driver features - supports testing for
 * dummy implementations
 */
#define TDEV_FEATURE(TUNER, FEATURE) ((TUNER)->tunerConfig->FEATURE)
#define TDEV_IMPLEMENTED(TUNER, FEATURE) (TDEV_FEATURE((TUNER), FEATURE) ? true : false)

#endif /* _TDEV_H_ */
