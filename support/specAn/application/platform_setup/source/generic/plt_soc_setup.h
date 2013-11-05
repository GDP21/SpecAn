#ifndef PLT_SOC_SETUP_H
#define PLT_SOC_SETUP_H

#include "plt_setup.h"

void PLT_setupAFE(unsigned ADCclkFreq, bool bypassPll);
void PLT_setupSOC(PLT_INFO_T *info);
void PLT_setupMeOS(PLT_INFO_T *info);

#endif
