/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/socs/oscar/agc/source/generic/oscar_agc_main.c,v $
**
** TITLE:       Oscar AGC
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Implementation of Oscar AGC
**
** NOTICE:      Copyright (C) 2008-2009, Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

#ifdef METAG
/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <math.h>


#include "oscar_agc.h"
#include "oscar_agc_install.h"
#include "oscar_agc_main.h"
#include "PHY_tuner.h"
/* Oscar SoC definitions from oscar_config */
#include "common.h"
#include "type_def.h"
#include "oscar_reg_defs.h"


/* Macro definitions */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// max mux IDs //
#define NUM_MUX_IDS                 (8)

/* PGA2 Parameters */
#define     AGC_AGCBYPASSMODE           (1)
#define     AGC_TGT_THRESH_COUNT_SHIFT  (15)
#define     ACG_VALUE_MIN               (0)
#define     ACG_VALUE_MAX               (65535)
#define     AGC2H_TH                    (42000)                 // AGCValue lower threshold to increase PGA1
#define     AGC2L_TH                    (32000)                 // AGCValue upper threshold to decrease PGA1
#define     AGC2L_GN                    (3)                     // PGA2 gain @ lower threshold AGCValue
#define     AGC2H_GN                    (16)                    // PGA2 gain @ upper threshold AGCValue
#define     dBRANGE_PGA2                (AGC2H_GN - AGC2L_GN)   // PGA2 gain range
#define     VALUEpDB                    ((AGC2H_TH-AGC2L_TH)/dBRANGE_PGA2)  // AGCValue per dB
#define     AGC2_MIN_GAIN_ERR_SHIFT     (5)                     // Min. gain error to apply AGCupdatePeriod >> x. (this value correspond to the error metric for PGA2 gain step 1dB)
#define     AGC_INIT_VALUE              (40000)

/* Digital Gain Parameters */
#define     DAGC_GAIN_STEP_MTS          (4871L)     // ctlGainPGA gain step mantissa
#define     DAGC_GAIN_STEP_IDX          (12)        // ctlGainPGA gain step index (2^-x) -> 4871*2^-12=1.18923...=1.5...dB
#define     DAGC_SCP_GAIN0dB_VAL        (256L)      // SCP early gain 0dB value
#define     DAGC_TH_H                   (((DAGC_SCP_GAIN0dB_VAL*256L)*DAGC_GAIN_STEP_MTS)>>DAGC_GAIN_STEP_IDX) // upper threshold scpEarlyGainx256 to increment ctlGainPGA
#define     DAGC_TH_L                   (((DAGC_SCP_GAIN0dB_VAL*256L)<<DAGC_GAIN_STEP_IDX)/DAGC_GAIN_STEP_MTS) // lower thershold scpEarlyGainx256 to decrement ctlGainPGA
#define     CTL_GAIN_PGA_MAX            (15)        // digital gain max value
#define     CTL_GAIN_PGA_MIN            (0)         // digital gain min value
#define     CTL_GAIN_PGA_MAX_TEMP       (6)        // digital gain max value larger than this leads to large DC offset.

/* Factor to convert PGA gain into dB */
#define PGA_TO_DB (1.51f)

/* PGA normalisation.
 * 20log10(4) because of 14 to 12 bit conversion.
 */
#define PGA_NORM (12.04119982655924780854955578898f)

/* ADC range
 * 20log10(1.4)
 */
#define ADC_NORM (2.9225607135647605185191030663426)

/* 20log10(65536) */
#define SCP_EARLY_GAIN_DB_NORM (96.329598612473982468396446311838f)

//#define FUDGE_DB (-3.5f)
#define FUDGE_DB (0.0f)

/*
** useful function define
*/
#define PHY_RESTRICT_RANGE(in, minval, maxval)      (((in)<=(minval))?(minval):(((in)>(maxval))?(maxval):(in)))

