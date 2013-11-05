/*!
*****************************************************************************
 
 @file   plt_setup.h
 @brief  Platform Setup

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

/* Keep these first ... */
#ifdef METAG
#define METAG_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <assert.h>

#include "plt_setup.h"
#include "plt_scp_setup.h"

#ifndef PLT_NO_SOC
#include "plt_soc_setup.h"
#endif

static int pltSetupDone = 0;
PLT_INFO_T pltInfo;

/* Sets up the platform */
void PLT_setup(void)
{
    assert(!pltSetupDone);
    
    /* This flag indicates if this is a real tuner or a dummy one. Assume
     * the former. Dummy tuners should set this flag to false in their setup
     * function.
     */
    pltInfo.rf = true;
    
    /* Setup tuner */
    PLT_setupTuner(&pltInfo);

    /* Setup SOC */
#ifndef PLT_NO_SOC
    PLT_setupSOC(&pltInfo);
#endif

    /* Platform setup completed now */
    pltSetupDone = 1;
}

/* Returns information about the current platform */
PLT_INFO_T *PLT_query(void)
{
    assert(pltSetupDone);

    return &pltInfo;
}
