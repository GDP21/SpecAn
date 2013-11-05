/*!****************************************************************************
 @File          main.c

 @Title         Spectrum analyser core test main() function

 @Date          29 Nov 2012

 @Copyright     Copyright (C) Imagination Technologies Limited 2012

 @Description   main() for the example Spectrum Analyser Application

 ******************************************************************************/

#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <assert.h>
#include <MeOS.h>
#include "SPECAN_core.h"
#include "plt_setup.h"
#include "SPECAN_appCtrl.h"

#define MAX_TOTAL_SPECTRAL_SIZE_SUPPORTED (32768)

// Select config as either DVBT or DVBS
#define USE_CONFIG_DVBT



/* Top-level context structure.  Make it global so it can be viewed for debugging purposes */
typedef struct
{
    UFW_COREINSTANCE_T SAcoreInstance;
    TV_ACTIVATION_PARAMETER_T tList;
    TV_INSTANCE_T *tvInstance;
    void *SA_ctxPtr;
    KRN_FLAGCLUSTER_T flags;
    SPECAN_APP_EVENT_FLAG_T stateEventFlag;
    KRN_TASKQ_T mainTaskHibernateQ;
} SPECAN_TOP_CTX_STRUCT;
SPECAN_TOP_CTX_STRUCT SPECAN_topCtx;


/*
 * Event codes.
 *
 * With the exception of timeout events, these values are actually the corresponding event flag masks
 */
#define EVENT_TIMEOUT 0
#define EVENT_STATE (1 << 0)

/*-----------------------------------------------------------------------------*/

static void initTopCtx(SPECAN_TOP_CTX_STRUCT *ctx)
{
    /* Set these pointers up for debugging purposes only */
    ctx->tvInstance = ctx->SAcoreInstance.instanceExtensionData;
    ctx->SA_ctxPtr = ctx->tvInstance->tvInstanceExtensionData;

    ctx->stateEventFlag.flagCluster = &ctx->flags;
    ctx->stateEventFlag.flagMask = EVENT_STATE;

    DQ_init(&ctx->mainTaskHibernateQ);
}

/*-----------------------------------------------------------------------------*/

static void setUpEventMonitors(SPECAN_TOP_CTX_STRUCT *ctx)
{
    KRN_initFlags(ctx->stateEventFlag.flagCluster);
    SA_APP_mapEF(&ctx->SAcoreInstance, TV_REG_STATE, &ctx->stateEventFlag);
}

/*-----------------------------------------------------------------------------*/
#define TOTAL_PEAKS ((SA_MAX_POWER_REG_7-SA_MAX_POWER_REG_0)+1)

/* Structure of control parameters */
typedef struct
{
    unsigned startFreq;
    unsigned scanRange;
    unsigned scanResolution;
    unsigned bandwidth;
    unsigned tunerGridBase;
    unsigned tunerGridInc;
    SA_AVERAGING_PERIOD_N averagingPeriod_N;
    unsigned averagingPeriod_M;
    bool tuningStep_auto;
    unsigned manualTuningStep;
    unsigned maxPeakWidth;
    bool enableDcComp;
    SPECAN_WINDOW_T windowFunc;
    bool overrideIfGain;        /* if true, disables the AGC and uses a constant gain. */
    uint16_t ifGain;            /* gain used if overrideIfGain is true */    
} SPECAN_APP_CTRL_T;

#ifdef USE_CONFIG_DVBS
/* Configuration for DVB-S satellite TV (using STV6110 tuner) */
volatile SPECAN_APP_CTRL_T appConfig =
{
    470e6,          /* startFreq: 1008MHz */
    400e6,          /* scanRange: 120MHz */
    200000,         /* scanResolution:  0.2MHz */
    65e6/2,         /* Tuner bandwidth: 65MHz */
    0,              /* tunerGridBase */
    1000000,        /* tunerGridInc: 1MHz */
    SA_AVERAGING_PERIOD_N_64, /* averagingPeriod_N */
    8,              /* averagingPeriod_M. Gives a total of 8*64=512 averages */
    false,          /* Tuning step set to auto */
    22e6,           /* Manual tuning step - unused */
    4,              /* Peak width in bins +/- central peak. */
    true,           /* DC compensation enabled? */
    WINDOW_HAMMING,
    0,              /* Gain override flag */
    0               /* Gain Override value */
};
#endif