/* Left-shift applied to error in IF AGC */
#define IF_AGC_ERROR_SHIFT (3)
/* In order to converge Rapid AGCValue from 40000 to 65535 in 6ms */
#define MAX_RAPID_AGC_SLOPE_PER_SEC ((ACG_VALUE_MAX - AGC_INIT_VALUE)/(8)*1000)
/* In order to converge Demod AGCValue from 40000 to 65535 in 200ms */
#define MAX_DEMOD_AGC_SLOPE_PER_SEC ((ACG_VALUE_MAX - AGC_INIT_VALUE)/(200)*1000)
/* In order to converge Digital Rapid AGCValue from DAGC_TH_L to DAGC_TH_L in about 11ms */
#define MAX_RAPID_DIGITAL_AGC_SLOPE_PER_SEC (2048000)


/*
** RSSI over/under flow state enum
*/
typedef enum {
    RSSI_IN_RANGE   = 0,
    RSSI_UNDER_FLOW = 1,
    RSSI_OVER_FLOW  = 2
} RSSI_STATUS_T;

/*
** AGC state structure
*/
typedef struct {
    signed      long    agcValue;         // PGA2 gain index
    signed      char    lnaIndex;         // LNA  gain index
    signed      char    pga1Index;        // PGA1 gain index
    RSSI_STATUS_T       rssiStatus;       // Holding RSSI status under flow/over flow/in range
    signed      long    scpEarlyGainx256; // SCP early gain value x256
    signed      long    ctlGainPGA;       // ADC output gain setting
#ifdef RSSI_RF_AGC_ENABLE
    signed      long    rssiValue;        // temporal RSSI value
    signed      long    rssiFltValuex256; // filtered RSSI value x256
#endif
} AGC_STATE_T;

/*
** AGC parameters structure
*/
typedef struct {
    signed      long    disableAGC;
    signed      long    clipCount;
    signed      long    threshCount;
    signed      long    threshLevel;
    signed      long    clipLevel;
    signed      long    demodAGCScaleUp;
    signed      long    demodAGCScaleDn;
    signed      long    rapidAGCScaleUp;
    signed      long    rapidAGCScaleDn;
    signed      long    demodDigitalAGCScaleUp;
    signed      long    demodDigitalAGCScaleDn;
    signed      long    rapidDigitalAGCScaleUp;
    signed      long    rapidDigitalAGCScaleDn;
    signed      long    targetThresholdCount;
    signed      long    AGCCounter;              // counter for count AGC ISR call
#ifdef RSSI_RF_AGC_ENABLE
    signed      long    initRSSIFilter;
    signed      long    enableRSSIUpdate;
    signed      long    leakageFactorUp;
    signed      long    leakageFactorDn;
    signed      long    rapidIntervalTick;
    signed      long    demodIntervalTick;
    signed      long    intervalTick;
    signed      long    rapidRFAGCUpdateCount;
    signed      long    demodRFAGCUpdateCount;
#endif
} AGC_PARAMS_T;

typedef enum {
    POWER_DOWN_ADC = 0,
    ACTIVE_ADC     = 1
} ADC_POWER_CTRL_T, *pADC_POWER_CTRL_T;

/*
** Parameters
*/
AGC_PARAMS_T    AGCParams;                  // AGC parameters
AGC_STATE_T     AGCStatus[NUM_MUX_IDS];     // AGCState for each muxID
AGC_STATE_T     *pCurrentAGCStatus;         // pointer to the current AGCState
PHY_TUNER_STANDARD_T demodStandard = PHY_TUNER_NOT_SIGNALLED;
long            bandwidth = 0;
unsigned long sampleRate=0;

/*
** Function Prototypes
*/
static unsigned long calculate_log2(unsigned long);
void updateAGCloopgains(TUNER_AGCISR_HELPER_T*);

