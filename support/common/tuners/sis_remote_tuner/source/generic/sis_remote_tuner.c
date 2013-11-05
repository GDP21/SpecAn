/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuners/sis_remote_tuner/source/generic/sis_remote_tuner.c,v $
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

#include "PHY_tuner.h"
#include "sis_remote_tuner.h"

#define PHY_TUNER_RF_POWER_TEST_VALUE (0x00007E57) /* Looks like 0x0000TEST (if you squint) */

/*
** Function prototypes
*/
static PHY_TUNER_RETURN_T sisRemoteTuner_initialise(TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T sisRemoteTuner_configure(PHY_TUNER_STANDARD_T standard, long bandwidthHz, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T sisRemoteTuner_tune(long frequencyHz, TUNER_COMPLETION_FUNC_T);
static long               sisRemoteTuner_readRFPower(TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T sisRemoteTuner_powerUp(unsigned long, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T sisRemoteTuner_powerDown(unsigned long, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T sisRemoteTuner_powerSave(PHY_RF_PWRSAV_T, unsigned long, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T sisRemoteTuner_setAGC(TUNER_AGCISR_HELPER_T *, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T sisRemoteTuner_initAGC(unsigned long, TUNER_COMPLETION_FUNC_T);
static PHY_TUNER_RETURN_T sisRemoteTuner_shutdown(TUNER_COMPLETION_FUNC_T);

/* 3.25MHz IF for 6MHz Bandwidth - NuTune FK1601 */
TUNER_CONTROL_T sisRemoteTunerNearZeroIF = {
    PHY_TUNER_VERSION_I32,      /* Version number - check that tuner and API are built with the same header */
    0,                          /* The IF interface is complex baseband   */
    3250000,                    /* The final IF frequency of the tuner (Hz) */
    1,                          /* Set true if the IF spectrum is inverted */
    166000,                     /* The step size of the tuner PLL (Hz)      */
    166000,                     /* The update margin of the tuner PLL (Hz) */
    0,                          /* Settling time in uS from power down to power up */
    0,                          /* Settling time in uS from power save level1 to power up */
    0,                          /* Settling time in uS from power save level2 to power up */
    0,                          /* Settling time in uS from tune to stable output  */
    sisRemoteTuner_initialise,  /* Initialise the tuner */
    sisRemoteTuner_configure,       /* Configure the tuner to the broadcast standard being demodulated and to set the RF bandwidth */
    sisRemoteTuner_tune,            /* Request that the RF is tuned to the given frequency */
    sisRemoteTuner_readRFPower, /* Request the signal power at input RF (RSSI) */
    sisRemoteTuner_powerUp,     /* Configure the tuner to its full "power on" state */
    sisRemoteTuner_powerDown,       /* Configure the tuner to its lowest power state */
    sisRemoteTuner_powerSave,       /* Configure the tuner to the given power state */
    NULL,                       /* setIFAGCTimeConstant is reserved for future use and should be set to NULL */
    sisRemoteTuner_setAGC,      /* Set IF gain for the AGC - Also can be used to modify other control loops */
    sisRemoteTuner_initAGC,     /* Initialise the AGC */
    sisRemoteTuner_shutdown,        /* Shut down the tuner */
    NULL,                       /* No standard specific info */
    NULL                        /* No tuner specific info */
};


/* 36MHz IF Tuner - DVB-C/T 8MHz bandwidth */
TUNER_CONTROL_T sisRemoteTuner = {
    PHY_TUNER_VERSION_I32,  	/* Version number - check that tuner and API are built with the same header */
    0,              			/* The IF interface is complex baseband   */
    36000000,          			/* The final IF frequency of the tuner (Hz) */
    1,              			/* Set true if the IF spectrum is inverted */
    166000,         			/* The step size of the tuner PLL (Hz)      */
    166000,         			/* The update margin of the tuner PLL (Hz) */
    0,              			/* Settling time in uS from power down to power up */
    0,              			/* Settling time in uS from power save level1 to power up */
    0,              			/* Settling time in uS from power save level2 to power up */
    0,              			/* Settling time in uS from tune to stable output  */
    sisRemoteTuner_initialise,	/* Initialise the tuner */
    sisRemoteTuner_configure,		/* Configure the tuner to the broadcast standard being demodulated and to set the RF bandwidth */
    sisRemoteTuner_tune,			/* Request that the RF is tuned to the given frequency */
    sisRemoteTuner_readRFPower,	/* Request the signal power at input RF (RSSI) */
    sisRemoteTuner_powerUp,		/* Configure the tuner to its full "power on" state */
    sisRemoteTuner_powerDown,		/* Configure the tuner to its lowest power state */
    sisRemoteTuner_powerSave,		/* Configure the tuner to the given power state */
    NULL,						/* setIFAGCTimeConstant is reserved for future use and should be set to NULL */
    sisRemoteTuner_setAGC,		/* Set IF gain for the AGC - Also can be used to modify other control loops */
    sisRemoteTuner_initAGC,		/* Initialise the AGC */
    sisRemoteTuner_shutdown,		/* Shut down the tuner */
    NULL,						/* No standard specific info */
    NULL						/* No tuner specific info */
};


static PHY_TUNER_RETURN_T sisRemoteTuner_initialise(TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	/* Perform any one off, at start-up initialisation */

    pCompletionFunc(PHY_TUNER_SUCCESS);
    return PHY_TUNER_SUCCESS;
}

static PHY_TUNER_RETURN_T sisRemoteTuner_configure(PHY_TUNER_STANDARD_T standard, long bandwidthHz, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
    (void)standard;         /* remove compiler warning about unused parameter */
    (void)bandwidthHz;      /* remove compiler warning about unused parameter */

	/* standard and bandwidthHz can be used to modify the RF configuration.
	** E.g. any filtering in the RF can be configured based upon bandwidthHz. */

    pCompletionFunc(PHY_TUNER_SUCCESS);
    return PHY_TUNER_SUCCESS;
}

static PHY_TUNER_RETURN_T sisRemoteTuner_tune(long frequencyHz, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
    (void)frequencyHz;      /* remove compiler warning about unused parameter */

	/* Set up the RF to down convert an RF signal from a frequency of frequencyHz */

    pCompletionFunc(PHY_TUNER_SUCCESS);
    return PHY_TUNER_SUCCESS;
}

static long sisRemoteTuner_readRFPower(TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	/* Return an indication of the incoming RF signal power */
    pCompletionFunc(PHY_TUNER_SUCCESS);
    return PHY_TUNER_RF_POWER_TEST_VALUE;
}

static PHY_TUNER_RETURN_T sisRemoteTuner_powerUp(unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
    (void)muxID;            /* remove compiler warning about unused parameter */

	/* Put the RF into a fully on state */

    pCompletionFunc(PHY_TUNER_SUCCESS);
    return PHY_TUNER_SUCCESS;
}

static PHY_TUNER_RETURN_T sisRemoteTuner_powerDown(unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
    (void)muxID;            /* remove compiler warning about unused parameter */

	/* Put the RF into a fully off state */

    pCompletionFunc(PHY_TUNER_SUCCESS);
    return PHY_TUNER_SUCCESS;
}

static PHY_TUNER_RETURN_T sisRemoteTuner_powerSave(PHY_RF_PWRSAV_T powerSaveMode, unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
    (void)muxID;            /* remove compiler warning about unused parameter */
    (void) powerSaveMode;   /* remove compiler warning about unused parameter */

	/* Put the RF into a power saving state corresponding to powerSaveMode */

    pCompletionFunc(PHY_TUNER_SUCCESS);
    return PHY_TUNER_SUCCESS;
}

static PHY_TUNER_RETURN_T sisRemoteTuner_setAGC(TUNER_AGCISR_HELPER_T *pAgcIsrHelper, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
    (void)pAgcIsrHelper;     /* remove compiler warning about unused parameter */

	/* Set AGC gain and possibly update SCP operation from pAgcIsrHelper */

	/* Send gain value out of SCP external gain 1 */
	Tuner_SetExtGain1((pAgcIsrHelper->IFgainValue)>>4); // 16 to 12 bit conversion


    pCompletionFunc(PHY_TUNER_SUCCESS);
    return PHY_TUNER_SUCCESS;
}

static PHY_TUNER_RETURN_T sisRemoteTuner_initAGC(unsigned long muxID, TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
    (void)muxID;            /* remove compiler warning about unused parameter */

	/* re-initialise AGC operation */

    pCompletionFunc(PHY_TUNER_SUCCESS);
    return PHY_TUNER_SUCCESS;
}

static PHY_TUNER_RETURN_T sisRemoteTuner_shutdown(TUNER_COMPLETION_FUNC_T pCompletionFunc)
{
	/* Shutdown the driver. Undo anything done in the initialise function. */

    pCompletionFunc(PHY_TUNER_SUCCESS);
    return PHY_TUNER_SUCCESS;
}
