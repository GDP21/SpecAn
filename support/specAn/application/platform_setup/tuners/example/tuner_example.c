/* Keep these first ... */
#define METAG_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include "plt_setup.h"
#include "plt_scp_setup.h"
#include "example_tuner.h"

/* The exported tuner use cases */

static TDEV_USE_T example_TDevUse =
{
	SPECAN_SCP_RATE_MAX, /* Single SCP parameter set config. */
	&exampleTuner,  /* Tuner driver/config */
	pltScpConfig,  /* Associated SCP parameters */
	EXAMPLE_TUNER_WORKSPACE_SIZE,  /* Tuner context workspace size */
	1,              /* UCC Id */
	1,              /* SCP Id */
	NULL 			/* No tuner specific config extension */
};

static TUNER_USE_T example_TunerUse =
{
	&example_TDevUse,
	1,
	false
};

void PLT_setupTuner(PLT_INFO_T *info)
{
	info->tuner = &example_TunerUse;
	info->tunerName = "example";
}

unsigned PLT_getGriddedTunerFreq(unsigned freq)
{
	return freq;
}