/*
** FUNCTION:    initAGCParam
**
** DESCRIPTION: Initialise AGCParam structure
**
** PARAMETERS:  void
**
** RETURNS:     void
*/
static void initAGCParam(void)
{
    AGCParams.disableAGC                = 0;
    AGCParams.clipCount                 = 0;
    AGCParams.threshCount               = 0;
    AGCParams.threshLevel               = 3;
    AGCParams.clipLevel                 = 5;
    AGCParams.demodAGCScaleUp           = 8 + IF_AGC_ERROR_SHIFT;
    AGCParams.demodAGCScaleDn           = 7 + IF_AGC_ERROR_SHIFT;
    AGCParams.demodDigitalAGCScaleUp    = 6;
    AGCParams.demodDigitalAGCScaleDn    = 5;
    AGCParams.targetThresholdCount      = (signed long)((1 << AGC_TGT_THRESH_COUNT_SHIFT) * 0.3434 * 0.56671693565266453115899825276645);
    AGCParams.rapidAGCScaleUp           = 3 + IF_AGC_ERROR_SHIFT;
    AGCParams.rapidAGCScaleDn           = 2 + IF_AGC_ERROR_SHIFT;
    AGCParams.rapidDigitalAGCScaleUp    = 3;
    AGCParams.rapidDigitalAGCScaleDn    = 2;
#ifdef RSSI_RF_AGC_ENABLE
    AGCParams.enableRSSIUpdate          = 0; // disable update RSSI at the begining
    AGCParams.intervalTick              = 2;
    AGCParams.leakageFactorUp           = (1<<(READRSSI_LEAKAGE_FACTOR_SHIFT-2));  // (1/4)
    AGCParams.leakageFactorDn           = (1<<(READRSSI_LEAKAGE_FACTOR_SHIFT-10)); // (1/1024)
    AGCParams.initRSSIFilter            = 1;
    AGCParams.rapidIntervalTick         = 0;
    AGCParams.demodIntervalTick         = 4;
    AGCParams.rapidRFAGCUpdateCount     = 8;
    AGCParams.demodRFAGCUpdateCount     = 64;
#endif
}

/*
** FUNCTION:    initAGCStatus
**
** DESCRIPTION: Initialise AGCStatus structure
**
** PARAMETERS:  AGC_STATE_T *pAGCStatus
**
** RETURNS:     void
*/
static void initAGCStatus(AGC_STATE_T *pAGCStatus)
{
    if (AGCParams.disableAGC == 0)
    {
        pAGCStatus->agcValue            = AGC_INIT_VALUE;
        pAGCStatus->lnaIndex            = 5;
        pAGCStatus->pga1Index           = 5;
    }
    pAGCStatus->rssiStatus          = RSSI_IN_RANGE;
    pAGCStatus->scpEarlyGainx256    = 256 * DAGC_SCP_GAIN0dB_VAL;
    pAGCStatus->ctlGainPGA          = 2;
#ifdef RSSI_RF_AGC_ENABLE
    pAGCStatus->rssiValue           = 0;
    pAGCStatus->rssiFltValuex256    = 0;
#endif
}

/*!
******************************************************************************
**
** @par Function:
**         s_SetADCPowerCtrl
**
** @brief
**         Power management for DS ADC.
**
** @param[in]
**         ePowerState        This indicates the logic state of the ADC power.
**                            (POWER_DOWN_ADC or ACTIVE_ADC).
**
** @retval
**         none
**
** @note
**         Power down control of I DS ADC core. (positive logic)
**         Power down control of Q DS ADC core. (positive logic)
**         Power down control of voltage reference circuit. (negative logic)
**         Reset signal for intergrations in the DS ADC core. (positive logic)
**
******************************************************************************/

