/* Keep these first ... */
#define METAG_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include "plt_setup.h"
#include "plt_scp_setup.h"
#include "tvtunerGenericUse.h"

/* The generic tuner driver includes an exported tuner use case, tvGenericTuner */

static TUNER_USE_T generic_TunerUse =
{
	&tvGenericTuner,
	1,
	false
};

void PLT_setupTuner(PLT_INFO_T *info)
{
	info->tuner = &generic_TunerUse;
	info->tunerName = "generic";
}

/* This is just a dummy function as this tuner should be treated as a host-controlled
tuner so this function should not be run */
unsigned PLT_getGriddedTunerFreq(unsigned freq)
{
	return freq;
}
