/* Keep these first ... */
#ifdef METAG
#define METAG_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include "plt_soc_setup.h"

void PLT_setupSOC(PLT_INFO_T *info)
{
    info->socName = "Example";
    
    /* Do SOC setup here */
}