static void setADCPowerCtrl(ADC_POWER_CTRL_T ePowerState)
{
    img_uint32  ui32RegVal = 0;

    ui32RegVal = READ_REG(OSCAR_REGS, CR_ADC_CTL) &
        ((CR_ADC_CTRL_DV_MASK)      |
         (CR_ADC_IF_SEL_I_MASK)     |
         (CR_ADC_IF_SEL_Q_MASK)     |
         (CR_ADC_DWA_ON_MASK)       |
         (CR_ADC_CTRL_STD_MASK)     |
         (CR_ADC_ENABLE_I_MASK)     |
         (CR_ADC_ENABLE_Q_MASK)     |
         (CR_ADC_NENABLE_VREF_MASK) |
         (CR_ADC_CTRL_VTH_MASK)     |
         (CR_ADC_RESET_ANALOG_MASK) |
         (CR_ADC_CTRL_BIAS_MASK)    |
         (CR_ADC_CTRL_DF_MASK)      |
         (CR_ADC_CTRL_DUTY_MASK)    |
         (CR_ADC_ENB_LPF_MASK));
    ui32RegVal &= ~(CR_ADC_ENABLE_I_MASK     |
                    CR_ADC_ENABLE_Q_MASK     |
                    CR_ADC_NENABLE_VREF_MASK |
                    CR_ADC_RESET_ANALOG_MASK);

    if (ePowerState == ACTIVE_ADC) {
        ui32RegVal |= ((IMG_ENABLE  << CR_ADC_ENABLE_I_SHIFT)     |
                       (IMG_ENABLE  << CR_ADC_ENABLE_Q_SHIFT)     |
                       (IMG_DISABLE << CR_ADC_NENABLE_VREF_SHIFT) |
                       (IMG_DISABLE << CR_ADC_RESET_ANALOG_SHIFT));
    } else {
        ui32RegVal |= ((IMG_DISABLE << CR_ADC_ENABLE_I_SHIFT)     |
                       (IMG_DISABLE << CR_ADC_ENABLE_Q_SHIFT)     |
                       (IMG_ENABLE  << CR_ADC_NENABLE_VREF_SHIFT) |
                       (IMG_ENABLE  << CR_ADC_RESET_ANALOG_SHIFT));
    }

    WRITE_REG(OSCAR_REGS, CR_ADC_CTL, ui32RegVal);
}

/*
** FUNCTION:    updateBBGain
**
** DESCRIPTION: update BB gain based on AGC counter in SCP. This function controls
**              two gains, one is in ADC the other is in SCP early gain.
**              Coarse gain setting is set to ADC gain and fine one is set into SCP gain.
**
** PARAMETERS:  long rapidAGCModeFlag
**                      TRUE for the rapid AGC mode, FALSE for the normal AGC mode
**              TUNER_AGCISR_HELPER_T *pAgcIsrHelper
**                      pointer to the structure holding SCP information
**
** RETURNS:     void
*/
static void updateBBGain(long rapidAGCModeFlag, TUNER_AGCISR_HELPER_T  *pAgcIsrHelper)
{
    long error;
    long AGCcount1;

    if (oscarAgcTuner->complexIF)
	    AGCcount1 = pAgcIsrHelper->AGCcount1I + pAgcIsrHelper->AGCcount1Q;
	else
		/* If real input (I only) double I count, ignoring Q and scale to same range/sense as a complex input */
	    AGCcount1 = pAgcIsrHelper->AGCcount1I << 1;


    // make error=0 for invalid value of AGCcount
    if (AGCcount1 > (pAgcIsrHelper->AGCupdatePeriod<<1))
        AGCcount1 = (pAgcIsrHelper->AGCupdatePeriod);

    // calc error
    error = (pAgcIsrHelper->AGCupdatePeriod - AGCcount1) >> 1;

    // scaling error
    if (error > 0)
        error >>= ((rapidAGCModeFlag)?(AGCParams.rapidDigitalAGCScaleUp):(AGCParams.demodDigitalAGCScaleUp));
    else
        error >>= ((rapidAGCModeFlag)?(AGCParams.rapidDigitalAGCScaleDn):(AGCParams.demodDigitalAGCScaleDn));

    // update loop
    pCurrentAGCStatus->scpEarlyGainx256 = pCurrentAGCStatus->scpEarlyGainx256 + error;
    pCurrentAGCStatus->scpEarlyGainx256 = PHY_RESTRICT_RANGE(pCurrentAGCStatus->scpEarlyGainx256, 64*256, 1023*256);

    // IFgainValue over theshold, then increment PGA gain
    if (pCurrentAGCStatus->scpEarlyGainx256 > DAGC_TH_H)
    {
        if (pCurrentAGCStatus->ctlGainPGA < CTL_GAIN_PGA_MAX_TEMP)
        {
            pCurrentAGCStatus->ctlGainPGA++;
            pCurrentAGCStatus->scpEarlyGainx256 = (pCurrentAGCStatus->scpEarlyGainx256 << DAGC_GAIN_STEP_IDX) / DAGC_GAIN_STEP_MTS; // scpEarlyGainx256 /= 1.18923
            Tuner_SetExtOffset2(pCurrentAGCStatus->ctlGainPGA << 8);
        }
    }
    // IFgainValue below theshold, then decrement PGA gain
    if (pCurrentAGCStatus->scpEarlyGainx256 < DAGC_TH_L)
    {
        if (pCurrentAGCStatus->ctlGainPGA > CTL_GAIN_PGA_MIN)
        {
            pCurrentAGCStatus->ctlGainPGA--;
            pCurrentAGCStatus->scpEarlyGainx256 = (pCurrentAGCStatus->scpEarlyGainx256 * DAGC_GAIN_STEP_MTS) >> DAGC_GAIN_STEP_IDX; // scpEarlyGainx256 *= 1.18923
            Tuner_SetExtOffset2(pCurrentAGCStatus->ctlGainPGA << 8);
        }
        else
            pCurrentAGCStatus->scpEarlyGainx256 = DAGC_TH_L;
    }
    // calc SCP gain
    if (!rapidAGCModeFlag)
    {
    pAgcIsrHelper->pSCPcontrol->fineGainI = (pAgcIsrHelper->pSCPcontrol->fineGainI * (pCurrentAGCStatus->scpEarlyGainx256>>8)) >> 8;
    pAgcIsrHelper->pSCPcontrol->fineGainQ = (pAgcIsrHelper->pSCPcontrol->fineGainQ * (pCurrentAGCStatus->scpEarlyGainx256>>8)) >> 8;
    pAgcIsrHelper->pSCPcontrol->fineGainI = PHY_RESTRICT_RANGE(pAgcIsrHelper->pSCPcontrol->fineGainI, -(1<<10), ((1<<10)-1));
    pAgcIsrHelper->pSCPcontrol->fineGainQ = PHY_RESTRICT_RANGE(pAgcIsrHelper->pSCPcontrol->fineGainQ, -(1<<10), ((1<<10)-1));
    }
}

