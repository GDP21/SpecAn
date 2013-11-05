/*
** FILE NAME:   $RCSfile: common_agc.h,v $
**
** TITLE:       Common AGC and related control loop handling
**
** PROJECT:		UCCP Common
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: The file contains the interface description for the common AGC Module.
**
**				Copyright (C) Imagination Technologies Ltd.
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

/*! \file common_agc.h
*******************************************************************************
	\brief	Common AGC and related control loop handling
*******************************************************************************/
/*! \mainpage Common AGC
*******************************************************************************
 \section intro Introduction

 This set of linked HTML pages provides the functional interface to the
 common AGC code. This provides a foundation and example implemenation of
 AGC and related control loops (DC offset and IQ phase offset correction).
 The code can be used complete, or just in parts, to implement these in the
 physical layer of the demodulation systems.

 This has been designed for ease of interfacing to the Tuner API, hence
 it uses types from it.

 Various aspects of the control loops are build time configurable.
 These include:
 - Initial IF gain
 - Initial thresholds
 - Loop gains

 <b>Copyright (C) Imagination Technologies Ltd.</b>

*******************************************************************************/


#ifndef COMMON_AGC_H
#define COMMON_AGC_H

#include "uccrt.h"

/*! Filter mode types. */
typedef enum
{
    AGC_FILT_INTEGRATOR=0,  /*! Integrate and scale. */
    AGC_FILT_SINGLE_POLE,   /*! Single pole filter. */
    AGC_FILT_MAX
} AGC_INPUT_FILT_MODE_T;

/*! Structure to hold an averaging filter to accumulate and dump over a set number of samples. */
typedef struct
{
    long valI;
    long valQ;
    long cnt;                       /* Counter to count up to numIntegrations */

    /* control values */
    unsigned numIntegrationsLog2;   /* Scale by shifting down this amount. */
    long     numIntegrations;       /* Number of samples to integrate over = 2^numIntegrationsLog2. */

} AGC_PRE_INTEGRATOR_T;

/*! A single pole filter with 8bit coefficients, output = input at n=0 */
typedef struct
{
    int y_I;    /* current output I */
    int y_Q;    /* current output Q */

    int a;      /* feed forward coefficient in Q0.8 */
    int b;      /* feedback coefficient in Q0.8 (typically 1-a)*/

    bool init;   /* If set, output = input. Cleared on first call. */

} AGC_SINGLE_POLE_T;

typedef union
{
    AGC_PRE_INTEGRATOR_T integrator;
    AGC_SINGLE_POLE_T    singlePole;
} AGC_INPUT_FILT_U;

/*! This structure holds all context for the common AGC and related control loops.
	Its content is private and not to be accessed outside of the common AGC code.
*/
typedef struct
{
    /* @cond DONT_DOXYGEN Control loop context etc. Private! */

    /* DC offset loop */
    AGC_INPUT_FILT_MODE_T DCOffset_filtMode;  /* Type of input noise reduction used. */

    AGC_INPUT_FILT_U      DCOffset_inputFilt;

	long 	DcOffsetI_Accum;
	long 	DcOffsetQ_Accum;

	unsigned scpDcOffsetDeadBand;
	unsigned scpDcOffsetGainShift;      /* Gain right shift */
	unsigned scpDcOffsetGainMult;       /* Gain multiplier in Q.8 */

	long	gain_accum;                 /* Gain loop */
	long	agctarget_accum;            /* Gain loop */
    long	gain_error_accum;           /* IQ offset loop */
    long	IQcorrectionAccum;          /* IQ offset loop */
    /* @endcond */

    /*! SCP configuration we are operating with */
    TDEV_SCP_CONFIG_T *SCPConfig;

    /*! Is the agc in rapid or normal mode */
    TDEV_AGC_MODE_T agcMode;

    /*! This flag should be set to a non-zero value if the output from the RF is complex. */
    long complexIF;
	/* Number of samples for slip count target. */
	long targetClipCount;

	/* Feedback constants for gain update */
    long agcErrScaleUpMul;                  /* AGC positive error scale multiplier in Q.8 */
	long agcErrScaleUpShift;                /* AGC positive error scale right shift */
    long agcErrScaleDnMul;                  /* AGC negative error scale multiplier in Q.8 */
	long agcErrScaleDnShift;                /* AGC negative error Scale right shift */

    long agcIQErrScale;

} AGC_CONTEXT_T;