#ifdef USE_CONFIG_DVBT //This is the config being used
/* Configuration for DVB-T terrestrial TV (using Si2153 tuner) */
volatile SPECAN_APP_CTRL_T appConfig =
{
    474e6,          /* startFreq: 474MHz */
//    200e6,          /* startFreq: 200MHz */
//    320e6,          /* scanRange: 320MHz */
    100e6,          /* scanRange: 100MHz */
    100000,         /* scanResolution:  0.1MHz */
    9.142857e6/2,   /* Tuner bandwidth: 9.142857MHz (8MHz mode configuration on SCP) */
    0,              /* tunerGridBase */
    100000,         /* tunerGridInc: 100kHz */
    SA_AVERAGING_PERIOD_N_64, /* averagingPeriod_N */
    8,              /* averagingPeriod_M. Gives a total of 8*64=512 averages */
    false,          /* Auto tuning step (false = set to manual) */
    1e6,            /* Manual tuning step (not used if tuningStep_auto == true) */
    4,              /* Peak width in bins +/- central peak. */
    false,          /* DC compensation enabled? */
    WINDOW_HAMMING,
    false,          /* Gain override flag */
    0              /* Gain Override value (not used if gain override flag==0) */
};

#endif

/*-----------------------------------------------------------------------------*/

/* Status structure */
typedef struct
{
    TV_STATE_T          state;
    SA_FAILURE_CODE_E   failureCode;
    int8_t              peakVals[TOTAL_PEAKS];
    int16_t             peakIdx[TOTAL_PEAKS];
    unsigned            bufferAddress;
    unsigned            bufferSize;
    int32_t             RSSICurrent;
    int32_t             RSSI;
    int8_t              rssi; //new variable 05/11/13
    int8_t              rssimax; //
    //int32_t             freq_rssi;
    
} SPECAN_APP_STATUS_T;

volatile SPECAN_APP_STATUS_T appStatus;
/*-----------------------------------------------------------------------------*/
int32_t testRSSI = 500000;
static void setupScan()
{
	//put outputBuf here??? or inside main()
    UFW_COREINSTANCE_T *coreInstance = &SPECAN_topCtx.SAcoreInstance;
    unsigned val;
    TVREG_wrapperWrite(coreInstance, TV_REG_ACTIVE_TUNER_RSSI, testRSSI);  //TEST
    TVREG_wrapperWrite(coreInstance, SA_MAX_RSSI_REG, testRSSI); 
    
    SA_APP_tune(coreInstance, appConfig.startFreq, appConfig.bandwidth*2);
    TVREG_wrapperWrite(coreInstance, SA_SCAN_RANGE, appConfig.scanRange);
    TVREG_wrapperWrite(coreInstance, SA_SCAN_RESOLUTION, appConfig.scanResolution);
    TVREG_wrapperWrite(coreInstance, TV_REG_TUNER_GRID_BASE, appConfig.tunerGridBase);
    TVREG_wrapperWrite(coreInstance, TV_REG_TUNER_GRID_INCR, appConfig.tunerGridInc);
    val = appConfig.averagingPeriod_N | (appConfig.averagingPeriod_M << 8);
    TVREG_wrapperWrite(coreInstance, SA_AVERAGING_PERIOD, val);
    val = appConfig.manualTuningStep;
    if (appConfig.tuningStep_auto)
        val |= (1 << SA_REG_AUTO_TUNE_STEP_BITSHIFT);
    TVREG_wrapperWrite(coreInstance, SA_TUNING_STEP, val);

    val = ((appConfig.windowFunc   << SA_REG_WINDOW_TYPE_BITSHIFT) |
           (appConfig.enableDcComp << SA_REG_ENABLE_DC_COMP_BITSHIFT) |
           (appConfig.maxPeakWidth << SA_REG_MAX_PEAK_WIDTH_BITSHIFT));

    TVREG_wrapperWrite(coreInstance, SA_MEASUREMENT_CONTROL, val);

    /* IF Gain Override */
    val = ((appConfig.overrideIfGain << SA_REG_OVERRIDE_IF_GAIN_BITSHIFT) |
           (appConfig.ifGain         << SA_REG_IF_GAIN_BITSHIF ));
    TVREG_wrapperWrite(coreInstance, SA_IF_GAIN_OVERRIDE, val);
}