/*
** FUNCTION:    updateIFGain
**
** DESCRIPTION: update IF gain based on AGC counter in ADC. This function mainly controls
**              PGA2 which is the last gain in the tuner.
**
** PARAMETERS:  long rapidAGCModeFlag
**                      TRUE for the rapid AGC mode, FALSE for the normal AGC mode
**              TUNER_AGCISR_HELPER_T *pAgcIsrHelper
**                      pointer to the structure holding SCP information
**
** RETURNS:     void
*/
static void updateIFGain(long rapidAGCModeFlag, TUNER_AGCISR_HELPER_T  *pAgcIsrHelper)
{
    long error;

    // get detector results
    AGCParams.clipCount   = (*(volatile unsigned int *)((0x04800000)+(0x30FC)) & (0xFFFF0000))>>16;
    AGCParams.threshCount = (*(volatile unsigned int *)((0x04800000)+(0x30FC)))&(0xFFFF);

    // make error=0 for invalid threshCount
    if (AGCParams.threshCount > pAgcIsrHelper->AGCupdatePeriod)
        AGCParams.threshCount = ((pAgcIsrHelper->AGCupdatePeriod * AGCParams.targetThresholdCount) >> AGC_TGT_THRESH_COUNT_SHIFT);

    // calc error
    error = (((pAgcIsrHelper->AGCupdatePeriod * AGCParams.targetThresholdCount) >> AGC_TGT_THRESH_COUNT_SHIFT) - (AGCParams.threshCount));

    // small errors cored to zero
    if ( (error > -(pAgcIsrHelper->AGCupdatePeriod >> AGC2_MIN_GAIN_ERR_SHIFT)) && (error < (pAgcIsrHelper->AGCupdatePeriod >> AGC2_MIN_GAIN_ERR_SHIFT)) )
        error = 0;

    // pre-left-shift error in order to avoid right-shifting by a negative number in the next step
    error <<= IF_AGC_ERROR_SHIFT;

    // scaling error
    if (error > 0)
        error >>= ((rapidAGCModeFlag)?(AGCParams.rapidAGCScaleUp):(AGCParams.demodAGCScaleUp));
    else
        error >>= ((rapidAGCModeFlag)?(AGCParams.rapidAGCScaleDn):(AGCParams.demodAGCScaleDn));

    // update loop if not disabled
    if (AGCParams.disableAGC == 0)
    {
        pCurrentAGCStatus->agcValue = pCurrentAGCStatus->agcValue + error;
    }

    pCurrentAGCStatus->agcValue = PHY_RESTRICT_RANGE(pCurrentAGCStatus->agcValue, ACG_VALUE_MIN, ACG_VALUE_MAX);

}