/*! This structure can be used to configure the AGC and overwrite the default settings */
typedef struct
{
    /* AGC loop gain values */
    long agcErrScaleUpMul;                  /* AGC positive error scale multiplier in Q.8 */
    long agcErrScaleUpShift;                /* AGC positive error scale right shift */
    long agcErrScaleDnMul;                  /* AGC negative error scale multiplier in Q.8 */
    long agcErrScaleDnShift;                /* AGC negative error Scale right shift */

    AGC_INPUT_FILT_MODE_T scpDcOffsetFiltMode; /* Type of input noise reduction used. */

    /* If using the DC loop pre-integrator filter: */
    unsigned scpDcOffsetPreIntegrationsLog2;    /*! Average DC values over power 2 of this number. */

    /* If using single pole filter: */
    int      scpDcOffsetSinglePoleLoopCoeff_a;  /* feed-forward coefficient in Q0.8 */
    int      scpDcOffsetSinglePoleLoopCoeff_b;  /* feed-back coefficients in Q0.8 */
    bool     scpDcOffsetSinglePoleInit;         /* output = input on next call */

    unsigned scpDcOffsetDeadBand;                /*! */
    unsigned scpDcOffsetGainShift;              /*! Coarse DC loop gain control (right shift) */
    unsigned scpDcOffsetGainMult;               /*! Fine DC loop gain control in Q.8 */

} AGC_CONFIG_T;

/*!
******************************************************************************

 @Function @AGC_Init

 <b>Description:</b>\n
 This function initialises the AGC context with the set up of the given SCP
 configuration and sets up the AGC in the SCP with default settings as required.
 To configure different settings, e.g. loop gains, DC input filter, use a
 read-modify-write approach with AGC_getConfig and AGC_Configure.

 \param[in]	scp					SCP to use
 \param[in,out] context			Common AGC code context.
 \param[in] SCPConfig			SCP configuration structure to be used.
 \param[in] agcMode				Is the agc in rapid or normal mode.
 \param[in] complexIF			This flag should be set to a non-zero value if the
 								output from the RF is complex.

******************************************************************************/
void AGC_Init(SCP_T *scp, AGC_CONTEXT_T *context, TDEV_SCP_CONFIG_T *SCPConfig,
				TDEV_AGC_MODE_T agcMode, long complexIF);

/*!
******************************************************************************

 @Function @AGC_Configure

 <b>Description:</b>\n
 This function configures parameters in the AGC context that are dependant on
 AGC period setup. An optional configuration structure can be passed otherwise
 default values are used, i.e. values returned by ::AGC_getDefaultConfig.

 \param[in,out] context			Common AGC code context.
 \param[in] agcMode				Is the agc in rapid or normal mode.
 \param[in] pConfig             Pointer to a configuration structure.
                                Use NULL to use default values otherwise
                                update with the given configuration.
                                Use AGC_getConfig first to get the current config.

******************************************************************************/
void AGC_Configure(AGC_CONTEXT_T *context, TDEV_AGC_MODE_T agcMode, AGC_CONFIG_T *pConfig);

/*!
******************************************************************************

 @Function @AGC_getConfig

 <b>Description:</b>\n
 This function fills the configuration structure from the given context.
 Use this for read/modify/write of the AGC configuration.

 \param[in]  context            Common AGC code context.
 \param[out] pConfig            Pointer to an empty configuration structure.

******************************************************************************/
void AGC_getConfig(AGC_CONTEXT_T *context, AGC_CONFIG_T *pConfig);


/*!
******************************************************************************

 @Function @AGC_getDefaultConfig

 <b>Description:</b>\n
 This function fills a configuration structure with the default AGC legacy settings
 for the select AGC mode. Use this for read/modify/write of the AGC configuration.

 \param[out] pConfig            Pointer to an empty configuration structure.
 \param[in]  agcMode            Is the agc in rapid or normal mode.

******************************************************************************/
void AGC_getDefaultConfig(AGC_CONFIG_T *pConfig, TDEV_AGC_MODE_T agcMode);

/*!
******************************************************************************

 @Function @AGC_DeInit

 <b>Description:</b>\n
 This function de-initialises the AGC; its context and SCP setup.
 Note this disables AGC interrupts if enabled at initialisation.

 \param[in]	scp				SCP to use
 \param[in,out] context		Common AGC code context.

******************************************************************************/
void AGC_DeInit(SCP_T *scp, AGC_CONTEXT_T *context);