/*-----------------------------------------------------------------------------*/
static void readPeaksToApi(UFW_COREINSTANCE_T *coreInstance)
{
    unsigned j, i, val;

    for (j=0, i=SA_MAX_POWER_REG_0; i<=SA_MAX_POWER_REG_7; j++, i++)
    {
        val = TVREG_read(coreInstance, i);
        appStatus.peakVals[j] = (val >> SA_MAX_POWER_N_BITSHIFT) & SA_MAX_POWER_N_MASK;
        appStatus.peakIdx[j]  = (val >> SA_MAX_POWER_N_INDEX_BITSHIFT) & SA_MAX_POWER_N_INDEX_MASK;
    }
}

/*-----------------------------------------------------------------------------*/
//volatile uint16_t *outputBuf = 0xB0000000 + 4*(appStatus.bufferAddress & 0x00FFFFFF);
volatile int16_t RSSIMax = SA_MAX_RSSI_REG;  

int main()
{
    /* Initialise top-level system */
    UFW_init();
    UCCP_init();
    UCCP_reset();

    /* Platform setup */
    PLT_setup();
    PLT_INFO_T *pltSetup = PLT_query();

    /* This platform setup only handles a single tuner use case */
    SPECAN_topCtx.tList.tunerUseCount = 1;
    SPECAN_topCtx.tList.tunerUseList = pltSetup->tuner;

    /* Activate our core (fills in DVBScoreInstance).
    DVBSdescriptor is declared in DVBS_core.h. */
    if (!UFW_activateCore(SPECANdescriptor, 1, &SPECAN_topCtx.SAcoreInstance, &SPECAN_topCtx.tList))
    {
        assert(0);/* system failure - couldn't activate TV core */
        return 1;
    }

    /* Initialise our top-level context structure above */
    initTopCtx(&SPECAN_topCtx);
    setUpEventMonitors(&SPECAN_topCtx);

    /* Set up and run an initial scan */
    setupScan();
    SA_APP_startScan(&SPECAN_topCtx.SAcoreInstance, &SPECAN_topCtx.stateEventFlag, -1);

    /* Loop around monitoring the system status and checking if user wants to re-run the scan */
    while (1)
    {
        /* Update status structure */
        appStatus.state = TVREG_read(&SPECAN_topCtx.SAcoreInstance, TV_REG_STATE);
        
        /*update RSSI value*/
        if(appStatus.state == TV_STATE_DETECTING)
        {
            //NEW LINE 05/11/13 to extract rssi for returning absolute value. 
            //appStatus.rssi = TVTUNER_readRFPower(SA_ctx->tvInstance);
            int8_t rssiTmp;
            rssiTmp = (int8_t)(TVTUNER_readRFPower(SPECAN_topCtx.tvInstance) & 0x000000FF);
            appStatus.rssi = rssiTmp;
            if (rssiTmp > appStatus.rssimax)
            {
            appStatus.rssimax = rssiTmp;
            }
        }
        
        /* Check for completion. */
        if(appStatus.state == TV_STATE_COMPLETED)
        {
            appStatus.failureCode = TVREG_read(&SPECAN_topCtx.SAcoreInstance, SA_FAILURE_CODE);

            /* Get the memory location and length of the spectrum */
            //outputBuf definitions are here
            appStatus.bufferAddress = TVREG_read(&SPECAN_topCtx.SAcoreInstance, SA_REG_OUT_SPECTRUM_PTR);
            appStatus.bufferSize    = TVREG_read(&SPECAN_topCtx.SAcoreInstance, SA_OUT_SPECTRUM_LEN);
            
            //Read RSSI
            appStatus.RSSICurrent = TVREG_read(&SPECAN_topCtx.SAcoreInstance, TV_REG_ACTIVE_TUNER_RSSI);
            appStatus.RSSI = TVREG_read(&SPECAN_topCtx.SAcoreInstance, SA_MAX_RSSI_REG);
            
            /* Read the location and size of the largest peaks detected. */
            readPeaksToApi(&SPECAN_topCtx.SAcoreInstance); 
        }

        /* Hibernate to allow things to run. */
        KRN_hibernate(&SPECAN_topCtx.mainTaskHibernateQ, MS2TICKS(5));
        
    }
    
//    *outputBuf = 0xB0000000 + 4*(appStatus.bufferAddress & 0x00FFFFFF);
    return 0;
}
