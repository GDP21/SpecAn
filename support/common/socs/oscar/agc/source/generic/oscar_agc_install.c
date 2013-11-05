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
#include "oscar_agc_install.h"
#include "oscar_agc_main.h"
#include "PHY_tuner.h"
#include "tuner_support.h"
#include <assert.h>

/* Registered tuner */
TUNER_CONTROL_T *oscarAgcTuner = NULL;

/* Tuner functions */
static TUNER_ROUTINE_T tunerInitialiseFunc;
static TUNER_ROUTINE_T tunerConfigureFunc;
static TUNER_ROUTINE_T tunerInitAGCFunc;
static TUNER_ROUTINE_T tunerPowerUpFunc;
static TUNER_ROUTINE_T tunerPowerDownFunc;
static TUNER_ROUTINE_T tunerPowerSaveFunc;
static TUNER_ROUTINE_T tunerSetAGCFunc;

/* Previous level translation function */
static TUNER_LEVEL_TRANS_ROUTINE_T prevLevelTransRoutine;

static PHY_TUNER_RETURN_T initialise(TUNER_COMPLETION_FUNC_T pfCompletionFunc)
{
    _OSCAR_AGC_initialise();

    return (*tunerInitialiseFunc.initialise)(pfCompletionFunc);
}

static PHY_TUNER_RETURN_T configure(PHY_TUNER_STANDARD_T standard,
								 long bandwidthHz,
                                 TUNER_COMPLETION_FUNC_T pfCompletionFunc)
{
    _OSCAR_AGC_config(standard,bandwidthHz);

    return (*tunerConfigureFunc.configure)(standard, bandwidthHz, pfCompletionFunc);
}

static PHY_TUNER_RETURN_T initAGC(unsigned long muxID,
    TUNER_COMPLETION_FUNC_T pfCompletionFunc)
{
    _OSCAR_AGC_initAGC(muxID);

    return (*tunerInitAGCFunc.initAGC)(muxID, pfCompletionFunc);
}

static PHY_TUNER_RETURN_T powerUp(unsigned long muxID,
    TUNER_COMPLETION_FUNC_T pfCompletionFunc)
{
    _OSCAR_AGC_powerUp(muxID);

    return (*tunerPowerUpFunc.powerUp)(muxID, pfCompletionFunc);
}

static PHY_TUNER_RETURN_T powerDown(unsigned long muxID,
    TUNER_COMPLETION_FUNC_T pfCompletionFunc)
{
    _OSCAR_AGC_powerDown(muxID);

    return (*tunerPowerDownFunc.powerDown)(muxID, pfCompletionFunc);
}

static PHY_TUNER_RETURN_T powerSave(PHY_RF_PWRSAV_T pwsav, unsigned long muxID,
    TUNER_COMPLETION_FUNC_T pfCompletionFunc)
{
    _OSCAR_AGC_powerSave(pwsav, muxID);

    return (*tunerPowerSaveFunc.powerSave)(pwsav, muxID, pfCompletionFunc);
}

static PHY_TUNER_RETURN_T setAGC(TUNER_AGCISR_HELPER_T *pAgcIsrHelper,
    TUNER_COMPLETION_FUNC_T pfCompletionFunc)
{
    _OSCAR_AGC_setAGC(pAgcIsrHelper);

    return (*tunerSetAGCFunc.setAGC)(pAgcIsrHelper, pfCompletionFunc);
}

void OSCAR_AGC_install(TUNER_CONTROL_T *tuner)
{
	TUNER_ROUTINE_T routine;

	assert(oscarAgcTuner == NULL);

	oscarAgcTuner = tuner;

	routine.initialise = initialise;
	tunerInitialiseFunc = TUNER_installFilter(tuner, TUNER_INITIALISE, routine);

	routine.configure = configure;
	tunerConfigureFunc = TUNER_installFilter(tuner, TUNER_CONFIGURE, routine);

	routine.initAGC = initAGC;
	tunerInitAGCFunc = TUNER_installFilter(tuner, TUNER_INIT_AGC, routine);

	routine.powerUp = powerUp;
	tunerPowerUpFunc = TUNER_installFilter(tuner, TUNER_POWER_UP, routine);

	routine.powerDown = powerDown;
	tunerPowerDownFunc = TUNER_installFilter(tuner, TUNER_POWER_DOWN, routine);

	routine.powerSave = powerSave;
	tunerPowerSaveFunc = TUNER_installFilter(tuner, TUNER_POWER_SAVE, routine);

	routine.setAGC = setAGC;
	tunerSetAGCFunc = TUNER_installFilter(tuner, TUNER_SET_AGC, routine);

	prevLevelTransRoutine = TUNER_installLevelTrans(_OSCAR_AGC_levelTrans);
}

void OSCAR_AGC_remove(void)
{
	assert(oscarAgcTuner != NULL);

	TUNER_removeFilter(oscarAgcTuner, TUNER_INITIALISE, tunerInitialiseFunc);
	TUNER_removeFilter(oscarAgcTuner, TUNER_CONFIGURE, tunerConfigureFunc);
	TUNER_removeFilter(oscarAgcTuner, TUNER_INIT_AGC, tunerInitAGCFunc);
	TUNER_removeFilter(oscarAgcTuner, TUNER_POWER_UP, tunerPowerUpFunc);
	TUNER_removeFilter(oscarAgcTuner, TUNER_POWER_DOWN, tunerPowerDownFunc);
	TUNER_removeFilter(oscarAgcTuner, TUNER_POWER_SAVE, tunerPowerSaveFunc);
	TUNER_removeFilter(oscarAgcTuner, TUNER_SET_AGC, tunerSetAGCFunc);

	TUNER_removeLevelTrans(prevLevelTransRoutine);

	oscarAgcTuner = NULL;
}
