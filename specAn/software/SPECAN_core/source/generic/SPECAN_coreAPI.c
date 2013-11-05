/*!****************************************************************************
 @File          SPECAN_coreAPI.c

 @Title         Virtual-register based API for the Spectrum Analyser core

 @Date          29 Nov 2012

 @Copyright     Copyright (C) Imagination Technologies Limited 2012

 @Description   Register initialisation functions etc.

 ******************************************************************************/
#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include "SPECAN_private.h"

bool _SPECAN_initRegisterAPI(UFW_COREINSTANCE_T *coreInstance, void *parameter)
{
    (void)parameter;
    unsigned regID, regVal;

    /* Common TV register API initialisation */
    TV_initRegisterAPI(coreInstance);

    /* These ID registers are the only ones within the common register set that we have
    to initialise separately */
    uint32_t val = ((UCC_STANDARD_ATV+1) << 24) | (VER_MAJ1 << 16) | (VER_MAJ2 << 8) | VER_MAJ3;
    TVREG_initValue(coreInstance, TV_REG_DEMOD_ID, TV_REG_NULL_ID, val);
    TVREG_initValue(coreInstance, TV_REG_BUILD_ID, TV_REG_NULL_ID, VER_BUILD);

	/* Initialise our own registers */
	for (regID = SA_SCAN_RANGE; regID < SPECAN_NUM_REGS; regID++)
	{
		if (regID == SA_MEASUREMENT_CONTROL)
			regVal = 0x200;
		else
			regVal = 0;
		TVREG_initValue(coreInstance, regID, TV_REG_NULL_ID, regVal);
	}

	return true;
}
