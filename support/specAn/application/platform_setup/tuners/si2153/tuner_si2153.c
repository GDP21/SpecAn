/* Keep these first ... */
#define METAG_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include "plt_tuner_setup.h"
#include "plt_scp_setup.h"
#include "si2153_tuner.h"

#ifdef SATURN
#define SERIAL_PERIPHERAL_PORT_NUMBER   0   /* Use SCBM port 0 */
#endif //#ifdef SATURN

#ifdef SATURN2
#define SERIAL_PERIPHERAL_PORT_NUMBER   0   /* Use SCBM port 0 */
#endif //#ifdef SATURN

#define I2C_ADDRESS (0xc0>>1)


static SI2153_CONFIG_T SI2153Config=
{
    SERIAL_PERIPHERAL_PORT_NUMBER,
    I2C_ADDRESS
};


static TDEV_USE_T SI2153_TDevUse =
{
    SPECAN_SCP_RATE_MAX  ,              /* Number of SCP configs */
    &SI2153Tuner,                       /* Tuner driver/config */
    pltScpConfig,                      /* Associated SCP parameters */
    SI2153_TUNER_DRIVER_WORKSPACE_SIZE, /* Tuner context workspace size */
    1,                                  /* UCC Id */
    1,                                  /* SCP Id */
    &SI2153Config
};

static TUNER_USE_T SI2153_TunerUse =
{
    &SI2153_TDevUse,
    1,
    false
};


void PLT_setupTuner(PLT_INFO_T *info)
{
    info->tuner = &SI2153_TunerUse;
    info->tunerName = "SI2153";
}
