/* Keep these first ... */
#ifdef METAG
#define METAG_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif
#include <string.h>
#include "uccrt.h"

#include "plt_soc_setup.h"

#include "img_defs.h"   // as required by sys_config.h
#include "sys_config.h"
#include "system.h"

/* Clock rates in Q12.20 format */
#ifdef GENERIC_TUNER
    /* When building with generic tuner driver set up 80MHz ADC clock.
    This frequency will need to be plugged into the front end override registers
    at run time. */
    #define ADC_CLK_FREQ    ((int)(80 * (1<<20)))   // 80MHz
#else
    #ifdef SCP_80000000
        #define ADC_CLK_FREQ    ((int)(80 * (1<<20)))   // 80MHz
    #else
        #ifdef SCP_24576000
            #define ADC_CLK_FREQ        (0x0189374B)            // 24.576MHz
        #else
            #ifdef SCP_24575066
                #define ADC_CLK_FREQ        (0x01893378)            // 24.575066MHz
            #else
                #ifdef SCP_61440000
                    #define ADC_CLK_FREQ    (0x3D70A3D)
                #else
                    #error "Unsupported IF clock rate"
                #endif
            #endif
        #endif
    #endif
#endif

/* Macro for writing to HW registers */
#define WRITE(A,V)  (*((volatile unsigned int *)(A))=((unsigned int)V))


void PLT_setupAFE(unsigned ADCclkFreq, bool bypassPll)
{
    SYS_sAFEConfig afeConfig;

    /* Clear config structure */
    memset(&afeConfig, 0 , sizeof(afeConfig));

    /*! Override the default clock divider, must be set to false if the API should calculate
    the divider values for a target frequency */
    afeConfig.bOverrideDivider = IMG_FALSE;

    /*! Override the default clock source for blocks that have multiple possible sources */                             \
    afeConfig.bOverrideClockSource = IMG_TRUE;                                                                  \
    /*! The clock source to use */                                                                                      \
    afeConfig.eClockSource = CLOCK_SOURCE_XTAL1;                                                                            \

    afeConfig.sRxADC.bConfigure = IMG_TRUE;
    afeConfig.sRxADC.bEnable = IMG_TRUE;

    /* If the AFE PLL is enabled, setting this to the desired sample rate will configure the dividers appropriately */
    afeConfig.sRxADC.ui32SampleRate_fp = ADCclkFreq;

    /* Configures the RXADC input voltage range */
    afeConfig.sRxADC.eInputRange = RANGE_1V_P2P;


    afeConfig.sIQADC.bConfigure = IMG_TRUE;
    afeConfig.sIQADC.bEnable = IMG_TRUE;

    /* If the AFE PLL is enabled, setting this to the desired sample rate will configure the dividers appropriately */
    afeConfig.sIQADC.ui32SampleRate_fp = ADCclkFreq;

    /* Set to true to use the AFE's internal PLL as the IQADC's clock source */
    /* Set to false to use the external clock input. This source can be configured through BLOCK_ADCDIVCLOCK */
    if (bypassPll)
        afeConfig.sIQADC.bUseInternalPLL = IMG_FALSE;
    else
        afeConfig.sIQADC.bUseInternalPLL = IMG_TRUE;

    /* If the above is set to false, use the following to configure the external IQADC clock input
    -- Possible values are: XTAL1, XTAL2, ADC_PLL, SYS_CLK_UNDELETED */
    afeConfig.sIQADC.sExtClock.eClockSource = CLOCK_SOURCE_XTAL1;

    /* Set to true to override the ADCPLLDIV divider manually. Must be set to false if the API should calculate
    the divider values for a target frequency (bTargetExternalFreq) */
    afeConfig.sIQADC.sExtClock.bOverrideDivider = IMG_FALSE;

    /* Set to true to have the API calculate the required ADCPLLDIV divider for the target frequency specified below */
    afeConfig.sIQADC.sExtClock.bTargetFreq = IMG_TRUE;

    /* The desired external clock frequency, in MHz, Q12.20 format */
    afeConfig.sIQADC.sExtClock.ui32TargetFreq_fp = ADCclkFreq;

    /* Configures the IQADC input voltage range */
    afeConfig.sIQADC.eInputRange = RANGE_1V_P2P;

    /* Set to true when bEnablePLL is true to internally bypass the AFE's internal PLL.
    This will result in the Fin being the same as Fvco */
    if (bypassPll)
    {
        afeConfig.bEnablePLL = IMG_FALSE;
        afeConfig.bBypassPLL = IMG_TRUE;
    }
    else
    {
        afeConfig.bEnablePLL = IMG_TRUE;
        afeConfig.bBypassPLL = IMG_FALSE;
    }

    SYS_ConfigureAFE(&afeConfig);
}





