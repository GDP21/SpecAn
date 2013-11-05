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

#include "tuner_support.h"

static TUNER_SCPCONTROL_T scpControl;
static TUNER_AGCISR_HELPER_T tunerAgcIsrHelper;

static float levelTrans(float rms)
{
    return rms - 20.0f;
}

int main(void)
{
	float rms;
	
	tunerAgcIsrHelper.pSCPcontrol = &scpControl;
	scpControl.AGCthresh1 = 1024;
	
	TUNER_installLevelTrans(levelTrans);
	
	rms = TUNER_getRmsLevel(&tunerAgcIsrHelper);
	
	return 0;
}
