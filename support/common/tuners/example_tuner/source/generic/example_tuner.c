/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuners/example_tuner/source/generic/example_tuner.c,v $
**
** TITLE:       Example tuner driver
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Implementation of example tuner driver
**
** NOTICE:		Copyright (C) Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
**				This example tuner driver is intended to be used as a template, a starting point for creating
**				tuner drivers. The tuner API software integration guide should be consulted for more information
**				on the requirements and capabilities of the tuner API.
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

#include "uccrt.h"
#include "example_tuner.h"

#define PHY_TUNER_RF_POWER_TEST_VALUE (0x00007E57) /* Looks like 0x0000TEST (if you squint) */

/*
** Function prototypes
*/
static TDEV_RETURN_T exampleTuner_initialise(TDEV_T *tunerInstance, TDEV_COMPLETION_FUNC_T, void* completionParameter);
static TDEV_RETURN_T exampleTuner_reset(TDEV_T *tunerInstance, TDEV_COMPLETION_FUNC_T, void* completionParameter);
static TDEV_RETURN_T exampleTuner_configure(TDEV_T *tunerInstance, UCC_STANDARD_T standard, unsigned bandwidthHz, TDEV_COMPLETION_FUNC_T, void* completionParameter);
static TDEV_RETURN_T exampleTuner_tune(TDEV_T *tunerInstance, unsigned frequencyHz, TDEV_COMPLETION_FUNC_T, void* completionParameter);
static TDEV_RETURN_T exampleTuner_readRFPower(TDEV_T *tunerInstance, TDEV_RSSI_COMPLETION_FUNC_T, void* completionParameter);
static TDEV_RETURN_T exampleTuner_powerUp(TDEV_T *tunerInstance, unsigned, TDEV_COMPLETION_FUNC_T, void* completionParameter);
static TDEV_RETURN_T exampleTuner_powerDown(TDEV_T *tunerInstance, unsigned, TDEV_COMPLETION_FUNC_T, void* completionParameter);
static TDEV_RETURN_T exampleTuner_powerSave(TDEV_T *tunerInstance, TDEV_RF_PWRSAV_T, unsigned, TDEV_COMPLETION_FUNC_T, void* completionParameter);
static TDEV_RETURN_T exampleTuner_setAGC(TDEV_T *tunerInstance, TDEV_AGCISR_HELPER_T *, TDEV_COMPLETION_FUNC_T, void* completionParameter);
static TDEV_RETURN_T exampleTuner_initAGC(TDEV_T *tunerInstance, unsigned, TDEV_COMPLETION_FUNC_T, void* completionParameter);
static TDEV_RETURN_T exampleTuner_shutdown(TDEV_T *tunerInstance, TDEV_COMPLETION_FUNC_T, void* completionParameter);


TDEV_CONFIG_T exampleTuner = {
    TDEV_VERSION_I32,  			/* Version number - check that tuner and API are built with the same header */
    1,              			/* The IF interface is complex baseband   */
    0,              			/* Set true if the IF spectrum is inverted */
    0,              			/* The final IF frequency of the tuner (Hz) */
    166000,         			/* The step size of the tuner PLL (Hz)      */
    166000,         			/* The update margin of the tuner PLL (Hz) */
    0,              			/* Settling time in uS from power down to power up */
    0,              			/* Settling time in uS from power save level1 to power up */
    0,              			/* Settling time in uS from power save level2 to power up */
    0,              			/* Settling time in uS from tune to stable output  */
    exampleTuner_initialise,	/* Initialise the tuner */
    exampleTuner_reset,			/* Reset the tuner */
    exampleTuner_configure,		/* Configure the tuner to the broadcast standard being demodulated and to set the RF bandwidth */
    exampleTuner_tune,			/* Request that the RF is tuned to the given frequency */
    exampleTuner_readRFPower,	/* Request the signal power at input RF (RSSI) */
    exampleTuner_powerUp,		/* Configure the tuner to its full "power on" state */
    exampleTuner_powerDown,		/* Configure the tuner to its lowest power state */
    exampleTuner_powerSave,		/* Configure the tuner to the given power state */
    NULL,						/* setIFAGCTimeConstant is reserved for future use and should be set to NULL */
    exampleTuner_setAGC,		/* Set IF gain for the AGC - Also can be used to modify other control loops */
    exampleTuner_initAGC,		/* Initialise the AGC */
    exampleTuner_shutdown		/* Shut down the tuner */
};


TDEV_CONFIG_T exampleTunerFM = {
    TDEV_VERSION_I32,  			/* Version number - check that tuner and API are built with the same header */
    1,              			/* The IF interface is complex baseband   */
    0,              			/* Set true if the IF spectrum is inverted */
    0,              			/* The final IF frequency of the tuner (Hz) */
    16000,         			    /* The step size of the tuner PLL (Hz)      */
    16000,         			     /* The update margin of the tuner PLL (Hz) */
    0,              			/* Settling time in uS from power down to power up */
    0,              			/* Settling time in uS from power save level1 to power up */
    0,              			/* Settling time in uS from power save level2 to power up */
    0,              			/* Settling time in uS from tune to stable output  */
    exampleTuner_initialise,	/* Initialise the tuner */
    exampleTuner_reset,			/* Reset the tuner */
    exampleTuner_configure,		/* Configure the tuner to the broadcast standard being demodulated and to set the RF bandwidth */
    exampleTuner_tune,			/* Request that the RF is tuned to the given frequency */
    exampleTuner_readRFPower,	/* Request the signal power at input RF (RSSI) */
    exampleTuner_powerUp,		/* Configure the tuner to its full "power on" state */
    exampleTuner_powerDown,		/* Configure the tuner to its lowest power state */
    exampleTuner_powerSave,		/* Configure the tuner to the given power state */
    NULL,						/* setIFAGCTimeConstant is reserved for future use and should be set to NULL */
    exampleTuner_setAGC,		/* Set IF gain for the AGC - Also can be used to modify other control loops */
    exampleTuner_initAGC,		/* Initialise the AGC */
    exampleTuner_shutdown		/* Shut down the tuner */
};