/*
** FUNCTION:    AGCLoop
**
** DESCRIPTION: Top level function for the IF/BB AGC loop
**
** PARAMETERS:  long rapidAGCModeFlag
**                      TRUE for rapid AGC mode, FALSE for normal AGC mode
**              TUNER_AGCISR_HELPER_T *pAgcIsrHelper
**                      Structre holding SCP information
**
** RETURNS:     void
*/
static void AGCLoop(long rapidAGCModeFlag, TUNER_AGCISR_HELPER_T *pAgcIsrHelper)
{
#ifdef DEBUG_AGC
    if (rapidAGCModeFlag)
        agcStateLog[agcStateLogCounter] = AGC_ACQ_IFAGC;
    else
        agcStateLog[agcStateLogCounter] = AGC_NOR_IFAGC;
    if (agcStateLogCounter<AGC_STATE_LOG_LENGTH) agcStateLogCounter++;
#endif

    /* update BB Gain */
    updateBBGain(rapidAGCModeFlag, pAgcIsrHelper);

    /* update IF Gain */
    updateIFGain(rapidAGCModeFlag, pAgcIsrHelper);
}

void _OSCAR_AGC_initialise(void)
{
    initAGCParam();
}

void _OSCAR_AGC_initAGC(unsigned long muxID)
{
    pCurrentAGCStatus = &(AGCStatus[muxID]);

    initAGCStatus(pCurrentAGCStatus);
}


void _OSCAR_AGC_config(PHY_TUNER_STANDARD_T standard, long bandwidthHz)
{
    demodStandard = standard;
    bandwidth = bandwidthHz;

    if (standard == PHY_TUNER_DAB)
    {
        AGCParams.demodAGCScaleUp           = 4;
        AGCParams.demodAGCScaleDn           = 3;
        AGCParams.demodDigitalAGCScaleUp    = 3;
        AGCParams.demodDigitalAGCScaleDn    = 2;
        AGCParams.targetThresholdCount      = (signed long)((1 << AGC_TGT_THRESH_COUNT_SHIFT) * 0.3434);
    }

    if (standard == PHY_TUNER_FM)
    {
        AGCParams.demodAGCScaleUp           = 6;
        AGCParams.demodAGCScaleDn           = 5;
        AGCParams.demodDigitalAGCScaleUp    = 3;
        AGCParams.demodDigitalAGCScaleDn    = 2;
        AGCParams.targetThresholdCount      = (signed long)((1 << AGC_TGT_THRESH_COUNT_SHIFT) * 0.3434);
	}

}

void _OSCAR_AGC_powerUp(unsigned long muxID)
{
    setADCPowerCtrl(ACTIVE_ADC);
    pCurrentAGCStatus = &(AGCStatus[muxID]);
}

void _OSCAR_AGC_powerDown(unsigned long muxID)
{
    (void)muxID;

#ifdef RSSI_RF_AGC_ENABLE
    AGCParams.intervalTick = AGCParams.demodIntervalTick;
    AGCParams.enableRSSIUpdate = 0;
#endif
    setADCPowerCtrl(POWER_DOWN_ADC);
}

void _OSCAR_AGC_powerSave(PHY_RF_PWRSAV_T pwsav, unsigned long muxID)
{
    (void)pwsav;
    (void)muxID;

#ifdef RSSI_RF_AGC_ENABLE
    AGCParams.intervalTick = AGCParams.demodIntervalTick;
    AGCParams.enableRSSIUpdate = 0;
#endif

    switch (pwsav)
    {
            /* For power saving power down the ADC */
        case PWRSAV_LEVEL1:
        case PWRSAV_LEVEL2:
            setADCPowerCtrl(POWER_DOWN_ADC);
            break;

            /* For anything else power up the ADC */
        case PWRSAV_OFF:
        case PWRSAV_INVALID:
        default:
            setADCPowerCtrl(ACTIVE_ADC);
            break;
    }

}