/*!
******************************************************************************

 @Function @AGC_ReadSCPRegs

 <b>Description:</b>\n
 This function read the following from the SCP and reports them in the
 relevant fields of TDEV_AGCISR_HELPER_T.
 - AGC threshold1/2 counts
 - DC offset detected
 - IQ phase error

 If required this function will split the AGC period in to a number of smaller
 periods, so that you can be sure that the measurement of DC offset etc will
 not overflow the limited range available in the SCP harwdare.
 When this feature is used this function will accumulate the reading from the
 SCP and only return a non-zero value one the whole AGC period has been measured.

 \param[in]	scp			SCP to use
 \param[in,out] context	Common AGC code context.
 \param[in] SCPInfo		Collection of SCP information, AGC set up and control
 						loop calculation results.

  \return				Can run control loops now (non-zero value).

******************************************************************************/
int AGC_ReadSCPRegs(SCP_T *scp, AGC_CONTEXT_T *context, TDEV_AGCISR_HELPER_T *SCPInfo);

/*!
******************************************************************************

 @Function @AGC_CalcLoops

 <b>Description:</b>\n
 This function calculates \e all the control loops using the component 'AGC'
 loop functions listed.
 The resultant values are written into the TDEV_AGCISR_HELPER_T structure.

 \param[in,out] context	Common AGC code context.
 \param[in] SCPInfo		Collection of SCP information, AGC set up and control
 						loop calculation results.

******************************************************************************/
void AGC_CalcLoops(AGC_CONTEXT_T *context, TDEV_AGCISR_HELPER_T *SCPInfo);

/*!
******************************************************************************

 @Function @AGC_WriteSCPRegs

 <b>Description:</b>\n
 This function writes the calculated loop control values into the SCP.
 These include:
 - DC offset values
 - AGC threashold values
 - Early gain and angle control

 The CFOcorrection is \e not written to the SCP as it may need to interact with
 the physical layers carrier frequency control.

 \param[in]	scp			SCP to use
 \param[in,out] context	Common AGC code context.
 \param[in] SCPInfo		Collection of SCP information, AGC set up and control
 						loop calculation results.

******************************************************************************/
void AGC_WriteSCPRegs(SCP_T *scp, AGC_CONTEXT_T *context, TDEV_AGCISR_HELPER_T *SCPInfo);

/*!
 \name Component 'AGC' Loop Functions.
 Each of these loops calculations are completely independent.
*/
/*@{*/
/*!
******************************************************************************

 @Function @AGC_CalcGainLoop

 <b>Description:</b>\n
 This function calculates the IF gain control value based upon the threshold
 counter statistics.
 The threshold values are also adapted in an attempt to optimise the AGC
 operation.
 The resultant value is written into the TDEV_AGCISR_HELPER_T structure.

 \param[in,out] context	Common AGC code context.
 \param[in] SCPInfo		Collection of SCP information, AGC set up and control
 						loop calculation results.

******************************************************************************/
void AGC_CalcGainLoop(AGC_CONTEXT_T *context, TDEV_AGCISR_HELPER_T *SCPInfo);

/*!
******************************************************************************

 @Function @AGC_CalcDCOffsetLoop

 <b>Description:</b>\n
 This function calculates the control value to mitigate the measured DC offset,
 but only if the output from the RF is complex.
 The resultant values are written into the TDEV_AGCISR_HELPER_T structure.

 \param[in,out] context	Common AGC code context.
 \param[in] SCPInfo		Collection of SCP information, AGC set up and control
 						loop calculation results.

******************************************************************************/
void AGC_CalcDCOffsetLoop(AGC_CONTEXT_T *context, TDEV_AGCISR_HELPER_T *SCPInfo);

/*!
******************************************************************************

 @Function @AGC_CalcIQOffsetLoop

 <b>Description:</b>\n
 This function calculates the control value to mitigate the measured IQ phase error,
 but only if the output from the RF is complex.
 The resultant values are written into the TDEV_AGCISR_HELPER_T structure.

 \param[in,out] context	Common AGC code context.
 \param[in] SCPInfo		Collection of SCP information, AGC set up and control
 						loop calculation results.

******************************************************************************/
void AGC_CalcIQOffsetLoop(AGC_CONTEXT_T *context, TDEV_AGCISR_HELPER_T *SCPInfo);

/*@}*/

#endif /* COMMON_AGC_H */