static TDEV_RETURN_T exampleTuner_initialise(TDEV_T *tunerInstance, TDEV_COMPLETION_FUNC_T pCompletionFunc, void* completionParameter)
{
	/* Perform any one off, at start-up initialisation */

    pCompletionFunc(tunerInstance, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}

static TDEV_RETURN_T exampleTuner_reset(TDEV_T *tunerInstance, TDEV_COMPLETION_FUNC_T pCompletionFunc, void* completionParameter)
{
	/* Perform any one off, at start-up initialisation */

    pCompletionFunc(tunerInstance, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}

static TDEV_RETURN_T exampleTuner_configure(TDEV_T *tunerInstance, UCC_STANDARD_T standard, unsigned bandwidthHz, TDEV_COMPLETION_FUNC_T pCompletionFunc, void* completionParameter)
{
    (void)standard;         /* remove compiler warning about unused parameter */
    (void)bandwidthHz;      /* remove compiler warning about unused parameter */

	/* standard and bandwidthHz can be used to modify the RF configuration.
	** E.g. any filtering in the RF can be configured based upon bandwidthHz. */

    pCompletionFunc(tunerInstance, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}

static TDEV_RETURN_T exampleTuner_tune(TDEV_T *tunerInstance, unsigned frequencyHz, TDEV_COMPLETION_FUNC_T pCompletionFunc, void* completionParameter)
{
    (void)frequencyHz;      /* remove compiler warning about unused parameter */

	/* Set up the RF to down convert an RF signal from a frequency of frequencyHz */

    pCompletionFunc(tunerInstance, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}

static TDEV_RETURN_T exampleTuner_readRFPower(TDEV_T *tunerInstance, TDEV_RSSI_COMPLETION_FUNC_T pCompletionFunc, void* completionParameter)
{
	/* Return an indication of the incoming RF signal power */
    pCompletionFunc(tunerInstance, PHY_TUNER_RF_POWER_TEST_VALUE, completionParameter);
    return PHY_TUNER_RF_POWER_TEST_VALUE;
}

static TDEV_RETURN_T exampleTuner_powerUp(TDEV_T *tunerInstance, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void* completionParameter)
{
    (void)muxID;            /* remove compiler warning about unused parameter */

	/* Put the RF into a fully on state */

    pCompletionFunc(tunerInstance, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}

static TDEV_RETURN_T exampleTuner_powerDown(TDEV_T *tunerInstance, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void* completionParameter)
{
    (void)muxID;            /* remove compiler warning about unused parameter */

	/* Put the RF into a fully off state */

    pCompletionFunc(tunerInstance, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}

static TDEV_RETURN_T exampleTuner_powerSave(TDEV_T *tunerInstance, TDEV_RF_PWRSAV_T powerSaveMode, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void* completionParameter)
{
    (void)muxID;            /* remove compiler warning about unused parameter */
    (void) powerSaveMode;   /* remove compiler warning about unused parameter */

	/* Put the RF into a power saving state corresponding to powerSaveMode */

    pCompletionFunc(tunerInstance, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}

static TDEV_RETURN_T exampleTuner_setAGC(TDEV_T *tunerInstance, TDEV_AGCISR_HELPER_T *pAgcIsrHelper, TDEV_COMPLETION_FUNC_T pCompletionFunc, void* completionParameter)
{
	SCP_T *scp = tunerInstance->scp;
    (void)pAgcIsrHelper;     /* remove compiler warning about unused parameter */

	/* Set AGC gain and possibly update SCP operation from pAgcIsrHelper */

	/* Send gain value out of SCP external gain 1 and offset 1 */
	SCP_setExtGainControl1(scp, (pAgcIsrHelper->IFgainValue)>>4); // 16 to 12 bit conversion
	SCP_setExtOffsetControl1(scp, (pAgcIsrHelper->IFgainValue)>>4); // 16 to 12 bit conversion


    pCompletionFunc(tunerInstance, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}

static TDEV_RETURN_T exampleTuner_initAGC(TDEV_T *tunerInstance, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void* completionParameter)
{
    (void)muxID;            /* remove compiler warning about unused parameter */

	/* re-initialise AGC operation */

    pCompletionFunc(tunerInstance, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}

static TDEV_RETURN_T exampleTuner_shutdown(TDEV_T *tunerInstance, TDEV_COMPLETION_FUNC_T pCompletionFunc, void* completionParameter)
{
	/* Shutdown the driver. Undo anything done in the initialise function. */

    pCompletionFunc(tunerInstance, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}
