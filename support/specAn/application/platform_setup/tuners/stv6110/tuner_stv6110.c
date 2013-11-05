/* Keep these first ... */
#define METAG_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include "plt_setup.h"
#include "plt_scp_setup.h"
#include "stv6110_tuner.h"

#if (defined SATURN || defined SATURN2)
#define SERIAL_PERIPHERAL_PORT_NUMBER	0	/* Use SCBM port 0 */
#endif //#ifdef SATURN

#define I2C_ADDRESS		96

static const STV6110_CONFIG_T tunerConfig =
{
	SERIAL_PERIPHERAL_PORT_NUMBER,
	I2C_ADDRESS
};


/* Tuner use structure for STV6110 tuner */
#if 0
static const TDEV_USE_T STV6110TunerUse =
{
	SPECAN_SCP_RATE_MAX,					/* Number of SCP configuration parameter sets */
    &STV6110Tuner,      					/* Pointer to the tuner driver definition */
    pltScpConfig,       					/* Pointer to the array of default SCP configuration parameter sets */
	STV6110_TUNER_DRIVER_WORKSPACE_SIZE,	/* Workspace size associated with TDEV use case */
    1,                  					/* Id of the UCC associated with the tuner [1..n] */
    1,                  					/* Id of the SCP (on the specified UCC) associated with the tuner */
	(void *)&tunerConfig
};
#else
static const TDEV_USE_T STV6110TunerUse =
{
    1,										/* Number of SCP configuration parameter sets */
    &STV6110Tuner,      					/* Pointer to the tuner driver definition */
    pltScpConfig,       					/* Pointer to the array of default SCP configuration parameter sets */
	STV6110_TUNER_DRIVER_WORKSPACE_SIZE,	/* Workspace size associated with TDEV use case */
    1,                  					/* Id of the UCC associated with the tuner [1..n] */
    1,                  					/* Id of the SCP (on the specified UCC) associated with the tuner */
	(void *)&tunerConfig
};
#endif


static TUNER_USE_T STV6110TunerUseCase =
{
	&STV6110TunerUse,
	1,
	false
};


void PLT_setupTuner(PLT_INFO_T *info)
{
	info->tuner = &STV6110TunerUseCase;
	info->tunerName = "STV6110";
}


unsigned PLT_getGriddedTunerFreq(unsigned freq)
{
	unsigned griddedFreq;

	/* This code must mirror exactly what the driver does in its tune function, which in our case
	is to snap to a 1MHz grid */
	griddedFreq = ((freq + 500000) / 1000000) * 1000000;
	return griddedFreq;
}