/*
** Calculates approximate Log with base2.
*/
static unsigned long calculate_log2(unsigned long n)
{
    int i;
    unsigned long log2n=0;

    i = n<<5; /* the left shift by 4 reduces rounding error to about 1/(2^4) */
    while (i>=45){
        i >>= 1;
        log2n++;
    }
    return log2n;
}

/*
** Calculates rapid AGC loop gains from sampleRate and required maximum convergence rate.
*/
void updateAGCloopgains(TUNER_AGCISR_HELPER_T *pAgcIsrHelper)
{
    unsigned long sampleRateshift           = calculate_log2(pAgcIsrHelper->sampleRate);
    unsigned long rapidAGCSlopeShift        = calculate_log2((MAX_RAPID_AGC_SLOPE_PER_SEC/AGCParams.targetThresholdCount)<<AGC_TGT_THRESH_COUNT_SHIFT);
    unsigned long rapidDigitalAGCSlopeShift = calculate_log2(MAX_RAPID_DIGITAL_AGC_SLOPE_PER_SEC*2);
    unsigned long demodAGCSlopeShift        = calculate_log2((MAX_DEMOD_AGC_SLOPE_PER_SEC/AGCParams.targetThresholdCount)<<AGC_TGT_THRESH_COUNT_SHIFT);
    AGCParams.rapidAGCScaleUp       = sampleRateshift - rapidAGCSlopeShift + IF_AGC_ERROR_SHIFT;
    AGCParams.rapidAGCScaleDn       = AGCParams.rapidAGCScaleUp;
    AGCParams.rapidDigitalAGCScaleUp= sampleRateshift - rapidDigitalAGCSlopeShift;
    AGCParams.rapidDigitalAGCScaleDn= AGCParams.rapidDigitalAGCScaleUp;
    AGCParams.demodAGCScaleUp       = sampleRateshift - demodAGCSlopeShift + IF_AGC_ERROR_SHIFT;
    AGCParams.demodAGCScaleDn       = AGCParams.demodAGCScaleUp;
    sampleRate = pAgcIsrHelper->sampleRate;
}


void _OSCAR_AGC_setAGC(TUNER_AGCISR_HELPER_T *pAgcIsrHelper)
{
    // Calculate Rapid AGC and Demod AGC loop gains from input sample rate.
    if (pAgcIsrHelper->sampleRate != sampleRate	&&
    	demodStandard != PHY_TUNER_DAB &&
    	demodStandard != PHY_TUNER_FM
    	)
        updateAGCloopgains(pAgcIsrHelper);

    if (pAgcIsrHelper->AGCupdatePeriod == 0)
    {
        // set Digital gain
        Tuner_SetExtOffset2(pCurrentAGCStatus->ctlGainPGA << 8);
    }
    else if (pAgcIsrHelper->AGCMode == PHY_TUNER_RAPID_AGC)
    {
        AGCLoop(TRUE,pAgcIsrHelper);
    }
    else
    {
        // agc loop calc for normal AGC and update tuner
        AGCLoop(FALSE, pAgcIsrHelper);
    }

    pAgcIsrHelper->IFgainValue = pCurrentAGCStatus->agcValue;
}

float _OSCAR_AGC_levelTrans(float rms)
{
    float gain;

    /* ADC norm */
    rms += ADC_NORM;

    /* Remove SCP early gain */
    gain = 20.0f*log10f(pCurrentAGCStatus->scpEarlyGainx256) -
        SCP_EARLY_GAIN_DB_NORM;
    rms -= gain;

    /* Remove PGA gain */
    rms += PGA_NORM;
    gain = pCurrentAGCStatus->ctlGainPGA*PGA_TO_DB;
    rms -= gain;

    /* Fudge */
    //rms += FUDGE_DB;

    return rms;
}
