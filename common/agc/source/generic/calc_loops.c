/*
** FILE NAME:   $RCSfile: calc_loops.c,v $
**
** TITLE:       Common AGC and related control loop handling
**
** PROJECT:		UCCP Common
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Top level function to implement all AGC and related
**				loops in the common AGC.
**
**				Copyright (C) Imagination Technologies Ltd.
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include "common_agc.h"

/* Run all the control loops */
void AGC_CalcLoops(AGC_CONTEXT_T *context, TDEV_AGCISR_HELPER_T *SCPInfo)
{
	/* Run each individual loop in turn. */
	AGC_CalcGainLoop(context, SCPInfo);
	AGC_CalcDCOffsetLoop(context, SCPInfo);
	AGC_CalcIQOffsetLoop(context, SCPInfo);

	return;
}
