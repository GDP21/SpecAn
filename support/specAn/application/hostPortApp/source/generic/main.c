/*!****************************************************************************
 @File          main.c

 @Title         Host Port Application main() function for DVB-S

 @Date          08 Feb 2012

 @Copyright     Copyright (C) Imagination Technologies Limited

 ******************************************************************************/

#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include "SPECAN_core.h"
#include "hostmsgproc.h"
#include "plt_setup.h"



/*-----------------------------------------------------------------------------*/

/*
 * This implements a host port application for the Spectrum Analyser
 *
 */
int main()
{
    TV_ACTIVATION_PARAMETER_T tList;
#ifdef INCLUDE_EMU_SETUP
	PLT_EMU_CONFIG_T EMUconfig;
#endif


    PLT_setup();
	PLT_INFO_T *pltInfo = PLT_query();

	tList.tunerUseCount = 1;
    tList.tunerUseList = pltInfo->tuner;

    IMGTV_MessageApp(SPECANdescriptor, 1, &tList);

    return 0;
}



