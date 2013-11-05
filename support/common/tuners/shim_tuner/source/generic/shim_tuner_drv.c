/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuners/shim_tuner/source/generic/shim_tuner_drv.c,v $
**
** TITLE:       Shim tuner driver
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Implementation of shim tuner driver
**
** NOTICE:		Copyright (C) 2008, Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
**				This shim tuner driver is intended to be used as an interface between tuner API's
**              allowing legacy systems to interface to later versions.
**              (The tuner API software integration guide should be consulted for more information
**				on the requirements and capabilities of the tuner API.)
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

#include "PHY_tuner.h"
#include "shim_tuner_drv.h"

#define PHY_TUNER_RF_POWER_TEST_VALUE (0x00007E57) /* Looks like 0x0000TEST (if you squint) */
#define SHIM_DRV_STACK_ZIZE (1536) /* 6kbyte stack */

/*
** Local static vars
*/
static TDEV_CONFIG_T *extTunerConfig;
static TDEV_T tdevShim;
static TDEV_AGCISR_HELPER_T extAgcIsrHelper;
static int shimTunerDrv_stack[SHIM_DRV_STACK_ZIZE]; /* stack */

/*
** Function prototypes
*/
static PHY_TUNER_RETURN_T shimTuner_initialise(TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T shimTuner_configure(PHY_TUNER_STANDARD_T standard, long bandwidthHz, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T shimTuner_tune(long frequencyHz, TUNER_COMPLETION_FUNC_T);
static long               shimTuner_readRFPower(TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T shimTuner_powerUp(unsigned long, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T shimTuner_powerDown(unsigned long, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T shimTuner_powerSave(PHY_RF_PWRSAV_T, unsigned long, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T shimTuner_setAGC(TUNER_AGCISR_HELPER_T *, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T shimTuner_initAGC(unsigned long, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T shimTuner_shutdown(TUNER_COMPLETION_FUNC_T);

TUNER_CONTROL_T shimTuner = {
    PHY_TUNER_VERSION_I32,  	/* Version number - check that tuner and API are built with the same header */
    1,              			/* The IF interface is complex baseband   */
    0,              			/* The final IF frequency of the tuner (Hz) */
    1,              			/* Set true if the IF spectrum is inverted */
    166000,         			/* The step size of the tuner PLL (Hz)      */
    166000,         			/* The update margin of the tuner PLL (Hz) */
    0,              			/* Settling time in uS from power down to power up */
    0,              			/* Settling time in uS from power save level1 to power up */
    0,              			/* Settling time in uS from power save level2 to power up */
    0,              			/* Settling time in uS from tune to stable output  */
    shimTuner_initialise,	    /* Initialise the tuner */
    shimTuner_configure,		/* Configure the tuner to the broadcast standard being demodulated and to set the RF bandwidth */
    shimTuner_tune,			    /* Request that the RF is tuned to the given frequency */
    shimTuner_readRFPower,   	/* Request the signal power at input RF (RSSI) */
    shimTuner_powerUp,		    /* Configure the tuner to its full "power on" state */
    shimTuner_powerDown,		/* Configure the tuner to its lowest power state */
    shimTuner_powerSave,		/* Configure the tuner to the given power state */
    NULL,						/* setIFAGCTimeConstant is reserved for future use and should be set to NULL */
    shimTuner_setAGC,		    /* Set IF gain for the AGC - Also can be used to modify other control loops */
    shimTuner_initAGC,		    /* Initialise the AGC */
    shimTuner_shutdown,		    /* Shut down the tuner */
    NULL,						/* No standard specific info */
    NULL						/* No tuner specific info */
};


void shimTuner_completionFunction(TDEV_T *tuner, TDEV_RETURN_T status, void *parameter)
{
	TUNER_COMPLETION_FUNC_T compFunc = parameter;
	PHY_TUNER_RETURN_T retval = (status == TDEV_SUCCESS) ? PHY_TUNER_SUCCESS : PHY_TUNER_FAILURE;

	(void) tuner;

	compFunc(retval);

	return;
}

static PHY_TUNER_RETURN_T shimTuner_initialise(TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	TDEV_RETURN_T retVal;

	/*
	** Perform any one off, at start-up initialisation
	**
	** Function Prototype:
	**          xxx_Initialise(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
	*/
    retVal =(extTunerConfig->init)(&tdevShim, (TDEV_COMPLETION_FUNC_T)shimTuner_completionFunction, pCompletionFunc);

    return (retVal == TDEV_SUCCESS) ? PHY_TUNER_SUCCESS : PHY_TUNER_FAILURE;
}

static PHY_TUNER_RETURN_T shimTuner_configure(PHY_TUNER_STANDARD_T standard, long bandwidthHz, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	TDEV_RETURN_T retVal;
	UCC_STANDARD_T extStandard;
	/*
	** standard and bandwidthHz can be used to modify the RF configuration.
	** E.g. any filtering in the RF can be configured based upon bandwidthHz.
	**
	** Function Prototype:
	**           xxx_Configure(TDEV_T *pTuner, UCC_STANDARD_T standard, unsigned bandwidthHz, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
	*/

    /* Convert between structures PHY_TUNER_STANDARD_T and UCC_STANDARD_T */
	switch (standard) /* PHY_TUNER_STANDARD_T; */
	{
        case PHY_TUNER_NOT_SIGNALLED:
            extStandard = UCC_STANDARD_NOT_SIGNALLED;
        break;
        case PHY_TUNER_DVBT:
            extStandard = UCC_STANDARD_DVBT;
        break;
        case PHY_TUNER_DVBH:
            extStandard = UCC_STANDARD_DVBH;
        break;
        case PHY_TUNER_ISDBT_1SEG:
            extStandard = UCC_STANDARD_ISDBT_1SEG;
        break;
        case PHY_TUNER_ISDBT_3SEG:
            extStandard = UCC_STANDARD_ISDBT_3SEG;
        break;
        case PHY_TUNER_ISDBT_13SEG:
            extStandard = UCC_STANDARD_ISDBT_13SEG;
        break;
        default:
            return PHY_TUNER_FAILURE; /* exit: standrad not supported  */
            break;
	}

    retVal =(extTunerConfig->configure)(&tdevShim, extStandard, (unsigned)bandwidthHz ,(TDEV_COMPLETION_FUNC_T)shimTuner_completionFunction, pCompletionFunc);

    return (retVal == TDEV_SUCCESS) ? PHY_TUNER_SUCCESS : PHY_TUNER_FAILURE;
}

static PHY_TUNER_RETURN_T shimTuner_tune(long frequencyHz, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	TDEV_RETURN_T retVal;
	/*
	** Set up the RF to down convert an RF signal from a frequency of frequencyHz
	**
	** Function Prototype:
	**          xxx_Tune(TDEV_T *pTuner, unsigned freq, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
	*/
    retVal =(extTunerConfig->tune)(&tdevShim, (unsigned)frequencyHz, (TDEV_COMPLETION_FUNC_T)shimTuner_completionFunction, pCompletionFunc);

    return (retVal == TDEV_SUCCESS) ? PHY_TUNER_SUCCESS : PHY_TUNER_FAILURE;
}

static long shimTuner_readRFPower(TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	/* Return an indication of the incoming RF signal power */
    pCompletionFunc(PHY_TUNER_SUCCESS);

    return PHY_TUNER_RF_POWER_TEST_VALUE;
}

static PHY_TUNER_RETURN_T shimTuner_powerUp(unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	TDEV_RETURN_T retVal;
	/*
	** Put the RF into a fully on state
	**
	** Function Prototype:
	**          xxx_powerUp(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
	*/
    retVal =(extTunerConfig->powerUp)(&tdevShim, (unsigned)muxID, (TDEV_COMPLETION_FUNC_T)shimTuner_completionFunction, pCompletionFunc);

    return (retVal == TDEV_SUCCESS) ? PHY_TUNER_SUCCESS : PHY_TUNER_FAILURE;
}

static PHY_TUNER_RETURN_T shimTuner_powerDown(unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	TDEV_RETURN_T retVal;
	/*
	** Put the RF into a fully off state
	**
	** Function Prototype:
	**          xxx_powerDown(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
	*/
    retVal =(extTunerConfig->powerDown)(&tdevShim, (unsigned)muxID, (TDEV_COMPLETION_FUNC_T)shimTuner_completionFunction, pCompletionFunc);

    return (retVal == TDEV_SUCCESS) ? PHY_TUNER_SUCCESS : PHY_TUNER_FAILURE;
}

static PHY_TUNER_RETURN_T shimTuner_powerSave(PHY_RF_PWRSAV_T powerSaveMode, unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	TDEV_RETURN_T retVal;
	/*
	** Put the RF into a power saving state corresponding to powerSaveMode
	**
	** Function Prototype:
	**           xxx_powerSave(TDEV_T *pTuner, TDEV_RF_PWRSAV_T powerSaveState, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
	*/
    retVal =(extTunerConfig->powerSave)(&tdevShim,(TDEV_RF_PWRSAV_T)powerSaveMode, (unsigned)muxID, (TDEV_COMPLETION_FUNC_T)shimTuner_completionFunction, pCompletionFunc);

    return (retVal == TDEV_SUCCESS) ? PHY_TUNER_SUCCESS : PHY_TUNER_FAILURE;
}

static PHY_TUNER_RETURN_T shimTuner_setAGC(TUNER_AGCISR_HELPER_T *pAgcIsrHelper, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	TDEV_RETURN_T retVal;

	/*
	** Set AGC gain and possibly update SCP operation from pAgcIsrHelper
	**
	** Function Prototype:
	**          xxx_SetAGC(TDEV_T *pTuner, TDEV_AGCISR_HELPER_T *pControl, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
	*/

	/* Convert structures TDEV_AGCISR_HELPER_T < TUNER_AGCISR_HELPER_T */
	extAgcIsrHelper.AGCcount1I       = (unsigned)pAgcIsrHelper->AGCcount1I;
	extAgcIsrHelper.AGCcount1Q       = (unsigned)pAgcIsrHelper->AGCcount1Q;
	extAgcIsrHelper.AGCcount2I       = (unsigned)pAgcIsrHelper->AGCcount2I;
	extAgcIsrHelper.AGCcount2Q       = (unsigned)pAgcIsrHelper->AGCcount2Q;
	extAgcIsrHelper.inputSignalLevel = (int)0; /* WARNING: Not currently used */
	extAgcIsrHelper.DCoffsetI        = (int)pAgcIsrHelper->DCoffsetI;
	extAgcIsrHelper.DCoffsetQ        = (int)pAgcIsrHelper->DCoffsetQ;
	extAgcIsrHelper.IQphaseError     = (int)pAgcIsrHelper->IQphaseError;
	extAgcIsrHelper.DLOvalue         = (int)pAgcIsrHelper->DCOvalue;
	extAgcIsrHelper.AGCMode		     = (TDEV_AGC_MODE_T)pAgcIsrHelper-> AGCMode; /* WARNING: Rely on enum definitions being equal */
	extAgcIsrHelper.AGCupdatePeriod  = (unsigned)pAgcIsrHelper->AGCupdatePeriod;
	extAgcIsrHelper.sampleRate       = (unsigned)pAgcIsrHelper->sampleRate;
	extAgcIsrHelper.IFgainValue      = (int)pAgcIsrHelper->IFgainValue;
    extAgcIsrHelper.muxID            = (unsigned)pAgcIsrHelper->muxID;
	extAgcIsrHelper.pSCPcontrol      = (TDEV_SCP_CONTROL_T *)pAgcIsrHelper->pSCPcontrol; /* WARNING: Rely on structure definitions being equal */

    retVal =(extTunerConfig->setAGC)(&tdevShim, &extAgcIsrHelper, (TDEV_COMPLETION_FUNC_T)shimTuner_completionFunction, pCompletionFunc);

    return (retVal == TDEV_SUCCESS) ? PHY_TUNER_SUCCESS : PHY_TUNER_FAILURE;
}

static PHY_TUNER_RETURN_T shimTuner_initAGC(unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	TDEV_RETURN_T retVal;
	/*
	** re-initialise AGC operation
	**
	** Function Prototype:
	**          xxx_initAGC(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
	*/
    retVal =(extTunerConfig->initAGC)(&tdevShim, (unsigned)muxID, (TDEV_COMPLETION_FUNC_T)shimTuner_completionFunction, pCompletionFunc);

    return (retVal == TDEV_SUCCESS) ? PHY_TUNER_SUCCESS : PHY_TUNER_FAILURE;
}

static PHY_TUNER_RETURN_T shimTuner_shutdown(TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	TDEV_RETURN_T retVal;
	/*
	** Shutdown the driver. Undo anything done in the initialise function.
	**
	** Function Prototype:
	**          xxx__Shutdown(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
	*/
    retVal =(extTunerConfig->shutdown)(&tdevShim, (TDEV_COMPLETION_FUNC_T)shimTuner_completionFunction, pCompletionFunc);

    return (retVal == TDEV_SUCCESS) ? PHY_TUNER_SUCCESS : PHY_TUNER_FAILURE;
}

/*
** FUNCTION:    SHIM_tunerDrv_configure()
**
** DESCRIPTION: Take external tuner driver and convert
**              to common API.
**
*/
void shim_tunerDrv_configure(TDEV_CONFIG_T *tdev_driver, void *extension)
{
    /* Take a local copy of external tuner */
    extTunerConfig = tdev_driver;

    /*
    ** Copy 'static' data from external (TDEV_CONFIG_T) tuner
    ** to local (TUNER_CONTROL_T) shim version.
    ** The function calls will be dealt with above.
    */
    /* Keep legacy version number */
    /* shimTuner->versionNumber         = extTunerConfig->versionNumber;*/
    shimTuner.complexIF                 = extTunerConfig->complexIF;
    shimTuner.spectrumInverted          = extTunerConfig->spectrumInverted;
    shimTuner.frequencyIF               = extTunerConfig->frequencyIF;
    shimTuner.PLLStepSize               = extTunerConfig->PLLStepSize;
    shimTuner.PLLUpdateMargin           = extTunerConfig->PLLUpdateMargin;
    shimTuner.powerUpSettlingTimeuS     = extTunerConfig->powerUpSettlingTimeuS;
    shimTuner.powerSvLvl1SettlingTimeuS = extTunerConfig->powerSvLvl1SettlingTimeuS;
    shimTuner.powerSvLvl2SettlingTimeuS = extTunerConfig->powerSvLvl2SettlingTimeuS;
    shimTuner.tuneSettlingTimeuS        = extTunerConfig->tuneSettlingTimeuS;

    /* Set-up the tuner device (tdev) structure which is passed in local funcitons */
    tdevShim.numConfigs     = 1;
    tdevShim.scp            = UCC_getSCP(UCCP_getUCC(1), 1);
    tdevShim.tunerConfig    = tdev_driver;
    tdevShim.activeConfigs  = 0;
    tdevShim.tunerUseId     = 0;
    tdevShim.tunerConfigExtension =	 extension;
    tdevShim.workSpace = shimTunerDrv_stack; /* 6kb of space, local int array */
}


