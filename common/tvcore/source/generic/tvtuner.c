/*!****************************************************************************
 * @File          tvtuner.c
 *
 * @Title         TV core tuner access module
 *
 * @Date          7 Jan 2011
 *
 * @Copyright     Copyright (C) Imagination Technologies Limited 2012
 *
 * @Description   Provides a wrapper around the TUNER_ function family to allow
 *                synchronisation of tuner activity and feedback registers
 *
 ******************************************************************************/
#include "tvcore.h"

bool
TVTUNER_tuneIsImplemented(TV_INSTANCE_T *tvInstance)
{
    return TUNER_tuneIsImplemented(&tvInstance->tuner);
}

/*--------------------------------------------------------------------------*/

bool
TVTUNER_tune(TV_INSTANCE_T *tvInstance, int rfFrequency)
{
    bool status;

    KRN_lock(&tvInstance->tunerLock, KRN_INFWAIT);
    status = TUNER_tune(&tvInstance->tuner, rfFrequency);
    TVREG_coreWrite(tvInstance->coreInstance, TV_REG_ACTIVE_TUNER_FREQ,
                    rfFrequency);
    tvInstance->activeFrequency = rfFrequency;
    KRN_unlock(&tvInstance->tunerLock);
    return status;
}

/*--------------------------------------------------------------------------*/

bool
TVTUNER_configure(TV_INSTANCE_T *tvInstance, int rfBandwidth)
{
    TV_COREDESC_EXTENSION_T *tvExt =
            tvInstance->coreInstance->coreDesc->coreExtension;
    bool status;

    KRN_lock(&tvInstance->tunerLock, KRN_INFWAIT);
    status = TUNER_configure(&tvInstance->tuner, tvExt->tvStandard,
                             rfBandwidth);
    TVREG_coreWrite(tvInstance->coreInstance, TV_REG_ACTIVE_TUNER_BW,
                    rfBandwidth);
    tvInstance->activeBandwidth = rfBandwidth;
    KRN_unlock(&tvInstance->tunerLock);
    return status;
}

/*--------------------------------------------------------------------------*/

bool
TVTUNER_setSCP(TV_INSTANCE_T *tvInstance, unsigned config)
{
    return TUNER_setSCP(&tvInstance->tuner, config);
}

/*--------------------------------------------------------------------------*/

bool
TVTUNER_init(TV_INSTANCE_T *tvInstance, TUNER_ACTIVE_CONFIG_T *activeConfigs,
             unsigned numConfigs)
{
    KRN_initLock(&tvInstance->tunerLock);
    return TUNER_init(&tvInstance->tuner, tvInstance->tunerUse, activeConfigs,
                      numConfigs, tvInstance->tunerWorkspace);
}

/*--------------------------------------------------------------------------*/

bool
TVTUNER_reset(TV_INSTANCE_T *tvInstance, unsigned config)
{
    return TUNER_reset(&tvInstance->tuner, config);
}

/*--------------------------------------------------------------------------*/

bool
TVTUNER_initAGC(TV_INSTANCE_T *tvInstance, unsigned muxId)
{
    return TUNER_initAGC(&tvInstance->tuner, muxId);
}

/*--------------------------------------------------------------------------*/

unsigned
TVTUNER_getWorkspaceSize(TV_INSTANCE_T *tvInstance)
{
    return TUNER_getWorkspaceSize(tvInstance->tunerUse);
}

/*--------------------------------------------------------------------------*/

unsigned
TVTUNER_getNumConfigs(TV_INSTANCE_T *tvInstance)
{
    return TUNER_getNumConfigs(tvInstance->tunerUse);
}

/*--------------------------------------------------------------------------*/

unsigned
TVTUNER_getSCPId(TV_INSTANCE_T *tvInstance)
{
    return TUNER_getSCPId(tvInstance->tunerUse);
}

/*--------------------------------------------------------------------------*/

unsigned
TVTUNER_getUCCId(TV_INSTANCE_T *tvInstance)
{
    return TUNER_getUCCId(tvInstance->tunerUse);
}

/*--------------------------------------------------------------------------*/

TDEV_USE_T *
TVTUNER_getDevice(TV_INSTANCE_T *tvInstance)
{
    return TUNER_getDevice(tvInstance->tunerUse);
}
/*--------------------------------------------------------------------------*/
int
TVTUNER_readRFPower(TV_INSTANCE_T *tvInstance)
{
    return TUNER_readRFPower(&tvInstance->tuner);
}

/*--------------------------------------------------------------------------*/

int
TVTUNER_pollAGC(TV_INSTANCE_T *tvInstance)
{
    return TUNER_pollAGC(&tvInstance->tuner);
}

/*--------------------------------------------------------------------------*/
void
TVTUNER_startAGC(TV_INSTANCE_T *tvInstance)
{
    TUNER_startAGC(&tvInstance->tuner);
}

