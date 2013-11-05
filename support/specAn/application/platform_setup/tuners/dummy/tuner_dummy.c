/* Keep these first ... */
#define METAG_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include "plt_setup.h"
#include "plt_scp_setup.h"
#include "SLTtunerDummy.h"


/* The exported tuner use cases */

TDEV_USE_T SLT_dummyTDEVuse = {
		SPECAN_SCP_RATE_MAX, /* Single SCP parameter set config. */
        &SLTtunerDummy, /* tuner driver/config */
        pltScpConfig,  /* associated SCP parameter set */
        0, /* tuner context workspace size */
        1, /* UCC Id */
        1, /* SCP Id} */
        NULL /* No tuner specific config extension */ };


TUNER_USE_T SLT_dummyTunerUse =
		{
                &SLT_dummyTDEVuse,
                1,
                false,
        };


void PLT_setupTuner(PLT_INFO_T *info)
{
	info->tuner = &SLT_dummyTunerUse;
	info->tunerName = "Dummy";
	/* We're a dummy tuner */
	info->rf = false;
}


unsigned PLT_getGriddedTunerFreq(unsigned freq)
{
	return freq;
}