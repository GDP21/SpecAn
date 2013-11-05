/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuner_support/source/generic/tuner_support.c,v $
**
** TITLE:       Tuner support functions
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Implementation of tuner support functions
**
** NOTICE:      Copyright (C) 2008-2009, Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
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
#include <stddef.h>
#include <math.h>
#include "PHY_tuner.h"
#include "tuner_support.h"

TUNER_LEVEL_TRANS_ROUTINE_T levelTransRoutine = NULL;

static TUNER_ROUTINE_T setFilter(TUNER_CONTROL_T *tuner,
    TUNER_FUNC_T func, TUNER_ROUTINE_T routine)
{
	TUNER_ROUTINE_T prev;

	switch (func)
	{
		case TUNER_INITIALISE:
			prev.initialise = tuner->initialise;
			tuner->initialise = routine.initialise;
			break;

		case TUNER_CONFIGURE:
			prev.configure = tuner->configure;
			tuner->configure = routine.configure;
			break;

		case TUNER_TUNE:
			prev.tune = tuner->tune;
			tuner->tune = routine.tune;
			break;

		case TUNER_READ_RF_POWER:
			prev.readRFPower = tuner->readRFPower;
			tuner->readRFPower = routine.readRFPower;
			break;

		case TUNER_POWER_UP:
			prev.powerUp = tuner->powerUp;
			tuner->powerUp = routine.powerUp;
			break;

		case TUNER_POWER_DOWN:
			prev.powerDown = tuner->powerDown;
			tuner->powerDown = routine.powerDown;
			break;

		case TUNER_POWER_SAVE:
			prev.powerSave = tuner->powerSave;
			tuner->powerSave = routine.powerSave;
			break;

		case TUNER_SET_IF_AGC_TIME_CONSTANT:
			prev.setIFAGCTimeConstant = tuner->setIFAGCTimeConstant;
			tuner->setIFAGCTimeConstant = routine.setIFAGCTimeConstant;
			break;

		case TUNER_SET_AGC:
			prev.setAGC = tuner->setAGC;
			tuner->setAGC = routine.setAGC;
			break;

		case TUNER_INIT_AGC:
			prev.initAGC = tuner->initAGC;
			tuner->initAGC = routine.initAGC;
			break;

		default:
		    prev.initialise = 0;
		    break;
	}

	return prev;
}

TUNER_ROUTINE_T TUNER_installFilter(TUNER_CONTROL_T *tuner, TUNER_FUNC_T func,
    TUNER_ROUTINE_T routine)
{
    return setFilter(tuner, func, routine);
}

void TUNER_removeFilter(TUNER_CONTROL_T *tuner, TUNER_FUNC_T func,
    TUNER_ROUTINE_T routine)
{
	setFilter(tuner, func, routine);
}


TUNER_LEVEL_TRANS_ROUTINE_T TUNER_installLevelTrans(
    TUNER_LEVEL_TRANS_ROUTINE_T routine)
{
    TUNER_LEVEL_TRANS_ROUTINE_T prev;

    prev = levelTransRoutine;
    levelTransRoutine = routine;

    return prev;
}

void TUNER_removeLevelTrans(TUNER_LEVEL_TRANS_ROUTINE_T routine)
{
    levelTransRoutine = routine;
}