void PLT_setupSOC(PLT_INFO_T *info)
{
    info->socName = "Saturn2";
    /* Enable TSO pads as TSO, not GPIO */
    WRITE(0x02015814,0xFFFF0003);


{
    SYS_sConfig sysConfig;

    /* Clear config structure */
    memset(&sysConfig, 0 , sizeof(sysConfig));

    /* Set to true to set up the system clock */
    sysConfig.bSetupSystemClock = IMG_FALSE;

    sysConfig.sUCCConfig.bEnable = IMG_TRUE;
    sysConfig.sUCCConfig.bEnableIFClock = IMG_TRUE;
    sysConfig.sUCCConfig.eIFClockSource = CLOCK_SOURCE_AFE_RXSYNC;

    SYS_Configure(&sysConfig);
}

{
    SYS_sSCBConfig  sSCBConfig;

    sSCBConfig.bOverrideClockSource = IMG_TRUE;
    sSCBConfig.eClockSource = CLOCK_SOURCE_SYS_UNDELETED;
    sSCBConfig.asBlockConfig[0].bEnable = IMG_TRUE;
    sSCBConfig.asBlockConfig[0].bConfigure = IMG_TRUE;
    sSCBConfig.asBlockConfig[1].bEnable = IMG_TRUE;
    sSCBConfig.asBlockConfig[1].bConfigure = IMG_TRUE;
    sSCBConfig.asBlockConfig[2].bEnable = IMG_TRUE;
    sSCBConfig.asBlockConfig[2].bConfigure = IMG_TRUE;

    SYS_ConfigureSCB(&sSCBConfig);
}

#ifdef SCP_24576000
    PLT_setupAFE(ADC_CLK_FREQ, IMG_TRUE);
#else
    PLT_setupAFE(ADC_CLK_FREQ, IMG_FALSE);
#endif

{
    SYS_sPDMConfig pdmConfig;

    /* Clear config structure */
    memset(&pdmConfig, 0 , sizeof(pdmConfig));

    pdmConfig.asBlockConfig[0].bConfigure = IMG_TRUE;
    pdmConfig.asBlockConfig[0].bEnable = IMG_TRUE;
    pdmConfig.asBlockConfig[0].ePadSource = PDM_PAD_SOURCE_PDM;
    pdmConfig.asBlockConfig[0].eControlSource = PDM_SOURCE_UCC0_EXT_CONTROL_1;

//  pdmConfig.asBlockConfig[1].bConfigure = IMG_TRUE;
//  pdmConfig.asBlockConfig[1].bEnable = IMG_TRUE;
//  pdmConfig.asBlockConfig[1].ePadSource = PDM_PAD_SOURCE_PDM;
//  pdmConfig.asBlockConfig[1].eControlSource = PDM_SOURCE_UCC1_EXT_CONTROL_1;
//  //pdmConfig.asBlockConfig[1].eGainSource = ;

    SYS_ConfigurePDM(&pdmConfig);
}
#if 1
{
    SYS_sClockOutConfig clkoutConfig[2];

    /* Clear config structure */
    memset(clkoutConfig, 0 , sizeof(clkoutConfig));

    clkoutConfig[0].bConfigure = IMG_TRUE;
    clkoutConfig[0].bEnable = IMG_TRUE;
    clkoutConfig[0].bOverrideClockSource = IMG_TRUE;
    clkoutConfig[0].eClockSource = CLOCK_SOURCE_UCC0_IF;
#ifdef SCP_61440000
    clkoutConfig[0].bOverrideDivider = IMG_TRUE;
    clkoutConfig[0].ui8Divider = 0;
#endif
    clkoutConfig[1].bConfigure = IMG_TRUE;
    clkoutConfig[1].bEnable = IMG_TRUE;
    clkoutConfig[1].bOverrideClockSource = IMG_TRUE;
    clkoutConfig[1].eClockSource = CLOCK_SOURCE_XTAL1;

    SYS_ConfigureClockOut(clkoutConfig);
}
#endif

    return;
}