/*--------------------------------------------------------------------------*/

void
TVTUNER_stopAGC(TV_INSTANCE_T *tvInstance)
{
    TUNER_stopAGC(&tvInstance->tuner);
}

/*--------------------------------------------------------------------------*/

int
TVTUNER_shutdown(TV_INSTANCE_T *tvInstance)
{
    return TUNER_shutdown(&tvInstance->tuner);
}

/*--------------------------------------------------------------------------*/

bool
TVTUNER_powerUp(TV_INSTANCE_T *tvInstance, unsigned muxId)
{
    return TUNER_powerUp(&tvInstance->tuner, muxId);
}

/*--------------------------------------------------------------------------*/

bool
TVTUNER_powerDown(TV_INSTANCE_T *tvInstance, unsigned muxId)
{
    return TUNER_powerDown(&tvInstance->tuner, muxId);
}

/*--------------------------------------------------------------------------*/

bool
TVTUNER_powerSave(TV_INSTANCE_T *tvInstance, TDEV_RF_PWRSAV_T pwrSaveLevel,
                  unsigned muxId)
{
    return TUNER_powerSave(&tvInstance->tuner, pwrSaveLevel, muxId);
}

/*--------------------------------------------------------------------------*/

bool
TVTUNER_setBandWide(TV_INSTANCE_T *tvInstance, unsigned config)
{
    return TUNER_setBandWide(&tvInstance->tuner, config);
}

/*--------------------------------------------------------------------------*/

bool
TVTUNER_setBandNarrow(TV_INSTANCE_T *tvInstance, unsigned config)
{
    return TUNER_setBandNarrow(&tvInstance->tuner, config);
}

/*--------------------------------------------------------------------------*/
bool
TVTUNER_setAGCRapid(TV_INSTANCE_T *tvInstance, unsigned config)
{
    return TUNER_setAGCRapid(&tvInstance->tuner, config);
}

/*--------------------------------------------------------------------------*/

bool
TVTUNER_setAGCNormal(TV_INSTANCE_T *tvInstance, unsigned config)
{
    return TUNER_setAGCNormal(&tvInstance->tuner, config);
}

/*--------------------------------------------------------------------------*/

bool
TVTUNER_setIFAGCTimeConstant(TV_INSTANCE_T *tvInstance, int tc)
{
    return TUNER_setIFAGCTimeConstant(&tvInstance->tuner, tc);
}

/*--------------------------------------------------------------------------*/
static void
_TVTUNER_updateIFGainRegister(TV_INSTANCE_T *tvInstance)
{
    int gain;

    gain = tvInstance->tuner.agcData.IFgainValue;
    if (gain > TDEV_MAX_IF_GAIN)
        gain = TDEV_MAX_IF_GAIN;
    else if (gain < 0)
        gain = 0;
    TVREG_coreWrite(tvInstance->coreInstance, TV_REG_ACTIVE_TUNER_GAIN, gain);

}
/*--------------------------------------------------------------------------*/

bool
TVTUNER_setAGC(TV_INSTANCE_T *tvInstance, int inputSignalLevel, unsigned muxId,
               TUNER_AGC_CACLULATOR_T *agcCalculator, void *agcCalcContext)
{
    if (TUNER_setAGC(&tvInstance->tuner, inputSignalLevel, muxId, agcCalculator,
                     agcCalcContext))
    {
        _TVTUNER_updateIFGainRegister(tvInstance);
        return true;
    }
    else
        return false;
}
/*--------------------------------------------------------------------------*/

bool
TVTUNER_setAGCImmediate(TV_INSTANCE_T *tvInstance, int inputSignalLevel,
                        unsigned muxId, TUNER_AGC_CACLULATOR_T *agcCalculator,
                        void *agcCalcContext)
{
    if (TUNER_setAGCImmediate(&tvInstance->tuner, inputSignalLevel, muxId,
                              agcCalculator, agcCalcContext))
    {
        _TVTUNER_updateIFGainRegister(tvInstance);
        return true;
    }
    else
        return false;
}

/*--------------------------------------------------------------------------*/

bool
TVTUNER_updateActiveConfig(TV_INSTANCE_T *tvInstance, unsigned config,
                           TUNER_ACTIVE_CONFIG_T *newConfig)
{
    return TUNER_updateActiveConfig(&tvInstance->tuner, config, newConfig);
}

/*--------------------------------------------------------------------------*/
TUNER_ACTIVE_CONFIG_T *
TVTUNER_getActiveConfig(TV_INSTANCE_T *tvInstance, unsigned config)
{
    return TUNER_getActiveConfig(&tvInstance->tuner, config);
}

/*--------------------------------------------------------------------------*/
SCP_T *
TVTUNER_getSCP(TV_INSTANCE_T *tvInstance)
{
    return TUNER_getSCP(&tvInstance->tuner);
}

/*--------------------------------------------------------------------------*/

